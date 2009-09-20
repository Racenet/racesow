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

#include "x11.h"

#include "../ref_gl/r_local.h"
#include "../client/keys.h"

#include "unix_glw.h"

#define DISPLAY_MASK ( VisibilityChangeMask | StructureNotifyMask | ExposureMask )
#define INIT_MASK ( KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | FocusChangeMask | DISPLAY_MASK )

x11display_t x11display;

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
   =================
   Sys_GetClipboardData

   Orginally from EzQuake
   There should be a smarter place to put this
   =================
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
   =================
   Sys_FreeClipboardData
   =================
 */
void Sys_FreeClipboardData( char *data )
{
	Q_free( data );
}


/*****************************************************************************/

/*
** GLimp_SetMode_Real
* Hack to get rid of the prints when toggling fullscreen
*/
int GLimp_SetMode_Real( int mode, qboolean fullscreen, qboolean silent )
{
	int width, height, screen_x, screen_y, screen_width, screen_height, screen_mode;
	float ratio;
	XSetWindowAttributes wa;
	unsigned long mask;
	qboolean wideScreen;

	if( !VID_GetModeInfo( &width, &height, &wideScreen, mode ) )
	{
		if( !silent )
			Com_Printf( " invalid mode\n" );
		return rserr_invalid_mode;
	}

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
			_xf86_VidmodesFindBest( &screen_mode, &screen_width, &screen_height, silent );
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
			Com_Printf( "...setting fullscreen mode %d:\n", mode );

		/* Create fulscreen window */
		wa.background_pixel = 0;
		wa.border_pixel = 0;
		wa.event_mask = INIT_MASK;
		wa.override_redirect = True;
		wa.backing_store = NotUseful;
		wa.save_under = False;
		mask = CWBackPixel | CWBorderPixel | CWEventMask | CWSaveUnder | CWBackingStore | CWOverrideRedirect;

		x11display.win = XCreateWindow( x11display.dpy, x11display.root, screen_x, screen_y, screen_width, screen_height,
		                                0, CopyFromParent, InputOutput, CopyFromParent, mask, &wa );

		XResizeWindow( x11display.dpy, x11display.gl_win, width, height );
		XReparentWindow( x11display.dpy, x11display.gl_win, x11display.win, ( screen_width/2 )-( width/2 ),
		                ( screen_height/2 )-( height/2 ) );

		XMapWindow( x11display.dpy, x11display.gl_win );
		XMapWindow( x11display.dpy, x11display.win );

		_x11_SetNoResize( x11display.win, screen_width, screen_height );

		if( screen_mode != -1 )
			_xf86_VidmodesSwitch( screen_mode );
	}
	else
	{
		if( !silent )
			Com_Printf( "...setting mode %d:\n", mode );

		/* Create managed window */
		wa.background_pixel = 0;
		wa.border_pixel = 0;
		wa.event_mask = INIT_MASK;
		mask = CWBackPixel | CWBorderPixel | CWEventMask;

		x11display.win = XCreateWindow( x11display.dpy, x11display.root, 0, 0, screen_width, screen_height,
		                                0, CopyFromParent, InputOutput, CopyFromParent, mask, &wa );
		x11display.wmDeleteWindow = XInternAtom( x11display.dpy, "WM_DELETE_WINDOW", False );
		XSetWMProtocols( x11display.dpy, x11display.win, &x11display.wmDeleteWindow, 1 );

		XResizeWindow( x11display.dpy, x11display.gl_win, width, height );
		XReparentWindow( x11display.dpy, x11display.gl_win, x11display.win, 0, 0 );

		XMapWindow( x11display.dpy, x11display.gl_win );
		XMapWindow( x11display.dpy, x11display.win );

		_x11_SetNoResize( x11display.win, width, height );

		_xf86_VidmodesSwitchBack();
	}

	XSetStandardProperties( x11display.dpy, x11display.win, APPLICATION, None, None, NULL, 0, NULL );
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

	// let the sound and input subsystems know about the new window
	VID_NewWindow( width, height );

	return rserr_ok;
}

/*
** GLimp_SetMode
*/
int GLimp_SetMode( int mode, qboolean fullscreen )
{
	return GLimp_SetMode_Real( mode, fullscreen, qfalse );
}

/*
** GLimp_Shutdown
*/
void GLimp_Shutdown( void )
{
	if( x11display.dpy )
	{
		_xf86_VidmodesSwitchBack();
		_xf86_VidmodesFree();
		_xf86_XineramaFree();

		if( x11display.cmap ) XFreeColormap( x11display.dpy, x11display.cmap );
		if( x11display.ctx ) qglXDestroyContext( x11display.dpy, x11display.ctx );
		if( x11display.gl_win ) XDestroyWindow( x11display.dpy, x11display.gl_win );
		if( x11display.win ) XDestroyWindow( x11display.dpy, x11display.win );

		XCloseDisplay( x11display.dpy );
	}

	x11display.visinfo = NULL;
	x11display.cmap = 0;
	x11display.ctx = NULL;
	x11display.gl_win = 0;
	x11display.win = 0;
	x11display.dpy = NULL;
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
int GLimp_Init( void *hinstance, void *wndproc )
{
	int colorbits, stencilbits;
	XSetWindowAttributes attr;
	unsigned long mask;

	hinstance = NULL;
	wndproc = NULL;

	// on unix we can change to fullscreen on fly
	vid_fullscreen->flags &= ~( CVAR_LATCH_VIDEO );

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
	x11display.root = RootWindow( x11display.dpy, x11display.scr );

	_xf86_VidmodesInit();
	_xf86_XineramaInit();

	if( r_colorbits->integer == 16 || r_colorbits->integer == 24 ) colorbits = r_colorbits->integer;
	else colorbits = 0;

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
	mask = CWBorderPixel | CWColormap;

	x11display.gl_win = XCreateWindow( x11display.dpy, x11display.root, 0, 0, 1, 1, 0,
	                                   x11display.visinfo->depth, InputOutput, x11display.visinfo->visual, mask, &attr );
	qglXMakeCurrent( x11display.dpy, x11display.gl_win, x11display.ctx );

	XSync( x11display.dpy, False );

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

	if( vid_fullscreen->modified || ( vid_fullscreen->integer && vid_multiscreen_head->modified ) )
	{
		GLimp_SetMode_Real( r_mode->integer, vid_fullscreen->integer, qtrue );
		vid_fullscreen->modified = qfalse;
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
void GLimp_AppActivate( qboolean active )
{
}
