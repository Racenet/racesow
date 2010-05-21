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
	ALLOCATOR_DEFINITION(Label)
	DELETER_DEFINITION(Label)

	void Label::Initialize( void )
	{
		setFont( NULL );
		setFontColor( Color( 0, 0, 0, 0 ) );
		setDisabledFontColor( Color( 0, 0, 0, 0 ) );
		setCaption( "" );
		setBorderWidth( 0 );
		setAlign( ALIGN_TOP_LEFT );
		setMultiline( false );
	}

	Label::Label()
		: BaseObject()
	{
		Initialize();
	}

	Label::Label( BaseObject *parent, float x, float y, float w, float h, std::string caption )
		: BaseObject( NULL, x, y, w, h )
	{
		Initialize();
		setParent( parent );
		setCaption( caption );
	}

	Label::~Label()
	{

	}

	void Label::setMultiline( bool m )
	{
		multiline = m;
		setAlign( ALIGN_TOP_LEFT );
	}

	void Label::FitTextToRect( const Rect &objectSrc, const Rect &objectDest, Rect &textSrc, Rect &textDest, const char *buf )
	{
		float strWidth, strHeight;
		strWidth = Importer::StringWidth( buf, font );
		strHeight = Importer::StringHeight( font );

		textSrc.x = 0;
		textSrc.y = 0;
		textDest.x = borderWidth;
		textDest.y = borderWidth;
		textDest.w = textSrc.w = strWidth;
		textDest.h = textSrc.h = strHeight;

		if( textAlign % 3 == 1 ) // center
			textDest.x += (objectDest.w - 2*borderWidth - strWidth) / 2;
		if( textAlign % 3 == 2 ) // right
			textDest.x += objectDest.w - 2*borderWidth - strWidth;

		if ( textDest.x < objectSrc.x + borderWidth )
		{
			textSrc.x = objectSrc.x + borderWidth - textDest.x;
			textSrc.w = strWidth - textSrc.x;
		}

		if ( textSrc.w + max(0.0f, textDest.x) > objectSrc.w - 2*borderWidth )
			textSrc.w = objectSrc.w - borderWidth - max(0.0f, textDest.x);
			
		textDest.x += objectDest.x;

		if( textAlign / 3 == 1 ) // center
			textDest.y += (objectDest.h - 2*borderWidth - strHeight) / 2;
		if( textAlign / 3 == 2 ) // right
			textDest.y += objectDest.h - 2*borderWidth - strHeight;

		if ( textDest.y < objectSrc.y + borderWidth )
		{
			textSrc.y = objectSrc.y + borderWidth - textDest.y;
			textSrc.h = strHeight - textSrc.y;
		}

		if ( textSrc.h + max(0.0f, textDest.y) > objectSrc.h - 2*borderWidth )
			textSrc.h = objectSrc.h - 2*borderWidth - max(0.0f, textDest.y);
			
		textDest.y += objectDest.y;
	}

	void Label::DrawText( const Rect &posSrc, const Rect &posDest, bool enabled )
	{
		if ( !font )
			return;

		Rect textSrc, textDest;
		Color color;

		FitTextToRect( posSrc, posDest, textSrc, textDest, caption.c_str() );

		color = enabled && isEnabled() ? fontColor : disabledFontColor;

		if( multiline && ( textSrc.w > textDest.w || caption.find( '\n' ) != std::string::npos ) )
		{
			// spacing between lines
			const int spacing = 2;

			float x, width, height;
			char *buffer, *text, *line, *c, *s;

			// create a copy of the caption
			buffer = text = line = new char[caption.length() + 1];
			strcpy( buffer, caption.c_str() );

			height = Importer::StringHeight( font );
			textSrc.h = height;

			// store original x position for the start of each line
			x = textDest.x;

			// split the caption up by its linebreaks
			while( ( c = strchr( line, '\n' ) ) != NULL || *line )
			{
				if( c )
					*c = 0;

				text = line;

				// check each word in the line to see where the line needs to wrap
				while( ( s = strchr( text, ' ' ) ) != NULL || *text )
				{
					if( s )
						*s = 0;

					width = Importer::StringWidth( text, font );

					// wrap down to the next line unless its the first word, then we can't wrap it
					if( textDest.x + width > posDest.x + posDest.w && text != line )
					{
						textDest.x = x;
						textDest.y += height + spacing;
					}

					textSrc.w = width;
					textDest.w = posDest.x + posDest.w - textDest.x - borderWidth;
					textDest.h = posDest.y + posDest.h - textDest.y - borderWidth;

					// line won't fit into the label
					if( textDest.y > posDest.y + posDest.h )
						break;

					Importer::DrawString( textSrc, textDest, text, font, color );
					
					textDest.x += width + Importer::StringWidth( " ", font );

					if( s )
						text = s + 1;
					else
						*text = 0;
				}

				textDest.x = x;
				textDest.y += height + spacing;

				// line won't fit into the label
				if( textDest.y > posDest.y + posDest.h )
					break;

				if( c )
					line = c + 1;
				else
					*line = 0;
			}

			delete buffer;
		}
		else
			Importer::DrawString( textSrc, textDest, caption.c_str(), font, color );
	}

	void Label::Draw( unsigned int deltatime, const Rect *parentPos, const Rect *parentPosSrc, bool enabled )
	{
		Rect posSrc, posDest;

		if ( !visible )
			return;

		FitPosition( parentPos, parentPosSrc, posSrc, posDest );
		DrawBackground( posSrc, posDest, enabled );
		DrawText( posSrc, posDest, enabled );
		DrawChildren( deltatime, posSrc, posDest, enabled );
		DrawBorder( posSrc, posDest, enabled );
	}
}
