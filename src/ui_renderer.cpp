#include "ui_renderer.h"
#include <cmath>
#include <string>

void UIRenderer::draw(olc::PixelGameEngine* pge, const UIState& state) {
    drawTopBar(pge, state);
    if (state.zoomLevel > 1.01f) {
        drawScrollbarH(pge, state);
        drawScrollbarV(pge, state);
    }
    drawFileStatus(pge, state);
    drawFileNav(pge, state);
    drawCmdStatus(pge, state);
    if (state.currentView == ViewMode::TRIGRAPH3D)
        draw3DOverlay(pge, state);
    drawSectionPanel(pge, state);
    if (state.inputMode)
        drawInputOverlay(pge, state);
}

void UIRenderer::drawTopBar(olc::PixelGameEngine* pge,
                             const UIState& state) {
    int sw = pge->ScreenWidth();
    pge->FillRect(0, 0, sw, TOPBAR_H, COL_BG());
    pge->DrawLine(0, TOPBAR_H-1, sw, TOPBAR_H-1, COL_BORDER());

    struct Tab { const char* num; const char* label; ViewMode mode; };
    static const Tab tabs[] = {
        {"1", "Digraph",   ViewMode::DIGRAPH   },
        {"2", "DotPlot",   ViewMode::DOTPLOT   },
        {"3", "Entropy",   ViewMode::ENTROPY   },
        {"4", "Histogram", ViewMode::HISTOGRAM },
        {"5", "3D View",   ViewMode::TRIGRAPH3D},
        {"6", "RawPix",    ViewMode::RAWPIXELS },
        {"7", "Metric",    ViewMode::METRICMAP },
    };
    static constexpr int N = 7;

    // Larghezza tab dinamica — usa tutto lo schermo
    int tabW     = sw / N;
    int txtScale = (tabW >= 130) ? 2 : 1;
    int charW    = 8 * txtScale;
    int charH    = 8 * txtScale;
    int textY    = (TOPBAR_H - charH) / 2;

    for (int i = 0; i < N; i++) {
        const auto& t  = tabs[i];
        int  tx        = i * tabW;
        bool active    = (state.currentView == t.mode);

        if (active) {
            pge->FillRect(tx+1, 2, tabW-2, TOPBAR_H-4, COL_BG_LIGHT());
            pge->DrawRect(tx+1, 2, tabW-2, TOPBAR_H-4, COL_YELLOW());
        } else {
            pge->DrawRect(tx+1, 2, tabW-2, TOPBAR_H-4, COL_BORDER());
        }

        std::string full = std::string("[") + t.num + "] " + t.label;
        int maxChars = (tabW - 8) / charW;
        if ((int)full.size() > maxChars)
            full = full.substr(0, std::max(0, maxChars));

        int textW = (int)full.size() * charW;
        int textX = tx + (tabW - textW) / 2;

        olc::Pixel col = active ? COL_YELLOW() : olc::Pixel(160,160,160);
        pge->DrawString(textX, textY, full, col, txtScale);
    }

    // FPS: piccolo testo dopo l'ultima tab se c'e' spazio
    // altrimenti nell'angolo in basso dell'ultima tab
    std::string fpsStr = std::to_string((int)state.fps) + "fps";
    olc::Pixel fpsCol  = state.fps > 50 ? COL_GREEN() :
                         state.fps > 25 ? COL_YELLOW() : olc::RED;
    int lastTabEnd = N * tabW;
    if (lastTabEnd + (int)fpsStr.size() * 6 + 4 <= sw) {
        pge->DrawString(lastTabEnd + 4, textY, fpsStr, fpsCol);
    } else {
        // Sovrapponi nell'angolo basso dell'ultima tab
        pge->DrawString(sw - (int)fpsStr.size()*6 - 2,
                        TOPBAR_H - 10, fpsStr, fpsCol);
    }
}

void UIRenderer::drawScrollbarH(olc::PixelGameEngine* pge,
                                 const UIState& state) {
    int y = SCROLLBAR_H_Y, w = CANVAS_W;
    pge->FillRect(0, y, w, SCROLLBAR_H, COL_SCROLLBAR());
    pge->DrawLine(0, y, w, y, COL_BORDER());
    int cw = std::max(20, (int)(w / state.zoomLevel));
    int cx = (int)((w - cw) * state.scrollH);
    pge->FillRect(cx, y+1, cw, SCROLLBAR_H-2, COL_CURSOR());
    pge->DrawRect(cx, y+1, cw, SCROLLBAR_H-2, olc::Pixel(120,180,255));
}

void UIRenderer::drawScrollbarV(olc::PixelGameEngine* pge,
                                 const UIState& state) {
    int x = CANVAS_W, y = CANVAS_Y, h = CANVAS_H;
    pge->FillRect(x, y, SCROLLBAR_V_W, h, COL_SCROLLBAR());
    pge->DrawLine(x, y, x, y+h, COL_BORDER());
    int ch = std::max(20, (int)(h / state.zoomLevel));
    int cy = y + (int)((h - ch) * state.scrollV);
    pge->FillRect(x+1, cy, SCROLLBAR_V_W-2, ch, COL_CURSOR());
    pge->DrawRect(x+1, cy, SCROLLBAR_V_W-2, ch, olc::Pixel(120,180,255));
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
        pge->DrawString(6, y+6,
                        "FILE: " + state.filepath + "  [" + sizeStr + "]",
                        COL_CYAN());
    }
    if (!state.byteInfo.empty())
        pge->DrawString(sw-260, y+6, state.byteInfo, COL_YELLOW());
}

