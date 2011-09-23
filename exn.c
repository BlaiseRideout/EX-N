/* EXN is written by John Anthony <johnanthony@lavabit.com>, 2011.
 *
 * This software is in the public domain
 * and is provided AS IS, with NO WARRANTY.
 */

#include <stdio.h>
#include <stdlib.h>
#include <X11/cursorfont.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/Xinerama.h>

#define LENGTH(X)   ( sizeof X / sizeof X[0] )

typedef struct Client Client;
struct Client {
    Window win;
    Client *next, *prev;
};

typedef struct {
    unsigned int mod;
    KeySym keysym;
    void (*func)(void);
} Key;

typedef struct {
    unsigned int x;
    unsigned int y;
    unsigned int width;
    unsigned int height;
    Client *clients;
} Monitor;

static void adjust_focus(void);
static void assign_keys(void);
static void clear_up(void);
static Monitor create_mon(unsigned int x, unsigned int y, unsigned int w, unsigned int h);
static void destroynotify(XEvent *e);
static void errout(char *msg);
static void ex_end_session(void);
static void ex_focus_monitor_down(void);
static void ex_focus_monitor_up(void);
static void ex_kill_client(void);
static Client* find_client(Window w);
static void init(void);
static void keypress(XEvent *e);
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
	[DestroyNotify] = destroynotify,
	[EnterNotify] = nohandler,
	[Expose] = nohandler,
	[FocusIn] = nohandler,
	[KeyPress] = keypress,
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
static Atom msgtype;
static Atom killmsg;

#include "config.h"

static void
adjust_focus(void) {
    Window w;
    int i;

    if (!mons[currmon].clients)
        return;

    XGetInputFocus(dpy, &w, &i);
    if (w == mons[currmon].clients->win)
        return;

    printf("Shifting focus from %d to %d\n", (int)w, (int)mons[currmon].clients->win);

    XRaiseWindow(dpy, mons[currmon].clients->win);
    XSetInputFocus(dpy, mons[currmon].clients->win, RevertToPointerRoot, CurrentTime);
}

static void
assign_keys(void) {
    unsigned int i;
    /* unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask | Lockmask }; */
    KeyCode code;

    for (i = 0; i < LENGTH(keys); ++i) {
        code = XKeysymToKeycode(dpy, keys[i].keysym);
        if (code)
            XGrabKey(dpy, code, keys[i].mod, root, True, GrabModeAsync, GrabModeAsync);
    }
}

static void
clear_up(void) {
    XCloseDisplay(dpy);
}

static Monitor
create_mon(unsigned int x, unsigned int y, unsigned int w, unsigned int h) {
    Monitor m;

    m.x = x;
    m.y = y;
    m.width = w;
    m.height = h;

    m.clients = NULL;

    return m;
}

static void
destroynotify(XEvent *e) {
    Client *c;
    XDestroyWindowEvent *ev;
    
    ev = &e->xdestroywindow;
    c = find_client(ev->window);
    if (!c) {
        printf("Asked to destroy an unmanaged window: %d\n", (int)ev->window);
        return;
    }

    printf("Asked to destroy managed window: %d\n", (int)ev->window);

    remove_client(c);
}

static void
errout(char* msg) {
    if (msg)
        puts(msg);
    exit(1);
}

static void
ex_end_session(void) {
    running = 0;
}

static void
ex_focus_monitor_down(void) {
    currmon--;
    if (currmon < 0)
        currmon = 0;

    adjust_focus();
}

static void
ex_focus_monitor_up(void) {
    currmon++;
    if (currmon >= nummons)
        currmon = nummons - 1;

    adjust_focus();
}

static void
ex_kill_client(void) {
    XEvent ev;

    if(!mons[currmon].clients)
        return;

    ev.type = ClientMessage;
    ev.xclient.window = mons[currmon].clients->win;
    ev.xclient.message_type = msgtype;
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = killmsg;
    ev.xclient.data.l[1] = CurrentTime;
    XSendEvent(dpy, mons[currmon].clients->win, False, NoEventMask, &ev);
}

static Client*
find_client(Window w) {
    unsigned int i;
    Client *c;

    for (i = 0; i < nummons; ++i)
        for (c = mons[i].clients; c; c = c->next)
            if (c->win == w)
                return c;

    /* Debug stuff begins */
    printf("find_client faild to match up window %d\n", (int)w);
    puts("Available windows are:");

    for (i = 0; i < nummons; ++i) {
        printf("Monitor %d:\n", i);
        for (c = mons[i].clients; c; c = c->next) {
            printf("Client window: %d\n", (int)c->win);
        }
    }
    /* Debug stuff ends */

    return NULL;
}

static void
init(void) {
    XineramaScreenInfo *info;
    XSetWindowAttributes wa;
    unsigned int i;

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

    msgtype = XInternAtom(dpy, "WM_PROTOCOLS", False);
    killmsg = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
 
    wa.cursor = XCreateFontCursor(dpy, XC_left_ptr);
    wa.event_mask = SubstructureRedirectMask|SubstructureNotifyMask|ButtonPressMask
        |EnterWindowMask|LeaveWindowMask|StructureNotifyMask
        |PropertyChangeMask;
    XChangeWindowAttributes(dpy, root, CWEventMask|CWCursor, &wa);
    XSelectInput(dpy, root, wa.event_mask);

    running = 1;
    XSync(dpy, False);

    assign_keys();
}

static void
keypress(XEvent *e) {
    unsigned int i;
    KeySym keysym;
    XKeyEvent *ev;

    ev = &e->xkey;
    keysym = XKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0);

    for (i = 0; i < LENGTH(keys); ++i)
        if (keysym == keys[i].keysym)
            if (keys[i].mod == ev->state)
                if (keys[i].func) {
                    keys[i].func();
                    return;
                }
}

static void
maprequest(XEvent *e) {
    XMapRequestEvent *ev;
    XWindowAttributes wa;
    Client *c;

    ev = &e->xmaprequest;
    if(!XGetWindowAttributes(dpy, ev->window, &wa))
        return;
    if(wa.override_redirect)
        return;

    c = new_client(ev->window);

    if (mons[currmon].clients) {
        c->next = mons[currmon].clients;
        mons[currmon].clients->prev = c;
    }
    else
        c->next = NULL;
    mons[currmon].clients = c;

    printf("Being asked to map window %d\n", (int)ev->window);

    if (mons[currmon].clients)
        printf("Monitor %d now has client %d\n", currmon, (int)ev->window);

    XMoveResizeWindow(dpy, c->win, mons[currmon].x, mons[currmon].y, mons[currmon].width, mons[currmon].height);
    XMapWindow(dpy, c->win);
    XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
}

static Client*
new_client(Window w) {
    Client *c;

    c = safe_malloc(sizeof(Client), "Failed to malloc space for client details.");
    c->win = w;
    c->prev = NULL;

    return c;
}

static void
nohandler(XEvent *e) {
}

static void
remove_client(Client *c) {
    unsigned int i;

    /* Adjust the head of our monitors if necessary */
    for (i = 0; i < nummons; ++i) {
        if (mons[i].clients == c) {
            mons[i].clients = mons[i].clients->next;
            break;
        }
    }

    if (c->prev)
        c->prev->next = c->next;
    if (c->next)
        c->next->prev = c->prev;

    adjust_focus();

    free(c);
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
    XUnmapEvent *ev = &e->xunmap;
    printf("Being asked to UNmap window %d\n", (int)ev->window);
}

int main(int argc, char** argv) {
    init();
    run();
    clear_up();
    return 0;
}
