#include "input_handler.h"

// --- Punto di ingresso chiamato ogni frame ---
Action InputHandler::update(olc::PixelGameEngine* pge, float fElapsedTime) {

    // Aggiorna il timer del cursore lampeggiante sempre,
    // anche quando l'overlay e' chiuso (non fa danni)
    m_cursorBlink += fElapsedTime;

    // Routing: due modalita' mutualmente esclusive
    if (m_inputMode)
        return handleTextInput(pge);
    else
        return handleNormalKeys(pge);
}

// --- Tasti normali: 1-4, O, S ---
Action InputHandler::handleNormalKeys(olc::PixelGameEngine* pge) {

    // Tasti 1-4: cambio vista
    // Mappa diretta tasto → ViewMode
    if (pge->GetKey(olc::Key::K1).bPressed) {
        m_requestedView = ViewMode::DIGRAPH;
        return Action::SWITCH_VIEW;
    }
    if (pge->GetKey(olc::Key::K2).bPressed) {
        m_requestedView = ViewMode::DOTPLOT;
        return Action::SWITCH_VIEW;
    }
    if (pge->GetKey(olc::Key::K3).bPressed) {
        m_requestedView = ViewMode::ENTROPY;
        return Action::SWITCH_VIEW;
    }
    if (pge->GetKey(olc::Key::K4).bPressed) {
        m_requestedView = ViewMode::HISTOGRAM;
        return Action::SWITCH_VIEW;
    }

    // O: apre l'overlay di input
    // Precompila "/" perche' su Linux i path assoluti iniziano con /
    if (pge->GetKey(olc::Key::O).bPressed) {
        m_inputMode   = true;
        m_inputBuffer = "/";
        m_cursorBlink = 0.0f;
        return Action::NONE; // il Visualizer vede solo che l'overlay e' aperto
    }

    // S: salva PNG della vista corrente
    if (pge->GetKey(olc::Key::S).bPressed)
        return Action::SAVE_PNG;

    return Action::NONE;
}

// --- Tasti in modalita' input: accumula caratteri nel buffer ---
Action InputHandler::handleTextInput(olc::PixelGameEngine* pge) {
    bool shift = pge->GetKey(olc::Key::SHIFT).bHeld;

    for (int k = 0; k < 256; k++) {
        olc::Key key = static_cast<olc::Key>(k);
        if (!pge->GetKey(key).bPressed) continue;

        // ENTER: conferma il path e chiudi overlay
        if (key == olc::Key::ENTER) {
            m_inputMode = false;
            return Action::OPEN_FILE;
            // Il Visualizer legge il path con getRequestedPath()
        }

        // ESC: annulla senza caricare nulla
        if (key == olc::Key::ESCAPE) {
            m_inputMode   = false;
            m_inputBuffer = "";
            return Action::CANCEL;
        }

        // BACKSPACE: cancella ultimo carattere
        if (key == olc::Key::BACK) {
            if (!m_inputBuffer.empty())
                m_inputBuffer.pop_back();
            continue;
        }

        // NP_DIV (/ sul tastierino): aggiunge slash per separatori path
        if (key == olc::Key::NP_DIV) {
            m_inputBuffer += '/';
            continue;
        }

        // Tutti gli altri tasti: converti in char e aggiungi
        char c = keyToChar(pge, key);
        if (c != 0) m_inputBuffer += c;
    }

    return Action::NONE;
}

// --- Converte tasto olcPGE in char ASCII ---
// olc::Key e' enum class quindi serve cast a int per aritmetica
char InputHandler::keyToChar(olc::PixelGameEngine* pge, olc::Key key) {
    bool shift = pge->GetKey(olc::Key::SHIFT).bHeld;
    int k  = static_cast<int>(key);
    int kA = static_cast<int>(olc::Key::A);
    int kZ = static_cast<int>(olc::Key::Z);
    int k0 = static_cast<int>(olc::Key::K0);
    int k9 = static_cast<int>(olc::Key::K9);

    // Lettere a-z / A-Z
    if (k >= kA && k <= kZ) {
        char c = 'a' + (k - kA);
        return shift ? (char)(c - 32) : c;
    }

    // Numeri 0-9
    if (k >= k0 && k <= k9)
        return '0' + (k - k0);

    // Simboli utili per path
    switch (key) {
        case olc::Key::PERIOD: return '.';
        case olc::Key::MINUS:  return shift ? '_' : '-';
        case olc::Key::SPACE:  return ' ';
        default:               return 0;
    }
}
