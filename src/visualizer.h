#pragma once
#include "binary_reader.h"
#include "digraph.h"
#include "dotplot.h"
#include "entropy.h"
#include "histogram.h"
#include "screenshot.h"
#include "../libs/olcPixelGameEngine.h"

// Le 4 modalita' di visualizzazione
enum class ViewMode {
    DIGRAPH   = 0,
    DOTPLOT   = 1,
    ENTROPY   = 2,
    HISTOGRAM = 3
};

class Visualizer : public olc::PixelGameEngine {
public:
    Visualizer();
    bool OnUserCreate() override;
    bool OnUserUpdate(float fElapsedTime) override;

private:
    void updateCanvas();   // rigenera il canvas in base alla vista attiva
    void drawUI();         // barra info + tasti
    std::vector<uint8_t> getCurrentRGB() const;
    void loadFile(const std::string& filepath);

    BinaryReader m_reader;
    DiGraph      m_digraph;
    DotPlot      m_dotplot;
    Entropy      m_entropy;
    Histogram    m_histogram;

    olc::Sprite* m_canvas = nullptr;
    olc::Decal*  m_decal  = nullptr;

    ViewMode    m_mode  = ViewMode::DIGRAPH;
    bool        m_dirty = false;
    std::string m_status;
};
