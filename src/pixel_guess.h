#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

// Quanti byte fa un pixel (o un campione, o un campo), e da quale byte
// iniziano. Serve al tasto A della vista Raw: e' il percorso della GIF di
// riferimento (8 bit con i "fantasmi" -> 24 bit coi colori scambiati -> giusto).
namespace PixelGuess {

// Dalla differenza media per distanza (Autocorr): se i byte a distanza p si
// somigliano molto piu' di quelli alle distanze in mezzo, ogni p byte si
// ripete lo stesso tipo di dato. Restituisce 1, 2, 3, 4 o 8; 1 = nessun passo.
//
//   punteggio(p) = 1 - media(mad[p], mad[2p], mad[3p])
//                      / media(mad[k], k <= 3p, k non multiplo di p)
size_t bytesPerPixel(const std::vector<float>& mad);

// Con 4 byte per pixel uno e' spesso fisso (alpha, riempimento): il pixel
// inizia subito dopo. Restituisce lo spostamento 0..bpp-1; 0 se non si capisce
// (a 3 byte per pixel i canali si somigliano tutti: non c'e' modo di saperlo).
size_t phase(const std::vector<uint8_t>& bytes, size_t start, size_t end, size_t bpp);

} // namespace PixelGuess
