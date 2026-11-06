#pragma once
#include <vector>
#include <array>
#include <cstdint>

// ByteCloud: griglia 16x16 di tutti i 256 byte possibili. Ogni cella mostra il
// valore in esadecimale: piu' compare, piu' e' grande e luminoso; un valore che
// non compare non si vede.
//
// Basato su ByteCloudVisualizer.java del CantorDust originale. Rispetto a
// quello 00 e FF restano visibili: sono fuori dalla scala (schiaccerebbero
// tutti gli altri), ma se ci sono si disegnano al massimo.
class ByteCloud {
public:
    // Il testo piu' grande deve stare nella cella: per questo la griglia ha un
    // canvas proprio, piu' grande dei 256x256 delle altre viste
    static constexpr int CELL = 40;
    static constexpr int SIZE = CELL * 16;

    // Frequenze dei byte su tutta la finestra
    void compute(const std::vector<uint8_t>& bytes);
    void clear();

    // Buffer RGB SIZE x SIZE
    std::vector<uint8_t> toRGB() const;

    // Frequenza normalizzata [0,1] per ogni byte
    float getFreq(uint8_t b) const { return m_freq[b]; }

    // Il valore disegnato nel pixel (x, y) del canvas
    static uint8_t valueAt(int x, int y) { return (uint8_t)((y / CELL) * 16 + x / CELL); }

private:
    std::array<float, 256> m_freq{};
};
