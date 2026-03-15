#pragma once

// --- Dimensioni finestra ---
static constexpr int WIN_W = 1024;
static constexpr int WIN_H = 768;

// --- Layout barre ---
static constexpr int TOPBAR_H      = 40;   // tab cliccabili
static constexpr int SCROLLBAR_H   = 12;   // scrollbar canvas orizzontale
static constexpr int FILE_STATUS_H = 20;   // file + byte sotto cursore
static constexpr int FILE_NAV_H    = 12;   // scrollbar navigazione file
static constexpr int CMD_STATUS_H  = 18;   // barra comandi in basso
static constexpr int PADDING       = 8;    // padding generico

// --- Area canvas ---
static constexpr int SCROLLBAR_V_W = 12;   // scrollbar verticale canvas
static constexpr int CANVAS_X      = 0;
static constexpr int CANVAS_Y      = TOPBAR_H;
static constexpr int CANVAS_W      = WIN_W - SCROLLBAR_V_W;
static constexpr int CANVAS_H      = WIN_H - TOPBAR_H
                                           - SCROLLBAR_H
                                           - FILE_STATUS_H
                                           - FILE_NAV_H
                                           - CMD_STATUS_H
                                           - PADDING;

// --- Posizioni Y delle barre dal basso ---
static constexpr int SCROLLBAR_H_Y  = CANVAS_Y + CANVAS_H;
static constexpr int FILE_STATUS_Y  = SCROLLBAR_H_Y + SCROLLBAR_H;
static constexpr int FILE_NAV_Y     = FILE_STATUS_Y  + FILE_STATUS_H;
static constexpr int CMD_STATUS_Y   = FILE_NAV_Y     + FILE_NAV_H;

// --- Colori ricorrenti ---
// Li definiamo come funzioni inline per non avere problemi con
// l'inizializzazione statica di olc::Pixel
#include "../libs/olcPixelGameEngine.h"
inline olc::Pixel COL_BG()        { return olc::Pixel(18,  18,  45);  }
inline olc::Pixel COL_BG_LIGHT()  { return olc::Pixel(30,  30,  70);  }
inline olc::Pixel COL_BORDER()    { return olc::Pixel(60,  60, 120);  }
inline olc::Pixel COL_SCROLLBAR() { return olc::Pixel(40,  40,  80);  }
inline olc::Pixel COL_CURSOR()    { return olc::Pixel(80, 140, 200);  }
inline olc::Pixel COL_CYAN()      { return olc::Pixel( 0, 200, 220);  }
inline olc::Pixel COL_YELLOW()    { return olc::Pixel(255, 220,  50); }
inline olc::Pixel COL_GREEN()     { return olc::Pixel( 80, 220, 100); }
