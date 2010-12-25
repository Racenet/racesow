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
#include <ctype.h>

#include "ui_local.h"

char *menu_in_sound	= S_UI_MENU_IN_SOUND;
char *menu_move_sound	= S_UI_MENU_MOVE_SOUND;
char *menu_out_sound	= S_UI_MENU_OUT_SOUND;

ui_local_t uis;

qboolean m_entersound;       // play after drawing a frame, so caching
                             // won't disrupt the sound
static unsigned int m_pushcount;

menuframework_s *m_active;
void *m_cursoritem;

cvar_t *developer;

static int m_keypressed;

// wsw: will
void ( *m_popped )( void );
void ( *m_drawfunc )(void);
const char *( *m_keyfunc )(int key);
const char *( *m_chareventfunc )(qwchar key);

//======================================================================

char *noyes_names[] =
{
	"no", "yes", 0
};

char *offon_names[] =
{
	"off", "on", 0
};

//======================================================================

/*
   ============
   UI_API
   ============
 */
int UI_API( void )
{
	return UI_API_VERSION;
}

/*
   ============
   UI_Error
   ============
 */
void UI_Error( const char *format, ... )
{
	va_list	argptr;
	char msg[1024];

	va_start( argptr, format );
	Q_vsnprintfz( msg, sizeof( msg ), format, argptr );
	va_end( argptr );

	trap_Error( msg );
}

/*
   ============
   UI_Printf
   ============
 */
void UI_Printf( const char *format, ... )
{
	va_list	argptr;
	char msg[1024];

	va_start( argptr, format );
	Q_vsnprintfz( msg, sizeof( msg ), format, argptr );
	va_end( argptr );

	trap_Print( msg );
}

//=============================================================================
/* Support Routines */

#define	MAX_MENU_DEPTH	8


typedef struct
{
	menuframework_s *m;
	void ( *draw )( void );
	const char *( *key )( int k );
	const char *( *charevent )( qwchar k );
	//wsw:will
	void ( *popped )( void );
} menulayer_t;

menulayer_t m_layers[MAX_MENU_DEPTH];
int m_menudepth;

static void UI_UpdateMousePosition( int dx, int dy );


//=================
//UI_RegisterFonts
//=================
static void UI_RegisterFonts( void )
{
	cvar_t *con_fontSystemSmall = trap_Cvar_Get( "con_fontSystemSmall", DEFAULT_FONT_SMALL, CVAR_ARCHIVE );
	cvar_t *con_fontSystemMedium = trap_Cvar_Get( "con_fontSystemMedium", DEFAULT_FONT_MEDIUM, CVAR_ARCHIVE );
	cvar_t *con_fontSystemBig = trap_Cvar_Get( "con_fontSystemBig", DEFAULT_FONT_BIG, CVAR_ARCHIVE );

	uis.fontSystemSmall = trap_SCR_RegisterFont( con_fontSystemSmall->string );
	if( !uis.fontSystemSmall )
	{
		uis.fontSystemSmall = trap_SCR_RegisterFont( DEFAULT_FONT_SMALL );
		if( !uis.fontSystemSmall )
			UI_Error( "Couldn't load default font \"%s\"", DEFAULT_FONT_SMALL );
	}
	uis.fontSystemMedium = trap_SCR_RegisterFont( con_fontSystemMedium->string );
	if( !uis.fontSystemMedium )
		uis.fontSystemMedium = trap_SCR_RegisterFont( DEFAULT_FONT_MEDIUM );

	uis.fontSystemBig = trap_SCR_RegisterFont( con_fontSystemBig->string );
	if( !uis.fontSystemBig )
		uis.fontSystemBig = trap_SCR_RegisterFont( DEFAULT_FONT_BIG );

}

