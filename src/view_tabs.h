#pragma once
#include "view_mode.h"

// Le tab, nell'ordine dei tasti 1-9
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
