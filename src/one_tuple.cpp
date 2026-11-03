#include "one_tuple.h"
#include <algorithm>
#include <cmath>
#include <cstring>

void OneTuple::clear() {
    m_canvas.fill(0);
    m_rows = 0;
}

// --- compute ---
// La finestra viene divisa in 256 righe di uguale lunghezza (sempre
// tutta la finestra, qualunque sia la sua dimensione). Per ogni riga:
//   X = valore del byte (0..255)
//   intensita' = quante volte quel valore compare nella riga
//                (scala logaritmica rispetto al valore piu' frequente
//                 della riga, cosi' ogni riga ha il suo contrasto)
// Si legge come "quali valori usa ogni tratto del file": testo, codice,
// zeri e dati casuali hanno firme diverse e si vedono cambiare scendendo.
//
// (La versione precedente, presa dal codice Java, usava righe da 4 KB
// fisse: un file da 138 KB riempiva solo 34 righe su 256.)
void OneTuple::compute(const std::vector<uint8_t>& bytes,
                        size_t start, size_t end,
                        int /*groupLines: non piu' usato*/) {
    clear();
    if (end == 0 || end > bytes.size()) end = bytes.size();
    if (start >= end) return;

    const size_t len = end - start;
    m_rows = (int)std::min<size_t>(256, len);

    for (int r = 0; r < m_rows; r++) {
        size_t a = start + (size_t)r * len / m_rows;
        size_t b = start + (size_t)(r + 1) * len / m_rows;
        std::array<uint32_t, 256> count{};
        uint32_t maxC = 0;
        for (size_t i = a; i < b; i++) maxC = std::max(maxC, ++count[bytes[i]]);
        if (maxC == 0) continue;
        float norm = std::log1p((float)maxC);
        for (int v = 0; v < 256; v++) {
            if (!count[v]) continue;
            // minimo 40: anche un valore visto una volta resta visibile
            float t = std::log1p((float)count[v]) / norm;
            m_canvas[r * 256 + v] = (uint8_t)(40 + t * 215);
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
