/*
   Copyright (C) 2007 Will Franklin

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

//====================
// LOGIN MENU
//====================

// this can be used to get the status back from the login window and
// take appropriate action
void ( *M_Login_Callback )( qboolean status ) = NULL;
void ( *UI_AuthReply_Callback )( auth_reply_t reply ) = NULL;

static menuframework_s s_login_menu;

static void M_Login_Popped( void )
{
	if( M_Login_Callback )
	{
		M_Login_Callback( qfalse );
		M_Login_Callback = NULL;
	}
}

static void Login_AuthReply_Callback( auth_reply_t reply )
{
	if( reply == AUTH_VALID )
	{
		if( M_Login_Callback )
		{
			M_Login_Callback( qtrue );
			M_Login_Callback = NULL;
		}
		M_genericBackFunc( NULL );
		return;
	}
	if( reply == AUTH_INVALID )
	{
		Menu_SetStatusBar( &s_login_menu, "email address or password incorrect" );
		return;
	}
	if( reply == AUTH_CHECKING )
	{
		Menu_SetStatusBar( &s_login_menu, "contacting auth server" );
		return;
	}
}

static void M_Login_Login( menucommon_t *unused )
{
	char *email, *pass;

	email = UI_GetMenuitemFieldBuffer( UI_MenuItemByName( "m_login_email" ) );
	pass = UI_GetMenuitemFieldBuffer( UI_MenuItemByName( "m_login_pass" ) );
	if( !email || !*email )
	{
		Menu_SetStatusBar( &s_login_menu, "please enter an email address" );
		return;
	}

	if( !pass || !*pass )
	{
		Menu_SetStatusBar( &s_login_menu, "please enter a password" );
		return;
	}

	UI_AuthReply_Callback = Login_AuthReply_Callback;
	//trap_Auth_CheckUser( email, pass );
}

static void M_Login_Register( menucommon_t *unused )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, "menu_register\n" );
}

static void M_Menu_Login_Init( void )
{
	menucommon_t *menuitem;
	int yoffset = 0;

	menuitem = UI_InitMenuItem( "m_login_title_1", "WARSOW LOGIN", 0, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemBig, NULL );
	Menu_AddItem( &s_login_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );
	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_login_email", "email", 0, yoffset, MTYPE_FIELD, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	UI_SetupField( menuitem, "", MAX_EMAIL_LENGTH, 200 );
	Menu_AddItem( &s_login_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_login_pass", "password", 0, yoffset, MTYPE_FIELD, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	UI_SetupField( menuitem, "", MAX_PASS_LENGTH, 150 );
	UI_SetupFlags( menuitem, F_PASSWORD );
	Menu_AddItem( &s_login_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_login_back", "back", -70, yoffset, MTYPE_ACTION, ALIGN_RIGHT_TOP, uis.fontSystemBig, M_genericBackFunc );
	Menu_AddItem( &s_login_menu, menuitem );

	menuitem = UI_InitMenuItem( "m_login_submit", "login", -25, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, M_Login_Login );
	Menu_AddItem( &s_login_menu, menuitem );

	menuitem = UI_InitMenuItem( "m_login_register", "register", 20, yoffset, MTYPE_ACTION, ALIGN_LEFT_TOP, uis.fontSystemBig, M_Login_Register );
	Menu_AddItem( &s_login_menu, menuitem );

	Menu_Center( &s_login_menu );
	Menu_Init( &s_login_menu, qfalse );
}

static void M_Login_Draw( void )
{
	Menu_AdjustCursor( &s_login_menu, 1 );
	Menu_Draw( &s_login_menu );
}

static const char *M_Login_Key( int key )
{
	return Default_MenuKey( &s_login_menu, key );
}


static const char *M_Login_CharEvent( qwchar key )
{
	return Default_MenuCharEvent( &s_login_menu, key );
}

void M_Menu_Login_f( void )
{
	M_Menu_Login_Init();
	M_PushMenu( &s_login_menu, M_Login_Draw, M_Login_Key, M_Login_CharEvent );
	M_SetupPoppedCallback( M_Login_Popped );
}

// handler for a reply from the auth server, this can then redirect to the
// necessary place
void UI_AuthReply( auth_reply_t reply )
{
	if( UI_AuthReply_Callback )
		UI_AuthReply_Callback( reply );
}
