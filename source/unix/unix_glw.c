/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
/*
** GLW_IMP.C
**
** This file contains ALL Linux specific stuff having to do with the
** OpenGL refresh.  When a port is being made the following functions
** must be implemented by the port:
**
** GLimp_EndFrame
** GLimp_Init
** GLimp_Shutdown
** GLimp_SwitchFullscreen
**
*/

#include <termios.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <dlfcn.h>

#include "../ref_gl/r_local.h"
#include "../client/keys.h"

#include "x11.h"

#include "unix_glw.h"

#define DISPLAY_MASK ( VisibilityChangeMask | StructureNotifyMask | ExposureMask | PropertyChangeMask )
#define INIT_MASK ( KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | FocusChangeMask | DISPLAY_MASK )

// use experimental Xrandr resolution?
#define _XRANDR_OVER_VIDMODE_

x11display_t x11display;
x11wndproc_t x11wndproc;

glwstate_t glw_state;

static qboolean _xf86_vidmodes_supported = qfalse;
static int default_dotclock, default_viewport[2];
static XF86VidModeModeLine default_modeline;
static XF86VidModeModeInfo **_xf86_vidmodes;
static int _xf86_vidmodes_num;
static qboolean _xf86_vidmodes_active = qfalse;
static qboolean _xf86_xinerama_supported = qfalse;

static void _xf86_VidmodesInit( void )
{
	int MajorVersion = 0, MinorVersion = 0;

	// Get video mode list
	if( XF86VidModeQueryVersion( x11display.dpy, &MajorVersion, &MinorVersion ) )
	{
		Com_Printf( "..XFree86-VidMode Extension Version %d.%d\n", MajorVersion, MinorVersion );
		XF86VidModeGetViewPort( x11display.dpy, x11display.scr, &default_viewport[0], &default_viewport[1] );
		XF86VidModeGetModeLine( x11display.dpy, x11display.scr, &default_dotclock, &default_modeline );
		XF86VidModeGetAllModeLines( x11display.dpy, x11display.scr, &_xf86_vidmodes_num, &_xf86_vidmodes );
		_xf86_vidmodes_supported = qtrue;
	}
	else
	{
		Com_Printf( "..XFree86-VidMode Extension not available\n" );
		_xf86_vidmodes_supported = qfalse;
	}
}

static void _xf86_VidmodesFree( void )
{
	if( _xf86_vidmodes_supported ) XFree( _xf86_vidmodes );

	_xf86_vidmodes_supported = qfalse;
}

static void _xf86_XineramaInit( void )
{
	int MajorVersion = 0, MinorVersion = 0;

	if( ( XineramaQueryVersion( x11display.dpy, &MajorVersion, &MinorVersion ) ) && ( XineramaIsActive( x11display.dpy ) ) )
	{
		Com_Printf( "..XFree86-Xinerama Extension Version %d.%d\n", MajorVersion, MinorVersion );
		_xf86_xinerama_supported = qtrue;
	}
	else
	{
		Com_Printf( "..XFree86-Xinerama Extension not available\n" );
		_xf86_xinerama_supported = qfalse;
	}
}

static void _xf86_XineramaFree( void )
{
	_xf86_xinerama_supported = qfalse;
}

static qboolean _xf86_XineramaFindBest( int *x, int *y, int *width, int *height, qboolean silent )
{
	int i, screens, head;
	int best_dist, dist;
	XineramaScreenInfo *xinerama;

	assert( _xf86_xinerama_supported );

	if( vid_multiscreen_head->integer == 0 )
		return qfalse;

	xinerama = XineramaQueryScreens( x11display.dpy, &screens );
	if( screens <= 1 )
		return qfalse;

	head = -1;
	if( vid_multiscreen_head->integer > 0 )
	{
		for( i = 0; i < screens; i++ )
		{
			if( xinerama[i].screen_number == vid_multiscreen_head->integer - 1 )
			{
				head = i;
				break;
			}
		}
		if( head == -1 && !silent )
			Com_Printf( "Xinerama: Head %i not found, using best fit\n", vid_multiscreen_head->integer );
		if( *width > xinerama[head].width || *height > xinerama[head].height )
		{
			head = -1;
			if( !silent )
				Com_Printf( "Xinerama: Window doesn't fit into head %i, using best fit\n", vid_multiscreen_head->integer );
		}
	}

	if( head == -1 ) // find best fit
	{
		best_dist = 999999999;
		for( i = 0; i < screens; i++ )
		{
			if( *width <= xinerama[i].width && *height <= xinerama[i].height )
			{
				if( xinerama[i].width - *width > xinerama[i].height - *height )
				{
					dist = xinerama[i].height - *height;
				}
				else
				{
					dist = xinerama[i].width - *width;
				}

				if( dist < 0 )
					dist = -dist; // Only positive number please

				if( dist < best_dist )
				{
					best_dist = dist;
					head = i;
				}
			}
		}
		if( head < -1 )
		{
			if( !silent )
				Com_Printf( "Xinerama: No fitting head found" );
			return qfalse;
		}
	}

	*x = xinerama[head].x_org;
	*y = xinerama[head].y_org;
	*width = xinerama[head].width;
	*height = xinerama[head].height;

	if( !silent )
		Com_Printf( "Xinerama: Using screen %d: %dx%d+%d+%d\n", xinerama[head].screen_number, xinerama[head].width, xinerama[head].height, xinerama[head].x_org, xinerama[head].y_org );

	return qtrue;
}

