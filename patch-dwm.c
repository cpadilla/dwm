--- dwm.c.orig	2019-02-02 12:55:28 UTC
+++ dwm.c
@@ -52,17 +52,35 @@
 #define ISVISIBLE(C)            ((C->tags & C->mon->tagset[C->mon->seltags]))
 #define LENGTH(X)               (sizeof X / sizeof X[0])
 #define MOUSEMASK               (BUTTONMASK|PointerMotionMask)
-#define WIDTH(X)                ((X)->w + 2 * (X)->bw)
-#define HEIGHT(X)               ((X)->h + 2 * (X)->bw)
+#define WIDTH(X)                ((X)->w + 2 * (X)->bw + gappx)
+#define HEIGHT(X)               ((X)->h + 2 * (X)->bw + gappx)
 #define TAGMASK                 ((1 << LENGTH(tags)) - 1)
 #define TEXTW(X)                (drw_fontset_getwidth(drw, (X)) + lrpad)
 
+#define SYSTEM_TRAY_REQUEST_DOCK    0
+
+/* XEMBED messages */
+#define XEMBED_EMBEDDED_NOTIFY      0
+#define XEMBED_WINDOW_ACTIVATE      1
+#define XEMBED_FOCUS_IN             4
+#define XEMBED_MODALITY_ON         10
+
+#define XEMBED_MAPPED              (1 << 0)
+#define XEMBED_WINDOW_ACTIVATE      1
+#define XEMBED_WINDOW_DEACTIVATE    2
+
+#define VERSION_MAJOR               0
+#define VERSION_MINOR               0
+#define XEMBED_EMBEDDED_VERSION (VERSION_MAJOR << 16) | VERSION_MINOR
+
 /* enums */
 enum { CurNormal, CurResize, CurMove, CurLast }; /* cursor */
 enum { SchemeNorm, SchemeSel }; /* color schemes */
 enum { NetSupported, NetWMName, NetWMState, NetWMCheck,
+       NetSystemTray, NetSystemTrayOP, NetSystemTrayOrientation, NetSystemTrayOrientationHorz,
        NetWMFullscreen, NetActiveWindow, NetWMWindowType,
        NetWMWindowTypeDialog, NetClientList, NetLast }; /* EWMH atoms */
+enum { Manager, Xembed, XembedInfo, XLast }; /* Xembed atoms */
 enum { WMProtocols, WMDelete, WMState, WMTakeFocus, WMLast }; /* default atoms */
 enum { ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle,
        ClkClientWin, ClkRootWin, ClkLast }; /* clicks */
@@ -111,6 +129,7 @@ typedef struct {
 	void (*arrange)(Monitor *);
 } Layout;
 
+typedef struct Pertag Pertag;
 struct Monitor {
 	char ltsymbol[16];
 	float mfact;
@@ -130,6 +149,7 @@ struct Monitor {
 	Monitor *next;
 	Window barwin;
 	const Layout *lt[2];
+	Pertag *pertag;
 };
 
 typedef struct {
@@ -141,12 +161,19 @@ typedef struct {
 	int monitor;
 } Rule;
 
+typedef struct Systray   Systray;
+struct Systray {
+	Window win;
+	Client *icons;
+};
+
 /* function declarations */
 static void applyrules(Client *c);
 static int applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact);
 static void arrange(Monitor *m);
 static void arrangemon(Monitor *m);
 static void attach(Client *c);
+static void attachbelow(Client *c);
 static void attachstack(Client *c);
 static void buttonpress(XEvent *e);
 static void checkotherwm(void);
@@ -169,8 +196,10 @@ static void focus(Client *c);
 static void focusin(XEvent *e);
 static void focusmon(const Arg *arg);
 static void focusstack(const Arg *arg);
+static Atom getatomprop(Client *c, Atom prop);
 static int getrootptr(int *x, int *y);
 static long getstate(Window w);
+static unsigned int getsystraywidth();
 static int gettextprop(Window w, Atom atom, char *text, unsigned int size);
 static void grabbuttons(Client *c, int focused);
 static void grabkeys(void);
@@ -188,13 +217,16 @@ static void pop(Client *);
 static void propertynotify(XEvent *e);
 static void quit(const Arg *arg);
 static Monitor *recttomon(int x, int y, int w, int h);
+static void removesystrayicon(Client *i);
 static void resize(Client *c, int x, int y, int w, int h, int interact);
+static void resizebarwin(Monitor *m);
 static void resizeclient(Client *c, int x, int y, int w, int h);
 static void resizemouse(const Arg *arg);
+static void resizerequest(XEvent *e);
 static void restack(Monitor *m);
 static void run(void);
 static void scan(void);
-static int sendevent(Client *c, Atom proto);
+static int sendevent(Window w, Atom proto, int m, long d0, long d1, long d2, long d3, long d4);
 static void sendmon(Client *c, Monitor *m);
 static void setclientstate(Client *c, long state);
 static void setfocus(Client *c);
@@ -206,6 +238,7 @@ static void seturgent(Client *c, int urg);
 static void showhide(Client *c);
 static void sigchld(int unused);
 static void spawn(const Arg *arg);
+static Monitor *systraytomon(Monitor *m);
 static void tag(const Arg *arg);
 static void tagmon(const Arg *arg);
 static void tile(Monitor *);
@@ -223,18 +256,25 @@ static int updategeom(void);
 static void updatenumlockmask(void);
 static void updatesizehints(Client *c);
 static void updatestatus(void);
+static void updatesystray(void);
+static void updatesystrayicongeom(Client *i, int w, int h);
+static void updatesystrayiconstate(Client *i, XPropertyEvent *ev);
 static void updatetitle(Client *c);
 static void updatewindowtype(Client *c);
 static void updatewmhints(Client *c);
 static void view(const Arg *arg);
 static Client *wintoclient(Window w);
 static Monitor *wintomon(Window w);
+static Client *wintosystrayicon(Window w);
 static int xerror(Display *dpy, XErrorEvent *ee);
 static int xerrordummy(Display *dpy, XErrorEvent *ee);
 static int xerrorstart(Display *dpy, XErrorEvent *ee);
 static void zoom(const Arg *arg);
+static void bstack(Monitor *m);
+static void bstackhoriz(Monitor *m);
 
 /* variables */
+static Systray *systray =  NULL;
 static const char broken[] = "broken";
 static char stext[256];
 static int screen;
@@ -257,9 +297,10 @@ static void (*handler[LASTEvent]) (XEvent *) = {
 	[MapRequest] = maprequest,
 	[MotionNotify] = motionnotify,
 	[PropertyNotify] = propertynotify,
+	[ResizeRequest] = resizerequest,
 	[UnmapNotify] = unmapnotify
 };
-static Atom wmatom[WMLast], netatom[NetLast];
+static Atom wmatom[WMLast], netatom[NetLast], xatom[XLast];
 static int running = 1;
 static Cur *cursor[CurLast];
 static Clr **scheme;
@@ -271,6 +312,14 @@ static Window root, wmcheckwin;
 /* configuration, allows nested code to access above variables */
 #include "config.h"
 
+struct Pertag {
+	unsigned int curtag, prevtag; /* current and previous tag */
+	int nmasters[LENGTH(tags) + 1]; /* number of windows in master area */
+	float mfacts[LENGTH(tags) + 1]; /* mfacts per tag */
+	unsigned int sellts[LENGTH(tags) + 1]; /* selected layouts */
+	const Layout *ltidxs[LENGTH(tags) + 1][2]; /* matrix of tags and layouts indexes  */
+};
+
 /* compile-time check if all tags fit into an unsigned int bit array. */
 struct NumTags { char limitexceeded[LENGTH(tags) > 31 ? -1 : 1]; };
 
@@ -407,6 +456,17 @@ attach(Client *c)
 }
 
 void
