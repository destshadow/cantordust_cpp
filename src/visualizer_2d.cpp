#include "visualizer.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

void Visualizer::updateCanvas() {
    // TRIGRAPH3D non usa il canvas 2D — skip
    if (m_mode == ViewMode::TRIGRAPH3D) return;

    auto rgb = getCurrentRGB();
    if (rgb.empty()) return;  // protezione extra

    for (int y = 0; y < 256; y++)
        for (int x = 0; x < 256; x++) {
            int idx = (y * 256 + x) * 3;
            m_canvas->SetPixel(x, y, olc::Pixel(
                rgb[idx], rgb[idx+1], rgb[idx+2]
            ));
        }
    m_decal->Update();
}

std::vector<uint8_t> Visualizer::getCurrentRGB() const {
    switch (m_mode) {
        case ViewMode::DIGRAPH:    return m_digraph.toRGB();
        case ViewMode::DOTPLOT:    return m_dotplot.toRGB();
        case ViewMode::ENTROPY:    return m_entropy.toRGB();
        case ViewMode::HISTOGRAM:  return m_histogram.toRGB();
        case ViewMode::TRIGRAPH3D: return {};
    }
    return {};
}

static void getCanvasTransform(float zoom2d,
                                float scrollH, float scrollV,
                                float& drawX,  float& drawY,
                                float& scale) {
    float scaledSize = 256.0f * zoom2d;
    scale = zoom2d;
    float maxOffX = std::max(0.0f, scaledSize - (float)CANVAS_W);
    float maxOffY = std::max(0.0f, scaledSize - (float)CANVAS_H);
    drawX = (scaledSize <= (float)CANVAS_W)
            ? CANVAS_X + ((float)CANVAS_W - scaledSize) * 0.5f
            : (float)CANVAS_X - scrollH * maxOffX;
    drawY = (scaledSize <= (float)CANVAS_H)
            ? CANVAS_Y + ((float)CANVAS_H - scaledSize) * 0.5f
            : (float)CANVAS_Y - scrollV * maxOffY;
}

void Visualizer::draw2DCanvas() {
    if (m_mode == ViewMode::TRIGRAPH3D) return;

    float drawX, drawY, scale;
    getCanvasTransform(m_zoom2d, m_scrollH, m_scrollV,
                       drawX, drawY, scale);

    float scaledSize  = 256.0f * scale;
    float decalLeft   = drawX;
    float decalTop    = drawY;
    float decalRight  = drawX + scaledSize;
    float decalBottom = drawY + scaledSize;

    float clipLeft   = std::max(decalLeft,   (float)CANVAS_X);
    float clipTop    = std::max(decalTop,    (float)CANVAS_Y);
    float clipRight  = std::min(decalRight,  (float)(CANVAS_X + CANVAS_W));
    float clipBottom = std::min(decalBottom, (float)(CANVAS_Y + CANVAS_H));

    if (clipRight <= clipLeft || clipBottom <= clipTop) return;

    float srcX = (clipLeft   - decalLeft) / scale;
    float srcY = (clipTop    - decalTop)  / scale;
    float srcW = (clipRight  - clipLeft)  / scale;
    float srcH = (clipBottom - clipTop)   / scale;

    DrawPartialDecal(
        {clipLeft,  clipTop},
        {clipRight - clipLeft, clipBottom - clipTop},
        m_decal,
        {srcX, srcY},
        {srcW, srcH}
    );
}

void Visualizer::handle2DInput() {
    if (m_mode == ViewMode::TRIGRAPH3D) return;

    int mx = GetMouseX();
    int my = GetMouseY();
    if (my < TOPBAR_H) return;

    bool inCanvas = (mx >= CANVAS_X && mx < CANVAS_X + CANVAS_W &&
                     my >= CANVAS_Y && my < CANVAS_Y + CANVAS_H);

    if (inCanvas) {
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

    if (m_zoom2d > 1.0f && inCanvas &&
        !m_input.lastClickWasTab()) {
        if (GetMouse(0).bPressed) {
            m_dragging   = true;
            m_lastMouseX = mx;
            m_lastMouseY = my;
        }
    }
    if (GetMouse(0).bReleased) m_dragging = false;

    if (m_dragging && m_zoom2d > 1.0f) {
        float dx      = (float)(mx - m_lastMouseX);
        float dy      = (float)(my - m_lastMouseY);
        float scaled  = 256.0f * m_zoom2d;
        float maxOffX = std::max(1.0f, scaled - (float)CANVAS_W);
        float maxOffY = std::max(1.0f, scaled - (float)CANVAS_H);
        m_scrollH = std::clamp(m_scrollH - dx/maxOffX, 0.0f, 1.0f);
        m_scrollV = std::clamp(m_scrollV - dy/maxOffY, 0.0f, 1.0f);
        m_lastMouseX = mx;
        m_lastMouseY = my;
    }

    if (m_zoom2d > 1.0f && GetMouse(0).bHeld &&
        my >= SCROLLBAR_H_Y && my < SCROLLBAR_H_Y + SCROLLBAR_H) {
        float norm = (float)(mx - 6) / (float)(CANVAS_W - 12);
        m_scrollH  = std::clamp(norm, 0.0f, 1.0f);
    }

    if (m_zoom2d > 1.0f && GetMouse(0).bHeld &&
        mx >= CANVAS_W && mx < CANVAS_W + SCROLLBAR_V_W &&
        my >= CANVAS_Y && my < CANVAS_Y + CANVAS_H) {
        float norm = (float)(my - CANVAS_Y) / (float)CANVAS_H;
        m_scrollV  = std::clamp(norm, 0.0f, 1.0f);
    }
}

std::string Visualizer::getByteInfo(int mouseX, int mouseY) const {
    if (!m_reader.isLoaded()) return "";
    if (m_mode == ViewMode::TRIGRAPH3D) return "";
    if (mouseX < CANVAS_X || mouseX >= CANVAS_X + CANVAS_W) return "";
    if (mouseY < CANVAS_Y || mouseY >= CANVAS_Y + CANVAS_H) return "";

    float drawX, drawY, scale;
    getCanvasTransform(m_zoom2d, m_scrollH, m_scrollV,
                       drawX, drawY, scale);

    float relX = ((float)mouseX - drawX) / scale;
    float relY = ((float)mouseY - drawY) / scale;
    int cx = (int)relX;
    int cy = (int)relY;
    if (cx < 0 || cx > 255 || cy < 0 || cy > 255) return "";

    std::ostringstream oss;
    if (m_mode == ViewMode::DIGRAPH) {
        oss << "COPPIA: 0x"
            << std::uppercase << std::hex
            << std::setw(2) << std::setfill('0') << cx
            << " -> 0x"
            << std::setw(2) << std::setfill('0') << cy;
    } else {
        oss << "POS: (" << std::dec << cx << "," << cy << ")";
    }
    return oss.str();
}
