// xsend: eventi XTest verso la finestra (solo per test)
//
//   xsend <id finestra> key 6 sleep 900 click 600 300 move 1100 700 wheel 4
//                       down 110 300 move 110 500 up
//
// XTest manda tasti e click allo schermo, non a una finestra: finiscono a
// quella attiva. Per questo prima di OGNI evento si controlla che la finestra
// dell'app esista ancora; se e' sparita (chiusa, crash, limite di tempo) si
// smette subito, rilasciando il pulsante, ed esce con 3. Senza questo
// controllo il resto della sequenza finirebbe sulle altre finestre dell'utente.
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int gone = 0;
static int onError(Display* d, XErrorEvent* e) { (void)d; (void)e; gone = 1; return 0; }

// La finestra esiste ancora ed e' visibile?
static int alive(Display* d, Window w) {
    XWindowAttributes a;
    gone = 0;
    if (!XGetWindowAttributes(d, w, &a)) return 0;
    XSync(d, False);
    return !gone && a.map_state == IsViewable;
}

int main(int c, char** v) {
    Display* d = XOpenDisplay(0);
    if (!d || c < 2) return 1;
    XSetErrorHandler(onError);
    Window w = strtoul(v[1], 0, 0);
    if (!alive(d, w)) return 3;
    int wx = 0, wy = 0;
    Window ch;
    XTranslateCoordinates(d, w, DefaultRootWindow(d), 0, 0, &wx, &wy, &ch);

    int held = 0;       // pulsante sinistro premuto da "down"
    for (int i = 2; i < c; i++) {
        if (!alive(d, w)) {
            if (held) XTestFakeButtonEvent(d, 1, 0, 0);
            XFlush(d);
            XCloseDisplay(d);
            return 3;
        }
        if (!strcmp(v[i], "key") && i + 1 < c) {
            KeyCode kc = XKeysymToKeycode(d, XStringToKeysym(v[++i]));
            XTestFakeKeyEvent(d, kc, 1, 0); XFlush(d); usleep(60000);
            XTestFakeKeyEvent(d, kc, 0, 0);
        } else if (!strcmp(v[i], "move") && i + 2 < c) {
            int x = atoi(v[++i]), y = atoi(v[++i]);
            XTestFakeMotionEvent(d, -1, wx + x, wy + y, 0);
        } else if (!strcmp(v[i], "click") && i + 2 < c) {
            int x = atoi(v[++i]), y = atoi(v[++i]);
            XTestFakeMotionEvent(d, -1, wx + x, wy + y, 0); XFlush(d); usleep(60000);
            XTestFakeButtonEvent(d, 1, 1, 0); XFlush(d); usleep(60000);
            XTestFakeButtonEvent(d, 1, 0, 0);
        } else if (!strcmp(v[i], "down") && i + 2 < c) {
            int x = atoi(v[++i]), y = atoi(v[++i]);
            XTestFakeMotionEvent(d, -1, wx + x, wy + y, 0); XFlush(d); usleep(60000);
            XTestFakeButtonEvent(d, 1, 1, 0);
            held = 1;
        } else if (!strcmp(v[i], "up")) {
            XTestFakeButtonEvent(d, 1, 0, 0);
            held = 0;
        } else if (!strcmp(v[i], "wheel") && i + 1 < c) {
            int b = atoi(v[++i]);
            XTestFakeButtonEvent(d, b, 1, 0); XFlush(d); usleep(30000);
            XTestFakeButtonEvent(d, b, 0, 0);
        } else if (!strcmp(v[i], "sleep") && i + 1 < c) {
            usleep(atoi(v[++i]) * 1000);
        }
        XFlush(d);
        usleep(80000);
    }
    if (held) XTestFakeButtonEvent(d, 1, 0, 0);
    XFlush(d);
    XCloseDisplay(d);
    return 0;
}
