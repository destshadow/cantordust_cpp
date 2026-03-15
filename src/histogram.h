#pragma once
#include <vector>
#include <cstdint>
#include <array>

class Histogram {
public:
    void compute(const std::vector<uint8_t>& bytes);
    void clear();

    // Frequenza grezza di ogni byte value
    uint32_t getFreq(uint8_t b) const { return m_freq[b]; }
    uint32_t getMax() const { return m_max; }

    // Genera buffer RGB 256x256
    std::vector<uint8_t> toRGB() const;

private:
    std::array<uint32_t, 256> m_freq{};
    uint32_t m_max = 0;
};
