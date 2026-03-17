#pragma once
#include <vector>
#include <cstdint>
#include "hilbert.h"
#include "wavelength_rgb.h"

// Modalita' di colorazione della MetricMap
enum class ColorMode {
    WAVELENGTH,  // byte → lunghezza d'onda (arcobaleno fisico)
    ENTROPY,     // entropia locale → rosso/blu
    GRADIENT,    // posizione nel file → gradiente
    SPECTRUM,    // Hilbert 3D per spazio colori
};

class MetricMap {
public:
    MetricMap();

    // Calcola la mappa su un buffer di bytes
    // order: ordine della curva Hilbert (default 9 → 512x512)
    void compute(const std::vector<uint8_t>& bytes,
                 ColorMode mode  = ColorMode::WAVELENGTH,
                 int       order = 9);
    void clear();

    // Buffer RGB — dimensione size x size x 3
    const std::vector<uint8_t>& getRGB()   const { return m_rgb; }
    int                          getSize()  const { return m_size; }
    ColorMode                    getMode()  const { return m_mode; }

    // Indice nel file corrispondente a un pixel (x,y)
    // Utile per il click sull'immagine
    size_t fileIndexAt(int x, int y) const;

    static const char* modeName(ColorMode m);

private:
    // Calcola il colore di un byte in base alla modalita'
    WavelengthRGB::RGB colorForByte(uint8_t byte_val,
                                    size_t  file_idx,
                                    size_t  total,
                                    ColorMode mode) const;

    // Entropia locale su finestra di 16 byte
    float localEntropy(const std::vector<uint8_t>& bytes,
                       size_t idx) const;

    std::vector<uint8_t> m_rgb;
    int                  m_size  = 512;
    ColorMode            m_mode  = ColorMode::WAVELENGTH;

    // Mappa inversa: indice Hilbert → indice nel file
    // Usata per il click sull'immagine
    std::vector<size_t>  m_indexMap;
};
