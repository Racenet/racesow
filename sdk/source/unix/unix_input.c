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

//#define KEYBINDINGS_HARDCODED
//#define PRINT_HARDCODING_TABLES
//#define MAX_HARDCODED_KEYS 118 // also in x11_hardcoded.h

#include "x11.h"
#include "../client/client.h"
#ifdef KEYBINDINGS_HARDCODED
#include "x11_hardcoded.h"
#endif

// TODO: add in_mouse?
static cvar_t *in_dgamouse;

cvar_t *in_grabinconsole;

static qboolean focus = qfalse;
static qboolean mapped = qfalse;

static qboolean input_inited = qfalse;
static qboolean mouse_active = qfalse;
static qboolean input_active = qfalse;
static qboolean dgamouse = qfalse;

static int mx, my;

static qboolean ignore_one = qfalse;

int Sys_XTimeToSysTime( unsigned long xtime );

static Cursor CreateNullCursor( Display *display, Window root )
{
	Pixmap cursormask;
	XGCValues xgc;
	GC gc;
	XColor dummycolour;
	Cursor cursor;

	cursormask = XCreatePixmap( display, root, 1, 1, 1 /*depth*/ );
	xgc.function = GXclear;
	gc = XCreateGC( display, cursormask, GCFunction, &xgc );
	XFillRectangle( display, cursormask, gc, 0, 0, 1, 1 );

	dummycolour.pixel = 0;
	dummycolour.red = 0;
	dummycolour.flags = 04;

	cursor = XCreatePixmapCursor( display, cursormask, cursormask, &dummycolour, &dummycolour, 0, 0 );
	XFreePixmap( display, cursormask );
	XFreeGC( display, gc );

	return cursor;
}


static void install_grabs( void )
{
	assert( x11display.dpy && x11display.win );	
	
	if( XGrabKeyboard( x11display.dpy, x11display.win, False, GrabModeAsync, GrabModeAsync, CurrentTime ) != GrabSuccess )
	{
		Com_Printf( "Warning: XGrabKeyboard failed\n" );
		return;
	}

	XDefineCursor( x11display.dpy, x11display.win, CreateNullCursor( x11display.dpy, x11display.win ) );

	if( XGrabPointer( x11display.dpy, x11display.win, True, 0, GrabModeAsync, GrabModeAsync, x11display.win, None,
	                  CurrentTime ) != GrabSuccess )
	{
		Com_Printf( "Warning: XGrabPointer failed\n" );
		XUngrabKeyboard( x11display.dpy, CurrentTime );
		XUndefineCursor( x11display.dpy, x11display.win );
		return;
	}

	if( in_dgamouse->integer )
	{
		int MajorVersion, MinorVersion;

		if( XF86DGAQueryVersion( x11display.dpy, &MajorVersion, &MinorVersion ) )
		{
			XF86DGADirectVideo( x11display.dpy, x11display.scr, XF86DGADirectMouse );
			XWarpPointer( x11display.dpy, None, x11display.win, 0, 0, 0, 0, 0, 0 );
			dgamouse = qtrue;

		}
		else
		{
			// unable to query, probalby not supported
			Com_Printf( "Failed to detect XF86DGA Mouse\n" );
			Cvar_Set( "in_dgamouse", "0" );
			dgamouse = qfalse;
		}
	}
	else
	{
		XWarpPointer( x11display.dpy, None, x11display.win, 0, 0, 0, 0,
		              x11display.win_width/2, x11display.win_height/2 );
	}	

	ignore_one = qtrue; // first mouse update after install_grabs is ignored
	mx = my = 0;
	mouse_active = qtrue;

	in_dgamouse->modified = qfalse;

	input_active = qtrue;
}

static void uninstall_grabs( void )
{
	assert( x11display.dpy && x11display.win );

	if( mouse_active )
	{
		if( dgamouse )
		{
			dgamouse = qfalse;
			XF86DGADirectVideo( x11display.dpy, x11display.scr, 0 );
		}

		XUngrabPointer( x11display.dpy, CurrentTime );
		XUndefineCursor( x11display.dpy, x11display.win ); // inviso cursor

		mouse_active = qfalse;
		mx = my = 0;
	}

	XUngrabKeyboard( x11display.dpy, CurrentTime );

	input_active = qfalse;
}

