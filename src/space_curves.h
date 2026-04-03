#pragma once
#include <cstdint>
#include <utility>
#include <vector>
#include <cmath>

// Tipo di curva spaziale
enum class CurveType {
    HILBERT,  // Curva di Hilbert  — massima locality
    ZORDER,   // Z-order/Morton    — buona locality, piu' veloce
    LINEAR,   // Serpentine scan   — semplice, nessuna locality
};

// --- Z-ORDER (Morton code) ---
// Mappa indice 1D ↔ coordinate 2D usando bit interleaving
// Dato un indice i, le sue coordinate (x,y) si ottengono
// separando i bit pari (→ x) e i bit dispari (→ y)
//
// Esempio con 2 bit per asse:
//   idx = 0b1101  →  x = 0b11 = 3, y = 0b10 = 2
class ZOrderCurve {
public:
    // order: numero di bit per asse
    // size  = 2^order (lato griglia)
    explicit ZOrderCurve(int order)
        : m_order(order)
        , m_size(1 << order)
    {}

    // Indice 1D → (x, y)
    std::pair<int,int> indexToPoint(int idx) const {
        int x = 0, y = 0;
        for (int i = 0; i < m_order; i++) {
            // I bit pari vanno a x, i bit dispari a y
            x |= ((idx >> (2*i))   & 1) << i;
            y |= ((idx >> (2*i+1)) & 1) << i;
        }
        return {x, y};
    }

    // (x, y) → indice 1D
    int pointToIndex(int x, int y) const {
        int idx = 0;
        for (int i = 0; i < m_order; i++) {
            idx |= ((x >> i) & 1) << (2*i);
            idx |= ((y >> i) & 1) << (2*i+1);
        }
        return idx;
    }

    int getSize()   const { return m_size; }
    int getOrder()  const { return m_order; }
    int getLength() const { return m_size * m_size; }

private:
    int m_order;
    int m_size;
};

// --- LINEAR (Serpentine scan) ---
// Scorre riga per riga con direzione alternata
// Riga pari:  → → → → →  (da sinistra a destra)
// Riga dispari: ← ← ← ←  (da destra a sinistra)
// Questa alternanza riduce i "salti" rispetto alla
// scansione lineare pura
class LinearCurve {
public:
    explicit LinearCurve(int size)
        : m_size(size)
    {}

    // Indice 1D → (x, y)
    // Le righe dispari vengono specchiate orizzontalmente
    std::pair<int,int> indexToPoint(int idx) const {
        int row = idx / m_size;
        int col = idx % m_size;

        // Righe dispari: inverti la direzione
        if (row % 2 == 1)
            col = m_size - 1 - col;

        return {col, row};
    }

    // (x, y) → indice 1D
    int pointToIndex(int x, int y) const {
        int col = x;
        // Righe dispari: inverti
        if (y % 2 == 1)
            col = m_size - 1 - x;
        return y * m_size + col;
    }

    int getSize()   const { return m_size; }
    int getLength() const { return m_size * m_size; }

private:
    int m_size;
};
