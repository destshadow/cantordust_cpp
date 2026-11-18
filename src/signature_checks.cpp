#include "signature_checks.h"
#include <cctype>
#include <cstring>

namespace SigCheck {

namespace {
bool has(const Bytes& b, size_t at, size_t n) { return at <= b.size() && n <= b.size() - at; }
uint32_t le32(const Bytes& b, size_t at) {
    return b[at] | (b[at + 1] << 8) | (b[at + 2] << 16) | ((uint32_t)b[at + 3] << 24);
}
uint16_t le16(const Bytes& b, size_t at) { return (uint16_t)(b[at] | (b[at + 1] << 8)); }
bool printable(uint8_t c) { return c >= 0x20 && c < 0x7F; }
} // namespace

bool pe(const Bytes& b, size_t at) {
    if (!has(b, at, 0x40)) return false;
    const size_t pe = le32(b, at + 0x3C);
    return pe >= 0x40 && pe < (1u << 20) && has(b, at, pe + 4) &&
           std::memcmp(&b[at + pe], "PE\0\0", 4) == 0;
}

bool elf(const Bytes& b, size_t at) {
    if (!has(b, at, 16)) return false;
    return (b[at + 4] == 1 || b[at + 4] == 2) && (b[at + 5] == 1 || b[at + 5] == 2) &&
           b[at + 6] == 1;
}

bool bmp(const Bytes& b, size_t at) {
    if (!has(b, at, 18)) return false;
    const uint32_t size = le32(b, at + 2), data = le32(b, at + 10), dib = le32(b, at + 14);
    const bool knownDib = dib == 12 || dib == 40 || dib == 52 || dib == 56 ||
                          dib == 108 || dib == 124;
    return le32(b, at + 6) == 0 && knownDib && data >= 14 + dib && data < size;
}

bool gzip(const Bytes& b, size_t at) {
    if (!has(b, at, 10)) return false;
    const uint8_t flags = b[at + 3], xfl = b[at + 8], os = b[at + 9];
    return (flags & 0xE0) == 0 && (xfl == 0 || xfl == 2 || xfl == 4) && (os <= 13 || os == 255);
}

bool jpeg(const Bytes& b, size_t at) {
    if (!has(b, at, 4)) return false;
    const uint8_t m = b[at + 3];           // il primo segmento dopo l'inizio
    return m == 0xE0 || m == 0xE1 || m == 0xE2 || m == 0xDB || m == 0xEE || m == 0xFE;
}

bool zip(const Bytes& b, size_t at) {
    if (!has(b, at, 30)) return false;
    const uint16_t version = le16(b, at + 4), method = le16(b, at + 8);
    return version < 100 && (method <= 14 || method == 93 || method == 95 || method == 99);
}

bool mp4(const Bytes& b, size_t at) {
    if (!has(b, at, 12)) return false;
    if (b[at] != 0 || b[at + 1] != 0) return false;     // intestazione sotto i 64 KB
    const size_t size = ((size_t)b[at + 2] << 8) | b[at + 3];
    if (size < 12) return false;
    for (int i = 8; i < 12; i++)
        if (!printable(b[at + i])) return false;        // la sigla del formato
    return true;
}

bool riff(const Bytes& b, size_t at) {
    if (!has(b, at, 12)) return false;
    for (int i = 8; i < 12; i++)
        if (!printable(b[at + i])) return false;        // "WAVE", "AVI ", "WEBP"
    return le32(b, at + 4) >= 4;
}

bool bzip2(const Bytes& b, size_t at) {
    static const uint8_t PI[6] = {0x31, 0x41, 0x59, 0x26, 0x53, 0x59};
    return has(b, at, 10) && b[at + 3] >= '1' && b[at + 3] <= '9' &&
           std::memcmp(&b[at + 4], PI, 6) == 0;
}

bool tiff(const Bytes& b, size_t at) {
    if (!has(b, at, 8)) return false;
    const uint32_t ifd = b[at] == 'I' ? le32(b, at + 4)
        : ((uint32_t)b[at + 4] << 24) | (b[at + 5] << 16) | (b[at + 6] << 8) | b[at + 7];
    return ifd >= 8 && ifd % 2 == 0 && ifd < (1u << 28);
}

namespace {
bool tagEnd(const Bytes& b, size_t i) {
    return i < b.size() && (b[i] == '>' || b[i] == ' ' || b[i] == '\n' ||
                            b[i] == '\r' || b[i] == '\t');
}
} // namespace

// "#!" poi, forse uno spazio, un percorso assoluto di lettere e "/._-+",
// lungo almeno 4 ("/bin" e' gia' troppo poco per un interprete: "/bin/sh"),
// chiuso da spazio o fine riga. Su dati casuali "#!/" capita: il percorso no.
bool shebang(const Bytes& b, size_t at) {
    size_t i = at + 2;
    if (i < b.size() && b[i] == ' ') i++;
    if (i >= b.size() || b[i] != '/') return false;
    const size_t from = i;
    while (i < b.size() && i - from < 64 &&
           (std::isalnum(b[i]) || b[i] == '/' || b[i] == '.' || b[i] == '_' ||
            b[i] == '-' || b[i] == '+'))
        i++;
    return i - from >= 7 && tagEnd(b, i) && b[i] != '>';
}

bool phpTag(const Bytes& b, size_t at)    { return tagEnd(b, at + 5) && b[at + 5] != '>'; }
bool scriptTag(const Bytes& b, size_t at) { return tagEnd(b, at + 7); }
bool htmlTag(const Bytes& b, size_t at)   { return tagEnd(b, at + 5); }

} // namespace SigCheck
