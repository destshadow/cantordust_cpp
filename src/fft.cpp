#include "fft.h"
#include <cmath>
#include <utility>

namespace FFT {

bool transform(std::vector<std::complex<double>>& a) {
    const size_t n = a.size();
    if (!isPowerOf2(n)) return false;

    // Ordine a bit invertiti: dopo, ogni passo unisce due meta' contigue
    for (size_t i = 1, j = 0; i < n; i++) {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j |= bit;
        if (i < j) std::swap(a[i], a[j]);
    }
    const double PI = std::acos(-1.0);
    for (size_t len = 2; len <= n; len <<= 1) {
        const std::complex<double> step = std::polar(1.0, -2.0 * PI / double(len));
        for (size_t i = 0; i < n; i += len) {
            std::complex<double> w(1.0, 0.0);
            for (size_t k = 0; k < len / 2; k++) {
                const std::complex<double> u = a[i + k];
                const std::complex<double> v = a[i + k + len / 2] * w;
                a[i + k]           = u + v;
                a[i + k + len / 2] = u - v;
                w *= step;
            }
        }
    }
    return true;
}

} // namespace FFT
