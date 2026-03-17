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
#include "binary_format.h"
#include "raw_pixels.h"
#include "constants.h"
#include <future>
#include <mutex>
#include <atomic>

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
    void handle2DInput();
    void draw2DCanvas();
    void drawScrollBars();
    void drawSectionOverlay();
    void drawSpinner(float fElapsedTime);
    std::vector<uint8_t> getCurrentRGB() const;
    std::string getByteInfo(int mouseX, int mouseY) const;

    BinaryReader  m_reader;
    DiGraph       m_digraph;
    DotPlot       m_dotplot;
    Entropy       m_entropy;
    Histogram     m_histogram;
    TriGraph      m_trigraph;
    Renderer3D    m_renderer3d;
    FileNavigator m_navigator;
    BinaryParser  m_parser;
    RawPixels     m_rawpixels;

    olc::Sprite*  m_canvas = nullptr;
    olc::Decal*   m_decal  = nullptr;

    InputHandler  m_input;
    UIRenderer    m_ui;

    ViewMode      m_mode    = ViewMode::DIGRAPH;
    BppMode       m_bppMode = BppMode::BPP_8;
    bool          m_dirty   = false;
    std::string   m_status;
    std::string   m_initPath;

    std::future<void>  m_future;
    std::atomic<bool>  m_computing{false};
    std::mutex         m_dataMutex;
    float              m_spinnerAngle = 0.0f;

    bool  m_dragging   = false;
    bool  m_panning    = false;
    int   m_lastMouseX = 0;
    int   m_lastMouseY = 0;

    float m_zoom2d  = 1.0f;
    float m_scrollH = 0.0f;
    float m_scrollV = 0.0f;
};
