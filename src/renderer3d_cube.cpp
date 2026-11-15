#include "renderer3d.h"
#include "renderer3d_draw.h"

// Il cubo attorno alla nuvola: dice dove siamo. Ogni punto e' una tripla di
// byte, quindi ogni spigolo va da 0x00 a 0xFF. I tre assi escono dall'angolo
// 00 00 00, ognuno con il suo colore; gli altri spigoli sono grigi.

namespace {
using Draw3D::Drawer;
using Draw3D::Vec;

const olc::Pixel AXIS[3] = {{240, 100, 100}, {100, 220, 120}, {110, 160, 255}};
const char* const AXIS_NAME[3] = {"byte 1", "byte 2", "byte 3"};

} // namespace

void Renderer3D::drawCube(olc::PixelGameEngine* pge) const {
    if (m_coords != CoordSystem::CUBE) { drawGuides(pge); return; }
    Drawer d{pge, m_cam, m_cam.rotation(), m_vp};
    const olc::Pixel edge(70, 70, 95);

    // I 12 spigoli: da ogni angolo, verso i vicini con una coordinata in piu'.
    // Quelli che partono da 000 sono gli assi, vanno disegnati per ultimi.
    for (int c = 1; c < 8; c++)
        for (int axis = 0; axis < 3; axis++) {
            if (c & (1 << axis)) continue;
            int e = c | (1 << axis);
            auto corner = [&](int i) {
                return d.at(i & 1 ? 1.0f : -1.0f, i & 2 ? 1.0f : -1.0f, i & 4 ? 1.0f : -1.0f);
            };
            d.line(corner(c), corner(e), edge);
        }

    const Vec origin = d.at(-1, -1, -1);
    for (int a = 0; a < 3; a++) {
        float end[3] = {-1, -1, -1}, mid[3] = {-1, -1, -1};
        end[a] = 1;
        mid[a] = 0;
        const Vec tip = d.at(end[0], end[1], end[2]);
        d.line(origin, tip, AXIS[a]);
        d.label(d.at(mid[0], mid[1], mid[2]), AXIS_NAME[a], AXIS[a]);
        d.label(tip, "FF", AXIS[a]);
    }
    d.label(origin, "00", olc::WHITE);
}

void Renderer3D::drawMarker(olc::PixelGameEngine* pge, uint32_t key,
                            int radius, olc::Pixel col) const {
    const Camera3D::Vec p = m_map.at(m_coords, (key >> 16) & 0xFF, (key >> 8) & 0xFF, key & 0xFF);
    float sx, sy;
    if (!m_cam.toScreen(m_cam.toCamera(m_cam.rotation(), p.x, p.y, p.z), m_vp, sx, sy)) return;
    if (sx < m_vp.x || sy < m_vp.y || sx >= m_vp.x + m_vp.w || sy >= m_vp.y + m_vp.h) return;
    pge->DrawCircle((int)sx, (int)sy, radius, col);
}
