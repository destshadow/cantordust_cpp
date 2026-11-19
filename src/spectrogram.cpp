#include "spectrogram.h"
#include "cancel.h"
#include "fft.h"
#include <algorithm>
#include <cmath>
#include <complex>

namespace {

// Periodo di una riga anche frazionaria: serve per i bordi della fascia
double periodOf(double row, double maxPeriod) {
    const double top = Spectrogram::SIZE - 1;
    return Spectrogram::MIN_PERIOD * std::pow(maxPeriod / Spectrogram::MIN_PERIOD, row / top);
}

} // namespace

void Spectrogram::clear() {
    m_cols = m_n = 0;
    m_len = 0;
    m_maxPeriod = 0;
    m_level.clear();
    m_seg.clear();
}

void Spectrogram::compute(const std::vector<uint8_t>& bytes, int fftSize, bool removeMean) {
    clear();
    const size_t n = std::min(FFT::floorPowerOf2(size_t(std::max(fftSize, 16))),
                              FFT::floorPowerOf2(bytes.size()));
    if (n < 16) return;                      // troppo corto per dire qualcosa
    m_n = int(n);
    m_len = bytes.size();
    m_cols = int(std::min<size_t>(SIZE, bytes.size()));
    m_maxPeriod = double(n) / 2;
    m_level.assign(size_t(SIZE) * SIZE, DARK);
    m_seg.resize(size_t(m_cols));

    // Hann: i bordi del segmento pesano zero, il picco resta stretto
    const double PI = std::acos(-1.0);
    std::vector<double> hann(n);
    for (size_t i = 0; i < n; i++) hann[i] = 0.5 - 0.5 * std::cos(2 * PI * double(i) / double(n - 1));

    // Per ogni riga, le frequenze k della sua fascia di periodi
    std::vector<size_t> kLo(SIZE), kHi(SIZE);
    std::vector<double> kMid(SIZE);
    for (int r = 0; r < SIZE; r++) {
        const double pLo = periodOf(r - 0.5, m_maxPeriod), pHi = periodOf(r + 0.5, m_maxPeriod);
        kLo[size_t(r)] = size_t(std::max(1.0, std::ceil(double(n) / pHi)));
        kHi[size_t(r)] = size_t(std::min(double(n) / 2, std::floor(double(n) / pLo)));
        kMid[size_t(r)] = double(n) / periodOf(r, m_maxPeriod);
    }

    std::vector<std::complex<double>> a(n);
    std::vector<double> power(n / 2 + 1);
    for (int c = 0; c < m_cols; c++) {
        if (Cancel::requested()) return;
        // Il segmento al centro del tratto, spostato dentro ai bordi
        const size_t mid = (columnStart(c) + columnStart(c + 1)) / 2;
        const size_t seg = std::min(mid > n / 2 ? mid - n / 2 : 0, bytes.size() - n);
        m_seg[size_t(c)] = seg;

        double mean = 0;
        for (size_t i = 0; i < n; i++) mean += bytes[seg + i];
        mean /= double(n);
        for (size_t i = 0; i < n; i++) a[i] = (bytes[seg + i] - mean) * hann[i];
        FFT::transform(a);

        double avg = 0;
        for (size_t k = 1; k <= n / 2; k++) { power[k] = std::norm(a[k]); avg += power[k]; }
        avg /= double(n / 2);
        if (avg <= 1e-12) continue;          // tratto costante: resta DARK

        for (int r = 0; r < SIZE; r++) {
            double p = 0;
            if (kLo[size_t(r)] <= kHi[size_t(r)]) {
                for (size_t k = kLo[size_t(r)]; k <= kHi[size_t(r)]; k++) p = std::max(p, power[k]);
            } else {                          // fascia piu' stretta di una frequenza
                const double k = std::clamp(kMid[size_t(r)], 1.0, double(n / 2));
                const size_t k0 = size_t(k), k1 = std::min(k0 + 1, n / 2);
                p = power[k0] + (power[k1] - power[k0]) * (k - double(k0));
            }
            m_level[size_t(r) * SIZE + size_t(c)] =
                p > 0 ? float(std::log10(p / avg)) : DARK;
        }
    }

    if (removeMean) {
        for (int r = 0; r < SIZE; r++) {
            float* row = &m_level[size_t(r) * SIZE];
            double sum = 0;
            int used = 0;
            for (int c = 0; c < m_cols; c++)
                if (row[c] > DARK) { sum += row[c]; used++; }
            if (used == 0) continue;
            const float avgRow = float(sum / used);
            for (int c = 0; c < m_cols; c++)
                if (row[c] > DARK) row[c] -= avgRow;
        }
    }
}

double Spectrogram::periodAt(int row) const {
    return m_maxPeriod > 0 ? periodOf(row, m_maxPeriod) : 0;
}

int Spectrogram::rowOf(double period) const {
    if (m_maxPeriod <= MIN_PERIOD || period <= MIN_PERIOD) return 0;
    const double r = std::log(period / MIN_PERIOD) / std::log(m_maxPeriod / MIN_PERIOD) * (SIZE - 1);
    return std::clamp(int(std::lround(r)), 0, SIZE - 1);
}

size_t Spectrogram::columnStart(int c) const {
    return m_cols > 0 ? size_t(c) * m_len / size_t(m_cols) : 0;
}

size_t Spectrogram::segmentStart(int c) const {
    return c >= 0 && c < m_cols ? m_seg[size_t(c)] : 0;
}

int Spectrogram::columnOf(size_t pos) const {
    if (m_cols == 0 || m_len == 0) return -1;
    size_t c = std::min(pos, m_len - 1) * size_t(m_cols) / m_len;
    // come partOf: l'inizio delle colonne e' arrotondato per difetto
    if (c + 1 < size_t(m_cols) && columnStart(int(c) + 1) <= pos) c++;
    return int(c);
}

float Spectrogram::level(int col, int row) const {
    if (col < 0 || col >= m_cols || row < 0 || row >= SIZE) return DARK;
    return m_level[size_t(row) * SIZE + size_t(col)];
}

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

std::vector<uint8_t> Spectrogram::toRGB() const {
    std::vector<uint8_t> rgb(size_t(SIZE) * SIZE * 3, 0);
    for (int r = 0; r < SIZE; r++)
        for (int c = 0; c < m_cols; c++) {
            const double t = std::clamp((double(level(c, r)) - NOISE) / (FULL - NOISE), 0.0, 1.0);
            // nero → rosso → giallo → bianco
            uint8_t* px = &rgb[(size_t(r) * SIZE + size_t(c)) * 3];
            px[0] = uint8_t(255 * std::clamp(3 * t, 0.0, 1.0));
            px[1] = uint8_t(255 * std::clamp(3 * t - 1, 0.0, 1.0));
            px[2] = uint8_t(255 * std::clamp(3 * t - 2, 0.0, 1.0));
        }
    return rgb;
}
