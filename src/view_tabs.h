#pragma once
#include "view_mode.h"
#include <cstring>

// Le tab, ognuna con il suo tasto: una cifra o una lettera maiuscola.
// Un carattere e non un olc::Key, cosi' questo file non dipende dal motore.
struct ViewTab {
    const char* label;
    ViewMode    mode;
    char        key;
};

inline constexpr ViewTab VIEW_TABS[] = {
    {"1 Digraph",     ViewMode::DIGRAPH,    '1'},
    {"2 Dot Plot",    ViewMode::DOTPLOT,    '2'},
    {"3 Entropia",    ViewMode::ENTROPY,    '3'},
    {"4 Istogramma",  ViewMode::HISTOGRAM,  '4'},
    {"5 3D",          ViewMode::TRIGRAPH3D, '5'},
    {"6 Raw",         ViewMode::RAWPIXELS,  '6'},
    {"7 Metric Map",  ViewMode::METRICMAP,  '7'},
    {"8 Byte Cloud",  ViewMode::BYTECLOUD,  '8'},
    {"9 One Tuple",   ViewMode::ONETUPLE,   '9'},
    {"0 Stringhe",    ViewMode::STRINGS,    '0'},
    {"H Hex",         ViewMode::HEX,        'H'},
    {"P Panoramica",  ViewMode::OVERVIEW,   'P'},
};
inline constexpr int VIEW_TAB_COUNT = 12;

// Ogni tab e' larga quanto il suo testo, cosi' ci stanno piu' viste
inline constexpr int TAB_PAD = 10;     // pixel ai lati del testo

inline int tabWidth(int i) {
    return (int)std::strlen(VIEW_TABS[i].label) * 8 + TAB_PAD * 2;
}

inline int tabX(int i) {
    int x = 0;
    for (int t = 0; t < i; t++) x += tabWidth(t);
    return x;
}

// La tab alla coordinata x; -1 se nessuna
inline int tabAt(int x) {
    for (int i = 0, tx = 0; i < VIEW_TAB_COUNT; tx += tabWidth(i), i++)
        if (x >= tx && x < tx + tabWidth(i)) return i;
    return -1;
}