+attachbelow(Client *c)
+{
+	Client *below = c->mon->clients;
+	for (; below && below->next; below = below->next);
+	if (below)
+		below->next = c;
+	else
+		c->mon->clients = c;
+}
+
+void
 attachstack(Client *c)
 {
 	c->snext = c->mon->stack;
@@ -482,6 +542,11 @@ cleanup(void)
 	XUngrabKey(dpy, AnyKey, AnyModifier, root);
 	while (mons)
 		cleanupmon(mons);
+	if (showsystray) {
+		XUnmapWindow(dpy, systray->win);
+		XDestroyWindow(dpy, systray->win);
+		free(systray);
+	}
 	for (i = 0; i < CurLast; i++)
 		drw_cur_free(drw, cursor[i]);
 	for (i = 0; i < LENGTH(colors); i++)
@@ -512,9 +577,52 @@ cleanupmon(Monitor *mon)
 void
 clientmessage(XEvent *e)
 {
+	XWindowAttributes wa;
+	XSetWindowAttributes swa;
 	XClientMessageEvent *cme = &e->xclient;
 	Client *c = wintoclient(cme->window);
 
+	if (showsystray && cme->window == systray->win && cme->message_type == netatom[NetSystemTrayOP]) {
+		/* add systray icons */
+		if (cme->data.l[1] == SYSTEM_TRAY_REQUEST_DOCK) {
+			if (!(c = (Client *)calloc(1, sizeof(Client))))
+				die("fatal: could not malloc() %u bytes\n", sizeof(Client));
+			if (!(c->win = cme->data.l[2])) {
+				free(c);
+				return;
+			}
+			c->mon = selmon;
+			c->next = systray->icons;
+			systray->icons = c;
+			XGetWindowAttributes(dpy, c->win, &wa);
+			c->x = c->oldx = c->y = c->oldy = 0;
+			c->w = c->oldw = wa.width;
+			c->h = c->oldh = wa.height;
+			c->oldbw = wa.border_width;
+			c->bw = 0;
+			c->isfloating = True;
+			/* reuse tags field as mapped status */
+			c->tags = 1;
+			updatesizehints(c);
+			updatesystrayicongeom(c, wa.width, wa.height);
+			XAddToSaveSet(dpy, c->win);
+			XSelectInput(dpy, c->win, StructureNotifyMask | PropertyChangeMask | ResizeRedirectMask);
+			XReparentWindow(dpy, c->win, systray->win, 0, 0);
+			/* use parents background color */
+			swa.background_pixel  = scheme[SchemeNorm][ColBg].pixel;
+			XChangeWindowAttributes(dpy, c->win, CWBackPixel, &swa);
+			sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_EMBEDDED_NOTIFY, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
+			/* FIXME not sure if I have to send these events, too */
+			sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_FOCUS_IN, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
+			sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_WINDOW_ACTIVATE, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
+			sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_MODALITY_ON, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
+			XSync(dpy, False);
+			resizebarwin(selmon);
+			updatesystray();
+			setclientstate(c, NormalState);
+		}
+		return;
+	}
 	if (!c)
 		return;
 	if (cme->message_type == netatom[NetWMState]) {
@@ -567,7 +675,7 @@ configurenotify(XEvent *e)
 				for (c = m->clients; c; c = c->next)
 					if (c->isfullscreen)
 						resizeclient(c, m->mx, m->my, m->mw, m->mh);
-				XMoveResizeWindow(dpy, m->barwin, m->wx, m->by, m->ww, bh);
+				resizebarwin(m);
 			}
 			focus(NULL);
 			arrange(NULL);
@@ -631,6 +739,7 @@ Monitor *
 createmon(void)
 {
 	Monitor *m;
+	int i;
 
 	m = ecalloc(1, sizeof(Monitor));
 	m->tagset[0] = m->tagset[1] = 1;
@@ -641,6 +750,21 @@ createmon(void)
 	m->lt[0] = &layouts[0];
 	m->lt[1] = &layouts[1 % LENGTH(layouts)];
 	strncpy(m->ltsymbol, layouts[0].symbol, sizeof m->ltsymbol);
+	if (!(m->pertag = (Pertag *)calloc(1, sizeof(Pertag))))
+		die("fatal: could not malloc() %u bytes\n", sizeof(Pertag));
+	m->pertag->curtag = m->pertag->prevtag = 1;
+	for (i=0; i <= LENGTH(tags); i++) {
+		/* init nmaster */
+		m->pertag->nmasters[i] = m->nmaster;
+
+		/* init mfacts */
+		m->pertag->mfacts[i] = m->mfact;
+
+		/* init layouts */
+		m->pertag->ltidxs[i][0] = m->lt[0];
+		m->pertag->ltidxs[i][1] = m->lt[1];
+		m->pertag->sellts[i] = m->sellt;
+	}
 	return m;
 }
 
@@ -652,6 +776,11 @@ destroynotify(XEvent *e)
 
 	if ((c = wintoclient(ev->window)))
 		unmanage(c, 1);
+	else if ((c = wintosystrayicon(ev->window))) {
+		removesystrayicon(c);
+		resizebarwin(selmon);
+		updatesystray();
+	}
 }
 
 void
@@ -695,19 +824,23 @@ dirtomon(int dir)
 void
 drawbar(Monitor *m)
 {
-	int x, w, sw = 0;
+	int x, w, sw = 0, stw = 0;
 	int boxs = drw->fonts->h / 9;
 	int boxw = drw->fonts->h / 6 + 2;
 	unsigned int i, occ = 0, urg = 0;
 	Client *c;
 
+	if(showsystray && m == systraytomon(m))
+		stw = getsystraywidth();
+
 	/* draw status first so it can be overdrawn by tags later */
 	if (m == selmon) { /* status is only drawn on selected monitor */
 		drw_setscheme(drw, scheme[SchemeNorm]);
-		sw = TEXTW(stext) - lrpad + 2; /* 2px right padding */
-		drw_text(drw, m->ww - sw, 0, sw, bh, 0, stext, 0);
+		sw = TEXTW(stext) - lrpad / 2 + 2; /* 2px right padding */
+		drw_text(drw, m->ww - sw - stw, 0, sw, bh, lrpad / 2 - 2, stext, 0);
 	}
 
+	resizebarwin(m);
 	for (c = m->clients; c; c = c->next) {
 		occ |= c->tags;
 		if (c->isurgent)
@@ -728,7 +861,7 @@ drawbar(Monitor *m)
 	drw_setscheme(drw, scheme[SchemeNorm]);
 	x = drw_text(drw, x, 0, w, bh, lrpad / 2, m->ltsymbol, 0);
 
-	if ((w = m->ww - sw - x) > bh) {
+	if ((w = m->ww - sw - stw - x) > bh) {
 		if (m->sel) {
 			drw_setscheme(drw, scheme[m == selmon ? SchemeSel : SchemeNorm]);
 			drw_text(drw, x, 0, w, bh, lrpad / 2, m->sel->name, 0);
@@ -739,7 +872,7 @@ drawbar(Monitor *m)
 			drw_rect(drw, x, 0, w, bh, 1, 1);
 		}
 	}
-	drw_map(drw, m->barwin, 0, 0, m->ww, bh);
+	drw_map(drw, m->barwin, 0, 0, m->ww - stw, bh);
 }
 
 void
@@ -776,8 +909,11 @@ expose(XEvent *e)
 	Monitor *m;
 	XExposeEvent *ev = &e->xexpose;
 
-	if (ev->count == 0 && (m = wintomon(ev->window)))
+	if (ev->count == 0 && (m = wintomon(ev->window))) {
 		drawbar(m);
+		if (m == selmon)
+			updatesystray();
+	}
 }
 
 void
@@ -862,10 +998,17 @@ getatomprop(Client *c, Atom prop)
 	unsigned long dl;
 	unsigned char *p = NULL;
 	Atom da, atom = None;
+	/* FIXME getatomprop should return the number of items and a pointer to
+	 * the stored data instead of this workaround */
+	Atom req = XA_ATOM;
+	if (prop == xatom[XembedInfo])
+		req = xatom[XembedInfo];
 
-	if (XGetWindowProperty(dpy, c->win, prop, 0L, sizeof atom, False, XA_ATOM,
+	if (XGetWindowProperty(dpy, c->win, prop, 0L, sizeof atom, False, req,
 		&da, &di, &dl, &dl, &p) == Success && p) {
 		atom = *(Atom *)p;
+		if (da == xatom[XembedInfo] && dl == 2)
+			atom = ((Atom *)p)[1];
 		XFree(p);
 	}
 	return atom;
@@ -899,6 +1042,16 @@ getstate(Window w)
 	return result;
 }
 
+unsigned int
+getsystraywidth()
+{
+	unsigned int w = 0;
+	Client *i;
+	if(showsystray)
+		for(i = systray->icons; i; w += i->w + systrayspacing, i = i->next) ;
+	return w ? w + systrayspacing : 1;
+}
+
 int
 gettextprop(Window w, Atom atom, char *text, unsigned int size)
 {
@@ -966,7 +1119,7 @@ grabkeys(void)
 void
 incnmaster(const Arg *arg)
 {
-	selmon->nmaster = MAX(selmon->nmaster + arg->i, 0);
+	selmon->nmaster = selmon->pertag->nmasters[selmon->pertag->curtag] = MAX(selmon->nmaster + arg->i, 0);
 	arrange(selmon);
 }
 
@@ -1003,7 +1156,7 @@ killclient(const Arg *arg)
 {
 	if (!selmon->sel)
 		return;
-	if (!sendevent(selmon->sel, wmatom[WMDelete])) {
+	if (!sendevent(selmon->sel->win, wmatom[WMDelete], NoEventMask, wmatom[WMDelete], CurrentTime, 0 , 0, 0)) {
 		XGrabServer(dpy);
 		XSetErrorHandler(xerrordummy);
 		XSetCloseDownMode(dpy, DestroyAll);
@@ -1062,7 +1215,7 @@ manage(Window w, XWindowAttributes *wa)
 		c->isfloating = c->oldstate = trans != None || c->isfixed;
 	if (c->isfloating)
 		XRaiseWindow(dpy, c->win);
-	attach(c);
+	attachbelow(c);
 	attachstack(c);
 	XChangeProperty(dpy, root, netatom[NetClientList], XA_WINDOW, 32, PropModeAppend,
 		(unsigned char *) &(c->win), 1);
@@ -1091,6 +1244,12 @@ maprequest(XEvent *e)
 {
 	static XWindowAttributes wa;
 	XMapRequestEvent *ev = &e->xmaprequest;
+	Client *i;
+	if ((i = wintosystrayicon(ev->window))) {
+		sendevent(i->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_WINDOW_ACTIVATE, 0, systray->win, XEMBED_EMBEDDED_VERSION);
+		resizebarwin(selmon);
+		updatesystray();
+	}
 
 	if (!XGetWindowAttributes(dpy, ev->window, &wa))
 		return;
@@ -1215,6 +1374,16 @@ propertynotify(XEvent *e)
 	Window trans;
 	XPropertyEvent *ev = &e->xproperty;
 
+	if ((c = wintosystrayicon(ev->window))) {
+		if (ev->atom == XA_WM_NORMAL_HINTS) {
+			updatesizehints(c);
+			updatesystrayicongeom(c, c->w, c->h);
+		}
+		else
+			updatesystrayiconstate(c, ev);
+		resizebarwin(selmon);
+		updatesystray();
+	}
 	if ((ev->window == root) && (ev->atom == XA_WM_NAME))
 		updatestatus();
 	else if (ev->state == PropertyDelete)
@@ -1266,6 +1435,20 @@ recttomon(int x, int y, int w, int h)
 }
 
 void
+removesystrayicon(Client *i)
+{
+	Client **ii;
+
+	if (!showsystray || !i)
+		return;
+	for (ii = &systray->icons; *ii && *ii != i; ii = &(*ii)->next);
+	if (ii)
+		*ii = i->next;
+	free(i);
+}
+
+
+void
 resize(Client *c, int x, int y, int w, int h, int interact)
 {
 	if (applysizehints(c, &x, &y, &w, &h, interact))
@@ -1273,15 +1456,47 @@ resize(Client *c, int x, int y, int w, int h, int inte
 }
 
 void
+resizebarwin(Monitor *m) {
+	unsigned int w = m->ww;
+	if (showsystray && m == systraytomon(m))
+		w -= getsystraywidth();
+	XMoveResizeWindow(dpy, m->barwin, m->wx, m->by, w, bh);
+}
+
+void
 resizeclient(Client *c, int x, int y, int w, int h)
 {
 	XWindowChanges wc;
+	unsigned int n;
+	unsigned int gapoffset;
+	unsigned int gapincr;
+	Client *nbc;
 
-	c->oldx = c->x; c->x = wc.x = x;
-	c->oldy = c->y; c->y = wc.y = y;
-	c->oldw = c->w; c->w = wc.width = w;
-	c->oldh = c->h; c->h = wc.height = h;
 	wc.border_width = c->bw;
+
+	/* Get number of clients for the selected monitor */
+	for (n = 0, nbc = nexttiled(selmon->clients); nbc; nbc = nexttiled(nbc->next), n++);
+
+	/* Do nothing if layout is floating */
+	if (c->isfloating || selmon->lt[selmon->sellt]->arrange == NULL) {
+		gapincr = gapoffset = 0;
+	} else {
+		/* Remove border and gap if layout is monocle or only one client */
+		if (selmon->lt[selmon->sellt]->arrange == monocle || n == 1) {
+			gapoffset = 0;
+			gapincr = -2 * borderpx;
+			wc.border_width = 0;
+		} else {
+			gapoffset = gappx;
+			gapincr = 2 * gappx;
+		}
+	}
+
+	c->oldx = c->x; c->x = wc.x = x + gapoffset;
+	c->oldy = c->y; c->y = wc.y = y + gapoffset;
+	c->oldw = c->w; c->w = wc.width = w - gapincr;
+	c->oldh = c->h; c->h = wc.height = h - gapincr;
+
 	XConfigureWindow(dpy, c->win, CWX|CWY|CWWidth|CWHeight|CWBorderWidth, &wc);
 	configure(c);
 	XSync(dpy, False);
@@ -1345,6 +1560,19 @@ resizemouse(const Arg *arg)
 }
 
 void
+resizerequest(XEvent *e)
+{
+	XResizeRequestEvent *ev = &e->xresizerequest;
+	Client *i;
+
+	if ((i = wintosystrayicon(ev->window))) {
+		updatesystrayicongeom(i, ev->width, ev->height);
+		resizebarwin(selmon);
+		updatesystray();
+	}
+}
+
+void
 restack(Monitor *m)
 {
 	Client *c;
@@ -1417,7 +1645,7 @@ sendmon(Client *c, Monitor *m)
 	detachstack(c);
 	c->mon = m;
 	c->tags = m->tagset[m->seltags]; /* assign tags of target monitor */
-	attach(c);
+	attachbelow(c);
 	attachstack(c);
 	focus(NULL);
 	arrange(NULL);
@@ -1433,26 +1661,36 @@ setclientstate(Client *c, long state)
 }
 
 int
-sendevent(Client *c, Atom proto)
+sendevent(Window w, Atom proto, int mask, long d0, long d1, long d2, long d3, long d4)
 {
 	int n;
-	Atom *protocols;
+	Atom *protocols, mt;
 	int exists = 0;
 	XEvent ev;
 
-	if (XGetWMProtocols(dpy, c->win, &protocols, &n)) {
-		while (!exists && n--)
-			exists = protocols[n] == proto;
-		XFree(protocols);
+	if (proto == wmatom[WMTakeFocus] || proto == wmatom[WMDelete]) {
+		mt = wmatom[WMProtocols];
+		if (XGetWMProtocols(dpy, w, &protocols, &n)) {
+			while (!exists && n--)
+				exists = protocols[n] == proto;
+			XFree(protocols);
+		}
 	}
+	else {
+		exists = True;
+		mt = proto;
+	}
 	if (exists) {
 		ev.type = ClientMessage;
-		ev.xclient.window = c->win;
-		ev.xclient.message_type = wmatom[WMProtocols];
+		ev.xclient.window = w;
+		ev.xclient.message_type = mt;
 		ev.xclient.format = 32;
-		ev.xclient.data.l[0] = proto;
-		ev.xclient.data.l[1] = CurrentTime;
-		XSendEvent(dpy, c->win, False, NoEventMask, &ev);
+		ev.xclient.data.l[0] = d0;
+		ev.xclient.data.l[1] = d1;
+		ev.xclient.data.l[2] = d2;
+		ev.xclient.data.l[3] = d3;
+		ev.xclient.data.l[4] = d4;
+		XSendEvent(dpy, w, False, mask, &ev);
 	}
 	return exists;
 }
@@ -1466,7 +1704,7 @@ setfocus(Client *c)
 			XA_WINDOW, 32, PropModeReplace,
 			(unsigned char *) &(c->win), 1);
 	}
-	sendevent(c, wmatom[WMTakeFocus]);
+	sendevent(c->win, wmatom[WMTakeFocus], NoEventMask, wmatom[WMTakeFocus], CurrentTime, 0, 0, 0);
 }
 
 void
@@ -1500,10 +1738,13 @@ setfullscreen(Client *c, int fullscreen)
 void
 setlayout(const Arg *arg)
 {
-	if (!arg || !arg->v || arg->v != selmon->lt[selmon->sellt])
-		selmon->sellt ^= 1;
+	if (!arg || !arg->v || arg->v != selmon->lt[selmon->sellt]) {
+		selmon->pertag->sellts[selmon->pertag->curtag] ^= 1;
+		selmon->sellt = selmon->pertag->sellts[selmon->pertag->curtag];
+	}
 	if (arg && arg->v)
-		selmon->lt[selmon->sellt] = (Layout *)arg->v;
+		selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt] = (Layout *)arg->v;
+	selmon->lt[selmon->sellt] = selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt];
 	strncpy(selmon->ltsymbol, selmon->lt[selmon->sellt]->symbol, sizeof selmon->ltsymbol);
 	if (selmon->sel)
 		arrange(selmon);
@@ -1522,7 +1763,7 @@ setmfact(const Arg *arg)
 	f = arg->f < 1.0 ? arg->f + selmon->mfact : arg->f - 1.0;
 	if (f < 0.1 || f > 0.9)
 		return;
-	selmon->mfact = f;
+	selmon->mfact = selmon->pertag->mfacts[selmon->pertag->curtag] = f;
 	arrange(selmon);
 }
 
@@ -1555,6 +1796,10 @@ setup(void)
 	wmatom[WMTakeFocus] = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
 	netatom[NetActiveWindow] = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
 	netatom[NetSupported] = XInternAtom(dpy, "_NET_SUPPORTED", False);
+	netatom[NetSystemTray] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_S0", False);
+	netatom[NetSystemTrayOP] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_OPCODE", False);
+	netatom[NetSystemTrayOrientation] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_ORIENTATION", False);
+	netatom[NetSystemTrayOrientationHorz] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_ORIENTATION_HORZ", False);
 	netatom[NetWMName] = XInternAtom(dpy, "_NET_WM_NAME", False);
 	netatom[NetWMState] = XInternAtom(dpy, "_NET_WM_STATE", False);
 	netatom[NetWMCheck] = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
@@ -1562,6 +1807,9 @@ setup(void)
 	netatom[NetWMWindowType] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
 	netatom[NetWMWindowTypeDialog] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
 	netatom[NetClientList] = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
+	xatom[Manager] = XInternAtom(dpy, "MANAGER", False);
+	xatom[Xembed] = XInternAtom(dpy, "_XEMBED", False);
+	xatom[XembedInfo] = XInternAtom(dpy, "_XEMBED_INFO", False);
 	/* init cursors */
 	cursor[CurNormal] = drw_cur_create(drw, XC_left_ptr);
 	cursor[CurResize] = drw_cur_create(drw, XC_sizing);
@@ -1570,6 +1818,8 @@ setup(void)
 	scheme = ecalloc(LENGTH(colors), sizeof(Clr *));
 	for (i = 0; i < LENGTH(colors); i++)
 		scheme[i] = drw_scm_create(drw, colors[i], 3);
+	/* init system tray */
+	updatesystray();
 	/* init bars */
 	updatebars();
 	updatestatus();
@@ -1701,7 +1951,18 @@ togglebar(const Arg *arg)
 {
 	selmon->showbar = !selmon->showbar;
 	updatebarpos(selmon);
-	XMoveResizeWindow(dpy, selmon->barwin, selmon->wx, selmon->by, selmon->ww, bh);
+	resizebarwin(selmon);
+	if (showsystray) {
+		XWindowChanges wc;
+		if (!selmon->showbar)
+			wc.y = -bh;
+		else if (selmon->showbar) {
+			wc.y = 0;
+			if (!selmon->topbar)
+				wc.y = selmon->mh - bh;
+		}
+		XConfigureWindow(dpy, systray->win, CWY, &wc);
+	}
 	arrange(selmon);
 }
 
@@ -1738,9 +1999,27 @@ void
 toggleview(const Arg *arg)
 {
 	unsigned int newtagset = selmon->tagset[selmon->seltags] ^ (arg->ui & TAGMASK);
+	int i;
 
 	if (newtagset) {
+		if (newtagset == ~0) {
+			selmon->pertag->prevtag = selmon->pertag->curtag;
+			selmon->pertag->curtag = 0;
+		}
+		/* test if the user did not select the same tag */
+		if (!(newtagset & 1 << (selmon->pertag->curtag - 1))) {
+			selmon->pertag->prevtag = selmon->pertag->curtag;
+			for (i=0; !(newtagset & 1 << i); i++) ;
+			selmon->pertag->curtag = i + 1;
+		}
 		selmon->tagset[selmon->seltags] = newtagset;
+
+		/* apply settings for this view */
+		selmon->nmaster = selmon->pertag->nmasters[selmon->pertag->curtag];
+		selmon->mfact = selmon->pertag->mfacts[selmon->pertag->curtag];
+		selmon->sellt = selmon->pertag->sellts[selmon->pertag->curtag];
+		selmon->lt[selmon->sellt] = selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt];
+		selmon->lt[selmon->sellt^1] = selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt^1];
 		focus(NULL);
 		arrange(selmon);
 	}
@@ -1796,11 +2075,18 @@ unmapnotify(XEvent *e)
 		else
 			unmanage(c, 0);
 	}
+	else if ((c = wintosystrayicon(ev->window))) {
+		/* KLUDGE! sometimes icons occasionally unmap their windows, but do
+		 * _not_ destroy them. We map those windows back */
+		XMapRaised(dpy, c->win);
+		updatesystray();
+	}
 }
 
 void
 updatebars(void)
 {
+	unsigned int w;
 	Monitor *m;
 	XSetWindowAttributes wa = {
 		.override_redirect = True,
@@ -1811,10 +2097,15 @@ updatebars(void)
 	for (m = mons; m; m = m->next) {
 		if (m->barwin)
 			continue;
-		m->barwin = XCreateWindow(dpy, root, m->wx, m->by, m->ww, bh, 0, DefaultDepth(dpy, screen),
+		w = m->ww;
+		if (showsystray && m == systraytomon(m))
+			w -= getsystraywidth();
+		m->barwin = XCreateWindow(dpy, root, m->wx, m->by, w, bh, 0, DefaultDepth(dpy, screen),
 				CopyFromParent, DefaultVisual(dpy, screen),
 				CWOverrideRedirect|CWBackPixmap|CWEventMask, &wa);
 		XDefineCursor(dpy, m->barwin, cursor[CurNormal]->cursor);
+		if (showsystray && m == systraytomon(m))
+			XMapRaised(dpy, systray->win);
 		XMapRaised(dpy, m->barwin);
 		XSetClassHint(dpy, m->barwin, &ch);
 	}
@@ -1897,7 +2188,7 @@ updategeom(void)
 					m->clients = c->next;
 					detachstack(c);
 					c->mon = mons;
-					attach(c);
+					attachbelow(c);
 					attachstack(c);
 				}
 				if (m == selmon)
@@ -1990,9 +2281,124 @@ updatestatus(void)
 	if (!gettextprop(root, XA_WM_NAME, stext, sizeof(stext)))
 		strcpy(stext, "dwm-"VERSION);
 	drawbar(selmon);
+	updatesystray();
 }
 
 void
+updatesystrayicongeom(Client *i, int w, int h)
+{
+	if (i) {
+		i->h = bh;
+		if (w == h)
+			i->w = bh;
+		else if (h == bh)
+			i->w = w;
+		else
+			i->w = (int) ((float)bh * ((float)w / (float)h));
+		applysizehints(i, &(i->x), &(i->y), &(i->w), &(i->h), False);
+		/* force icons into the systray dimenons if they don't want to */
+		if (i->h > bh) {
+			if (i->w == i->h)
+				i->w = bh;
+			else
+				i->w = (int) ((float)bh * ((float)i->w / (float)i->h));
+			i->h = bh;
+		}
+	}
+}
+
+void
+updatesystrayiconstate(Client *i, XPropertyEvent *ev)
+{
+	long flags;
+	int code = 0;
+
+	if (!showsystray || !i || ev->atom != xatom[XembedInfo] ||
+			!(flags = getatomprop(i, xatom[XembedInfo])))
+		return;
+
+	if (flags & XEMBED_MAPPED && !i->tags) {
+		i->tags = 1;
+		code = XEMBED_WINDOW_ACTIVATE;
+		XMapRaised(dpy, i->win);
+		setclientstate(i, NormalState);
+	}
+	else if (!(flags & XEMBED_MAPPED) && i->tags) {
+		i->tags = 0;
+		code = XEMBED_WINDOW_DEACTIVATE;
+		XUnmapWindow(dpy, i->win);
+		setclientstate(i, WithdrawnState);
+	}
+	else
+		return;
+	sendevent(i->win, xatom[Xembed], StructureNotifyMask, CurrentTime, code, 0,
+			systray->win, XEMBED_EMBEDDED_VERSION);
+}
+
+void
+updatesystray(void)
+{
+	XSetWindowAttributes wa;
+	XWindowChanges wc;
+	Client *i;
+	Monitor *m = systraytomon(NULL);
+	unsigned int x = m->mx + m->mw;
+	unsigned int w = 1;
+
+	if (!showsystray)
+		return;
+	if (!systray) {
+		/* init systray */
+		if (!(systray = (Systray *)calloc(1, sizeof(Systray))))
+			die("fatal: could not malloc() %u bytes\n", sizeof(Systray));
+		systray->win = XCreateSimpleWindow(dpy, root, x, m->by, w, bh, 0, 0, scheme[SchemeSel][ColBg].pixel);
+		wa.event_mask        = ButtonPressMask | ExposureMask;
+		wa.override_redirect = True;
+		wa.background_pixel  = scheme[SchemeNorm][ColBg].pixel;
+		XSelectInput(dpy, systray->win, SubstructureNotifyMask);
+		XChangeProperty(dpy, systray->win, netatom[NetSystemTrayOrientation], XA_CARDINAL, 32,
+				PropModeReplace, (unsigned char *)&netatom[NetSystemTrayOrientationHorz], 1);
+		XChangeWindowAttributes(dpy, systray->win, CWEventMask|CWOverrideRedirect|CWBackPixel, &wa);
+		XMapRaised(dpy, systray->win);
+		XSetSelectionOwner(dpy, netatom[NetSystemTray], systray->win, CurrentTime);
+		if (XGetSelectionOwner(dpy, netatom[NetSystemTray]) == systray->win) {
+			sendevent(root, xatom[Manager], StructureNotifyMask, CurrentTime, netatom[NetSystemTray], systray->win, 0, 0);
+			XSync(dpy, False);
+		}
+		else {
+			fprintf(stderr, "dwm: unable to obtain system tray.\n");
+			free(systray);
+			systray = NULL;
+			return;
+		}
+	}
+	for (w = 0, i = systray->icons; i; i = i->next) {
+		/* make sure the background color stays the same */
+		wa.background_pixel  = scheme[SchemeNorm][ColBg].pixel;
+		XChangeWindowAttributes(dpy, i->win, CWBackPixel, &wa);
+		XMapRaised(dpy, i->win);
+		w += systrayspacing;
+		i->x = w;
+		XMoveResizeWindow(dpy, i->win, i->x, 0, i->w, i->h);
+		w += i->w;
+		if (i->mon != m)
+			i->mon = m;
+	}
+	w = w ? w + systrayspacing : 1;
+	x -= w;
+	XMoveResizeWindow(dpy, systray->win, x, m->by, w, bh);
+	wc.x = x; wc.y = m->by; wc.width = w; wc.height = bh;
+	wc.stack_mode = Above; wc.sibling = m->barwin;
+	XConfigureWindow(dpy, systray->win, CWX|CWY|CWWidth|CWHeight|CWSibling|CWStackMode, &wc);
+	XMapWindow(dpy, systray->win);
+	XMapSubwindows(dpy, systray->win);
+	/* redraw background */
+	XSetForeground(dpy, drw->gc, scheme[SchemeNorm][ColBg].pixel);
+	XFillRectangle(dpy, systray->win, drw->gc, 0, 0, w, bh);
+	XSync(dpy, False);
+}
+
+void
 updatetitle(Client *c)
 {
 	if (!gettextprop(c->win, netatom[NetWMName], c->name, sizeof c->name))
@@ -2035,11 +2441,31 @@ updatewmhints(Client *c)
 void
 view(const Arg *arg)
 {
+	int i;
+	unsigned int tmptag;
+
 	if ((arg->ui & TAGMASK) == selmon->tagset[selmon->seltags])
 		return;
 	selmon->seltags ^= 1; /* toggle sel tagset */
-	if (arg->ui & TAGMASK)
+	if (arg->ui & TAGMASK) {
+		selmon->pertag->prevtag = selmon->pertag->curtag;
 		selmon->tagset[selmon->seltags] = arg->ui & TAGMASK;
+		if (arg->ui == ~0)
+			selmon->pertag->curtag = 0;
+		else {
+			for (i=0; !(arg->ui & 1 << i); i++) ;
+			selmon->pertag->curtag = i + 1;
+		}
+	} else {
+		tmptag = selmon->pertag->prevtag;
+		selmon->pertag->prevtag = selmon->pertag->curtag;
+		selmon->pertag->curtag = tmptag;
+	}
+	selmon->nmaster = selmon->pertag->nmasters[selmon->pertag->curtag];
+	selmon->mfact = selmon->pertag->mfacts[selmon->pertag->curtag];
+	selmon->sellt = selmon->pertag->sellts[selmon->pertag->curtag];
+	selmon->lt[selmon->sellt] = selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt];
+	selmon->lt[selmon->sellt^1] = selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt^1];
 	focus(NULL);
 	arrange(selmon);
 }
@@ -2057,6 +2483,16 @@ wintoclient(Window w)
 	return NULL;
 }
 
