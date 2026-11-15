#include "visualizer.h"
#include "inspector_format.h"

// Pannello dell'Inspector per la Somiglianza: i due tratti che il pixel
// confronta, e quanto hanno in comune.

using namespace Inspect;

void Visualizer::inspectSimilarity(PanelWriter& pw, int cx, int cy) {
    const InspectWindow w = inspectWindow();
    auto range = [&](int cell, size_t& a, size_t& b) {
        a = std::min(w.start + (size_t)cell * w.len / Similarity::SIZE, w.end - 1);
        b = std::min(std::max(w.start + (size_t)(cell + 1) * w.len / Similarity::SIZE, a + 1), w.end);
    };
    size_t xa, xb, ya, yb;
    range(cx, xa, xb);
    range(cy, ya, yb);

    pw.field("TRATTO X", "0x" + hex(xa, 8) + "-0x" + hex(xb, 8), COL_ACCENT());
    pw.field("TRATTO Y", "0x" + hex(ya, 8) + "-0x" + hex(yb, 8), COL_ACCENT());
    pw.field("LUNGH.", sizeStr(xb - xa) + " ciascuno");
    pw.field("DISTANZA", sizeStr(xa > ya ? xa - ya : ya - xa));
    const float v = m_similarity.at(cx, cy);
    if (cx == cy)      pw.field("COMUNE", "ripetizioni nel tratto", COL_DIM());
    else if (v <= 0)   pw.field("COMUNE", "nulla", COL_DIM());
    else               pw.field("COMUNE", v < 3 ? "poco" : v < 30 ? "abbastanza" : "molto");

    pw.separator("inizio del tratto X");
    writeHexDump(pw, xa, xa, 3);
    pw.separator("inizio del tratto Y");
    writeHexDump(pw, ya, ya, 3);
    pw.gap();
    pw.text("click = cursore sul tratto Y (poi H)", COL_DIM());
    pw.text("diagonale = contenuto ripetuto", COL_DIM());
    pw.text("quadrato = zona omogenea", COL_DIM());
}
