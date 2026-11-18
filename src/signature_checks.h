#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

// Controlli in piu' per le firme troppo corte per fidarsi del solo marcatore.
// `at` e' dove inizierebbe il file. Non leggono mai fuori da `b`.
namespace SigCheck {

using Bytes = std::vector<uint8_t>;

bool pe(const Bytes& b, size_t at);      // "MZ" che porta a "PE\0\0"
bool elf(const Bytes& b, size_t at);
bool bmp(const Bytes& b, size_t at);
bool gzip(const Bytes& b, size_t at);
bool jpeg(const Bytes& b, size_t at);
bool zip(const Bytes& b, size_t at);
bool mp4(const Bytes& b, size_t at);     // "ftyp" al byte 4
bool riff(const Bytes& b, size_t at);
bool bzip2(const Bytes& b, size_t at);
bool tiff(const Bytes& b, size_t at);
// Script nascosti: il marcatore deve essere seguito da cio' che lo chiude
bool shebang(const Bytes& b, size_t at);   // "#!/bin/sh", "#! /usr/bin/env python"
bool phpTag(const Bytes& b, size_t at);    // "<?php" + spazio
bool scriptTag(const Bytes& b, size_t at); // "<script" + spazio o '>'
bool htmlTag(const Bytes& b, size_t at);   // "<html" + spazio o '>'

} // namespace SigCheck
