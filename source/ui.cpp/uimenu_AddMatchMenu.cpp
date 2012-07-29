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
#include "uimenu_AddMatchMenu.h"
#include "uiwsw_Export.h"
#include "uiwsw_SysCalls.h"
#include "uiwsw_Utils.h"

using namespace UICore;
using namespace UIMenu;
using namespace UIWsw;

AddMatchMenu *addmatchmenu;

void AddMatchMenu::btnAddHandler( BaseObject * )
{
	int players, scorelimit;
	float timelimit;
	char *skilltype;

	if( !addmatchmenu->lstGametype->getNbSelectedItem() )
		return;

	players = atoi( addmatchmenu->txtNoOfPlayers->getText() );
	scorelimit = atoi( addmatchmenu->txtScorelimit->getText() );
	timelimit = atof( addmatchmenu->txtTimelimit->getText() );

	skilltype = ( char* )(addmatchmenu->btnAllSkills->isPressed() ? "all" : "dependent");

	Trap::MM_UIRequest( ACTION_ADDMATCH, va( "%s %d %d %.2f %s",
		gametype_names[addmatchmenu->lstGametype->getSelectedPosition()][1],
		players, scorelimit, timelimit, skilltype ) );
}

void AddMatchMenu::btnBackHandler( BaseObject * )
{
	if( addmatchmenu->cancelCallback )
		addmatchmenu->cancelCallback();
	addmatchmenu->Hide();
}

void AddMatchMenu::skillsHandler( BaseObject *btn, bool )
{
	static_cast<SwitchButton *>( btn )->setPressed( true );

	if( btn == addmatchmenu->btnAllSkills )
		addmatchmenu->btnDependent->setPressed( false );
	else
		addmatchmenu->btnAllSkills->setPressed( false );
}

AddMatchMenu::AddMatchMenu( void )
{
	pBackground = Factory::newPanel( rootPanel, 0, 0, 800, 600 );
	pBackground->setBorderWidth( 0 );
	pBackground->setVisible( false );

	panel = Factory::newPanel( pBackground, 225, 185, 350, 230 );

	lblTitle = Factory::newLabel( panel, 20, 20, 200, 20, "Add match" );
	lblTitle->setFont( Local::getFontBig() );

	lblGametype = Factory::newLabel( panel, 20, 50, 90, 20, "Gametype:" );
	lblGametype->setAlign( ALIGN_MIDDLE_LEFT );
	lstGametype = Factory::newDropDownBox( panel, 100, 50, 200, 20, 75 );
	for( int i = 0 ; i < GAMETYPE_NB ; i++ )
		Factory::newListItem( lstGametype, gametype_names[i][0] );

	lblNoOfPlayers = Factory::newLabel( panel, 20, 75, 120, 20, "No. of players:" );
	lblNoOfPlayers->setAlign( ALIGN_MIDDLE_LEFT );
	txtNoOfPlayers = Factory::newTextBox( panel, 140, 75, 25, 20, 3 );
	txtNoOfPlayers->setFlags( TEXTBOX_NUMBERSONLY );

	lblTimelimit = Factory::newLabel( panel, 20, 100, 120, 20, "Timelimit:" );
	lblTimelimit->setAlign( ALIGN_MIDDLE_LEFT );
	txtTimelimit = Factory::newTextBox( panel, 140, 100, 50, 20, 6 );
	txtTimelimit->setFlags( TEXTBOX_NUMBERSONLY );

	lblScorelimit = Factory::newLabel( panel, 20, 125, 120, 20, "Scorelimit:" );
	lblScorelimit->setAlign( ALIGN_MIDDLE_LEFT );
	txtScorelimit = Factory::newTextBox( panel, 140, 125, 25, 20, 3 );
	txtScorelimit->setFlags( TEXTBOX_NUMBERSONLY );

	lblSkill = Factory::newLabel( panel, 20, 150, 100, 20, "Skill type:" );
	lblSkill->setAlign( ALIGN_MIDDLE_LEFT );
	btnAllSkills = Factory::newSwitchButton( panel, 120, 150, 50, 20, "All" );
	btnAllSkills->setSwitchHandler( AddMatchMenu::skillsHandler );
	btnAllSkills->setPressed( true );
	btnDependent = Factory::newSwitchButton( panel, 175, 150, 100, 20, "Dependent" );
	btnDependent->setSwitchHandler( AddMatchMenu::skillsHandler );

	btnAdd = Factory::newButton( panel, 20, 180, 100, 30, "Add match" );
	btnAdd->setClickHandler( AddMatchMenu::btnAddHandler );

	btnBack = Factory::newButton( panel, 130, 180, 80, 30, "Cancel" );
	btnBack->setClickHandler( AddMatchMenu::btnBackHandler );

#if 0
	panel->PutOnTop( lstGametype );
#endif

	messageBox = Factory::newMessageBox();

	setCancelCallback( NULL );
}

ALLOCATOR_DEFINITION( AddMatchMenu );
DELETER_DEFINITION( AddMatchMenu );

void AddMatchMenu::Show( void )
{
	pBackground->setVisible( true );
	addmatchmenu = this;

	lstGametype->showList( false );
	txtNoOfPlayers->ClearText();
	txtTimelimit->ClearText();
	txtScorelimit->ClearText();
	btnAllSkills->setPressed( true );
	btnDependent->setPressed( false );
}

void AddMatchMenu::Hide( void )
{
	pBackground->setVisible( false );
	addmatchmenu = NULL;

	messageBox->Hide();
}

void AddMatchMenu::setGametype( int index )
{
	addmatchmenu->lstGametype->selectItem( index );
}
