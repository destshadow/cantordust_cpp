#include "tests.h"

using Scan = StringScan;
using Tag  = StringScan::Tag;

static std::vector<uint8_t> bytesOf(const std::string& s) { return {s.begin(), s.end()}; }

static void appendUtf16(std::vector<uint8_t>& b, const std::string& s) {
    for (char c : s) { b.push_back((uint8_t)c); b.push_back(0); }
}

void testStrings() {
    // --- Ricerca: ASCII, UTF-16 anche a offset dispari, lunghezza minima ---
    {
        Scan s;
        s.compute({});
        assert(s.hits().empty());

        std::vector<uint8_t> b = bytesOf("abc");     // troppo corta
        b.push_back(0xFF);
        const size_t asciiAt = b.size();
        for (uint8_t c : bytesOf("hello")) b.push_back(c);
        b.push_back(0x01);
        b.push_back(0x02);                           // ora l'offset e' dispari
        const size_t utf16At = b.size();
        assert(utf16At % 2 == 1);
        appendUtf16(b, "wide");
        b.push_back(0xFF);

        s.compute(b);
        assert(s.hits().size() == 2);
        const auto& h0 = s.hits()[0];
        const auto& h1 = s.hits()[1];
        assert(h0.offset == asciiAt && h0.chars == 5 && h0.enc == Scan::Enc::ASCII);
        assert(h1.offset == utf16At && h1.chars == 4 && h1.enc == Scan::Enc::UTF16);
        assert(Scan::textOf(b, 0, h0, 100) == "hello");
        assert(Scan::textOf(b, 0, h1, 100) == "wide");
        assert(Scan::textOf(b, 0, h1, 2) == "wi");

        // base: la finestra inizia dopo l'inizio del file
        std::vector<uint8_t> file(100, 0xFF);
        file.insert(file.end(), b.begin(), b.end());
        assert(Scan::textOf(file, 100, h0, 100) == "hello");

        // Mai fuori dai byte, anche con un risultato vecchio
        Scan::Hit stale{1000, 50, Scan::Enc::UTF16, Tag::NONE};
        assert(Scan::textOf(b, 0, stale, 100).empty());

        // Una stringa alla fine, senza niente dopo
        Scan e;
        e.compute(bytesOf("\x01" "tail"));
        assert(e.hits().size() == 1 && e.hits()[0].chars == 4);
        std::vector<uint8_t> w;
        appendUtf16(w, "tail");
        w.pop_back();                                // ultimo byte mancante
        e.compute(w);
        assert(e.hits().empty());                    // 3 caratteri interi: troppo corta
    }

    // --- Filtri ---
    {
        std::vector<uint8_t> b = bytesOf("abcd");
        b.push_back(0);
        for (uint8_t c : bytesOf("/usr/lib/x")) b.push_back(c);
        b.push_back(0);
        for (uint8_t c : bytesOf("https://a.org")) b.push_back(c);
        Scan s;
        s.compute(b);
        assert(s.select(4, Tag::NONE).size() == 3);
        assert(s.select(5, Tag::NONE).size() == 2);
        assert(s.select(4, Tag::URL).size() == 1);
        assert(s.select(4, Tag::PATH).size() == 1);
        assert(s.select(20, Tag::NONE).empty());
    }

    // --- Tag ---
    {
        assert(Scan::classify("https://example.org/x") == Tag::URL);
        assert(Scan::classify("vedi ftp://host") == Tag::URL);
        assert(Scan::classify("C://nope") != Tag::URL);
        assert(Scan::classify("10.0.0.1") == Tag::IP);
        assert(Scan::classify("server 192.168.1.20:8080") == Tag::IP);
        assert(Scan::classify("300.1.1.1") == Tag::NONE);
        assert(Scan::classify("1.2.3") == Tag::NONE);
        assert(Scan::classify("1.2.3.4.5") == Tag::NONE);
        assert(Scan::classify("v1.2.3.4") == Tag::NONE);
        assert(Scan::classify("/usr/lib/libc.so") == Tag::PATH);
        assert(Scan::classify("apre \"/etc/passwd\" qui") == Tag::PATH);
        assert(Scan::classify("C:\\Windows\\system32") == Tag::PATH);
        assert(Scan::classify("\\\\server\\share") == Tag::PATH);
        assert(Scan::classify("../lib/x") == Tag::PATH);
        assert(Scan::classify("/x") == Tag::NONE);
        assert(Scan::classify("and/or") == Tag::NONE);
        assert(Scan::classify("") == Tag::NONE);
    }

    // --- ListState: scelta e scorrimento restano sempre validi ---
    {
        ListState l;
        l.visible = 10;
        l.moveBy(1);                                 // lista vuota
        assert(l.selected == ListState::NONE);
        l.scrollBy(5);
        assert(l.top == 0);
        assert(l.thumbH(100) == 100 && l.thumbY(100) == 0);

        l.setCount(100);
        l.scrollBy(-5);
        assert(l.top == 0);
        l.scrollBy(1000);
        assert(l.top == 90);
        l.scrollBy(-1000);
        assert(l.top == 0);

        l.moveBy(1);                                 // nessuna scelta: la prima visibile
        assert(l.selected == 0);
        l.select(50);                                // la scelta resta in vista
        assert(l.selected >= l.top && l.selected < l.top + l.visible);
        l.moveBy(-1000);
        assert(l.selected == 0 && l.top == 0);
        l.moveBy(1000);
        assert(l.selected == 99 && l.top == 90);

        l.scrollToTrack(0, 200);
        assert(l.top == 0);
        l.scrollToTrack(200, 200);
        assert(l.top == 90);
        assert(l.thumbY(200) + l.thumbH(200) <= 200);

        l.setCount(5);                               // lista piu' corta dell'area
        assert(l.top == 0 && l.selected == ListState::NONE && l.maxTop() == 0);
        l.scrollToTrack(50, 100);
        assert(l.top == 0);
    }
}