static void _xf86_VidmodesSwitch( int mode )
{
	if( _xf86_vidmodes_supported )
	{
		XF86VidModeSwitchToMode( x11display.dpy, x11display.scr, _xf86_vidmodes[mode] );
		XF86VidModeSetViewPort( x11display.dpy, x11display.scr, 0, 0 );
	}

	_xf86_vidmodes_active = qtrue;
}

static void _xf86_VidmodesSwitchBack( void )
{
	if( _xf86_vidmodes_supported && _xf86_vidmodes_active )
	{
		XF86VidModeModeInfo modeinfo;

		modeinfo.dotclock = default_dotclock;
		modeinfo.hdisplay = default_modeline.hdisplay;
		modeinfo.hsyncstart = default_modeline.hsyncstart;
		modeinfo.hsyncend = default_modeline.hsyncend;
		modeinfo.htotal = default_modeline.htotal;
		modeinfo.vdisplay = default_modeline.vdisplay;
		modeinfo.vsyncstart = default_modeline.vsyncstart;
		modeinfo.vsyncend = default_modeline.vsyncend;
		modeinfo.vtotal = default_modeline.vtotal;
		modeinfo.flags = default_modeline.flags;
		modeinfo.privsize = default_modeline.privsize;
		modeinfo.private = default_modeline.private;

		XF86VidModeSwitchToMode( x11display.dpy, x11display.scr, &modeinfo );
		XF86VidModeSetViewPort( x11display.dpy, x11display.scr, default_viewport[0], default_viewport[1] );
	}

	_xf86_vidmodes_active = qfalse;
}

static void _xf86_VidmodesFindBest( int *mode, int *pwidth, int *pheight, qboolean silent )
{
	int i, best_fit, best_dist, dist, x, y;

	best_fit = -1;
	best_dist = 999999999;

	if( _xf86_vidmodes_supported )
	{
		for( i = 0; i < _xf86_vidmodes_num; i++ )
		{
			if( _xf86_vidmodes[i]->hdisplay < *pwidth || _xf86_vidmodes[i]->vdisplay < *pheight )
				continue;

			x = _xf86_vidmodes[i]->hdisplay - *pwidth;
			y = _xf86_vidmodes[i]->vdisplay - *pheight;

			if( x > y ) dist = y;
			else dist = x;

			if( dist < 0 ) dist = -dist; // Only positive number please

			if( dist < best_dist )
			{
				best_dist = dist;
				best_fit = i;
			}

			if( !silent )
				Com_Printf( "%ix%i -> %ix%i: %i\n", *pwidth, *pheight, _xf86_vidmodes[i]->hdisplay, _xf86_vidmodes[i]->vdisplay, dist );
		}

		if( best_fit >= 0 )
		{
			if( !silent )
				Com_Printf( "%ix%i selected\n", _xf86_vidmodes[best_fit]->hdisplay, _xf86_vidmodes[best_fit]->vdisplay );

			*pwidth = _xf86_vidmodes[best_fit]->hdisplay;
			*pheight = _xf86_vidmodes[best_fit]->vdisplay;
		}
	}

	*mode = best_fit;
}

#ifdef _XRANDR_OVER_VIDMODE_
// XRANDR
qboolean _xrandr_supported = qfalse;
qboolean _xrandr_active = qfalse;
int _xrandr_eventbase;
int _xrandr_errorbase;
// list of resolutions
XRRScreenConfiguration *_xrandr_config = 0;
XRRScreenSize *_xrandr_sizes = 0;
int _xrandr_numsizes = 0;
// original rate and resolution
short _xrandr_default_rate;
Rotation _xrandr_default_rotation;
SizeID _xrandr_default_size;

