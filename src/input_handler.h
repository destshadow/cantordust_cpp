#pragma once
#include <string>
#include "constants.h"
#include "view_mode.h"

enum class Action {
    NONE,
    SWITCH_VIEW,
    OPEN_FILE,
    SAVE_PNG,
    CANCEL
};

class InputHandler {
public:
    Action update(olc::PixelGameEngine* pge, float fElapsedTime);

    ViewMode    getRequestedView()   const { return m_requestedView; }
    std::string getRequestedPath()   const { return m_inputBuffer; }
    bool        isInputMode()        const { return m_inputMode; }
    std::string getInputBuffer()     const { return m_inputBuffer; }
    float       getCursorBlink()     const { return m_cursorBlink; }
    bool        lastClickWasTab()    const { return m_lastClickWasTab; }

private:
    Action handleNormalKeys(olc::PixelGameEngine* pge);
    Action handleTextInput(olc::PixelGameEngine* pge);
    char   keyToChar(olc::Key key, bool shift);

    bool        m_inputMode         = false;
    std::string m_inputBuffer;
    float       m_cursorBlink       = 0.0f;
    ViewMode    m_requestedView     = ViewMode::DIGRAPH;
    bool        m_lastClickWasTab   = false;
};
