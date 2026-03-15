#include "visualizer.h"
#include <filesystem>

void Visualizer::drawScrollBars() {}

void Visualizer::recomputeWindow() {
    if (m_computing.load()) return;
    m_computing = true;
    m_dirty     = false;

    auto   bytes = m_reader.getBytes();
    size_t start = m_navigator.getStart();
    size_t end   = m_navigator.getEnd();

    m_future = std::async(std::launch::async,
                          [this, bytes, start, end]() {
        // Calcola tutto in locale — nessun accesso ai membri
        std::vector<uint8_t> window(bytes.begin() + start,
                                    bytes.begin() + end);
        DiGraph   dg;  dg.compute(window);
        DotPlot   dp;  dp.compute(window);
        Entropy   en;  en.compute(window);
        Histogram hi;  hi.compute(window);
        TriGraph  tr;  tr.compute(bytes, start, end);

        // Swap atomico sotto mutex
        {
            std::lock_guard<std::mutex> lk(m_dataMutex);
            m_digraph   = std::move(dg);
            m_dotplot   = std::move(dp);
            m_entropy   = std::move(en);
            m_histogram = std::move(hi);
            m_trigraph  = std::move(tr);
            m_dirty     = true;
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

    // Aspetta eventuale calcolo precedente
    if (m_future.valid())
        m_future.wait();

    const auto& bytes = m_reader.getBytes();
    m_navigator.setData(bytes, 0);
    m_parser.parse(bytes);

    m_computing = true;
    m_dirty     = false;
    m_zoom2d    = 1.0f;
    m_scrollH   = 0.0f;
    m_scrollV   = 0.0f;

    m_future = std::async(std::launch::async, [this, bytes]() {
        DiGraph   dg;  dg.compute(bytes);
        DotPlot   dp;  dp.compute(bytes);
        Entropy   en;  en.compute(bytes);
        Histogram hi;  hi.compute(bytes);
        TriGraph  tr;  tr.compute(bytes);

        {
            std::lock_guard<std::mutex> lk(m_dataMutex);
            m_digraph   = std::move(dg);
            m_dotplot   = std::move(dp);
            m_entropy   = std::move(en);
            m_histogram = std::move(hi);
            m_trigraph  = std::move(tr);
            m_dirty     = true;
        }
        m_computing = false;
    });

    std::string fmt = m_parser.getFormatName();
    size_t nsec     = m_parser.getSections().size();
    m_status = "Caricato: " + filepath +
               "  [" + fmt + "]" +
               (nsec > 0 ? "  " + std::to_string(nsec) + " sezioni" : "");
}
