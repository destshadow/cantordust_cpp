#include "byte_cloud.h"
#include <algorithm>
#include <cmath>
#include <cstring>

// Font bitmap 5x7 per cifre hex 0-9 e lettere A-F
// Ogni riga e' una maschera di bit per i 5 pixel orizzontali
// 1 = pixel acceso, 0 = pixel spento
const uint8_t ByteCloud::FONT_5X7[16][7] = {
    {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}, // 0
    {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}, // 1
    {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}, // 2
    {0x1F,0x02,0x04,0x02,0x01,0x11,0x0E}, // 3
    {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}, // 4
    {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E}, // 5
    {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}, // 6
    {0x1F,0x01,0x02,0x04,0x08,0x08,0x08}, // 7
    {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}, // 8
    {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C}, // 9
    {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}, // A
    {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}, // B
    {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}, // C
    {0x1E,0x09,0x09,0x09,0x09,0x09,0x1E}, // D
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}, // E
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}, // F
};

void ByteCloud::clear() {
    m_freq.fill(0.0f);
    m_maxFreqByte = 0;
}

void ByteCloud::compute(const std::vector<uint8_t>& bytes,
                         size_t start, size_t end,
                         size_t maxSamples) {
    clear();
    if (bytes.empty()) return;

    if (end == 0 || end > bytes.size())
        end = bytes.size();

    // Conta le frequenze grezze
    // Escludiamo 0x00 e 0xFF come nel codice Java originale
    // perche' tendono a dominare e nascondere gli altri
    std::array<uint32_t, 256> counts{};
    counts.fill(0);

    size_t limit = std::min(start + maxSamples, end);
    uint32_t maxCount = 0;

    for (size_t i = start; i < limit; i++) {
        uint8_t b = bytes[i];
        if (b != 0x00 && b != 0xFF) {
            counts[b]++;
            if (counts[b] > maxCount) {
                maxCount = counts[b];
                m_maxFreqByte = b;
            }
        }
    }

    // Normalizza in [0, 1]
    if (maxCount == 0) return;
    for (int i = 0; i < 256; i++)
        m_freq[i] = (float)counts[i] / (float)maxCount;
}

// --- toRGB ---
// Disegna la griglia 16x16 su canvas 256x256
// Ogni cella e' 16x16 pixel
// Il testo scala da 0 a 12px in base alla frequenza
std::vector<uint8_t> ByteCloud::toRGB() const {
    // Canvas 256x256 RGB inizializzato a nero
    std::vector<uint8_t> rgb(256 * 256 * 3, 0);

    const int CELL   = 16;  // dimensione cella in pixel
    const int MARGIN = 2;   // margine interno cella

    for (int row = 0; row < 16; row++) {
        for (int col = 0; col < 16; col++) {
            int byteVal = row * 16 + col;
            float freq  = m_freq[byteVal];

            if (freq <= 0.0f) continue;

            // Scala del carattere: 1-3 pixel per bit del font
            // freq alta → scale grande → carattere grande
            float scaleF = freq * 2.5f;
            if (scaleF < 0.2f) scaleF = 0.2f;
            if (scaleF > 2.5f) scaleF = 2.5f;

            // Luminosita': alta frequenza → verde brillante
            uint8_t brightness = (uint8_t)(freq * 255.0f);
            if (brightness < 30) brightness = 30; // minimo visibile

            // Posizione del carattere nella cella
            // Centrato nella cella 16x16
            int cellX = col * CELL + MARGIN;
            int cellY = row * CELL + MARGIN;

            // Primo nibble (alto)
            char c0 = "0123456789ABCDEF"[byteVal >> 4];
            // Secondo nibble (basso)
            char c1 = "0123456789ABCDEF"[byteVal & 0xF];


            // Disegna i due caratteri hex affiancati
            // a scala proporzionale alla frequenza
            drawChar(rgb, cellX,     cellY,
                     c0, 0, brightness, 0, scaleF);
            drawChar(rgb, cellX + 7, cellY,
                     c1, 0, brightness, 0, scaleF);
        }
    }

    return rgb;
}

// --- drawChar ---
// Disegna un singolo carattere hex usando il font 5x7
// x, y = posizione angolo in alto a sinistra
// scale = fattore di scala (1.0 = 5x7 pixel)
void ByteCloud::drawChar(std::vector<uint8_t>& rgb,
                          int x, int y,
                          char c,
                          uint8_t r, uint8_t g, uint8_t b,
                          float scale) const {
    // Trova l'indice nel font
    int fi = -1;
    if (c >= '0' && c <= '9') fi = c - '0';
    else if (c >= 'A' && c <= 'F') fi = c - 'A' + 10;
    else if (c >= 'a' && c <= 'f') fi = c - 'a' + 10;
    if (fi < 0 || fi > 15) return;

    int pixScale = std::max(1, (int)(scale + 0.5f));

    for (int row = 0; row < 7; row++) {
        uint8_t mask = FONT_5X7[fi][row];
        for (int col = 0; col < 5; col++) {
            if (!(mask & (0x10 >> col))) continue;

            // Disegna pixScale x pixScale pixel per ogni bit
            for (int dy = 0; dy < pixScale; dy++) {
                for (int dx = 0; dx < pixScale; dx++) {
                    int px = x + col * pixScale + dx;
                    int py = y + row * pixScale + dy;

                    if (px < 0 || px >= 256) continue;
                    if (py < 0 || py >= 256) continue;

                    int idx = (py * 256 + px) * 3;
                    rgb[idx]     = r;
                    rgb[idx + 1] = g;
                    rgb[idx + 2] = b;
                }
            }
        }
    }
}
