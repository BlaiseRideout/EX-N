#define MODKEY Mod4Mask

static void spring(void)  { start_stuff("spring"); }
static void term(void)    { start_stuff("urxvt"); }
static void browser(void) { start_stuff("chromium"); }

static Key keys[] = {
    { MODKEY,               XK_Right,           ex_focus_monitor_up },
    { MODKEY,               XK_Left,            ex_focus_monitor_down },
    { MODKEY,               XK_q,               ex_kill_client },
    { MODKEY|ShiftMask,     XK_q,               ex_end_session },
    { MODKEY,               XK_Tab,             next_window },
    { MODKEY|ShiftMask,     XK_Tab,             prev_window },

    { MODKEY,               XK_t,               term },
    { MODKEY,               XK_r,               spring },
    { MODKEY,               XK_w,               browser },
};
