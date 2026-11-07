#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Le stringhe di testo in una sequenza di byte, come il comando `strings`:
// ASCII e UTF-16 little endian (quello dei file Windows).
//
// Di ogni stringa si tiene solo dove sta: in un file grande possono essere
// milioni, e il testo si legge dal file quando serve (textOf).
class StringScan {
public:
    enum class Enc : uint8_t { ASCII, UTF16 };
    enum class Tag : uint8_t { NONE, URL, IP, PATH };

    struct Hit {
        size_t   offset;   // relativo alla finestra
        uint32_t chars;    // caratteri, non byte
        Enc      enc;
        Tag      tag;
    };

    static constexpr int    MIN_CHARS = 4;          // sotto e' quasi tutto rumore
    static constexpr size_t MAX_HITS  = 2000000;    // 32 MB al massimo

    void compute(const std::vector<uint8_t>& window);
    void clear();

    const std::vector<Hit>& hits() const { return m_hits; }
    bool truncated() const { return m_truncated; }   // oltre MAX_HITS

    // Il testo di una stringa, al massimo maxChars caratteri.
    // base = dove inizia in `bytes` la finestra della ricerca.
    static std::string textOf(const std::vector<uint8_t>& bytes, size_t base,
                              const Hit& hit, size_t maxChars);

    // Le posizioni in hits() che passano il filtro; Tag::NONE = qualunque tag
    std::vector<uint32_t> select(int minChars, Tag tag) const;

    static const char* encName(Enc e);
    static const char* tagName(Tag t);
    static Tag classify(const std::string& text);    // string_tag.cpp

private:
    std::vector<Hit> m_hits;
    bool m_truncated = false;
};
