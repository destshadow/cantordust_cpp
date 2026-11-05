#pragma once
#include <algorithm>
#include <array>
#include <cmath>

using Mat3 = std::array<std::array<float, 3>, 3>;

// La camera della vista 3D: da un punto del cubo [-1, +1] allo schermo.
// Niente grafica qui: la stessa matematica serve a disegnare i punti, il
// cubo e i segni sopra i punti, e si puo' provare nei test.
struct Camera3D {
    static constexpr float DIST = 3.0f;    // piano di proiezione
    static constexpr float NEAR = 0.05f;   // piu' vicino di cosi' non si proietta

    struct Vec      { float x, y, z; };
    struct Viewport { int x, y, w, h; };

    // Si guarda il cubo dall'alto, con l'angolo 00 00 00 in fondo e i tre assi
    // che vengono verso di noi aprendosi di circa 120 gradi. Un cubo di fili
    // visto cosi' si legge senza ambiguita'; visto dal basso l'occhio lo
    // ribalta e ogni rotazione sembra al contrario.
    float angleX = -0.5f, angleY = 2.4f, zoom = 1.0f;
    float panX = 0.0f, panY = 0.0f;
    float camX = 0.0f, camY = 0.0f, camZ = 0.0f;   // parte dal centro del cubo

    // Trascinamento col mouse, in pixel: la parte del cubo piu' vicina segue
    // il mouse
    void orbit(float dx, float dy) {
        const float sensitivity = 0.005f;
        angleY -= dx * sensitivity;
        angleX -= dy * sensitivity;
        angleX  = std::max(-1.5f, std::min(1.5f, angleX));
    }

    // Prima si gira attorno all'asse verticale del cubo (angleY), poi si
    // inclina attorno all'asse orizzontale dello SCHERMO (angleX). Al
    // contrario, trascinare in verticale inclinerebbe attorno a un asse del
    // cubo, e da dietro il movimento si invertirebbe.
    Mat3 rotation() const {
        float cx = std::cos(angleX), sx = std::sin(angleX);
        float cy = std::cos(angleY), sy = std::sin(angleY);
        Mat3 m;
        m[0][0] =  cy;       m[0][1] = 0;   m[0][2] = sy;
        m[1][0] =  sx * sy;  m[1][1] = cx;  m[1][2] = -sx * cy;
        m[2][0] = -cx * sy;  m[2][1] = sx;  m[2][2] = cx * cy;
        return m;
    }

    // Spazio del cubo -> spazio della camera. La camera vive nello spazio
    // gia' ruotato: si ruota il punto e poi si sottrae la posizione.
    Vec toCamera(const Mat3& m, float x, float y, float z) const {
        return { m[0][0] * x + m[0][1] * y + m[0][2] * z - camX,
                 m[1][0] * x + m[1][1] * y + m[1][2] * z - camY,
                 m[2][0] * x + m[2][1] * y + m[2][2] * z - camZ };
    }

    bool inFront(const Vec& v) const { return v.z + DIST >= NEAR; }

    // Spazio della camera -> pixel. False se il punto e' dietro la camera;
    // il risultato puo' cadere fuori dal viewport.
    bool toScreen(const Vec& v, const Viewport& vp, float& sx, float& sy) const {
        if (!inFront(v)) return false;
        float scale = vp.w * 0.45f * zoom / (v.z + DIST);
        sx = vp.x + vp.w * 0.5f + panX + v.x * scale;
        sy = vp.y + vp.h * 0.5f + panY - v.y * scale;
        return true;
    }
};
