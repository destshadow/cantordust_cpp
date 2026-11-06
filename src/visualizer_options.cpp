#include "visualizer.h"
#include "option_bar.h"

// Le opzioni delle viste, da tastiera e dalla riga in basso: le due strade
// passano dalle stesse funzioni.
//
// Un click sulla riga non si applica subito: la riga si disegna a frame gia'
// composto, e cambiare la dimensione dell'immagine distruggerebbe il decal che
// il motore ha in coda per quel frame. Si applica all'inizio del frame dopo.

bool Visualizer::hasOptions() const {
    return m_mode == ViewMode::RAWPIXELS || m_mode == ViewMode::METRICMAP;
}

void Visualizer::setBpp(BppMode m) {
    m_bppMode = m;
    syncRawSliders();      // l'offset dipende dai byte per pixel
    recomputeRawPixels();  // sulla finestra corrente
    m_suggestWidth = m_autocorr.bestWidth(RawPixels::bytesPerPixel(m_bppMode));
}

void Visualizer::setRawFilter(RawFilter f) {
    m_rawFilter = f;
    recomputeRawPixels();
}

void Visualizer::applySuggestedWidth() {
    if (m_suggestWidth <= 0) return;
    m_rawWidth = m_suggestWidth;
    syncRawSliders();      // riallinea l'offset alla nuova larghezza
    recomputeRawPixels();
}

void Visualizer::setColorMode(ColorMode m) {
    m_colorMode = m;
    recomputeWindow();
}

void Visualizer::setCurveMode(CurveMode m) {
    m_curveMode = m;
    recomputeWindow();
}

void Visualizer::applyPendingOption() {
    if (!m_pendingOption) return;
    auto action = std::move(m_pendingOption);
    m_pendingOption = nullptr;
    if (!m_computing) action();
}

void Visualizer::handleOptionKeys() {
    if (m_computing) return;
    if (m_mode == ViewMode::RAWPIXELS) {
        if (GetKey(olc::Key::OEM_4).bPressed) setBpp((BppMode)(((int)m_bppMode + 4) % 5));
        if (GetKey(olc::Key::OEM_6).bPressed) setBpp((BppMode)(((int)m_bppMode + 1) % 5));
    }
    if (m_mode == ViewMode::METRICMAP) {
        if (GetKey(olc::Key::C).bPressed)
            setColorMode(MetricMap::nextColorMode(m_colorMode, m_classifier.isReady()));
        if (GetKey(olc::Key::V).bPressed)
            setCurveMode((CurveMode)(((int)m_curveMode + 1) % 3));
    }
}

void Visualizer::drawRawOptions(bool clickable) {
    OptionBar bar(this, 6, CMD_STATUS_Y + 1, clickable);

    bar.title("Bpp [ ]:");
    const BppMode modes[] = {BppMode::BPP_8, BppMode::BPP_16, BppMode::BPP_24,
                             BppMode::BPP_32, BppMode::BPP_64};
    const char* const names[] = {"8", "16", "24", "32", "64"};
    for (int i = 0; i < 5; i++)
        if (bar.option(names[i], m_bppMode == modes[i])) {
            const BppMode m = modes[i];
            m_pendingOption = [this, m] { setBpp(m); };
        }

    bar.gap();
    bar.title("Filtro [F]:");
    for (int id = 0; id < 3; id++)
        if (bar.option(RawFilter{id}.name(), m_rawFilter.id == id))
            m_pendingOption = [this, id] { setRawFilter(RawFilter{id}); };
    bar.title("bit");
    for (int id = 3; id < RawFilter::COUNT; id++)
        if (bar.option(std::to_string(RawFilter{id}.bit()), m_rawFilter.id == id))
            m_pendingOption = [this, id] { setRawFilter(RawFilter{id}); };

    bar.gap();
    if (bar.option("[A] larghezza auto", false, m_suggestWidth > 0))
        m_pendingOption = [this] { applySuggestedWidth(); };
}

void Visualizer::drawMetricOptions(bool clickable) {
    OptionBar bar(this, 6, CMD_STATUS_Y + 1, clickable);

    bar.title("Colore [C]:");
    for (int i = 0; i < 6; i++) {
        const ColorMode m = (ColorMode)i;
        const bool usable = m != ColorMode::CLASSIFIER || m_classifier.isReady();
        if (bar.option(MetricMap::colorModeName(m), m_colorMode == m, usable))
            m_pendingOption = [this, m] { setColorMode(m); };
    }

    bar.gap();
    bar.title("Curva [V]:");
    for (int i = 0; i < 3; i++) {
        const CurveMode m = (CurveMode)i;
        if (bar.option(MetricMap::curveModeName(m), m_curveMode == m))
            m_pendingOption = [this, m] { setCurveMode(m); };
    }
}

void Visualizer::drawOptions(bool clickable) {
    if (m_mode == ViewMode::RAWPIXELS)      drawRawOptions(clickable);
    else if (m_mode == ViewMode::METRICMAP) drawMetricOptions(clickable);
}
