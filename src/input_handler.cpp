#include "input_handler.h"

// Posizioni tab calcolate dinamicamente
// N=7 tab, FPS_AREA=64px fissi a destra
// tabW = (screenW - FPS_AREA) / N
// Ma InputHandler non conosce la screenW →
// usiamo il metodo GetScreenWidth() di olcPGE
static const ViewMode TAB_MODES[] = {
    ViewMode::DIGRAPH,
    ViewMode::DOTPLOT,
    ViewMode::ENTROPY,
    ViewMode::HISTOGRAM,
    ViewMode::TRIGRAPH3D,
    ViewMode::RAWPIXELS,
    ViewMode::METRICMAP,
};
static constexpr int N_TABS = 7;
static constexpr int FPS_AREA = 80;

Action InputHandler::update(olc::PixelGameEngine* pge,
                             float fElapsedTime) {
    m_cursorBlink += fElapsedTime;
    if (m_inputMode)
        return handleTextInput(pge);
    else
        return handleNormalKeys(pge);
}

Action InputHandler::handleNormalKeys(olc::PixelGameEngine* pge) {
    // Click sulle tab — larghezza calcolata dinamicamente
    if (pge->GetMouse(0).bPressed) {
        int mx  = pge->GetMouseX();
        int my  = pge->GetMouseY();
        if (my >= 0 && my < TOPBAR_H) {
            int sw   = pge->ScreenWidth();
            int tabW = (sw - FPS_AREA) / N_TABS;
            for (int i = 0; i < N_TABS; i++) {
                int tx = i * tabW;
                if (mx >= tx && mx < tx + tabW) {
                    m_requestedView   = TAB_MODES[i];
                    m_lastClickWasTab = true;
                    return Action::SWITCH_VIEW;
                }
            }
        }
    }
    m_lastClickWasTab = false;

    if (pge->GetKey(olc::Key::K1).bPressed) {
        m_requestedView = ViewMode::DIGRAPH;    return Action::SWITCH_VIEW; }
    if (pge->GetKey(olc::Key::K2).bPressed) {
        m_requestedView = ViewMode::DOTPLOT;    return Action::SWITCH_VIEW; }
    if (pge->GetKey(olc::Key::K3).bPressed) {
        m_requestedView = ViewMode::ENTROPY;    return Action::SWITCH_VIEW; }
    if (pge->GetKey(olc::Key::K4).bPressed) {
        m_requestedView = ViewMode::HISTOGRAM;  return Action::SWITCH_VIEW; }
    if (pge->GetKey(olc::Key::K5).bPressed) {
        m_requestedView = ViewMode::TRIGRAPH3D; return Action::SWITCH_VIEW; }
    if (pge->GetKey(olc::Key::K6).bPressed) {
        m_requestedView = ViewMode::RAWPIXELS;  return Action::SWITCH_VIEW; }
    if (pge->GetKey(olc::Key::K7).bPressed) {
        m_requestedView = ViewMode::METRICMAP;  return Action::SWITCH_VIEW; }

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
        char c = keyToChar(pge, key, shift);
        if (c != 0) m_inputBuffer += c;
    }
    return Action::NONE;
}

char InputHandler::keyToChar(olc::PixelGameEngine* pge,
                              olc::Key key, bool shift) {
    int k  = static_cast<int>(key);
    int kA = static_cast<int>(olc::Key::A);
    int kZ = static_cast<int>(olc::Key::Z);
    int k0 = static_cast<int>(olc::Key::K0);
    int k9 = static_cast<int>(olc::Key::K9);
    if (k >= kA && k <= kZ) { char c = 'a'+(k-kA); return shift?(char)(c-32):c; }
    if (k >= k0 && k <= k9) return '0'+(k-k0);
    switch (key) {
        case olc::Key::PERIOD: return '.';
        case olc::Key::MINUS:  return shift?'_':'-';
        case olc::Key::SPACE:  return ' ';
        default:               return 0;
    }
}
