#include "autocorr.h"
#include <algorithm>
#include <cmath>

// Tabelle di record: i pozzi sono uguali a ogni multiplo della dimensione del
// record, quindi nessuno spicca sugli altri e bestWidth() li scarterebbe come
// rumore. Qui si guardano tutti insieme: per ogni periodo p si confrontano le
// distanze p, 2p, 3p... con quelle in mezzo.
int Autocorr::bestPeriod(size_t bpp) const {
    constexpr int    MIN_PERIOD = 8;       // sotto e' il passo delle istruzioni
    constexpr double MIN_Z      = 8.0;     // quanto emerge dal rumore
    constexpr double MIN_DROP   = 0.05;    // calo minimo, in frazione

    const int n = (int)(maxLag() / bpp);
    if (n < MIN_PERIOD * 3) return 0;

    std::vector<double> s(n + 1, 0.0), sum(n + 1, 0.0);
    for (int w = 1; w <= n; w++) {
        s[w]   = m_mad[(size_t)w * bpp];
        sum[w] = sum[w - 1] + s[w];
    }

    // Il rumore si stima dai salti tra distanze vicine. Con la mediana i
    // pozzi stessi non contano: con la media gonfierebbero la stima.
    std::vector<double> jumps;
    jumps.reserve(n);
    for (int w = 2; w <= n; w++) jumps.push_back(std::fabs(s[w] - s[w - 1]));
    auto mid = jumps.begin() + (ptrdiff_t)(jumps.size() / 2);
    std::nth_element(jumps.begin(), mid, jumps.end());
    const double noise = std::max(*mid / 0.95, 1e-3);    // 0.95: salto mediano / sigma

    std::vector<double> drop(n / 3 + 1, 0.0);
    double top = 0.0;
    for (int p = MIN_PERIOD; p <= n / 3; p++) {
        const int count = n / p, last = count * p;
        double comb = 0.0;
        for (int k = 1; k <= count; k++) comb += s[k * p];
        const double others = (sum[last] - sum[p - 1] - comb) /
                              (last - p + 1 - count);
        comb /= count;
        const double d = others - comb;
        if (d < MIN_DROP * others || d * std::sqrt((double)count) < MIN_Z * noise)
            continue;
        drop[p] = d;
        top = std::max(top, d);
    }
    if (top <= 0.0) return 0;

    // I multipli del periodo vanno bene quanto lui: si prende il piu' piccolo
    for (int p = MIN_PERIOD; p <= n / 3; p++)
        if (drop[p] >= 0.7 * top) return p;
    return 0;
}
