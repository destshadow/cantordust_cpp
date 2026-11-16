#include "visualizer.h"

// La riga sopra la vista: cosa indicano gli assi e il colore. Il testo di
// ogni vista e' nella tabella (visualizer_views.cpp).

void Visualizer::drawLegend() {
    FillRect(CANVAS_X, LEGEND_Y, CANVAS_W + SCROLLBAR_V_W, LEGEND_H, COL_BG());
    DrawLine(CANVAS_X, LEGEND_Y + LEGEND_H - 1,
             CANVAS_X + CANVAS_W + SCROLLBAR_V_W, LEGEND_Y + LEGEND_H - 1, COL_BORDER());
    const std::string s = view().legend(*this);
    int room = (CANVAS_W + SCROLLBAR_V_W - 12) / 8;
    DrawString(CANVAS_X + 6, LEGEND_Y + 4, s.substr(0, room), COL_TEXT());
}
