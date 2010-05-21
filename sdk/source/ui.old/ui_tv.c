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

TV MENU

=======================================================================
*/

#define REFRESH_DELAY	5000

#define	NO_CHANNEL_STRING   ""

static menuframework_s s_tv_menu;

static void M_TV_RefreshFunc( menucommon_t *menuitem )
{
static unsigned int lastRefresh = 0;

	if( lastRefresh >= uis.time )
		return;
	lastRefresh = uis.time + REFRESH_DELAY;

	trap_Cmd_ExecuteText( EXEC_APPEND, "cmd channels\n" );
}

static void M_TV_MenuMainFunc( menucommon_t *menuitem )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, "menu_main" );
}

// -----

typedef struct tv_channel_s tv_channel_t;

struct tv_channel_s
{
	int id;
	char name[100];
	char realname[100];
	int numplayers, numspecs;
	char gametype[MAX_CONFIGSTRING_CHARS];
	char mapname[MAX_CONFIGSTRING_CHARS];
	char matchname[MAX_CONFIGSTRING_CHARS];
	char address[23];
	tv_channel_t *next;
};

static tv_channel_t *channels = NULL;

m_itemslisthead_t channelsScrollList;
static int scrollbar_curvalue = 0;
static int max_menu_channels = 0;

//==================
//M_TV_JoinChannel
//==================
static void M_TV_JoinChannel( menucommon_t *menuitem )
{
	char buffer[128];
	tv_channel_t *channel = NULL;
	m_listitem_t *listitem;

	menuitem->localdata[1] = menuitem->localdata[0] + scrollbar_curvalue;
	listitem = UI_FindItemInScrollListWithId( &channelsScrollList, menuitem->localdata[1] );
	if( listitem && listitem->data )
		channel = (tv_channel_t *)listitem->data;
	if( channel )
	{
		Q_snprintfz( buffer, sizeof( buffer ), "cmd watch %i\n", channel->id );
		trap_Cmd_ExecuteText( EXEC_APPEND, buffer );
	}
}

//==================
//M_RefreshScrollWindowList
//==================
static void M_RefreshScrollWindowList( void )
{
	int addedItems;
	tv_channel_t *iter;

	UI_FreeScrollItemList( &channelsScrollList );

	addedItems = 0;
	iter = channels;
	while( iter )
	{
		UI_AddItemToScrollList( &channelsScrollList, va( "%i", addedItems++ ), (void *)iter );
		iter = iter->next;
	}
}

//==================
//M_TV_UpdateScrollbar
//==================
static void M_TV_UpdateScrollbar( menucommon_t *menuitem )
{
	menuitem->maxvalue = max( 0, channelsScrollList.numItems - max_menu_channels );
	if( menuitem->curvalue > menuitem->maxvalue )  //if filters shrunk the list size, shrink the scrollbar and its value
		menuitem->curvalue = menuitem->maxvalue;
	trap_Cvar_SetValue( menuitem->title, menuitem->curvalue );
	scrollbar_curvalue = menuitem->curvalue;
}

#define COLUMN_WIDTH_NO			25
#define COLUMN_WIDTH_NAME		205
#define COLUMN_WIDTH_PLAYERS	75
#define COLUMN_WIDTH_GAMETYPE	95
#define COLUMN_WIDTH_MAPNAME	105
#define COLUMN_WIDTH_MATCHNAME	0

#define ROW_PATTERN		"\\w:%i\\%s\\w:%i\\%s%s\\w:%i\\%s%s\\w:%i\\%s%s\\w:%i\\%s%s\\w:%i\\%s%s"

//==================
//M_UpdateChannelButton
//==================
static void M_UpdateChannelButton( menucommon_t *menuitem )
{
	tv_channel_t *channel = NULL;
	m_listitem_t *listitem;

	menuitem->localdata[1] = menuitem->localdata[0] + scrollbar_curvalue;

	listitem = UI_FindItemInScrollListWithId( &channelsScrollList, menuitem->localdata[1] );
	if( listitem && listitem->data )
		channel = (tv_channel_t *)listitem->data;
	if( channel )
	{
		char str[2][20];

		Q_snprintfz( str[0], sizeof( str[0] ), "%i", channel->id );
		Q_snprintfz( str[1], sizeof( str[0] ), " %i/%i", channel->numplayers, channel->numspecs );

		Q_snprintfz( menuitem->title, MAX_STRING_CHARS, ROW_PATTERN,
					 COLUMN_WIDTH_NO, str[0], 
					 COLUMN_WIDTH_NAME, S_COLOR_WHITE, channel->realname[0] ? channel->realname : channel->name,
					 COLUMN_WIDTH_PLAYERS, S_COLOR_WHITE, str[1],
					 COLUMN_WIDTH_GAMETYPE, S_COLOR_WHITE, channel->gametype,
					 COLUMN_WIDTH_MAPNAME, S_COLOR_WHITE, channel->mapname,
					 COLUMN_WIDTH_MATCHNAME, S_COLOR_WHITE, channel->matchname
					 );
		menuitem->statusbar = "press ENTER to watch";
	}
	else
	{
		Q_snprintfz( menuitem->title, MAX_STRING_CHARS, NO_CHANNEL_STRING );
		menuitem->statusbar = "";
	}
}