static void _xf86_XrandrInit( void )
{
	int MajorVersion = 0, MinorVersion = 0;

	// Already initialized?
	if( _xrandr_supported )
		return;

	// Check extension
	if( XRRQueryExtension( x11display.dpy, &_xrandr_eventbase, &_xrandr_errorbase ) &&
		XRRQueryVersion( x11display.dpy, &MajorVersion, &MinorVersion ) )
	{
		Com_Printf( "..Xrandr Extension Version %d.%d\n", MajorVersion, MinorVersion );

		// Get current resolution
		_xrandr_config = XRRGetScreenInfo( x11display.dpy, x11display.root );
		_xrandr_default_rate = XRRConfigCurrentRate( _xrandr_config );
		_xrandr_default_size = XRRConfigCurrentConfiguration( _xrandr_config, &_xrandr_default_rotation );

		// Get a list of resolutions (first here is actually the current resolution too ^^)
		_xrandr_sizes = XRRSizes( x11display.dpy, 0, &_xrandr_numsizes );
		_xrandr_supported = qtrue;
	}
	else
	{
		Com_Printf( "..Xrandr Extension not available\n" );
		_xrandr_supported = qfalse;
	}
}

static void _xf86_XrandrFree( void )
{
	if( _xrandr_supported )
		XRRFreeScreenConfigInfo( _xrandr_config );

	_xrandr_config = 0;
	_xrandr_supported = qfalse;
	_xrandr_active = qfalse;
}

static short _xf86_XrandrClosestRate( int mode, short preferred_rate )
{
	short *rates, delta, min, best;
	int i, numrates;

	// fetch the rates for given resolution
	rates = XRRRates( x11display.dpy, x11display.scr, mode, &numrates );

	min = 0x7fff;
	best = 0;
	for( i = 0; i < numrates; i++ )
	{
		if( rates[i] > preferred_rate )
			continue;
		delta = preferred_rate - rates[i];
		if( delta < min )
		{
			best = rates[i];
			min = delta;
		}

		Com_Printf("  rate %i -> %i: %i\n", preferred_rate, rates[i], delta );
	}

	Com_Printf("_xf86_XrandrClosestRate found %i for %i\n", best, preferred_rate );
	return best;
}

static void _xf86_XrandrSwitch( int mode )
{
	if( _xrandr_supported )
	{
		short rate;

		// prefer user defined rate
		rate = vid_displayfrequency->integer ? vid_displayfrequency->integer : _xrandr_default_rate;
		// find the closest rate on this resolution
		rate = _xf86_XrandrClosestRate( mode, rate );

		/* CHANGE RESOLUTION */
		XRRSetScreenConfigAndRate( x11display.dpy, _xrandr_config, x11display.root, mode, RR_Rotate_0, rate, CurrentTime );

		// "Clients must call back into Xlib using XRRUpdateConfiguration when screen configuration change notify events are generated"
		// ??
	}

	_xrandr_active = qtrue;
}

static void _xf86_XrandrSwitchBack( void )
{
	if( _xrandr_supported && _xrandr_active )
	{
		XRRSetScreenConfigAndRate( x11display.dpy, _xrandr_config, x11display.root, _xrandr_default_size, _xrandr_default_rotation, _xrandr_default_rate, CurrentTime );
	}

	_xrandr_active = qfalse;
}

static void _xf86_XrandrFindBest( int *mode, int *pwidth, int *pheight, qboolean silent )
{
	int i, best_fit, best_dist, dist, x, y;

	best_fit = -1;
	best_dist = 999999999;

	if( _xrandr_supported )
	{
		for( i = 0; i < _xrandr_numsizes; i++ )
		{
			if( _xrandr_sizes[i].width < *pwidth || _xrandr_sizes[i].height < *pheight )
				continue;

			x = _xrandr_sizes[i].width - *pwidth;
			y = _xrandr_sizes[i].height - *pheight;

			if( x > y ) dist = y;
			else dist = x;

			if( dist < 0 ) dist = -dist; // Only positive number please

			if( dist < best_dist )
			{
				best_dist = dist;
				best_fit = i;
			}

			if( !silent )
				Com_Printf( "%ix%i -> %ix%i: %i\n", *pwidth, *pheight, _xrandr_sizes[i].width, _xrandr_sizes[i].height, dist );
		}

		if( best_fit >= 0 )
		{
			if( !silent )
				Com_Printf( "%ix%i selected\n", _xrandr_sizes[best_fit].width, _xrandr_sizes[best_fit].height );

			*pwidth = _xrandr_sizes[best_fit].width;
			*pheight = _xrandr_sizes[best_fit].height;
		}
	}

	*mode = best_fit;
}

