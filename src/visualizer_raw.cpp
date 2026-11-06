#include "visualizer.h"
#include <algorithm>

// Pannello in fondo alla vista RawPixels:
//
//   AUTOCORR  suggerita W=317 [A]              (hover: W=...)
//   ▁▂▁▁▃▁▁▁▁▁▁█▁▁▁▁▁▁▁▁▁▁▅▁▁▁▁   ← somiglianza per larghezza; click = usa
//   WIDTH 317 [<] ───●─── [>]      OFFSET 0 [<] ●────── [>]

namespace {
constexpr int PANEL_PAD = 8;
constexpr int GRAPH_H   = 40;

struct PanelLayout {
    int py;            // inizio pannello
    int gx, gy, gw;    // grafico
    int sy;            // riga slider
};

PanelLayout panelLayout() {
    PanelLayout L;
    L.py = CANVAS_Y + CANVAS_H - RAW_PANEL_H;
    L.gx = CANVAS_X + PANEL_PAD;
    L.gy = L.py + 16;
    L.gw = CANVAS_W - 2 * PANEL_PAD;
    L.sy = L.gy + GRAPH_H + 10;
    return L;
}
} // namespace

// Larghezza massima mostrata nel grafico per il bpp corrente
static int graphMaxWidth(const Autocorr& ac, BppMode bpp) {
    int byLag = ac.maxLag() / (int)RawPixels::bytesPerPixel(bpp);
    return std::min(RawPixels::MAX_WIDTH, std::max(0, byLag));
}

void Visualizer::syncRawSliders() {
    PanelLayout L = panelLayout();
    int half = CANVAS_W / 2;
    int x0   = CANVAS_X;

    m_widthSlider.label = "WIDTH";
    m_widthSlider.x = x0 + PANEL_PAD;
    m_widthSlider.y = L.sy;
    m_widthSlider.w = half - 2 * PANEL_PAD;
    m_widthSlider.setRange(RawPixels::MIN_WIDTH, RawPixels::MAX_WIDTH);
    if (!m_widthSlider.dragging) m_widthSlider.value = m_rawWidth;

    // Offset: abbastanza per allineare canali e spostare l'inizio riga
    int rowBytes = m_rawWidth * (int)RawPixels::bytesPerPixel(m_bppMode);
    m_rawOffset = std::clamp(m_rawOffset, 0, std::max(0, rowBytes - 1));
    m_offsetSlider.label = "OFFSET";
    m_offsetSlider.x = x0 + half + PANEL_PAD;
    m_offsetSlider.y = L.sy;
    m_offsetSlider.w = half - 2 * PANEL_PAD;
    m_offsetSlider.setRange(0, std::max(0, rowBytes - 1));
    if (!m_offsetSlider.dragging) m_offsetSlider.value = m_rawOffset;
}

// Ricalcolo immediato (thread UI) dopo un cambio di larghezza/offset/bpp:
// e' veloce perche' l'immagine e' al massimo MAX_WIDTH x MAX_ROWS.
void Visualizer::recomputeRawPixels() {
    m_rawpixels.compute(m_reader.getBytes(),
                        m_navigator.getStart(), m_navigator.getEnd(),
                        m_bppMode, m_rawWidth, (size_t)m_rawOffset, m_rawFilter);
    updateCanvas();
}

