/*
Copyright (C) 2007 Will Franklin

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
#include "uimenu_MatchMakerMenu.h"
#include "uiwsw_Export.h"
#include "uiwsw_SysCalls.h"
#include "uiwsw_Utils.h"

using namespace UICore;
using namespace UIMenu;
using namespace UIWsw;

MatchMakerMenu *matchmakermenu = NULL;
int clientslot;

// what is currently being shown in the msgbox

void UIMenu::M_Menu_MatchMaker_f( void )
{
	if( !matchmakermenu )
		matchmakermenu = new MatchMakerMenu;

	matchmakermenu->Show();
}

ALLOCATOR_DEFINITION(MatchMakerMenu)
DELETER_DEFINITION(MatchMakerMenu)

void MatchMakerMenu::tabsHandler( BaseObject *tab, bool )
{
	int tabno = 0;

	// select one of the tabs
	// unselect the rest
	for( int i = 0 ; i < MatchMakerMenu_TabCount ; i++ )
	{
		if( tab == matchmakermenu->btnTabs[i] )
		{
			matchmakermenu->btnTabs[i]->setPressed( true );
			tabno = i;
		}
		else
			matchmakermenu->btnTabs[i]->setPressed( false );
	}

	// hide all panels
	matchmakermenu->pMain->setVisible( false );
	matchmakermenu->pCups->setVisible( false );

	// show the correct panel
	if( tabno == 0 )
		matchmakermenu->pMain->setVisible( true );
	else if( tabno == 1 )
		matchmakermenu->pCups->setVisible( true );
}

void MatchMakerMenu::btnBackHandler( BaseObject * )
{
	Trap::Cmd_ExecuteText( EXEC_APPEND, "menu_main" );
}

void MatchMakerMenu::btnMainDropHandler( BaseObject * )
{
	Trap::MM_UIRequest( ACTION_DROP, "" );
}

void MatchMakerMenu::matchesHandler( MessageBoxButton result )
{
	switch( result )
	{
	case BUTTON_YES:
		matchmakermenu->addMatch->Show();
		matchmakermenu->addMatch->setGametype( matchmakermenu->lstMainGametype->getSelectedPosition() );
		break;
	case BUTTON_NO:
		matchmakermenu->lstMainGametype->clearSelection();
		break;
	default:
		break;
	}
}

void MatchMakerMenu::joiningHandler( MessageBoxButton result )
{
	switch( result )
	{
	case BUTTON_CANCEL:
		Trap::MM_UIRequest( ACTION_DISCONNECT, "" );
		matchmakermenu->messageBox->Hide();
		break;
	case BUTTON_RETRY:
		Trap::MM_UIRequest( ACTION_JOIN, gametype_names[matchmakermenu->lstMainGametype->getSelectedPosition()][1] );
		break;
	default:
		break;
	}
}

ListItem *MatchMakerMenu::CreatePlayerItem( char *name, int slot )
{
	ListItem *item;
	Panel *panel, *ping;
	Label *label;
	char *image;

	item = Factory::newListItem( lstMainPlayers, "", slot );

	if( strlen( name ) )
		image = "gfx/ui/matchmaker/player";
	else
		image = "gfx/ui/matchmaker/noplayer";

	panel = Factory::newPanel( item, 5, ( item->getHeight() - 20 ) / 2, 22, 20 );
	panel->setBorderWidth( 0 );
	panel->setClickable( false );
	panel->setBackgroundImage( Trap::R_RegisterPic( image ) );

	if( strlen( name ) )
	{
		ping = Factory::newPanel( item, item->getWidth() - 5 - 19, ( item->getHeight() - 14 ) / 2, 19, 14 );
		ping->setBorderWidth( 0 );
		ping->setClickable( false );
		ping->setBackgroundImage( Trap::R_RegisterPic( "gfx/ui/matchmaker/ping" ) );
	}

	if( !strlen( name ) )
		name = "Empty slot";
	label = Factory::newLabel( item, 0, 0, 100, 20, name );
	label->setPosition( 35, ( item->getHeight() - Importer::StringHeight( label->getFont() ) ) / 2.0f );
	label->setClickable( false );

	return item;
}

void MatchMakerMenu::btnMainJoinMatchHandler( BaseObject * )
{
	matchmakermenu->lstMainPlayers->clear();

	if( !matchmakermenu->lstMainGametype->getNbSelectedItem() )
	{
		matchmakermenu->lstMainPlayers->setEnabled( false );
		matchmakermenu->btnMainDrop->setEnabled( false );
		return;
	}

	Trap::MM_UIRequest( ACTION_JOIN, gametype_names[matchmakermenu->lstMainGametype->getSelectedPosition()][1] );
}

void MatchMakerMenu::addMatchCancelHandler( void )
{
	matchmakermenu->lstMainGametype->clearSelection();
}

void MatchMakerMenu::lstMainPlayersHandler( ListItem *item, int, bool )
{
	// we dont want users to be able to click on a player
	item->setPressed( !item->isPressed() );
}

MatchMakerMenu::MatchMakerMenu( void )
{
	// basic panel
	panel = Factory::newPanel( rootPanel, 20, 20, 760, 560 );
	panel->setBorderColor( transparent );
	panel->setBackColor( transparent );
	btnBack = Factory::newButton( panel, 0, 0, 120, 40, "Back" );
	btnBack->setClickHandler( btnBackHandler );

	// tab panels
	pMain = Factory::newPanel( panel, 0, 40, 760, 520 );
	pCups = Factory::newPanel( panel, 0, 40, 760, 520 );
	// tab buttons
	btnTabs[0] = Factory::newSwitchButton( panel, 520, 0, 120, 40, "Main" );
	btnTabs[1] = Factory::newSwitchButton( panel, 640, 0, 120, 40, "Cups" );
	// set click event handler
	for( int i = 0 ; i < MatchMakerMenu_TabCount ; i++ )
		btnTabs[i]->setSwitchHandler( tabsHandler );

	// main tab
	lblMainTitle = Factory::newLabel( pMain, 20, 20, 300, 20, "Matchmaker" );
	lblMainTitle->setFont( Local::getFontBig() );

	grpMainGametype = Factory::newPanel( pMain, 25, 45, 405, 30 );
	grpMainGametype->setBackColor( Color( 0, 0, 0, 0 ) );
	grpMainGametype->setDisabledColor( Color( 0, 0, 0, 0 ) );
	grpMainGametype->setBorderWidth( 0 );

	lblMainGametype = Factory::newLabel( grpMainGametype, 0, 5, 100, 20, "Gametype" );
	lblMainGametype->setAlign( ALIGN_MIDDLE_LEFT );
	lstMainGametype = Factory::newDropDownBox( pMain, 100, 50, 200, 20, 75, "Select to begin ..." );
	for( int i = 0 ; i < GAMETYPE_NB ; i++ )
		Factory::newListItem( lstMainGametype, gametype_names[i][0] );
	btnMainJoinMatch = Factory::newButton( grpMainGametype, 295, 0, 110, 30, "Join match" );
	btnMainJoinMatch->setClickHandler( btnMainJoinMatchHandler );

	// this will contain all the controls that are only enabled when we are in a match
	// for easy enabling and disabling
	grpMainMatch = Factory::newPanel( pMain, 20, 75, 760, 250 );
	grpMainMatch->setBackColor( Color( 0, 0, 0, 0 ) );
	grpMainMatch->setDisabledColor( Color( 0, 0, 0, 0 ) );
	grpMainMatch->setBorderWidth( 0 );

	lblMainPlayers = Factory::newLabel( grpMainMatch, 5, 0, 200, 20, "Players" );
	lstMainPlayers = Factory::newListBox( grpMainMatch, 0, 20, 230, 230, false );
	lstMainPlayers->setItemSize( 46 );
	lstMainPlayers->setItemSelectedHandler( lstMainPlayersHandler );

	lblMainMatchDetails = Factory::newLabel( grpMainMatch, 240, 20, 100, 20, "Match details:" );
	lblMainMatchDetails->setFont( Local::getFontMedium() );
	lblMainSlots = Factory::newLabel( grpMainMatch, 240, 45, 100, 20 );
	lblMainTimelimit = Factory::newLabel( grpMainMatch, 240, 62, 100, 20 );
	lblMainScorelimit = Factory::newLabel( grpMainMatch, 240,79, 100, 20 );

	btnMainDrop = Factory::newButton( grpMainMatch, 520, 220, 120, 30, "Leave match" );
	btnMainDrop->setClickHandler( btnMainDropHandler );

	lblMainChannels = Factory::newLabel( pMain, 25, 345, 100, 20, "Channels" );
	lstMainChannels = Factory::newListBox( pMain, 20, 365, 200, 101, false );
	lblMainChatMsgs = Factory::newLabel( pMain, 245, 345, 100, 20, "Chat messages" );
	lstMainChatMsgs = Factory::newListBox( pMain, 240, 365, 500, 101, false );
	txtMainChatMsg = Factory::newTextBox( pMain, 240, 476, 420, 24, 100 );
	btnMainChatMsg = Factory::newButton( pMain, 670, 476, 70, 24, "Send" );

	pMain->PutOnTop( lstMainGametype );

	// cups tab
	lblCupsTitle = Factory::newLabel( pCups, 20, 20, 300, 20, "Matchmaker cups" );
	lblCupsTitle->setFont( Local::getFontBig() );

	lblCupsTest = Factory::newLabel( pCups, 510, 150, 200, 200, "This is just a lot of random text that I'm using to test the new multiline feature of labels. I hope it works\nSigned: Will" );
	lblCupsTest->setMultiline( true );
	lblCupsTest->setBorderWidth( 1 );

	messageBox = Factory::newMessageBox();

	addMatch = new AddMatchMenu;
	addMatch->setCancelCallback( addMatchCancelHandler );
}

void MatchMakerMenu::Show( void )
{
	setActiveMenu( this );
	panel->setVisible( true );

	tabsHandler( btnTabs[0], true );

	lstMainGametype->setEnabled( true );
	lstMainGametype->showList( false );
	lstMainGametype->clearSelection();
	lstMainChannels->clear();
	lstMainPlayers->clear();
	grpMainMatch->setEnabled( false );
	grpMainGametype->setEnabled( true );
}

void MatchMakerMenu::Hide( void )
{
	panel->setVisible( false );

	Trap::MM_UIRequest( ACTION_DROP, "" );

	messageBox->Hide();
	addMatch->Hide();
}

void MatchMakerMenu::MM_UIReply( MatchMakerAction action, const char *d )
{
	MessageBox *msg = matchmakermenu->messageBox;
	char *c;
	char *original, *data;
	
	original = data = UI_CopyString( d );

	c = COM_Parse( &data );

	switch( action )
	{
	case ACTION_ADDMATCH:
		if( !strcmp( c, "adding" ) )
		{
			matchmakermenu->addMatch->Hide();
			msg->updateDisplay( "Adding match to matchmaker...", BUTTON_CANCEL, IMAGE_LOADING );
			msg->Show();
			msg->setResultHandler( matchesHandler );
		}
		else
			msg->Hide();
		break;

	case ACTION_JOIN:
		// no matches available to join
		if( !strcmp( c, "nomatches" ) )
		{
			msg->updateDisplay( "No matches found for this gametype. Create a match?", BUTTON_YES | BUTTON_NO, IMAGE_WARNING );
			msg->Show();
			msg->setResultHandler( matchesHandler );
		}

		// joining match
		if( !strcmp( c, "joining" ) )
		{
			msg->updateDisplay( "Finding and joining match...", BUTTON_CANCEL, IMAGE_LOADING );
			msg->Show();
			msg->setResultHandler( joiningHandler );
		}

		// failed to join
		if( !strcmp( c, "failed" ) )
		{
			msg->updateDisplay( "Connection to matchmaker failed.", BUTTON_RETRY | BUTTON_CANCEL, IMAGE_WARNING );
			msg->Show();
			msg->setResultHandler( joiningHandler );
		}

		// successfully joined match
		if( !strcmp( c, "joined" ) )
		{
			int maxclients, scorelimit, skilllevel;
			float timelimit;
			char *gametype, *skilltype;

			maxclients = atoi( COM_Parse( &data ) );
			gametype = COM_Parse( &data );
			scorelimit = atoi( COM_Parse( &data ) );
			timelimit = atof( COM_Parse( &data ) );
			skilltype = COM_Parse( &data );
			skilllevel = atoi( COM_Parse( &data ) );
			// store for later use
			clientslot = atoi( COM_Parse( &data ) );

			matchmakermenu->lblMainSlots->setCaption( va( "%d slots", maxclients ) );

			if( scorelimit )
				matchmakermenu->lblMainScorelimit->setCaption( va( "Scorelimit: %d", scorelimit ) );
			else
				matchmakermenu->lblMainScorelimit->setCaption( "No scorelimit" );

			if( timelimit )
				matchmakermenu->lblMainTimelimit->setCaption( va( "Timelimit: %.2f minutes", timelimit ) );
			else
				matchmakermenu->lblMainTimelimit->setCaption( "No timelimit" );

			matchmakermenu->lstMainPlayers->clear();

			for( int i = 0 ; i < maxclients ; i++ )
				matchmakermenu->CreatePlayerItem();

			matchmakermenu->grpMainMatch->setEnabled( true );
			matchmakermenu->grpMainGametype->setEnabled( false );
			matchmakermenu->lstMainGametype->setEnabled( false );

			msg->Hide();
		}

		// add a new client to the current match
		if( !strcmp( c, "add" ) )
		{
			int slot;
			char *nickname;
			ListItem *item;

			slot = atoi( COM_Parse( &data ) );
			nickname = COM_Parse( &data );

			matchmakermenu->lstMainPlayers->removeItem( slot );
			item = matchmakermenu->CreatePlayerItem( nickname, slot );
			if( slot == clientslot )
				item->setPressed( true );
		}

		break;

	case ACTION_DROP:
		int slot;

		// drop from the current match
		if( !strlen( d ) )
		{
			matchmakermenu->grpMainMatch->setEnabled( false );
			matchmakermenu->grpMainGametype->setEnabled( true );
			matchmakermenu->lstMainGametype->setEnabled( true );
			matchmakermenu->lstMainPlayers->clear();
			matchmakermenu->lstMainGametype->clearSelection();
			break;
		}

		// drop another client from the current match
		slot = atoi( c );
		matchmakermenu->lstMainPlayers->removeItem( slot );
		matchmakermenu->CreatePlayerItem( "", slot );
		break;
	default:
		break;
	}

	UIMem::Free( original );
}
