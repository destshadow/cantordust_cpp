#include "nav_bars.h"
#include "constants.h"
#include <array>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace {
// Colonne: sezioni | FILE | ZONA
constexpr int SEC_X = 4,            SEC_W = 72;
constexpr int BAR_X[2] = {80, 146};
constexpr int BAR_W = 62;
constexpr int HANDLE_H = 9;
constexpr int TRACK_Y = SIDE_Y + 16 + 11 + HANDLE_H;   // titolo + etichette + maniglia
constexpr int TRACK_H = SIDE_H - (TRACK_Y - SIDE_Y) - HANDLE_H - 40;

std::string hex8(size_t v) {
    std::ostringstream o;
    o << "0x" << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << v;
    return o.str();
}
} // namespace

NavBars::~NavBars() {
    delete m_strip[0];
    delete m_strip[1];
}

void NavBars::setFile(const std::vector<uint8_t>* bytes,
                      const std::vector<BinarySection>* sections) {
    m_bytes = bytes;
    m_secs  = sections;
    m_builtLo[0] = m_builtLo[1] = 1;   // forza la ricostruzione
    m_builtHi[0] = m_builtHi[1] = 0;
    m_drag = Drag::NONE;
    reset();
}

void NavBars::reset() {
    size_t total = m_bytes ? m_bytes->size() : 0;
    m_macro.min = 0;  m_macro.max = total;
    m_macro.set(0, total);
    m_micro.min = 0;  m_micro.max = total;
    m_micro.set(0, total);
}

void NavBars::toggleStrip() { m_entropy = !m_entropy; }

size_t NavBars::valueAtY(int bar, int my) const {
    const RangeSel& s = sel(bar);
    double t = std::clamp((double)(my - TRACK_Y) / TRACK_H, 0.0, 1.0);
    return s.min + (size_t)(t * (double)(s.max - s.min));
}

int NavBars::yOf(int bar, size_t value) const {
    const RangeSel& s = sel(bar);
    if (s.max <= s.min) return TRACK_Y;
    return TRACK_Y + (int)((double)(value - s.min) / (double)(s.max - s.min) * TRACK_H);
}

int NavBars::barAt(int mx, int my) const {
    if (my < TRACK_Y - HANDLE_H || my >= TRACK_Y + TRACK_H + HANDLE_H) return -1;
    for (int b = 0; b < 2; b++)
        if (mx >= BAR_X[b] && mx < BAR_X[b] + BAR_W) return b;
    return -1;
}

// La sezione piu' piccola sotto il mouse, cosi' anche quelle minuscole
// si possono raggiungere
int NavBars::sectionAtY(int mx, int my) const {
    if (!m_secs || mx < SEC_X || mx >= SEC_X + SEC_W) return -1;
    int best = -1;
    for (int i = 0; i < (int)m_secs->size(); i++) {
        const auto& s = (*m_secs)[i];
        int y0 = yOf(0, s.offset);
        int y1 = std::max(y0 + 2, yOf(0, s.offset + s.size));
        if (my >= y0 && my < y1 && (best < 0 || s.size < (*m_secs)[best].size))
            best = i;
    }
    return best;
}

void NavBars::selectSection(int index) {
    const auto& s = (*m_secs)[index];
    size_t a = s.offset, b = s.offset + s.size;
    if (a < m_macro.lo || b > m_macro.hi)   // fuori dalla zona: si allarga
        m_macro.set(0, m_macro.max);
    m_micro.setBounds(m_macro.lo, m_macro.hi);
    m_micro.set(a, b);
}

