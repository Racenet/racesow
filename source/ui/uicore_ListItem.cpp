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
	ALLOCATOR_DEFINITION(ListItem)
	DELETER_DEFINITION(ListItem)

	void ListItem::Initialize( void )
	{
		setFocusable( false );
	}

	ListItem::ListItem()
		: SwitchButton()
	{
		Initialize();
	}

	ListItem::ListItem( float x, float y, float w, float h, std::string caption )
		: SwitchButton( NULL, x, y, w, h, caption )
	{
		Initialize();
	}

	ListItem::~ListItem()
	{

	}

	void ListItem::DrawBackground( const Rect &posSrc, const Rect &posDest, bool enabled )
	{
		Rect rect;
		rect.x = posDest.x + posSrc.x;
		rect.y = posDest.y + posSrc.y;
		rect.w = posSrc.w;
		rect.h = posSrc.h;
		if ( !enabled || !isEnabled() )
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
			if ( mouseOverObject == this /*|| focused*/ )
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
}
