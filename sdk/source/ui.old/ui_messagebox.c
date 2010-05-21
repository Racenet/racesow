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
static menuframework_s s_msgbox_menu;

#define M_MSGBOX_LINELEN 64
static char mbtext[MAX_STRING_CHARS];

static void M_Msgbox_Init( void )
{
	int i;
	menucommon_t *menuitem = NULL;
	char menuitem_name[40];
	int yoffset = 40;

	s_msgbox_menu.nitems = 0;

	mbtext[0] = 0;

	for( i = 1; i < trap_Cmd_Argc(); i++ )
	{
		Q_strncpyz( mbtext, trap_Cmd_Argv(i), sizeof( mbtext ) );
		if( strlen( mbtext ) < M_MSGBOX_LINELEN )
		{
			Q_snprintfz( menuitem_name, sizeof( menuitem_name ), "m_msgbox_textline_%i", i+1 );
			menuitem = UI_InitMenuItem( menuitem_name, mbtext, 0, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemSmall, NULL );
			Menu_AddItem( &s_msgbox_menu, menuitem );
			yoffset += trap_SCR_strHeight( menuitem->font );
		}
		else
		{
			// todo: split in lines
		}
	}

	//if we printed something, add one line separation
	if( menuitem )
		yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_msgbox_back", "ok", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, M_genericBackFunc );
	Menu_AddItem( &s_msgbox_menu, menuitem );
	yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;

	Menu_Center( &s_msgbox_menu );
	Menu_Init( &s_msgbox_menu, qfalse );
	Menu_SetStatusBar( &s_msgbox_menu, NULL );
}


static void M_Msgbox_Draw( void )
{
	Menu_AdjustCursor( &s_msgbox_menu, 1 );
	Menu_Draw( &s_msgbox_menu );
}

static const char *M_Msgbox_Key( int key )
{
	return Default_MenuKey( &s_msgbox_menu, key );
}

static const char *M_Msgbox_CharEvent( qwchar key )
{
	return Default_MenuCharEvent( &s_msgbox_menu, key );
}

void M_Menu_MsgBox_f( void )
{
	M_Msgbox_Init();
	M_PushMenu( &s_msgbox_menu, M_Msgbox_Draw, M_Msgbox_Key, M_Msgbox_CharEvent );
}