static void M_Cache( void )
{
	// precache sounds
	trap_S_RegisterSound( S_UI_MENU_IN_SOUND );
	trap_S_RegisterSound( S_UI_MENU_MOVE_SOUND );
	trap_S_RegisterSound( S_UI_MENU_OUT_SOUND );

	uis.whiteShader = trap_R_RegisterPic( "gfx/ui/white" );

	UI_RegisterFonts();

	trap_R_RegisterPic( UI_SHADER_FXBACK );
	trap_R_RegisterPic( UI_SHADER_BIGLOGO );
	trap_R_RegisterPic( UI_SHADER_CURSOR );

	uis.gfxSlidebar_1 = trap_R_RegisterPic( "gfx/ui/slidebar_1" );
	uis.gfxSlidebar_2 = trap_R_RegisterPic( "gfx/ui/slidebar_2" );
	uis.gfxSlidebar_3 = trap_R_RegisterPic( "gfx/ui/slidebar_3" );
	uis.gfxSlidebar_4 = trap_R_RegisterPic( "gfx/ui/slidebar_4" );

	uis.gfxScrollbar_1 = trap_R_RegisterPic( "gfx/ui/scrollbar_1" );
	uis.gfxScrollbar_2 = trap_R_RegisterPic( "gfx/ui/scrollbar_2" );
	uis.gfxScrollbar_3 = trap_R_RegisterPic( "gfx/ui/scrollbar_3" );
	uis.gfxScrollbar_4 = trap_R_RegisterPic( "gfx/ui/scrollbar_4" );

	uis.gfxArrowUp = trap_R_RegisterPic( "gfx/ui/arrow_up" );
	uis.gfxArrowDown = trap_R_RegisterPic( "gfx/ui/arrow_down" );
	uis.gfxArrowRight = trap_R_RegisterPic( "gfx/ui/arrow_right" );

	// precache box images
	uis.gfxBoxUpLeft = trap_R_RegisterPic( "gfx/ui/ui_box_upleft" );
	uis.gfxBoxBorderUpLeft = trap_R_RegisterPic( "gfx/ui/ui_box_border_upleft" );
	uis.gfxBoxBorderLeft = trap_R_RegisterPic( "gfx/ui/ui_box_border_left" );
	uis.gfxBoxLeft = trap_R_RegisterPic( "gfx/ui/ui_box_left" );
	uis.gfxBoxBorderBottomLeft = trap_R_RegisterPic( "gfx/ui/ui_box_border_bottomleft" );
	uis.gfxBoxBottomLeft = trap_R_RegisterPic( "gfx/ui/ui_box_bottomleft" );
	uis.gfxBoxBorderUp = trap_R_RegisterPic( "gfx/ui/ui_box_border_up" );
	uis.gfxBoxUp = trap_R_RegisterPic( "gfx/ui/ui_box_up" );
	uis.gfxBoxBorderBottom = trap_R_RegisterPic( "gfx/ui/ui_box_border_bottom" );
	uis.gfxBoxBottom = trap_R_RegisterPic( "gfx/ui/ui_box_bottom" );
	uis.gfxBoxBorderUp = trap_R_RegisterPic( "gfx/ui/ui_box_border_up" );
	uis.gfxBoxUp = trap_R_RegisterPic( "gfx/ui/ui_box_up" );
	uis.gfxBoxBorderBottom = trap_R_RegisterPic( "gfx/ui/ui_box_border_bottom" );
	uis.gfxBoxBottom = trap_R_RegisterPic( "gfx/ui/ui_box_bottom" );
	uis.gfxBoxBorderUpRight = trap_R_RegisterPic( "gfx/ui/ui_box_border_upright" );
	uis.gfxBoxUpRight = trap_R_RegisterPic( "gfx/ui/ui_box_upright" );
	uis.gfxBoxBorderRight = trap_R_RegisterPic( "gfx/ui/ui_box_border_right" );
	uis.gfxBoxRight = trap_R_RegisterPic( "gfx/ui/ui_box_right" );
	uis.gfxBoxBorderBottomRight = trap_R_RegisterPic( "gfx/ui/ui_box_border_bottomright" );
	uis.gfxBoxBottomRight = trap_R_RegisterPic( "gfx/ui/ui_box_bottomright" );
}

void M_PushMenu( menuframework_s *m, void ( *draw )(void), const char *( *key )(int k), const char *( *charevent )(qwchar k) )
{
	int i;

	// if this menu is already present, drop back to that level
	// to avoid stacking menus by hotkeys
	for( i = 0; i < m_menudepth; i++ )
	{
		if( m_layers[i].m == m && m_layers[i].draw == draw && m_layers[i].key == key )
		{
			m_menudepth = i;
		}
	}

	if( i == m_menudepth )
	{
		if( m_menudepth >= MAX_MENU_DEPTH )
		{
			UI_Error( "M_PushMenu: MAX_MENU_DEPTH" );
			return;
		}

		m_layers[m_menudepth].m = m_active;
		m_layers[m_menudepth].draw = m_drawfunc;
		m_layers[m_menudepth].key = m_keyfunc;
		m_layers[m_menudepth].charevent = m_chareventfunc;
		//wsw:will
		m_layers[m_menudepth].popped = m_popped;
		m_menudepth++;
	}

	m_drawfunc = draw;
	m_keyfunc = key;
	m_chareventfunc = charevent;
	m_active = m;
	//wsw:will:set elsewhere
	m_popped = NULL;

	m_entersound = m_pushcount > 0;
	m_pushcount++;

	UI_UpdateMousePosition( 0, 0 );

	trap_CL_SetKeyDest( key_menu );
}

void M_ForceMenuOff( void )
{
	m_active = 0;
	m_drawfunc = 0;
	m_keyfunc = 0;
	trap_CL_SetKeyDest( key_game );
	m_menudepth = 0;
	trap_Key_ClearStates();
	m_chareventfunc = 0;
}

void M_PopMenu( void )
{
	if( m_popped )
		m_popped();

	if( m_menudepth == 1 )
	{
		if( uis.clientState < CA_CONNECTING || uis.forceUI )
			return;
		M_ForceMenuOff();
		return;
	}

	trap_S_StartLocalSound( menu_out_sound );

	if( m_menudepth < 1 )
	{
		UI_Error( "M_PopMenu: depth < 1" );
		return;
	}

	m_menudepth--;

	m_drawfunc = m_layers[m_menudepth].draw;
	m_keyfunc = m_layers[m_menudepth].key;
	m_active = m_layers[m_menudepth].m;
	//wsw:will
	m_popped = m_layers[m_menudepth].popped;
	m_chareventfunc = m_layers[m_menudepth].charevent;

	UI_UpdateMousePosition( 0, 0 );
}

// a safe way to pop a menu from command line
static void Cmd_PopMenu_f( void )
{
	if( m_menudepth >= 1 )
		M_PopMenu();
}

