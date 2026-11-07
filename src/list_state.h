#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>

// Lo stato di una lista scorrevole: quante righe, da quale si parte, quale e'
// scelta. Solo logica, nessuna grafica, cosi' si puo' testare da sola.
struct ListState {
    static constexpr size_t NONE = SIZE_MAX;

    size_t count    = 0;
    size_t top      = 0;      // prima riga visibile
    size_t selected = NONE;
    size_t visible  = 1;      // righe che stanno nell'area

    size_t maxTop() const { return count > visible ? count - visible : 0; }

    void setCount(size_t n) {
        count = n;
        top = 0;
        selected = NONE;
    }

    void scrollTo(size_t row) { top = std::min(row, maxTop()); }

    void scrollBy(int rows) {
        if (rows < 0) top -= std::min(top, (size_t)-rows);
        else          top = std::min(top + rows, maxTop());
    }

    void select(size_t row) {
        if (count == 0) return;
        selected = std::min(row, count - 1);
        if (selected < top) top = selected;
        else if (selected >= top + visible) top = selected - visible + 1;
    }

    // Con la tastiera: da nessuna scelta si parte dalla prima riga visibile
    void moveBy(int rows) {
        if (count == 0) return;
        if (selected == NONE) { select(top); return; }
        if (rows < 0) select(selected - std::min(selected, (size_t)-rows));
        else          select(selected + rows);
    }

    // Il cursore della barra di scorrimento, in un binario alto trackH pixel
    int thumbH(int trackH) const {
        if (count <= visible) return trackH;
        return std::max(8, (int)((double)trackH * visible / count));
    }
    int thumbY(int trackH) const {
        const size_t m = maxTop();
        return m ? (int)((double)(trackH - thumbH(trackH)) * top / m) : 0;
    }
    // Trascinamento: y e' il centro voluto del cursore, dentro il binario
    void scrollToTrack(int y, int trackH) {
        const int room = trackH - thumbH(trackH);
        if (room <= 0) return;
        const double t = std::clamp((y - thumbH(trackH) / 2.0) / room, 0.0, 1.0);
        top = (size_t)(t * maxTop() + 0.5);
    }
};
