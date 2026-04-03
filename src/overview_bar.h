#pragma once
#include "olcPixelGameEngine.h"
#include <vector>
#include <cstdint>

// Due pannelli verticali affiancati:
//   Pannello A — overview statico, sola lettura
//   Pannello B — stesso overview + finestra di selezione trascinabile
class OverviewBar {
public:
    static constexpr int BAR_W = 40;
    static constexpr int BAR_H = 256;

    // Calcola il thumbnail dell'intero file.
    // Complessità O(fileSize + BAR_H*256) — sicuro nel thread async.
    void compute(const std::vector<uint8_t>& allBytes);

    // Rendering. px/py = top-left del pannello A; gap = spazio tra A e B.
    // selNorm  = navigator.getNormalized()
    // selRatio = navigator.getWindowRatio()
    void draw(olc::PixelGameEngine* pge,
              int px, int py, int gap,
              float selNorm,
              float selRatio) const;

    // true se il mouse è sul pannello B
    bool hitTestPanelB(int mx, int my,
                       int px, int py, int gap) const;

    // Converte Y mouse in valore normalizzato [0,1] per jumpTo()
    float mouseYToNorm(int my, int py) const;

    bool isDragging() const { return m_dragging; }
    void startDrag()        { m_dragging = true;  }
    void stopDrag()         { m_dragging = false; }
    bool isComputed() const { return m_computed;  }

private:
    std::vector<olc::Pixel> m_thumbnail; // BAR_W * BAR_H
    bool m_dragging = false;
    bool m_computed = false;
};