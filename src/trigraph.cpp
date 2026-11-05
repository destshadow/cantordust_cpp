#include "trigraph.h"
#include "density.h"
#include <algorithm>

namespace {
inline uint32_t tripleAt(const std::vector<uint8_t>& b, size_t i) {
    return ((uint32_t)b[i] << 16) | ((uint32_t)b[i + 1] << 8) | b[i + 2];
}

// Sezione prevalente di ogni tripla, con il voto di maggioranza
// (Boyer-Moore): se una sezione ha piu' di meta' delle occorrenze vince, con
// un solo candidato per tripla invece di un contatore per sezione.
std::vector<int16_t> sectionVotes(const std::vector<uint8_t>& bytes,
                                  const std::vector<TriGraph::Range>& ranges) {
    std::vector<int16_t>  winner((size_t)1 << 24, -1);
    std::vector<uint32_t> votes ((size_t)1 << 24, 0);
    for (const auto& r : ranges) {
        const size_t end = std::min(r.end, bytes.size());
        for (size_t i = r.start; i + 2 < end; i++) {
            uint32_t key = tripleAt(bytes, i);
            if (votes[key] == 0)            { winner[key] = r.section; votes[key] = 1; }
            else if (winner[key] == r.section) votes[key]++;
            else                               votes[key]--;
        }
    }
    return winner;
}
} // namespace

void TriGraph::clear() {
    m_points.clear();
}

const Point3D* TriGraph::find(uint32_t key) const {
    auto it = std::lower_bound(m_points.begin(), m_points.end(), key,
        [](const Point3D& p, uint32_t k) { return p.key < k; });
    return it != m_points.end() && it->key == key ? &*it : nullptr;
}

void TriGraph::compute(const std::vector<uint8_t>& bytes,
                       const std::vector<Range>& sections) {
    clear();
    if (bytes.size() < 3) return;

    // Le triple possibili sono 2^24: un array e' molto piu' veloce di una
    // mappa (su 100 MB casuali: 1,6 s invece di 11).
    std::vector<uint32_t> freq((size_t)1 << 24, 0);
    for (size_t i = 0; i + 2 < bytes.size(); i++) freq[tripleAt(bytes, i)]++;

    const std::vector<int16_t> winner =
        sections.empty() ? std::vector<int16_t>() : sectionVotes(bytes, sections);
    const float ref = Density::reference(freq.data(), freq.size());

    size_t present = 0;
    for (uint32_t c : freq) present += (c != 0);
    m_points.reserve(present);

    for (uint32_t key = 0; key < (uint32_t)1 << 24; key++) {
        if (freq[key] == 0) continue;
        Point3D p{};
        p.key     = key;
        p.count   = freq[key];
        p.level   = Density::intensity(freq[key], ref);
        p.section = winner.empty() ? (int16_t)-1 : winner[key];
        m_points.push_back(p);
    }
}