#ifdef PRINT_HARDCODING_TABLES

static char *KeysymToKey( KeySym keysym )
{
	char *buf;

	switch( keysym )
	{
	case XK_KP_Page_Up:
	case XK_KP_9:  return "KP_PGUP";
	case XK_Page_Up:   return "K_PGUP";

	case XK_KP_Page_Down:
	case XK_KP_3: return "KP_PGDN";
	case XK_Page_Down:   return "K_PGDN";

	case XK_KP_Home: return "KP_HOME";
	case XK_KP_7: return "KP_HOME";
	case XK_Home:  return "K_HOME";

	case XK_KP_End:
	case XK_KP_1:   return "KP_END";
	case XK_End:   return "K_END";

	case XK_KP_Left: return "KP_LEFTARROW";
	case XK_KP_4: return "KP_LEFTARROW";
	case XK_Left:  return "K_LEFTARROW";

	case XK_KP_Right: return "KP_RIGHTARROW";
	case XK_KP_6: return "KP_RIGHTARROW";
	case XK_Right:  return "K_RIGHTARROW";

	case XK_KP_Down:
	case XK_KP_2:    return "KP_DOWNARROW";
	case XK_Down:  return "K_DOWNARROW";

	case XK_KP_Up:
	case XK_KP_8:    return "KP_UPARROW";
	case XK_Up:    return "K_UPARROW";

	case XK_Escape: return "K_ESCAPE";

	case XK_KP_Enter: return "KP_ENTER";
	case XK_Return: return "K_ENTER";

	case XK_Tab:    return "K_TAB";

	case XK_F1:    return "K_F1";
	case XK_F2:    return "K_F2";
	case XK_F3:    return "K_F3";
	case XK_F4:    return "K_F4";
	case XK_F5:    return "K_F5";
	case XK_F6:    return "K_F6";
	case XK_F7:    return "K_F7";
	case XK_F8:    return "K_F8";
	case XK_F9:    return "K_F9";
	case XK_F10:    return "K_F10";
	case XK_F11:    return "K_F11";
	case XK_F12:    return "K_F12";

		// bk001206 - from Ryan's Fakk2
		//case XK_BackSpace: return 8; // ctrl-h
	case XK_BackSpace: return "K_BACKSPACE";

	case XK_KP_Delete:
	case XK_KP_Decimal: return "KP_DEL";
	case XK_Delete: return "K_DEL";

	case XK_Pause:  return "K_PAUSE";

	case XK_Shift_L:
	case XK_Shift_R:  return "K_SHIFT";

	case XK_Execute:
	case XK_Control_L:
	case XK_Control_R:  return "K_CTRL";

	case XK_Alt_L:
	case XK_Meta_L:
	case XK_Alt_R:
	case XK_Meta_R: return "K_ALT";

	case XK_Super_L:
	case XK_Super_R:     return "K_WIN";

	case XK_Menu: return "K_MENU";

	case XK_KP_Begin: return "KP_5";

	case XK_Insert:   return "K_INS";
	case XK_KP_Insert:
	case XK_KP_0: return "KP_INS";

	case XK_KP_Multiply: return "KP_STAR";
	case XK_KP_Add:  return "KP_PLUS";
	case XK_KP_Subtract: return "KP_MINUS";
	case XK_KP_Divide: return "KP_SLASH";

		// bk001130 - from cvs1.17 (mkv)
	case XK_exclam: return va( "%i", '1' );
	case XK_at: return va( "%i", '2' );
	case XK_numbersign: return va( "%i", '3' );
	case XK_dollar: return va( "%i", '4' );
	case XK_percent: return va( "%i", '5' );
	case XK_asciicircum: return va( "%i", '6' );
	case XK_ampersand: return va( "%i", '7' );
	case XK_asterisk: return va( "%i", '8' );
	case XK_parenleft: return va( "%i", '9' );
	case XK_parenright: return va( "%i", '0' );
		// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=472
	case XK_space:
	case XK_KP_Space: return "K_SPACE";

		// wsw : mdr
	case XK_Caps_Lock: return "K_CAPSLOCK";
	case XK_Num_Lock: return "K_NUMLOCK";
	case XK_Scroll_Lock: return "K_SCROLLLOCK";
	}

	buf = XKeysymToString( keysym );
	if( buf != NULL )
	{
		int key = *(unsigned char *)buf;
		if( strlen( buf ) == 1 && ( key >= 9 && key <= 127 ) )
			return va( "%i", key );
	}

	if( keysym >= 9 && keysym < 127 )
		return va( "%i", (int)keysym );

	return "-1"; // no luck
}

