#include "visualizer.h"
#include "hex_layout.h"
#include "inspector_format.h"

// Vista Hex: i byte della finestra, 16 per riga, colorati per tipo.
// Il click mette il cursore; il cursore scelto altrove (una stringa, una
// firma, un'occorrenza) qui e' gia' in vista.

using namespace Inspect;
using namespace HexLayout;

namespace {
constexpr int HEADER_H = 12;
constexpr int PAD_X    = 6;
} // namespace

bool Visualizer::isHexView() const { return m_mode == ViewMode::HEX; }

// La finestra e' cambiata (o si e' appena entrati): righe nuove, e se il
// cursore e' qui dentro lo si porta in vista
void Visualizer::syncHexList() {
    const InspectWindow w = inspectWindow();
    if (w.start == m_hexStart && w.end == m_hexEnd) return;
    m_hexStart = w.start;
    m_hexEnd   = w.end;
    m_hexList.state.setCount(rowCount(w.len));
    if (m_cursorOffset >= w.start && m_cursorOffset < w.end) {
        const size_t row = (m_cursorOffset - w.start) / PER_ROW;
        m_hexList.state.selected = row;
        m_hexList.state.scrollTo(row > 5 ? row - 5 : 0);
    }
}

// Offset del file sotto il mouse; SIZE_MAX se nessuno
size_t Visualizer::hexOffsetUnderMouse() const {
    if (!isHexView() || m_hexList.hover == ListState::NONE) return SIZE_MAX;
    const int i = byteAtX(GetMouseX() - (m_hexList.x + PAD_X));
    if (i < 0) return SIZE_MAX;
    const size_t off = m_hexStart + m_hexList.hover * PER_ROW + i;
    return off < m_hexEnd ? off : SIZE_MAX;
}

void Visualizer::handleHexInput() {
    syncHexList();
    if (!m_hexList.update(this)) return;
    const size_t row = m_hexList.state.selected;
    if (row == ListState::NONE) return;

    // Col mouse: il byte cliccato. Con la tastiera: stessa colonna di prima.
    size_t off = hexOffsetUnderMouse();
    if (!GetMouse(0).bPressed || off == SIZE_MAX) {
        const size_t col = m_cursorOffset >= m_hexStart && m_cursorOffset < m_hexEnd
                               ? (m_cursorOffset - m_hexStart) % PER_ROW : 0;
        off = std::min(m_hexStart + row * PER_ROW + col, m_hexEnd - 1);
    }
    goToOffset(off);
}

void Visualizer::drawHex() {
    const int x = CANVAS_X, y = CANVAS_Y, w = CANVAS_W + SCROLLBAR_V_W;
    const int h = CANVAS_H + SCROLLBAR_H;
    FillRect(x, y, w, h, COL_BG_DEEP());
    syncHexList();
    m_hexList.setArea(x, y + HEADER_H, w, h - HEADER_H);

    const int tx = x + PAD_X;
    auto at = [&](int col) { return tx + col * CHAR_W; };
    DrawString(at(0), y + 2, "OFFSET", COL_DIM());
    for (int i = 0; i < PER_ROW; i++) DrawString(at(hexCol(i)), y + 2, hex(i, 2), COL_DIM());
    DrawString(at(COL_ASCII), y + 2, "TESTO", COL_DIM());

    const auto& bytes = m_reader.getBytes();
    const size_t hover = hexOffsetUnderMouse();
    const bool section = m_hlEnd > m_hlStart;

    m_hexList.draw(this, [&](size_t row, int ry) {
        const size_t rowOff = m_hexStart + row * PER_ROW;
        DrawString(at(0), ry, hex(rowOff, 8), COL_DIM());
        for (int i = 0; i < PER_ROW; i++) {
            const size_t off = rowOff + i;
            if (off >= m_hexEnd || off >= bytes.size()) break;
            const uint8_t v = bytes[off];
            const char ch = (v >= 0x20 && v < 0x7F) ? (char)v : '.';
            const int hx = at(hexCol(i)), cx = at(textCol(i));

            olc::Pixel col = classPixel(ByteInfo::classOf(v)), text = COL_TEXT();
            if (section && (off < m_hlStart || off >= m_hlEnd)) col = text = COL_BORDER();
            if (off == m_cursorOffset) {
                FillRect(hx - 1, ry - 1, 18, 10, COL_ACCENT());
                FillRect(cx, ry - 1, 8, 10, COL_ACCENT());
                col = text = olc::BLACK;
            } else if (off == hover) {
                DrawRect(hx - 2, ry - 2, 19, 11, COL_TEXT());
                DrawRect(cx - 1, ry - 2, 9, 11, COL_TEXT());
            }
            DrawString(hx, ry, hex(v, 2), col);
            DrawString(cx, ry, std::string(1, ch), text);
        }
    });
}
