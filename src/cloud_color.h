#pragma once
#include "binary_format.h"
#include "trigraph.h"
#include <vector>

// Di che colore sono i punti della nuvola 3D
namespace CloudColor {

enum class Mode { SECTION, BYTE_CLASS, FREQUENCY };

const char* name(Mode m);

// Il modo dopo, saltando quelli che in questo file non hanno senso
Mode next(Mode m, bool hasSections);

// Colore di una tripla secondo le classi dei suoi byte. Una classe vale se
// riguarda almeno 2 byte su 3; altrimenti la tripla e' "mista".
void classColor(uint32_t key, uint8_t& r, uint8_t& g, uint8_t& b);
void mixedColor(uint8_t& r, uint8_t& g, uint8_t& b);

// Da raro (0) a frequente (255)
void frequencyColor(uint8_t level, uint8_t& r, uint8_t& g, uint8_t& b);

void paint(std::vector<Point3D>& points, Mode mode,
           const std::vector<BinarySection>& sections);

} // namespace CloudColor
