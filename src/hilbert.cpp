#include "hilbert.h"
#include <cmath>

HilbertCurve::HilbertCurve(int order)
    : m_order(order)
    , m_size(1 << order)  // 2^order
{}

// Rotazione/riflessione di un sotto-quadrante
// Questo e' il cuore dell'algoritmo di Hilbert
void HilbertCurve::rot(int n, int& x, int& y,
                        int rx, int ry) const {
    if (ry == 0) {
        if (rx == 1) {
            x = n - 1 - x;
            y = n - 1 - y;
        }
        std::swap(x, y);
    }
}

// Indice 1D → punto 2D sulla curva di Hilbert
// Algoritmo: decompone l'indice in bit e applica
// rotazioni successive per ogni livello della curva
std::pair<int,int> HilbertCurve::indexToPoint(int idx) const {
    int x = 0, y = 0;

    for (int s = 1; s < m_size; s *= 2) {
        int rx = 1 & (idx / 2);
        int ry = 1 & (idx ^ rx);
        rot(s, x, y, rx, ry);
        x += s * rx;
        y += s * ry;
        idx /= 4;
    }
    return {x, y};
}

// Punto 2D → indice 1D sulla curva di Hilbert
// Operazione inversa: ricostruisce l'indice dai bit
int HilbertCurve::pointToIndex(int x, int y) const {
    int idx = 0;
    for (int s = m_size / 2; s > 0; s /= 2) {
        int rx = (x & s) > 0 ? 1 : 0;
        int ry = (y & s) > 0 ? 1 : 0;
        idx += s * s * ((3 * rx) ^ ry);
        rot(s, x, y, rx, ry);
    }
    return idx;
}
