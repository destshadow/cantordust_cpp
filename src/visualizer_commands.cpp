#include "visualizer.h"

// Comandi globali: menu' "Altro", azioni chieste dall'InputHandler (vista,
// apri, salva) e tasti che valgono in ogni vista (B, firme, Home).

// Il menu' sotto la voce "Altro": le viste che non hanno una tab
void Visualizer::openMoreMenu() {
    std::vector<PopupMenu::Item> items;
    for (const ViewTab& t : VIEW_TABS)
        if (t.inMenu) items.push_back({t.label, (int)t.mode, t.mode == m_mode});
    m_menu.open(moreTabX(), TOPBAR_H, std::move(items));
}

// I comandi valgono anche durante un calcolo: la vista nuova parte appena
// finisce quello in corso
void Visualizer::handleAction(Action action) {
    switch (action) {
        case Action::SWITCH_VIEW:
            switchView(m_input.getRequestedView());
            break;
        case Action::OPEN_MENU:
            openMoreMenu();
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
    if (!m_computing) handleSignatureKeys();

    if (!m_computing && GetKey(olc::Key::HOME).bPressed) {
        m_bars.reset();
        syncNavigator();
        m_zoom2d  = 1.0f;
        m_scrollH = 0.0f;
        m_scrollV = 0.0f;
        m_renderer3d.resetCamera();
        m_dragging = m_panning = false;
        m_status  = "Reset: file intero";
    }
}
