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
	BaseObject *draggingObject = NULL;
	BaseObject *mouseOverObject = NULL;
	BaseObject *focusedObject = NULL;
	Panel *rootPanel = NULL;

	float scaleX = 1.0f, scaleY = 1.0f;

	void setScale( float x, float y )
	{
		scaleX = x;
		scaleY = y;
	}

	void EventMouseDown( float x, float y, int button )
	{
		float ax, ay;
		rootPanel->getPosition( ax, ay );
		rootPanel->BaseMouseDown( x - ax, y - ay, button );
	}

	void EventMouseUp( float x, float y, int button )
	{
		float ax, ay;
		rootPanel->getPosition( ax, ay );
		rootPanel->BaseMouseUp( x - ax, y - ay, button );
	}
	
	void EventMouseMove( float x, float y, float xrel, float yrel )
	{
		float ax, ay;
		rootPanel->getPosition( ax, ay );
		rootPanel->BaseMouseMove( x - ax, y - ay, xrel, yrel );
	}
	
	void EventKeyDown( int key, int charVal )
	{
		rootPanel->BaseKeyDown( key, charVal );

		// TODO : sound playing
		if ( focusedObject )
		{
			if ( focusedObject->getType() == UICORE_TYPE_TEXTBOX )
			{
				// trap_S_StartLocalSound( "sound" );
				// ...
			}
		}
	}

	void EventKeyUp( int key, int charVal )
	{
		rootPanel->BaseKeyUp( key, charVal );
	}
}
