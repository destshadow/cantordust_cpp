#include "visualizer.h"
#include <filesystem>

Visualizer::Visualizer(const std::string& filepath)
    : m_initPath(filepath) {
    sAppName = "CantorDust++";
}

bool Visualizer::OnUserCreate() {
    m_canvas = new olc::Sprite(256, 256);
    m_decal  = new olc::Decal(m_canvas);
    m_cloudSprite = new olc::Sprite(ByteCloud::SIZE, ByteCloud::SIZE);
    m_cloudDecal  = new olc::Decal(m_cloudSprite);
    std::error_code ec;
    if (std::filesystem::is_directory("templates", ec))
        m_classifier.loadTemplates("templates");
    loadFile(m_initPath.empty() ? "/bin/ls" : m_initPath);
    return true;
}

bool Visualizer::OnUserUpdate(float fElapsedTime) {
    applyResults();
    applyPendingOption();
    bool busy = m_computing.load();

    // Nella finestra "Apri file" i tasti servono solo per scrivere
    const bool typing = m_input.isInputMode();
    handleAction(m_input.update(this, fElapsedTime));

    if (!busy && !typing) handleCommandKeys();
    if (!typing) updateBars();

    // Finestra cambiata e nessun calcolo in corso: aggiorna le viste
    if (!m_computing && (m_recomputeNeeded ||
                         m_navigator.getStart() != m_computedStart ||
                         m_navigator.getEnd()   != m_computedEnd))
        recomputeWindow();

    busy = m_computing.load();
    if (!typing && !busy) updateLinks(fElapsedTime);
    if (!typing) {
        if (m_mode == ViewMode::TRIGRAPH3D) handle3DInput(fElapsedTime);
        else if (!busy)                     handle2DInput();
    }

    drawFrame(fElapsedTime, busy);
    drawChrome(fElapsedTime, busy);
    if (!typing) drawOptions(!busy);
    return true;
}

// I comandi valgono anche durante un calcolo: la vista nuova parte appena
// finisce quello in corso
void Visualizer::handleAction(Action action) {
    switch (action) {
        case Action::SWITCH_VIEW:
            m_mode     = m_input.getRequestedView();
            m_pick.clear();     // ogni vista ha i suoi valori
            m_dragging = false;
            m_panning  = false;
            m_recomputeNeeded = true;
            if (m_mode == ViewMode::TRIGRAPH3D) buildTripleMask();
            m_status = "";
            break;
        case Action::OPEN_FILE:
            loadFile(m_input.getRequestedPath());
            break;
        case Action::SAVE_PNG:
            saveView();
            break;
        case Action::CANCEL:
            m_status = "Annullato.";
            break;
        default: break;
    }
}

void Visualizer::handleCommandKeys() {
    handleOptionKeys();

    // --- Larghezza / offset / autocorrelazione (solo RawPixels) ---
    if (!m_computing && m_mode == ViewMode::RAWPIXELS)
        handleRawInput();

    // --- Frecce: disattivate, ora si naviga con le barre a sinistra.
    // Lasciate commentate nel caso servissero di nuovo (la prima
    // pressione di RIGHT stringe la finestra al 25% del file;
    // moveForward/moveBackward sono ancora in FileNavigator).
    // if (!m_computing && GetKey(olc::Key::RIGHT).bPressed) {
    //     m_navigator.moveForward();
    //     recomputeWindow();
    //     m_status = "";
    // }
    // if (!m_computing && GetKey(olc::Key::LEFT).bPressed) {
    //     m_navigator.moveBackward();
    //     recomputeWindow();
    //     m_status = "";
    // }

    if (GetKey(olc::Key::B).bPressed) m_bars.toggleStrip();

    if (!m_computing && GetKey(olc::Key::HOME).bPressed) {
        m_bars.reset();
        syncNavigator();
        m_zoom2d  = 1.0f;
        m_scrollH = 0.0f;
        m_scrollV = 0.0f;
        m_status  = "Reset: file intero";
    }
}

// Barre di navigazione: anche durante un calcolo, cosi' il trascinamento
// resta fluido; le viste si aggiornano appena il calcolo finisce
void Visualizer::updateBars() {
    if (m_bars.update(this)) syncNavigator();

    int hs = m_bars.hoveredSection();
    size_t hlA = 0, hlB = 0;
    if (hs >= 0) {
        hlA = m_parser.getSections()[hs].offset;
        hlB = hlA + m_parser.getSections()[hs].size;
    }
    if ((hlA != m_hlStart || hlB != m_hlEnd) && !m_computing)
        setHighlight(hlA, hlB);
}

void Visualizer::drawFrame(float fElapsedTime, bool busy) {
    Clear(olc::BLACK);

    if (m_mode == ViewMode::TRIGRAPH3D) {
        draw3D(fElapsedTime);
    } else {
        // I decal stanno sopra ai pixel: la vista coprirebbe la finestra
        // "Apri file", quindi mentre si scrive non si disegna
        if (!m_input.isInputMode()) draw2DCanvas();
        if (m_mode == ViewMode::RAWPIXELS) drawRawPanel();
    }
    drawLegend();
    m_bars.draw(this);
    drawInspector();

    if (busy) drawSpinner(fElapsedTime);
}

// Le barre decidono la finestra; le viste si aggiornano nel ciclo principale
void Visualizer::syncNavigator() {
    m_navigator.selectRange(m_bars.start(), m_bars.end() - m_bars.start());
}

Visualizer::~Visualizer() {
    if (m_future.valid()) m_future.wait();
}

bool Visualizer::OnUserDestroy() {
    if (m_future.valid()) m_future.wait();
    delete m_rawDecal;
    m_rawDecal = nullptr;
    delete m_rawSprite;
    m_rawSprite = nullptr;
    delete m_cloudDecal;
    m_cloudDecal = nullptr;
    delete m_cloudSprite;
    m_cloudSprite = nullptr;
    delete m_decal;
    m_decal = nullptr;
    delete m_canvas;
    m_canvas = nullptr;
    return true;
}