#endif

static void _x11_SetNoResize( Window w, int width, int height )
{
	XSizeHints *hints;

	if( x11display.dpy )
	{
		hints = XAllocSizeHints();

		if( hints )
		{
			hints->min_width = hints->max_width = width;
			hints->min_height = hints->max_height = height;

			hints->flags = PMaxSize | PMinSize;

			XSetWMNormalHints( x11display.dpy, w, hints );
			XFree( hints );
		}
	}
}

/*****************************************************************************/

/*
* Sys_GetClipboardData
*
* Orginally from EzQuake
* There should be a smarter place to put this
*/
char *Sys_GetClipboardData( qboolean primary )
{
	Window win;
	Atom type;
	int format, ret;
	unsigned long nitems, bytes_after, bytes_left;
	unsigned char *data;
	char *buffer;
	Atom atom;

	if( !x11display.dpy )
		return NULL;

	if( primary )
	{
		atom = XInternAtom( x11display.dpy, "PRIMARY", True );
	}
	else
	{
		atom = XInternAtom( x11display.dpy, "CLIPBOARD", True );
	}
	if( atom == None )
		return NULL;

	win = XGetSelectionOwner( x11display.dpy, atom );
	if( win == None )
		return NULL;

	XConvertSelection( x11display.dpy, atom, XA_STRING, atom, win, CurrentTime );
	XFlush( x11display.dpy );

	XGetWindowProperty( x11display.dpy, win, atom, 0, 0, False, AnyPropertyType, &type, &format, &nitems, &bytes_left,
		&data );
	if( bytes_left <= 0 )
		return NULL;

	ret = XGetWindowProperty( x11display.dpy, win, atom, 0, bytes_left, False, AnyPropertyType, &type,
		&format, &nitems, &bytes_after, &data );
	if( ret == Success )
	{
		buffer = Q_malloc( bytes_left + 1 );
		Q_strncpyz( buffer, (char *)data, bytes_left + 1 );
	}
	else
	{
		buffer = NULL;
	}

	XFree( data );

	return buffer;
}


/*
* Sys_SetClipboardData
*/
qboolean Sys_SetClipboardData( char *data )
{
	return qfalse;
}

/*
* Sys_FreeClipboardData
*/
void Sys_FreeClipboardData( char *data )
{
	Q_free( data );
}

/*
* _NET_WM_CHECK_SUPPORTED
*/
static qboolean _NET_WM_CHECK_SUPPORTED( Atom NET_ATOM )
{
	qboolean issupported = qfalse;
	unsigned char *atomdata;
	Atom *atoms;
	int status, real_format;
	Atom real_type;
	unsigned long items_read, items_left, i;
	int result = 1;
	Atom _NET_SUPPORTED;

	_NET_SUPPORTED = XInternAtom( x11display.dpy, "_NET_SUPPORTED", 0 );

	status = XGetWindowProperty( x11display.dpy, x11display.root, _NET_SUPPORTED,
		0L, 8192L, False, XA_ATOM, &real_type, &real_format,
		&items_read, &items_left, &atomdata );

	if( status != Success )
		return qfalse;

	atoms = (Atom *)atomdata;
	for( i = 0; result && i < items_read; i++ )
	{
		if( atoms[i] == NET_ATOM )
		{
			issupported = qtrue;
			break;
		}
	}

	XFree( atomdata );
	return issupported;
}

/*
* _NET_WM_STATE_FULLSCREEN_SUPPORTED
*/
static qboolean _NET_WM_STATE_FULLSCREEN_SUPPORTED( void )
{
	Atom _NET_WM_STATE_FULLSCREEN = XInternAtom( x11display.dpy, "_NET_WM_STATE_FULLSCREEN", 0 );
	return _NET_WM_CHECK_SUPPORTED( _NET_WM_STATE_FULLSCREEN );
}

