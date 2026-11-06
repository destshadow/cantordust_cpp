#pragma once

#include <cstddef>

// Dot Plot ed Entropy: la posizione i di n sta nella colonna i * 256 / n.
// Primo indice della colonna x, l'inverso di quella formula.
inline size_t columnStart(int x, size_t n) {
    return ((size_t)x * n + 255) / 256;
}

enum class ViewMode {
    DIGRAPH    = 0,
    DOTPLOT    = 1,
    ENTROPY    = 2,
    HISTOGRAM  = 3,
    TRIGRAPH3D = 4,
    RAWPIXELS  = 5,
    METRICMAP  = 6,
    BYTECLOUD  = 7,
    ONETUPLE   = 8
};
