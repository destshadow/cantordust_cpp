#include "nav_bars.h"
#include "nav_bars_layout.h"
#include <array>
#include <cmath>
#include <iomanip>
#include <sstream>

// Disegno delle barre: strisce, maniglie, segni, sezioni.

using namespace NavLayout;

namespace {
std::string hex8(size_t v) {
    std::ostringstream o;
    o << "0x" << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << v;
    return o.str();
}
} // namespace

// La striscia: i byte in sequenza (verde = valore), oppure l'entropia di
// ogni riga. FILE mostra tutto, ZONA solo la parte selezionata in FILE.
void NavBars::buildStrip(int bar) {
    const size_t lo = bar == 0 ? 0 : m_macro.lo;
    const size_t hi = bar == 0 ? m_bytes->size() : m_macro.hi;
    if (m_strip[bar] && m_builtLo[bar] == lo && m_builtHi[bar] == hi &&
        m_builtEntropy[bar] == m_entropy) return;
    m_builtLo[bar] = lo;  m_builtHi[bar] = hi;  m_builtEntropy[bar] = m_entropy;

    if (!m_strip[bar]) m_strip[bar] = new olc::Sprite(BAR_W, TRACK_H);
    olc::Pixel* px = m_strip[bar]->GetData();
    const size_t span = hi - lo, n = (size_t)BAR_W * TRACK_H;
    const std::vector<uint8_t>& b = *m_bytes;

    if (span == 0) {
        std::fill(px, px + n, olc::BLACK);
    } else if (!m_entropy) {
        for (size_t i = 0; i < n; i++)
            px[i] = olc::Pixel(0, b[lo + (size_t)((double)i / n * span)], 0);
    } else {
        for (int y = 0; y < TRACK_H; y++) {
            size_t a = lo + (size_t)((double)y / TRACK_H * span);
            size_t e = lo + (size_t)((double)(y + 1) / TRACK_H * span);
            e = std::min(std::max(e, a + 64), hi);    // almeno 64 campioni
            a = e > 64 ? std::max(lo, std::min(a, e - 64)) : lo;
            // Al massimo 4 KB per riga: il costo non cresce col file
            size_t step = std::max<size_t>(1, (e - a) / 4096);
            std::array<uint32_t, 256> hist{};
            size_t count = 0;
            for (size_t i = a; i < e; i += step) { hist[b[i]]++; count++; }
            double bits = 0;
            for (uint32_t c : hist)
                if (c) { double p = (double)c / count; bits -= p * std::log2(p); }
            // Scala termica: nero -> rosso -> giallo -> bianco
            int v = std::clamp((int)(255.0 * bits / 8.0), 0, 255);
            olc::Pixel c(std::min(255, v * 3),
                         std::clamp(v * 3 - 255, 0, 255),
                         std::clamp(v * 3 - 510, 0, 255));
            for (int x = 0; x < BAR_W; x++) px[(size_t)y * BAR_W + x] = c;
        }
    }
}

// Segni: dove compare il valore sotto il mouse (colonna a destra della
// barra) e, se ne e' stata scelta una, la posizione corrente
void NavBars::drawMarks(olc::PixelGameEngine* pge, int bar) {
    const int x = BAR_X[bar];
    const olc::Pixel mark(255, 70, 255);
    const auto& m = m_marks[bar];
    for (int y = 0; y < (int)m.size() && y < TRACK_H; y++)
        if (m[y]) pge->DrawLine(x + BAR_W - 12, TRACK_Y + y, x + BAR_W - 1, TRACK_Y + y, mark);

    if (m_cursor == SIZE_MAX) return;
    const RangeSel& s = sel(bar);
    if (m_cursor < s.min || m_cursor >= s.max) return;
    int y = yOf(bar, m_cursor);
    pge->DrawLine(x, y, x + BAR_W - 1, y, olc::WHITE);
}

