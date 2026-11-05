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
#include "raw_pixels.h"
#include "trigraph.h"
#include "view_mode.h"
#include <array>
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
};

// Il risultato di un calcolo: si riempie nel thread di calcolo, che non
// tocca mai le viste mostrate. L'interfaccia lo prende quando e' pronto,
// cosi' ogni vista e' usata da un solo thread per volta.
struct ViewResults {
    DiGraph   digraph;
    DotPlot   dotplot;
    Entropy   entropy;
    Histogram histogram;
    TriGraph  trigraph;
    RawPixels rawpixels;
    MetricMap metricmap;
    ByteCloud bytecloud;
    OneTuple  onetuple;
    Autocorr  autocorr;
    int       suggestWidth = 0;
    std::array<uint32_t, 256> windowHist{};
};

// Calcola solo la vista richiesta (piu' l'istogramma, che serve sempre)
std::unique_ptr<ViewResults> computeViews(const std::vector<uint8_t>& window,
                                          const ComputeParams& params,
                                          ClassifierModel& classifier);
