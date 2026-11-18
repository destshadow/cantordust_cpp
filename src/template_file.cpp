#include "template_file.h"
#include <algorithm>
#include <cstring>

namespace TemplateFile {

namespace {

const uint8_t NET_MAGIC[] = {0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
const char    MODEL[]     = "statisticalModel";

uint32_t le32(const uint8_t* p) {
    return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24;
}

size_t find(const std::vector<uint8_t>& f, const void* what, size_t n, size_t from, size_t to) {
    to = std::min(to, f.size());
    for (size_t i = from; i + n <= to; i++)
        if (std::memcmp(&f[i], what, n) == 0) return i;
    return SIZE_MAX;
}

// Tipo 1: il record ArraySinglePrimitive di byte (0x0F, id, lunghezza,
// tipo 2 = Byte) nell'intestazione, dove il .NET mette il primo membro
bool dataArray(const std::vector<uint8_t>& f, Content& out) {
    const size_t HEAD = 1024;
    for (size_t i = 0; i + 10 <= std::min(f.size(), HEAD); i++) {
        if (f[i] != 0x0F || f[i + 9] != 2) continue;
        const uint32_t len = le32(&f[i + 5]);
        if (len == 0 || len > f.size() - (i + 10)) continue;
        out.bytes.assign(f.begin() + long(i + 10), f.begin() + long(i + 10 + len));
        return true;
    }
    return false;
}

// Tipo 2: subito dopo "statisticalModel" e il nome, voci da 16 byte
bool modelEntries(const std::vector<uint8_t>& f, const std::string& name, Content& out) {
    const size_t m = find(f, MODEL, sizeof(MODEL) - 1, 0, 1024);
    if (m == SIZE_MAX) return false;
    size_t i = m + sizeof(MODEL) - 1;
    if (f.size() - i < name.size() || std::memcmp(&f[i], name.data(), name.size()) != 0)
        return false;
    for (i += name.size(); i + 16 <= f.size() && le32(&f[i]) == 4; i += 16) {
        double p;
        std::memcpy(&p, &f[i + 8], sizeof p);
        if (p > 0 && p <= 1) out.grams.push_back({gramKey(&f[i + 4]), p});
    }
    return !out.grams.empty();
}

} // namespace

Content parse(const std::vector<uint8_t>& f, const std::string& name) {
    Content out;
    const bool net = f.size() >= sizeof(NET_MAGIC) &&
                     std::memcmp(f.data(), NET_MAGIC, sizeof(NET_MAGIC)) == 0;
    if (net) {
        if (modelEntries(f, name, out) || dataArray(f, out)) return out;
        return out;                     // .NET ma illeggibile: niente
    }
    out.bytes = f;                      // byte grezzi
    return out;
}

} // namespace TemplateFile