// wsw:will
void M_SetupPoppedCallback( void ( *popped )( void ) )
{
	m_popped = popped;
}

void M_genericBackFunc( struct menucommon_s *menuitem )
{
	M_PopMenu();
}


const char *Default_MenuKey( menuframework_s *m, int key )
{
	const char *sound = NULL;
	menucommon_t *item = NULL;

	if( m )
	{
		item = Menu_ItemAtCursor( m );
		if( item != NULL )
		{
			if( item->type == MTYPE_FIELD )
			{
				if( Field_Key( item, key ) )
					return NULL;
			}
		}
	}

	switch( key )
	{
	case 70: //F  for adding to favorites
	case 102: //f
		if( m && ( item->type == MTYPE_ACTION ) )
			M_AddToFavorites( item );
		break;
	case 82: //R  for removing favorites
	case 114: //r
		if( m && ( item->type == MTYPE_ACTION ) )
			M_RemoveFromFavorites( item );
		break;
	case K_ESCAPE:
		M_PopMenu();
		return menu_out_sound;

	case K_MOUSE1:
		if( !Menu_SlideItem( m, 1, key ) )
			Menu_SelectItem( m );
		sound = menu_move_sound;
		break;

	case K_MOUSE2:
		if( m && ( m_cursoritem == item ) && Menu_SlideItem( m, -1, key ) )
			sound = menu_move_sound;
		else
		{
			M_PopMenu();
			sound = menu_out_sound;

		}
		break;

	case K_MWHEELUP:
		if( Menu_ItemAtCursor( m )->scrollbar_id )  //sliding a scrollbar moves 3 lines
			Menu_SlideItem( m, -3, key ); //scrolling up is the equivalent of sliding left, therefore inverted.
		else if( Menu_ItemAtCursor( m )->type == MTYPE_SPINCONTROL || Menu_ItemAtCursor( m )->type == MTYPE_SLIDER )  //sliding a spincontrol moves 1 item
			Menu_SlideItem( m, 1, key );
		break;
	case KP_UPARROW:
	case K_UPARROW:
		if( m )
		{
			menucommon_t *item = Menu_ItemAtCursor( m );
			menucommon_t *scroll = m->items[item->scrollbar_id];
			if( item->scrollbar_id && item->type == MTYPE_ACTION && item->localdata[0] == 0 && scroll->curvalue > 0 )
				Menu_SlideItem( m, -1, key );
			else
				m->cursor--;
			Menu_AdjustCursor( m, -1 );
			sound = menu_move_sound;
		}
		break;
	case K_TAB:
		if( m )
		{
			if( Menu_ItemAtCursor( m )->scrollbar_id && Menu_ItemAtCursor( m )->type == MTYPE_ACTION )
			{
				int i = Menu_ItemAtCursor( m )->scrollbar_id;
				while( i <= MAXMENUITEMS )
				{
					if( !m->items[i]->scrollbar_id )
					{
						m->cursor = i;
						break;
					}
					i++;
				}
			}
			else
				m->cursor++;
			Menu_AdjustCursor( m, 1 );
			sound = menu_move_sound;
		}
		break;

	case K_MWHEELDOWN:
		if( Menu_ItemAtCursor( m )->scrollbar_id )  //sliding a scrollbar moves 3 lines
			Menu_SlideItem( m, 3, key ); //scrolling down is the equivalent of sliding right, therefore inverted.
		else if( Menu_ItemAtCursor( m )->type == MTYPE_SPINCONTROL || Menu_ItemAtCursor( m )->type == MTYPE_SLIDER )  //sliding a spincontrol moves 1 item
			Menu_SlideItem( m, -1, key );
		break;
	case KP_DOWNARROW:
	case K_DOWNARROW:
		if( m )
		{
			menucommon_t *item = Menu_ItemAtCursor( m );
			menucommon_t *scroll = m->items[item->scrollbar_id];
			if( item->scrollbar_id && item->type == MTYPE_ACTION && m->items[m->cursor + 1]->scrollbar_id != item->scrollbar_id && scroll->curvalue < scroll->maxvalue )
				Menu_SlideItem( m, 1, key );
			else
				m->cursor++;
			Menu_AdjustCursor( m, 1 );
			sound = menu_move_sound;
		}
		break;
	case KP_LEFTARROW:
	case K_LEFTARROW:
		if( m )
		{
			Menu_SlideItem( m, -1, key );
			sound = menu_move_sound;
		}
		break;
	case KP_RIGHTARROW:
	case K_RIGHTARROW:
		if( m )
		{
			Menu_SlideItem( m, 1, key );
			sound = menu_move_sound;
		}
		break;

	case K_MOUSE3:
	case K_JOY1:
	case K_JOY2:
	case K_JOY3:
	case K_JOY4:
	case K_AUX1:
	case K_AUX2:
	case K_AUX3:
	case K_AUX4:
	case K_AUX5:
	case K_AUX6:
	case K_AUX7:
	case K_AUX8:
	case K_AUX9:
	case K_AUX10:
	case K_AUX11:
	case K_AUX12:
	case K_AUX13:
	case K_AUX14:
	case K_AUX15:
	case K_AUX16:
	case K_AUX17:
	case K_AUX18:
	case K_AUX19:
	case K_AUX20:
	case K_AUX21:
	case K_AUX22:
	case K_AUX23:
	case K_AUX24:
	case K_AUX25:
	case K_AUX26:
	case K_AUX27:
	case K_AUX28:
	case K_AUX29:
	case K_AUX30:
	case K_AUX31:
	case K_AUX32:

	case KP_ENTER:
	case K_ENTER:
		if( m )
			Menu_SelectItem( m );
		sound = menu_move_sound;
		break;

	case K_MOUSE1DBLCLK:
		if (m)
		{
			menucommon_t *item;
			Menu_SelectItem( m );
			item = Menu_ItemAtCursor( m );
			if (item && item->callback_doubleclick)
				item->callback_doubleclick(item);
		}
		sound = menu_move_sound;
		break;
	}

	return sound;
}

