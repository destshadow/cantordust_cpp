#include "renderer3d.h"
#include "line_clip.h"

// Il cubo attorno alla nuvola: dice dove siamo. Ogni punto e' una tripla di
// byte, quindi ogni spigolo va da 0x00 a 0xFF. I tre assi escono dall'angolo
// 00 00 00, ognuno con il suo colore; gli altri spigoli sono grigi.

namespace {
using Vec = Camera3D::Vec;
using Viewport = Camera3D::Viewport;

const olc::Pixel AXIS[3] = {{240, 100, 100}, {100, 220, 120}, {110, 160, 255}};
const char* const AXIS_NAME[3] = {"byte 1", "byte 2", "byte 3"};

struct Drawer {
    olc::PixelGameEngine* pge;
    const Camera3D& cam;
    Mat3 mat;
    Viewport vp;

    Vec at(float x, float y, float z) const { return cam.toCamera(mat, x, y, z); }

    void line(Vec a, Vec b, olc::Pixel col) const {
        // Un capo puo' stare dietro la camera (siamo dentro al cubo): si
        // taglia prima di proiettare, la divisione lo ribalterebbe
        if (!cam.inFront(a) && !cam.inFront(b)) return;
        const float za = a.z + Camera3D::DIST, zb = b.z + Camera3D::DIST;
        const float t = (Camera3D::NEAR - za) / (zb - za);
        Vec cut{a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t};
        if (!cam.inFront(a)) a = cut;
        else if (!cam.inFront(b)) b = cut;

        float x0, y0, x1, y1;
        if (!cam.toScreen(a, vp, x0, y0) || !cam.toScreen(b, vp, x1, y1)) return;
        if (!clipLine(x0, y0, x1, y1, (float)vp.x, (float)vp.y,
                      (float)(vp.x + vp.w - 1), (float)(vp.y + vp.h - 1))) return;
        pge->DrawLine((int)x0, (int)y0, (int)x1, (int)y1, col);
    }

    void label(Vec p, const std::string& s, olc::Pixel col) const {
        float x, y;
        if (!cam.toScreen(p, vp, x, y)) return;
        int ix = (int)x + 4, iy = (int)y - 4;
        if (ix < vp.x || iy < vp.y || ix + (int)s.size() * 8 > vp.x + vp.w ||
            iy + 8 > vp.y + vp.h) return;
        pge->DrawString(ix, iy, s, col);
    }
};
} // namespace

void Renderer3D::drawCube(olc::PixelGameEngine* pge) const {
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
    const float x = Camera3D::coord((key >> 16) & 0xFF);
    const float y = Camera3D::coord((key >>  8) & 0xFF);
    const float z = Camera3D::coord( key        & 0xFF);
    float sx, sy;
    if (!m_cam.toScreen(m_cam.toCamera(m_cam.rotation(), x, y, z), m_vp, sx, sy)) return;
    if (sx < m_vp.x || sy < m_vp.y || sx >= m_vp.x + m_vp.w || sy >= m_vp.y + m_vp.h) return;
    pge->DrawCircle((int)sx, (int)sy, radius, col);
}
