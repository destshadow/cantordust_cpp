#include "view_results.h"
#include "pixel_guess.h"
#include "cancel.h"

std::unique_ptr<ViewResults> computeViews(const std::vector<uint8_t>& window,
                                          const ComputeParams& p,
                                          ClassifierModel& classifier,
                                          const std::atomic<bool>* cancel) {
    const Cancel::Scope scope(cancel);
    auto r = std::make_unique<ViewResults>();
    r->windowStart = p.windowStart;
    r->mode        = p.mode;

    switch (p.mode) {
        case ViewMode::DIGRAPH:    r->digraph.compute(window);   break;
        case ViewMode::DOTPLOT:
            if (p.similarity) r->similarity.compute(window);
            else              r->dotplot.compute(window);
            break;
        case ViewMode::ENTROPY:    r->entropy.compute(window);   break;
        case ViewMode::HISTOGRAM:  r->histogram.compute(window); break;
        case ViewMode::TRIGRAPH3D: r->trigraph.compute(window, p.sections); break;
        case ViewMode::BYTECLOUD:  r->bytecloud.compute(window); break;
        case ViewMode::ONETUPLE:   r->onetuple.compute(window);  break;
        case ViewMode::STRINGS:    r->strings.compute(window);   break;
        case ViewMode::HEX:        break;     // legge i byte direttamente
        case ViewMode::OVERVIEW:   r->overview.compute(window);  break;
        case ViewMode::RAWPIXELS:
            r->rawpixels.compute(window, p.bpp, p.rawWidth, p.rawOffset, p.rawFilter);
            r->autocorr.compute(window);
            r->suggestWidth = r->autocorr.bestWidth(RawPixels::bytesPerPixel(p.bpp));
            r->suggestBpp   = PixelGuess::bytesPerPixel(r->autocorr.differences());
            break;
        case ViewMode::METRICMAP: {
            ClassifierData cd;
            if (p.colorMode == ColorMode::CLASSIFIER && classifier.isReady()) {
                classifier.classifyFile(window);
                cd.classifications = &classifier.getClassifications();
            }
            r->metricmap.compute(window, p.colorMode, p.curveMode, 8, &cd);
            break;
        }
    }

    for (size_t i = 0; i < window.size(); i++) {
        if (Cancel::every(i)) break;
        r->windowHist[window[i]]++;
    }
    if (Cancel::requested()) return nullptr;     // risultato a meta': si butta
    return r;
}