/*
* _NETWM_CHECK_FULLSCREEN
*/
qboolean _NETWM_CHECK_FULLSCREEN( void )
{
	qboolean isfullscreen = qfalse;
	unsigned char *atomdata;
	Atom *atoms;
	int status, real_format;
	Atom real_type;
	unsigned long items_read, items_left, i;
	int result = 1;
	Atom _NET_WM_STATE;
	Atom _NET_WM_STATE_FULLSCREEN;
	cvar_t *vid_fullscreen;

	if( !x11display.features.wmStateFullscreen )
		return qfalse;

	_NET_WM_STATE = XInternAtom( x11display.dpy, "_NET_WM_STATE", 0 );
	_NET_WM_STATE_FULLSCREEN = XInternAtom( x11display.dpy, "_NET_WM_STATE_FULLSCREEN", 0 );

	status = XGetWindowProperty( x11display.dpy, x11display.win, _NET_WM_STATE,
		0L, 8192L, False, XA_ATOM, &real_type, &real_format,
		&items_read, &items_left, &atomdata );

	if( status != Success )
		return qfalse;

	atoms = (Atom *)atomdata;
	for( i = 0; result && i < items_read; i++ )
	{
		if( atoms[i] == _NET_WM_STATE_FULLSCREEN )
		{
			isfullscreen = qtrue;
			break;
		}
	}

	vid_fullscreen = Cvar_Get( "vid_fullscreen", "0", CVAR_ARCHIVE );
	if( isfullscreen )
	{
		glState.fullScreen = qtrue;
		Cvar_SetValue( vid_fullscreen->name, 1 );
		vid_fullscreen->modified = qfalse;
	}
	else
	{
		glState.fullScreen = qfalse;
		Cvar_SetValue( vid_fullscreen->name, 0 );
		vid_fullscreen->modified = qfalse;
	}

	XFree( atomdata );
	return isfullscreen;
}

/*
* _NETWM_SET_FULLSCREEN
*
* Tell Window-Manager to toggle fullscreen
*/
void _NETWM_SET_FULLSCREEN( qboolean fullscreen )
{
	XEvent xev;
	Atom wm_state;
	Atom wm_fullscreen;

	if( !x11display.features.wmStateFullscreen )
		return;

	wm_state = XInternAtom( x11display.dpy, "_NET_WM_STATE", False );
	wm_fullscreen = XInternAtom( x11display.dpy, "_NET_WM_STATE_FULLSCREEN", False );

	memset(&xev, 0, sizeof(xev));
	xev.type = ClientMessage;
	xev.xclient.window = x11display.win;
	xev.xclient.message_type = wm_state;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = fullscreen ? 1 : 0;
	xev.xclient.data.l[1] = wm_fullscreen;
	xev.xclient.data.l[2] = 0;

	XSendEvent( x11display.dpy, DefaultRootWindow( x11display.dpy ), False,
		SubstructureNotifyMask, &xev );
}

/*
* Sys_OpenURLInBrowser
*/
void Sys_OpenURLInBrowser( const char *url )
{
    int r;

    r = system( va( "xdg-open \"%s\"", url ) );
    if( r == 0 ) {
		// FIXME: XIconifyWindow does minimize the window, however
		// it seems that FocusIn even which follows grabs the input afterwards
		// XIconifyWindow( x11display.dpy, x11display.win, x11display.scr );
    }
}

/*****************************************************************************/

static void GLimp_SetXPMIcon( const int *xpm_icon )
{
	int width, height;
	size_t i, cardinalSize;
	long *cardinalData;
	Atom NET_WM_ICON;
	Atom CARDINAL;

	// allocate memory for icon data: width + height + width * height pixels
	// note: sizeof(long) shall be used according to XChangeProperty() man page
	width = xpm_icon[0];
	height = xpm_icon[1];
	cardinalSize = width * height + 2;
	cardinalData = Q_malloc( cardinalSize * sizeof( *cardinalData ) );
	for( i = 0; i < cardinalSize; i++ )
		cardinalData[i] = xpm_icon[i];

	NET_WM_ICON = XInternAtom( x11display.dpy, "_NET_WM_ICON", False );
	CARDINAL = XInternAtom( x11display.dpy, "CARDINAL", False );

	XChangeProperty( x11display.dpy, x11display.win, NET_WM_ICON, CARDINAL, 32,
		PropModeReplace, (unsigned char *)cardinalData, cardinalSize );

	Q_free( cardinalData );
}

int *parse_xpm_icon ( int num_xpm_elements, char *xpm_data[] );

static void GLimp_SetApplicationIcon( void )
{
#include "warsow128x128.xpm"

	const int *xpm_icon;

	xpm_icon = parse_xpm_icon( sizeof( warsow128x128_xpm ) / sizeof( warsow128x128_xpm[0] ), warsow128x128_xpm );
	if( xpm_icon )
	{
		GLimp_SetXPMIcon( xpm_icon );
		free( ( void * )xpm_icon );
	}
}

