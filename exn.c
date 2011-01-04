/* EXN is written by John Anthony <john_anthony@kingkill.com>, 2010.
 *
 * This software is in the public domain
 * and is provided AS IS, with NO WARRANTY.
 * 
 * Extra special thanks go to the dwm engineers for inspiration and
 * both the dwm engineers (from whom I stole a fair few chunks of code)
 * and Nick Welch (author of TinyWM) for providing such great learning
 * material -- without whom I would probably still be looking through
 * obscure documentation. */

#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif

#include <stdio.h>

/* Macros */
#define CLEANMASK(mask)         (mask & ~(numlockmask|LockMask))
#define LENGTH(X)               (sizeof X / sizeof X[0])
#define MAX(a, b)               ((a) > (b) ? (a) : (b))

#define NUMMONS         2

/* Datatypes */
typedef union {
    int i;
    unsigned int ui;
    float f;
    const void *v;
} Arg; 

typedef struct Monitor Monitor;
typedef struct Client Client;
struct Client{
    Window win;
    Monitor *parent;
    Client *prev, *next;
};

struct Monitor{
    int x, y, w, h;
    Client *first, *last, *current;
};

typedef struct {
    unsigned int mod;
    KeySym keysym;
    void (*func)(const Arg *);
    const Arg arg;
} Key;

/* Predecs */
static void attachwindow(Monitor *m, Window w);
static void cyclewin(const Arg *arg);
static void destroynotify(XEvent *e);
static void detachwindow(Window w);
static Client* findclient(Window w);
static void grabkeys(void);
static void initmons(void);
static void keypress(XEvent *e);
static void killclient(const Arg *arg);
static void refocus(void);
static void quit(const Arg *arg);
static void mapnotify(XEvent *e);
static void monfoc(const Arg *arg);
static void monmove(const Arg *arg);
static void updatenumlockmask(void);

/* Variables */
static unsigned int numlockmask = 0;
static int selmon = 1;
static void (*handler[LASTEvent]) (XEvent *) = {
/*     [ButtonPress] = XXXXX, */
/*     [ClientMessage] = XXXXX, */
/*     [ConfigureRequest] = XXXXX, */
/*     [ConfigureNotify] = XXXXX, */
    [DestroyNotify] = destroynotify,
/*     [EnterNotify] = XXXXX, */
/*     [Expose] = XXXXX, */
/*     [FocusIn] = XXXXX, */
    [KeyPress] = keypress,
    [MapNotify] = mapnotify,
/*     [MapRequest] = XXXXX, */
/*     [PropertyNotify] = XXXXX, */
/*     [UnmapNotify] = XXXXX */
};
static Bool running = True;
static Display *dpy;
static Monitor mons[NUMMONS];
static Window root;

/* Config file goes here */
#include "config.h"

void
attachwindow(Monitor* m, Window w) {

    Client *c, *find;

    c = (Client *)malloc(sizeof(Client));
    c->win = w;
    c->next = NULL;
    c->parent = m;

    if (!m->first) {
        m->first = c;
        c->prev = NULL;
    }
    else {
        for (find = m->first; find->next; find = find->next);
        find->next = c;
        c->prev = find;
    }

    m->last = c;
    m->current = c;
}

void
cyclewin(const Arg *arg) {

    if (arg->i > 0) {
        if(mons[selmon].current->next)
            mons[selmon].current = mons[selmon].current->next;
        else
            mons[selmon].current = mons[selmon].first;
    }
    else {
        if(mons[selmon].current->prev)
            mons[selmon].current = mons[selmon].current->prev;
        else
            mons[selmon].current = mons[selmon].last;
    }
   
    refocus();
}

void
destroynotify(XEvent *e) {
    XDestroyWindowEvent *ev = &e->xdestroywindow;
    Window w = ev->window;
    Client *c = findclient(w);

    if (c) {
        detachwindow(c->win);
        free(c);
    }
    
    refocus();
}

void
detachwindow(Window w) {
    Client *c = findclient(w);
    Monitor *m = c->parent;

    if (!c)
        return;

    if (c->next)
        c->next->prev = c->prev;
    else                                /* Deal with the eventuality that it could be the last in the list */
        m->last = c->prev;

    if (c->prev) {
        m->current = c->prev;
        c->prev->next = c->next;
    }
    else {                               /* Deal with the eventuality of it being the first in the list */
        m->first = c->next;
        m->current = m->first;
    }
}

Client *
findclient(Window w) {
    unsigned int i;
    Client *c;

    for (i = 0; i < NUMMONS; i++) {
        for (c = mons[i].first; c; c = c->next) {
            if (c->win == w)
                return c;
        }
    }
    return NULL;
}

