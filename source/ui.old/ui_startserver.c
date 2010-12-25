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
   =============================================================================

   START SERVER MENU

   =============================================================================
 */
static menuframework_s s_startserver_menu;

static void *s_levelshot;
static menucommon_t *m_gametypes_item;

m_itemslisthead_t mapList;
static int mapList_selpos;
static int mapList_viewable_items;
static int mapList_cur_idx = 0;
static int mapList_suggested_gametype;
static char mapList_gamemap_statusbar[128];

static char **startserver_gametype_names;
cvar_t *ui_gametype_names;

#define MAPPIC_HEIGHT 180
#define MAPPIC_WIDTH  MAPPIC_HEIGHT * ( 4.0f/3.0f )

#define RCOLUMN_OFFSET  16
#define LCOLUMN_OFFSET -16

static int m_gametype;
static int m_skill;
static int m_cheats;
static int m_public;
static int m_instagib;

static void M_GametypeFunc( menucommon_t *menuitem )
{
	m_gametype = menuitem->curvalue;
}
static void M_SkillLevelFunc( menucommon_t *menuitem )
{
	m_skill = menuitem->curvalue;
}
static void M_CheatsFunc( menucommon_t *menuitem )
{
	m_cheats = menuitem->curvalue;
}
static void M_InstagibFunc( menucommon_t *menuitem )
{
	m_instagib = menuitem->curvalue;
}
static void M_PublicFunc( menucommon_t *menuitem )
{
	m_public = menuitem->curvalue;
}

//#define SUGGEST_MAP_GAMETYPE

static int SuggestGameType( const char *mapname )
{
	int i;
	const char *gametype;

	gametype = trap_Cvar_String( "g_gametype" );

	Q_strncpyz( mapList_gamemap_statusbar, "select the initial map", sizeof( mapList_gamemap_statusbar ) );

#ifdef SUGGEST_MAP_GAMETYPE
	if( mapname )
	{
		i = 0;

		while( startserver_gametype_names && startserver_gametype_names[i] != 0 )
		{
			if( !strncmp( mapname + 1, startserver_gametype_names[i], strlen( startserver_gametype_names[i] ) ) )
			{
				Q_strncatz( mapList_gamemap_statusbar, va( " (suggested gametype: %s)", startserver_gametype_names[i] ), sizeof( mapList_gamemap_statusbar ) );
				//return i;
				break;
			}
			i++;
		}
	}
#endif

	i = 0;
	while( startserver_gametype_names && startserver_gametype_names[i] != 0 )
	{
		if( !Q_stricmp( gametype, startserver_gametype_names[i] ) )
			return i;
		i++;
	}

	return 0;
}


typedef struct
{
	char *name;
	char *mapname;
	int index;
} maplist_item_t;

static int MapList_MapSort( const maplist_item_t *a, const maplist_item_t *b )
{
	return Q_stricmp( a->name, b->name );
}

static void MapsList_CreateItems( const char *lastmap )
{
	int i, j, validmaps;
	int order;
	maplist_item_t *items;
	char mapinfo[MAX_CONFIGSTRING_CHARS], *mapname, *fullname;

	UI_FreeScrollItemList( &mapList );

	for( validmaps = 0; trap_ML_GetMapByNum( validmaps, NULL, 0 ); validmaps++ )
		;

	if( !validmaps )
	{
		Menu_SetStatusBar( &s_startserver_menu, "No maps found" );
		return;
	}

	order = trap_Cvar_Value( "ui_maplist_sortmethod" );
	items = UI_Malloc( sizeof( *items ) * validmaps );
	for( i = 0; i < validmaps; i++ )
	{
		trap_ML_GetMapByNum( i, mapinfo, sizeof( mapinfo ) );

		mapname = mapinfo;
		fullname = mapinfo + strlen( mapname ) + 1;

		if( order )
			items[i].name = UI_CopyString( *fullname ? fullname : mapname );
		else
			items[i].name = UI_CopyString( mapname );
		items[i].mapname = UI_CopyString( mapname );
		items[i].index = i;

		// uppercase the first letter and letters that follow whitespaces
		if( order )
		{
			if( *fullname )
			{
				items[i].name[0] = toupper( items[i].name[0] );
				for( j = 1; items[i].name[j]; j++ )
					if( items[i].name[j-1] == ' ' )
						items[i].name[j] = toupper( items[i].name[j] );
			}
			else
			{
				Q_strlwr( items[i].name );
			}
		}
		else
		{
			Q_strlwr( items[i].name );
		}
	}

	qsort( items, validmaps, sizeof( *items ), ( int ( * )( const void *, const void * ) )MapList_MapSort );

	for( i = 0; i < validmaps; i++ )
	{
		if( !strcmp( items[i].mapname, lastmap ) )
			mapList_cur_idx = i;

		UI_AddItemToScrollList( &mapList, items[i].name, (void *)((size_t)(items[i].index)) );
		UI_Free( items[i].mapname );
		UI_Free( items[i].name );
	}

	UI_Free( items );
}

