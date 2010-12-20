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
#include "g_local.h"

static struct
{
	char *g_gametype;
	int g_numbots, g_scorelimit;
	float g_timelimit, g_warmup_timelimit;
	qboolean g_allow_falldamage;
} g_mm_old;

void G_MM_Setup( const char *gametype, int scorelimit, float timelimit, qboolean falldamage )
{
	// store old cvar values
	g_mm_old.g_gametype = G_CopyString( gs.gametypeName );
	g_mm_old.g_numbots = g_numbots->integer;
	g_mm_old.g_scorelimit = g_scorelimit->integer;
	g_mm_old.g_timelimit = g_timelimit->value;
	g_mm_old.g_warmup_timelimit = g_warmup_timelimit->value;
	g_mm_old.g_allow_falldamage = g_allow_falldamage->integer;

	// set new cvar values
	trap_Cvar_Set( "g_gametype", gametype ); // this needs some sort of checking
	trap_Cvar_Set( "g_numbots", "0" );
	trap_Cvar_Set( "g_scorelimit", va( "%d", scorelimit ) );
	trap_Cvar_Set( "g_timelimit", va( "%f", timelimit ) );
	trap_Cvar_Set( "g_warmup_timelimit", "5" );
	trap_Cvar_Set( "g_allow_falldamage", va( "%d", falldamage ) );

	G_RespawnLevel();
}

void G_MM_Reset( void )
{
	trap_Cvar_Set( "g_gametype", g_mm_old.g_gametype );
	trap_Cvar_Set( "g_numbots", va( "%d", g_mm_old.g_numbots ) );
	trap_Cvar_Set( "g_scorelimit", va( "%d", g_mm_old.g_scorelimit ) );
	trap_Cvar_Set( "g_timelimit", va( "%f", g_mm_old.g_timelimit ) );
	trap_Cvar_Set( "g_warmup_timelimit", va( "%d", g_mm_old.g_warmup_timelimit ) );
	trap_Cvar_Set( "g_allow_falldamage", va( "%d", g_mm_old.g_allow_falldamage ) );

	G_Free( g_mm_old.g_gametype );
	memset( &g_mm_old, 0, sizeof( g_mm_old ) );

	G_RespawnLevel();
}
