#include "tests.h"

void testBytes() {
    // --- ByteCloud: conta tutta la finestra, 00/FF fuori scala, testo nella cella ---
    {
        std::vector<uint8_t> b(30000, 'a');
        b[29999] = 'z';                         // oltre i vecchi 9999 byte
        b[0] = 0x00;
        ByteCloud c;
        c.compute(b);
        assert(c.getFreq('a') == 1.0f);
        assert(c.getFreq('z') > 0.0f);
        assert(c.getFreq(0x00) > 0.0f && c.getFreq('b') == 0.0f);

        std::vector<uint8_t> zeros(1000, 0x00);  // 00 domina ma non cambia la scala
        zeros[0] = 'a';
        zeros[1] = 'a';
        zeros[2] = 'b';
        c.compute(zeros);
        assert(c.getFreq('a') == 1.0f && c.getFreq('b') == 0.5f);
        assert(c.getFreq(0x00) == 1.0f);         // saturo, ma visibile

        c.compute({});
        assert(c.toRGB().size() == (size_t)ByteCloud::SIZE * ByteCloud::SIZE * 3);

        // Tutti i valori alla grandezza massima: nessun pixel sul bordo di una cella
        std::vector<uint8_t> all;
        for (int v = 0; v < 256; v++) all.push_back(uint8_t(v));
        c.compute(all);
        const auto rgb = c.toRGB();
        const int C = ByteCloud::CELL;
        for (int y = 0; y < ByteCloud::SIZE; y++)
            for (int x = 0; x < ByteCloud::SIZE; x++)
                if (x % C == 0 || y % C == 0 || x % C == C - 1 || y % C == C - 1)
                    assert(rgb[((size_t)y * ByteCloud::SIZE + x) * 3 + 1] == 0);

        assert(ByteCloud::valueAt(0, 0) == 0x00);
        assert(ByteCloud::valueAt(ByteCloud::SIZE - 1, ByteCloud::SIZE - 1) == 0xFF);
        assert(ByteCloud::valueAt(C, 2 * C) == 0x21);
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
}
