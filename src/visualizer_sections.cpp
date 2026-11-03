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

// Converte l'offset (assoluto nel file) dell'inizio di una sezione in
// coordinate del canvas 256x256 della vista corrente.
// cx >= 0 → linea verticale in colonna cx; cy >= 0 → linea orizzontale.
// Ritorna false se la vista non ha un asse "offset" (digraph, istogramma,
// ...) oppure se la sezione non inizia dentro la finestra corrente.
bool Visualizer::sectionToCanvas(size_t offset, int& cx, int& cy) const {
    cx = cy = -1;
    size_t start = m_navigator.getStart();
    size_t len   = m_navigator.getEnd() - start;
    if (len == 0 || offset < start || offset >= start + len) return false;
    size_t rel = offset - start;

    switch (m_mode) {
        case ViewMode::DOTPLOT:   // X = posizione nella finestra
        case ViewMode::ENTROPY:   // X = blocchi della finestra
            cx = (int)((rel * 255) / len);
            return true;
        case ViewMode::RAWPIXELS: {  // riga dell'immagine (larghezza variabile)
            int w = m_rawpixels.getWidth();
            if (w <= 0 || rel < (size_t)m_rawOffset) return false;
            size_t px  = (rel - m_rawOffset) / RawPixels::bytesPerPixel(m_bppMode);
            size_t row = px / w / m_rawpixels.getRowStep();
            if (row >= (size_t)m_rawpixels.getHeight()) return false;
            cy = (int)row;
            return true;
        }
        default:
            return false;
    }
}

void Visualizer::drawSectionOverlay() {
    m_sectionInfos.clear();
    m_hoveredSection = -1;

    if (!m_parser.hasSections()) return;
    if (m_mode == ViewMode::TRIGRAPH3D) return;

    float drawX, drawY, scale;
    canvasTransform(drawX, drawY, scale);
    int AX, AY, AW, AH;
    viewArea(AX, AY, AW, AH);

    int mx = GetMouseX();
    int my = GetMouseY();
    bool mouseInCanvas = (mx >= AX && mx < AX + AW &&
                          my >= AY && my < AY + AH);
    const int HOVER_R = 4;

    const auto& sections = m_parser.getSections();
    for (int i = 0; i < (int)sections.size(); i++) {
        const auto& sec = sections[i];

        int cx, cy;
        if (!sectionToCanvas(sec.offset, cx, cy)) continue;

        // Centro della cella del canvas sullo schermo
        int screenX = (cx >= 0) ? (int)(drawX + (cx + 0.5f) * scale) : -1;
        int screenY = (cy >= 0) ? (int)(drawY + (cy + 0.5f) * scale) : -1;

        bool visX = screenX >= AX && screenX < AX + AW;
        bool visY = screenY >= AY && screenY < AY + AH;
        if (!visX && !visY) continue;

        olc::Pixel col(sec.r, sec.g, sec.b);
        bool hovered = mouseInCanvas && m_hoveredSection < 0 &&
                       ((visX && std::abs(mx - screenX) <= HOVER_R) ||
                        (visY && std::abs(my - screenY) <= HOVER_R));
        if (hovered) {
            m_hoveredSection = (int)m_sectionInfos.size();
            col = olc::Pixel(std::min(255, sec.r + 80),
                             std::min(255, sec.g + 80),
                             std::min(255, sec.b + 80));
        }

        if (visX) {
            DrawLine(screenX, AY, screenX, AY + AH - 1, col);
            if (hovered) {
                DrawLine(screenX-1, AY, screenX-1, AY + AH - 1, col);
                DrawLine(screenX+1, AY, screenX+1, AY + AH - 1, col);
            }
        }
        if (visY) {
            DrawLine(AX, screenY, AX + AW - 1, screenY, col);
            if (hovered) {
                DrawLine(AX, screenY-1, AX + AW - 1, screenY-1, col);
                DrawLine(AX, screenY+1, AX + AW - 1, screenY+1, col);
            }
        }

        // Info per tooltip e click (indice → sezione nel parser)
        SectionInfo info;
        info.name      = sec.name;
        info.offsetHex = toHex(sec.offset);
        info.sizeStr   = toSizeStr(sec.size);
        info.screenX   = screenX;
        info.screenY   = screenY;
        info.r         = sec.r;
        info.g         = sec.g;
        info.b         = sec.b;
        info.sectionIndex = i;
        m_sectionInfos.push_back(info);
    }
}

void Visualizer::handleSectionClick() {
    if (m_computing) return;
    if (m_hoveredSection < 0 ||
        m_hoveredSection >= (int)m_sectionInfos.size()) return;

    // Click sinistro, ma non durante un drag o un click su una tab
    if (!GetMouse(0).bPressed) return;
    if (m_dragging) return;
    if (m_input.lastClickWasTab()) return;

    int idx = m_sectionInfos[m_hoveredSection].sectionIndex;
    const auto& sec = m_parser.getSections()[idx];

    // La finestra diventa esattamente la sezione
    m_navigator.selectRange(sec.offset, sec.size);
    m_zoom2d  = 1.0f;
    m_scrollH = 0.0f;
    m_scrollV = 0.0f;
    recomputeWindow();

    std::ostringstream oss;
    oss << "Sezione: " << sec.name
        << "  @ 0x" << std::uppercase << std::hex
        << std::setw(8) << std::setfill('0') << sec.offset
        << "  (" << toSizeStr(sec.size) << ")";
    m_status = oss.str();
}
