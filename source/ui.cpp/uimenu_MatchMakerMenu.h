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

#ifndef _UIMENU_MATCHMAKERMENU_H_
#define _UIMENU_MATCHMAKERMENU_H_

#include "uimenu_Global.h"
#include "uimenu_AddMatchMenu.h"
#include "../matchmaker/mm_ui.h"

using namespace UICore;

namespace UIMenu
{
	const int MatchMakerMenu_TabCount = 2;
	typedef mm_action_t MatchMakerAction;

	class MatchMakerMenu : public MenuBase
	{
	private:
		AddMatchMenu *addMatch;

		Panel *pMain;
		Panel *pCups;
		Panel *pAddMatch;
		SwitchButton *btnTabs[MatchMakerMenu_TabCount];

		Button *btnBack;

		// main panel
		Panel *grpMainMatch, *grpMainGametype;
		Button *btnMainChatMsg;
		Button *btnMainDrop;
		Button *btnMainJoinMatch;
		DropDownBox *lstMainGametype;
		Label *lblMainTitle;
		Label *lblMainGametype;
		Label *lblMainPlayers;
		Label *lblMainChannels;
		Label *lblMainChatMsgs;
		Label *lblMainMatchDetails;
		Label *lblMainTimelimit;
		Label *lblMainScorelimit;
		Label *lblMainSlots;
		MessageBox *messageBox;
		ListBox *lstMainPlayers;
		ListBox *lstMainChannels;
		ListBox *lstMainChatMsgs;
		TextBox *txtMainChatMsg;

		// cups panel
		Label *lblCupsTitle;
		Label *lblCupsTest;

		// event handlers
		static void tabsHandler( BaseObject *tab, bool );
		static void btnBackHandler( BaseObject *btn );
		static void btnMainDropHandler( BaseObject *btn );
		static void connectingHandler( MessageBoxButton result );
		static void matchesHandler( MessageBoxButton result );
		static void joiningHandler( MessageBoxButton result );
		static void btnMainJoinMatchHandler( BaseObject *btn );
		static void addMatchCancelHandler( void );
		static void lstMainPlayersHandler( ListItem *item, int, bool );

		ListItem *CreatePlayerItem( char *name = "", int slot = -1 );

	public:
		MatchMakerMenu();

		MEMORY_OPERATORS

		virtual void Show( void );
		virtual void Hide( void );

		static void MM_UIReply( MatchMakerAction action, const char *data );
	};
}

#endif
