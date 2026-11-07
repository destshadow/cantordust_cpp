#include "string_scan.h"
#include <algorithm>

namespace {

// Oltre questi caratteri una stringa non cambia tag
constexpr size_t TAG_CHARS = 256;

bool printable(uint8_t b) { return b >= 0x20 && b < 0x7F; }

size_t asciiRun(const std::vector<uint8_t>& b, size_t i) {
    size_t n = 0;
    while (i + n < b.size() && printable(b[i + n])) n++;
    return n;
}

size_t utf16Run(const std::vector<uint8_t>& b, size_t i) {
    size_t n = 0;
    while (i + n * 2 + 1 < b.size() && printable(b[i + n * 2]) && b[i + n * 2 + 1] == 0)
        n++;
    return n;
}

} // namespace

void StringScan::clear() {
    m_hits.clear();
    m_truncated = false;
}

void StringScan::compute(const std::vector<uint8_t>& w) {
    clear();
    auto add = [&](size_t off, size_t chars, Enc enc) {
        Hit h{off, (uint32_t)std::min<size_t>(chars, UINT32_MAX), enc, Tag::NONE};
        h.tag = classify(textOf(w, 0, h, TAG_CHARS));
        m_hits.push_back(h);
    };

    for (size_t i = 0; i < w.size(); ) {
        if (m_hits.size() >= MAX_HITS) { m_truncated = true; return; }
        if (!printable(w[i])) { i++; continue; }

        const size_t a = asciiRun(w, i);
        if (a >= MIN_CHARS) {
            add(i, a, Enc::ASCII);
            i += a;
            continue;
        }
        const size_t u = utf16Run(w, i);
        if (u >= MIN_CHARS) {
            add(i, u, Enc::UTF16);
            i += u * 2;
            continue;
        }
        i++;
    }
}

std::string StringScan::textOf(const std::vector<uint8_t>& bytes, size_t base,
                               const Hit& hit, size_t maxChars) {
    const size_t step = hit.enc == Enc::UTF16 ? 2 : 1;
    const size_t n = std::min<size_t>(hit.chars, maxChars);
    std::string s;
    s.reserve(n);
    for (size_t i = 0, off = base + hit.offset; i < n && off < bytes.size(); i++, off += step)
        s.push_back((char)bytes[off]);
    return s;
}

std::vector<uint32_t> StringScan::select(int minChars, Tag tag) const {
    std::vector<uint32_t> rows;
    for (size_t i = 0; i < m_hits.size(); i++)
        if ((int64_t)m_hits[i].chars >= minChars && (tag == Tag::NONE || m_hits[i].tag == tag))
            rows.push_back((uint32_t)i);
    return rows;
}

const char* StringScan::encName(Enc e) {
    return e == Enc::UTF16 ? "UTF-16" : "ASCII";
}

const char* StringScan::tagName(Tag t) {
    switch (t) {
        case Tag::URL:  return "URL";
        case Tag::IP:   return "IP";
        case Tag::PATH: return "PERCORSO";
        default:        return "";
    }
}
