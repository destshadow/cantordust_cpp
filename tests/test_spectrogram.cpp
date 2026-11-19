#include "tests.h"
#include "fft.h"
#include "spectrogram.h"
#include <complex>

namespace {

// Record da `period` byte: un campo fisso, un contatore, il resto casuale
std::vector<uint8_t> records(size_t count, size_t period, std::mt19937& g) {
    std::vector<uint8_t> b;
    for (size_t i = 0; i < count; i++) {
        b.push_back(0xAA); b.push_back(0x55);
        b.push_back(uint8_t(i));
        for (size_t k = 3; k < period; k++) b.push_back(k < 6 ? uint8_t(g()) : uint8_t(k * 7));
    }
    return b;
}

// Il periodo di base della colonna, in byte
double basePeriod(const Spectrogram& s, int col) {
    const int r = s.baseRow(col);
    return r < 0 ? 0 : s.periodAt(r);
}

bool near(double a, double b, double tol) { return std::abs(a - b) <= tol * b; }

} // namespace

void testSpectrogram() {
    {   // FFT: uguale alla DFT scritta per esteso
        std::mt19937 g(7);
        std::vector<std::complex<double>> a(64), slow(64);
        for (auto& x : a) x = {double(g() % 256), 0.0};
        const double PI = std::acos(-1.0);
        for (size_t k = 0; k < 64; k++)
            for (size_t j = 0; j < 64; j++)
                slow[k] += a[j] * std::polar(1.0, -2 * PI * double(j * k) / 64.0);
        assert(FFT::transform(a));
        for (size_t k = 0; k < 64; k++) assert(std::abs(a[k] - slow[k]) < 1e-6);
        std::vector<std::complex<double>> odd(12, 1.0);
        assert(!FFT::transform(odd) && odd[0] == 1.0);      // non tocca nulla
        assert(FFT::floorPowerOf2(0) == 0 && FFT::floorPowerOf2(1) == 1);
        assert(FFT::floorPowerOf2(1000) == 512 && FFT::floorPowerOf2(1024) == 1024);
    }
    {   // Record da 24 byte: il periodo di base e' 24, ben acceso
        std::mt19937 g(1);
        const auto b = records(20000, 24, g);
        Spectrogram s;
        s.compute(b, 1024);
        assert(s.columns() == 256 && s.fftSize() == 1024 && s.maxPeriod() == 512);
        for (int c : {0, 100, 255}) {
            assert(near(basePeriod(s, c), 24, 0.04));
            assert(s.level(c, s.rowOf(24)) > 1.0f);          // 10 volte la media
        }
        // Togliendo lo spettro medio un file tutto uguale si spegne
        s.compute(b, 1024, true);
        assert(std::abs(s.level(100, s.rowOf(24))) < 0.5f);
    }
    {   // Record da 24 byte con 12 zeri in fondo: le armoniche alte (8, 6, 4...)
        // sono forti, ma il periodo di base resta 24
        std::mt19937 g(6);
        std::vector<uint8_t> b;
        for (int i = 0; i < 20000; i++) {
            const uint8_t head[12] = {0xAA, 0x55, uint8_t(i), uint8_t(i >> 8),
                                      uint8_t(g()), uint8_t(g()), uint8_t(g()), uint8_t(g()),
                                      0, 0, uint8_t(i), 0x41};
            b.insert(b.end(), head, head + 12);
            b.insert(b.end(), 12, 0);
        }
        Spectrogram s;
        for (int n : {512, 1024, 4096}) {
            s.compute(b, n);
            for (int c : {3, 128, 250}) assert(near(basePeriod(s, c), 24, 0.04));
        }
    }
    {   // Il periodo intero: da 23,7 o 24,4 a 24; bordi
        std::mt19937 g(8);
        const auto b = records(500, 24, g);
        for (double approx : {23.4, 23.73, 24.0, 24.4, 25.0})
            assert(Spectrogram::exactPeriod(b, 0, 4096, approx) == 24);
        const auto forty = records(300, 40, g);
        for (double approx : {39.1, 40.2, 41.5})
            assert(Spectrogram::exactPeriod(forty, 0, 8192, approx) == 40);
        assert(Spectrogram::exactPeriod(b, 0, 4096, 48.3) == 48);     // un multiplo resta se stesso
        assert(Spectrogram::exactPeriod({}, 0, 100, 7.0) == 7);
        assert(Spectrogram::exactPeriod(b, b.size() + 10, 100, 0.2) == 1);
    }
    {   // Due tipi di record: il periodo cambia dove cambia il tipo
        std::mt19937 g(2);
        auto b = records(10000, 24, g);
        const auto second = records(6000, 40, g);
        b.insert(b.end(), second.begin(), second.end());
        Spectrogram s;
        s.compute(b, 2048);
        assert(near(basePeriod(s, 20), 24, 0.04));
        assert(near(basePeriod(s, 240), 40, 0.04));
        assert(s.columnOf(240000) < s.columnOf(250000));
        // Il tratto dove dura il periodo 24: dall'inizio fin quasi al cambio
        // (i record da 24 sono 240000 byte su 480000: meta' delle colonne)
        int first = -1, last = -1;
        assert(s.runAt(20, s.rowOf(24), first, last));
        assert(first == 0 && last >= 120 && last <= 130);
        assert(s.runAt(240, s.rowOf(40), first, last) && first >= 126 && first <= 132 && last == 255);
        assert(!s.runAt(20, s.rowOf(100), first, last));     // li' non c'e' niente
        assert(!s.runAt(-1, 0, first, last) && !s.runAt(300, 0, first, last));
    }
    {   // Dati casuali: quasi tutto scuro, nessun periodo di base
        std::mt19937 g(3);
        std::vector<uint8_t> b(1 << 20);
        for (auto& x : b) x = uint8_t(g());
        Spectrogram s;
        s.compute(b, 1024);
        const auto rgb = s.toRGB();
        size_t bright = 0;
        for (size_t i = 0; i < rgb.size(); i += 3) bright += rgb[i + 1] > 128;   // giallo o piu'
        assert(bright < rgb.size() / 3 / 200);
        int withBase = 0;
        for (int c = 0; c < 256; c++) withBase += s.baseRow(c) >= 0;
        assert(withBase < 3);                       // il caso non inventa periodi
    }
    {   // Bordi: vuoto, troppo corto, costante, piu' corto della FFT
        Spectrogram s;
        s.compute({});
        assert(s.columns() == 0 && s.toRGB().size() == 256u * 256 * 3 && s.columnOf(0) == -1);
        s.compute(std::vector<uint8_t>(15, 1));
        assert(s.columns() == 0 && s.periodAt(10) == 0 && s.level(0, 0) == Spectrogram::DARK);
        s.compute(std::vector<uint8_t>(5000, 9));
        assert(s.columns() == 256 && s.baseRow(10) == -1 && s.level(10, 10) == Spectrogram::DARK);
        std::mt19937 g(4);
        const auto small = records(13, 24, g);              // 312 byte
        s.compute(small, 4096);
        assert(s.fftSize() == 256 && s.columns() == 256);
        for (int c = 0; c < s.columns(); c++)
            assert(s.segmentStart(c) + 256 <= small.size());
        s.compute(std::vector<uint8_t>(100, 3), 1024);
        assert(s.columns() == 100 && s.fftSize() == 64);
    }
    {   // Colonna ↔ posizione, anche con lunghezze che non si dividono
        std::mt19937 g(5);
        for (size_t len : {16u, 255u, 257u, 1000u, 163957u}) {
            std::vector<uint8_t> b(len);
            for (auto& x : b) x = uint8_t(g());
            Spectrogram s;
            s.compute(b, 64);
            for (int c = 0; c < s.columns(); c++) {
                assert(s.columnOf(s.columnStart(c)) == c);
                if (s.columnStart(c + 1) > s.columnStart(c))
                    assert(s.columnOf(s.columnStart(c + 1) - 1) == c);
            }
            assert(s.columnOf(len + 100) == s.columns() - 1);
        }
        Spectrogram s;
        s.compute(std::vector<uint8_t>(4096, 1), 64);
        for (double p : {2.0, 7.0, 24.0, 32.0}) assert(near(s.periodAt(s.rowOf(p)), p, 0.02));
        assert(s.rowOf(1.0) == 0 && s.rowOf(1e9) == Spectrogram::SIZE - 1);
    }
    {   // Il calcolo guarda la bandierina
        std::atomic<bool> flag{false};
        Cancel::Scope scope(&flag);
        const size_t before = Cancel::polls;
        Spectrogram s;
        s.compute(std::vector<uint8_t>(1 << 16, 1), 256);
        assert(Cancel::polls - before >= 256);
        flag = true;
        s.compute(std::vector<uint8_t>(1 << 16, 1), 256);    // si ferma subito, senza crash
    }
}
