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


#ifndef _UICORE_LABEL_H_
#define _UICORE_LABEL_H_

#include "uicore_BaseObject.h"

namespace UICore
{
	class Label : public BaseObject
	{
	protected:
		Font font;
		Color fontColor;
		Color disabledFontColor;
		std::string caption;
		Alignment textAlign;
		bool multiline;

		virtual void Initialize( void );

		virtual void FitTextToRect( const Rect &objectSrc, const Rect &objectDest, Rect &textSrc, Rect &textDest, const char *buf );
		virtual void DrawText( const Rect &posSrc, const Rect &posDest, bool enabled = true );


	public:
		Label();
		Label( BaseObject *parent, float x, float y, float w, float h, std::string caption = "" );
		virtual	~Label();

		MEMORY_OPERATORS

		virtual void Draw( unsigned int deltatime, const Rect *parentPos = NULL, const Rect *parentPosSrc = NULL, bool enabled = true );

		inline virtual void setFont( Font font );
		inline virtual void setFontColor( const Color &color );
		inline virtual void setDisabledFontColor( const Color &color );
		inline virtual void setCaption( const std::string caption );
		inline virtual void setAlign( Alignment align );
		virtual void setMultiline( bool m );

		inline virtual Font getFont( void ) const;
		inline virtual Color getFontColor( void ) const;
		inline virtual Color getDisabledFontColor( void ) const;
		inline virtual std::string getCaption( void ) const;
		inline virtual Alignment getAlign( void ) const;
		inline virtual bool getMultiline( void ) const;

		inline virtual std::string getType( void ) const;
	};

	void Label::setFont( Font font )
	{
		this->font = font;
	}

	void Label::setFontColor( const Color &color )
	{
		fontColor = color;
	}

	void Label::setDisabledFontColor( const Color &color )
	{
		disabledFontColor = color;
	}

	void Label::setCaption( const std::string caption )
	{
		this->caption = caption;
	}

	void Label::setAlign( Alignment align )
	{
		textAlign = align;
	}

	Font Label::getFont( void ) const
	{
		return font;
	}

	Color Label::getFontColor( void ) const
	{
		return fontColor;
	}

	Color Label::getDisabledFontColor( void ) const
	{
		return disabledFontColor;
	}

	std::string Label::getCaption( void ) const
	{
		return caption;
	}

	Alignment Label::getAlign( void ) const
	{
		return textAlign;
	}

	bool Label::getMultiline( void ) const
	{
		return multiline;
	}

	std::string Label::getType( void ) const
	{
		return UICORE_TYPE_LABEL;
	}
}

#endif
