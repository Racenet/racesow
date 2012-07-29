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
#include "../gameshared/q_shared.h"
#include "../gameshared/q_math.h"
#include "../gameshared/q_cvar.h"
#include "../gameshared/q_dynvar.h"
#include "../gameshared/gs_ref.h"
#include "../gameshared/q_keycodes.h"
#include "uiwsw_Utils.h"
#include "uicore_Import.h"

using namespace UIWsw;

#define D_ROUND(x)	round(x)
#define R_ROUND(r)	{ r.x = D_ROUND(r.x); r.y = D_ROUND(r.y); r.w = D_ROUND(r.w); r.h = D_ROUND(r.h); }

namespace UICore
{
	void* Importer::newFunc( unsigned int size )
	{
		return UIMem::Malloc( size );
	}

	void Importer::deleteFunc( void *p )
	{
		UIMem::Free( p );
	}

	void Importer::FixRect( Rect &rect )
	{
		// !! IMPORTANT !! - prevent rounding errors
		rect.w = round( (rect.w + rect.x) * Local::getScaleX() ) - round(rect.x * Local::getScaleX());
		rect.h = round( (rect.h + rect.y) * Local::getScaleY() ) - round(rect.y * Local::getScaleY());
		rect.x = round(rect.x * Local::getScaleX());
		rect.y = round(rect.y * Local::getScaleY());
	}

	void Importer::DrawImage( Image img, const Rect &src, const Rect &dest, const Color &color, const float angle )
	{
		Rect pos;
		pos.x = dest.x + src.x; 
		pos.y = dest.y + src.y;
		pos.w = max( 0, src.w );
		pos.h = max( 0, src.h );
		FixRect(pos);

		if( fabs( angle ) < 0.001 ) {
			Trap::R_DrawStretchPic( (int)pos.x, (int)pos.y, (int)pos.w, (int)pos.h, 0, 0, 1, 1, (float*)(&color), img );
		} else {
			Trap::R_DrawRotatedStretchPic( (int)pos.x, (int)pos.y, (int)pos.w, (int)pos.h, 0, 0, 1, 1, angle, (float*)(&color), img );
		}
	}

	void Importer::FillRect( const Rect &rect, const Color &color )
	{
		Rect pos;
		pos.x = rect.x;
		pos.y = rect.y;
		pos.w = max( 0, rect.w );
		pos.h = max( 0, rect.h );
		FixRect(pos);
		Trap::R_DrawStretchPic( (int)pos.x, (int)pos.y, (int)pos.w, (int)pos.h, 0, 0, 1, 1, (float*)(&color), Local::getWhiteShader() );
	}

	float Importer::StringWidth( const char *text, const Font font, int maxlen )
	{
		struct mufont_s *ft;

		if( !font )
			ft = Local::getFontSmall();
		else
			ft = (struct mufont_s *)font;

		return Trap::SCR_strWidth( text, ft, maxlen ) / Local::getScaleX();
	}

	float Importer::StringHeight( const Font font )
	{
		struct mufont_s *ft;

		if( !font )
			ft = Local::getFontSmall();
		else
			ft = (struct mufont_s *)font;

		return Trap::SCR_strHeight( ft ) / Local::getScaleY();
	}

	void Importer::DrawString( const Rect &src, const Rect &dest, const char *text, const Font font, const Color &color )
	{
		// the src argument is the part of the image generated from the font that must be displayed.
		// when the text is fully displayed, src.x = 0 and src.w = stringWidth.
		// otherwise, it depends of the aligment of the font (eg: if centered, src.x = stringWidth - src.w)

		// Alignment offsets are already calculated at this point
		vec4_t clr;
		struct mufont_s *ft;
		Rect pos;

		if ( src.w <= 0 || src.h <= 0 )
			return;

		color.get( clr[0], clr[1], clr[2], clr[3] );

		if( !font )
			ft = Local::getFontSmall();
		else
			ft = (struct mufont_s *)font;

		pos.x = (dest.x + src.x);
		pos.y = (dest.y + src.y);
		pos.w = (dest.w);
		pos.h = (dest.h);
		FixRect(pos);

		if( src.w < dest.w || src.h < dest.h ) {
			Trap::SCR_DrawClampString( (int)pos.x, (int)pos.y, text, (int)pos.x, (int)pos.y,
										(int)round( (dest.x + src.x + src.w) * Local::getScaleX() ),
										(int)round( (dest.y + src.y + src.h) * Local::getScaleY() ),
										ft, clr );
			/*
			Trap::SCR_DrawClampString( x, y, text, x, y,
										x + (int)round( pos.x ),
										y + (int)round( pos.y ),
										ft, clr );
			*/
		} else {
			Trap::SCR_DrawString( (int)pos.x, (int)pos.y, ALIGN_TOP_LEFT, text, ft, clr );
		}
	}

	const int Importer::clickButton = K_MOUSE1;
	const int Importer::mouseWheelUp = K_MWHEELUP;
	const int Importer::mouseWheelDown = K_MWHEELDOWN;

	const int Importer::keyLeft = K_LEFTARROW;
	const int Importer::keyRight = K_RIGHTARROW;
	const int Importer::keyUp = K_UPARROW;
	const int Importer::keyDown = K_DOWNARROW;
	const int Importer::keyBackspace = K_BACKSPACE;
	const int Importer::keyDelete = K_DEL;
	const int Importer::keyInsert = K_INS;
	const int Importer::keyEnter = K_ENTER;
	const int Importer::keySpace = K_SPACE;
	const int Importer::keyPgUp = K_PGUP;
	const int Importer::keyPgDn = K_PGDN;

	const int Importer::keyPadLeft = KP_LEFTARROW;
	const int Importer::keyPadRight = KP_RIGHTARROW;
	const int Importer::keyPadUp = KP_UPARROW;
	const int Importer::keyPadDown = KP_DOWNARROW;
	const int Importer::keyPadDelete = KP_DEL;
	const int Importer::keyPadInsert = KP_INS;
	const int Importer::keyPadEnter = KP_ENTER;
	const int Importer::keyPadPgUp = KP_PGUP;
	const int Importer::keyPadPgDn = KP_PGDN;

	const int Importer::keyTab = K_TAB;
	const int Importer::keyShift = K_SHIFT;
	const int Importer::keyCtrl = K_CTRL;
	const int Importer::keyAlt = K_ALT;
}
