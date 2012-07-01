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
#include "uimenu_ForceMenu.h"
#include "uiwsw_Export.h"
#include "uiwsw_SysCalls.h"
#include "uiwsw_Utils.h"

using namespace UICore;
using namespace UIMenu;
using namespace UIWsw;

ForceMenu *forcemenu = NULL;

void UIMenu::M_Menu_Force_f( void )
{
	if ( !forcemenu )
		forcemenu = new ForceMenu();

	forcemenu->Show();
}

void ForceMenu::specHandler( BaseObject* )
{
	Trap::Cmd_ExecuteText( EXEC_APPEND, "spec" );
}

void ForceMenu::mainHandler( BaseObject* )
{
	Trap::Cmd_ExecuteText( EXEC_APPEND, "menu_main" );
}

void ForceMenu::backHandler( BaseObject* )
{
	setActiveMenu( NULL );
}

void ForceMenu::joinHandler( BaseObject* )
{
	Trap::Cmd_ExecuteText( EXEC_APPEND, "join" );
	setActiveMenu( NULL );
}


ALLOCATOR_DEFINITION(ForceMenu)
DELETER_DEFINITION(ForceMenu)

ForceMenu::ForceMenu()
{
	int yoffset = 0;
	panel = Factory::newPanel( rootPanel, 200, 120, 400, 430 );
	title = Factory::newLabel( panel, 0, yoffset += 10, 400, 20, "Game Menu" );
	title->setFont( Local::getFontBig() );
	title->setAlign( ALIGN_MIDDLE_CENTER );

	join = Factory::newButton( panel, 50, yoffset += 40, 300, 30, "Join game" );
	join->setClickHandler( joinHandler );

	spectate = Factory::newButton( panel, 50, yoffset += 40, 300, 30, "Spectate" );
	spectate->setClickHandler( specHandler );

	mainmenu = Factory::newButton( panel, 50, yoffset += 40, 300, 30, "Main menu" );
	mainmenu->setClickHandler( mainHandler );

	back = Factory::newButton( panel, 50, yoffset += 40, 300, 30, "Back" );
	back->setClickHandler( backHandler );	

}

void ForceMenu::Show( void )
{
	setActiveMenu( this );
	panel->setVisible( true );
}

void ForceMenu::Hide( void )
{
	panel->setVisible( false );
}
