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
	ALLOCATOR_DEFINITION(TitleBar)
	DELETER_DEFINITION(TitleBar)

	void TitleBar::Initialize( void )
	{
		padLeft = 0;
	}

	TitleBar::TitleBar()
		: Label()
	{
		Initialize();
	}

	TitleBar::TitleBar( BaseObject *parent, float x, float y, float w, float h, std::string caption )
		: Label( parent, x, y, w, h, caption )
	{
		Initialize();
	}

	void TitleBar::FitTextToRect( const Rect &objectSrc, const Rect &objectDest, Rect &textSrc, Rect &textDest, const char *buf, float padLeft, float padTop )
	{
		Label::FitTextToRect( objectSrc, objectDest, textSrc, textDest, buf, padLeft + this->padLeft, padTop );
	}
}