void UIRenderer::drawFileNav(olc::PixelGameEngine* pge,
                              const UIState& state) {
    int sw = pge->ScreenWidth();
    int y  = FILE_NAV_Y;
    pge->FillRect(0, y, sw, FILE_NAV_H, COL_SCROLLBAR());
    pge->DrawLine(0, y, sw, y, COL_BORDER());
    int cw = std::max(12, (int)(sw * state.fileNavRatio));
    int cx = (int)((sw - cw) * state.fileNavPos);
    pge->FillRect(cx, y+1, cw, FILE_NAV_H-2, COL_CURSOR());
    pge->DrawRect(cx, y+1, cw, FILE_NAV_H-2, olc::Pixel(120,180,255));
    if (!state.position.empty()) {
        int tx = sw/2 - (int)(state.position.size()*3);
        pge->DrawString(tx, y+2, state.position, olc::Pixel(200,200,100));
    }
}

void UIRenderer::drawCmdStatus(olc::PixelGameEngine* pge,
                                const UIState& state) {
    int sw = pge->ScreenWidth();
    int y  = CMD_STATUS_Y;
    pge->FillRect(0, y, sw, CMD_STATUS_H, COL_BG());
    pge->DrawLine(0, y, sw, y, COL_BORDER());
    if (!state.cmdStatus.empty())
        pge->DrawString(6, y+5, state.cmdStatus, olc::WHITE);
    std::string cmds = "O=Apri  S=Salva  Frecce=Naviga  Home=Reset  Rotella=Zoom";
    int cmdsX = sw - (int)(cmds.size()*6) - 6;
    if (cmdsX > 200)
        pge->DrawString(cmdsX, y+5, cmds, olc::Pixel(140,140,140));
}

void UIRenderer::drawSectionPanel(olc::PixelGameEngine* pge,
                                   const UIState& state) {
    if (state.hoveredSection < 0) return;
    if (state.hoveredSection >= (int)state.sections.size()) return;

    const auto& sec = state.sections[state.hoveredSection];

    int sw = pge->ScreenWidth();
    int sh = pge->ScreenHeight();

    // Posizione tooltip vicino al mouse
    int tx = state.mouseX + 12;
    int ty = state.mouseY - 10;

    std::string line1 = sec.name;
    std::string line2 = "OFF: 0x" + sec.offsetHex;
    std::string line3 = "SZ:  "   + sec.sizeStr;
    std::string line4 = "Click = vai alla sezione";

    int maxLen = std::max({(int)line1.size(), (int)line2.size(),
                           (int)line3.size(), (int)line4.size()});
    int boxW = maxLen * 6 + 12;
    int boxH = 52;

    // Mantieni dentro lo schermo
    if (tx + boxW > sw) tx = state.mouseX - boxW - 6;
    if (tx < 0)         tx = 2;
    if (ty + boxH > sh) ty = state.mouseY - boxH - 6;
    if (ty < TOPBAR_H)  ty = TOPBAR_H + 4;

    // Sfondo + bordo colorato dalla sezione
    pge->FillRect(tx, ty, boxW, boxH, olc::Pixel(15,15,40,240));
    pge->DrawRect(tx, ty, boxW, boxH, olc::Pixel(sec.r, sec.g, sec.b));

    // Separatore sotto il nome
    pge->DrawLine(tx+1, ty+13, tx+boxW-1, ty+13,
                  olc::Pixel(sec.r/2, sec.g/2, sec.b/2));

    pge->DrawString(tx+4, ty+3,  line1, olc::Pixel(sec.r, sec.g, sec.b));
    pge->DrawString(tx+4, ty+16, line2, olc::WHITE);
    pge->DrawString(tx+4, ty+26, line3, olc::Pixel(180,180,180));
    pge->DrawString(tx+4, ty+38, line4, olc::Pixel(100,200,100));
}

void UIRenderer::draw3DOverlay(olc::PixelGameEngine* pge,
                                const UIState& state) {
    int sw = pge->ScreenWidth();
    int bx = sw-180, by = TOPBAR_H+8;
    pge->FillRect(bx, by, 172, 76, olc::Pixel(18,18,45,200));
    pge->DrawRect(bx, by, 172, 76, COL_BORDER());
    pge->DrawString(bx+8, by+6,  "3D Controls:",          COL_YELLOW());
    pge->DrawString(bx+8, by+18, "LMB Drag = Ruota",      olc::WHITE);
    pge->DrawString(bx+8, by+30, "RMB Drag = Pan",        olc::WHITE);
    pge->DrawString(bx+8, by+42, "WASD=Muovi  QE=Su/Giu", COL_GREEN());
    pge->DrawString(bx+8, by+54, "Scroll=Zoom  R=Reset",  olc::Pixel(100,200,100));
}

void UIRenderer::drawInputOverlay(olc::PixelGameEngine* pge,
                                   const UIState& state) {
    int sw = pge->ScreenWidth();
    int sh = pge->ScreenHeight();
    int bx = 30, by = sh/2-40, bw = sw-60, bh = 80;
    pge->FillRect(bx, by, bw, bh, olc::Pixel(12,12,35));
    pge->DrawRect(bx, by, bw, bh, COL_YELLOW());
    pge->DrawString(bx+10, by+10,
                    "Apri file  (ENTER=conferma  ESC=annulla  NP_DIV=/)",
                    COL_YELLOW(), 2);
    pge->DrawLine(bx+6, by+32, bx+bw-6, by+32, COL_BORDER());
    std::string display = state.inputBuffer;
    if (std::fmod(state.cursorBlink, 1.0f) < 0.5f) display += "_";
    pge->DrawString(bx+10, by+42, display, olc::WHITE, 2);
}