static void PrintKeycodeToKeysymToKeyTable( void )
{
	int i;
	KeySym keysym;
	char *key;

	printf( "#define MAX_HARDCODED_KEYS %i\n", MAX_HARDCODED_KEYS );
	printf( "\n" );
	printf( "int keycode_to_keysym_key[MAX_HARDCODED_KEYS][2] = {\n" );

	for( i = 0; i < MAX_HARDCODED_KEYS; i++ )
	{
		keysym = XKeycodeToKeysym( x11display.dpy, i, 0 );
		key = KeysymToKey( keysym );
		printf( "\t{ %i, %s },\n", (int)keysym, key );
	}

	printf( "};\n" );
}

#endif // PRINT_HARDCODING_TABLES

#ifdef KEYBINDINGS_HARDCODED

// Q3 version
static char *XLateKey( XKeyEvent *ev, int *key )
{
	static char buf[64];

#ifdef PRINT_HARDCODING_TABLES
	static qboolean done = qfalse;
	if( !done )
	{
		PrintKeycodeToKeysymToKeyTable();
		done = qtrue;
	}
#endif

	// binds come from hardcoded US keymap
	if( ev->keycode < MAX_HARDCODED_KEYS )
		*key = keycode_to_keysym_key[ev->keycode][1];
	else
		*key = -1; // can't handle it

	// char events use the real layout
	XLookupString( ev, buf, sizeof( buf ), NULL, 0 );

	return buf;
}

#else // KEYBINDINGS_HARDCODED

