#include "signatures.h"
#include "signature_checks.h"
#include <algorithm>
#include <array>
#include <cstring>

namespace Signatures {

namespace {

struct Sig {
    const char* name;
    const char* magic;
    size_t      len;
    size_t      before;                    // byte del file prima del marcatore
    bool (*check)(const std::vector<uint8_t>&, size_t);
};

#define MAGIC(s) s, sizeof(s) - 1

const Sig TABLE[] = {
    // Immagini
    {"PNG",     MAGIC("\x89PNG\r\n\x1a\n"), 0, nullptr},
    {"JPEG",    MAGIC("\xFF\xD8\xFF"), 0, SigCheck::jpeg},
    {"GIF",     MAGIC("GIF87a"), 0, nullptr},
    {"GIF",     MAGIC("GIF89a"), 0, nullptr},
    {"BMP",     MAGIC("BM"), 0, SigCheck::bmp},
    {"TIFF",    MAGIC("II*\0"), 0, SigCheck::tiff},
    {"TIFF",    MAGIC("MM\0*"), 0, SigCheck::tiff},
    // Archivi e dati compressi
    {"ZIP",     MAGIC("PK\x03\x04"), 0, SigCheck::zip},
    {"gzip",    MAGIC("\x1F\x8B\x08"), 0, SigCheck::gzip},
    {"bzip2",   MAGIC("BZh"), 0, SigCheck::bzip2},
    {"xz",      MAGIC("\xFD" "7zXZ\0"), 0, nullptr},
    {"zstd",    MAGIC("\x28\xB5\x2F\xFD"), 0, nullptr},
    {"7z",      MAGIC("7z\xBC\xAF\x27\x1C"), 0, nullptr},
    {"RAR",     MAGIC("Rar!\x1A\x07"), 0, nullptr},
    {"tar",     MAGIC("ustar"), 257, nullptr},
    {"cpio",    MAGIC("070701"), 0, nullptr},
    {"SquashFS", MAGIC("hsqs"), 0, nullptr},
    // Eseguibili
    {"ELF",     MAGIC("\x7F" "ELF"), 0, SigCheck::elf},
    {"PE",      MAGIC("MZ"), 0, SigCheck::pe},
    {"Mach-O",  MAGIC("\xCF\xFA\xED\xFE"), 0, nullptr},
    {"Mach-O",  MAGIC("\xCE\xFA\xED\xFE"), 0, nullptr},
    {"Java class", MAGIC("\xCA\xFE\xBA\xBE"), 0, nullptr},
    {"DEX",     MAGIC("dex\n03"), 0, nullptr},
    // Documenti e dati
    {"PDF",     MAGIC("%PDF-"), 0, nullptr},
    {"SQLite",  MAGIC("SQLite format 3\0"), 0, nullptr},
    {"XML",     MAGIC("<?xml "), 0, nullptr},
    {"PEM",     MAGIC("-----BEGIN "), 0, nullptr},
    // Script e pagine: codice di testo dentro un file qualunque
    {"script #!", MAGIC("#!"), 0, SigCheck::shebang},
    {"PHP",     MAGIC("<?php"), 0, SigCheck::phpTag},
    {"JavaScript", MAGIC("<script"), 0, SigCheck::scriptTag},
    {"JavaScript", MAGIC("<SCRIPT"), 0, SigCheck::scriptTag},
    {"HTML",    MAGIC("<!DOCTYPE html"), 0, nullptr},
    {"HTML",    MAGIC("<!doctype html"), 0, nullptr},
    {"HTML",    MAGIC("<html"), 0, SigCheck::htmlTag},
    {"HTML",    MAGIC("<HTML"), 0, SigCheck::htmlTag},
    // Audio e video
    {"RIFF",    MAGIC("RIFF"), 0, SigCheck::riff},
    {"MP4",     MAGIC("ftyp"), 4, SigCheck::mp4},
    {"Ogg",     MAGIC("OggS\0"), 0, nullptr},
    {"FLAC",    MAGIC("fLaC\0"), 0, nullptr},
    {"Matroska", MAGIC("\x1A\x45\xDF\xA3"), 0, nullptr},
};

#undef MAGIC

// Per ogni primo byte, le firme che iniziano cosi'
std::array<std::vector<const Sig*>, 256> buildIndex() {
    std::array<std::vector<const Sig*>, 256> idx;
    for (const Sig& s : TABLE) idx[(uint8_t)s.magic[0]].push_back(&s);
    return idx;
}

} // namespace

std::vector<Hit> scan(const std::vector<uint8_t>& b) {
    static const auto index = buildIndex();
    std::vector<Hit> hits;
    for (size_t i = 0; i < b.size(); i++) {
        for (const Sig* s : index[b[i]]) {
            if (s->len > b.size() - i || i < s->before) continue;
            if (std::memcmp(&b[i], s->magic, s->len) != 0) continue;
            const size_t at = i - s->before;
            if (s->check && !s->check(b, at)) continue;
            hits.push_back({at, s->name});
            if (hits.size() >= MAX_HITS) return hits;
        }
    }
    // "tar" e "MP4" iniziano prima del marcatore: rimette in ordine
    std::stable_sort(hits.begin(), hits.end(),
                     [](const Hit& a, const Hit& c) { return a.offset < c.offset; });
    return hits;
}

size_t next(const std::vector<Hit>& hits, size_t from) {
    for (size_t i = 0; i < hits.size(); i++)
        if (hits[i].offset > from) return i;
    return SIZE_MAX;
}

size_t prev(const std::vector<Hit>& hits, size_t from) {
    for (size_t i = hits.size(); i-- > 0; )
        if (hits[i].offset < from) return i;
    return SIZE_MAX;
}

} // namespace Signatures
