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


#ifndef _UICORE_TYPES_H_
#define _UICORE_TYPES_H_

#include <list>
#include <map>
#include <string>

#define UICORE_TYPE_PANEL			"Panel"
#define UICORE_TYPE_LABEL			"Label"
#define UICORE_TYPE_BUTTON			"Button"
#define UICORE_TYPE_SWITCHBUTTON	"SwitchButton"
#define UICORE_TYPE_TEXTBOX			"TextBox"
#define UICORE_TYPE_CHECKBOX		"CheckBox"
#define UICORE_TYPE_SLIDER			"Slider"
#define UICORE_TYPE_LISTBOX			"ListBox"
#define UICORE_TYPE_LISTITEM		"ListItem"
#define UICORE_TYPE_DROPDOWNBOX  "DropDownBox"

struct mufont_s;
struct shader_s;

namespace UICore
{
	typedef struct shader_s* Image;
	typedef struct mufont_s* Font;

	typedef enum Alignment
	{
		ALIGN_TOP_LEFT,
		ALIGN_TOP_CENTER,
		ALIGN_TOP_RIGHT,
		ALIGN_MIDDLE_LEFT,
		ALIGN_MIDDLE_CENTER,
		ALIGN_MIDDLE_RIGHT,
		ALIGN_BOTTOM_LEFT,
		ALIGN_BOTTOM_CENTER,
		ALIGN_BOTTOM_RIGHT
	} Alignment;

	class Color
	{
	private:
		float fv[4];
	public:
		Color( void ) { fv[0] = fv[1] = fv[2] = fv[3] = 0.0f; }
		Color( float r, float g, float b, float a ) { set( r, g, b, a ); }
		Color( const Color &c ) { fv[0] = c.fv[0]; fv[1] = c.fv[1]; fv[2] = c.fv[2]; fv[3] = c.fv[3]; }

		inline Color & operator = ( const Color &c ) { fv[0] = c.fv[0]; fv[1] = c.fv[1]; fv[2] = c.fv[2]; fv[3] = c.fv[3]; return *this; }

		inline void set( float r, float g, float b, float a ) { fv[0] = r; fv[1] = g; fv[2] = b; fv[3] = a; }
		inline void get( float &r, float &g, float &b, float &a ) const { r = fv[0]; g = fv[1]; b = fv[2]; a = fv[3]; }

		inline void setAlpha( float a ) { fv[3] = a; }
		inline float getAlpha( void ) { return fv[3]; }
	};

	class Rect
	{
	public:
		float x, y;
		float w, h;
	public:
		Rect( float x = 0, float y = 0, float w = 0, float h = 0 ) : x( x ), y( y ), w( w ), h( h ) {}
		Rect( const Rect & r ) : x( r.x ), y( r.y ), w( r.w ), h( r.h ) {}
		inline Rect & operator = ( const Rect & r ) { x = r.x; y = r.y; w = r.w; h = r.h; return *this; }

		inline bool isPointInside( float px, float py ) { return ( px >= x && px <= x+w && py >= y && py <= y+h ); } 
	};
}

#define MEMORY_OPERATORS void *operator new( size_t size ); void operator delete( void *p );

#define ALLOCATOR_DEFINITION(classname) void *classname::operator new( size_t size ) { return Importer::newFunc( size ); }
#define DELETER_DEFINITION(classname) void classname::operator delete( void *p ) { Importer::deleteFunc( p ); }
	
#endif
