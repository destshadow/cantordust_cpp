#pragma once
#include <vector>
#include <cstdint>
#include "hilbert.h"
#include "space_curves.h"
#include "wavelength_rgb.h"

// Modalita' di colorazione della MetricMap
enum class ColorMode {
    WAVELENGTH,   // byte → lunghezza d'onda (arcobaleno fisico)
    ENTROPY,      // entropia locale → rosso/blu
    GRADIENT,     // posizione nel file → gradiente blu→rosso
    SPECTRUM,     // variante dello spettro
    CLASSIFIER,   // classificazione NGram → colore per classe
};

// Modalita' di curva spaziale
enum class CurveMode {
    HILBERT,  // massima locality
    ZORDER,   // Z-order/Morton
    LINEAR,   // serpentine scan
};

// Dati classificatore passati opzionalmente
struct ClassifierData {
    const std::vector<int>* classifications = nullptr;
    int blockSize = 512;
};

class MetricMap {
public:
    MetricMap();

    // Calcola la mappa
    // order: ordine curva (default 8 → 256x256)
    void compute(const std::vector<uint8_t>& bytes,
                 ColorMode  colorMode  = ColorMode::WAVELENGTH,
                 CurveMode  curveMode  = CurveMode::HILBERT,
                 int        order      = 8,
                 const ClassifierData* classData = nullptr);
    void clear();

    const std::vector<uint8_t>& getRGB()  const { return m_rgb; }
    int                          getSize() const { return m_size; }
    ColorMode  getColorMode()  const { return m_colorMode; }
    CurveMode  getCurveMode()  const { return m_curveMode; }

    // Indice file per coordinate pixel — utile per click
    size_t fileIndexAt(int x, int y) const;

    static const char* colorModeName(ColorMode m);
    static const char* curveModeName(CurveMode m);

private:
    WavelengthRGB::RGB colorForByte(uint8_t   byte_val,
                                    size_t    file_idx,
                                    size_t    total,
                                    ColorMode mode,
                                    const ClassifierData* cd) const;

    float localEntropy(const std::vector<uint8_t>& bytes,
                       size_t idx) const;

    std::vector<uint8_t> m_rgb;
    std::vector<size_t>  m_indexMap;
    int       m_size      = 256;
    ColorMode m_colorMode = ColorMode::WAVELENGTH;
    CurveMode m_curveMode = CurveMode::HILBERT;
};
