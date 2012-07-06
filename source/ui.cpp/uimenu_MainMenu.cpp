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
		mainmenu = new MainMenu( "", true );
	mainmenu->Show();
}

void MainMenu::joinBtnHandler( BaseObject* )
{
	Trap::Cmd_ExecuteText( EXEC_APPEND, "menu_joinserver" );
}

void MainMenu::startBtnHandler( BaseObject* )
{
	Trap::Cmd_ExecuteText( EXEC_APPEND, "menu_startserver" );
}

void MainMenu::setupBtnHandler( BaseObject* )
{
	Trap::Cmd_ExecuteText( EXEC_APPEND, "menu_setup" );
}

void MainMenu::demosBtnHandler( BaseObject* )
{
	Trap::Cmd_ExecuteText( EXEC_APPEND, "menu_demos" );
}

void MainMenu::modsBtnHandler( BaseObject* )
{
	Trap::Cmd_ExecuteText( EXEC_APPEND, "menu_mods" );
}

void MainMenu::consoleBtnHandler( BaseObject* )
{
	if ( Local::getClientState() > CA_DISCONNECTED ) {
		UI_ForceMenuOff();
	}

	Trap::Cmd_ExecuteText( EXEC_APPEND, "toggleconsole" );	
}

void MainMenu::quitBtnHandler( BaseObject* )
{
	Trap::Cmd_ExecuteText( EXEC_APPEND, "menu_quit" );
}

ALLOCATOR_DEFINITION(MainMenu)
DELETER_DEFINITION(MainMenu)

