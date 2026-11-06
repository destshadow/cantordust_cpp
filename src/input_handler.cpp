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

// I tasti speciali si leggono come tasti, i caratteri dalla cache del motore:
// ha il carattere vero digitato, qualunque sia la tastiera. I tasti di olcPGE
// conoscono pochi simboli: '_', '+' o '(' non si potrebbero scrivere.
Action InputHandler::handleTextInput(olc::PixelGameEngine* pge) {
    if (pge->GetKey(olc::Key::ENTER).bPressed) {
        m_inputMode = false;
        return Action::OPEN_FILE;
    }
    if (pge->GetKey(olc::Key::ESCAPE).bPressed) {
        m_inputMode = false;
        m_inputBuffer = "";
        return Action::CANCEL;
    }
    if (pge->GetKey(olc::Key::BACK).bPressed && !m_inputBuffer.empty())
        m_inputBuffer.pop_back();
    if (pge->GetKey(olc::Key::NP_DIV).bPressed) m_inputBuffer += '/';
    for (int32_t code : pge->GetKeyPressCache())
        if (code >= 0x20 && code < 0x7F) m_inputBuffer += (char)code;
    return Action::NONE;
}
