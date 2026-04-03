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
#include "metric_map.h"
#include "byte_cloud.h"
#include "one_tuple.h"
#include "classifier_model.h"
#include "constants.h"
#include <future>
#include <mutex>
#include <atomic>
#include "overview_bar.h"

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
    void handleSectionClick();
    void drawSpinner(float fElapsedTime);
    std::vector<uint8_t> getCurrentRGB() const;
    std::string getByteInfo(int mouseX, int mouseY) const;

    // Visualizzazioni
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
    MetricMap        m_metricmap;
    ByteCloud        m_bytecloud;
    OneTuple         m_onetuple;
    ClassifierModel  m_classifier;

    // Canvas
    olc::Sprite*  m_canvas = nullptr;
    olc::Decal*   m_decal  = nullptr;

    // Componenti
    InputHandler  m_input;
    UIRenderer    m_ui;

    // Stato
    ViewMode      m_mode      = ViewMode::DIGRAPH;
    BppMode       m_bppMode   = BppMode::BPP_8;
    ColorMode     m_colorMode = ColorMode::WAVELENGTH;
    CurveMode     m_curveMode = CurveMode::HILBERT;
    bool          m_dirty     = false;
    std::string   m_status;
    std::string   m_initPath;

    // Threading
    std::future<void>  m_future;
    std::atomic<bool>  m_computing{false};
    std::mutex         m_dataMutex;
    float              m_spinnerAngle = 0.0f;

    // Mouse
    bool  m_dragging   = false;
    bool  m_panning    = false;
    int   m_lastMouseX = 0;
    int   m_lastMouseY = 0;

    // Zoom 2D
    float m_zoom2d  = 1.0f;
    float m_scrollH = 0.0f;
    float m_scrollV = 0.0f;

    // Sezioni interattive
    std::vector<SectionInfo> m_sectionInfos;
    int m_hoveredSection = -1;

    // Overview bar
    OverviewBar m_overviewBar;
    bool        m_overviewDragging = false;
};
