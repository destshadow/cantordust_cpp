#pragma once
#include <cstdint>
#include <cmath>

// Converte una lunghezza d'onda (nm) in RGB fisicamente accurato
// Basato su: Wyman et al. "Simple Analytic Approximations to the
// CIE XYZ Color Matching Functions", JCGT 2013
// Range valido: 380-780 nm (luce visibile)
class WavelengthRGB {
public:
    struct RGB { uint8_t r, g, b; };

    // Converte lunghezza d'onda in nm → RGB
    static RGB wavelengthToRGB(double wavelength);

    // Mappa un valore [0,1] all'arcobaleno completo
    // 0.0 = viola (380nm), 1.0 = rosso (780nm)
    static RGB valueToRainbow(double t);

    // Mappa un byte [0,255] all'arcobaleno
    static RGB byteToRainbow(uint8_t byte_val);

private:
    // Approssimazione CIE 1931 XYZ
    static void wavelengthToXYZ(double wave,
                                 double& x, double& y, double& z);
    // Conversione XYZ → sRGB
    static void xyzToSRGB(double x,  double y,  double z,
                           double& r, double& g, double& b);
    // Gamma correction sRGB
    static double srgbPostprocess(double c);
};
