#pragma once
#include "view_mode.h"
#include <cstring>

// Le tab, nell'ordine dei tasti 1-9, poi 0
struct ViewTab {
    const char* label;
    ViewMode    mode;
};

inline constexpr ViewTab VIEW_TABS[] = {
    {"1 Digraph",     ViewMode::DIGRAPH   },
    {"2 Dot Plot",    ViewMode::DOTPLOT   },
    {"3 Entropia",    ViewMode::ENTROPY   },
    {"4 Istogramma",  ViewMode::HISTOGRAM },
    {"5 3D",          ViewMode::TRIGRAPH3D},
    {"6 Raw",         ViewMode::RAWPIXELS },
    {"7 Metric Map",  ViewMode::METRICMAP },
    {"8 Byte Cloud",  ViewMode::BYTECLOUD },
    {"9 One Tuple",   ViewMode::ONETUPLE  },
};
inline constexpr int VIEW_TAB_COUNT = 9;

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
