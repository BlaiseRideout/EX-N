#define MODKEY Mod4Mask

static Key keys[] = {
    { MODKEY,               XK_Left,            ex_focus_monitor_up },
    { MODKEY,               XK_Right,           ex_focus_monitor_down },
    { MODKEY,               XK_q,               ex_kill_client },
    { MODKEY|ShiftMask,     XK_q,               ex_end_session },
};
