#include "visualizer.h"
#include "inspector_format.h"

// Pannello dell'Inspector per le viste dove un pixel e' un valore, non una
// posizione: Digraph (una coppia), Histogram, ByteCloud, OneTuple (un byte),
// 3D (una tripla).

using namespace Inspect;

void Visualizer::inspectValue(PanelWriter& pw, int cx, int cy) {
    const auto& bytes = m_reader.getBytes();
    const InspectWindow w = inspectWindow();
    uint8_t pat[3];
    const size_t n = valueAt(cx, cy, pat);
    size_t count = 0, total = w.len;

    if (m_mode == ViewMode::DIGRAPH) {
        count  = m_digraph.get(pat[0], pat[1]);
        total  = w.len > 0 ? w.len - 1 : 0;
        pw.field("COPPIA", "0x" + hex(pat[0], 2) + " seguito da 0x" + hex(pat[1], 2), COL_ACCENT());
        pw.field("1o BYTE", byteDesc(pat[0]), classPixel(ByteInfo::classOf(pat[0])));
        pw.field("2o BYTE", byteDesc(pat[1]), classPixel(ByteInfo::classOf(pat[1])));
    } else {
        count = m_windowHist[pat[0]];
        pw.field("VALORE", byteDesc(pat[0]), classPixel(ByteInfo::classOf(pat[0])));
        if (m_mode == ViewMode::ONETUPLE && cy < m_onetuple.getRows()) {
            // Riga y = 1/256 della finestra (vedi OneTuple::compute)
            int rows = m_onetuple.getRows();
            size_t a = w.start + (size_t)cy * w.len / rows;
            size_t b = w.start + (size_t)(cy + 1) * w.len / rows;
            size_t inRow = 0;
            for (size_t i = a; i < b; i++) inRow += (bytes[i] == pat[0]);
            pw.field("RIGA", "0x" + hex(a, 8) + "-0x" + hex(b, 8));
            pw.field("IN RIGA", std::to_string(inRow) + " volte su " + sizeStr(b - a));
            writeSectionField(pw, a);
        }
    }

    pw.field("VOLTE", std::to_string(count) +
             (total ? "  (" + pct((double)count / total) + ")" : ""));
    if (count == 0) {
        pw.text("non compare in questa finestra", COL_DIM());
        return;
    }

    writeOccurrences(pw, pat, n);
}

// 3D: la tripla di byte sotto il mouse
void Visualizer::inspectTriple(PanelWriter& pw, uint32_t key) {
    const uint8_t pat[3] = {(uint8_t)(key >> 16), (uint8_t)(key >> 8), (uint8_t)key};
    const Point3D* p = m_trigraph.find(key);
    if (!p) return;
    const InspectWindow w = inspectWindow();
    const size_t total = w.len > 2 ? w.len - 2 : 0;

    pw.field("TRIPLA", "0x" + hex(pat[0], 2) + " 0x" + hex(pat[1], 2) + " 0x" + hex(pat[2], 2), COL_ACCENT());
    pw.field("1o BYTE", byteDesc(pat[0]), classPixel(ByteInfo::classOf(pat[0])));
    pw.field("2o BYTE", byteDesc(pat[1]), classPixel(ByteInfo::classOf(pat[1])));
    pw.field("3o BYTE", byteDesc(pat[2]), classPixel(ByteInfo::classOf(pat[2])));
    pw.field("VOLTE", std::to_string(p->count) +
             (total ? "  (" + pct((double)p->count / total) + ")" : ""));
    const auto& secs = m_parser.getSections();
    if (p->section >= 0 && (size_t)p->section < secs.size()) {
        const auto& s = secs[p->section];
        pw.field("PREVALE", s.name, olc::Pixel(s.r, s.g, s.b));
    }

    // La ricerca scorre il file: si aspetta che il mouse si fermi, come per i
    // segni sulle barre, cosi' attraversare la nuvola resta fluido
    if (m_markWait >= MARK_WAIT && m_markWanted == std::vector<uint8_t>(pat, pat + 3))
        writeOccurrences(pw, pat, 3);
}

// Dove compare il valore: la prima volta, o quella scelta col click
void Visualizer::writeOccurrences(PanelWriter& pw, const uint8_t* pat, size_t n) {
    // Se questo valore e' stato scelto col click si segue la sua posizione,
    // altrimenti si mostra la prima volta che compare
    const bool picked = m_pick.active() && m_pick.sameAs(pat, n);
    size_t at = picked ? m_pick.target : firstOccurrence(pat, n);
    if (at == SIZE_MAX) return;
    if (picked)
        pw.field("POSIZ.", std::to_string(m_pick.index) + " di " + std::to_string(m_pick.count));
    pw.field(picked ? "OFFSET" : "PRIMA", "0x" + hex(at, 8), COL_ACCENT());
    writeSectionField(pw, at);
    pw.separator(picked ? "questa occorrenza" : "prima occorrenza");
    writeHexDump(pw, at, at, 3);
    pw.gap(6);
    pw.text("segni viola: dove compare nel file", olc::Pixel(255, 110, 255));
    pw.text(picked ? "click = la prossima, Esc = togli"
                   : "click = scorri le occorrenze", COL_DIM());
}

// Valore scelto col click, con il mouse altrove
void Visualizer::inspectPick(PanelWriter& pw) {
    const auto& p = m_pick.pattern;
    std::string v = "0x" + hex(p[0], 2);
    for (size_t i = 1; i < p.size(); i++) v += " 0x" + hex(p[i], 2);
    pw.field("SCELTO", v, COL_ACCENT());
    pw.field("POSIZ.", std::to_string(m_pick.index) + " di " + std::to_string(m_pick.count));
    pw.field("OFFSET", "0x" + hex(m_pick.target, 8), COL_ACCENT());
    writeSectionField(pw, m_pick.target);
    pw.separator("questa occorrenza");
    writeHexDump(pw, m_pick.target, m_pick.target, 6);
    pw.gap(6);
    pw.text("Esc = togli la scelta", COL_DIM());
}
