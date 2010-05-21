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

// TODO : a special class for listitems, inherited from switch button, but
// - private inheritance with only useful functions set as public
// - not focusable (focus is for list itself, scrolling is done with arrows instead of tab)
// and add a propery to listbox that disallow to have no item selected


#ifndef _UICORE_LISTBOX_H_
#define _UICORE_LISTBOX_H_

#include "uicore_Slider.h"
#include "uicore_ListItem.h"

namespace UICore
{
	class ListBox : public Panel
	{
	protected:

		virtual void Initialize( void );
		std::list<ListItem*> items;
		std::map<int, ListItem*> selectedItems;
		Slider *scrollbar;
		bool horizontal;
		bool multipleSelection;
		bool itemsswitchable;
		float scrollbarSize;
		float itemSize;
		ListItem *focusedItem;

		void (*ListGotFocus) (BaseObject *target );
		void (*ListLostFocus) (BaseObject *target );
		void (*ItemSelected)( ListItem *target, int position, bool isSelected );
		
		static void GotFocusHandler( BaseObject *target );
		static void LostFocusHandler( BaseObject *target );
		static void ItemSwitch( BaseObject *target, bool newValue );

		virtual void BaseMouseDown( float x, float y, int button );
		virtual void BaseKeyDown( int key, int charVal );

		static void Scroll( BaseObject *target, int, int );

		virtual void updateItemPositions( void );
		virtual void updateScrollbarBounds( void );

	public:
		ListBox();
		ListBox( BaseObject *parent, float x, float y, float w, float h, bool horizontal );
		virtual ~ListBox();

		MEMORY_OPERATORS

		virtual int addItem( ListItem *item, int position = -1, bool deletePrevious = false );
		virtual ListItem * getItem( int position = -1 );
		virtual int getItemPosition( ListItem *item ) const;
		virtual bool removeItem( ListItem *item, bool deleteIt = true );
		virtual bool removeItem( int position, bool deleteIt = true ); 
		virtual void clear( bool deleteIt = true );
		virtual int getNbItem( void ) const;

		virtual bool selectItem( int position );
		virtual void ensureItemVisible(ListItem *item, int visible = 0);
		virtual bool selectItem( ListItem *item );
		virtual bool unselectItem( int position );
		virtual bool unselectItem( ListItem *item );
		virtual void clearSelection( void );
		virtual ListItem * getSelectedItem( int index = 0 );
		virtual int getSelectedPosition( int index = 0 );
		virtual int getNbSelectedItem( void ) const;

		virtual void setSize( float w, float h );
		inline virtual void setItemsSwitchable( bool v );
		inline virtual void setMultipleSelection( bool v );
		inline virtual void setHorizontal( bool v );
		inline virtual void setScrollbarSize( float size );
		inline virtual void setItemSize( float size );

		/** Returns an iterator set at the first item of the list. @Remark Using this function is UNSAFE */
		inline virtual std::list<ListItem*>::iterator getFirstItemIterator( void );
		/** Returns an iterator set just after the last item of the list. @Remark Using this function is UNSAFE */
		inline virtual std::list<ListItem*>::iterator getEndItemIterator( void );
		inline virtual bool getMultipleSelection( void ) const;
		inline virtual bool isHorizontal( void ) const;
		inline virtual float getScrollbarSize( void ) const;
		inline virtual float getItemSize( void ) const;

		inline virtual void setItemSelectedHandler( void (*ItemSelected)( ListItem *target, int position, bool isSelected ) );
		inline virtual void setGotFocusHandler( void (*GotFocus)( BaseObject *target ) );
		inline virtual void setLostFocusHandler( void (*LostFocus)( BaseObject *target ) );

		inline virtual std::string getType( void ) const;
	};
	
	void ListBox::setItemsSwitchable( bool v )
	{
		itemsswitchable = v;
	}
	void ListBox::setMultipleSelection( bool v )
	{
		multipleSelection = v;
	}

	void ListBox::setHorizontal( bool v )
	{
		horizontal = v;
		scrollbar->setHorizontal( v );
		setSize( getWidth(), getHeight() );
		updateScrollbarBounds();
		updateItemPositions();
	}

	void ListBox::setScrollbarSize( float size )
	{
		scrollbarSize = size;
		setSize( getWidth(), getHeight() );
	}

	void ListBox::setItemSize( float size )
	{
		itemSize = size;
		setSize( getWidth(), getHeight() );
	}

	std::list<ListItem*>::iterator ListBox::getFirstItemIterator( void )
	{
		return items.begin();
	}

	std::list<ListItem*>::iterator ListBox::getEndItemIterator( void )
	{
		return items.end();
	}

	bool ListBox::getMultipleSelection( void ) const
	{
		return multipleSelection;
	}

	bool ListBox::isHorizontal( void ) const
	{
		return horizontal;
	}

	float ListBox::getScrollbarSize( void ) const
	{
		return scrollbarSize;
	}

	float ListBox::getItemSize( void ) const
	{
		return itemSize;
	}

	void ListBox::setItemSelectedHandler( void (*ItemSelected)( ListItem *target, int position, bool isSelected ) )
	{
		this->ItemSelected = ItemSelected;
	}

	void ListBox::setGotFocusHandler( void (*GotFocus)( BaseObject *target ) )
	{
		ListGotFocus = GotFocus;
	}

	void ListBox::setLostFocusHandler( void (*LostFocus)( BaseObject *target ) )
	{
		ListGotFocus = LostFocus;
	}

	std::string ListBox::getType( void ) const
	{
		return UICORE_TYPE_LISTBOX;
	}
}

#endif
