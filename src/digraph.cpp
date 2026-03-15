#include "digraph.h"
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

    for (int y = 0; y < 256; y++) {
        for (int x = 0; x < 256; x++) {
            // Normalizza 0-255 con scala logaritmica
            // La scala log esalta i pattern anche con poche occorrenze
            float norm = 0.0f;
            if (m_canvas[y][x] > 0)
                norm = std::log1p((float)m_canvas[y][x]) /
                       std::log1p((float)m_max);

            uint8_t intensity = static_cast<uint8_t>(norm * 255.0f);

            int idx = (y * 256 + x) * 3;
            rgb[idx]     = intensity; // R
            rgb[idx + 1] = intensity; // G
            rgb[idx + 2] = intensity; // B
        }
    }

    return rgb;
}
