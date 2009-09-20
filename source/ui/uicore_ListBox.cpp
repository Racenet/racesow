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
	ALLOCATOR_DEFINITION(ListBox)
	DELETER_DEFINITION(ListBox)

	void ListBox::Initialize( void )
	{
		scrollbar = Factory::newSlider( this, 0, 0, 1, 1, horizontal );
		scrollbar->setMinValue( 0 );
		scrollbar->setMaxValue( 0 );
		scrollbar->setValueChangeHandler( Scroll );
		setFocusable( true );
		multipleSelection = false;
		itemsswitchable = true;
		scrollbarSize = 20;
		itemSize = 20;
		ItemSelected = NULL;
		focusedItem = NULL;
		setSize( getWidth(), getHeight() );
		GotFocus = GotFocusHandler;
		LostFocus = LostFocusHandler;
		ListGotFocus = ListLostFocus = NULL;
	}

	ListBox::ListBox()
		: Panel()
	{
		Initialize();
	}

	ListBox::ListBox( BaseObject *parent, float x, float y, float w, float h, bool horizontal )
		: Panel( NULL, x, y, w, h )
	{
		this->horizontal = horizontal;
		Initialize();
		setParent( parent );
	}

	ListBox::~ListBox()
	{
		delete scrollbar;
	}

	void ListBox::BaseMouseDown( float x, float y, int button )
	{
		if ( button == Importer::mouseWheelUp )
			scrollbar->doMinus();
		else if ( button == Importer::mouseWheelDown )
			scrollbar->doPlus();
		else
			BaseObject::BaseMouseDown( x, y, button );
	}
	
	void ListBox::ensureItemVisible(ListItem *item, int visible)
	{
		float pos, height, maxheight, minheight;
		int itpos;
		
		if (!item)
			return;
		//pheight = ( (horizontal) ? getWidth() : getHeight() );
		/*
		for ( std::list<ListItem*>::iterator it = listBox->items.begin() ; it != listBox->items.end() ; ++it ) {
			//
		}
		*/
		
		itpos = getItemPosition(item);
		
		pos = (horizontal) ? item->getPositionX() : item->getPositionY();
		height = (horizontal) ? item->getWidth() : item->getHeight();
		minheight = ( bound( visible, 0, itpos ) * itemSize );
		maxheight = ( (horizontal) ? getWidth() : getHeight() ) - ( ( bound( visible, 0, (int)items.size() ) * itemSize ) + height);

		if ( pos < minheight ) {
			itpos -= visible;
			if (itpos < 0) {
				itpos = 0;
			}
		} else if ( pos > maxheight) {
			// Adapt scrollbar
			itpos = itpos - (int)(maxheight / itemSize);
			//if (itpos 
		} else {
			itpos = -1;
		}
		if (itpos >= 0) {
			scrollbar->setCurValue(itpos); // Will scroll?
			updateItemPositions();
		}
	}

	void ListBox::BaseKeyDown( int key, int charVal )
	{
		int i = -1;
		/*
		if ( !focusedItem )
		{
			BaseObject::BaseKeyDown( key, charVal );
			return;
		}
		*/
		if (!multipleSelection) {
			i = getSelectedPosition(0);
		}
		if (i < 0) {
			i = getItemPosition( focusedItem );
			if (i < 0) {
				i = 0;
			}
		}
		if ( key == Importer::keyDown || key == Importer::keyPadDown || key == Importer::keyRight || key == Importer::keyPadRight )
		{
			if (! focusedItem) {
				i = -1;
			}
			if ( (i + 1) < getNbItem() ) {
				if (focusedItem) focusedItem->focused = false;
				focusedItem = getItem( bound( i + 1, 0, getNbItem() - 1 ) );
				if (!multipleSelection) {
					selectItem(focusedItem);
				}
				ensureItemVisible(focusedItem, 2);
			}
		}
		else if ( key == Importer::keyUp || key == Importer::keyPadUp || key == Importer::keyLeft || key == Importer::keyPadLeft )
		{
			if (! focusedItem) {
				i = 1;
			}
			if ( (i - 1) >= 0 ) {
				if (focusedItem) focusedItem->focused = false;
				focusedItem = getItem( bound( i - 1, 0, getNbItem() - 1 ) );
				if (!multipleSelection) {
					selectItem(focusedItem);
				}
				ensureItemVisible(focusedItem, 2);
			}
		} else if ((key == Importer::keyPgUp) || (key == Importer::keyPadPgUp)) {
			int vis_items = (int)( getHeight() / itemSize ) - 2;
			if (vis_items <= 0) vis_items = 1;
			if (! focusedItem) {
				i = vis_items;
			}
			if ( (i - vis_items) < 0 ) {
				vis_items = i;
			}
			if (focusedItem) focusedItem->focused = false;
			focusedItem = getItem( bound( i - vis_items, 0, getNbItem() - 1 ) );
			if (!multipleSelection) {
				selectItem(focusedItem);
			}
			ensureItemVisible(focusedItem, 2);
		} else if ((key == Importer::keyPgDn) || (key == Importer::keyPadPgDn)) {
			int vis_items = (int)( getHeight() / itemSize ) - 2;
			if (vis_items <= 0) vis_items = 1;
			if (! focusedItem) {
				i = -vis_items;
			}
			if ( (i + vis_items) > getNbItem() ) {
				vis_items = getNbItem() - i;
			}
			if (focusedItem) focusedItem->focused = false;
			focusedItem = getItem( bound( i + vis_items, 0, getNbItem() - 1 ) );
			if (!multipleSelection) {
				selectItem(focusedItem);
			}
			ensureItemVisible(focusedItem, 2);

		} else if ( charVal == Importer::keySpace )
		{
			if (focusedItem) focusedItem->doSwitch();
			return;
		} else if ((charVal == Importer::keyEnter ) || (charVal == Importer::keyPadEnter  )) {
			if ((focusedItem) && (focusedItem->DoubleClick))
				focusedItem->DoubleClick(focusedItem);
		} else {
			BaseObject::BaseKeyDown( key, charVal );
			return;
		}
		focusedItem->focused = true;

		float itemsVisible;
		if ( horizontal )
			itemsVisible = (getWidth() / itemSize) - 1;
		else
			itemsVisible = (getHeight() / itemSize) - 1;

		i = getItemPosition( focusedItem );

		int scrollval = scrollbar->getCurValue();
		if ( scrollval < max( i - int(itemsVisible) + 1, 0 ) )
			scrollbar->setCurValue( i - int(itemsVisible) );
		else if ( scrollval > i )
			scrollbar->setCurValue( i );
		updateItemPositions();
	}

	void ListBox::Scroll( BaseObject *target, int, int )
	{
		ListBox *listBox = static_cast<ListBox*>( target->getParent() );
		listBox->updateItemPositions();
	}

	void ListBox::updateItemPositions( void )
	{
		std::list<ListItem*>::iterator it;
		float pos = -scrollbar->getCurValue() * itemSize;

		if ( horizontal ) {
			for ( it = items.begin() ; it != items.end() ; it++ ) {
				(*it)->setPosition( pos, 0 );
				pos += (*it)->getWidth();
			}
		} else {
			for ( it = items.begin() ; it != items.end() ; it++) {
				(*it)->setPosition( 0, pos );
				pos += (*it)->getHeight();
			}
		}
	}

	void ListBox::updateScrollbarBounds( void )
	{
		float listSize;
		if ( horizontal )
			listSize = max( 0.0f, getWidth() - 2 * getBorderWidth() );
		else
			listSize = max( 0.0f, getHeight() - 2 * getBorderWidth() );

		if ( signed(items.size()) * itemSize > listSize )
		{
			scrollbar->setMaxValue( max( 0, signed(items.size()) - int(listSize / itemSize) - 1 ) );
			scrollbar->setBarClickStepValue( max( 1, int(listSize / itemSize) - 1 ) );
		}
	}

	void ListBox::GotFocusHandler( BaseObject *target )
	{
		ListBox *listBox = static_cast<ListBox*>( target );
		if ( !listBox->items.empty() )
		{
			listBox->items.front()->focused = true;
			listBox->focusedItem = listBox->items.front();
		}

		if ( listBox->ListGotFocus )
			listBox->ListGotFocus( target );
	}
	void ListBox::LostFocusHandler( BaseObject *target )
	{
		ListBox *listBox = static_cast<ListBox*>( target );
		for ( std::list<ListItem*>::iterator it = listBox->items.begin() ; it != listBox->items.end() ; ++it )
			(*it)->focused = false;

		listBox->focusedItem = NULL;

		if ( listBox->ListLostFocus )
			listBox->ListLostFocus( target );
	}

	void ListBox::ItemSwitch( BaseObject *target, bool newValue )
	{
		ListBox *listBox = static_cast<ListBox*>( target->getParent() );
		ListItem *item = static_cast<ListItem*>( target );
		
		listBox->setFocus();
		if ((listBox->focusedItem != item) && ((!listBox->multipleSelection) || (newValue))) {
			if ( listBox->focusedItem )
				listBox->focusedItem->focused = false;
			listBox->focusedItem = item;
			item->focused = true;
		}

		if (!listBox->itemsswitchable) {
			if ((!item->isPressed()) && (!newValue)) {
				item->setPressed(true);
				listBox->selectItem( item );
				return;
			}
		}
		item->setPressed( !newValue );
		if ( newValue )
			listBox->selectItem( item );
		else
			listBox->unselectItem( item );
	}

	int ListBox::addItem( ListItem *item, int position, bool deletePrevious )
	{
		std::list<ListItem*>::iterator it;
		int i = 0;

		if ( position < 0 || position > signed(items.size()) )
			position = signed(items.size());

		if ( !item )
			return -1;

		for ( it = items.begin() ; it != items.end() ; it++, i++ )
			if ( i == position )
				break;

		item->setPressed( false );
		if ( horizontal )
			item->setSize( itemSize, getHeight() - scrollbarSize );
		else
			item->setSize( getWidth() - scrollbarSize, itemSize );

		if ( position < signed(items.size()) )
		{
			if ( deletePrevious )
				delete (*it);

			items.insert( it, item );
		}
		else
			items.push_back( item );

		item->setParent( this );
		item->setSwitchHandler( ItemSwitch );
		updateItemPositions();
		updateScrollbarBounds();

		return position;
	}

	ListItem * ListBox::getItem( int position )
	{
		if ( position < 0 || position >= signed(items.size()) )
			return NULL;		
	
		std::list<ListItem*>::iterator it;
		int i = 0;
		for ( it = items.begin() ; it != items.end() ; it++, i++ )
			if ( i == position )
				break;

		return (*it);
	}

	int ListBox::getItemPosition( ListItem *item ) const
	{
		if ( !item )
			return -1;

		std::list<ListItem*>::const_iterator it;
		int i = 0;
		for ( it = items.begin() ; it != items.end() ; it++, i++ )
			if ( (*it) == item )
				return i;

		return -1;
	}

	bool ListBox::removeItem( ListItem *item, bool deleteIt )
	{
		return removeItem( getItemPosition( item ), deleteIt );
	}

	bool ListBox::removeItem( int index, bool deleteIt )
	{
		if ( index < 0 || index >= signed(items.size()) )
			return false;
		std::list<ListItem*>::iterator it;
		int i = 0;
		for ( it = items.begin() ; it != items.end() ; it++, i++ )
			if ( i == index )
				break;
		if ( (*it) == focusedItem ) {
			focusedItem = NULL;
		}
		(*it)->setSwitchHandler( NULL );
		(*it)->setPressed(false);
		unselectItem(*it);

		if ( deleteIt )
			delete( *it );

		items.erase( it );

		updateScrollbarBounds();
		updateItemPositions();

		return true;
	}
	
	void ListBox::clear( bool deleteIt )
	{
		std::list<ListItem*>::iterator it;

		clearSelection();
		selectedItems.clear();

		if ( deleteIt )
		{
			for ( it = items.begin() ; it != items.end() ; it++ )
				delete *it;
		}
		else
		{
			for ( it = items.begin() ; it != items.end() ; it++ )
				(*it)->setSwitchHandler( NULL );
		}

		items.clear();

		updateScrollbarBounds();
		updateItemPositions();
	}

	int ListBox::getNbItem( void ) const
	{
		return (int)items.size();
	}

	bool ListBox::selectItem( int position )
	{
		return selectItem( getItem( position ) );
	}

	bool ListBox::selectItem( ListItem *item )
	{
		if ( item )
		{
			ListItem *currsel = getSelectedItem();
			bool oldvalue = item->isPressed();
			int position = getItemPosition( item );
			if (focusedItem)
				focusedItem->focused = false;
			focusedItem = item;
			if (currsel == item) {
				return false; // Don't do anything for currently selected items
			}
			focusedItem->focused = true;

			if ( !multipleSelection )
				unselectItem( currsel );
			item->setPressed( true );
			selectedItems.insert( std::make_pair<int, ListItem*>( position, item ) );
			if ( ItemSelected && !oldvalue ) {
				ItemSelected( item, position, true );
				updateItemPositions();
			}
			return true;
		}
		return false;
	}

	bool ListBox::unselectItem( int position )
	{
		return unselectItem( getItem( position ) );
	}

	bool ListBox::unselectItem( ListItem *item )
	{
		if ( item )
		{
			bool oldvalue = item->isPressed();
			int position = getItemPosition( item );
			item->setPressed( false );
			selectedItems.erase( position );
			if ( ItemSelected && oldvalue )
				ItemSelected( item, position, false );
			return true;
		}
		return false;
	}

	void ListBox::clearSelection( void )
	{
		while ( selectedItems.begin() != selectedItems.end() )
				unselectItem( selectedItems.begin()->second );
	}

	ListItem * ListBox::getSelectedItem( int index )
	{
		for ( std::map<int, ListItem*>::iterator it = selectedItems.begin() ; 
				it != selectedItems.end() && (index >= 0) && (index < signed(selectedItems.size()) ) ;
				++it, --index )
		{
			if ( index == 0 )
				return it->second;
		}
		return NULL;
	}

	int ListBox::getSelectedPosition( int index )
	{
		return getItemPosition( getSelectedItem( index ) );
	}

	int ListBox::getNbSelectedItem( void ) const
	{
		return signed(selectedItems.size());
	}

	void ListBox::setSize( float w, float h )
	{
		Panel::setSize( w, h );
		if ( horizontal )
		{
			scrollbar->setSize( w, scrollbarSize );
			scrollbar->setPosition( 0, h - scrollbarSize );
			for ( std::list<ListItem*>::iterator it = items.begin() ; it != items.end() ; it++ )
				(*it)->setSize( itemSize, h - scrollbarSize );
		}
		else
		{
			scrollbar->setSize( scrollbarSize, h );
			scrollbar->setPosition( w - scrollbarSize, 0 );
			for ( std::list<ListItem*>::iterator it = items.begin() ; it != items.end() ; it++ )
				(*it)->setSize( w - scrollbarSize, itemSize );
		}
		updateScrollbarBounds();
		updateItemPositions();
	}
}
