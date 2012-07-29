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

#include "ui_local.h"

/*
=======================================================================

MAIN MENU

=======================================================================
*/
static menuframework_s s_game_menu;

static void M_Game_MenuMainFunc( menucommon_t *menuitem )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, "menu_main\n" );
}

static void M_Game_DisconnectFunc( menucommon_t *menuitem )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, "disconnect\n" );
}

static void M_Game_JoinFunc( menucommon_t *menuitem )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, "cmd join\n" );
	M_PopMenu();
}

static void M_Game_JoinFuncAlpha( menucommon_t *menuitem )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, "cmd join alpha\n" );
	M_PopMenu();
}

static void M_Game_JoinFuncBeta( menucommon_t *menuitem )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, "cmd join beta\n" );
	M_PopMenu();
}

static void M_Game_SpecFunc( menucommon_t *menuitem )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, "cmd spec\n" );
	M_PopMenu();
}

static void M_Game_LeaveQueueFunc( menucommon_t *menuitem )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, "cmd leavequeue\n" );
	M_PopMenu();
}

static void M_Game_JoinQueueFunc( menucommon_t *menuitem )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, "cmd enterqueue\n" );
	M_PopMenu();
}

static void M_Game_ReadyFunc( menucommon_t *menuitem )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, "cmd ready\n" );
	M_PopMenu();
}

static void M_Game_NotReadyFunc( menucommon_t *menuitem )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, "cmd notready\n" );
	M_PopMenu();
}

static void M_Game_GametypeMenuFunc( menucommon_t *menuitem )
{
	if( uis.clientState >= CA_ACTIVE && !uis.demoplaying )
	{
		trap_Cmd_ExecuteText( EXEC_APPEND, "gametypemenu\n" );
		M_ForceMenuOff();
	}
}

static void M_Game_SpecModeMenuFunc( menucommon_t *menuitem )
{
	if( uis.clientState >= CA_ACTIVE && !uis.demoplaying )
		trap_Cmd_ExecuteText( EXEC_APPEND, "menu_chasecam\n" );
}

