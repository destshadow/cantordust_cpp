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
    m_computing = true;
    m_dirty     = false;

    auto      bytes     = m_reader.getBytes();
    size_t    start     = m_navigator.getStart();
    size_t    end       = m_navigator.getEnd();
    BppMode   bpp       = m_bppMode;
    ColorMode cmode     = m_colorMode;
    CurveMode curvemode = m_curveMode;

    m_future = std::async(std::launch::async,
        [this, bytes, start, end, bpp, cmode, curvemode]() {
            std::vector<uint8_t> window(bytes.begin() + start,
                                        bytes.begin() + end);
            DiGraph   dg;  dg.compute(window);
            DotPlot   dp;  dp.compute(window);
            Entropy   en;  en.compute(window);
            Histogram hi;  hi.compute(window);
            TriGraph  tr;  tr.compute(bytes, start, end);
            RawPixels rp;  rp.compute(window, bpp);
            MetricMap mm;  mm.compute(window, cmode, curvemode, 8);
            ByteCloud bc;  bc.compute(window);
            OneTuple  ot;  ot.compute(window);

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
            m_computing = false;
    });
}

void Visualizer::loadFile(const std::string& filepath) {
    if (!std::filesystem::exists(filepath)) {
        m_status = "Errore: file non trovato: " + filepath;
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

    m_computing  = true;
    m_dirty      = false;
    m_zoom2d     = 1.0f;
    m_scrollH    = 0.0f;
    m_scrollV    = 0.0f;
    m_bppMode    = BppMode::BPP_8;
    m_colorMode  = ColorMode::WAVELENGTH;
    m_curveMode  = CurveMode::HILBERT;

    ColorMode cmode     = m_colorMode;
    CurveMode curvemode = m_curveMode;

    m_future = std::async(std::launch::async,
        [this, bytes, cmode, curvemode]() {
            DiGraph   dg;  dg.compute(bytes);
            DotPlot   dp;  dp.compute(bytes);
            Entropy   en;  en.compute(bytes);
            Histogram hi;  hi.compute(bytes);
            TriGraph  tr;  tr.compute(bytes);
            RawPixels rp;  rp.compute(bytes, BppMode::BPP_8);
            MetricMap mm;  mm.compute(bytes, cmode, curvemode, 8);
            ByteCloud bc;  bc.compute(bytes);
            OneTuple  ot;  ot.compute(bytes);
            OverviewBar ob; ob.compute(bytes);

            {
                std::lock_guard<std::mutex> lk(m_dataMutex);
                m_overviewBar = std::move(ob);
                m_dirty = true;
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
            m_computing = false;
    });

    std::string fmt  = m_parser.getFormatName();
    size_t      nsec = m_parser.getSections().size();
    m_status = "Caricato: " + filepath +
               "  [" + fmt + "]" +
               (nsec > 0 ? "  " + std::to_string(nsec) +
                " sezioni" : "");
}
