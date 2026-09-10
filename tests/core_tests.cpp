#include "file_navigator.h"
#include "ngram_model.h"
#include "classifier_model.h"
#include "binary_format.h"
#include "binary_reader.h"
#include "metric_map.h"
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
    std::cout << "All core regression tests passed\n";
}
