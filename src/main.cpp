#define OLC_PGE_APPLICATION
#include "visualizer.h"
#include <iostream>

int main(int argc, char* argv[]) {
    std::string filepath = "";
    if (argc > 1) filepath = argv[1];

    Visualizer app(filepath);
    // 512 wide, 574 tall = 512 canvas + 26 topbar + 36 bottombar
    if (app.Construct(512, 574, 1, 1))
        app.Start();
    return 0;
}
