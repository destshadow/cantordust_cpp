#include "binary_format.h"

// --- Parser ELF ---
// Supporta sia ELF32 che ELF64
void BinaryParser::parseELF(const std::vector<uint8_t>& bytes) {
    if (bytes.size() < 16) return;
    if ((bytes[4] != 1 && bytes[4] != 2) || bytes[5] != 1) {
        m_format = BinaryFormat::UNKNOWN;
        return; // Al momento supportiamo solo ELF little-endian.
    }
    if (bytes.size() < (bytes[4] == 2 ? 64u : 52u)) return;

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
    if (shentsize < (is64 ? 64 : 40) || shstrndx >= shnum) return;
    if (shoff > bytes.size() || shnum > (bytes.size() - shoff) / shentsize) return;

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
    for (uint16_t i = 0; i < shnum && i < MAX_SECTIONS; i++) {
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

        if (read32(bytes, entOff + 4) == 8) continue; // SHT_NOBITS: nessun byte nel file
        if (secOffset > bytes.size() || secSize > bytes.size() - secOffset) continue;
        if (secSize == 0) continue; // sezione vuota → skip

        // Leggi il nome dalla strtab
        std::string name = "?";
        if (strtabOff > 0 &&
            strtabOff < bytes.size() && nameOff < bytes.size() - strtabOff) {
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

        addSection(name, (size_t)secOffset, (size_t)secSize);
    }
}