+Client *
+wintosystrayicon(Window w) {
+	Client *i = NULL;
+
+	if (!showsystray || !w)
+		return i;
+	for (i = systray->icons; i && i->win != w; i = i->next) ;
+	return i;
+}
+
 Monitor *
 wintomon(Window w)
 {
@@ -2110,6 +2546,22 @@ xerrorstart(Display *dpy, XErrorEvent *ee)
 	return -1;
 }
 
+Monitor *
+systraytomon(Monitor *m) {
+	Monitor *t;
+	int i, n;
+	if(!systraypinning) {
+		if(!m)
+			return selmon;
+		return m == selmon ? m : NULL;
+	}
+	for(n = 1, t = mons; t && t->next; n++, t = t->next) ;
+	for(i = 1, t = mons; t && t->next && i < systraypinning; i++, t = t->next) ;
+	if(systraypinningfailfirst && n < systraypinning)
+		return mons;
+	return t;
+}
+
 void
 zoom(const Arg *arg)
 {
@@ -2146,4 +2598,66 @@ main(int argc, char *argv[])
 	cleanup();
 	XCloseDisplay(dpy);
 	return EXIT_SUCCESS;
+}
+
+static void
+bstack(Monitor *m) {
+	int w, h, mh, mx, tx, ty, tw;
+	unsigned int i, n;
+	Client *c;
+
+	for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);
+	if (n == 0)
+		return;
+	if (n > m->nmaster) {
+		mh = m->nmaster ? m->mfact * m->wh : 0;
+		tw = m->ww / (n - m->nmaster);
+		ty = m->wy + mh;
+	} else {
+		mh = m->wh;
+		tw = m->ww;
+		ty = m->wy;
+	}
+	for (i = mx = 0, tx = m->wx, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++) {
+		if (i < m->nmaster) {
+			w = (m->ww - mx) / (MIN(n, m->nmaster) - i);
+			resize(c, m->wx + mx, m->wy, w - (2 * c->bw), mh - (2 * c->bw), 0);
+			mx += WIDTH(c);
+		} else {
+			h = m->wh - mh;
+			resize(c, tx, ty, tw - (2 * c->bw), h - (2 * c->bw), 0);
+			if (tw != m->ww)
+				tx += WIDTH(c);
+		}
+	}
+}
+
+static void
+bstackhoriz(Monitor *m) {
+	int w, mh, mx, tx, ty, th;
+	unsigned int i, n;
+	Client *c;
+
+	for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);
+	if (n == 0)
+		return;
+	if (n > m->nmaster) {
+		mh = m->nmaster ? m->mfact * m->wh : 0;
+		th = (m->wh - mh) / (n - m->nmaster);
+		ty = m->wy + mh;
+	} else {
+		th = mh = m->wh;
+		ty = m->wy;
+	}
+	for (i = mx = 0, tx = m->wx, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++) {
+		if (i < m->nmaster) {
+			w = (m->ww - mx) / (MIN(n, m->nmaster) - i);
+			resize(c, m->wx + mx, m->wy, w - (2 * c->bw), mh - (2 * c->bw), 0);
+			mx += WIDTH(c);
+		} else {
+			resize(c, tx, ty, m->ww - (2 * c->bw), th - (2 * c->bw), 0);
+			if (th != m->wh)
+				ty += HEIGHT(c);
+		}
+	}
 }
