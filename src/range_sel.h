#pragma once
#include <algorithm>
#include <cstddef>

// Un intervallo [lo, hi) dentro i limiti [min, max): la selezione di una
// barra. Solo logica, nessuna grafica, cosi' si puo' testare da sola.
struct RangeSel {
    static constexpr size_t MIN_SIZE = 16;   // byte, selezione minima

    size_t min = 0, max = 0;
    size_t lo = 0, hi = 0;

    size_t size() const { return hi - lo; }
    size_t minSize() const { return std::min(MIN_SIZE, max - min); }

    void clamp() {
        lo = std::clamp(lo, min, max);
        hi = std::clamp(hi, min, max);
        if (hi < lo) std::swap(lo, hi);
        size_t ms = minSize();
        if (hi - lo < ms) {
            hi = std::min(max, lo + ms);
            lo = hi - ms;
        }
    }
    // I byte restano gli stessi: si limita solo ai nuovi confini
    void setBounds(size_t a, size_t b) { min = a; max = b; clamp(); }
    void set(size_t a, size_t b)       { lo = a; hi = b; clamp(); }

    void setLo(size_t v) { lo = std::clamp(v, min, hi - std::min(minSize(), hi - min)); }
    void setHi(size_t v) { hi = std::clamp(v, lo + std::min(minSize(), max - lo), max); }

    // Sposta tenendo la dimensione
    void slideTo(size_t newLo) {
        size_t s = size();
        lo = std::clamp(newLo, min, max - s);
        hi = lo + s;
    }

    // Allarga o restringe tenendo fermo il byte `center`
    void zoomAround(size_t center, double factor) {
        size_t oldSize = size();
        if (oldSize == 0) return;
        double t = (double)(std::clamp(center, lo, hi) - lo) / (double)oldSize;
        double want = std::clamp((double)oldSize * factor,
                                 (double)minSize(), (double)(max - min));
        size_t s = (size_t)want;
        double newLo = (double)center - t * (double)s;
        hi = lo + s;                 // slideTo usa la nuova dimensione
        slideTo(newLo < (double)min ? min : (size_t)newLo);
    }
};
