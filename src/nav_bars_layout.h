#pragma once
#include "constants.h"

// Geometria delle barre, condivisa tra logica e disegno.
// Colonne: sezioni | file | zona
namespace NavLayout {
constexpr int SEC_X = 4,            SEC_W = 72;
constexpr int BAR_X[2] = {80, 146};
constexpr int BAR_W = 62;
constexpr int HANDLE_H = 9;
constexpr int TRACK_Y = SIDE_Y + 16 + 11 + HANDLE_H;   // titolo + etichette + maniglia
constexpr int TRACK_H = SIDE_H - (TRACK_Y - SIDE_Y) - HANDLE_H - 40;
} // namespace NavLayout
