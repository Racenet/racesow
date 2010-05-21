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

// wsw : jal : configuration menu

#include "ui_local.h"

/*
   =======================================================================

   MAIN MENU

   =======================================================================
 */
static menuframework_s s_setup_menu;

static void PlayerSetupFunc( menucommon_t *unused )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, "menu_playerconfig" );
}

static void TeamSetupFunc( menucommon_t *unused )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, "menu_teamconfig" );
}

static void OptionsSetupFunc( menucommon_t *unused )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, "menu_options" );
}

static void CustomizeKeysFunc( menucommon_t *unused )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, "menu_keys" );
}

static void CustomizeVsaysFunc( menucommon_t *unused )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, "menu_vsays" );
}

static void SoundMenuFunc( menucommon_t *unused )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, "menu_sound" );
}

static void PerformanceMenuFunc( menucommon_t *unused )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, "menu_performance" );
}

static void M_SetupInit( void )
{
	int yoffset = 0;
	menucommon_t *menuitem;

	s_setup_menu.nitems = 0;

	menuitem = UI_InitMenuItem( "m_setup_title1", "CONFIGURATION MENU", 0, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemBig, NULL );
	Menu_AddItem( &s_setup_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_setup_playersetup", "player setup", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, PlayerSetupFunc );
	Menu_AddItem( &s_setup_menu, menuitem );
	yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;

	menuitem = UI_InitMenuItem( "m_setup_teamsetup", "teams aspect setup", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, TeamSetupFunc );
	Menu_AddItem( &s_setup_menu, menuitem );
	yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;

	menuitem = UI_InitMenuItem( "m_setup_controller", "controller options", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, OptionsSetupFunc );
	Menu_AddItem( &s_setup_menu, menuitem );
	yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;


	menuitem = UI_InitMenuItem( "m_setup_keyboard", "keyboard controls", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, CustomizeKeysFunc );
	Menu_AddItem( &s_setup_menu, menuitem );
	yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;

	menuitem = UI_InitMenuItem( "m_setup_vsays", "voice messages", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, CustomizeVsaysFunc );
	Menu_AddItem( &s_setup_menu, menuitem );
	yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;

	/*menuitem = UI_InitMenuItem( "m_setup_gfx", "graphics options", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, GfxMenuFunc );
	Menu_AddItem( &s_setup_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_setup_video", "video options", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, VideoMenuFunc );
	Menu_AddItem( &s_setup_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );*/

	menuitem = UI_InitMenuItem( "m_setup_performance", "graphics options", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, PerformanceMenuFunc );
	Menu_AddItem( &s_setup_menu, menuitem );
	yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;

	menuitem = UI_InitMenuItem( "m_setup_sound", "sound options", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, SoundMenuFunc );
	Menu_AddItem( &s_setup_menu, menuitem );
	yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;

	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_setup_back", "back", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, M_genericBackFunc );
	Menu_AddItem( &s_setup_menu, menuitem );
	yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;

	Menu_Center( &s_setup_menu );
	Menu_Init( &s_setup_menu, qtrue );
	Menu_SetStatusBar( &s_setup_menu, NULL );
}


static void M_Setup_Draw( void )
{
	Menu_AdjustCursor( &s_setup_menu, 1 );
	Menu_Draw( &s_setup_menu );
}

static const char *M_Setup_Key( int key )
{
	return Default_MenuKey( &s_setup_menu, key );
}

static const char *M_Setup_CharEvent( qwchar key )
{
	return Default_MenuCharEvent( &s_setup_menu, key );
}

void M_Menu_Setup_f( void )
{
	M_SetupInit();
	M_PushMenu( &s_setup_menu, M_Setup_Draw, M_Setup_Key, M_Setup_CharEvent );
}
