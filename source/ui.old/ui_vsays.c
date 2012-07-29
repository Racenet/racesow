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

VSAYS MENU

=======================================================================
*/

static menuframework_s s_vsay_menu;

// wsw : pb : automatic menu
#define VSAYMENU_MAX_NAME   32
#define VSAYMENU_MAX_TITLE  64

typedef struct
{
	char name[VSAYMENU_MAX_NAME];
	char title[VSAYMENU_MAX_TITLE];
} KeyMenuItem;

static const int TeamItemsStart = 9; // ugly hack
static KeyMenuItem KMItems[] =
{
	{ "vsay yes", "Yes" },
	{ "vsay no", "No" },
	{ "vsay oops", "Oops" },
	{ "vsay sorry",	"Sorry" },
	{ "vsay thanks", "Thanks" },
	{ "vsay yeehaa", "Yeehaa" },
	{ "vsay goodgame", "Good Game" },
	{ "vsay booo", "Boo" },
	{ "vsay shutup", "Shut Up" },
	{ "vsay_team affirmative", "Affirmative" },
	{ "vsay_team roger", "Roger" },
	{ "vsay_team negative",	"Negative" },
	{ "vsay_team needhealth", "Need Health" },
	{ "vsay_team needweapon", "Need Weapon" },
	{ "vsay_team needarmor", "Need Armor" },
	{ "vsay_team ondefense", "On Defense" },
	{ "vsay_team onoffense", "On Offense" },
	{ "vsay_team noproblem", "No Problem" },
	{ "vsay_team defend", "Defend" },
	{ "vsay_team attack", "Attack" },
	{ "vsay_team needbackup", "Need Backup" },
	{ "vsay_team needdefense", "Need Defense" },
	{ "vsay_team needoffense", "Need Offense" },
	{ "vsay_team needhelp",	"Need Help" },
	{ "vsay_team armorfree", "Armor Free" },
	{ "vsay_team areasecured", "Area Secured" },
};

static int KM_NbItems = sizeof( KMItems )/sizeof( KeyMenuItem );
// wsw : pb : !automatic menu

static void M_UnbindCommand( char *command )
{
	int j;
	const char *b;

	for( j = 0; j < 256; j++ )
	{
		b = trap_Key_GetBindingBuf( j );
		if( !b )
			continue;
		if( !Q_stricmp( b, command ) )
			trap_Key_SetBinding( j, NULL );
	}
}

static void M_FindKeysForCommand( char *command, int *twokeys )
{
	int count, j;
	const char *b;

	twokeys[0] = twokeys[1] = -1;
	count = 0;

	for( j = 0; j < 256; j++ )
	{
		b = trap_Key_GetBindingBuf( j );
		if( !b )
			continue;
		if( !Q_stricmp( b, command ) )
		{
			twokeys[count] = j;
			count++;
			if( count == 2 )
				break;
		}
	}
}

static void KeyCursorDrawFunc( menuframework_s *menu )
{
	menucommon_t *item;
	struct mufont_s *font = uis.fontSystemSmall;
	int picsize = trap_SCR_strHeight( font );

	item = Menu_ItemAtCursor( menu );

	if( !Q_stricmp( item->name, "m_vsay_back" ) )
		return;

	if( uis.bind_grab )
		trap_SCR_DrawString( menu->x + item->cursor_offset, menu->y + item->y, ALIGN_LEFT_TOP, "=", font, UI_COLOR_HIGHLIGHT );
	else
	{
		if( ( int ) ( uis.time / 250 ) & 1 )
		{
			trap_R_DrawStretchPic( menu->x + item->cursor_offset, menu->y + item->y, picsize, picsize, 0, 0, 1, 1,
				UI_COLOR_HIGHLIGHT, trap_R_RegisterPic( "gfx/ui/arrow_right" ) );
		}
	}
}

static void DrawKeyBindingFunc( menucommon_t *menuitem )
{
	const char *name;
	int keys[2];
	int total_width = trap_SCR_strWidth( menuitem->title, uis.fontSystemSmall, 0 ) + 16;
	vec4_t text_color;
	Menu_ItemAtCursor( menuitem->parent ) == menuitem ? Vector4Copy( UI_COLOR_HIGHLIGHT, text_color ) : Vector4Copy( UI_COLOR_LIVETEXT, text_color );

	M_FindKeysForCommand( KMItems[menuitem->localdata[0]].name, keys );

	if( keys[0] == -1 )
		name = "???";
	else
	{
		name = trap_Key_KeynumToString( keys[0] );

		if( keys[1] != -1 )
		{
			name = va( "%s or", name ); //evidently va is broken on single character strings, seperating to two lines
			name = va( "%s %s", name, trap_Key_KeynumToString( keys[1] ) );
		}
	}

	if( Menu_ItemAtCursor( menuitem->parent ) == menuitem )
		UI_DrawStringHigh( menuitem->x + menuitem->parent->x + 16,
		menuitem->y + menuitem->parent->y, ALIGN_LEFT_TOP, name, 0, uis.fontSystemSmall, text_color );
	else
		UI_DrawString( menuitem->x + menuitem->parent->x + 16,
		menuitem->y + menuitem->parent->y, ALIGN_LEFT_TOP, name, 0, uis.fontSystemSmall, text_color );
	menuitem->width = total_width + trap_SCR_strWidth( name, uis.fontSystemSmall, 0 );
}

