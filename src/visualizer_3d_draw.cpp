#include "visualizer.h"
#include "byte_info.h"
#include <algorithm>

// Vista 3D: nuvola, cubo, segni e legenda dei colori.

CloudColor::Mode Visualizer::cloudColorMode() const {
    if (m_cloudColor == CloudColor::Mode::SECTION && m_parser.getSections().empty())
        return CloudColor::Mode::BYTE_CLASS;   // formato sconosciuto: niente sezioni
    return m_cloudColor;
}

void Visualizer::repaintCloud() {
    CloudColor::paint(m_trigraph.getPoints(), cloudColorMode(), m_parser.getSections());
    m_renderer3d.invalidate();
}

// Le sezioni che toccano la finestra, in coordinate della finestra
std::vector<TriGraph::Range> Visualizer::sectionRanges() const {
    std::vector<TriGraph::Range> out;
    const size_t ws = m_navigator.getStart(), we = m_navigator.getEnd();
    const auto& secs = m_parser.getSections();
    for (size_t i = 0; i < secs.size(); i++) {
        size_t a = std::max(secs[i].offset, ws);
        size_t b = std::min(secs[i].offset + secs[i].size, we);
        if (a < b) out.push_back({a - ws, b - ws, (int16_t)i});
    }
    return out;
}

void Visualizer::draw3D(float fElapsedTime) {
    m_renderer3d.render(this, m_trigraph.getPoints(),
                        {CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H}, fElapsedTime,
                        m_tripleMaskOn ? &m_tripleMask : nullptr, m_tripleTint);
    m_renderer3d.drawCube(this);

    // La tripla scelta col click in giallo, quella sotto il mouse in bianco
    const auto& p = m_pick.pattern;
    if (m_pick.active() && p.size() == 3)
        m_renderer3d.drawMarker(this, ((uint32_t)p[0] << 16) | ((uint32_t)p[1] << 8) | p[2],
                                7, COL_YELLOW());
    uint32_t key;
    if (!m_computing && tripleUnderMouse(key))
        m_renderer3d.drawMarker(this, key, 5, olc::WHITE);

    drawCloudLegend();
}

// In basso a sinistra nella vista: cosa vuol dire il colore
void Visualizer::drawCloudLegend() {
    const int x = CANVAS_X + 8, y = CANVAS_Y + CANVAS_H - 26;
    const olc::Pixel grey(170, 170, 170);
    const CloudColor::Mode mode = cloudColorMode();

    std::string title = std::string("COLORE [C]: ") + CloudColor::name(mode) +
                        "   punti: " + std::to_string(m_trigraph.getPointCount());
    DrawString(x, y, title, grey);

    int cx = x;
    auto swatch = [&](uint8_t r, uint8_t g, uint8_t b, const char* name) {
        FillRect(cx, y + 13, 8, 8, olc::Pixel(r, g, b));
        DrawString(cx + 11, y + 13, name, grey);
        cx += 11 + (int)std::string(name).size() * 8 + 12;
    };

    if (mode == CloudColor::Mode::SECTION) {
        DrawString(x, y + 13, "dove prevale: colori delle bande a sinistra", grey);
    } else if (mode == CloudColor::Mode::BYTE_CLASS) {
        uint8_t r, g, b;
        CloudColor::classColor(0x000000, r, g, b);  swatch(r, g, b, "zero");
        CloudColor::classColor(0x010101, r, g, b);  swatch(r, g, b, "controllo");
        CloudColor::classColor(0x414141, r, g, b);  swatch(r, g, b, "testo");
        CloudColor::classColor(0x808080, r, g, b);  swatch(r, g, b, "alti");
        CloudColor::classColor(0xFFFFFF, r, g, b);  swatch(r, g, b, "0xFF");
        CloudColor::mixedColor(r, g, b);            swatch(r, g, b, "misti");
    } else {
        DrawString(x, y + 13, "raro", grey);
        for (int i = 0; i < 128; i++) {
            uint8_t r, g, b;
            CloudColor::frequencyColor((uint8_t)(i * 2), r, g, b);
            DrawLine(x + 40 + i, y + 13, x + 40 + i, y + 20, olc::Pixel(r, g, b));
        }
        DrawString(x + 40 + 134, y + 13, "frequente", grey);
    }
}
