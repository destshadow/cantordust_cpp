#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

// Il fotogramma della nuvola 3D, largo come la vista.
//
// Idea presa dal CantorDust originale: i punti non si sovrascrivono ma si
// sommano in ogni pixel, cosi' si vede la densita'; e ogni pixel ricorda il
// suo punto piu' pesante, cosi' sapere cosa c'e' sotto il mouse e' immediato.
//
// Il colore di un pixel e' la media dei colori dei suoi punti, la luminosita'
// dipende da quanti ne contiene: sommando e basta, i dati densi diventerebbero
// tutti bianchi.
class CloudFrame {
public:
    static constexpr uint32_t NO_KEY = 0xFFFFFFFFu;

    void resize(int w, int h);
    void clear();

    void add(int x, int y, uint8_t r, uint8_t g, uint8_t b,
             uint32_t weight, uint32_t key) {
        Cell& c = m_cells[(size_t)y * m_w + x];
        c.r += (float)r * weight;
        c.g += (float)g * weight;
        c.b += (float)b * weight;
        c.weight += weight;
        if (weight > c.heaviest) { c.heaviest = weight; c.key = key; }
    }

    // Trasforma i pixel accumulati in RGBA (come olc::Pixel), una riga dopo
    // l'altra
    void resolve();
    const std::vector<uint32_t>& pixels() const { return m_pixels; }

    // Il punto piu' vicino a (x, y) entro `radius` pixel; NO_KEY se nessuno
    uint32_t keyNear(int x, int y, int radius) const;

    int width()  const { return m_w; }
    int height() const { return m_h; }

private:
    struct Cell {
        float r = 0, g = 0, b = 0;
        uint32_t weight = 0, heaviest = 0, key = NO_KEY;
    };
    int m_w = 0, m_h = 0;
    std::vector<Cell>     m_cells;
    std::vector<uint32_t> m_pixels;
};
