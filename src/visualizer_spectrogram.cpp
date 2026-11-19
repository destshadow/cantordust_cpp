#include "visualizer.h"
#include "inspector_format.h"
#include "option_bar.h"
#include <cmath>
#include <cstdio>

// Spettrogramma (tasto G, nel menu' "Altro"): opzioni, Inspector, click.
// Il calcolo e' in spectrogram.*; qui solo cio' che riguarda l'interfaccia.

using namespace Inspect;

namespace {
constexpr int SPECTRO_MIN = 256, SPECTRO_MAX = 8192;

std::string fmt1(double v) {                 // una cifra decimale
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.1f", v);
    return buf;
}
} // namespace

void Visualizer::setSpectroSize(int n) {
    m_spectroSize = std::clamp(n, SPECTRO_MIN, SPECTRO_MAX);
    recomputeWindow();
}

void Visualizer::setSpectroFlat(bool on) {
    m_spectroFlat = on;
    recomputeWindow();
}

void Visualizer::drawSpectroOptions(bool clickable) {
    OptionBar bar(this, 6, CMD_STATUS_Y + 1, clickable);
    bar.title("Byte per tratto [ ]:");
    for (int n = SPECTRO_MIN; n <= SPECTRO_MAX; n *= 2)
        if (bar.option(std::to_string(n), m_spectroSize == n))
            m_pendingOption = [this, n] { setSpectroSize(n); };
    bar.gap();
    if (bar.option("[T] togli lo spettro medio", m_spectroFlat))
        m_pendingOption = [this] { setSpectroFlat(!m_spectroFlat); };
}

void Visualizer::handleSpectroKeys() {
    if (GetKey(olc::Key::OEM_4).bPressed) setSpectroSize(m_spectroSize / 2);    // [
    if (GetKey(olc::Key::OEM_6).bPressed) setSpectroSize(m_spectroSize * 2);    // ]
    if (GetKey(olc::Key::T).bPressed)     setSpectroFlat(!m_spectroFlat);
}

// Click (senza trascinare) su un punto: il cursore va al tratto e si apre il
// Raw largo quel periodo, cosi' i record si mettono in colonna
void Visualizer::handleSpectroInput() {
    if (!GetMouse(0).bReleased) return;
    if (std::abs(GetMouseX() - m_pressX) > 2 || std::abs(GetMouseY() - m_pressY) > 2) return;
    int cx, cy;
    if (!mouseToContent(GetMouseX(), GetMouseY(), cx, cy) || cx >= m_spectrogram.columns()) return;
    const int period = periodUnder(cx, cy);
    const int bpp = (int)RawPixels::bytesPerPixel(m_bppMode);
    goToOffset(m_computedStart + m_spectrogram.columnStart(cx));
    m_rawWidth = std::max(1, (period + bpp / 2) / bpp);
    syncRawSliders();
    switchView(ViewMode::RAWPIXELS);
    m_status = "Raw largo " + std::to_string(m_rawWidth) + " = il periodo";
}

// Il periodo intero della riga cy nel tratto cx, misurato sui byte veri
int Visualizer::periodUnder(int cx, int cy) const {
    const Spectrogram& s = m_spectrogram;
    return Spectrogram::exactPeriod(m_reader.getBytes(), m_computedStart + s.segmentStart(cx),
                                    (size_t)s.fftSize(), s.periodAt(cy));
}

bool Visualizer::inspectSpectrogram(PanelWriter& pw) {
    int cx, cy;
    if (!mouseToContent(GetMouseX(), GetMouseY(), cx, cy)) return false;
    const Spectrogram& s = m_spectrogram;
    if (cx >= s.columns()) return false;
    const size_t a = m_computedStart + s.columnStart(cx);
    const size_t b = m_computedStart + s.columnStart(cx + 1);
    const float lv = s.level(cx, cy);

    pw.field("TRATTO", "0x" + hex(a, 8) + "-0x" + hex(b, 8), COL_ACCENT());
    pw.field("LUNGH.", sizeStr(b - a));
    writeSectionField(pw, a);
    pw.field("PERIODO", std::to_string(periodUnder(cx, cy)) + " byte");
    if (lv <= Spectrogram::DARK)
        pw.field("FORZA", "costante: nessun periodo", COL_DIM());
    else
        pw.field("FORZA", fmt1(std::pow(10.0, lv)) + (m_spectroFlat ? " x la riga" : " x la media"));
    const int base = s.baseRow(cx);
    if (base >= 0)
        pw.field("DI BASE", std::to_string(periodUnder(cx, base)) + " byte (con le armoniche)", COL_ACCENT());
    else
        pw.field("DI BASE", "nessun periodo spicca", COL_DIM());
    pw.separator("come si legge");
    pw.text("riga luminosa = periodo che dura", COL_DIM());
    pw.text("p accende anche p/2, p/3...", COL_DIM());
    pw.text("analizzati " + std::to_string(s.fftSize()) + " byte al centro", COL_DIM());
    pw.text("click = Raw largo questo periodo", COL_DIM());
    return true;
}
