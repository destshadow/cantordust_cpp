#include "metric_map.h"
#include <cmath>
#include <algorithm>
#include <array>

MetricMap::MetricMap() {}

void MetricMap::clear() {
    m_rgb.clear();
    m_indexMap.clear();
}

// --- Entropia locale su finestra di 16 byte ---
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
    return entropy / 8.0f; // normalizza in [0,1]
}

// --- Calcola colore per un byte ---
WavelengthRGB::RGB MetricMap::colorForByte(
    uint8_t byte_val, size_t file_idx,
    size_t total, ColorMode mode) const {

    switch (mode) {
        case ColorMode::WAVELENGTH:
            // byte → lunghezza d'onda arcobaleno
            // 0x00 = viola, 0xFF = rosso
            return WavelengthRGB::byteToRainbow(byte_val);

        case ColorMode::GRADIENT:
            // posizione nel file → gradiente blu→rosso
            {
                float t = (total > 1)
                          ? (float)file_idx / (float)(total-1)
                          : 0.0f;
                return {
                    (uint8_t)(t * 255),
                    0,
                    (uint8_t)((1.0f-t) * 255)
                };
            }

        case ColorMode::SPECTRUM:
            // Usa il valore del byte come "lunghezza d'onda"
            // ma con una mappatura non lineare per evidenziare
            // la struttura dello spazio dei byte
            {
                double wave = 380.0 + (byte_val / 255.0) * 400.0;
                return WavelengthRGB::wavelengthToRGB(wave);
            }

        default:
            return WavelengthRGB::byteToRainbow(byte_val);
    }
}

// --- Compute principale ---
void MetricMap::compute(const std::vector<uint8_t>& bytes,
                         ColorMode mode, int order) {
    clear();
    if (bytes.empty()) return;

    m_mode = mode;
    m_size = 1 << order;  // 2^order

    HilbertCurve hilbert(order);
    int totalPixels = m_size * m_size;

    // Alloca buffer RGB e mappa indici
    m_rgb.assign(m_size * m_size * 3, 0);
    m_indexMap.assign(totalPixels, 0);

    // Step: quanti byte per pixel
    // Se il file e' piu' corto della griglia, mostriamo solo
    // i byte disponibili e lasciamo nero il resto
    float step = (bytes.size() >= (size_t)totalPixels)
                 ? (float)bytes.size() / (float)totalPixels
                 : 1.0f;

    for (int i = 0; i < totalPixels; i++) {
        // Indice nel file corrispondente a questo pixel
        size_t fileIdx = std::min((size_t)(i * step),
                                   bytes.size() - 1);

        // Posizione sulla griglia tramite curva di Hilbert
        auto [px, py] = hilbert.indexToPoint(i);

        // Colore del byte
        WavelengthRGB::RGB col;

        if (mode == ColorMode::ENTROPY) {
            // Per ENTROPY usiamo una logica speciale
            // ispirata a ColorEntropy del codice Java:
            // e > 0.5 → componente rossa (alta entropia)
            // sempre  → componente blu (quadrato dell'entropia)
            float e = localEntropy(bytes, fileIdx);
            float curve_val = 0.0f;
            if (e > 0.5f) {
                float v = e - 0.5f;
                // curve(v) = (4v - 4v²)^4
                float f = 4.0f*v - 4.0f*v*v;
                curve_val = f*f*f*f;
            }
            col.r = (uint8_t)(255.0f * curve_val);
            col.g = 0;
            col.b = (uint8_t)(255.0f * e * e);
        } else {
            col = colorForByte(bytes[fileIdx],
                               fileIdx, bytes.size(), mode);
        }

        // Scrivi nel buffer RGB
        int bufIdx = (py * m_size + px) * 3;
        m_rgb[bufIdx]     = col.r;
        m_rgb[bufIdx + 1] = col.g;
        m_rgb[bufIdx + 2] = col.b;

        // Salva la mappa inversa
        m_indexMap[py * m_size + px] = fileIdx;
    }
}

// --- Indice nel file per coordinate pixel ---
size_t MetricMap::fileIndexAt(int x, int y) const {
    if (x < 0 || x >= m_size || y < 0 || y >= m_size)
        return 0;
    return m_indexMap[y * m_size + x];
}

const char* MetricMap::modeName(ColorMode m) {
    switch (m) {
        case ColorMode::WAVELENGTH: return "Wavelength";
        case ColorMode::ENTROPY:    return "Entropy";
        case ColorMode::GRADIENT:   return "Gradient";
        case ColorMode::SPECTRUM:   return "Spectrum";
    }
    return "???";
}
