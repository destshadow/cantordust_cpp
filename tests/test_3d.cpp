#include "tests.h"

void test3D() {
    {   // 3D: conteggi, ricerca, sezione prevalente
        //              A A A A A   B B B B B
        std::vector<uint8_t> b = {1,2,3,1,2,3,1,2,3, 9,9,9, 1,2,3, 7,7,7};
        std::vector<TriGraph::Range> secs = {{0, 11, 0}, {12, 18, 5}};
        TriGraph t;
        t.compute(b, secs);
        const Point3D* p = t.find(0x010203);
        assert(p && p->count == 4);
        assert(p->section == 0);                 // 3 volte in A, 1 in B
        assert(t.find(0x070707)->section == 5);
        assert(t.find(0x040404) == nullptr);
        assert(std::is_sorted(t.getPoints().begin(), t.getPoints().end(),
            [](const Point3D& x, const Point3D& y) { return x.key < y.key; }));

        // A cavallo tra due sezioni: non appartiene a nessuna
        assert(t.find(0x090901)->section == -1);

        TriGraph none;
        none.compute(b);
        assert(none.find(0x010203)->section == -1);
        none.compute({1, 2});                    // meno di 3 byte
        assert(none.getPointCount() == 0);

        // Il colore per sezione arriva dalla sezione; fuori, grigio
        std::vector<BinarySection> ss(6);
        ss[5].r = 10; ss[5].g = 20; ss[5].b = 30;
        CloudColor::paint(t.getPoints(), CloudColor::Mode::SECTION, ss);
        const Point3D* q = t.find(0x070707);
        assert(q->r == 10 && q->g == 20 && q->b == 30);
        // Una sezione che non esiste non deve far leggere fuori dall'array
        CloudColor::paint(t.getPoints(), CloudColor::Mode::SECTION, {});
        assert(t.find(0x070707)->r == 90);
    }

    {   // 3D: colori per classe e ciclo dei modi
        uint8_t r, g, b, er, eg, eb;
        CloudColor::classColor(0x414243, r, g, b);        // 'ABC' = testo
        ByteInfo::classColor(ByteInfo::Class::TEXT, er, eg, eb);
        assert(r == er && g == eg && b == eb);
        CloudColor::classColor(0x4100FF, r, g, b);        // tutte diverse
        CloudColor::mixedColor(er, eg, eb);
        assert(r == er && g == eg && b == eb);
        using CloudColor::Mode;
        assert(CloudColor::next(Mode::FREQUENCY, true)  == Mode::SECTION);
        assert(CloudColor::next(Mode::FREQUENCY, false) == Mode::BYTE_CLASS);
    }

    {   // 3D: fotogramma, densita' e picking
        CloudFrame f;
        f.resize(20, 10);
        assert(f.keyNear(5, 5, 4) == CloudFrame::NO_KEY);
        f.add(5, 5, 200, 0, 0, 3, 111);
        f.add(5, 5, 200, 0, 0, 9, 222);          // piu' pesante nello stesso pixel
        f.add(9, 5, 0, 200, 0, 1, 333);
        assert(f.keyNear(5, 5, 4) == 222);
        assert(f.keyNear(8, 5, 4) == 333);       // il piu' vicino, non il piu' pesante
        assert(f.keyNear(5, 1, 3) == CloudFrame::NO_KEY);   // fuori dal raggio
        assert(f.keyNear(-3, 5, 0) == CloudFrame::NO_KEY);  // fuori dal fotogramma

        f.resolve();
        uint32_t px = f.pixels()[5 * 20 + 5];
        assert((px & 0xFF) > 0 && ((px >> 8) & 0xFF) == 0);  // resta rosso, non bianco
        assert(f.pixels()[0] == 0xFF000000u);                // vuoto = nero
        // Un punto isolato deve restare visibile accanto a un pixel denso
        uint32_t lone = f.pixels()[5 * 20 + 9];
        assert(((lone >> 8) & 0xFF) >= 60);

        f.clear();
        assert(f.keyNear(5, 5, 4) == CloudFrame::NO_KEY);
    }

    {   // 3D: il pixel di un punto e' quello che il picking ritrova
        Camera3D cam;
        Camera3D::Viewport vp{0, 0, 700, 700};
        Mat3 m = cam.rotation();
        CloudFrame f;
        f.resize(vp.w, vp.h);
        const uint32_t key = 0x20407F;
        float sx, sy;
        assert(cam.toScreen(cam.toCamera(m, 0x20 / 127.5f - 1, 0x40 / 127.5f - 1, 0x7F / 127.5f - 1),
                            vp, sx, sy));
        f.add((int)sx, (int)sy, 1, 1, 1, 1, key);
        assert(f.keyNear((int)sx, (int)sy, 0) == key);

        // Dietro la camera non si proietta (la divisione ribalterebbe il punto)
        cam.camZ = 2.9f;
        assert(!cam.inFront({0, 0, -3.0f}));
        assert(!cam.toScreen({0, 0, -3.0f}, vp, sx, sy));
    }

    {   // 3D: trascinando, il punto del cubo piu' vicino segue il mouse.
        // Deve valere da qualunque lato si stia guardando il cubo.
        Camera3D::Viewport vp{0, 0, 700, 700};
        for (float ax : {-1.2f, -0.5f, 0.0f, 0.5f, 1.2f})
            for (float ay : {-3.0f, -0.7f, 0.0f, 0.6f, 1.5f, 2.4f, 4.0f}) {
                Camera3D cam;
                cam.angleX = ax;
                cam.angleY = ay;
                // Il punto del cubo davanti agli occhi: la riga Z della
                // rotazione, cambiata di segno, e' la direzione verso la camera
                Mat3 m = cam.rotation();
                const float px = -m[2][0], py = -m[2][1], pz = -m[2][2];
                auto screen = [&](const Camera3D& c, float& sx, float& sy) {
                    assert(c.toScreen(c.toCamera(c.rotation(), px, py, pz), vp, sx, sy));
                };
                float x0, y0, x1, y1;
                screen(cam, x0, y0);
                Camera3D right = cam;
                right.orbit(10, 0);
                screen(right, x1, y1);
                assert(x1 > x0);
                Camera3D down = cam;
                down.orbit(0, 10);
                screen(down, x1, y1);
                assert(y1 > y0);
            }
    }

    {   // Ritaglio delle linee
        float x0 = -10, y0 = 5, x1 = 30, y1 = 5;
        assert(clipLine(x0, y0, x1, y1, 0, 0, 20, 10));
        assert(x0 == 0 && x1 == 20 && y0 == 5 && y1 == 5);
        float a = 2, b = 2, c = 3, d = 3;                    // tutta dentro
        assert(clipLine(a, b, c, d, 0, 0, 20, 10) && a == 2 && d == 3);
        float e = -5, f = -5, g = -1, h = 30;                // tutta fuori
        assert(!clipLine(e, f, g, h, 0, 0, 20, 10));
        float i = 25, j = 1, k = 25, l = 9;                  // parallela e fuori
        assert(!clipLine(i, j, k, l, 0, 0, 20, 10));
    }
}
