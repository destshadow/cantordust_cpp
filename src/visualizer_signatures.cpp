#include "visualizer.h"
#include "inspector_format.h"

// Firme: i file trovati dentro al file. Segni azzurri sulle barre, elenco nel
// riassunto dell'Inspector, N / Shift+N per passare dall'una all'altra.

using namespace Inspect;

void Visualizer::scanSignatures() {
    m_signatures = Signatures::scan(m_reader.getBytes());
    std::vector<size_t> offsets;
    for (const auto& h : m_signatures) offsets.push_back(h.offset);
    m_bars.setSignatures(std::move(offsets));
}

void Visualizer::handleSignatureKeys() {
    if (!GetKey(olc::Key::N).bPressed || m_signatures.empty()) return;
    const bool back = GetKey(olc::Key::SHIFT).bHeld;
    size_t i;
    if (m_cursorOffset == SIZE_MAX) {
        // Si parte dalla finestra corrente; se non ce ne sono, dal file
        const size_t start = m_navigator.getStart();
        i = back ? Signatures::prev(m_signatures, m_navigator.getEnd())
                 : start == 0 ? 0 : Signatures::next(m_signatures, start - 1);
    } else {
        i = back ? Signatures::prev(m_signatures, m_cursorOffset)
                 : Signatures::next(m_signatures, m_cursorOffset);
    }
    if (i == SIZE_MAX) i = back ? m_signatures.size() - 1 : 0;   // ricomincia
    goToOffset(m_signatures[i].offset);
    m_status = std::string("Firma ") + std::to_string(i + 1) + " di " +
               std::to_string(m_signatures.size()) + ": " + m_signatures[i].name +
               " a 0x" + hex(m_signatures[i].offset, 8);
}

// Nel riassunto: le firme attorno a quella corrente (o le prime)
void Visualizer::writeSignatures(PanelWriter& pw) {
    static constexpr size_t MAX_LINES = 6;
    if (m_signatures.empty()) return;
    const size_t n = m_signatures.size();
    pw.separator("firme: " + std::to_string(n) + (n >= Signatures::MAX_HITS ? "+" : "") +
                 "  [N] prossima");

    size_t current = SIZE_MAX;
    for (size_t i = 0; i < n; i++)
        if (m_signatures[i].offset == m_cursorOffset) current = i;
    size_t first = 0;
    if (current != SIZE_MAX && current >= MAX_LINES / 2) first = current - MAX_LINES / 2;
    if (first + MAX_LINES > n) first = n > MAX_LINES ? n - MAX_LINES : 0;

    for (size_t i = first; i < n && i < first + MAX_LINES; i++) {
        const auto& h = m_signatures[i];
        std::string line = "0x" + hex(h.offset, 8) + "  " + h.name;
        if (h.offset == 0) line += "  (inizio file)";
        pw.text(line, i == current ? COL_ACCENT() : COL_TEXT());
    }
    if (n > MAX_LINES)
        pw.text("solo l'inizio: la fine non e' nota", COL_DIM());
}
