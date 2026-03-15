#pragma once
#include "binary_reader.h"
#include "digraph.h"
#include "screenshot.h"
#include "../libs/olcPixelGameEngine.h"

class Visualizer : public olc::PixelGameEngine {
public:
    Visualizer();

    bool OnUserCreate() override;
    bool OnUserUpdate(float fElapsedTime) override;

private:
    void drawDigraph();
    void drawUI();
    void loadFile(const std::string& filepath);

    BinaryReader m_reader;
    DiGraph      m_digraph;

    olc::Sprite* m_canvas = nullptr;
    olc::Decal*  m_decal  = nullptr;

    bool         m_dirty  = false;
    std::string  m_status = "Benvenuto in CantorDust++";
};
