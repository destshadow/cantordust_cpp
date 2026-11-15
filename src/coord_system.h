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

// Cilindro e sfera stanno dentro il cubo e a parita' di camera sembrerebbero
// piu' piccoli: si ingrandiscono per riempire la vista quanto lui
inline float coordScale(CoordSystem s) {
    switch (s) {
        case CoordSystem::CYLINDER: return 1.25f;
        case CoordSystem::SPHERE:   return 1.5f;
        default:                    return 1.0f;
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
        const float k = coordScale(s);
        switch (s) {
            case CoordSystem::CYLINDER: {
                const float r = m_unit[b1] * k;
                return {m_cosTurn[b0] * r, m_sinTurn[b0] * r, m_line[b2] * k};
            }
            case CoordSystem::SPHERE: {
                const float r = m_unit[b2] * m_sinHalf[b1] * k;
                return {m_cosTurn[b0] * r, m_sinTurn[b0] * r, m_cosHalf[b1] * m_unit[b2] * k};
            }
            default:
                return {m_line[b0], m_line[b1], m_line[b2]};
        }
    }

private:
    float m_line[256], m_unit[256];
    float m_cosTurn[256], m_sinTurn[256], m_sinHalf[256], m_cosHalf[256];
};
