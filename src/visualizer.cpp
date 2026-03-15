#include "visualizer.h"
#include <iostream>

Visualizer::Visualizer() {
    sAppName = "CantorDust++";
}

bool Visualizer::OnUserCreate() {
    m_canvas = new olc::Sprite(256, 256);
    m_decal  = new olc::Decal(m_canvas);
    loadFile("/bin/ls");
    return true;
}

bool Visualizer::OnUserUpdate(float fElapsedTime) {
    // --- Input: switch viste con tasti 1 2 3 4 ---

    // Tasto 1 → Digraph
    // Mostra le coppie di byte consecutive come punti su piano 2D
    if (GetKey(olc::Key::K1).bPressed) {
        m_mode  = ViewMode::DIGRAPH;
        m_dirty = true;
        m_status = "Digraph — coppie di byte consecutive";
    }

    // Tasto 2 → Dot Plot
    // Mostra offset vs valore byte — la "radiografia" del file
    if (GetKey(olc::Key::K2).bPressed) {
        m_mode  = ViewMode::DOTPLOT;
        m_dirty = true;
        m_status = "Dot Plot — offset vs valore byte";
    }

    // Tasto 3 → Entropy
    // Mostra l'entropia di Shannon su finestre scorrevoli
    if (GetKey(olc::Key::K3).bPressed) {
        m_mode  = ViewMode::ENTROPY;
        m_dirty = true;
        m_status = "Entropy — disordine per sezione (rosso=cifrato, blu=testo)";
    }

    // Tasto 4 → Histogram
    // Mostra la frequenza di ogni byte value 0-255
    if (GetKey(olc::Key::K4).bPressed) {
        m_mode  = ViewMode::HISTOGRAM;
        m_dirty = true;
        m_status = "Histogram — frequenza di ogni byte value";
    }

    // Tasto S → salva PNG della vista corrente
    if (GetKey(olc::Key::S).bPressed) {
        auto rgb = getCurrentRGB();
        std::string fname = "export_" + std::to_string((int)m_mode) + ".png";
        Screenshot::save(fname, rgb);
        m_status = "Salvato: " + fname;
    }

    // --- Aggiorna canvas se necessario ---
    if (m_dirty) {
        updateCanvas();
        m_dirty = false;
    }

    // --- Rendering ---
    Clear(olc::BLACK);
    DrawDecal({0, 0}, m_decal, {2.0f, 2.0f});
    drawUI();

    return true;
}

// Recupera il buffer RGB della vista corrente
std::vector<uint8_t> Visualizer::getCurrentRGB() const {
    switch (m_mode) {
        case ViewMode::DIGRAPH:   return m_digraph.toRGB();
        case ViewMode::DOTPLOT:   return m_dotplot.toRGB();
        case ViewMode::ENTROPY:   return m_entropy.toRGB();
        case ViewMode::HISTOGRAM: return m_histogram.toRGB();
    }
    return {};
}

void Visualizer::updateCanvas() {
    // Prende il buffer RGB della vista attiva
    // e lo copia pixel per pixel nel canvas olcPGE
    auto rgb = getCurrentRGB();

    for (int y = 0; y < 256; y++)
        for (int x = 0; x < 256; x++) {
            int idx = (y * 256 + x) * 3;
            m_canvas->SetPixel(x, y, olc::Pixel(
                rgb[idx], rgb[idx+1], rgb[idx+2]
            ));
        }

    // Aggiorna la texture GPU dal sprite CPU
    m_decal->Update();
}

void Visualizer::drawUI() {
    // --- Barra superiore: nome viste ---
    FillRect(0, 0, ScreenWidth(), 18, olc::VERY_DARK_BLUE);

    // Evidenzia la vista attiva
    struct Tab { const char* label; ViewMode mode; int x; };
    Tab tabs[] = {
        {"[1] Digraph",   ViewMode::DIGRAPH,   2},
        {"[2] DotPlot",   ViewMode::DOTPLOT,   90},
        {"[3] Entropy",   ViewMode::ENTROPY,   178},
        {"[4] Histogram", ViewMode::HISTOGRAM, 266},
    };

    for (auto& t : tabs) {
        olc::Pixel col = (m_mode == t.mode) ? olc::YELLOW : olc::WHITE;
        DrawString(t.x, 4, t.label, col);
    }

    // --- Barra inferiore: status + info file ---
    FillRect(0, ScreenHeight()-30, ScreenWidth(), 30, olc::VERY_DARK_BLUE);

    if (m_reader.isLoaded()) {
        std::string info = m_reader.getFilepath() +
                           "  (" +
                           std::to_string(m_reader.getSize()) +
                           " bytes)";
        DrawString(4, ScreenHeight()-26, info, olc::CYAN);
    }

    DrawString(4, ScreenHeight()-14, m_status, olc::WHITE);
    DrawString(ScreenWidth()-80, ScreenHeight()-14, "S=Salva PNG", olc::YELLOW);
}

void Visualizer::loadFile(const std::string& filepath) {
    if (!m_reader.load(filepath)) {
        m_status = "Errore: " + filepath;
        return;
    }

    // Calcola tutte e 4 le visualizzazioni subito al caricamento
    const auto& bytes = m_reader.getBytes();
    m_digraph.compute(bytes);
    m_dotplot.compute(bytes);
    m_entropy.compute(bytes);
    m_histogram.compute(bytes);

    m_dirty  = true;
    m_status = "Caricato: " + filepath;
}
