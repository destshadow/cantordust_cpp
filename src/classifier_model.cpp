#include "classifier_model.h"
#include "template_file.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace {

// Le prime 22 sono le classi di Battelle, con i nomi dei loro file. Le ultime
// cinque (gli script) sono nostre: i campioni li fa tools/make_templates.sh.
// Colori a famiglie: codice macchina caldo, bytecode verde, testo blu,
// script azzurro-verdi, compresso e immagini viola, zeri e 0xFF grigi.
const ContentClass CLASSES[] = {
    {"arm4",           "ARM (arm4)",     255, 200,  40},
    {"arm7",           "ARM (arm7)",     230, 160,  30},
    {"ascii_english",  "testo ASCII",     90, 160, 255},
    {"compressed",     "compresso",      160,  80, 220, true},
    {"java",           "bytecode Java",  120, 220,  80},
    {"mips",           "MIPS",           220,  90, 140},
    {"msil",           ".NET MSIL",       60, 170,  90},
    {"ones",           "0xFF",           235, 235, 235},
    {"png",            "dati PNG",       200, 110, 255, true},
    {"powerpc",        "PowerPC",        200,  60,  90},
    {"sparc_32",       "SPARC",          180, 100,  40},
    {"utf_16_english", "testo UTF-16",    60, 100, 220},
    {"x64",            "x86-64",         255, 130,  40},
    {"x86",            "x86",            255,  80,  60},
    {"x86_padding",    "riempimento x86",150,  90,  60},
    {"zeros",          "zeri",            70,  70,  70},
    {"embedded_image", "immagine",       230, 150, 255},
    {"arm64",          "ARM64",          255, 235, 100},
    {"riscv64",        "RISC-V 64",      255, 170, 130},
    {"s390x",          "s390x",          190, 140, 100},
    {"ppc64el",        "PowerPC 64",     230,  90, 130},
    {"mips64el",       "MIPS 64",        240, 130, 180},
    {"javascript",     "JavaScript",      60, 220, 220},
    {"python",         "Python",         110, 240, 190},
    {"shell",          "script shell",   160, 230, 255},
    {"html",           "HTML",            70, 190, 170},
    {"php",            "PHP",            140, 170, 240},
};
constexpr int COUNT = int(sizeof(CLASSES) / sizeof(CLASSES[0]));
const ContentClass UNKNOWN = {"", "?", 128, 128, 128};

// Aggiunge al modello al massimo `budget` byte di un campione: tutto se e'
// piccolo, altrimenti pezzi da CHUNK distribuiti su tutto il campione (un
// campione grande e' spesso fatto di parti diverse, l'inizio non basta).
// Restituisce i byte usati.
size_t addBytes(NGramModel& m, int cls, const std::vector<uint8_t>& b, size_t budget) {
    const size_t take = std::min(b.size(), budget);
    if (take < (size_t)NGramModel::N) return 0;
    const size_t chunk  = std::min(take, ClassifierModel::CHUNK);
    const size_t pieces = take / chunk;
    for (size_t i = 0; i < pieces; i++) {
        const size_t at = pieces == 1 ? 0 : (b.size() - chunk) / (pieces - 1) * i;
        m.add(cls, &b[at], chunk);
    }
    return pieces * chunk;
}

// Un file di templates/: campione di byte o modello gia' fatto (template_file.h)
size_t addFile(NGramModel& m, int cls, const std::string& path, size_t budget) {
    std::error_code ec;                 // una cartella con quel nome: leggerla lancia
    if (!std::filesystem::is_regular_file(path, ec)) return 0;
    std::ifstream f(path, std::ios::binary);
    if (!f || budget == 0) return 0;
    const std::vector<uint8_t> raw((std::istreambuf_iterator<char>(f)), {});
    const TemplateFile::Content c = TemplateFile::parse(raw, CLASSES[cls].file);
    if (!c.grams.empty()) {
        // Un modello vale come un campione da `budget` grammi
        for (const auto& g : c.grams) m.addGram(cls, g.first, g.second * double(budget));
        return budget;
    }
    return addBytes(m, cls, c.bytes, budget);
}

// Entropia dei byte di [p, p+n), in bit
double entropyBits(const uint8_t* p, size_t n) {
    uint32_t f[256] = {};
    for (size_t i = 0; i < n; i++) f[p[i]]++;
    double h = 0;
    for (uint32_t c : f)
        if (c) { const double q = double(c) / double(n); h -= q * std::log2(q); }
    return h;
}

} // namespace

int ClassifierModel::count() { return COUNT; }

const ContentClass& ClassifierModel::info(int cls) {
    return cls >= 0 && cls < COUNT ? CLASSES[cls] : UNKNOWN;
}

int ClassifierModel::loadTemplates(const std::string& dir) {
    NGramModel model(COUNT);
    int found = 0;
    for (int c = 0; c < COUNT; c++) {
        size_t used = 0;
        for (const char* ext : {".template", ".bin"})
            if (used < MAX_SAMPLE)
                used += addFile(model, c, dir + "/" + CLASSES[c].file + ext, MAX_SAMPLE - used);
        found += used > 0;
    }
    if (found == 0) return 0;
    model.finish();
    m_model = std::move(model);
    m_ready.store(true, std::memory_order_release);
    std::cout << "[Classifier] " << found << "/" << COUNT << " classi con un campione\n";
    return found;
}

void ClassifierModel::train(const std::vector<std::vector<uint8_t>>& samples) {
    NGramModel model(COUNT);
    for (size_t c = 0; c < samples.size() && c < size_t(COUNT); c++)
        model.add(int(c), samples[c].data(), samples[c].size());
    model.finish();
    m_model = std::move(model);
    m_ready.store(true, std::memory_order_release);
}

int ClassifierModel::classAt(const uint8_t* data, size_t size, size_t at) const {
    if (!isReady() || size < (size_t)NGramModel::N) return -1;
    // CONTEXT byte centrati sul punto, spostati dentro i limiti ai bordi
    const size_t ctx = std::min(CONTEXT, size);
    size_t low = at > ctx / 2 ? at - ctx / 2 : 0;
    low = std::min(low, size - ctx);

    thread_local std::vector<double> score;
    const int best = m_model.best(data + low, ctx, &score);
    if (best < 0 || !CLASSES[best].noise || entropyBits(data + low, ctx) >= NOISE_MIN_BITS)
        return best;
    int other = -1;                     // non sembra casuale: il migliore fra gli altri
    for (int c = 0; c < COUNT; c++)
        if (!CLASSES[c].noise && m_model.hasData(c) && (other < 0 || score[c] > score[other]))
            other = c;
    return other;
}