MainMenu::MainMenu( std::string caption, bool isParentMain )
{
	int xoffset = 0, yoffset = 0;
	int menubtnheight = 30;

	panel_ = Factory::newPanel( rootPanel, 0, 0, 930, 674 );
	panel_->setPosition( (panel_->getParent()->getWidth()-panel_->getWidth())/2, (panel_->getParent()->getHeight()-panel_->getHeight())/2 );
	panel_->setBorderWidth( 0 );
	panel_->setBackgroundImage( NULL );
	panel_->setBackColor( UIMenu::transparent );

	UICore::Image mainBorders[UICore::BORDER_MAX_BORDERS];
	mainBorders[UICore::BORDER_TOP_LEFT] = UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_mainmenu_corner" );
	mainBorders[UICore::BORDER_TOP] = UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_mainmenu_border_top" );
	mainBorders[UICore::BORDER_TOP_RIGHT] = UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_mainmenu_corner_right" );
	mainBorders[UICore::BORDER_RIGHT] = UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_mainmenu_border_right" );
	mainBorders[UICore::BORDER_BOTTOM_RIGHT] = UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_mainmenu_corner_right_bottom" );
	mainBorders[UICore::BORDER_BOTTOM] = UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_mainmenu_border_bottom" );
	mainBorders[UICore::BORDER_BOTTOM_LEFT] = UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_mainmenu_corner_left_bottom" );
	mainBorders[UICore::BORDER_LEFT] = UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_mainmenu_border_left" );

	menuPanel = Factory::newPanel( panel_, 0, 0, panel_->getWidth(), 82 );
	menuPanel->setPosition( 0, menuPanel->getParent()->getHeight()-menuPanel->getHeight() );
	menuPanel->setBackgroundImage( UIWsw::Trap::R_RegisterPic( "gfx/newui/ui_mainmenu_bg" ) );
	menuPanel->setBackColor( UIMenu::white );
	menuPanel->setBorderImages( mainBorders );
	menuPanel->setBorderColor( UIMenu::white );
	menuPanel->setBorderWidth( 12 );

	mainPanel = Factory::newPanel( panel_, 0, 0, panel_->getWidth(), panel_->getHeight() - (menuPanel->getHeight() + 8) );
	mainPanel->setPosition( 0, 0 );

	titleBar = Factory::newTitleBar( mainPanel, 0, 0, caption );
	titleBar->setPosition( (titleBar->getParent()->getWidth()-titleBar->getWidth())/2, 16 );

	contentPanel = Factory::newPanel( mainPanel, 0, 0, titleBar->getWidth(), mainPanel->getHeight() - (titleBar->getHeight() + titleBar->getPositionY() + 16) - 20 );
	contentPanel->setPosition( (contentPanel->getParent()->getWidth()-contentPanel->getWidth())/2, titleBar->getHeight() + titleBar->getPositionY() + 16 );
	contentPanel->setBackgroundImage( NULL );
	contentPanel->setBackColor( UIMenu::transparent );
	contentPanel->setBorderWidth( 0 );

	xoffset = 440;
	yoffset = (menuPanel->getHeight() - menubtnheight) / 2;

	joinserverBtn = Factory::newButton( menuPanel, xoffset, yoffset, 58, 30, "Join" );
	joinserverBtn->setClickHandler( joinBtnHandler );
	joinserverBtn->setBackgroundImage( NULL );
	joinserverBtn->setBackColor( UIMenu::transparent );
	joinserverBtn->setBorderWidth( 0 );
	joinserverBtn->setHighlightColor( UIMenu::transparent );
	joinserverBtn->setFont( UIWsw::Local::getFontMedium() );
	joinserverBtn->setHighlightFontColor( UIMenu::orange );

	startserverBtn = Factory::newButton( menuPanel, xoffset += joinserverBtn->getWidth(), yoffset, 110, 30, "Local Game" );
	startserverBtn->setClickHandler( startBtnHandler );
	startserverBtn->setBackgroundImage( NULL );
	startserverBtn->setBackColor( UIMenu::transparent );
	startserverBtn->setBorderWidth( 0 );
	startserverBtn->setHighlightColor( UIMenu::transparent );
	startserverBtn->setFont( UIWsw::Local::getFontMedium() );
	startserverBtn->setHighlightFontColor( UIMenu::orange );

	setupBtn = Factory::newButton( menuPanel, xoffset += startserverBtn->getWidth(), yoffset, 64, 30, "Setup" );
	setupBtn->setClickHandler( setupBtnHandler );
	setupBtn->setBackgroundImage( NULL );
	setupBtn->setBackColor( UIMenu::transparent );
	setupBtn->setBorderWidth( 0 );
	setupBtn->setHighlightColor( UIMenu::transparent );
	setupBtn->setFont( UIWsw::Local::getFontMedium() );
	setupBtn->setHighlightFontColor( UIMenu::orange );

	demosBtn = Factory::newButton( menuPanel, xoffset += setupBtn->getWidth(), yoffset, 64, 30, "Demos" );
	demosBtn->setClickHandler( demosBtnHandler );
	demosBtn->setBackgroundImage( NULL );
	demosBtn->setBackColor( UIMenu::transparent );
	demosBtn->setBorderWidth( 0 );
	demosBtn->setHighlightColor( UIMenu::transparent );
	demosBtn->setFont( UIWsw::Local::getFontMedium() );
	demosBtn->setHighlightFontColor( UIMenu::orange );

/*
	modsBtn = Factory::newButton( menuPanel, xoffset += demosBtn->getWidth(), yoffset, 58, 30, "Mods" );
	modsBtn->setClickHandler( modsBtnHandler );
	modsBtn->setBackgroundImage( NULL );
	modsBtn->setBackColor( UIMenu::transparent );
	modsBtn->setBorderWidth( 0 );
	modsBtn->setHighlightColor( UIMenu::transparent );
	modsBtn->setFont( UIWsw::Local::getFontMedium() );
	modsBtn->setHighlightFontColor( UIMenu::orange );
*/

	consoleBtn = Factory::newButton( menuPanel, xoffset += demosBtn->getWidth(), yoffset, 78, 30, "Console" );
	consoleBtn->setClickHandler( consoleBtnHandler );
	consoleBtn->setBackgroundImage( NULL );
	consoleBtn->setBackColor( UIMenu::transparent );
	consoleBtn->setBorderWidth( 0 );
	consoleBtn->setHighlightColor( UIMenu::transparent );
	consoleBtn->setFont( UIWsw::Local::getFontMedium() );
	consoleBtn->setHighlightFontColor( UIMenu::orange );

	quitBtn = Factory::newButton( menuPanel, xoffset += consoleBtn->getWidth(), yoffset, 58, 30, "Quit" );
	quitBtn->setClickHandler( quitBtnHandler );
	quitBtn->setBackgroundImage( NULL );
	quitBtn->setBackColor( UIMenu::transparent );
	quitBtn->setBorderWidth( 0 );
	quitBtn->setHighlightColor( UIMenu::transparent );
	quitBtn->setFont( UIWsw::Local::getFontMedium() );
	quitBtn->setHighlightFontColor( UIMenu::orange );

	this->isParentMain = isParentMain;
}

void MainMenu::Show( void )
{
	if( isParentMain )
		setActiveMenu( this );
	panel_->setVisible( true );
}

void MainMenu::Hide( void )
{
	panel_->setVisible( false );
}
