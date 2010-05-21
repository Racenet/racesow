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


#ifndef _UIMENU_STARTSERVERMENU_H_
#define _UIMENU_STARTSERVERMENU_H_

#include "uimenu_Global.h"

using namespace UICore;

namespace UIMenu
{
	class StartServerMenu : public MenuBase
	{
	private:
		Panel *panel;
		Label *lservname;
		TextBox *servname;
		Panel *gameplay;
		Label *lgametype;
		ListBox *gametype;
		CheckBox *suggestgametype;
		Label *lskill;
		SwitchButton *skill[4];
		CheckBox *instagib;
		Label *ltimelimit;
		TextBox *timelimit;
		Label *lscorelimit;
		TextBox *scorelimit;

		Panel *players;
		CheckBox *cheats;
		CheckBox *pub;
		Label *lbotnum;
		TextBox *botnum;
		Label *lplrnum;
		TextBox *plrnum;

		Label *lselmap;
		ListBox *selmap;

		Button *back;
		Button *start;

		static void startHandler( BaseObject* );
		static void backHandler( BaseObject* );
		static void skillHandler( BaseObject* target, bool newValue );
		static void mapSelectedHandler( ListItem *target, int position, bool isSelected );

		void SuggestGametype( const char *mapname );
		void UpdateFields( void );
		void BrowseMaps( void );

	public:
		StartServerMenu();

		MEMORY_OPERATORS

		virtual void Show( void );
		virtual void Hide( void );
	};

}

#endif
