#pragma once
#include "camera3d.h"
#include <cmath>
#include <cstdint>

// Dove finisce una tripla di byte nello spazio della vista 3D.
// Gli stessi punti in tre forme, come nel cantor.dust originale:
//
//   CUBO      i tre byte sono X, Y, Z
//   CILINDRO  byte 1 = angolo, byte 2 = distanza dall'asse, byte 3 = altezza
//   SFERA     byte 1 = angolo attorno all'asse, byte 2 = angolo dal polo,
//             byte 3 = distanza dal centro
//
// Nelle forme curve i valori vicini a 00 e a FF di un byte "angolo" finiscono
// accanto: aiuta a vedere strutture che nel cubo stanno su facce opposte.
// Formule come in Veles (codilime/veles, trigram/coords.glsl).
enum class CoordSystem { CUBE = 0, CYLINDER = 1, SPHERE = 2 };

inline const char* coordSystemName(CoordSystem s) {
    switch (s) {
        case CoordSystem::CYLINDER: return "cilindro";
        case CoordSystem::SPHERE:   return "sfera";
        default:                    return "cubo";
    }
}

// Le funzioni trigonometriche si calcolano una volta per valore di byte:
// in un fotogramma ci sono milioni di punti
class CoordMap {
public:
    CoordMap() {
        const float TAU = 6.283185307179586f;
        for (int i = 0; i < 256; i++) {
            const float t = (i + 0.5f) / 256.0f;      // 0..1
            m_line[i]   = Camera3D::coord(i);         // -1..+1
            m_unit[i]   = t;
            m_cosTurn[i] = std::cos(t * TAU);
            m_sinTurn[i] = std::sin(t * TAU);
            m_sinHalf[i] = std::sin(t * TAU / 2);
            m_cosHalf[i] = std::cos(t * TAU / 2);
        }
    }

    Camera3D::Vec at(CoordSystem s, uint8_t b0, uint8_t b1, uint8_t b2) const {
        switch (s) {
            case CoordSystem::CYLINDER:
                return {m_cosTurn[b0] * m_unit[b1], m_sinTurn[b0] * m_unit[b1], m_line[b2]};
            case CoordSystem::SPHERE: {
                const float r = m_unit[b2] * m_sinHalf[b1];
                return {m_cosTurn[b0] * r, m_sinTurn[b0] * r, m_cosHalf[b1] * m_unit[b2]};
            }
            default:
                return {m_line[b0], m_line[b1], m_line[b2]};
        }
    }

private:
    float m_line[256], m_unit[256];
    float m_cosTurn[256], m_sinTurn[256], m_sinHalf[256], m_cosHalf[256];
};
