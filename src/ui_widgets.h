#pragma once
#include "constants.h"
#include <algorithm>
#include <string>
#include <cmath>

// Slider orizzontale stile CantorDust:  LABEL [<] ────●──── [>]
// Click su [<] / [>] = -1 / +1 (con Shift = ±10), click o trascinamento
// sulla traccia = salto diretto al valore.
struct Slider {
    static constexpr int H      = 14;
    static constexpr int BTN    = 14;
    static constexpr int LABEL_W = 96;

    int x = 0, y = 0, w = 200;       // area totale, label compresa
    int minV = 0, maxV = 100, value = 0;
    std::string label;
    bool dragging = false;

    int trackX() const { return x + LABEL_W + BTN + 4; }
    int trackW() const { return std::max(1, w - LABEL_W - 2 * (BTN + 4)); }
    bool contains(int mx, int my) const {
        return mx >= x && mx < x + w && my >= y && my < y + H;
    }

    void setRange(int lo, int hi) {
        minV = lo; maxV = std::max(lo, hi);
        value = std::clamp(value, minV, maxV);
    }

    // Ritorna true se il valore e' cambiato
    bool update(olc::PixelGameEngine* pge) {
        int mx = pge->GetMouseX(), my = pge->GetMouseY();
        int old = value;
        int step = pge->GetKey(olc::Key::SHIFT).bHeld ? 10 : 1;
        const auto& lmb = pge->GetMouse(0);

        if (lmb.bPressed && contains(mx, my)) {
            int bl = x + LABEL_W;                 // pulsante [<]
            int br = trackX() + trackW() + 4;     // pulsante [>]
            if (mx >= bl && mx < bl + BTN)        value -= step;
            else if (mx >= br && mx < br + BTN)   value += step;
            else if (mx >= trackX() - 2 && mx <= trackX() + trackW() + 2)
                dragging = true;
        }
        if (!lmb.bHeld) dragging = false;
        if (dragging) {
            float t = (float)(mx - trackX()) / (float)trackW();
            t = std::clamp(t, 0.0f, 1.0f);
            value = minV + (int)std::lround(t * (maxV - minV));
        }
        value = std::clamp(value, minV, maxV);
        return value != old;
    }

    void draw(olc::PixelGameEngine* pge) const {
        const olc::Pixel btnBg(220, 215, 235), btnFg(60, 60, 90);
        const olc::Pixel track = COL_CURSOR(), knob = COL_ACCENT();

        pge->DrawString(x, y + 3, label + " " + std::to_string(value),
                        COL_TEXT());

        int bl = x + LABEL_W, br = trackX() + trackW() + 4;
        pge->FillRect(bl, y, BTN, H, btnBg);
        pge->DrawString(bl + 3, y + 3, "<", btnFg);
        pge->FillRect(br, y, BTN, H, btnBg);
        pge->DrawString(br + 3, y + 3, ">", btnFg);

        int ty = y + H / 2;
        pge->DrawLine(trackX(), ty, trackX() + trackW(), ty, track);
        float t = (maxV > minV) ? (float)(value - minV) / (maxV - minV) : 0.0f;
        int kx = trackX() + (int)(t * trackW());
        pge->FillRect(kx - 2, y + 1, 5, H - 2, knob);
    }
};

// Pannello con bordo e titolo; restituisce l'area interna utile.
inline void drawPanel(olc::PixelGameEngine* pge, int x, int y, int w, int h,
                      const std::string& title) {
    pge->FillRect(x, y, w, h, COL_BG());
    pge->DrawRect(x, y, w - 1, h - 1, COL_BORDER());
    if (!title.empty()) {
        pge->FillRect(x + 1, y + 1, w - 2, 12, COL_BG_LIGHT());
        pge->DrawString(x + 6, y + 3, title, COL_TEXT());
    }
}

// Scrive righe di testo una sotto l'altra dentro un pannello
// (font 8x8 di olcPGE). Tronca le righe troppo lunghe.
struct PanelWriter {
    olc::PixelGameEngine* pge;
    int x, y, w;                 // area interna
    int lineH = 10;

    int maxChars() const { return std::max(0, w / 8); }

    void text(const std::string& s, olc::Pixel col = COL_TEXT()) {
        std::string t = (int)s.size() > maxChars() ? s.substr(0, maxChars()) : s;
        pge->DrawString(x, y, t, col);
        y += lineH;
    }
    // "ETICHETTA  valore" con l'etichetta in grigio
    void field(const std::string& label, const std::string& value,
               olc::Pixel col = COL_TEXT()) {
        pge->DrawString(x, y, label, COL_DIM());
        int vx = x + 9 * 8;
        int room = std::max(0, (x + w - vx) / 8);
        pge->DrawString(vx, y, value.substr(0, room), col);
        y += lineH;
    }
    void gap(int px = 4) { y += px; }
    void separator(const std::string& title = "") {
        y += 3;
        pge->DrawLine(x, y + 3, x + w, y + 3, COL_BORDER());
        if (!title.empty()) {
            int tw = (int)title.size() * 8 + 8;
            pge->FillRect(x + 8, y - 1, tw, 9, COL_BG());
            pge->DrawString(x + 12, y, title, COL_DIM());
        }
        y += 12;
    }
    // Barra orizzontale di percentuale con etichetta
    void bar(const std::string& label, float frac, olc::Pixel col) {
        frac = std::clamp(frac, 0.0f, 1.0f);
        pge->DrawString(x, y, label, COL_TEXT());
        int bx = x + 12 * 8, bw = w - 12 * 8 - 5 * 8;
        pge->FillRect(bx, y + 1, bw, 6, COL_BG_LIGHT());
        pge->FillRect(bx, y + 1, (int)(bw * frac), 6, col);
        std::string pct = std::to_string((int)std::lround(frac * 100)) + "%";
        pge->DrawString(x + w - (int)pct.size() * 8, y, pct, COL_TEXT());
        y += lineH;
    }
};
