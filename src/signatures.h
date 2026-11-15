#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

// Firme: i marcatori con cui iniziano i formati noti (PNG, ZIP, ELF...).
// Servono a trovare un file dentro un altro file. Si riconosce solo l'INIZIO:
// dove finisce il contenuto non si sa, e non si prova a leggerlo.
//
// I marcatori corti (2-3 byte) capitano anche per caso: per quelli si
// controlla qualche campo in piu' dell'intestazione (signature_checks.cpp).
namespace Signatures {

struct Hit {
    size_t      offset;    // dove inizierebbe il file
    const char* name;      // "PNG", "ZIP", ...
};

static constexpr size_t MAX_HITS = 10000;

// Tutte le firme in `bytes`, in ordine di offset
std::vector<Hit> scan(const std::vector<uint8_t>& bytes);

// La prima firma con offset > from (o >= se inclusive); SIZE_MAX se nessuna.
// Restituisce la posizione in `hits`.
size_t next(const std::vector<Hit>& hits, size_t from);
size_t prev(const std::vector<Hit>& hits, size_t from);

} // namespace Signatures
