#include "visualizer.h"
#include <algorithm>
#include <array>

// Sezione -> vista: passando su una sezione la vista la mostra.
//
// Viste per posizione (RAW, Metric Map, Dot Plot, Entropy, One Tuple):
//   ogni pixel ha un offset, si attenua cio' che sta fuori dalla sezione.
// Viste per valore (Digraph, Histogram, Byte Cloud, 3D):
//   un pixel e' un valore, che puo' venire da tutto il file. Si calcolano i
//   valori presenti nella sezione: quelli restano accesi, gli altri grigi.

namespace {
void dim(olc::Pixel& p)  { p = olc::Pixel(p.r / 5, p.g / 5, p.b / 5); }
void gray(olc::Pixel& p) {
    uint8_t v = std::max({p.r, p.g, p.b}) / 4;
    p = olc::Pixel(v, v, v);
}
} // namespace

// Chiamata quando cambia la sezione sotto il mouse (0,0 = nessuna)
void Visualizer::setHighlight(size_t start, size_t end) {
    m_hlStart = start;
    m_hlEnd   = end;
    if (m_mode == ViewMode::TRIGRAPH3D) {
        buildTripleMask();
        return;
    }
    updateCanvas();
}

// Parte della sezione che cade nella finestra corrente
bool Visualizer::highlightInWindow(size_t& a, size_t& b) const {
    if (m_hlEnd <= m_hlStart) return false;
    a = std::max(m_hlStart, m_navigator.getStart());
    b = std::min({m_hlEnd, m_navigator.getEnd(), m_reader.getSize()});
    return a < b;
}

void Visualizer::applyHighlight(olc::Sprite* spr) {
    if (m_hlEnd <= m_hlStart) return;
    switch (m_mode) {
        case ViewMode::DIGRAPH:
        case ViewMode::HISTOGRAM:
        case ViewMode::BYTECLOUD:
            highlightByValue(spr);
            break;
        case ViewMode::TRIGRAPH3D:
            break;
        default:
            highlightByPosition(spr);
    }
}

void Visualizer::highlightByPosition(olc::Sprite* spr) {
    const size_t ws  = m_navigator.getStart();
    const size_t len = m_navigator.getEnd() - ws;
    const int w = spr->width, h = spr->height, rows = m_onetuple.getRows();
    olc::Pixel* px = spr->GetData();

    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            size_t a, b;
            if (m_mode == ViewMode::RAWPIXELS) {
                size_t rel = m_rawpixels.byteOffsetAt(x, y);
                if (rel == SIZE_MAX) continue;
                a = b = ws + rel;
            } else if (m_mode == ViewMode::METRICMAP) {
                size_t rel = m_metricmap.fileIndexAt(x, y);
                if (rel == SIZE_MAX) continue;
                a = b = ws + rel;
            } else if (m_mode == ViewMode::ONETUPLE) {   // riga = tratto
                if (y >= rows) continue;
                a = ws + (size_t)y * len / rows;
                b = ws + (size_t)(y + 1) * len / rows;
            } else {                                     // colonna = tratto
                a = ws + (size_t)((double)x * len / 255.0);
                b = ws + (size_t)((double)(x + 1) * len / 255.0);
            }
            if (b < m_hlStart || a >= m_hlEnd) dim(px[(size_t)y * w + x]);
        }
}

void Visualizer::highlightByValue(olc::Sprite* spr) {
    const auto& bytes = m_reader.getBytes();
    size_t a = 0, b = 0;
    bool any = highlightInWindow(a, b);
    olc::Pixel* px = spr->GetData();

    if (m_mode == ViewMode::DIGRAPH) {
        // Le coppie presenti nella sezione restano accese
        std::vector<bool> pairs(65536, false);
        for (size_t i = a; any && i + 1 < b; i++)
            pairs[((size_t)bytes[i + 1] << 8) | bytes[i]] = true;
        for (size_t i = 0; i < 65536; i++)
            if (!pairs[i]) gray(px[i]);      // pixel (x, y) = byte x, poi byte y
        return;
    }

    // Una sezione grande contiene quasi tutti i valori: conta la QUOTA di
    // ogni valore che viene da li', non il fatto che ci sia
    std::array<uint32_t, 256> inSection{};
    for (size_t i = a; any && i < b; i++) inSection[bytes[i]]++;
    auto share = [&](int v) {
        return m_windowHist[v] ? (float)inSection[v] / m_windowHist[v] : 0.0f;
    };

    if (m_mode == ViewMode::HISTOGRAM) {
        // Resta colorata la parte bassa della barra, in proporzione
        for (int x = 0; x < 256; x++) {
            int top = 0;
            while (top < 256 && px[top * 256 + x] == olc::BLACK) top++;
            int keep = top + (int)((256 - top) * (1.0f - share(x)));
            for (int y = top; y < keep; y++) gray(px[y * 256 + x]);
        }
    } else {
        // Byte Cloud: acceso se almeno meta' delle volte viene dalla sezione
        for (int y = 0; y < 256; y++)
            for (int x = 0; x < 256; x++)
                if (share((y / 16) * 16 + x / 16) < 0.5f) gray(px[y * 256 + x]);
    }
}

// 3D: le triple presenti nella sezione (2 MB di bit, una per tripla possibile)
void Visualizer::buildTripleMask() {
    m_tripleMaskOn = m_hlEnd > m_hlStart;
    if (!m_tripleMaskOn) return;

    m_tripleMask.assign((size_t)1 << 24, false);
    size_t a = 0, b = 0;
    if (highlightInWindow(a, b)) {
        const auto& bytes = m_reader.getBytes();
        for (size_t i = a; i + 2 < b; i++)
            m_tripleMask[((size_t)bytes[i] << 16) | ((size_t)bytes[i + 1] << 8) | bytes[i + 2]] = true;
    }
    int hs = m_bars.hoveredSection();
    if (hs >= 0) {
        const auto& s = m_parser.getSections()[hs];
        m_tripleTint = olc::Pixel(s.r, s.g, s.b);
    }
}
