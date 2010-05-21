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


#ifndef _UICORE_LISTITEM_H_
#define _UICORE_LISTITEM_H_

#include "uicore_SwitchButton.h"

namespace UICore
{
	class ListBox;

	class ListItem : public SwitchButton
	{
		friend class ListBox;
		friend class Factory;
	protected:
		bool focused;

		virtual void Initialize( void );

	public:
		ListItem();
		ListItem( float x, float y, float w, float h, std::string caption = "" );
		virtual ~ListItem();

		MEMORY_OPERATORS

		virtual void DrawBackground( const Rect &posSrc, const Rect &posDest, bool enabled );
		inline virtual std::string getType( void ) const;
	};

	std::string ListItem::getType( void ) const
	{
		return UICORE_TYPE_LISTITEM;
	}
}

#endif
