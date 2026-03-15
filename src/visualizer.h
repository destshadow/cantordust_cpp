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
#include "ui_renderer.h"
#include "../libs/olcPixelGameEngine.h"

class Visualizer : public olc::PixelGameEngine {
public:
    Visualizer(const std::string& filepath = "");
    bool OnUserCreate() override;
    bool OnUserUpdate(float fElapsedTime) override;

private:
    void loadFile(const std::string& filepath);
    void updateCanvas();
    void recomputeWindow();
    void handle3DInput(float fElapsedTime);
    void drawScrollBar();
    std::vector<uint8_t> getCurrentRGB() const;

    BinaryReader  m_reader;
    DiGraph       m_digraph;
    DotPlot       m_dotplot;
    Entropy       m_entropy;
    Histogram     m_histogram;
    TriGraph      m_trigraph;
    Renderer3D    m_renderer3d;
    FileNavigator m_navigator;

    olc::Sprite*  m_canvas = nullptr;
    olc::Decal*   m_decal  = nullptr;

    InputHandler  m_input;
    UIRenderer    m_ui;

    ViewMode      m_mode    = ViewMode::DIGRAPH;
    bool          m_dirty   = false;
    std::string   m_status;
    std::string   m_initPath;

    bool          m_dragging   = false;
    int           m_lastMouseX = 0;
    int           m_lastMouseY = 0;
};
