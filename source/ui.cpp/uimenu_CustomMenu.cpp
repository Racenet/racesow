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
#include "uimenu_CustomMenu.h"
#include "uiwsw_Export.h"
#include "uiwsw_SysCalls.h"
#include "uiwsw_Utils.h"

using namespace UICore;
using namespace UIMenu;
using namespace UIWsw;

CustomMenu *custommenu = NULL;

void UIMenu::M_Menu_Custom_f( void )
{
	if( !custommenu )
		custommenu = new CustomMenu;

	custommenu->Show();
}

ALLOCATOR_DEFINITION( CustomMenu )
DELETER_DEFINITION( CustomMenu )

CustomMenu::CustomMenu( void )
{
	panel = Factory::newPanel( rootPanel, 0, 0, 0, 0 );
	title = Factory::newLabel( panel, 0, 20, 0, 0 );
	title->setFont( Local::getFontBig() );
	title->setAlign( ALIGN_TOP_CENTER );
}

void CustomMenu::Show( void )
{
	int maxw;

	setActiveMenu( this );

	if( !buttons.empty() )
	{
		for( std::list<ButtonPair>::iterator it = buttons.begin() ; it != buttons.end() ; it++ )
		{
			delete (*it).first;
			(*it).second.clear();
		}
		buttons.clear();
	}

	title->setCaption( Trap::Cmd_Argv( 1 ) );

	maxw = Importer::StringWidth( title->getCaption().c_str(), title->getFont() );

	for( int i = 2 ; i < Trap::Cmd_Argc() ; i += 2 )
	{
		Button *button;

		button = Factory::newButton( panel, 20, 50 + buttons.size() * 40, 0, 0, Trap::Cmd_Argv( i ) );
		buttons.push_front(	std::make_pair( button,	Trap::Cmd_Argv( i + 1 ) ) );

		maxw = max( maxw, Importer::StringWidth( button->getCaption().c_str(), button->getFont() ) );
	}

	panel->setSize( maxw + 40 + 20, buttons.size() * 40 + 60 );
	panel->setPosition( ( REL_WIDTH - panel->getWidth() ) / 2.0f, ( REL_HEIGHT - panel->getHeight() )  / 2.0f );

	title->setSize( panel->getWidth(), 40 );

	for( std::list<ButtonPair>::iterator it = buttons.begin() ; it != buttons.end() ; it++ )
	{
		(*it).first->setSize( maxw + 20, 30 );
		(*it).first->setClickHandler( clickHandler );
	}

	panel->setVisible( true );
}

void CustomMenu::clickHandler( BaseObject *btn )
{
	for( std::list<ButtonPair>::iterator it = custommenu->buttons.begin() ; it != custommenu->buttons.end() ; it++ )
	{
		if( btn == (*it).first )
		{
			Trap::Cmd_ExecuteText( EXEC_APPEND, (*it).second.c_str() );
			UI_ForceMenuOff();
			break;
		}
	}

	custommenu->Hide();
}

void CustomMenu::Hide( void )
{
	panel->setVisible( false );
}
