#include "visualizer.h"
#include <filesystem>

// Altezze barre — devono corrispondere a ui_renderer.cpp
static constexpr int TOP_BAR_H    = 26;
static constexpr int BOTTOM_BAR_H = 36;

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

    // --- 2. Esegui azione ---
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
            } else {
                m_status = "Screenshot 3D non supportato";
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

    // Click sulla scrollbar
    if (GetMouse(0).bHeld &&
        GetMouseY() > ScreenHeight() - BOTTOM_BAR_H) {
        float norm = (float)(GetMouseX() - 10) /
                     (float)(ScreenWidth() - 20);
        m_navigator.jumpTo(norm);
        recomputeWindow();
    }

    // --- 4. Input 3D ---
    if (m_mode == ViewMode::TRIGRAPH3D)
        handle3DInput(fElapsedTime);

    // --- 5. Aggiorna canvas 2D ---
    if (m_dirty && m_mode != ViewMode::TRIGRAPH3D) {
        updateCanvas();
        m_dirty = false;
    }

    // --- 6. Rendering ---
    Clear(olc::BLACK);

    if (m_mode == ViewMode::TRIGRAPH3D) {
        int renderH = ScreenHeight() - TOP_BAR_H - BOTTOM_BAR_H;
        m_renderer3d.render(this,
                            m_trigraph.getPoints(),
                            0, TOP_BAR_H,
                            ScreenWidth(), renderH);
    } else {
        // Scala il canvas 256x256 per riempire l'area disponibile
        // tra la topbar e la bottombar
        float scaleX = (float)ScreenWidth()  / 256.0f;
        float scaleY = (float)(ScreenHeight() - TOP_BAR_H - BOTTOM_BAR_H)
                       / 256.0f;
        DrawDecal({0.0f, (float)TOP_BAR_H}, m_decal,
                  {scaleX, scaleY});
    }

    drawScrollBar();

    // Costruisce UIState con tutti i dati aggiornati
    UIState state;
    state.currentView  = m_mode;
    state.status       = m_status;
    state.filepath     = m_reader.getFilepath();
    state.filesize     = m_reader.getSize();
    state.position     = m_navigator.isValid()
                         ? m_navigator.getPositionString() : "";
    state.fps          = 1.0f / fElapsedTime;
    state.inputMode    = m_input.isInputMode();
    state.inputBuffer  = m_input.getInputBuffer();
    state.cursorBlink  = m_input.getCursorBlink();

    m_ui.draw(this, state);

    return true;
}

void Visualizer::handle3DInput(float fElapsedTime) {
    if (GetKey(olc::Key::R).bPressed)
        m_renderer3d.resetRotation();

    if (GetMouse(0).bPressed) {
        m_dragging   = true;
        m_lastMouseX = GetMouseX();
        m_lastMouseY = GetMouseY();
    }
    if (GetMouse(0).bReleased)
        m_dragging = false;

    if (m_dragging) {
        float dx = (float)(GetMouseX() - m_lastMouseX);
        float dy = (float)(GetMouseY() - m_lastMouseY);
        m_renderer3d.rotate(dx, dy);
        m_lastMouseX = GetMouseX();
        m_lastMouseY = GetMouseY();
    }

    if (GetMouseWheel() > 0) m_renderer3d.zoom(+1.0f);
    if (GetMouseWheel() < 0) m_renderer3d.zoom(-1.0f);
}

void Visualizer::drawScrollBar() {
    if (!m_navigator.isValid()) return;

    int sw   = ScreenWidth();
    int sh   = ScreenHeight();
    int barY = sh - BOTTOM_BAR_H + 2;
    int barX = 6;
    int barW = sw - 12;

    // Track della scrollbar
    FillRect(barX, barY, barW, 10, olc::Pixel(40, 40, 60));
    DrawRect(barX, barY, barW, 10, olc::Pixel(80, 80, 120));

    // Cursore proporzionale
    float ratio   = (float)m_navigator.getWindowSize() /
                    (float)m_navigator.getTotalSize();
    int cursorW   = std::max(12, (int)(barW * ratio));
    int cursorX   = barX + (int)((barW - cursorW) *
                                  m_navigator.getNormalized());

    FillRect(cursorX, barY, cursorW, 10, olc::Pixel(80, 140, 200));
    DrawRect(cursorX, barY, cursorW, 10, olc::Pixel(120, 180, 255));
}

void Visualizer::updateCanvas() {
    auto rgb = getCurrentRGB();
    for (int y = 0; y < 256; y++)
        for (int x = 0; x < 256; x++) {
            int idx = (y * 256 + x) * 3;
            m_canvas->SetPixel(x, y, olc::Pixel(
                rgb[idx], rgb[idx+1], rgb[idx+2]
            ));
        }
    m_decal->Update();
}

std::vector<uint8_t> Visualizer::getCurrentRGB() const {
    switch (m_mode) {
        case ViewMode::DIGRAPH:    return m_digraph.toRGB();
        case ViewMode::DOTPLOT:    return m_dotplot.toRGB();
        case ViewMode::ENTROPY:    return m_entropy.toRGB();
        case ViewMode::HISTOGRAM:  return m_histogram.toRGB();
        case ViewMode::TRIGRAPH3D: return {};
    }
    return {};
}

void Visualizer::recomputeWindow() {
    auto window = m_navigator.getWindow(m_reader.getBytes());
    if (window.empty()) return;
    m_digraph.compute(window);
    m_dotplot.compute(window);
    m_entropy.compute(window);
    m_histogram.compute(window);
    m_trigraph.compute(m_reader.getBytes(),
                       m_navigator.getStart(),
                       m_navigator.getEnd());
    m_dirty = true;
}

void Visualizer::loadFile(const std::string& filepath) {
    if (!std::filesystem::exists(filepath)) {
        m_status = "Errore: file non trovato → " + filepath;
        return;
    }
    if (!m_reader.load(filepath)) {
        m_status = "Errore caricamento: " + filepath;
        return;
    }
    const auto& bytes = m_reader.getBytes();
    m_navigator.setData(bytes);
    m_digraph.compute(bytes);
    m_dotplot.compute(bytes);
    m_entropy.compute(bytes);
    m_histogram.compute(bytes);
    m_trigraph.compute(bytes);
    m_dirty  = true;
    m_status = "Caricato: " + filepath;
}
