#include "visualizer.h"
#include <sstream>
#include <iomanip>
#include <cmath>

static std::string toHex(size_t val) {
    std::ostringstream oss;
    oss << std::uppercase << std::hex
        << std::setw(8) << std::setfill('0') << val;
    return oss.str();
}

static std::string toSizeStr(size_t sz) {
    if (sz >= 1024*1024) return std::to_string(sz/(1024*1024)) + " MB";
    if (sz >= 1024)      return std::to_string(sz/1024) + " KB";
    return std::to_string(sz) + " B";
}

// Calcola la posizione schermo di un offset normalizzato
// tenendo conto di zoom e scroll — stessa logica di draw2DCanvas
static int offsetToScreenX(float norm, float drawX, float scale) {
    return (int)(drawX + norm * 256.0f * scale);
}
static int offsetToScreenY(float norm, float drawY, float scale) {
    return (int)(drawY + norm * 256.0f * scale);
}

void Visualizer::drawSectionOverlay() {
    m_sectionInfos.clear();
    m_hoveredSection = -1;
    if (!m_parser.hasSections()) return;
    if (m_mode == ViewMode::TRIGRAPH3D) return;

    size_t total = m_navigator.getWindowSize();
    if (total == 0) return;

    m_sectionInfos.clear();
    m_hoveredSection = -1;

    // Calcola la trasformazione corrente del canvas
    // (identica a draw2DCanvas — deve essere coerente)
    float scaledSize = 256.0f * m_zoom2d;
    float maxOffX    = std::max(0.0f, scaledSize - (float)CANVAS_W);
    float maxOffY    = std::max(0.0f, scaledSize - (float)CANVAS_H);
    float drawX      = (scaledSize <= (float)CANVAS_W)
                       ? CANVAS_X + ((float)CANVAS_W - scaledSize) * 0.5f
                       : (float)CANVAS_X - m_scrollH * maxOffX;
    float drawY      = (scaledSize <= (float)CANVAS_H)
                       ? CANVAS_Y + ((float)CANVAS_H - scaledSize) * 0.5f
                       : (float)CANVAS_Y - m_scrollV * maxOffY;
    float scale      = m_zoom2d;

    int mx = GetMouseX();
    int my = GetMouseY();

    const int HOVER_R = 6;

    for (int i = 0; i < (int)m_parser.getSections().size(); i++) {
        const auto& sec = m_parser.getSections()[i];

        float norm = float(double(sec.offset) - double(m_navigator.getStart())) / float(total);

        // Posizione sullo schermo CON zoom/scroll
        int screenX = offsetToScreenX(norm, drawX, scale);
        int screenY = offsetToScreenY(norm, drawY, scale);

        olc::Pixel col(sec.r, sec.g, sec.b);

        // Disegna solo la parte visibile nell'area canvas
        if (screenX >= CANVAS_X && screenX < CANVAS_X + CANVAS_W)
            DrawLine(screenX, CANVAS_Y,
                     screenX, CANVAS_Y + CANVAS_H - 1, col);

        if (screenY >= CANVAS_Y && screenY < CANVAS_Y + CANVAS_H)
            DrawLine(CANVAS_X, screenY,
                     CANVAS_X + CANVAS_W - 1, screenY, col);

        // Salva info per tooltip e click
        SectionInfo info;
        info.name      = sec.name;
        info.offsetHex = toHex(sec.offset);
        info.sizeStr   = toSizeStr(sec.size);
        info.screenX   = screenX;
        info.screenY   = screenY;
        info.r         = sec.r;
        info.g         = sec.g;
        info.b         = sec.b;
        m_sectionInfos.push_back(info);

        // Hover: mouse vicino alla linea verticale o orizzontale
        bool nearV = (std::abs(mx - screenX) <= HOVER_R &&
                      my >= CANVAS_Y && my < CANVAS_Y + CANVAS_H);
        bool nearH = (std::abs(my - screenY) <= HOVER_R &&
                      mx >= CANVAS_X && mx < CANVAS_X + CANVAS_W);

        if (nearV || nearH) {
            m_hoveredSection = i;

            // Evidenzia con linee aggiuntive
            if (nearV && screenX >= CANVAS_X &&
                screenX < CANVAS_X + CANVAS_W) {
                olc::Pixel bright(
                    std::min(255, (int)sec.r + 80),
                    std::min(255, (int)sec.g + 80),
                    std::min(255, (int)sec.b + 80));
                DrawLine(screenX-1, CANVAS_Y,
                         screenX-1, CANVAS_Y + CANVAS_H - 1, bright);
                DrawLine(screenX+1, CANVAS_Y,
                         screenX+1, CANVAS_Y + CANVAS_H - 1, bright);
            }
        }
    }
}

void Visualizer::handleSectionClick() {
    if (m_computing || !m_parser.hasSections()) return;
    if (m_mode == ViewMode::TRIGRAPH3D) return;
    if (m_hoveredSection < 0 || size_t(m_hoveredSection) >= m_parser.getSections().size()) return;

    // Rileva click sinistro — ma solo se NON stiamo facendo drag
    if (!GetMouse(0).bPressed) return;
    if (m_dragging) return;
    if (m_input.lastClickWasTab()) return;

    const auto& sec = m_parser.getSections()[m_hoveredSection];
    size_t total    = m_reader.getSize();
    if (total == 0) return;

    m_navigator.selectRange(sec.offset, sec.size);
    recomputeWindow();

    std::ostringstream oss;
    oss << "Sezione: " << sec.name
        << "  @ 0x" << std::uppercase << std::hex
        << std::setw(8) << std::setfill('0') << sec.offset
        << "  (" << toSizeStr(sec.size) << ")";
    m_status = oss.str();
}
