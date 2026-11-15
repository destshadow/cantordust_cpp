// xsend: eventi XTest verso la finestra (solo per test)
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
int main(int c,char**v){ Display*d=XOpenDisplay(0); if(!d)return 1;
 Window w=strtoul(v[1],0,0); int wx=0,wy=0; Window ch; XTranslateCoordinates(d,w,DefaultRootWindow(d),0,0,&wx,&wy,&ch);
 for(int i=2;i<c;i++){
  if(!strcmp(v[i],"key")){ KeyCode kc=XKeysymToKeycode(d,XStringToKeysym(v[++i])); XTestFakeKeyEvent(d,kc,1,0); XFlush(d); usleep(60000); XTestFakeKeyEvent(d,kc,0,0);}
  else if(!strcmp(v[i],"move")){int x=atoi(v[++i]),y=atoi(v[++i]); XTestFakeMotionEvent(d,-1,wx+x,wy+y,0);}
  else if(!strcmp(v[i],"click")){int x=atoi(v[++i]),y=atoi(v[++i]); XTestFakeMotionEvent(d,-1,wx+x,wy+y,0); XFlush(d); usleep(60000); XTestFakeButtonEvent(d,1,1,0); XFlush(d); usleep(60000); XTestFakeButtonEvent(d,1,0,0);}
  else if(!strcmp(v[i],"wheel")){int b=atoi(v[++i]); XTestFakeButtonEvent(d,b,1,0); XFlush(d); usleep(30000); XTestFakeButtonEvent(d,b,0,0);}
  else if(!strcmp(v[i],"sleep")){ usleep(atoi(v[++i])*1000); }
  XFlush(d); usleep(80000);}
 XCloseDisplay(d); return 0;}
