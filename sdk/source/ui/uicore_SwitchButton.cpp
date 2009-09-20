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
	ALLOCATOR_DEFINITION(SwitchButton)
	DELETER_DEFINITION(SwitchButton)

	void SwitchButton::Initialize( void )
	{
		Click = ClickHandler;
		setPressed( false );
		setPressedColor( Color( 0, 0, 0, 0 ) );
		setPressedFontColor( Color( 0, 0, 0, 0 ) );
		setPressedImage( NULL );
		setClickHandler( NULL );
		setSwitchHandler( NULL );
	}

	SwitchButton::SwitchButton()
		: Button()
	{
		Initialize();
	}

	SwitchButton::SwitchButton( BaseObject *parent, float x, float y, float w, float h, std::string caption )
		: Button( NULL, x, y, w, h, caption )
	{
		Initialize();
		setParent( parent );
	}

	SwitchButton::~SwitchButton()
	{

	}

	void SwitchButton::DrawBackground( const Rect &posSrc, const Rect &posDest, bool enabled )
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
		else
		{
			if ( pressed )
			{
				if ( pressedImage )
					Importer::DrawImage( pressedImage, posSrc, posDest, pressedColor );
				else
					Importer::FillRect( rect, pressedColor );
			}
			if ( mouseOverObject == this || focusedObject == this )
			{
				if ( highlightImage )
					Importer::DrawImage( highlightImage, posSrc, posDest, highlightColor );
				else
					Importer::FillRect( rect, highlightColor );
			}
			else if ( !pressed )
			{
				if ( backgroundImage )
					Importer::DrawImage( backgroundImage, posSrc, posDest, backColor );
				else
					Importer::FillRect( rect, backColor );
			}
		}
	}

	void SwitchButton::DrawText( const Rect &posSrc, const Rect &posDest, bool enabled )
	{
		if ( !font )
			return;

		Rect textSrc, textDest;
		FitTextToRect( posSrc, posDest, textSrc, textDest, caption.c_str() );

		if ( !enabled || !this->enabled )
			Importer::DrawString( textSrc, textDest, caption.c_str(), font, disabledFontColor );
		else if ( pressed )
			Importer::DrawString( textSrc, textDest, caption.c_str(), font, pressedFontColor );
		else
			Importer::DrawString( textSrc, textDest, caption.c_str(), font, fontColor );
	}

	void SwitchButton::doSwitch()
	{
		setPressed( !pressed );
		if ( Switch )
			Switch( this, pressed );
	}
	
	void SwitchButton::ClickHandler( BaseObject *target )
	{
		SwitchButton *switchButton = static_cast<SwitchButton*>( target );
		switchButton->doSwitch();
		if ( switchButton->SwitchClick )
			switchButton->SwitchClick( switchButton );
	}

	void SwitchButton::setClickHandler( void (*Click)( BaseObject *target ) )
	{
		this->SwitchClick = Click;
	}

	void SwitchButton::setSwitchHandler( void (*Switch)( BaseObject *target, bool newValue ) )
	{
		this->Switch = Switch;
	}

	void SwitchButton::BaseKeyDown( int key, int charVal )
	{
		if ( charVal == Importer::keySpace || charVal == Importer::keyEnter || charVal == Importer::keyPadEnter )
			doSwitch();
		else
			BaseObject::BaseKeyDown( key, charVal );
	}

	void SwitchButton::BaseMouseUp( float x, float y, int button )
	{
		BaseObject::BaseMouseUp( x, y, button );
		// special for switchbutton only (not inheritable)
		if ( getType() == UICORE_TYPE_SWITCHBUTTON )
		{
			// buttons don't keep focus after being pressed
			if ( LostFocus )
				LostFocus( this );
			focusedObject = NULL;
		}
	}

}
