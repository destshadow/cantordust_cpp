#include "dotplot.h"
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
        // X = posizione nel file normalizzata in [0, 255]
        // Dividiamo l'offset per la dimensione totale e scaliamo a 256
        uint8_t x = static_cast<uint8_t>((i * 255) / total);

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

    for (int y = 0; y < 256; y++) {
        for (int x = 0; x < 256; x++) {
            // Scala logaritmica: esalta anche i pattern rari
            float norm = 0.0f;
            if (m_canvas[y][x] > 0)
                norm = std::log1p((float)m_canvas[y][x]) /
                       std::log1p((float)m_max);

            uint8_t intensity = static_cast<uint8_t>(norm * 255.0f);

            int idx = (y * 256 + x) * 3;
            // Dot plot in verde per distinguerlo dal digraph
            rgb[idx]     = 0;
            rgb[idx + 1] = intensity;
            rgb[idx + 2] = 0;
        }
    }
    return rgb;
}
