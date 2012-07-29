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

#if 0
	int shadowoffset = 1;
	if( !font )
		font = uis.fontSystemSmall;

	shadowoffset += ( trap_SCR_strHeight( font ) >= trap_SCR_strHeight( uis.fontSystemBig ) );

	if( maxwidth > 0 )
	{
		//trap_SCR_DrawStringWidth( x+shadowoffset, y+shadowoffset, align, COM_RemoveColorTokens( str ), maxwidth, font, colorBlack );
		//trap_SCR_DrawStringWidth( x, y, align, COM_RemoveColorTokens( str ), maxwidth, font, UI_COLOR_HIGHLIGHT );
		x += UISCR_HorizontalAlignOffset( align, maxwidth );
		y += UISCR_VerticalAlignOffset( align, trap_SCR_strHeight( font ) );
		trap_SCR_DrawClampString( x+shadowoffset, y+shadowoffset, COM_RemoveColorTokens( str ), x+shadowoffset, y+shadowoffset, x+shadowoffset+maxwidth, y+shadowoffset+trap_SCR_strHeight( font ), font, colorBlack );
		trap_SCR_DrawClampString( x, y, COM_RemoveColorTokens( str ), x, y, x+maxwidth, y+trap_SCR_strHeight( font ), font, UI_COLOR_HIGHLIGHT );
	}
	else
	{
		trap_SCR_DrawString( x+shadowoffset, y+shadowoffset, align, str, font, colorBlack );
#endif

namespace UICore
{
	ALLOCATOR_DEFINITION(Button)
	DELETER_DEFINITION(Button)

	void Button::Initialize( void )
	{
		setHighlightColor( Color( 0, 0, 0, 0 ) );
		setHighlightFontColor( Color( 0, 0, 0, 0 ) );
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

	void Button::DrawText( const Rect &posSrc, const Rect &posDest, bool enabled )
	{
		if ( !font )
			return;

		Color backupColor;

		backupColor = fontColor;
		if( mouseOverObject == this || focusedObject == this )
		{
			Rect shadowRect;
			float shadowOffset = 1;

			shadowRect = posDest;
			shadowOffset += ( Importer::StringHeight( font ) >= 30 ); // FIXME

			shadowRect.x += shadowOffset;
			shadowRect.y += shadowOffset;

			fontColor = Color( 0, 0, 0, 1 );
			Label::DrawText( posSrc, shadowRect, enabled );

			fontColor = highlightFontColor;
		}

		Label::DrawText( posSrc, posDest, enabled );

		fontColor = backupColor;
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
				Importer::DrawImage( disabledImage, posSrc, posDest, disabledColor, rotation );
			else
				Importer::FillRect( rect, disabledColor );
		}
		else if ( mouseOverObject == this || focusedObject == this )
		{
			if ( highlightImage )
				Importer::DrawImage( highlightImage, posSrc, posDest, highlightColor, rotation );
			else
				Importer::FillRect( rect, highlightColor );
		}
		else
		{
			if ( backgroundImage )
				Importer::DrawImage( backgroundImage, posSrc, posDest, backColor, rotation );
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