/*****************************************************************************/

/*
** GLimp_SetMode_Real
* Hack to get rid of the prints when toggling fullscreen
*/
static rserr_t GLimp_SetMode_Real( int width, int height, qboolean fullscreen, qboolean wideScreen, qboolean silent )
{
	int screen_x, screen_y, screen_width, screen_height, screen_mode;
	float ratio;
	XSetWindowAttributes wa;
	unsigned long mask;

	screen_mode = -1;
	screen_x = screen_y = 0;
	screen_width = width;
	screen_height = height;

	x11display.old_win = x11display.win;

	if( fullscreen )
	{
		if( !_xf86_xinerama_supported ||
			!_xf86_XineramaFindBest( &screen_x, &screen_y, &screen_width, &screen_height, silent ) )
		{
#ifdef _XRANDR_OVER_VIDMODE_
			_xf86_XrandrFindBest( &screen_mode, &screen_width, &screen_height, silent );
#else
			_xf86_VidmodesFindBest( &screen_mode, &screen_width, &screen_height, silent );
#endif
			if( screen_mode < 0 )
			{
				if( !silent )
					Com_Printf( " no mode found\n" );
				return rserr_invalid_mode;
			}
		}

		if( screen_width < width || screen_height < height )
		{
			if( width > height )
			{
				ratio = width / height;
				height = height * ratio;
				width = screen_width;
			}
			else
			{
				ratio = height / width;
				width = width * ratio;
				height = screen_height;
			}
		}

		if( !silent )
			Com_Printf( "...setting fullscreen mode %ix%i:\n", width, height );

		/* Create fulscreen window */
		wa.background_pixel = 0;
		wa.border_pixel = 0;
		wa.event_mask = INIT_MASK;
		wa.backing_store = NotUseful;
		wa.save_under = False;

		if( x11display.features.wmStateFullscreen )
		{
			wa.override_redirect = False;
			mask = CWBackPixel | CWBorderPixel | CWEventMask | CWSaveUnder | CWBackingStore;
		}
		else
		{
			wa.override_redirect = True;
			mask = CWBackPixel | CWBorderPixel | CWEventMask | CWSaveUnder | CWBackingStore | CWOverrideRedirect;
		}

		x11display.wa = wa;

		x11display.win = XCreateWindow( x11display.dpy, x11display.root, screen_x, screen_y, screen_width, screen_height,
			0, CopyFromParent, InputOutput, CopyFromParent, mask, &wa );

		XResizeWindow( x11display.dpy, x11display.gl_win, width, height );
		XReparentWindow( x11display.dpy, x11display.gl_win, x11display.win, ( screen_width/2 )-( width/2 ),
			( screen_height/2 )-( height/2 ) );

		x11display.modeset = qtrue;

		XMapWindow( x11display.dpy, x11display.gl_win );
		XMapWindow( x11display.dpy, x11display.win );

		if ( !x11display.features.wmStateFullscreen )
			_x11_SetNoResize( x11display.win, width, height );
		else
			_NETWM_SET_FULLSCREEN( qtrue );

		if( screen_mode != -1 )
		{
#ifdef _XRANDR_OVER_VIDMODE_
			_xf86_XrandrSwitch( screen_mode );
#else
			_xf86_VidmodesSwitch( screen_mode );
#endif
		}
	}
	else
	{
		if( !silent )
			Com_Printf( "...setting mode %ix%i:\n", width, height );

		/* Create managed window */
		wa.background_pixel = 0;
		wa.border_pixel = 0;
		wa.event_mask = INIT_MASK;
		mask = CWBackPixel | CWBorderPixel | CWEventMask;
		x11display.wa = wa;

		x11display.win = XCreateWindow( x11display.dpy, x11display.root, 0, 0, screen_width, screen_height,
			0, CopyFromParent, InputOutput, CopyFromParent, mask, &wa );
		x11display.wmDeleteWindow = XInternAtom( x11display.dpy, "WM_DELETE_WINDOW", False );
		XSetWMProtocols( x11display.dpy, x11display.win, &x11display.wmDeleteWindow, 1 );

		XResizeWindow( x11display.dpy, x11display.gl_win, width, height );
		XReparentWindow( x11display.dpy, x11display.gl_win, x11display.win, 0, 0 );

		x11display.modeset = qtrue;

		XMapWindow( x11display.dpy, x11display.gl_win );
		XMapWindow( x11display.dpy, x11display.win );

		if( !x11display.features.wmStateFullscreen )
			_x11_SetNoResize( x11display.win, width, height );

#ifdef _XRANDR_OVER_VIDMODE_
		_xf86_XrandrSwitchBack();
#else
		_xf86_VidmodesSwitchBack();
#endif
	}

	XSetStandardProperties( x11display.dpy, x11display.win, APPLICATION, None, None, NULL, 0, NULL );

	GLimp_SetApplicationIcon();

	XSetIconName( x11display.dpy, x11display.win, APPLICATION );
	XStoreName( x11display.dpy, x11display.win, APPLICATION );

	// save the parent window size for mouse use. this is not the gl context window
	x11display.win_width = width;
	x11display.win_height = height;

	if( x11display.old_win )
	{
		XDestroyWindow( x11display.dpy, x11display.old_win );
		x11display.old_win = 0;
	}

	XFlush( x11display.dpy );

	glState.width = width;
	glState.height = height;
	glState.fullScreen = fullscreen;
	glState.wideScreen = wideScreen;

	return rserr_ok;
}

