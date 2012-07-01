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

#define M_MSGBOX_LINELEN 120
static char mbtext[MAX_STRING_CHARS];

static void M_Msgbox_SecondButton( menucommon_t *menuitem )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, va( "%s\n", ( char * )(menuitem->itemlocal) ) );

	UI_Free( menuitem->itemlocal );
	menuitem->itemlocal = NULL;

	trap_Cmd_Execute();
}

static void M_Msgbox_Init( void )
{
	int i, j, s;
	int lineend, len;
	int n = 0;
	const char *p;
	menucommon_t *menuitem = NULL;
	char menuitem_name[40];
	char scnd_btn_name[120] = { '\0' }, scnd_btn_action[120] = { '\0' };
	int width = 0, yoffset = 40;

	s_msgbox_menu.nitems = 0;

	mbtext[0] = 0;

	for( i = 1; i < trap_Cmd_Argc(); i++ )
	{
		Q_strncpyz( mbtext, trap_Cmd_Argv(i), sizeof( mbtext ) );
		len = strlen( mbtext );

		// a secret second button
		if( !strncmp( mbtext, "\\btn\\", 5 ) )
		{
			p = strstr( mbtext + 6, "\\" );
			if( p ) {
				mbtext[p - mbtext] = '\0';

				Q_strncpyz( scnd_btn_name, mbtext + 5, sizeof( scnd_btn_name ) );
				Q_strncpyz( scnd_btn_action, p + 1, sizeof( scnd_btn_action ) );
			}

			continue;
		}

		// split the text into lines
		for( s = 0; s <= len; s += j + 1 )
		{
			lineend = min( len - s, M_MSGBOX_LINELEN );

			for( j = lineend; j && mbtext[s+j] && mbtext[s+j] != ' '; j-- );
			if( !j ) j = lineend;

			mbtext[s+j] = '\0';

			Q_snprintfz( menuitem_name, sizeof( menuitem_name ), "m_msgbox_textline_%i", n );
			menuitem = UI_InitMenuItem( menuitem_name, mbtext + s, 0, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemSmall, NULL );
			Menu_AddItem( &s_msgbox_menu, menuitem );
			yoffset += trap_SCR_strHeight( menuitem->font );

			n++;
		}
	}

	//if we printed something, add one line separation
	if( menuitem )
		yoffset += trap_SCR_strHeight( menuitem->font );

	if( scnd_btn_name[0] && scnd_btn_action[0] )
		width = UI_StringWidth( "close", uis.fontSystemBig );
	else
		width = 0;

	menuitem = UI_InitMenuItem( "m_msgbox_close", "close", -width, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, M_genericBackFunc );
	Menu_AddItem( &s_msgbox_menu, menuitem );

	if( scnd_btn_name[0] && scnd_btn_action[0] )
	{
		UI_SetupButton( menuitem, qtrue );

		menuitem = UI_InitMenuItem( "m_msgbox_connect", scnd_btn_name, width, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, M_Msgbox_SecondButton );
		menuitem->itemlocal = UI_CopyString( scnd_btn_action );
		Menu_AddItem( &s_msgbox_menu, menuitem );
	}

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
