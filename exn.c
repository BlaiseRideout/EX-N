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
    Client *next, *prev;
};

typedef struct {
    int x;
    int y;
    int width;
    int height;
    Client *clients;
} Monitor;

static void clear_up(void);
static Monitor create_mon(int x, int y, int w, int h);
static void errout(char *msg);
static Client* find_client(Window w);
static void init(void);
static void maprequest(XEvent *e);
static Client* new_client(Window w);
static void nohandler(XEvent *e);
static void remove_client(Client *c);
static void run(void);
static void* safe_malloc(unsigned int sz, char *errmsg);
static void unmapnotify(XEvent *e);
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
	[UnmapNotify] = unmapnotify
};

static Display *dpy;
static int screen;
static Window root;
static Monitor *mons;
static int running;
static int nummons;
static int currmon;

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

    m.clients = NULL;

    return m;
}

static void
errout(char* msg) {
    if (msg)
        puts(msg);
    exit(1);
}

static Client*
find_client(Window w) {
    int i;
    Client *c;

    printf("Lookign for window %d\n", w);

    for (i = 0; i < nummons; ++i) {
        for (c = mons[i].clients; c; c = c->next) {
            printf("Trying against window %d\n", c->win);
            if (c->win == w)
                return c;
        }
    }

    return NULL;
}

static void
maprequest(XEvent *e) {
    XMapRequestEvent *ev;
    XWindowAttributes wa;
    Client *c;
    Monitor *m;

    m = &mons[currmon];

    ev = &e->xmaprequest;
    if(!XGetWindowAttributes(dpy, ev->window, &wa))
        return;

    c = new_client(ev->window);

    if (m->clients) {
        c->next = m->clients;
        m->clients->prev = c;
    }
    else
        c->next = NULL;
    m->clients = c;
    c->prev = NULL;

    XMoveResizeWindow(dpy, c->win, m->x, m->y, m->width, m->height);
    XMapWindow(dpy, c->win);
    XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
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
remove_client(Client *c) {
    int i;

    /* Adjust the head of out monitors is necessary */
    for (i = 0; i < nummons; ++i)
        if (mons[i].clients == c)
            mons[i].clients = mons[i].clients->next;

    if (c->prev)
        c->prev->next = c->next;
    if (c->next)
        c->next->prev = c->prev;

    free(c);
}

static void
init(void) {
    XineramaScreenInfo *info;
    XSetWindowAttributes wa;
    int i;

    dpy = XOpenDisplay(NULL);
    if (!dpy)
        errout("EX^N: Unable to open X display!");

    screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);
    currmon = 0;

    /* Handle Xinerama screen loading */
    info = XineramaQueryScreens(dpy, &nummons);
    mons = safe_malloc(nummons * sizeof(Monitor), "Failed to allocate memory for monitors");
    for (i = 0; i < nummons; ++i)
        mons[i] = create_mon(info[i].x_org, info[i].y_org, info[i].width, info[i].height);
    XFree(info);

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

static void
unmapnotify(XEvent *e) {
    XUnmapEvent *ev;
    Client *c;

    ev = &e->xunmap;
    c = find_client(ev->window);
    if (!c) {
        puts("Unable to find matching client.");
        return;
    }

    remove_client(c);
}

int main(int argc, char** argv) {
    init();
    run();
    clear_up();
    return 0;
}
