#include "ngram_model.h"
#include <cmath>

namespace {

// Finale di murmur3: sparpaglia 4-grammi simili su secchi lontani
inline uint32_t bucketOf(uint32_t h) {
    h ^= h >> 16;  h *= 0x85ebca6bu;
    h ^= h >> 13;  h *= 0xc2b2ae35u;
    h ^= h >> 16;
    return h & uint32_t(NGramModel::BUCKETS - 1);
}

// Chiama f(secchio) per ogni 4-gramma di [p, p+n)
template <class F>
void eachGram(const uint8_t* p, size_t n, F f) {
    uint32_t key = 0;
    for (size_t i = 0; i < n; i++) {
        key = (key << 8) | p[i];
        if (i + 1 >= (size_t)NGramModel::N) f(bucketOf(key));
    }
}

} // namespace

NGramModel::NGramModel(int classes)
    : m_classes(classes > 0 ? classes : 0),
      m_counts(BUCKETS * size_t(m_classes), 0),
      m_total(size_t(m_classes), 0) {}

void NGramModel::add(int cls, const uint8_t* p, size_t n) {
    if (cls < 0 || cls >= m_classes || m_counts.empty()) return;
    const size_t k = size_t(m_classes);
    eachGram(p, n, [&](uint32_t b) { m_counts[b * k + size_t(cls)] += 1.0f; });
    if (n >= (size_t)N) m_total[size_t(cls)] += double(n - N + 1);
}

void NGramModel::addGram(int cls, uint32_t gram, double weight) {
    if (cls < 0 || cls >= m_classes || m_counts.empty() || !(weight > 0)) return;
    m_counts[bucketOf(gram) * size_t(m_classes) + size_t(cls)] += float(weight);
    m_total[size_t(cls)] += weight;
}

void NGramModel::finish() {
    const size_t k = size_t(m_classes);
    m_logP.assign(BUCKETS * k, 0.0f);
    const double uniform = LAMBDA / double(BUCKETS);
    for (size_t c = 0; c < k; c++) {
        const double total = double(m_total[c]);
        for (size_t b = 0; b < BUCKETS; b++) {
            const double seen = total > 0 ? m_counts[b * k + c] / total : 0.0;
            m_logP[b * k + c] = float(std::log((1.0 - LAMBDA) * seen + uniform));
        }
    }
    m_counts.clear();
    m_counts.shrink_to_fit();
}

bool NGramModel::hasData(int cls) const {
    return cls >= 0 && cls < m_classes && m_total[size_t(cls)] > 0;
}

int NGramModel::best(const uint8_t* p, size_t n, std::vector<double>* score) const {
    if (n < (size_t)N || m_logP.empty()) return -1;
    const size_t k = size_t(m_classes);
    double sum[64] = {};                    // le classi sono poche decine
    if (k > 64) return -1;
    eachGram(p, n, [&](uint32_t b) {
        const float* row = &m_logP[b * k];
        for (size_t c = 0; c < k; c++) sum[c] += row[c];
    });
    int bestCls = -1;
    for (size_t c = 0; c < k; c++) {
        if (m_total[c] == 0) continue;
        if (bestCls < 0 || sum[c] > sum[size_t(bestCls)]) bestCls = int(c);
    }
    if (score) score->assign(sum, sum + k);
    return bestCls;
}
