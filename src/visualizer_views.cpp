#include "visualizer.h"
#include <array>
#include <cassert>

// La tabella delle viste: tutto cio' che una vista ha di suo sta qui.
// L'ordine e' quello di ViewMode.

const ViewDef& Visualizer::viewDef(ViewMode mode) {
    using V = Visualizer;
    using Rgb = std::vector<uint8_t>;
    static const std::array<ViewDef, VIEW_MODE_COUNT> table = [] {
        std::array<ViewDef, VIEW_MODE_COUNT> t{};
        auto add = [&](ViewMode m) -> ViewDef& {
            t[(size_t)m].mode = m;
            return t[(size_t)m];
        };

        {   ViewDef& d = add(ViewMode::DIGRAPH);
            d.rgb    = [](const V& v) -> Rgb { return v.m_digraph.toRGB(); };
            d.legend = [](const V&) -> std::string {
                return "DIGRAPH  X = byte i   Y = byte i+1   luminosita' = quante volte (log)"; };
        }
        {   ViewDef& d = add(ViewMode::DOTPLOT);
            d.rgb    = [](const V& v) -> Rgb {
                return v.m_dotSimilarity ? v.m_similarity.toRGB() : v.m_dotplot.toRGB(); };
            d.legend = [](const V& v) -> std::string {
                return v.m_dotSimilarity
                    ? "SOMIGLIANZA  X e Y = posizione   acceso = sequenze in comune   diagonali = ripetizioni"
                    : "DOT PLOT  X = posizione nella finestra   Y = valore del byte"; };
            d.options = [](V& v, bool click) { v.drawDotOptions(click); };
            d.keys    = [](V& v) {
                if (v.GetKey(olc::Key::T).bPressed) v.setDotSimilarity(!v.m_dotSimilarity); };
        }
        {   ViewDef& d = add(ViewMode::ENTROPY);
            d.rgb    = [](const V& v) -> Rgb { return v.m_entropy.toRGB(); };
            d.legend = [](const V&) -> std::string {
                return "ENTROPIA  X = posizione   altezza = entropia (0-8 bit)   rosso = alta"; };
        }
        {   ViewDef& d = add(ViewMode::HISTOGRAM);
            d.rgb    = [](const V& v) -> Rgb { return v.m_histogram.toRGB(); };
            d.legend = [](const V&) -> std::string {
                return "ISTOGRAMMA  X = valore del byte 0x00-0xFF   altezza = frequenza (log)"; };
        }
        {   ViewDef& d = add(ViewMode::TRIGRAPH3D);
            d.canvas = false;
            d.draw   = [](V& v) { v.draw3D(v.m_dt); };
            d.legend = [](const V&) -> std::string {
                return "3D  punto = 3 byte consecutivi   luce = densita'   click = dove compare"; };
            d.input  = [](V& v) { v.handle3DInput(v.m_dt); };
            d.inputWhileBusy = true;       // la camera si muove anche durante un calcolo
            d.options = [](V& v, bool click) { v.draw3DOptions(click); };
            d.enter  = [](V& v) { v.buildTripleMask(); };
        }
        {   ViewDef& d = add(ViewMode::RAWPIXELS);
            d.rgb    = [](const V& v) -> Rgb { return v.m_rawpixels.toRGB(); };
            d.legend = [](const V&) -> std::string {
                return "RAW  i byte come pixel, riga per riga   bpp: [ ]   larghezza: , .   filtro: F"; };
            d.panel   = [](V& v) { v.drawRawPanel(); };
            d.options = [](V& v, bool click) { v.drawRawOptions(click); };
            d.keys    = [](V& v) {
                if (v.GetKey(olc::Key::OEM_4).bPressed) v.setBpp((BppMode)(((int)v.m_bppMode + 4) % 5));
                if (v.GetKey(olc::Key::OEM_6).bPressed) v.setBpp((BppMode)(((int)v.m_bppMode + 1) % 5));
                v.handleRawInput();        // slider, larghezza, offset, A, F
            };
        }
        {   ViewDef& d = add(ViewMode::METRICMAP);
            d.rgb    = [](const V& v) -> Rgb { return v.m_metricmap.getRGB(); };
            d.legend = [](const V& v) -> std::string {
                if (v.m_colorMode == ColorMode::CLASSIFIER)
                    return "METRIC MAP  classifier: colore = contenuto a cui somiglia (nome nell'Inspector)";
                if (v.m_colorMode == ColorMode::FREQUENCY)
                    return "METRIC MAP  frequenza nella zona: blu = byte comune   rosso = byte raro";
                return v.m_colorMode == ColorMode::BYTE_CLASS
                    ? "METRIC MAP  grigio=zero  verde=controllo  blu=testo  rosso=alti  bianco=0xFF"
                    : "METRIC MAP  curva: byte vicini nel file = pixel vicini   colore: C   curva: V"; };
            d.options = [](V& v, bool click) { v.drawMetricOptions(click); };
            d.keys    = [](V& v) {
                if (v.GetKey(olc::Key::C).bPressed)
                    v.setColorMode(MetricMap::nextColorMode(v.m_colorMode, v.m_classifier.isReady()));
                if (v.GetKey(olc::Key::V).bPressed)
                    v.setCurveMode((CurveMode)(((int)v.m_curveMode + 1) % 3));
            };
        }
        {   ViewDef& d = add(ViewMode::BYTECLOUD);
            d.rgb    = [](const V& v) -> Rgb { return v.m_bytecloud.toRGB(); };
            d.legend = [](const V&) -> std::string {
                return "BYTE CLOUD  griglia 16x16 = valori 0x00-0xFF   grandezza = frequenza   00 e FF fuori scala"; };
        }
        {   ViewDef& d = add(ViewMode::ONETUPLE);
            d.rgb    = [](const V& v) -> Rgb { return v.m_onetuple.toRGB(); };
            d.legend = [](const V&) -> std::string {
                return "ONE TUPLE  X = valore del byte   Y = tratto del file (256 righe)   luminosita' = frequenza nella riga"; };
        }
        {   ViewDef& d = add(ViewMode::STRINGS);
            d.canvas = false;
            d.draw   = [](V& v) { v.drawStrings(); };
            d.legend = [](const V& v) -> std::string { return v.stringsLegend(); };
            d.input  = [](V& v) { v.handleStringsInput(); };
            d.options = [](V& v, bool click) { v.drawStringOptions(click); };
            d.keys    = [](V& v) { v.handleStringKeys(); };
            d.inspect = [](V& v, PanelWriter& pw) {
                const size_t hover = v.m_stringList.hover;
                const size_t row = hover != ListState::NONE ? hover : v.m_stringList.state.selected;
                const StringScan::Hit* s = v.stringAtRow(row);
                if (s) v.inspectString(pw, *s);
                return s != nullptr;
            };
        }
        {   ViewDef& d = add(ViewMode::HEX);
            d.canvas = false;
            d.draw   = [](V& v) { v.drawHex(); };
            d.legend = [](const V&) -> std::string {
                return "HEX  16 byte per riga   colore = tipo di byte   click = cursore   frecce, PagSu/PagGiu"; };
            d.input  = [](V& v) { v.handleHexInput(); };
        }
        {   ViewDef& d = add(ViewMode::OVERVIEW);
            d.canvas = false;
            d.draw   = [](V& v) { v.drawOverview(); };
            d.legend = [](const V&) -> std::string {
                return "PANORAMICA  X = posizione   una riga per indicatore   click = cursore"; };
            d.input  = [](V& v) { v.handleOverviewInput(); };
            d.inspect = [](V& v, PanelWriter& pw) {
                const int cell = v.overviewCellUnderMouse();
                if (cell >= 0) v.inspectOverview(pw, cell);
                return cell >= 0;
            };
        }

        // Ogni vista ha la sua voce, completa
        for (size_t i = 0; i < t.size(); i++) {
            assert(t[i].mode == (ViewMode)i && t[i].legend);
            assert(t[i].canvas ? t[i].rgb != nullptr : t[i].draw != nullptr);
        }
        return t;
    }();
    return table[(size_t)mode];
}
