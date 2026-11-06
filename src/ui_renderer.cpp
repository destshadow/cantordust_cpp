#include "ui_renderer.h"
#include <cmath>
#include <string>

static const char* const TAB_LABEL[] = {
    "1 Digraph", "2 Dot Plot", "3 Entropia", "4 Istogramma", "5 3D",
    "6 Raw", "7 Metric Map", "8 Byte Cloud", "9 One Tuple",
};
static const ViewMode TAB_VIEW[] = {
    ViewMode::DIGRAPH, ViewMode::DOTPLOT, ViewMode::ENTROPY, ViewMode::HISTOGRAM,
    ViewMode::TRIGRAPH3D, ViewMode::RAWPIXELS, ViewMode::METRICMAP,
    ViewMode::BYTECLOUD, ViewMode::ONETUPLE,
};
static constexpr int TOTAL_TABS = 9;

// Accorcia un testo a maxChars tenendo la parte finale ("...fine"):
// per i percorsi la parte utile e' il nome del file.
static std::string fitTail(const std::string& s, int maxChars) {
    if (maxChars <= 3) return "";
    if ((int)s.size() <= maxChars) return s;
    return "..." + s.substr(s.size() - (maxChars - 3));
}

void UIRenderer::draw(olc::PixelGameEngine* pge,
                       const UIState& state) {
    drawTopBar(pge, state);
    if (state.zoomLevel > 1.01f) {
        drawScrollbarH(pge, state);
        drawScrollbarV(pge, state);
    }
    drawFileStatus(pge, state);
    drawCmdStatus(pge, state);
    if (state.currentView == ViewMode::TRIGRAPH3D)
        draw3DOverlay(pge, state);
    if (state.inputMode)
        drawInputOverlay(pge, state);
}

void UIRenderer::drawTopBar(olc::PixelGameEngine* pge,
                              const UIState& state) {
    const int sw = pge->ScreenWidth();
    pge->FillRect(0, 0, sw, TOPBAR_H, COL_BG());
    pge->DrawLine(0, TOPBAR_H - 1, sw, TOPBAR_H - 1, COL_BORDER());

    const int textY = (TOPBAR_H - 8) / 2;
    for (int i = 0; i < TOTAL_TABS; i++) {
        const int  tx     = i * TAB_W;
        const bool active = state.currentView == TAB_VIEW[i];
        const bool hover  = state.mouseY < TOPBAR_H &&
                            state.mouseX >= tx && state.mouseX < tx + TAB_W;
        if (active) {
            pge->FillRect(tx, 0, TAB_W, TOPBAR_H - 1, COL_BG_LIGHT());
            pge->FillRect(tx, TOPBAR_H - 2, TAB_W, 2, COL_ACCENT());
        }
        pge->DrawString(tx + 10, textY, TAB_LABEL[i],
                        active ? COL_ACCENT() : hover ? COL_TEXT() : COL_DIM());
    }

    std::string fps = std::to_string((int)state.fps) + " fps";
    pge->DrawString(sw - (int)fps.size() * 8 - 6, textY, fps, COL_DIM());
}

void UIRenderer::drawScrollbarH(olc::PixelGameEngine* pge,
                                  const UIState& state) {
    int x0 = CANVAS_X, y = SCROLLBAR_H_Y, w = CANVAS_W;
    pge->FillRect(x0, y, w, SCROLLBAR_H, COL_SCROLLBAR());
    pge->DrawLine(x0, y, x0 + w, y, COL_BORDER());
    int cw = std::max(20, (int)(w / state.zoomLevel));
    int cx = x0 + (int)((w - cw) * state.scrollH);
    pge->FillRect(cx, y+1, cw, SCROLLBAR_H-2, COL_CURSOR());
    pge->DrawRect(cx, y+1, cw, SCROLLBAR_H-2, COL_TEXT());
}

void UIRenderer::drawScrollbarV(olc::PixelGameEngine* pge,
                                  const UIState& state) {
    int x = SCROLLBAR_V_X, y = CANVAS_Y, h = CANVAS_H;
    pge->FillRect(x, y, SCROLLBAR_V_W, h, COL_SCROLLBAR());
    pge->DrawLine(x, y, x, y+h, COL_BORDER());
    int ch = std::max(20, (int)(h / state.zoomLevel));
    int cy = y + (int)((h - ch) * state.scrollV);
    pge->FillRect(x+1, cy, SCROLLBAR_V_W-2, ch, COL_CURSOR());
    pge->DrawRect(x+1, cy, SCROLLBAR_V_W-2, ch, COL_TEXT());
}

