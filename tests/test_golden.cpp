#include "tests.h"
#include <cstdlib>

// Le immagini delle viste, su dati fissi, confrontate con impronte salvate
// (tests/golden_hashes.inc). Scopre un cambiamento nel disegno senza aprire
// la finestra.
//
// Se un'immagine cambia APPOSTA:  make golden   (riscrive le impronte)
// e si controlla a occhio che il cambiamento sia quello voluto.
//
// Le impronte dipendono dalla libreria matematica: su un'altra piattaforma
// (es. Windows) vanno rigenerate una volta.

namespace {

struct Golden { const char* name; uint64_t hash; };
const Golden GOLDEN[] = {
#include "golden_hashes.inc"
    {nullptr, 0}
};

uint64_t fnv(const std::vector<uint8_t>& b) {
    uint64_t h = 1469598103934665603ULL;
    for (uint8_t v : b) { h ^= v; h *= 1099511628211ULL; }
    return h;
}

// Un file finto con un po' di tutto: testo, zeri, record, un'immagine liscia
// a colori, rumore. Sempre uguale: mt19937 e' definito dallo standard.
std::vector<uint8_t> sampleFile() {
    std::mt19937 g(2026);
    std::vector<uint8_t> b;
    const char* text = "The quick brown fox jumps over the lazy dog. 0123456789\n";
    for (int i = 0; i < 400; i++) b.insert(b.end(), text, text + 56);
    b.insert(b.end(), 9000, 0x00);
    for (int r = 0; r < 800; r++) {                       // record da 24 byte
        const uint8_t head[4] = {0xCA, 0xFE, (uint8_t)r, (uint8_t)(r >> 8)};
        b.insert(b.end(), head, head + 4);
        for (int k = 0; k < 20; k++) b.push_back(uint8_t(g()));
    }
    for (int y = 0; y < 120; y++)                         // immagine 200x120 RGB
        for (int x = 0; x < 200; x++) {
            b.push_back((uint8_t)(x + y));
            b.push_back((uint8_t)(2 * x));
            b.push_back((uint8_t)(255 - y * 2));
        }
    b.insert(b.end(), 3000, 0xFF);
    for (int i = 0; i < 40000; i++) b.push_back(uint8_t(g()));
    return b;
}

} // namespace

void testGolden() {
    const std::vector<uint8_t> b = sampleFile();
    std::vector<std::pair<std::string, uint64_t>> got;
    auto image = [&](const std::string& name, const std::vector<uint8_t>& rgb) {
        got.push_back({name, fnv(rgb)});
    };

    { DiGraph v;    v.compute(b); image("digraph", v.toRGB()); }
    { DotPlot v;    v.compute(b); image("dotplot", v.toRGB()); }
    { Similarity v; v.compute(b); image("somiglianza", v.toRGB()); }
    { Entropy v;    v.compute(b); image("entropia", v.toRGB()); }
    { Histogram v;  v.compute(b); image("istogramma", v.toRGB()); }
    { ByteCloud v;  v.compute(b); image("bytecloud", v.toRGB()); }
    { OneTuple v;   v.compute(b); image("onetuple", v.toRGB()); }
    { Spectrogram v; v.compute(b, 1024);       image("spettrogramma", v.toRGB());
                     v.compute(b, 1024, true); image("spettrogramma_medio", v.toRGB()); }

    const BppMode bpps[] = {BppMode::BPP_8, BppMode::BPP_16, BppMode::BPP_24,
                            BppMode::BPP_32, BppMode::BPP_64};
    for (int m = 0; m < 5; m++) {
        RawPixels v;
        v.compute(b, bpps[m], 200, 0);
        image(std::string("raw_") + RawPixels::modeName(bpps[m]), v.getRGB());
    }
    for (int f = 1; f < RawFilter::COUNT; f++) {
        RawPixels v;
        v.compute(b, BppMode::BPP_8, 256, 3, RawFilter{f});
        image(std::string("raw_filtro_") + std::to_string(f), v.getRGB());
    }
    for (int c = 0; c < 5; c++)                           // senza Classifier: servono i template
        for (int k = 0; k < 3; k++) {
            MetricMap v;
            v.compute(b, (ColorMode)c, (CurveMode)k, 8, nullptr);
            image(std::string("metric_") + MetricMap::colorModeName((ColorMode)c) + "_" +
                  MetricMap::curveModeName((CurveMode)k), v.getRGB());
        }

    // make golden: stampa la tabella nuova e basta
    if (std::getenv("GOLDEN_PRINT")) {
        for (const auto& g : got)
            std::printf("    {\"%s\", 0x%016llxULL},\n", g.first.c_str(), (unsigned long long)g.second);
        std::exit(0);
    }

    int wrong = 0;
    size_t known = 0;
    while (GOLDEN[known].name) known++;
    for (size_t i = 0; i < got.size(); i++) {
        const bool same = i < known && got[i].first == GOLDEN[i].name &&
                          got[i].second == GOLDEN[i].hash;
        if (!same) {
            std::cerr << "IMMAGINE CAMBIATA: " << got[i].first << std::endl;
            wrong++;
        }
    }
    if (wrong || known != got.size()) {
        std::cerr << wrong << " immagini diverse dalle impronte salvate. Se e' voluto: make golden"
                  << std::endl;
        assert(false && "immagini cambiate");
    }
}
