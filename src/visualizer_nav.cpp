#include "visualizer.h"
#include <filesystem>

void Visualizer::recomputeWindow() {
    if (m_computing.load()) return;
    m_recomputeNeeded = false;
    m_computedStart   = m_navigator.getStart();
    m_computedEnd     = m_navigator.getEnd();
    try {
        auto window = m_navigator.getWindow(m_reader.getBytes());
        ComputeParams params;
        params.mode      = m_mode;
        params.bpp       = m_bppMode;
        params.colorMode = m_colorMode;
        params.curveMode = m_curveMode;
        params.rawWidth  = m_rawWidth;
        params.rawOffset = (size_t)m_rawOffset;
        if (m_mode == ViewMode::TRIGRAPH3D) params.sections = sectionRanges();

        m_computing = true;
        m_future = std::async(std::launch::async,
            [this, window = std::move(window), params]() {
                try {
                    auto results = computeViews(window, params, m_classifier);
                    std::lock_guard<std::mutex> lk(m_dataMutex);
                    m_ready = std::move(results);
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

// Prende i risultati pronti: da qui in poi le viste sono dell'interfaccia
void Visualizer::applyResults() {
    std::unique_ptr<ViewResults> r;
    {
        std::lock_guard<std::mutex> lk(m_dataMutex);
        if (!m_computeError.empty()) {
            m_status = std::move(m_computeError);
            m_computeError.clear();
        }
        r = std::move(m_ready);
    }
    if (!r) return;

    m_digraph      = std::move(r->digraph);
    m_dotplot      = std::move(r->dotplot);
    m_entropy      = std::move(r->entropy);
    m_histogram    = std::move(r->histogram);
    m_trigraph     = std::move(r->trigraph);
    m_rawpixels    = std::move(r->rawpixels);
    m_metricmap    = std::move(r->metricmap);
    m_bytecloud    = std::move(r->bytecloud);
    m_onetuple     = std::move(r->onetuple);
    m_autocorr     = std::move(r->autocorr);
    m_suggestWidth = r->suggestWidth;
    m_windowHist   = r->windowHist;
    repaintCloud();
    updateCanvas();
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
    {   // un risultato del file precedente non serve piu'
        std::lock_guard<std::mutex> lk(m_dataMutex);
        m_ready.reset();
    }

    const auto& bytes = m_reader.getBytes();
    m_navigator.setData(bytes, 0);
    m_parser.parse(bytes);
    m_bars.setFile(&bytes, &m_parser.getSections());

    m_zoom2d = 1.0f;
    m_scrollH = m_scrollV = 0.0f;
    m_bppMode = BppMode::BPP_8;
    m_colorMode = ColorMode::ENTROPY;
    m_curveMode = CurveMode::HILBERT;
    m_rawWidth  = 256;
    m_rawOffset = 0;
    m_suggestWidth = 0;
    m_occKey.clear();     // la cache vale per il file precedente
    m_hlStart = m_hlEnd = 0;
    m_tripleMaskOn = false;
    m_pick.clear();
    m_marked.clear();
    m_markWanted.clear();
    m_bars.clearMarks();
    m_bars.setCursor(SIZE_MAX);
    m_recomputeNeeded = true;
    recomputeWindow();

    std::string fmt  = m_parser.getFormatName();
    size_t      nsec = m_parser.getSections().size();
    m_status = "Caricato: " + filepath +
               "  [" + fmt + "]" +
               (nsec > 0 ? "  " + std::to_string(nsec) +
                " sezioni" : "");
}
