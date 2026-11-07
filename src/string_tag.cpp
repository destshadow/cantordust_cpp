#include "string_scan.h"
#include <cctype>

// Il tag di una stringa, se somiglia a qualcosa di riconoscibile. Sono
// indizi, non certezze: "1.2.3.4" puo' anche essere un numero di versione.

namespace {

bool alnum(char c) { return std::isalnum((unsigned char)c) != 0; }
bool digit(char c) { return std::isdigit((unsigned char)c) != 0; }

// "qualcosa://", con almeno due lettere prima (non "C://")
bool hasUrl(const std::string& s) {
    for (size_t p = s.find("://"); p != std::string::npos; p = s.find("://", p + 1)) {
        size_t letters = 0;
        while (letters < p && std::isalpha((unsigned char)s[p - letters - 1])) letters++;
        if (letters >= 2) return true;
    }
    return false;
}

// Quattro numeri da 0 a 255 separati da punti, non attaccati ad altro
bool ipAt(const std::string& s, size_t i, size_t& end) {
    for (int part = 0; part < 4; part++) {
        int value = 0, digits = 0;
        while (i < s.size() && digit(s[i]) && digits < 4) {
            value = value * 10 + (s[i] - '0');
            digits++;
            i++;
        }
        if (digits == 0 || digits > 3 || value > 255) return false;
        if (part < 3) {
            if (i >= s.size() || s[i] != '.') return false;
            i++;
        }
    }
    end = i;
    return i == s.size() || !(alnum(s[i]) || s[i] == '.');
}

bool hasIp(const std::string& s) {
    for (size_t i = 0; i < s.size(); i++) {
        if (!digit(s[i]) || (i > 0 && (alnum(s[i - 1]) || s[i - 1] == '.'))) continue;
        size_t end;
        if (ipAt(s, i, end)) return true;
    }
    return false;
}

// La parola che inizia in i e' un percorso?
bool pathAt(const std::string& s, size_t i) {
    const size_t n = s.size() - i;
    const char* p = s.c_str() + i;
    if (n >= 3 && std::isalpha((unsigned char)p[0]) && p[1] == ':' && p[2] == '\\') return true;
    if (n >= 3 && p[0] == '\\' && p[1] == '\\' && alnum(p[2])) return true;
    if (n >= 3 && p[0] == '.' && p[1] == '/') return true;
    if (n >= 4 && p[0] == '.' && p[1] == '.' && p[2] == '/') return true;
    if (n >= 3 && p[0] == '~' && p[1] == '/') return true;
    if (n >= 4 && p[0] == '/' && alnum(p[1]))         // "/usr/lib", non "/x"
        for (size_t k = 2; k < n && p[k] != ' '; k++)
            if (p[k] == '/') return true;
    return false;
}

bool hasPath(const std::string& s) {
    for (size_t i = 0; i < s.size(); i++)
        if ((i == 0 || s[i - 1] == ' ' || s[i - 1] == '"' || s[i - 1] == '\'') && pathAt(s, i))
            return true;
    return false;
}

} // namespace

StringScan::Tag StringScan::classify(const std::string& s) {
    if (hasUrl(s))  return Tag::URL;
    if (hasIp(s))   return Tag::IP;
    if (hasPath(s)) return Tag::PATH;
    return Tag::NONE;
}
