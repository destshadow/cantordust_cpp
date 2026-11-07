#include "tests.h"

void testNavigation() {
    // --- Tab: larghe quanto il testo, senza buchi ne' sovrapposizioni ---
    {
        for (int i = 0; i < VIEW_TAB_COUNT; i++) {
            assert(tabAt(tabX(i)) == i);
            assert(tabAt(tabX(i) + tabWidth(i) - 1) == i);
        }
        assert(tabAt(-1) == -1);
        const int end = tabX(VIEW_TAB_COUNT - 1) + tabWidth(VIEW_TAB_COUNT - 1);
        assert(tabAt(end) == -1);
        assert(end <= 1280 - 70);              // a destra restano gli fps
    }

    // --- RangeSel: la selezione delle barre resta sempre valida ---
    {
        RangeSel s;
        s.min = 0; s.max = 1000; s.set(0, 1000);
        assert(s.lo == 0 && s.hi == 1000);
        s.setLo(990);                      // la maniglia non supera l'altra
        s.setHi(995);
        assert(s.hi - s.lo == RangeSel::MIN_SIZE);
        s.set(100, 200);
        s.slideTo(950);                    // spostando la dimensione non cambia
        assert(s.lo == 900 && s.hi == 1000);
        s.slideTo(0);
        assert(s.lo == 0 && s.hi == 100);

        s.set(100, 200);
        s.reveal(150);                     // gia' dentro: non si muove
        assert(s.lo == 100 && s.hi == 200);
        s.reveal(700);                     // fuori: ci va, stessa dimensione
        assert(s.lo == 650 && s.hi == 750);
        s.reveal(0);
        assert(s.lo == 0 && s.hi == 100);
        s.reveal(999);
        assert(s.lo == 900 && s.hi == 1000);

        s.set(400, 600);
        s.zoomAround(500, 0.5);            // il byte sotto il mouse resta fermo
        assert(s.lo == 450 && s.hi == 550);
        for (int i = 0; i < 100; i++) s.zoomAround(500, 0.5);
        assert(s.size() == RangeSel::MIN_SIZE);
        for (int i = 0; i < 100; i++) s.zoomAround(500, 2.0);
        assert(s.lo == 0 && s.hi == 1000);

        s.set(100, 200);                   // ZONA dentro FILE: stessi byte...
        s.setBounds(150, 800);
        assert(s.lo == 150 && s.hi == 200);
        s.setBounds(500, 800);             // ...e mai fuori dai confini
        assert(s.lo == 500 && s.hi == 516);

        for (size_t tiny : {0, 1, 5, 16}) {   // file piu' piccoli della selezione
            RangeSel t;
            t.min = 0; t.max = tiny; t.set(0, tiny);
            t.setLo(tiny); t.setHi(0); t.slideTo(tiny); t.zoomAround(0, 0.5);
            assert(t.lo <= t.hi && t.hi <= tiny);
        }
    }

    // --- Occurrences: ricerca, conteggio, segni, passaggio alla successiva ---
    {
        std::vector<uint8_t> b = {1, 0, 0, 0, 2, 1, 0, 9, 1, 0};
        const uint8_t zz[] = {0, 0}, oz[] = {1, 0}, none[] = {7}, nine[] = {9};
        assert(Occur::next(b, 0, b.size(), zz, 2) == 1);
        assert(Occur::next(b, 2, b.size(), zz, 2) == 2);   // si sovrappongono
        assert(Occur::next(b, 3, b.size(), zz, 2) == SIZE_MAX);
        assert(Occur::count(b, 0, b.size(), zz, 2) == 2);
        assert(Occur::count(b, 0, b.size(), oz, 2) == 3);
        assert(Occur::count(b, 0, 9, oz, 2) == 2);         // deve stare tutta dentro
        assert(Occur::count(b, 0, b.size(), none, 1) == 0);
        assert(Occur::next(b, 9, b.size(), nine, 1) == SIZE_MAX);
        std::vector<uint8_t> empty;
        assert(Occur::next(empty, 0, 0, nine, 1) == SIZE_MAX);
        assert(Occur::count(empty, 0, 0, zz, 2) == 0);

        auto r = Occur::rows(b, 0, b.size(), nine, 1, 10);
        assert(r.size() == 10 && r[7] && !r[6] && !r[8]);
        r = Occur::rows(b, 0, b.size(), oz, 2, 2);         // poche righe
        assert(r[0] && r[1]);
        assert(Occur::rows(empty, 0, 0, zz, 2, 5).size() == 5);

        Occur::Walker w;
        w.step(b, 0, b.size(), oz, 2);
        assert(w.count == 3 && w.index == 1 && w.target == 0);
        w.step(b, 0, b.size(), oz, 2);
        assert(w.index == 2 && w.target == 5);
        w.step(b, 0, b.size(), oz, 2);
        assert(w.index == 3 && w.target == 8);
        w.step(b, 0, b.size(), oz, 2);                     // dopo l'ultima, la prima
        assert(w.index == 1 && w.target == 0);
        w.step(b, 0, b.size(), zz, 2);                     // altro valore: si riparte
        assert(w.count == 2 && w.index == 1 && w.target == 1);
        w.step(b, 2, b.size(), zz, 2);                     // altra finestra: si riparte
        assert(w.count == 1 && w.index == 1 && w.target == 2);
        w.step(b, 0, b.size(), none, 1);                   // assente: niente selezione
        assert(!w.active() && w.pattern.empty());
    }

    // --- Occurrences: i segni veloci coincidono con il calcolo diretto ---
    {
        std::mt19937 g(5);
        for (int t = 0; t < 3000; ++t) {
            std::vector<uint8_t> b(g() % 3000);
            int alpha = 1 + g() % 4;
            for (auto& x : b) x = uint8_t(g() % alpha);
            size_t n  = b.size();
            size_t lo = n ? g() % n : 0;
            size_t hi = lo + (n > lo ? g() % (n - lo + 1) : 0);
            uint8_t p[2] = {uint8_t(g() % alpha), uint8_t(g() % alpha)};
            size_t m = 1 + g() % 2;
            int rows = 1 + g() % 700;
            std::vector<bool> direct(rows, false);
            for (size_t i = lo; hi > lo && i + m <= hi; ++i)
                if (b[i] == p[0] && (m == 1 || b[i + 1] == p[1])) {
                    int r = int(double(i - lo) / double(hi - lo) * rows);
                    direct[std::min(r, rows - 1)] = true;
                }
            assert(Occur::rows(b, lo, hi, p, m, rows) == direct);
        }
    }
}
