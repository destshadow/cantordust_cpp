#include "binary_format.h"
#include <cstring>

// --- Parser PE ---
void BinaryParser::parsePE(const std::vector<uint8_t>& bytes) {
    if (bytes.size() < 0x40) return;

    // Offset dell'header PE (a 0x3C nel DOS header)
    size_t peOff = read32(bytes, 0x3C);
    if (peOff > bytes.size() || bytes.size() - peOff < 24) return;

    // Verifica magic "PE\0\0"
    if (bytes[peOff]   != 'P' || bytes[peOff+1] != 'E' ||
        bytes[peOff+2] != 0   || bytes[peOff+3] != 0)
        return;

    m_format = BinaryFormat::PE;

    // COFF header (subito dopo il magic PE)
    uint16_t numSections    = read16(bytes, peOff + 6);
    uint16_t optHeaderSize  = read16(bytes, peOff + 20);

    // Le section headers iniziano dopo COFF header (24 bytes)
    // + optional header
    size_t secTableOff = peOff + 24 + optHeaderSize;

    // Ogni PE section header è 40 bytes
    for (uint16_t i = 0; i < numSections && i < MAX_SECTIONS; i++) {
        size_t entOff = secTableOff + i * 40;
        if (entOff + 40 > bytes.size()) break;

        // Nome: 8 bytes ASCII null-padded
        char name[9] = {};
        memcpy(name, &bytes[entOff], 8);

        uint32_t rawSize   = read32(bytes, entOff + 16); // SizeOfRawData
        uint32_t rawOffset = read32(bytes, entOff + 20); // PointerToRawData

        if (rawSize == 0 || rawOffset > bytes.size() || rawSize > bytes.size() - rawOffset) continue;

        addSection(std::string(name), rawOffset, rawSize);
    }
}
