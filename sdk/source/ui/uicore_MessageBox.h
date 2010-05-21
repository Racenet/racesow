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

#ifndef _UICORE_MSGBOX_H_
#define _UICORE_MSGBOX_H_

#include "uicore_Panel.h"
#include "uicore_Label.h"
#include "uicore_Button.h"

namespace UICore
{
	const float MSGBOX_WIDTH = 400.0f;
	const float MSGBOX_HEIGHT = 200.0f;
	const float BUTTON_SPACING = 10.0f;
	const float BUTTON_HEIGHT = 30.0f;
	const int BUTTON_COUNT = 5;

	typedef enum
	{
		BUTTON_OK = 1,
		BUTTON_YES = 2,
		BUTTON_NO = 4,
		BUTTON_RETRY = 8,
		BUTTON_CANCEL = 16
	} MessageBoxButton;

	typedef enum
	{
		IMAGE_NONE,
		IMAGE_LOADING,
		IMAGE_WARNING
	} MessageBoxImage;

	class MessageBox : public Panel
	{
	protected:
		Panel *pBackground, *pImage;
		Label *lblCaption;
		Button *btnButtons[BUTTON_COUNT];
		float imagew, imageh;

		void ( *resultHandler )( MessageBoxButton );

		virtual void Initialize( void );
		virtual void updateLayout( void );

		static MessageBox *activeMessageBox;
		static void clickHandler( BaseObject *button );

	public:
		MessageBox();
		MessageBox( std::string caption, int buttons = BUTTON_OK, MessageBoxImage image = IMAGE_NONE );
		~MessageBox();

		void Show( void );
		void Hide( void );

		MEMORY_OPERATORS

		inline void setResultHandler( void ( *resultHandler )( MessageBoxButton ) );
		void updateDisplay( std::string caption, int buttons, MessageBoxImage image );
		void setCaption( std::string caption );
		void setButtons( int buttons );
		void setImage( MessageBoxImage image );
		void setImage( Image customImage, float width, float height );
	};

	void MessageBox::setResultHandler( void ( *resultHandler )( MessageBoxButton ) )
	{
		this->resultHandler = resultHandler;
	}
}

#endif
