#pragma once
#include "view_mode.h"
#include <algorithm>
#include <cstring>

// Le tab, ognuna con il suo tasto: una cifra o una lettera maiuscola.
// Un carattere e non un olc::Key, cosi' questo file non dipende dal motore.
// Le viste con `inMenu` non hanno una tab propria: stanno nel menu' "Altro",
// l'ultima voce della barra. Il loro tasto funziona lo stesso.
struct ViewTab {
    const char* label;
    ViewMode    mode;
    char        key;
    bool        inMenu;
};

inline constexpr ViewTab VIEW_TABS[] = {
    {"1 Digraph",     ViewMode::DIGRAPH,    '1', false},
    {"2 Dot Plot",    ViewMode::DOTPLOT,    '2', false},
    {"3 Entropia",    ViewMode::ENTROPY,    '3', false},
    {"4 Istogramma",  ViewMode::HISTOGRAM,  '4', false},
    {"5 3D",          ViewMode::TRIGRAPH3D, '5', false},
    {"6 Raw",         ViewMode::RAWPIXELS,  '6', false},
    {"7 Metric Map",  ViewMode::METRICMAP,  '7', false},
    {"8 Byte Cloud",  ViewMode::BYTECLOUD,  '8', true },
    {"9 One Tuple",   ViewMode::ONETUPLE,   '9', true },
    {"0 Stringhe",    ViewMode::STRINGS,    '0', false},
    {"H Hex",         ViewMode::HEX,        'H', false},
    {"P Panoramica",  ViewMode::OVERVIEW,   'P', false},
};
inline constexpr int VIEW_TAB_COUNT = 12;
static_assert(VIEW_TAB_COUNT == VIEW_MODE_COUNT, "ogni vista ha la sua tab");

// Ogni tab e' larga quanto il suo testo, cosi' ci stanno piu' viste
inline constexpr int TAB_PAD  = 10;    // pixel ai lati del testo
inline constexpr int TAB_MORE = -2;    // tabAt: la voce "Altro"
inline constexpr const char* TAB_MORE_LABEL = "Altro";

// Larghezza della tab i; 0 se la vista sta nel menu'
inline int tabWidth(int i) {
    return VIEW_TABS[i].inMenu ? 0 : (int)std::strlen(VIEW_TABS[i].label) * 8 + TAB_PAD * 2;
}

inline int tabX(int i) {
    int x = 0;
    for (int t = 0; t < i; t++) x += tabWidth(t);
    return x;
}

// La voce "Altro": dopo l'ultima tab, larga quanto il nome piu' lungo del
// menu' piu' la freccia (quando una sua vista e' attiva ne mostra il nome)
inline int moreTabX() { return tabX(VIEW_TAB_COUNT); }

inline int moreTabWidth() {
    size_t longest = std::strlen(TAB_MORE_LABEL);
    for (const ViewTab& t : VIEW_TABS)
        if (t.inMenu) longest = std::max(longest, std::strlen(t.label));
    return (int)(longest + 2) * 8 + TAB_PAD * 2;
}

// La tab alla coordinata x: il suo indice, TAB_MORE, oppure -1
inline int tabAt(int x) {
    for (int i = 0, tx = 0; i < VIEW_TAB_COUNT; tx += tabWidth(i), i++)
        if (x >= tx && x < tx + tabWidth(i)) return i;
    if (x >= moreTabX() && x < moreTabX() + moreTabWidth()) return TAB_MORE;
    return -1;
}

// La voce del menu' della vista `mode`; nullptr se ha una tab propria
inline const ViewTab* menuTabOf(ViewMode mode) {
    for (const ViewTab& t : VIEW_TABS)
        if (t.mode == mode && t.inMenu) return &t;
    return nullptr;
}
