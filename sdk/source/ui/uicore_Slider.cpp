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
#include <cmath>

namespace UICore
{
	ALLOCATOR_DEFINITION(Slider)
	DELETER_DEFINITION(Slider)

	void Slider::Initialize( void )
	{	
		minus = Factory::newButton( this, 0, 0, 1, 1 );
		minus->setClickHandler( MinusClickHandler );
		plus = Factory::newButton( this, 0, 0, 1, 1 );
		plus->setClickHandler( PlusClickHandler );
		scrollbar = Factory::newButton( this, 0, 0, 1, 1 );
		scrollbar->setMouseDownHandler( ScrollbarMouseDownClickHandler );
		cursor = Factory::newButton( scrollbar, 0, 0, 1, 1 );
		cursor->setDraggable( true );
		cursor->setDraggingHandler( DraggingCursorHandler );
		cursor->setMouseUpHandler( MouseUpCursorHandler );
		setFocusable( true );
		setValueChangeHandler( NULL );
		setBoundValues( 0, 10 );
		setButtonStepValue( 1 );
		setBarClickStepValue( 3 );
		BaseObject::setBorderWidth( 0 );
	}

	Slider::Slider()
		: BaseObject()
	{
		Initialize();
	}

	Slider::Slider( BaseObject *parent, float x, float y, float w, float h, bool horizontal )
		: BaseObject( NULL, x, y, w, h )
	{
		Initialize();
		setParent( parent );
		setHorizontal( horizontal );
	}

	Slider::~Slider()
	{
		delete minus;
		delete plus;
		delete cursor;
		delete scrollbar;
	}

	void Slider::Draw( unsigned int deltatime, const Rect *parentPos, const Rect *parentPosSrc, bool enabled )
	{
		if ( this == focusedObject )
		{
			// hack to draw focus on cursor
			focusedObject = cursor;
			BaseObject::Draw( deltatime, parentPos, parentPosSrc, enabled );
			focusedObject = this;
		}
		else
			BaseObject::Draw( deltatime, parentPos, parentPosSrc, enabled );
	}

	void Slider::BaseKeyDown( int key, int charVal )
	{
		if ( key == Importer::keyDown || key == Importer::keyPadDown || key == Importer::keyRight || key == Importer::keyPadRight )
			doPlus();
		else if ( key == Importer::keyUp || key == Importer::keyPadUp || key == Importer::keyLeft || key == Importer::keyPadLeft )
			doMinus();
		else
			BaseObject::BaseKeyDown( key, charVal );
	}

	void Slider::refreshCursor( void )
	{
		int interval = maxvalue - minvalue;
		float percent;
		float x, y;

		if ( interval )
			percent = float(curvalue - minvalue) / float(interval);
		else
			percent = 0.0;

		cursor->getPosition( x, y );
		if ( horizontal )
			x = float( percent * ( scrollbar->getWidth() - cursor->getWidth() ) );
		else
			y = float( percent * ( scrollbar->getHeight() - cursor->getHeight() ) );

		cursor->setPosition( x, y );
	}

	void Slider::doMinus( void )
	{
		int oldvalue = curvalue;
		curvalue = max( minvalue, curvalue - buttonstep );
		refreshCursor();
		if ( oldvalue != curvalue && ValueChange )
			ValueChange( this, oldvalue, curvalue );
	}

	void Slider::doPlus( void )
	{
		int oldvalue = curvalue;
		curvalue = min( maxvalue, curvalue + buttonstep );
		refreshCursor();
		if ( oldvalue != curvalue && ValueChange )
			ValueChange( this, oldvalue, curvalue );
	}

	void Slider::doScrollMinus( void )
	{
		int oldvalue = curvalue;
		curvalue = max( minvalue, curvalue - clickstep );
		refreshCursor();
		if ( oldvalue != curvalue && ValueChange )
			ValueChange( this, oldvalue, curvalue );
	}

	void Slider::doScrollPlus( void )
	{
		int oldvalue = curvalue;
		curvalue = min( maxvalue, curvalue + clickstep );
		refreshCursor();
		if ( oldvalue != curvalue && ValueChange )
			ValueChange( this, oldvalue, curvalue );
	}

	void Slider::setHorizontal( bool horizontal )
	{
		this->horizontal = horizontal;
		setSize( getWidth(), getHeight() );
	}

	void Slider::setBoundValues( int minVal, int maxVal )
	{
		minvalue = minVal;
		maxvalue = maxVal;
		curvalue = bound( curvalue, minvalue, maxvalue );
		refreshCursor();
	}

	void Slider::setMinValue( int value )
	{
		minvalue = value;
		curvalue = bound( curvalue, minvalue, maxvalue );
		refreshCursor();
	}

	void Slider::setMaxValue( int value )
	{
		maxvalue = value;
		curvalue = bound( curvalue, minvalue, maxvalue );
		refreshCursor();
	}