static void KeyBindingFunc( menucommon_t *menuitem )
{
	int keys[2];

	M_FindKeysForCommand( KMItems[menuitem->localdata[0]].name, keys );

	if( keys[1] != -1 )
		M_UnbindCommand( KMItems[menuitem->localdata[0]].name );

	uis.bind_grab = qtrue;

	Menu_SetStatusBar( &s_vsay_menu, "press a key or button for this action" );
}

static void Vsays_MenuInit( void )
{
	int i, yoffset = 0;
	menucommon_t *menuitem;

	s_vsay_menu.nitems = 0;
	s_vsay_menu.cursordraw = KeyCursorDrawFunc;

	// title
	menuitem = UI_InitMenuItem( "m_vsay_title1", "VOICE MESSAGES", 0, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemBig, NULL );
	Menu_AddItem( &s_vsay_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_vsay_title2", "Public Messages", -160, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_vsay_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	for( i = 0; i < TeamItemsStart; i++ )
	{
		menuitem = UI_InitMenuItem( KMItems[i].name, KMItems[i].title, -160, yoffset, MTYPE_ACTION, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
		menuitem->cursor_offset = menuitem->x;
		menuitem->localdata[0] = i;
		menuitem->ownerdraw = DrawKeyBindingFunc;
		Menu_AddItem( &s_vsay_menu, menuitem );

		yoffset += trap_SCR_strHeight( menuitem->font );
	}

	yoffset = 2 *trap_SCR_strHeight( uis.fontSystemBig );
	menuitem = UI_InitMenuItem( "m_vsay_title3", "Team Messages", 160, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_vsay_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	for( i = TeamItemsStart; i < KM_NbItems; i++ )
	{
		menuitem = UI_InitMenuItem( KMItems[i].name, KMItems[i].title, 160, yoffset, MTYPE_ACTION, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
		menuitem->cursor_offset = menuitem->x;
		menuitem->localdata[0] = i;
		menuitem->ownerdraw = DrawKeyBindingFunc;
		Menu_AddItem( &s_vsay_menu, menuitem );

		yoffset += trap_SCR_strHeight( menuitem->font );
	}

	// back (to previous menu)
	yoffset += trap_SCR_strHeight( uis.fontSystemSmall ); //put in a little padding
	menuitem = UI_InitMenuItem( "m_vsay_back", "back", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, M_genericBackFunc );
	Menu_AddItem( &s_vsay_menu, menuitem );
	UI_SetupButton( menuitem, qtrue );

	Menu_Center( &s_vsay_menu );
	Menu_Init( &s_vsay_menu, qfalse );

	Menu_SetStatusBar( &s_vsay_menu, "enter to change, backspace to clear" );
}

static void Vsays_MenuDraw( void )
{
	Menu_AdjustCursor( &s_vsay_menu, 1 );
	Menu_Draw( &s_vsay_menu );
}

static const char *Vsays_MenuKey( int key )
{
	menucommon_t *menuitem = Menu_ItemAtCursor( &s_vsay_menu );

	if( uis.bind_grab )
	{
		if( key != K_ESCAPE && key != '`' )
		{
			char cmd[1024];

			Q_snprintfz( cmd, sizeof( cmd ), "bind \"%s\" \"%s\"\n", trap_Key_KeynumToString( key ), KMItems[menuitem->localdata[0]].name );
			trap_Cmd_ExecuteText( EXEC_INSERT, cmd );
		}

		Menu_SetStatusBar( &s_vsay_menu, "enter to change, backspace to clear" );
		uis.bind_grab = qfalse;
		return menu_out_sound;
	}

	switch( key )
	{
	case KP_ENTER:
	case K_ENTER:
	case K_MOUSE1:
		if( !Q_stricmp( menuitem->name, "m_vsay_back" ) )
		{
			menuitem->callback( menuitem );
			return menu_out_sound;
		}
		else
		{
			KeyBindingFunc( menuitem );
			return menu_in_sound;
		}
	case K_BACKSPACE:   // delete bindings
	case K_DEL:         // delete bindings
	case KP_DEL:
		M_UnbindCommand( KMItems[menuitem->localdata[0]].name );
		return menu_out_sound;
	default:
		return Default_MenuKey( &s_vsay_menu, key );
	}
}

static const char *Vsays_MenuCharEvent( qwchar key )
{
	return Default_MenuCharEvent( &s_vsay_menu, key );
}

void M_Menu_Vsays_f( void )
{
	Vsays_MenuInit();
	M_PushMenu( &s_vsay_menu, Vsays_MenuDraw, Vsays_MenuKey, Vsays_MenuCharEvent );
}
