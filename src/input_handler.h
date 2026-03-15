#pragma once
#include <string>
#include "../libs/olcPixelGameEngine.h"

// Azioni che InputHandler comunica al Visualizer
// Il Visualizer non sa COME sono stati premuti i tasti,
// sa solo COSA deve fare questo frame
enum class Action {
    NONE,           // nessun input rilevante
    SWITCH_VIEW,    // l'utente ha premuto 1-4
    OPEN_FILE,      // l'utente ha confermato un path con ENTER
    SAVE_PNG,       // l'utente ha premuto S
    CANCEL          // l'utente ha premuto ESC nell'overlay
};

// ViewMode qui perche' InputHandler deve sapere
// quale tasto corrisponde a quale vista
enum class ViewMode {
    DIGRAPH   = 0,
    DOTPLOT   = 1,
    ENTROPY   = 2,
    HISTOGRAM = 3
};

class InputHandler {
public:
    // Chiamato ogni frame dal Visualizer
    // pge = puntatore all'engine per leggere i tasti
    // fElapsedTime = tempo frame per il cursore lampeggiante
    // Ritorna l'azione da eseguire questo frame
    Action update(olc::PixelGameEngine* pge, float fElapsedTime);

    // --- Getters: il Visualizer legge questi dopo update() ---

    // Vista richiesta (valida solo se Action == SWITCH_VIEW)
    ViewMode  getRequestedView()  const { return m_requestedView; }

    // Path digitato (valido solo se Action == OPEN_FILE)
    std::string getRequestedPath() const { return m_inputBuffer; }

    // Stato corrente dell'overlay
    bool        isInputMode()      const { return m_inputMode; }
    std::string getInputBuffer()   const { return m_inputBuffer; }
    float       getCursorBlink()   const { return m_cursorBlink; }

private:
    // Gestisce i tasti normali (1-4, O, S)
    Action handleNormalKeys(olc::PixelGameEngine* pge);

    // Gestisce i tasti quando l'overlay e' aperto
    Action handleTextInput(olc::PixelGameEngine* pge);

    // Converte un tasto olcPGE in carattere ASCII
    char keyToChar(olc::PixelGameEngine* pge, olc::Key key);

    bool        m_inputMode    = false;
    std::string m_inputBuffer  = "";
    float       m_cursorBlink  = 0.0f;
    ViewMode    m_requestedView = ViewMode::DIGRAPH;
};