	void Slider::setCurValue( int value )
	{
		curvalue = bound( value, minvalue, maxvalue );
		refreshCursor();
	}

	void Slider::setButtonStepValue( int value )
	{
		buttonstep = value;
	}

	void Slider::setBarClickStepValue( int value )
	{
		clickstep = value;
	}

	void Slider::setStepValues( int buttonStep, int barclickStep )
	{
		buttonstep = buttonStep;
		clickstep = barclickStep;
	}

	// Scrollbar panel
	void Slider::setSize( float w, float h )
	{
		BaseObject::setSize( w, h );
		if ( horizontal )
		{
			minus->setSize( h, h );
			plus->setPosition( w - h, 0 );
			plus->setSize( h, h );
			scrollbar->setPosition( h, 0 );
			scrollbar->setSize( w - 2*h, h );
			cursor->setSize( h, h );
		}
		else
		{
			minus->setSize( w, w );
			plus->setPosition( 0, h - w );
			plus->setSize( w, w );
			scrollbar->setPosition( 0, w );
			scrollbar->setSize( w, h - 2*w );
			cursor->setSize( w, w );
		}
		refreshCursor();
	}

	void Slider::setBackColor( const Color &color )
	{
		scrollbar->setBackColor( color );
	}

	void Slider::setBorderColor( const Color &color )
	{
		scrollbar->setBorderColor( color );
	}

	void Slider::setDisabledColor( const Color &color )
	{
		scrollbar->setDisabledColor( color );
	}

	void Slider::setBackgroundImage( Image image )
	{
		scrollbar->setBackgroundImage( image );
	}

	void Slider::setDisabledImage( Image image )
	{
		scrollbar->setDisabledImage( image );
	}

	void Slider::setBorderWidth( const float width )
	{
		scrollbar->setBorderWidth( width );
	}

	// Buttons
	void Slider::setButtonBackColor( const Color &color )
	{
		minus->setBackColor( color );
		plus->setBackColor( color );
	}

	void Slider::setButtonBorderColor( const Color &color )
	{
		minus->setBorderColor( color );
		plus->setBorderColor( color );
	}

	void Slider::setButtonDisabledColor( const Color &color )
	{
		minus->setDisabledColor( color );
		plus->setDisabledColor( color );
	}

	void Slider::setButtonBackgroundImage( Image image )
	{
		minus->setBackgroundImage( image );
		plus->setBackgroundImage( image );
	}

	void Slider::setButtonDisabledImage( Image image )
	{
		minus->setDisabledImage( image );
		plus->setDisabledImage( image );
	}

	void Slider::setButtonBorderWidth( const float width )
	{
		minus->setBorderWidth( width );
		plus->setBorderWidth( width );
	}

	void Slider::setButtonHighlightColor( const Color &color )
	{
		minus->setHighlightColor( color );
		plus->setHighlightColor( color );
	}

	void Slider::setButtonHighlightImage( Image image )
	{
		minus->setHighlightImage( image );
		plus->setHighlightImage( image );
	}

	// Cursor
	void Slider::setCursorBackColor( const Color &color )
	{
		cursor->setBackColor( color );
	}

	void Slider::setCursorBorderColor( const Color &color )
	{
		cursor->setBorderColor( color );
	}

	void Slider::setCursorDisabledColor( const Color &color )
	{
		cursor->setDisabledColor( color );
	}

	void Slider::setCursorBackgroundImage( Image image )
	{
		cursor->setBackgroundImage( image );
	}

	void Slider::setCursorDisabledImage( Image image )
	{
		cursor->setDisabledImage( image );
	}

	void Slider::setCursorBorderWidth( const float width )
	{
		cursor->setBorderWidth( width );
	}

	void Slider::setCursorHighlightColor( const Color &color )
	{
		cursor->setHighlightColor( color );
	}

	void Slider::setCursorHighlightImage( Image image )
	{
		cursor->setHighlightImage( image );
	}

	bool Slider::isHorizontal( void ) const
	{
		return horizontal;
	}

	int Slider::getMinValue( void ) const
	{
		return minvalue;
	}

	int Slider::getMaxValue( void ) const
	{
		return maxvalue;
	}
	
	int Slider::getCurValue( void ) const
	{
		return curvalue;
	}

	int Slider::getButtonStepValue( void ) const
	{
		return buttonstep;
	}

	int Slider::getBarClickStepValue( void ) const
	{
		return clickstep;
	}

	// Scrollbar
	Color Slider::getBackColor( void ) const
	{
		return scrollbar->getBackColor();
	}

	Color Slider::getBorderColor( void ) const
	{
		return scrollbar->getBorderColor();
	}

	Color Slider::getDisabledColor( void ) const
	{
		return scrollbar->getDisabledColor();
	}

