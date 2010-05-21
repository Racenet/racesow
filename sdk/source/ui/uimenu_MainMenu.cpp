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
#include "uimenu_MainMenu.h"
#include "uiwsw_Export.h"
#include "uiwsw_SysCalls.h"
#include "uiwsw_Utils.h"

using namespace UICore;
using namespace UIMenu;
using namespace UIWsw;

MainMenu *mainmenu = NULL;

void UIMenu::M_Menu_Main_f( void )
{
	if ( !mainmenu )
		mainmenu = new MainMenu();

	mainmenu->Show();
}

void MainMenu::joinHandler( BaseObject* )
{
	Trap::Cmd_ExecuteText( EXEC_APPEND, "menu_joinserver" );
}

void MainMenu::startHandler( BaseObject* )
{
	Trap::Cmd_ExecuteText( EXEC_APPEND, "menu_startserver" );
}

void MainMenu::matchmakerHandler( BaseObject* )
{
  Trap::Cmd_ExecuteText( EXEC_APPEND, "menu_matchmaker" );
}

void MainMenu::setupHandler( BaseObject* )
{
	Trap::Cmd_ExecuteText( EXEC_APPEND, "menu_setup" );
}

void MainMenu::demosHandler( BaseObject* )
{
	Trap::Cmd_ExecuteText( EXEC_APPEND, "menu_demos" );
}

void MainMenu::modsHandler( BaseObject* )
{
	Trap::Cmd_ExecuteText( EXEC_APPEND, "menu_mods" );
}

void MainMenu::consoleHandler( BaseObject* )
{
	if ( Local::getClientState() > CA_DISCONNECTED ) {
		UI_ForceMenuOff();
	}

	Trap::Cmd_ExecuteText( EXEC_APPEND, "toggleconsole" );	
}

void MainMenu::quitHandler( BaseObject* )
{
	Trap::Cmd_ExecuteText( EXEC_APPEND, "menu_quit" );
}

ALLOCATOR_DEFINITION(MainMenu)
DELETER_DEFINITION(MainMenu)

MainMenu::MainMenu()
{
	int yoffset = 0;
	panel = Factory::newPanel( rootPanel, 200, 120, 400, 430 );
	title = Factory::newLabel( panel, 0, yoffset += 10, 400, 20, "Main menu" );
	title->setFont( Local::getFontBig() );
	title->setAlign( ALIGN_MIDDLE_CENTER );

	joinserver = Factory::newButton( panel, 50, yoffset += 40, 300, 30, "Join server" );
	joinserver->setClickHandler( joinHandler );
	startserver = Factory::newButton( panel, 50, yoffset += 40, 300, 30, "Start server" );
	startserver->setClickHandler( startHandler );
	matchmaker = Factory::newButton( panel, 50, yoffset += 40, 300, 30, "Matchmaker" );
	matchmaker->setClickHandler( matchmakerHandler );
	setup = Factory::newButton( panel, 50, yoffset += 60, 300, 30, "Setup" );
	setup->setClickHandler( setupHandler );
	demos = Factory::newButton( panel, 50, yoffset += 40, 300, 30, "Demos" );
	demos->setClickHandler( demosHandler );
	mods = Factory::newButton( panel, 50, yoffset += 40, 300, 30, "Mods" );
	mods->setClickHandler( modsHandler );
	console = Factory::newButton( panel, 50, yoffset += 60, 300, 30, "Console" );
	console->setClickHandler( consoleHandler );
	quit = Factory::newButton( panel, 50, yoffset += 40, 300, 30, "Quit Warsow" );
	quit->setClickHandler( quitHandler );
}

void MainMenu::Show( void )
{
	setActiveMenu( this );
	panel->setVisible( true );
}

void MainMenu::Hide( void )
{
	panel->setVisible( false );
}
