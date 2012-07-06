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
#include "uimenu_QuitMenu.h"
#include "uiwsw_Export.h"
#include "uiwsw_SysCalls.h"
#include "uiwsw_Utils.h"

using namespace UICore;
using namespace UIMenu;
using namespace UIWsw;

QuitMenu *quitmenu = NULL;

void UIMenu::M_Menu_Quit_f( void )
{
	if ( !quitmenu )
		quitmenu = new QuitMenu();

	quitmenu->Show();
}

void QuitMenu::yesHandler( BaseObject* )
{
	Trap::CL_SetKeyDest( key_console );
	Trap::CL_Quit();
}

void QuitMenu::noHandler( BaseObject* )
{
	Trap::Cmd_ExecuteText( EXEC_APPEND, "menu_main" );
}

ALLOCATOR_DEFINITION(QuitMenu)
DELETER_DEFINITION(QuitMenu)

QuitMenu::QuitMenu()
{
	panel = Factory::newPanel( rootPanel, 250, 200, 300, 200 );

	msg = Factory::newLabel( panel, 0, 50, 300, 20, "Are you sure?" );
	msg->setFont( Local::getFontBig() );
	msg->setAlign( ALIGN_MIDDLE_CENTER );

	yes = Factory::newButton( panel, 50, 130, 80, 30, "YES" );
	yes->setClickHandler( yesHandler );

	no = Factory::newButton( panel, 170, 130, 80, 30, "NO" );
	no->setClickHandler( noHandler );
}

void QuitMenu::Show( void )
{
	setActiveMenu( this );
	panel->setVisible( true );
}

void QuitMenu::Hide( void )
{
	panel->setVisible( false );
}
