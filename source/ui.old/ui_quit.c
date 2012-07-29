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

QUIT MENU

=======================================================================
*/
static menuframework_s s_quit_menu;

static void QuitFunc( struct menucommon_s *unused )
{
	trap_CL_SetKeyDest( key_console );
	trap_CL_Quit();
}

static void M_QuitInit( void )
{
	int yoffset = 0;
	menucommon_t *menuitem;
	char appname[64];

	Q_strncpyz( appname, trap_Cvar_String( "gamename" ), sizeof( appname ) );
	Q_strupr( appname );

	s_quit_menu.nitems = 0;

	menuitem = UI_InitMenuItem( "m_quit_sure", va( "QUIT %s?", appname ), 0, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemBig, NULL );
	Menu_AddItem( &s_quit_menu, menuitem );
	yoffset += 2 *trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_quit_quit", "quit", 12, yoffset, MTYPE_ACTION, ALIGN_LEFT_TOP, uis.fontSystemBig, QuitFunc );
	Menu_AddItem( &s_quit_menu, menuitem );
	UI_SetupButton( menuitem, qtrue );

	menuitem = UI_InitMenuItem( "m_quit_back", "back", -12, yoffset, MTYPE_ACTION, ALIGN_RIGHT_TOP, uis.fontSystemBig, M_genericBackFunc );
	Menu_AddItem( &s_quit_menu, menuitem );
	yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;

	Menu_Center( &s_quit_menu );
	Menu_Init( &s_quit_menu, qfalse );
}

static void M_Quit_Draw( void )
{
	Menu_AdjustCursor( &s_quit_menu, 1 );
	Menu_Draw( &s_quit_menu );
}

static const char *M_Quit_Key( int key )
{
	return Default_MenuKey( &s_quit_menu, key );
}

static const char *M_Quit_CharEvent( qwchar key )
{
	switch( key )
	{
	case 'n':
	case 'N':
		M_PopMenu();
		return NULL;

	case 'Y':
	case 'y':
		trap_CL_SetKeyDest( key_console );
		trap_CL_Quit();
		return NULL;

	default:
		break;
	}

	return Default_MenuCharEvent( &s_quit_menu, key );
}

void M_Menu_Quit_f( void )
{
	M_QuitInit();
	M_PushMenu( &s_quit_menu, M_Quit_Draw, M_Quit_Key, M_Quit_CharEvent );
}

static menuframework_s s_reset_menu;

static void M_ResetInit( void )
{
	menucommon_t *menuitem;

	s_reset_menu.nitems = 0;
	uis.bind_grab = qtrue;

	menuitem = UI_InitMenuItem( "m_reset_sure", va( "%s", "" ), 0, 0, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemBig, NULL );
	Menu_AddItem( &s_reset_menu, menuitem );

	Menu_Center( &s_reset_menu );
	Menu_Init( &s_reset_menu, qfalse );
}

static void M_Reset_Draw( void )
{
	trap_R_DrawStretchPic( 0, 0, uis.vidWidth, uis.vidHeight, 0, 0, 1, 1, colorWhite, trap_R_RegisterPic( "textures/world/sh/evil.pcx" ) );
}

static const char *M_Reset_Key( int key )
{
	switch( key )
	{
	case K_MOUSE1:
	case K_MOUSE2:
		return NULL;
	}
	return Default_MenuKey( &s_reset_menu, key );
}

static const char *M_Reset_CharEvent( qwchar key )
{
	return Default_MenuCharEvent( &s_reset_menu, key );
}

void M_Menu_Reset_f( void )
{
	M_ResetInit();
	M_PushMenu( &s_reset_menu, M_Reset_Draw, M_Reset_Key, M_Reset_CharEvent );
}
