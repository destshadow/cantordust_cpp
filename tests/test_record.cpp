#include "tests.h"
#include "record_profile.h"

namespace {

using Field = RecordProfile::Field;

// Record da 24 byte: AA 55 | contatore | 3 casuali | 6 lettere | flag |
// 60 valori sparsi | 10 zeri
std::vector<uint8_t> sample(size_t count, std::mt19937& g) {
    std::vector<uint8_t> b;
    for (size_t i = 0; i < count; i++) {
        b.push_back(0xAA); b.push_back(0x55);
        b.push_back(uint8_t(i));
        for (int k = 0; k < 3; k++) b.push_back(uint8_t(g()));
        for (int k = 0; k < 6; k++) b.push_back(uint8_t('a' + g() % 26));
        b.push_back(uint8_t(1u << (g() % 4)));
        b.push_back(uint8_t(100 + g() % 60));
        b.insert(b.end(), 10, 0);
    }
    return b;
}

} // namespace

void testRecordProfile() {
    {   // Periodo e inizio giusti: ogni campo e' riconosciuto
        std::mt19937 g(1);
        const auto b = sample(5000, g);
        RecordProfile p;
        p.compute(b, 24, 0);
        assert(p.records() == 5000 && p.sampled() == 5000 && p.period() == 24);
        assert(p.column(0).field == Field::FIXED && p.column(0).top == 0xAA);
        assert(p.column(1).field == Field::FIXED && p.column(1).top == 0x55);
        assert(p.column(2).field == Field::COUNTER && p.column(2).step == 1);
        for (int j = 3; j < 6; j++) assert(p.column(j).field == Field::RANDOM);
        for (int j = 6; j < 12; j++) assert(p.column(j).field == Field::TEXT);
        assert(p.column(12).field == Field::FEW && p.column(12).distinct == 4);
        assert(p.column(13).field == Field::VARIED && p.column(13).distinct == 60);
        for (int j = 14; j < 24; j++) assert(p.column(j).field == Field::FIXED && p.column(j).top == 0);
        assert(p.count(0, 0xAA) == 5000 && p.count(0, 0x55) == 0 && p.count(99, 0) == 0);
        assert(p.pattern(4) == "AA55C ? ");
        assert(p.column(-1).field == Field::NONE && p.column(24).field == Field::NONE);

        // Inizio spostato di uno: tutto scorre di una colonna
        p.compute(b, 24, 1);
        assert(p.phase() == 1 && p.column(0).top == 0x55 && p.column(1).field == Field::COUNTER);
        assert(p.column(23).field == Field::FIXED && p.column(23).top == 0xAA);
        p.compute(b, 24, -1);                          // -1 = 23
        assert(p.phase() == 23 && p.column(1).top == 0xAA);

        // Periodo sbagliato: i campi fissi spariscono quasi tutti
        p.compute(b, 25, 0);
        int fixed = 0;
        for (int j = 0; j < 25; j++) fixed += p.column(j).field == Field::FIXED;
        assert(fixed == 0);
    }
    {   // Pochi valori stampabili (il byte alto di un float: E F G) non sono testo
        std::vector<uint32_t> values(256, 0), deltas(256, 0);
        values['E'] = 400; values['F'] = 350; values['G'] = 250;
        deltas[0] = 900; deltas[1] = 99;
        assert(RecordProfile::classify(values.data(), deltas.data(), 1000).field == Field::FEW);
    }
    {   // Tanti record: si campiona, ma un contatore resta un contatore
        std::mt19937 g(2);
        const auto b = sample(200000, g);
        RecordProfile p;
        p.compute(b, 24, 0);
        assert(p.records() == 200000 && p.sampled() <= RecordProfile::MAX_RECORDS);
        assert(p.sampled() > RecordProfile::MAX_RECORDS / 2);
        assert(p.column(2).field == Field::COUNTER && p.column(2).step == 4);   // un record ogni 4
        assert(p.column(0).field == Field::FIXED);
    }
    {   // Bordi
        RecordProfile p;
        p.compute({}, 24, 0);
        assert(p.empty() && p.period() == 24 && p.toRGB().size() == 256u * 256 * 3);
        p.compute(std::vector<uint8_t>(30, 1), 24, 10);  // non ci sta un record intero
        assert(p.empty() && p.column(0).field == Field::NONE);
        p.compute(std::vector<uint8_t>(30, 1), 0, 0);    // periodo minimo 1
        assert(p.period() == 1 && p.records() == 30 && p.column(0).field == Field::FIXED);
        p.compute(std::vector<uint8_t>(30, 1), 100000, 0);
        assert(p.period() == RecordProfile::MAX_PERIOD && p.empty());
        p.compute(std::vector<uint8_t>(24, 7), 24, 0);   // un record solo: nessun tipo
        assert(p.records() == 1 && p.column(0).field == Field::NONE);
        assert(p.pattern(2) == "- - ");
    }
    {   // Pixel ↔ byte del record, record corti e lunghi
        for (int period : {1, 3, 24, 255, 256, 300, 4096}) {
            RecordProfile p;
            p.compute(std::vector<uint8_t>(size_t(period) * 2, 0), period, 0);
            for (int x = 0; x < RecordProfile::SIZE; x++) {
                const int j = p.byteAt(x);
                assert(j >= 0 && j < period);
                assert(p.firstX(j) <= x);
            }
            if (period <= 256)
                for (int j = 0; j < period; j++) assert(p.byteAt(p.firstX(j)) == j);
            assert(p.byteAt(-1) == -1 && p.byteAt(RecordProfile::SIZE) == -1);
        }
    }
    {   // Immagine: il campo fisso e' un punto bianco, il resto della colonna nero
        std::mt19937 g(3);
        RecordProfile p;
        p.compute(sample(1000, g), 24, 0);
        const auto rgb = p.toRGB();
        const size_t at = (size_t(0xAA) * 256 + 0) * 3;           // x = 0, y = 0xAA
        assert(rgb[at] == 230 && rgb[at + 1] == 230 && rgb[at + 2] == 230);
        assert(rgb[(size_t(0x10) * 256 + 0) * 3] == 0);
    }
    {   // Il calcolo guarda la bandierina
        std::atomic<bool> flag{false};
        Cancel::Scope scope(&flag);
        const size_t before = Cancel::polls;
        RecordProfile p;
        p.compute(std::vector<uint8_t>(1 << 22, 1), 16, 0);
        assert(Cancel::polls - before >= 60);
    }
}
