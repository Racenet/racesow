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
#include "uimenu_SetupMenu.h"
#include "uiwsw_Export.h"
#include "uiwsw_SysCalls.h"
#include "uiwsw_Utils.h"
#include "uimenu_SetupPlayerMenu.h"
#include "uimenu_SetupSoundMenu.h"
#include "uimenu_SetupGraphicsMenu.h"

using namespace UICore;
using namespace UIMenu;
using namespace UIWsw;

extern SetupPlayerMenu *setupplayer;
extern SetupSoundMenu *setupsounds;
extern SetupGraphicsMenu *setupgraphics;

SetupMenu *setupmenu = NULL;

void UIMenu::M_Menu_Setup_f( void )
{
	if ( !setupmenu )
		setupmenu = new SetupMenu();

	setupmenu->Show();
}

void SetupMenu::playerHandler( BaseObject* )
{
	setupmenu->currentSubPanel->setVisible( false );
	Trap::Cmd_ExecuteText( EXEC_APPEND, "menu_playersetup" );
}

void SetupMenu::controllerHandler( BaseObject* )
{
	
}

void SetupMenu::graphicsHandler( BaseObject* )
{
	setupmenu->currentSubPanel->setVisible( false );
	Trap::Cmd_ExecuteText( EXEC_APPEND, "menu_graphics" );
}

void SetupMenu::soundsHandler( BaseObject* )
{
	setupmenu->currentSubPanel->setVisible( false );
	Trap::Cmd_ExecuteText( EXEC_APPEND, "menu_sound" );
}

void SetupMenu::ircHandler( BaseObject* )
{
	
}

void SetupMenu::mainmenuHandler( BaseObject* )
{
	if ( setupplayer )
		setupplayer->UpdatePlayerConfig();
	if ( setupsounds )
		setupsounds->UpdateSoundConfig();
	if ( setupgraphics )
		setupgraphics->UpdateGraphicsConfig();

	Trap::Cmd_ExecuteText( EXEC_APPEND, "menu_main" );
}

ALLOCATOR_DEFINITION(SetupMenu)
DELETER_DEFINITION(SetupMenu)

SetupMenu::SetupMenu() : MainMenu( "Game Settings" )
{
	int yoffset = 0;

	panel = Factory::newPanel( contentPanel, 0, 0, contentPanel->getWidth(), contentPanel->getHeight() );
	panel->setBorderWidth( 1 );

	player = Factory::newButton( panel, 20, yoffset += 150, 150, 30, "PLAYER" );
	player->setClickHandler( playerHandler );
	controller = Factory::newButton( panel, 20, yoffset += 40, 150, 30, "CONTROLLER" );
	controller->setClickHandler( controllerHandler );
	graphics = Factory::newButton( panel, 20, yoffset += 40, 150, 30, "GRAPHICS" );
	graphics->setClickHandler( graphicsHandler );
	sounds = Factory::newButton( panel, 20, yoffset += 40, 150, 30, "SOUNDS" );
	sounds->setClickHandler( soundsHandler );
	irc = Factory::newButton( panel, 20, yoffset += 40, 150, 30, "IRC" );
	irc->setClickHandler( ircHandler );
	mainmenu = Factory::newButton( panel, 20, yoffset += 60, 150, 30, "MAIN MENU" );
	mainmenu->setClickHandler( mainmenuHandler );

	subpanel = Factory::newPanel( panel, 200, 20, 540, 520 );
	selectcat = Factory::newLabel( subpanel, 0, 250, 540, 20, "Select a category" );
	selectcat->setAlign( ALIGN_MIDDLE_CENTER );

	currentSubPanel = subpanel;
}

void SetupMenu::Show( void )
{
	currentSubPanel = subpanel;

	setActiveMenu( this );
	panel->setVisible( true );

	MainMenu::Show();
}

void SetupMenu::Hide( void )
{
	MainMenu::Hide();

	panel->setVisible( false );
}
