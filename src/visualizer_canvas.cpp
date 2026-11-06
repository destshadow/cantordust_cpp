#include "visualizer.h"
#include <algorithm>

// Le viste 2D nello sprite e sullo schermo, e il salvataggio in PNG.

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
        applyHighlight(m_rawSprite);
        m_rawDecal->Update();
        return;
    }

    auto rgb = getCurrentRGB();
    if (rgb.size() < 256 * 256 * 3) {     // vista vuota (es. file vuoto)
        std::fill_n(m_canvas->GetData(), 256 * 256, olc::BLACK);
        m_decal->Update();
        return;
    }
    for (int y = 0; y < 256; y++)
        for (int x = 0; x < 256; x++) {
            int idx = (y * 256 + x) * 3;
            m_canvas->SetPixel(x, y, olc::Pixel(
                rgb[idx], rgb[idx+1], rgb[idx+2]
            ));
        }
    applyHighlight(m_canvas);
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

// Salva la vista corrente come PNG, alla sua dimensione reale
void Visualizer::saveView() {
    if (m_mode == ViewMode::TRIGRAPH3D) return;
    if (m_computing) { m_status = "Calcolo in corso: riprova tra un attimo"; return; }
    int w = 256, h = 256;
    contentSize(w, h);
    std::vector<uint8_t> rgb = getCurrentRGB();
    if (rgb.size() < (size_t)w * h * 3) {   // es. file vuoto
        m_status = "Niente da salvare in questa vista";
        return;
    }
    std::string fname = "export_" + std::to_string((int)m_mode) + ".png";
    Screenshot::save(fname, rgb, w, h);
    m_status = "Salvato: " + fname;
}
