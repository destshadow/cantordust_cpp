#pragma once
#include <vector>
#include <cstdint>

class Entropy {
public:
    // Calcola entropia su finestre scorrevoli
    // window_size: quanti byte per finestra (default 256)
    void compute(const std::vector<uint8_t>& bytes, size_t window_size = 256);
    void clear();

    // Valori di entropia normalizzati [0.0, 1.0] per ogni finestra
    const std::vector<float>& getValues() const { return m_values; }

    // Genera buffer RGB 256x256 — grafico a linea
    std::vector<uint8_t> toRGB() const;

private:
    // Calcola entropia di Shannon su un blocco di bytes
    // Ritorna valore in [0.0, 8.0] (max = log2(256))
    float shannonEntropy(const std::vector<uint8_t>& bytes,
                         size_t start, size_t len) const;

    std::vector<float> m_values; // entropia normalizzata per ogni finestra
};
