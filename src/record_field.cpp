#include "record_profile.h"
#include <cmath>

// Il tipo di una colonna del record. Soglie scelte sui casi di prova
// (tests/test_record.cpp), non prese da un articolo:
//   FIXED    un valore ha almeno il 98% dei record
//   COUNTER  la stessa differenza (non zero) fra record consecutivi nel 90%
//   TEXT     almeno il 90% e' ASCII stampabile, con piu' di 16 valori (con
//            meno e' un codice: il byte alto di un float vale 0x45-0x47,
//            cioe' "E", "F", "G", ed era scambiato per testo)
//   RANDOM   entropia almeno il 90% del massimo possibile con quei record
//   FEW      al massimo 16 valori diversi (flag, enumerazioni)
//   VARIED   il resto (numeri, misure, puntatori...)
// L'ordine conta: un contatore ha anche entropia alta.
RecordProfile::Column RecordProfile::classify(const uint32_t* values, const uint32_t* deltas,
                                             size_t samples) {
    Column c;
    if (samples == 0) return c;
    const double n = double(samples);
    uint32_t best = 0, printable = 0;
    double h = 0;
    for (int v = 0; v < 256; v++) {
        const uint32_t k = values[v];
        if (k == 0) continue;
        c.distinct++;
        if (k > best) { best = k; c.top = uint8_t(v); }
        if (v >= 0x20 && v < 0x7F) printable += k;
        const double q = k / n;
        h -= q * std::log2(q);
    }
    c.topShare = best / n;
    c.entropy  = h;

    uint32_t bestDelta = 0;
    for (int d = 1; d < 256; d++)
        if (deltas[d] > bestDelta) { bestDelta = deltas[d]; c.step = d; }
    const double pairs = n - 1;
    const double maxH = std::log2(std::min(256.0, n));

    if (samples < 2)                                        c.field = Field::NONE;
    else if (c.topShare >= 0.98)                            c.field = Field::FIXED;
    else if (pairs > 0 && bestDelta >= 0.9 * pairs)         c.field = Field::COUNTER;
    else if (printable >= 0.9 * n && c.distinct > 16)       c.field = Field::TEXT;
    else if (maxH > 0 && h >= 0.9 * maxH)                   c.field = Field::RANDOM;
    else if (c.distinct <= 16)                              c.field = Field::FEW;
    else                                                    c.field = Field::VARIED;
    if (c.field != Field::COUNTER) c.step = 0;
    return c;
}

const char* RecordProfile::fieldName(Field f) {
    switch (f) {
        case Field::FIXED:   return "fisso";
        case Field::COUNTER: return "contatore";
        case Field::TEXT:    return "testo";
        case Field::FEW:     return "pochi valori";
        case Field::VARIED:  return "vario";
        case Field::RANDOM:  return "casuale";
        case Field::NONE:    break;
    }
    return "-";
}

void RecordProfile::fieldColor(Field f, uint8_t& r, uint8_t& g, uint8_t& b) {
    switch (f) {
        case Field::FIXED:   r = 230; g = 230; b = 230; return;   // bianco
        case Field::COUNTER: r = 255; g = 210; b =  40; return;   // giallo
        case Field::TEXT:    r =  80; g = 160; b = 255; return;   // azzurro
        case Field::FEW:     r =  60; g = 220; b =  90; return;   // verde
        case Field::VARIED:  r = 200; g = 120; b = 255; return;   // viola
        case Field::RANDOM:  r = 255; g =  70; b =  60; return;   // rosso
        case Field::NONE:    break;
    }
    r = g = b = 128;
}

std::string RecordProfile::pattern(int maxBytes) const {
    static const char HEX[] = "0123456789ABCDEF";
    std::string s;
    for (int j = 0; j < m_period && j < maxBytes; j++) {
        const Column& c = column(j);
        switch (c.field) {
            case Field::FIXED:   s += HEX[c.top >> 4]; s += HEX[c.top & 15]; break;
            case Field::COUNTER: s += "C "; break;
            case Field::TEXT:    s += "t "; break;
            case Field::FEW:     s += "e "; break;
            case Field::VARIED:  s += ". "; break;
            case Field::RANDOM:  s += "? "; break;
            case Field::NONE:    s += "- "; break;
        }
    }
    return s;
}
