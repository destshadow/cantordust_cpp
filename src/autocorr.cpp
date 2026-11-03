#include "autocorr.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

void Autocorr::compute(const std::vector<uint8_t>& bytes,
                       size_t start, size_t end) {
    m_mad.clear();
    m_sim.clear();
    end = std::min(end, bytes.size());
    if (start >= end) return;

    const size_t len    = end - start;
    const int    maxLag = (int)std::min<size_t>(MAX_LAG, len / 2);
    if (maxLag < 2) return;

    // Posizioni campionate distribuite su tutta la finestra:
    // costo MAX_SAMPLES * MAX_LAG indipendente dalla dimensione del file
    const size_t span    = len - maxLag;
    const size_t samples = std::min<size_t>(MAX_SAMPLES, span);
    const uint8_t* data  = bytes.data() + start;

    std::vector<uint64_t> sum(maxLag + 1, 0);
    for (size_t j = 0; j < samples; j++) {
        const uint8_t* p = data + (j * span) / samples;
        const int a = p[0];
        for (int k = 1; k <= maxLag; k++)
            sum[k] += (uint64_t)std::abs(a - (int)p[k]);
    }

    m_mad.assign(maxLag + 1, 0.0f);
    float lo = 1e9f, hi = -1e9f;
    for (int k = 1; k <= maxLag; k++) {
        m_mad[k] = (float)sum[k] / (float)samples;
        lo = std::min(lo, m_mad[k]);
        hi = std::max(hi, m_mad[k]);
    }

    m_sim.assign(maxLag + 1, 0.0f);
    float range = hi - lo;
    for (int k = 1; k <= maxLag; k++)
        m_sim[k] = (range > 1e-6f) ? (hi - m_mad[k]) / range : 0.0f;
}

int Autocorr::bestWidth(size_t bpp, int minWidth) const {
    if (bpp == 0 || m_mad.empty()) return 0;
    const int n = (int)(maxLag() / bpp);   // larghezza massima misurabile
    const int NEAR = 8, FAR = 16;           // anello per la linea di base
    minWidth = std::max(minWidth, FAR + 3);
    if (n < minWidth + FAR + 2) return 0;

    // Solo distanze multiple del pixel: la riga e' un numero intero di
    // pixel. s[w] = differenza media alla distanza di w pixel.
    std::vector<float> s(n + 1, 0.0f);
    for (int w = 1; w <= n; w++) s[w] = m_mad[(size_t)w * bpp];

    // Smussatura triangolare: anche i vicini in diagonale (w±1, w±2)
    // si somigliano, il centro della "gobba" e' la larghezza vera.
    std::vector<float> sm(n + 1, 0.0f);
    for (int w = 3; w <= n - 2; w++)
        sm[w] = (s[w-2] + 2*s[w-1] + 3*s[w] + 2*s[w+1] + s[w+2]) / 9.0f;

    // Profondita' del pozzo rispetto ai valori un po' piu' lontani:
    // una tendenza lenta non conta, conta solo un calo localizzato.
    std::vector<float> depth(n + 1, 0.0f);
    for (int w = minWidth; w <= n - FAR - 2; w++) {
        float acc = 0.0f;
        for (int d = NEAR; d <= FAR; d++) acc += sm[w - d] + sm[w + d];
        depth[w] = acc / (2 * (FAR - NEAR + 1)) - sm[w];
    }

    int best = 0;
    for (int w = minWidth; w <= n; w++)
        if (depth[w] > (best ? depth[best] : 0.0f)) best = w;
    if (best == 0) return 0;

    // Significativita': il pozzo deve emergere nettamente dal rumore
    // (dati casuali o compressi danno solo piccole fluttuazioni).
    double mean = 0.0, var = 0.0; int cnt = 0;
    for (int w = minWidth; w <= n - FAR - 2; w++) { mean += depth[w]; cnt++; }
    if (cnt < 2) return 0;
    mean /= cnt;
    for (int w = minWidth; w <= n - FAR - 2; w++)
        var += (depth[w] - mean) * (depth[w] - mean);
    double sigma = std::sqrt(var / cnt);
    if (depth[best] - mean < 5.0 * sigma) return 0;  // rumore puro: z < ~4.5

    // I multipli della riga (2W, 3W, ...) danno pozzi simili: se una
    // frazione del migliore e' quasi altrettanto profonda, e' quella.
    for (int div = 6; div >= 2; div--) {
        int w = (int)std::lround((double)best / div);
        if (w < minWidth) continue;
        int cand = 0;
        for (int c = w - 1; c <= w + 1; c++)
            if (c >= minWidth && c <= n &&
                (cand == 0 || depth[c] > depth[cand])) cand = c;
        if (cand && depth[cand] >= 0.6f * depth[best]) return cand;
    }
    return best;
}