void Visualizer::handleRawInput() {
    syncRawSliders();
    bool changed = false;

    if (m_widthSlider.update(this)) {
        m_rawWidth = m_widthSlider.value;
        changed = true;
    }
    if (m_offsetSlider.update(this)) {
        m_rawOffset = m_offsetSlider.value;
        changed = true;
    }

    // Tastiera: , .  = larghezza ±1   Ctrl+, .  = offset ±1   Shift = ×10
    int step = GetKey(olc::Key::SHIFT).bHeld ? 10 : 1;
    bool ctrl = GetKey(olc::Key::CTRL).bHeld;
    int delta = 0;
    if (GetKey(olc::Key::COMMA).bPressed)  delta -= step;
    if (GetKey(olc::Key::PERIOD).bPressed) delta += step;
    if (delta != 0) {
        if (ctrl) m_rawOffset = std::max(0, m_rawOffset + delta);
        else      m_rawWidth  = std::clamp(m_rawWidth + delta,
                                           RawPixels::MIN_WIDTH,
                                           RawPixels::MAX_WIDTH);
        changed = true;
    }

    // A = applica la larghezza suggerita dall'autocorrelazione
    if (GetKey(olc::Key::A).bPressed && m_suggestWidth > 0) {
        m_rawWidth = m_suggestWidth;
        changed = true;
    }

    // F = filtro successivo, Shift+F = precedente
    if (GetKey(olc::Key::F).bPressed) {
        m_rawFilter = GetKey(olc::Key::SHIFT).bHeld ? m_rawFilter.prev() : m_rawFilter.next();
        changed = true;
    }

    // Click sul grafico = usa quella larghezza
    PanelLayout L = panelLayout();
    int mx = GetMouseX(), my = GetMouseY();
    int maxW = graphMaxWidth(m_autocorr, m_bppMode);
    if (GetMouse(0).bPressed && maxW > 0 &&
        mx >= L.gx && mx < L.gx + L.gw &&
        my >= L.gy && my < L.gy + GRAPH_H) {
        m_rawWidth = std::clamp(1 + (mx - L.gx) * maxW / L.gw,
                                RawPixels::MIN_WIDTH, RawPixels::MAX_WIDTH);
        changed = true;
    }

    if (changed) {
        syncRawSliders();   // riallinea l'offset alla nuova larghezza
        recomputeRawPixels();
        m_status = "Filtro: " + m_rawFilter.name();
    }
}

void Visualizer::drawRawPanel() {
    syncRawSliders();   // geometria/valori aggiornati anche durante i calcoli
    PanelLayout L = panelLayout();
    FillRect(CANVAS_X, L.py, CANVAS_W, RAW_PANEL_H, COL_BG());
    DrawLine(CANVAS_X, L.py, CANVAS_X + CANVAS_W, L.py, COL_BORDER());

    int mx = GetMouseX(), my = GetMouseY();
    int maxW = graphMaxWidth(m_autocorr, m_bppMode);
    size_t bpp = RawPixels::bytesPerPixel(m_bppMode);

    // --- Titolo ---
    std::string title = "AUTOCORR  ";
    if (m_suggestWidth > 0)
        title += "suggerita W=" + std::to_string(m_suggestWidth) + " [A]";
    else
        title += "nessuna larghezza evidente";
    DrawString(L.gx, L.py + 5, title, COL_CYAN());

    bool hoverGraph = maxW > 0 && mx >= L.gx && mx < L.gx + L.gw &&
                      my >= L.gy && my < L.gy + GRAPH_H;
    if (hoverGraph) {
        int w = 1 + (mx - L.gx) * maxW / L.gw;
        std::string h = "click: W=" + std::to_string(w);
        DrawString(L.gx + L.gw - (int)h.size() * 8, L.py + 5, h, COL_YELLOW());
    }

    // --- Grafico: una colonna per gruppo di larghezze, altezza = max ---
    FillRect(L.gx, L.gy, L.gw, GRAPH_H, olc::Pixel(8, 8, 24));
    const auto& sim = m_autocorr.similarity();
    if (maxW > 0) {
        for (int c = 0; c < L.gw; c++) {
            int w0 = 1 + c * maxW / L.gw;
            int w1 = std::max(w0, (c + 1) * maxW / L.gw);
            float v = 0.0f;
            for (int w = w0; w <= w1 && (size_t)w * bpp < sim.size(); w++)
                v = std::max(v, sim[(size_t)w * bpp]);
            int bh = (int)(v * (GRAPH_H - 2));
            if (bh > 0)
                DrawLine(L.gx + c, L.gy + GRAPH_H - 1,
                         L.gx + c, L.gy + GRAPH_H - 1 - bh,
                         olc::Pixel(0, 90 + (int)(v * 165), 0));
        }
        // Marcatori: larghezza corrente (giallo) e suggerita (magenta)
        auto markX = [&](int w) { return L.gx + (w - 1) * L.gw / maxW; };
        if (m_suggestWidth > 0 && m_suggestWidth <= maxW) {
            int x = markX(m_suggestWidth);
            DrawLine(x, L.gy, x, L.gy + GRAPH_H - 1, olc::Pixel(255, 80, 255));
        }
        if (m_rawWidth <= maxW) {
            int x = markX(m_rawWidth);
            DrawLine(x, L.gy, x, L.gy + GRAPH_H - 1, COL_YELLOW());
        }
    }
    DrawRect(L.gx, L.gy, L.gw - 1, GRAPH_H - 1, COL_BORDER());

    // --- Slider ---
    m_widthSlider.draw(this);
    m_offsetSlider.draw(this);
}
