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

	Trap::Cvar_SetValue( "sv_skilllevel", startserver->skills->getSelectedPosition() );

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
/*
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
*/
void StartServerMenu::mapSelectedHandler( ListItem *target, int, bool isSelected )
{
	startserver->start->setEnabled( isSelected );
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
	//struct shader_s *screenshot;
	struct shader_s *unknownmapshot;
	ListItem *listitem;
	//Panel *mappicture;
	//Label *mapnamelabel, *mapfullnamelabel;
	maplist_item_t *items;
	//char screenshotname[MAX_CONFIGSTRING_CHARS];
	char mapinfo[MAX_CONFIGSTRING_CHARS], *mapname = NULL, *fullname;
	char *lastmap;
	const char *cleanName;
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

		items[i].name = UI_CopyString( fullname );
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
		cleanName = COM_RemoveColorTokensExt( items[i].name, qtrue );

		listitem = Factory::newListItem( selmap, va( "%s\n" S_COLOR_ORANGE "%s", items[i].shortname, cleanName ), i );
		listitem->setUserData( (void *)items[i].index );
		listitem->setAlign( ALIGN_MIDDLE_LEFT );
		listitem->setSize( listitem->getWidth(), selmap->getItemSize() );
		listitem->setMultiline( true );
		listitem->setWordwrap( false );
/*
		Q_snprintfz( screenshotname, sizeof(screenshotname), "levelshots/%s.jpg", items[i].shortname );
		screenshot = Trap::R_RegisterLevelshot( screenshotname, unknownmapshot, NULL ); 

		mappicture = Factory::newPanel( listitem, 10, 5, 40, 30 );
		mappicture->setBorderWidth( 0 );
		mappicture->setBackgroundImage( screenshot );
		mappicture->setClickable( false );
		mappicture->setBackColor( Color( 1, 1, 1, 1 ) );

		mapnamelabel = Factory::newLabel( listitem, 50, 0, 90, listitem->getHeight() );
		mapnamelabel->setCaption( items[i].shortname );
		mapnamelabel->setClickable( false );
		mapnamelabel->setAlign( ALIGN_MIDDLE_LEFT );

		if( cleanName[0] )
		{
			mapfullnamelabel = Factory::newLabel( listitem, 150, 0, listitem->getWidth() - 150, listitem->getHeight() );
			mapfullnamelabel->setCaption( cleanName );
			mapfullnamelabel->setClickable( false );
			mapfullnamelabel->setAlign( ALIGN_MIDDLE_LEFT );
		}
*/
		if( items[i].index == lastmapindex )
			selmap->selectItem( listitem );

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
	skills->selectItem( skill_lvl );
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

StartServerMenu::StartServerMenu() : MainMenu( "Start A Server" )
{
	int xoffset = 10, yoffset = 0, ysuboffset;
	int spacing = 22;

	yoffset = 20;

	panel = Factory::newFrame( contentPanel, 0, 0, contentPanel->getWidth(), contentPanel->getHeight() );

	// map selection
	lselmap = Factory::newLabel( panel, 430, yoffset, 110, 20, "Select map:" );
	selmap = Factory::newDropDownBox( panel, 430, yoffset + lselmap->getHeight() + spacing / 2, panel->getWidth() - 20 - 430, 22, 200, "Map selection" );
	selmap->setItemSize( 36 );
	selmap->setItemSelectedHandler( mapSelectedHandler );

	// main section
	lservname = Factory::newLabel( panel, 15, yoffset, 110, 20, "Server name:" );
	lservname->setAlign( ALIGN_MIDDLE_RIGHT );

	servname = Factory::newTextBox( panel, 140, yoffset, 200, 20, 20 );

	lplrnum = Factory::newLabel( panel, 15, yoffset += servname->getHeight() + spacing, 110, 20, "Max players:" );
	lplrnum->setAlign( ALIGN_MIDDLE_RIGHT );
	plrnum = Factory::newTextBox( panel, 140, yoffset, 50, 20, 4 );

	cheats = Factory::newCheckBox( panel, 35, yoffset += servname->getHeight() + spacing, "Cheats:", 120 );
	cheats->setAlign( ALIGN_MIDDLE_RIGHT );

	pub = Factory::newCheckBox( panel, 35, yoffset += cheats->getHeight() + spacing, "Public:", 120 );
	pub->setAlign( ALIGN_MIDDLE_RIGHT );

	// gameplay
	ysuboffset = 0;

	gameplay = Factory::newPanel( panel, xoffset, yoffset += pub->getHeight() + spacing, 380, 340 );
	gameplay->setBackColor( UIMenu::transparent );
	gameplay->setBorderWidth( 0 );
	gameplay->setSize( gameplay->getWidth(), 180 );

	lgametype = Factory::newLabel( gameplay, 15, ysuboffset += spacing, 100, 20, "Game type:" );
	lgametype->setAlign( ALIGN_MIDDLE_RIGHT );

	gametype = Factory::newDropDownBox( gameplay, 130, ysuboffset, 200, 22, 100, "Select to begin..." );
	gametype->setMultipleSelection( false );
	for ( int i = 0 ; i < GAMETYPE_NB ; ++i )
		Factory::newListItem( gametype, gametype_names[i][0] );

	instagib = Factory::newCheckBox( gameplay, 15, ysuboffset += gametype->getHeight() + spacing, "Instagib:", 130 );
	instagib->setAlign( ALIGN_MIDDLE_RIGHT );

	ltimelimit = Factory::newLabel( gameplay, 15, ysuboffset += instagib->getHeight() + spacing, 100, 20, "Time limit:" );
	ltimelimit->setAlign( ALIGN_MIDDLE_RIGHT );

	timelimit = Factory::newTextBox( gameplay, 130, ysuboffset, 50, 20, 4 );

	lscorelimit = Factory::newLabel( gameplay, 15, ysuboffset += timelimit->getHeight() + spacing, 100, 20, "Score limit:" );
	lscorelimit->setAlign( ALIGN_MIDDLE_RIGHT );

	scorelimit = Factory::newTextBox( gameplay, 130, ysuboffset, 50, 20, 4 );

	// bots
	ysuboffset = 0;

	bots = Factory::newPanel( panel, xoffset, yoffset += gameplay->getHeight() + spacing, 380, 340 );
	bots->setBackColor( Color( 1, 1, 1, 0 ) );
	bots->setBorderWidth( 0 );
	bots->setSize( gameplay->getWidth(), 100 );

	lskill = Factory::newLabel( bots, 15, ysuboffset, 100, 20, "Bot skill:" );
	lskill->setAlign( ALIGN_MIDDLE_RIGHT );

	skills = Factory::newDropDownBox( bots, 130, ysuboffset, 200, 22, 70, "Select to begin..." );
	Factory::newListItem( skills, "Low" );
	Factory::newListItem( skills, "Medium" );
	Factory::newListItem( skills, "High" );
	Factory::newListItem( skills, "Any" );

	lbotnum = Factory::newLabel( bots, 15, ysuboffset += lskill->getHeight() + spacing, 100, 20, "Bot number:" );
	lbotnum->setAlign( ALIGN_MIDDLE_RIGHT );

	botnum = Factory::newTextBox( bots, 130, ysuboffset, 50, 20, 4 );

	// start button
	start = Factory::newButton( panel, panel->getWidth() - 20 - 130, 
		selmap->getPositionY() + selmap->getHeight(), 130, 30, "Start Server" );
	start->setPosition( start->getPositionX(), start->getPositionY() + (panel->getHeight() - start->getPositionY() - start->getHeight()) / 2 );
	start->setClickHandler( startHandler );
	start->setEnabled( false );
}

void StartServerMenu::Show( void )
{
	BrowseMaps();
	UpdateFields();

	setActiveMenu( this );
	panel->setVisible( true );

	MainMenu::Show();
}

void StartServerMenu::Hide( void )
{
	MainMenu::Hide();

	panel->setVisible( false );
}