static void MapsList_UpdateButton( menucommon_t *menuitem )
{
	m_listitem_t *item;

	menuitem->localdata[1] = menuitem->localdata[0] + mapList_selpos;
	item = UI_FindItemInScrollListWithId( &mapList, menuitem->localdata[1] );
	if( item )
	{
		if( menuitem->localdata[1] == mapList_cur_idx )
			Q_snprintfz( menuitem->title, MAX_STRING_CHARS, "%s%s",
			             S_COLOR_RED, item->name );
		else
			Q_snprintfz( menuitem->title, MAX_STRING_CHARS, item->name );
	}
	else
		Q_snprintfz( menuitem->title, MAX_STRING_CHARS, "" );
}

static void MapsList_UpdateScrollbar( menucommon_t *menuitem )
{
	menuitem->maxvalue = max( 0, mapList.numItems - mapList_viewable_items );
	trap_Cvar_SetValue( menuitem->title, menuitem->curvalue );
	mapList_selpos = menuitem->curvalue;
}

static void MapsList_ChooseMap( menucommon_t *menuitem )
{
	char path[MAX_CONFIGSTRING_CHARS + 6]; // wsw: Medar: could do this in not so ugly way
	m_listitem_t *item;
	menucommon_t *mapitem;
	char mapinfo[MAX_CONFIGSTRING_CHARS];
	const char *mapname, *fullname;
	int id = ( menuitem ? menuitem->localdata[1] : mapList_cur_idx );

	mapitem = UI_MenuItemByName( "m_startserver_map" );
	if( mapitem )
		Q_strncpyz( mapitem->title, "initial map", sizeof( mapitem->title ) );

	mapList_suggested_gametype = 0;

	item = UI_FindItemInScrollListWithId( &mapList, id );
	if( item && item->name )
	{
		if( !trap_ML_GetMapByNum( (int)((size_t)item->data), mapinfo, sizeof( mapinfo ) ) )
			return;
		mapname = mapinfo;
		fullname = mapinfo + strlen( mapname ) + 1;

		if( menuitem )
		{
			mapList_cur_idx = id;
			trap_Cvar_ForceSet( "ui_startserver_lastselectedmap", "" );
		}

		if( mapitem )
		{
			Q_strncatz( mapitem->title, ": " S_COLOR_WHITE, sizeof( mapitem->title ) );
			if( !trap_Cvar_Value( "ui_maplist_sortmethod" ) )
				Q_strncatz( mapitem->title, mapname, sizeof( mapitem->title ) );
			else
				Q_strncatz( mapitem->title, *fullname ? fullname : mapname, sizeof( mapitem->title ) );
		}

#ifdef SUGGEST_MAP_GAMETYPE
		mapList_suggested_gametype = SuggestGameType( mapname );
//		if( m_gametypes_item )
//		{
//			m_gametypes_item->curvalue = mapList_suggested_gametype;
//			M_GametypeFunc( m_gametypes_item );
//		}
#endif

		Q_snprintfz( path, sizeof( path ), "levelshots/%s.jpg", mapname );
		s_levelshot = trap_R_RegisterLevelshot( path, trap_R_RegisterPic( PATH_UKNOWN_MAP_PIC ), NULL );
	}
}

