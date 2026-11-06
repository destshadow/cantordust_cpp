#include "byte_cloud.h"
#include <algorithm>
#include <cmath>

namespace {
// Font bitmap 5x7 per cifre hex 0-9 e lettere A-F
// Ogni riga e' una maschera di bit per i 5 pixel orizzontali
// 1 = pixel acceso, 0 = pixel spento
const uint8_t FONT_5X7[16][7] = {
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

const int GLYPH_W = 5, GLYPH_H = 7, MAX_SCALE = 3;

// Un carattere con ogni punto del font disegnato `scale` x `scale` pixel
void drawDigit(std::vector<uint8_t>& rgb, int x, int y, int digit,
               int scale, uint8_t green) {
    for (int row = 0; row < GLYPH_H * scale; row++)
        for (int col = 0; col < GLYPH_W * scale; col++)
            if (FONT_5X7[digit][row / scale] & (0x10 >> (col / scale)))
                rgb[((size_t)(y + row) * ByteCloud::SIZE + x + col) * 3 + 1] = green;
}
} // namespace

void ByteCloud::clear() {
    m_freq.fill(0.0f);
}

void ByteCloud::compute(const std::vector<uint8_t>& bytes) {
    clear();
    std::array<size_t, 256> counts{};
    for (uint8_t b : bytes) counts[b]++;

    // 00 e FF non entrano nella scala: dominerebbero tutti gli altri
    size_t maxCount = 0;
    for (int i = 1; i < 255; i++) maxCount = std::max(maxCount, counts[i]);

    for (int i = 0; i < 256; i++) {
        if (counts[i] == 0) continue;
        m_freq[i] = maxCount ? std::min(1.0f, (float)counts[i] / maxCount) : 1.0f;
    }
}

std::vector<uint8_t> ByteCloud::toRGB() const {
    std::vector<uint8_t> rgb((size_t)SIZE * SIZE * 3, 0);

    for (int v = 0; v < 256; v++) {
        if (m_freq[v] <= 0.0f) continue;

        // Le frequenze coprono vari ordini di grandezza: con la radice restano
        // visibili anche i valori meno comuni
        const float t = std::sqrt(m_freq[v]);
        const int scale = std::min(MAX_SCALE, 1 + (int)(t * MAX_SCALE));
        const uint8_t green = (uint8_t)(255 * (0.25f + 0.75f * t));

        const int textW = (GLYPH_W * 2 + 1) * scale;
        const int textH = GLYPH_H * scale;
        const int x = (v % 16) * CELL + (CELL - textW) / 2;
        const int y = (v / 16) * CELL + (CELL - textH) / 2;
        drawDigit(rgb, x, y, v >> 4, scale, green);
        drawDigit(rgb, x + (GLYPH_W + 1) * scale, y, v & 0xF, scale, green);
    }
    return rgb;
}
