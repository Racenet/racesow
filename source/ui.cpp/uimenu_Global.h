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

#ifndef _UIMENU_GLOBAL_H_
#define _UIMENU_GLOBAL_H_

#include "uicore_Global.h"
//#include "uiwsw_utils.h"

#define REL_WIDTH	1024
#define REL_HEIGHT	768

namespace UIMenu
{
	class MenuBase
	{
	protected:
		UICore::Panel *panel;
	public:
		MenuBase() {}
		virtual ~MenuBase() {}

		virtual void Show( void ) = 0;
		virtual void Hide( void ) = 0;
		inline bool isActive( void );
	};

	extern MenuBase *activeMenu;

	void BuildTemplates( void );
	void FreeTemplates( void );
	void setActiveMenu( MenuBase *menu );

	void M_Menu_Main_f( void );
	void M_Menu_Force_f( void );
	void M_Menu_JoinServer_f( void );
	void M_Menu_StartServer_f( void );
	void M_Menu_Setup_f( void );
	void M_Menu_SetupPlayer_f( void );
	void M_Menu_SetupSound_f( void );
	void M_Menu_SetupGraphics_f( void );
	void M_Menu_Quit_f( void );
	void M_Menu_Login_f( void );
	void M_Menu_Custom_f( void );

	inline bool MenuBase::isActive( void )
	{
		return activeMenu == this;
	}

	extern UICore::Color black;
	extern UICore::Color darkgray;
	extern UICore::Color midgray;

	extern UICore::Color blue1;
	extern UICore::Color blue2;
	extern UICore::Color blue3;
	extern UICore::Color blue4;
	extern UICore::Color blue5;
	extern UICore::Color blue6;
	extern UICore::Color blue7;
	extern UICore::Color blue9;

	extern UICore::Color orange;
	extern UICore::Color white;
	extern UICore::Color transparent;

}

#endif
