#include "visualizer.h"
#include "inspector_format.h"

// Pannello dell'Inspector quando il mouse non e' su nulla di particolare:
// riassunto del file, della finestra corrente e delle sezioni piu' grandi.

using namespace Inspect;

void Visualizer::inspectSummary(PanelWriter& pw) {
    const auto& bytes = m_reader.getBytes();
    const InspectWindow w = inspectWindow();

    std::string path = m_reader.getFilepath();
    std::string name = path.substr(path.find_last_of('/') + 1);
    pw.field("FILE", name, COL_CYAN());
    pw.field("DIMENS.", sizeStr(bytes.size()) + "  (" + std::to_string(bytes.size()) + " B)");
    pw.field("FORMATO", m_parser.getFormatName());
    if (m_parser.hasSections())
        pw.field("SEZIONI", std::to_string(m_parser.getSections().size()));

    pw.separator("finestra corrente");
    pw.field("RANGE", "0x" + hex(w.start, 8) + "-0x" + hex(w.end, 8));
    pw.field("LUNGH.", sizeStr(w.len));
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
