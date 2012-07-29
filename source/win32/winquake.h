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
// winquake.h: Win32-specific Quake header file


// can't handle anything less than WinXP in MinGW
#if defined(__GNUC__)
# if defined(_WIN32_WINNT) && _WIN32_WINNT < 0x501
#  undef _WIN32_WINNT
# endif
# ifndef _WIN32_WINNT
#  define _WIN32_WINNT 0x0501
# endif
#endif

#include <winsock2.h>
#include <ws2tcpip.h>

// Windows 2000 needs an additional header for IPv6 support
#if !defined( _WIN32_WINNT_WIN2K )
#	define _WIN32_WINNT_WIN2K	0x0500
#endif
#if !defined( _WIN32_WINNT ) || ( _WIN32_WINNT <= _WIN32_WINNT_WIN2K )
#	include <wspiapi.h>
#endif

#include <windows.h>
#ifdef HAVE_MMSYSTEM
#include <mmsystem.h>
#endif

#include <io.h>

#include "win_input.h"

#define UWM_APPACTIVE		(WM_APP+100)

enum
{
	MWHEEL_UNKNOWN,
	MWHEEL_DINPUT,
	MWHEEL_WM
} mwheel_type;

extern HINSTANCE global_hInstance;

extern HWND cl_hwnd, cl_parent_hwnd;
extern int ActiveApp, Minimized, AppFocused;
