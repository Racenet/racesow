#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>

#include <X11/extensions/xf86dga.h>
#include <X11/extensions/xf86vmode.h>
#include <X11/extensions/Xinerama.h>

#include <GL/glx.h>

typedef struct x11display_s
{
	Display *dpy;
	int scr;
	Window root, win, gl_win, old_win;
	Colormap cmap;
	GLXContext ctx;
	XVisualInfo *visinfo;
	Atom wmDeleteWindow;

	unsigned int win_width, win_height;
} x11display_t;

// defined by glx_imp.c, used also by in_x11.c
extern x11display_t x11display;
