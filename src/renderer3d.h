#pragma once
#include <vector>
#include <array>
#include <cstdint>
#include "trigraph.h"
#include "../libs/olcPixelGameEngine.h"

// Matrice 3x3 per rotazioni 3D
// Usiamo row-major: m[riga][colonna]
using Mat3 = std::array<std::array<float, 3>, 3>;

class Renderer3D {
public:
    Renderer3D();

    // Renderizza la nuvola di punti su un buffer RGB 512x512
    // Il buffer viene poi disegnato su olcPGE
    void render(olc::PixelGameEngine* pge,
                const std::vector<Point3D>& points,
                int offsetX, int offsetY,
                int width,   int height);

    // Aggiorna la rotazione in base al drag del mouse
    // dx, dy = delta pixels del mouse questo frame
    void rotate(float dx, float dy);

    // Reset della rotazione alla vista default (isometrica)
    void resetRotation();

    // Zoom in/out con la rotella del mouse
    void zoom(float delta);

private:
    // Proietta un punto 3D (gia' ruotato) in 2D
    // Ritorna le coordinate pixel dentro il rettangolo offsetX/Y + w/h
    bool project(float x, float y, float z,
                 int offsetX, int offsetY,
                 int width,   int height,
                 int& outX,   int& outY) const;

    // Costruisce la matrice di rotazione da angoli Euler
    Mat3 buildRotationMatrix() const;

    // Moltiplica un vettore per la matrice di rotazione
    void applyRotation(const Mat3& mat,
                       float  ix, float  iy, float  iz,
                       float& ox, float& oy, float& oz) const;

    float m_angleX = 0.4f;   // rotazione attorno asse X (radianti)
    float m_angleY = 0.6f;   // rotazione attorno asse Y
    float m_zoom   = 1.0f;   // fattore di zoom prospettico
};
