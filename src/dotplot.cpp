#include "dotplot.h"
#include "density.h"
#include <cmath>
#include <array>

void DotPlot::clear() {
    for (auto& row : m_canvas)
        row.fill(0);
    m_max = 0;
}

void DotPlot::compute(const std::vector<uint8_t>& bytes) {
    clear();
    if (bytes.empty()) return;

    size_t total = bytes.size();

    for (size_t i = 0; i < total; i++) {
        // X = posizione nel file, sulle 256 colonne
        uint8_t x = static_cast<uint8_t>((i * 256) / total);

        // Y = valore del byte — direttamente 0-255
        uint8_t y = bytes[i];

        m_canvas[y][x]++;
        if (m_canvas[y][x] > m_max)
            m_max = m_canvas[y][x];
    }
}

std::vector<uint8_t> DotPlot::toRGB() const {
    std::vector<uint8_t> rgb(256 * 256 * 3, 0);
    if (m_max == 0) return rgb;

    const float ref = Density::reference(&m_canvas[0][0], 256 * 256);
    for (int y = 0; y < 256; y++) {
        for (int x = 0; x < 256; x++) {
            int idx = (y * 256 + x) * 3;
            rgb[idx + 1] = Density::intensity(m_canvas[y][x], ref);
        }
    }
    return rgb;
}
