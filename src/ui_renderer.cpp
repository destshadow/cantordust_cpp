#include "ui_renderer.h"
#include <cmath>
#include <sstream>
#include <iomanip>

void UIRenderer::draw(olc::PixelGameEngine* pge, const UIState& state) {
    drawTopBar(pge, state);

    // Scrollbar canvas solo se zoom > 1
    if (state.zoomLevel > 1.01f) {
        drawScrollbarH(pge, state);
        drawScrollbarV(pge, state);
    }

    drawFileStatus(pge, state);
    drawFileNav(pge, state);
    drawCmdStatus(pge, state);

    if (state.currentView == ViewMode::TRIGRAPH3D)
        draw3DOverlay(pge, state);

    if (state.inputMode)
        drawInputOverlay(pge, state);
}

// --- Topbar: tab cliccabili ---
void UIRenderer::drawTopBar(olc::PixelGameEngine* pge,
                             const UIState& state) {
    // Sfondo
    pge->FillRect(0, 0, WIN_W, TOPBAR_H, COL_BG());
    pge->DrawLine(0, TOPBAR_H-1, WIN_W, TOPBAR_H-1, COL_BORDER());

    struct Tab { const char* num; const char* label; ViewMode mode; int x; };
    Tab tabs[] = {
        {"1", "Digraph",   ViewMode::DIGRAPH,      8},
        {"2", "DotPlot",   ViewMode::DOTPLOT,     208},
        {"3", "Entropy",   ViewMode::ENTROPY,     408},
        {"4", "Histogram", ViewMode::HISTOGRAM,   608},
        {"5", "3D View",   ViewMode::TRIGRAPH3D,  808},
    };

    for (auto& t : tabs) {
        bool active = (state.currentView == t.mode);
        int  tw     = 190;   // larghezza tab

        if (active) {
            pge->FillRect(t.x, 2, tw, TOPBAR_H-4, COL_BG_LIGHT());
            pge->DrawRect(t.x, 2, tw, TOPBAR_H-4, COL_YELLOW());
            // Numero tasto
            pge->DrawString(t.x + 8,  12,
                            std::string("[") + t.num + "]",
                            COL_YELLOW(), 2);
            // Label
            pge->DrawString(t.x + 42, 12, t.label, olc::WHITE, 2);
        } else {
            pge->DrawRect(t.x, 2, tw, TOPBAR_H-4, COL_BORDER());
            pge->DrawString(t.x + 8,  12,
                            std::string("[") + t.num + "]",
                            olc::Pixel(150,150,80), 2);
            pge->DrawString(t.x + 42, 12, t.label,
                            olc::Pixel(130,130,130), 2);
        }
    }

    // FPS in alto a destra
    std::string fpsStr = "FPS:" + std::to_string((int)state.fps);
    olc::Pixel fpsCol  = state.fps > 50 ? COL_GREEN() :
                         state.fps > 25 ? COL_YELLOW() : olc::RED;
    pge->DrawString(WIN_W - 56, 14, fpsStr, fpsCol);
}

// --- Scrollbar orizzontale canvas ---
void UIRenderer::drawScrollbarH(olc::PixelGameEngine* pge,
                                 const UIState& state) {
    int y = SCROLLBAR_H_Y;
    int w = CANVAS_W;

    pge->FillRect(0, y, w, SCROLLBAR_H, COL_SCROLLBAR());
    pge->DrawLine(0, y, w, y, COL_BORDER());

    // Cursore: dimensione proporzionale allo zoom
    float ratio   = 1.0f / state.zoomLevel;
    int   cw      = std::max(20, (int)(w * ratio));
    int   cx      = (int)((w - cw) * state.scrollH);
    pge->FillRect(cx, y+1, cw, SCROLLBAR_H-2, COL_CURSOR());
    pge->DrawRect(cx, y+1, cw, SCROLLBAR_H-2,
                  olc::Pixel(120, 180, 255));
}

// --- Scrollbar verticale canvas ---
void UIRenderer::drawScrollbarV(olc::PixelGameEngine* pge,
                                 const UIState& state) {
    int x = CANVAS_W;
    int y = CANVAS_Y;
    int h = CANVAS_H;

    pge->FillRect(x, y, SCROLLBAR_V_W, h, COL_SCROLLBAR());
    pge->DrawLine(x, y, x, y+h, COL_BORDER());

    float ratio  = 1.0f / state.zoomLevel;
    int   ch     = std::max(20, (int)(h * ratio));
    int   cy     = y + (int)((h - ch) * state.scrollV);
    pge->FillRect(x+1, cy, SCROLLBAR_V_W-2, ch, COL_CURSOR());
    pge->DrawRect(x+1, cy, SCROLLBAR_V_W-2, ch,
                  olc::Pixel(120, 180, 255));
}

