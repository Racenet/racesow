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

#include <string.h>
#include "ui_local.h"

/*
   =============================================================================

   DEMOS MENU

   =============================================================================
 */

static menuframework_s s_mods_menu;



//=====================================================================
// Items list
//=====================================================================

static int MAX_MENU_LIST_ITEMS = 0;

m_itemslisthead_t modsItemsList;


//==================
//M_Mods_CreateFolderList
//==================
static void M_Mods_CreateFolderList( void )
{
	const char *s;
	char buffer[8*1024], foldername[MAX_QPATH];
	int numfolders, length, i;

	if( ( numfolders = trap_FS_GetGameDirectoryList( buffer, sizeof( buffer ) ) ) == 0 )
		return;

	s = buffer;
	length = 0;
	for( i = 0; i < numfolders; i++, s += length+1 )
	{
		length = strlen( s );
		Q_strncpyz( foldername, s, sizeof( foldername ) );
		UI_AddItemToScrollList( &modsItemsList, foldername, NULL );
	}
}

//==================
//M_Mods_CreateItemList
//==================
static void M_Mods_CreateItemList( void )
{
	// first free the current list
	UI_FreeScrollItemList( &modsItemsList );

	// get all the folders
	M_Mods_CreateFolderList();
}


//=====================================================================
// Buttons & actions
//=====================================================================
#define	NO_ITEM_STRING	""
static int scrollbar_curvalue = 0;
static int scrollbar_id = 0;

//==================
//M_Mods_UpdateScrollbar
//==================
static void M_Mods_UpdateScrollbar( menucommon_t *menuitem )
{
	menuitem->maxvalue = max( 0, modsItemsList.numItems - MAX_MENU_LIST_ITEMS );
	if( menuitem->curvalue > menuitem->maxvalue )  //if filters shrunk the list size, shrink the scrollbar and its value
		menuitem->curvalue = menuitem->maxvalue;
	trap_Cvar_SetValue( menuitem->title, menuitem->curvalue );
	scrollbar_curvalue = menuitem->curvalue;
}

//==================
//M_Mods_LoadMod
//==================
static void M_Mods_LoadMod( menucommon_t *menuitem )
{
	m_listitem_t *item;

	menuitem->localdata[1] = menuitem->localdata[0] + scrollbar_curvalue;
	item = UI_FindItemInScrollListWithId( &modsItemsList, menuitem->localdata[1] );
	if( item && item->name )
	{
		UI_Printf( "fs_game \"%s\"\n", item->name );
		trap_Cmd_ExecuteText( EXEC_APPEND, va( "fs_game \"%s\"", item->name ) );
	}
}

//==================
//M_Mods_DemoButton
//==================
static void M_Mods_UpdateButton( menucommon_t *menuitem )
{
	m_listitem_t *item;

	menuitem->localdata[1] = menuitem->localdata[0] + scrollbar_curvalue;
	item = UI_FindItemInScrollListWithId( &modsItemsList, menuitem->localdata[1] );
	if( item )
	{
		Q_snprintfz( menuitem->title, MAX_STRING_CHARS, item->name );
	}
	else
		Q_snprintfz( menuitem->title, MAX_STRING_CHARS, NO_ITEM_STRING );
}

