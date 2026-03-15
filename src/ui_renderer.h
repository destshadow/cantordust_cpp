#pragma once
#include <string>
#include "input_handler.h"        // per ViewMode
#include "../libs/olcPixelGameEngine.h"

// Struttura dati che il Visualizer passa all'UIRenderer ogni frame
// E' un semplice contenitore di stato — niente logica
struct UIState {
    ViewMode    currentView;       // per evidenziare la tab attiva
    std::string status;            // messaggio in basso
    std::string filepath;          // path del file caricato
    size_t      filesize = 0;      // dimensione in bytes
    bool        inputMode = false; // overlay aperto?
    std::string inputBuffer;       // testo digitato
    float       cursorBlink = 0.0f;// timer cursore
};

class UIRenderer {
public:
    // Disegna tutta la UI
    // pge = puntatore all'engine per DrawString, FillRect ecc.
    // state = stato corrente passato dal Visualizer
    void draw(olc::PixelGameEngine* pge, const UIState& state);

private:
    // Sotto-metodi: ognuno disegna una parte specifica
    void drawTopBar(olc::PixelGameEngine* pge, const UIState& state);
    void drawBottomBar(olc::PixelGameEngine* pge, const UIState& state);
    void drawInputOverlay(olc::PixelGameEngine* pge, const UIState& state);
};
