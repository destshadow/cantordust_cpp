#include "visualizer.h"
#include "inspector_format.h"

// Pannello dell'Inspector per la Panoramica: gli indicatori del tratto sotto
// il mouse, in numeri e in parole.

using namespace Inspect;

void Visualizer::inspectOverview(PanelWriter& pw, int cell) {
    const auto& cells = m_overview.cells();
    const Overview::Cell& c = cells[(size_t)cell];
    const InspectWindow w = inspectWindow();
    const size_t a = m_overviewBase + m_overview.cellStart((size_t)cell);
    const size_t b = (size_t)cell + 1 < cells.size()
                         ? m_overviewBase + m_overview.cellStart((size_t)cell + 1) : w.end;

    pw.field("TRATTO", "0x" + hex(a, 8) + "-0x" + hex(b, 8), COL_ACCENT());
    pw.field("LUNGH.", sizeStr(b > a ? b - a : 0));
    writeSectionField(pw, a);

    pw.separator("indicatori");
    pw.field("ENTROPIA", pct(c.entropy) + (c.jump > 0.3f ? "  (cambia qui)" : ""));
    pw.field("TESTO", pct(c.text));
    pw.field("ZERI", pct(c.zeros));
    pw.field("0xFF", pct(c.ff));
    if (c.periodLen > 0)
        pw.field("PERIODO", "ogni " + std::to_string(c.periodLen) + " byte  (" + pct(c.period) + ")");
    else
        pw.field("PERIODO", "nessuno", COL_DIM());
    if (c.lowBits > 0.5f) {
        pw.field("B. BASSI", "casuali", COL_ACCENT());
        pw.text("normale in foto e audio;", COL_DIM());
        pw.text("insolito in codice e testo", COL_DIM());
    } else {
        pw.field("B. BASSI", "niente da segnalare", COL_DIM());
    }

    pw.separator("inizio del tratto");
    writeHexDump(pw, a, a, 3);
    pw.gap();
    pw.text("click = cursore qui (poi H, 6, 7...)", COL_DIM());
}