static void StartServerActionFunc( menucommon_t *unused );

static int MapsList_CreateScrollbox( int parent_width, int yoffset )
{
	int i;
	menucommon_t *menuitem;
	int width, scrollbar_id;
	const int lineheight = trap_SCR_strHeight( uis.fontSystemSmall );
	
	width = parent_width / 2 - 8;
	mapList_viewable_items = MAPPIC_HEIGHT / lineheight - 1;

	menuitem = UI_InitMenuItem( "m_mapList_scrollbar", NULL, 0, yoffset,
	                            MTYPE_SCROLLBAR, ALIGN_RIGHT_TOP,
	                            uis.fontSystemSmall, MapsList_UpdateScrollbar );
	menuitem->scrollbar_id = scrollbar_id = s_startserver_menu.nitems;
	Q_strncpyz( menuitem->title, va( "m_mapList_scrollbar%i_curvalue", scrollbar_id ), sizeof( menuitem->title ) );
	UI_SetupScrollbar( menuitem, mapList_viewable_items, trap_Cvar_Value( menuitem->title ), 0, 0 );
	Menu_AddItem( &s_startserver_menu, menuitem );

	for( i = 0; i < mapList_viewable_items; i++ )
	{
		menuitem = UI_InitMenuItem( va( "m_maps_button_%i", i ), "",
		                            -width, yoffset,
		                            MTYPE_ACTION, ALIGN_LEFT_TOP,
		                            uis.fontSystemSmall, MapsList_ChooseMap );
		menuitem->callback_doubleclick = StartServerActionFunc;
		menuitem->ownerdraw = MapsList_UpdateButton;
		menuitem->scrollbar_id = scrollbar_id;
		menuitem->localdata[0] = i;
		menuitem->localdata[1] = i;
		menuitem->width = width;
		menuitem->statusbar = mapList_gamemap_statusbar;

		menuitem->pict.shader = uis.whiteShader;
		menuitem->pict.shaderHigh = uis.whiteShader;

		Vector4Copy( colorWhite, menuitem->pict.colorHigh );
		Vector4Copy( ( i & 1 ) ? colorDkGrey : colorMdGrey, menuitem->pict.color );
		menuitem->pict.color[3] = menuitem->pict.colorHigh[3] = 0.65f;
		menuitem->pict.yoffset = 0;
		menuitem->pict.xoffset = 0;
		menuitem->pict.width = width;
		menuitem->pict.height = lineheight;

		Menu_AddItem( &s_startserver_menu, menuitem );

		yoffset += lineheight;
	}

	yoffset += lineheight;

	return yoffset;
}

