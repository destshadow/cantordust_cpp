#include "visualizer.h"
#include <filesystem>

void Visualizer::drawScrollBars() {
    // La scrollbar del file (FILE_NAV) e quella del canvas
    // sono disegnate dall'UIRenderer tramite UIState
    // Qui gestiamo solo il click sulla FILE_NAV scrollbar
}

void Visualizer::recomputeWindow() {
    auto window = m_navigator.getWindow(m_reader.getBytes());
    if (window.empty()) return;
    m_digraph.compute(window);
    m_dotplot.compute(window);
    m_entropy.compute(window);
    m_histogram.compute(window);
    m_trigraph.compute(m_reader.getBytes(),
                       m_navigator.getStart(),
                       m_navigator.getEnd());
    m_dirty = true;
}

void Visualizer::loadFile(const std::string& filepath) {
    if (!std::filesystem::exists(filepath)) {
        m_status = "Errore: file non trovato: " + filepath;
        return;
    }
    if (!m_reader.load(filepath)) {
        m_status = "Errore caricamento: " + filepath;
        return;
    }
    const auto& bytes = m_reader.getBytes();

    // windowSize=0 → finestra = file intero
    m_navigator.setData(bytes, 0);

    m_digraph.compute(bytes);
    m_dotplot.compute(bytes);
    m_entropy.compute(bytes);
    m_histogram.compute(bytes);
    m_trigraph.compute(bytes);

    // Reset zoom 2D al caricamento
    m_zoom2d  = 1.0f;
    m_scrollH = 0.0f;
    m_scrollV = 0.0f;

    m_dirty  = true;
    m_status = "Caricato: " + filepath;
}
