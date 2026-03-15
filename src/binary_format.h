#pragma once
#include <string>
#include <vector>
#include <cstdint>

// Rappresenta una sezione di un binario (ELF o PE)
struct BinarySection {
    std::string name;       // ".text", ".data", ecc.
    size_t      offset;     // offset nel file
    size_t      size;       // dimensione in bytes
    uint8_t     r, g, b;    // colore per la visualizzazione
};

// Tipo di formato rilevato
enum class BinaryFormat {
    UNKNOWN,
    ELF,
    PE
};

class BinaryParser {
public:
    // Analizza i bytes e popola le sezioni
    void parse(const std::vector<uint8_t>& bytes);
    void clear();

    BinaryFormat                    getFormat()   const { return m_format; }
    const std::vector<BinarySection>& getSections() const { return m_sections; }
    bool                            hasSections() const { return !m_sections.empty(); }
    std::string                     getFormatName() const;

private:
    void parseELF(const std::vector<uint8_t>& bytes);
    void parsePE (const std::vector<uint8_t>& bytes);

    // Helper per leggere valori little-endian dal buffer
    uint16_t read16(const std::vector<uint8_t>& b, size_t off) const;
    uint32_t read32(const std::vector<uint8_t>& b, size_t off) const;
    uint64_t read64(const std::vector<uint8_t>& b, size_t off) const;

    BinaryFormat              m_format = BinaryFormat::UNKNOWN;
    std::vector<BinarySection> m_sections;

    // Palette colori per le sezioni — ciclica
    static const uint8_t PALETTE[][3];
};
