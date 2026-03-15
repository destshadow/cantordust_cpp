#define OLC_PGE_APPLICATION
#include "visualizer.h"
#include <iostream>

int main(int argc, char* argv[]) {
    // Se l'utente passa un file da CLI lo usiamo
    // altrimenti il visualizer carica /bin/ls come demo
    std::string filepath = "";
    if (argc > 1) {
        filepath = argv[1];
        std::cout << "[CantorDust++] File da analizzare: " << filepath << "\n";
    }

    Visualizer app(filepath);
    if (app.Construct(512, 542, 1, 1))
        app.Start();
    return 0;
}
