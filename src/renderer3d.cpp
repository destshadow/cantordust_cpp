#include "renderer3d.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace {
// Oltre questi punti, mentre ci si muove se ne disegna solo una parte
constexpr size_t MAX_MOVING_POINTS = 1500000;
constexpr float  REFINE_AFTER      = 0.15f;     // secondi da fermi
constexpr int    PICK_RADIUS       = 4;         // pixel attorno al mouse

// Campione stabile: la stessa tripla c'e' o non c'e' in tutti i fotogrammi,
// e non segue l'ordine delle chiavi (eviterebbe righe regolari nei dati)
inline uint32_t sampleHash(uint32_t key) { return (key * 2654435761u) >> 16; }
} // namespace

void Renderer3D::rasterize(const std::vector<Point3D>& points, uint32_t sample,
                           const std::vector<bool>* highlight, olc::Pixel tint) {
    float coord[256];
    for (int i = 0; i < 256; i++) coord[i] = i / 127.5f - 1.0f;

    const Mat3 mat = m_cam.rotation();
    const Camera3D::Viewport view{0, 0, m_vp.w, m_vp.h};   // coordinate del fotogramma
    m_frame.clear();

    for (const Point3D& p : points) {
        if (sampleHash(p.key) >= sample) continue;
        Camera3D::Vec v = m_cam.toCamera(mat, coord[p.b0()], coord[p.b1()], coord[p.b2()]);
        float sx, sy;
        if (!m_cam.toScreen(v, view, sx, sy)) continue;
        if (sx < 0.0f || sy < 0.0f || sx >= (float)m_vp.w || sy >= (float)m_vp.h) continue;

        if (!highlight)
            m_frame.add((int)sx, (int)sy, p.r, p.g, p.b, (uint32_t)p.level + 1, p.key);
        else if ((*highlight)[p.key])
            m_frame.add((int)sx, (int)sy, tint.r, tint.g, tint.b, (uint32_t)p.level + 64, p.key);
        else
            m_frame.add((int)sx, (int)sy, 60, 60, 68, 1, p.key);    // accanto ai colorati pesa poco
    }
    m_frame.resolve();
}

void Renderer3D::render(olc::PixelGameEngine* pge,
                        const std::vector<Point3D>& points,
                        const Camera3D::Viewport& vp, float dt,
                        const std::vector<bool>* highlight, olc::Pixel tint) {
    if (vp.w != m_vp.w || vp.h != m_vp.h) m_dirty = true;
    m_vp = vp;
    m_frame.resize(vp.w, vp.h);

    const uint32_t ALL = 1u << 16;
    if (m_dirty || m_moved) {
        bool coarse = m_moved && points.size() > MAX_MOVING_POINTS;
        uint32_t sample = coarse ? (uint32_t)(ALL * MAX_MOVING_POINTS / points.size()) : ALL;
        rasterize(points, sample, highlight, tint);
        m_refine = coarse;
        m_idle   = 0.0f;
        m_dirty  = m_moved = false;
    } else if (m_refine && (m_idle += dt) >= REFINE_AFTER) {
        rasterize(points, ALL, highlight, tint);
        m_refine = false;
    }

    // Copia sullo schermo: olc::Pixel e' un RGBA a 32 bit
    static_assert(sizeof(olc::Pixel) == sizeof(uint32_t), "olc::Pixel non e' RGBA32");
    olc::Sprite* target = pge->GetDrawTarget();
    const uint32_t* src = m_frame.pixels().data();
    for (int y = 0; y < vp.h && vp.y + y < target->height; y++)
        std::memcpy(static_cast<void*>(target->GetData() + (size_t)(vp.y + y) * target->width + vp.x),
                    src + (size_t)y * vp.w, (size_t)vp.w * sizeof(uint32_t));
}

uint32_t Renderer3D::keyAt(int x, int y) const {
    return m_frame.keyNear(x - m_vp.x, y - m_vp.y, PICK_RADIUS);
}

void Renderer3D::rotate(float dx, float dy) {
    m_cam.orbit(dx, dy);
    m_moved = true;
}

void Renderer3D::pan(float dx, float dy) {
    m_cam.panX += dx * 0.8f;
    m_cam.panY += dy * 0.8f;
    m_moved = true;
}

// Muove la camera come in un gioco FPS: avanti = verso dove guardi.
// La camera vive nello spazio GIA' RUOTATO, quindi il movimento va applicato
// cosi' com'e': ruotarlo di nuovo lo manderebbe nella direzione sbagliata.
//   forward > 0 → avanti (W),  right > 0 → destra (D),  up > 0 → su (Q)
void Renderer3D::moveCamera(float forward, float right, float up) {
    const float speed = 0.02f;
    // Limiti: non allontanarsi troppo e non oltrepassare il piano di proiezione
    m_cam.camX = std::clamp(m_cam.camX + right   * speed, -2.0f, 2.0f);
    m_cam.camY = std::clamp(m_cam.camY + up      * speed, -2.0f, 2.0f);
    m_cam.camZ = std::clamp(m_cam.camZ + forward * speed, -4.0f, 2.9f);
    m_moved = true;
}

void Renderer3D::resetRotation() {
    const float zoom = m_cam.zoom;     // R rimette la camera, non lo zoom
    m_cam = Camera3D{};
    m_cam.zoom = zoom;
    m_moved = true;
}

void Renderer3D::zoom(float delta) {
    m_cam.zoom = std::clamp(m_cam.zoom + delta * 0.1f, 0.3f, 3.0f);
    m_moved = true;
}
