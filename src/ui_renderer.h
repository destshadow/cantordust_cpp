#pragma once
#include <string>
#include "input_handler.h"
#include "constants.h"

struct UIState {
    ViewMode    currentView;

    // Barra FILE_STATUS
    std::string filepath;
    size_t      filesize    = 0;
    std::string byteInfo;       // "BYTE: 0x1A2B3C = 0xFF (255)"

    // Barra FILE_NAV (scrollbar file)
    std::string position;       // "0x00001A2B - 0x00003C4D (file intero)"
    float       fileNavPos = 0.0f;
    float       fileNavRatio = 1.0f;

    // Barra CMD_STATUS
    std::string cmdStatus;      // messaggio operazione corrente

    // FPS
    float fps = 0.0f;

    // Scrollbar canvas 2D
    float zoomLevel  = 1.0f;
    float scrollH    = 0.0f;   // [0.0, 1.0]
    float scrollV    = 0.0f;   // [0.0, 1.0]

    // Input overlay
    bool        inputMode   = false;
    std::string inputBuffer;
    float       cursorBlink = 0.0f;
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
    void draw3DOverlay(olc::PixelGameEngine* pge,  const UIState& state);
    void drawInputOverlay(olc::PixelGameEngine* pge, const UIState& state);
};