/*
** GLimp_SetMode
*/
rserr_t GLimp_SetMode( int x, int y, int width, int height, qboolean fullscreen, qboolean wideScreen )
{
	return GLimp_SetMode_Real( width, height, fullscreen, wideScreen, qfalse );
}

/*
** GLimp_Shutdown
*/
void GLimp_Shutdown( void )
{
	if( x11display.dpy )
	{
#ifdef _XRANDR_OVER_VIDMODE_
		_xf86_XrandrSwitchBack();
		_xf86_XrandrFree();
#else
		_xf86_VidmodesSwitchBack();
		_xf86_VidmodesFree();
#endif
		_xf86_XineramaFree();

		if( x11display.cmap ) XFreeColormap( x11display.dpy, x11display.cmap );
		if( x11display.ctx ) qglXDestroyContext( x11display.dpy, x11display.ctx );
		if( x11display.gl_win ) XDestroyWindow( x11display.dpy, x11display.gl_win );
		if( x11display.win ) XDestroyWindow( x11display.dpy, x11display.win );

		XCloseDisplay( x11display.dpy );
	}

	memset(&x11display.features, 0, sizeof(x11display.features));
	x11display.modeset = qfalse;
	x11display.visinfo = NULL;
	x11display.cmap = 0;
	x11display.ctx = NULL;
	x11display.gl_win = 0;
	x11display.win = 0;
	x11display.dpy = NULL;

	if( x11wndproc ) {
		x11wndproc( NULL, 0, 0, 0 );
	}
}

static qboolean gotstencil = qfalse; // evil hack!
static qboolean ChooseVisual( int colorbits, int stencilbits )
{
	int colorsize;
	int depthbits = colorbits;

	if( colorbits == 24 ) colorsize = 8;
	else colorsize = 4;

	{
		int attributes[] = {
			GLX_RGBA,
			GLX_DOUBLEBUFFER,
			GLX_RED_SIZE, colorsize,
			GLX_GREEN_SIZE, colorsize,
			GLX_BLUE_SIZE, colorsize,
			GLX_DEPTH_SIZE, depthbits,
			GLX_STENCIL_SIZE, stencilbits,
			None
		};

		x11display.visinfo = qglXChooseVisual( x11display.dpy, x11display.scr, attributes );
		if( !x11display.visinfo )
		{
			Com_Printf( "..Failed to get colorbits %i, depthbits %i, stencilbits %i\n", colorbits, depthbits, stencilbits );
			return qfalse;
		}
		else
		{
			Com_Printf( "..Got colorbits %i, depthbits %i, stencilbits %i\n", colorbits, depthbits, stencilbits );
			if( stencilbits > 0 ) gotstencil = qtrue;
			return qtrue;
		}
	}
}

