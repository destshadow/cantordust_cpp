#include "ui_renderer.h"
#include <cmath>
#include <string>

static const struct TabDef {
    const char* num;
    const char* label;
    ViewMode    mode;
} ALL_TABS[] = {
    {"1", "Digraph",   ViewMode::DIGRAPH   },
    {"2", "DotPlot",   ViewMode::DOTPLOT   },
    {"3", "Entropy",   ViewMode::ENTROPY   },
    {"4", "Histogram", ViewMode::HISTOGRAM },
    {"5", "3D View",   ViewMode::TRIGRAPH3D},
    {"6", "RawPix",    ViewMode::RAWPIXELS },
    {"7", "Metric",    ViewMode::METRICMAP },
    {"8", "ByteCloud", ViewMode::BYTECLOUD },
    {"9", "OneTuple",  ViewMode::ONETUPLE  },
};
static constexpr int TOTAL_TABS  = 9;
static constexpr int TAB_AREA_H  = TOPBAR_H - 6;  // altezza tab
static constexpr int TAB_SCROLL_H = 12;             // altezza scrollbar

void UIRenderer::draw(olc::PixelGameEngine* pge,
                       const UIState& state) {
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

    // Sfondo topbar
    pge->FillRect(0, 0, sw, TOPBAR_H, COL_BG());
    pge->DrawLine(0, TOPBAR_H-1, sw, TOPBAR_H-1, COL_BORDER());

    // Larghezza fissa per ogni tab
    // Con 9 tab su 1024px → ~113px/tab → usiamo 120px
    // L'utente trascina la scrollbar per vedere le tab fuori schermo
    static constexpr int TAB_W = 130;
    int totalTabsW = TOTAL_TABS * TAB_W;  // larghezza totale virtuale

    // Offset in pixel dalla scrollbar
    // tabOffset e' in [0, totalTabsW - sw]
    int maxOffset = std::max(0, totalTabsW - sw);
    int offset    = std::min(state.tabOffset, maxOffset);

    // Scala testo
    int txtScale = (TAB_W >= 100) ? 2 : 1;
    int charH    = 8 * txtScale;
    int textY    = (TAB_AREA_H - charH) / 2 + 1;

    // Disegna tutte le tab — quelle fuori schermo vengono
    // naturalmente clippate da olcPGE
    for (int i = 0; i < TOTAL_TABS; i++) {
        const auto& t  = ALL_TABS[i];
        int  tx        = i * TAB_W - offset;
        bool active    = (state.currentView == t.mode);

        // Skip tab completamente fuori schermo
        if (tx + TAB_W < 0 || tx > sw) continue;

        if (active) {
            pge->FillRect(tx+1, 1, TAB_W-2, TAB_AREA_H-1,
                          COL_BG_LIGHT());
            pge->DrawRect(tx+1, 1, TAB_W-2, TAB_AREA_H-1,
                          COL_YELLOW());
        } else {
            pge->DrawRect(tx+1, 1, TAB_W-2, TAB_AREA_H-1,
                          COL_BORDER());
        }

        // Testo centrato — tronca se necessario
        std::string full = std::string("[") + t.num + "] " + t.label;
        int charW    = 8 * txtScale;
        int maxChars = (TAB_W - 8) / charW;
        if ((int)full.size() > maxChars)
            full = full.substr(0, std::max(0, maxChars));

        int textW = (int)full.size() * charW;
        int textX = tx + (TAB_W - textW) / 2;

        // Clamp textX per non disegnare fuori schermo
        if (textX < 0) textX = 0;
        if (textX + textW > sw) continue;

        olc::Pixel col = active ? COL_YELLOW() : olc::Pixel(160,160,160);
        pge->DrawString(textX, textY, full, col, txtScale);
    }

    // --- Scrollbar orizzontale delle tab ---
    // Stile sottile in basso alla topbar
    int sbY = TOPBAR_H - TAB_SCROLL_H;
    int sbW = sw;

    pge->FillRect(0, sbY, sbW, TAB_SCROLL_H,
                  olc::Pixel(25, 25, 55));
    pge->DrawLine(0, sbY, sbW, sbY, COL_BORDER());

    if (maxOffset > 0) {
        // Cursore proporzionale alla viewport
        float ratio = (float)sw / (float)totalTabsW;
        int   cw    = std::max(30, (int)(sbW * ratio));
        int   cx    = (int)((sbW - cw) *
                            ((float)offset / (float)maxOffset));
        pge->FillRect(cx, sbY+1, cw, TAB_SCROLL_H-2, COL_CURSOR());
        pge->DrawRect(cx, sbY+1, cw, TAB_SCROLL_H-2,
                      olc::Pixel(120,180,255));
    } else {
        // Tutte visibili — barra piena
        pge->FillRect(0, sbY+1, sbW, TAB_SCROLL_H-2, COL_CURSOR());
    }

    // FPS nell'angolo in alto a destra
    std::string fpsStr = std::to_string((int)state.fps) + "fps";
    olc::Pixel  fpsCol = state.fps > 50 ? COL_GREEN() :
                         state.fps > 25 ? COL_YELLOW() : olc::RED;
    int fpsX = sw - (int)fpsStr.size() * 6 - 4;
    // Sfondo piccolo per leggibilità
    pge->FillRect(fpsX - 2, 2,
                  (int)fpsStr.size()*6 + 4, charH + 2,
                  olc::Pixel(0,0,0,180));
    pge->DrawString(fpsX, textY, fpsStr, fpsCol);
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
                        "FILE: " + state.filepath +
                        "  [" + sizeStr + "]", COL_CYAN());
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
        pge->DrawString(tx, y+2, state.position,
                        olc::Pixel(200,200,100));
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
    std::string cmds =
        "O=Apri  S=Salva  Frecce=Naviga  Home=Reset  Rotella=Zoom";
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
    if (tx + boxW > sw) tx = state.mouseX - boxW - 6;
    if (tx < 0)         tx = 2;
    if (ty + boxH > sh) ty = state.mouseY - boxH - 6;
    if (ty < TOPBAR_H)  ty = TOPBAR_H + 4;
    pge->FillRect(tx, ty, boxW, boxH, olc::Pixel(15,15,40,240));
    pge->DrawRect(tx, ty, boxW, boxH,
                  olc::Pixel(sec.r, sec.g, sec.b));
    pge->DrawLine(tx+1, ty+13, tx+boxW-1, ty+13,
                  olc::Pixel(sec.r/2, sec.g/2, sec.b/2));
    pge->DrawString(tx+4, ty+3,  line1,
                    olc::Pixel(sec.r, sec.g, sec.b));
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
    pge->DrawString(bx+8, by+42, "WASD=Muovi QE=Su/Giu",  COL_GREEN());
    pge->DrawString(bx+8, by+54, "Scroll=Zoom  R=Reset",
                    olc::Pixel(100,200,100));
}

void UIRenderer::drawInputOverlay(olc::PixelGameEngine* pge,
                                    const UIState& state) {
    int sw = pge->ScreenWidth();
    int sh = pge->ScreenHeight();
    int bx = 30, by = sh/2-40, bw = sw-60, bh = 80;
    pge->FillRect(bx, by, bw, bh, olc::Pixel(12,12,35));
    pge->DrawRect(bx, by, bw, bh, COL_YELLOW());
    pge->DrawString(bx+10, by+10,
                    "Apri file (ENTER=ok  ESC=annulla  NP_DIV=/)",
                    COL_YELLOW(), 2);
    pge->DrawLine(bx+6, by+32, bx+bw-6, by+32, COL_BORDER());
    std::string display = state.inputBuffer;
    if (std::fmod(state.cursorBlink, 1.0f) < 0.5f) display += "_";
    pge->DrawString(bx+10, by+42, display, olc::WHITE, 2);
}
