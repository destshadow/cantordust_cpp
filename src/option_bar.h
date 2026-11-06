#pragma once
#include "constants.h"
#include <string>

// Una riga di opzioni: tutte le scelte a vista, quella attiva evidenziata,
// il tasto accanto al titolo. Si disegna e si clicca nello stesso passaggio:
//
//   OptionBar bar(this, x, y, clickable);
//   bar.title("Colore [C]:");
//   if (bar.option("Entropy", attiva)) ...   // true = appena cliccata
class OptionBar {
public:
    OptionBar(olc::PixelGameEngine* pge, int x, int y, bool clickable)
        : m_pge(pge), m_x(x), m_y(y), m_clickable(clickable) {}

    void title(const std::string& s) {
        m_pge->DrawString(m_x, m_y + TEXT_Y, s, COL_DIM());
        m_x += (int)s.size() * 8 + 6;
    }

    // Spazio tra due gruppi di opzioni
    void gap() { m_x += 20; }

    bool option(const std::string& s, bool active, bool enabled = true) {
        const int w = (int)s.size() * 8 + 8, x = m_x;
        m_x += w + 2;
        const int mx = m_pge->GetMouseX(), my = m_pge->GetMouseY();
        const bool hover = enabled && m_clickable && mx >= x && mx < x + w &&
                           my >= m_y && my < m_y + H;

        if (active) m_pge->FillRect(x, m_y, w, H, COL_BG_LIGHT());
        olc::Pixel col = active ? COL_ACCENT() : hover ? COL_TEXT() : COL_DIM();
        if (!enabled) col = COL_BORDER();
        m_pge->DrawString(x + 4, m_y + TEXT_Y, s, col);
        return hover && m_pge->GetMouse(0).bPressed;
    }

private:
    static constexpr int H = CMD_STATUS_H - 2, TEXT_Y = 4;
    olc::PixelGameEngine* m_pge;
    int  m_x, m_y;
    bool m_clickable;
};
