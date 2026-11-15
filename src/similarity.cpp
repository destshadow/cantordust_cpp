#include "similarity.h"
#include "density.h"
#include <algorithm>
#include <cstring>

namespace {
struct Token { uint32_t hash; uint32_t cell; };

inline uint32_t mix(uint32_t v) {          // sparge i bit: serve a campionare
    v ^= v >> 16;  v *= 0x7feb352dU;
    v ^= v >> 15;  v *= 0x846ca68bU;
    return v ^ (v >> 16);
}
} // namespace

void Similarity::clear() {
    std::fill(m_cells.begin(), m_cells.end(), 0.0f);
}

void Similarity::compute(const std::vector<uint8_t>& b) {
    clear();
    if (b.size() < GRAM) return;
    const size_t n = b.size() - GRAM + 1;
    uint32_t step = (uint32_t)(n / MAX_TOKENS + 1);
    const size_t cap = MAX_TOKENS * 2;

    std::vector<Token> tokens;
    tokens.reserve(std::min(n, cap));
    uint32_t prev = 0;
    for (size_t i = 0; i < n; i++) {
        uint32_t gram;
        std::memcpy(&gram, &b[i], GRAM);
        // Una fila di byte uguali da' sempre lo stesso gettone: basta il primo
        if (i > 0 && gram == prev) continue;
        prev = gram;
        if (mix(gram) % step != 0) continue;
        tokens.push_back({gram, (uint32_t)(i * SIZE / n)});

        // Troppi gettoni (pochi valori ripetuti moltissimo): se ne tiene la
        // meta', sempre scelta dal contenuto
        if (tokens.size() >= cap) {
            step *= 2;
            auto drop = [&](const Token& t) { return mix(t.hash) % step != 0; };
            tokens.erase(std::remove_if(tokens.begin(), tokens.end(), drop), tokens.end());
            if (tokens.size() >= cap) break;      // tutti lo stesso gettone
        }
    }
    // Stesso gettone vicino, e dentro il gruppo in ordine di posizione
    std::stable_sort(tokens.begin(), tokens.end(),
                     [](const Token& a, const Token& c) { return a.hash < c.hash; });

    for (size_t lo = 0; lo < tokens.size(); ) {
        size_t hi = lo;
        while (hi < tokens.size() && tokens[hi].hash == tokens[lo].hash) hi++;
        const size_t freq = hi - lo;
        if (freq > 1) {
            // L'approssimazione di Church e Helfman scarta i gettoni troppo
            // comuni. Qui si confrontano solo con le MAX_FREQ occorrenze dopo:
            // un file fatto di cento copie non resta nero, e il costo non cresce.
            const float w = 1.0f / (float)std::min(freq, MAX_FREQ);
            for (size_t p = lo; p < hi; p++) {
                const size_t last = std::min(hi, p + 1 + MAX_FREQ);
                for (size_t q = p + 1; q < last; q++) {
                    m_cells[(size_t)tokens[q].cell * SIZE + tokens[p].cell] += w;
                    m_cells[(size_t)tokens[p].cell * SIZE + tokens[q].cell] += w;
                }
            }
        }
        lo = hi;
    }
}

std::vector<uint8_t> Similarity::toRGB() const {
    // Stessa curva delle altre viste a conteggi, sui pesi in millesimi
    std::vector<uint32_t> counts(m_cells.size());
    for (size_t i = 0; i < m_cells.size(); i++)
        counts[i] = (uint32_t)std::min(m_cells[i] * 1000.0f, 4.0e9f);
    const float ref = Density::reference(counts.data(), counts.size());

    std::vector<uint8_t> rgb((size_t)SIZE * SIZE * 3, 0);
    for (int y = 0; y < SIZE; y++)
        for (int x = 0; x < SIZE; x++) {
            const size_t i = (size_t)y * SIZE + x;
            uint8_t v = Density::intensity(counts[i], ref);
            if (x == y) v = std::max<uint8_t>(v, 40);    // la diagonale: dove siamo
            rgb[i * 3 + 1] = v;
        }
    return rgb;
}
