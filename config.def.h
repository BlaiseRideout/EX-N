#define MODKEY Mod4Mask

static Key keys[] = { 
    {MODKEY,            XK_Left,        monfoc,     {.i = -1}   },  
    {MODKEY|ShiftMask,  XK_Left,        monmove,    {.i = -1}   },  
    {MODKEY,            XK_Right,       monfoc,     {.i = 1}    },  
    {MODKEY|ShiftMask,  XK_Right,       monmove,    {.i = 1}    },  
    {MODKEY,            XK_Tab,         cyclewin,   {.i = 1}    },  
    {MODKEY|ShiftMask,  XK_Tab,         cyclewin,   {.i = -1},  },  
    {MODKEY|ShiftMask,  XK_q,           quit,       {0}         },  
    {Mod1Mask,          XK_F4,          killclient, {0}         },  
};


