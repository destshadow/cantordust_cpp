#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

// Come trattare i byte prima di mostrarli come pixel. Cambia solo l'immagine:
// offset e Inspector continuano a riferirsi ai byte veri del file.
//
//   Delta       differenza dal pixel precedente: audio e immagini non
//               compressi diventano quasi piatti, restano i bordi
//   Delta riga  differenza dalla riga sopra: con la larghezza giusta una
//               tabella di record diventa piatta, restano i campi che cambiano
//   Bit n       un solo bit di ogni byte, acceso o spento: rivela dati nascosti
//               nei bit bassi e campi di flag
struct RawFilter {
    static constexpr int COUNT = 11;   // normale, 2 delta, 8 bit
    int id = 0;

    bool none()     const { return id == 0; }
    bool delta()    const { return id == 1; }
    bool deltaRow() const { return id == 2; }
    bool bitPlane() const { return id >= 3; }
    int  bit()      const { return 10 - id; }     // id 3 = bit 7 ... id 10 = bit 0

    RawFilter next() const { return {(id + 1) % COUNT}; }
    RawFilter prev() const { return {(id + COUNT - 1) % COUNT}; }

    std::string name() const {
        if (none())     return "normale";
        if (delta())    return "delta";
        if (deltaRow()) return "delta riga";
        return "bit " + std::to_string(bit());
    }

    // Il byte numero `i` di `data` dopo il filtro. Il byte con cui confrontarlo
    // sta `dist` byte prima; se non c'e' la differenza vale zero. Differenza
    // zero = 128, grigio medio: le zone uniformi si vedono piatte, non nere.
    uint8_t apply(const uint8_t* data, size_t i, size_t dist) const {
        if (bitPlane()) return (data[i] >> bit()) & 1 ? 255 : 0;
        if (delta() || deltaRow())
            return i >= dist ? (uint8_t)(data[i] - data[i - dist] + 128) : 128;
        return data[i];
    }
};
