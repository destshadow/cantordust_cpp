#pragma once
#include <string>
#include "input_handler.h"
#include "../libs/olcPixelGameEngine.h"

struct UIState {
    ViewMode    currentView;
    std::string status;          // messaggio operazione corrente
    std::string filepath;        // path file caricato
    size_t      filesize  = 0;   // dimensione in bytes
    std::string position;        // "0x00001A2B - 0x00003C4D"
    float       fps       = 0.0f;// frame per secondo
    bool        inputMode = false;
    std::string inputBuffer;
    float       cursorBlink = 0.0f;
};

class UIRenderer {
public:
    void draw(olc::PixelGameEngine* pge, const UIState& state);

private:
    void drawTopBar(olc::PixelGameEngine* pge, const UIState& state);
    void drawBottomBar(olc::PixelGameEngine* pge, const UIState& state);
    void draw3DOverlay(olc::PixelGameEngine* pge, const UIState& state);
    void drawInputOverlay(olc::PixelGameEngine* pge, const UIState& state);
};
