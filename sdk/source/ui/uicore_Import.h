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



#ifndef _UICORE_IMPORT_H_
#define _UICORE_IMPORT_H_


#include "uicore_Types.h"

namespace UICore
{
	class Importer
	{
	public:

		static void* newFunc( unsigned int size );
		static void deleteFunc( void *p );

		static void DrawImage( Image img, const Rect &src, const Rect &dest, const Color &color );
		static void FixRect( Rect &rect );
		static void FillRect( const Rect &rect, const Color &color );

		static float StringWidth( const char *text, const Font font, int maxlen = 0 );
		static float StringHeight( const Font font );
		static void DrawString( const Rect &src, const Rect &dest, const char *text, const Font font, const Color &color );

		static const int clickButton;
		static const int mouseWheelUp;
		static const int mouseWheelDown;

		static const int keyLeft;
		static const int keyRight;
		static const int keyUp;
		static const int keyDown;
		static const int keyBackspace;
		static const int keyDelete;
		static const int keyInsert;
		static const int keyEnter;
		static const int keySpace;
		static const int keyPgUp;
		static const int keyPgDn;

		static const int keyPadLeft;
		static const int keyPadRight;
		static const int keyPadUp;
		static const int keyPadDown;
		static const int keyPadDelete;
		static const int keyPadInsert;
		static const int keyPadEnter;
		static const int keyPadPgUp;
		static const int keyPadPgDn;

		static const int keyTab;
		static const int keyShift;
		static const int keyCtrl;
		static const int keyAlt;
	};
}

#endif