// --- Barra FILE STATUS ---
// Mostra: filepath + size a sinistra, byte sotto cursore a destra
void UIRenderer::drawFileStatus(olc::PixelGameEngine* pge,
                                 const UIState& state) {
    int y = FILE_STATUS_Y;
    pge->FillRect(0, y, WIN_W, FILE_STATUS_H, COL_BG());
    pge->DrawLine(0, y, WIN_W, y, COL_BORDER());

    // Sinistra: file + dimensione
    if (!state.filepath.empty()) {
        std::string sizeStr;
        if      (state.filesize >= 1024*1024)
            sizeStr = std::to_string(state.filesize/(1024*1024)) + "MB";
        else if (state.filesize >= 1024)
            sizeStr = std::to_string(state.filesize/1024) + "KB";
        else
            sizeStr = std::to_string(state.filesize) + "B";

        std::string info = "FILE: " + state.filepath +
                           "  [" + sizeStr + "]";
        pge->DrawString(6, y+6, info, COL_CYAN());
    }

    // Destra: byte sotto il cursore (quando cliccheremo sull'immagine)
    if (!state.byteInfo.empty())
        pge->DrawString(WIN_W - 260, y+6, state.byteInfo,
                        COL_YELLOW());
}

// --- Barra FILE NAV: scrollbar di navigazione nel file ---
void UIRenderer::drawFileNav(olc::PixelGameEngine* pge,
                              const UIState& state) {
    int y = FILE_NAV_Y;
    int w = WIN_W;

    pge->FillRect(0, y, w, FILE_NAV_H, COL_SCROLLBAR());
    pge->DrawLine(0, y, w, y, COL_BORDER());

    // Cursore proporzionale alla finestra sul file
    int cw  = std::max(12, (int)(w * state.fileNavRatio));
    int cx  = (int)((w - cw) * state.fileNavPos);
    pge->FillRect(cx, y+1, cw, FILE_NAV_H-2, COL_CURSOR());
    pge->DrawRect(cx, y+1, cw, FILE_NAV_H-2,
                  olc::Pixel(120, 180, 255));

    // Posizione in esadecimale centrata sulla scrollbar
    if (!state.position.empty())
        pge->DrawString(WIN_W/2 - (int)(state.position.size()*3),
                        y+2, state.position,
                        olc::Pixel(200, 200, 100));
}

// --- Barra CMD STATUS: comandi sempre visibili ---
void UIRenderer::drawCmdStatus(olc::PixelGameEngine* pge,
                                const UIState& state) {
    int y = CMD_STATUS_Y;
    pge->FillRect(0, y, WIN_W, CMD_STATUS_H, COL_BG());
    pge->DrawLine(0, y, WIN_W, y, COL_BORDER());

    // Sinistra: messaggio operazione corrente
    if (!state.cmdStatus.empty())
        pge->DrawString(6, y+5, state.cmdStatus, olc::WHITE);

    // Destra: shortcuts sempre visibili
    std::string cmds = "O=Apri  S=Salva  ←→=Naviga  Home=Reset  Scroll=Zoom";
    pge->DrawString(WIN_W - (int)(cmds.size()*6) - 6,
                    y+5, cmds, olc::Pixel(140, 140, 140));
}

// --- Overlay 3D controls ---
void UIRenderer::draw3DOverlay(olc::PixelGameEngine* pge,
                                const UIState& state) {
    int bx = WIN_W - 180, by = TOPBAR_H + 8;
    pge->FillRect(bx, by, 172, 76, olc::Pixel(18, 18, 45, 200));
    pge->DrawRect(bx, by, 172, 76, COL_BORDER());

    pge->DrawString(bx+8, by+6,  "3D Controls:",        COL_YELLOW());
    pge->DrawString(bx+8, by+18, "LMB Drag = Ruota",    olc::WHITE);
    pge->DrawString(bx+8, by+30, "RMB Drag = Pan",      olc::WHITE);
    pge->DrawString(bx+8, by+42, "WASD=Muovi  QE=Su/Giu", COL_GREEN());
    pge->DrawString(bx+8, by+54, "Scroll=Zoom  R=Reset", olc::Pixel(100,200,100));
}

// --- Overlay input path ---
void UIRenderer::drawInputOverlay(olc::PixelGameEngine* pge,
                                   const UIState& state) {
    int bx = 30,          by = WIN_H/2 - 40;
    int bw = WIN_W - 60,  bh = 80;

    pge->FillRect(bx, by, bw, bh, olc::Pixel(12, 12, 35));
    pge->DrawRect(bx, by, bw, bh, COL_YELLOW());

    pge->DrawString(bx+10, by+10,
                    "Apri file  (ENTER=conferma  ESC=annulla  NP_DIV=/)",
                    COL_YELLOW(), 2);

    pge->DrawLine(bx+6, by+32, bx+bw-6, by+32, COL_BORDER());

    std::string display = state.inputBuffer;
    if (std::fmod(state.cursorBlink, 1.0f) < 0.5f)
        display += "_";
    pge->DrawString(bx+10, by+42, display, olc::WHITE, 2);
}
