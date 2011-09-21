/* EXN is written by John Anthony <johnanthony@lavabit.com>, 2011.
 *
 * This software is in the public domain
 * and is provided AS IS, with NO WARRANTY.
 */

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>

#define LENGTH(x)   (sizeof(x) / sizeof(x[0])

typedef struct Client Client;
struct Client {
    Window win;
    Client *next;
};

typedef struct Monitor Monitor;
struct Monitor {
    Client *heads;
    int curr;
    Monitor *next;
    int x;
    int y;
    int width;
    int height;
};

static void clear_up(void);
static Monitor* create_mon(int x, int y, int w, int h);
static void errout(char *msg);
static void init(void);
static void* safe_malloc(unsigned int sz, char *errmsg);

static Display *dpy;
static int screen;
static Window root;
static Monitor *MonLL = NULL;

#include "config.h"

static void
clear_up(void) {
    XCloseDisplay(dpy);
}

static Monitor*
create_mon(int x, int y, int w, int h) {
    Monitor *m;

    m = safe_malloc(sizeof(Monitor), "Error allocating space for new monitor.");
    m->x = x;
    m->y = y;
    m->width = w;
    m->height = h;

    return m;
}

static void
errout(char* msg) {
    if (msg)
        puts(msg);
    exit(1);
}

static void
init(void) {
    XineramaScreenInfo *info;
    int i, numscreens;
    Monitor *m;

    dpy = XOpenDisplay(NULL);
    if (!dpy)
        errout("EX^N: Unable to open X display!");

    screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);

    /* Handle Xinerama screen loading */
    info = XineramaQueryScreens(dpy, &numscreens);
    for (i = 0; i < numscreens; ++i) {
        m = MonLL;
        MonLL = create_mon(info[i].x_org, info[i].y_org, info[i].width, info[i].height);
        MonLL->next = m;
    }
    XFree(info);
}

static void*
safe_malloc(unsigned int sz, char *errmsg) {
    void *mem = malloc(sz);
    if (!mem)
        errout(errmsg);
    return mem;
}

int main(int argc, char** argv) {
    init();


    clear_up();
    return 0;
}
