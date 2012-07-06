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
static void Demos_MenuInit( void );

static menuframework_s s_demos_menu;


//=====================================================================
// PATH
//=====================================================================
// the current working directory
// init with demos
// each time we go in a new folder we append /name to cwd
// when user press backspace we go back one folder:
//   ie: we reverse search for a / and replace by a \0
char cwd[MAX_QPATH];
static int curnumfolders;

//=====================================================================
// Items list
//=====================================================================

//#define MAX_MENU_LIST_ITEMS 15
static int MAX_MENU_LIST_ITEMS = 15;

m_itemslisthead_t demosItemsList;

static void M_Demos_PreviousFolder( void )
{
	// go back to previous folder
	char *slash;

	slash = strrchr( cwd, '/' );

	if( slash != NULL )
	{
		// erase prev folder
		*slash = '\0';
		// rebuild list
		Demos_MenuInit();
	}
}
/*
* M_Demos_CreateFolderList
*/
static void M_Demos_CreateFolderList( void )
{
	char *s;
	char buffer[1024];
	int numfolders;
	int length;
	int i, j;
	char foldername[MAX_QPATH];

	curnumfolders = 0;

	if( strcmp( cwd, "demos" ) )
	{
		UI_AddItemToScrollList( &demosItemsList, S_COLOR_YELLOW "..", NULL );
		curnumfolders = 1;
	}

	if( ( numfolders = trap_FS_GetFileList( cwd, "/", NULL, 0, 0, 0 ) ) == 0 )
		return;

	i = 0;
	do
	{
		if( ( j = trap_FS_GetFileList( cwd, "/", buffer, sizeof( buffer ), i, numfolders ) ) == 0 )
		{
			i++; // can happen if the filename is too long to fit into the buffer or we're done
			continue;
		}

		i += j;
		for( s = buffer; j > 0; j--, s += length+1 )
		{
			length = strlen( s );
			Q_strncpyz( foldername, s, sizeof( foldername ) );
			foldername[length-1] = '\0'; // to remove the ending slash
			UI_AddItemToScrollList( &demosItemsList, va( "%s%s", S_COLOR_YELLOW, foldername ), NULL );
			curnumfolders++;
		}
	}
	while( i < numfolders );
}

/*
* M_Demos_CreateDemosList
*/
static void M_Demos_CreateDemosList( void )
{
	char *s;
	char buffer[1024];
	int numdemos;
	int length;
	int i, j;
	char demoname[MAX_STRING_CHARS];

	if( ( numdemos = trap_FS_GetFileList( cwd, va( ".wd%d", uis.gameProtocol ), NULL, 0, 0, 0 ) ) == 0 )
	{
		return;
	}

	i = 0;
	do
	{
		if( ( j = trap_FS_GetFileList( cwd, va( ".wd%d", uis.gameProtocol ), buffer, sizeof( buffer ), i, numdemos ) ) == 0 )
		{
			i++; // can happen if the filename is too long to fit into the buffer or we're done
			continue;
		}

		i += j;
		for( s = buffer; j > 0; j--, s += length+1 )
		{
			length = strlen( s );
			Q_strncpyz( demoname, s, sizeof( demoname ) );
			UI_AddItemToScrollList( &demosItemsList, demoname, NULL );
		}
	}
	while( i < numdemos );
}

/*
* M_Demos_CreateItemList
*/
static void M_Demos_CreateItemList( void )
{
	// first free the current list
	UI_FreeScrollItemList( &demosItemsList );

	// get all the folders
	M_Demos_CreateFolderList();

	// get all the demos
	M_Demos_CreateDemosList();
}


//=====================================================================
// Buttons & actions
//=====================================================================
#define	NO_ITEM_STRING	""
static int scrollbar_curvalue = 0;
static int scrollbar_id = 0;

/*
* M_Connect_UpdateScrollbar
*/
static void M_Demos_UpdateScrollbar( menucommon_t *menuitem )
{
	menuitem->maxvalue = max( 0, demosItemsList.numItems - MAX_MENU_LIST_ITEMS );
	if( menuitem->curvalue > menuitem->maxvalue )  //if filters shrunk the list size, shrink the scrollbar and its value
		menuitem->curvalue = menuitem->maxvalue;
	trap_Cvar_SetValue( menuitem->title, menuitem->curvalue );
	scrollbar_curvalue = menuitem->curvalue;
}

