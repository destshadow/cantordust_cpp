#pragma once
#include "camera3d.h"
#include "line_clip.h"
#include <olcPixelGameEngine.h>
#include <string>

// Linee e scritte nello spazio della vista 3D, tagliate sul viewport.
// Usato dal cubo e dalle guide di cilindro e sfera.
namespace Draw3D {
using Vec = Camera3D::Vec;
using Viewport = Camera3D::Viewport;

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
} // namespace Draw3D
