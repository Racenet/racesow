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

#ifndef GAME_QCVAR_H
#define GAME_QCVAR_H

#include "q_arch.h"

#ifdef __cplusplus
extern "C" {
#endif

//==========================================================
//
//CVARS (console variables)
//
//==========================================================

typedef int cvar_flag_t;

// bit-masked cvar flags
#define CVAR_ARCHIVE		1		// set to cause it to be saved to vars.rc
#define CVAR_USERINFO		2		// added to userinfo  when changed
#define CVAR_SERVERINFO		4		// added to serverinfo when changed
#define CVAR_NOSET			8		// don't allow change from console at all,
									// but can be set from the command line
#define CVAR_LATCH			16		// save changes until map restart
#define CVAR_LATCH_VIDEO	32		// save changes until video restart
#define CVAR_LATCH_SOUND	64		// save changes until video restart
#define CVAR_CHEAT			128		// will be reset to default unless cheats are enabled
#define CVAR_READONLY		256		// don't allow changing by user, ever
#define CVAR_DEVELOPER		512		// allow changing in dev builds, hide in release builds

// nothing outside the Cvar_*() functions should access these fields!!!
typedef struct cvar_s
{
	char *name;
	char *string;
	char *dvalue;
	char *latched_string;       // for CVAR_LATCH vars
	cvar_flag_t flags;
	qboolean modified;          // set each time the cvar is changed
	float value;
	int integer;
} cvar_t;

#ifdef __cplusplus
};
#endif

#endif // GAME_QCVAR_H

