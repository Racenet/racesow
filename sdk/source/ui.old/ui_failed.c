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

   CONNECTION FAILED DIALOG

   =======================================================================
 */
static menuframework_s s_failed_menu;

static void GeneralReconnectFunc( menucommon_t *menuitem )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, "reconnect" );
	trap_Cmd_Execute();
}

static void GeneralOkFunc( menucommon_t *menuitem )
{
	M_Menu_Main_f();
}

static void PasswordCancelFunc( menucommon_t *menuitem )
{
	M_Menu_Main_f();
}

static void PasswordOkFunc( menucommon_t *menuitem )
{
	menucommon_t *passworditem = UI_MenuItemByName( "m_failed_password" );

	trap_Cmd_ExecuteText( EXEC_APPEND, va( "set password \"%s\"\nreconnect",
	                                       ( (menufield_t *)passworditem->itemlocal )->buffer ) );
	trap_Cmd_Execute();
}

static void M_FailedInit( int screentype, char *servername, int errortype, char *reason )
{
	int width, yoffset = 0;
	menucommon_t *menuitem;

	s_failed_menu.nitems = 0;

	if( screentype == 0 )
		menuitem = UI_InitMenuItem( "m_failed_title_1", "Connection Failed", 0, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemBig, NULL );
	else
		menuitem = UI_InitMenuItem( "m_failed_title_1", "Connection Terminated", 0, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemBig, NULL );
	Menu_AddItem( &s_failed_menu, menuitem );
	yoffset += 2 *trap_SCR_strHeight( menuitem->font );

	if( screentype == 0 )
	{
		menuitem = UI_InitMenuItem( "m_failed_title_2", va( "%sCouldn't connect to server %s",
		                                                    S_COLOR_WHITE, servername ), -250, yoffset, MTYPE_SEPARATOR, ALIGN_LEFT_TOP, uis.fontSystemSmall, NULL );
	}
	else if( screentype == 1 )
	{
		menuitem = UI_InitMenuItem( "m_failed_title_2", va( "%sConnection was closed by server %s",
		                                                    S_COLOR_WHITE, servername ), -250, yoffset, MTYPE_SEPARATOR, ALIGN_LEFT_TOP, uis.fontSystemSmall, NULL );
	}
	else
	{
		menuitem = UI_InitMenuItem( "m_failed_title_2", va( "%sError in connection with server %s",
		                                                    S_COLOR_WHITE, servername ), -250, yoffset, MTYPE_SEPARATOR, ALIGN_LEFT_TOP, uis.fontSystemSmall, NULL );
	}
	Menu_AddItem( &s_failed_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_failed_reason", va( "%sReason: %s%s", S_COLOR_WHITE, S_COLOR_YELLOW, reason ), -250, yoffset, MTYPE_SEPARATOR, ALIGN_LEFT_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_failed_menu, menuitem );
	yoffset += 2 * trap_SCR_strHeight( menuitem->font );

	if( errortype == DROP_TYPE_PASSWORD )
	{
		const char *password = trap_Cvar_String( "password" );
		struct mufont_s *font = uis.fontSystemMedium;

		menuitem = UI_InitMenuItem( "m_failed_password", "Password:", -97, yoffset, MTYPE_FIELD, ALIGN_RIGHT_TOP, uis.fontSystemSmall, PasswordOkFunc );
		UI_SetupField( menuitem, password, 20, -1 );
		UI_SetupFlags( menuitem, F_PASSWORD );
		Menu_AddItem( &s_failed_menu, menuitem );
		yoffset += 2 * trap_SCR_strHeight( menuitem->font );

		width = UI_StringWidth( "Cancel", font );
		menuitem = UI_InitMenuItem( "m_failed_cancel", "Cancel", -width, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, font, PasswordCancelFunc );
		Menu_AddItem( &s_failed_menu, menuitem );
		UI_SetupButton( menuitem, qtrue );

		menuitem = UI_InitMenuItem( "m_failed_ok1", "OK", width, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, font, PasswordOkFunc );
		Menu_AddItem( &s_failed_menu, menuitem );
		yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;
	}
	else
	{
		struct mufont_s *font = uis.fontSystemMedium;

		if( errortype == DROP_TYPE_NORECONNECT )
		{
			width = -UI_StringWidth( "OK", font );
		}
		else if( screentype == 0 )
		{
			width = UI_StringWidth( "Try again", font );
			menuitem = UI_InitMenuItem( "m_failed_again", "Try again", -width, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, font, GeneralReconnectFunc );
			UI_SetupButton( menuitem, qtrue );
		}
		else
		{
			width = UI_StringWidth( "Reconnect", NULL );
			menuitem = UI_InitMenuItem( "m_failed_again", "Reconnect", -width, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, font, GeneralReconnectFunc );
			UI_SetupButton( menuitem, qtrue );
		}

		Menu_AddItem( &s_failed_menu, menuitem );

		menuitem = UI_InitMenuItem( "m_failed_ok2", "OK", width, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, font, GeneralOkFunc );
		Menu_AddItem( &s_failed_menu, menuitem );
		yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;
	}

	Menu_Center( &s_failed_menu );
	Menu_Init( &s_failed_menu, qfalse );
	Menu_SetStatusBar( &s_failed_menu, NULL );
}


static void M_Failed_Draw( void )
{
	Menu_AdjustCursor( &s_failed_menu, 1 );
	Menu_Draw( &s_failed_menu );
}

static const char *M_Failed_Key( int key )
{
	return Default_MenuKey( &s_failed_menu, key );
}

static const char *M_Failed_CharEvent( qwchar key )
{
	return Default_MenuCharEvent( &s_failed_menu, key );
}

void M_Menu_Failed_f( void )
{
	if( trap_Cmd_Argc() == 5 )
		M_FailedInit( atoi( trap_Cmd_Argv( 1 ) ), trap_Cmd_Argv( 2 ), atoi( trap_Cmd_Argv( 3 ) ), trap_Cmd_Argv( 4 ) );
	else
		M_FailedInit( qfalse, "Error", DROP_TYPE_GENERAL, "Unknown reason" );

	M_PushMenu( &s_failed_menu, M_Failed_Draw, M_Failed_Key, M_Failed_CharEvent );
}
