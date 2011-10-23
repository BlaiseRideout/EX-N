/* EXN is written by John Anthony <johnanthony@lavabit.com>, 2011.
 *
 * This software is in the public domain
 * and is provided AS IS, with NO WARRANTY.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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
static void win_nextprev_mon(char action);
static void next_space(void);
static void prev_space(void);
static void win_next_space(void);
static void win_prev_space(void);
static void win_nextprev_space(char action);
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
static void buttonpress(XEvent *e);
static void keypress(XEvent *e);
static void maprequest(XEvent *e);
static Client* new_client(Window w);
static void nohandler(XEvent *e);
static void remove_client(Client *c);
static void run(void);
static void* safe_malloc(unsigned int sz, char *errmsg);
static void unmapnotify(XEvent *e);
static void (*handler[LASTEvent]) (XEvent *) = {
	[ButtonPress] = buttonpress,
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
static int curspace;

#include "config.h"

static void
adjust_focus(void) {
    Monitor *m = &mons[currmon];

    if (!m->clients[curspace]) {
        XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
        return;
    }

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
    win_nextprev_mon(0);
}

static void
win_next_mon(void) {
    win_nextprev_mon(1);
}

static void
win_nextprev_mon(char action) {
    Monitor *m;
    Client *c;

    m = &mons[currmon];

    if(!m->clients[curspace])
        return;

    c = new_client(m->clients[curspace]->win);
    remove_client(m->clients[curspace]);

    if(action)
        ex_focus_next_mon();
    else
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
next_space(void) {
    unsigned int i;

    for(i = 0; i < nummons; ++i)
        hide(&mons[i]);

    curspace++;
    if (curspace >= numspaces) {
        if(WRAP)
            curspace = 0;
        else
            curspace = numspaces - 1;
    }

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
    if (curspace < 0) {
        if(WRAP)
            curspace = numspaces - 1;
        else
            curspace = 0;
    }

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
    win_nextprev_space(1);
}

static void
win_prev_space(void) {
    win_nextprev_space(0);
}

static void
win_nextprev_space(char action) {
    Monitor *m;
    Client *c;

    m = &mons[currmon];

    if(!m->clients[curspace])
        return;

    c = new_client(m->clients[curspace]->win);
    remove_client(m->clients[curspace]);

    if(action)        //if argument is non-zero, go to next space
        next_space();
    else
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
    XGrabButton(dpy, 1, MODKEY, root, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
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
    if (currmon < 0) {
      if(WRAP)
        currmon = nummons - 1;
      else
        currmon = 0;
   }

    adjust_focus();
}

static void
ex_focus_next_mon(void) {
  currmon++;
  if (currmon >= nummons) {
    if(WRAP)
      currmon = 0;
    else
      currmon = nummons - 1;
  }

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
    Monitor *m;
    Client *c;

    for (i = 0; i < nummons; ++i) {
        m = &mons[i];
        for(j = 0; j < numspaces; ++j) {
            c = m->clients[j];
            if(c)
                while(c->prev)
                    c = c->prev;
            while(c) {
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
    wa.event_mask = SubstructureRedirectMask|SubstructureNotifyMask//|ButtonPressMask
        |EnterWindowMask|LeaveWindowMask|StructureNotifyMask
        |PropertyChangeMask;
    XChangeWindowAttributes(dpy, root, CWEventMask|CWCursor, &wa);
    XSelectInput(dpy, root, wa.event_mask);

    running = 1;
    XSync(dpy, False);

    assign_keys();
}

static void
buttonpress(XEvent *e) {
    unsigned int i;
    XKeyEvent *ev;

    ev = &e->xkey;

    FILE *file = fopen("/home/aoeu/test.txt", "a");
    fprintf(file, "Caught buttonpress\n");
    fclose(file);

    if(ev->subwindow == root)
        return;

    for(i = 0; i < nummons; ++i) {
        if(ev->subwindow == mons[i].clients[curspace]->win) {
            currmon = i;
            adjust_focus();
            break;
        }
    }
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
    XSetWindowAttributes swa;
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
//    swa.event_mask = ButtonPressMask;
//    XChangeWindowAttributes(dpy, c->win, CWEventMask, &swa);
//    XSelectInput(dpy, c->win, swa.event_mask);
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
                mons[i].clients[j] = c->next;
                break;
            }


    if(!c->next)
        return;

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
