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


#ifndef _UICORE_SWITCHBUTTON_H_
#define _UICORE_SWITCHBUTTON_H_

#include "uicore_Button.h"

namespace UICore
{
	class SwitchButton : public Button
	{
	protected:
		bool pressed;
		Color pressedColor;
		Color pressedFontColor;
		Image pressedImage;

		static void ClickHandler( BaseObject *target );
		void (*SwitchClick)( BaseObject *target );
		void (*Switch)( BaseObject *target, bool newValue );

		virtual void BaseMouseUp( float x, float y, int button );
		virtual void BaseKeyDown( int key, int charVal );

		virtual void Initialize( void );

		virtual void DrawBackground( const Rect &posSrc, const Rect &posDest, bool enabled = true );
		virtual void DrawText( const Rect &posSrc, const Rect &posDest, bool enabled = true );

	public:
		SwitchButton();
		SwitchButton( BaseObject *parent, float x, float y, float w, float h, std::string caption = "" );
		virtual ~SwitchButton();

		MEMORY_OPERATORS

		void doSwitch();

		inline virtual void setPressed( bool v );
		inline virtual void setPressedColor( const Color &color );
		inline virtual void setPressedFontColor( const Color &color );
		inline virtual void setPressedImage( Image image );

		inline virtual bool isPressed( void ) const;
		inline virtual Color getPressedColor( void ) const;
		inline virtual Color getPressedFontColor( void ) const;
		inline virtual Image getPressedImage( void ) const;

		inline virtual std::string getType( void ) const;

		virtual void setClickHandler( void (*Click)( BaseObject *target ) );
		virtual void setSwitchHandler( void (*Switch)( BaseObject *target, bool newValue ) );
	};

	void SwitchButton::setPressed( bool v )
	{
		pressed = v;
	}

	void SwitchButton::setPressedColor( const Color &color )
	{
		pressedColor = color;
	}

	void SwitchButton::setPressedFontColor( const Color &color )
	{
		pressedFontColor = color;
	}

	void SwitchButton::setPressedImage( Image image )
	{
		pressedImage = image;
	}

	bool SwitchButton::isPressed( void ) const
	{
		return pressed;
	}

	Color SwitchButton::getPressedColor( void ) const
	{
		return pressedColor;
	}

	Color SwitchButton::getPressedFontColor( void ) const
	{
		return pressedFontColor;
	}

	Image SwitchButton::getPressedImage( void ) const
	{
		return pressedImage;
	}

	std::string SwitchButton::getType( void ) const
	{
		return UICORE_TYPE_SWITCHBUTTON;
	}
}

#endif
