#include "tests.h"

void testAutocorr() {
    // --- Autocorr: trova la larghezza di un'immagine nascosta nel rumore ---
    {
        std::mt19937 noise(7);
        auto hidden = [&](int width, int bpp, int rows) {
            std::vector<uint8_t> b(200000);
            for (auto& x : b) x = uint8_t(noise());
            // "Foto" sintetica non periodica: la prima riga e' una passeggiata
            // casuale, ogni riga successiva e' la precedente piu' poco rumore
            // (come in un'immagine vera, righe vicine si somigliano).
            std::vector<int> row(width * bpp);
            int v = 128;
            for (auto& r : row) { v = std::clamp(v + int(noise() % 17) - 8, 0, 255); r = v; }
            for (int y = 0; y < rows; ++y)
                for (auto& r : row) {
                    r = std::clamp(r + int(noise() % 7) - 3, 0, 255);
                    b.push_back(uint8_t(r));
                }
            for (int i = 0; i < 100000; ++i) b.push_back(uint8_t(noise()));
            return b;
        };
        struct Case { int width, bpp; } cases[] = {{317, 3}, {450, 1}, {200, 4}, {733, 3}};
        for (auto [width, bpp] : cases) {
            Autocorr ac;
            ac.compute(hidden(width, bpp, 180));
            int found = ac.bestWidth(bpp);
            if (std::abs(found - width) > 1)
                std::cerr << "Autocorr: width " << width << " bpp " << bpp
                          << " -> trovata " << found << "\n";
            assert(std::abs(found - width) <= 1);
        }
        std::vector<uint8_t> pureNoise(300000);
        for (auto& x : pureNoise) x = uint8_t(noise());
        Autocorr ac;
        ac.compute(pureNoise);
        assert(ac.bestWidth(3) == 0 && ac.bestWidth(1) == 0);  // niente falsi positivi
    }

    // --- Autocorr: dimensione dei record di una tabella ---
    {
        std::mt19937 noise(11);
        auto table = [&](int size, int rows) {
            std::vector<uint8_t> b;
            for (int r = 0; r < rows; r++)
                for (int i = 0; i < size; i++) {
                    uint8_t v = uint8_t(i * 37);                 // campi fissi
                    if (i == 3)        v = uint8_t(r);           // contatore
                    if (i == size / 2) v = uint8_t(noise());     // campo casuale
                    b.push_back(v);
                }
            return b;
        };
        for (int size : {16, 24, 64, 100, 256}) {
            Autocorr ac;
            ac.compute(table(size, 40000 / size + 200));
            int found = ac.bestWidth(1);
            if (found != size)
                std::cerr << "Autocorr: record " << size << " -> " << found << "\n";
            assert(found == size);
        }
        // Una tabella in mezzo a dati casuali
        std::vector<uint8_t> mixed(50000);
        for (auto& x : mixed) x = uint8_t(noise());
        auto t = table(64, 2000);
        mixed.insert(mixed.end(), t.begin(), t.end());
        Autocorr ac;
        ac.compute(mixed);
        assert(ac.bestWidth(1) == 64);
        // Un file tutto uguale non ha nessun periodo
        Autocorr flat;
        flat.compute(std::vector<uint8_t>(100000, 7));
        assert(flat.bestWidth(1) == 0);
    }
}
