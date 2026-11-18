#include "visualizer.h"

// Vista Panoramica: una riga per indicatore, la posizione nel file in
// orizzontale. Serve a decidere dove guardare: click = cursore su quel tratto.

namespace {
constexpr int LABEL_W = 100, PAD = 6, GAP = 6;

struct Row {
    const char* name;
    olc::Pixel  color;
    float (*value)(const Overview::Cell&);
};

const Row ROWS[] = {
    {"entropia",   olc::Pixel(220,  90, 110), [](const Overview::Cell& c) { return c.entropy; }},
    {"salti",      olc::Pixel(255, 190,  70), [](const Overview::Cell& c) { return c.jump; }},
    {"testo",      olc::Pixel( 70, 140, 255), [](const Overview::Cell& c) { return c.text; }},
    {"zeri",       olc::Pixel(130, 130, 140), [](const Overview::Cell& c) { return c.zeros; }},
    {"0xFF",       olc::Pixel(230, 230, 235), [](const Overview::Cell& c) { return c.ff; }},
    {"periodo",    olc::Pixel( 90, 210, 120), [](const Overview::Cell& c) { return c.period; }},
    {"bit bassi",  olc::Pixel(200, 110, 240), [](const Overview::Cell& c) { return c.lowBits; }},
};
constexpr int VALUE_ROWS = (int)(sizeof(ROWS) / sizeof(ROWS[0]));
constexpr int MARK_ROWS  = 2;            // firme, fuori dalle sezioni
} // namespace

bool Visualizer::isOverviewView() const { return m_mode == ViewMode::OVERVIEW; }

// Dove stanno le strisce: x0 e larghezza in pixel
void Visualizer::overviewStrip(int& x0, int& w) const {
    x0 = CANVAS_X + LABEL_W;
    w  = Overview::COLS;
}

// La cella sotto il mouse; -1 se nessuna
int Visualizer::overviewCellUnderMouse() const {
    const int n = (int)m_overview.cells().size();
    if (!isOverviewView() || n == 0) return -1;
    int x0, w;
    overviewStrip(x0, w);
    const int mx = GetMouseX(), my = GetMouseY();
    if (mx < x0 || mx >= x0 + w || my < CANVAS_Y || my >= CANVAS_Y + CANVAS_H + SCROLLBAR_H)
        return -1;
    return std::min(n - 1, (mx - x0) * n / w);
}

void Visualizer::handleOverviewInput() {
    const int c = overviewCellUnderMouse();
    if (c >= 0 && GetMouse(0).bPressed)
        goToOffset(m_overviewBase + m_overview.cellStart((size_t)c));
}

void Visualizer::drawOverview() {
    const int x = CANVAS_X, y = CANVAS_Y, w = CANVAS_W + SCROLLBAR_V_W;
    const int h = CANVAS_H + SCROLLBAR_H;
    FillRect(x, y, w, h, COL_BG_DEEP());

    const auto& cells = m_overview.cells();
    const int n = (int)cells.size();
    if (n == 0) return;
    int x0, sw;
    overviewStrip(x0, sw);

    const int rows  = VALUE_ROWS + MARK_ROWS;
    const int rowH  = (h - 2 * PAD - GAP * (rows - 1)) / rows;
    const bool section = m_hlEnd > m_hlStart;
    const int hover = overviewCellUnderMouse();
    auto rowY  = [&](int r) { return y + PAD + r * (rowH + GAP); };
    auto cellX = [&](int c) { return x0 + c * sw / n; };
    auto offsetOf = [&](int c) { return m_overviewBase + m_overview.cellStart((size_t)c); };

    for (int r = 0; r < rows; r++) {
        const int ry = rowY(r);
        FillRect(x0, ry, sw, rowH, COL_BG());
        const char* name = r < VALUE_ROWS ? ROWS[r].name
                         : r == VALUE_ROWS ? "firme" : "fuori sez.";
        DrawString(x + PAD, ry + rowH / 2 - 4, name, COL_TEXT());
    }

    const auto& secs = m_parser.getSections();
    size_t nextSig = 0;
    for (int c = 0; c < n; c++) {
        const int cx = cellX(c), cw = std::max(1, cellX(c + 1) - cx);
        const size_t a = offsetOf(c);
        const size_t end = c + 1 < n ? offsetOf(c + 1) : m_computedEnd;
        const bool dim = section && (a >= m_hlEnd || std::max(end, a + 1) <= m_hlStart);

        for (int r = 0; r < VALUE_ROWS; r++) {
            const float v = std::clamp(ROWS[r].value(cells[c]), 0.0f, 1.0f);
            const int bar = (int)(v * rowH + 0.5f);
            if (bar <= 0) continue;
            olc::Pixel col = ROWS[r].color;
            if (dim) col = olc::Pixel(col.r / 4, col.g / 4, col.b / 4);
            FillRect(cx, rowY(r) + rowH - bar, cw, bar, col);
        }

        // Firme che iniziano in questo tratto
        while (nextSig < m_signatures.size() && m_signatures[nextSig].offset < a) nextSig++;
        if (nextSig < m_signatures.size() && m_signatures[nextSig].offset < end)
            FillRect(cx, rowY(VALUE_ROWS), std::max(cw, 2), rowH, olc::Pixel(90, 220, 255));

        // Byte che nessuna sezione dichiara (solo se il formato e' noto)
        if (!secs.empty() && sectionAt(a) < 0)
            FillRect(cx, rowY(VALUE_ROWS + 1), cw, rowH, olc::Pixel(200, 90, 90));
    }
    if (secs.empty())
        DrawString(x0 + 6, rowY(VALUE_ROWS + 1) + rowH / 2 - 4,
                   "formato non riconosciuto: nessuna sezione", COL_DIM());

    // Il tratto sotto il mouse e il cursore
    auto column = [&](int c, olc::Pixel col) {
        const int cx = cellX(c);
        DrawLine(cx, y + PAD, cx, rowY(rows - 1) + rowH - 1, col);
    };
    if (hover >= 0) column(hover, COL_DIM());
    if (m_cursorOffset >= m_overviewBase && m_cursorOffset < m_computedEnd &&
        m_overviewBase == m_computedStart)
        column((int)m_overview.cellOf(m_cursorOffset - m_overviewBase), olc::WHITE);
}
