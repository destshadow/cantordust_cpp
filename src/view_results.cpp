#include "view_results.h"

std::unique_ptr<ViewResults> computeViews(const std::vector<uint8_t>& window,
                                          const ComputeParams& p,
                                          ClassifierModel& classifier) {
    auto r = std::make_unique<ViewResults>();

    switch (p.mode) {
        case ViewMode::DIGRAPH:    r->digraph.compute(window);   break;
        case ViewMode::DOTPLOT:    r->dotplot.compute(window);   break;
        case ViewMode::ENTROPY:    r->entropy.compute(window);   break;
        case ViewMode::HISTOGRAM:  r->histogram.compute(window); break;
        case ViewMode::TRIGRAPH3D: r->trigraph.compute(window, p.sections); break;
        case ViewMode::BYTECLOUD:  r->bytecloud.compute(window); break;
        case ViewMode::ONETUPLE:   r->onetuple.compute(window);  break;
        case ViewMode::RAWPIXELS:
            r->rawpixels.compute(window, p.bpp, p.rawWidth, p.rawOffset);
            r->autocorr.compute(window);
            r->suggestWidth = r->autocorr.bestWidth(RawPixels::bytesPerPixel(p.bpp));
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

    for (uint8_t b : window) r->windowHist[b]++;
    return r;
}
