#include "visualizer.h"
#include <algorithm>

// Il cursore: un punto del file scelto dall'utente (una stringa, una firma,
// un'occorrenza). Sta sulle barre e, nelle viste dove un pixel e' un offset
// (Raw, Metric Map), anche sulla vista.

// Porta le barre su un punto del file e lo segna; la finestra si sposta solo
// se quel punto ne e' fuori
void Visualizer::goToOffset(size_t offset) {
    m_cursorOffset = offset;
    m_bars.goTo(offset);
    syncNavigator();
}

// Il pixel del contenuto che mostra il cursore; false se non si vede
bool Visualizer::cursorPixel(int& cx, int& cy) const {
    if (m_cursorOffset == SIZE_MAX || m_computing) return false;
    if (m_cursorOffset < m_computedStart || m_cursorOffset >= m_computedEnd) return false;
    const size_t rel = m_cursorOffset - m_computedStart;
    if (m_mode == ViewMode::RAWPIXELS) return m_rawpixels.pixelOf(rel, cx, cy);
    if (m_mode == ViewMode::METRICMAP) return m_metricmap.pixelOf(rel, cx, cy);
    return false;
}

// Un riquadro attorno al pixel. Con i decal: i pixel normali finirebbero
// sotto la vista.
void Visualizer::drawCursorMarker() {
    int cx, cy;
    if (!cursorPixel(cx, cy)) return;
    int ax, ay, aw, ah;
    viewArea(ax, ay, aw, ah);
    float drawX, drawY, scale;
    canvasTransform(drawX, drawY, scale);
    const float x = drawX + cx * scale, y = drawY + cy * scale;
    const float side = std::max(scale, 1.0f);
    if (x < ax || y < ay || x + side > ax + aw || y + side > ay + ah) return;

    // Due cornici: bianca fuori, nera dentro, cosi' si vede su ogni sfondo
    auto frame = [&](float gap, float thick, olc::Pixel col) {
        const float x0 = x - gap - thick, y0 = y - gap - thick;
        const float len = side + 2 * (gap + thick);
        FillRectDecal({x0, y0}, {len, thick}, col);
        FillRectDecal({x0, y0 + len - thick}, {len, thick}, col);
        FillRectDecal({x0, y0}, {thick, len}, col);
        FillRectDecal({x0 + len - thick, y0}, {thick, len}, col);
    };
    frame(4, 2, olc::WHITE);
    frame(3, 1, olc::BLACK);
}
