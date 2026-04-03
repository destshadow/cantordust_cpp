#include "overview_bar.h"
#include <array>
#include <algorithm>

void OverviewBar::compute(const std::vector<uint8_t>& allBytes) {
    m_computed = false;
    m_thumbnail.assign(BAR_W * BAR_H, olc::BLACK);
    if (allBytes.empty()) return;

    const size_t total = allBytes.size();

    for (int row = 0; row < BAR_H; row++) {
        const size_t chunkStart =
            static_cast<size_t>(static_cast<double>(row)     / BAR_H * total);
        const size_t chunkEnd   =
            std::min(static_cast<size_t>(
                static_cast<double>(row + 1) / BAR_H * total), total);
        const size_t chunkLen = chunkEnd - chunkStart;

        if (chunkLen == 0) continue;

        // Istogramma in un solo passaggio — O(chunkLen)
        std::array<int, 256> hist = {};
        for (size_t i = chunkStart; i < chunkEnd; i++)
            hist[allBytes[i]]++;

        // Accumula per colonna: ogni colonna = range di 256/BAR_W valori byte
        std::array<int, BAR_W> colCount = {};
        int maxCount = 1;
        for (int b = 0; b < 256; b++) {
            const int col = b * BAR_W / 256;
            colCount[col] += hist[b];
            if (colCount[col] > maxCount) maxCount = colCount[col];
        }

        for (int col = 0; col < BAR_W; col++) {
            const float norm = static_cast<float>(colCount[col]) / maxCount;
            const uint8_t v  = static_cast<uint8_t>(norm * 255);
            // Verde-teal: coerente con il resto dell'UI
            m_thumbnail[row * BAR_W + col] = olc::Pixel(0, v, v / 2);
        }
    }
    m_computed = true;
}

void OverviewBar::draw(olc::PixelGameEngine* pge,
                       int px, int py, int gap,
                       float selNorm, float selRatio) const {
    if (!m_computed) return;

    const int bx = px + BAR_W + gap; // origine X pannello B

    // Disegna entrambi i pannelli dal thumbnail condiviso
    for (int row = 0; row < BAR_H; row++) {
        for (int col = 0; col < BAR_W; col++) {
            const olc::Pixel p = m_thumbnail[row * BAR_W + col];
            pge->Draw(px + col, py + row, p);  // A
            pge->Draw(bx + col, py + row, p);  // B
        }
    }

    // Bordi pannelli
    const olc::Pixel BORDER = olc::Pixel(90, 90, 90);
    pge->DrawRect(px - 1, py - 1, BAR_W + 1, BAR_H + 1, BORDER);
    pge->DrawRect(bx - 1, py - 1, BAR_W + 1, BAR_H + 1, BORDER);

    // Finestra di selezione sul pannello B
    // Quando selRatio == 1 l'intero file è visibile: nessuna finestra da disegnare
    if (selRatio >= 1.0f) return;

    const int selH = std::max(4, static_cast<int>(selRatio * BAR_H));
    const int selY = std::clamp(
        py + static_cast<int>(selNorm * (BAR_H - selH)),
        py,
        py + BAR_H - selH);

    // Schiarisci l'area selezionata schiarendo pixel per pixel
    for (int row = selY; row < selY + selH; row++) {
        for (int col = bx; col < bx + BAR_W; col++) {
            const olc::Pixel orig = m_thumbnail[(row - py) * BAR_W + (col - bx)];
            pge->Draw(col, row, olc::Pixel(
                std::min(255, orig.r + 55),
                std::min(255, orig.g + 55),
                std::min(255, orig.b + 55)));
        }
    }
    // Bordo bianco della selezione
    pge->DrawRect(bx, selY, BAR_W - 1, selH - 1, olc::WHITE);
}

bool OverviewBar::hitTestPanelB(int mx, int my,
                                 int px, int py, int gap) const {
    const int bx = px + BAR_W + gap;
    return mx >= bx && mx < bx + BAR_W &&
           my >= py && my < py + BAR_H;
}

float OverviewBar::mouseYToNorm(int my, int py) const {
    return std::clamp(
        static_cast<float>(my - py) / static_cast<float>(BAR_H),
        0.0f, 1.0f);
}