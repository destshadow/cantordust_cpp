#include "visualizer.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

// Copia l'RGB della vista corrente nello sprite giusto.
// Le viste classiche usano il canvas 256x256; i raw pixel hanno uno
// sprite proprio, ricreato quando cambia la dimensione.
void Visualizer::updateCanvas() {
    if (m_mode == ViewMode::TRIGRAPH3D) return;

    if (m_mode == ViewMode::RAWPIXELS) {
        int w = m_rawpixels.getWidth(), h = m_rawpixels.getHeight();
        if (w <= 0 || h <= 0) return;
        if (!m_rawSprite || m_rawSprite->width != w || m_rawSprite->height != h) {
            delete m_rawDecal;  m_rawDecal  = nullptr;
            delete m_rawSprite;
            m_rawSprite = new olc::Sprite(w, h);
            m_rawDecal  = new olc::Decal(m_rawSprite);
        }
        const auto& rgb = m_rawpixels.getRGB();
        olc::Pixel* dst = m_rawSprite->GetData();
        for (size_t i = 0, n = (size_t)w * h; i < n; i++)
            dst[i] = olc::Pixel(rgb[i*3], rgb[i*3 + 1], rgb[i*3 + 2]);
        m_rawDecal->Update();
        return;
    }

    auto rgb = getCurrentRGB();
    if (rgb.size() < 256 * 256 * 3) return;
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
        case ViewMode::RAWPIXELS:  return m_rawpixels.toRGB();
        case ViewMode::METRICMAP:  return m_metricmap.getRGB();
        case ViewMode::BYTECLOUD:  return m_bytecloud.toRGB();
        case ViewMode::ONETUPLE:   return m_onetuple.toRGB();
    }
    return {};
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

void Visualizer::draw2DCanvas() {
    if (m_mode == ViewMode::TRIGRAPH3D) return;

    olc::Decal* decal = m_decal;
    if (m_mode == ViewMode::RAWPIXELS) {
        if (!m_rawDecal || m_rawpixels.getWidth() <= 0) return;
        decal = m_rawDecal;
    }

    int ax, ay, aw, ah, cw, ch;
    viewArea(ax, ay, aw, ah);
    contentSize(cw, ch);
    float drawX, drawY, scale;
    canvasTransform(drawX, drawY, scale);

    float clipLeft   = std::max(drawX,              (float)ax);
    float clipTop    = std::max(drawY,              (float)ay);
    float clipRight  = std::min(drawX + cw * scale, (float)(ax + aw));
    float clipBottom = std::min(drawY + ch * scale, (float)(ay + ah));

    if (clipRight <= clipLeft || clipBottom <= clipTop) return;

    DrawPartialDecal(
        {clipLeft, clipTop},
        {clipRight - clipLeft, clipBottom - clipTop},
        decal,
        {(clipLeft - drawX) / scale, (clipTop - drawY) / scale},
        {(clipRight - clipLeft) / scale, (clipBottom - clipTop) / scale}
    );
}

void Visualizer::handle2DInput() {
    if (m_mode == ViewMode::TRIGRAPH3D) return;

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
    // NON iniziare drag se siamo in hover su una sezione
    bool onSection = (m_hoveredSection >= 0);
    if (m_zoom2d > 1.0f && inArea &&
        !m_input.lastClickWasTab() && !onSection) {
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

    // Scrollbar H canvas
    if (m_zoom2d > 1.0f && GetMouse(0).bHeld &&
        my >= SCROLLBAR_H_Y && my < SCROLLBAR_H_Y + SCROLLBAR_H &&
        mx >= CANVAS_X && mx < CANVAS_X + CANVAS_W)
        m_scrollH = std::clamp((float)(mx-CANVAS_X)/(float)CANVAS_W,
                                0.0f, 1.0f);

    // Scrollbar V canvas
    if (m_zoom2d > 1.0f && GetMouse(0).bHeld &&
        mx >= SCROLLBAR_V_X && mx < SCROLLBAR_V_X + SCROLLBAR_V_W &&
        my >= ay && my < ay + ah)
        m_scrollV = std::clamp((float)(my-ay)/(float)ah, 0.0f, 1.0f);
}

std::string Visualizer::getByteInfo(int mouseX, int mouseY) const {
    if (!m_reader.isLoaded()) return "";
    if (m_mode == ViewMode::TRIGRAPH3D) return "";

    int ax, ay, aw, ah, cw, ch;
    viewArea(ax, ay, aw, ah);
    contentSize(cw, ch);
    if (mouseX < ax || mouseX >= ax + aw) return "";
    if (mouseY < ay || mouseY >= ay + ah) return "";

    float drawX, drawY, scale;
    canvasTransform(drawX, drawY, scale);

    float fx = ((float)mouseX - drawX) / scale;
    float fy = ((float)mouseY - drawY) / scale;
    if (fx < 0 || fy < 0) return "";
    int cx = (int)fx, cy = (int)fy;
    if (cx >= cw || cy >= ch) return "";

    std::ostringstream oss;
    if (m_mode == ViewMode::DIGRAPH) {
        oss << "COPPIA: 0x" << std::uppercase << std::hex
            << std::setw(2) << std::setfill('0') << cx
            << " -> 0x" << std::setw(2) << std::setfill('0') << cy;
    } else if (m_mode == ViewMode::METRICMAP) {
        // fileIndexAt e' relativo alla finestra: aggiungiamo l'inizio
        size_t fidx = m_navigator.getStart() +
                      m_metricmap.fileIndexAt(cx, cy);
        oss << "FILE IDX: 0x" << std::uppercase << std::hex
            << std::setw(8) << std::setfill('0') << fidx;
    } else if (m_mode == ViewMode::RAWPIXELS) {
        size_t rel = m_rawpixels.byteOffsetAt(cx, cy);
        if (rel == SIZE_MAX) return "";
        oss << "OFF: 0x" << std::uppercase << std::hex
            << std::setw(8) << std::setfill('0')
            << (m_navigator.getStart() + rel)
            << std::dec << "  (" << cx << "," << cy << ")";
    } else {
        oss << "POS: (" << std::dec << cx << "," << cy << ")";
    }
    return oss.str();
}
