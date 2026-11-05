#include "visualizer.h"
#include "inspector_format.h"
#include <cstring>

// Inspector: spiega cosa c'e' sotto il mouse. Qui si decide QUALE pannello
// mostrare e ci sono i pezzi comuni; i pannelli stanno in:
//   visualizer_inspector_file.cpp     sezione, PUNTO, TRATTO (hanno un offset)
//   visualizer_inspector_value.cpp    VALORE (un byte, una coppia, una tripla)
//   visualizer_inspector_summary.cpp  riassunto, se il mouse non e' su nulla
//
// Cosa sia "un pixel" dipende dalla vista:
//   - PUNTO  (RAW, MetricMap)          → un offset del file
//   - TRATTO (DotPlot, Entropy)        → un range di offset
//   - VALORE (Digraph, Histogram, 3D, ...) → un byte, una coppia o una
//                                        tripla, con quante volte e dove compare

using namespace Inspect;

void Visualizer::drawInspector() {
    drawPanel(this, INSPECTOR_X, SIDE_Y, INSPECTOR_W, SIDE_H, "INSPECTOR");
    PanelWriter pw{this, INSPECTOR_X + 8, SIDE_Y + 20, INSPECTOR_W - 16};

    if (m_computing.load()) { pw.text("calcolo in corso...", COL_YELLOW()); return; }

    const InspectWindow w = inspectWindow();
    int cx = 0, cy = 0;
    bool over = mouseToContent(GetMouseX(), GetMouseY(), cx, cy);

    int hs = m_bars.hoveredSection();
    if (hs >= 0) { inspectSection(pw, hs); return; }

    size_t point = pointUnderMouse(over, cx, cy);
    if (point < m_reader.getSize()) { inspectPoint(pw, point); return; }

    if (over && w.len > 0 &&
        (m_mode == ViewMode::DOTPLOT || m_mode == ViewMode::ENTROPY)) {
        inspectRange(pw, cx, cy);
        return;
    }
    if (over && (m_mode == ViewMode::DIGRAPH || m_mode == ViewMode::HISTOGRAM ||
                 m_mode == ViewMode::BYTECLOUD || m_mode == ViewMode::ONETUPLE)) {
        inspectValue(pw, cx, cy);
        return;
    }
    uint32_t key;
    if (tripleUnderMouse(key)) { inspectTriple(pw, key); return; }
    if (m_pick.active()) { inspectPick(pw); return; }
    inspectSummary(pw);
}

// Offset del file sotto il mouse (barre, RAW, MetricMap); SIZE_MAX se nessuno
size_t Visualizer::pointUnderMouse(bool over, int cx, int cy) const {
    const InspectWindow w = inspectWindow();
    size_t barOff = 0;
    if (m_bars.hoverOffset(GetMouseX(), GetMouseY(), barOff))
        return barOff;     // mouse su una barra: il byte sotto il mouse
    if (over && m_mode == ViewMode::RAWPIXELS) {
        size_t rel = m_rawpixels.byteOffsetAt(cx, cy);
        if (rel != SIZE_MAX) return w.start + rel;
    } else if (over && m_mode == ViewMode::METRICMAP) {
        size_t rel = m_metricmap.fileIndexAt(cx, cy);
        if (rel != SIZE_MAX) return w.start + rel;
    }
    return SIZE_MAX;
}

Visualizer::InspectWindow Visualizer::inspectWindow() const {
    InspectWindow w;
    w.start = m_navigator.getStart();
    w.end   = std::min(m_navigator.getEnd(), m_reader.getSize());
    w.len   = w.end > w.start ? w.end - w.start : 0;
    return w;
}

// Coordinate (pixel del contenuto) sotto il mouse; false se fuori
bool Visualizer::mouseToContent(int mx, int my, int& cx, int& cy) const {
    if (m_mode == ViewMode::TRIGRAPH3D) return false;
    int ax, ay, aw, ah, cw, ch;
    viewArea(ax, ay, aw, ah);
    contentSize(cw, ch);
    if (mx < ax || mx >= ax + aw || my < ay || my >= ay + ah) return false;
    float drawX, drawY, scale;
    canvasTransform(drawX, drawY, scale);
    float fx = (mx - drawX) / scale, fy = (my - drawY) / scale;
    if (fx < 0 || fy < 0) return false;
    cx = (int)fx;  cy = (int)fy;
    return cx < cw && cy < ch;
}

