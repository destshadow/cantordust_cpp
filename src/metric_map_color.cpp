#include "metric_map.h"
#include "classifier_model.h"
#include "byte_info.h"

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

        case ColorMode::BYTE_CLASS: {
            WavelengthRGB::RGB c{};
            ByteInfo::classColor(ByteInfo::classOf(byte_val), c.r, c.g, c.b);
            return c;
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
        case ColorMode::SPECTRUM:    return "Spectrum";
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
