#include "visualizer.h"
#include <algorithm>

// Viste 2D: dove sta il contenuto sullo schermo, zoom e scorrimento.

bool Visualizer::hasCanvas() const {
    return m_mode != ViewMode::TRIGRAPH3D && m_mode != ViewMode::STRINGS &&
           m_mode != ViewMode::HEX;
}

void Visualizer::viewArea(int& x, int& y, int& w, int& h) const {
    x = CANVAS_X;  y = CANVAS_Y;  w = CANVAS_W;  h = CANVAS_H;
    if (m_mode == ViewMode::RAWPIXELS) h -= RAW_PANEL_H;
}

void Visualizer::contentSize(int& w, int& h) const {
    w = h = 256;
    if (m_mode == ViewMode::RAWPIXELS &&
        m_rawpixels.getWidth() > 0 && m_rawpixels.getHeight() > 0) {
        w = m_rawpixels.getWidth();
        h = m_rawpixels.getHeight();
    } else if (m_mode == ViewMode::BYTECLOUD) {
        w = h = ByteCloud::SIZE;
    }
}

// Posizione e scala del contenuto sullo schermo.
// A zoom 1 il contenuto riempie l'area mantenendo le proporzioni;
// lo zoom moltiplica, lo scroll sposta quando il contenuto e' piu' grande.
// Unica fonte di verita': usata da disegno, overlay sezioni e info byte.
void Visualizer::canvasTransform(float& drawX, float& drawY,
                                 float& scale) const {
    int ax, ay, aw, ah, cw, ch;
    viewArea(ax, ay, aw, ah);
    contentSize(cw, ch);

    float fit = std::min((float)aw / cw, (float)ah / ch);
    scale = fit * m_zoom2d;
    float sw = cw * scale, sh = ch * scale;
    float maxOffX = std::max(0.0f, sw - (float)aw);
    float maxOffY = std::max(0.0f, sh - (float)ah);
    drawX = (sw <= (float)aw) ? ax + ((float)aw - sw) * 0.5f
                              : (float)ax - m_scrollH * maxOffX;
    drawY = (sh <= (float)ah) ? ay + ((float)ah - sh) * 0.5f
                              : (float)ay - m_scrollV * maxOffY;
}

void Visualizer::handle2DInput() {
    if (!hasCanvas()) return;

    int mx = GetMouseX();
    int my = GetMouseY();
    if (my < TOPBAR_H) return;

    int ax, ay, aw, ah, cw, ch;
    viewArea(ax, ay, aw, ah);
    contentSize(cw, ch);
    bool inArea = (mx >= ax && mx < ax + aw && my >= ay && my < ay + ah);

    // Zoom con rotella
    if (inArea) {
        if (GetMouseWheel() > 0)
            m_zoom2d = std::min(16.0f, m_zoom2d * 1.25f);
        if (GetMouseWheel() < 0) {
            m_zoom2d = std::max(1.0f, m_zoom2d / 1.25f);
            if (m_zoom2d <= 1.0f) {
                m_scrollH = 0.0f;
                m_scrollV = 0.0f;
            }
        }
    }

    float drawX, drawY, scale;
    canvasTransform(drawX, drawY, scale);
    float maxOffX = std::max(1.0f, cw * scale - (float)aw);
    float maxOffY = std::max(1.0f, ch * scale - (float)ah);

    // Pan con drag sinistro
    if (m_zoom2d > 1.0f && inArea && !m_input.lastClickWasTab()) {
        if (GetMouse(0).bPressed) {
            m_dragging   = true;
            m_lastMouseX = mx;
            m_lastMouseY = my;
        }
    }
    if (GetMouse(0).bReleased) m_dragging = false;

    if (m_dragging && m_zoom2d > 1.0f) {
        float dx = (float)(mx - m_lastMouseX);
        float dy = (float)(my - m_lastMouseY);
        m_scrollH = std::clamp(m_scrollH - dx/maxOffX, 0.0f, 1.0f);
        m_scrollV = std::clamp(m_scrollV - dy/maxOffY, 0.0f, 1.0f);
        m_lastMouseX = mx;
        m_lastMouseY = my;
    }

    // Scrollbar: il trascinamento inizia sulla barra e continua finche' si
    // tiene premuto, anche uscendo; il cursore resta sotto il mouse
    const bool onH = my >= SCROLLBAR_H_Y && my < SCROLLBAR_H_Y + SCROLLBAR_H &&
                     mx >= CANVAS_X && mx < CANVAS_X + CANVAS_W;
    const bool onV = mx >= SCROLLBAR_V_X && mx < SCROLLBAR_V_X + SCROLLBAR_V_W &&
                     my >= CANVAS_Y && my < CANVAS_Y + CANVAS_H;
    if (GetMouse(0).bPressed && m_zoom2d > 1.0f)
        m_scrollDrag = onH ? ScrollDrag::H : onV ? ScrollDrag::V : ScrollDrag::NONE;
    if (!GetMouse(0).bHeld || m_zoom2d <= 1.0f) m_scrollDrag = ScrollDrag::NONE;

    // Stessa geometria del disegno (ui_renderer.cpp)
    auto fromBar = [&](int pos, int start, int len) {
        const float thumb = std::max(20.0f, len / m_zoom2d);
        const float room  = std::max(1.0f, len - thumb);
        return std::clamp((pos - start - thumb * 0.5f) / room, 0.0f, 1.0f);
    };
    if (m_scrollDrag == ScrollDrag::H) m_scrollH = fromBar(mx, CANVAS_X, CANVAS_W);
    if (m_scrollDrag == ScrollDrag::V) m_scrollV = fromBar(my, CANVAS_Y, CANVAS_H);
}
