#pragma once
#include "binary_reader.h"
#include "digraph.h"
#include "dotplot.h"
#include "entropy.h"
#include "histogram.h"
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
    std::vector<uint8_t> getCurrentRGB() const;

    // --- Visualizzazioni ---
    BinaryReader m_reader;
    DiGraph      m_digraph;
    DotPlot      m_dotplot;
    Entropy      m_entropy;
    Histogram    m_histogram;

    // --- Canvas olcPGE ---
    olc::Sprite* m_canvas = nullptr;
    olc::Decal*  m_decal  = nullptr;

    // --- Componenti delegati ---
    InputHandler m_input;
    UIRenderer   m_ui;

    // --- Stato minimo ---
    ViewMode    m_mode    = ViewMode::DIGRAPH;
    bool        m_dirty   = false;
    std::string m_status;
    std::string m_initPath;
};
