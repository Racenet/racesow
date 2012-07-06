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


#ifndef _UIMENU_SETUPMENU_H_
#define _UIMENU_SETUPMENU_H_

#include "uimenu_Global.h"
#include "uimenu_MainMenu.h"

using namespace UICore;

namespace UIMenu
{
	class SetupPlayerMenu;
	class SetupSoundMenu;
	class SetupGraphicsMenu;

	class SetupMenu : public MainMenu
	{
		friend class SetupPlayerMenu;
		friend class SetupSoundMenu;
		friend class SetupGraphicsMenu;

	private:
		Button *player;
		Button *controller;
		Button *graphics;
		Button *sounds;
		Button *irc;
		Button *mainmenu;

		Panel *subpanel;
		Label *selectcat;

		Panel *currentSubPanel;

		static void playerHandler( BaseObject* );
		static void controllerHandler( BaseObject* );
		static void graphicsHandler( BaseObject* );
		static void soundsHandler( BaseObject* );
		static void ircHandler( BaseObject* );
		static void mainmenuHandler( BaseObject* );

	public:
		SetupMenu();

		MEMORY_OPERATORS

		virtual void Show( void );
		virtual void Hide( void );
	};

}

#endif
