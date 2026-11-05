#include "file_navigator.h"
#include "ngram_model.h"
#include "classifier_model.h"
#include "binary_format.h"
#include "binary_reader.h"
#include "metric_map.h"
#include "autocorr.h"
#include "raw_pixels.h"
#include "byte_info.h"
#include "density.h"
#include "range_sel.h"
#include "occurrences.h"
#include "trigraph.h"
#include "cloud_frame.h"
#include "cloud_color.h"
#include "camera3d.h"
#include "line_clip.h"
#include <string>
#include <cmath>
#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>
#include <random>

static void put(std::vector<uint8_t>& b, size_t off, uint64_t value, int width) {
    for (int i = 0; i < width; ++i) b.at(off + i) = uint8_t(value >> (8*i));
}

int main() {
    for (size_t size : {0, 1, 10, 255, 256, 1024, 10000}) {
        std::vector<uint8_t> bytes(size, 42);
        FileNavigator nav;
        nav.setData(bytes);
        for (int i = 0; i < 20; ++i) {
            nav.moveForward();
            assert(nav.getStart() <= nav.getEnd() && nav.getEnd() <= size);
            assert(nav.getWindow(bytes).size() == nav.getEnd() - nav.getStart());
        }
        nav.moveBackward();
        assert(nav.getEnd() <= size);
        nav.selectRange(size / 2, std::numeric_limits<size_t>::max());
        assert(nav.getStart() == size / 2 && nav.getEnd() == size);
        nav.resetWindow(bytes);
        assert(nav.getWindow(bytes) == bytes);
    }
    FileNavigator nav;
    nav.setData(std::vector<uint8_t>(1000));
    nav.selectRange(120, 45);
    assert(nav.getStart() == 120 && nav.getEnd() == 165);

    NGramModel exact(std::vector<uint8_t>{1, 2}, 2);
    assert(exact.getModelSize() == 1 && exact.getModelEntries() == 1);
    NGramModel window(std::vector<uint8_t>(10, 42), 0, 4, 2);
    assert(window.getModelSize() == 1 && window.getModelEntries() == 3);
    NGramModel distinct(std::vector<uint8_t>{1, 2, 3, 4}, 2);
    assert(distinct.getModelSize() == 3);
    bool rejected = false;
    try { NGramModel invalid(std::vector<uint8_t>{1}, 0, 2, 1); }
    catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected);

    ClassifierModel classifier;
    classifier.generateTemplates({{}, std::vector<uint8_t>(64, 42)}, 2);
    classifier.classifyFile(std::vector<uint8_t>(515, 42));
    assert(classifier.getBlockCount() == 2);
    assert(classifier.classAtIndex(0) == 1 && classifier.classAtIndex(514) == 1);
    assert(classifier.classAtIndex(515 + 512) == -1);
    classifier.classifyFile({});
    assert(classifier.getBlockCount() == 0);

    BinaryParser parser;
    std::vector<uint8_t> pe(64, 0);
    pe[0] = 'M'; pe[1] = 'Z';
    put(pe, 0x3c, 0xfffffff0, 4);
    parser.parse(pe);
    assert(parser.getFormat() == BinaryFormat::UNKNOWN && !parser.hasSections());

    std::vector<uint8_t> elf(256, 0);
    elf[0] = 0x7f; elf[1] = 'E'; elf[2] = 'L'; elf[3] = 'F';
    elf[4] = 2; elf[5] = 1;
    put(elf, 0x28, 64, 8); put(elf, 0x3a, 64, 2);
    put(elf, 0x3c, 2, 2); put(elf, 0x3e, 0, 2);
    put(elf, 64+0x18, 192, 8);
    elf[193] = '.'; elf[194] = 'b'; elf[195] = 's'; elf[196] = 's';
    put(elf, 128, 1, 4); put(elf, 128+4, 8, 4);
    put(elf, 128+0x18, 220, 8); put(elf, 128+0x20, 16, 8);
    parser.parse(elf);
    assert(!parser.hasSections()); // NOBITS is not file content.
    put(elf, 128+4, 1, 4);
    parser.parse(elf);
    assert(parser.getSections().size() == 1 && parser.getSections()[0].offset == 220);
    put(elf, 128+0x20, 1000, 8);
    parser.parse(elf);
    assert(!parser.hasSections());
    elf[5] = 2;
    parser.parse(elf);
    assert(parser.getFormat() == BinaryFormat::UNKNOWN);

    // Mutated headers must never produce sections outside the input buffer.
    std::mt19937 rng(42);
    for (int trial = 0; trial < 2000; ++trial) {
        std::vector<uint8_t> fuzz(64 + rng() % 512);
        for (auto& byte : fuzz) byte = uint8_t(rng());
        if (trial % 2) {
            fuzz[0] = 'M'; fuzz[1] = 'Z';
        } else {
            fuzz[0] = 0x7f; fuzz[1] = 'E'; fuzz[2] = 'L'; fuzz[3] = 'F';
            fuzz[4] = 1 + rng() % 2; fuzz[5] = 1;
        }
        parser.parse(fuzz);
        for (const auto& sec : parser.getSections()) {
            assert(sec.offset <= fuzz.size());
            assert(sec.size <= fuzz.size() - sec.offset);
        }
    }

    MetricMap map;
    map.compute({});
    assert(map.fileIndexAt(0, 0) == SIZE_MAX);          // nessun byte
    std::vector<uint8_t> unique(16);
    for (int i = 0; i < 16; ++i) unique[i] = i;
    map.compute(unique, ColorMode::ENTROPY, CurveMode::LINEAR);
    assert(map.getRGB()[0] > 0); // High local entropy reaches the red channel.
    ClassifierData cd;
    std::vector<int> classes{1}; cd.classifications = &classes;
    map.compute(unique, ColorMode::CLASSIFIER, CurveMode::LINEAR, 8, &cd);
    auto rgb = map.getRGB();
    map.compute(unique, ColorMode::WAVELENGTH, CurveMode::LINEAR);
    assert(rgb != map.getRGB());

    BinaryReader reader;
    assert(reader.load("src/main.cpp"));
    auto original = reader.getBytes();
    assert(!reader.load("/this/path/does/not/exist"));
    assert(reader.isLoaded() && reader.getBytes() == original);
    // --- RawPixels: dimensioni, offset, campionamento righe ---
    {
        std::vector<uint8_t> px(3 * 100 * 10 + 2, 7);
        RawPixels rp;
        rp.compute(px, 0, px.size(), BppMode::BPP_24, 100, 2);
        assert(rp.getWidth() == 100 && rp.getHeight() == 10);
        assert(rp.byteOffsetAt(0, 0) == 2 && rp.byteOffsetAt(1, 1) == 2 + 3 * 101);
        assert(rp.byteOffsetAt(100, 0) == SIZE_MAX);
        rp.compute(px, 0, px.size(), BppMode::BPP_8, 1);  // 3002 righe > MAX_ROWS
        assert(rp.getHeight() <= RawPixels::MAX_ROWS && rp.getRowStep() == 2);
        rp.compute(px, 10, 10, BppMode::BPP_8, 64);       // finestra vuota
        assert(rp.getWidth() == 0 && rp.getRGB().empty());
    }

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

    // --- ByteInfo: classi di byte e ipotesi sul contenuto ---
    {
        using ByteInfo::Class;
        assert(ByteInfo::classOf(0x00) == Class::ZERO);
        assert(ByteInfo::classOf('A')  == Class::TEXT);
        assert(ByteInfo::classOf('\n') == Class::TEXT);
        assert(ByteInfo::classOf(0x07) == Class::CONTROL);
        assert(ByteInfo::classOf(0x80) == Class::HIGH);
        assert(ByteInfo::classOf(0xFF) == Class::FF);

        auto guessOf = [](const std::vector<uint8_t>& b) {
            return std::string(ByteInfo::guess(ByteInfo::statsOf(b, 0, b.size())));
        };
        std::mt19937 g(3);
        std::vector<uint8_t> zeros(1024, 0), random(1024), text, smooth;
        for (auto& x : random) x = uint8_t(g());
        const char* lorem = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. ";
        while (text.size() < 1024) text.insert(text.end(), lorem, lorem + 57);
        int v = 100;                                   // "immagine": passeggiata lenta
        for (int i = 0; i < 1024; ++i) {
            v = std::clamp(v + int(g() % 9) - 4, 0, 255);
            smooth.push_back(uint8_t(v));
        }
        for (auto& x : smooth) x = uint8_t(x + (g() % 3));   // un po' di grana
        assert(guessOf(zeros)  == "zeri / padding");
        assert(guessOf(random) == "alta entropia: compresso/cifrato");
        assert(guessOf(text)   == "testo");
        ByteInfo::Stats st = ByteInfo::statsOf(smooth, 0, smooth.size());
        if (guessOf(smooth) != "valori lisci: immagine/audio raw?")
            std::cerr << "smooth: entropia " << st.entropy << " liscio " << st.smooth << "\n";
        assert(guessOf(smooth) == "valori lisci: immagine/audio raw?");
    }

    // --- Density: un valore dominante non deve spegnere gli altri ---
    {
        std::vector<uint32_t> cells(1000, 3);
        cells[0] = 100000000;                       // es. la coppia 00 00
        cells[1] = 0;
        float ref = Density::reference(cells.data(), cells.size());
        assert(ref == 3.0f);
        assert(Density::intensity(3, ref) == 255);  // la massa resta luminosa
        assert(Density::intensity(0, ref) == 0);    // il vuoto resta nero
        assert(Density::intensity(cells[0], ref) == 255);
        assert(Density::intensity(1, ref) > 60);    // visto una volta: visibile
        std::vector<uint32_t> empty(16, 0);
        assert(Density::reference(empty.data(), empty.size()) == 2.0f);
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

    // --- Parser: piu' di 64 sezioni non vengono perse ---
    {
        const size_t nsec = 100, shoff = 64, stroff = shoff + nsec * 64;
        std::vector<uint8_t> e(stroff + 64, 0);
        e[0] = 0x7f; e[1] = 'E'; e[2] = 'L'; e[3] = 'F'; e[4] = 2; e[5] = 1;
        put(e, 0x28, shoff, 8); put(e, 0x3a, 64, 2);
        put(e, 0x3c, nsec, 2);  put(e, 0x3e, 0, 2);
        put(e, shoff + 0x18, stroff, 8);                    // tabella dei nomi
        e[stroff + 1] = '.'; e[stroff + 2] = 'x';
        for (size_t i = 1; i < nsec; ++i) {
            size_t h = shoff + i * 64;
            put(e, h, 1, 4); put(e, h + 4, 1, 4);           // nome ".x", PROGBITS
            put(e, h + 0x18, 16, 8); put(e, h + 0x20, 8, 8);
        }
        BinaryParser p;
        p.parse(e);
        assert(p.getSections().size() == nsec - 1);
    }

    {   // 3D: conteggi, ricerca, sezione prevalente
        //              A A A A A   B B B B B
        std::vector<uint8_t> b = {1,2,3,1,2,3,1,2,3, 9,9,9, 1,2,3, 7,7,7};
        std::vector<TriGraph::Range> secs = {{0, 11, 0}, {12, 18, 5}};
        TriGraph t;
        t.compute(b, secs);
        const Point3D* p = t.find(0x010203);
        assert(p && p->count == 4);
        assert(p->section == 0);                 // 3 volte in A, 1 in B
        assert(t.find(0x070707)->section == 5);
        assert(t.find(0x040404) == nullptr);
        assert(std::is_sorted(t.getPoints().begin(), t.getPoints().end(),
            [](const Point3D& x, const Point3D& y) { return x.key < y.key; }));

        // A cavallo tra due sezioni: non appartiene a nessuna
        assert(t.find(0x090901)->section == -1);

        TriGraph none;
        none.compute(b);
        assert(none.find(0x010203)->section == -1);
        none.compute({1, 2});                    // meno di 3 byte
        assert(none.getPointCount() == 0);

        // Il colore per sezione arriva dalla sezione; fuori, grigio
        std::vector<BinarySection> ss(6);
        ss[5].r = 10; ss[5].g = 20; ss[5].b = 30;
        CloudColor::paint(t.getPoints(), CloudColor::Mode::SECTION, ss);
        const Point3D* q = t.find(0x070707);
        assert(q->r == 10 && q->g == 20 && q->b == 30);
        // Una sezione che non esiste non deve far leggere fuori dall'array
        CloudColor::paint(t.getPoints(), CloudColor::Mode::SECTION, {});
        assert(t.find(0x070707)->r == 90);
    }

    {   // 3D: colori per classe e ciclo dei modi
        uint8_t r, g, b, er, eg, eb;
        CloudColor::classColor(0x414243, r, g, b);        // 'ABC' = testo
        ByteInfo::classColor(ByteInfo::Class::TEXT, er, eg, eb);
        assert(r == er && g == eg && b == eb);
        CloudColor::classColor(0x4100FF, r, g, b);        // tutte diverse
        CloudColor::mixedColor(er, eg, eb);
        assert(r == er && g == eg && b == eb);
        using CloudColor::Mode;
        assert(CloudColor::next(Mode::FREQUENCY, true)  == Mode::SECTION);
        assert(CloudColor::next(Mode::FREQUENCY, false) == Mode::BYTE_CLASS);
    }

    {   // 3D: fotogramma, densita' e picking
        CloudFrame f;
        f.resize(20, 10);
        assert(f.keyNear(5, 5, 4) == CloudFrame::NO_KEY);
        f.add(5, 5, 200, 0, 0, 3, 111);
        f.add(5, 5, 200, 0, 0, 9, 222);          // piu' pesante nello stesso pixel
        f.add(9, 5, 0, 200, 0, 1, 333);
        assert(f.keyNear(5, 5, 4) == 222);
        assert(f.keyNear(8, 5, 4) == 333);       // il piu' vicino, non il piu' pesante
        assert(f.keyNear(5, 1, 3) == CloudFrame::NO_KEY);   // fuori dal raggio
        assert(f.keyNear(-3, 5, 0) == CloudFrame::NO_KEY);  // fuori dal fotogramma

        f.resolve();
        uint32_t px = f.pixels()[5 * 20 + 5];
        assert((px & 0xFF) > 0 && ((px >> 8) & 0xFF) == 0);  // resta rosso, non bianco
        assert(f.pixels()[0] == 0xFF000000u);                // vuoto = nero
        // Un punto isolato deve restare visibile accanto a un pixel denso
        uint32_t lone = f.pixels()[5 * 20 + 9];
        assert(((lone >> 8) & 0xFF) >= 60);

        f.clear();
        assert(f.keyNear(5, 5, 4) == CloudFrame::NO_KEY);
    }

    {   // 3D: il pixel di un punto e' quello che il picking ritrova
        Camera3D cam;
        Camera3D::Viewport vp{0, 0, 700, 700};
        Mat3 m = cam.rotation();
        CloudFrame f;
        f.resize(vp.w, vp.h);
        const uint32_t key = 0x20407F;
        float sx, sy;
        assert(cam.toScreen(cam.toCamera(m, 0x20 / 127.5f - 1, 0x40 / 127.5f - 1, 0x7F / 127.5f - 1),
                            vp, sx, sy));
        f.add((int)sx, (int)sy, 1, 1, 1, 1, key);
        assert(f.keyNear((int)sx, (int)sy, 0) == key);

        // Dietro la camera non si proietta (la divisione ribalterebbe il punto)
        cam.camZ = 2.9f;
        assert(!cam.inFront({0, 0, -3.0f}));
        assert(!cam.toScreen({0, 0, -3.0f}, vp, sx, sy));
    }

    {   // 3D: trascinando, il punto del cubo piu' vicino segue il mouse.
        // Deve valere da qualunque lato si stia guardando il cubo.
        Camera3D::Viewport vp{0, 0, 700, 700};
        for (float ax : {-1.2f, -0.5f, 0.0f, 0.5f, 1.2f})
            for (float ay : {-3.0f, -0.7f, 0.0f, 0.6f, 1.5f, 2.4f, 4.0f}) {
                Camera3D cam;
                cam.angleX = ax;
                cam.angleY = ay;
                // Il punto del cubo davanti agli occhi: la riga Z della
                // rotazione, cambiata di segno, e' la direzione verso la camera
                Mat3 m = cam.rotation();
                const float px = -m[2][0], py = -m[2][1], pz = -m[2][2];
                auto screen = [&](const Camera3D& c, float& sx, float& sy) {
                    assert(c.toScreen(c.toCamera(c.rotation(), px, py, pz), vp, sx, sy));
                };
                float x0, y0, x1, y1;
                screen(cam, x0, y0);
                Camera3D right = cam;
                right.orbit(10, 0);
                screen(right, x1, y1);
                assert(x1 > x0);
                Camera3D down = cam;
                down.orbit(0, 10);
                screen(down, x1, y1);
                assert(y1 > y0);
            }
    }

    {   // Ritaglio delle linee
        float x0 = -10, y0 = 5, x1 = 30, y1 = 5;
        assert(clipLine(x0, y0, x1, y1, 0, 0, 20, 10));
        assert(x0 == 0 && x1 == 20 && y0 == 5 && y1 == 5);
        float a = 2, b = 2, c = 3, d = 3;                    // tutta dentro
        assert(clipLine(a, b, c, d, 0, 0, 20, 10) && a == 2 && d == 3);
        float e = -5, f = -5, g = -1, h = 30;                // tutta fuori
        assert(!clipLine(e, f, g, h, 0, 0, 20, 10));
        float i = 25, j = 1, k = 25, l = 9;                  // parallela e fuori
        assert(!clipLine(i, j, k, l, 0, 0, 20, 10));
    }

    std::cout << "All core regression tests passed\n";
}
