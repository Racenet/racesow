/*
Copyright (C) 2007 Will Franklin

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

#ifndef _UIMENU_ADDMATCHMENU_H_
#define _UIMENU_ADDMATCHMENU_H_

#include "uimenu_Global.h"
#include "../matchmaker/mm_ui.h"

using namespace UICore;

namespace UIMenu
{
	class AddMatchMenu : public MenuBase
	{
	private:
		Button *btnAdd, *btnBack;
		DropDownBox *lstGametype;
		Label *lblTitle;
		Label *lblGametype;
		Label *lblNoOfPlayers;
		Label *lblScorelimit;
		Label *lblTimelimit;
		Label *lblSkill;
		MessageBox *messageBox;
		Panel *pBackground;
		SwitchButton *btnAllSkills;
		SwitchButton *btnDependent;
		TextBox *txtNoOfPlayers;
		TextBox *txtScorelimit;
		TextBox *txtTimelimit;

		static void btnAddHandler( BaseObject *btn );
		static void btnBackHandler( BaseObject *btn );
		static void skillsHandler( BaseObject *btn, bool  );

		void ( *cancelCallback )( void );

	public:
		AddMatchMenu();

		MEMORY_OPERATORS

		virtual void Hide();
		virtual void Show();

		inline void setCancelCallback( void ( *cb )( void ) );

		void setGametype( int index );
	};

	void AddMatchMenu::setCancelCallback( void ( *cb )( void ) )
	{
		cancelCallback = cb;
	}
}

#endif

