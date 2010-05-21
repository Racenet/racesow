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


#ifndef _UICORE_TEXTBOX_H_
#define _UICORE_TEXTBOX_H_

#include "uicore_Label.h"

namespace UICore
{
	enum 
	{
		TEXTBOX_NORMAL = 0,
		TEXTBOX_NUMBERSONLY = 1,
		TEXTBOX_PASSWORD = 2
		// next is = 4
	};

	class TextBox : public Button
	{
	protected:
		Color editingColor;
		Color editingFontColor;
		char *buffer;
		unsigned int bufferSize;
		unsigned int cursorPosition;
		int flags;

		static void KeyDownHandler( BaseObject *target, int key, int charVal );
		void (*TextChange)( BaseObject *target );
		void (*Validate)( BaseObject *target, const char *text );
		void (*TextBoxKeyDown)( BaseObject *target, int key, int charVal );

		virtual void BaseLostFocus( void );

		virtual void Initialize( void );

		virtual void DrawBackground( const Rect &posSrc, const Rect &posDest, bool enabled = true );
		virtual void DrawText( const Rect &posSrc, const Rect &posDest, bool enabled = true );

		unsigned int cursorTime;

	public:
		TextBox( int bufferSize );
		TextBox( BaseObject *parent, float x, float y, float w, float h, int bufferSize );
		virtual ~TextBox();

		MEMORY_OPERATORS

		virtual void Draw( unsigned int deltatime, const Rect *parentPos = NULL, const Rect *parentPosSrc = NULL, bool enabled = true );

		virtual void ClearText();
		virtual void doValidate();

		virtual void setText( const char *text );
		virtual void setEditingColor( const Color &color );
		virtual void setEditingFontColor( const Color &color );
		virtual void setFlags( int flags );

		virtual const char *getText( void ) const;
		virtual Color getEditingColor( void ) const;
		virtual Color getEditingFontColor( void ) const;
		virtual int getFlags( void ) const;

		virtual std::string getType( void ) const;

		virtual void setTextChangeHandler( void (*TextChange)( BaseObject *target ) );
		virtual void setValidateHandler( void (*Validate)( BaseObject *target, const char *text ) );
		virtual void setKeyDownHandler( void (*KeyDown)( BaseObject *target, int key, int charVal ) );
	};
}

#endif
