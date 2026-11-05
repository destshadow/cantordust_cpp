#include "digraph.h"
#include "density.h"
#include <algorithm>
#include <cmath>

void DiGraph::clear() {
    for (auto& row : m_canvas)
        row.fill(0);
    m_max = 0;
}

void DiGraph::compute(const std::vector<uint8_t>& bytes) {
    clear();

    if (bytes.size() < 2) return;

    // Sliding window: ogni coppia (bytes[i], bytes[i+1]) → punto sul canvas
    for (size_t i = 0; i < bytes.size() - 1; i++) {
        uint8_t x = bytes[i];
        uint8_t y = bytes[i + 1];
        m_canvas[y][x]++;
        if (m_canvas[y][x] > m_max)
            m_max = m_canvas[y][x];
    }
}

std::vector<uint8_t> DiGraph::toRGB() const {
    std::vector<uint8_t> rgb(256 * 256 * 3, 0);
    if (m_max == 0) return rgb;

    const float ref = Density::reference(&m_canvas[0][0], 256 * 256);
    for (int y = 0; y < 256; y++) {
        for (int x = 0; x < 256; x++) {
            uint8_t v = Density::intensity(m_canvas[y][x], ref);
            int idx = (y * 256 + x) * 3;
            rgb[idx]     = v >> 3;   // verde con un filo di bianco
            rgb[idx + 1] = v;
            rgb[idx + 2] = v >> 3;
        }
    }
    return rgb;
}