/*
** GLimp_Init
*/
int GLimp_Init( void *hinstance, void *wndproc, void *parenthWnd )
{
	int colorbits, stencilbits;
	XSetWindowAttributes attr;
	unsigned long mask;

	hinstance = NULL;
	x11wndproc = (x11wndproc_t )wndproc;

	if( x11display.dpy )
		GLimp_Shutdown();

	Com_Printf( "Display initialization\n" );

	x11display.dpy = XOpenDisplay( NULL );
	if( !x11display.dpy )
	{
		Com_Printf( "..Error couldn't open the X display\n" );
		return 0;
	}

	x11display.scr = DefaultScreen( x11display.dpy );
	if( parenthWnd )
		x11display.root = (Window )parenthWnd;
	else
		x11display.root = RootWindow( x11display.dpy, x11display.scr );

	x11display.wmState = XInternAtom( x11display.dpy, "WM_STATE", False );
	x11display.features.wmStateFullscreen = _NET_WM_STATE_FULLSCREEN_SUPPORTED();

#ifdef _XRANDR_OVER_VIDMODE_
	_xf86_XrandrInit();
#else
	_xf86_VidmodesInit();
#endif
	_xf86_XineramaInit();

	colorbits = 0;

	if( r_stencilbits->integer == 8 || r_stencilbits->integer == 16 ) stencilbits = r_stencilbits->integer;
	else stencilbits = 0;

	gotstencil = qfalse;
	if( colorbits > 0 )
	{
		ChooseVisual( colorbits, stencilbits );
		if( !x11display.visinfo && stencilbits > 8 ) ChooseVisual( colorbits, 8 );
		if( !x11display.visinfo && stencilbits > 0 ) ChooseVisual( colorbits, 0 );
		if( !x11display.visinfo && colorbits > 16 ) ChooseVisual( 16, 0 );
	}
	else
	{
		ChooseVisual( 24, stencilbits );
		if( !x11display.visinfo ) ChooseVisual( 16, stencilbits );
		if( !x11display.visinfo && stencilbits > 8 ) ChooseVisual( 24, 8 );
		if( !x11display.visinfo && stencilbits > 8 ) ChooseVisual( 16, 8 );
		if( !x11display.visinfo && stencilbits > 0 ) ChooseVisual( 24, 0 );
		if( !x11display.visinfo && stencilbits > 0 ) ChooseVisual( 16, 0 );
	}

	if( !x11display.visinfo )
	{
		GLimp_Shutdown(); // hope this doesn't do anything evil when we don't have window etc.
		Com_Printf( "..Error couldn't set GLX visual\n" );
		return 0;
	}

	glState.stencilEnabled = gotstencil;

	x11display.ctx = qglXCreateContext( x11display.dpy, x11display.visinfo, NULL, True );
	x11display.cmap = XCreateColormap( x11display.dpy, x11display.root, x11display.visinfo->visual, AllocNone );

	attr.colormap = x11display.cmap;
	attr.border_pixel = 0;
	attr.event_mask = DISPLAY_MASK;
	attr.override_redirect = False;
	mask = CWBorderPixel | CWColormap | ExposureMask;

	x11display.gl_win = XCreateWindow( x11display.dpy, x11display.root, 0, 0, 1, 1, 0,
		x11display.visinfo->depth, InputOutput, x11display.visinfo->visual, mask, &attr );
	qglXMakeCurrent( x11display.dpy, x11display.gl_win, x11display.ctx );

	XSync( x11display.dpy, False );
	
	if( x11wndproc ) {
		x11wndproc( &x11display, 0, 0, 0 );
	}

	return 1;
}

/*
** GLimp_BeginFrame
*/
void GLimp_BeginFrame( void )
{
}

/*
** GLimp_EndFrame
**
** Responsible for doing a swapbuffers and possibly for other stuff
** as yet to be determined.  Probably better not to make this a GLimp
** function and instead do a call to GLimp_SwapBuffers.
*/
void GLimp_EndFrame( void )
{
	qglXSwapBuffers( x11display.dpy, x11display.gl_win );

	if( glState.fullScreen && vid_multiscreen_head->modified )
	{
		GLimp_SetMode_Real( glState.width, glState.height, qtrue, glState.wideScreen, qtrue );
		vid_multiscreen_head->modified = qfalse;
	}
}

/*
** GLimp_GetGammaRamp
*/
qboolean GLimp_GetGammaRamp( size_t stride, unsigned short *ramp )
{
	if( XF86VidModeGetGammaRamp( x11display.dpy, x11display.scr, stride, ramp, ramp + stride, ramp + ( stride << 1 ) ) != 0 )
		return qtrue;
	return qfalse;
}

/*
** GLimp_SetGammaRamp
*/
void GLimp_SetGammaRamp( size_t stride, unsigned short *ramp )
{
	XF86VidModeSetGammaRamp( x11display.dpy, x11display.scr, stride, ramp, ramp + stride, ramp + ( stride << 1 ) );
}

/*
** GLimp_AppActivate
*/
void GLimp_AppActivate( qboolean active, qboolean destroy )
{
}
