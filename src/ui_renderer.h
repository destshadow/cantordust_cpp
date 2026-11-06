#pragma once
#include <string>
#include <vector>
#include "input_handler.h"
#include "constants.h"

struct UIState {
    ViewMode    currentView;
    std::string cmdStatus;
    std::string filepath;
    size_t      filesize    = 0;
    std::string byteInfo;
    float       fps         = 0.0f;
    float       zoomLevel   = 1.0f;
    float       scrollH     = 0.0f;
    float       scrollV     = 0.0f;
    bool        inputMode   = false;
    std::string inputBuffer;
    float       cursorBlink = 0.0f;

    // Sezioni interattive
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
    void drawCmdStatus(olc::PixelGameEngine* pge,  const UIState& state);
    void draw3DOverlay(olc::PixelGameEngine* pge,  const UIState& state);
    void drawInputOverlay(olc::PixelGameEngine* pge, const UIState& state);
};