const char *Default_MenuCharEvent( menuframework_s *m, qwchar key )
{
	menucommon_t *item = NULL;

	if( m )
	{
		item = Menu_ItemAtCursor( m );
		if( item != NULL )
		{
			if( item->type == MTYPE_FIELD )
			{
				if( Field_CharEvent( item, key ) )
					return NULL;
			}
		}
	}

	return NULL;
}


float M_ClampCvar( float min, float max, float value )
{
	if( value < min ) return min;
	if( value > max ) return max;
	return value;
}

//=============================================================================

/*
   =================
   UI_CopyString
   =================
 */
char *_UI_CopyString( const char *in, const char *filename, int fileline )
{
	char *out;

	out = (char *)trap_Mem_Alloc( strlen( in )+1, filename, fileline );
	strcpy( out, in );
	return out;
}

//=============================================================================

//=============================================================================
/* User Interface Subsystem */

/*
   =================
   UI_Force_f
   =================
 */
void UI_Force_f( void )
{
	if( trap_Cmd_Argc() != 2 )
		return;

	uis.forceUI = (qboolean)( atoi( trap_Cmd_Argv( 1 ) ) != 0 );
}


/*
   =================
   UI_Init
   =================
 */
void UI_Init( int vidWidth, int vidHeight, int protocol, int sharedSeed )
{
	m_active = NULL;
	m_cursoritem = NULL;
	m_drawfunc = NULL;
	m_keyfunc = NULL;
	m_entersound = qfalse;
	m_keypressed = 0;

	memset( &uis, 0, sizeof( uis ) );

	uis.vidWidth = vidWidth;
	uis.vidHeight = vidHeight;
	uis.gameProtocol = protocol;

#if 0
	uis.scaleX = UI_WIDTHSCALE;
	uis.scaleY = UI_HEIGHTSCALE;
#else
	uis.scaleX = 1;
	uis.scaleY = 1;
#endif

	uis.cursorX = uis.vidWidth / 2;
	uis.cursorY = uis.vidHeight / 2;

	uis.initialSharedSeed = sharedSeed;
	uis.sharedSeed = uis.initialSharedSeed;

	uis.backgroundNum = Q_rand( &uis.sharedSeed ) % UI_SHADER_MAX_BACKGROUNDS;

	developer =	trap_Cvar_Get( "developer", "0", CVAR_CHEAT );

	// wsw/Mokshu : test svn addin for VS 2005 and what about a "help/news" menu (latest news from website for example)
	trap_Cmd_AddCommand( "menu_main", M_Menu_Main_f );
	trap_Cmd_AddCommand( "menu_main_sbar", M_Menu_Main_Statusbar_f );
	trap_Cmd_AddCommand( "menu_setup", M_Menu_Setup_f );
	trap_Cmd_AddCommand( "menu_joinserver", M_Menu_JoinServer_f );
	trap_Cmd_AddCommand( "menu_matchmaker", M_Menu_MatchMaker_f );
#ifdef AUTH_CODE
	trap_Cmd_AddCommand( "menu_login", M_Menu_Login_f );
	trap_Cmd_AddCommand( "menu_register", M_Menu_Register_f );
#endif
	trap_Cmd_AddCommand( "menu_playerconfig", M_Menu_PlayerConfig_f );
	trap_Cmd_AddCommand( "menu_startserver", M_Menu_StartServer_f );
	trap_Cmd_AddCommand( "menu_sound", M_Menu_Sound_f );
	trap_Cmd_AddCommand( "menu_options", M_Menu_Options_f );
	trap_Cmd_AddCommand( "menu_performance", M_Menu_Performance_f );
	trap_Cmd_AddCommand( "menu_performanceadv", M_Menu_PerformanceAdv_f );
	trap_Cmd_AddCommand( "menu_keys", M_Menu_Keys_f );
	trap_Cmd_AddCommand( "menu_vsays", M_Menu_Vsays_f );
	trap_Cmd_AddCommand( "menu_quit", M_Menu_Quit_f );
	trap_Cmd_AddCommand( "menu_reset", M_Menu_Reset_f );
	trap_Cmd_AddCommand( "menu_demos", M_Menu_Demos_f );
	trap_Cmd_AddCommand( "menu_mods", M_Menu_Mods_f );
	trap_Cmd_AddCommand( "menu_game", M_Menu_Game_f );
	trap_Cmd_AddCommand( "menu_tv", M_Menu_TV_f );
	trap_Cmd_AddCommand( "menu_tv_channel_add", M_Menu_TV_ChannelAdd_f );
	trap_Cmd_AddCommand( "menu_tv_channel_remove", M_Menu_TV_ChannelRemove_f );
	trap_Cmd_AddCommand( "menu_failed", M_Menu_Failed_f );
	trap_Cmd_AddCommand( "menu_msgbox", M_Menu_MsgBox_f );
	trap_Cmd_AddCommand( "menu_custom", M_Menu_Custom_f );
	trap_Cmd_AddCommand( "menu_chasecam", M_Menu_Chasecam_f );
	trap_Cmd_AddCommand( "menu_teamconfig", M_Menu_TeamConfig_f );
	trap_Cmd_AddCommand( "menu_force", UI_Force_f );
	trap_Cmd_AddCommand( "menu_tutorials", M_Menu_Tutorials_f );
	trap_Cmd_AddCommand( "menu_demoplay", M_Menu_Demoplay_f );
	trap_Cmd_AddCommand( "menu_pop", Cmd_PopMenu_f );

	M_Cache();
	UI_Playermodel_Init(); // create a list with the available player models
	UI_InitTemporaryBoneposesCache();

	uis.backGroundTrackStarted = qfalse;

	// jal: this is a small trick to assign userinfo flag to cg_oldMovement before cgame is loaded
	trap_Cvar_Get( "cg_oldMovement", "0", CVAR_USERINFO | CVAR_ARCHIVE );
	trap_Cvar_Get( "cg_noAutohop", "0", CVAR_USERINFO | CVAR_ARCHIVE );
}

