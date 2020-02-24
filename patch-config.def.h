--- config.def.h.orig	2019-02-02 12:55:28 UTC
+++ config.def.h
@@ -3,6 +3,11 @@
 /* appearance */
 static const unsigned int borderpx  = 1;        /* border pixel of windows */
 static const unsigned int snap      = 32;       /* snap pixel */
+static const unsigned int gappx     = 6;        /* gap pixel between windows */
+static const unsigned int systraypinning = 0;   /* 0: sloppy systray follows selected monitor, >0: pin systray to monitor X */
+static const unsigned int systrayspacing = 2;   /* systray spacing */
+static const int systraypinningfailfirst = 1;   /* 1: if pinning fails, display systray on the first monitor, False: display systray on the last monitor*/
+static const int showsystray        = 1;        /* 0 means no systray */
 static const int showbar            = 1;        /* 0 means no bar */
 static const int topbar             = 1;        /* 0 means bottom bar */
 static const char *fonts[]          = { "monospace:size=10" };
@@ -36,11 +41,15 @@ static const float mfact     = 0.55; /* factor of mast
 static const int nmaster     = 1;    /* number of clients in master area */
 static const int resizehints = 1;    /* 1 means respect size hints in tiled resizals */
 
+#include "layouts.c"
 static const Layout layouts[] = {
 	/* symbol     arrange function */
 	{ "[]=",      tile },    /* first entry is default */
 	{ "><>",      NULL },    /* no layout function means floating behavior */
 	{ "[M]",      monocle },
+	{ "TTT",      bstack },
+	{ "===",      bstackhoriz },
+	{ "H[]",      deck },
 };
 
 /* key definitions */
@@ -76,6 +85,9 @@ static Key keys[] = {
 	{ MODKEY,                       XK_t,      setlayout,      {.v = &layouts[0]} },
 	{ MODKEY,                       XK_f,      setlayout,      {.v = &layouts[1]} },
 	{ MODKEY,                       XK_m,      setlayout,      {.v = &layouts[2]} },
+	{ MODKEY,                       XK_u,      setlayout,      {.v = &layouts[3]} },
+	{ MODKEY,                       XK_o,      setlayout,      {.v = &layouts[4]} },
+	{ MODKEY,                       XK_c,      setlayout,      {.v = &layouts[5]} },
 	{ MODKEY,                       XK_space,  setlayout,      {0} },
 	{ MODKEY|ShiftMask,             XK_space,  togglefloating, {0} },
 	{ MODKEY,                       XK_0,      view,           {.ui = ~0 } },
