#include "visualizer.h"
#include <filesystem>

Visualizer::Visualizer(const std::string& filepath)
    : m_initPath(filepath) {
    sAppName = "CantorDust++";
}

bool Visualizer::OnUserCreate() {
    m_canvas = new olc::Sprite(256, 256);
    m_decal  = new olc::Decal(m_canvas);
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
    bool typing = m_input.isInputMode();
    Action action = m_input.update(this, fElapsedTime);

    // I comandi valgono anche durante un calcolo: la vista nuova parte
    // appena finisce quello in corso
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

    if (!busy) {
        if (!typing) {
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
    }

    // Barre di navigazione: anche durante un calcolo, cosi' il trascinamento
    // resta fluido; le viste si aggiornano appena il calcolo finisce
    if (!typing) {
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

    // Finestra cambiata e nessun calcolo in corso: aggiorna le viste
    if (!m_computing && (m_recomputeNeeded ||
                         m_navigator.getStart() != m_computedStart ||
                         m_navigator.getEnd()   != m_computedEnd))
        recomputeWindow();

    busy = m_computing.load();
    if (!typing && !busy) updateLinks(fElapsedTime);

    // Input modalita'
    if (!typing) {
        if (m_mode == ViewMode::TRIGRAPH3D)
            handle3DInput(fElapsedTime);
        else if (!busy)
            handle2DInput();
    }

    // Rendering
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

    UIState state;
    state.currentView    = m_mode;
    // Dove ci sono le opzioni, i messaggi vanno nella riga del file
    const std::string status = busy ? "Calcolo in corso..." : m_status;
    (hasOptions() ? state.notice : state.cmdStatus) = status;
    state.filepath       = m_reader.getFilepath();
    state.filesize       = m_reader.getSize();
    state.byteInfo       = busy ? "" : getByteInfo(GetMouseX(), GetMouseY());
    state.fps            = fElapsedTime > 0.0f ? 1.0f / fElapsedTime : 0.0f;
    state.zoomLevel      = m_zoom2d;
    state.scrollH        = m_scrollH;
    state.scrollV        = m_scrollV;
    state.mouseX         = GetMouseX();
    state.mouseY         = GetMouseY();
    state.inputMode      = m_input.isInputMode();
    state.inputBuffer    = m_input.getInputBuffer();
    state.cursorBlink    = m_input.getCursorBlink();

    m_ui.draw(this, state);
    if (!typing) drawOptions(!busy);
    return true;
}

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

// Salva la vista corrente come PNG, alla sua dimensione reale
void Visualizer::saveView() {
    if (m_mode == ViewMode::TRIGRAPH3D) return;
    if (m_computing) { m_status = "Calcolo in corso: riprova tra un attimo"; return; }
    int w = 256, h = 256;
    contentSize(w, h);
    std::vector<uint8_t> rgb = getCurrentRGB();
    if (rgb.size() < (size_t)w * h * 3) {   // es. file vuoto
        m_status = "Niente da salvare in questa vista";
        return;
    }
    std::string fname = "export_" + std::to_string((int)m_mode) + ".png";
    Screenshot::save(fname, rgb, w, h);
    m_status = "Salvato: " + fname;
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
    delete m_decal;
    m_decal = nullptr;
    delete m_canvas;
    m_canvas = nullptr;
    return true;
}