/*
   =================
   UI_Shutdown
   =================
 */
void UI_Shutdown( void )
{
	trap_S_StopBackgroundTrack();

	trap_Cmd_RemoveCommand( "menu_main" );
	trap_Cmd_RemoveCommand( "menu_main_sbar" );
	trap_Cmd_RemoveCommand( "menu_setup" );
	trap_Cmd_RemoveCommand( "menu_joinserver" );
	trap_Cmd_RemoveCommand( "menu_matchmaker" );
#ifdef AUTH_CODE
	trap_Cmd_RemoveCommand( "menu_login" );
	trap_Cmd_RemoveCommand( "menu_register" );
#endif
	trap_Cmd_RemoveCommand( "menu_playerconfig" );
	trap_Cmd_RemoveCommand( "menu_startserver" );
	trap_Cmd_RemoveCommand( "menu_sound" );
	trap_Cmd_RemoveCommand( "menu_options" );
	trap_Cmd_RemoveCommand( "menu_performance" );
	trap_Cmd_RemoveCommand( "menu_performanceadv" );
	trap_Cmd_RemoveCommand( "menu_keys" );
	trap_Cmd_RemoveCommand( "menu_vsays" );
	trap_Cmd_RemoveCommand( "menu_quit" );
	trap_Cmd_RemoveCommand( "menu_reset" );
	trap_Cmd_RemoveCommand( "menu_demos" );
	trap_Cmd_RemoveCommand( "menu_mods" );
	trap_Cmd_RemoveCommand( "menu_game" );
	trap_Cmd_RemoveCommand( "menu_tv" );
	trap_Cmd_RemoveCommand( "menu_tv_channel_add" );
	trap_Cmd_RemoveCommand( "menu_tv_channel_remove" );
	trap_Cmd_RemoveCommand( "menu_failed" );
	trap_Cmd_RemoveCommand( "menu_msgbox" );
	trap_Cmd_RemoveCommand( "menu_custom" );
	trap_Cmd_RemoveCommand( "menu_teamconfig" );
	trap_Cmd_RemoveCommand( "menu_force" );
	trap_Cmd_RemoveCommand( "menu_tutorials" );
	trap_Cmd_RemoveCommand( "menu_demoplay" );
	trap_Cmd_RemoveCommand( "menu_pop" );
}

/*
   =================
   UI_UpdateMousePosition
   =================
 */
static void UI_UpdateMousePosition( int dx, int dy )
{
	int i;
	menucommon_t *menuitem;

	if( !m_active || !m_active->nitems )
		return;

	/*
	** check items
	*/
	m_cursoritem = NULL;
	for( i = 0; i < m_active->nitems; i++ )
	{
		menuitem = m_active->items[i];
		if( uis.cursorX > menuitem->maxs[0] ||
		    uis.cursorY > menuitem->maxs[1] ||
		    uis.cursorX < menuitem->mins[0] ||
		    uis.cursorY < menuitem->mins[1] )
			continue;

		m_cursoritem = m_active->items[i];

		if( m_active->cursor == i )
		{
			if( dy <= -1 || dy >= 1 )
				if( m_keypressed == K_MOUSE1 || m_keypressed == K_MOUSE2 )
					Menu_DragItem( m_active, dy, m_keypressed );
			break;
		}

		Menu_AdjustCursor( m_active, i - m_active->cursor );
		m_active->cursor = i;

		//trap_S_StartLocalSound( ( char * )menu_move_sound );
		break;
	}
}

/*
   =================
   UI_MouseMove
   =================
 */
void UI_MouseMove( int dx, int dy )
{
	if( uis.bind_grab )
		return; //don't move the mouse if we're grabbing binds

	uis.cursorX += dx;
	uis.cursorY += dy;

	clamp( uis.cursorX, 0, uis.vidWidth );
	clamp( uis.cursorY, 0, uis.vidHeight );

	if( dx || dy )
		UI_UpdateMousePosition( dx, dy );
}

