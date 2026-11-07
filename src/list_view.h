#pragma once
#include "constants.h"
#include "list_state.h"
#include <functional>

// Lista scorrevole, con la barra a destra.
// Mouse: rotella = scorre, click = sceglie, trascina la barra = scorre.
// Tastiera: frecce e PagSu/PagGiu spostano la scelta.
struct ListView {
    static constexpr int ROW_H = 10, BAR_W = 10, WHEEL_ROWS = 3;

    int x = 0, y = 0, w = 0, h = 0;
    ListState state;
    size_t hover = ListState::NONE;   // riga sotto il mouse
    bool dragging = false;

    void setArea(int ax, int ay, int aw, int ah) {
        x = ax;  y = ay;  w = aw;  h = ah;
        state.visible = std::max(1, h / ROW_H);
    }
    int rowsW() const { return w - BAR_W; }   // larghezza senza la barra

    // true se la scelta e' cambiata
    bool update(olc::PixelGameEngine* pge) {
        const size_t before = state.selected;
        const int mx = pge->GetMouseX(), my = pge->GetMouseY();
        const bool inside = mx >= x && mx < x + w && my >= y && my < y + h;
        const bool inRows = inside && mx < x + rowsW();
        const auto& lmb = pge->GetMouse(0);

        if (inside && pge->GetMouseWheel() > 0) state.scrollBy(-WHEEL_ROWS);
        if (inside && pge->GetMouseWheel() < 0) state.scrollBy(WHEEL_ROWS);

        if (lmb.bPressed && inside && !inRows) dragging = true;
        if (!lmb.bHeld) dragging = false;
        if (dragging) state.scrollToTrack(my - y, h);

        hover = ListState::NONE;
        if (inRows) {
            const size_t row = state.top + (my - y) / ROW_H;
            if (row < state.count) hover = row;
        }
        if (lmb.bPressed && hover != ListState::NONE) state.select(hover);

        const int page = (int)state.visible - 1;
        if (pge->GetKey(olc::Key::UP).bPressed)   state.moveBy(-1);
        if (pge->GetKey(olc::Key::DOWN).bPressed) state.moveBy(1);
        if (pge->GetKey(olc::Key::PGUP).bPressed) state.moveBy(-page);
        if (pge->GetKey(olc::Key::PGDN).bPressed) state.moveBy(page);
        return state.selected != before;
    }

    // drawRow(riga, y) disegna il contenuto; lo sfondo lo mette la lista
    void draw(olc::PixelGameEngine* pge,
              const std::function<void(size_t, int)>& drawRow) const {
        const size_t last = std::min(state.count, state.top + state.visible);
        for (size_t r = state.top; r < last; r++) {
            const int ry = y + (int)(r - state.top) * ROW_H;
            if (r == state.selected)
                pge->FillRect(x, ry - 1, rowsW(), ROW_H, COL_BG_LIGHT());
            drawRow(r, ry);
        }
        const int bx = x + w - BAR_W;
        pge->FillRect(bx, y, BAR_W, h, COL_SCROLLBAR());
        if (state.count > state.visible)
            pge->FillRect(bx + 1, y + state.thumbY(h), BAR_W - 2, state.thumbH(h), COL_CURSOR());
    }
};
