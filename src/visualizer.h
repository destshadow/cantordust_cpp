#pragma once
#include "binary_reader.h"
#include "digraph.h"
#include "dotplot.h"
#include "entropy.h"
#include "histogram.h"
#include "trigraph.h"
#include "renderer3d.h"
#include "file_navigator.h"
#include "screenshot.h"
#include "input_handler.h"
#include "binary_format.h"
#include "ui_renderer.h"
#include "constants.h"

class Visualizer : public olc::PixelGameEngine {
public:
    Visualizer(const std::string& filepath = "");
    bool OnUserCreate() override;
    bool OnUserUpdate(float fElapsedTime) override;

private:
    // --- Rendering ---
    void updateCanvas();
    std::vector<uint8_t> getCurrentRGB() const;

    // --- Navigazione ---
    void loadFile(const std::string& filepath);
    void recomputeWindow();
    void drawScrollBars();

    // --- 3D ---
    void handle3DInput(float fElapsedTime);

    // --- 2D zoom/pan ---
    void handle2DInput();
    void draw2DCanvas();

    // --- Utility ---
    void drawSectionOverlay();
    std::string getByteInfo(int mouseX, int mouseY) const;

    // Visualizzazioni
    BinaryReader  m_reader;
    DiGraph       m_digraph;
    DotPlot       m_dotplot;
    Entropy       m_entropy;
    Histogram     m_histogram;
    TriGraph      m_trigraph;
    Renderer3D    m_renderer3d;
    FileNavigator  m_navigator;
    BinaryParser   m_parser;

    // Canvas
    olc::Sprite*  m_canvas = nullptr;
    olc::Decal*   m_decal  = nullptr;

    // Componenti
    InputHandler  m_input;
    UIRenderer    m_ui;

    // Stato
    ViewMode      m_mode    = ViewMode::DIGRAPH;
    bool          m_dirty   = false;
    std::string   m_status;
    std::string   m_initPath;

    // Mouse 3D
    bool          m_dragging   = false;
    bool          m_panning    = false;
    int           m_lastMouseX = 0;
    int           m_lastMouseY = 0;

    // Zoom/pan 2D
    float         m_zoom2d    = 1.0f;
    float         m_scrollH   = 0.0f;  // [0.0, 1.0]
    float         m_scrollV   = 0.0f;  // [0.0, 1.0]
};