/*
* UI_DrawConnectScreen
*/
void UI_DrawConnectScreen( const char *serverName, const char *rejectmessage, int downloadType, const char *downloadFilename,
						  float downloadPercent, int downloadSpeed, int connectCount, qboolean demoplaying, qboolean backGround )
{
	qboolean localhost, design = qfalse;
	char str[MAX_QPATH];
	int x, y, xoffset, yoffset, width, height;
	unsigned int maxwidth;
	qboolean downloadFromWeb;
	char hostName[MAX_CONFIGSTRING_CHARS], mapname[MAX_CONFIGSTRING_CHARS], mapmessage[MAX_CONFIGSTRING_CHARS],
		gametype[MAX_CONFIGSTRING_CHARS], gametypeTitle[MAX_CONFIGSTRING_CHARS],
		gametypeVersion[MAX_CONFIGSTRING_CHARS], gametypeAuthor[MAX_CONFIGSTRING_CHARS],
		matchName[MAX_CONFIGSTRING_CHARS];

	uis.demoplaying = demoplaying;

	//trap_S_StopBackgroundTrack();

	localhost = (qboolean)( !serverName || !serverName[0] || !Q_stricmp( serverName, "localhost" ) );

	trap_GetConfigString( CS_MAPNAME, mapname, sizeof( mapname ) );
	trap_GetConfigString( CS_MESSAGE, mapmessage, sizeof( mapmessage ) );

	if( backGround )
	{
		Q_snprintfz( str, sizeof( str ), UI_SHADER_BACKGROUND, uis.backgroundNum );
		trap_R_DrawStretchPic( 0, 0, uis.vidWidth, uis.vidHeight, 0, 0, 1, 1, colorWhite, trap_R_RegisterPic( str ) );
	}

	//
	// not yet connected
	//

	x = 64;
	y = 64;
	xoffset = yoffset = 0;

	if( demoplaying )
		Q_snprintfz( str, sizeof( str ), "Loading demo: %s", serverName );
	else if( localhost )
		Q_strncpyz( str, "Loading...", sizeof( str ) );
	else if( mapname[0] )
		Q_snprintfz( str, sizeof( str ), "Connecting to %s", serverName );
	else
		Q_snprintfz( str, sizeof( str ), "Awaiting connection... %i", connectCount );

	trap_SCR_DrawString( x + xoffset, y + yoffset, ALIGN_LEFT_TOP, str, uis.fontSystemBig, colorWhite );
	yoffset += trap_SCR_strHeight( uis.fontSystemBig );

	if( design && !rejectmessage )
		rejectmessage = "Connection was interrupted because the weather sux :P";

	if( rejectmessage )
	{
		x = uis.vidWidth / 2;
		y = uis.vidHeight / 3;

		height = trap_SCR_strHeight( uis.fontSystemMedium ) * 4;
		Q_strncpyz( str, "Refused: ", sizeof( str ) );
		width = max( trap_SCR_strWidth( str, uis.fontSystemMedium, 0 ), trap_SCR_strWidth( rejectmessage, uis.fontSystemSmall, 0 ) );
		width += 32 * 2;

		xoffset = UISCR_HorizontalAlignOffset( ALIGN_CENTER_MIDDLE, width );
		yoffset = UISCR_VerticalAlignOffset( ALIGN_CENTER_MIDDLE, height );

		UI_DrawBox( x + xoffset, y + yoffset, width, height, colorWarsowOrange, colorWhite, NULL, colorDkGrey );

		yoffset += trap_SCR_strHeight( uis.fontSystemMedium );
		xoffset += 32;

		trap_SCR_DrawString( x + xoffset, y + yoffset, ALIGN_LEFT_TOP, str, uis.fontSystemMedium, colorWhite );
		yoffset += trap_SCR_strHeight( uis.fontSystemMedium );

		trap_SCR_DrawString( x + xoffset, y + yoffset, ALIGN_LEFT_TOP, rejectmessage, uis.fontSystemSmall, colorBlack );

		return;
	}

	if( mapname[0] )
	{
		char levelshot[MAX_QPATH];
		struct shader_s *levelshotShader;
		qboolean isDefaultlevelshot = qtrue;

		//
		// connected
		//

		x = 64;
		y = uis.vidHeight - 300;
		xoffset = yoffset = 0;
		width = uis.vidWidth;
		height = 200;

		UI_DrawBox( x + xoffset, y + yoffset, width, height, colorWarsowPurple, colorWhite, NULL, colorDkGrey );
		xoffset += 16;
		yoffset += 16 + 4;

		maxwidth = uis.vidWidth - ( x + xoffset );

		trap_GetConfigString( CS_HOSTNAME, hostName, sizeof( hostName ) );
		trap_GetConfigString( CS_GAMETYPENAME, gametype, sizeof( gametype ) );
		trap_GetConfigString( CS_GAMETYPETITLE, gametypeTitle, sizeof( gametypeTitle ) );
		trap_GetConfigString( CS_GAMETYPEVERSION, gametypeVersion, sizeof( gametypeVersion ) );
		trap_GetConfigString( CS_GAMETYPEAUTHOR, gametypeAuthor, sizeof( gametypeAuthor ) );
		trap_GetConfigString( CS_MATCHNAME, matchName, sizeof( matchName ) );

		Q_snprintfz( levelshot, sizeof( levelshot ), "levelshots/%s.jpg", mapname );

		levelshotShader = trap_R_RegisterLevelshot( levelshot, uis.whiteShader, &isDefaultlevelshot );

		if( !isDefaultlevelshot )
		{
			int lw, lh, lx, ly;

			lh = height - 8;
			lw = lh * ( 4.0f/3.0f );
			lx = uis.vidWidth - lw;
			ly = y + 4;

			trap_R_DrawStretchPic( lx, ly, lw, lh, 0, 0, 1, 1, colorWhite, levelshotShader );
		}

		if( !localhost && !demoplaying )
		{
			Q_snprintfz( str, sizeof( str ), "Server: %s", hostName );
			trap_SCR_DrawStringWidth( x + xoffset, y + yoffset, ALIGN_LEFT_TOP, str, maxwidth, uis.fontSystemSmall, colorWhite );
			yoffset += trap_SCR_strHeight( uis.fontSystemSmall ) + 8;
		}

		if( mapmessage[0] && Q_stricmp( mapname, mapmessage ) )
		{
			Q_snprintfz( str, sizeof( str ), "Level: "S_COLOR_ORANGE"%s", COM_RemoveColorTokens( mapmessage ) );
			trap_SCR_DrawStringWidth( x + xoffset, y + yoffset, ALIGN_LEFT_TOP, str, maxwidth, uis.fontSystemSmall, colorWhite );
			yoffset += trap_SCR_strHeight( uis.fontSystemSmall ) + 8;
		}

		Q_snprintfz( str, sizeof( str ), "Map: %s", mapname );
		trap_SCR_DrawStringWidth( x + xoffset, y + yoffset, ALIGN_LEFT_TOP, str, maxwidth, uis.fontSystemSmall, colorWhite );
		yoffset += trap_SCR_strHeight( uis.fontSystemSmall ) + 8;

		if( matchName[0] )
		{
			Q_snprintfz( str, sizeof( str ), "Match: %s", matchName );
			trap_SCR_DrawStringWidth( x + xoffset, y + yoffset, ALIGN_LEFT_TOP, str, maxwidth, uis.fontSystemSmall, colorWhite );
			yoffset += trap_SCR_strHeight( uis.fontSystemSmall ) + 8;
		}
		yoffset += trap_SCR_strHeight( uis.fontSystemSmall ) + 8;

		Q_snprintfz( str, sizeof( str ), "Gametype: "S_COLOR_ORANGE"%s", COM_RemoveColorTokens( gametypeTitle ) );
		trap_SCR_DrawStringWidth( x + xoffset, y + yoffset, ALIGN_LEFT_TOP, str, maxwidth, uis.fontSystemSmall, colorWhite );
		yoffset += trap_SCR_strHeight( uis.fontSystemSmall ) + 8;

		Q_snprintfz( str, sizeof( str ), "Version: %s", COM_RemoveColorTokens( gametypeVersion ) );
		trap_SCR_DrawStringWidth( x + xoffset, y + yoffset, ALIGN_LEFT_TOP, str, maxwidth, uis.fontSystemSmall, colorWhite );
		yoffset += trap_SCR_strHeight( uis.fontSystemSmall ) + 8;

		Q_snprintfz( str, sizeof( str ), "Author: %s", gametypeAuthor );
		trap_SCR_DrawStringWidth( x + xoffset, y + yoffset, ALIGN_LEFT_TOP, str, maxwidth, uis.fontSystemSmall, colorWhite );
		yoffset += trap_SCR_strHeight( uis.fontSystemSmall ) + 8;
	}

	// downloading

	if( design && !downloadFilename )
	{
		downloadFilename = "http://www.warsow.net/autoupdate/basewsw/map_wdm9a.pk3";
		downloadType = 2;
		downloadPercent = 65.8;
		downloadSpeed = 325;
	}

	if( downloadType && downloadFilename )
	{
		size_t len;
		const char *s;

		downloadFromWeb = ( downloadType == 2 );

		x = uis.vidWidth / 2;
		y = uis.vidHeight / 3;
		width = 400;
		height = 128 - trap_SCR_strHeight( uis.fontSystemSmall );
		if( uis.vidWidth <= width )
			width = uis.vidWidth - 64;

		maxwidth = width - 48;

		xoffset = UISCR_HorizontalAlignOffset( ALIGN_CENTER_MIDDLE, width );
		yoffset = UISCR_VerticalAlignOffset( ALIGN_CENTER_MIDDLE, height );

		// adjust the box size for the extra number of lines needed to draw the file path
		s = downloadFilename;
		while( ( len = trap_SCR_StrlenForWidth( s, uis.fontSystemSmall, maxwidth ) ) > 0 )
		{
			s += len;
			height += trap_SCR_strHeight( uis.fontSystemSmall );
		}

		UI_DrawBox( x + xoffset, y + yoffset, width, height, colorWarsowPurple, colorWhite, NULL, colorDkGrey );

		xoffset += 24;
		yoffset += 24;

		trap_SCR_DrawStringWidth( x + xoffset, y + yoffset, ALIGN_LEFT_TOP, downloadFromWeb ? "Downloading from web" : "Downloading from server", maxwidth, uis.fontSystemSmall, colorWhite );
		yoffset += trap_SCR_strHeight( uis.fontSystemSmall );

		s = downloadFilename;
		while( ( len = trap_SCR_DrawStringWidth( x + xoffset, y + yoffset, ALIGN_LEFT_TOP, s, maxwidth, uis.fontSystemSmall, colorWhite ) ) > 0 )
		{
			s += len;
			yoffset += trap_SCR_strHeight( uis.fontSystemSmall );
		}

		yoffset += 16;

		UI_DrawPicBar( x + xoffset, y + yoffset, maxwidth, 24, ALIGN_LEFT_TOP, downloadPercent, trap_R_RegisterPic( "gfx/ui/progressbar" ),colorDkGrey, colorOrange );
		Q_snprintfz( str, sizeof( str ), "%3.1f%c", downloadPercent, '%' );
		trap_SCR_DrawStringWidth( x + xoffset + 12, y + yoffset + 12, ALIGN_LEFT_MIDDLE, str, maxwidth, uis.fontSystemSmall, colorWhite );
		Q_snprintfz( str, sizeof( str ), "%ik/s", downloadSpeed );
		trap_SCR_DrawStringWidth( x + xoffset + maxwidth - 12, y + yoffset + 12, ALIGN_RIGHT_MIDDLE, str, maxwidth, uis.fontSystemSmall, colorWhite );

		yoffset += 24 + 8;
	}
}

