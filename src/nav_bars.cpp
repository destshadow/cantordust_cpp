#include "nav_bars.h"
#include "nav_bars_layout.h"

// Logica delle barre: mouse e selezioni. Il disegno e' in nav_bars_draw.cpp.

using namespace NavLayout;

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
    m_hoverSec = -1;     // l'indice valeva per le sezioni del file precedente
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
    m_hoverSec = -1;
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

int NavBars::rows() { return TRACK_H; }

void NavBars::setMarks(std::vector<bool> fileRows, std::vector<bool> zoneRows) {
    m_marks[0] = std::move(fileRows);
    m_marks[1] = std::move(zoneRows);
}

void NavBars::clearMarks() {
    m_marks[0].clear();
    m_marks[1].clear();
}