bool NavBars::update(olc::PixelGameEngine* pge) {
    if (!m_bytes || m_bytes->empty()) return false;

    int mx = m_mouseX = pge->GetMouseX();
    int my = m_mouseY = pge->GetMouseY();
    const size_t oldLo = m_micro.lo, oldHi = m_micro.hi;
    const auto& lmb = pge->GetMouse(0);

    m_hoverSec = (m_drag == Drag::NONE) ? sectionAtY(mx, my) : -1;

    if (lmb.bPressed) {
        if (m_hoverSec >= 0) {
            selectSection(m_hoverSec);
        } else {
            int bar = barAt(mx, my);
            if (bar >= 0) {
                RangeSel& s = sel(bar);
                int y0 = yOf(bar, s.lo), y1 = std::max(yOf(bar, s.hi), y0 + 2);
                m_dragBar = bar;
                if      (my >= y0 - HANDLE_H && my < y0)        m_drag = Drag::LO;
                else if (my >= y1 && my < y1 + HANDLE_H)        m_drag = Drag::HI;
                else if (my >= y0 && my < y1) {
                    m_drag = Drag::SLIDE;
                    m_grab = valueAtY(bar, my) - s.lo;
                } else {                                 // fuori: vai li'
                    m_drag = Drag::SLIDE;
                    m_grab = s.size() / 2;
                }
            }
        }
    }
    if (!lmb.bHeld) m_drag = Drag::NONE;

    if (m_drag != Drag::NONE) {
        RangeSel& s = sel(m_dragBar);
        size_t v = valueAtY(m_dragBar, my);
        if      (m_drag == Drag::LO) s.setLo(v);
        else if (m_drag == Drag::HI) s.setHi(v);
        else                         s.slideTo(v > m_grab ? v - m_grab : s.min);
    }

    if (int wheel = pge->GetMouseWheel()) {
        int bar = barAt(mx, my);
        if (bar >= 0) {
            // Rotella avanti = ingrandisci (selezione piu' stretta)
            sel(bar).zoomAround(valueAtY(bar, my), wheel > 0 ? 0.8 : 1.25);
        }
    }
    if (pge->GetMouse(1).bPressed && barAt(mx, my) >= 0) toggleStrip();

    // La selezione di ZONA resta sugli stessi byte, limitata alla nuova
    // zona (riproporzionarla la farebbe saltare altrove)
    m_micro.setBounds(m_macro.lo, m_macro.hi);

    return m_micro.lo != oldLo || m_micro.hi != oldHi;
}

bool NavBars::hoverOffset(int mx, int my, size_t& off) const {
    if (!m_bytes || m_bytes->empty()) return false;
    if (my < TRACK_Y || my >= TRACK_Y + TRACK_H) return false;
    int bar = barAt(mx, my);
    if (bar < 0) return false;
    // Bar 0 mostra il file, bar 1 la zona: stessa formula della striscia
    size_t lo = bar == 0 ? 0 : m_macro.lo, hi = bar == 0 ? m_bytes->size() : m_macro.hi;
    size_t px = (size_t)BAR_W * TRACK_H;
    size_t i  = (size_t)(my - TRACK_Y) * BAR_W + (mx - BAR_X[bar]);
    off = lo + (size_t)((double)i / px * (hi - lo));
    return off < m_bytes->size();
}

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
}

// Le sezioni come bande colorate, con il nome dove c'e' spazio
void NavBars::drawSections(olc::PixelGameEngine* pge) {
    pge->FillRect(SEC_X, TRACK_Y, SEC_W, TRACK_H, olc::Pixel(8, 8, 24));
    if (!m_secs || m_secs->empty()) {
        pge->DrawString(SEC_X + 2, TRACK_Y + 4, "nessuna", olc::Pixel(110, 110, 150));
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
            pge->DrawLine(SEC_X, y0, SEC_X + SEC_W - 1, y0, olc::Pixel(8, 8, 24));
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
    pge->DrawString(6, SIDE_Y + 3, "NAVIGAZIONE", COL_YELLOW());

    const olc::Pixel label(170, 170, 190);
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
        const olc::Pixel addr(200, 200, 100);
        pge->DrawString(6, ty, hex8(m_micro.lo), addr);
        pge->DrawString(6 + 11 * 8, ty, "inizio", olc::Pixel(130, 130, 160));
        pge->DrawString(6, ty + 10, hex8(m_micro.hi), addr);
        pge->DrawString(6 + 11 * 8, ty + 10, "fine", olc::Pixel(130, 130, 160));
        std::ostringstream o;
        if (m_micro.size() >= total) o << "file intero";
        else o << std::fixed << std::setprecision(1)
               << 100.0 * m_micro.size() / total << "% del file";
        pge->DrawString(6, ty + 20, o.str(), olc::Pixel(160, 160, 160));
    }
}
