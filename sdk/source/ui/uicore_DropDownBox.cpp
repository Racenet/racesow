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
#include "uiwsw_Utils.h"
using namespace UIWsw;

namespace UICore
{
	ALLOCATOR_DEFINITION( DropDownBox );
	DELETER_DEFINITION( DropDownBox );

	void DropDownBox::Initialize( void )
	{
		current = Factory::newButton( this, 0, 0, 0, 0 );
		current->setAlign( ALIGN_MIDDLE_LEFT );
		current->setClickHandler( expandHandler );

		expand = Factory::newButton( this, 0, 0, 0, 0 );
		expand->setClickHandler( expandHandler );

		setTitle( "" );
		setBackColor( Color( 1.0f, 1.0f, 1.0f, 0.0f ) );

		showList( false );

		LostFocus = LostFocusHandler;

		expanded = false;
	}

	DropDownBox::DropDownBox()
		: ListBox()
	{
		Initialize();
	}

	DropDownBox::DropDownBox( BaseObject *parent, float x, float y, float w, float h, float listh , std::string title )
		: ListBox( NULL, x, y, w, h, false )
	{
		Initialize();

		setParent( parent );
		setTitle( title );
		setSize( w, h );
		setListHeight( listh );

		current->setSize( w - scrollbarSize + 1, h );

		// overlap slightly
		expand->setPosition( w - scrollbarSize, 0 );
		expand->setSize( scrollbarSize, h );
	}

	DropDownBox::~DropDownBox( void )
	{
		delete current;
		delete expand;
	}

	void DropDownBox::updateItemPositions( void )
	{
		std::list<ListItem*>::iterator it;
		int pos = -scrollbar->getCurValue();

		for( it = items.begin() ; it != items.end() ; it++, pos++ )
		{
			if( pos < 0 || pos >= floor( ( position.h - height ) / itemSize ) )
			{
				(*it)->setVisible( false );
				continue;
			}

			(*it)->setPosition( 0, pos * itemSize + height );
			(*it)->setVisible( true );
		}
	}

	void DropDownBox::updateScrollbarBounds( void )
	{
		float listSize;

		listSize = max( 0.0f, getHeight() - 2 * getBorderWidth() - height );

		if ( signed(items.size()) * itemSize > listSize )
		{
			scrollbar->setMaxValue( max( 0, signed(items.size()) - int(listSize / itemSize) ) );
			scrollbar->setBarClickStepValue( max( 1, int(listSize / itemSize) - 1 ) );
		}
	}

	int DropDownBox::addItem( ListItem *item, int position, bool deletePrevious )
	{
		int ret = ListBox::addItem( item, position, deletePrevious );

		if( ret != -1 )
		{
			item->setAlign( ALIGN_MIDDLE_LEFT );
			item->setSwitchHandler( ItemSwitch );
		}

		return ret;
	}

	bool DropDownBox::selectItem( int position )
	{
		return selectItem( getItem( position ) );
	}

	bool DropDownBox::selectItem( ListItem *item )
	{
		// selectItem might call unselectItem, caption must be set after this
		bool ret = ListBox::selectItem( item );
		if( ret )
			current->setCaption( item->getCaption() );
		return ret;
	}

	bool DropDownBox::unselectItem( int position )
	{
		return unselectItem( getItem( position ) );
	}

	bool DropDownBox::unselectItem( ListItem *item )
	{
		current->setCaption( getTitle() );
		
		return ListBox::unselectItem( item );
	}

	void DropDownBox::showList( bool show )
	{
		Image img;
		const char *pic = (show) ? "gfx/ui/arrow_up" : "gfx/ui/arrow_down";
		expanded = show;
		setSize( getWidth(), height );

		img = Trap::R_RegisterPic( (char*)pic );
		expand->setHighlightImage( img );
		expand->setBackgroundImage( img );
	}

	void DropDownBox::setSize( float w, float h )
	{
		if( expanded )
			ListBox::setSize( w, h + listheight );
		else
			ListBox::setSize( w, h );

		scrollbar->setPosition( w - scrollbarSize, h - 1 );
		scrollbar->setSize( scrollbarSize, listheight );

		height = h;
	}

	void DropDownBox::setListHeight( float listh )
	{
		listheight = listh;
		// this will make the new list height come into effect
		setSize( getWidth(), height );
	}

	void DropDownBox::setTitle( std::string title )
	{
		if( selectedItems.empty() )
			current->setCaption( title );

		this->title = title;
	}

	void DropDownBox::expandHandler( BaseObject *target )
	{
		DropDownBox *dropDownBox = static_cast<DropDownBox *>( target->getParent() );
		
		dropDownBox->showList( !dropDownBox->expanded );
	}

	void DropDownBox::LostFocusHandler( BaseObject *target )
	{
		DropDownBox *dropDownBox = static_cast<DropDownBox *>( target );

		dropDownBox->showList( false );

		ListBox::LostFocusHandler( target );
	}

	void DropDownBox::ItemSwitch( BaseObject *target, bool newValue )
	{
		DropDownBox *dropDownBox = static_cast<DropDownBox *>( target->getParent() );

		dropDownBox->showList( false );

		ListBox::ItemSwitch( target, newValue );
	}
}
