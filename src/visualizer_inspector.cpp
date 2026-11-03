#include "visualizer.h"
#include <sstream>
#include <iomanip>
#include <cstring>

// Legenda, Inspector e barra di navigazione provvisoria.
//
// Inspector: spiega cosa c'e' sotto il mouse. Cosa sia "un pixel"
// dipende dalla vista:
//   - PUNTO  (RAW, MetricMap)          → un offset del file
//   - TRATTO (DotPlot, Entropy)        → un range di offset
//   - VALORE (Digraph, Histogram, ...) → un byte o una coppia di byte,
//                                        con quante volte e dove compare
// Senza mouse sulla vista mostra il riassunto del file e della finestra.

namespace {

std::string hex(size_t v, int digits) {
    std::ostringstream o;
    o << std::uppercase << std::hex << std::setw(digits) << std::setfill('0') << v;
    return o.str();
}

std::string sizeStr(size_t sz) {
    if (sz >= 1024 * 1024) return std::to_string(sz / (1024 * 1024)) + " MB";
    if (sz >= 1024)        return std::to_string(sz / 1024) + " KB";
    return std::to_string(sz) + " B";
}

std::string byteDesc(uint8_t b) {
    std::string s = "0x" + hex(b, 2);
    if (b >= 0x20 && b < 0x7F) s += std::string(" '") + (char)b + "'";
    s += std::string("  ") + ByteInfo::className(ByteInfo::classOf(b));
    return s;
}

std::string pct(double f) {
    std::ostringstream o;
    o << std::fixed << std::setprecision(f < 0.01 ? 3 : 1) << f * 100 << "%";
    return o.str();
}

olc::Pixel classPixel(ByteInfo::Class c) {
    uint8_t r = 0, g = 0, b = 0;
    ByteInfo::classColor(c, r, g, b);
    if (c == ByteInfo::Class::ZERO) r = g = b = 110;   // grigio leggibile su sfondo scuro
    return olc::Pixel(r, g, b);
}

// Composizione + entropia + ipotesi
void writeStats(PanelWriter& pw, const ByteInfo::Stats& s) {
    using ByteInfo::Class;
    pw.bar("zero",        s.frac[(int)Class::ZERO],    classPixel(Class::ZERO));
    pw.bar("controllo",   s.frac[(int)Class::CONTROL], classPixel(Class::CONTROL));
    pw.bar("testo",       s.frac[(int)Class::TEXT],    classPixel(Class::TEXT));
    pw.bar("alti >0x7F",  s.frac[(int)Class::HIGH],    classPixel(Class::HIGH));
    pw.bar("0xFF",        s.frac[(int)Class::FF],      classPixel(Class::FF));
    std::ostringstream e;
    e << std::fixed << std::setprecision(2) << s.entropy << " / 8 bit";
    pw.field("ENTROPIA", e.str());
    pw.field("IPOTESI", ByteInfo::guess(s), COL_GREEN());
}

const char* legendFor(ViewMode m) {
    switch (m) {
        case ViewMode::DIGRAPH:
            return "DIGRAPH  X = byte i   Y = byte i+1   luminosita' = quante volte (log)";
        case ViewMode::DOTPLOT:
            return "DOT PLOT  X = posizione nella finestra   Y = valore del byte";
        case ViewMode::ENTROPY:
            return "ENTROPIA  X = posizione   altezza = entropia (0-8 bit)   rosso = alta";
        case ViewMode::HISTOGRAM:
            return "ISTOGRAMMA  X = valore del byte 0x00-0xFF   altezza = frequenza (log)";
        case ViewMode::TRIGRAPH3D:
            return "3D  assi = 3 byte consecutivi (i, i+1, i+2)   luminosita' = frequenza";
        case ViewMode::RAWPIXELS:
            return "RAW  i byte come pixel, in ordine, riga per riga   (bpp: [ ]  larghezza: , .)";
        case ViewMode::METRICMAP:
            return "METRIC MAP  curva: byte vicini nel file = pixel vicini   colore: C   curva: V";
        case ViewMode::BYTECLOUD:
            return "BYTE CLOUD  griglia 16x16 = valori 0x00-0xFF   grandezza = frequenza";
        case ViewMode::ONETUPLE:
            return "ONE TUPLE  X = valore del byte   Y = tratto del file (256 righe)   luminosita' = frequenza nella riga";
    }
    return "";
}

} // namespace

// ---------------------------------------------------------------------

