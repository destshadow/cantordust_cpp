#pragma once

// --- Dimensioni finestra ---
static constexpr int WIN_W = 1280;
static constexpr int WIN_H = 800;

// Layout (vedi TODO.md):
//
//   ┌──────────────────── topbar (tab) ─────────────────────┐
//   ├─ NAV ─┬─ legenda ──────────────────────┬─┬─ INSPECTOR ─┤
//   │ barre │ vista (CANVAS)                 │V│             │
//   │       │                                │ │             │
//   │       ├─ scrollbar H ──────────────────┴─┤             │
//   ├───────┴──────── file / navigazione / comandi ──────────┤

// --- Barre orizzontali ---
static constexpr int TOPBAR_H      = 40;   // tab cliccabili
static constexpr int LEGEND_H      = 16;   // "cosa mostra questa vista"
static constexpr int SCROLLBAR_H   = 12;   // scrollbar canvas orizzontale
static constexpr int FILE_STATUS_H = 20;   // file + byte sotto cursore
static constexpr int FILE_NAV_H    = 12;   // scrollbar navigazione file
static constexpr int CMD_STATUS_H  = 18;   // barra comandi in basso
static constexpr int PADDING       = 8;    // padding generico

// --- Colonne ---
static constexpr int NAV_W         = 136;  // barre di navigazione (passo 2)
static constexpr int INSPECTOR_W   = 328;  // pannello informazioni (39 char utili)
static constexpr int SCROLLBAR_V_W = 12;   // scrollbar verticale canvas

// --- Area canvas (la vista) ---
static constexpr int LEGEND_Y      = TOPBAR_H;
static constexpr int CANVAS_X      = NAV_W;
static constexpr int CANVAS_Y      = TOPBAR_H + LEGEND_H;
static constexpr int CANVAS_W      = WIN_W - NAV_W - INSPECTOR_W - SCROLLBAR_V_W;
static constexpr int CANVAS_H      = WIN_H - CANVAS_Y
                                           - SCROLLBAR_H
                                           - FILE_STATUS_H
                                           - FILE_NAV_H
                                           - CMD_STATUS_H
                                           - PADDING;
static constexpr int SCROLLBAR_V_X = CANVAS_X + CANVAS_W;

// Pannello in fondo all'area canvas per i raw pixel:
// grafico autocorrelazione + slider larghezza/offset
static constexpr int RAW_PANEL_H   = 92;

// --- Posizioni Y delle barre dal basso ---
static constexpr int SCROLLBAR_H_Y  = CANVAS_Y + CANVAS_H;
static constexpr int FILE_STATUS_Y  = SCROLLBAR_H_Y + SCROLLBAR_H;
static constexpr int FILE_NAV_Y     = FILE_STATUS_Y  + FILE_STATUS_H;
static constexpr int CMD_STATUS_Y   = FILE_NAV_Y     + FILE_NAV_H;

// --- Pannelli laterali (dalla topbar alla barra di stato) ---
static constexpr int SIDE_Y         = TOPBAR_H;
static constexpr int SIDE_H         = FILE_STATUS_Y - TOPBAR_H;
static constexpr int INSPECTOR_X    = WIN_W - INSPECTOR_W;

// --- Colori ricorrenti ---
// Li definiamo come funzioni inline per non avere problemi con
// l'inizializzazione statica di olc::Pixel
#include <olcPixelGameEngine.h>
inline olc::Pixel COL_BG()        { return olc::Pixel(18,  18,  45);  }
inline olc::Pixel COL_BG_LIGHT()  { return olc::Pixel(30,  30,  70);  }
inline olc::Pixel COL_BORDER()    { return olc::Pixel(60,  60, 120);  }
inline olc::Pixel COL_SCROLLBAR() { return olc::Pixel(40,  40,  80);  }
inline olc::Pixel COL_CURSOR()    { return olc::Pixel(80, 140, 200);  }
inline olc::Pixel COL_CYAN()      { return olc::Pixel( 0, 200, 220);  }
inline olc::Pixel COL_YELLOW()    { return olc::Pixel(255, 220,  50); }
inline olc::Pixel COL_GREEN()     { return olc::Pixel( 80, 220, 100); }
