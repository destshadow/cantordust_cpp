#define OLC_PGE_APPLICATION
#include "visualizer.h"

int main() {
    Visualizer app;
    if (app.Construct(512, 542, 1, 1))
        app.Start();
    return 0;
}