/*
* M_Demo_Playdemo
*/
static void M_Demo_Playdemo( menucommon_t *menuitem )
{
	char buffer[MAX_STRING_CHARS];
	m_listitem_t *item;

	menuitem->localdata[1] = menuitem->localdata[0] + scrollbar_curvalue;
	item = UI_FindItemInScrollListWithId( &demosItemsList, menuitem->localdata[1] );
	if( item && item->name )
	{ // can be a demo or a folder
		if( menuitem->localdata[1] >= curnumfolders )
		{
			// it is a demo
			//cwd is "demos/path"
			// we need to remove demos
			char *slash = strchr( cwd, '/' );
			if( slash == NULL )
				Q_snprintfz( buffer, sizeof( buffer ), "demo \"%s\"\n", item->name );
			else
			{
				slash++; // skip the slash
				trap_Print( va( "demo %s/%s\n", slash, item->name ) );
				Q_snprintfz( buffer, sizeof( buffer ), va( "demo \"%s/%s\"\n", slash, item->name ) );
			}
			trap_Cmd_ExecuteText( EXEC_APPEND, buffer );
		}
		else
		{
			// it is a folder
			const char *folderName = COM_RemoveColorTokens( item->name );
			size_t len = strlen( folderName );

			if( !strcmp( folderName, ".." ) )
			{
				// go back to previous folder
				M_Demos_PreviousFolder();
				return;
			}

			// sanity
			if( strlen( cwd ) + len >= sizeof( cwd ) )
			{
				UI_Printf( "Max sub folder limit reached\n" );
				return;
			}

			// add /name to cwd
			strcat( cwd, va( "/%s", folderName ) );

			// rebuild the lists
			Demos_MenuInit();
		}
	}
}

/*
* M_Demo_DemoButton
*/
static void M_Demo_UpdateButton( menucommon_t *menuitem )
{
	m_listitem_t *item;

	menuitem->localdata[1] = menuitem->localdata[0] + scrollbar_curvalue;
	item = UI_FindItemInScrollListWithId( &demosItemsList, menuitem->localdata[1] );
	if( item )
	{
		Q_snprintfz( menuitem->title, MAX_STRING_CHARS, item->name );
	}
	else
		Q_snprintfz( menuitem->title, MAX_STRING_CHARS, NO_ITEM_STRING );
}