// Q3 version
static char *XLateKey( XKeyEvent *ev, int *key )
{
	static char buf[64];
	KeySym keysym;
	int XLookupRet;

#ifdef PRINT_HARDCODING_TABLES
	static qboolean done = qfalse;
	if( !done )
	{
		PrintKeycodeToKeysymToKeyTable();
		done = qtrue;
	}
#endif

	*key = 0;

	XLookupRet = XLookupString( ev, buf, sizeof buf, &keysym, 0 );
#ifdef KBD_DBG
	ri.Printf( "XLookupString ret: %d buf: %s keysym: %x\n", XLookupRet, buf, keysym );
#endif

	switch( keysym )
	{
	case XK_KP_Page_Up:
	case XK_KP_9:  *key = KP_PGUP; break;
	case XK_Page_Up:   *key = K_PGUP; break;

	case XK_KP_Page_Down:
	case XK_KP_3: *key = KP_PGDN; break;
	case XK_Page_Down:   *key = K_PGDN; break;

	case XK_KP_Home: *key = KP_HOME; break;
	case XK_KP_7: *key = KP_HOME; break;
	case XK_Home:  *key = K_HOME; break;

	case XK_KP_End:
	case XK_KP_1:   *key = KP_END; break;
	case XK_End:   *key = K_END; break;

	case XK_KP_Left: *key = KP_LEFTARROW; break;
	case XK_KP_4: *key = KP_LEFTARROW; break;
	case XK_Left:  *key = K_LEFTARROW; break;

	case XK_KP_Right: *key = KP_RIGHTARROW; break;
	case XK_KP_6: *key = KP_RIGHTARROW; break;
	case XK_Right:  *key = K_RIGHTARROW;    break;

	case XK_KP_Down:
	case XK_KP_2:    *key = KP_DOWNARROW; break;
	case XK_Down:  *key = K_DOWNARROW; break;

	case XK_KP_Up:
	case XK_KP_8:    *key = KP_UPARROW; break;
	case XK_Up:    *key = K_UPARROW;   break;

	case XK_Escape: *key = K_ESCAPE;    break;

	case XK_KP_Enter: *key = KP_ENTER;  break;
	case XK_Return: *key = K_ENTER;    break;

	case XK_Tab:    *key = K_TAB;      break;

	case XK_F1:    *key = K_F1;       break;

	case XK_F2:    *key = K_F2;       break;

	case XK_F3:    *key = K_F3;       break;

	case XK_F4:    *key = K_F4;       break;

	case XK_F5:    *key = K_F5;       break;

	case XK_F6:    *key = K_F6;       break;

	case XK_F7:    *key = K_F7;       break;

	case XK_F8:    *key = K_F8;       break;

	case XK_F9:    *key = K_F9;       break;

	case XK_F10:    *key = K_F10;      break;

	case XK_F11:    *key = K_F11;      break;

	case XK_F12:    *key = K_F12;      break;

		// bk001206 - from Ryan's Fakk2
		//case XK_BackSpace: *key = 8; break; // ctrl-h
	case XK_BackSpace: *key = K_BACKSPACE; break; // ctrl-h

	case XK_KP_Delete:
	case XK_KP_Decimal: *key = KP_DEL; break;
	case XK_Delete: *key = K_DEL; break;

	case XK_Pause:  *key = K_PAUSE;    break;

	case XK_Shift_L:
	case XK_Shift_R:  *key = K_SHIFT;   break;

	case XK_Execute:
	case XK_Control_L:
	case XK_Control_R:  *key = K_CTRL;  break;

	case XK_Alt_L:
	case XK_Meta_L:
	case XK_Alt_R:
	case XK_Meta_R: *key = K_ALT;     break;

	case XK_Super_L:
	case XK_Super_R:     *key = K_WIN;   break;

	case XK_Menu: *key = K_MENU;	break;

	case XK_KP_Begin: *key = KP_5;  break;

	case XK_Insert:   *key = K_INS; break;
	case XK_KP_Insert:
	case XK_KP_0: *key = KP_INS; break;

	case XK_KP_Multiply: *key = KP_STAR; break;
	case XK_KP_Add:  *key = KP_PLUS; break;
	case XK_KP_Subtract: *key = KP_MINUS; break;
	case XK_KP_Divide: *key = KP_SLASH; break;

		// bk001130 - from cvs1.17 (mkv)
	case XK_exclam: *key = '1'; break;
	case XK_at: *key = '2'; break;
	case XK_numbersign: *key = '3'; break;
	case XK_dollar: *key = '4'; break;
	case XK_percent: *key = '5'; break;
	case XK_asciicircum: *key = '6'; break;
	case XK_ampersand: *key = '7'; break;
	case XK_asterisk: *key = '8'; break;
	case XK_parenleft: *key = '9'; break;
	case XK_parenright: *key = '0'; break;

		// weird french keyboards ..
		// NOTE: console toggle is hardcoded in cl_keys.c, can't be unbound
		//   cleaner would be .. using hardware key codes instead of the key syms
		//   could also add a new K_KP_CONSOLE
	case XK_twosuperior: *key = '~'; break;

		// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=472
	case XK_space:
	case XK_KP_Space: *key = K_SPACE; break;

		// wsw : mdr
	case XK_Caps_Lock: *key = K_CAPSLOCK; break;
	case XK_Num_Lock: *key = K_NUMLOCK; break;
	case XK_Scroll_Lock: *key = K_SCROLLLOCK; break;

	default:
		if( XLookupRet == 0 )
		{
			/*
			   if (com_developer->value)
			   {
			    ri.Printf(PRINT_ALL, "Warning: XLookupString failed on KeySym %d\n", keysym);
			   }
			 */
			return NULL;
		}
		else
		{
			// XK_* tests failed, but XLookupString got a buffer, so let's try it
			*key = *(unsigned char *)buf;
			if( *key >= 'A' && *key <= 'Z' )
				*key = *key - 'A' + 'a';
			// if ctrl is pressed, the keys are not between 'A' and 'Z', for instance ctrl-z == 26 ^Z ^C etc.
			// see https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=19
			else if( *key >= 1 && *key <= 26 )
				*key = *key + 'a' - 1;
		}
		break;
	}

	return buf;
}

