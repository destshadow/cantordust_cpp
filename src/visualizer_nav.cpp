#include "visualizer.h"
#include <filesystem>

void Visualizer::drawScrollBars() {
    // Posiziona le due barre a sinistra del canvas
    // Adatta OV_X / OV_Y alle tue costanti di layout
    constexpr int OV_X   = 6;
    constexpr int OV_Y   = 30;
    constexpr int OV_GAP = 6;

    m_overviewBar.draw(this, OV_X, OV_Y, OV_GAP,
                       m_navigator.getNormalized(),
                       m_navigator.getWindowRatio());
}

void Visualizer::recomputeWindow() {
    if (m_computing.load()) return;
    try {
        auto window = m_navigator.getWindow(m_reader.getBytes());
        BppMode bpp = m_bppMode;
        ColorMode cmode = m_colorMode;
        CurveMode curvemode = m_curveMode;

        ViewMode mode = m_mode;
        m_computing = true;
        m_dirty = false;
        m_future = std::async(std::launch::async,
            [this, window = std::move(window), bpp, cmode, curvemode, mode]() {
                try {
                    DiGraph   dg;  if (mode == ViewMode::DIGRAPH) dg.compute(window);
                    DotPlot   dp;  if (mode == ViewMode::DOTPLOT) dp.compute(window);
                    Entropy   en;  if (mode == ViewMode::ENTROPY) en.compute(window);
                    Histogram hi;  if (mode == ViewMode::HISTOGRAM) hi.compute(window);
                    TriGraph  tr;  if (mode == ViewMode::TRIGRAPH3D) tr.compute(window);
                    RawPixels rp;  if (mode == ViewMode::RAWPIXELS) rp.compute(window, bpp);
                    ClassifierData cd;
                    if (mode == ViewMode::METRICMAP && cmode == ColorMode::CLASSIFIER && m_classifier.isReady()) {
                        m_classifier.classifyFile(window);
                        cd.classifications = &m_classifier.getClassifications();
                    }
                    MetricMap mm;  if (mode == ViewMode::METRICMAP) mm.compute(window, cmode, curvemode, 8, &cd);
                    ByteCloud bc;  if (mode == ViewMode::BYTECLOUD) bc.compute(window);
                    OneTuple  ot;  if (mode == ViewMode::ONETUPLE) ot.compute(window);

                    {
                        std::lock_guard<std::mutex> lk(m_dataMutex);
                        m_digraph    = std::move(dg);
                        m_dotplot    = std::move(dp);
                        m_entropy    = std::move(en);
                        m_histogram  = std::move(hi);
                        m_trigraph   = std::move(tr);
                        m_rawpixels  = std::move(rp);
                        m_metricmap  = std::move(mm);
                        m_bytecloud  = std::move(bc);
                        m_onetuple   = std::move(ot);
                        m_dirty      = true;
                    }
                } catch (const std::exception& e) {
                    std::lock_guard<std::mutex> lk(m_dataMutex);
                    m_computeError = std::string("Errore calcolo: ") + e.what();
                }
                m_computing = false;
            });
    } catch (const std::exception& e) {
        m_computing = false;
        m_status = std::string("Errore avvio calcolo: ") + e.what();
    }
}

void Visualizer::loadFile(const std::string& filepath) {
    std::error_code ec;
    if (!std::filesystem::is_regular_file(filepath, ec)) {
        m_status = "Errore: file non accessibile o non regolare: " + filepath;
        return;
    }
    if (!m_reader.load(filepath)) {
        m_status = "Errore caricamento: " + filepath;
        return;
    }

    if (m_future.valid()) m_future.wait();

    const auto& bytes = m_reader.getBytes();
    m_navigator.setData(bytes, 0);
    m_parser.parse(bytes);
    m_overviewBar.compute(bytes);

    m_zoom2d = 1.0f;
    m_scrollH = m_scrollV = 0.0f;
    m_bppMode = BppMode::BPP_8;
    m_colorMode = ColorMode::WAVELENGTH;
    m_curveMode = CurveMode::HILBERT;
    m_sectionInfos.clear();
    m_hoveredSection = -1;
    recomputeWindow();

    std::string fmt  = m_parser.getFormatName();
    size_t      nsec = m_parser.getSections().size();
    m_status = "Caricato: " + filepath +
               "  [" + fmt + "]" +
               (nsec > 0 ? "  " + std::to_string(nsec) +
                " sezioni" : "");
}
