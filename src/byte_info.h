#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <vector>
#include <algorithm>
#include <cstdlib>

// Classificazione dei singoli byte e statistiche su una zona.
// Generica: vale per qualunque file, formato noto o no.
// Usata dall'Inspector (e in futuro dalla ColorMode "classe di byte").
namespace ByteInfo {

enum class Class { ZERO, CONTROL, TEXT, HIGH, FF };

inline Class classOf(uint8_t b) {
    if (b == 0x00) return Class::ZERO;
    if (b == 0xFF) return Class::FF;
    if ((b >= 0x20 && b < 0x7F) || b == '\t' || b == '\n' || b == '\r')
        return Class::TEXT;
    if (b < 0x20 || b == 0x7F) return Class::CONTROL;
    return Class::HIGH;
}

inline const char* className(Class c) {
    switch (c) {
        case Class::ZERO:    return "zero";
        case Class::CONTROL: return "controllo";
        case Class::TEXT:    return "testo ASCII";
        case Class::HIGH:    return "alto (>0x7F)";
        case Class::FF:      return "0xFF";
    }
    return "?";
}

// Colori stile binvis.io: zero nero, testo blu, controllo verde,
// alti rosso, 0xFF bianco
inline void classColor(Class c, uint8_t& r, uint8_t& g, uint8_t& b) {
    switch (c) {
        case Class::ZERO:    r = 40;  g = 40;  b = 40;  break;  // visibile su nero
        case Class::CONTROL: r = 60;  g = 200; b = 80;  break;
        case Class::TEXT:    r = 70;  g = 140; b = 255; break;
        case Class::HIGH:    r = 230; g = 70;  b = 60;  break;
        case Class::FF:      r = 240; g = 240; b = 240; break;
    }
}

// Composizione di una zona: frazione (0..1) di ogni classe + entropia
struct Stats {
    size_t total   = 0;
    float  frac[5] = {0, 0, 0, 0, 0};   // indicizzato da (int)Class
    float  entropy = 0.0f;              // bit per byte, 0..8
    // Levigatezza: differenza media minima tra byte a distanza 1..4.
    // Bassa = valori che variano poco tra vicini (immagini/audio raw,
    // anche a 24/32 bpp, dove il vicino "giusto" e' a 3-4 byte).
    // -1 = non calcolata.
    float  smooth  = -1.0f;
};

inline Stats statsFromHist(const std::array<uint32_t, 256>& hist) {
    Stats s;
    for (int v = 0; v < 256; v++) s.total += hist[v];
    if (s.total == 0) return s;
    for (int v = 0; v < 256; v++) {
        if (!hist[v]) continue;
        float p = (float)hist[v] / (float)s.total;
        s.frac[(int)classOf((uint8_t)v)] += p;
        s.entropy -= p * std::log2(p);
    }
    return s;
}

inline float smoothness(const std::vector<uint8_t>& bytes, size_t start, size_t end) {
    end = std::min(end, bytes.size());
    if (start >= end || end - start < 16) return -1.0f;
    // Bastano 64 KB: si ricalcola a ogni frame col mouse su una sezione
    end = std::min(end, start + (size_t)65536);
    float best = 1e9f;
    for (size_t k = 1; k <= 4; k++) {
        uint64_t sum = 0;
        size_t n = 0;
        for (size_t i = start; i + k < end; i++, n++)
            sum += (uint64_t)std::abs((int)bytes[i] - (int)bytes[i + k]);
        if (n) best = std::min(best, (float)sum / (float)n);
    }
    return best;
}

inline Stats statsOf(const std::vector<uint8_t>& bytes, size_t start, size_t end) {
    std::array<uint32_t, 256> hist{};
    end = std::min(end, bytes.size());
    for (size_t i = start; i < end; i++) hist[bytes[i]]++;
    Stats s = statsFromHist(hist);
    s.smooth = smoothness(bytes, start, end);
    return s;
}

// Ipotesi a buon senso sul contenuto, dalla composizione.
// Non e' una classificazione certa: serve a orientarsi.
inline const char* guess(const Stats& s) {
    if (s.total < 16)                    return "troppo pochi byte";
    if (s.frac[(int)Class::ZERO] > 0.90f) return "zeri / padding";
    if (s.frac[(int)Class::FF]   > 0.90f) return "0xFF (flash vuota?)";
    if (s.frac[(int)Class::TEXT] > 0.85f) return "testo";
    // Con pochi campioni l'entropia massima raggiungibile e' piu' bassa
    float maxH = std::min(8.0f, std::log2((float)s.total));
    if (s.entropy > 0.94f * maxH)        return "alta entropia: compresso/cifrato";
    // Valori che cambiano poco tra vicini, ma non banali (entropia
    // non bassa): tipico di immagini/audio non compressi
    if (s.smooth >= 0.0f && s.smooth < 18.0f && s.entropy > 4.5f)
        return "valori lisci: immagine/audio raw?";
    if (s.frac[(int)Class::ZERO] > 0.35f) return "dati strutturati (molti zeri)";
    return "misto: codice o dati";
}

} // namespace ByteInfo
