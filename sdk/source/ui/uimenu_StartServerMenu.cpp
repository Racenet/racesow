/*
Copyright (C) 2007 Benjamin Litzelmann

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


#include "uicore_Global.h"
#include "uimenu_StartServerMenu.h"
#include "uiwsw_MapList.h"
#include "uiwsw_Export.h"
#include "uiwsw_SysCalls.h"
#include "uiwsw_Utils.h"

using namespace UICore;
using namespace UIMenu;
using namespace UIWsw;

// TODO : static every instance of menu classes if possible
StartServerMenu *startserver = NULL;

void UIMenu::M_Menu_StartServer_f( void )
{
	if ( !startserver )
		startserver = new StartServerMenu();

	startserver->Show();
}

void StartServerMenu::backHandler( BaseObject* )
{
	Trap::Cmd_ExecuteText( EXEC_APPEND, "menu_main" );
}

void StartServerMenu::startHandler( BaseObject* )
{
	int i;
	char mapinfo[MAX_CONFIGSTRING_CHARS], *mapname;

	Trap::Cvar_Set( "g_gametype", gametype_names[startserver->gametype->getItemPosition(startserver->gametype->getSelectedItem())][1] );
	for ( i = 0 ; i < 4; ++i )
	{
		if( startserver->skill[i]->isPressed() )
		{
			Trap::Cvar_SetValue( "sv_skilllevel", i );
			break;
		}
	}

	Trap::Cvar_SetValue( "sv_cheats", startserver->cheats->isPressed() );
	Trap::Cvar_SetValue( "g_instagib", startserver->instagib->isPressed() );
	Trap::Cvar_SetValue( "sv_public", startserver->pub->isPressed() );

	Trap::Cvar_Set( "sv_hostname", startserver->servname->getText() );

	Trap::Cvar_SetValue( "g_timelimit", max( 0, atoi(startserver->timelimit->getText()) ) );
	Trap::Cvar_SetValue( "g_scorelimit", max( 0, atoi(startserver->scorelimit->getText()) ) );

	Trap::Cvar_SetValue( "g_numbots", max( 0, atoi(startserver->botnum->getText()) ) );
	i = atoi(startserver->plrnum->getText());
	if ( i <= 1 )
		i = 8;
	Trap::Cvar_SetValue( "sv_maxclients", i );

	if( Local::getServerState() )
		Trap::Cmd_ExecuteText( EXEC_APPEND, "disconnect\n" );

	size_t index = (size_t)startserver->selmap->getSelectedItem()->getUserData();
	if( !Trap::ML_GetMapByNum( (int)index, mapinfo, sizeof( mapinfo ) ) )
		return;
	mapname = mapinfo;

	Trap::Cmd_ExecuteText( EXEC_APPEND, va("map %s\n", mapname) );

	Trap::Cvar_ForceSet( "ui_startserver_lastselectedmap", startserver->selmap->getSelectedItem()->getCaption().c_str() );
}

void StartServerMenu::skillHandler( BaseObject* target, bool newValue )
{
	SwitchButton *button = static_cast<SwitchButton*>( target );
	if ( newValue == false ) // disallow having no skill selected
		button->setPressed( true );
	else
	{
		for ( int i = 0 ; i < 4 ; ++i ) // one skill selected at one
			if ( startserver->skill[i] != button )
				startserver->skill[i]->setPressed( false );
	}
}

void StartServerMenu::mapSelectedHandler( ListItem *target, int, bool isSelected )
{
	startserver->start->setEnabled( isSelected );

	if ( startserver->suggestgametype->isPressed() )
	{
		startserver->SuggestGametype( target->getCaption().c_str() );
	}
}

void StartServerMenu::SuggestGametype( const char *mapname )
{
	for ( int i = 0 ; i < GAMETYPE_NB ; ++i )
		if( strstr( mapname, gametype_names[i][1] ) )
		{
			gametype->selectItem( i );
			return;
		}
}

typedef struct
{
	char *name;
	char *shortname;
	int index;
} maplist_item_t;

static int MapList_MapSort( const maplist_item_t *a, const maplist_item_t *b )
{
	return Q_stricmp( a->name, b->name );
}

void StartServerMenu::BrowseMaps( void )
{
	int validmaps;
	int i, j, lastmapindex = -1;
	struct shader_s *screenshot, *unknownmapshot;
	ListItem *listitem;
	Panel *mappicture;
	maplist_item_t *items;
	char screenshotname[MAX_CONFIGSTRING_CHARS];
	char mapinfo[MAX_CONFIGSTRING_CHARS], *mapname = NULL, *fullname;
	char *lastmap;
	//cvar_t *cvar_lastmap;

	// deleting previews and clearing the list
	for ( int i = 0 ; i < selmap->getNbItem() ; ++i )
		selmap->getItem(i)->deleteChildren();

	selmap->clear();

    lastmap = Trap::Cvar_Get( "ui_startserver_lastselectedmap", "", CVAR_NOSET )->string;
   
	for( validmaps = 0; Trap::ML_GetMapByNum( validmaps, NULL, 0 ); validmaps++ )
		;

	if( !validmaps )
	{
		//Menu_SetStatusBar( &s_startserver_menu, "No maps found" );
		return;
	}

	items = (maplist_item_t *) UIMem::Malloc( sizeof( *items ) * validmaps );
	for( i = 0; i < validmaps; i++ )
	{
	    Trap::ML_GetMapByNum( i, mapinfo, sizeof( mapinfo ) );

		mapname = mapinfo;
		fullname = mapinfo + strlen( mapname ) + 1;
		if( !strcmp( mapname, lastmap ) )
			lastmapindex = i;

		items[i].name = UI_CopyString( *fullname ? fullname : mapname );
		items[i].shortname = UI_CopyString( mapname );
		items[i].index = i;

		// uppercase the first letter and letters that follow whitespaces
		if( *fullname )
		{
			items[i].name[0] = toupper( items[i].name[0] );
			for( j = 1; items[i].name[j]; j++ )
				if( items[i].name[j-1] == ' ' )
					items[i].name[j] = toupper( items[i].name[j] );
		}
	}

	qsort( items, validmaps, sizeof( *items ), ( int ( * )( const void *, const void * ) ) MapList_MapSort );

	unknownmapshot = MapList::GetUnknownMapPic();
	for( i = 0; i < validmaps; i++ )
	{
		listitem = Factory::newListItem( selmap, items[i].name, i );
		listitem->setUserData( (void *)items[i].index );
		if( items[i].index == lastmapindex )
			selmap->selectItem( listitem );

		Q_snprintfz( screenshotname, sizeof(screenshotname), "levelshots/%s.jpg", items[i].shortname );
		screenshot = Trap::R_RegisterLevelshot( screenshotname, unknownmapshot ); 

		mappicture = Factory::newPanel( listitem, 10, 10, 54, 40 );
		mappicture->setBorderWidth( 0 );
		mappicture->setBackgroundImage( screenshot );
		mappicture->setClickable( false );
		mappicture->setBackColor( Color( 1, 1, 1, 1 ) );

		//mappicture->setDisabledImage( screenshot );
		//mappicture->setDisabledColor( Color( 1, 1, 1, 1 ) ); // fullbright pic
		//mappicture->setEnabled( false ); // avoid having picture getting mouse events
		//UI_AddItemToScrollList( &mapsList, items[i].name, (void *)(items[i].index) );
		UIMem::Free( items[i].name );
		UIMem::Free( items[i].shortname );
	}

	UIMem::Free( items );
}
// TODO : make a kind of statusbar...

void StartServerMenu::UpdateFields( void )
{
	int maxclients;
	int skill_lvl;
	int gametype_id = 0;
	const char *gametype_name;

	skill_lvl = Trap::Cvar_Int( "sv_skilllevel" );
	if ( skill_lvl < 0 || skill_lvl > 3 )
		skill_lvl = 3;

	maxclients = Trap::Cvar_Int( "sv_maxclients" );
	if ( maxclients <= 1 )
		maxclients = 8;

	gametype_name = Trap::Cvar_String("g_gametype");
	for ( int i = 0 ; i < GAMETYPE_NB ; ++i )
		if ( !Q_stricmp( gametype_name, gametype_names[i][1] ) )
		{
			gametype_id = i;
			break;
		}

	servname->setText( Trap::Cvar_String("sv_hostname") );

	gametype->selectItem( gametype_id );
	skill[skill_lvl]->setPressed( true );
	instagib->setPressed( Trap::Cvar_Value( "g_instagib" ) != 0.0f );
	timelimit->setText( Trap::Cvar_String("g_timelimit") );
	scorelimit->setText( Trap::Cvar_String("g_scorelimit") );

	cheats->setPressed( Trap::Cvar_Value( "sv_cheats" ) != 0.0f );
	pub->setPressed( Trap::Cvar_Value( "sv_public" ) != 0.0f );
	botnum->setText( Trap::Cvar_String("g_numbots") );
	plrnum->setText( va( "%i", maxclients ) );
}

ALLOCATOR_DEFINITION(StartServerMenu)
DELETER_DEFINITION(StartServerMenu)

StartServerMenu::StartServerMenu()
{
	int yoffset = 0, ysuboffset = 0;
	panel = Factory::newPanel( rootPanel, 25, 25, 750, 550 );

	lservname = Factory::newLabel( panel, 20, yoffset += 20, 110, 20, "Server name:" );
	servname = Factory::newTextBox( panel, 140, yoffset, 200, 20, 20 );
	
	gameplay = Factory::newPanel( panel, 20, yoffset += 30, 380, 340 );
	lgametype = Factory::newLabel( gameplay, 20, ysuboffset += 20, 90, 20, "Game type:" );
	gametype = Factory::newListBox( gameplay, 120, ysuboffset, 250, 150, false );
	suggestgametype = Factory::newCheckBox( gameplay, 120, ysuboffset += 160, 250, 20, "Auto-select if available" );
	instagib = Factory::newCheckBox( gameplay, 20, ysuboffset += 30, 120, 20, "Instagib:" );
	lskill = Factory::newLabel( gameplay, 20, ysuboffset += 30, 90, 20, "Skill:" );
	skill[0] = Factory::newSwitchButton( gameplay, 130, ysuboffset, 50, 20, "Low" );
	skill[1] = Factory::newSwitchButton( gameplay, 190, ysuboffset, 50, 20, "Med" );
	skill[2] = Factory::newSwitchButton( gameplay, 250, ysuboffset, 50, 20, "High" );
	skill[3] = Factory::newSwitchButton( gameplay, 310, ysuboffset, 50, 20, "Any" );
	ltimelimit = Factory::newLabel( gameplay, 20, ysuboffset += 30, 90, 20, "Time limit:" );
	timelimit = Factory::newTextBox( gameplay, 120, ysuboffset, 180, 20, 4 );
	lscorelimit = Factory::newLabel( gameplay, 20, ysuboffset += 30, 90, 20, "Score limit:" );
	scorelimit = Factory::newTextBox( gameplay, 120, ysuboffset, 180, 20, 4 );

	ysuboffset = 0;
	players = Factory::newPanel( panel, 20, yoffset += 350, 380, 150 );
	cheats = Factory::newCheckBox( players, 20, ysuboffset += 20, 120, 20, "Cheats:" );
	pub = Factory::newCheckBox( players, 20, ysuboffset += 30, 120, 20, "Public:" );
	lbotnum = Factory::newLabel( players, 20, ysuboffset += 30, 90, 20, "Bot number:" );
	botnum = Factory::newTextBox( players, 120, ysuboffset, 180, 20, 4 );
	lplrnum = Factory::newLabel( players, 20, ysuboffset += 30, 90, 20, "Player number:" );
	plrnum = Factory::newTextBox( players, 120, ysuboffset, 180, 20, 4 );

	lselmap = Factory::newLabel( panel, 430, 20, 300, 20, "Select map:" );
	selmap = Factory::newListBox( panel, 430, 50, 300, 420, false );

	back = Factory::newButton( panel, 430, 500, 100, 30, "Back" );
	start = Factory::newButton( panel, 630, 500, 100, 30, "Start" );

	gametype->setMultipleSelection( false );
	for ( int i = 0 ; i < 4 ; ++i )
		skill[i]->setSwitchHandler( skillHandler );
	instagib->setAlign( ALIGN_MIDDLE_RIGHT );
	lgametype->setAlign( ALIGN_MIDDLE_RIGHT );
	suggestgametype->setPressed( true );
	lskill->setAlign( ALIGN_MIDDLE_RIGHT );
	ltimelimit->setAlign( ALIGN_MIDDLE_RIGHT );
	lscorelimit->setAlign( ALIGN_MIDDLE_RIGHT );
	lgametype->setAlign( ALIGN_MIDDLE_RIGHT );
	cheats->setAlign( ALIGN_MIDDLE_RIGHT );
	pub->setAlign( ALIGN_MIDDLE_RIGHT );
	selmap->setItemSize( 60 );
	selmap->setItemSelectedHandler( mapSelectedHandler );
	back->setClickHandler( backHandler );
	start->setClickHandler( startHandler );
	start->setEnabled( false );

	for ( int i = 0 ; i < GAMETYPE_NB ; ++i )
		Factory::newListItem( gametype, gametype_names[i][0] );
}

void StartServerMenu::Show( void )
{
	BrowseMaps();
	UpdateFields();

	setActiveMenu( this );
	panel->setVisible( true );
}

void StartServerMenu::Hide( void )
{
	panel->setVisible( false );
}
