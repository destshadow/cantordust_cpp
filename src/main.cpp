#define OLC_PGE_APPLICATION
#include "visualizer.h"
#include <iostream>

int main(int argc, char* argv[]) {
    std::string filepath = "";
    if (argc > 1) filepath = argv[1];

    Visualizer app(filepath);
    if (app.Construct(WIN_W, WIN_H, 1, 1))
        app.Start();
    return 0;
}
