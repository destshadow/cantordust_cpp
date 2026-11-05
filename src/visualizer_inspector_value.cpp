#include "visualizer.h"
#include "inspector_format.h"

// Pannello dell'Inspector per le viste dove un pixel e' un valore, non una
// posizione: Digraph (una coppia), Histogram, ByteCloud, OneTuple (un byte).

using namespace Inspect;

void Visualizer::inspectValue(PanelWriter& pw, int cx, int cy) {
    const auto& bytes = m_reader.getBytes();
    const InspectWindow w = inspectWindow();
    uint8_t pat[2];
    size_t  n = 1, count = 0, total = w.len;

    if (m_mode == ViewMode::DIGRAPH) {
        pat[0] = (uint8_t)cx;  pat[1] = (uint8_t)cy;  n = 2;
        count  = m_digraph.get(pat[0], pat[1]);
        total  = w.len > 0 ? w.len - 1 : 0;
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
    if (count > 0) {
        size_t first = firstOccurrence(pat, n);
        if (first != SIZE_MAX) {
            pw.field("PRIMA", "0x" + hex(first, 8));
            writeSectionField(pw, first);
            pw.separator("prima occorrenza");
            writeHexDump(pw, first, first, 3);
        }
    } else {
        pw.text("non compare in questa finestra", olc::Pixel(150, 150, 150));
    }
    pw.gap(6);
    pw.text("dove compare nel file: in arrivo", olc::Pixel(120, 120, 150));
    pw.text("(segni sulle barre, passo 3)", olc::Pixel(120, 120, 150));
}
