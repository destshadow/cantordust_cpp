#include "visualizer.h"

Visualizer::Visualizer(const std::string& filepath)
    : m_initPath(filepath) {
    sAppName = "CantorDust++";
}

bool Visualizer::OnUserCreate() {
    m_canvas = new olc::Sprite(256, 256);
    m_decal  = new olc::Decal(m_canvas);
    loadFile(m_initPath.empty() ? "/bin/ls" : m_initPath);
    return true;
}

bool Visualizer::OnUserUpdate(float fElapsedTime) {

    // --- 1. Input ---
    Action action = m_input.update(this, fElapsedTime);

    // --- 2. Azioni ---
    switch (action) {
        case Action::SWITCH_VIEW:
            m_mode  = m_input.getRequestedView();
            m_dirty = true;
            m_status = "";
            break;
        case Action::OPEN_FILE:
            loadFile(m_input.getRequestedPath());
            break;
        case Action::SAVE_PNG:
            if (m_mode != ViewMode::TRIGRAPH3D) {
                std::string fname = "export_" +
                                    std::to_string((int)m_mode) + ".png";
                Screenshot::save(fname, getCurrentRGB());
                m_status = "Salvato: " + fname;
            }
            break;
        case Action::CANCEL:
            m_status = "Annullato.";
            break;
        default: break;
    }

    // --- 3. Navigazione file ---
    if (GetKey(olc::Key::RIGHT).bPressed) {
        m_navigator.moveForward();
        recomputeWindow();
        m_status = "";
    }
    if (GetKey(olc::Key::LEFT).bPressed) {
        m_navigator.moveBackward();
        recomputeWindow();
        m_status = "";
    }
    // Home: reset finestra al file intero
    if (GetKey(olc::Key::HOME).bPressed) {
        m_navigator.resetWindow(m_reader.getBytes());
        recomputeWindow();
        m_zoom2d  = 1.0f;
        m_scrollH = 0.0f;
        m_scrollV = 0.0f;
        m_status  = "Reset: file intero";
    }

    // Click sulla scrollbar file
    if (GetMouse(0).bHeld &&
        GetMouseY() >= FILE_NAV_Y &&
        GetMouseY() <  FILE_NAV_Y + FILE_NAV_H) {
        float norm = (float)(GetMouseX() - 6) /
                     (float)(WIN_W - 12);
        m_navigator.jumpTo(norm);
        recomputeWindow();
    }

    // --- 4. Input specifico per modalita' ---
    if (m_mode == ViewMode::TRIGRAPH3D)
        handle3DInput(fElapsedTime);
    else
        handle2DInput();

    // --- 5. Aggiorna canvas 2D ---
    if (m_dirty && m_mode != ViewMode::TRIGRAPH3D) {
        updateCanvas();
        m_dirty = false;
    }

    // --- 6. Rendering ---
    Clear(olc::BLACK);

    if (m_mode == ViewMode::TRIGRAPH3D) {
        m_renderer3d.render(this,
                            m_trigraph.getPoints(),
                            CANVAS_X, CANVAS_Y,
                            CANVAS_W, CANVAS_H);
    } else {
        draw2DCanvas();
    }

    // --- 7. UI ---
    UIState state;
    state.currentView   = m_mode;
    state.cmdStatus     = m_status;
    state.filepath      = m_reader.getFilepath();
    state.filesize      = m_reader.getSize();
    state.byteInfo      = getByteInfo(GetMouseX(), GetMouseY());
    state.position      = m_navigator.isValid()
                          ? m_navigator.getPositionString() : "";
    state.fileNavPos    = m_navigator.getNormalized();
    state.fileNavRatio  = m_navigator.getWindowRatio();
    state.fps           = 1.0f / fElapsedTime;
    state.zoomLevel     = m_zoom2d;
    state.scrollH       = m_scrollH;
    state.scrollV       = m_scrollV;
    state.inputMode     = m_input.isInputMode();
    state.inputBuffer   = m_input.getInputBuffer();
    state.cursorBlink   = m_input.getCursorBlink();

    m_ui.draw(this, state);
    return true;
}
