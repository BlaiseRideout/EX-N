/* EXN is written by John Anthony <johnanthony@lavabit.com>, 2011.
 *
 * This software is in the public domain
 * and is provided AS IS, with NO WARRANTY.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <X11/cursorfont.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/Xinerama.h>

#define LENGTH(X)   ( sizeof X / sizeof X[0] )

#define numspaces 4

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
    unsigned int x, y, width, height;
    Client *clients[numspaces];
} Monitor;

static void adjust_focus(void);
static void start_stuff(const char *name, const char *arg);
static void next_window(void);
static void prev_window(void);
static void win_prev_mon(void);
static void win_next_mon(void);
static void next_space(void);
static void prev_space(void);
static void win_next_space(void);
static void win_prev_space(void);
static void hide(Monitor *m);
static void show(Monitor *m);
static void assign_keys(void);
static void clear_up(void);
static Monitor create_mon(unsigned int x, unsigned int y, unsigned int w, unsigned int h);
static void destroynotify(XEvent *e);
static void errout(char *msg);
static void ex_end_session(void);
static void ex_focus_prev_mon(void);
static void ex_focus_next_mon(void);
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

static unsigned int tw, th;
static Display *dpy;
static int screen;
static Window root;
static Monitor *mons;
static char running;
static int nummons;
static int currmon;
static unsigned int curspace;

#include "config.h"

static void
adjust_focus(void) {
    Monitor *m = &mons[currmon];

    if (!m->clients[curspace])
        return;

    XRaiseWindow(dpy, m->clients[curspace]->win);
    XSetInputFocus(dpy, m->clients[curspace]->win, RevertToPointerRoot, CurrentTime);
}

static void
start_stuff(const char *name, const char *arg) {
    if(fork() == 0) {
        if(dpy)
            close(ConnectionNumber(dpy));
        setsid();
        execlp(name, name, arg, (char *)NULL);
        fprintf(stderr, "EX^N: execlp %s failed\n", name);
        exit(0);
    }
}

static void
next_window(void) {
    Monitor *m;
    Client *c;

    m = &mons[currmon];

    if(m->clients[curspace]) {
        c = m->clients[curspace];

        if(c->next)
            c = c->next;
        else if(c->prev){
            while(c->prev)
                c = c->prev;
        }

        if(c)
            m->clients[curspace] = c;
    }

    adjust_focus();
}

static void
prev_window(void) {
    Monitor *m;
    Client *c;

    m = &mons[currmon];

    if(m->clients[curspace]) {
        c = m->clients[curspace];

        if(c->prev)
            c = c->prev;
        else if(c->next){
            while(c->next)
                c = c->next;
        }

        if(c)
            m->clients[curspace] = c;
    }

    adjust_focus();
}

static void
win_prev_mon(void) {
    Monitor *m;
    Client *c;

    m = &mons[currmon];

    if(!m->clients[curspace])
        return;

    c = new_client(m->clients[curspace]->win);
    remove_client(m->clients[curspace]);

    ex_focus_prev_mon();

    m = &mons[currmon];

    if(m->clients[curspace]) {
        c->next = m->clients[curspace];
        m->clients[curspace]->prev = c;
    }
    else
        c->next = NULL;
    m->clients[curspace] = c;

    XMoveResizeWindow(dpy, c->win, m->x, m->y, m->width, m->height);
    XMapWindow(dpy, c->win);
    adjust_focus();
}

static void
win_next_mon(void) {
    Monitor *m;
    Client *c;

    m = &mons[currmon];

    if(!m->clients[curspace])
        return;

    c = new_client(m->clients[curspace]->win);
    remove_client(m->clients[curspace]);

    ex_focus_next_mon();

    m = &mons[currmon];

    if(m->clients[curspace]) {
        c->next = m->clients[curspace];
        m->clients[curspace]->prev = c;
    }
    else
        c->next = NULL;
    m->clients[curspace] = c;

    XMoveResizeWindow(dpy, c->win, m->x, m->y, m->width, m->height);
    XMapWindow(dpy, c->win);
    adjust_focus();
}

static void
next_space(void) {
    unsigned int i;

    for(i = 0; i < nummons; ++i)
          hide(&mons[i]);

    curspace++;
    if (curspace >= numspaces)
        curspace = 0;

    for(i = 0; i < nummons; ++i)
        show(&mons[i]);

    adjust_focus();
}

static void
prev_space(void) {
    unsigned int i;

    for(i = 0; i < nummons; ++i)
        hide(&mons[i]);

    curspace--;
    if (curspace < 0)
        curspace = numspaces - 1;

    for(i = 0; i < nummons; ++i)
        show(&mons[i]);

    adjust_focus();
}

static void
show(Monitor *m) {
    Client *c;

    if(!m)
        return;

    c = m->clients[curspace];

    if(!c)
        return;

    while(c->next)
        c = c->next;

    while(c) {
        XMoveWindow(dpy, c->win, m->x, m->y);
        c = c->prev;
    }
}

static void
hide(Monitor *m) {
    Client *c;

    if(!m)
        return;

    c = m->clients[curspace];

    if(!c)
        return;

    while(c->next)
        c = c->next;

    while(c) {
        XMoveWindow(dpy, c->win, tw * 2, th * 2);
        c = c->prev;
    }
}

static void
win_next_space(void) {
    Monitor *m;
    Client *c;

    m = &mons[currmon];

    if(!m->clients[curspace])
        return;

    c = new_client(m->clients[curspace]->win);
    remove_client(m->clients[curspace]);

    next_space();

    m = &mons[currmon];

    if(m->clients[curspace]) {
        c->next = m->clients[curspace];
        m->clients[curspace]->prev = c;
    }
    else
        c->next = NULL;
    m->clients[curspace] = c;

    XMoveResizeWindow(dpy, c->win, m->x, m->y, m->width, m->height);
    XMapWindow(dpy, c->win);
    adjust_focus();
}

static void
win_prev_space(void) {
    Monitor *m;
    Client *c;

    m = &mons[currmon];

    if(!m->clients[curspace])
        return;

    c = new_client(m->clients[curspace]->win);
    remove_client(m->clients[curspace]);

    prev_space();

    m = &mons[currmon];

    if(m->clients[curspace]) {
        c->next = m->clients[curspace];
        m->clients[curspace]->prev = c;
    }
    else
        c->next = NULL;
    m->clients[curspace] = c;

    XMoveResizeWindow(dpy, c->win, m->x, m->y, m->width, m->height);
    XMapWindow(dpy, c->win);
    adjust_focus();
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

    tw += w;
    th += h;

    for(int i = 0; i < numspaces; ++i)
        m.clients[i] = NULL;

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
ex_focus_prev_mon(void) {
    currmon--;
    if (currmon < 0)
        currmon = nummons - 1;

    adjust_focus();
}

static void
ex_focus_next_mon(void) {
    currmon++;
    if (currmon >= nummons)
        currmon = 0;

    adjust_focus();
}

static void
ex_kill_client(void) {
    Monitor *m = &mons[currmon];

    if(!m->clients[curspace])
        return;

    XDestroyWindow(dpy, m->clients[curspace]->win);
}

static Client*
find_client(Window w) {
    unsigned int i, j;
    Client *c;

    for (i = 0; i < nummons; ++i) {
        for(j = 0; j < numspaces; ++j) {
            c = mons[i].clients[j];
            if(c)
                while(c->prev)
                    c = c->prev;
            while (c) {
                if (c->win == w)
                    return c;
                c = c->next;
            }
        }
    }

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
    currmon = 0, curspace = 0;

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
    Monitor *m;
    Client *c;

    /* Don't manage if the window doesn't want to be managed */
    ev = &e->xmaprequest;
    if(!XGetWindowAttributes(dpy, ev->window, &wa))
        return;
    if(wa.override_redirect) {
        puts("Override redirect respected during map request. Window not managed.");
        return;
    }

    /* Don't manage if the window is already attached to a client */
    c = find_client(ev->window);
    if (c)
        return;

    /* Manage the new window */
    c = new_client(ev->window);
    m = &mons[currmon];

    if (m->clients[curspace]) {
        c->next = m->clients[curspace];
        m->clients[curspace]->prev = c;
    }
    else
        c->next = NULL;
    m->clients[curspace] = c;

    printf("Being asked to map window %d\n", (int)ev->window);

    if (m->clients[curspace])
        printf("Monitor %d now has client %d\n", currmon, (int)ev->window);

    XMoveResizeWindow(dpy, c->win, m->x, m->y, m->width, m->height);
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
    unsigned int i, j;

    /* Adjust the head of our monitors if necessary */
    for(j = 0; j < numspaces; ++j)
        for (i = 0; i < nummons; ++i)
            if (mons[i].clients[j] == c) {
                mons[i].clients[j] = mons[i].clients[j]->next;
                break;
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
            handler[ev.type](&ev);
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
