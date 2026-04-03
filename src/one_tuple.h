#pragma once
#include <vector>
#include <array>
#include <cstdint>

// OneTuple: gradient plot 256x256
// Asse X = valore byte (0-255)
// Asse Y = gruppo di scansione (riga)
//
// Per ogni gruppo di 256*groupLines byte:
//   Conta quante volte appare ogni valore byte
//   Disegna una riga colorata proporzionale alla frequenza
//
// Basato su OneTupleVisualizer.java del codice originale
class OneTuple {
public:
    // Calcola il gradient plot
    // groupLines: quante "righe" di dati per riga del canvas
    void compute(const std::vector<uint8_t>& bytes,
                 size_t start = 0, size_t end = 0,
                 int groupLines = 16);
    void clear();

    std::vector<uint8_t> toRGB() const;

    // Cambia il colore di base (R, G o B)
    void setColor(uint8_t r, uint8_t g, uint8_t b) {
        m_r = r; m_g = g; m_b = b;
    }

private:
    // Canvas 256x256 — ogni pixel ha un valore di intensita'
    std::array<uint8_t, 256*256> m_canvas{};
    uint8_t m_r = 0, m_g = 255, m_b = 0;  // verde di default
};
