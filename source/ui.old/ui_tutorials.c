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

static menuframework_s s_tutorials_menu;

#define TUTORIALS_PATH "tutorials/"
#define TUTORIALS_MENUFILE "menu.lst"

static void TutorialFunc( menucommon_t *menuitem )
{
	if( menuitem && menuitem->itemlocal )
	{
		int i;

		trap_Cmd_ExecuteText( EXEC_APPEND, va( "demo " TUTORIALS_PATH "%s\n", (char *)menuitem->itemlocal ) );

		for( i = 0; i < s_tutorials_menu.nitems; i++ )
		{
			if( s_tutorials_menu.items[i] )
			{
				if( s_tutorials_menu.items[i]->itemlocal )
				{
					UI_Free( s_tutorials_menu.items[i]->itemlocal );
					s_tutorials_menu.items[i]->itemlocal = NULL;
				}

				if( s_tutorials_menu.items[i]->statusbar )
				{
					UI_Free( s_tutorials_menu.items[i]->statusbar );
					s_tutorials_menu.items[i]->statusbar = NULL;
				}
			}
		}
	}
}

static void M_TutorialsInit( void )
{
	int yoffset = 0;
	int filenum, length;
	const char *filename = "demos/" TUTORIALS_PATH TUTORIALS_MENUFILE;
	menucommon_t *menuitem;

	s_tutorials_menu.nitems = 0;

	menuitem = UI_InitMenuItem( "m_tutorials_title1", "WARSOW TUTORIALS", 0, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemBig, NULL );
	Menu_AddItem( &s_tutorials_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	yoffset += trap_SCR_strHeight( menuitem->font );

	length = trap_FS_FOpenFile( filename, &filenum, FS_READ );
	if( length != -1 )
	{
		// load the menu list into memory
		int cnt;
		char *data, **ptr, *token;
		
		data = UI_Malloc( length + 1 );
		trap_FS_Read( data, length, filenum );
		trap_FS_FCloseFile( filenum );

		cnt = 0;
		ptr = &data;
		while( *ptr )
		{
			char name[64];

			token = COM_Parse( ptr );
			if( !token[0] )
				break;

			Q_strncpyz( name, token, sizeof( name ) );

			token = COM_ParseExt( ptr, qfalse );
			if( !token[0] )
				break;

			cnt++;
			menuitem = UI_InitMenuItem( va( "m_tutorials_%i", cnt ), 
				name, 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, TutorialFunc );
			menuitem->itemlocal = UI_CopyString( token );

			token = COM_ParseExt( ptr, qfalse );
			if( token[0] )
				menuitem->statusbar = UI_CopyString( token );

			Menu_AddItem( &s_tutorials_menu, menuitem );

			yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;
		}
	}

	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_tutorials_back", "back", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, M_genericBackFunc );
	Menu_AddItem( &s_tutorials_menu, menuitem );
	yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;

	Menu_Center( &s_tutorials_menu );
	Menu_Init( &s_tutorials_menu, qtrue );
	Menu_SetStatusBar( &s_tutorials_menu, NULL );
}


static void M_Tutorials_Draw( void )
{
	Menu_AdjustCursor( &s_tutorials_menu, 1 );
	Menu_Draw( &s_tutorials_menu );
}

static const char *M_Tutorials_Key( int key )
{
	return Default_MenuKey( &s_tutorials_menu, key );
}

static const char *M_Tutorials_CharEvent( qwchar key )
{
	return Default_MenuCharEvent( &s_tutorials_menu, key );
}

void M_Menu_Tutorials_f( void )
{
	M_TutorialsInit();
	M_PushMenu( &s_tutorials_menu, M_Tutorials_Draw, M_Tutorials_Key, M_Tutorials_CharEvent );
	Menu_SetStatusBar( &s_tutorials_menu, "these tutorials are narrated, if you do not hear anything, please ensure the music volume is not muted in the 'Sound Options' menu" );
}
