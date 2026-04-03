#include "one_tuple.h"
#include <algorithm>
#include <cstring>

void OneTuple::clear() {
    m_canvas.fill(0);
}

// --- compute ---
// Algoritmo dal codice Java originale:
//
// Per ogni "riga" i (da 0 a 255):
//   Per ogni j in [0, 256*groupLines):
//     dataIndex = start + i * 256 * groupLines + j
//     p = i * 256 + data[dataIndex]  // posizione nel canvas
//     canvas[p] += 256/groupLines    // accumula intensita'
//
// Il risultato e' un canvas dove:
//   - La colonna X rappresenta il valore del byte
//   - La riga Y rappresenta il "gruppo" di dati
//   - L'intensita' rappresenta la frequenza
void OneTuple::compute(const std::vector<uint8_t>& bytes,
                        size_t start, size_t end,
                        int groupLines) {
    clear();
    if (bytes.empty()) return;
    if (end == 0 || end > bytes.size()) end = bytes.size();
    if (groupLines < 1) groupLines = 1;
    if (groupLines > 128) groupLines = 128;

    int increment = 256 / groupLines;
    if (increment == 0) increment = 1;

    for (int i = 0; i < 256; i++) {
        for (int j = 0; j < 256 * groupLines; j++) {
            size_t dataIdx = start + (size_t)i * 256 * groupLines + j;
            if (dataIdx >= end) break;

            // Posizione nel canvas 256x256
            int p = i * 256 + (bytes[dataIdx] & 0xFF);
            if (p < 0 || p >= 256*256) continue;

            // Accumula intensita' — clamped a 255
            // Se raggiunge il massimo del ciclo successivo
            // viene portato a 255 (saturazione)
            uint8_t current = m_canvas[p];
            int threshold   = (j / 256 + 1) * increment;

            if (current == threshold || current == 255)
                continue;

            int newVal = current + increment;
            m_canvas[p] = (newVal == 0)
                          ? 255
                          : (uint8_t)std::min(newVal, 255);
        }
    }
}

std::vector<uint8_t> OneTuple::toRGB() const {
    std::vector<uint8_t> rgb(256 * 256 * 3, 0);

    for (int i = 0; i < 256 * 256; i++) {
        uint8_t intensity = m_canvas[i];
        if (intensity == 0) continue;

        // Applica il colore base scalato per l'intensita'
        // Sottrae la differenza (256-intensity) da ogni canale
        int diff = 256 - (int)intensity;

        int r = (int)m_r - diff;
        int g = (int)m_g - diff;
        int b = (int)m_b - diff;

        // Clamp a [0, 255]
        r = std::max(0, r);
        g = std::max(0, g);
        b = std::max(0, b);

        int idx = i * 3;
        rgb[idx]     = (uint8_t)r;
        rgb[idx + 1] = (uint8_t)g;
        rgb[idx + 2] = (uint8_t)b;
    }

    return rgb;
}
