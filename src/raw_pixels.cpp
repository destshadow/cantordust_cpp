#include "raw_pixels.h"
#include <algorithm>

void RawPixels::clear() {
    m_rgb.clear();
    m_width = m_height = 0;
    m_rowStep = 1;
    m_offset  = 0;
}

size_t RawPixels::bytesPerPixel(BppMode m) {
    switch (m) {
        case BppMode::BPP_8:  return 1;
        case BppMode::BPP_16: return 2;
        case BppMode::BPP_24: return 3;
        case BppMode::BPP_32: return 4;
        case BppMode::BPP_64: return 8;
    }
    return 1;
}

// Decodifica un pixel; p punta ad almeno bytesPerPixel(mode) byte validi.
void RawPixels::decodePixel(BppMode mode, const uint8_t* p,
                            uint8_t& r, uint8_t& g, uint8_t& b) {
    switch (mode) {
        case BppMode::BPP_8:
            // 1 byte = luminosita' verde, come il CantorDust originale
            r = 0; g = p[0]; b = 0;
            break;
        case BppMode::BPP_16: {
            // RGB565 little-endian: RRRRRGGGGGGBBBBB
            // (firmware embedded, display LCD, texture compresse)
            uint16_t px = (uint16_t)p[0] | ((uint16_t)p[1] << 8);
            r = ((px >> 11) & 0x1F) << 3;
            g = ((px >>  5) & 0x3F) << 2;
            b =  (px        & 0x1F) << 3;
            break;
        }
        case BppMode::BPP_24:
            // RGB standard (BMP, raw capture, texture non compresse)
            r = p[0]; g = p[1]; b = p[2];
            break;
        case BppMode::BPP_32:
            // RGBA: l'alpha (p[3]) viene ignorato
            r = p[0]; g = p[1]; b = p[2];
            break;
        case BppMode::BPP_64: {
            // Come Color64bpp di binvis: 32 bit bassi come ARGB
            // (utile per strutture a 64 bit: double, int64, puntatori)
            uint32_t px = (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
                          ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
            r = (px >> 16) & 0xFF;
            g = (px >>  8) & 0xFF;
            b =  px        & 0xFF;
            break;
        }
    }
}

// Riempie un'immagine width x height con i pixel di bytes[start+offset, end).
// Se le righe sono piu' di MAX_ROWS ne prende una ogni rowStep, cosi'
// l'intera finestra resta visibile (a risoluzione ridotta in verticale).
void RawPixels::compute(const std::vector<uint8_t>& bytes,
                        size_t start, size_t end,
                        BppMode mode, int width, size_t offset) {
    clear();
    m_mode  = mode;
    m_width = std::clamp(width, MIN_WIDTH, MAX_WIDTH);

    end = std::min(end, bytes.size());
    if (start >= end || offset >= end - start) { m_width = 0; return; }

    m_offset = offset;
    const size_t   bpp       = bytesPerPixel(mode);
    const uint8_t* data      = bytes.data() + start + offset;
    const size_t   len       = end - start - offset;
    const size_t   rowBytes  = (size_t)m_width * bpp;
    const size_t   totalRows = (len + rowBytes - 1) / rowBytes;

    m_rowStep = (totalRows + MAX_ROWS - 1) / MAX_ROWS;  // >= 1
    m_height  = (int)((totalRows + m_rowStep - 1) / m_rowStep);
    m_rgb.assign((size_t)m_width * m_height * 3, 0);

    for (int y = 0; y < m_height; y++) {
        size_t rowStart = (size_t)y * m_rowStep * rowBytes;
        for (int x = 0; x < m_width; x++) {
            size_t i = rowStart + (size_t)x * bpp;
            if (i + bpp > len) break;  // ultimo pixel incompleto: resta nero
            uint8_t r, g, b;
            decodePixel(mode, data + i, r, g, b);
            size_t o = ((size_t)y * m_width + x) * 3;
            m_rgb[o] = r; m_rgb[o + 1] = g; m_rgb[o + 2] = b;
        }
    }
}

size_t RawPixels::byteOffsetAt(int x, int y) const {
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) return SIZE_MAX;
    size_t bpp = bytesPerPixel(m_mode);
    return m_offset + ((size_t)y * m_rowStep * m_width + x) * bpp;
}

const char* RawPixels::modeName(BppMode m) {
    switch (m) {
        case BppMode::BPP_8:  return "8bpp";
        case BppMode::BPP_16: return "16bpp";
        case BppMode::BPP_24: return "24bpp";
        case BppMode::BPP_32: return "32bpp";
        case BppMode::BPP_64: return "64bpp";
    }
    return "???";
}
