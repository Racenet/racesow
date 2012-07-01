/*
Copyright (C) 2011 Victor Luchits

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

#include "ui_precompiled.h"
#include "kernel/ui_common.h"
#include "kernel/ui_main.h"
#include "kernel/ui_demoinfo.h"
#include "as/asui.h"
#include "as/asui_local.h"

namespace ASUI {

// dummy cGame class, single referenced, sorta like 'window' JS
class Game 
{
public:
	Game() : backgroundTrackPlaying(false)
	{
	}

	bool backgroundTrackPlaying;
};

typedef WSWUI::DemoInfo DemoInfo;

// ch : whats up with these statics?
static Game dummyGame;

// =====================================================================================

void PrebindGame( ASInterface *as )
{
	ASBind::Class<Game, ASBind::class_singleref>( as->getEngine() );
}

static const DemoInfo & Game_GetDemoInfo( Game *game )
{
	return *UI_Main::Get()->getDemoInfo();
}

static void Game_StartLocalSound( const asstring_t &s )
{
	trap::S_StartLocalSound( s.buffer );
}

static void Game_StartBackgroundTrack( Game *game, const asstring_t &intro, const asstring_t &loop, bool stopIfPlaying )
{
	if( stopIfPlaying || !game->backgroundTrackPlaying ) {
		trap::S_StartBackgroundTrack( intro.buffer, loop.buffer );
		game->backgroundTrackPlaying = true;
	}
}

static void Game_StopBackgroundTrack( Game *game )
{
	trap::S_StopBackgroundTrack();
	game->backgroundTrackPlaying = false;
}

static asstring_t *Game_Name( Game *game )
{
	return ASSTR( trap::Cvar_String( "gamename" ) );
}

static asstring_t *Game_ConfigString( Game *game, int cs )
{
	char configstring[MAX_CONFIGSTRING_CHARS];

	if( cs < 0 || cs >= MAX_CONFIGSTRINGS ) {
		Com_Printf( S_COLOR_RED "Game_ConfigString: bogus configstring index: %i", cs );
		return ASSTR( "" );
	}

	trap::GetConfigString( cs, configstring, sizeof( configstring ) );
	return ASSTR( configstring );
}

static int Game_ClientState( Game *game )
{
	return UI_Main::Get()->getRefreshState().clientState;
}

static int Game_ServerState( Game *game )
{
	return UI_Main::Get()->getRefreshState().serverState;
}

static void Game_Exec( Game *game, const asstring_t &cmd )
{
	trap::Cmd_ExecuteText( EXEC_NOW, cmd.buffer );
}

static void Game_ExecAppend( Game *game, const asstring_t &cmd )
{
	trap::Cmd_ExecuteText( EXEC_APPEND, cmd.buffer );
}

static void Game_ExecInsert( Game *game, const asstring_t &cmd )
{
	trap::Cmd_ExecuteText( EXEC_INSERT, cmd.buffer );
}

static void Game_Print( Game *game, const asstring_t &s )
{
	trap::Print( s.buffer );
}

static void Game_DPrint( Game *game, const asstring_t &s )
{
	if( UI_Main::Get()->debugOn() ) {
		trap::Print( s.buffer );
	}
}

void BindGame( ASInterface *as )
{
	ASBind::Enum( as->getEngine(), "eConfigString" )
		( "CS_MODMANIFEST", CS_MODMANIFEST )
		( "CS_MESSAGE", CS_MESSAGE )
		( "CS_MAPNAME", CS_MAPNAME )
		( "CS_AUDIOTRACK", CS_AUDIOTRACK )
		( "CS_HOSTNAME", CS_HOSTNAME )
		( "CS_GAMETYPETITLE", CS_GAMETYPETITLE )
		( "CS_GAMETYPENAME", CS_GAMETYPENAME )
		( "CS_GAMETYPEVERSION", CS_GAMETYPEVERSION )
		( "CS_GAMETYPEAUTHOR", CS_GAMETYPEAUTHOR )
		( "CS_TEAM_ALPHA_NAME", CS_TEAM_ALPHA_NAME )
		( "CS_TEAM_BETA_NAME", CS_TEAM_BETA_NAME )
		( "CS_MATCHNAME", CS_MATCHNAME )
		( "CS_MATCHSCORE", CS_MATCHSCORE )
	;

	ASBind::Enum( as->getEngine(), "eClientState" )
		( "CA_UNITIALIZED", CA_UNINITIALIZED )
		( "CA_DISCONNECTED", CA_DISCONNECTED )
		( "CA_GETTING_TICKET", CA_GETTING_TICKET )
		( "CA_CONNECTING", CA_CONNECTING )
		( "CA_HANDSHAKE", CA_HANDSHAKE )
		( "CA_CONNECTED", CA_CONNECTED )
		( "CA_LOADING", CA_LOADING )
		( "CA_ACTIVE", CA_ACTIVE )
	;

	ASBind::Enum( as->getEngine(), "eDropReason" )
		( "DROP_REASON_CONNFAILED", DROP_REASON_CONNFAILED )
		( "DROP_REASON_CONNTERMINATED", DROP_REASON_CONNTERMINATED )
		( "DROP_REASON_CONNERROR", DROP_REASON_CONNERROR )
		;

	ASBind::Enum( as->getEngine(), "eDropType" )
		( "DROP_TYPE_GENERAL", DROP_TYPE_GENERAL )
		( "DROP_TYPE_PASSWORD", DROP_TYPE_PASSWORD )
		( "DROP_TYPE_NORECONNECT", DROP_TYPE_NORECONNECT )
		( "DROP_TYPE_TOTAL", DROP_TYPE_TOTAL )
		;

	ASBind::GetClass<Game>( as->getEngine() )
		// current refresh state

		// gives access to properties and controls of the currently playing demo instance
		.constmethod( Game_GetDemoInfo, "get_demo", true )

		// FIXME: move this to window.
		.constmethod( Game_StartLocalSound, "startLocalSound", true )
		.method2( Game_StartBackgroundTrack, "void startBackgroundTrack( String &in intro, String &in loop, bool stopIfPlaying = true ) const", true )
		.constmethod( Game_StopBackgroundTrack, "stopBackgroundTrack", true )

		.constmethod( Game_Name, "get_name", true )

		.constmethod( Game_ConfigString, "configString", true )
		.constmethod( Game_ConfigString, "cs", true )

		.constmethod( Game_ClientState, "get_clientState", true )
		.constmethod( Game_ServerState, "get_serverState", true )

		.constmethod( Game_Exec, "exec", true )
		.constmethod( Game_ExecAppend, "execAppend", true )
		.constmethod( Game_ExecInsert, "execInsert", true )

		.constmethod( Game_Print, "print", true )
		.constmethod( Game_DPrint, "dprint", true )
	;
}

void BindGameGlobal( ASInterface *as )
{
	ASBind::Global( as->getEngine() )
		// global variable
		.var( &dummyGame, "game" )
	;
}

}

ASBIND_TYPE( ASUI::Game, Game );
