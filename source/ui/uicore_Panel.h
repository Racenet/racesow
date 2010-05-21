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


#ifndef _UICORE_PANEL_H_
#define _UICORE_PANEL_H_

#include "uicore_BaseObject.h"

namespace UICore
{
	class Panel : public BaseObject
	{
		friend void EventMouseDown( float x, float y, int button );
		friend void EventMouseUp( float x, float y, int button );
		friend void EventMouseMove( float x, float y, float xrel, float yrel );
		friend void EventKeyDown( int key, int charVal );
		friend void EventKeyUp( int key, int charVal );

		virtual void Initialize( void );

	public:
		Panel();
		Panel( BaseObject *parent, float x, float y, float w, float h );
		virtual ~Panel();

		MEMORY_OPERATORS

		virtual std::string getType( void ) const;		
	};
}

#endif
