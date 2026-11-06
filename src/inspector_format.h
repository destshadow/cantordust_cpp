#pragma once
#include "byte_info.h"
#include "ui_widgets.h"
#include <iomanip>
#include <sstream>
#include <string>

// Formattazione condivisa dai pannelli dell'Inspector
namespace Inspect {

inline std::string hex(size_t v, int digits) {
    std::ostringstream o;
    o << std::uppercase << std::hex << std::setw(digits) << std::setfill('0') << v;
    return o.str();
}

inline std::string sizeStr(size_t sz) {
    if (sz >= 1024 * 1024) return std::to_string(sz / (1024 * 1024)) + " MB";
    if (sz >= 1024)        return std::to_string(sz / 1024) + " KB";
    return std::to_string(sz) + " B";
}

inline std::string byteDesc(uint8_t b) {
    std::string s = "0x" + hex(b, 2);
    if (b >= 0x20 && b < 0x7F) s += std::string(" '") + (char)b + "'";
    s += std::string("  ") + ByteInfo::className(ByteInfo::classOf(b));
    return s;
}

inline std::string pct(double f) {
    std::ostringstream o;
    o << std::fixed << std::setprecision(f < 0.01 ? 3 : 1) << f * 100 << "%";
    return o.str();
}

inline olc::Pixel classPixel(ByteInfo::Class c) {
    uint8_t r = 0, g = 0, b = 0;
    ByteInfo::classColor(c, r, g, b);
    if (c == ByteInfo::Class::ZERO) r = g = b = 110;   // grigio leggibile su sfondo scuro
    return olc::Pixel(r, g, b);
}

// Composizione + entropia + ipotesi
inline void writeStats(PanelWriter& pw, const ByteInfo::Stats& s) {
    using ByteInfo::Class;
    pw.bar("zero",        s.frac[(int)Class::ZERO],    classPixel(Class::ZERO));
    pw.bar("controllo",   s.frac[(int)Class::CONTROL], classPixel(Class::CONTROL));
    pw.bar("testo",       s.frac[(int)Class::TEXT],    classPixel(Class::TEXT));
    pw.bar("alti >0x7F",  s.frac[(int)Class::HIGH],    classPixel(Class::HIGH));
    pw.bar("0xFF",        s.frac[(int)Class::FF],      classPixel(Class::FF));
    std::ostringstream e;
    e << std::fixed << std::setprecision(2) << s.entropy << " / 8 bit";
    pw.field("ENTROPIA", e.str());
    pw.field("IPOTESI", ByteInfo::guess(s), COL_TEXT());
}

} // namespace Inspect
