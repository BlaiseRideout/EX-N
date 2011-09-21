/* EXN is written by John Anthony <johnanthony@lavabit.com>, 2011.
 *
 * This software is in the public domain
 * and is provided AS IS, with NO WARRANTY.
 */

#include <stdio.h>
#include <stdlib.h>
#include <X11/cursorfont.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>

#define LENGTH(X)   ( sizeof X / sizeof X[0] )

typedef struct Client Client;
struct Client {
    Window win;
    Client *next;
};

typedef struct Monitor Monitor;
struct Monitor {
    Client *heads, *currhead, *currclient;
    int x;
    int y;
    int width;
    int height;
};

static void clear_up(void);
static Monitor create_mon(int x, int y, int w, int h);
static void errout(char *msg);
static void init(void);
static void maprequest(XEvent *e);
static Client* new_client(Window w);
static void nohandler(XEvent *e);
static void run(void);
static void* safe_malloc(unsigned int sz, char *errmsg);
static void (*handler[LASTEvent]) (XEvent *) = {
	[ButtonPress] = nohandler,
	[ClientMessage] = nohandler,
	[ConfigureRequest] = nohandler,
	[ConfigureNotify] = nohandler,
	[DestroyNotify] = nohandler,
	[EnterNotify] = nohandler,
	[Expose] = nohandler,
	[FocusIn] = nohandler,
	[KeyPress] = nohandler,
	[MappingNotify] = nohandler,
	[MapRequest] = maprequest,
	[PropertyNotify] = nohandler,
	[UnmapNotify] = nohandler
};

static Display *dpy;
static int screen;
static Window root;
static Monitor *mons, *currmon;
static int running;

#include "config.h"

static void
clear_up(void) {
    XCloseDisplay(dpy);
}

static Monitor
create_mon(int x, int y, int w, int h) {
    Monitor m;

    m.x = x;
    m.y = y;
    m.width = w;
    m.height = h;

    m.currhead = NULL;
    m.heads = safe_malloc(LENGTH(headnames) * sizeof(Client), "Failed allocating client space");

    return m;
}

static void
errout(char* msg) {
    if (msg)
        puts(msg);
    exit(1);
}

static void
maprequest(XEvent *e) {
    XMapRequestEvent *ev;
    XWindowAttributes wa;
    Client *c;

    ev = &e->xmaprequest;
    if(!XGetWindowAttributes(dpy, ev->window, &wa))
        return;

    c = new_client(ev->window);
    c->next = currmon->currhead;
    currmon->currhead = c;

    XMoveResizeWindow(dpy, c->win, currmon->x, currmon->y, currmon->width, currmon->height);
    XMapWindow(dpy, c->win);
}

static Client*
new_client(Window w) {
    Client *c;

    c = safe_malloc(sizeof(Client), "Failed to malloc space for client details.");
    c->win = w;

    return c;
}

static void
nohandler(XEvent *e) {
}

static void
init(void) {
    XineramaScreenInfo *info;
    XSetWindowAttributes wa;
    int i, numscreens;

    dpy = XOpenDisplay(NULL);
    if (!dpy)
        errout("EX^N: Unable to open X display!");

    screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);

    /* Handle Xinerama screen loading */
    mons = safe_malloc(numscreens * sizeof(Monitor), "Failed to allocate memory for monitors");
    info = XineramaQueryScreens(dpy, &numscreens);
    for (i = 0; i < numscreens; ++i)
        mons[i] = create_mon(info[i].x_org, info[i].y_org, info[i].width, info[i].height);
    XFree(info);
    currmon = mons;

    wa.cursor = XCreateFontCursor(dpy, XC_left_ptr);
    wa.event_mask = SubstructureRedirectMask|SubstructureNotifyMask|ButtonPressMask
        |EnterWindowMask|LeaveWindowMask|StructureNotifyMask
        |PropertyChangeMask;
    XChangeWindowAttributes(dpy, root, CWEventMask|CWCursor, &wa);
    XSelectInput(dpy, root, wa.event_mask);

    running = 1;
    XSync(dpy, False);
}

static void
run(void) {
    XEvent ev;
    while(running && !XNextEvent(dpy, &ev)) {
        if(handler[ev.type])
            handler[ev.type](&ev); /* call handler */
    }
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
    run();
    clear_up();
    return 0;
}
