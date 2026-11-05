#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

// Dove compare un valore (un byte o una coppia) nel file.
// Le occorrenze si sovrappongono come nel digraph: 00 00 00 contiene due
// volte la coppia 00 00.
namespace Occur {

// Prima occorrenza in [from, end); SIZE_MAX se non c'e'
size_t next(const std::vector<uint8_t>& b, size_t from, size_t end,
            const uint8_t* pat, size_t n);

size_t count(const std::vector<uint8_t>& b, size_t lo, size_t hi,
             const uint8_t* pat, size_t n);

// Per i segni sulle barre: [lo, hi) diviso in `rows` righe uguali,
// true dove la riga contiene almeno un'occorrenza
std::vector<bool> rows(const std::vector<uint8_t>& b, size_t lo, size_t hi,
                       const uint8_t* pat, size_t n, int rows);

// Passa da un'occorrenza alla successiva ("3 di 120"), ricominciando
// dalla prima dopo l'ultima.
struct Walker {
    std::vector<uint8_t> pattern;
    size_t lo = 0, hi = 0;
    size_t count  = 0;
    size_t index  = 0;              // 1 = la prima
    size_t target = SIZE_MAX;       // offset corrente

    bool active() const { return count > 0; }
    void clear();
    bool sameAs(const uint8_t* pat, size_t n) const;
    // Stesso valore e stessa finestra: la successiva. Altrimenti: la prima.
    void step(const std::vector<uint8_t>& b, size_t lo, size_t hi,
              const uint8_t* pat, size_t n);
};

} // namespace Occur
