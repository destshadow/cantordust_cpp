#include "overview.h"
#include "cancel.h"
#include "chi_square.h"
#include <algorithm>
#include <cmath>

float Overview::lowBitRandomness(const std::array<uint64_t, 256>& h) {
    uint64_t n = 0;
    for (uint64_t c : h) n += c;
    if (n < 1024) return 0.0f;

    // Tutto il byte casuale? Allora lo sono anche i bit bassi, e non dice nulla
    const double flat = (double)n / 256.0;
    double uniform = 0.0;
    for (uint64_t c : h) uniform += ((double)c - flat) * ((double)c - flat) / flat;
    if (ChiSquare::pValue(uniform, 255) > 0.001) return 0.0f;

    double chi = 0.0;
    int pairs = 0;
    for (int k = 0; k < 128; k++) {
        const double expected = (double)(h[2 * k] + h[2 * k + 1]) / 2.0;
        if (expected <= 4.0) continue;          // troppo pochi per contare
        const double d = (double)h[2 * k] - expected;
        chi += d * d / expected;
        pairs++;
    }
    return pairs < 2 ? 0.0f : (float)ChiSquare::pValue(chi, pairs - 1);
}

float Overview::periodicity(const uint8_t* a, const uint8_t* b, uint8_t& len) {
    len = 0;
    const size_t n = (size_t)(b - a);
    if (n < 64) return 0.0f;

    // Quanto spesso due byte a caso sono uguali: e' la linea di base
    std::array<uint32_t, 256> h{};
    for (const uint8_t* p = a; p < b; p++) h[*p]++;
    double chance = 0.0;
    for (uint32_t c : h) chance += ((double)c / n) * ((double)c / n);
    if (chance > 0.98) return 0.0f;             // quasi un solo valore: non e' un periodo

    double best = 0.0;
    const int maxLen = (int)std::min<size_t>(MAX_PERIOD, n / 4);
    for (int p = 1; p <= maxLen; p++) {
        size_t same = 0;
        for (const uint8_t* q = a; q + p < b; q++) same += (q[0] == q[p]);
        const double match = (double)same / (double)(n - p);
        const double strength = (match - chance) / (1.0 - chance);
        if (strength > best + 0.02) {            // a parita', il periodo piu' corto
            best = strength;
            len = (uint8_t)p;
        }
    }
    if (best < 0.05) { len = 0; return 0.0f; }
    return (float)std::min(1.0, best);
}

void Overview::compute(const std::vector<uint8_t>& bytes) {
    clear();
    m_len = bytes.size();
    if (m_len == 0) return;
    const size_t cols = std::min<size_t>(COLS, m_len);
    m_cells.assign(cols, Cell{});

    // Istogramma di ogni tratto, e la somma di tutti quelli prima
    std::vector<std::array<uint64_t, 256>> upTo(cols + 1);
    upTo[0].fill(0);
    for (size_t c = 0; c < cols; c++) {
        if (Cancel::requested()) { clear(); return; }
        const size_t a = c * m_len / cols, b = (c + 1) * m_len / cols;
        std::array<uint64_t, 256> h{};
        for (size_t i = a; i < b; i++) h[bytes[i]]++;

        Cell& cell = m_cells[c];
        const double n = (double)(b - a);
        double entropy = 0.0;
        uint64_t printable = 0;
        for (int v = 0; v < 256; v++) {
            if (!h[v]) continue;
            const double p = (double)h[v] / n;
            entropy -= p * std::log2(p);
            if ((v >= 0x20 && v < 0x7F) || v == '\n' || v == '\r' || v == '\t') printable += h[v];
        }
        // Un tratto corto non puo' arrivare a 8 bit: si misura sul massimo possibile
        const double maxBits = std::log2(std::min(256.0, n));
        cell.entropy = maxBits > 0.0 ? (float)std::min(1.0, entropy / maxBits) : 0.0f;
        cell.text  = (float)(printable / n);
        cell.zeros = (float)(h[0x00] / n);
        cell.ff    = (float)(h[0xFF] / n);
        cell.period = periodicity(&bytes[a], &bytes[std::min(b, a + PERIOD_SAMPLE)],
                                  cell.periodLen);

        for (int v = 0; v < 256; v++) upTo[c + 1][v] = upTo[c][v] + h[v];
    }

    for (size_t c = 0; c < cols; c++) {
        if (c > 0) m_cells[c].jump =
            std::min(1.0f, std::fabs(m_cells[c].entropy - m_cells[c - 1].entropy) * 4.0f);

        // Bit bassi: servono abbastanza byte, quindi anche i tratti vicini
        const size_t cellBytes = std::max<size_t>(1, m_len / cols);
        const size_t radius = LOWBIT_MIN / cellBytes / 2;
        const size_t lo = c > radius ? c - radius : 0, hi = std::min(cols, c + radius + 1);
        std::array<uint64_t, 256> h;
        for (int v = 0; v < 256; v++) h[v] = upTo[hi][v] - upTo[lo][v];
        m_cells[c].lowBits = lowBitRandomness(h);
    }
}
