#include "visualizer.h"
#include <algorithm>
#include <cstdlib>

// Il cursore: un punto del file scelto dall'utente (una stringa, una firma,
// un'occorrenza). Sta sulle barre e su ogni vista che ha una posizione nel
// file: Raw, Metric Map, Hex, Dot Plot, Entropia, One Tuple.

// Porta le barre su un punto del file e lo segna; la finestra si sposta solo
// se quel punto ne e' fuori
void Visualizer::goToOffset(size_t offset) {
    m_cursorOffset = offset;
    m_bars.goTo(offset);
    syncNavigator();
}

// Dove sta il cursore nel contenuto della vista. Nelle viste dove un pixel
// e' un offset e' un punto; dove una colonna o una riga e' un tratto del
// file e' una linea (x o y = -1 se quella coordinata non c'e').
bool Visualizer::cursorPixel(int& cx, int& cy) const {
    cx = cy = -1;
    if (m_cursorOffset == SIZE_MAX || m_computing) return false;
    const size_t end = std::min(m_computedEnd, m_reader.getSize());
    if (m_cursorOffset < m_computedStart || m_cursorOffset >= end) return false;
    const size_t rel = m_cursorOffset - m_computedStart, len = end - m_computedStart;

    switch (m_mode) {
        case ViewMode::RAWPIXELS: return m_rawpixels.pixelOf(rel, cx, cy);
        case ViewMode::METRICMAP: return m_metricmap.pixelOf(rel, cx, cy);
        case ViewMode::ENTROPY:
            cx = (int)(rel * 256 / len);
            return true;
        case ViewMode::DOTPLOT:
            cx = (int)(rel * 256 / len);
            if (m_dotSimilarity) cy = cx;          // X e Y sono la stessa posizione
            return true;
        case ViewMode::ONETUPLE:
            if (m_onetuple.getRows() <= 0) return false;
            cy = (int)(rel * m_onetuple.getRows() / len);
            return true;
        default: return false;
    }
}

// Un riquadro attorno al pixel, o una linea lungo la colonna o la riga.
// Con i decal: i pixel normali finirebbero sotto la vista.
void Visualizer::drawCursorMarker() {
    int cx, cy;
    if (!cursorPixel(cx, cy)) return;
    int ax, ay, aw, ah, cw, ch;
    viewArea(ax, ay, aw, ah);
    contentSize(cw, ch);
    float drawX, drawY, scale;
    canvasTransform(drawX, drawY, scale);
    const float side = std::max(scale, 1.0f);
    const float x = drawX + cx * scale, y = drawY + cy * scale;
    const bool xIn = cx >= 0 && x >= ax && x + side <= ax + aw;
    const bool yIn = cy >= 0 && y >= ay && y + side <= ay + ah;

    if (cx < 0 || cy < 0 || m_mode == ViewMode::DOTPLOT) {
        // Linee sottili al centro della colonna o della riga, tagliate sull'area
        const olc::Pixel line(255, 255, 255, 170);
        const float top  = std::max(drawY, (float)ay), left = std::max(drawX, (float)ax);
        const float bottom = std::min(drawY + ch * scale, (float)(ay + ah));
        const float right  = std::min(drawX + cw * scale, (float)(ax + aw));
        if (xIn) FillRectDecal({x + side / 2, top}, {1, bottom - top}, line);
        if (yIn) FillRectDecal({left, y + side / 2}, {right - left, 1}, line);
        return;
    }
    if (!xIn || !yIn) return;

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

// Somiglianza: un click (non un trascinamento) porta il cursore al tratto Y
void Visualizer::updateSimilarityClick() {
    if (m_mode != ViewMode::DOTPLOT || !m_dotSimilarity) return;
    if (!GetMouse(0).bReleased) return;
    int cx, cy;
    if (!mouseToContent(GetMouseX(), GetMouseY(), cx, cy)) return;
    if (std::abs(GetMouseX() - m_pressX) > 2 || std::abs(GetMouseY() - m_pressY) > 2) return;
    const InspectWindow w = inspectWindow();
    if (w.len == 0) return;
    goToOffset(std::min(w.start + (size_t)cy * w.len / Similarity::SIZE, w.end - 1));
}
