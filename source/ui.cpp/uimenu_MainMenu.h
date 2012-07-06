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


#ifndef _UIMENU_MAINMENU_H_
#define _UIMENU_MAINMENU_H_

#include "uimenu_Global.h"

using namespace UICore;

#define MAINMENU_SUBMENU_PAD_X		24
#define MAINMENU_SUBMENU_PAD_Y		24

namespace UIMenu
{
	class MainMenu : public MenuBase
	{
	private:
		Panel *panel_;
		Panel *mainPanel;
		Panel *menuPanel;
		bool  isParentMain;

		Button *joinserverBtn;
		Button *startserverBtn;
		Button *setupBtn;
		Button *demosBtn;
		Button *modsBtn;
		Button *consoleBtn;
		Button *quitBtn;

		static void joinBtnHandler( BaseObject* );
		static void startBtnHandler( BaseObject* );
		static void setupBtnHandler( BaseObject* );
		static void demosBtnHandler( BaseObject* );
		static void modsBtnHandler( BaseObject* );
		static void consoleBtnHandler( BaseObject* );
		static void quitBtnHandler( BaseObject* );

	protected:
		TitleBar *titleBar;
		Panel *contentPanel;

	public:
		MainMenu(std::string caption = "", bool isParentMain = false);

		MEMORY_OPERATORS

		virtual void Show( void );
		virtual void Hide( void );
	};

}

#endif
