#pragma once
#include <algorithm>

// Ritaglia un segmento sul rettangolo [xmin,xmax]x[ymin,ymax]
// (Liang-Barsky). False se il segmento e' tutto fuori.
inline bool clipLine(float& x0, float& y0, float& x1, float& y1,
                     float xmin, float ymin, float xmax, float ymax) {
    float t0 = 0.0f, t1 = 1.0f;
    const float dx = x1 - x0, dy = y1 - y0;
    const float p[4] = {-dx, dx, -dy, dy};
    const float q[4] = {x0 - xmin, xmax - x0, y0 - ymin, ymax - y0};
    for (int i = 0; i < 4; i++) {
        if (p[i] == 0.0f) {
            if (q[i] < 0.0f) return false;   // parallelo e fuori
            continue;
        }
        float t = q[i] / p[i];
        if (p[i] < 0.0f) t0 = std::max(t0, t);
        else             t1 = std::min(t1, t);
        if (t0 > t1) return false;
    }
    const float ax = x0, ay = y0;
    x0 = ax + t0 * dx;  y0 = ay + t0 * dy;
    x1 = ax + t1 * dx;  y1 = ay + t1 * dy;
    return true;
}
