#include "visualizer.h"

// La riga sopra la vista: cosa indicano gli assi e il colore.

static const char* legendFor(ViewMode m) {
    switch (m) {
        case ViewMode::DIGRAPH:
            return "DIGRAPH  X = byte i   Y = byte i+1   luminosita' = quante volte (log)";
        case ViewMode::DOTPLOT:
            return "DOT PLOT  X = posizione nella finestra   Y = valore del byte";
        case ViewMode::ENTROPY:
            return "ENTROPIA  X = posizione   altezza = entropia (0-8 bit)   rosso = alta";
        case ViewMode::HISTOGRAM:
            return "ISTOGRAMMA  X = valore del byte 0x00-0xFF   altezza = frequenza (log)";
        case ViewMode::TRIGRAPH3D:
            return "3D  punto = 3 byte consecutivi   luce = densita'   C = colore   click = dove compare";
        case ViewMode::RAWPIXELS:
            return "RAW  i byte come pixel, riga per riga   bpp: [ ]   larghezza: , .   filtro: F";
        case ViewMode::METRICMAP:
            return "METRIC MAP  curva: byte vicini nel file = pixel vicini   colore: C   curva: V";
        case ViewMode::BYTECLOUD:
            return "BYTE CLOUD  griglia 16x16 = valori 0x00-0xFF   grandezza = frequenza";
        case ViewMode::ONETUPLE:
            return "ONE TUPLE  X = valore del byte   Y = tratto del file (256 righe)   luminosita' = frequenza nella riga";
    }
    return "";
}

void Visualizer::drawLegend() {
    FillRect(CANVAS_X, LEGEND_Y, CANVAS_W + SCROLLBAR_V_W, LEGEND_H, COL_BG());
    DrawLine(CANVAS_X, LEGEND_Y + LEGEND_H - 1,
             CANVAS_X + CANVAS_W + SCROLLBAR_V_W, LEGEND_Y + LEGEND_H - 1, COL_BORDER());
    std::string s = legendFor(m_mode);
    if (m_mode == ViewMode::METRICMAP && m_colorMode == ColorMode::BYTE_CLASS)
        s = "METRIC MAP  grigio=zero  verde=controllo  blu=testo  rosso=alti  bianco=0xFF";
    int room = (CANVAS_W + SCROLLBAR_V_W - 12) / 8;
    DrawString(CANVAS_X + 6, LEGEND_Y + 4, s.substr(0, room), olc::Pixel(200, 200, 140));
}
