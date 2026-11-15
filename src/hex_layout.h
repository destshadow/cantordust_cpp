#pragma once
#include <cstddef>

// La griglia della vista Hex: 16 byte per riga, in caratteri da 8 pixel.
//
//   00001A40  7F 45 4C 46 02 01 01 00  00 00 00 00 00 00 00 00  .ELF............
//
// Solo conti, nessuna grafica, cosi' si possono provare nei test.
namespace HexLayout {

constexpr int PER_ROW  = 16;
constexpr int CHAR_W   = 8;
constexpr int COL_HEX  = 10;                       // carattere dove inizia l'hex
constexpr int COL_ASCII = COL_HEX + 16 * 3 + 2;     // dove inizia il testo
constexpr int COLS     = COL_ASCII + PER_ROW;       // caratteri in tutto

inline size_t rowCount(size_t bytes) { return (bytes + PER_ROW - 1) / PER_ROW; }

// Carattere dove inizia il byte `i` della riga (c'e' uno spazio in piu' a meta')
inline int hexCol(int i)  { return COL_HEX + i * 3 + (i >= 8 ? 1 : 0); }
inline int textCol(int i) { return COL_ASCII + i; }

// Il byte della riga sotto il pixel x (contato dal bordo della griglia), sia
// nella parte hex sia nel testo; -1 se x non e' su un byte
inline int byteAtX(int x) {
    if (x < 0) return -1;
    const int col = x / CHAR_W;
    if (col >= COL_ASCII) return col < COLS ? col - COL_ASCII : -1;
    for (int i = 0; i < PER_ROW; i++)
        if (col >= hexCol(i) && col < hexCol(i) + 2) return i;
    return -1;
}

} // namespace HexLayout
