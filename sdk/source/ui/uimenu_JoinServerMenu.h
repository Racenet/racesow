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


#ifndef _UIMENU_JOINSERVERMENU_H_
#define _UIMENU_JOINSERVERMENU_H_

#include "uimenu_Global.h"
#include "uiwsw_ServerInfo.h"

using namespace UICore;
using namespace UIWsw;

namespace UIMenu
{
	class JoinServerMenu : public MenuBase
	{
	private:
		typedef enum {
			SORT_PING,
			SORT_PLAYER,
			SORT_MAP,
			SORT_GAMETYPE,
			SORT_NAME
		} sorttype_t;
		
		std::list<ServerInfo*> servers;
		std::multimap<unsigned int, ServerInfo*> pingSortMap;
		std::multimap<int, ServerInfo*> playerSortMap;
		std::multimap<std::string, ServerInfo*> mapSortMap;
		std::multimap<std::string, ServerInfo*> gametypeSortMap;
		std::multimap<std::string, ServerInfo*> nameSortMap;
		sorttype_t sorttype;
		bool reverse_sort;
		ServerInfo *pingingServer;
		char last_sel[80];
		int pingbatch;

		unsigned int nextServerTime;
		bool refreshingList;

		Label *lservers;
		SwitchButton *sortping;
		SwitchButton *sortplayer;
		SwitchButton *sortgametype;
		SwitchButton *sortmap;
		SwitchButton *sortname;
		Panel *arrow;
		ListBox *serverlist;

		Panel *filter;
		Label *lfilter;

		Button *showfull;
		Button *showempty;
		Button *showpassworded;
		Button *showinstagib;
		Button *skill;
		ListBox *gametype;
		Label *lmaxping;
		TextBox *maxping;
		Label *lmatchedname;
		TextBox *matchedname;

		Button *refresh;
		Button *connect;
		Button *back;

		static void StyleArrow( Panel *arr, SwitchButton *sorter, bool reverse ); // should be static, called from handler
		void StyleSorter( SwitchButton *sorter );
		void SearchGames( const char *s );
		void RefreshServerList( void );

		bool checkFilters( ServerInfo *s );
		template <class Iter> void FillListBoxWithMap( Iter begin, Iter end );

		static void sortHandler( BaseObject *target, bool newVal );
		static void refreshHandler( BaseObject * );
		static void connectHandler( BaseObject * );
		static void backHandler( BaseObject* );
		static void pingServers( BaseObject *, unsigned int deltatime, const Rect *, const Rect *, bool );
		static void filterHandler( BaseObject *target );
		static void ClickItemHandler(BaseObject *);
		static void DoubleClickItemHandler(BaseObject *);
		static void gametypeFilterHandler( ListItem *target, int position, bool isSelected );
		static void textFilterHandler( BaseObject *target, const char *text );
		static void serverItemSelected(ListItem *item, int itempos, bool sel);
		

		void setFilter( Button *button, int filter, const char *filtername );
		void LoadUserSettings( void );
		void SaveUserSettings( void );

	public:
		JoinServerMenu();
		virtual ~JoinServerMenu() { }

		MEMORY_OPERATORS

		static void AddToServerList( char *adr, char *info );

		virtual void Show( void );
		virtual void Hide( void );
	};

}

#endif
