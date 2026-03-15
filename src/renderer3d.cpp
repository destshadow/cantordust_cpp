#include "renderer3d.h"
#include <cmath>
#include <algorithm>

Renderer3D::Renderer3D() {
    // Vista iniziale leggermente ruotata → si vede bene il cubo
    m_angleX = 0.4f;
    m_angleY = 0.6f;
    m_zoom   = 1.0f;
}

// --- Costruisce la matrice di rotazione combinata Ry * Rx ---
//
// Rotazione attorno X:          Rotazione attorno Y:
// [1,    0,     0  ]            [ cos(b), 0, sin(b)]
// [0,  cos(a),-sin(a)]          [   0,    1,   0   ]
// [0,  sin(a), cos(a)]          [-sin(b), 0, cos(b)]
//
// Il prodotto delle due da la matrice finale
Mat3 Renderer3D::buildRotationMatrix() const {
    float cx = std::cos(m_angleX), sx = std::sin(m_angleX);
    float cy = std::cos(m_angleY), sy = std::sin(m_angleY);

    // Ry * Rx
    Mat3 m;
    m[0][0] =  cy;       m[0][1] = sy*sx;    m[0][2] = sy*cx;
    m[1][0] =  0;        m[1][1] = cx;       m[1][2] = -sx;
    m[2][0] = -sy;       m[2][1] = cy*sx;    m[2][2] = cy*cx;
    return m;
}

// --- Moltiplica vettore (ix,iy,iz) per la matrice ---
void Renderer3D::applyRotation(const Mat3& m,
                                float  ix, float  iy, float  iz,
                                float& ox, float& oy, float& oz) const {
    ox = m[0][0]*ix + m[0][1]*iy + m[0][2]*iz;
    oy = m[1][0]*ix + m[1][1]*iy + m[1][2]*iz;
    oz = m[2][0]*ix + m[2][1]*iy + m[2][2]*iz;
}

// --- Proiezione prospettica ---
// dist = distanza virtuale della camera
// La formula e': px = (x / (z + dist)) * scale + centerX
bool Renderer3D::project(float x, float y, float z,
                          int offsetX, int offsetY,
                          int width,   int height,
                          int& outX,   int& outY) const {
    const float dist  = 3.0f;         // distanza camera
    const float scale = (width * 0.45f) * m_zoom;

    float denom = z + dist;
    if (denom < 0.001f) return false;  // punto dietro la camera

    // Centro del rettangolo di rendering
    float cx = offsetX + width  * 0.5f;
    float cy = offsetY + height * 0.5f;

    outX = static_cast<int>(cx + (x / denom) * scale);
    outY = static_cast<int>(cy - (y / denom) * scale); // Y invertita (schermo)

    // Scarta punti fuori dal rettangolo
    if (outX < offsetX || outX >= offsetX + width)  return false;
    if (outY < offsetY || outY >= offsetY + height) return false;

    return true;
}

// --- Render principale ---
void Renderer3D::render(olc::PixelGameEngine* pge,
                         const std::vector<Point3D>& points,
                         int offsetX, int offsetY,
                         int width,   int height) {
    if (points.empty()) return;

    Mat3 mat = buildRotationMatrix();

    for (const auto& p : points) {
        // 1. Ruota il punto
        float rx, ry, rz;
        applyRotation(mat, p.x, p.y, p.z, rx, ry, rz);

        // 2. Proietta in 2D
        int px, py;
        if (!project(rx, ry, rz, offsetX, offsetY, width, height, px, py))
            continue;

        // 3. Disegna il pixel con il colore del punto
        // L'intensita' modula la luminosita'
        uint8_t r = static_cast<uint8_t>(p.r * p.intensity);
        uint8_t g = static_cast<uint8_t>(p.g * p.intensity);
        uint8_t b = static_cast<uint8_t>(p.b * p.intensity);

        // Minima luminosita' per punti rari (almeno visibili)
        if (r < 20 && g < 20 && b < 20) b = 40;

        pge->Draw(px, py, olc::Pixel(r, g, b));
    }
}

// --- Rotazione con mouse drag ---
// dx e dy sono in pixel → convertiamo in radianti
void Renderer3D::rotate(float dx, float dy) {
    const float sensitivity = 0.005f;
    m_angleY -= dx * sensitivity;
    m_angleX += dy * sensitivity;

    // Clamp angleX per evitare gimbal lock
    m_angleX = std::clamp(m_angleX, -1.5f, 1.5f);
}

void Renderer3D::resetRotation() {
    m_angleX = 0.4f;
    m_angleY = 0.6f;
}

void Renderer3D::zoom(float delta) {
    m_zoom += delta * 0.1f;
    m_zoom = std::clamp(m_zoom, 0.3f, 3.0f);
}
