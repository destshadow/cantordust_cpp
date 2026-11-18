#include "tests.h"

void testCore() {
    // --- Un calcolo si puo' annullare: esce subito e non da' un risultato ---
    {
        using Clock = std::chrono::steady_clock;
        auto seconds = [](Clock::time_point from) {
            return std::chrono::duration<double>(Clock::now() - from).count();
        };
        std::mt19937_64 g(9);
        std::vector<uint8_t> noise(64 << 20), lines, copies;
        for (size_t k = 0; k + 8 <= noise.size(); k += 8) {
            const uint64_t v = g();
            std::memcpy(&noise[k], &v, 8);
        }
        // Righe di testo di lunghezza varia: nella ricerca delle stringhe
        // l'indice salta di una riga alla volta
        while (lines.size() < (32u << 20)) {
            lines.insert(lines.end(), 5 + g() % 60, (uint8_t)('a' + g() % 26));
            lines.push_back('\n');
        }
        // Pochi blocchi ripetuti moltissimo: nella Somiglianza i gruppi sono grandi
        for (int k = 0; k < 6000; k++) copies.insert(copies.end(), noise.begin(), noise.begin() + 4096);

        ClassifierModel model;
        struct Case { ViewMode mode; const std::vector<uint8_t>* data; };
        const Case cases[] = {
            {ViewMode::TRIGRAPH3D, &noise}, {ViewMode::ENTROPY, &noise},
            {ViewMode::DIGRAPH, &noise},    {ViewMode::DOTPLOT, &noise},
            {ViewMode::STRINGS, &noise},    {ViewMode::STRINGS, &lines},
            {ViewMode::DOTPLOT, &copies},   {ViewMode::DOTPLOT, &lines},
        };
        for (const Case& c : cases) {
            ComputeParams p;
            p.mode = c.mode;
            p.similarity = c.data != &noise;         // sul rumore il dot plot dei valori
            if (c.mode == ViewMode::TRIGRAPH3D) p.sections = {{0, c.data->size(), 0}};

            // Il calcolo intero, per sapere quanto dura su questa macchina
            auto t0 = Clock::now();
            const auto full = computeViews(*c.data, p, model);
            const double whole = seconds(t0);
            assert(full && full->mode == c.mode);

            // Bandierina gia' alzata: nessun risultato, e in una frazione del tempo
            // (il migliore di tre: un singolo tempo puo' essere rovinato da
            // un attimo di carico della macchina)
            std::atomic<bool> cancel{true};
            double stopped = 1e9;
            for (int attempt = 0; attempt < 3; attempt++) {
                t0 = Clock::now();
                assert(computeViews(*c.data, p, model, &cancel) == nullptr);
                stopped = std::min(stopped, seconds(t0));
            }
            assert(stopped < 0.05 || stopped < whole / 4);

            // Alzata a meta' da un altro thread: torna poco dopo, mai bloccato
            for (int delayMs : {0, 1, 5, 30}) {
                cancel = false;
                auto run = std::async(std::launch::async,
                                      [&] { return computeViews(*c.data, p, model, &cancel); });
                std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
                cancel = true;
                t0 = Clock::now();
                const auto r = run.get();
                const double after = seconds(t0);
                // Largo apposta: qui conta che torni, la prontezza la misurano
                // il controllo sopra e il conteggio dei controlli piu' sotto
                assert(after < 0.25 || after < whole / 2);
                assert(!r || r->mode == c.mode);     // finito prima, o annullato
            }
        }

        // Dentro le classi: annullate lasciano il lavoro a meta', non dati sbagliati
        {
            std::atomic<bool> cancel{true};
            const Cancel::Scope scope(&cancel);
            TriGraph tri;
            tri.compute(noise, {{0, noise.size(), 0}});
            assert(tri.getPointCount() == 0);
            StringScan strings;
            strings.compute(lines);
            assert(strings.hits().empty());
            Similarity sim;
            sim.compute(copies);
            assert(sim.at(0, 100) == 0.0f);
        }
        assert(Cancel::flag == nullptr);             // lo Scope si e' chiuso

        // Senza cronometro: ogni calcolo lungo guarda la bandierina almeno una
        // volta ogni 128 KB di dati, anche quando l'indice del ciclo salta
        {
            // Testo in tratti da 100003 caratteri: il ciclo delle stringhe
            // avanza di un tratto alla volta e non cade mai su un multiplo di 64 KB
            std::vector<uint8_t> runs;
            while (runs.size() < (16u << 20)) {
                runs.insert(runs.end(), 100003, (uint8_t)'x');
                runs.push_back(0);
            }
            std::atomic<bool> never{false};
            const Cancel::Scope scope(&never);
            auto polled = [&](size_t bytes, auto&& work) {
                Cancel::polls = 0;
                work();
                return Cancel::polls >= bytes / (128 << 10);
            };
            StringScan strings;
            assert(polled(runs.size(), [&] { strings.compute(runs); }));
            assert(strings.hits().size() == runs.size() / 100004);   // e il risultato e' intero
            assert(polled(lines.size(), [&] { strings.compute(lines); }));
            Similarity sim;
            assert(polled(copies.size(), [&] { sim.compute(copies); }));
            TriGraph tri;
            assert(polled(noise.size(), [&] { tri.compute(noise, {{0, noise.size(), 0}}); }));
            DotPlot dots;
            assert(polled(noise.size(), [&] { dots.compute(noise); }));
            Entropy entropy;
            assert(polled(noise.size(), [&] { entropy.compute(noise); }));
            DiGraph pairs;
            assert(polled(noise.size(), [&] { pairs.compute(noise); }));
            Histogram hist;
            assert(polled(noise.size(), [&] { hist.compute(noise); }));
            ByteCloud cloud;
            assert(polled(noise.size(), [&] { cloud.compute(noise); }));
            OneTuple rows;
            assert(polled(noise.size(), [&] { rows.compute(noise); }));
            Overview overview;
            assert(polled(noise.size(), [&] { overview.compute(noise); }));
        }

        // Poll controlla ogni 64 KB percorsi, qualunque sia il passo
        {
            std::atomic<bool> cancel{false};
            const Cancel::Scope scope(&cancel);
            Cancel::Poll poll;
            assert(!poll.at(0));
            cancel = true;
            assert(!poll.at(1000) && !poll.at(65535));   // troppo presto
            assert(poll.at(65537));                      // un passo che scavalca il multiplo
            assert(!Cancel::every(65537) && Cancel::every(65536 * 3));
        }

        // Senza bandierina, o mai alzata, il risultato c'e'
        std::vector<uint8_t> small(4096, 7);
        std::atomic<bool> never{false};
        ComputeParams p;
        assert(computeViews(small, p, model) != nullptr);
        assert(computeViews(small, p, model, &never) != nullptr);
        assert(Cancel::flag == nullptr);             // lo Scope si e' chiuso
    }

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
        nav.resetWindow();
        assert(nav.getWindow(bytes) == bytes);
    }
    FileNavigator nav;
    nav.setData(std::vector<uint8_t>(1000));
    nav.selectRange(120, 45);
    assert(nav.getStart() == 120 && nav.getEnd() == 165);


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

    BinaryReader reader;
    assert(reader.load("src/main.cpp"));
    auto original = reader.getBytes();
    assert(!reader.load("/this/path/does/not/exist"));
    assert(reader.isLoaded() && reader.getBytes() == original);
}