void UIRenderer::drawFileStatus(olc::PixelGameEngine* pge,
                                  const UIState& state) {
    int sw = pge->ScreenWidth();
    int y  = FILE_STATUS_Y;
    pge->FillRect(0, y, sw, FILE_STATUS_H, COL_BG());
    pge->DrawLine(0, y, sw, y, COL_BORDER());
    if (!state.filepath.empty()) {
        std::string sizeStr;
        if      (state.filesize >= 1024*1024)
            sizeStr = std::to_string(state.filesize/(1024*1024)) + "MB";
        else if (state.filesize >= 1024)
            sizeStr = std::to_string(state.filesize/1024) + "KB";
        else
            sizeStr = std::to_string(state.filesize) + "B";
        // Spazio fino all'info byte (a destra, larga 260 px)
        std::string tail = "  [" + sizeStr + "]";
        int fileEnd  = state.notice.empty() ? sw - 260 : sw / 2;
        int maxChars = (fileEnd - 12) / 8 - 6 - (int)tail.size();
        pge->DrawString(6, y+6,
                        "FILE: " + fitTail(state.filepath, maxChars) + tail,
                        COL_DIM());
    }
    if (!state.notice.empty())
        pge->DrawString(sw / 2, y+6, fitTail(state.notice, (sw / 2 - 260) / 8), COL_TEXT());
    if (!state.byteInfo.empty())
        pge->DrawString(sw-260, y+6, fitTail(state.byteInfo, 32), COL_TEXT());
}

void UIRenderer::drawCmdStatus(olc::PixelGameEngine* pge,
                                 const UIState& state) {
    int sw = pge->ScreenWidth();
    int y  = CMD_STATUS_Y;
    pge->FillRect(0, y, sw, CMD_STATUS_H, COL_BG());
    pge->DrawLine(0, y, sw, y, COL_BORDER());
    std::string cmds =
        "O apri   S salva   B barre   Home file intero";
    int cmdsX = sw - (int)(cmds.size()*8) - 6;
    // Lo stato a sinistra non deve finire sotto l'elenco comandi
    if (!state.cmdStatus.empty())
        pge->DrawString(6, y+5, fitTail(state.cmdStatus, (cmdsX - 18) / 8),
                        COL_TEXT());
    pge->DrawString(cmdsX, y+5, cmds, COL_DIM());
}

void UIRenderer::draw3DOverlay(olc::PixelGameEngine* pge,
                                 const UIState& state) {
    (void)state;
    const int boxW = 24 * 8 + 16;      // la riga piu' lunga + i margini
    int bx = CANVAS_X + CANVAS_W - boxW - 8, by = CANVAS_Y + 8;
    pge->FillRect(bx, by, boxW, 80, COL_BG());
    pge->DrawRect(bx, by, boxW, 80, COL_BORDER());
    int y = by + 6;
    auto row = [&](const char* s) { pge->DrawString(bx + 8, y, s, COL_DIM()); y += 12; };
    row("Tasto sx: ruota");
    row("Tasto dx: sposta");
    row("WASD: muovi  QE: su/giu'");
    row("Rotella: zoom  R: reset");
    row("C: colore  Click: dove");
}

void UIRenderer::drawInputOverlay(olc::PixelGameEngine* pge,
                                    const UIState& state) {
    int sw = pge->ScreenWidth();
    int sh = pge->ScreenHeight();
    int bx = 30, by = sh/2-40, bw = sw-60, bh = 80;
    pge->FillRect(bx, by, bw, bh, COL_BG());
    pge->DrawRect(bx, by, bw, bh, COL_ACCENT());
    pge->DrawString(bx+10, by+10,
                    "Apri file (Invio = ok, Esc = annulla)",
                    COL_ACCENT(), 2);
    pge->DrawLine(bx+6, by+32, bx+bw-6, by+32, COL_BORDER());
    std::string display = state.inputBuffer;
    if (std::fmod(state.cursorBlink, 1.0f) < 0.5f) display += "_";
    pge->DrawString(bx+10, by+42, display, COL_TEXT(), 2);
}
