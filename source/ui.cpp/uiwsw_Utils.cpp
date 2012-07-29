/*
Copyright (C) 2002-2008 The Warsow devteam

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

#include "uiwsw_Utils.h"
#include "uicore_Global.h"
#include "uimenu_Global.h"

namespace UIWsw
{
	int Local::vidWidth = 0;
	int Local::vidHeight = 0;
	int Local::gameProtocol = 0;
	unsigned int Local::time = 0;
	float Local::frameTime = 0.0f;
	float Local::scaleX = 0.0f;
	float Local::scaleY = 0.0f;
	int Local::cursorX = 0;
	int Local::cursorY = 0;
	int Local::clientState = 0;
	int Local::serverState = 0;
	bool Local::forceUI = false;
	bool Local::bind_grab = false;
	struct shader_s *Local::whiteShader = NULL;
	struct mufont_s *Local::fontSystemSmall = NULL;
	struct mufont_s *Local::fontSystemMedium = NULL;
	struct mufont_s *Local::fontSystemBig = NULL;
	bool Local::background = false; // has to draw the ui background
	bool Local::backgroundTrackStarted = false;

	const char *gametype_names[GAMETYPE_NB][2] =
	{
		{ "Deathmatch", "dm" },
		{ "Duel", "duel" },
		{ "Team Deathmatch", "tdm" },
		{ "Capture the flag", "ctf" },
		{ "Race", "race" },
		{ "Clan Arena", "ca" },
	};


	void UI_ForceMenuOff( void )
	{
		UIMenu::setActiveMenu( NULL );
		Trap::CL_SetKeyDest( key_game );
		Trap::Key_ClearStates();
	}

	char *UI_CopyString( const char *in )
	{
		char	*out;
		
		out = (char*)UIMem::Malloc(strlen(in)+1);
		strcpy (out, in);
		return out;
	}

	void UI_Error( const char *format, ... )
	{
		va_list		argptr;
		char		msg[1024];

		va_start( argptr, format );
		Q_vsnprintfz( msg, sizeof(msg), format, argptr );
		va_end( argptr );

		Trap::Error( msg );
	}

	void UI_Printf( const char *format, ... )
	{
		va_list		argptr;
		char		msg[1024];

		va_start( argptr, format );
		Q_vsnprintfz( msg, sizeof(msg), format, argptr );
		va_end ( argptr );

		Trap::Print ( msg );
	}
}

//extern "C" {
// this is only here so the functions in q_shared.c and q_math.c can link
void Sys_Error( const char *format, ... )
{
	va_list		argptr;
	char		msg[1024];

	va_start( argptr, format );
	Q_vsnprintfz( msg, sizeof(msg), format, argptr );
	va_end( argptr );

	UIWsw::Trap::Error( msg );
}
//}
