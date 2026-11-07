#include "visualizer.h"
#include <cstdlib>

// Vista -> file: nelle viste dove un pixel e' un valore, passando sopra
// compaiono sulle barre i segni di dove quel valore compare; con un click si
// sceglie il valore e ogni click successivo passa all'occorrenza dopo.

bool Visualizer::isValueView() const {
    return m_mode == ViewMode::DIGRAPH   || m_mode == ViewMode::HISTOGRAM ||
           m_mode == ViewMode::BYTECLOUD || m_mode == ViewMode::ONETUPLE  ||
           m_mode == ViewMode::TRIGRAPH3D;
}

// Il valore rappresentato dal pixel (cx, cy) di una vista 2D: quanti byte (1 o 2)
size_t Visualizer::valueAt(int cx, int cy, uint8_t pat[3]) const {
    if (m_mode == ViewMode::DIGRAPH) {
        pat[0] = (uint8_t)cx;
        pat[1] = (uint8_t)cy;
        return 2;
    }
    if (m_mode == ViewMode::BYTECLOUD) pat[0] = ByteCloud::valueAt(cx, cy);
    else                               pat[0] = (uint8_t)cx;   // X = valore
    return 1;
}

// Il valore sotto il mouse, in qualunque vista; 0 = nessuno
size_t Visualizer::valueUnderMouse(uint8_t pat[3]) const {
    if (m_mode == ViewMode::TRIGRAPH3D) {
        uint32_t key;
        if (!tripleUnderMouse(key)) return 0;
        pat[0] = (key >> 16) & 0xFF;
        pat[1] = (key >>  8) & 0xFF;
        pat[2] =  key        & 0xFF;
        return 3;
    }
    int cx = 0, cy = 0;
    if (isValueView() && mouseToContent(GetMouseX(), GetMouseY(), cx, cy))
        return valueAt(cx, cy, pat);
    return 0;
}

void Visualizer::rebuildMarks() {
    const auto& b = m_reader.getBytes();
    const uint8_t* p = m_marked.data();
    size_t n = m_marked.size();
    m_markLo = m_bars.macroLo();
    m_markHi = m_bars.macroHi();
    m_bars.setMarks(Occur::rows(b, 0, b.size(), p, n, NavBars::rows()),
                    Occur::rows(b, m_markLo, m_markHi, p, n, NavBars::rows()));
}

void Visualizer::updateLinks(float dt) {
    // Una scelta vale per la finestra e la vista in cui e' stata fatta
    if (m_pick.active() && (!isValueView() ||
                            m_pick.lo != m_navigator.getStart() ||
                            m_pick.hi != m_navigator.getEnd()))
        m_pick.clear();
    if (GetKey(olc::Key::ESCAPE).bPressed) {
        m_pick.clear();
        m_cursorOffset = SIZE_MAX;
        m_stringList.state.selected = ListState::NONE;
    }

    uint8_t pat[3] = {0, 0, 0};
    const size_t n = valueUnderMouse(pat);

    // Click = premuto e rilasciato senza spostarsi (altrimenti e' un pan)
    if (GetMouse(0).bPressed) {
        m_pressed = n > 0;
        m_pressX  = GetMouseX();
        m_pressY  = GetMouseY();
    }
    if (GetMouse(0).bReleased && m_pressed) {
        m_pressed = false;
        if (n > 0 && std::abs(GetMouseX() - m_pressX) <= 2 &&
                     std::abs(GetMouseY() - m_pressY) <= 2)
            m_pick.step(m_reader.getBytes(), m_navigator.getStart(),
                        m_navigator.getEnd(), pat, n);
    }
    m_bars.setCursor(m_pick.active() ? m_pick.target : m_cursorOffset);

    // Segni: il valore sotto il mouse, altrimenti quello scelto.
    // Si aspetta un istante: su file grandi cercare a ogni pixel attraversato
    // bloccherebbe il mouse.
    std::vector<uint8_t> wanted = n > 0 ? std::vector<uint8_t>(pat, pat + n)
                                        : m_pick.pattern;
    if (wanted != m_markWanted) {
        m_markWanted = wanted;
        m_markWait   = 0.0f;
    }
    m_markWait += dt;

    if (wanted.empty()) {
        if (!m_marked.empty()) { m_marked.clear(); m_bars.clearMarks(); }
    } else if (m_markWait >= MARK_WAIT &&
               (wanted != m_marked || m_markLo != m_bars.macroLo() ||
                                      m_markHi != m_bars.macroHi())) {
        m_marked = wanted;
        rebuildMarks();
    }
}
