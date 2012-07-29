/*
Copyright (C) 1997-2001 Id Software, Inc.

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

#include "ui_local.h"

ui_import_t UI_IMPORT;

/*
* GetUIAPI
* 
* Returns a pointer to the structure with all entry points
*/
ui_export_t *GetUIAPI( ui_import_t *import )
{
	static ui_export_t globals;

	UI_IMPORT = *import;

	globals.API = UI_API;

	globals.Init = UI_Init;
	globals.Shutdown = UI_Shutdown;

	globals.Refresh = UI_Refresh;
	globals.DrawConnectScreen = UI_DrawConnectScreen;

	globals.Keydown = UI_Keydown;
	globals.Keyup = UI_Keyup;
	globals.CharEvent = UI_CharEvent;
	globals.MouseMove = UI_MouseMove;

	globals.ForceMenuOff = M_ForceMenuOff;
	globals.AddToServerList = M_AddToServerList;

	//globals.MM_UIReply = UIMenu::MatchMakerMenu::MM_UIReply;

	return &globals;
}

#if defined ( HAVE_DLLMAIN ) && !defined ( UI_HARD_LINKED )
int _stdcall DLLMain( void *hinstDll, unsigned long dwReason, void *reserved )
{
	return 1;
}
#endif
