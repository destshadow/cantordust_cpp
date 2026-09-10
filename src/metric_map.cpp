#include "metric_map.h"
#include "classifier_model.h"
#include <cmath>
#include <algorithm>
#include <array>

MetricMap::MetricMap() {}

void MetricMap::clear() {
    m_rgb.clear();
    m_indexMap.clear();
}

float MetricMap::localEntropy(const std::vector<uint8_t>& bytes,
                               size_t idx) const {
    const size_t WIN = 16;
    std::array<uint32_t, 256> freq{};
    freq.fill(0);
    size_t start = (idx >= WIN/2) ? idx - WIN/2 : 0;
    size_t end   = std::min(start + WIN, bytes.size());
    size_t count = end - start;
    for (size_t i = start; i < end; i++)
        freq[bytes[i]]++;
    float entropy = 0.0f;
    for (int b = 0; b < 256; b++) {
        if (freq[b] == 0) continue;
        float p = (float)freq[b] / (float)count;
        entropy -= p * std::log2(p);
    }
    return entropy / 4.0f; // Una finestra di 16 campioni ha massimo 4 bit.
}

WavelengthRGB::RGB MetricMap::colorForByte(
    uint8_t byte_val, size_t file_idx,
    size_t total, ColorMode mode,
    const ClassifierData* cd) const {

    switch (mode) {
        case ColorMode::WAVELENGTH:
            return WavelengthRGB::byteToRainbow(byte_val);

        case ColorMode::GRADIENT: {
            float t = (total > 1)
                      ? (float)file_idx / (float)(total-1)
                      : 0.0f;
            return { (uint8_t)(t*255), 0,
                     (uint8_t)((1.0f-t)*255) };
        }

        case ColorMode::SPECTRUM: {
            double wave = 380.0 + (byte_val/255.0) * 400.0;
            return WavelengthRGB::wavelengthToRGB(wave);
        }

        case ColorMode::CLASSIFIER: {
            // Usa la classificazione NGram se disponibile
            // Ogni classe ha un colore distinto tramite WavelengthRGB
            if (cd && cd->classifications && cd->blockSize > 0) {
                size_t blockIdx = file_idx / cd->blockSize;
                if (blockIdx < cd->classifications->size()) {
                    int cls = (*cd->classifications)[blockIdx];
                    if (cls < 0 || cls >= NUM_CLASSES) return {128, 128, 128};
                    // Mappa classe [0,16] → lunghezza d'onda [400,780]
                    double t    = (double)cls / (double)(NUM_CLASSES-1);
                    double wave = 400.0 + t * (780.0 - 400.0);
                    return WavelengthRGB::wavelengthToRGB(wave);
                }
            }
            // Fallback se classificatore non disponibile
            return WavelengthRGB::byteToRainbow(byte_val);
        }

        default:
            return WavelengthRGB::byteToRainbow(byte_val);
    }
}

void MetricMap::compute(const std::vector<uint8_t>& bytes,
                         ColorMode  colorMode,
                         CurveMode  curveMode,
                         int        order,
                         const ClassifierData* classData) {
    clear();
    if (bytes.empty()) return;

    m_colorMode = colorMode;
    m_curveMode = curveMode;
    m_size      = 1 << order;  // 2^order

    int totalPixels = m_size * m_size;
    m_rgb.assign(totalPixels * 3, 0);
    m_indexMap.assign(totalPixels, 0);

    float step = (bytes.size() >= (size_t)totalPixels)
                 ? (float)bytes.size() / (float)totalPixels
                 : 1.0f;

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
        return 0;
    return m_indexMap[y * m_size + x];
}

const char* MetricMap::colorModeName(ColorMode m) {
    switch (m) {
        case ColorMode::WAVELENGTH:  return "Wavelength";
        case ColorMode::ENTROPY:     return "Entropy";
        case ColorMode::GRADIENT:    return "Gradient";
        case ColorMode::SPECTRUM:    return "Spectrum";
        case ColorMode::CLASSIFIER:  return "Classifier";
    }
    return "???";
}

const char* MetricMap::curveModeName(CurveMode m) {
    switch (m) {
        case CurveMode::HILBERT: return "Hilbert";
        case CurveMode::ZORDER:  return "Z-Order";
        case CurveMode::LINEAR:  return "Linear";
    }
    return "???";
}