void NavBars::drawBar(olc::PixelGameEngine* pge, int bar) {
    buildStrip(bar);
    const int x = BAR_X[bar];
    pge->DrawSprite(x, TRACK_Y, m_strip[bar]);

    const RangeSel& s = sel(bar);
    int y0 = yOf(bar, s.lo), y1 = std::max(yOf(bar, s.hi), y0 + 2);

    // Fuori dalla selezione: attenuato
    pge->SetPixelMode(olc::Pixel::ALPHA);
    if (y0 > TRACK_Y)
        pge->FillRect(x, TRACK_Y, BAR_W, y0 - TRACK_Y, olc::Pixel(0, 0, 0, 170));
    if (y1 < TRACK_Y + TRACK_H)
        pge->FillRect(x, y1, BAR_W, TRACK_Y + TRACK_H - y1, olc::Pixel(0, 0, 0, 170));
    pge->SetPixelMode(olc::Pixel::NORMAL);

    // Maniglie grigie con il triangolino, come nell'originale
    const olc::Pixel grip(150, 150, 155), arrow(30, 30, 40);
    int mid = x + BAR_W / 2;
    pge->FillRect(x, y0 - HANDLE_H, BAR_W, HANDLE_H, grip);
    pge->FillRect(x, y1, BAR_W, HANDLE_H, grip);
    for (int i = 0; i < 4; i++) {
        pge->DrawLine(mid - i, y0 - HANDLE_H + 2 + i, mid + i, y0 - HANDLE_H + 2 + i, arrow);
        pge->DrawLine(mid - 3 + i, y1 + 2 + i, mid + 3 - i, y1 + 2 + i, arrow);
    }
    drawMarks(pge, bar);
}

// Le sezioni come bande colorate, con il nome dove c'e' spazio
void NavBars::drawSections(olc::PixelGameEngine* pge) {
    pge->FillRect(SEC_X, TRACK_Y, SEC_W, TRACK_H, COL_BG_DEEP());
    if (!m_secs || m_secs->empty()) {
        pge->DrawString(SEC_X + 2, TRACK_Y + 4, "nessuna", COL_DIM());
        return;
    }
    const int maxChars = (SEC_W - 4) / 8;
    int lastTextY = -100;
    for (int i = 0; i < (int)m_secs->size(); i++) {
        const auto& s = (*m_secs)[i];
        int y0 = yOf(0, s.offset);
        int y1 = std::max(y0 + 2, yOf(0, s.offset + s.size));
        pge->FillRect(SEC_X, y0, SEC_W, y1 - y0, olc::Pixel(s.r, s.g, s.b));
        // Riga scura in cima: separa due sezioni vicine di colore simile
        if (y1 - y0 >= 4)
            pge->DrawLine(SEC_X, y0, SEC_X + SEC_W - 1, y0, COL_BG_DEEP());
        if (y1 - y0 >= 11 && y0 >= lastTextY + 9) {
            pge->DrawString(SEC_X + 3, y0 + 2, s.name.substr(0, maxChars), olc::BLACK);
            lastTextY = y0;
        }
    }
    // Cornice per ultima: le bande successive non devono coprirla
    if (m_hoverSec >= 0) {
        const auto& s = (*m_secs)[m_hoverSec];
        int y0 = yOf(0, s.offset);
        int y1 = std::max(y0 + 2, yOf(0, s.offset + s.size));
        pge->DrawRect(SEC_X, y0, SEC_W - 1, y1 - y0 - 1, olc::WHITE);
    }
}

void NavBars::draw(olc::PixelGameEngine* pge) {
    pge->FillRect(0, SIDE_Y, NAV_W, SIDE_H, COL_BG());
    pge->DrawRect(0, SIDE_Y, NAV_W - 1, SIDE_H - 1, COL_BORDER());
    pge->FillRect(1, SIDE_Y + 1, NAV_W - 2, 12, COL_BG_LIGHT());
    pge->DrawString(6, SIDE_Y + 3, "NAVIGAZIONE", COL_TEXT());

    const olc::Pixel label = COL_DIM();
    int ly = SIDE_Y + 16;
    pge->DrawString(SEC_X + 2, ly, "sezioni", label);
    pge->DrawString(BAR_X[0] + 14, ly, "file", label);
    pge->DrawString(BAR_X[1] + 14, ly, "zona", label);

    if (m_bytes && !m_bytes->empty()) {
        drawSections(pge);
        drawBar(pge, 0);
        drawBar(pge, 1);

        size_t total = m_bytes->size();
        int ty = TRACK_Y + TRACK_H + HANDLE_H + 4;
        const olc::Pixel addr = COL_TEXT();
        pge->DrawString(6, ty, hex8(m_micro.lo), addr);
        pge->DrawString(6 + 11 * 8, ty, "inizio", COL_DIM());
        pge->DrawString(6, ty + 10, hex8(m_micro.hi), addr);
        pge->DrawString(6 + 11 * 8, ty + 10, "fine", COL_DIM());
        std::ostringstream o;
        if (m_micro.size() >= total) o << "file intero";
        else o << std::fixed << std::setprecision(1)
               << 100.0 * m_micro.size() / total << "% del file";
        pge->DrawString(6, ty + 20, o.str(), COL_DIM());
    }
}