	Image Slider::getBackgroundImage( void ) const
	{
		return scrollbar->getBackgroundImage();
	}

	Image Slider::getDisabledImage( void ) const
	{
		return scrollbar ? scrollbar->getDisabledImage() : NULL;
	}

	float Slider::getBorderWidth( void ) const
	{
		return scrollbar->getBorderWidth();
	}

	// Buttons
	Color Slider::getButtonBackColor( void ) const
	{
		return minus->getBackColor();
	}

	Color Slider::getButtonBorderColor( void ) const
	{
		return minus->getBorderColor();
	}

	Color Slider::getButtonDisabledColor( void ) const
	{
		return minus->getDisabledColor();
	}

	Image Slider::getButtonBackgroundImage( void ) const
	{
		return minus->getBackgroundImage();
	}

	Image Slider::getButtonDisabledImage( void ) const
	{
		return minus->getDisabledImage();
	}

	float Slider::getButtonBorderWidth( void ) const
	{
		return minus->getBorderWidth();
	}

	Color Slider::getButtonHighlightColor( void ) const
	{
		return minus->getHighlightColor();
	}

	Image Slider::getButtonHighlightImage( void ) const
	{
		return minus->getHighlightImage();
	}

	// Cursor
	Color Slider::getCursorBackColor( void ) const
	{
		return cursor->getBackColor();
	}

	Color Slider::getCursorBorderColor( void ) const
	{
		return cursor->getBorderColor();
	}

	Color Slider::getCursorDisabledColor( void ) const
	{
		return cursor->getDisabledColor();
	}

	Image Slider::getCursorBackgroundImage( void ) const
	{
		return cursor->getBackgroundImage();
	}

	Image Slider::getCursorDisabledImage( void ) const
	{
		return cursor->getDisabledImage();
	}

	float Slider::getCursorBorderWidth( void ) const
	{
		return cursor->getBorderWidth();
	}

	Color Slider::getCursorHighlightColor( void ) const
	{
		return cursor->getHighlightColor();
	}

	Image Slider::getCursorHighlightImage( void ) const
	{
		return cursor->getHighlightImage();
	}
	
	void Slider::MinusClickHandler( BaseObject *target )
	{
		Slider *slider = static_cast<Slider*>( target->getParent() );
		slider->doMinus();
	}

	void Slider::PlusClickHandler( BaseObject *target )
	{
		Slider *slider = static_cast<Slider*>( target->getParent() );
		slider->doPlus();
	}
	
	void Slider::ScrollbarMouseDownClickHandler( BaseObject *target, float x, float y, int button )
	{
		Slider *slider = static_cast<Slider*>( target->getParent() );
		float cx, cy;
		if ( button != Importer::clickButton )
			return;

		slider->cursor->getPosition( cx, cy );
		if ( slider->horizontal )
		{
			if ( x < cx )
				slider->doScrollMinus();
			else
				slider->doScrollPlus();
		}
		else
		{
			if ( y < cy )
				slider->doScrollMinus();
			else
				slider->doScrollPlus();
		}
	}

	void Slider::DraggingCursorHandler( BaseObject *target, float, float )
	{
		Slider *slider = static_cast<Slider*>( target->getParent()->getParent() );
		float x, y;
		float percent;
		slider->cursor->getPosition( x, y );
		if ( slider->horizontal )
		{
			x = bound( x, 0.0f, slider->scrollbar->getWidth() - slider->cursor->getWidth() );
			percent = float(x) / float( slider->scrollbar->getWidth() - slider->cursor->getWidth() );
			slider->cursor->setPosition( x, 0.0f );
		}
		else
		{
			y = bound( y, 0.0f, slider->scrollbar->getHeight() - slider->cursor->getHeight() );
			percent = float(y) / float( slider->scrollbar->getHeight() - slider->cursor->getHeight() );
			slider->cursor->setPosition( 0.0f, y );
		}
		int oldvalue = slider->curvalue;
		int interval = slider->maxvalue - slider->minvalue;
		slider->curvalue = bound( int(slider->minvalue + percent * interval + 0.5f), slider->minvalue, slider->maxvalue );
		if ( oldvalue != slider->curvalue && slider->ValueChange )
			slider->ValueChange( slider, oldvalue, slider->curvalue );
	}

	void Slider::MouseUpCursorHandler( BaseObject *target, float, float, int )
	{
		Slider *slider = static_cast<Slider*>( target->getParent()->getParent() );
		slider->refreshCursor();
	}

	void Slider::setValueChangeHandler( void (*ValueChange)( BaseObject *target, int oldvalue, int newvalue ) )
	{
		this->ValueChange = ValueChange;
	}

	std::string Slider::getType( void ) const
	{
		return UICORE_TYPE_SLIDER;
	}
}
