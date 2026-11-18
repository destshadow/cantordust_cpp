#pragma once
#include "autocorr.h"
#include "byte_cloud.h"
#include "classifier_model.h"
#include "digraph.h"
#include "dotplot.h"
#include "entropy.h"
#include "histogram.h"
#include "metric_map.h"
#include "one_tuple.h"
#include "overview.h"
#include "raw_pixels.h"
#include "similarity.h"
#include "string_scan.h"
#include "trigraph.h"
#include "view_mode.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

// Cosa calcolare e con quali impostazioni
struct ComputeParams {
    ViewMode  mode      = ViewMode::DIGRAPH;
    BppMode   bpp       = BppMode::BPP_8;
    ColorMode colorMode = ColorMode::ENTROPY;
    CurveMode curveMode = CurveMode::HILBERT;
    int       rawWidth  = 256;
    size_t    rawOffset = 0;
    bool      similarity = false;            // Dot Plot: il file con se stesso
    size_t    windowStart = 0;               // Stringhe: per gli offset nel file
    RawFilter rawFilter;
    std::vector<TriGraph::Range> sections;   // 3D: relative alla finestra
};

// Il risultato di un calcolo: si riempie nel thread di calcolo, che non
// tocca mai le viste mostrate. L'interfaccia lo prende quando e' pronto,
// cosi' ogni vista e' usata da un solo thread per volta.
struct ViewResults {
    DiGraph   digraph;
    DotPlot   dotplot;
    Similarity similarity;
    Entropy   entropy;
    Histogram histogram;
    TriGraph  trigraph;
    RawPixels rawpixels;
    MetricMap metricmap;
    ByteCloud bytecloud;
    OneTuple  onetuple;
    StringScan strings;
    Overview  overview;
    Autocorr  autocorr;
    int       suggestWidth = 0;
    size_t    suggestBpp   = 1;                // byte per pixel, 1 = nessun indizio
    size_t    windowStart  = 0;
    ViewMode  mode         = ViewMode::DIGRAPH;   // la vista calcolata
    std::array<uint32_t, 256> windowHist{};
};

// Calcola solo la vista richiesta (piu' l'istogramma, che serve sempre).
// Se `cancel` viene alzata durante il calcolo restituisce nullptr appena puo'.
std::unique_ptr<ViewResults> computeViews(const std::vector<uint8_t>& window,
                                          const ComputeParams& params,
                                          const ClassifierModel& classifier,
                                          const std::atomic<bool>* cancel = nullptr);
