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


#ifndef _UICORE_BUTTON_H_
#define _UICORE_BUTTON_H_

#include "uicore_Label.h"

namespace UICore
{
	class Button : public Label
	{
	protected:
		Color highlightColor;
		Image highlightImage;

		virtual void BaseMouseUp( float x, float y, int button );
		virtual void BaseKeyDown( int key, int charVal );

		virtual void Initialize( void );

		virtual void DrawBackground( const Rect &posSrc, const Rect &posDest, bool enabled = true );

	public:
		Button();
		Button( BaseObject *parent, float x, float y, float w, float h, std::string caption = "" );
		virtual ~Button();

		MEMORY_OPERATORS

		inline virtual void setHighlightColor( const Color &color );
		inline virtual void setHighlightImage( Image image );

		inline virtual Color getHighlightColor( void ) const;
		inline virtual Image getHighlightImage( void ) const;

		inline virtual std::string getType( void ) const;
	};

	void Button::setHighlightColor( const Color &color )
	{
		highlightColor = color;
	}

	void Button::setHighlightImage( Image image )
	{
		highlightImage = image;
	}

	Color Button::getHighlightColor( void ) const
	{
		return highlightColor;
	}

	Image Button::getHighlightImage( void ) const
	{
		return highlightImage;
	}

	std::string Button::getType( void ) const
	{
		return UICORE_TYPE_BUTTON;
	}
}

#endif