void M_Menu_TV_ChannelAdd_f( void )
{
	int num, id;
	char *name, *realname, *address, *gametype, *mapname, *matchname;
	int numplayers, numspecs;
	tv_channel_t *prev, *next, *new_chan;

	if( trap_Cmd_Argc() < 5 )
		return;

	id = atoi( trap_Cmd_Argv( 1 ) );
	name = trap_Cmd_Argv( 2 );
	realname = trap_Cmd_Argv( 3 );
	address = trap_Cmd_Argv( 4 );
	numplayers = atoi( trap_Cmd_Argv( 5 ) );
	numspecs = atoi( trap_Cmd_Argv( 6 ) );
	gametype = trap_Cmd_Argv( 7 );
	mapname = trap_Cmd_Argv( 8 );
	matchname = trap_Cmd_Argv( 9 );
	if( id <= 0 || !name[0] )
		return;

	num = 0;
	prev = NULL;
	next = channels;
	while( next && next->id < id )
	{
		prev = next;
		next = next->next;
		num++;
	}
	if( next && next->id == id )
	{
		new_chan = next;
		next = new_chan->next;
	}
	else
	{
		new_chan = (tv_channel_t *)UI_Malloc( sizeof( tv_channel_t ) );
		if( num < scrollbar_curvalue )
			scrollbar_curvalue++;
	}
	if( prev )
	{
		prev->next = new_chan;
	}
	else
	{
		channels = new_chan;
	}
	new_chan->next = next;
	new_chan->id = id;
	Q_strncpyz( new_chan->name, name, sizeof( new_chan->name ) );
	Q_strncpyz( new_chan->realname, realname, sizeof( new_chan->realname ) );
	Q_strncpyz( new_chan->address, address, sizeof( new_chan->address ) );
	Q_strncpyz( new_chan->gametype, gametype, sizeof( new_chan->gametype ) );
	Q_strncpyz( new_chan->mapname, mapname, sizeof( new_chan->mapname ) );
	Q_strncpyz( new_chan->matchname, matchname, sizeof( new_chan->matchname ) );

	Q_strlwr( new_chan->gametype );
	Q_strlwr( new_chan->mapname );

	new_chan->numplayers = numplayers;
	new_chan->numspecs = numspecs;

	M_RefreshScrollWindowList();
}

void M_Menu_TV_ChannelRemove_f( void )
{
	int num, id;
	tv_channel_t *prev, *iter;

	if( trap_Cmd_Argc() != 2 )
		return;

	id = atoi( trap_Cmd_Argv( 1 ) );
	if( id <= 0 )
		return;

	num = 0;
	prev = NULL;
	iter = channels;
	while( iter && iter->id != id )
	{
		prev = iter;
		iter = iter->next;
		num++;
	}
	if( !iter )
		return;

	if( prev )
	{
		prev->next = iter->next;
	}
	else
	{
		channels = iter->next;
	}
	UI_Free( iter );

	if( num < scrollbar_curvalue )
		scrollbar_curvalue--;

	M_RefreshScrollWindowList();
}

