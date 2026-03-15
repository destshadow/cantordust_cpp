#pragma once
#include <vector>
#include <array>
#include <cstdint>
#include "trigraph.h"
#include "../libs/olcPixelGameEngine.h"

using Mat3 = std::array<std::array<float, 3>, 3>;

class Renderer3D {
public:
    Renderer3D();

    void render(olc::PixelGameEngine* pge,
                const std::vector<Point3D>& points,
                int offsetX, int offsetY,
                int width,   int height);

    // Rotazione (mouse sinistro)
    void rotate(float dx, float dy);

    // Pan schermo (mouse destro)
    void pan(float dx, float dy);

    // Movimento camera nello spazio 3D (WASD + QE)
    // forward/right/up sono in unita' mondo [-1,+1]
    void moveCamera(float forward, float right, float up);

    void resetRotation();
    void zoom(float delta);

private:
    bool project(float x, float y, float z,
                 int offsetX, int offsetY,
                 int width,   int height,
                 int& outX,   int& outY) const;

    Mat3 buildRotationMatrix() const;
    void applyRotation(const Mat3& mat,
                       float  ix, float  iy, float  iz,
                       float& ox, float& oy, float& oz) const;

    // Rotazione mondo
    float m_angleX =  0.4f;
    float m_angleY =  0.6f;
    float m_zoom   =  1.0f;
    float m_panX   =  0.0f;
    float m_panY   =  0.0f;

    // Posizione camera nello spazio 3D
    // Parte in (0,0,0) = centro del cubo
    float m_camX   =  0.0f;
    float m_camY   =  0.0f;
    float m_camZ   =  0.0f;
};