int Visualizer::sectionAt(size_t off) const {
    // La sezione piu' piccola che contiene l'offset (le sezioni possono
    // annidarsi, es. segmenti vs sezioni)
    int best = -1;
    const auto& secs = m_parser.getSections();
    for (int i = 0; i < (int)secs.size(); i++)
        if (off >= secs[i].offset && off < secs[i].offset + secs[i].size &&
            (best < 0 || secs[i].size < secs[best].size))
            best = i;
    return best;
}

// Primo offset (assoluto) dove compare `pat` nella finestra corrente.
// Il risultato e' in cache: si ricerca solo quando cambia il valore.
size_t Visualizer::firstOccurrence(const uint8_t* pat, size_t n) {
    std::vector<uint8_t> key(pat, pat + n);
    size_t start = m_navigator.getStart(), end = m_navigator.getEnd();
    if (key == m_occKey && start == m_occStart && end == m_occEnd)
        return m_occResult;
    m_occKey = key;  m_occStart = start;  m_occEnd = end;
    m_occResult = SIZE_MAX;
    const auto& b = m_reader.getBytes();
    end = std::min(end, b.size());
    for (size_t i = start; i + n <= end; i++)
        if (std::memcmp(&b[i], pat, n) == 0) { m_occResult = i; break; }
    return m_occResult;
}

// ------------------- Pezzi comuni ai pannelli -------------------

void Visualizer::writeSectionField(PanelWriter& pw, size_t off) {
    if (m_parser.getFormat() == BinaryFormat::UNKNOWN) {
        pw.field("SEZIONE", "(formato non riconosciuto)", olc::Pixel(150, 150, 150));
        return;
    }
    int s = sectionAt(off);
    if (s < 0) { pw.field("SEZIONE", "(fuori dalle sezioni)", olc::Pixel(150, 150, 150)); return; }
    const auto& sec = m_parser.getSections()[s];
    pw.field("SEZIONE", sec.name + "  (" + sizeStr(sec.size) + ")",
             olc::Pixel(sec.r, sec.g, sec.b));
}

// Dump esadecimale: `rows` righe da 8 byte, con evidenziato `mark`
void Visualizer::writeHexDump(PanelWriter& pw, size_t center, size_t mark, int rows) {
    const auto& bytes = m_reader.getBytes();
    if (bytes.empty()) return;
    size_t first = (center & ~(size_t)7);
    first = first >= (size_t)(rows / 2) * 8 ? first - (rows / 2) * 8 : 0;
    for (int r = 0; r < rows; r++) {
        size_t rowOff = first + (size_t)r * 8;
        if (rowOff >= bytes.size()) break;
        int y = pw.y;
        DrawString(pw.x, y, hex(rowOff & 0xFFFFFF, 6), olc::Pixel(110, 110, 150));
        for (int c = 0; c < 8; c++) {
            size_t o = rowOff + c;
            if (o >= bytes.size()) break;
            uint8_t v = bytes[o];
            int hx = pw.x + (7 + c * 3) * 8;
            int ax = pw.x + (31 + c) * 8;
            bool sel = (o == mark);
            if (sel) { FillRect(hx - 1, y - 1, 18, 10, COL_YELLOW()); FillRect(ax, y - 1, 8, 10, COL_YELLOW()); }
            olc::Pixel col = sel ? olc::BLACK : classPixel(ByteInfo::classOf(v));
            DrawString(hx, y, hex(v, 2), col);
            char ch = (v >= 0x20 && v < 0x7F) ? (char)v : '.';
            DrawString(ax, y, std::string(1, ch), sel ? olc::BLACK : olc::Pixel(180, 180, 180));
        }
        pw.y += pw.lineH;
    }
}

// Statistiche su una zona, al massimo 1 MB (oltre si campiona l'inizio)
ByteInfo::Stats Visualizer::zoneStats(size_t a, size_t b) const {
    b = std::min(b, a + (size_t)(1 << 20));
    return ByteInfo::statsOf(m_reader.getBytes(), a, b);
}
