#include "visualizer.h"
#include <iostream>

Visualizer::Visualizer() {
    sAppName = "CantorDust++";
}

bool Visualizer::OnUserCreate() {
    m_canvas = new olc::Sprite(256, 256);
    m_decal  = new olc::Decal(m_canvas);

    // Carica subito /bin/ls come demo
    loadFile("/bin/ls");
    return true;
}

bool Visualizer::OnUserUpdate(float fElapsedTime) {
    Clear(olc::BLACK);

    // Aggiorna decal se il canvas e' cambiato
    if (m_dirty) {
        auto rgb = m_digraph.toRGB();
        for (int y = 0; y < 256; y++)
            for (int x = 0; x < 256; x++) {
                int idx = (y * 256 + x) * 3;
                m_canvas->SetPixel(x, y, olc::Pixel(
                    rgb[idx], rgb[idx+1], rgb[idx+2]
                ));
            }
        m_decal->Update();
        m_dirty = false;
    }

    // Disegna il digraph scalato a 512x512
    DrawDecal({0, 0}, m_decal, {2.0f, 2.0f});

    // UI testuale
    drawUI();

    // Salva screenshot con S
    if (GetKey(olc::Key::S).bPressed) {
        auto rgb = m_digraph.toRGB();
        Screenshot::save("digraph_export.png", rgb);
        m_status = "Salvato: digraph_export.png";
    }

    return true;
}

void Visualizer::drawUI() {
    // Barra info in basso
    FillRect(0, ScreenHeight() - 20, ScreenWidth(), 20, olc::DARK_BLUE);
    DrawString(4, ScreenHeight() - 14, m_status, olc::WHITE);
    DrawString(4, 4, "S = Salva PNG", olc::YELLOW);

    // Info file
    if (m_reader.isLoaded()) {
        std::string info = "File: " + m_reader.getFilepath() +
                           "  |  " +
                           std::to_string(m_reader.getSize()) + " bytes";
        DrawString(4, 20, info, olc::CYAN);
    }
}

void Visualizer::loadFile(const std::string& filepath) {
    if (m_reader.load(filepath)) {
        m_digraph.compute(m_reader.getBytes());
        m_dirty  = true;
        m_status = "Caricato: " + filepath;
    } else {
        m_status = "Errore caricamento: " + filepath;
    }
}