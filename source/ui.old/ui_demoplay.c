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

static menuframework_s s_demoplay_menu;

static void demoplayJump( menucommon_t *menuitem )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, va( "demojump %i\n", menuitem->curvalue ) );
}

static void demoplayPauseDemo( menucommon_t *unused )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, "demopause\n" );
}

static void demoplayStopDemo( menucommon_t *unused )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, "disconnect\n" );
}

static void demoplayOpenMain( menucommon_t *unused )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, "menu_main\n" );
}

static void M_demoplayInit( void )
{
	int yoffset = 0;
	menucommon_t *menuitem;

	s_demoplay_menu.nitems = 0;

	menuitem = UI_InitMenuItem( "m_demoplay_title1", "DEMOPLAY MENU", 0, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemBig, NULL );
	Menu_AddItem( &s_demoplay_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	yoffset += trap_SCR_strHeight( menuitem->font );

	if( !strstr( trap_Cvar_String( "demoname" ), "tutorials/" ) )
	{
		menuitem = UI_InitMenuItem( "m_demoplay_time", "", uis.vidWidth/2 - 16 - 2, yoffset, MTYPE_SEPARATOR, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
		Menu_AddItem( &s_demoplay_menu, menuitem );
		yoffset += trap_SCR_strHeight( menuitem->font );

		menuitem = UI_InitMenuItem( "m_demoplay_slider", NULL, -uis.vidWidth/2, yoffset, MTYPE_SLIDER, ALIGN_CENTER_TOP, uis.fontSystemSmall, demoplayJump );
		Menu_AddItem( &s_demoplay_menu, menuitem );
		UI_SetupSlider( menuitem, uis.vidWidth/16 - 2, trap_Cvar_Value( "demotime" ), 0, trap_Cvar_Value( "demoduration" ) );
		yoffset += trap_SCR_strHeight( menuitem->font );

		yoffset += trap_SCR_strHeight( menuitem->font );

		menuitem = UI_InitMenuItem( "m_demoplay_pause", (trap_Cvar_Value( "demopaused" ) ? "resume demo" : "pause demo"), 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, demoplayPauseDemo );
		Menu_AddItem( &s_demoplay_menu, menuitem );
		yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;
	}

	menuitem = UI_InitMenuItem( "m_demoplay_stop", "stop demo", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, demoplayStopDemo );
	Menu_AddItem( &s_demoplay_menu, menuitem );
	yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;

	menuitem = UI_InitMenuItem( "m_demoplay_disconnect", "main menu", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, demoplayOpenMain );
	Menu_AddItem( &s_demoplay_menu, menuitem );
	yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;

	Menu_Center( &s_demoplay_menu );
	Menu_Init( &s_demoplay_menu, qtrue );
	Menu_SetStatusBar( &s_demoplay_menu, NULL );
}

static void M_Demoplay_Draw( void )
{
	menucommon_t *menuitem;

	menuitem = UI_MenuItemByName( "m_demoplay_slider" );
	if( menuitem )
		menuitem->curvalue = trap_Cvar_Value( "demotime" );

	menuitem = UI_MenuItemByName( "m_demoplay_time" );
	if( menuitem )
	{
		int min, sec, milli;

		milli = trap_Cvar_Value( "demotime" ) * 10;
		min = milli/600;
		milli -= min*600;
		sec = milli/10;
		Q_snprintfz( menuitem->title, sizeof( menuitem->title ), "%02d:%02d", min, sec );

		milli = trap_Cvar_Value( "demoduration" ) * 10;
		min = milli/600;
		milli -= min*600;
		sec = milli/10;
		Q_strncatz( menuitem->title, va( "/%02d:%02d", min, sec ), sizeof( menuitem->title ) );
	}

	menuitem = UI_MenuItemByName( "m_demoplay_pause" );
	if( menuitem )
		Q_strncpyz( menuitem->title, (trap_Cvar_Value( "demopaused" ) ? "resume demo" : "pause demo"), sizeof( menuitem->title ) );

	Menu_AdjustCursor( &s_demoplay_menu, 1 );
	Menu_Draw( &s_demoplay_menu );
}

static const char *M_Demoplay_Key( int key )
{
	return Default_MenuKey( &s_demoplay_menu, key );
}

static const char *M_Demoplay_CharEvent( qwchar key )
{
	return Default_MenuCharEvent( &s_demoplay_menu, key );
}

void M_Menu_Demoplay_f( void )
{
	M_demoplayInit();
	M_PushMenu( &s_demoplay_menu, M_Demoplay_Draw, M_Demoplay_Key, M_Demoplay_CharEvent );
}