/*
* UI_Refresh
*/
void UI_Refresh( unsigned int time, int clientState, int serverState, qboolean demoplaying, qboolean backGround )
{
	uis.frameTime = ( time - uis.time ) * 0.001f;
	uis.time = time;
	uis.clientState = clientState;
	uis.serverState = serverState;
	uis.backGround = backGround;
	uis.demoplaying = demoplaying;

	if( !m_drawfunc )  // ui is inactive
		return;

	// draw background
	if( uis.backGround )
	{
		trap_R_DrawStretchPic( 0, 0, uis.vidWidth, uis.vidHeight,
		                      0, 0, 1, 1, colorWhite, trap_R_RegisterPic( UI_SHADER_VIDEOBACK ) );

		trap_R_DrawStretchPic( 0, 0, uis.vidWidth, uis.vidHeight,
		                      0, 0, 1, 1, colorWhite, trap_R_RegisterPic( UI_SHADER_FXBACK ) );

		trap_R_DrawStretchPic( 0, 0, uis.vidWidth, uis.vidHeight,
		                      0, 0, 1, 1, colorWhite, trap_R_RegisterPic( UI_SHADER_BIGLOGO ) );

		if( !uis.backGroundTrackStarted )
		{
			trap_S_StartBackgroundTrack( S_PLAYLIST_MENU, "3" ); // shuffle and loop
			uis.backGroundTrackStarted = qtrue;
		}
	}
	else
	{
		uis.backGroundTrackStarted = qfalse;
		//trap_R_DrawStretchPic( 0, 0, uis.vidWidth, uis.vidHeight,
		//	0, 0, 1, 1, colorDkGrey, trap_R_RegisterPic( "gfx/ui/novideoback" ) );

		//trap_R_DrawStretchPic ( 0, 0, uis.vidWidth, uis.vidHeight,
		//	0, 0, 1, 1, colorWhite, trap_R_RegisterPic( UI_SHADER_BIGLOGO ) );
	}

	m_drawfunc();

	// draw cursor
	if( !uis.bind_grab )
		trap_R_DrawStretchPic( uis.cursorX - 16, uis.cursorY - 16, 32, 32,
		                      0, 0, 1, 1, colorWhite, trap_R_RegisterPic( UI_SHADER_CURSOR ) );

	// delay playing the enter sound until after the
	// menu has been drawn, to avoid delay while
	// caching images
	if( m_entersound )
	{
		trap_S_StartLocalSound( menu_in_sound );
		m_entersound = qfalse;
	}
}

