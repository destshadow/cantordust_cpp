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
    bool busy = m_computing.load();

    // Aggiorna canvas solo quando calcolo terminato
    // e solo dal main thread sotto mutex
    {
        std::lock_guard<std::mutex> lk(m_dataMutex);
        if (m_dirty && !busy) {
            updateCanvas();
            m_dirty = false;
        }
    }

    // Input — bloccato durante il calcolo
    Action action = m_input.update(this, fElapsedTime);

    if (!busy) {
        switch (action) {
            case Action::SWITCH_VIEW:
                m_mode   = m_input.getRequestedView();
                // Rigenera canvas dalla vista appena calcolata
                {
                    std::lock_guard<std::mutex> lk(m_dataMutex);
                    updateCanvas();
                }
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
        if (GetKey(olc::Key::HOME).bPressed) {
            m_navigator.resetWindow(m_reader.getBytes());
            recomputeWindow();
            m_zoom2d  = 1.0f;
            m_scrollH = 0.0f;
            m_scrollV = 0.0f;
            m_status  = "Reset: file intero";
        }

        if (GetMouse(0).bHeld &&
            GetMouseY() >= FILE_NAV_Y &&
            GetMouseY() <  FILE_NAV_Y + FILE_NAV_H) {
            float norm = (float)(GetMouseX() - 6) /
                         (float)(WIN_W - 12);
            m_navigator.jumpTo(norm);
            recomputeWindow();
        }
    }

    // Input modalita'
    if (m_mode == ViewMode::TRIGRAPH3D)
        handle3DInput(fElapsedTime);
    else if (!busy)
        handle2DInput();

    // Rendering
    Clear(olc::BLACK);

    if (m_mode == ViewMode::TRIGRAPH3D) {
        // Copia punti sotto lock per evitare race
        std::vector<Point3D> pts;
        {
            std::lock_guard<std::mutex> lk(m_dataMutex);
            pts = m_trigraph.getPoints();
        }
        m_renderer3d.render(this, pts,
                            CANVAS_X, CANVAS_Y,
                            CANVAS_W, CANVAS_H);
    } else {
        draw2DCanvas();
        if (!busy) drawSectionOverlay();
    }

    if (busy) drawSpinner(fElapsedTime);

    UIState state;
    state.currentView  = m_mode;
    state.cmdStatus    = busy ? "Calcolo in corso..." : m_status;
    state.filepath     = m_reader.getFilepath();
    state.filesize     = m_reader.getSize();
    state.byteInfo     = busy ? "" : getByteInfo(GetMouseX(), GetMouseY());
    state.position     = m_navigator.isValid()
                         ? m_navigator.getPositionString() : "";
    state.fileNavPos   = m_navigator.getNormalized();
    state.fileNavRatio = m_navigator.getWindowRatio();
    state.fps          = 1.0f / fElapsedTime;
    state.zoomLevel    = m_zoom2d;
    state.scrollH      = m_scrollH;
    state.scrollV      = m_scrollV;
    state.inputMode    = m_input.isInputMode();
    state.inputBuffer  = m_input.getInputBuffer();
    state.cursorBlink  = m_input.getCursorBlink();

    m_ui.draw(this, state);
    return true;
}

void Visualizer::drawSpinner(float fElapsedTime) {
    m_spinnerAngle += fElapsedTime * 3.0f;
    int bx = WIN_W/2 - 80, by = WIN_H/2 - 20;
    FillRect(bx, by, 160, 40, olc::Pixel(10, 10, 30, 200));
    DrawRect(bx, by, 160, 40, COL_BORDER());
    int dots = (int)(m_spinnerAngle * 2) % 4;
    std::string txt = "Calcolo";
    for (int i = 0; i < dots; i++) txt += ".";
    DrawString(bx + 10, by + 14, txt, COL_YELLOW());
}
