#pragma once
#include <cstdint>
#include <cstddef>
#include <utility>

// Curva di Hilbert 2D — mappa indice 1D ↔ coordinate 2D
// La proprieta' chiave e' la locality preservation:
// punti vicini nell'indice 1D restano vicini nella griglia 2D
//
// order = ordine della curva (es: 9 → griglia 512x512)
// size  = 2^order (lato della griglia)
class HilbertCurve {
public:
    // Costruisce una curva di ordine 'order'
    // La griglia risultante e' (2^order) x (2^order)
    explicit HilbertCurve(int order);

    // Indice 1D → coordinate 2D (x, y)
    std::pair<int,int> indexToPoint(int idx) const;

    // Coordinate 2D → indice 1D
    int pointToIndex(int x, int y) const;

    int getOrder() const { return m_order; }
    int getSize()  const { return m_size; }  // lato griglia
    int getLength()const { return m_size * m_size; } // totale pixel

private:
    // Rotazione di un quadrante — algoritmo core di Hilbert
    void rot(int n, int& x, int& y, int rx, int ry) const;

    int m_order;
    int m_size;
};
