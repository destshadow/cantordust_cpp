#pragma once
#include "binary_reader.h"
#include "digraph.h"
#include "dotplot.h"
#include "entropy.h"
#include "histogram.h"
#include "trigraph.h"
#include "renderer3d.h"
#include "cloud_color.h"
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
#include "nav_bars.h"
#include "autocorr.h"
#include "ui_widgets.h"
#include "byte_info.h"
#include "view_results.h"
#include "occurrences.h"
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
    void applyResults();
    void handle3DInput(float fElapsedTime);
    // Vista 3D (visualizer_3d.cpp, visualizer_3d_draw.cpp)
    void draw3D(float fElapsedTime);
    void drawCloudLegend();
    void repaintCloud();
    CloudColor::Mode cloudColorMode() const;
    std::vector<TriGraph::Range> sectionRanges() const;
    bool tripleUnderMouse(uint32_t& key) const;
    void handle2DInput();
    void draw2DCanvas();
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
    // Finestra limitata alla dimensione del file
    struct InspectWindow { size_t start = 0, end = 0, len = 0; };
    InspectWindow inspectWindow() const;
    size_t pointUnderMouse(bool over, int cx, int cy) const;  // SIZE_MAX = nessuno
    void inspectSection(PanelWriter& pw, int index);
    void inspectPoint(PanelWriter& pw, size_t point);
    void inspectRange(PanelWriter& pw, int cx, int cy);
    void inspectValue(PanelWriter& pw, int cx, int cy);
    void inspectTriple(PanelWriter& pw, uint32_t key);
    void writeOccurrences(PanelWriter& pw, const uint8_t* pat, size_t n);
    void inspectPick(PanelWriter& pw);
    void inspectSummary(PanelWriter& pw);
    void writeSectionField(PanelWriter& pw, size_t off);
    void writeHexDump(PanelWriter& pw, size_t center, size_t mark, int rows);
    ByteInfo::Stats zoneStats(size_t a, size_t b) const;
    void syncNavigator();                  // barre -> finestra
    void saveView();
    // Vista -> file (visualizer_links.cpp)
    bool   isValueView() const;
    size_t valueAt(int cx, int cy, uint8_t pat[3]) const;
    size_t valueUnderMouse(uint8_t pat[3]) const;    // quanti byte, 0 = nessuno
    void   updateLinks(float dt);
    void   rebuildMarks();
    // Sezione -> vista (visualizer_highlight.cpp)
    void setHighlight(size_t start, size_t end);
    bool highlightInWindow(size_t& a, size_t& b) const;
    void applyHighlight(olc::Sprite* spr);
    void highlightByPosition(olc::Sprite* spr);
    void highlightByValue(olc::Sprite* spr);
    void buildTripleMask();
    bool mouseToContent(int mx, int my, int& cx, int& cy) const;
    int  sectionAt(size_t offset) const;   // -1 se nessuna
    size_t firstOccurrence(const uint8_t* pat, size_t n);  // con cache
    // Pannello raw pixel
    void drawRawPanel();
    void handleRawInput();
    void recomputeRawPixels();
    void syncRawSliders();
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
    ColorMode     m_colorMode = ColorMode::ENTROPY;  // come l'originale
    CurveMode     m_curveMode = CurveMode::HILBERT;
    std::string   m_status;
    std::string   m_computeError;
    std::string   m_initPath;

    // Threading
    std::future<void>  m_future;
    std::atomic<bool>  m_computing{false};
    std::mutex         m_dataMutex;       // protegge solo m_ready e m_computeError
    std::unique_ptr<ViewResults> m_ready; // risultati pronti da prendere
    bool               m_recomputeNeeded = false;
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

    // Barre di navigazione
    NavBars m_bars;

    // Vista -> file: valore scelto col click e segni sulle barre
    Occur::Walker        m_pick;
    std::vector<uint8_t> m_markWanted, m_marked;
    static constexpr float MARK_WAIT = 0.08f;   // secondi col mouse fermo
    float  m_markWait = 0.0f;
    size_t m_markLo = 0, m_markHi = 0;
    bool   m_pressed = false;
    int    m_pressX = 0, m_pressY = 0;

    // Zona evidenziata nella vista (hover su una sezione); start == end = nessuna
    size_t m_hlStart = 0, m_hlEnd = 0;
    std::vector<bool> m_tripleMask;      // 3D: triple presenti nella sezione
    bool       m_tripleMaskOn = false;
    olc::Pixel m_tripleTint   = olc::WHITE;
    CloudColor::Mode m_cloudColor = CloudColor::Mode::SECTION;

    // Finestra su cui sono state calcolate le viste correnti
    size_t m_computedStart = SIZE_MAX, m_computedEnd = SIZE_MAX;
};
