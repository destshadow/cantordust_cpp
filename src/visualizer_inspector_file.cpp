#include "visualizer.h"
#include "inspector_format.h"

// Pannelli dell'Inspector per cio' che ha una posizione nel file:
// una sezione, un punto (un offset), un tratto (un range di offset).

using namespace Inspect;

// Mouse su una banda delle sezioni
void Visualizer::inspectSection(PanelWriter& pw, int index) {
    const auto& sec = m_parser.getSections()[index];
    const size_t total = m_reader.getSize();
    pw.field("SEZIONE", sec.name, olc::Pixel(sec.r, sec.g, sec.b));
    pw.field("RANGE", "0x" + hex(sec.offset, 8) + "-0x" + hex(sec.offset + sec.size, 8));
    pw.field("DIMENS.", sizeStr(sec.size) + "  (" +
             pct((double)sec.size / std::max<size_t>(1, total)) + " del file)");
    pw.text("click = mostra questa sezione", COL_GREEN());
    pw.separator("inizio della sezione");
    writeHexDump(pw, sec.offset, sec.offset, 3);
    pw.separator("composizione");
    writeStats(pw, zoneStats(sec.offset, sec.offset + sec.size));
}

// PUNTO: un offset del file
void Visualizer::inspectPoint(PanelWriter& pw, size_t point) {
    const auto& bytes = m_reader.getBytes();
    pw.field("OFFSET", "0x" + hex(point, 8) + "  (" + std::to_string(point) + ")", COL_YELLOW());
    writeSectionField(pw, point);
    pw.field("BYTE", byteDesc(bytes[point]), classPixel(ByteInfo::classOf(bytes[point])));
    pw.separator("byte intorno");
    writeHexDump(pw, point, point, 6);
    pw.separator("zona: 1 KB intorno");
    size_t a = point >= 512 ? point - 512 : 0;
    writeStats(pw, zoneStats(a, std::min(bytes.size(), a + 1024)));
}

// TRATTO: una colonna = un range di offset (DotPlot, Entropy)
void Visualizer::inspectRange(PanelWriter& pw, int cx, int cy) {
    const auto& bytes = m_reader.getBytes();
    const InspectWindow w = inspectWindow();
    // Le due viste mettono la posizione i in colonna i*255/len
    size_t a = w.start + (size_t)((double)cx * w.len / 255.0);
    size_t b = w.start + (size_t)((double)(cx + 1) * w.len / 255.0);
    b = std::min(std::max(b, a + 1), w.end);
    pw.field("TRATTO", "0x" + hex(a, 8) + "-0x" + hex(b, 8), COL_YELLOW());
    pw.field("LUNGH.", sizeStr(b - a));
    writeSectionField(pw, a);
    if (m_mode == ViewMode::DOTPLOT) {
        uint8_t v = (uint8_t)cy;   // Y = valore del byte
        size_t n = 0;
        for (size_t i = a; i < b; i++) n += (bytes[i] == v);
        pw.field("VALORE", byteDesc(v), classPixel(ByteInfo::classOf(v)));
        pw.field("VOLTE", std::to_string(n) + " in questo tratto");
    }
    pw.separator("inizio del tratto");
    writeHexDump(pw, a, a, 3);
    pw.separator("composizione del tratto");
    writeStats(pw, zoneStats(a, b));
}
