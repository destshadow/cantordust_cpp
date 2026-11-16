#pragma once
#include "constants.h"
#include "menu_state.h"
#include <string>
#include <vector>

// Menu' a comparsa: un elenco di voci sopra tutto il resto.
//
// Mentre e' aperto il resto dell'interfaccia non riceve mouse ne' tasti (lo
// garantisce Visualizer::OnUserUpdate, non questo file): cosi' un click su una
// voce non arriva anche a cio' che sta sotto.
//
// Si disegna con i decal, per ultimo nel frame: i decal delle viste coprono
// tutto cio' che e' disegnato con i pixel normali.
//
// Mouse: click su una voce = scelta, click fuori = chiude.
// Tastiera: frecce, Invio = scelta, Esc = chiude.
class PopupMenu {
public:
    static constexpr int NONE = -1;

    struct Item {
        std::string label;
        int  id     = 0;        // restituito quando la voce viene scelta
        bool active = false;    // la voce corrente, evidenziata
    };

    bool isOpen() const { return m_open; }
    void close() { m_open = false; }

    void open(int x, int y, std::vector<Item> items) {
        m_items = std::move(items);
        m_x = x;  m_y = y;
        m_state.count = (int)m_items.size();
        m_state.hover = MenuState::NONE;
        m_width = 0;
        for (const Item& it : m_items)
            m_width = std::max(m_width, (int)it.label.size() * 8 + 16);
        m_open = !m_items.empty();
    }

    // L'id della voce scelta in questo frame, altrimenti NONE.
    // Una scelta, un click fuori o Esc chiudono il menu'.
    int update(olc::PixelGameEngine* pge) {
        if (!m_open) return NONE;
        const int mx = pge->GetMouseX() - m_x, my = pge->GetMouseY() - m_y;
        const int row = m_state.rowAt(mx, my, m_width);
        if (row != MenuState::NONE) m_state.hover = row;

        if (pge->GetKey(olc::Key::UP).bPressed)   m_state.move(-1);
        if (pge->GetKey(olc::Key::DOWN).bPressed) m_state.move(1);

        int chosen = MenuState::NONE;
        if (pge->GetMouse(0).bPressed) {
            chosen = row;
            m_open = false;                 // su una voce o fuori: si chiude
        } else if (pge->GetKey(olc::Key::ENTER).bPressed && m_state.hover != MenuState::NONE) {
            chosen = m_state.hover;
            m_open = false;
        } else if (pge->GetKey(olc::Key::ESCAPE).bPressed) {
            m_open = false;
        }
        return chosen == MenuState::NONE ? NONE : m_items[(size_t)chosen].id;
    }

    void draw(olc::PixelGameEngine* pge) const {
        if (!m_open) return;
        const float x = (float)m_x, y = (float)m_y;
        const float w = (float)m_width, h = (float)m_state.height();
        pge->FillRectDecal({x - 1, y - 1}, {w + 2, h + 2}, COL_BORDER());
        pge->FillRectDecal({x, y}, {w, h}, COL_BG());
        for (int i = 0; i < m_state.count; i++) {
            const float ry = y + MenuState::PAD + (float)(i * MenuState::ROW_H);
            const Item& it = m_items[(size_t)i];
            if (i == m_state.hover)
                pge->FillRectDecal({x + 1, ry}, {w - 2, (float)MenuState::ROW_H}, COL_BG_LIGHT());
            const olc::Pixel col = it.active ? COL_ACCENT()
                                 : i == m_state.hover ? COL_TEXT() : COL_DIM();
            pge->DrawStringDecal({x + 8, ry + 3}, it.label, col);
        }
    }

private:
    std::vector<Item> m_items;
    MenuState m_state;
    int  m_x = 0, m_y = 0, m_width = 0;
    bool m_open = false;
};
