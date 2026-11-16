#pragma once
#include <cmath>

// Test del chi-quadro: la probabilita' che uno scarto grande almeno `chi2`
// capiti per caso, con `dof` gradi di liberta'. Vicino a 1 = i dati sono
// compatibili con l'ipotesi; vicino a 0 = non lo sono.
namespace ChiSquare {

// Gamma incompleta superiore regolarizzata Q(a, x): serie per x piccolo,
// frazione continua (Lentz) per x grande
inline double gammaQ(double a, double x) {
    if (x <= 0.0 || a <= 0.0) return 1.0;
    const double scale = std::exp(-x + a * std::log(x) - std::lgamma(a));
    if (x < a + 1.0) {
        double ap = a, sum = 1.0 / a, term = sum;
        for (int n = 0; n < 500; n++) {
            ap += 1.0;
            term *= x / ap;
            sum += term;
            if (std::fabs(term) < std::fabs(sum) * 1e-12) break;
        }
        return std::fmax(0.0, 1.0 - sum * scale);
    }
    const double tiny = 1e-300;
    double b = x + 1.0 - a, c = 1.0 / tiny, d = 1.0 / b, h = d;
    for (int i = 1; i < 500; i++) {
        const double an = -i * (i - a);
        b += 2.0;
        d = an * d + b;
        if (std::fabs(d) < tiny) d = tiny;
        c = b + an / c;
        if (std::fabs(c) < tiny) c = tiny;
        d = 1.0 / d;
        const double delta = d * c;
        h *= delta;
        if (std::fabs(delta - 1.0) < 1e-12) break;
    }
    return std::fmin(1.0, scale * h);
}

inline double pValue(double chi2, int dof) {
    return dof > 0 ? gammaQ(dof / 2.0, chi2 / 2.0) : 0.0;
}

} // namespace ChiSquare
