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

CONTROLS MENU

=======================================================================
*/
static menuframework_s s_options_menu;

static void JoystickFunc( menucommon_t *menuitem )
{
	trap_Cvar_SetValue( "in_joystick", menuitem->curvalue );
}

static void MouseSpeedFunc( menucommon_t *menuitem )
{
	trap_Cvar_Set( "sensitivity", va( "%g", (float)menuitem->curvalue / 4 ) );
}

static void MouseAccelFunc( menucommon_t *menuitem )
{
	trap_Cvar_Set( "m_accel", va( "%g", (float)menuitem->curvalue / 4 ) );
}

static void MouseFilterFunc( menucommon_t *menuitem )
{
	trap_Cvar_SetValue( "m_filter", menuitem->curvalue );
}

#ifdef _WIN32
static void NoAltTabFunc( menucommon_t *menuitem )
{
	trap_Cvar_SetValue( "win_noalttab", menuitem->curvalue );
}
#endif

static void InvertMouseFunc( menucommon_t *menuitem )
{
	trap_Cvar_SetValue( "m_pitch", -trap_Cvar_Value( "m_pitch" ) );
}

static void Options_MenuInit( void )
{
	menucommon_t *menuitem;
	int yoffset = 0;

	/*
	** configure controls menu and menu items
	*/
	s_options_menu.nitems = 0;

	menuitem = UI_InitMenuItem( "m_options_title1", "CONTROLLER OPTIONS", 0, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemBig, NULL );
	Menu_AddItem( &s_options_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	yoffset += trap_SCR_strHeight( menuitem->font );

	// sensitivity
	menuitem = UI_InitMenuItem( "m_options_mousespeed", "mouse speed", 0, yoffset, MTYPE_SLIDER, ALIGN_RIGHT_TOP, uis.fontSystemSmall, MouseSpeedFunc );
	Menu_AddItem( &s_options_menu, menuitem );
	UI_SetupSlider( menuitem, 16, trap_Cvar_Value( "sensitivity" )* 4, 4, 64 );
	yoffset += trap_SCR_strHeight( menuitem->font );

	// accel
	menuitem = UI_InitMenuItem( "m_options_mouseaccel", "mouse acceleration", 0, yoffset, MTYPE_SLIDER, ALIGN_RIGHT_TOP, uis.fontSystemSmall, MouseAccelFunc );
	Menu_AddItem( &s_options_menu, menuitem );
	UI_SetupSlider( menuitem, 16, trap_Cvar_Value( "m_accel" )* 4, 4, 64 );
	yoffset += trap_SCR_strHeight( menuitem->font );

	// m_filter
	menuitem = UI_InitMenuItem( "m_options_mfilter", "filter mouse", 0, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, MouseFilterFunc );
	Menu_AddItem( &s_options_menu, menuitem );
	UI_SetupSpinControl( menuitem, noyes_names, trap_Cvar_Value( "m_filter" ) );
	yoffset += trap_SCR_strHeight( menuitem->font );

	// invert mouse
	menuitem = UI_InitMenuItem( "m_options_invertmouse", "invert mouse", 0, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, InvertMouseFunc );
	Menu_AddItem( &s_options_menu, menuitem );
	UI_SetupSpinControl( menuitem, noyes_names, trap_Cvar_Value( "m_pitch" ) < 0 );
	yoffset += trap_SCR_strHeight( menuitem->font );

#ifdef _WIN32
	// win_noalttab
	menuitem = UI_InitMenuItem( "m_options_noalttab", "disable alt-tab", 0, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NoAltTabFunc );
	Menu_AddItem( &s_options_menu, menuitem );
	UI_SetupSpinControl( menuitem, noyes_names, trap_Cvar_Value( "win_noalttab" ) != 0 );
	yoffset += trap_SCR_strHeight( menuitem->font );
#endif

	// in_joystick
	menuitem = UI_InitMenuItem( "m_options_joystick", "use joystick", 0, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, JoystickFunc );
	Menu_AddItem( &s_options_menu, menuitem );
	UI_SetupSpinControl( menuitem, noyes_names, trap_Cvar_Value( "in_joystick" ) != 0 );
	yoffset += trap_SCR_strHeight( menuitem->font );

	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_options_back", "back", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, M_genericBackFunc );
	Menu_AddItem( &s_options_menu, menuitem );
	yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;

	Menu_Center( &s_options_menu );
	Menu_Init( &s_options_menu, qfalse );
}

static void Options_MenuDraw( void )
{
	Menu_AdjustCursor( &s_options_menu, 1 );
	Menu_Draw( &s_options_menu );
}

static const char *Options_MenuKey( int key )
{
	return Default_MenuKey( &s_options_menu, key );
}

static const char *Options_MenuCharEvent( qwchar key )
{
	return Default_MenuCharEvent( &s_options_menu, key );
}

void M_Menu_Options_f( void )
{
	Options_MenuInit();
	M_PushMenu( &s_options_menu, Options_MenuDraw, Options_MenuKey, Options_MenuCharEvent );
}
