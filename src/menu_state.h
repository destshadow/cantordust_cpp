#pragma once
#include <algorithm>

// Lo stato di un menu' a comparsa: quante voci, quale e' sotto il mouse o
// scelta con le frecce. Solo conti, nessuna grafica, cosi' si prova nei test.
struct MenuState {
    static constexpr int ROW_H = 14, PAD = 4;
    static constexpr int NONE = -1;

    int count = 0;
    int hover = NONE;

    int height() const { return count * ROW_H + PAD * 2; }

    // La voce alla coordinata (x, y) contate dall'angolo del menu' largo w;
    // NONE se il punto e' fuori dalle voci
    int rowAt(int x, int y, int w) const {
        if (x < 0 || x >= w || y < PAD) return NONE;
        const int row = (y - PAD) / ROW_H;
        return row < count ? row : NONE;
    }

    bool contains(int x, int y, int w) const {
        return x >= 0 && x < w && y >= 0 && y < height();
    }

    // Frecce: dalla prima se non c'e' ancora una voce, e si ferma ai bordi
    void move(int delta) {
        if (count == 0) return;
        hover = hover == NONE ? (delta > 0 ? 0 : count - 1)
                              : std::clamp(hover + delta, 0, count - 1);
    }
};