void
grabkeys(void) {
    updatenumlockmask();
        unsigned int i, j;
        unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
        KeyCode code;

        XUngrabKey(dpy, AnyKey, AnyModifier, root);
        for(i = 0; i < LENGTH(keys); i++) {
            if((code = XKeysymToKeycode(dpy, keys[i].keysym)))
                for(j = 0; j < LENGTH(modifiers); j++)
                    XGrabKey(dpy, code, keys[i].mod | modifiers[j], root,
                            True, GrabModeAsync, GrabModeAsync);
        }
}

void
initmons(void) {    /* Needs to be generalised using Xinerama */

    mons[0].x = 0;
    mons[0].y = 0;
    mons[0].w = 1280;
    mons[0].h = 1024;
    mons[1].x = 1280;
    mons[1].y = 0;
    mons[1].w = 1920;
    mons[1].h = 1080;

    mons[0].first = NULL;
    mons[0].last = NULL;
    mons[0].current = NULL;

    mons[1].first = NULL;
    mons[1].last = NULL;
    mons[1].current = NULL;
}

void
keypress(XEvent *e) {
    unsigned int i;
    KeySym keysym;
    XKeyEvent *ev; 

    ev = &e->xkey;
    keysym = XKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0);
    for(i = 0; i < LENGTH(keys); i++) 
        if(keysym == keys[i].keysym
                && CLEANMASK(keys[i].mod) == CLEANMASK(ev->state)
                && keys[i].func)
            keys[i].func(&(keys[i].arg));

}

void
killclient(const Arg *arg) {

    if (!mons[selmon].current)
        return;

    /* OUT OF ACTION until window atoms are set up correctly! */

    /* Window w = mons[selmon].current->win; */

    /* detachwindow(&mons[selmon], w); */
    /* refocus(); */
    /* XKillClient(dpy, w); */
}

void
mapnotify(XEvent *e) {
    Window w = e->xmap.window;
    attachwindow(&mons[selmon], w);
    XMoveResizeWindow(dpy, w, mons[selmon].x, mons[selmon].y, mons[selmon].w, mons[selmon].h);
    refocus();
}

void
monmove(const Arg *arg) {

    Window w = mons[selmon].current->win;
    Monitor *new, *old = &mons[selmon];
    monfoc(arg);
    new = &mons[selmon];
    if (new != old) {
        detachwindow(w);
        attachwindow(new, w);
        XMoveResizeWindow(dpy, w, mons[selmon].x, mons[selmon].y, mons[selmon].w, mons[selmon].h);
        refocus();
    }
}

void
monfoc(const Arg *arg) {
    int n = selmon + arg->i;
    if (n >= 0 && n < NUMMONS) {
        selmon = n;
        if (mons[selmon].current){
            refocus();
        }
    }
}

void
refocus(void) {
    /* Safety */
    if (!mons[selmon].current)
        mons[selmon].current = mons[selmon].first;

    if (mons[selmon].current) {
        XSetInputFocus(dpy, mons[selmon].current->win, RevertToPointerRoot, CurrentTime);
        XRaiseWindow(dpy, mons[selmon].current->win);
    }
}

void
quit(const Arg *arg) {
    running = False;
}

void
updatenumlockmask(void) {
    unsigned int i, j;
    XModifierKeymap *modmap;

    numlockmask = 0;
    modmap = XGetModifierMapping(dpy);
    for(i = 0; i < 8; i++)
        for(j = 0; j < modmap->max_keypermod; j++)
            if(modmap->modifiermap[i * modmap->max_keypermod + j] == XKeysymToKeycode(dpy, XK_Num_Lock))
                numlockmask = (1 << i);
    XFreeModifiermap(modmap);
}

int main()
{
    XEvent ev;
    XSetWindowAttributes wa;

    if(!(dpy = XOpenDisplay(0x0))) return 1;
    root = DefaultRootWindow(dpy);

    initmons();

    wa.cursor = XCreateFontCursor(dpy, XC_left_ptr);
    /* wa.event_mask = SubstructureRedirectMask|SubstructureNotifyMask|ButtonPressMask|EnterWindowMask|LeaveWindowMask|StructureNotifyMask|PropertyChangeMask; */
    wa.event_mask = SubstructureNotifyMask|ButtonPressMask|EnterWindowMask|LeaveWindowMask|StructureNotifyMask|PropertyChangeMask;
    XChangeWindowAttributes(dpy, root, CWEventMask|CWCursor, &wa);
    XSelectInput(dpy, root, wa.event_mask);
    grabkeys();
    XSync(dpy, False);

    /* Main loop */
    while(running && !XNextEvent(dpy, &ev)) {
        if(handler[ev.type])
            handler[ev.type](&ev);
    }

}