void Visualizer::drawLegend() {
    FillRect(CANVAS_X, LEGEND_Y, CANVAS_W + SCROLLBAR_V_W, LEGEND_H, COL_BG());
    DrawLine(CANVAS_X, LEGEND_Y + LEGEND_H - 1,
             CANVAS_X + CANVAS_W + SCROLLBAR_V_W, LEGEND_Y + LEGEND_H - 1, COL_BORDER());
    std::string s = legendFor(m_mode);
    int room = (CANVAS_W + SCROLLBAR_V_W - 12) / 8;
    DrawString(CANVAS_X + 6, LEGEND_Y + 4, s.substr(0, room), olc::Pixel(200, 200, 140));
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

// ---------------------------------------------------------------------

void Visualizer::drawInspector() {
    drawPanel(this, INSPECTOR_X, SIDE_Y, INSPECTOR_W, SIDE_H, "INSPECTOR");
    PanelWriter pw{this, INSPECTOR_X + 8, SIDE_Y + 20, INSPECTOR_W - 16};

    if (m_computing.load()) { pw.text("calcolo in corso...", COL_YELLOW()); return; }

    const auto& bytes = m_reader.getBytes();
    const size_t wStart = m_navigator.getStart();
    const size_t wEnd   = std::min(m_navigator.getEnd(), bytes.size());
    const size_t wLen   = wEnd > wStart ? wEnd - wStart : 0;

    auto writeSection = [&](size_t off) {
        if (m_parser.getFormat() == BinaryFormat::UNKNOWN) {
            pw.field("SEZIONE", "(formato non riconosciuto)", olc::Pixel(150, 150, 150));
            return;
        }
        int s = sectionAt(off);
        if (s < 0) { pw.field("SEZIONE", "(fuori dalle sezioni)", olc::Pixel(150, 150, 150)); return; }
        const auto& sec = m_parser.getSections()[s];
        pw.field("SEZIONE", sec.name + "  (" + sizeStr(sec.size) + ")",
                 olc::Pixel(sec.r, sec.g, sec.b));
    };

    // Dump esadecimale: `rows` righe da 8 byte, con evidenziato `mark`
    auto writeHex = [&](size_t center, size_t mark, int rows) {
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
    };

    // Statistiche su una zona, al massimo 1 MB (oltre si campiona l'inizio)
    auto zoneStats = [&](size_t a, size_t b) {
        b = std::min(b, a + (size_t)(1 << 20));
        return ByteInfo::statsOf(bytes, a, b);
    };

    int cx = 0, cy = 0;
    bool over = mouseToContent(GetMouseX(), GetMouseY(), cx, cy);

    // ---------------- PUNTO: un offset del file ----------------
    size_t point = SIZE_MAX;
    if (over && m_mode == ViewMode::RAWPIXELS) {
        size_t rel = m_rawpixels.byteOffsetAt(cx, cy);
        if (rel != SIZE_MAX) point = wStart + rel;
    } else if (over && m_mode == ViewMode::METRICMAP) {
        point = wStart + m_metricmap.fileIndexAt(cx, cy);
    }
    if (point != SIZE_MAX && point < bytes.size()) {
        pw.field("OFFSET", "0x" + hex(point, 8) + "  (" + std::to_string(point) + ")", COL_YELLOW());
        writeSection(point);
        pw.field("BYTE", byteDesc(bytes[point]), classPixel(ByteInfo::classOf(bytes[point])));
        pw.separator("byte intorno");
        writeHex(point, point, 6);
        pw.separator("zona: 1 KB intorno");
        size_t a = point >= 512 ? point - 512 : 0;
        writeStats(pw, zoneStats(a, std::min(bytes.size(), a + 1024)));
        return;
    }

    // ---------------- TRATTO: una colonna = un range ----------------
    if (over && wLen > 0 &&
        (m_mode == ViewMode::DOTPLOT || m_mode == ViewMode::ENTROPY)) {
        // Le due viste mettono la posizione i in colonna i*255/len
        size_t a = wStart + (size_t)((double)cx * wLen / 255.0);
        size_t b = wStart + (size_t)((double)(cx + 1) * wLen / 255.0);
        b = std::min(std::max(b, a + 1), wEnd);
        pw.field("TRATTO", "0x" + hex(a, 8) + "-0x" + hex(b, 8), COL_YELLOW());
        pw.field("LUNGH.", sizeStr(b - a));
        writeSection(a);
        if (m_mode == ViewMode::DOTPLOT) {
            uint8_t v = (uint8_t)cy;   // Y = valore del byte
            size_t n = 0;
            for (size_t i = a; i < b; i++) n += (bytes[i] == v);
            pw.field("VALORE", byteDesc(v), classPixel(ByteInfo::classOf(v)));
            pw.field("VOLTE", std::to_string(n) + " in questo tratto");
        }
        pw.separator("inizio del tratto");
        writeHex(a, a, 3);
        pw.separator("composizione del tratto");
        writeStats(pw, zoneStats(a, b));
        return;
    }

    // ---------------- VALORE: byte o coppia di byte ----------------
    if (over && (m_mode == ViewMode::DIGRAPH || m_mode == ViewMode::HISTOGRAM ||
                 m_mode == ViewMode::BYTECLOUD || m_mode == ViewMode::ONETUPLE)) {
        uint8_t pat[2];
        size_t  n = 1, count = 0, total = wLen;
        if (m_mode == ViewMode::DIGRAPH) {
            pat[0] = (uint8_t)cx;  pat[1] = (uint8_t)cy;  n = 2;
            count  = m_digraph.get(pat[0], pat[1]);
            total  = wLen > 0 ? wLen - 1 : 0;
            pw.field("COPPIA", "0x" + hex(pat[0], 2) + " seguito da 0x" + hex(pat[1], 2), COL_YELLOW());
            pw.field("1o BYTE", byteDesc(pat[0]), classPixel(ByteInfo::classOf(pat[0])));
            pw.field("2o BYTE", byteDesc(pat[1]), classPixel(ByteInfo::classOf(pat[1])));
        } else {
            if (m_mode == ViewMode::BYTECLOUD) pat[0] = (uint8_t)((cy / 16) * 16 + cx / 16);
            else                               pat[0] = (uint8_t)cx;   // X = valore
            count = m_windowHist[pat[0]];
            pw.field("VALORE", byteDesc(pat[0]), classPixel(ByteInfo::classOf(pat[0])));
            if (m_mode == ViewMode::ONETUPLE && cy < m_onetuple.getRows()) {
                // Riga y = 1/256 della finestra (vedi OneTuple::compute)
                int rows = m_onetuple.getRows();
                size_t a = wStart + (size_t)cy * wLen / rows;
                size_t b = wStart + (size_t)(cy + 1) * wLen / rows;
                size_t inRow = 0;
                for (size_t i = a; i < b; i++) inRow += (bytes[i] == pat[0]);
                pw.field("RIGA", "0x" + hex(a, 8) + "-0x" + hex(b, 8));
                pw.field("IN RIGA", std::to_string(inRow) + " volte su " + sizeStr(b - a));
                writeSection(a);
            }
        }
        pw.field("VOLTE", std::to_string(count) +
                 (total ? "  (" + pct((double)count / total) + ")" : ""));
        if (count > 0) {
            size_t first = firstOccurrence(pat, n);
            if (first != SIZE_MAX) {
                pw.field("PRIMA", "0x" + hex(first, 8));
                writeSection(first);
                pw.separator("prima occorrenza");
                writeHex(first, first, 3);
            }
        } else {
            pw.text("non compare in questa finestra", olc::Pixel(150, 150, 150));
        }
        pw.gap(6);
        pw.text("dove compare nel file: in arrivo", olc::Pixel(120, 120, 150));
        pw.text("(segni sulle barre, passo 3)", olc::Pixel(120, 120, 150));
        return;
    }

    // ---------------- Nessun hover: riassunto ----------------
    std::string path = m_reader.getFilepath();
    std::string name = path.substr(path.find_last_of('/') + 1);
    pw.field("FILE", name, COL_CYAN());
    pw.field("DIMENS.", sizeStr(bytes.size()) + "  (" + std::to_string(bytes.size()) + " B)");
    pw.field("FORMATO", m_parser.getFormatName());
    if (m_parser.hasSections())
        pw.field("SEZIONI", std::to_string(m_parser.getSections().size()));

    pw.separator("finestra corrente");
    pw.field("RANGE", "0x" + hex(wStart, 8) + "-0x" + hex(wEnd, 8));
    pw.field("LUNGH.", sizeStr(wLen));
    writeStats(pw, ByteInfo::statsFromHist(m_windowHist));

    if (m_parser.hasSections()) {
        pw.separator("sezioni piu' grandi");
        std::vector<int> idx(m_parser.getSections().size());
        for (size_t i = 0; i < idx.size(); i++) idx[i] = (int)i;
        const auto& secs = m_parser.getSections();
        std::sort(idx.begin(), idx.end(), [&](int a, int b) { return secs[a].size > secs[b].size; });
        int bottom = SIDE_Y + SIDE_H - 40;
        int shown = 0;
        for (int i : idx) {
            float f = bytes.empty() ? 0.0f : (float)secs[i].size / bytes.size();
            if (f < 0.005f || pw.y > bottom) break;   // le piccole: riassunte sotto
            pw.bar(secs[i].name.substr(0, 11), f,
                   olc::Pixel(secs[i].r, secs[i].g, secs[i].b));
            shown++;
        }
        int rest = (int)idx.size() - shown;
        if (rest > 0)
            pw.text("+ " + std::to_string(rest) + " sezioni piccole (<0.5%)",
                    olc::Pixel(150, 150, 150));
    }
    pw.y = SIDE_Y + SIDE_H - 14;
    pw.text("mouse sulla vista = dettagli", olc::Pixel(120, 120, 150));
}

// ---------------------------------------------------------------------
// Barra di navigazione a sinistra (versione semplice; le due barre con le
// sezioni arrivano al passo 2). Mostra i byte del file in sequenza, riga
// per riga, in verde come i raw pixel a 8bpp: zeri scuri, dati chiari.
// La parte fuori dalla finestra corrente e' attenuata.

namespace {
struct NavBar { int x, y, w, h; };
NavBar navBar() { return {14, SIDE_Y + 18, NAV_W - 28, SIDE_H - 18 - 46}; }
}

void Visualizer::drawNavPanel() {
    drawPanel(this, 0, SIDE_Y, NAV_W, SIDE_H, "FILE");
    NavBar b = navBar();
    const auto& bytes = m_reader.getBytes();
    const size_t total = bytes.size();

    // Miniatura: un pixel = un byte campionato, in ordine di file
    if (!m_navSprite || m_navData != bytes.data() || m_navSize != total) {
        delete m_navSprite;
        m_navSprite = new olc::Sprite(b.w, b.h);
        m_navData = bytes.data();
        m_navSize = total;
        const size_t px = (size_t)b.w * b.h;
        for (int y = 0; y < b.h; y++)
            for (int x = 0; x < b.w; x++) {
                uint8_t v = 0;
                if (total > 0) v = bytes[((size_t)y * b.w + x) * total / px];
                m_navSprite->SetPixel(x, y, olc::Pixel(0, v, 0));
            }
    }
    DrawSprite(b.x, b.y, m_navSprite);

    if (total > 0) {
        int y0 = b.y + (int)((double)m_navigator.getStart() / total * b.h);
        int y1 = b.y + (int)((double)m_navigator.getEnd()   / total * b.h);
        y1 = std::max(y1, y0 + 2);
        // Attenua fuori dalla finestra
        SetPixelMode(olc::Pixel::ALPHA);
        if (y0 > b.y)       FillRect(b.x, b.y, b.w, y0 - b.y, olc::Pixel(0, 0, 0, 170));
        if (y1 < b.y + b.h) FillRect(b.x, y1, b.w, b.y + b.h - y1, olc::Pixel(0, 0, 0, 170));
        SetPixelMode(olc::Pixel::NORMAL);
        // Cornice della finestra (sporge un po' per vederla anche sottile)
        DrawRect(b.x - 3, y0 - 1, b.w + 5, y1 - y0 + 1, COL_YELLOW());
    }
    DrawRect(b.x - 1, b.y - 1, b.w + 1, b.h + 1, COL_BORDER());

    // Posizione: inizio, fine, quanta parte del file
    int ty = b.y + b.h + 6;
    DrawString(6, ty,      "0x" + hex(m_navigator.getStart(), 8), olc::Pixel(200, 200, 100));
    DrawString(6, ty + 10, "0x" + hex(m_navigator.getEnd(),   8), olc::Pixel(200, 200, 100));
    std::string share = (total == 0 || m_navigator.getWindowSize() >= total)
                        ? "file intero"
                        : pct((double)m_navigator.getWindowSize() / total) + " del file";
    DrawString(6, ty + 20, share, olc::Pixel(160, 160, 160));
    DrawString(6, ty + 32, "click = vai", olc::Pixel(110, 110, 150));
}

void Visualizer::handleNavPanel() {
    NavBar b = navBar();
    int mx = GetMouseX(), my = GetMouseY();
    if (!GetMouse(0).bHeld) return;
    if (mx < b.x - 3 || mx >= b.x + b.w + 3 || my < b.y || my >= b.y + b.h) return;
    // Centra la finestra sul punto cliccato
    size_t total = m_navigator.getTotalSize(), win = m_navigator.getWindowSize();
    if (total == 0 || win >= total) return;
    double c = (double)(my - b.y) / b.h * total - win / 2.0;
    float norm = (float)std::clamp(c / (double)(total - win), 0.0, 1.0);
    size_t before = m_navigator.getStart();
    m_navigator.jumpTo(norm);
    if (m_navigator.getStart() != before) recomputeWindow();
}