static void M_TV_Init( void )
{
	int i, yoffset, xoffset, vspacing;
	static char titlename[64];
	menucommon_t *menuitem;
	int scrollwindow_height, scrollwindow_width, scrollbar_id;

	yoffset = 0;
	xoffset = 0;

	s_tv_menu.nitems = 0;

	Q_snprintfz( titlename, sizeof( titlename ), "%s TV", trap_Cvar_String( "gamename" ) );

	menuitem = UI_InitMenuItem( "m_tv_title1", titlename, 0, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemBig, NULL );
	Menu_AddItem( &s_tv_menu, menuitem );
	yoffset += 2 *trap_SCR_strHeight( menuitem->font );

	// scrollbar
	if( uis.vidWidth < 800 )
		scrollwindow_width = uis.vidWidth * 0.85;
	else if( uis.vidWidth < 1024 )
		scrollwindow_width = uis.vidWidth * 0.75;
	else
		scrollwindow_width = uis.vidWidth * 0.65;

	xoffset = scrollwindow_width / 2;

	vspacing = trap_SCR_strHeight( uis.fontSystemSmall ) + 4;
	scrollwindow_height = uis.vidHeight - ( yoffset + ( 16 * trap_SCR_strHeight( uis.fontSystemBig ) ) );
	max_menu_channels = scrollwindow_height / vspacing;
	if( max_menu_channels < 5 )
		max_menu_channels = 5;

	menuitem = UI_InitMenuItem( "m_tv_titlerow", NULL, -xoffset, yoffset, MTYPE_SEPARATOR, ALIGN_LEFT_TOP, uis.fontSystemSmall, NULL );
	Q_snprintfz( menuitem->title, MAX_STRING_CHARS, 
			ROW_PATTERN,
			COLUMN_WIDTH_NO, "No", 
			COLUMN_WIDTH_NAME, "", "Server",
			COLUMN_WIDTH_PLAYERS, "", " Pl/Sp",
			COLUMN_WIDTH_GAMETYPE, "", "Gametype",
			COLUMN_WIDTH_MAPNAME, "", "Map",
			COLUMN_WIDTH_MATCHNAME, "", "Match"
	);
	Menu_AddItem( &s_tv_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_tv_scrollbar", NULL, xoffset, yoffset, MTYPE_SCROLLBAR, ALIGN_LEFT_TOP, uis.fontSystemSmall, M_TV_UpdateScrollbar );
	menuitem->vspacing = vspacing;
	menuitem->scrollbar_id = scrollbar_id = s_tv_menu.nitems; //give the scrollbar an id to pass onto its list
	Q_strncpyz( menuitem->title, va( "ui_tv_scrollbar%i_curvalue", scrollbar_id ), sizeof( menuitem->title ) );
	if( !trap_Cvar_Value( menuitem->title ) )
		trap_Cvar_SetValue( menuitem->title, 0 );
	UI_SetupScrollbar( menuitem, max_menu_channels, trap_Cvar_Value( menuitem->title ), 0, 0 );
	Menu_AddItem( &s_tv_menu, menuitem );

	for( i = 0; i < max_menu_channels; i++ )
	{
		menuitem = UI_InitMenuItem( va( "m_tv_button_%i", i ), NO_CHANNEL_STRING, -xoffset, yoffset, MTYPE_ACTION,
		                            ALIGN_LEFT_TOP, uis.fontSystemSmall, NULL );
		menuitem->callback_doubleclick = M_TV_JoinChannel;
		menuitem->scrollbar_id = scrollbar_id; //id of the scrollbar so that mwheelup/down can scroll from the list
		menuitem->height = vspacing;
		menuitem->statusbar = "press ENTER to watch";
		menuitem->ownerdraw = M_UpdateChannelButton;
		menuitem->localdata[0] = i; // line in the window
		menuitem->localdata[1] = i; // line in list
		menuitem->width = scrollwindow_width; // adjust strings to this width
		Menu_AddItem( &s_tv_menu, menuitem );

		// create an associated picture to the items to act as window background
		menuitem->pict.shader = uis.whiteShader;
		menuitem->pict.shaderHigh = uis.whiteShader;
		Vector4Copy( colorWhite, menuitem->pict.colorHigh );
		Vector4Copy( ( i & 1 ) ? colorDkGrey : colorMdGrey, menuitem->pict.color );
		menuitem->pict.color[3] = menuitem->pict.colorHigh[3] = 0.65f;
		menuitem->pict.yoffset = 0;
		menuitem->pict.xoffset = 0;
		menuitem->pict.width = scrollwindow_width;
		menuitem->pict.height = vspacing;

		yoffset += vspacing;
	}

	yoffset += 1.5 * trap_SCR_strHeight( menuitem->font );

	//xoffset = -scrollwindow_width / 2;
	menuitem = UI_InitMenuItem( "m_tv_refresh", "refresh", -xoffset, yoffset, MTYPE_ACTION, ALIGN_LEFT_TOP, uis.fontSystemBig, M_TV_RefreshFunc );
	Menu_AddItem( &s_tv_menu, menuitem );
	UI_SetupButton( menuitem, qtrue );

	//xoffset = scrollwindow_width / 2;
	menuitem = UI_InitMenuItem( "m_tv_disconnect", "watch", xoffset, yoffset, MTYPE_ACTION, ALIGN_RIGHT_TOP, uis.fontSystemBig, M_TV_JoinChannel );
	Menu_AddItem( &s_tv_menu, menuitem );
	UI_SetupButton( menuitem, qtrue );
	//yoffset += 1.5 * UI_SetupButton( menuitem, qtrue );
	xoffset -= ( menuitem->width + 16 );

	menuitem = UI_InitMenuItem( "m_tv_setup", "main menu", xoffset, yoffset, MTYPE_ACTION, ALIGN_RIGHT_TOP, uis.fontSystemBig, M_TV_MenuMainFunc );
	Menu_AddItem( &s_tv_menu, menuitem );
	yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;

	Menu_Center( &s_tv_menu );
	Menu_Init( &s_tv_menu, qfalse );

	Menu_SetStatusBar( &s_tv_menu, NULL );
}

static void M_TV_Draw( void )
{
	Menu_AdjustCursor( &s_tv_menu, 1 );
	Menu_Draw( &s_tv_menu );
}

static const char *M_TV_Key( int key )
{
	return Default_MenuKey( &s_tv_menu, key );
}

static const char *M_TV_CharEvent( qwchar key )
{
	return Default_MenuCharEvent( &s_tv_menu, key );
}

void M_Menu_TV_f( void )
{
	M_TV_Init();
	M_PushMenu( &s_tv_menu, M_TV_Draw, M_TV_Key, M_TV_CharEvent );
}
