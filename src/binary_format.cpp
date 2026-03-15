#include "binary_format.h"
#include <cstring>
#include <algorithm>
#include <iostream>

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
    if (off + 1 >= b.size()) return 0;
    return (uint16_t)b[off] | ((uint16_t)b[off+1] << 8);
}

uint32_t BinaryParser::read32(const std::vector<uint8_t>& b,
                               size_t off) const {
    if (off + 3 >= b.size()) return 0;
    return (uint32_t)b[off]       | ((uint32_t)b[off+1] << 8) |
           ((uint32_t)b[off+2] << 16) | ((uint32_t)b[off+3] << 24);
}

uint64_t BinaryParser::read64(const std::vector<uint8_t>& b,
                               size_t off) const {
    if (off + 7 >= b.size()) return 0;
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
        m_format = BinaryFormat::PE;
        parsePE(bytes);
        return;
    }

    m_format = BinaryFormat::UNKNOWN;
}

// --- Parser ELF ---
// Supporta sia ELF32 che ELF64
void BinaryParser::parseELF(const std::vector<uint8_t>& bytes) {
    if (bytes.size() < 64) return;

    bool is64 = (bytes[4] == 2);  // EI_CLASS: 1=32bit, 2=64bit

    // Leggi posizione e struttura della section header table
    uint64_t shoff;     // offset della section header table
    uint16_t shentsize; // dimensione di ogni entry
    uint16_t shnum;     // numero di sezioni
    uint16_t shstrndx;  // indice della sezione con i nomi

    if (is64) {
        shoff     = read64(bytes, 0x28);
        shentsize = read16(bytes, 0x3A);
        shnum     = read16(bytes, 0x3C);
        shstrndx  = read16(bytes, 0x3E);
    } else {
        shoff     = read32(bytes, 0x20);
        shentsize = read16(bytes, 0x2E);
        shnum     = read16(bytes, 0x30);
        shstrndx  = read16(bytes, 0x32);
    }

    if (shoff == 0 || shnum == 0 || shentsize == 0) return;
    if (shoff + shnum * shentsize > bytes.size())    return;

    // Leggi la sezione dei nomi (.shstrtab)
    // È la sezione all'indice shstrndx
    size_t   strtabOff = 0;


    size_t nameSecOff = shoff + shstrndx * shentsize;
    if (nameSecOff + shentsize <= bytes.size()) {
        if (is64) {
            strtabOff  = read64(bytes, nameSecOff + 0x18);
        } else {
            strtabOff  = read32(bytes, nameSecOff + 0x10);
        }
    }

    // Leggi ogni sezione
    int colorIdx = 0;
    for (uint16_t i = 0; i < shnum && i < 64; i++) {
        size_t entOff = shoff + i * shentsize;
        if (entOff + shentsize > bytes.size()) break;

        uint32_t nameOff;   // offset del nome nella strtab
        uint64_t secOffset; // offset della sezione nel file
        uint64_t secSize;   // dimensione della sezione

        if (is64) {
            nameOff   = read32(bytes, entOff + 0x00);
            secOffset = read64(bytes, entOff + 0x18);
            secSize   = read64(bytes, entOff + 0x20);
        } else {
            nameOff   = read32(bytes, entOff + 0x00);
            secOffset = read32(bytes, entOff + 0x10);
            secSize   = read32(bytes, entOff + 0x14);
        }

        if (secSize == 0) continue; // sezione vuota → skip

        // Leggi il nome dalla strtab
        std::string name = "?";
        if (strtabOff > 0 &&
            strtabOff + nameOff < bytes.size()) {
            size_t nameStart = strtabOff + nameOff;
            size_t nameEnd   = nameStart;
            while (nameEnd < bytes.size() &&
                   bytes[nameEnd] != 0 &&
                   nameEnd - nameStart < 32)
                nameEnd++;
            name = std::string((char*)&bytes[nameStart],
                               nameEnd - nameStart);
        }

        if (name.empty() || name == "?") continue;

        BinarySection sec;
        sec.name   = name;
        sec.offset = (size_t)secOffset;
        sec.size   = (size_t)secSize;
        sec.r      = PALETTE[colorIdx % 16][0];
        sec.g      = PALETTE[colorIdx % 16][1];
        sec.b      = PALETTE[colorIdx % 16][2];
        colorIdx++;

        m_sections.push_back(sec);
    }
}

// --- Parser PE ---
void BinaryParser::parsePE(const std::vector<uint8_t>& bytes) {
    if (bytes.size() < 0x40) return;

    // Offset dell'header PE (a 0x3C nel DOS header)
    uint32_t peOff = read32(bytes, 0x3C);
    if (peOff + 24 >= bytes.size()) return;

    // Verifica magic "PE\0\0"
    if (bytes[peOff]   != 'P' || bytes[peOff+1] != 'E' ||
        bytes[peOff+2] != 0   || bytes[peOff+3] != 0)
        return;

    // COFF header (subito dopo il magic PE)
    uint16_t numSections    = read16(bytes, peOff + 6);
    uint16_t optHeaderSize  = read16(bytes, peOff + 20);

    // Le section headers iniziano dopo COFF header (24 bytes)
    // + optional header
    size_t secTableOff = peOff + 24 + optHeaderSize;

    // Ogni PE section header è 40 bytes
    int colorIdx = 0;
    for (uint16_t i = 0; i < numSections && i < 64; i++) {
        size_t entOff = secTableOff + i * 40;
        if (entOff + 40 > bytes.size()) break;

        // Nome: 8 bytes ASCII null-padded
        char name[9] = {};
        memcpy(name, &bytes[entOff], 8);

        uint32_t rawSize   = read32(bytes, entOff + 16); // SizeOfRawData
        uint32_t rawOffset = read32(bytes, entOff + 20); // PointerToRawData

        if (rawSize == 0) continue;

        BinarySection sec;
        sec.name   = std::string(name);
        sec.offset = rawOffset;
        sec.size   = rawSize;
        sec.r      = PALETTE[colorIdx % 16][0];
        sec.g      = PALETTE[colorIdx % 16][1];
        sec.b      = PALETTE[colorIdx % 16][2];
        colorIdx++;

        m_sections.push_back(sec);
    }
}

std::string BinaryParser::getFormatName() const {
    switch (m_format) {
        case BinaryFormat::ELF:     return "ELF";
        case BinaryFormat::PE:      return "PE";
        case BinaryFormat::UNKNOWN: return "???";
    }
    return "???";
}
