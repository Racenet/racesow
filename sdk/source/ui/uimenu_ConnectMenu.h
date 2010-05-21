
#ifndef _UIMENU_CONNECTMENU_H_
#define _UIMENU_CONNECTMENU_H_

#include "uimenu_Global.h"

using namespace UICore;

namespace UIMenu
{
	class ConnectMenu : public MenuBase
	{
	private:
		Label *msg;
		Button *yes;
		Button *no;


	public:
		ConnectMenu();

		MEMORY_OPERATORS

		virtual void Show( void );
		virtual void Hide( void );
	};

}

#endif
