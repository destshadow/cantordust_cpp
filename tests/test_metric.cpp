#include "tests.h"

void testMetricMap() {
    // Dot Plot ed Entropy: tutte le 256 colonne, anche con pochi dati
    {
        auto litColumns = [](const std::vector<uint8_t>& rgb) {
            int n = 0;
            for (int x = 0; x < 256; x++) {
                bool on = false;
                for (int y = 0; y < 256; y++)
                    if (y != 128 && (rgb[(y * 256 + x) * 3] || rgb[(y * 256 + x) * 3 + 1] ||
                                     rgb[(y * 256 + x) * 3 + 2])) on = true;
                n += on;
            }
            return n;
        };
        std::mt19937 g(3);
        for (size_t size : {300u, 2000u, 20000u, 500000u}) {
            std::vector<uint8_t> v(size);
            for (auto& x : v) x = uint8_t(g());
            Entropy e;
            e.compute(v);
            assert(litColumns(e.toRGB()) == 256);
        }
        std::vector<uint8_t> v(100000);
        for (auto& x : v) x = uint8_t(g());
        DotPlot d;
        d.compute(v);
        assert(litColumns(d.toRGB()) == 256);
        // columnStart e' l'inverso esatto della formula delle colonne
        for (size_t n : {1u, 7u, 255u, 256u, 257u, 1000u, 123456u})
            for (size_t i = 0; i < n; i += 1 + n / 500) {
                int x = (int)(i * 256 / n);
                assert(columnStart(x, n) <= i && i < std::max(columnStart(x + 1, n), columnStart(x, n) + 1));
            }
        assert(columnStart(256, 1000) == 1000);
    }

    // --- MetricMap: entropia locale su 32 byte, 0 sui dati uniformi ---
    {
        std::vector<uint8_t> uniform(1024, 0x41), random(1024);
        std::mt19937 g(11);
        for (auto& x : random) x = uint8_t(g());
        MetricMap m;
        m.compute(uniform, ColorMode::ENTROPY, CurveMode::LINEAR);
        assert(m.getRGB()[0] == 0 && m.getRGB()[2] == 0);   // uniforme: nero
        m.compute(random, ColorMode::ENTROPY, CurveMode::LINEAR);
        assert(m.getRGB()[0] > 200 && m.getRGB()[2] > 200); // casuale: viola/magenta
    }

    // --- MetricMap: offset esatti anche su file grandi, pixel senza byte ---
    {
        std::vector<uint8_t> big(50 * 1000 * 1000, 1);
        MetricMap m;
        m.compute(big, ColorMode::WAVELENGTH, CurveMode::LINEAR, 8);
        // Ultimo pixel della curva: riga 255 (dispari) percorsa a rovescio
        assert(m.fileIndexAt(0, 255) == size_t(65535.0 * big.size() / 65536.0));
        std::vector<uint8_t> small(10, 1);
        m.compute(small, ColorMode::WAVELENGTH, CurveMode::LINEAR, 8);
        assert(m.fileIndexAt(9, 0) == 9);
        assert(m.fileIndexAt(10, 0) == SIZE_MAX);           // oltre la fine del file
    }

    {   // Metric Map: giro dei colori, senza template si salta il classifier
        ColorMode m = ColorMode::SPECTRUM;
        m = MetricMap::nextColorMode(m, false);
        assert(m == ColorMode::BYTE_CLASS);
        assert(MetricMap::nextColorMode(m, false) == ColorMode::WAVELENGTH);
        assert(MetricMap::nextColorMode(m, true)  == ColorMode::CLASSIFIER);
        assert(MetricMap::nextColorMode(ColorMode::CLASSIFIER, true) == ColorMode::WAVELENGTH);
    }

    {   // Metric Map: colore per tipo di byte
        std::vector<uint8_t> b = {'A', 0x00, 0xFF, 0x80, 0x01};
        MetricMap m;
        m.compute(b, ColorMode::BYTE_CLASS, CurveMode::LINEAR);
        uint8_t r, g, bl;
        ByteInfo::classColor(ByteInfo::Class::TEXT, r, g, bl);
        assert(m.getRGB()[0] == r && m.getRGB()[1] == g && m.getRGB()[2] == bl);
        ByteInfo::classColor(ByteInfo::Class::FF, r, g, bl);
        assert(m.getRGB()[6] == r);
    }
}
