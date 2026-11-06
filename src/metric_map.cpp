#include "metric_map.h"
#include <cmath>
#include <algorithm>
#include <array>

MetricMap::MetricMap() {}

void MetricMap::clear() {
    m_rgb.clear();
    m_indexMap.clear();
}

// Entropia della finestra di 32 byte centrata su idx, normalizzata in [0,1].
// Con 32 campioni il massimo possibile e' log2(32) = 5 bit (non 8), quindi
// la normalizzazione usa il numero di campioni. La finestra resta piena
// anche ai bordi; solo un file piu' corto di 32 byte la riduce.
float MetricMap::localEntropy(const std::vector<uint8_t>& bytes,
                               size_t idx) const {
    const size_t WIN = std::min<size_t>(32, bytes.size());
    if (WIN < 2) return 0.0f;
    size_t start = (idx >= WIN/2) ? idx - WIN/2 : 0;
    start = std::min(start, bytes.size() - WIN);

    std::array<uint32_t, 256> freq{};
    for (size_t i = start; i < start + WIN; i++)
        freq[bytes[i]]++;
    float entropy = 0.0f;
    for (int b = 0; b < 256; b++) {
        if (freq[b] == 0) continue;
        float p = (float)freq[b] / (float)WIN;
        entropy -= p * std::log2(p);
    }
    return entropy / std::log2((float)WIN);
}

void MetricMap::compute(const std::vector<uint8_t>& bytes,
                         ColorMode  colorMode,
                         CurveMode  curveMode,
                         int        order,
                         const ClassifierData* classData) {
    clear();
    if (bytes.empty()) return;

    m_size      = 1 << order;  // 2^order

    int totalPixels = m_size * m_size;
    m_rgb.assign(totalPixels * 3, 0);
    m_indexMap.assign(totalPixels, SIZE_MAX);   // SIZE_MAX = nessun byte

    // double: con float l'offset sbaglia di qualche byte sui file grandi
    double step = (bytes.size() >= (size_t)totalPixels)
                  ? (double)bytes.size() / (double)totalPixels
                  : 1.0;

    // Lambda che mappa indice → (px, py) in base alla curva scelta
    auto getPoint = [&](int i) -> std::pair<int,int> {
        switch (curveMode) {
            case CurveMode::HILBERT: {
                HilbertCurve hc(order);
                return hc.indexToPoint(i);
            }
            case CurveMode::ZORDER: {
                ZOrderCurve zc(order);
                return zc.indexToPoint(i);
            }
            case CurveMode::LINEAR: {
                LinearCurve lc(m_size);
                return lc.indexToPoint(i);
            }
        }
        return {i % m_size, i / m_size};
    };

    for (int i = 0; i < totalPixels; i++) {
        // File piu' piccolo del canvas: i pixel restanti restano neri
        if (bytes.size() < (size_t)totalPixels && (size_t)i >= bytes.size())
            break;
        size_t fileIdx = std::min((size_t)(i * step),
                                   bytes.size() - 1);
        auto [px, py] = getPoint(i);

        WavelengthRGB::RGB col;

        if (colorMode == ColorMode::ENTROPY) {
            float e = localEntropy(bytes, fileIdx);
            float curve_val = 0.0f;
            if (e > 0.5f) {
                float v = e - 0.5f;
                float f = 4.0f*v - 4.0f*v*v;
                curve_val = f*f*f*f;
            }
            col.r = (uint8_t)(255.0f * curve_val);
            col.g = 0;
            col.b = (uint8_t)(255.0f * e * e);
        } else {
            col = colorForByte(bytes[fileIdx],
                               fileIdx, bytes.size(),
                               colorMode, classData);
        }

        int bufIdx = (py * m_size + px) * 3;
        if (bufIdx + 2 < (int)m_rgb.size()) {
            m_rgb[bufIdx]     = col.r;
            m_rgb[bufIdx + 1] = col.g;
            m_rgb[bufIdx + 2] = col.b;
            m_indexMap[py * m_size + px] = fileIdx;
        }
    }
}

size_t MetricMap::fileIndexAt(int x, int y) const {
    if (m_indexMap.empty() || x < 0 || x >= m_size || y < 0 || y >= m_size)
        return SIZE_MAX;
    return m_indexMap[y * m_size + x];
}
