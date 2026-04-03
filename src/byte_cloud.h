#pragma once
#include <vector>
#include <array>
#include <cstdint>
#include <string>

// ByteCloud: griglia 16x16 di tutti i 256 byte possibili
// Ogni cella mostra il valore hex del byte
// La dimensione e la luminosita' sono proporzionali alla frequenza
// Byte rari = piccoli/invisibili, byte frequenti = grandi e luminosi
//
// Basato su ByteCloudVisualizer.java del codice originale CantorDust
class ByteCloud {
public:
    // Calcola le frequenze dei byte nella finestra [low, high)
    // maxSamples: quanti byte campionare (default 9999 come Java originale)
    void compute(const std::vector<uint8_t>& bytes,
                 size_t start = 0, size_t end = 0,
                 size_t maxSamples = 9999);
    void clear();

    // Genera buffer RGB 256x256
    // La griglia 16x16 viene disegnata su canvas 256x256
    // ogni cella e' 16x16 pixel
    std::vector<uint8_t> toRGB() const;

    // Frequenza normalizzata [0,1] per ogni byte
    float getFreq(uint8_t b) const { return m_freq[b]; }

private:
    // Disegna un carattere esadecimale su canvas RGB
    // Gestisce font 5x7 pixel minimal
    void drawChar(std::vector<uint8_t>& rgb,
                  int x, int y,
                  char c,
                  uint8_t r, uint8_t g, uint8_t b,
                  float scale) const;

    // Font bitmap 5x7 per caratteri hex (0-9, A-F)
    static const uint8_t FONT_5X7[16][7];

    std::array<float, 256> m_freq{};  // frequenza normalizzata
    uint8_t m_maxFreqByte = 0;         // byte con frequenza massima
};
