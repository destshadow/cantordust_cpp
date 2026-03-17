#include "wavelength_rgb.h"
#include <algorithm>

// --- Approssimazione CIE 1931 XYZ ---
// Multi-lobe Gaussian fit da Wyman et al. 2013
void WavelengthRGB::wavelengthToXYZ(double wave,
                                     double& x, double& y, double& z) {
    // X
    double t1 = (wave-442.0)*((wave<442.0)?0.0624:0.0374);
    double t2 = (wave-599.8)*((wave<599.8)?0.0264:0.0323);
    double t3 = (wave-501.1)*((wave<501.1)?0.0490:0.0382);
    x =  0.362*std::exp(-0.5*t1*t1)
       + 1.056*std::exp(-0.5*t2*t2)
       - 0.065*std::exp(-0.5*t3*t3);

    // Y
    t1 = (wave-568.8)*((wave<568.8)?0.0213:0.0247);
    t2 = (wave-530.9)*((wave<530.9)?0.0613:0.0322);
    y =  0.821*std::exp(-0.5*t1*t1)
       + 0.286*std::exp(-0.5*t2*t2);

    // Z
    t1 = (wave-437.0)*((wave<437.0)?0.0845:0.0278);
    t2 = (wave-459.0)*((wave<459.0)?0.0385:0.0725);
    z =  1.217*std::exp(-0.5*t1*t1)
       + 0.681*std::exp(-0.5*t2*t2);
}

// --- Gamma correction sRGB ---
double WavelengthRGB::srgbPostprocess(double c) {
    c = std::clamp(c, 0.0, 1.0);
    return c <= 0.0031308
           ? c * 12.92
           : 1.055 * std::pow(c, 1.0/2.4) - 0.055;
}

// --- Matrice XYZ → sRGB (IEC 61966-2-1) ---
void WavelengthRGB::xyzToSRGB(double x,  double y,  double z,
                                double& r, double& g, double& b) {
    r = srgbPostprocess( 3.2406255*x - 1.537208*y  - 0.4986286*z);
    g = srgbPostprocess(-0.9689307*x + 1.8757561*y + 0.0415175*z);
    b = srgbPostprocess( 0.0557101*x - 0.2040211*y + 1.0569959*z);
}

WavelengthRGB::RGB WavelengthRGB::wavelengthToRGB(double wavelength) {
    double x, y, z;
    wavelengthToXYZ(wavelength, x, y, z);

    double r, g, b;
    xyzToSRGB(x, y, z, r, g, b);

    return {
        (uint8_t)(r * 255.0),
        (uint8_t)(g * 255.0),
        (uint8_t)(b * 255.0)
    };
}

// Mappa t[0,1] → arcobaleno 380-780nm
WavelengthRGB::RGB WavelengthRGB::valueToRainbow(double t) {
    t = std::clamp(t, 0.0, 1.0);
    double wavelength = 380.0 + t * (780.0 - 380.0);
    return wavelengthToRGB(wavelength);
}

// Mappa byte[0,255] → arcobaleno
WavelengthRGB::RGB WavelengthRGB::byteToRainbow(uint8_t byte_val) {
    return valueToRainbow(byte_val / 255.0);
}
