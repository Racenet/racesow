/*
Copyright (C) 2008 Will Franklin

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
#include "uiwsw_Utils.h"

using namespace UIWsw;

#define INDEX( button ) ( ( int )( log( ( float )button ) / log( 2.0f ) ) )
#define BUTTON( index ) ( ( MessageBoxButton )( ( int )pow( 2.0f, index ) ) )

namespace UICore
{
	MessageBox *MessageBox::activeMessageBox = NULL;

	ALLOCATOR_DEFINITION( MessageBox );
	DELETER_DEFINITION( MessageBox );

	void MessageBox::Initialize( void )
	{
		pBackground = Factory::newPanel( rootPanel, 0, 0, 1024, 768 );
		pBackground->setBorderWidth( 0 );
		pBackground->setVisible( false );

		lblCaption = Factory::newLabel( this, 0, 0, 0, 0 );

		btnButtons[INDEX( BUTTON_CANCEL )] = Factory::newButton( this, 0, 0, 90, BUTTON_HEIGHT, "Cancel" );
		btnButtons[INDEX( BUTTON_OK )] = Factory::newButton( this, 0, 0, 60, BUTTON_HEIGHT, "OK" );
		btnButtons[INDEX( BUTTON_YES )] = Factory::newButton( this, 0, 0, 60, BUTTON_HEIGHT, "Yes" );
		btnButtons[INDEX( BUTTON_NO )] = Factory::newButton( this, 0, 0, 60, BUTTON_HEIGHT, "No" );
		btnButtons[INDEX( BUTTON_RETRY )] = Factory::newButton( this, 0, 0, 80, BUTTON_HEIGHT, "Retry" );

		pImage = Factory::newPanel( this, 0, 0, 0, 0 );
		pImage->setBorderWidth( 0 );
		pImage->setBackColor( Color( 0, 0, 0, 0 ) );

		setCaption( "" );
		setResultHandler( NULL );

		setParent( pBackground );

		for( int i = 0 ; i < BUTTON_COUNT ; i++ )
			btnButtons[i]->setClickHandler( MessageBox::clickHandler );
	}

	MessageBox::MessageBox( void )
		: Panel()
	{
		Initialize();
	}

	MessageBox::MessageBox( std::string caption, int buttons, MessageBoxImage image )
		: Panel( NULL, 0, 0, 0, 0 )
	{
		Initialize();
		updateDisplay( caption, buttons, image );
		Show();
	}

	MessageBox::~MessageBox( void )
	{
		delete pBackground;
		delete pImage;
		delete lblCaption;
		for ( int i = 0 ; i < BUTTON_COUNT ; i++ )
			delete btnButtons[i];
	}

	void MessageBox::updateDisplay( std::string caption, int buttons, MessageBoxImage image )
	{
		setCaption( caption );
		setButtons( buttons );
		setImage( image );
	}

	void MessageBox::setCaption( std::string caption )
	{
		this->lblCaption->setCaption( caption );

		updateLayout();
	}

	void MessageBox::setButtons( int buttons )
	{
		for( int i = 0 ; i < BUTTON_COUNT ; i++ )
			btnButtons[i]->setVisible( ( buttons & BUTTON( i ) ) > 0 );

		updateLayout();
	}

	void MessageBox::setImage( MessageBoxImage type )
	{
		Image image = NULL;

		switch( type )
		{
		case IMAGE_NONE:
			imagew = 0;
			imageh = 0;
			break;
		case IMAGE_LOADING:
			image = Trap::R_RegisterPic( "gfx/ui/loading" );
			imagew = 40;
			imageh = 40;
			break;
		case IMAGE_WARNING:
			image = Trap::R_RegisterPic( "gfx/ui/warning" );
			imagew = 23;
			imageh = 50;
			break;
		default:
			return;
		}

		pImage->setBackgroundImage( image );
		pImage->setSize( imagew, imageh );

		updateLayout();
	}

	void MessageBox::setImage( Image customImage, float width, float height )
	{
		imagew = width;
		imageh = height;

		pImage->setBackgroundImage( customImage );
		pImage->setSize( width, height );

		updateLayout();
	}

	void MessageBox::updateLayout( void )
	{
		float totalheight, totalwidth, yoffset;
		float width, height, x, y;
		float msgboxwidth, msgboxheight;

		int fontheight = Trap::SCR_strHeight( lblCaption->getFont() ),
			fontwidth = ( int )Trap::SCR_strWidth( lblCaption->getCaption().c_str(), lblCaption->getFont(), 0 );

		// calculate the total height of everything in the msgbox
		totalheight = fontheight;
		if( pImage )
			totalheight = max( imageh, totalheight ); // image is next to caption

		// add height of buttons if they exist
		for( int i = 0 ; i < BUTTON_COUNT ; i++ )
		{
			if( btnButtons[i]->isVisible() )
			{
				totalheight += BUTTON_HEIGHT + 2 * BUTTON_SPACING; // spacing between caption and button
				break;
			}
		}
		
		// check that everything will fit inside the box

		// we only need to check the width of the caption
		// buttons should never exceed standard msgbox width limit
		totalwidth = ( pImage ? ( imagew + 20 ) : 0 ) + fontwidth;
		
		msgboxwidth = ( totalwidth + 40 ) > MSGBOX_WIDTH ? ( totalwidth + 40 ) : MSGBOX_WIDTH;
		msgboxheight = ( totalheight + 40 ) > MSGBOX_HEIGHT ? ( totalheight + 40 ) : MSGBOX_HEIGHT;

		setSize( msgboxwidth, msgboxheight );
		setPosition( ( 1024 - msgboxwidth ) / 2.0f, ( 768 - msgboxheight ) / 2.0f );

		// move buttons, caption and image into the right place
		yoffset = ( msgboxheight - totalheight ) / 2.0f;

		// put the image next to caption
		if( pImage )
		{
			width = imagew + fontwidth + 20;
			height = max( imageh, fontheight );

			x = ( msgboxwidth - width ) / 2.0f;
			y = yoffset + ( height - imageh ) / 2.0f;
			pImage->setPosition( x, y );

			x += imagew + 20;
			y = yoffset + ( height - fontheight ) / 2.0f;
			lblCaption->setPosition( x, y );

			yoffset += height;
		}
		// just align the caption to the middle
		else
		{
			x = ( msgboxwidth - fontwidth ) / 2.0f;
			lblCaption->setPosition( x, yoffset );
			
			yoffset += fontheight;
		}

		lblCaption->setSize( fontwidth, fontheight );

		// space between buttons and caption/image
		yoffset += 2 * BUTTON_SPACING;

		// put buttons in the center of the message box
		width = 0;
		for( int i = 0 ; i < BUTTON_COUNT ; i++ )
		{
			if( btnButtons[i]->isVisible() )
				width += btnButtons[i]->getWidth() + BUTTON_SPACING;
		}

		// Remove excess spacing
		width -= BUTTON_SPACING;

		x = ( msgboxwidth - width ) / 2.0f;
		for( int i = 0 ; i < BUTTON_COUNT ; i++ )
		{
			if( btnButtons[i]->isVisible() )
			{
				btnButtons[i]->setPosition( x, yoffset );
				x += btnButtons[i]->getWidth() + BUTTON_SPACING;
			}
		}
	}

	void MessageBox::Show( void )
	{
		updateLayout();
		activeMessageBox = this;
		pBackground->setVisible( true );
	}

	void MessageBox::Hide( void )
	{
		pBackground->setVisible( false );

		activeMessageBox = NULL;
	}

	void MessageBox::clickHandler( BaseObject *button )
	{
		// This shouldn't happen
		if( !activeMessageBox )
			return;


		if( activeMessageBox->resultHandler )
		{
			for( int i = 0 ; i < BUTTON_COUNT ; i++ )
			{
				if( button == activeMessageBox->btnButtons[i] )
					activeMessageBox->resultHandler( BUTTON( i ) );
			}
		}
	}
}
