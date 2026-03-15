#include "histogram.h"
#include <cmath>
#include <algorithm>

void Histogram::clear() {
    m_freq.fill(0);
    m_max = 0;
}

void Histogram::compute(const std::vector<uint8_t>& bytes) {
    clear();
    if (bytes.empty()) return;

    // Conta quante volte appare ogni valore byte 0-255
    for (uint8_t b : bytes) {
        m_freq[b]++;
        if (m_freq[b] > m_max)
            m_max = m_freq[b];
    }
}

std::vector<uint8_t> Histogram::toRGB() const {
    std::vector<uint8_t> rgb(256 * 256 * 3, 0);
    if (m_max == 0) return rgb;

    // 256 barre verticali, una per ogni byte value (0-255)
    // L'asse X = valore del byte
    // L'asse Y = frequenza (barra che sale dal basso)
    for (int b = 0; b < 256; b++) {
        if (m_freq[b] == 0) continue;

        // Altezza barra normalizzata con scala logaritmica
        float norm = std::log1p((float)m_freq[b]) /
                     std::log1p((float)m_max);
        int barHeight = static_cast<int>(norm * 255.0f);

        // Disegna la barra dal basso verso l'alto
        for (int y = 255; y >= (255 - barHeight); y--) {
            int idx = (y * 256 + b) * 3;

            // Gradiente colore: basso = blu, alto = giallo
            float t = 1.0f - (float)(255 - y) / (float)barHeight;
            rgb[idx]     = static_cast<uint8_t>(t * 255);       // R
            rgb[idx + 1] = static_cast<uint8_t>(t * 200);       // G
            rgb[idx + 2] = static_cast<uint8_t>((1.0f-t)*255);  // B
        }
    }
    return rgb;
}
