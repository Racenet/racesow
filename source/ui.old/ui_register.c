/*

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

static menuframework_s s_register_menu;

static char pass[MAX_PASS_LENGTH+1];
static char confirmpass[MAX_PASS_LENGTH+1];

/*
* Register_MenuInit
* ================
static void Register_MenuInit( void )
{
	menucommon_t *menuitem;
	int yoffset = 0;

	memset( pass, 0, sizeof( pass ) );
	memset( confirmpass, 0, sizeof( confirmpass ) );

	menuitem = UI_InitMenuItem( "m_register_title1", "REGISTER", 0, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemBig, NULL );
	Menu_AddItem( &s_register_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );
	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_register_email", "email", 0, yoffset, MTYPE_FIELD, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	UI_SetupField( menuitem, "", MAX_EMAIL_LENGTH, 200 );
	Menu_AddItem( &s_register_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_register_email2", "confirm email", 0, yoffset, MTYPE_FIELD, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	UI_SetupField( menuitem, "", MAX_EMAIL_LENGTH, 200 );
	Menu_AddItem( &s_register_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );
	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_register_pass", "password", 0, yoffset, MTYPE_FIELD, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	UI_SetupField( menuitem, "", MAX_PASS_LENGTH, 150 );
	UI_SetupFlags( menuitem, F_PASSWORD );
	Menu_AddItem( &s_register_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_register_pass2", "confirm password", 0, yoffset, MTYPE_FIELD, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	UI_SetupField( menuitem, "", MAX_PASS_LENGTH, 150 );
	UI_SetupFlags( menuitem, F_PASSWORD );
	Menu_AddItem( &s_register_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );
	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_register_back", "back", -12, yoffset, MTYPE_ACTION, ALIGN_RIGHT_TOP, uis.fontSystemBig, M_genericBackFunc );
	Menu_AddItem( &s_register_menu, menuitem );

	menuitem = UI_InitMenuItem( "m_register_register", "register", 12, yoffset, MTYPE_ACTION, ALIGN_LEFT_TOP, uis.fontSystemBig, NULL );
	Menu_AddItem( &s_register_menu, menuitem );

	Menu_Center( &s_register_menu );
	Menu_Init( &s_register_menu, qfalse );
}
*/
// Register_MenuDraw
// ================
static void Register_MenuDraw( void )
{
	Menu_AdjustCursor( &s_register_menu, 1 );
	Menu_Draw( &s_register_menu );
}

/*
* Register_MenuKey
*/
static const char *Register_MenuKey( int key )
{
	return Default_MenuKey( &s_register_menu, key );
}

/*
* Register_MenuCharEvent
*/
static const char *Register_MenuCharEvent( qwchar key )
{
	return Default_MenuCharEvent( &s_register_menu, key );
}

/*
* M_Menu_Register_f
*/
void M_Menu_Register_f( void )
{
	Register_MenuInit();

	//UI_AuthReply_Callback = Register_AuthReply_Callback;
	//trap_Auth_CheckUser( NULL, NULL );

	M_PushMenu( &s_register_menu, Register_MenuDraw, Register_MenuKey, Register_MenuCharEvent );
	//M_SetupCloseCallback( Register_Closing );
}
