/*
Copyright (C) 2008 Will Franklin

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
#include "uimenu_LoginMenu.h"
#include "uiwsw_Export.h"
#include "uiwsw_SysCalls.h"
#include "uiwsw_Utils.h"

using namespace UICore;
using namespace UIMenu;
using namespace UIWsw;

LoginMenu *loginmenu = NULL;

void UIMenu::M_Menu_Login_f( void )
{
	if( !loginmenu )
		loginmenu = new LoginMenu;

	loginmenu->setPopup( strcmp( Trap::Cmd_Argv( 1 ), "popup" ) == 0 );
	loginmenu->Show();
}

void LoginMenu::setPopup( bool p )
{
	popup = p;

	panel->setParent( popup ? pBackground : rootPanel );
}

LoginMenu::LoginMenu()
{
	pBackground = Factory::newPanel( rootPanel, 0, 0, REL_WIDTH, REL_HEIGHT );
	pBackground->setBorderWidth( 0 );
	pBackground->setVisible( false );
	panel = Factory::newPanel( rootPanel, 250, 200, 300, 200 );
	lblTitle = Factory::newLabel( panel, 0, 10, 300, 20, "Login" );
	lblTitle->setAlign( ALIGN_MIDDLE_CENTER );
	lblTitle->setFont( Local::getFontBig() );
	lblUser = Factory::newLabel( panel, 10, 45, 100, 20, "Username:" );
	lblPass = Factory::newLabel( panel, 10, 70, 100, 20, "Password:" );
	txtUser = Factory::newTextBox( panel, 110, 45, 180, 20, 30 );
	txtPass = Factory::newTextBox( panel, 110, 70, 180, 20, 30 );

	btnLogin = Factory::newButton( panel, 10, 100, 80, 30, "Login" );
	btnLogin->setClickHandler( buttonHandler );
	btnCancel = Factory::newButton( panel, 100, 100, 80, 30, "Cancel" );
	btnCancel->setClickHandler( buttonHandler );
}

ALLOCATOR_DEFINITION( LoginMenu );
DELETER_DEFINITION( LoginMenu );

void LoginMenu::Show( void )
{
	if( popup )
		pBackground->setVisible( true );
	else
		setActiveMenu( this );

	panel->setVisible( true );
}

void LoginMenu::Hide( void )
{
	if( popup )
		pBackground->setVisible( false );

	panel->setVisible( false );

	if( resultHandler )
		resultHandler( LOGIN_FAILED );
}

void LoginMenu::buttonHandler( BaseObject *obj )
{
	if( loginmenu->resultHandler )
	{
		if( loginmenu->btnCancel == obj )
			loginmenu->resultHandler( LOGIN_FAILED );
		else
			loginmenu->resultHandler( LOGIN_SUCCESS );

		loginmenu->resultHandler = NULL;
	}

	loginmenu->Hide();

	if( !loginmenu->popup )
		Trap::Cmd_ExecuteText( EXEC_APPEND, "menu_main" );
}
