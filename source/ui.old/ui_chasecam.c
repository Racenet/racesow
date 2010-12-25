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

static menuframework_s s_chasecam_menu;

void M_Chasecam_ExecuteButton( struct menucommon_s *menuitem )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, va( "chase %s\n", menuitem->title ) );
	M_ForceMenuOff();
}

static void M_Chasecam_Init( void )
{
	menucommon_t *menuitem = NULL;
	int yoffset = 40;
	int i;
	static char *chasecam_modes[] = { "auto", "objectives", "powerups", "score", "none", 0 };

	s_chasecam_menu.nitems = 0;

	menuitem = UI_InitMenuItem( "m_chasecam_title", "chasecam mode", 0, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemBig, NULL );
	Menu_AddItem( &s_chasecam_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	yoffset += trap_SCR_strHeight( menuitem->font );

	for( i = 0; chasecam_modes[i]; i++ )
	{
		char *mode = chasecam_modes[i];

		menuitem = UI_InitMenuItem( va( "m_chasecam_%s", mode ), mode, 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, M_Chasecam_ExecuteButton );
		Menu_AddItem( &s_chasecam_menu, menuitem );

		if( chasecam_modes[i+1] )
			yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;
		else
			yoffset += 1.5 * UI_SetupButton( menuitem, qtrue );
	}

	menuitem = UI_InitMenuItem( "m_chasecam_back", "back", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, M_genericBackFunc );
	Menu_AddItem( &s_chasecam_menu, menuitem );
	yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;

	Menu_Center( &s_chasecam_menu );
	Menu_Init( &s_chasecam_menu, qtrue );
	Menu_SetStatusBar( &s_chasecam_menu, NULL );
}


static void M_Chasecam_Draw( void )
{
	Menu_AdjustCursor( &s_chasecam_menu, 1 );
	Menu_Draw( &s_chasecam_menu );
}

static const char *M_Chasecam_Key( int key )
{
	return Default_MenuKey( &s_chasecam_menu, key );
}

static const char *M_Chasecam_CharEvent( qwchar key )
{
	return Default_MenuCharEvent( &s_chasecam_menu, key );
}

void M_Menu_Chasecam_f( void )
{
	M_Chasecam_Init();
	M_PushMenu( &s_chasecam_menu, M_Chasecam_Draw, M_Chasecam_Key, M_Chasecam_CharEvent );
}
