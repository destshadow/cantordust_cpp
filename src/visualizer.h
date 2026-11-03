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
#include "autocorr.h"
#include "ui_widgets.h"
#include "byte_info.h"
#include <array>

class Visualizer : public olc::PixelGameEngine {
public:
    Visualizer(const std::string& filepath = "");
    ~Visualizer() override;
    bool OnUserCreate() override;
    bool OnUserDestroy() override;
    bool OnUserUpdate(float fElapsedTime) override;

private:
    void loadFile(const std::string& filepath);
    void updateCanvas();
    void recomputeWindow();
    void handle3DInput(float fElapsedTime);
    void handle2DInput();
    void draw2DCanvas();
    void drawSectionOverlay();
    void handleSectionClick();
    void drawSpinner(float fElapsedTime);
    // Area dello schermo riservata alla vista (in RawPixels piu' bassa:
    // sotto c'e' il pannello autocorrelazione + slider)
    void viewArea(int& x, int& y, int& w, int& h) const;
    // Dimensione in pixel del contenuto: 256x256, o WxH per i raw pixel
    void contentSize(int& w, int& h) const;
    // Posizione e scala del contenuto sullo schermo (fit + zoom + scroll)
    void canvasTransform(float& drawX, float& drawY, float& scale) const;
    // Legenda, Inspector, barra di navigazione provvisoria
    void drawLegend();
    void drawInspector();
    void drawNavPanel();
    void handleNavPanel();
    bool mouseToContent(int mx, int my, int& cx, int& cy) const;
    int  sectionAt(size_t offset) const;   // -1 se nessuna
    size_t firstOccurrence(const uint8_t* pat, size_t n);  // con cache
    // Pannello raw pixel
    void drawRawPanel();
    void handleRawInput();
    void recomputeRawPixels();
    void syncRawSliders();
    bool sectionToCanvas(size_t offset, int& cx, int& cy) const;
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

    // Canvas (256x256) e canvas raw pixel (dimensione variabile)
    olc::Sprite*  m_canvas    = nullptr;
    olc::Decal*   m_decal     = nullptr;
    olc::Sprite*  m_rawSprite = nullptr;
    olc::Decal*   m_rawDecal  = nullptr;

    // Miniatura del file per la barra laterale (ricostruita al cambio file)
    olc::Sprite*   m_navSprite = nullptr;
    const uint8_t* m_navData   = nullptr;
    size_t         m_navSize   = SIZE_MAX;

    // Istogramma della finestra corrente (calcolato nel thread di calcolo):
    // base per Inspector, riassunto e viste "per valore"
    std::array<uint32_t, 256> m_windowHist{};

    // Cache della ricerca "prima volta che compare" per l'Inspector
    std::vector<uint8_t> m_occKey;
    size_t m_occStart = SIZE_MAX, m_occEnd = SIZE_MAX, m_occResult = SIZE_MAX;

    // Raw pixel: larghezza/offset, autocorrelazione, slider
    int           m_rawWidth     = 256;
    int           m_rawOffset    = 0;
    Autocorr      m_autocorr;
    int           m_suggestWidth = 0;   // 0 = nessuna larghezza trovata
    Slider        m_widthSlider;
    Slider        m_offsetSlider;

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
    std::string   m_computeError;
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
};
