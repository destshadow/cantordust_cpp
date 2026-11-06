#include "tests.h"

void testMetricMap() {
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
