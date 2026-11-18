#include "tests.h"
#include "template_file.h"
#include <filesystem>
#include <fstream>

namespace {

int classNamed(const char* file) {
    for (int c = 0; c < ClassifierModel::count(); c++)
        if (std::string(ClassifierModel::info(c).file) == file) return c;
    assert(false && "classe inesistente");
    return -1;
}

std::vector<uint8_t> text(size_t n) {
    const std::string s = "the quick brown fox jumps over the lazy dog and runs away. ";
    std::vector<uint8_t> b;
    while (b.size() < n) b.insert(b.end(), s.begin(), s.end());
    b.resize(n);
    return b;
}

void putLE32(std::vector<uint8_t>& b, uint32_t v) {
    for (int i = 0; i < 4; i++) b.push_back(uint8_t(v >> (8 * i)));
}

const uint8_t NET[] = {0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};

} // namespace

void testClassifier() {
    {   // NGramModel: ogni tratto va alla classe che lo ha visto
        NGramModel m(3);
        const auto t = text(4000);
        const std::vector<uint8_t> z(4000, 0);
        m.add(0, t.data(), t.size());
        m.add(1, z.data(), z.size());      // la 2 resta senza dati: non vince mai
        m.finish();
        assert(m.hasData(0) && m.hasData(1) && !m.hasData(2) && !m.hasData(7));
        assert(m.best(t.data() + 100, 64) == 0);
        assert(m.best(z.data(), 64) == 1);
        assert(m.best(t.data(), 3) == -1);                 // meno di un 4-gramma
        std::vector<double> score;
        assert(m.best(t.data(), 64, &score) == 0 && score.size() == 3 && score[0] > score[1]);
        // Un 4-gramma mai visto e' improbabile, non impossibile: punteggio finito
        const uint8_t odd[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x01};
        m.best(odd, 5, &score);
        assert(std::isfinite(score[0]) && std::isfinite(score[1]));
    }
    {   // Un campione che e' gia' un modello: gramma → probabilita'
        NGramModel m(2);
        const std::vector<uint8_t> z(1000, 0);
        m.add(0, z.data(), z.size());
        m.addGram(1, TemplateFile::gramKey((const uint8_t*)"ABCD"), 1000.0);
        m.addGram(1, 0, -5.0);                              // peso non valido: ignorato
        m.finish();
        const uint8_t abcd[] = {'A', 'B', 'C', 'D'};
        assert(m.best(abcd, 4) == 1 && m.best(z.data(), 8) == 0);
    }
    {   // I tre tipi di .template e i byte grezzi
        const std::vector<uint8_t> sample = text(3000);
        // 1. .NET con l'array "data" (record 0x0F, tipo 2 = Byte), poi il modello
        std::vector<uint8_t> f(NET, NET + sizeof NET);
        f.resize(200, 'x');
        f.push_back(0x0F); putLE32(f, 3); putLE32(f, uint32_t(sample.size())); f.push_back(2);
        f.insert(f.end(), sample.begin(), sample.end());
        f.insert(f.end(), 5000, 0x55);                     // il modello serializzato
        auto c = TemplateFile::parse(f, "x86");
        assert(c.bytes == sample && c.grams.empty());
        // ...con una lunghezza oltre la fine: non e' l'array giusto
        std::vector<uint8_t> bad(NET, NET + sizeof NET);
        bad.push_back(0x0F); putLE32(bad, 3); putLE32(bad, 999999); bad.push_back(2);
        assert(TemplateFile::parse(bad, "x86").bytes.empty());
        // 2. solo il modello: voci da 16 byte dopo "statisticalModel" e il nome
        std::vector<uint8_t> g(NET, NET + sizeof NET);
        const std::string head = "....statisticalModelarm64";
        g.insert(g.end(), head.begin(), head.end());
        for (int k = 0; k < 3; k++) {
            putLE32(g, 4);
            const uint8_t gram[4] = {uint8_t(k), 0x20, 0x03, 0xD5};
            g.insert(g.end(), gram, gram + 4);
            double p = 0.25 * (k + 1);
            const uint8_t* pp = (const uint8_t*)&p;
            g.insert(g.end(), pp, pp + 8);
        }
        g.insert(g.end(), {1, 2, 3});                      // coda troncata
        c = TemplateFile::parse(g, "arm64");
        assert(c.bytes.empty() && c.grams.size() == 3);
        assert(c.grams[1].first == 0x012003D5u && c.grams[2].second == 0.75);
        assert(TemplateFile::parse(g, "mips").grams.empty());   // nome diverso
        // 3. byte grezzi; e i casi vuoti
        assert(TemplateFile::parse(sample, "x").bytes == sample);
        assert(TemplateFile::parse({}, "x").bytes.empty());
        assert(TemplateFile::parse(std::vector<uint8_t>(NET, NET + 4), "x").bytes.size() == 4);
    }
    {   // ClassifierModel: classi, bordi, colori
        const int zeros = classNamed("zeros"), ascii = classNamed("ascii_english");
        assert(ClassifierModel::info(-1).label == std::string("?"));
        assert(ClassifierModel::info(ClassifierModel::count()).label == std::string("?"));
        ClassifierModel cm;
        assert(!cm.isReady() && cm.classAt(nullptr, 0, 0) == -1);
        std::vector<std::vector<uint8_t>> samples(size_t(ClassifierModel::count()));
        samples[size_t(zeros)] = std::vector<uint8_t>(5000, 0);
        samples[size_t(ascii)] = text(5000);
        cm.train(samples);
        assert(cm.isReady() && cm.hasClass(zeros) && !cm.hasClass(classNamed("x86")));

        std::vector<uint8_t> mix = text(1000);
        mix.insert(mix.end(), 1000, 0);
        assert(cm.classAt(mix.data(), mix.size(), 0) == ascii);           // bordo sinistro
        assert(cm.classAt(mix.data(), mix.size(), 500) == ascii);
        assert(cm.classAt(mix.data(), mix.size(), 1500) == zeros);
        assert(cm.classAt(mix.data(), mix.size(), mix.size() - 1) == zeros);  // bordo destro
        assert(cm.classAt(mix.data(), mix.size(), mix.size() + 50) == zeros); // oltre: l'ultimo tratto
        assert(cm.classAt(mix.data(), 3, 0) == -1);
        assert(cm.classAt(mix.data(), 4, 0) == ascii);

        // Il campione "compresso" (quasi uniforme) non vince su un tratto che
        // non sembra casuale, anche se lo spiega meglio degli altri
        {
            std::mt19937 g(3);
            auto s2 = samples;
            s2[size_t(classNamed("compressed"))].resize(200000);
            for (auto& x : s2[size_t(classNamed("compressed"))]) x = uint8_t(g());
            ClassifierModel withNoise;
            withNoise.train(s2);
            std::vector<uint8_t> odd;                         // cifre in UTF-16
            for (int k = 0; k < 64; k++) { odd.push_back(uint8_t('0' + g() % 10)); odd.push_back(0); }
            assert(withNoise.classAt(odd.data(), odd.size(), 64) != classNamed("compressed"));
            const auto& rnd = s2[size_t(classNamed("compressed"))];
            assert(withNoise.classAt(rnd.data(), rnd.size(), 5000) == classNamed("compressed"));
        }

        // Metric Map: ogni pixel prende il colore della sua classe
        MetricMap map;
        map.compute(mix, ColorMode::CLASSIFIER, CurveMode::LINEAR, 8, &cm);
        const ContentClass& a = ClassifierModel::info(ascii);
        assert(map.getRGB()[0] == a.r && map.getRGB()[1] == a.g && map.getRGB()[2] == a.b);
        // Senza modello: grigio, nessun crash
        map.compute(mix, ColorMode::CLASSIFIER, CurveMode::LINEAR, 8, nullptr);
        assert(map.getRGB()[0] == 128);

        // Il calcolo guarda la bandierina
        std::atomic<bool> flag{false};
        Cancel::Scope scope(&flag);
        const size_t before = Cancel::polls;
        map.compute(std::vector<uint8_t>(1 << 20, 7), ColorMode::CLASSIFIER, CurveMode::HILBERT, 8, &cm);
        assert(Cancel::polls - before >= 64);
    }
    {   // loadTemplates da una cartella: .bin, .template, file mancanti
        namespace fs = std::filesystem;
        const fs::path dir = fs::temp_directory_path() /
                             ("cantordust_tpl_" + std::to_string(std::random_device{}()));
        fs::create_directories(dir);
        auto write = [&](const char* name, const std::vector<uint8_t>& b) {
            std::ofstream(dir / name, std::ios::binary).write((const char*)b.data(), (std::streamsize)b.size());
        };
        ClassifierModel empty;
        assert(empty.loadTemplates(dir.string()) == 0 && !empty.isReady());
        write("zeros.bin", std::vector<uint8_t>(10000, 0));
        write("ascii_english.template", text(300000));      // piu' di un pezzo da 64 KB
        write("x86.bin", {1, 2});                            // troppo corto: non conta
        fs::create_directories(dir / "mips.bin");            // una cartella: si salta
        ClassifierModel cm;
        assert(cm.loadTemplates(dir.string()) == 2 && cm.isReady());
        assert(!cm.hasClass(classNamed("x86")));
        const auto t = text(200);
        assert(cm.classAt(t.data(), t.size(), 100) == classNamed("ascii_english"));
        for (const char* n : {"zeros.bin", "ascii_english.template", "x86.bin", "mips.bin"}) fs::remove(dir / n);
        fs::remove(dir);
    }
}