//==================
//Mods_MenuInit
//==================
static void Mods_MenuInit( void )
{
	menucommon_t *menuitem;
	int i, scrollwindow_width, scrollwindow_height, xoffset, yoffset = 0;

	// scroll window width
	if( uis.vidWidth < 800 )
		scrollwindow_width = uis.vidWidth * 0.85;
	else if( uis.vidWidth < 1024 )
		scrollwindow_width = uis.vidWidth * 0.75;
	else
		scrollwindow_width = uis.vidWidth * 0.65;

	s_mods_menu.nitems = 0;

	// build the list folder and demos
	M_Mods_CreateItemList();

	/*This is a hack, but utilizes the most room at low resolutions
	   while leaving room so that the logo does not interfere. */
	if( uis.vidHeight < 768 )
		yoffset = uis.vidHeight * 0.07;
	else
		yoffset = uis.vidHeight * 0.1125;
	xoffset = scrollwindow_width / 2;

	// title
	menuitem = UI_InitMenuItem( "m_mods_title1", "MODS", 0, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemBig, NULL );
	Menu_AddItem( &s_mods_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	yoffset += trap_SCR_strHeight( menuitem->font );

	// scrollbar
	scrollwindow_height = uis.vidHeight - ( yoffset + ( 6 * trap_SCR_strHeight( uis.fontSystemBig ) ) );
	MAX_MENU_LIST_ITEMS = ( scrollwindow_height / trap_SCR_strHeight( uis.fontSystemSmall ) );
	if( MAX_MENU_LIST_ITEMS < 5 )
		MAX_MENU_LIST_ITEMS = 5;

	menuitem = UI_InitMenuItem( "m_mods_scrollbar", NULL, xoffset, yoffset, MTYPE_SCROLLBAR, ALIGN_LEFT_TOP, uis.fontSystemSmall, M_Mods_UpdateScrollbar );
	menuitem->scrollbar_id = scrollbar_id = s_mods_menu.nitems; //give the scrollbar an id to pass onto its list
	Q_strncpyz( menuitem->title, va( "ui_mods_scrollbar%i_curvalue", scrollbar_id ), sizeof( menuitem->title ) );
	if( !trap_Cvar_Value( menuitem->title ) )
		trap_Cvar_SetValue( menuitem->title, 0 );
	UI_SetupScrollbar( menuitem, MAX_MENU_LIST_ITEMS, trap_Cvar_Value( menuitem->title ), 0, 0 );
	scrollbar_id = s_mods_menu.nitems; //give the scrollbar an id to pass onto its list
	Menu_AddItem( &s_mods_menu, menuitem );

	// window buttons
	for( i = 0; i < MAX_MENU_LIST_ITEMS; i++ )
	{
		menuitem = UI_InitMenuItem( va( "m_mods_button_%i", i ), NO_ITEM_STRING, -xoffset, yoffset, MTYPE_ACTION, ALIGN_LEFT_TOP, uis.fontSystemSmall, NULL );
		menuitem->callback_doubleclick = M_Mods_LoadMod;
		//menuitem->statusbar = "press ENTER to play demos/enter folder, BACKSPACE for previous folder";
		menuitem->scrollbar_id = scrollbar_id; //id of the scrollbar so that mwheelup/down can scroll from the list
		menuitem->ownerdraw = M_Mods_UpdateButton;
		menuitem->localdata[0] = i; // line in the window
		menuitem->localdata[1] = i; // line in list
		menuitem->width = scrollwindow_width; // adjust strings to this width
		Menu_AddItem( &s_mods_menu, menuitem );

		// create an associated picture to the items to act as window background
		menuitem->pict.shader = uis.whiteShader;
		menuitem->pict.shaderHigh = uis.whiteShader;
		Vector4Copy( colorWhite, menuitem->pict.colorHigh );
		Vector4Copy( ( i & 1 ) ? colorDkGrey : colorMdGrey, menuitem->pict.color );
		menuitem->pict.color[3] = menuitem->pict.colorHigh[3] = 0.65f;
		menuitem->pict.yoffset = menuitem->pict.xoffset = 0;
		menuitem->pict.width = scrollwindow_width;
		menuitem->pict.height = trap_SCR_strHeight( menuitem->font );

		yoffset += trap_SCR_strHeight( menuitem->font );
	}

	yoffset += trap_SCR_strHeight( menuitem->font );

	// back (to previous menu)
	menuitem = UI_InitMenuItem( "m_mods_back", "back", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, M_genericBackFunc );
	Menu_AddItem( &s_mods_menu, menuitem );
	yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;

	Menu_Center( &s_mods_menu ); //this is effectless, but we'll leave it to keep consistant.
	Menu_Init( &s_mods_menu, qfalse );
}

static void Mods_MenuDraw( void )
{
	Menu_Draw( &s_mods_menu );
}

static const char *Mods_MenuKey( int key )
{
	menucommon_t *item;

	item = Menu_ItemAtCursor( &s_mods_menu );

	if( key == K_ESCAPE || ( ( key == K_MOUSE2 ) && ( item->type != MTYPE_SPINCONTROL ) &&
	                        ( item->type != MTYPE_SLIDER ) ) )
	{
		UI_FreeScrollItemList( &modsItemsList );
	}

	return Default_MenuKey( &s_mods_menu, key );
}

static const char *Mods_MenuCharEvent( qwchar key )
{
	return Default_MenuCharEvent( &s_mods_menu, key );
}

void M_Menu_Mods_f( void )
{
	Mods_MenuInit();
	M_PushMenu( &s_mods_menu, Mods_MenuDraw, Mods_MenuKey, Mods_MenuCharEvent );
}
