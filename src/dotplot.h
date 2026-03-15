#pragma once
#include <vector>
#include <array>
#include <cstdint>

class DotPlot {
public:
    void compute(const std::vector<uint8_t>& bytes);
    void clear();
    std::vector<uint8_t> toRGB() const;

private:
    std::array<std::array<uint32_t, 256>, 256> m_canvas{};
    uint32_t m_max = 0;
};
