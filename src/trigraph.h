#pragma once
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <cmath>

// Un punto nello spazio 3D 256x256x256
// x = byte[i], y = byte[i+1], z = byte[i+2]
struct Point3D {
    float x, y, z;     // coordinate normalizzate [-1, +1]
    float intensity;    // frequenza normalizzata → luminosita'
    uint8_t r, g, b;   // colore calcolato da posizione
};

class TriGraph {
public:
    // Calcola la nuvola di punti 3D dai bytes
    // start/end permettono di lavorare su una finestra del file
    void compute(const std::vector<uint8_t>& bytes,
                 size_t start = 0, size_t end = 0);
    void clear();

    const std::vector<Point3D>& getPoints() const { return m_points; }
    size_t getPointCount() const { return m_points.size(); }

private:
    std::vector<Point3D> m_points;
};
