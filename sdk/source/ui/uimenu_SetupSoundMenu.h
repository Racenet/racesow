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


#ifndef _UIMENU_SETUPSOUNDMENU_H_
#define _UIMENU_SETUPSOUNDMENU_H_

#include "uimenu_Global.h"

using namespace UICore;

namespace UIMenu
{
	class SetupSoundMenu : public MenuBase
	{
	private:		
		Label *lmodule;
		ListBox *module;

		Label *lsound;
		Slider *sound;
		Label *lmusic;
		Slider *music;
		Label *lplayers;
		Slider *players;
		Label *leffects;
		Slider *effects;
		Label *lannouncer;
		Slider *announcer;
		Label *lvoicechats;
		Slider *voicechats;
		Label *lhit;
		Slider *hit;
		
		void UpdateSoundFields( void );

	public:
		SetupSoundMenu();

		MEMORY_OPERATORS

		void UpdateSoundConfig( void );

		virtual void Show( void );
		virtual void Hide( void );
	};

}

#endif
