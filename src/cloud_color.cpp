#include "cloud_color.h"
#include "byte_info.h"

namespace CloudColor {

const char* name(Mode m) {
    switch (m) {
        case Mode::SECTION:    return "sezione";
        case Mode::BYTE_CLASS: return "tipo di byte";
        case Mode::FREQUENCY:  return "frequenza";
    }
    return "?";
}

Mode next(Mode m, bool hasSections) {
    switch (m) {
        case Mode::SECTION:    return Mode::BYTE_CLASS;
        case Mode::BYTE_CLASS: return Mode::FREQUENCY;
        case Mode::FREQUENCY:  return hasSections ? Mode::SECTION : Mode::BYTE_CLASS;
    }
    return Mode::BYTE_CLASS;
}

void mixedColor(uint8_t& r, uint8_t& g, uint8_t& b) {
    r = 170; g = 110; b = 210;
}

void classColor(uint32_t key, uint8_t& r, uint8_t& g, uint8_t& b) {
    using namespace ByteInfo;
    const Class c0 = classOf((key >> 16) & 0xFF);
    const Class c1 = classOf((key >>  8) & 0xFF);
    const Class c2 = classOf( key        & 0xFF);
    Class major;
    if (c0 == c1 || c0 == c2) major = c0;
    else if (c1 == c2)        major = c1;
    else { mixedColor(r, g, b); return; }
    ByteInfo::classColor(major, r, g, b);
    if (major == Class::ZERO) r = g = b = 120;   // il nero sparirebbe sul fondo
}

// Blu -> magenta -> giallo
void frequencyColor(uint8_t level, uint8_t& r, uint8_t& g, uint8_t& b) {
    if (level < 128) {
        int t = level * 2;
        r = (uint8_t)(50 + t * 190 / 255);
        g = 60;
        b = 255;
    } else {
        int t = (level - 128) * 2;
        r = 240;
        g = (uint8_t)(60 + t * 180 / 255);
        b = (uint8_t)(255 - t * 200 / 255);
    }
}

void paint(std::vector<Point3D>& points, Mode mode,
           const std::vector<BinarySection>& sections) {
    for (Point3D& p : points) {
        if (mode == Mode::BYTE_CLASS) {
            classColor(p.key, p.r, p.g, p.b);
        } else if (mode == Mode::FREQUENCY) {
            frequencyColor(p.level, p.r, p.g, p.b);
        } else if (p.section >= 0 && (size_t)p.section < sections.size()) {
            const BinarySection& s = sections[p.section];
            p.r = s.r; p.g = s.g; p.b = s.b;
        } else {
            p.r = p.g = p.b = 90;                 // fuori dalle sezioni
        }
    }
}

} // namespace CloudColor
