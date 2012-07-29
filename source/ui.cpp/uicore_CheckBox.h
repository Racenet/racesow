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


#ifndef _UICORE_CHECKBOX_H_
#define _UICORE_CHECKBOX_H_

#include "uicore_Button.h"
#include "uicore_Label.h"

namespace UICore
{
	class CheckBox : public Panel
	{
	protected:

		Label *label;
		SwitchButton *button;
		Alignment align;

		static void SwitchHandler( BaseObject *target, bool newValue );
		void (*Switch)( BaseObject *target, bool newValue );

		virtual void Initialize( void );

	public:
		CheckBox();
		CheckBox( BaseObject *parent, float x, float y, float w, float h, std::string caption = "" );
		virtual ~CheckBox();

		MEMORY_OPERATORS

		void doSwitch();

		// BaseObject
		virtual void setSize( float w, float h );
		inline virtual void setBackColor( const Color &color );
		inline virtual void setBorderColor( const Color &color );
		inline virtual void setDisabledColor( const Color &color );
		inline virtual void setBackgroundImage( Image image );
		inline virtual void setDisabledImage( Image image );
		inline virtual void setBorderWidth( const float width );

		// Label
		inline virtual void setFont( Font font );
		inline virtual void setFontColor( const Color &color );
		inline virtual void setDisabledFontColor( const Color &color );
		inline virtual void setCaption( const std::string caption );
		inline virtual void setAlign( Alignment align );

		// Switch button
		inline virtual void setHighlightColor( const Color &color );
		inline virtual void setHighlightImage( Image image );
		inline virtual void setPressed( bool v );
		inline virtual void setPressedColor( const Color &color );
		inline virtual void setPressedHighlightColor( const Color &color );
		inline virtual void setPressedImage( Image image );
		inline virtual void setPressedHighlightImage( Image image );

		// CheckBox
		inline virtual void setLabelBackColor( const Color &color );

		// BaseObject
		inline virtual Color getBackColor( void ) const;
		inline virtual Color getBorderColor( void ) const;
		inline virtual Color getDisabledColor( void ) const;
		inline virtual Image getBackgroundImage( void ) const;
		inline virtual Image getDisabledImage( void ) const;
		inline virtual float getBorderWidth( void ) const;

		// Label
		inline virtual Font getFont( void ) const;
		inline virtual Color getFontColor( void ) const;
		inline virtual Color getDisabledFontColor( void ) const;
		inline virtual std::string getCaption( void ) const;
		inline virtual Alignment getAlign( void ) const;

		// Switch button
		inline virtual Color getHighlightColor( void ) const;
		inline virtual Image getHighlightImage( void ) const;
		inline virtual bool isPressed( void ) const;
		inline virtual Color getPressedColor( void ) const;
		inline virtual Color getPressedHighlightColor( void ) const;
		inline virtual Image getPressedImage( void ) const;
		inline virtual Image getPressedHighlightImage( void ) const;

		// CheckBox
		inline virtual Color getLabelBackColor( void ) const;

		virtual void setSwitchHandler( void (*Switch)( BaseObject *target, bool newValue ) );

		virtual std::string getType( void ) const;
	};

	void CheckBox::setBackColor( const Color &color )
	{
		button->setBackColor( color );
	}

	void CheckBox::setBorderColor( const Color &color )
	{
		button->setBorderColor( color );
	}

	void CheckBox::setDisabledColor( const Color &color )
	{
		button->setDisabledColor( color );
	}

	void CheckBox::setBackgroundImage( Image image )
	{
		button->setBackgroundImage( image );
	}

	void CheckBox::setDisabledImage( Image image )
	{
		button->setDisabledImage( image );
	}

	void CheckBox::setBorderWidth( float width )
	{
		button->setBorderWidth( width );
	}

	void CheckBox::setLabelBackColor( const Color &color )
	{
		Panel::setBackColor( color );
		label->setBackColor( color );
	}

	void CheckBox::setFont( Font font )
	{
		label->setFont( font );
	}

	void CheckBox::setFontColor( const Color &color )
	{
		label->setFontColor( color );
	}

	void CheckBox::setDisabledFontColor( const Color &color )
	{
		label->setDisabledFontColor( color );
	}

	void CheckBox::setCaption( const std::string caption )
	{
		label->setCaption( caption );
	}

	void CheckBox::setAlign( Alignment align )
	{
		this->align = align;
		label->setAlign( align );
		setSize( position.w, position.h );
	}

	void CheckBox::setHighlightColor( const Color &color )
	{
		button->setHighlightColor( color );
	}

	void CheckBox::setHighlightImage( Image image )
	{
		button->setHighlightImage( image );
	}

	void CheckBox::setPressed( bool v )
	{
		button->setPressed( v );
	}

	void CheckBox::setPressedColor( const Color &color )
	{
		button->setPressedColor( color );
	}

	void CheckBox::setPressedHighlightColor( const Color &color )
	{
		button->setPressedHighlightColor( color );
	}

	void CheckBox::setPressedImage( Image image )
	{
		button->setPressedImage( image );
	}

	void CheckBox::setPressedHighlightImage( Image image )
	{
		button->setPressedHighlightImage( image );
	}

	Color CheckBox::getBackColor( void ) const
	{
		return button->getBackColor();
	}

	Color CheckBox::getBorderColor( void ) const
	{
		return button->getBorderColor();
	}

	Color CheckBox::getDisabledColor( void ) const
	{
		return button->getDisabledColor();
	}

	Image CheckBox::getBackgroundImage( void ) const
	{
		return button->getBackgroundImage();
	}

	Image CheckBox::getDisabledImage( void ) const
	{
		return button ? button->getDisabledImage() : NULL;
	}

	float CheckBox::getBorderWidth( void ) const
	{
		return button->getBorderWidth();
	}

	Color CheckBox::getLabelBackColor( void ) const
	{
		return label->getBackColor();
	}

	Font CheckBox::getFont( void ) const
	{
		return label->getFont();
	}

	Color CheckBox::getFontColor( void ) const
	{
		return label->getFontColor();
	}

	Color CheckBox::getDisabledFontColor( void ) const
	{
		return label->getDisabledFontColor();
	}

	std::string CheckBox::getCaption( void ) const
	{
		return label->getCaption();
	}

	Alignment CheckBox::getAlign( void ) const
	{
		return label->getAlign();
	}

	Color CheckBox::getHighlightColor( void ) const
	{
		return button->getHighlightColor();
	}

	Image CheckBox::getHighlightImage( void ) const
	{
		return button->getHighlightImage();
	}

	bool CheckBox::isPressed( void ) const
	{
		return button->isPressed();
	}

	Color CheckBox::getPressedColor( void ) const
	{
		return button->getPressedColor();
	}

	Color CheckBox::getPressedHighlightColor( void ) const
	{
		return button->getPressedHighlightColor();
	}

	Image CheckBox::getPressedImage( void ) const
	{
		return button->getPressedImage();
	}

	Image CheckBox::getPressedHighlightImage( void ) const
	{
		return button->getPressedHighlightImage();
	}
}

#endif