/*
* Demos_MenuInit
*/
void Demos_MenuInit( void )
{
	char path[MAX_QPATH];
	int vspacing;
	menucommon_t *menuitem;
	int i, xoffset, yoffset = 0;
	int scrollwindow_width, scrollwindow_height;

	// scroll window width
	if( uis.vidWidth < 800 )
		scrollwindow_width = uis.vidWidth * 0.85;
	else if( uis.vidWidth < 1024 )
		scrollwindow_width = uis.vidWidth * 0.75;
	else
		scrollwindow_width = uis.vidWidth * 0.65;

	/*This is a hack, but utilizes the most room at low resolutions
	while leaving room so that the logo does not interfere. */
	if( uis.vidHeight < 768 )
		yoffset = uis.vidHeight * 0.07;
	else
		yoffset = uis.vidHeight * 0.1125;
	xoffset = scrollwindow_width / 2;
	s_demos_menu.nitems = 0;

	// build the list folder and demos
	M_Demos_CreateItemList();

	// title
	menuitem = UI_InitMenuItem( "m_demos_title1", "DEMOS", 0, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemBig, NULL );
	Menu_AddItem( &s_demos_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	// current folder
	Q_snprintfz( path, sizeof( path ), "%sCurrent folder: %s%s", S_COLOR_YELLOW, S_COLOR_WHITE, cwd );
	menuitem = UI_InitMenuItem( "m_demos_folder", path, -xoffset, yoffset, MTYPE_SEPARATOR, ALIGN_LEFT_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_demos_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

#if 0
	// Back button (back in demo browser)
	menuitem = UI_InitMenuItem( "m_demo_button_back", "Directory up", -xoffset, yoffset, MTYPE_ACTION, ALIGN_LEFT_TOP, uis.fontSystemSmall, M_Demos_PreviousFolder );
	menuitem->statusbar = "press for previous folder";
	Menu_AddItem( &s_demos_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );
#endif

	yoffset += trap_SCR_strHeight( menuitem->font );

	// scrollbar
	vspacing = trap_SCR_strHeight( uis.fontSystemSmall ) + 4;
	scrollwindow_height = uis.vidHeight - ( yoffset + ( 6 * trap_SCR_strHeight( uis.fontSystemBig ) ) );
	MAX_MENU_LIST_ITEMS = ( scrollwindow_height / vspacing );
	if( MAX_MENU_LIST_ITEMS < 5 ) MAX_MENU_LIST_ITEMS = 5;

	menuitem = UI_InitMenuItem( "m_demos_scrollbar", NULL, xoffset, yoffset, MTYPE_SCROLLBAR, ALIGN_RIGHT_TOP, uis.fontSystemSmall, M_Demos_UpdateScrollbar );
	menuitem->vspacing = vspacing;
	menuitem->scrollbar_id = scrollbar_id = s_demos_menu.nitems; //give the scrollbar an id to pass onto its list
	Q_strncpyz( menuitem->title, va( "ui_demos_scrollbar%i_curvalue", scrollbar_id ), sizeof( menuitem->title ) );
	if( !trap_Cvar_Value( menuitem->title ) )
		trap_Cvar_SetValue( menuitem->title, 0 );
	UI_SetupScrollbar( menuitem, MAX_MENU_LIST_ITEMS, trap_Cvar_Value( menuitem->title ), 0, 0 );
	Menu_AddItem( &s_demos_menu, menuitem );

	// demos/folder buttons
	for( i = 0; i < MAX_MENU_LIST_ITEMS; i++ )
	{
		menuitem = UI_InitMenuItem( va( "m_demos_button_%i", i ), NO_ITEM_STRING, -xoffset, yoffset, MTYPE_ACTION, ALIGN_LEFT_TOP, uis.fontSystemSmall, NULL );
		menuitem->callback_doubleclick = M_Demo_Playdemo;
		menuitem->statusbar = "press ENTER to play demos/enter folder, BACKSPACE for previous folder";
		menuitem->scrollbar_id = scrollbar_id; //id of the scrollbar so that mwheelup/down can scroll from the list
		menuitem->height = vspacing;
		menuitem->ownerdraw = M_Demo_UpdateButton;
		menuitem->localdata[0] = i; // line in the window
		menuitem->localdata[1] = i; // line in list
		menuitem->width = scrollwindow_width; // adjust strings to this width
		Menu_AddItem( &s_demos_menu, menuitem );

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

	yoffset += trap_SCR_strHeight( menuitem->font );

	// back (to previous menu)
	menuitem = UI_InitMenuItem( "m_demos_back", "back", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, M_genericBackFunc );
	Menu_AddItem( &s_demos_menu, menuitem );
	yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;

	Menu_Center( &s_demos_menu ); //this is effectless, but we'll leave it to keep consistant.
	Menu_Init( &s_demos_menu, qfalse );
}

static void Demos_MenuDraw( void )
{
	Menu_Draw( &s_demos_menu );
}

static const char *Demos_MenuKey( int key )
{
	menucommon_t *item;

	item = Menu_ItemAtCursor( &s_demos_menu );

	if( key == K_ESCAPE || ( ( key == K_MOUSE2 ) && ( item->type != MTYPE_SPINCONTROL ) &&
		( item->type != MTYPE_SLIDER ) ) )
	{
		UI_FreeScrollItemList( &demosItemsList );
	}
	else if( key == K_BACKSPACE )
	{
		M_Demos_PreviousFolder();
	}
	return Default_MenuKey( &s_demos_menu, key );
}

static const char *Demos_MenuCharEvent( qwchar key )
{
	return Default_MenuCharEvent( &s_demos_menu, key );
}

void M_Menu_Demos_f( void )
{
	// init current working directory
	memset( cwd, '\0', sizeof( cwd ) );
	Q_snprintfz( cwd, sizeof( cwd ), "demos" );

	// lets go
	Demos_MenuInit();
	M_PushMenu( &s_demos_menu, Demos_MenuDraw, Demos_MenuKey, Demos_MenuCharEvent );
}
