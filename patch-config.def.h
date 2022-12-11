--- config.def.h.OG	2022-12-10 18:37:51.502199000 -0800
+++ config.def.h	2022-12-10 18:42:32.448550000 -0800
@@ -2,7 +2,13 @@
 
 /* appearance */
 static const unsigned int borderpx  = 1;        /* border pixel of windows */
+static const unsigned int gappx     = 6;        /* gaps between windows */
 static const unsigned int snap      = 32;       /* snap pixel */
+static const unsigned int systraypinning = 0;   /* 0: sloppy systray follows selected monitor, >0: pin systray to monitor X */
+static const unsigned int systrayonleft = 0;   	/* 0: systray in the right corner, >0: systray on left of status text */
+static const unsigned int systrayspacing = 2;   /* systray spacing */
+static const int systraypinningfailfirst = 1;   /* 1: if pinning fails, display systray on the first monitor, False: display systray on the last monitor*/
+static const int showsystray        = 1;     /* 0 means no systray */
 static const int showbar            = 1;        /* 0 means no bar */
 static const int topbar             = 1;        /* 0 means bottom bar */
 static const char *fonts[]          = { "monospace:size=10" };
@@ -34,7 +40,7 @@
 /* layout(s) */
 static const float mfact     = 0.55; /* factor of master area size [0.05..0.95] */
 static const int nmaster     = 1;    /* number of clients in master area */
-static const int resizehints = 1;    /* 1 means respect size hints in tiled resizals */
+static const int resizehints = 0;    /* 1 means respect size hints in tiled resizals */
 static const int lockfullscreen = 1; /* 1 will force focus on the fullscreen window */
 
 static const Layout layouts[] = {
@@ -42,6 +48,9 @@
 	{ "[]=",      tile },    /* first entry is default */
 	{ "><>",      NULL },    /* no layout function means floating behavior */
 	{ "[M]",      monocle },
+	{ "TTT",      bstack },
+	{ "===",      bstackhoriz },
+	{ "[D]",      deck },
 };
 
 /* key definitions */
@@ -77,6 +86,9 @@
 	{ MODKEY,                       XK_t,      setlayout,      {.v = &layouts[0]} },
 	{ MODKEY,                       XK_f,      setlayout,      {.v = &layouts[1]} },
 	{ MODKEY,                       XK_m,      setlayout,      {.v = &layouts[2]} },
+	{ MODKEY,                       XK_u,      setlayout,      {.v = &layouts[3]} },
+	{ MODKEY,                       XK_o,      setlayout,      {.v = &layouts[4]} },
+	{ MODKEY,                       XK_r,      setlayout,      {.v = &layouts[5]} },
 	{ MODKEY,                       XK_space,  setlayout,      {0} },
 	{ MODKEY|ShiftMask,             XK_space,  togglefloating, {0} },
 	{ MODKEY,                       XK_0,      view,           {.ui = ~0 } },
@@ -101,8 +113,8 @@
 /* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
 static Button buttons[] = {
 	/* click                event mask      button          function        argument */
-	{ ClkLtSymbol,          0,              Button1,        setlayout,      {0} },
-	{ ClkLtSymbol,          0,              Button3,        setlayout,      {.v = &layouts[2]} },
+	{ ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
+	{ ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
 	{ ClkWinTitle,          0,              Button2,        zoom,           {0} },
 	{ ClkStatusText,        0,              Button2,        spawn,          {.v = termcmd } },
 	{ ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
