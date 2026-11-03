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
        const olc::Pixel track(200, 200, 210), knob(40, 150, 170);

        pge->DrawString(x, y + 3, label + " " + std::to_string(value),
                        olc::Pixel(200, 200, 200));

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
