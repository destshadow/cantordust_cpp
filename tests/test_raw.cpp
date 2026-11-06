#include "tests.h"

void testRaw() {
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

    {   // RAW: filtri. Il byte d'immagine a 8 bpp e' il canale verde.
        std::vector<uint8_t> b = {10, 13, 13, 200, 0x81};
        RawPixels rp;
        auto green = [&](int x) { return rp.getRGB()[(size_t)x * 3 + 1]; };

        rp.compute(b, BppMode::BPP_8, 5, 0, RawFilter{1});   // delta
        assert(green(0) == 128);                  // nessun byte prima
        assert(green(1) == 131);                  // +3
        assert(green(2) == 128);                  // uguale al precedente
        assert(green(3) == (uint8_t)(187 + 128)); // differenza che va a capo

        RawFilter bit0{10}, bit7{3};
        assert(bit0.bit() == 0 && bit7.bit() == 7);
        rp.compute(b, BppMode::BPP_8, 5, 0, bit7);
        assert(green(3) == 255 && green(0) == 0 && green(4) == 255);
        rp.compute(b, BppMode::BPP_8, 5, 0, bit0);
        assert(green(0) == 0 && green(1) == 255 && green(4) == 255);

        // Delta riga: record identici diventano piatti, un campo diverso resta
        std::vector<uint8_t> rec = {1, 2, 3, 4,  1, 2, 3, 4,  1, 9, 3, 4};
        rp.compute(rec, BppMode::BPP_8, 4, 0, RawFilter{2});
        for (int x = 0; x < 4; x++) assert(rp.getRGB()[(4 + x) * 3 + 1] == 128);
        assert(rp.getRGB()[(8 + 1) * 3 + 1] == 128 + 7);

        // 24 bpp: il delta confronta lo stesso canale del pixel prima
        std::vector<uint8_t> rgb = {10, 20, 30,  11, 22, 33};
        rp.compute(rgb, BppMode::BPP_24, 2, 0, RawFilter{1});
        assert(rp.getRGB()[3] == 129 && rp.getRGB()[4] == 130 && rp.getRGB()[5] == 131);

        // Stessa finestra = stessa immagine, che i byte stiano in un buffer
        // piu' grande o siano una copia della finestra (thread di calcolo)
        std::vector<uint8_t> file = {99, 98, 97, 10, 13, 13, 200};
        std::vector<uint8_t> win(file.begin() + 3, file.end());
        RawPixels a, c;
        a.compute(file, 3, file.size(), BppMode::BPP_8, 4, 0, RawFilter{1});
        c.compute(win, BppMode::BPP_8, 4, 0, RawFilter{1});
        assert(a.getRGB() == c.getRGB());
        // L'offset del pixel non cambia con il filtro
        assert(a.byteOffsetAt(2, 0) == 2);

        RawFilter ring;
        for (int i = 0; i < RawFilter::COUNT; i++) ring = ring.next();
        assert(ring.none());
        assert(RawFilter{}.prev().id == 10 && RawFilter{10}.next().none());
    }
}
