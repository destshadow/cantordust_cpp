#pragma once
#include <cmath>
#include <string>
#include <cstdint>

// Notazione esponenziale normalizzata in base 10
// Evita underflow/overflow quando si moltiplicano
// migliaia di probabilita' piccole (come nel classificatore NGram)
//
// Rappresenta il numero: coefficient * 10^exponent
// con coefficient sempre in [1.0, 10.0) oppure 0
class ExponentialNotation {
public:
    // Costruttori
    ExponentialNotation()
        : m_coeff(0.0), m_exp(0) {}

    explicit ExponentialNotation(double coefficient, long exponent = 0)
        : m_coeff(coefficient), m_exp(exponent) {
        normalize();
    }

    // --- Operazioni aritmetiche ---

    // Moltiplicazione: (a * 10^ea) * (b * 10^eb) = (a*b) * 10^(ea+eb)
    ExponentialNotation multiply(const ExponentialNotation& rhs) const {
        return ExponentialNotation(m_coeff * rhs.m_coeff,
                                   m_exp  + rhs.m_exp);
    }

    // Divisione: (a * 10^ea) / (b * 10^eb) = (a/b) * 10^(ea-eb)
    ExponentialNotation divide(const ExponentialNotation& rhs) const {
        if (rhs.m_coeff == 0.0) return ExponentialNotation(0.0);
        return ExponentialNotation(m_coeff / rhs.m_coeff,
                                   m_exp  - rhs.m_exp);
    }

    // Somma — allinea gli esponenti prima di sommare
    ExponentialNotation plus(const ExponentialNotation& rhs) const {
        if (m_exp > rhs.m_exp) {
            double factor = std::pow(10.0, (double)(m_exp - rhs.m_exp));
            return ExponentialNotation(m_coeff + rhs.m_coeff / factor,
                                       m_exp);
        } else if (rhs.m_exp > m_exp) {
            double factor = std::pow(10.0, (double)(rhs.m_exp - m_exp));
            return ExponentialNotation(rhs.m_coeff + m_coeff / factor,
                                       rhs.m_exp);
        }
        return ExponentialNotation(m_coeff + rhs.m_coeff, m_exp);
    }

    // Sottrazione
    ExponentialNotation minus(const ExponentialNotation& rhs) const {
        if (m_exp > rhs.m_exp) {
            double factor = std::pow(10.0, (double)(m_exp - rhs.m_exp));
            return ExponentialNotation(m_coeff - rhs.m_coeff / factor,
                                       m_exp);
        } else if (rhs.m_exp > m_exp) {
            double factor = std::pow(10.0, (double)(rhs.m_exp - m_exp));
            return ExponentialNotation(rhs.m_coeff - m_coeff / factor,
                                       rhs.m_exp);
        }
        return ExponentialNotation(m_coeff - rhs.m_coeff, m_exp);
    }

    // Potenza intera: (a * 10^e)^k = a^k * 10^(e*k)
    ExponentialNotation pow(int k) const {
        if (k == 0) return ExponentialNotation(1.0);
        if (m_coeff == 0.0) return ExponentialNotation(0.0);
        return ExponentialNotation(std::pow(m_coeff, (double)k),
                                   m_exp * k);
    }

    // --- Confronto ---
    bool greaterThan(const ExponentialNotation& rhs) const {
        return compare(rhs) > 0;
    }
    bool lessThan(const ExponentialNotation& rhs) const {
        return compare(rhs) < 0;
    }
    bool equalTo(const ExponentialNotation& rhs) const {
        return compare(rhs) == 0;
    }

    // Converte in double (perde precisione per esponenti grandi)
    double toDouble() const {
        return m_coeff * std::pow(10.0, (double)m_exp);
    }

    std::string toString() const {
        return std::to_string(m_coeff) + "*10^" +
               std::to_string(m_exp);
    }

    double getCoeff() const { return m_coeff; }
    long   getExp()   const { return m_exp; }

private:
    // Porta coefficient in [1.0, 10.0) aggiustando l'esponente
    // Gestisce anche 0 e numeri negativi
    void normalize() {
        while (true) {
            double ma = std::abs(m_coeff);
            if (ma == 0.0) {
                m_exp = 0;
                break;
            } else if (ma >= 10.0) {
                m_coeff /= 10.0;
                m_exp++;
            } else if (ma < 1.0) {
                m_coeff *= 10.0;
                m_exp--;
            } else {
                break;
            }
        }
    }

    // Confronto: gestisce segno ed esponente
    int compare(const ExponentialNotation& rhs) const {
        if (m_coeff == rhs.m_coeff && m_exp == rhs.m_exp) return 0;
        if (rhs.m_coeff == 0.0) return (m_coeff < 0) ? -1 : (m_coeff > 0) ? 1 : 0;
        if (m_coeff   == 0.0) return (rhs.m_coeff < 0) ? 1 : -1;
        if (m_coeff < 0 && rhs.m_coeff >= 0) return -1;
        if (m_coeff >= 0 && rhs.m_coeff < 0) return  1;
        if (m_exp < rhs.m_exp) return -1;
        if (m_exp > rhs.m_exp) return  1;
        if (m_coeff < rhs.m_coeff) return -1;
        if (m_coeff > rhs.m_coeff) return  1;
        return 0;
    }

    double m_coeff;
    long   m_exp;
};

// Potenza veloce: base^exp usando quadratura ripetuta
// Usata da NGramModel per calcolare p^k efficientemente
namespace MathUtils {
    inline ExponentialNotation fastPow(const ExponentialNotation& base,
                                       int exp) {
        if (exp == 0) return ExponentialNotation(1.0);
        if (exp == 1) return base;

        ExponentialNotation result(1.0);
        ExponentialNotation b = base;
        int e = exp;

        while (e > 0) {
            if (e & 1)
                result = result.multiply(b);
            b = b.multiply(b);
            e >>= 1;
        }
        return result;
    }
}
