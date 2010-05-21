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
	ALLOCATOR_DEFINITION(Button)
	DELETER_DEFINITION(Button)

	void Button::Initialize( void )
	{
		setHighlightColor( Color( 0, 0, 0, 0 ) );
		setHighlightImage( NULL );
		setFocusable( true );
		setAlign( ALIGN_MIDDLE_CENTER );
	}

	Button::Button()
		: Label()
	{
		Initialize();
	}

	Button::Button( BaseObject *parent, float x, float y, float w, float h, std::string caption )
		: Label( NULL, x, y, w, h, caption )
	{
		Initialize();
		setParent( parent );
	}

	Button::~Button()
	{

	}

	void Button::DrawBackground( const Rect &posSrc, const Rect &posDest, bool enabled )
	{
		Rect rect;
		rect.x = posDest.x + posSrc.x;
		rect.y = posDest.y + posSrc.y;
		rect.w = posSrc.w;
		rect.h = posSrc.h;
		if ( !enabled || !this->enabled )
		{
			if ( disabledImage )
				Importer::DrawImage( disabledImage, posSrc, posDest, disabledColor );
			else
				Importer::FillRect( rect, disabledColor );
		}
		else if ( mouseOverObject == this || focusedObject == this )
		{
			if ( highlightImage )
				Importer::DrawImage( highlightImage, posSrc, posDest, highlightColor );
			else
				Importer::FillRect( rect, highlightColor );
		}
		else
		{
			if ( backgroundImage )
				Importer::DrawImage( backgroundImage, posSrc, posDest, backColor );
			else
				Importer::FillRect( rect, backColor );
		}
	}

	void Button::BaseKeyDown( int key, int charVal )
	{
		if( !isEnabled() )
			return;

		if ( charVal == Importer::keySpace || charVal == Importer::keyEnter || charVal == Importer::keyPadEnter )
		{
			if ( Click )
				Click( this );
		}
		else
			BaseObject::BaseKeyDown( key, charVal );
	}

	void Button::BaseMouseUp( float x, float y, int button )
	{
		if( !isEnabled() )
			return;

		BaseObject::BaseMouseUp( x, y, button );
		// special for button only (not inheritable)
		if ( getType() == UICORE_TYPE_BUTTON )
		{
			// buttons don't keep focus after being pressed
			if ( LostFocus )
				LostFocus( this );
			focusedObject = NULL;
		}
	}
}