static void StartServerActionFunc( menucommon_t *unused )
{
	char *str;
	char mapname[MAX_CONFIGSTRING_CHARS];
	char starservercmd[MAX_STRING_CHARS];

	m_listitem_t *mapitem;
	mapitem = UI_FindItemInScrollListWithId( &mapList, mapList_cur_idx );
	if( !mapitem || !mapitem->name )
		return;

	trap_Cvar_Set( "g_gametype", startserver_gametype_names[m_gametype] );
	trap_Cvar_SetValue( "sv_skilllevel", m_skill );
	trap_Cvar_SetValue( "sv_cheats", m_cheats );
	trap_Cvar_SetValue( "sv_public", m_public );
	trap_Cvar_SetValue( "rs_mysqlenabled", 0 ); //racesow: only dedicated servers use mysql ;)

	str = UI_GetMenuitemFieldBuffer( UI_MenuItemByName( "m_startserver_hostname" ) );
	if( str ) trap_Cvar_Set( "sv_hostname", str );

	str = UI_GetMenuitemFieldBuffer( UI_MenuItemByName( "m_startserver_maxplayers" ) );
	if( str ) trap_Cvar_Set( "sv_maxclients", str );

	// game stuff, overriding local gametype config
	starservercmd[0] = '\0';
	Q_strncatz( starservercmd, va( "g_instagib %i;", m_instagib ), sizeof( starservercmd ) );
	trap_Cvar_SetValue( "g_instagib", (float)m_instagib );

	str = UI_GetMenuitemFieldBuffer( UI_MenuItemByName( "m_startserver_timelimit" ) );
	if( str )
	{
		Q_strncatz( starservercmd, va( "g_timelimit %s;", str ), sizeof( starservercmd ) );
		trap_Cvar_Set( "g_timelimit", str );
	}

	str = UI_GetMenuitemFieldBuffer( UI_MenuItemByName( "m_startserver_scorelimit" ) );
	if( str ) 
	{
		Q_strncatz( starservercmd, va( "g_scorelimit %s;", str ), sizeof( starservercmd ) );
		trap_Cvar_Set( "g_scorelimit", str );
	}

	str = UI_GetMenuitemFieldBuffer( UI_MenuItemByName( "m_startserver_numbots" ) );
	if( str ) 
	{
		Q_strncatz( starservercmd, va( "g_numbots %s;", str ), sizeof( starservercmd ) );
		trap_Cvar_Set( "g_numbots", str );
	}

	trap_Cvar_ForceSet( "ui_startservercmd", starservercmd );

	if( uis.serverState )
		trap_Cmd_ExecuteText( EXEC_APPEND, "disconnect\n" );

	if( trap_ML_GetMapByNum( (int)((size_t)mapitem->data), mapname, sizeof( mapname ) ) )
		trap_Cvar_ForceSet( "ui_startserver_lastselectedmap", mapname );

	trap_Cmd_ExecuteText( EXEC_APPEND, va( "map \"%s\"\n", mapitem->name ) );
}

static void StartServer_UpdateOrderMethod( menucommon_t *menuitem )
{
	char mapinfo[MAX_CONFIGSTRING_CHARS] = { 0 };
	m_listitem_t *mapitem;

	trap_Cvar_SetValue( "ui_maplist_sortmethod", menuitem->curvalue );

	mapitem = UI_FindItemInScrollListWithId( &mapList, mapList_cur_idx );
	if( mapitem && mapitem->name )
		trap_ML_GetMapByNum( (int)((size_t)mapitem->data), mapinfo, sizeof( mapinfo ) );

	MapsList_CreateItems( mapinfo );
	MapsList_ChooseMap( NULL );

	menuitem = UI_MenuItemByName( "m_mapList_scrollbar" );
	menuitem->curvalue = bound( menuitem->minvalue, mapList_cur_idx, menuitem->maxvalue );
	MapsList_UpdateScrollbar( menuitem );
}

void M_StartServer_FreeGametypesNames( void )
{
	int i;

	if( !startserver_gametype_names )
		return;

	for( i = 0; startserver_gametype_names[i] != NULL; i++ )
	{
		if( startserver_gametype_names[i] )
		{
			UI_Free( startserver_gametype_names[i] );
			startserver_gametype_names[i] = NULL;
		}
	}

	UI_Free( startserver_gametype_names );
	startserver_gametype_names = NULL;
}

void M_StartServer_MakeGametypesNames( char *list )
{
	char *s, **ptr;
	int count, i;
	size_t size;

	M_StartServer_FreeGametypesNames();

	for( count = 0; UI_ListNameForPosition( list, count, ';' ) != NULL; count++ );

	startserver_gametype_names = UI_Malloc( sizeof( ptr ) * ( count + 1 ) );

	for( i = 0; i < count; i++ )
	{
		s = UI_ListNameForPosition( list, i, ';' );

		size = strlen( s ) + 1;
		startserver_gametype_names[i] = UI_Malloc( size );
		Q_strncpyz( startserver_gametype_names[i], s, size );
	}

	startserver_gametype_names[i] = NULL;
}