#endif // KEYBINDINGS_HARDCODED

/**
 * XPending() actually performs a blocking read if no events available. From Fakk2, by way of
 * Heretic2, by way of SDL, original idea GGI project. The benefit of this approach over the quite
 * badly behaved XAutoRepeatOn/Off is that you get focus handling for free, which is a major win
 * with debug and windowed mode. It rests on the assumption that the X server will use the same
 * timestamp on press/release event pairs for key repeats.
 */
static qboolean X11_PendingInput( void )
{
	assert( x11display.dpy );

	// Flush the display connection and look to see if events are queued
	XFlush( x11display.dpy );
	if( XEventsQueued( x11display.dpy, QueuedAlready ) )
		return qtrue;

	{ // More drastic measures are required -- see if X is ready to talk
		static struct timeval zero_time;
		int x11_fd;
		fd_set fdset;

		x11_fd = ConnectionNumber( x11display.dpy );
		FD_ZERO( &fdset );
		FD_SET( x11_fd, &fdset );
		if( select( x11_fd+1, &fdset, NULL, NULL, &zero_time ) == 1 )
			return ( XPending( x11display.dpy ) );
	}

	// Oh well, nothing is ready ..
	return qfalse;
}

static qboolean repeated_press( XEvent *event )
{
	XEvent peekevent;
	qboolean repeated = qfalse;

	assert( x11display.dpy );

	if( X11_PendingInput() )
	{
		XPeekEvent( x11display.dpy, &peekevent );

		if( ( peekevent.type == KeyPress ) &&
		   ( peekevent.xkey.keycode == event->xkey.keycode ) &&
		   ( peekevent.xkey.time == event->xkey.time ) )
		{
			repeated = qtrue;
			// we only skip the KeyRelease event, so we send many key down events, but no releases, while repeating
			//XNextEvent(x11display.dpy, &peekevent);  // skip event.
		}
	}

	return repeated;
}

