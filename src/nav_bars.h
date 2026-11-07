#pragma once
#include "binary_format.h"
#include "range_sel.h"
#include <olcPixelGameEngine.h>
#include <cstdint>
#include <vector>

// Le barre di navigazione a sinistra, come nel CantorDust originale:
//
//   sezioni | FILE | ZONA
//
//   FILE  tutto il file; la selezione sceglie quale parte mostrare in ZONA
//   ZONA  quella parte ingrandita; la selezione decide cosa va nella vista
//
// Mouse: maniglie = estremi, trascina dentro = sposta, click fuori = vai li',
// rotella = allarga/restringe, tasto destro = valore/entropia.
class NavBars {
public:
    ~NavBars();

    void setFile(const std::vector<uint8_t>* bytes,
                 const std::vector<BinarySection>* sections);
    void reset();                // file intero
    void toggleStrip();          // valore dei byte <-> entropia

    // true se la selezione (quella che va nella vista) e' cambiata
    bool update(olc::PixelGameEngine* pge);
    void draw(olc::PixelGameEngine* pge);

    size_t start() const { return m_micro.lo; }
    size_t end()   const { return m_micro.hi; }

    size_t macroLo() const { return m_macro.lo; }
    size_t macroHi() const { return m_macro.hi; }

    // Segni sulle barre: una voce per riga, vedi rows()
    static int rows();
    void setMarks(std::vector<bool> fileRows, std::vector<bool> zoneRows);
    void clearMarks();
    void setCursor(size_t offset) { m_cursor = offset; }   // SIZE_MAX = nessuno
    void goTo(size_t offset);    // sposta le selezioni solo se e' fuori

    int  hoveredSection() const { return m_hoverSec; }   // -1 = nessuna
    // Byte sotto il mouse, se e' sopra una delle due barre
    bool hoverOffset(int mx, int my, size_t& off) const;

private:
    enum class Drag { NONE, LO, HI, SLIDE };

    RangeSel&       sel(int bar)       { return bar == 0 ? m_macro : m_micro; }
    const RangeSel& sel(int bar) const { return bar == 0 ? m_macro : m_micro; }
    int    barAt(int mx, int my) const;       // 0, 1 o -1
    size_t valueAtY(int bar, int my) const;
    int    yOf(int bar, size_t value) const;
    int    sectionAtY(int mx, int my) const;
    void   selectSection(int index);
    void   buildStrip(int bar);
    void   drawBar(olc::PixelGameEngine* pge, int bar);
    void   drawMarks(olc::PixelGameEngine* pge, int bar);
    void   drawSections(olc::PixelGameEngine* pge);

    const std::vector<uint8_t>*       m_bytes = nullptr;
    const std::vector<BinarySection>* m_secs  = nullptr;

    RangeSel m_macro;            // barra FILE
    RangeSel m_micro;            // barra ZONA (dentro m_macro)

    Drag   m_drag    = Drag::NONE;
    int    m_dragBar = -1;
    size_t m_grab    = 0;        // byte afferrato rispetto a lo
    int    m_hoverSec = -1;
    int    m_mouseX = 0, m_mouseY = 0;

    bool   m_entropy = false;    // la striscia mostra l'entropia
    olc::Sprite* m_strip[2] = {nullptr, nullptr};
    size_t m_builtLo[2] = {1, 1}, m_builtHi[2] = {0, 0};
    bool   m_builtEntropy[2] = {false, false};

    std::vector<bool> m_marks[2];
    size_t m_cursor = SIZE_MAX;
};
