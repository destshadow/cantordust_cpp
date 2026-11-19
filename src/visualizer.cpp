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
    // I campioni del Classifier si leggono in un thread: circa mezzo secondo
    // che non deve ritardare l'apertura. Finche' non e' pronto il colore
    // Classifier resta grigio; poi il modello si legge soltanto.
    std::error_code ec;
    if (std::filesystem::is_directory("templates", ec))
        m_classifierLoad = std::async(std::launch::async,
                                      [this] { return m_classifier.loadTemplates("templates"); });
    loadFile(m_initPath.empty() ? "/bin/ls" : m_initPath);
    return true;
}

bool Visualizer::OnUserUpdate(float fElapsedTime) {
    m_dt = fElapsedTime;
    applyResults();
    applyPendingOption();
    bool busy = m_computing.load();

    // Un file trascinato sulla finestra. Il motore lo riceve solo su Windows:
    // su Linux l'elenco resta vuoto.
    if (!GetDroppedFiles().empty()) loadFile(GetDroppedFiles().front());

    // Un menu' aperto prende tutto: mouse e tasti non arrivano a nient'altro
    // in questo frame, nemmeno il click che lo chiude
    const bool menu = m_menu.isOpen();
    if (menu) {
        const int chosen = m_menu.update(this);
        if (chosen != PopupMenu::NONE) switchView((ViewMode)chosen);
    }

    // Nella finestra "Apri file" i tasti servono solo per scrivere
    const bool typing = m_input.isInputMode() || menu;
    if (!menu) handleAction(m_input.update(this, fElapsedTime));

    if (!busy && !typing) handleCommandKeys();
    if (!typing) updateBars();

    // Vista o opzione cambiata durante un calcolo: quello in corso non serve
    // piu'. (Se cambia solo la finestra lo si lascia finire: trascinando le
    // barre la vista si aggiorna a ogni calcolo, invece di restare ferma.)
    if (m_computing && m_recomputeNeeded) m_cancel = true;

    // Finestra cambiata e nessun calcolo in corso: aggiorna le viste
    if (!m_computing && (m_recomputeNeeded ||
                         m_navigator.getStart() != m_computedStart ||
                         m_navigator.getEnd()   != m_computedEnd))
        recomputeWindow();

    busy = m_computing.load();
    if (!typing && !busy) { updateLinks(fElapsedTime); updateSimilarityClick(); }
    if (!typing) {
        const ViewDef& v = view();
        if (v.input && (!busy || v.inputWhileBusy)) v.input(*this);
        if (v.canvas && !busy) handle2DInput();
    }

    drawFrame(fElapsedTime, busy);
    drawChrome(fElapsedTime, busy);
    if (!m_input.isInputMode()) drawOptions(!busy && !menu);
    m_menu.draw(this);        // per ultimo: sta sopra a tutto
    return true;
}

void Visualizer::switchView(ViewMode mode) {
    m_mode = mode;
    // L'occorrenza scelta resta come cursore nella vista nuova
    if (m_pick.active()) m_cursorOffset = m_pick.target;
    m_hexStart = m_hexEnd = SIZE_MAX;   // rientrando, il cursore torna in vista
    m_pick.clear();     // ogni vista ha i suoi valori
    m_dragging = false;
    m_panning  = false;
    m_scrollDrag = ScrollDrag::NONE;
    m_recomputeNeeded = true;
    if (view().enter) view().enter(*this);
    m_status = "";
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

    const ViewDef& v = view();
    if (v.canvas) {
        // I decal stanno sopra ai pixel: la vista coprirebbe la finestra
        // "Apri file", quindi mentre si scrive non si disegna
        if (!m_input.isInputMode()) { draw2DCanvas(); drawCursorMarker(); }
    } else {
        v.draw(*this);
    }
    if (v.panel) v.panel(*this);
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
    cancelCompute();
}

bool Visualizer::OnUserDestroy() {
    cancelCompute();
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
