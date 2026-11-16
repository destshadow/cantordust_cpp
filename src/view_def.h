#pragma once
#include "view_mode.h"
#include <cstdint>
#include <string>
#include <vector>

class Visualizer;
struct PanelWriter;

// Una vista, descritta in un posto solo: la tabella e' in visualizer_views.cpp.
// Il resto del programma non chiede "che vista e'?", chiede alla vista cosa
// sa fare. Un campo vuoto (nullptr) vuol dire "niente di speciale".
//
// Per aggiungere una vista: una voce in ViewMode, una in VIEW_TABS, una nella
// tabella, il calcolo in view_results.cpp.
struct ViewDef {
    ViewMode mode = ViewMode::DIGRAPH;

    // true: un'immagine nel canvas, con zoom, scorrimento e cursore gia' pronti
    // (serve `rgb`). false: la vista si disegna da sola (serve `draw`).
    bool canvas = true;
    std::vector<uint8_t> (*rgb)(const Visualizer&) = nullptr;
    void (*draw)(Visualizer&) = nullptr;

    // La riga sopra la vista: cosa indicano gli assi e il colore
    std::string (*legend)(const Visualizer&) = nullptr;

    // Mouse e tastiera della vista. Di norma solo a calcolo finito.
    void (*input)(Visualizer&) = nullptr;
    bool inputWhileBusy = false;

    // Un pannello disegnato in piu', sopra il fondo della vista (Raw)
    void (*panel)(Visualizer&) = nullptr;

    // La riga di opzioni in basso e i suoi tasti
    void (*options)(Visualizer&, bool clickable) = nullptr;
    void (*keys)(Visualizer&) = nullptr;

    // Pannello proprio dell'Inspector; false = non c'e' nulla sotto il mouse,
    // si passa a quelli generici
    bool (*inspect)(Visualizer&, PanelWriter&) = nullptr;

    // Appena si entra nella vista
    void (*enter)(Visualizer&) = nullptr;
};
