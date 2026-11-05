#pragma once
#include <vector>
#include <cstddef>
#include <cstdint>

// Una tripla di byte consecutivi presente nel file: un punto del cubo
// 256x256x256 con X = byte[i], Y = byte[i+1], Z = byte[i+2].
struct Point3D {
    uint32_t key;          // byte[i]<<16 | byte[i+1]<<8 | byte[i+2]
    uint32_t count;        // quante volte compare
    uint8_t  level;        // frequenza, 0..255 (vedi density.h)
    uint8_t  r, g, b;      // decisi da CloudColor
    int16_t  section;      // sezione in cui prevale, -1 = nessuna

    uint8_t b0() const { return (key >> 16) & 0xFF; }
    uint8_t b1() const { return (key >>  8) & 0xFF; }
    uint8_t b2() const { return  key        & 0xFF; }
};

class TriGraph {
public:
    // Un tratto dei byte passati a compute() che appartiene a una sezione
    struct Range { size_t start, end; int16_t section; };

    void compute(const std::vector<uint8_t>& bytes,
                 const std::vector<Range>& sections = {});
    void clear();

    const std::vector<Point3D>& getPoints() const { return m_points; }
    std::vector<Point3D>&       getPoints()       { return m_points; }
    size_t getPointCount() const { return m_points.size(); }

    // Il punto di una tripla, nullptr se non c'e'
    const Point3D* find(uint32_t key) const;

private:
    std::vector<Point3D> m_points;   // ordinati per key
};
