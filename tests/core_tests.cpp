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
    assert(map.fileIndexAt(0, 0) == 0);
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

    std::cout << "All core regression tests passed\n";
}
