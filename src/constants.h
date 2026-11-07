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
static constexpr int TOPBAR_H      = 26;   // tab cliccabili
static constexpr int LEGEND_H      = 16;   // "cosa mostra questa vista"
static constexpr int SCROLLBAR_H   = 12;   // scrollbar canvas orizzontale
static constexpr int FILE_STATUS_H = 20;   // file + byte sotto cursore
static constexpr int CMD_STATUS_H  = 18;   // barra comandi in basso
static constexpr int PADDING       = 8;    // padding generico

// --- Colonne ---
static constexpr int NAV_W         = 214;  // sezioni + barre FILE e ZONA
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
                                           - CMD_STATUS_H
                                           - PADDING;
static constexpr int SCROLLBAR_V_X = CANVAS_X + CANVAS_W;

// Pannello in fondo all'area canvas per i raw pixel:
// grafico autocorrelazione + slider larghezza/offset
static constexpr int RAW_PANEL_H   = 92;

// --- Posizioni Y delle barre dal basso ---
static constexpr int SCROLLBAR_H_Y  = CANVAS_Y + CANVAS_H;
static constexpr int FILE_STATUS_Y  = SCROLLBAR_H_Y + SCROLLBAR_H;
static constexpr int CMD_STATUS_Y   = FILE_STATUS_Y  + FILE_STATUS_H;

// --- Pannelli laterali (dalla topbar alla barra di stato) ---
static constexpr int SIDE_Y         = TOPBAR_H;
static constexpr int SIDE_H         = FILE_STATUS_Y - TOPBAR_H;
static constexpr int INSPECTOR_X    = WIN_W - INSPECTOR_W;

// --- Colori dell'interfaccia ---
// Ogni colore ha un ruolo. L'interfaccia resta neutra e c'e' un solo accento,
// per cio' che e' attivo o scelto: cosi' spiccano i dati, che sono l'unica
// cosa colorata (sezioni, tipi di byte, le viste).
// Sono funzioni inline per non avere problemi con l'inizializzazione statica
// di olc::Pixel.
#include <olcPixelGameEngine.h>
inline olc::Pixel COL_BG()        { return olc::Pixel( 22,  23,  29); }  // pannelli
inline olc::Pixel COL_BG_LIGHT()  { return olc::Pixel( 34,  36,  44); }  // intestazioni, tab attiva
inline olc::Pixel COL_BG_DEEP()   { return olc::Pixel( 12,  13,  17); }  // dietro ai dati
inline olc::Pixel COL_BORDER()    { return olc::Pixel( 56,  59,  70); }
inline olc::Pixel COL_SCROLLBAR() { return olc::Pixel( 40,  42,  50); }
inline olc::Pixel COL_CURSOR()    { return olc::Pixel(110, 115, 130); }
inline olc::Pixel COL_TEXT()      { return olc::Pixel(205, 208, 214); }  // testo normale
inline olc::Pixel COL_DIM()       { return olc::Pixel(128, 132, 145); }  // etichette, suggerimenti
inline olc::Pixel COL_ACCENT()    { return olc::Pixel(255, 190,  70); }  // attivo, scelto
