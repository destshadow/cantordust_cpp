#include "visualizer.h"
#include "inspector_format.h"

// Pannello dell'Inspector per la vista Stringhe

using namespace Inspect;

void Visualizer::inspectString(PanelWriter& pw, const StringScan::Hit& s) {
    static constexpr size_t MAX_LINES = 12;
    const size_t off = m_stringBase + s.offset;

    pw.field("OFFSET", "0x" + hex(off, 8), COL_ACCENT());
    pw.field("TIPO", StringScan::encName(s.enc));
    pw.field("LUNGH.", std::to_string(s.chars) + " caratteri");
    if (s.tag != StringScan::Tag::NONE) pw.field("TAG", StringScan::tagName(s.tag));
    writeSectionField(pw, off);

    pw.separator("testo");
    const size_t perLine = (size_t)pw.maxChars();
    const std::string text = StringScan::textOf(m_reader.getBytes(), m_stringBase, s,
                                                perLine * MAX_LINES);
    for (size_t i = 0; i < text.size(); i += perLine)
        pw.text(text.substr(i, perLine));
    if (s.chars > text.size()) pw.text("...", COL_DIM());

    pw.separator("nel file");
    writeHexDump(pw, off, off, 5);
}
