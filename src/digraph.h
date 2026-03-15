#pragma once
#include <vector>
#include <cstdint>
#include <array>

// Canvas 256x256 — ogni cella conta le occorrenze della coppia (x,y)
using DiGraphCanvas = std::array<std::array<uint32_t, 256>, 256>;

class DiGraph {
public:
    void compute(const std::vector<uint8_t>& bytes);
    void clear();

    // Restituisce il valore grezzo di una cella
    uint32_t get(uint8_t x, uint8_t y) const { return m_canvas[y][x]; }

    // Restituisce il massimo valore presente (per normalizzare)
    uint32_t getMax() const { return m_max; }

    // Genera buffer RGB 256x256 pronto per stb/olcPGE
    std::vector<uint8_t> toRGB() const;

private:
    DiGraphCanvas m_canvas{};
    uint32_t m_max = 0;
};
