#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

// Luminosita' delle viste "a conteggi" (digraph, dot plot): pochi valori
// enormi e moltissimi piccoli.
//
// Dividere per il massimo non funziona: una sola coppia dominante (00 00,
// FF FF) supera le altre di ordini di grandezza e spegne tutto il resto.
// Quindi, come fa il CantorDust originale, il riferimento e' il 90esimo
// percentile delle celle non vuote, con una curva logaritmica. Il 10% piu'
// denso satura: e' quello che ci si aspetta dal nucleo di un addensamento.
namespace Density {

// Valore che corrisponde al massimo della luminosita'
inline float reference(const uint32_t* counts, size_t n) {
    std::vector<uint32_t> lit;
    for (size_t i = 0; i < n; i++)
        if (counts[i] > 0) lit.push_back(counts[i]);
    if (lit.empty()) return 2.0f;
    auto nth = lit.begin() + (ptrdiff_t)((lit.size() - 1) * 9 / 10);
    std::nth_element(lit.begin(), nth, lit.end());
    return std::max(2.0f, (float)*nth);
}

inline uint8_t intensity(uint32_t count, float ref) {
    if (count == 0) return 0;
    float v = 255.0f * std::log1p((float)count) / std::log1p(ref);
    return (uint8_t)std::min(255.0f, v);
}

} // namespace Density
