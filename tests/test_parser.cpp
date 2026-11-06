#include "tests.h"

void testParser() {
    // --- Parser: piu' di 64 sezioni non vengono perse ---
    {
        const size_t nsec = 100, shoff = 64, stroff = shoff + nsec * 64;
        std::vector<uint8_t> e(stroff + 64, 0);
        e[0] = 0x7f; e[1] = 'E'; e[2] = 'L'; e[3] = 'F'; e[4] = 2; e[5] = 1;
        put(e, 0x28, shoff, 8); put(e, 0x3a, 64, 2);
        put(e, 0x3c, nsec, 2);  put(e, 0x3e, 0, 2);
        put(e, shoff + 0x18, stroff, 8);                    // tabella dei nomi
        e[stroff + 1] = '.'; e[stroff + 2] = 'x';
        for (size_t i = 1; i < nsec; ++i) {
            size_t h = shoff + i * 64;
            put(e, h, 1, 4); put(e, h + 4, 1, 4);           // nome ".x", PROGBITS
            put(e, h + 0x18, 16, 8); put(e, h + 0x20, 8, 8);
        }
        BinaryParser p;
        p.parse(e);
        assert(p.getSections().size() == nsec - 1);
    }
}
