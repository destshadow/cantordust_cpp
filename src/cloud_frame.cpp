#include "cloud_frame.h"
#include "density.h"
#include <algorithm>

void CloudFrame::resize(int w, int h) {
    if (w == m_w && h == m_h) return;
    m_w = w;
    m_h = h;
    m_cells.assign((size_t)w * h, Cell{});
    m_pixels.assign((size_t)w * h, 0xFF000000u);
}

void CloudFrame::clear() {
    std::fill(m_cells.begin(), m_cells.end(), Cell{});
}

void CloudFrame::resolve() {
    const size_t n = m_cells.size();
    std::vector<uint32_t> weights(n);
    for (size_t i = 0; i < n; i++) weights[i] = m_cells[i].weight;
    const float ref = Density::reference(weights.data(), n);

    for (size_t i = 0; i < n; i++) {
        const Cell& c = m_cells[i];
        if (c.weight == 0) { m_pixels[i] = 0xFF000000u; continue; }
        // Il minimo tiene visibile anche un punto isolato: nel 3D i punti
        // rari sono spesso quelli interessanti
        float k = (0.35f + 0.65f * Density::intensity(c.weight, ref) / 255.0f)
                  / (float)c.weight;
        uint32_t r = (uint32_t)std::min(255.0f, c.r * k);
        uint32_t g = (uint32_t)std::min(255.0f, c.g * k);
        uint32_t b = (uint32_t)std::min(255.0f, c.b * k);
        m_pixels[i] = 0xFF000000u | (b << 16) | (g << 8) | r;
    }
}

uint32_t CloudFrame::keyNear(int x, int y, int radius) const {
    uint32_t best = NO_KEY;
    int bestDist = radius * radius + 1;
    for (int dy = -radius; dy <= radius; dy++)
        for (int dx = -radius; dx <= radius; dx++) {
            int px = x + dx, py = y + dy, d = dx * dx + dy * dy;
            if (px < 0 || py < 0 || px >= m_w || py >= m_h || d >= bestDist) continue;
            const Cell& c = m_cells[(size_t)py * m_w + px];
            if (c.weight == 0) continue;
            best = c.key;
            bestDist = d;
        }
    return best;
}
