#include "record_profile.h"
#include "cancel.h"
#include <algorithm>
#include <cmath>

void RecordProfile::clear() {
    m_period = m_phase = 0;
    m_records = m_sampled = 0;
    m_counts.clear();
    m_deltas.clear();
    m_columns.clear();
}

void RecordProfile::compute(const std::vector<uint8_t>& bytes, int period, int phase) {
    clear();
    m_period = std::clamp(period, 1, MAX_PERIOD);
    m_phase  = ((phase % m_period) + m_period) % m_period;
    const size_t p = size_t(m_period), start = size_t(m_phase);
    m_columns.assign(p, Column{});
    if (bytes.size() < start + p) return;

    m_records = (bytes.size() - start) / p;
    // Oltre MAX_RECORDS si prende un record ogni `stride`: un contatore resta
    // un contatore (cresce di stride per volta, sempre uguale)
    const size_t stride = (m_records + MAX_RECORDS - 1) / MAX_RECORDS;
    m_counts.assign(p * 256, 0);
    m_deltas.assign(p * 256, 0);
    std::vector<uint8_t> prev(p);
    Cancel::Poll poll;
    for (size_t r = 0; r < m_records; r += stride) {
        const size_t base = start + r * p;
        if (poll.at(base)) return;
        const uint8_t* rec = &bytes[base];
        for (size_t j = 0; j < p; j++) {
            m_counts[j * 256 + rec[j]]++;
            if (m_sampled > 0) m_deltas[j * 256 + uint8_t(rec[j] - prev[j])]++;
            prev[j] = rec[j];
        }
        m_sampled++;
    }
    for (size_t j = 0; j < p; j++)
        m_columns[j] = classify(&m_counts[j * 256], &m_deltas[j * 256], m_sampled);
}

int RecordProfile::byteAt(int x) const {
    if (m_period <= 0 || x < 0 || x >= SIZE) return -1;
    return int(size_t(x) * size_t(m_period) / SIZE);
}

int RecordProfile::firstX(int byte) const {
    if (m_period <= 0) return 0;
    return int((size_t(byte) * SIZE + size_t(m_period) - 1) / size_t(m_period));
}

const RecordProfile::Column& RecordProfile::column(int byte) const {
    static const Column none;
    return byte >= 0 && byte < int(m_columns.size()) ? m_columns[size_t(byte)] : none;
}

uint32_t RecordProfile::count(int byte, uint8_t value) const {
    if (byte < 0 || size_t(byte) * 256 >= m_counts.size()) return 0;
    return m_counts[size_t(byte) * 256 + value];
}

std::vector<uint8_t> RecordProfile::toRGB() const {
    std::vector<uint8_t> rgb(size_t(SIZE) * SIZE * 3, 0);
    if (m_sampled == 0) return rgb;
    for (int x = 0; x < SIZE; x++) {
        const int j = byteAt(x);
        const Column& c = column(j);
        uint8_t fr, fg, fb;
        fieldColor(c.field, fr, fg, fb);
        uint32_t most = 1;
        for (int v = 0; v < 256; v++) most = std::max(most, count(j, uint8_t(v)));
        const double top = std::log1p(double(most));
        // Una riga scura fra un byte e l'altro, se ci sta (record corti)
        const bool edge = m_period <= 64 && j > 0 && x == firstX(j);
        for (int v = 0; v < 256; v++) {
            uint8_t* px = &rgb[(size_t(v) * SIZE + size_t(x)) * 3];
            const uint32_t n = count(j, uint8_t(v));
            if (n == 0) {
                if (edge) px[0] = px[1] = px[2] = 40;
                continue;
            }
            const double t = 0.25 + 0.75 * std::log1p(double(n)) / top;
            px[0] = uint8_t(fr * t); px[1] = uint8_t(fg * t); px[2] = uint8_t(fb * t);
        }
    }
    return rgb;
}
