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

namespace UICore
{
	ALLOCATOR_DEFINITION(CheckBox)
	DELETER_DEFINITION(CheckBox)

	//MEMORY_OPERATORS

	void CheckBox::Initialize( void )
	{
		button = Factory::newSwitchButton( this, 0, 0, position.h, position.h );
		label = Factory::newLabel( this, float(position.h * 2.0f), 0, position.w - position.h, position.h );

		BaseObject::setBorderWidth( 0 );

		button->setSwitchHandler( SwitchHandler );

		setSwitchHandler( NULL );
	}

	CheckBox::CheckBox()
		: Panel()
	{
		Initialize();
	}

	CheckBox::CheckBox( BaseObject *parent, float x, float y, float w, float h, std::string caption )
		: Panel( NULL, x, y, w, h )
	{
		Initialize();
		setParent( parent );
		label->setCaption( caption );
	}

	CheckBox::~CheckBox()
	{
		delete button;
		delete label;
	}

	void CheckBox::setSize( float w, float h )
	{
		if ( align % 3 < 2 ) // left or center (treated as left)
		{
			Panel::setSize( w, h );
			button->setPosition( 0, 0 );
			label->setPosition( float(h * 2.0f), 0 );
			button->setSize( h, h );
			label->setSize( w - h, h );
		}
		else if ( align % 3 == 2 ) // right
		{
			Panel::setSize( w, h );
			button->setPosition( w - h, 0 );
			label->setPosition( 0, 0 );
			button->setSize( h, h );
			label->setSize( w - float(h * 2.0f), h );
		}
	}
	
	void CheckBox::doSwitch()
	{
		setPressed( !button->isPressed() );
		if ( Switch )
			Switch( this, button->isPressed() );
	}
	
	void CheckBox::SwitchHandler( BaseObject *target, bool newValue )
	{
		CheckBox *checkBox = static_cast<CheckBox*>( target->getParent() );
		if ( checkBox->Switch )
			checkBox->Switch( checkBox, newValue );
	}

	void CheckBox::setSwitchHandler( void (*Switch)( BaseObject *target, bool newValue ) )
	{
		this->Switch = Switch;
	}

	std::string CheckBox::getType( void ) const
	{
		return UICORE_TYPE_CHECKBOX;
	}
}
