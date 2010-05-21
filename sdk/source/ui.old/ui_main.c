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
static menuframework_s s_main_menu;

static void JoinNetworkServerFunc( menucommon_t *unused )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, "menu_joinserver" );
}

static void MatchMakerFunc( menucommon_t *unused )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, "menu_matchmaker" );
}

static void TutorialsFunc( menucommon_t *unused )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, "menu_tutorials" );
}

static void StartNetworkServerFunc( menucommon_t *unused )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, "menu_startserver" );
}

static void SetUpMenuFunc( menucommon_t *unused )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, "menu_setup" );
}
static void ConsoleFunc( menucommon_t *unused )
{
	if( uis.clientState > CA_DISCONNECTED )
		M_ForceMenuOff();
	trap_Cmd_ExecuteText( EXEC_APPEND, "toggleconsole" );
}

static void DemosMenuFunc( menucommon_t *unused )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, "menu_demos" );
}

static void ModsMenuFunc( menucommon_t *unused )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, "menu_mods" );
}

static void QuitMenuFunc( menucommon_t *unused )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, "menu_quit" );
}

static void M_MainInit( void )
{
	int yoffset = 0;
	menucommon_t *menuitem;

	s_main_menu.nitems = 0;

	menuitem = UI_InitMenuItem( "m_main_title1", "MAIN MENU", 0, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemBig, NULL );
	Menu_AddItem( &s_main_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_main_join_game", "find a game", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, JoinNetworkServerFunc );
	Menu_AddItem( &s_main_menu, menuitem );
	yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;

#ifdef MATCHMAKER_SUPPORT
	menuitem = UI_InitMenuItem( "m_main_matchmaker", "match maker", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, MatchMakerFunc );
	Menu_AddItem( &s_main_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );
#endif

	menuitem = UI_InitMenuItem( "m_main_tutorials", "tutorials", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, TutorialsFunc );
	Menu_AddItem( &s_main_menu, menuitem );
	yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;

	menuitem = UI_InitMenuItem( "m_main_setup", "setup", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, SetUpMenuFunc );
	Menu_AddItem( &s_main_menu, menuitem );
	yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;

	menuitem = UI_InitMenuItem( "m_main_start_server", "start local game", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, StartNetworkServerFunc );
	Menu_AddItem( &s_main_menu, menuitem );
	yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;

	menuitem = UI_InitMenuItem( "m_main_demos", "demos", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, DemosMenuFunc );
	Menu_AddItem( &s_main_menu, menuitem );
	yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;

	menuitem = UI_InitMenuItem( "m_main_mods", "mods", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, ModsMenuFunc );
	Menu_AddItem( &s_main_menu, menuitem );
	yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;

	menuitem = UI_InitMenuItem( "m_main_console", "console", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, ConsoleFunc );
	Menu_AddItem( &s_main_menu, menuitem );
	yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;

	menuitem = UI_InitMenuItem( "m_main_quit", "quit", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, QuitMenuFunc );
	Menu_AddItem( &s_main_menu, menuitem );
	yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;

	Menu_Center( &s_main_menu );
	Menu_Init( &s_main_menu, qtrue );

	Menu_SetStatusBar( &s_main_menu, NULL );
}


static void M_Main_Draw( void )
{
	Menu_AdjustCursor( &s_main_menu, 1 );
	Menu_Draw( &s_main_menu );
}

static const char *M_Main_Key( int key )
{
	return Default_MenuKey( &s_main_menu, key );
}

static const char *M_Main_CharEvent( qwchar key )
{
	return Default_MenuCharEvent( &s_main_menu, key );
}

void M_Menu_Main_f( void )
{
	M_MainInit();
	M_PushMenu( &s_main_menu, M_Main_Draw, M_Main_Key, M_Main_CharEvent );
}
