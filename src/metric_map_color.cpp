#include "metric_map.h"
#include "byte_info.h"
#include "cancel.h"
#include <cmath>
#include <cstdint>

WavelengthRGB::RGB MetricMap::colorForByte(
    uint8_t byte_val, size_t file_idx,
    size_t total, ColorMode mode) const {

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

        case ColorMode::FREQUENCY:
            return m_freqColor[byte_val];

        case ColorMode::BYTE_CLASS: {
            WavelengthRGB::RGB c{};
            ByteInfo::classColor(ByteInfo::classOf(byte_val), c.r, c.g, c.b);
            return c;
        }

        default:
            return WavelengthRGB::byteToRainbow(byte_val);
    }
}

// Frequenza: si contano i byte della zona e si mette il conteggio su scala
// logaritmica (altrimenti lo zero, che spesso domina, schiaccia tutto il
// resto). La scala va dal valore piu' comune della zona (450 nm, blu) al
// piu' raro fra quelli presenti (650 nm, rosso): un pezzo con byte insoliti
// per quel file risalta in caldo anche se in Wavelength sembra uguale agli
// altri. I colori sono relativi alla zona, non assoluti.
std::array<WavelengthRGB::RGB, 256>
MetricMap::frequencyColors(const std::vector<uint8_t>& bytes) {
    std::array<uint64_t, 256> count{};
    for (size_t i = 0; i < bytes.size(); i++) {
        if (Cancel::every(i)) break;
        count[bytes[i]]++;
    }
    uint64_t most = 0, least = UINT64_MAX;
    for (uint64_t c : count) {
        if (c == 0) continue;
        most  = std::max(most, c);
        least = std::min(least, c);
    }

    std::array<WavelengthRGB::RGB, 256> col{};
    const double lo = least == UINT64_MAX ? 0.0 : std::log((double)least);
    const double span = most > least && least != UINT64_MAX
                      ? std::log((double)most) - lo : 0.0;
    for (int b = 0; b < 256; b++) {
        // Un solo conteggio per tutti (o zona vuota): tutto "comune"
        const double t = span > 0 && count[b] > 0
                       ? (std::log((double)count[b]) - lo) / span : 1.0;
        col[b] = WavelengthRGB::wavelengthToRGB(450.0 + (1.0 - t) * 200.0);
    }
    return col;
}

ColorMode MetricMap::nextColorMode(ColorMode m, bool classifierReady) {
    int next = ((int)m + 1) % 6;
    if (next == (int)ColorMode::CLASSIFIER && !classifierReady) next = 0;
    return (ColorMode)next;
}

const char* MetricMap::colorModeName(ColorMode m) {
    switch (m) {
        case ColorMode::WAVELENGTH:  return "Wavelength";
        case ColorMode::ENTROPY:     return "Entropy";
        case ColorMode::GRADIENT:    return "Gradient";
        case ColorMode::FREQUENCY:   return "Frequenza";
        case ColorMode::BYTE_CLASS:  return "Tipo di byte";
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
