#include "visualizer.h"

// --- Disegna overlay sezioni sul canvas 2D ---
// Per ogni sezione disegniamo due linee:
// - Verticale a x = (offset / filesize) * 256
//   → dove inizia la sezione sull'asse X del digraph
// - Orizzontale a y = stessa posizione
//   → simmetria del digraph
// Questo perché nel digraph (x,y) = (byte[i], byte[i+1])
// I byte di una sezione cadono in una fascia diagonale
void Visualizer::drawSectionOverlay() {
    if (!m_parser.hasSections()) return;
    if (m_mode == ViewMode::TRIGRAPH3D) return;

    size_t total = m_reader.getSize();
    if (total == 0) return;

    float drawX, drawY, scale;
    // Recupera la trasformazione corrente del canvas
    // (stessa funzione usata in visualizer_2d.cpp)
    {
        float scaledSize = 256.0f * m_zoom2d;
        float maxOffX = std::max(0.0f, scaledSize - (float)CANVAS_W);
        float maxOffY = std::max(0.0f, scaledSize - (float)CANVAS_H);
        drawX = (scaledSize <= (float)CANVAS_W)
                ? CANVAS_X + ((float)CANVAS_W - scaledSize) * 0.5f
                : (float)CANVAS_X - m_scrollH * maxOffX;
        drawY = (scaledSize <= (float)CANVAS_H)
                ? CANVAS_Y + ((float)CANVAS_H - scaledSize) * 0.5f
                : (float)CANVAS_Y - m_scrollV * maxOffY;
        scale = m_zoom2d;
    }

    for (const auto& sec : m_parser.getSections()) {
        // Normalizza l'offset della sezione in [0, 255]
        float norm = (float)sec.offset / (float)total;
        float px   = norm * 256.0f * scale;

        // Posizione sullo schermo
        int screenX = (int)(drawX + px);
        int screenY = (int)(drawY + px);

        olc::Pixel col(sec.r, sec.g, sec.b, 180);

        // Linea verticale (x = offset sezione)
        if (screenX >= CANVAS_X && screenX < CANVAS_X + CANVAS_W)
            DrawLine(screenX, CANVAS_Y,
                     screenX, CANVAS_Y + CANVAS_H - 1, col);

        // Linea orizzontale (y = offset sezione)
        if (screenY >= CANVAS_Y && screenY < CANVAS_Y + CANVAS_H)
            DrawLine(CANVAS_X,             screenY,
                     CANVAS_X + CANVAS_W - 1, screenY, col);

        // Label sezione sopra la linea verticale
        if (screenX >= CANVAS_X && screenX < CANVAS_X + CANVAS_W - 20) {
            // Sfondo piccolo per leggibilità
            FillRect(screenX + 2, CANVAS_Y + 2, 
                     (int)sec.name.size() * 6 + 2, 10,
                     olc::Pixel(0, 0, 0, 160));
            DrawString(screenX + 3, CANVAS_Y + 3,
                       sec.name, col);
        }
    }
}
