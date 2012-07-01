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


#ifndef _UICORE_GLOBAL_H_
#define _UICORE_GLOBAL_H_

#include "uicore_BaseObject.h"
#include "uicore_Panel.h"
#include "uicore_Label.h"
#include "uicore_Button.h"
#include "uicore_SwitchButton.h"
#include "uicore_TextBox.h"
#include "uicore_CheckBox.h"
#include "uicore_Slider.h"
#include "uicore_ListBox.h"
#include "uicore_ListItem.h"
#include "uicore_DropDownBox.h"
#include "uicore_MessageBox.h"
#include "uicore_TitleBar.h"
#include "uicore_Import.h"
#include "uicore_Factory.h"

#include "math.h"
#include "string.h"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#ifdef bound
#undef bound
#endif

namespace UICore
{
	extern BaseObject *draggingObject;
	extern BaseObject *mouseOverObject;
	extern BaseObject *focusedObject;
	extern Panel *rootPanel;

	template <typename T> inline T min( T a, T b ) { return (a < b ? a : b); }
	template <typename T> inline T max( T a, T b ) { return (a > b ? a : b); }
	template <typename T> inline T bound( T v, T min, T max ) { return (v < min ? min : (v > max ? max : v) ); }
	inline float round( float v ) { return (v - floor(v) < .5) ? floor(v) : ceil(v); }

	/** Scale is needed to properly display borders */
	void setScale( float scaleX, float scaleY );

	void EventMouseDown( float x, float y, int button );
	void EventMouseUp( float x, float y, int button );
	void EventMouseMove( float x, float y, float xrel, float yrel );
	void EventKeyDown( int key, int charVal );
	void EventKeyUp( int key, int charVal );
}

#endif
