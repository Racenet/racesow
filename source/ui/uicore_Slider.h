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


#ifndef _UICORE_SLIDER_H_
#define _UICORE_SLIDER_H_

#include "uicore_Button.h"
#include "uicore_Label.h"

namespace UICore
{
	class Slider : public BaseObject
	{
	protected:

		Button *minus;
		Button *plus;
		Button *cursor;
		Button *scrollbar;

		bool horizontal;
		int minvalue;
		int maxvalue;
		int curvalue;
		int buttonstep;
		int clickstep;
		static void MinusClickHandler( BaseObject *target );
		static void PlusClickHandler( BaseObject *target );
		static void ScrollbarMouseDownClickHandler( BaseObject *target, float x, float y, int button );
		static void DraggingCursorHandler( BaseObject *target, float, float );
		static void MouseUpCursorHandler( BaseObject *target, float, float, int );

		void (*ValueChange)( BaseObject *target, int oldvalue, int newvalue );

		void refreshCursor( void );

		virtual void BaseKeyDown( int key, int charVal );

		virtual void Initialize( void );

	public:
		Slider();
		Slider( BaseObject *parent, float x, float y, float w, float h, bool horizontal );
		virtual ~Slider();

		MEMORY_OPERATORS

		virtual void Draw( unsigned int deltatime, const Rect *parentPos = NULL, const Rect *parentPosSrc = NULL, bool enabled = true );

		void doMinus( void );
		void doPlus( void );
		void doScrollMinus( void );
		void doScrollPlus( void );

		virtual void setHorizontal( bool horizontal );
		virtual void setMinValue( int value );
		virtual void setMaxValue( int value );
		virtual void setBoundValues( int minVal, int maxVal );
		virtual void setCurValue( int value );
		virtual void setButtonStepValue( int value );
		virtual void setBarClickStepValue( int value );
		virtual void setStepValues( int buttonStep, int barclickStep );

		// Scrollbar panel
		virtual void setSize( float w, float h );
		virtual void setBackColor( const Color &color );
		virtual void setBorderColor( const Color &color );
		virtual void setDisabledColor( const Color &color );
		virtual void setBackgroundImage( Image image );
		virtual void setDisabledImage( Image image );
		virtual void setBorderWidth( const float width );

		// Buttons
		virtual void setButtonBackColor( const Color &color );
		virtual void setButtonBorderColor( const Color &color );
		virtual void setButtonDisabledColor( const Color &color );
		virtual void setButtonBackgroundImage( Image image );
		virtual void setButtonDisabledImage( Image image );
		virtual void setButtonBorderWidth( const float width );
		virtual void setButtonHighlightColor( const Color &color );
		virtual void setButtonHighlightImage( Image image );

		// Cursor
		virtual void setCursorBackColor( const Color &color );
		virtual void setCursorBorderColor( const Color &color );
		virtual void setCursorDisabledColor( const Color &color );
		virtual void setCursorBackgroundImage( Image image );
		virtual void setCursorDisabledImage( Image image );
		virtual void setCursorBorderWidth( const float width );
		virtual void setCursorHighlightColor( const Color &color );
		virtual void setCursorHighlightImage( Image image );

		virtual bool isHorizontal( void ) const;
		virtual int getMinValue( void ) const;
		virtual int getMaxValue( void ) const;
		virtual int getCurValue( void ) const;
		virtual int getButtonStepValue( void ) const;
		virtual int getBarClickStepValue( void ) const;

		// Scrollbar
		virtual Color getBackColor( void ) const;
		virtual Color getBorderColor( void ) const;
		virtual Color getDisabledColor( void ) const;
		virtual Image getBackgroundImage( void ) const;
		virtual Image getDisabledImage( void ) const;
		virtual float getBorderWidth( void ) const;

		// Buttons
		virtual Color getButtonBackColor( void ) const;
		virtual Color getButtonBorderColor( void ) const;
		virtual Color getButtonDisabledColor( void ) const;
		virtual Image getButtonBackgroundImage( void ) const;
		virtual Image getButtonDisabledImage( void ) const;
		virtual float getButtonBorderWidth( void ) const;
		virtual Color getButtonHighlightColor( void ) const;
		virtual Image getButtonHighlightImage( void ) const;

		// Cursor
		virtual Color getCursorBackColor( void ) const;
		virtual Color getCursorBorderColor( void ) const;
		virtual Color getCursorDisabledColor( void ) const;
		virtual Image getCursorBackgroundImage( void ) const;
		virtual Image getCursorDisabledImage( void ) const;
		virtual float getCursorBorderWidth( void ) const;
		virtual Color getCursorHighlightColor( void ) const;
		virtual Image getCursorHighlightImage( void ) const;

		virtual void setValueChangeHandler( void (*ValueChange)( BaseObject *target, int oldvalue, int newvalue ) );

		virtual std::string getType( void ) const;
	};
}

#endif