/*
   =================
   UI_Keydown
   =================
 */
void UI_Keydown( int key )
{
	const char *s;

	m_keypressed = key;

	if( m_keyfunc )
		if( ( s = m_keyfunc( key ) ) != 0 )
			trap_S_StartLocalSound( s );
}

/*
   =================
   UI_Keyup
   =================
 */
void UI_Keyup( int key )
{
	m_keypressed = 0;
}

/*
   =================
   UI_CharEvent
   =================
 */
void UI_CharEvent( qwchar key )
{
	const char *s;

	if( m_chareventfunc )
		if( ( s = m_chareventfunc( key ) ) != 0 )
			trap_S_StartLocalSound( s );
}

//======================================================================

#ifndef UI_HARD_LINKED
// this is only here so the functions in q_shared.c and q_math.c can link
void Sys_Error( const char *format, ... )
{
	va_list	argptr;
	char msg[1024];

	va_start( argptr, format );
	Q_vsnprintfz( msg, sizeof( msg ), format, argptr );
	va_end( argptr );

	trap_Error( msg );
}

void Com_Printf( const char *fmt, ... )
{
	va_list	argptr;
	char text[1024];

	va_start( argptr, fmt );
	vsnprintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );
	text[sizeof( text )-1] = 0;

	trap_Print( text );
}
#endif
