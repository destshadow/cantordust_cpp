#include "raw_pixels.h"
#include <algorithm>
#include <cstring>

void RawPixels::clear() {
    m_rgb.assign(256 * 256 * 3, 0);
}

// --- Decodifica 8bpp ---
// 1 byte = 1 pixel
// Interpretiamo come valore di luminosita' verde
// (come CantorDust originale e come il DotPlot)
// Byte 0x00 = nero, 0xFF = verde brillante
void RawPixels::decodePixel8(const std::vector<uint8_t>& b,
                              size_t idx,
                              uint8_t& r, uint8_t& g, uint8_t& bu) {
    if (idx >= b.size()) { r = g = bu = 0; return; }
    uint8_t v = b[idx];
    r  = 0;
    g  = v;   // canale verde — come CantorDust originale
    bu = 0;
}

// --- Decodifica 16bpp RGB565 ---
// 2 bytes = 1 pixel
// Formato: RRRRRGGGGGGBBBBB (5 bit R, 6 bit G, 5 bit B)
// Molto usato in firmware embedded, display LCD, texture compresse
void RawPixels::decodePixel16(const std::vector<uint8_t>& b,
                               size_t idx,
                               uint8_t& r, uint8_t& g, uint8_t& bu) {
    if (idx + 1 >= b.size()) { r = g = bu = 0; return; }

    // Little-endian: byte basso prima
    uint16_t pixel = (uint16_t)b[idx] | ((uint16_t)b[idx+1] << 8);

    // Estrai i 3 canali e scala a 8 bit
    // R: bit 11-15 (5 bit) → scala moltiplicando per 8
    // G: bit 5-10  (6 bit) → scala moltiplicando per 4
    // B: bit 0-4   (5 bit) → scala moltiplicando per 8
    r  = ((pixel >> 11) & 0x1F) << 3;  // 5 bit → 8 bit
    g  = ((pixel >>  5) & 0x3F) << 2;  // 6 bit → 8 bit
    bu =  (pixel        & 0x1F) << 3;  // 5 bit → 8 bit
}

// --- Decodifica 24bpp RGB ---
// 3 bytes = 1 pixel (R, G, B)
// Formato standard per immagini non compresse
// Molto usato in BMP, raw capture, texture non compresse
void RawPixels::decodePixel24(const std::vector<uint8_t>& b,
                               size_t idx,
                               uint8_t& r, uint8_t& g, uint8_t& bu) {
    if (idx + 2 >= b.size()) { r = g = bu = 0; return; }
    r  = b[idx];
    g  = b[idx+1];
    bu = b[idx+2];
}

// --- Decodifica 32bpp RGBA ---
// 4 bytes = 1 pixel (R, G, B, A)
// Formato con canale alpha — usiamo solo RGB e ignoriamo A
// Molto usato in PNG, texture DX/GL, icone
void RawPixels::decodePixel32(const std::vector<uint8_t>& b,
                               size_t idx,
                               uint8_t& r, uint8_t& g, uint8_t& bu) {
    if (idx + 3 >= b.size()) { r = g = bu = 0; return; }
    r  = b[idx];
    g  = b[idx+1];
    bu = b[idx+2];
    // b[idx+3] = alpha → ignorato per ora
}

// --- Decodifica 64bpp ---
// 8 bytes = 1 pixel — come Color64bpp di binvis/CantorDust
// Prende 8 byte, li assembla in un uint64_t little-endian
// poi usa i 32 bit bassi come colore ARGB (come java.awt.Color)
// Utile per file con strutture dati a 64 bit (double, int64, ptr)
void RawPixels::decodePixel64(const std::vector<uint8_t>& b,
                               size_t idx,
                               uint8_t& r, uint8_t& g, uint8_t& bu) {
    if (idx + 7 >= b.size()) { r = g = bu = 0; return; }

    // Assembla 8 byte in uint64_t little-endian
    uint64_t pixel64 = 0;
    for (int i = 0; i < 8; i++)
        pixel64 |= ((uint64_t)b[idx+i] << (i*8));

    // Prendi i 32 bit bassi come ARGB (come Java Color)
    // & 0xFFFFFFFF per sicurezza
    uint32_t pixel32 = (uint32_t)(pixel64 & 0xFFFFFFFF);

    // Estrai R, G, B dai bit 16-23, 8-15, 0-7
    r  = (pixel32 >> 16) & 0xFF;
    g  = (pixel32 >>  8) & 0xFF;
    bu =  pixel32        & 0xFF;
}

// --- Compute principale ---
// Scorre i byte del file con stride = bytesPerPixel
// e riempie il canvas 256x256
void RawPixels::compute(const std::vector<uint8_t>& bytes,
                         BppMode mode, int width) {
    m_mode  = mode;
    m_width = std::clamp(width, 64, 1024);
    m_rgb.assign(256 * 256 * 3, 0);

    if (bytes.empty()) return;

    // Bytes per pixel in base alla modalita'
    size_t bpp;
    switch (mode) {
        case BppMode::BPP_8:  bpp = 1; break;
        case BppMode::BPP_16: bpp = 2; break;
        case BppMode::BPP_24: bpp = 3; break;
        case BppMode::BPP_32: bpp = 4; break;
        case BppMode::BPP_64: bpp = 8; break;
        default:              bpp = 1; break;
    }

    // Quanti pixel entrano nel canvas 256x256?
    size_t totalPixels = 256 * 256;

    for (size_t px = 0; px < totalPixels; px++) {
        size_t byteIdx = px * bpp;
        if (byteIdx >= bytes.size()) break;

        uint8_t r = 0, g = 0, b = 0;
        switch (mode) {
            case BppMode::BPP_8:
                decodePixel8 (bytes, byteIdx, r, g, b); break;
            case BppMode::BPP_16:
                decodePixel16(bytes, byteIdx, r, g, b); break;
            case BppMode::BPP_24:
                decodePixel24(bytes, byteIdx, r, g, b); break;
            case BppMode::BPP_32:
                decodePixel32(bytes, byteIdx, r, g, b); break;
            case BppMode::BPP_64:
                decodePixel64(bytes, byteIdx, r, g, b); break;
        }

        // Scrivi nel canvas — layout row-major
        int canvasX = (int)(px % 256);
        int canvasY = (int)(px / 256);
        int idx     = (canvasY * 256 + canvasX) * 3;

        if (idx + 2 < (int)m_rgb.size()) {
            m_rgb[idx]     = r;
            m_rgb[idx + 1] = g;
            m_rgb[idx + 2] = b;
        }
    }
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
