/*
Copyright (C) 2002-2008 The Warsow devteam

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

#include "../gameshared/q_shared.h"
#include "../gameshared/q_math.h"
#include "../gameshared/q_cvar.h"
#include "../gameshared/q_dynvar.h"
#include "../gameshared/gs_ref.h"

namespace UIWsw
{
	int UI_API( void );
	void UI_Init( int vidWidth, int vidHeight, int protocol, int sharedSeed, qboolean demoPlaying, const char *demoName );
	void UI_Shutdown( void );
	void UI_Refresh( unsigned int time, int clientState, int serverState, qboolean demoPaused, unsigned int demoTime, qboolean backGround );
	void UI_DrawConnectScreen( const char *serverName, const char *rejectmessage, int downloadType, const char *downloadFilename,
						  float downloadPercent, int downloadSpeed, int connectCount, qboolean backGround );
	void UI_Keydown( int key );
	void UI_Keyup( int key );
	void UI_CharEvent( qwchar key );
	void UI_MouseMove( int dx, int dy );
}
