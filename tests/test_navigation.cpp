#include "tests.h"

void testNavigation() {
    // --- Tab: larghe quanto il testo, senza buchi ne' sovrapposizioni ---
    {
        int visible = 0, inMenu = 0;
        for (int i = 0; i < VIEW_TAB_COUNT; i++) {
            if (VIEW_TABS[i].inMenu) {               // nessuna tab propria
                inMenu++;
                assert(tabWidth(i) == 0);
                assert(menuTabOf(VIEW_TABS[i].mode) == &VIEW_TABS[i]);
                continue;
            }
            visible++;
            assert(tabAt(tabX(i)) == i);
            assert(tabAt(tabX(i) + tabWidth(i) - 1) == i);
            assert(menuTabOf(VIEW_TABS[i].mode) == nullptr);
        }
        assert(visible + inMenu == VIEW_TAB_COUNT && visible > 0 && inMenu > 0);
        assert(tabAt(-1) == -1);
        // "Altro" segue l'ultima tab ed e' abbastanza larga per ogni suo nome
        assert(tabAt(moreTabX() - 1) >= 0);
        assert(tabAt(moreTabX()) == TAB_MORE);
        assert(tabAt(moreTabX() + moreTabWidth() - 1) == TAB_MORE);
        const int end = moreTabX() + moreTabWidth();
        assert(tabAt(end) == -1);
        assert(end <= 1280 - 70);              // a destra restano gli fps
        for (const ViewTab& t : VIEW_TABS)
            if (t.inMenu) assert((int)std::strlen(t.label) * 8 + TAB_PAD * 2 + 16 <= moreTabWidth());
        // Ogni vista si raggiunge con un tasto diverso
        for (int i = 0; i < VIEW_TAB_COUNT; i++)
            for (int k = i + 1; k < VIEW_TAB_COUNT; k++) assert(VIEW_TABS[i].key != VIEW_TABS[k].key);
    }

    // --- Menu' a comparsa: quale voce sotto il mouse, frecce ---
    {
        MenuState m;
        m.count = 3;
        const int w = 120;
        assert(m.height() == 3 * MenuState::ROW_H + 2 * MenuState::PAD);
        assert(m.rowAt(5, MenuState::PAD, w) == 0);
        assert(m.rowAt(5, MenuState::PAD + MenuState::ROW_H * 3 - 1, w) == 2);
        assert(m.rowAt(5, MenuState::PAD - 1, w) == MenuState::NONE);       // sul bordo
        assert(m.rowAt(5, MenuState::PAD + MenuState::ROW_H * 3, w) == MenuState::NONE);
        assert(m.rowAt(-1, 10, w) == MenuState::NONE && m.rowAt(w, 10, w) == MenuState::NONE);
        assert(m.contains(0, 0, w) && !m.contains(0, m.height(), w) && !m.contains(w, 0, w));

        m.move(1);                                  // senza voce: la prima
        assert(m.hover == 0);
        m.move(1); m.move(1); m.move(1);            // si ferma all'ultima
        assert(m.hover == 2);
        m.move(-5);
        assert(m.hover == 0);
        m.hover = MenuState::NONE;
        m.move(-1);                                 // da sotto: l'ultima
        assert(m.hover == 2);

        MenuState empty;
        empty.move(1);
        assert(empty.hover == MenuState::NONE && empty.rowAt(0, 5, w) == MenuState::NONE);
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

        // I confini si spostano: dentro resta cio' che c'era, ma se non
        // resta nulla si prende tutto (non 16 byte in un angolo)
        s.set(100, 200);
        s.setBounds(150, 400);
        assert(s.lo == 150 && s.hi == 200);
        s.setBounds(600, 800);
        assert(s.lo == 600 && s.hi == 800);
        s.setBounds(0, 1000);
        // I confini scorrono: la selezione li segue
        s.set(100, 200);
        s.follow(300, 1300, 300);
        assert(s.lo == 400 && s.hi == 500);
        s.follow(0, 1000, -300);
        assert(s.lo == 100 && s.hi == 200);

        // Rotella col mouse fuori dalla selezione: non salta sotto il mouse
        s.set(400, 600);
        s.zoomAround(900, 0.5);
        assert(s.hi == 600 && s.lo == 500);
        s.set(400, 600);
        s.zoomAround(0, 0.5);
        assert(s.lo == 400 && s.hi == 500);

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
        s.setBounds(500, 800);             // ...ma se non ne resta nessuno, tutta
        assert(s.lo == 500 && s.hi == 800);

        // Qualunque sequenza di mosse lascia la selezione valida
        std::mt19937_64 gen(3);
        for (size_t total : {size_t(1), size_t(17), size_t(1000), size_t(1) << 40}) {
            RangeSel macro, micro;
            macro.min = 0; macro.max = total; macro.set(0, total);
            micro.min = 0; micro.max = total; micro.set(0, total);
            for (int step = 0; step < 20000; step++) {
                const size_t v = gen() % (total + 1), oldLo = macro.lo, oldSize = macro.size();
                RangeSel& t = gen() % 2 ? macro : micro;
                switch (gen() % 6) {
                    case 0: t.setLo(v); break;
                    case 1: t.setHi(v); break;
                    case 2: t.slideTo(v); break;
                    case 3: t.zoomAround(v, gen() % 2 ? 0.8 : 1.25); break;
                    case 4: t.reveal(v); break;
                    case 5: t.set(v, gen() % (total + 1)); break;
                }
                if (macro.lo != oldLo && macro.size() == oldSize)
                    micro.follow(macro.lo, macro.hi, (long long)macro.lo - (long long)oldLo);
                else
                    micro.setBounds(macro.lo, macro.hi);
                for (const RangeSel* r : {&macro, &micro}) {
                    assert(r->min <= r->lo && r->lo <= r->hi && r->hi <= r->max);
                    assert(r->size() >= r->minSize());
                }
                assert(macro.max == total && micro.min == macro.lo && micro.max == macro.hi);
            }
        }

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
