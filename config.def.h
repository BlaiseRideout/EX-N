#define MODKEY Mod4Mask
#define WRAP 1

static void spring(void)  {
    char *args[] = { "spring", NULL };
    start_stuff(args);
}
static void term(void) { 
    char *args[] = { "urxvt", NULL };
    start_stuff(args);
}
static void browser(void) { 
    char *args[] = { "chromium", NULL };
    start_stuff(args);
}
static void lock(void) { 
    char *args[] = { "xscreensaver-command", "--lock", NULL };
    start_stuff(args);
}
static void volup(void) {
    char *args[] = { "amixer", "sset", "Master", "5+", NULL };
    start_stuff(args);
}
static void voldown(void) {
    char *args[] = { "amixer", "sset", "Master", "5-", NULL };
    start_stuff(args);
}

static Key keys[] = {
    { MODKEY,                         XK_Right,                 ex_focus_next_mon },
    { MODKEY,                         XK_Left,                  ex_focus_prev_mon },
    { MODKEY|ShiftMask,               XK_Right,                 win_next_mon },
    { MODKEY|ShiftMask,               XK_Left,                  win_prev_mon },

    { Mod1Mask|ControlMask,           XK_Right,                 next_space },
    { Mod1Mask|ControlMask,           XK_Left,                  prev_space },
    { Mod1Mask|ControlMask|ShiftMask, XK_Right,                 win_next_space },
    { Mod1Mask|ControlMask|ShiftMask, XK_Left,                  win_prev_space },


    { MODKEY,                         XK_q,                     ex_kill_client },
    { MODKEY|ShiftMask,               XK_q,                     ex_end_session },
    { MODKEY,                         XK_Tab,                   next_window },
    { MODKEY|ShiftMask,               XK_Tab,                   prev_window },

    { MODKEY,                         XK_t,                     term },
    { MODKEY,                         XK_r,                     spring },
    { MODKEY,                         XK_w,                     browser },
    { MODKEY,                         XK_l,                     lock },
    { 0,                              XF86XK_AudioRaiseVolume,  volup },
    { 0,                              XF86XK_AudioLowerVolume,  voldown },
};
