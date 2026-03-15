#include "ui_renderer.h"
#include <cmath>
#include <string>

// Altezze barre — costanti per coerenza
static constexpr int TOP_BAR_H    = 26;
static constexpr int BOTTOM_BAR_H = 36;

void UIRenderer::draw(olc::PixelGameEngine* pge, const UIState& state) {
    drawTopBar(pge, state);
    drawBottomBar(pge, state);

    // Overlay 3D: shortcuts visibili solo in modalita' 3D
    if (state.currentView == ViewMode::TRIGRAPH3D)
        draw3DOverlay(pge, state);

    // Input overlay sempre sopra tutto
    if (state.inputMode)
        drawInputOverlay(pge, state);
}

// --- Barra superiore ---
// Layout: [tab1] [tab2] [tab3] [tab4] [tab5]    FPS: XX.X
void UIRenderer::drawTopBar(olc::PixelGameEngine* pge, const UIState& state) {
    int sw = pge->ScreenWidth();

    // Sfondo sfumato: due righe di colore leggermente diverso
    pge->FillRect(0, 0, sw, TOP_BAR_H, olc::Pixel(20, 20, 50));
    pge->DrawLine(0, TOP_BAR_H - 1, sw, TOP_BAR_H - 1, olc::DARK_BLUE);

    // Definizione tab con tasto, label e vista
    struct Tab {
        const char* key;
        const char* label;
        ViewMode    mode;
        int         x;
    };
    Tab tabs[] = {
        {"1", "Digraph",   ViewMode::DIGRAPH,    6},
        {"2", "DotPlot",   ViewMode::DOTPLOT,    90},
        {"3", "Entropy",   ViewMode::ENTROPY,   174},
        {"4", "Histogram", ViewMode::HISTOGRAM, 258},
        {"5", "3D View",   ViewMode::TRIGRAPH3D,352},
    };

    for (auto& t : tabs) {
        bool active = (state.currentView == t.mode);

        if (active) {
            // Tab attivo: sfondo evidenziato + testo giallo
            pge->FillRect(t.x - 2, 2, 78, TOP_BAR_H - 4,
                          olc::Pixel(40, 40, 100));
            pge->DrawRect(t.x - 2, 2, 78, TOP_BAR_H - 4,
                          olc::YELLOW);
            // Numero tasto in giallo brillante
            pge->DrawString(t.x, 5,
                            std::string("[") + t.key + "]",
                            olc::YELLOW);
            pge->DrawString(t.x + 22, 5, t.label, olc::WHITE);
        } else {
            // Tab inattivo: solo testo grigio
            pge->DrawString(t.x, 5,
                            std::string("[") + t.key + "]",
                            olc::DARK_YELLOW);
            pge->DrawString(t.x + 22, 5, t.label,
                            olc::Pixel(160, 160, 160));
        }
    }

    // FPS in alto a destra
    // Formato: "FPS: XX.X" con colore che cambia in base al valore
    std::string fpsStr = "FPS: " +
                         std::to_string((int)state.fps);
    olc::Pixel fpsCol = state.fps > 50 ? olc::GREEN :
                        state.fps > 25 ? olc::YELLOW : olc::RED;
    pge->DrawString(sw - 64, 5, fpsStr, fpsCol);
}

// --- Barra inferiore ---
// Riga 1: filepath  (dimensione file)
// Riga 2: posizione nella finestra | status operazione | O=Apri S=Salva
void UIRenderer::drawBottomBar(olc::PixelGameEngine* pge,
                                const UIState& state) {
    int sw = pge->ScreenWidth();
    int sh = pge->ScreenHeight();
    int barY = sh - BOTTOM_BAR_H;

    // Sfondo
    pge->FillRect(0, barY, sw, BOTTOM_BAR_H, olc::Pixel(20, 20, 50));
    pge->DrawLine(0, barY, sw, barY, olc::DARK_BLUE);

    // --- Riga 1: filepath + dimensione ---
    if (!state.filepath.empty()) {
        // Converti bytes in formato leggibile (KB, MB)
        std::string sizeStr;
        if (state.filesize >= 1024 * 1024)
            sizeStr = std::to_string(state.filesize / (1024*1024)) + " MB";
        else if (state.filesize >= 1024)
            sizeStr = std::to_string(state.filesize / 1024) + " KB";
        else
            sizeStr = std::to_string(state.filesize) + " B";

        std::string fileInfo = state.filepath + "  [" + sizeStr + "]";
        pge->DrawString(6, barY + 4, fileInfo, olc::CYAN);
    }

    // --- Riga 2 sinistra: posizione nella finestra ---
    if (!state.position.empty())
        pge->DrawString(6, barY + 16, state.position,
                        olc::Pixel(180, 180, 100));

    // --- Riga 2 centro: messaggio status ---
    if (!state.status.empty())
        pge->DrawString(sw/2 - 80, barY + 16, state.status, olc::WHITE);

    // --- Riga 2 destra: shortcuts ---
    pge->DrawString(sw - 120, barY + 16,
                    "O=Apri  S=Salva", olc::YELLOW);
}

// --- Overlay 3D: shortcuts visibili solo in modalita' 3D ---
// Box trasparente in alto a destra che scompare sulle altre viste
void UIRenderer::draw3DOverlay(olc::PixelGameEngine* pge,
                                const UIState& state) {
    int sw = pge->ScreenWidth();

    // Box compatto con sfondo semi-opaco
    int bx = sw - 130, by = TOP_BAR_H + 6;
    pge->FillRect(bx, by, 124, 52, olc::Pixel(20, 20, 50, 200));
    pge->DrawRect(bx, by, 124, 52, olc::Pixel(80, 80, 120));

    pge->DrawString(bx + 6, by +  6, "3D Controls:",   olc::YELLOW);
    pge->DrawString(bx + 6, by + 18, "Drag = Ruota",   olc::WHITE);
    pge->DrawString(bx + 6, by + 28, "Scroll = Zoom",  olc::WHITE);
    pge->DrawString(bx + 6, by + 38, "R = Reset",      olc::Pixel(100, 220, 100));
}

// --- Overlay input: box per digitare il path ---
void UIRenderer::drawInputOverlay(olc::PixelGameEngine* pge,
                                   const UIState& state) {
    int sw = pge->ScreenWidth();
    int sh = pge->ScreenHeight();

    // Box centrato verticalmente
    int bx = 20,      by = sh/2 - 35;
    int bw = sw - 40, bh = 70;

    pge->FillRect(bx, by, bw, bh, olc::Pixel(15, 15, 40));
    pge->DrawRect(bx, by, bw, bh, olc::YELLOW);

    // Titolo
    pge->DrawString(bx + 8, by + 8,
                    "Apri file  (ENTER=conferma  ESC=annulla  NP_DIV=/)",
                    olc::YELLOW);

    // Separatore
    pge->DrawLine(bx + 4, by + 22, bx + bw - 4, by + 22,
                  olc::Pixel(60, 60, 100));

    // Buffer con cursore lampeggiante
    std::string display = state.inputBuffer;
    if (std::fmod(state.cursorBlink, 1.0f) < 0.5f)
        display += "_";
    pge->DrawString(bx + 8, by + 30, display, olc::WHITE, 2);
}
