#include "renderer3d.h"
#include "renderer3d_draw.h"
#include <cmath>

// Le guide del cilindro e della sfera: dicono in che forma sono disposti i
// punti, come il cubo di fili per la forma a cubo.

namespace {
using Draw3D::Drawer;
using Draw3D::Vec;

constexpr int   STEPS = 48;
constexpr float TAU   = 6.283185307179586f;

// Cerchio di raggio 1: point(angolo) da' il punto nello spazio della vista
template <class F>
void circle(const Drawer& d, F point, olc::Pixel col) {
    Vec prev = point(0.0f);
    for (int i = 1; i <= STEPS; i++) {
        const Vec cur = point(TAU * i / STEPS);
        d.line(prev, cur, col);
        prev = cur;
    }
}
} // namespace

void Renderer3D::drawGuides(olc::PixelGameEngine* pge) const {
    const Drawer d{pge, m_cam, m_cam.rotation(), m_vp};
    const olc::Pixel edge(70, 70, 95), axis(110, 160, 255);
    const float k = coordScale(m_coords);      // stessa scala dei punti
    auto at = [&](float x, float y, float z) { return d.at(x * k, y * k, z * k); };

    if (m_coords == CoordSystem::CYLINDER) {
        // Byte 1 gira attorno all'asse, byte 2 si allontana, byte 3 sale
        for (float z : {-1.0f, 1.0f})
            circle(d, [&](float a) { return at(std::cos(a), std::sin(a), z); }, edge);
        d.line(at(0, 0, -1), at(0, 0, 1), axis);
        d.line(at(0, 0, -1), at(1, 0, -1), edge);
        d.label(at(0, 0, 1), "byte 3 = FF", axis);
        d.label(at(0, 0, -1), "00", olc::WHITE);
        d.label(at(1, 0, -1), "byte 2 = FF", edge);
        return;
    }
    // Sfera: byte 3 e' la distanza dal centro, byte 2 va da un polo all'altro
    circle(d, [&](float a) { return at(std::cos(a), std::sin(a), 0); }, edge);
    circle(d, [&](float a) { return at(std::cos(a), 0, std::sin(a)); }, edge);
    circle(d, [&](float a) { return at(0, std::cos(a), std::sin(a)); }, edge);
    d.line(at(0, 0, -1), at(0, 0, 1), axis);
    d.label(at(0, 0, 1), "byte 2 = 00", axis);
    d.label(at(0, 0, -1), "FF", axis);
}
