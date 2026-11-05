#include "occurrences.h"
#include <algorithm>
#include <cstring>

namespace Occur {

size_t next(const std::vector<uint8_t>& b, size_t from, size_t end,
            const uint8_t* pat, size_t n) {
    end = std::min(end, b.size());
    if (n == 0 || end < n) return SIZE_MAX;
    const size_t last = end - n;     // ultimo inizio possibile
    if (from > last) return SIZE_MAX;
    const uint8_t* d = b.data();

    if (n == 2) {
        // Ciclo diretto: se il primo byte e' ovunque (file di zeri) chiamare
        // memchr a ogni posizione costa molto di piu'
        const uint8_t p0 = pat[0], p1 = pat[1];
        for (size_t i = from; i <= last; i++)
            if (d[i] == p0 && d[i + 1] == p1) return i;
        return SIZE_MAX;
    }
    while (from <= last) {
        const void* p = std::memchr(d + from, pat[0], last - from + 1);
        if (!p) return SIZE_MAX;
        size_t at = (size_t)((const uint8_t*)p - d);
        if (std::memcmp(d + at, pat, n) == 0) return at;
        from = at + 1;
    }
    return SIZE_MAX;
}

size_t count(const std::vector<uint8_t>& b, size_t lo, size_t hi,
             const uint8_t* pat, size_t n) {
    size_t c = 0;
    for (size_t at = next(b, lo, hi, pat, n); at != SIZE_MAX;
         at = next(b, at + 1, hi, pat, n))
        c++;
    return c;
}

std::vector<bool> rows(const std::vector<uint8_t>& b, size_t lo, size_t hi,
                       const uint8_t* pat, size_t n, int rowsCount) {
    std::vector<bool> out(rowsCount > 0 ? rowsCount : 0, false);
    if (hi <= lo || rowsCount <= 0) return out;
    const double span = (double)(hi - lo);
    size_t at = next(b, lo, hi, pat, n);
    while (at != SIZE_MAX) {
        int r = std::min((int)((double)(at - lo) / span * rowsCount), rowsCount - 1);
        out[r] = true;
        // Riga gia' decisa: si riparte dalla successiva. Arrotondato per
        // difetto: al massimo si rilegge un byte, mai se ne salta uno
        size_t rowEnd = lo + (size_t)((double)(r + 1) / rowsCount * span);
        at = next(b, std::max(at + 1, rowEnd), hi, pat, n);
    }
    return out;
}

void Walker::clear() {
    pattern.clear();
    lo = hi = count = index = 0;
    target = SIZE_MAX;
}

bool Walker::sameAs(const uint8_t* pat, size_t n) const {
    return pattern.size() == n && std::equal(pattern.begin(), pattern.end(), pat);
}

void Walker::step(const std::vector<uint8_t>& b, size_t newLo, size_t newHi,
                  const uint8_t* pat, size_t n) {
    if (active() && sameAs(pat, n) && lo == newLo && hi == newHi) {
        size_t at = next(b, target + 1, hi, pat, n);
        if (at == SIZE_MAX) { target = next(b, lo, hi, pat, n); index = 1; }
        else                { target = at; index++; }
        return;
    }
    clear();
    size_t c = Occur::count(b, newLo, newHi, pat, n);
    if (c == 0) return;
    pattern.assign(pat, pat + n);
    lo = newLo;  hi = newHi;  count = c;  index = 1;
    target = next(b, lo, hi, pat, n);
}

} // namespace Occur