static void M_Game_Init( void )
{
	int yoffset = 0;
	int m_is_teambased = 0, m_clientteam = TEAM_SPECTATOR, m_is_challenger = 0, m_needs_ready = 0, m_is_ready = 0;
	static char titlename[64];
	char mapname[MAX_QPATH], mapmessage[MAX_QPATH];
	static char mapnametitle[MAX_QPATH+MAX_QPATH+4];
	static char gametypename[64];
	menucommon_t *menuitem;

	s_game_menu.nitems = 0;

	Q_strncpyz( titlename, trap_Cvar_String( "gamename" ), sizeof( titlename ) );
	Q_strupr( titlename );

	// first is teambased
	if( trap_Cmd_Argc() > 1 )
	{
		m_is_teambased = atoi( trap_Cmd_Argv( 1 ) );
	}

	// 2nd is team
	if( trap_Cmd_Argc() > 2 )
	{
		m_clientteam = atoi( trap_Cmd_Argv( 2 ) );
		if( m_clientteam < TEAM_SPECTATOR || m_clientteam > GS_MAX_TEAMS )
			m_clientteam = TEAM_SPECTATOR;

	}
	//3rd is challengers queue ( 0 = gametype has no queue, 1 = is not in queue, 2 = is in queue
	if( trap_Cmd_Argc() > 3 )
	{
		m_is_challenger = atoi( trap_Cmd_Argv( 3 ) );
	}

	//4th is needs_ready
	if( trap_Cmd_Argc() > 4 )
	{
		m_needs_ready = atoi( trap_Cmd_Argv( 4 ) );
	}

	//5th is is_ready
	if( trap_Cmd_Argc() > 5 )
	{
		m_is_ready = atoi( trap_Cmd_Argv( 5 ) );
	}

	// 6th title
	if( trap_Cmd_Argc() > 6 )
	{
		Q_snprintfz( titlename, sizeof( titlename ), "%s ", trap_Cmd_Argv( 6 ) );
	}

	// 7th gametype name
	if( trap_Cmd_Argc() > 7 )
	{
		Q_snprintfz( gametypename, sizeof( gametypename ), "%s ", trap_Cmd_Argv( 7 ) );
	}

	trap_GetConfigString( CS_MAPNAME, mapname, sizeof( mapname ) );
	trap_GetConfigString( CS_MESSAGE, mapmessage, sizeof( mapmessage ) );

	Q_strncpyz( mapnametitle, "Map: ", sizeof( mapnametitle ) );
	Q_strncatz( mapnametitle, mapname, sizeof( mapnametitle ) );	
	if( mapmessage[0] && Q_stricmp( mapname,  mapmessage ) )
		Q_strncatz( mapnametitle, va( " \"%s\"", mapmessage ), sizeof( mapnametitle ) );

	menuitem = UI_InitMenuItem( "m_game_title1", titlename, 0, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemBig, NULL );
	Menu_AddItem( &s_game_menu, menuitem );
	yoffset += 2 *trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_game_mapname", mapnametitle, 0, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemBig, NULL );
	Menu_AddItem( &s_game_menu, menuitem );
	yoffset += 2 * trap_SCR_strHeight( menuitem->font );

	if( m_needs_ready )
	{
		menuitem = UI_InitMenuItem( "m_game_ready", "ready", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, M_Game_ReadyFunc );
		Menu_AddItem( &s_game_menu, menuitem );
		yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;
	}

	if( m_is_ready )
	{
		menuitem = UI_InitMenuItem( "m_game_notready", "not ready", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, M_Game_NotReadyFunc );
		Menu_AddItem( &s_game_menu, menuitem );
		yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;
	}

	if( m_is_challenger )
	{
		if( m_is_challenger == 1 )
		{
			menuitem = UI_InitMenuItem( "m_game_join_challengers", "join challengers queue", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, M_Game_JoinQueueFunc );
			Menu_AddItem( &s_game_menu, menuitem );
			UI_SetupButton( menuitem, qtrue );
		}
		else
		{
			menuitem = UI_InitMenuItem( "m_game_spec_challengers", "leave challengers queue", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, M_Game_LeaveQueueFunc );
			Menu_AddItem( &s_game_menu, menuitem );
			UI_SetupButton( menuitem, qtrue );
		}
	}
	else
	{
		if( m_clientteam == TEAM_SPECTATOR )
		{
			if( m_is_teambased == 1 )
			{
				char teamname[MAX_CONFIGSTRING_CHARS];
				char itemname[MAX_CONFIGSTRING_CHARS+100];

				menuitem = UI_InitMenuItem( "m_game_join_any", "join any", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, M_Game_JoinFunc );
				Menu_AddItem( &s_game_menu, menuitem );
				yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;

				trap_GetConfigString( CS_TEAM_SPECTATOR_NAME + TEAM_ALPHA, teamname, sizeof( teamname ) );
				Q_snprintfz( itemname, sizeof( itemname ), "join %s", teamname );
				menuitem = UI_InitMenuItem( "m_game_join_alpha", Q_strlwr( itemname ), 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, M_Game_JoinFuncAlpha );
				Menu_AddItem( &s_game_menu, menuitem );
				yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;

				trap_GetConfigString( CS_TEAM_SPECTATOR_NAME + TEAM_BETA, teamname, sizeof( teamname ) );
				Q_snprintfz( itemname, sizeof( itemname ), "join %s", teamname );
				menuitem = UI_InitMenuItem( "m_game_join_beta", Q_strlwr( itemname ), 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, M_Game_JoinFuncBeta );
				Menu_AddItem( &s_game_menu, menuitem );
				UI_SetupButton( menuitem, qtrue );
			}
			else
			{
				menuitem = UI_InitMenuItem( "m_game_join", "join", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, M_Game_JoinFunc );
				Menu_AddItem( &s_game_menu, menuitem );
				UI_SetupButton( menuitem, qtrue );
			}
		}
		else
		{
			menuitem = UI_InitMenuItem( "m_game_spec", "spectate", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, M_Game_SpecFunc );
			Menu_AddItem( &s_game_menu, menuitem );
			UI_SetupButton( menuitem, qtrue );
		}
	}

	yoffset += 1.5 * UI_SetupButton( menuitem, qtrue );

	{
		int i;
		char cmd[MAX_CONFIGSTRING_CHARS];

		for( i = 0; i < MAX_GAMECOMMANDS; i++ )
		{
			trap_GetConfigString( CS_GAMECOMMANDS + i, cmd, sizeof( cmd ) );
			if( !Q_stricmp( cmd, "gametypemenu" ) )
				break;
		}

		if( i < MAX_GAMECOMMANDS )
		{
			menuitem = UI_InitMenuItem( "m_gametype_menu", "game menu", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, M_Game_GametypeMenuFunc );
			Menu_AddItem( &s_game_menu, menuitem );
			yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;

			yoffset += trap_SCR_strHeight( uis.fontSystemSmall );
		}
	}

	menuitem = UI_InitMenuItem( "m_game_setup", "main menu", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, M_Game_MenuMainFunc );
	Menu_AddItem( &s_game_menu, menuitem );
	yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;

	if( m_clientteam == TEAM_SPECTATOR )
	{
		menuitem = UI_InitMenuItem( "m_game_chasecam", "chasecam mode", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, M_Game_SpecModeMenuFunc );
		Menu_AddItem( &s_game_menu, menuitem );
		yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;
	}

	menuitem = UI_InitMenuItem( "m_game_disconnect", "disconnect", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, M_Game_DisconnectFunc );
	Menu_AddItem( &s_game_menu, menuitem );
	yoffset += 1.5 * UI_SetupButton( menuitem, qtrue );

	menuitem = UI_InitMenuItem( "m_ingame_back", "back", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, M_genericBackFunc );
	Menu_AddItem( &s_game_menu, menuitem );
	yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;

	Menu_Center( &s_game_menu );
	Menu_Init( &s_game_menu, qtrue );

	Menu_SetStatusBar( &s_game_menu, NULL );
}

static void M_Game_Draw( void )
{
	Menu_AdjustCursor( &s_game_menu, 1 );
	Menu_Draw( &s_game_menu );
}

static const char *M_Game_Key( int key )
{
	return Default_MenuKey( &s_game_menu, key );
}

static const char *M_Game_CharEvent( qwchar key )
{
	return Default_MenuCharEvent( &s_game_menu, key );
}

void M_Menu_Game_f( void )
{
	M_Game_Init();
	M_PushMenu( &s_game_menu, M_Game_Draw, M_Game_Key, M_Game_CharEvent );
}
