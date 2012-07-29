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
	ALLOCATOR_DEFINITION(TextBox)
	DELETER_DEFINITION(TextBox)

	void TextBox::Initialize( void )
	{
		buffer = (char*)Importer::newFunc( bufferSize );
		KeyDown = KeyDownHandler;
		buffer[0] = 0;
		cursorPosition = 0;
		cursorTime = 0;
		setEditingColor( Color(0, 0, 0, 0) );
		setEditingFontColor( Color(0, 0, 0, 0) );
		setAlign( ALIGN_MIDDLE_LEFT );
		setKeyDownHandler( NULL );
		setValidateHandler( NULL );
		setTextChangeHandler( NULL );
		setFlags( TEXTBOX_NORMAL );
	}

	TextBox::TextBox( int bufferSize )
		: Button(), bufferSize( bufferSize )
	{
		Initialize();
	}

	TextBox::TextBox( BaseObject *parent, float x, float y, float w, float h, int bufferSize )
		: Button( NULL, x, y, w, h, "" ), bufferSize( bufferSize )
	{
		Initialize();
		setParent( parent );
	}

	TextBox::~TextBox()
	{
		delete buffer;
	}

	void TextBox::BaseLostFocus( void )
	{
		doValidate();
		BaseObject::BaseLostFocus();
	}

	void TextBox::DrawBackground( const Rect &posSrc, const Rect &posDest, bool enabled )
	{
		Rect rect;
		rect.x = posDest.x + posSrc.x;
		rect.y = posDest.y + posSrc.y;
		rect.w = posSrc.w;
		rect.h = posSrc.h;
		if ( !enabled || !this->enabled )
			Importer::FillRect( rect, disabledColor );
		else if ( focusedObject == this )
			Importer::FillRect( rect, editingColor );
		else if ( mouseOverObject == this )
			Importer::FillRect( rect, highlightColor );
		else
			Importer::FillRect( rect, backColor );
	}

	void TextBox::DrawText( const Rect &posSrc, const Rect &posDest, bool enabled )
	{
		if ( !font )
			return;

		char tmp, *text, *c;
		Rect textSrc, textDest;
		FitTextToRect( posSrc, posDest, textSrc, textDest, buffer );

		text = new char[strlen( buffer ) + 1];
		strcpy( text, buffer );
		if( flags & TEXTBOX_PASSWORD )
		{
			for( c = text ; *c ; c++ )
				*c = '*';
		}

		if ( !enabled || !this->enabled )
			Importer::DrawString( textSrc, textDest, text, font, disabledFontColor );
		else if ( focusedObject == this )
		{
			Importer::DrawString( textSrc, textDest, text, font, editingFontColor );
			if (  (( int ) ( cursorTime / 250 ) & 1 ) && Importer::StringWidth( text, font ) - textSrc.x < position.w )
			{
				tmp = text[cursorPosition];
				text[cursorPosition] = '\0';
				textDest.x += Importer::StringWidth( text, font );
				textDest.w = 1;
				text[cursorPosition] = tmp;
				Importer::FillRect( textDest, editingFontColor );
			}
		}
		else
			Importer::DrawString( textSrc, textDest, text, font, fontColor );

		delete text;
	}

	void TextBox::Draw( unsigned int deltatime, const Rect *parentPos, const Rect *parentPosSrc, bool enabled )
	{
		cursorTime = deltatime;
		Label::Draw( deltatime, parentPos, parentPosSrc, enabled );
	}

	void TextBox::setText( const char *text )
	{
		int size = min( (unsigned int)(strlen(text)), bufferSize-1 );
		memcpy( buffer, text, size );
		buffer[size] = '\0';
		cursorPosition = size;
		if ( TextChange )
			TextChange( this );
	}

	void TextBox::setEditingColor( const Color &color )
	{
		editingColor = color;
	}

	void TextBox::setEditingFontColor( const Color &color )
	{
		editingFontColor = color;
	}

	void TextBox::setFlags( int f )
	{
		flags = f;
	}

	const char *TextBox::getText( void ) const
	{
		return buffer;
	}

	Color TextBox::getEditingColor( void ) const
	{
		return editingColor;
	}

	Color TextBox::getEditingFontColor( void ) const
	{
		return editingFontColor;
	}

	int TextBox::getFlags( void ) const
	{
		return flags;
	}

	void TextBox::ClearText()
	{
		if ( !bufferSize )
			return;

		buffer[0] = '\0';

		if ( TextChange )
			TextChange( this );
	}

	void TextBox::doValidate()
	{
		focusedObject = NULL;
		if ( Validate )
			Validate( this, buffer );
	}

	void TextBox::KeyDownHandler( BaseObject *target, int key, int charVal )
	{
		TextBox *textBox = static_cast<TextBox*>( target );
		unsigned int i, length = (unsigned int)strlen(textBox->buffer);

		if ( key == Importer::keyLeft /*|| key == Importer::keyPadLeft*/ )
		{
			if ( textBox->cursorPosition > 0 )
				textBox->cursorPosition--;
		}
		else if ( key == Importer::keyRight /*|| key == Importer::keyPadRight*/ )
		{
			if ( textBox->cursorPosition < length )
				textBox->cursorPosition++;
		}
		else if ( key == Importer::keyDelete /*| key == Importer::keyPadDelete*/ )
		{
			if ( length > textBox->cursorPosition )
			{
				i = textBox->cursorPosition;
				while ( textBox->buffer[i] != '\0' )
				{
					textBox->buffer[i] = textBox->buffer[i+1];
					i++;
				}
			}
			return;
		}
		else if ( key == Importer::keyBackspace )
		{
			if ( textBox->cursorPosition > 0 )
			{
				i = textBox->cursorPosition - 1;
				while ( textBox->buffer[i] != '\0' )
				{
					textBox->buffer[i] = textBox->buffer[i+1];
					i++;
				}
				textBox->cursorPosition--;
			}
		}
		else if ( key == Importer::keyEnter || key == Importer::keyPadEnter )
		{
			textBox->doValidate();
		}
		else if ( charVal >= 32 && charVal < 256 )
		{
			if ( length + 1 >= textBox->bufferSize )
				return;

			if ( textBox->flags & TEXTBOX_NUMBERSONLY && ( charVal < '0' && charVal > '9' && charVal != '.' ) )
				return;

			if ( textBox->cursorPosition < length )
			{
				i = length;
				while ( i >= textBox->cursorPosition && i <= length )
				{
					textBox->buffer[i+1] = textBox->buffer[i];
					i--;
				}
			}
			textBox->buffer[textBox->cursorPosition++] = (char)charVal;
			if ( textBox->cursorPosition == length + 1 )
				textBox->buffer[textBox->cursorPosition] = '\0';
			if ( textBox->TextChange )
				textBox->TextChange( textBox );
		}
		else return;
	}

	void TextBox::setTextChangeHandler( void (*TextChange)( BaseObject *target ) )
	{
		this->TextChange = TextChange;
	}

	void TextBox::setValidateHandler( void (*Validate)( BaseObject *target, const char *text ) )
	{
		this->Validate = Validate;
	}

	void TextBox::setKeyDownHandler( void (*KeyDown)( BaseObject *target, int key, int charVal ) )
	{
		TextBoxKeyDown = KeyDown;
	}

	std::string TextBox::getType( void ) const
	{
		return UICORE_TYPE_TEXTBOX;
	}
}
