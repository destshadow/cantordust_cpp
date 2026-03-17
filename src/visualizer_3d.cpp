#include "visualizer.h"

void Visualizer::handle3DInput(float fElapsedTime) {

    // R: reset tutto
    if (GetKey(olc::Key::R).bPressed) {
        m_renderer3d.resetRotation();
        m_dragging = false;
        m_panning  = false;
    }

    // --- Mouse sinistro: ROTAZIONE ---
    // Usiamo bPressed/bReleased per tracciare il drag
    // Controlliamo che il mouse sia nell'area canvas
    int mx = GetMouseX();
    int my = GetMouseY();
    bool inCanvas = (mx >= CANVAS_X && mx < CANVAS_X + CANVAS_W &&
                     my >= CANVAS_Y && my < CANVAS_Y + CANVAS_H);

    if (GetMouse(0).bPressed && inCanvas) {
        m_dragging   = true;
        m_panning    = false;  // mutex: solo uno attivo
        m_lastMouseX = mx;
        m_lastMouseY = my;
    }
    if (GetMouse(0).bReleased) m_dragging = false;

    if (m_dragging && GetMouse(0).bHeld) {
        float dx = (float)(mx - m_lastMouseX);
        float dy = (float)(my - m_lastMouseY);

        // Applica solo se il movimento e' significativo
        // Questo evita il flip quando il mouse
        // torna all'interno del canvas
        if (std::abs(dx) < 50.0f && std::abs(dy) < 50.0f) {
            m_renderer3d.rotate(dx, dy);
        }
        m_lastMouseX = mx;
        m_lastMouseY = my;
    }

    // --- Mouse destro: PAN ---
    if (GetMouse(1).bPressed && inCanvas) {
        m_panning    = true;
        m_dragging   = false;  // mutex
        m_lastMouseX = mx;
        m_lastMouseY = my;
    }
    if (GetMouse(1).bReleased) m_panning = false;

    if (m_panning && GetMouse(1).bHeld) {
        float dx = (float)(mx - m_lastMouseX);
        float dy = (float)(my - m_lastMouseY);
        if (std::abs(dx) < 50.0f && std::abs(dy) < 50.0f)
            m_renderer3d.pan(dx, dy);
        m_lastMouseX = mx;
        m_lastMouseY = my;
    }

    // --- WASD + QE: movimento camera FPS ---
    float fwd  = 0.0f, right = 0.0f, up = 0.0f;
    float speed = 30.0f;
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
