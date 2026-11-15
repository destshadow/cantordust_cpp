#include "tests.h"

static void putStr(std::vector<uint8_t>& b, size_t at, const std::string& s) {
    for (size_t i = 0; i < s.size(); i++) b.at(at + i) = (uint8_t)s[i];
}

void testSignatures() {
    using Signatures::scan;

    // --- Firme lunghe: trovate all'inizio e in mezzo, in ordine ---
    {
        std::vector<uint8_t> b(4096, 0xEE);
        putStr(b, 0, std::string("\x89PNG\r\n\x1a\n", 8));
        putStr(b, 1000, "%PDF-1.7");
        putStr(b, 2000, std::string("SQLite format 3\0", 16));
        putStr(b, 4090, "GIF89a");                 // proprio alla fine
        auto h = scan(b);
        assert(h.size() == 4);
        assert(h[0].offset == 0 && std::string(h[0].name) == "PNG");
        assert(h[1].offset == 1000 && std::string(h[1].name) == "PDF");
        assert(h[2].offset == 2000 && h[3].offset == 4090);

        assert(Signatures::next(h, 0) == 1);
        assert(Signatures::next(h, 4090) == SIZE_MAX);
        assert(Signatures::prev(h, 1000) == 0);
        assert(Signatures::prev(h, 0) == SIZE_MAX);
        assert(Signatures::next({}, 0) == SIZE_MAX);
    }

    // --- Firme corte: servono anche i campi dell'intestazione ---
    {
        std::vector<uint8_t> b(2048, 0);
        putStr(b, 100, "MZ");                      // "MZ" da solo non basta
        putStr(b, 300, "BM");
        assert(scan(b).empty());

        putStr(b, 500, "MZ");                      // PE vero: porta a "PE\0\0"
        put(b, 500 + 0x3C, 0x80, 4);
        putStr(b, 500 + 0x80, "PE");
        auto h = scan(b);
        assert(h.size() == 1 && h[0].offset == 500 && std::string(h[0].name) == "PE");

        std::vector<uint8_t> g(64, 0xEE);          // gzip: flag e sistema validi
        const uint8_t gz[10] = {0x1F, 0x8B, 8, 0, 0, 0, 0, 0, 0, 3};
        std::copy(gz, gz + 10, g.begin() + 5);
        assert(scan(g).size() == 1 && scan(g)[0].offset == 5);
        g[5 + 3] = 0xFF;                           // flag impossibili
        assert(scan(g).empty());
    }

    // --- Il marcatore non e' all'inizio del file: tar (257) e MP4 (4) ---
    {
        std::vector<uint8_t> b(2048, 0);
        putStr(b, 600 + 257, "ustar");
        const uint8_t mp4[12] = {0, 0, 0, 0x18, 'f', 't', 'y', 'p', 'i', 's', 'o', 'm'};
        std::copy(mp4, mp4 + 12, b.begin() + 100);
        auto h = scan(b);
        assert(h.size() == 2);
        assert(h[0].offset == 100 && std::string(h[0].name) == "MP4");
        assert(h[1].offset == 600 && std::string(h[1].name) == "tar");

        std::vector<uint8_t> early(300, 0);        // "ustar" troppo presto
        putStr(early, 10, "ustar");
        assert(scan(early).empty());
    }

    // --- Mai fuori dai byte: intestazioni troncate e file minuscoli ---
    {
        assert(scan({}).empty());
        for (const char* m : {"MZ", "BM", "\x7F" "ELF", "PK\x03\x04", "RIFF", "II*", "\xFF\xD8\xFF"}) {
            std::string s(m);
            std::vector<uint8_t> b(s.begin(), s.end());
            scan(b);                                // non deve leggere oltre
            b.insert(b.begin(), 3, 0);
            scan(b);
        }
        std::mt19937 gen(7);                        // rumore: pochissimi falsi positivi
        std::vector<uint8_t> noise(4 << 20);
        for (auto& x : noise) x = uint8_t(gen());
        assert(scan(noise).size() <= 2);
    }

    // --- Raw e Metric Map: dal byte al pixel e ritorno ---
    {
        std::vector<uint8_t> b(1000);
        for (size_t i = 0; i < b.size(); i++) b[i] = uint8_t(i);
        RawPixels r;
        r.compute(b, BppMode::BPP_24, 10, 4);
        int x = -1, y = -1;
        assert(!r.pixelOf(3, x, y));                // prima dell'offset
        assert(r.pixelOf(4, x, y) && x == 0 && y == 0);
        assert(r.pixelOf(4 + 3 * 13 + 2, x, y) && x == 3 && y == 1);
        assert(r.byteOffsetAt(x, y) == 4 + 3 * 13);
        assert(!r.pixelOf(5000, x, y));

        MetricMap m;
        m.compute(b, ColorMode::ENTROPY, CurveMode::HILBERT, 8, nullptr);
        for (size_t off : {size_t(0), size_t(500), size_t(999)}) {
            assert(m.pixelOf(off, x, y));
            assert(m.fileIndexAt(x, y) <= off);
        }
        MetricMap empty;
        assert(!empty.pixelOf(0, x, y));
    }
}
