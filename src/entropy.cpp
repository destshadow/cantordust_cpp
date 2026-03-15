#include "entropy.h"
#include <cmath>
#include <algorithm>
#include <array>

void Entropy::clear() {
    m_values.clear();
}

float Entropy::shannonEntropy(const std::vector<uint8_t>& bytes,
                               size_t start, size_t len) const {
    // Conta le occorrenze di ogni byte value (0-255) nella finestra
    std::array<uint32_t, 256> freq{};
    freq.fill(0);

    for (size_t i = start; i < start + len && i < bytes.size(); i++)
        freq[bytes[i]]++;

    // Calcola H = -sum(p * log2(p))
    float entropy = 0.0f;
    float total = static_cast<float>(len);

    for (int b = 0; b < 256; b++) {
        if (freq[b] == 0) continue;
        float p = freq[b] / total;      // probabilita' del byte b
        entropy -= p * std::log2(p);    // contributo all'entropia
    }

    // Normalizza: max entropia e' log2(256) = 8.0
    return entropy / 8.0f;
}

void Entropy::compute(const std::vector<uint8_t>& bytes, size_t window_size) {
    clear();
    if (bytes.empty()) return;

    // Scorri il file a finestre non sovrapposte
    // Ogni finestra produce un valore di entropia
    for (size_t i = 0; i < bytes.size(); i += window_size) {
        size_t len = std::min(window_size, bytes.size() - i);
        m_values.push_back(shannonEntropy(bytes, i, len));
    }
}

std::vector<uint8_t> Entropy::toRGB() const {
    std::vector<uint8_t> rgb(256 * 256 * 3, 0);
    if (m_values.empty()) return rgb;

    // Disegna sfondo scuro con griglia
    for (int y = 0; y < 256; y++) {
        for (int x = 0; x < 256; x++) {
            int idx = (y * 256 + x) * 3;
            // Linea tratteggiata a meta' (entropia 0.5 = soglia)
            if (y == 128)
                rgb[idx] = rgb[idx+1] = rgb[idx+2] = 40;
        }
    }

    // Mappa i valori di entropia sull'asse X (256 colonne)
    size_t n = m_values.size();
    for (size_t i = 0; i < n; i++) {
        // X = posizione normalizzata in [0, 255]
        int x = static_cast<int>((i * 255) / n);

        // Y = entropia normalizzata invertita (0 in alto = alta entropia)
        // entropia 1.0 (massima) → y = 0 (in cima)
        // entropia 0.0 (minima)  → y = 255 (in fondo)
        int y = static_cast<int>((1.0f - m_values[i]) * 255.0f);
        y = std::max(0, std::min(255, y));

        // Disegna la colonna verticale dal punto y fino in fondo
        // Effetto "riempimento" come un grafico a barre
        for (int py = y; py < 256; py++) {
            int idx = (py * 256 + x) * 3;
            // Colore: caldo (rosso) per alta entropia, freddo (blu) per bassa
            float e = m_values[i];
            rgb[idx]     = static_cast<uint8_t>(e * 255);         // R
            rgb[idx + 1] = static_cast<uint8_t>((1.0f-e) * 128); // G
            rgb[idx + 2] = static_cast<uint8_t>((1.0f-e) * 255); // B
        }
    }
    return rgb;
}
