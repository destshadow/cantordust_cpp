#include "tests.h"

void testOverview() {
    // --- Chi-quadro: valori noti delle tabelle ---
    {
        assert(std::fabs(ChiSquare::pValue(3.841, 1) - 0.05) < 0.001);
        assert(std::fabs(ChiSquare::pValue(11.070, 5) - 0.05) < 0.001);
        assert(std::fabs(ChiSquare::pValue(293.25, 255) - 0.05) < 0.002);
        assert(ChiSquare::pValue(0.0, 10) == 1.0);
        assert(ChiSquare::pValue(5000.0, 10) < 1e-9);
        assert(ChiSquare::pValue(10.0, 0) == 0.0);
    }

    std::mt19937 g(4);
    auto histOf = [](const std::vector<uint8_t>& b) {
        std::array<uint64_t, 256> h{};
        for (uint8_t v : b) h[v]++;
        return h;
    };

    // --- Bit bassi: casuali solo dove qualcuno li ha resi tali ---
    {
        std::vector<uint8_t> text;
        const char* lorem = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. ";
        while (text.size() < 100000) text.insert(text.end(), lorem, lorem + 57);
        assert(Overview::lowBitRandomness(histOf(text)) < 0.01f);

        std::vector<uint8_t> hidden = text;          // un messaggio nei bit bassi
        for (auto& v : hidden) v = (uint8_t)((v & 0xFE) | (g() & 1));
        assert(Overview::lowBitRandomness(histOf(hidden)) > 0.5f);

        std::vector<uint8_t> noise(100000);          // tutto casuale: non dice nulla
        for (auto& v : noise) v = uint8_t(g());
        assert(Overview::lowBitRandomness(histOf(noise)) == 0.0f);

        std::vector<uint8_t> few(100, 'a');          // troppo pochi byte
        assert(Overview::lowBitRandomness(histOf(few)) == 0.0f);
        assert(Overview::lowBitRandomness(histOf({})) == 0.0f);
    }

    // --- Periodo: record, non zeri ne' rumore ---
    {
        uint8_t len = 0;
        std::vector<uint8_t> records;                // 16 byte: 4 fissi, 12 casuali
        for (int r = 0; r < 200; r++) {
            const uint8_t head[4] = {0xCA, 0xFE, 0x00, 0x10};
            records.insert(records.end(), head, head + 4);
            for (int k = 0; k < 12; k++) records.push_back(uint8_t(g()));
        }
        const float s = Overview::periodicity(records.data(), records.data() + records.size(), len);
        assert(len == 16 && s > 0.2f);

        std::vector<uint8_t> noise(4096);
        for (auto& v : noise) v = uint8_t(g());
        assert(Overview::periodicity(noise.data(), noise.data() + noise.size(), len) == 0.0f);
        assert(len == 0);

        std::vector<uint8_t> zeros(4096, 0);         // un solo valore non e' un periodo
        assert(Overview::periodicity(zeros.data(), zeros.data() + zeros.size(), len) == 0.0f);
        assert(Overview::periodicity(zeros.data(), zeros.data() + 10, len) == 0.0f);   // troppo corto
    }

    // --- La vista intera: tratti giusti e indicatori dove devono stare ---
    {
        const size_t part = 200000;
        std::vector<uint8_t> b;
        for (size_t i = 0; i < part; i++) b.push_back((uint8_t)("abcdefgh "[i % 9]));   // testo
        b.insert(b.end(), part, 0x00);                                                   // zeri
        for (size_t i = 0; i < part; i++) b.push_back(uint8_t(g()));                    // rumore

        Overview o;
        o.compute(b);
        const auto& c = o.cells();
        assert((int)c.size() == Overview::COLS);
        assert(o.cellStart(0) == 0 && o.cellStart(c.size() - 1) < b.size());
        assert(o.cellOf(0) == 0 && o.cellOf(b.size() - 1) == c.size() - 1);
        assert(o.cellOf(part + part / 2) == c.size() / 2);

        const auto& text = c[c.size() / 6];
        const auto& zero = c[c.size() / 2];
        const auto& rand = c[c.size() * 5 / 6];
        assert(text.text > 0.99f && text.zeros == 0.0f && text.periodLen == 9);
        assert(zero.zeros == 1.0f && zero.entropy == 0.0f && zero.periodLen == 0);
        assert(rand.entropy > 0.9f && rand.text < 0.6f && rand.lowBits == 0.0f);

        // I salti di entropia stanno ai confini, non dentro i pezzi
        float inside = 0, edge = 0;
        for (size_t i = 1; i < c.size(); i++) {
            const bool nearEdge = (i % (c.size() / 3)) <= 1 || (i % (c.size() / 3)) >= c.size() / 3 - 1;
            (nearEdge ? edge : inside) = std::max(nearEdge ? edge : inside, c[i].jump);
        }
        assert(edge > 0.9f && inside < 0.3f);
    }

    // --- Dal tratto alla posizione e ritorno: sempre lo stesso tratto ---
    {
        for (size_t n : {size_t(1), size_t(7), size_t(255), size_t(257), size_t(1000),
                         size_t(163957), size_t(1) << 33})
            for (size_t parts : {size_t(1), size_t(3), size_t(256), size_t(624)}) {
                if (parts > n) continue;
                for (size_t c = 0; c < parts; c += std::max<size_t>(1, parts / 97)) {
                    const size_t start = c * n / parts, end = (c + 1) * n / parts;
                    assert(partOf(start, n, parts) == c);
                    assert(partOf(end - 1, n, parts) == c);
                }
                assert(partOf(n - 1, n, parts) == parts - 1);
                assert(partOf(n + 5, n, parts) == parts - 1);      // oltre la fine: l'ultimo
            }
        assert(partOf(0, 0, 10) == 0 && partOf(3, 10, 0) == 0);
        // Dot Plot, Entropia, Somiglianza: colonna = i * 256 / n, inizio = columnStart
        for (size_t n : {size_t(300), size_t(1000), size_t(163957)})
            for (int x = 0; x < 256; x++) {
                const size_t start = columnStart(x, n);
                if (start >= n || columnStart(x + 1, n) == start) continue;   // colonna vuota
                assert(start * 256 / n == (size_t)x);
            }
    }

    // --- File minuscoli: mai piu' tratti che byte ---
    {
        Overview o;
        o.compute({});
        assert(o.cells().empty() && o.cellStart(0) == 0 && o.cellOf(5) == 0);
        for (size_t n : {size_t(1), size_t(2), size_t(17), size_t(623), size_t(625)}) {
            std::vector<uint8_t> b(n, 0x41);
            o.compute(b);
            assert(o.cells().size() == std::min<size_t>(n, Overview::COLS));
            for (const auto& cell : o.cells()) {
                assert(cell.entropy >= 0.0f && cell.entropy <= 1.0f);
                assert(cell.text == 1.0f && cell.lowBits == 0.0f);
            }
        }
    }

    // --- Annullata: nessun risultato a meta' ---
    {
        std::vector<uint8_t> b(1 << 20, 7);
        std::atomic<bool> cancel{true};
        const Cancel::Scope scope(&cancel);
        Overview o;
        o.compute(b);
        assert(o.cells().empty());
    }
}
