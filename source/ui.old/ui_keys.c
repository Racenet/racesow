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

   KEYS MENU

   =======================================================================
 */

static menuframework_s s_keys_menu;


// wsw : pb : automatic menu
#define KEYMENU_MAX_NAME    32
#define KEYMENU_MAX_TITLE   64

typedef struct
{
	char name[KEYMENU_MAX_NAME];
	char title[KEYMENU_MAX_TITLE];
} KeyMenuItem;

static int MovementKeys = 8; //ugly hacks
static int WeaponKeys = 17;
static int UseWeaponKeys = 25;

static KeyMenuItem KMItems[] =
{
	// Movement
	{ "+forward", "forward" },
	{ "+back", "back" },
	{ "+moveleft", "left" },
	{ "+moveright",	"right" },
	{ "+moveup", "up / jump" },
	{ "+movedown", "down / crouch" },
	{ "+special", "dash / wj" },
	{ "+speed", "run / walk" },

	// Attack & Weapons
	{ "+attack", "attack" },
	{ "weapprev", "previous weapon" },
	{ "weapnext", "next weapon" },
	{ "weaplast", "last weapon" },
	{ "+zoom", "zoom" },
	{ "drop weapon", "drop weapon" },
	{ "drop flag", "drop flag" },
	{ "classaction1", "class action 1" },
	{ "classaction2", "class action 2" },

	{ "use Gunblade", "use gunblade" },
	{ "use Machinegun", "use machinegun" },
	{ "use Riotgun", "use riotgun" },
	{ "use Grenade Launcher", "use grenade launcher" },
	{ "use Rocket Launcher", "use rocket launcher" },
	{ "use Plasmagun", "use plasmagun" },
	{ "use LaserGun", "use lasergun" },
	{ "use Electrobolt", "use electrobolt" },

	// Misc
	{ "+scores", "show scores" },
	{ "vote yes", "vote yes" },
	{ "vote no", "vote no" },
	{ "join", "join" },
	{ "ready", "ready" },
	{ "chase", "chase cam" },
	{ "messagemode", "chat" },
	{ "messagemode2", "team chat" },
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
	int j, count = 0;
	const char *b;

	twokeys[0] = twokeys[1] = -1;
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

	if( !Q_stricmp( item->name, "m_keys_back" ) )
		return;

	if( uis.bind_grab )
		trap_SCR_DrawString( menu->x + item->cursor_offset, menu->y + item->y, ALIGN_LEFT_TOP, "=", font, UI_COLOR_HIGHLIGHT );
	else
	{
		if( ( int ) ( uis.time / 250 ) & 1 )
		{
			trap_R_DrawStretchPic( menu->x + item->cursor_offset, menu->y + item->y, picsize, picsize, 0, 0, 1, 1,
			                      UI_COLOR_HIGHLIGHT, uis.gfxArrowRight );
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
	menuitem->active_width = total_width + trap_SCR_strWidth( name, uis.fontSystemSmall, 0 );
}

static void KeyBindingFunc( menucommon_t *menuitem )
{
	int keys[2];

	M_FindKeysForCommand( KMItems[menuitem->localdata[0]].name, keys );

	if( keys[1] != -1 )
		M_UnbindCommand( KMItems[menuitem->localdata[0]].name );

	uis.bind_grab = qtrue;

	Menu_SetStatusBar( &s_keys_menu, "press a key or button for this action" );
}

static void Keys_MenuInit( void )
{
	int i, tempoffset, yoffset = 0;
	menucommon_t *menuitem;

	s_keys_menu.nitems = 0;
	s_keys_menu.cursordraw = KeyCursorDrawFunc;

	menuitem = UI_InitMenuItem( "m_keys_title1", "KEYBOARD CONTROLS", 0, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemBig, NULL );
	Menu_AddItem( &s_keys_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_keys_title2", "Movement", -160, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_keys_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	for( i = 0; i < MovementKeys; i++ )
	{
		menuitem = UI_InitMenuItem( KMItems[i].name, KMItems[i].title, -160, yoffset, MTYPE_ACTION, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
		menuitem->cursor_offset = -160;
		menuitem->localdata[0] = i;
		menuitem->ownerdraw = DrawKeyBindingFunc;
		Menu_AddItem( &s_keys_menu, menuitem );

		yoffset += trap_SCR_strHeight( menuitem->font );
	}

	yoffset = 2 *trap_SCR_strHeight( uis.fontSystemBig );

	menuitem = UI_InitMenuItem( "m_keys_title3", "Misc", 160, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_keys_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	for( i = UseWeaponKeys; i < KM_NbItems; i++ )
	{
		menuitem = UI_InitMenuItem( KMItems[i].name, KMItems[i].title, 160, yoffset, MTYPE_ACTION, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
		menuitem->cursor_offset = 160;
		menuitem->localdata[0] = i;
		menuitem->ownerdraw = DrawKeyBindingFunc;
		Menu_AddItem( &s_keys_menu, menuitem );

		yoffset += trap_SCR_strHeight( menuitem->font );
	}

	yoffset += 2 *trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_keys_title4", "Weapons", 0, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_keys_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	tempoffset = yoffset;

	for( i = MovementKeys; i < WeaponKeys; i++ )
	{
		menuitem = UI_InitMenuItem( KMItems[i].name, KMItems[i].title, -160, yoffset, MTYPE_ACTION, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
		menuitem->cursor_offset = -160;
		menuitem->localdata[0] = i;
		menuitem->ownerdraw = DrawKeyBindingFunc;
		Menu_AddItem( &s_keys_menu, menuitem );

		yoffset += trap_SCR_strHeight( menuitem->font );
	}

	yoffset = tempoffset;

	for( i = WeaponKeys; i < UseWeaponKeys; i++ )
	{
		menuitem = UI_InitMenuItem( KMItems[i].name, KMItems[i].title, 160, yoffset, MTYPE_ACTION, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
		menuitem->cursor_offset = 160;
		menuitem->localdata[0] = i;
		menuitem->ownerdraw = DrawKeyBindingFunc;
		Menu_AddItem( &s_keys_menu, menuitem );

		yoffset += trap_SCR_strHeight( menuitem->font );
	}

	// back (to previous menu)
	yoffset += trap_SCR_strHeight( uis.fontSystemSmall ); //put in a little padding
	menuitem = UI_InitMenuItem( "m_keys_back", "back", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, M_genericBackFunc );
	Menu_AddItem( &s_keys_menu, menuitem );
	UI_SetupButton( menuitem, qtrue );

	Menu_Center( &s_keys_menu );
	Menu_Init( &s_keys_menu, qfalse );

	Menu_SetStatusBar( &s_keys_menu, "enter to change, backspace to clear" );
}

static void Keys_MenuDraw( void )
{
	Menu_AdjustCursor( &s_keys_menu, 1 );
	Menu_Draw( &s_keys_menu );
}

static const char *Keys_MenuKey( int key )
{
	menucommon_t *menuitem = Menu_ItemAtCursor( &s_keys_menu );

	if( uis.bind_grab )
	{
		if( key != K_ESCAPE && key != '`' )
		{
			char cmd[1024];

			Q_snprintfz( cmd, sizeof( cmd ), "bind \"%s\" \"%s\"\n", trap_Key_KeynumToString( key ), KMItems[menuitem->localdata[0]].name );
			trap_Cmd_ExecuteText( EXEC_INSERT, cmd );
		}

		Menu_SetStatusBar( &s_keys_menu, "enter to change, backspace to clear" );
		uis.bind_grab = qfalse;
		return menu_out_sound;
	}

	switch( key )
	{
	case KP_ENTER:
	case K_ENTER:
	case K_MOUSE1:
		if( !Q_stricmp( menuitem->name, "m_keys_back" ) )
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
		return Default_MenuKey( &s_keys_menu, key );
	}
}

static const char *Keys_MenuCharEvent( qwchar key )
{
	return Default_MenuCharEvent( &s_keys_menu, key );
}

void M_Menu_Keys_f( void )
{
	Keys_MenuInit();
	M_PushMenu( &s_keys_menu, Keys_MenuDraw, Keys_MenuKey, Keys_MenuCharEvent );
}
