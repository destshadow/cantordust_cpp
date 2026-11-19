#pragma once
#include <complex>
#include <cstddef>
#include <vector>

// Trasformata di Fourier veloce, scritta qui per non aggiungere librerie.
// Cooley-Tukey iterativa, radice 2: la lunghezza deve essere una potenza di 2.
// Serve allo Spettrogramma (spectrogram.h): quanto e' forte ogni periodo in
// un tratto di byte.
namespace FFT {

inline bool isPowerOf2(size_t n) { return n >= 1 && (n & (n - 1)) == 0; }

// La potenza di 2 piu' grande che non supera n (0 se n = 0)
inline size_t floorPowerOf2(size_t n) {
    size_t p = 1;
    if (n == 0) return 0;
    while (p <= n / 2) p *= 2;
    return p;
}

// Sul posto: a[k] = somma_j a[j] * e^(-2 pi i j k / n). Lunghezza non
// potenza di 2: non tocca nulla e restituisce false.
bool transform(std::vector<std::complex<double>>& a);

} // namespace FFT
