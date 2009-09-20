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

#ifndef _UICORE_DROPDOWNBOX_H_
#define _UICORE_DROPDOWNBOX_H_

#include "uicore_ListBox.h"
#include "uicore_Button.h"

namespace UICore
{
	class DropDownBox : public ListBox
	{
	protected:
		Button *current, *expand;
		std::string title;
		float height, listheight;
		bool expanded;

		virtual void Initialize( void );

		virtual void updateItemPositions( void );
		virtual void updateScrollbarBounds( void );

		static void expandHandler( BaseObject *target );
		static void LostFocusHandler( BaseObject *target );
		static void ItemSwitch( BaseObject *target, bool newValue );

	public:
		MEMORY_OPERATORS;

		DropDownBox();
		DropDownBox( BaseObject *parent, float x, float y, float w, float h, float listh = 150, std::string title = "");
		~DropDownBox();

		virtual int addItem( ListItem *item, int position = -1, bool deletePrevious = false );
		virtual bool selectItem( int position );
		virtual bool selectItem( ListItem *item );
		virtual bool unselectItem( int position );
		virtual bool unselectItem( ListItem *item );

		void showList( bool show );

		virtual void setSize( float w, float h );
		void setListHeight( float listh );
		void setTitle( std::string title );

		inline std::string getTitle( void ) const;
		inline virtual std::string getType( void ) const;
	};

	std::string DropDownBox::getTitle( void ) const
	{
		return title;
	}

	std::string DropDownBox::getType( void ) const
	{
		return UICORE_TYPE_DROPDOWNBOX;
	}
}

#endif
