#include "visualizer.h"
#include "inspector_format.h"
#include "option_bar.h"

// Vista Stringhe: il testo trovato nella finestra, in una lista.
// Il click mostra dove sta nel file; i dettagli sono nell'Inspector.

using Tag = StringScan::Tag;

namespace {
// Colonne, in caratteri dal bordo della lista
constexpr int COL_ENC = 10, COL_LEN = 18, COL_TAG = 25, COL_TEXT_AT = 35;
constexpr int HEADER_H = 12;
} // namespace

bool Visualizer::isStringsView() const { return m_mode == ViewMode::STRINGS; }

void Visualizer::rebuildStringRows() {
    m_stringRows = m_strings.select(m_stringMin, m_stringTag);
    m_stringList.state.setCount(m_stringRows.size());
}

const StringScan::Hit* Visualizer::stringAtRow(size_t row) const {
    return row < m_stringRows.size() ? &m_strings.hits()[m_stringRows[row]] : nullptr;
}

void Visualizer::setStringFilter(int minChars, Tag tag) {
    m_stringMin = minChars;
    m_stringTag = tag;
    rebuildStringRows();
}

void Visualizer::handleStringsInput() {
    if (!m_stringList.update(this)) return;
    if (const StringScan::Hit* h = stringAtRow(m_stringList.state.selected))
        goToOffset(m_stringBase + h->offset);
}

void Visualizer::drawStrings() {
    using namespace Inspect;
    const int x = CANVAS_X, y = CANVAS_Y, w = CANVAS_W + SCROLLBAR_V_W;
    const int h = CANVAS_H + SCROLLBAR_H;
    FillRect(x, y, w, h, COL_BG_DEEP());

    const int tx = x + 6;
    auto at = [&](int col) { return tx + col * 8; };
    DrawString(at(0), y + 2, "OFFSET", COL_DIM());
    DrawString(at(COL_ENC), y + 2, "TIPO", COL_DIM());
    DrawString(at(COL_LEN), y + 2, "LEN", COL_DIM());
    DrawString(at(COL_TAG), y + 2, "TAG", COL_DIM());
    DrawString(at(COL_TEXT_AT), y + 2, "TESTO", COL_DIM());

    m_stringList.setArea(x, y + HEADER_H, w, h - HEADER_H);
    if (m_stringRows.empty() && !m_computing)
        DrawString(tx, y + HEADER_H + 4, "nessuna stringa con questi filtri", COL_DIM());

    const auto& bytes = m_reader.getBytes();
    const size_t base = m_stringBase;
    const size_t room = (size_t)std::max(0, (m_stringList.rowsW() - 6) / 8 - COL_TEXT_AT);
    const bool section = m_hlEnd > m_hlStart;

    m_stringList.draw(this, [&](size_t row, int ry) {
        const StringScan::Hit* s = stringAtRow(row);
        if (!s) return;
        const size_t off = base + s->offset;
        const bool chosen = row == m_stringList.state.selected;
        const bool inSec  = !section || (off >= m_hlStart && off < m_hlEnd);
        olc::Pixel main = chosen ? COL_ACCENT() : row == m_stringList.hover ? olc::WHITE : COL_TEXT();
        olc::Pixel dim  = COL_DIM();
        if (!inSec) main = dim = COL_BORDER();
        DrawString(at(0), ry, hex(off, 8), dim);
        DrawString(at(COL_ENC), ry, StringScan::encName(s->enc), dim);
        DrawString(at(COL_LEN), ry, std::to_string(s->chars), dim);
        DrawString(at(COL_TAG), ry, StringScan::tagName(s->tag), inSec ? COL_ACCENT() : dim);
        DrawString(at(COL_TEXT_AT), ry, StringScan::textOf(bytes, base, *s, room), main);
    });
}

std::string Visualizer::stringsLegend() const {
    std::string s = "STRINGHE  " + std::to_string(m_stringRows.size()) + " su " +
                    std::to_string(m_strings.hits().size());
    if (m_strings.truncated()) s += " (elenco troncato)";
    return s + " nella finestra   click = dove sta nel file";
}

void Visualizer::handleStringKeys() {
    static const int MINS[] = {4, 6, 8, 16};
    static const Tag TAGS[] = {Tag::NONE, Tag::URL, Tag::IP, Tag::PATH};
    if (GetKey(olc::Key::L).bPressed) {
        int i = 0;
        while (MINS[i] != m_stringMin) i++;
        setStringFilter(MINS[(i + 1) % 4], m_stringTag);
    }
    if (GetKey(olc::Key::T).bPressed) {
        int i = 0;
        while (TAGS[i] != m_stringTag) i++;
        setStringFilter(m_stringMin, TAGS[(i + 1) % 4]);
    }
}

void Visualizer::drawStringOptions(bool clickable) {
    OptionBar bar(this, 6, CMD_STATUS_Y + 1, clickable);

    bar.title("Lunghezza min [L]:");
    for (int m : {4, 6, 8, 16})
        if (bar.option(std::to_string(m), m_stringMin == m))
            m_pendingOption = [this, m] { setStringFilter(m, m_stringTag); };

    bar.gap();
    bar.title("Tipo [T]:");
    const Tag tags[] = {Tag::NONE, Tag::URL, Tag::IP, Tag::PATH};
    const char* const names[] = {"tutte", "URL", "IP", "percorsi"};
    for (int i = 0; i < 4; i++) {
        const Tag t = tags[i];
        if (bar.option(names[i], m_stringTag == t))
            m_pendingOption = [this, t] { setStringFilter(m_stringMin, t); };
    }
}
