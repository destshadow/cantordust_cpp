#pragma once
#include <string>
#include <vector>
#include "input_handler.h"
#include "constants.h"

// Info sezione per il tooltip
struct SectionInfo {
    std::string name;
    std::string offsetHex;
    std::string sizeStr;
    int screenX;   // posizione X della linea sullo schermo
    int screenY;   // posizione Y della linea sullo schermo
    uint8_t r, g, b;
};

struct UIState {
    ViewMode    currentView;
    std::string cmdStatus;
    std::string filepath;
    size_t      filesize    = 0;
    std::string byteInfo;
    std::string position;
    float       fileNavPos  = 0.0f;
    float       fileNavRatio = 1.0f;
    float       fps         = 0.0f;
    float       zoomLevel   = 1.0f;
    float       scrollH     = 0.0f;
    float       scrollV     = 0.0f;
    bool        inputMode   = false;
    std::string inputBuffer;
    float       cursorBlink = 0.0f;

    // Sezioni interattive
    std::vector<SectionInfo> sections;
    int  hoveredSection = -1;  // -1 = nessuno
    int  mouseX = 0;
    int  mouseY = 0;
};

class UIRenderer {
public:
    void draw(olc::PixelGameEngine* pge, const UIState& state);

private:
    void drawTopBar(olc::PixelGameEngine* pge,     const UIState& state);
    void drawScrollbarH(olc::PixelGameEngine* pge, const UIState& state);
    void drawScrollbarV(olc::PixelGameEngine* pge, const UIState& state);
    void drawFileStatus(olc::PixelGameEngine* pge, const UIState& state);
    void drawFileNav(olc::PixelGameEngine* pge,    const UIState& state);
    void drawCmdStatus(olc::PixelGameEngine* pge,  const UIState& state);
    void drawSectionPanel(olc::PixelGameEngine* pge, const UIState& state);
    void draw3DOverlay(olc::PixelGameEngine* pge,  const UIState& state);
    void drawInputOverlay(olc::PixelGameEngine* pge, const UIState& state);
};
