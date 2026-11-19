#pragma once

#include <cstddef>

// Dot Plot ed Entropy: la posizione i di n sta nella colonna i * 256 / n.
// Primo indice della colonna x, l'inverso di quella formula.
inline size_t columnStart(int x, size_t n) {
    return ((size_t)x * n + 255) / 256;
}

// Una finestra di n byte divisa in `parts` tratti: il tratto c inizia a
// c * n / parts (One Tuple, Panoramica). Questa e' l'inversa esatta: il
// tratto che contiene la posizione i. Una semplice i * parts / n sbaglia di
// uno quando l'inizio del tratto e' stato arrotondato per difetto.
inline size_t partOf(size_t i, size_t n, size_t parts) {
    if (n == 0 || parts == 0) return 0;
    size_t c = i * parts / n;
    if (c + 1 < parts && (c + 1) * n / parts <= i) c++;
    return c < parts ? c : parts - 1;
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
    ONETUPLE   = 8,
    STRINGS    = 9,
    HEX        = 10,
    OVERVIEW   = 11,
    SPECTROGRAM = 12,
    RECORD     = 13
};
inline constexpr int VIEW_MODE_COUNT = 14;
