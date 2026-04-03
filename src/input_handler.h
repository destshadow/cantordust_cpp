#pragma once
#include <string>
#include "constants.h"

enum class Action {
    NONE,
    SWITCH_VIEW,
    OPEN_FILE,
    SAVE_PNG,
    CANCEL
};

enum class ViewMode {
    DIGRAPH    = 0,
    DOTPLOT    = 1,
    ENTROPY    = 2,
    HISTOGRAM  = 3,
    TRIGRAPH3D = 4,
    RAWPIXELS  = 5,
    METRICMAP  = 6,
    BYTECLOUD  = 7,
    ONETUPLE   = 8
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
    int         getTabScrollOffset() const { return m_tabScrollOffset; }

private:
    Action handleNormalKeys(olc::PixelGameEngine* pge);
    Action handleTextInput(olc::PixelGameEngine* pge);
    char   keyToChar(olc::PixelGameEngine* pge, olc::Key key, bool shift);

    bool        m_inputMode         = false;
    std::string m_inputBuffer;
    float       m_cursorBlink       = 0.0f;
    ViewMode    m_requestedView     = ViewMode::DIGRAPH;
    bool        m_lastClickWasTab   = false;
    int         m_tabScrollOffset   = 0;
    bool        m_tabScrollDragging = false;
    int         m_lastMouseX        = 0;
};
