#include "pixel_guess.h"
#include <algorithm>

namespace PixelGuess {

namespace {
constexpr int    REPEATS   = 3;       // si guardano p, 2p, 3p
constexpr double MIN_SCORE = 0.25;    // quanto devono somigliarsi di piu'
constexpr size_t SAMPLES   = 65536;

double score(const std::vector<float>& mad, size_t p) {
    if (mad.size() <= p * REPEATS) return 0.0;
    double on = 0.0, off = 0.0;
    size_t nOn = 0, nOff = 0;
    for (size_t k = 1; k <= p * REPEATS; k++) {
        if (k % p == 0) { on += mad[k]; nOn++; }
        else            { off += mad[k]; nOff++; }
    }
    if (nOff == 0 || off <= 0.0) return 0.0;
    return 1.0 - (on / nOn) / (off / nOff);
}
} // namespace

size_t bytesPerPixel(const std::vector<float>& mad) {
    // Dal piu' piccolo: un passo di 2 fa somigliare anche le distanze 4 e 8
    for (size_t p : {2, 3, 4, 8})
        if (score(mad, p) >= MIN_SCORE) return p;
    return 1;
}

size_t phase(const std::vector<uint8_t>& bytes, size_t start, size_t end, size_t bpp) {
    end = std::min(end, bytes.size());
    if (bpp != 4 || start >= end || end - start < bpp * 64) return 0;

    // Varianza di ogni posizione dentro il pixel, su pixel campionati
    const size_t pixels = (end - start) / bpp;
    const size_t n = std::min(pixels, SAMPLES);
    double sum[4] = {0, 0, 0, 0}, sq[4] = {0, 0, 0, 0};
    for (size_t s = 0; s < n; s++) {
        const uint8_t* p = &bytes[start + (s * pixels / n) * bpp];
        for (size_t c = 0; c < bpp; c++) { sum[c] += p[c]; sq[c] += (double)p[c] * p[c]; }
    }
    double var[4], total = 0.0;
    size_t flat = 0;
    for (size_t c = 0; c < bpp; c++) {
        var[c] = sq[c] / n - (sum[c] / n) * (sum[c] / n);
        total += var[c];
        if (var[c] < var[flat]) flat = c;
    }
    // Un solo byte quasi fermo, gli altri no
    const double others = (total - var[flat]) / (bpp - 1);
    if (others < 16.0 || var[flat] > 0.02 * others) return 0;
    return (flat + 1) % bpp;
}

} // namespace PixelGuess
