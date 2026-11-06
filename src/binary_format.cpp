#include "binary_format.h"

// Palette colori per le sezioni — massimo 16 sezioni
const uint8_t BinaryParser::PALETTE[][3] = {
    {255, 80,  80 },  // rosso       → .text (codice)
    { 80, 255, 80 },  // verde       → .data
    { 80, 180, 255},  // azzurro     → .rodata
    {255, 200,  50},  // giallo      → .bss
    {200,  80, 255},  // viola       → .plt
    {255, 140,  50},  // arancione   → .got
    { 50, 255, 200},  // turchese    → .dynamic
    {255,  80, 180},  // rosa        → altri
    {180, 255,  80},
    { 80, 120, 255},
    {255, 180, 120},
    {120, 255, 180},
    {255, 120, 255},
    {120, 200, 255},
    {255, 255, 120},
    {200, 120, 120},
};

// --- Helper: lettura little-endian ---
uint16_t BinaryParser::read16(const std::vector<uint8_t>& b,
                               size_t off) const {
    if (off > b.size() || b.size() - off < 2) return 0;
    return (uint16_t)b[off] | ((uint16_t)b[off+1] << 8);
}

uint32_t BinaryParser::read32(const std::vector<uint8_t>& b,
                               size_t off) const {
    if (off > b.size() || b.size() - off < 4) return 0;
    return (uint32_t)b[off]       | ((uint32_t)b[off+1] << 8) |
           ((uint32_t)b[off+2] << 16) | ((uint32_t)b[off+3] << 24);
}

uint64_t BinaryParser::read64(const std::vector<uint8_t>& b,
                               size_t off) const {
    if (off > b.size() || b.size() - off < 8) return 0;
    return (uint64_t)read32(b, off) |
           ((uint64_t)read32(b, off+4) << 32);
}

void BinaryParser::clear() {
    m_format = BinaryFormat::UNKNOWN;
    m_sections.clear();
}

void BinaryParser::parse(const std::vector<uint8_t>& bytes) {
    clear();
    if (bytes.size() < 4) return;

    // Rileva il formato dai magic bytes
    // ELF: 7F 45 4C 46
    if (bytes[0] == 0x7F && bytes[1] == 'E' &&
        bytes[2] == 'L'  && bytes[3] == 'F') {
        m_format = BinaryFormat::ELF;
        parseELF(bytes);
        return;
    }

    // PE: 4D 5A ("MZ")
    if (bytes[0] == 'M' && bytes[1] == 'Z') {
        parsePE(bytes);
        return;
    }

    m_format = BinaryFormat::UNKNOWN;
}

// Il colore segue l'ordine delle sezioni, ricominciando dopo 16
void BinaryParser::addSection(const std::string& name, size_t offset, size_t size) {
    const uint8_t* c = PALETTE[m_sections.size() % 16];
    m_sections.push_back({name, offset, size, c[0], c[1], c[2]});
}

std::string BinaryParser::getFormatName() const {
    switch (m_format) {
        case BinaryFormat::ELF:     return "ELF";
        case BinaryFormat::PE:      return "PE";
        case BinaryFormat::UNKNOWN: return "???";
    }
    return "???";
}