static void HandleEvents( void )
{
	XEvent event;
	qboolean dowarp = qfalse;
	int mwx = x11display.win_width / 2;
	int mwy = x11display.win_height / 2;
	unsigned char *p;
	int key = 0;
	int time = 0;

	assert( x11display.dpy && x11display.win );

	if( mouse_active && !dgamouse )
	{
		int root_x, root_y, win_x, win_y;
		unsigned int mask;
		Window root, child;

		if( XQueryPointer( x11display.dpy, x11display.win, &root, &child,
		                   &root_x, &root_y, &win_x, &win_y, &mask ) )
		{
			mx += ( (int)win_x - mwx );
			my += ( (int)win_y - mwy );
			mwx = win_x;
			mwy = win_y;

			if( mx || my )
				dowarp = qtrue;

			if( ignore_one )
			{
				mx = my = 0;
				ignore_one = qfalse;
			}
		}
	}


	while( XPending( x11display.dpy ) )
	{
		XNextEvent( x11display.dpy, &event );

		switch( event.type )
		{
		case KeyPress:
			time = Sys_XTimeToSysTime(event.xkey.time);
			p = (unsigned char *)XLateKey( &event.xkey, &key );
			if( key >= 0 && key <= 256 )
				Key_Event( key, qtrue, time );
			if( p )
			{
				while( *p )
				{
					// the delete character is an ASCII code 127 (HACK!)
					if( *p != 127 )
						Key_CharEvent( key, *p );
					p++;
				}
			}
			break;

		case KeyRelease:
			if( repeated_press( &event ) )
				break; // don't send release events when repeating
				
			time = Sys_XTimeToSysTime(event.xkey.time);
			XLateKey( &event.xkey, &key );
			Key_Event( key, event.type == KeyPress, time );
			break;

		case MotionNotify:
			if( mouse_active && dgamouse )
			{
				mx += event.xmotion.x_root;
				my += event.xmotion.y_root;
				if( ignore_one )
				{
					mx = my = 0;
					ignore_one = qfalse;
				}
			}
			break;

		case ButtonPress:
			time = Sys_XTimeToSysTime(event.xkey.time);
			if( event.xbutton.button == 1 ) Key_MouseEvent( K_MOUSE1, 1, time );
			else if( event.xbutton.button == 2 ) Key_MouseEvent( K_MOUSE3, 1, time );
			else if( event.xbutton.button == 3 ) Key_MouseEvent( K_MOUSE2, 1, time );
			else if( event.xbutton.button == 4 ) Key_Event( K_MWHEELUP, 1, time );
			else if( event.xbutton.button == 5 ) Key_Event( K_MWHEELDOWN, 1, time );
			else if( event.xbutton.button >= 6 && event.xbutton.button <= 10 ) Key_MouseEvent( K_MOUSE4+event.xbutton.button-6, 1, time );
			break;

		case ButtonRelease:
			time = Sys_XTimeToSysTime(event.xkey.time);
			if( event.xbutton.button == 1 ) Key_MouseEvent( K_MOUSE1, 0, time );
			else if( event.xbutton.button == 2 ) Key_MouseEvent( K_MOUSE3, 0, time );
			else if( event.xbutton.button == 3 ) Key_MouseEvent( K_MOUSE2, 0, time );
			else if( event.xbutton.button == 4 ) Key_Event( K_MWHEELUP, 0, time );
			else if( event.xbutton.button == 5 ) Key_Event( K_MWHEELDOWN, 0, time );
			else if( event.xbutton.button >= 6 && event.xbutton.button <= 10 ) Key_MouseEvent( K_MOUSE4+event.xbutton.button-6, 0, time );
			break;

		case FocusIn:
			if( !focus )
			{
				focus = qtrue;
			}
			break;

		case FocusOut:
			if( focus )
			{
				Key_ClearStates();
				focus = qfalse;
			}
			break;

		case ClientMessage:
			if( event.xclient.data.l[0] == x11display.wmDeleteWindow )
				Cbuf_ExecuteText( EXEC_NOW, "quit" );
			break;

		case MapNotify:
			mapped = qtrue;
			if( input_active )
			{
				uninstall_grabs();
				install_grabs();
			}
			break;
		}
	}

	if( dowarp )
	{
		XWarpPointer( x11display.dpy, None, x11display.win, 0, 0, 0, 0,
		              x11display.win_width/2, x11display.win_height/2 );
	}
}

/*****************************************************************************/

void IN_Commands( void )
{
}

void IN_MouseMove( usercmd_t *cmd )
{
	if( mouse_active )
	{
		CL_MouseMove( cmd, mx, my );
		mx = my = 0;
	}
}

void IN_JoyMove( usercmd_t *cmd )
{
}

void IN_Activate( qboolean active )
{
	if( !input_inited )
		return;

	assert( x11display.dpy && x11display.win );

	if( active )
	{
		install_grabs();
	}
	else
	{
		uninstall_grabs();
	}
}


void IN_Init( void )
{
	if( input_inited )
		return;

	in_dgamouse = Cvar_Get( "in_dgamouse", "1", CVAR_ARCHIVE );
	in_grabinconsole = Cvar_Get( "in_grabinconsole", "0", CVAR_ARCHIVE );

	input_inited = qtrue;
	input_active = qfalse; // will be activated by IN_Frame if necessary
	mapped = qfalse;
}

void IN_Shutdown( void )
{
	if( !input_inited )
		return;

	uninstall_grabs();
	input_inited = qfalse;
}

void IN_Restart( void )
{
	IN_Shutdown();
	IN_Init();
}

void IN_Frame( void )
{
	if( !input_inited )
		return;

	if( !mapped || ( !Cvar_Value( "vid_fullscreen" ) && ( !focus || ( ( cls.key_dest == key_console ) && !in_grabinconsole->integer ) ) ) )
	{
		if( input_active )
			IN_Activate( qfalse );
	}
	else
	{
		if( !input_active )
			IN_Activate( qtrue );
	}

	HandleEvents();

	if( input_active && in_dgamouse->modified )
	{
		uninstall_grabs();
		install_grabs();
	}
}
