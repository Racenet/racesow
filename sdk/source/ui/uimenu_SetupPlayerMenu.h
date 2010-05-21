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


#ifndef _UIMENU_SETUPPLAYERMENU_H_
#define _UIMENU_SETUPPLAYERMENU_H_

#include "uimenu_Global.h"

using namespace UICore;

namespace UIMenu
{
	class SetupPlayerMenu : public MenuBase
	{
	private:		
		Panel *player;
		Panel *hud;
		Panel *team;
		SwitchButton *tabs[3];

		Label *lplrname;
		TextBox *plrname;
		Label *lmodel;
		ListBox *model;
		Label *lskin;
		ListBox *skin;
		Label *lrgb[3];
		Slider *rgb[3];
		Label *lhandedness;
		SwitchButton *handedness[3];
		Label *lfov;
		TextBox *fov;
		Panel *modeldisplay;

		Label *lclienthud;
		ListBox *clienthud;
		Label *lwcrosshair;
		ListBox *wcrosshair;
		Label *lscrosshair;
		ListBox *scrosshair;
		CheckBox *weaponlist;
		CheckBox *showfps;
		CheckBox *showspeed;
		CheckBox *showhelp;
		Label *lautoaction;
		ListBox *autoaction;

		SwitchButton *teamselection[5];
		Panel *teamsetup[5];
		CheckBox *forcemodel[5];
		Label *lteammodel[5];
		ListBox *teammodel[5];
		Label *lteamskin[5];
		ListBox *teamskin[5];
		CheckBox *forcecolor[5];
		Label *lteamcolor[3][5];
		Slider *teamcolor[3][5];
		Panel *teampreview[5];
		CheckBox *playerasalpha;
		CheckBox *teamlessasbeta;
		
		static void tabsHandler( BaseObject* target, bool newVal );

		static void modelSelectedHandler( ListItem *target, int position, bool isSelected );
		static void handednessHandler( BaseObject* target, bool newVal );
		static void playerDisplayHandler( BaseObject *target, unsigned int deltatime, const Rect *parentPos, const Rect *parentPosSrc, bool enabled );
		static void teamTabsHandler( BaseObject* target, bool newVal );

		static void forceModelHandler( BaseObject *target, bool newVal );
		static void teamModelSelectedHandler( ListItem *target, int position, bool isSelected );
		static void forceColorHandler( BaseObject *target, bool newVal );

		void UpdateHUDList( void );
		void UpdateCrosshairLists( void );

		void UpdatePlayerFields( void );

	public:
		SetupPlayerMenu();

		MEMORY_OPERATORS

		void UpdatePlayerConfig( void );

		virtual void Show( void );
		virtual void Hide( void );
	};

}

#endif
