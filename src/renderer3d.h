#pragma once
#include <vector>
#include <cstdint>
#include "camera3d.h"
#include "cloud_frame.h"
#include "trigraph.h"
#include <olcPixelGameEngine.h>

class Renderer3D {
public:
    // Disegna la nuvola nel viewport. Il fotogramma si ricalcola solo quando
    // cambia qualcosa; con troppi punti, mentre ci si muove se ne usa una
    // parte, e tutti appena ci si ferma.
    void render(olc::PixelGameEngine* pge,
                const std::vector<Point3D>& points,
                const Camera3D::Viewport& vp, float dt,
                const std::vector<bool>* highlight = nullptr,
                olc::Pixel tint = olc::WHITE);

    // Da chiamare quando cambiano i punti, i colori o l'evidenziazione
    void invalidate() { m_dirty = true; }

    // Cubo, assi e nomi (renderer3d_cube.cpp), sopra la nuvola
    void drawCube(olc::PixelGameEngine* pge) const;
    // Cerchio attorno a una tripla; se e' fuori vista non disegna nulla
    void drawMarker(olc::PixelGameEngine* pge, uint32_t key,
                    int radius, olc::Pixel col) const;

    // La tripla sotto il punto (x, y) dello schermo
    uint32_t keyAt(int x, int y) const;

    void rotate(float dx, float dy);     // mouse sinistro
    void pan(float dx, float dy);        // mouse destro
    // WASD + QE: forward/right/up in unita' mondo
    void moveCamera(float forward, float right, float up);
    void resetRotation();
    void zoom(float delta);

private:
    void rasterize(const std::vector<Point3D>& points, uint32_t sample,
                   const std::vector<bool>* highlight, olc::Pixel tint);

    Camera3D           m_cam;
    Camera3D::Viewport m_vp{0, 0, 0, 0};
    CloudFrame         m_frame;
    bool  m_dirty  = true;    // da ridisegnare
    bool  m_moved  = false;   // la camera si e' mossa
    bool  m_refine = false;   // l'ultimo fotogramma usa solo una parte dei punti
    float m_idle   = 0.0f;    // secondi da quando la camera e' ferma
};
