#include "tests.h"

void testBytes() {
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
