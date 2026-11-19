#include "spectrogram.h"
#include <algorithm>
#include <cmath>

// Spettrogramma: dai livelli ai periodi (di base, tratto dove dura, intero
// esatto). Il calcolo dei livelli e' in spectrogram.cpp.

// Il periodo di base: somma dei sottoarmonici (Hermes, "Measurement of
// pitch by subharmonic summation", 1988). Per ogni periodo p acceso (almeno
// 3 volte la media) si sommano le sue armoniche p/m con peso 0,84^(m-1), fino
// a 15: il periodo vero raccoglie anche le righe delle sue armoniche, un suo
// sottomultiplo no. Provato prima e scartato:
//   - solo 4 armoniche, peso uguale: su record da 24 con 12 zeri vinceva 8;
//   - tutte le armoniche, peso uguale: un periodo lunghissimo (una deriva
//     lenta) ha armoniche su tutte le righe e vinceva sempre.
// In piu' p deve starci almeno 8 volte nei byte analizzati.
int Spectrogram::baseRow(int col) const {
    constexpr double WEIGHT = 0.84;
    constexpr int    HARMONICS = 15;
    int best = -1;
    double bestScore = 0;
    for (int r = 0; r < SIZE; r++) {
        const double p = periodAt(r);
        if (level(col, r) < 0.5f || p * 8 > m_n) continue;
        double score = 0, w = 1;
        for (int m = 1; m <= HARMONICS && p / m >= MIN_PERIOD; m++, w *= WEIGHT)
            score += w * std::max(0.0f, level(col, rowOf(p / m)));
        if (score > bestScore * 1.05) { bestScore = score; best = r; }
    }
    return best;
}

bool Spectrogram::runAt(int col, int row, int& first, int& last) const {
    auto lit = [&](int c) {
        for (int r = row - 1; r <= row + 1; r++)
            if (level(c, r) >= 0.5f) return true;
        return false;
    };
    if (col < 0 || col >= m_cols || !lit(col)) return false;
    first = last = col;
    while (first > 0 && (lit(first - 1) || (first > 1 && lit(first - 2))))
        first -= lit(first - 1) ? 1 : 2;
    while (last + 1 < m_cols && (lit(last + 1) || (last + 2 < m_cols && lit(last + 2))))
        last += lit(last + 1) ? 1 : 2;
    return true;
}

int Spectrogram::exactPeriod(const std::vector<uint8_t>& bytes, size_t from, size_t len,
                             double approx) {
    const int mid = std::max(1, int(std::lround(approx)));
    const int span = std::max(1, int(std::ceil(approx * 0.06)));
    from = std::min(from, bytes.size());
    const size_t end = std::min(bytes.size(), from + len);
    int best = mid;
    size_t bestSame = 0;
    for (int q = std::max(1, mid - span); q <= mid + span; q++) {
        size_t same = 0;
        for (size_t i = from; i + size_t(q) < end; i++) same += bytes[i] == bytes[i + size_t(q)];
        // a parita' vince il piu' vicino a `approx` (si scorre da sinistra:
        // basta un ">" e un confronto della distanza)
        if (same > bestSame || (same == bestSame &&
                                std::abs(q - approx) < std::abs(best - approx))) {
            bestSame = same;
            best = q;
        }
    }
    return best;
}
