#pragma once
#include <vector>
#include <cstdint>
#include "hilbert.h"
#include "space_curves.h"
#include "wavelength_rgb.h"
#include <array>

// Modalita' di colorazione della MetricMap
enum class ColorMode {
    WAVELENGTH,   // byte → lunghezza d'onda (arcobaleno fisico)
    ENTROPY,      // entropia locale → rosso/blu
    GRADIENT,     // posizione nel file → gradiente blu→rosso
    FREQUENCY,    // quanto e' comune il byte nella zona: comune = freddo, raro = caldo
    BYTE_CLASS,   // tipo di byte: zero, controllo, testo, alti, 0xFF
    CLASSIFIER,   // a quale contenuto somiglia (x86, testo...): un colore per classe
};

// Modalita' di curva spaziale
enum class CurveMode {
    HILBERT,  // massima locality
    ZORDER,   // Z-order/Morton
    LINEAR,   // serpentine scan
};

class ClassifierModel;

class MetricMap {
public:
    MetricMap();

    // Calcola la mappa
    // order: ordine curva (default 8 → 256x256)
    void compute(const std::vector<uint8_t>& bytes,
                 ColorMode  colorMode  = ColorMode::WAVELENGTH,
                 CurveMode  curveMode  = CurveMode::HILBERT,
                 int        order      = 8,
                 const ClassifierModel* classifier = nullptr);   // per CLASSIFIER
    void clear();

    const std::vector<uint8_t>& getRGB()  const { return m_rgb; }
    int                          getSize() const { return m_size; }

    // Indice file per coordinate pixel; SIZE_MAX se il pixel non corrisponde
    // a nessun byte (file piu' piccolo della mappa)
    size_t fileIndexAt(int x, int y) const;
    // L'inverso: il pixel che contiene quell'indice (ogni pixel copre i byte
    // fino all'indice del pixel dopo). false se la mappa e' vuota.
    bool pixelOf(size_t index, int& x, int& y) const;

    static const char* colorModeName(ColorMode m);
    // Il modo dopo, saltando il classifier se non ci sono i template
    static ColorMode nextColorMode(ColorMode m, bool classifierReady);
    static const char* curveModeName(CurveMode m);

private:
    WavelengthRGB::RGB colorForByte(uint8_t   byte_val,
                                    size_t    file_idx,
                                    size_t    total,
                                    ColorMode mode) const;

    // Colore di ogni valore di byte secondo quante volte compare in bytes
    static std::array<WavelengthRGB::RGB, 256>
        frequencyColors(const std::vector<uint8_t>& bytes);

    float localEntropy(const std::vector<uint8_t>& bytes,
                       size_t idx) const;

    std::vector<uint8_t> m_rgb;
    std::vector<size_t>  m_indexMap;
    int       m_size      = 256;
    std::array<WavelengthRGB::RGB, 256> m_freqColor{};   // solo per FREQUENCY
};
