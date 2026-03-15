#include "visualizer.h"
#include <filesystem>

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

// OnUserUpdate: orchestrazione pura
// 1. chiedi all'InputHandler cosa e' successo
// 2. esegui l'azione
// 3. aggiorna canvas se serve
// 4. disegna tutto
bool Visualizer::OnUserUpdate(float fElapsedTime) {

    // --- 1. Input ---
    Action action = m_input.update(this, fElapsedTime);

    // --- 2. Esegui azione ---
    switch (action) {
        case Action::SWITCH_VIEW:
            m_mode  = m_input.getRequestedView();
            m_dirty = true;
            m_status = "Vista: " + std::to_string((int)m_mode + 1);
            break;

        case Action::OPEN_FILE:
            loadFile(m_input.getRequestedPath());
            break;

        case Action::SAVE_PNG: {
            std::string fname = "export_" +
                                std::to_string((int)m_mode) + ".png";
            Screenshot::save(fname, getCurrentRGB());
            m_status = "Salvato: " + fname;
            break;
        }

        case Action::CANCEL:
            m_status = "Annullato.";
            break;

        case Action::NONE:
        default:
            break;
    }

    // --- 3. Aggiorna canvas ---
    if (m_dirty) {
        updateCanvas();
        m_dirty = false;
    }

    // --- 4. Rendering ---
    Clear(olc::BLACK);
    DrawDecal({0, 0}, m_decal, {2.0f, 2.0f});

    // Costruisce UIState e passa tutto all'UIRenderer
    UIState state;
    state.currentView  = m_mode;
    state.status       = m_status;
    state.filepath     = m_reader.getFilepath();
    state.filesize     = m_reader.getSize();
    state.inputMode    = m_input.isInputMode();
    state.inputBuffer  = m_input.getInputBuffer();
    state.cursorBlink  = m_input.getCursorBlink();

    m_ui.draw(this, state);

    return true;
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
        case ViewMode::DIGRAPH:   return m_digraph.toRGB();
        case ViewMode::DOTPLOT:   return m_dotplot.toRGB();
        case ViewMode::ENTROPY:   return m_entropy.toRGB();
        case ViewMode::HISTOGRAM: return m_histogram.toRGB();
    }
    return {};
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
    m_digraph.compute(bytes);
    m_dotplot.compute(bytes);
    m_entropy.compute(bytes);
    m_histogram.compute(bytes);

    m_dirty  = true;
    m_status = "Caricato: " + filepath;
}
