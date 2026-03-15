#include "ui_renderer.h"
#include <cmath>

// --- Entry point: disegna tutto ---
void UIRenderer::draw(olc::PixelGameEngine* pge, const UIState& state) {
    drawTopBar(pge, state);
    drawBottomBar(pge, state);

    // L'overlay va disegnato per ultimo: sta sopra tutto
    if (state.inputMode)
        drawInputOverlay(pge, state);
}

// --- Barra superiore: tab delle 4 viste ---
void UIRenderer::drawTopBar(olc::PixelGameEngine* pge, const UIState& state) {
    pge->FillRect(0, 0, pge->ScreenWidth(), 18, olc::VERY_DARK_BLUE);

    // Definizione delle tab: label, vista associata, posizione X
    struct Tab { const char* label; ViewMode mode; int x; };
    Tab tabs[] = {
        {"[1] Digraph",   ViewMode::DIGRAPH,    2},
        {"[2] DotPlot",   ViewMode::DOTPLOT,   90},
        {"[3] Entropy",   ViewMode::ENTROPY,  178},
        {"[4] Histogram", ViewMode::HISTOGRAM, 266},
    };

    for (auto& t : tabs) {
        // Vista attiva → giallo, le altre → bianco
        olc::Pixel col = (state.currentView == t.mode)
                         ? olc::YELLOW : olc::WHITE;
        pge->DrawString(t.x, 4, t.label, col);
    }
}

// --- Barra inferiore: info file + status + shortcuts ---
void UIRenderer::drawBottomBar(olc::PixelGameEngine* pge, const UIState& state) {
    int sw = pge->ScreenWidth();
    int sh = pge->ScreenHeight();

    pge->FillRect(0, sh-30, sw, 30, olc::VERY_DARK_BLUE);

    // Prima riga: path + dimensione file
    if (!state.filepath.empty()) {
        std::string info = state.filepath +
                           "  (" + std::to_string(state.filesize) + " bytes)";
        pge->DrawString(4, sh-26, info, olc::CYAN);
    }

    // Seconda riga: messaggio status a sinistra, shortcuts a destra
    pge->DrawString(4, sh-14, state.status, olc::WHITE);
    pge->DrawString(sw-112, sh-14, "O=Apri  S=Salva", olc::YELLOW);
}

// --- Overlay input: box centrato per digitare il path ---
void UIRenderer::drawInputOverlay(olc::PixelGameEngine* pge, const UIState& state) {
    int sw = pge->ScreenWidth();
    int sh = pge->ScreenHeight();

    // Box centrato verticalmente
    int bx = 20,        by = sh/2 - 30;
    int bw = sw - 40,   bh = 60;

    // Sfondo scuro + bordo giallo
    pge->FillRect(bx, by, bw, bh, olc::VERY_DARK_GREY);
    pge->DrawRect(bx, by, bw, bh, olc::YELLOW);

    pge->DrawString(bx+8, by+8, "Apri file (ESC=annulla, NP_DIV=/):", olc::YELLOW);

    // Cursore lampeggiante: visibile per meta' del periodo (0.5s)
    std::string display = state.inputBuffer;
    if (std::fmod(state.cursorBlink, 1.0f) < 0.5f)
        display += "_";

    pge->DrawString(bx+8, by+22, display, olc::WHITE);
}
