#include "visualizer.h"
#include <iomanip>
#include <sstream>

// Cio' che sta attorno alla vista: spinner, tab e barre di stato.

// Con i decal: i pixel normali finirebbero sotto la vista
void Visualizer::drawSpinner(float fElapsedTime) {
    m_spinnerAngle += fElapsedTime * 3.0f;
    float bx = CANVAS_X + CANVAS_W / 2.0f - 80, by = WIN_H / 2.0f - 20;
    FillRectDecal({bx, by}, {160, 40}, olc::Pixel(22, 23, 29, 230));
    int dots = (int)(m_spinnerAngle * 2) % 4;
    std::string txt = "Calcolo";
    for (int i = 0; i < dots; i++) txt += ".";
    DrawStringDecal({bx + 10, by + 14}, txt, COL_ACCENT());
}

void Visualizer::drawChrome(float fElapsedTime, bool busy) {
    UIState state;
    state.currentView    = m_mode;
    // Dove ci sono le opzioni, i messaggi vanno nella riga del file
    const std::string status = busy ? "Calcolo in corso..." : m_status;
    (hasOptions() ? state.notice : state.cmdStatus) = status;
    state.filepath       = m_reader.getFilepath();
    state.filesize       = m_reader.getSize();
    state.byteInfo       = busy ? "" : getByteInfo(GetMouseX(), GetMouseY());
    state.fps            = fElapsedTime > 0.0f ? 1.0f / fElapsedTime : 0.0f;
    state.zoomLevel      = hasCanvas() ? m_zoom2d : 1.0f;
    state.scrollH        = m_scrollH;
    state.scrollV        = m_scrollV;
    state.mouseX         = GetMouseX();
    state.mouseY         = GetMouseY();
    state.inputMode      = m_input.isInputMode();
    state.inputBuffer    = m_input.getInputBuffer();
    state.cursorBlink    = m_input.getCursorBlink();

    m_ui.draw(this, state);
}

std::string Visualizer::getByteInfo(int mouseX, int mouseY) const {
    int cx, cy;
    if (!m_reader.isLoaded() || !mouseToContent(mouseX, mouseY, cx, cy)) return "";

    std::ostringstream oss;
    if (m_mode == ViewMode::DIGRAPH) {
        oss << "COPPIA: 0x" << std::uppercase << std::hex
            << std::setw(2) << std::setfill('0') << cx
            << " -> 0x" << std::setw(2) << std::setfill('0') << cy;
    } else if (m_mode == ViewMode::METRICMAP) {
        // fileIndexAt e' relativo alla finestra: aggiungiamo l'inizio
        size_t rel = m_metricmap.fileIndexAt(cx, cy);
        if (rel == SIZE_MAX) return "";
        size_t fidx = m_navigator.getStart() + rel;
        oss << "OFFSET: 0x" << std::uppercase << std::hex
            << std::setw(8) << std::setfill('0') << fidx;
    } else if (m_mode == ViewMode::RAWPIXELS) {
        size_t rel = m_rawpixels.byteOffsetAt(cx, cy);
        if (rel == SIZE_MAX) return "";
        oss << "OFFSET: 0x" << std::uppercase << std::hex
            << std::setw(8) << std::setfill('0')
            << (m_navigator.getStart() + rel)
            << std::dec << "  (" << cx << "," << cy << ")";
    } else {
        oss << "POS: (" << std::dec << cx << "," << cy << ")";
    }
    return oss.str();
}
