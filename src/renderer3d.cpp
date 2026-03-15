#include "renderer3d.h"
#include <cmath>
#include <algorithm>

Renderer3D::Renderer3D() {
    m_angleX = 0.4f;
    m_angleY = 0.6f;
    m_zoom   = 1.0f;
    m_panX   = 0.0f;
    m_panY   = 0.0f;
    m_camX   = 0.0f;
    m_camY   = 0.0f;
    m_camZ   = 0.0f;
}

Mat3 Renderer3D::buildRotationMatrix() const {
    float cx = std::cos(m_angleX), sx = std::sin(m_angleX);
    float cy = std::cos(m_angleY), sy = std::sin(m_angleY);
    Mat3 m;
    m[0][0] =  cy;   m[0][1] = sy*sx;  m[0][2] = sy*cx;
    m[1][0] =  0;    m[1][1] = cx;     m[1][2] = -sx;
    m[2][0] = -sy;   m[2][1] = cy*sx;  m[2][2] = cy*cx;
    return m;
}

void Renderer3D::applyRotation(const Mat3& m,
                                float  ix, float  iy, float  iz,
                                float& ox, float& oy, float& oz) const {
    ox = m[0][0]*ix + m[0][1]*iy + m[0][2]*iz;
    oy = m[1][0]*ix + m[1][1]*iy + m[1][2]*iz;
    oz = m[2][0]*ix + m[2][1]*iy + m[2][2]*iz;
}

bool Renderer3D::project(float x, float y, float z,
                          int offsetX, int offsetY,
                          int width,   int height,
                          int& outX,   int& outY) const {
    const float dist  = 3.0f;
    const float scale = (width * 0.45f) * m_zoom;

    // Sottrai la posizione camera dal punto
    // → i punti si spostano nella direzione opposta alla camera
    float px = x - m_camX;
    float py = y - m_camY;
    float pz = z - m_camZ;

    float denom = pz + dist;
    if (denom < 0.001f) return false;

    float cx = offsetX + width  * 0.5f + m_panX;
    float cy = offsetY + height * 0.5f + m_panY;

    outX = static_cast<int>(cx + (px / denom) * scale);
    outY = static_cast<int>(cy - (py / denom) * scale);

    if (outX < offsetX || outX >= offsetX + width)  return false;
    if (outY < offsetY || outY >= offsetY + height) return false;
    return true;
}

void Renderer3D::render(olc::PixelGameEngine* pge,
                         const std::vector<Point3D>& points,
                         int offsetX, int offsetY,
                         int width,   int height) {
    if (points.empty()) return;

    Mat3 mat = buildRotationMatrix();

    for (const auto& p : points) {
        float rx, ry, rz;
        applyRotation(mat, p.x, p.y, p.z, rx, ry, rz);

        int px, py;
        if (!project(rx, ry, rz, offsetX, offsetY, width, height, px, py))
            continue;

        float boost  = std::min(1.0f, p.intensity * 1.8f);
        uint8_t r = static_cast<uint8_t>(p.r * boost);
        uint8_t g = static_cast<uint8_t>(p.g * boost);
        uint8_t b = static_cast<uint8_t>(p.b * boost);

        uint8_t maxCh = std::max({r, g, b});
        if (maxCh < 60) {
            if (p.b >= p.r && p.b >= p.g) b = 60;
            else if (p.r >= p.g)           r = 60;
            else                            g = 60;
        }

        pge->Draw(px, py, olc::Pixel(r, g, b));
    }
}

void Renderer3D::rotate(float dx, float dy) {
    const float sensitivity = 0.005f;
    m_angleY -= dx * sensitivity;
    m_angleX += dy * sensitivity;
    m_angleX  = std::clamp(m_angleX, -1.5f, 1.5f);
}

void Renderer3D::pan(float dx, float dy) {
    m_panX += dx * 0.8f;
    m_panY += dy * 0.8f;
}

// Muove la camera nello spazio 3D
// forward = avanti/indietro lungo Z
// right   = destra/sinistra lungo X
// up      = su/giu lungo Y
void Renderer3D::moveCamera(float forward, float right, float up) {
    const float speed = 0.02f;

    // La direzione "avanti" dipende dalla rotazione corrente
    // Usiamo la matrice di rotazione per trasformare
    // il vettore di movimento nel sistema del mondo
    Mat3 mat = buildRotationMatrix();

    // Vettore forward locale → mondo
    float wx, wy, wz;
    applyRotation(mat, right, -up, -forward, wx, wy, wz);

    m_camX += wx * speed;
    m_camY += wy * speed;
    m_camZ += wz * speed;

    // Clamp: non uscire troppo dal cubo [-2, +2]
    m_camX = std::clamp(m_camX, -2.0f, 2.0f);
    m_camY = std::clamp(m_camY, -2.0f, 2.0f);
    m_camZ = std::clamp(m_camZ, -2.0f, 2.0f);
}

void Renderer3D::resetRotation() {
    m_angleX = 0.4f;
    m_angleY = 0.6f;
    m_panX   = 0.0f;
    m_panY   = 0.0f;
    m_camX   = 0.0f;
    m_camY   = 0.0f;
    m_camZ   = 0.0f;
}

void Renderer3D::zoom(float delta) {
    m_zoom += delta * 0.1f;
    m_zoom  = std::clamp(m_zoom, 0.3f, 3.0f);
}
