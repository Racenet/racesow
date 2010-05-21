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

static menuframework_s s_custom_menu;

void M_Custom_ExecuteButton( struct menucommon_s *menuitem )
{
	if( menuitem && menuitem->itemlocal )
	{
		int i;

		trap_Cmd_ExecuteText( EXEC_APPEND, (char *)menuitem->itemlocal );

		for( i = 0; i < s_custom_menu.nitems; i++ )
		{
			if( s_custom_menu.items[i] && s_custom_menu.items[i]->itemlocal )
			{
				UI_Free( s_custom_menu.items[i]->itemlocal );
				s_custom_menu.items[i]->itemlocal = NULL;
			}
		}

		M_ForceMenuOff();
	}
}

static void M_Custom_Init( void )
{
	menucommon_t *menuitem = NULL;
	int yoffset = 40;
	int i, count;

	s_custom_menu.nitems = 0;

	// parse the command line to create the buttons

	if( trap_Cmd_Argc() < 1 )
		return;

	// first one is always the tittle

	menuitem = UI_InitMenuItem( "m_custom_title1", trap_Cmd_Argv( 1 ), 0, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemBig, NULL );
	Menu_AddItem( &s_custom_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	yoffset += trap_SCR_strHeight( menuitem->font );

	// from now on each 2 new arguments define a new button
	for( i = 2, count = 0; i < trap_Cmd_Argc(); i += 2, count++ )
	{
		menuitem = UI_InitMenuItem( va( "m_custom_button%i", count ), trap_Cmd_Argv( i ), 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, M_Custom_ExecuteButton );
		menuitem->itemlocal = UI_CopyString( trap_Cmd_Argv( i + 1 ) );
		Menu_AddItem( &s_custom_menu, menuitem );
		yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;
	}

	Menu_Center( &s_custom_menu );
	Menu_Init( &s_custom_menu, qtrue );
	Menu_SetStatusBar( &s_custom_menu, NULL );
}


static void M_Custom_Draw( void )
{
	Menu_AdjustCursor( &s_custom_menu, 1 );
	Menu_Draw( &s_custom_menu );
}

static const char *M_Custom_Key( int key )
{
	return Default_MenuKey( &s_custom_menu, key );
}

static const char *M_Custom_CharEvent( qwchar key )
{
	return Default_MenuCharEvent( &s_custom_menu, key );
}

void M_Menu_Custom_f( void )
{
	M_Custom_Init();
	M_PushMenu( &s_custom_menu, M_Custom_Draw, M_Custom_Key, M_Custom_CharEvent );
}
