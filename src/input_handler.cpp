#include "input_handler.h"
#include "view_tabs.h"
#include <algorithm>

Action InputHandler::update(olc::PixelGameEngine* pge,
                             float fElapsedTime) {
    m_cursorBlink += fElapsedTime;
    if (m_inputMode)
        return handleTextInput(pge);
    else
        return handleNormalKeys(pge);
}

Action InputHandler::handleNormalKeys(olc::PixelGameEngine* pge) {
    // --- Click sulle tab ---
    if (pge->GetMouse(0).bPressed && pge->GetMouseY() < TOPBAR_H) {
        int tabIdx = pge->GetMouseX() / TAB_W;
        if (tabIdx >= 0 && tabIdx < VIEW_TAB_COUNT) {
            m_requestedView   = VIEW_TABS[tabIdx].mode;
            m_lastClickWasTab = true;
            return Action::SWITCH_VIEW;
        }
    }

    // Reset flag SOLO se non abbiamo appena cliccato una tab
    if (!pge->GetMouse(0).bPressed)
        m_lastClickWasTab = false;

    // --- Tasti 1-9 ---
    for (int i = 0; i < VIEW_TAB_COUNT; i++)
        if (pge->GetKey((olc::Key)((int)olc::Key::K1 + i)).bPressed) {
            m_requestedView = VIEW_TABS[i].mode;
            return Action::SWITCH_VIEW;
        }

    if (pge->GetKey(olc::Key::O).bPressed) {
        m_inputMode   = true;
        m_inputBuffer = "/";
        m_cursorBlink = 0.0f;
        return Action::NONE;
    }
    if (pge->GetKey(olc::Key::S).bPressed)
        return Action::SAVE_PNG;

    return Action::NONE;
}

Action InputHandler::handleTextInput(olc::PixelGameEngine* pge) {
    bool shift = pge->GetKey(olc::Key::SHIFT).bHeld;
    for (int k = 0; k < 256; k++) {
        olc::Key key = static_cast<olc::Key>(k);
        if (!pge->GetKey(key).bPressed) continue;
        if (key == olc::Key::ENTER)  { m_inputMode = false; return Action::OPEN_FILE; }
        if (key == olc::Key::ESCAPE) { m_inputMode = false; m_inputBuffer = ""; return Action::CANCEL; }
        if (key == olc::Key::BACK)   { if (!m_inputBuffer.empty()) m_inputBuffer.pop_back(); continue; }
        if (key == olc::Key::NP_DIV) { m_inputBuffer += '/'; continue; }
        char c = keyToChar(key, shift);
        if (c != 0) m_inputBuffer += c;
    }
    return Action::NONE;
}

char InputHandler::keyToChar(olc::Key key, bool shift) {
    int k  = static_cast<int>(key);
    int kA = static_cast<int>(olc::Key::A);
    int kZ = static_cast<int>(olc::Key::Z);
    int k0 = static_cast<int>(olc::Key::K0);
    int k9 = static_cast<int>(olc::Key::K9);
    if (k >= kA && k <= kZ) { char c = 'a'+(k-kA); return shift?(char)(c-32):c; }
    if (k >= k0 && k <= k9) return '0'+(k-k0);
    switch (key) {
        case olc::Key::PERIOD: return '.';
        case olc::Key::OEM_2:  return '/';   // tasto '/' (layout US)
        case olc::Key::MINUS:  return shift?'_':'-';
        case olc::Key::SPACE:  return ' ';
        default:               return 0;
    }
}
