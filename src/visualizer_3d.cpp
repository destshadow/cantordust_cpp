#include "visualizer.h"

void Visualizer::handle3DInput(float fElapsedTime) {

    // R: reset tutto (rotazione + camera + pan)
    if (GetKey(olc::Key::R).bPressed)
        m_renderer3d.resetRotation();

    // --- Mouse sinistro: ROTAZIONE del mondo ---
    if (GetMouse(0).bPressed) {
        m_dragging   = true;
        m_lastMouseX = GetMouseX();
        m_lastMouseY = GetMouseY();
    }
    if (GetMouse(0).bReleased) m_dragging = false;

    if (m_dragging) {
        float dx = (float)(GetMouseX() - m_lastMouseX);
        float dy = (float)(GetMouseY() - m_lastMouseY);
        m_renderer3d.rotate(dx, dy);
        m_lastMouseX = GetMouseX();
        m_lastMouseY = GetMouseY();
    }

    // --- Mouse destro: PAN della proiezione ---
    if (GetMouse(1).bPressed) {
        m_panning    = true;
        m_lastMouseX = GetMouseX();
        m_lastMouseY = GetMouseY();
    }
    if (GetMouse(1).bReleased) m_panning = false;

    if (m_panning) {
        float dx = (float)(GetMouseX() - m_lastMouseX);
        float dy = (float)(GetMouseY() - m_lastMouseY);
        m_renderer3d.pan(dx, dy);
        m_lastMouseX = GetMouseX();
        m_lastMouseY = GetMouseY();
    }

    // --- WASD + QE: movimento camera FPS ---
    // W/S = avanti/indietro (asse Z del mondo ruotato)
    // A/D = sinistra/destra (asse X)
    // Q/E = su/giu (asse Y)
    // La velocita' e' moltiplicata per fElapsedTime
    // per essere frame-rate independent
    float fwd   = 0.0f, right = 0.0f, up = 0.0f;
    float speed = 30.0f; // unita' al secondo

    if (GetKey(olc::Key::W).bHeld) fwd   += speed * fElapsedTime;
    if (GetKey(olc::Key::S).bHeld) fwd   -= speed * fElapsedTime;
    if (GetKey(olc::Key::A).bHeld) right -= speed * fElapsedTime;
    if (GetKey(olc::Key::D).bHeld) right += speed * fElapsedTime;
    if (GetKey(olc::Key::Q).bHeld) up    += speed * fElapsedTime;
    if (GetKey(olc::Key::E).bHeld) up    -= speed * fElapsedTime;

    if (fwd != 0.0f || right != 0.0f || up != 0.0f)
        m_renderer3d.moveCamera(fwd, right, up);

    // --- Rotella: zoom ---
    if (GetMouseWheel() > 0) m_renderer3d.zoom(+1.0f);
    if (GetMouseWheel() < 0) m_renderer3d.zoom(-1.0f);
}
