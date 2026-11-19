#include "visualizer.h"
#include "inspector_format.h"
#include "option_bar.h"
#include <cstdio>

// Profilo record (tasto K, nel menu' "Altro"): periodo e inizio, Inspector.
// Il calcolo e' in record_profile.* e record_field.cpp.

using namespace Inspect;

void Visualizer::setRecordShape(int period, int phase) {
    m_recordPeriod = std::clamp(period, 1, RecordProfile::MAX_PERIOD);
    m_recordPhase  = ((phase % m_recordPeriod) + m_recordPeriod) % m_recordPeriod;
    recomputeWindow();
}

void Visualizer::drawRecordOptions(bool clickable) {
    OptionBar bar(this, 6, CMD_STATUS_Y + 1, clickable);
    bar.title("Periodo [, .]: " + std::to_string(m_recordPeriod));
    if (bar.option("-", false)) m_pendingOption = [this] { setRecordShape(m_recordPeriod - 1, m_recordPhase); };
    if (bar.option("+", false)) m_pendingOption = [this] { setRecordShape(m_recordPeriod + 1, m_recordPhase); };
    bar.gap();
    bar.title("Inizio [ ]: " + std::to_string(m_recordPhase));
    if (bar.option("-", false)) m_pendingOption = [this] { setRecordShape(m_recordPeriod, m_recordPhase - 1); };
    if (bar.option("+", false)) m_pendingOption = [this] { setRecordShape(m_recordPeriod, m_recordPhase + 1); };
    bar.gap();
    const std::string autoLabel = m_suggestWidth > 0
        ? "[A] auto: " + std::to_string(m_suggestWidth) : "[A] auto";
    if (bar.option(autoLabel, false, m_suggestWidth > 0))
        m_pendingOption = [this] { setRecordShape(m_suggestWidth, 0); };
}

// , . = periodo +-1 (Shift = 10: < > americana, ; : italiana); [ ] = inizio;
// A = il periodo suggerito dall'autocorrelazione
void Visualizer::handleRecordKeys() {
    int period = 0;
    for (int32_t code : GetKeyPressCache()) {
        if (code == ',')                 period -= 1;
        if (code == '.')                 period += 1;
        if (code == '<' || code == ';')  period -= 10;
        if (code == '>' || code == ':')  period += 10;
    }
    int phase = 0;
    if (GetKey(olc::Key::OEM_4).bPressed) phase -= 1;
    if (GetKey(olc::Key::OEM_6).bPressed) phase += 1;
    if (period != 0 || phase != 0) setRecordShape(m_recordPeriod + period, m_recordPhase + phase);
    if (GetKey(olc::Key::A).bPressed && m_suggestWidth > 0) setRecordShape(m_suggestWidth, 0);
}

bool Visualizer::inspectRecord(PanelWriter& pw) {
    int cx, cy;
    if (!mouseToContent(GetMouseX(), GetMouseY(), cx, cy)) return false;
    const RecordProfile& r = m_record;
    if (r.empty()) {
        pw.text("nessun record intero nella finestra", COL_DIM());
        return true;
    }
    const int j = r.byteAt(cx);
    const RecordProfile::Column& c = r.column(j);
    const uint8_t v = (uint8_t)cy;
    const uint32_t n = r.count(j, v);
    uint8_t fr, fg, fb;
    RecordProfile::fieldColor(c.field, fr, fg, fb);

    pw.field("BYTE", std::to_string(j) + " del record (" + std::to_string(r.period()) + ")", COL_ACCENT());
    pw.field("VALORE", byteDesc(v), classPixel(ByteInfo::classOf(v)));
    pw.field("VOLTE", std::to_string(n) + "  " + pct(r.sampled() ? double(n) / r.sampled() : 0));
    pw.field("CAMPO", RecordProfile::fieldName(c.field), olc::Pixel(fr, fg, fb));
    if (c.field == RecordProfile::Field::COUNTER)
        pw.field("PASSO", "+" + std::to_string(c.step) + " a ogni record contato");
    pw.field("PREVALE", byteDesc(c.top) + "  " + pct(c.topShare));
    char ent[48];
    std::snprintf(ent, sizeof ent, "%d diversi, %.1f bit", c.distinct, c.entropy);
    pw.field("VALORI", ent);
    pw.separator("il record");
    pw.text(r.pattern(19), COL_TEXT());
    pw.text("hex = fisso  C contatore  t testo", COL_DIM());
    pw.text("e pochi valori  . vario  ? casuale", COL_DIM());
    pw.separator("quanti");
    pw.field("RECORD", std::to_string(r.records()) + " nella zona");
    if (r.sampled() < r.records())
        pw.field("CONTATI", std::to_string(r.sampled()) + " (uno ogni " +
                 std::to_string((r.records() + r.sampled() - 1) / r.sampled()) + ")");
    return true;
}
