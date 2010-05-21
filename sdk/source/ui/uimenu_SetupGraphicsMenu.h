/*
Copyright (C) 2007 Benjamin Litzelmann, Dexter Haslem et al.

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


#ifndef _UIMENU_SETUPGRAPHICSMENU_H_
#define _UIMENU_SETUPGRAPHICSMENU_H_

#include "uimenu_Global.h"

using namespace UICore;

namespace UIMenu
{
	class SetupGraphicsMenu : public MenuBase
	{
	private:		
		Panel *basic;
		Panel *advanced;
		SwitchButton *tabs[2];

		Label *titles[2];		
		
		// basic tab
		Label *resolutionListLabel;
		ListBox *resolutionList;		
		CheckBox *fullscreen;
		CheckBox *vsync;
		Label *colorDepthLabel;
		ListBox *colorDepth;
		Label *brightnessLabel;
		Slider *brightness;
		Label *texQualityLabel;
		Slider *texQuality;
		Label *textFilterLabel;
		Label *skyQualityLabel;
		Slider *skyQuality;
		Label *geometryLODLabel;
		Slider *geometryLOD;
		/*
		Label *pplLabel;
		ListBox *perPixelLighting;
		*/
		CheckBox *perPixelLighting;
		Label *texFilterLabel;
		ListBox *texFilter;
		Label *profileListLabel;
		ListBox *profileList;
		Button *loadProfile;

		// advanced tab
		Label *maxFPSLabel;
		TextBox *maxFPS;				
		CheckBox *simpleItems;
		CheckBox *wallMarks;
		CheckBox *bloom;
		CheckBox *selfShadow;
		CheckBox *reliefMapping;
		CheckBox *portalMaps;
		CheckBox *specularLighting;
		CheckBox *fastSky;

		void UpdateGraphicsFields( void );
		static void tabsHandler( BaseObject* target, bool newVal );
		static void picmipSliderHandler( BaseObject* target, int v1, int v2 );

	public:
		SetupGraphicsMenu();

		MEMORY_OPERATORS

		void UpdateGraphicsConfig( void );

		virtual void Show( void );
		virtual void Hide( void );
	};

}

#endif