//==================
//M_StartServer_DrawSettingsBox
//==================
static void M_StartServer_DrawSettingsBox( menucommon_t *menuitem )
{
	int x, y;
	int width, height;

	x = menuitem->x + menuitem->parent->x;
	y = menuitem->y + menuitem->parent->y - trap_SCR_strHeight( menuitem->font );

	width = menuitem->pict.width;
	height = menuitem->pict.height + trap_SCR_strHeight( menuitem->font ) + 4;

	UI_DrawBox( x, y, width, height, colorWarsowPurple, colorWhite, NULL, colorDkGrey );
	UI_DrawString( x + 24, y + 10, ALIGN_LEFT_TOP, "Settings", 0, menuitem->font, colorWhite );
}

static qboolean StartServer_MenuInit( void )
{
	menucommon_t *menuitem_settings_background;
	menucommon_t *menuitem, *col_title;
	static char *skill_names[] = { "easy", "normal", "hard", 0 };
	static char *sortmethod_names[] = { "file name", "title", 0 };
	cvar_t *cvar_lastmap;
	int maxclients;
	int scrollwindow_width, xoffset, yoffset = 0; //leave some room for preview pic

	trap_Cvar_Get( "ui_maplist_sortmethod", "1", CVAR_ARCHIVE );

	// create a list with the installed gametype names
	ui_gametype_names = trap_Cvar_Get( "ui_gametype_names", ";", CVAR_NOSET );
	if( !UI_CreateFileNamesListCvar( ui_gametype_names, "progs/gametypes", ".gt", ';' ) )
		trap_Cvar_ForceSet( "ui_gametype_names", "dm;" );

	if( uis.vidWidth < 800 )
		scrollwindow_width = uis.vidWidth * 0.85;
	else if( uis.vidWidth < 1024 )
		scrollwindow_width = uis.vidWidth * 0.75;
	else
		scrollwindow_width = uis.vidWidth * 0.45;
	xoffset = scrollwindow_width / 2;

	// convert to item names format
	M_StartServer_MakeGametypesNames( ui_gametype_names->string );

	s_startserver_menu.nitems = 0;

	menuitem = UI_InitMenuItem( "m_startserver_title1", "SERVER SETUP", 0, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemBig, NULL );
	Menu_AddItem( &s_startserver_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	// separator
	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_startserver_map", "initial map", 0, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemMedium, NULL );
	Menu_AddItem( &s_startserver_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	yoffset += trap_SCR_strHeight( menuitem->font ) * 0.5;

	// order type
	menuitem = col_title = UI_InitMenuItem( "m_startserver_order_title", "order by: ", -xoffset - LCOLUMN_OFFSET / 2, yoffset, MTYPE_SEPARATOR, ALIGN_LEFT_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_startserver_menu, menuitem );

	menuitem = UI_InitMenuItem( "m_startserver_order", NULL, col_title->x + trap_SCR_strWidth( col_title->title, uis.fontSystemSmall, 0 ),
		yoffset, MTYPE_SPINCONTROL, ALIGN_LEFT_TOP, uis.fontSystemSmall, StartServer_UpdateOrderMethod );
	UI_SetupSpinControl( menuitem, sortmethod_names, trap_Cvar_Value( "ui_maplist_sortmethod" ) );
	Menu_AddItem( &s_startserver_menu, menuitem );

	menuitem = UI_InitMenuItem( "m_startserver_mappic", NULL, xoffset - MAPPIC_WIDTH - 8, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_startserver_menu, menuitem );

	cvar_lastmap = trap_Cvar_Get( "ui_startserver_lastselectedmap", "", CVAR_NOSET );
	MapsList_CreateItems( cvar_lastmap->string );
	MapsList_ChooseMap( NULL );

	yoffset += trap_SCR_strHeight( menuitem->font );
	yoffset = MapsList_CreateScrollbox( scrollwindow_width, yoffset );

	yoffset += trap_SCR_strHeight( menuitem->font ) * 0.5;

	menuitem_settings_background = UI_InitMenuItem( "m_startserver_settings_back", "", -xoffset, yoffset, MTYPE_SEPARATOR, ALIGN_LEFT_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_startserver_menu, menuitem_settings_background );
	// create an associated picture to the items to act as window background
	menuitem = menuitem_settings_background;
	menuitem->ownerdraw = M_StartServer_DrawSettingsBox;
	menuitem->pict.shader = uis.whiteShader;
	menuitem->pict.shaderHigh = NULL;
	Vector4Copy( colorMdGrey, menuitem->pict.color );
	menuitem->pict.color[3] = 0;
	menuitem->pict.yoffset = 0;
	menuitem->pict.xoffset = 0;
	menuitem->pict.width = scrollwindow_width;
	menuitem->pict.height = yoffset + menuitem->pict.yoffset; // will be set later

	yoffset += trap_SCR_strHeight( menuitem->font );

	// g_gametype
	m_gametype = mapList_suggested_gametype ? mapList_suggested_gametype : SuggestGameType( NULL );
	menuitem = m_gametypes_item = UI_InitMenuItem( "m_startserver_gametype", "gametype", -130, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, M_GametypeFunc );
	menuitem->statusbar = "select the server gametype";
	UI_SetupSpinControl( menuitem, startserver_gametype_names, m_gametype );
	Menu_AddItem( &s_startserver_menu, menuitem );
	//yoffset += trap_SCR_strHeight( menuitem->font );

	// g_timelimit
	menuitem = UI_InitMenuItem( "m_startserver_timelimit", "time limit", 100, yoffset, MTYPE_FIELD, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	menuitem->statusbar = "0 = no limit";
	UI_SetupField( menuitem, trap_Cvar_String( "g_timelimit" ), 6, -1 );
	UI_SetupFlags( menuitem, F_NUMBERSONLY );
	Menu_AddItem( &s_startserver_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	// sv_skilllevel
	menuitem = UI_InitMenuItem( "m_startserver_skill", "skill level", -130, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, M_SkillLevelFunc );
	m_skill = trap_Cvar_Value( "sv_skilllevel" );
	menuitem->statusbar = "select server skill level";
	UI_SetupSpinControl( menuitem, skill_names, m_skill );
	Menu_AddItem( &s_startserver_menu, menuitem );
	//yoffset += trap_SCR_strHeight( menuitem->font );

	// g_scorelimit
	menuitem = UI_InitMenuItem( "m_startserver_scorelimit", "score limit", 100, yoffset, MTYPE_FIELD, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	menuitem->statusbar = "0 = no limit";
	UI_SetupField( menuitem, trap_Cvar_String( "g_scorelimit" ), 6, -1 );
	UI_SetupFlags( menuitem, F_NUMBERSONLY );
	Menu_AddItem( &s_startserver_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	// sv_cheats
	menuitem = UI_InitMenuItem( "m_startserver_cheats", "cheats", -130, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, M_CheatsFunc );
	m_cheats = trap_Cvar_Value( "sv_cheats" );
	menuitem->statusbar = "enable cheats on the server";
	UI_SetupSpinControl( menuitem, offon_names, m_cheats );
	Menu_AddItem( &s_startserver_menu, menuitem );
	//yoffset += trap_SCR_strHeight( menuitem->font );

	// g_numbots
	menuitem = UI_InitMenuItem( "m_startserver_numbots", "number of bots", 100, yoffset, MTYPE_FIELD, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	menuitem->statusbar = "Can't be more than maxclients";
	UI_SetupField( menuitem, trap_Cvar_String( "g_numbots" ), 6, -1 );
	UI_SetupFlags( menuitem, F_NUMBERSONLY );
	Menu_AddItem( &s_startserver_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	// g_instagib
	menuitem = UI_InitMenuItem( "m_startserver_instagib", "instagib", -130, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, M_InstagibFunc );
	m_instagib = trap_Cvar_Value( "g_instagib" );
	menuitem->statusbar = "enable instagib mode";
	UI_SetupSpinControl( menuitem, offon_names, m_instagib );
	Menu_AddItem( &s_startserver_menu, menuitem );
	//yoffset += trap_SCR_strHeight( menuitem->font );

	// sv_maxclients
	/*
	** maxclients determines the maximum number of players that can join
	** the game.  If maxclients is only "1" then we should default the menu
	** option to 8 players, otherwise use whatever its current value is.
	*/
	maxclients = trap_Cvar_Value( "sv_maxclients" ) <= 1 ? 8 : trap_Cvar_Value( "sv_maxclients" );
	menuitem = UI_InitMenuItem( "m_startserver_maxplayers", "max players", 100, yoffset, MTYPE_FIELD, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	UI_SetupField( menuitem, va( "%i", maxclients ), 6, -1 );
	UI_SetupFlags( menuitem, F_NUMBERSONLY );
	Menu_AddItem( &s_startserver_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	// sv_public
	menuitem = UI_InitMenuItem( "m_startserver_public", "public", -130, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, M_PublicFunc );
	m_public = trap_Cvar_Value( "sv_public" );
	menuitem->statusbar = "announce this server to metaservers";
	UI_SetupSpinControl( menuitem, offon_names, m_public );
	Menu_AddItem( &s_startserver_menu, menuitem );
	//yoffset += trap_SCR_strHeight( menuitem->font );

	// sv_hostname
	menuitem = UI_InitMenuItem( "m_startserver_hostname", "server name", 100, yoffset, MTYPE_FIELD, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	UI_SetupField( menuitem, trap_Cvar_String( "sv_hostname" ), 14, -1 );
	Menu_AddItem( &s_startserver_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	yoffset += trap_SCR_strHeight( menuitem->font ) * 0.5;

	// here ends the settings background, set it's image height now
	menuitem_settings_background->pict.height = yoffset - menuitem_settings_background->pict.height + ( 0.5 * trap_SCR_strHeight( menuitem->font ) );

	yoffset += trap_SCR_strHeight( menuitem->font );

	// begin button
	menuitem = UI_InitMenuItem( "m_startserver_begin", "begin", 16, yoffset, MTYPE_ACTION, ALIGN_LEFT_TOP, uis.fontSystemBig, StartServerActionFunc );
	Menu_AddItem( &s_startserver_menu, menuitem );
	UI_SetupButton( menuitem, qtrue );

	menuitem = UI_InitMenuItem( "m_startserver_back", "back", -16, yoffset, MTYPE_ACTION, ALIGN_RIGHT_TOP, uis.fontSystemBig, M_genericBackFunc );
	Menu_AddItem( &s_startserver_menu, menuitem );
	yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;

	Menu_Center( &s_startserver_menu );
	s_startserver_menu.x = ( uis.vidWidth / 2 );
	Menu_Init( &s_startserver_menu, qfalse );
	return qtrue;
}

static void StartServer_MenuDraw( void )
{
	int x, y;
	menucommon_t *item = UI_MenuItemByName( "m_startserver_mappic" );

	x = item->parent->x + item->x;
	y = item->parent->y + item->y;

	Menu_Draw( &s_startserver_menu );

	trap_R_DrawStretchPic( x, y, MAPPIC_WIDTH, MAPPIC_HEIGHT,
	                       0, 0, 1, 1, colorWhite, (struct shader_s *)s_levelshot );
}

static const char *StartServer_MenuKey( int key )
{
	menucommon_t *item;

	item = Menu_ItemAtCursor( &s_startserver_menu );

	if( key == K_ESCAPE || ( ( key == K_MOUSE2 ) && ( item->type != MTYPE_SPINCONTROL ) &&
	                        ( item->type != MTYPE_SLIDER ) ) )
	{
		UI_FreeScrollItemList( &mapList );
	}

	return Default_MenuKey( &s_startserver_menu, key );
}

static const char *StartServer_MenuCharEvent( qwchar key )
{
	return Default_MenuCharEvent( &s_startserver_menu, key );
}

void M_Menu_StartServer_f( void )
{
	if( !StartServer_MenuInit() )
		return;

	Menu_SetStatusBar( &s_startserver_menu, NULL );
	M_PushMenu( &s_startserver_menu, StartServer_MenuDraw, StartServer_MenuKey, StartServer_MenuCharEvent );
}
