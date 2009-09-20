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

#ifndef _UIWSW_UTILS_H_
#define _UIWSW_UTILS_H_

#include "../game/q_shared.h"
#include "../gameshared/gs_ref.h"
#include "uiwsw_SysCalls.h"

struct mempool_s;

namespace UIWsw
{
	class Local
	{
		// Only modifiable by export functions
		friend int UI_API( void );
		friend void UI_Init( int vidWidth, int vidHeight, int protocol );
		friend void UI_Shutdown( void );
		friend void UI_Refresh( unsigned int time, int clientState, int serverState, qboolean backGround );
		friend void UI_DrawConnectScreen( char *serverName, char *rejectmessage, char *downloadfilename, int connectCount, qboolean backGround );
		friend void UI_Keydown( int key );
		friend void UI_Keyup( int key );
		friend void UI_CharEvent( qwchar key );
		friend void UI_MouseMove( int dx, int dy );

	private:
		static int		vidWidth;
		static int		vidHeight;
		static int		gameProtocol;
		static unsigned int	time;
		static float	frameTime;

		static float	scaleX;
		static float	scaleY;

		static int		cursorX;
		static int		cursorY;

		static int		clientState;
		static int		serverState;

		static bool		forceUI;

		static bool		bind_grab;

		static struct shader_s *whiteShader;

		static struct mufont_s *fontSystemSmall;
		static struct mufont_s *fontSystemMedium;
		static struct mufont_s *fontSystemBig;

		static bool background; // has to draw the ui background
		static bool backgroundTrackStarted;

	public:
		inline static int getVidWidth( void ) { return vidWidth; }
		inline static int getVidHeight( void ) { return vidHeight; }
		inline static int getGameProtocol( void ) { return gameProtocol; }
		inline static unsigned int getTime( void ) { return time; }
		inline static float	getFrameTime( void ) { return frameTime; }

		inline static float	getScaleX( void ) { return scaleX; }
		inline static float	getScaleY( void ) { return scaleY; }

		inline static int getCursorX( void ) { return cursorX; }
		inline static int getCursorY( void ) { return cursorY; }

		inline static int getClientState( void ) { return clientState; }
		inline static int getServerState( void ) { return serverState; }

		inline static bool getForceUI( void ) { return forceUI; }

		inline static bool getBindGrab( void ) { return bind_grab; }

		inline static struct shader_s *getWhiteShader( void ) { return whiteShader; }

		inline static struct mufont_s *getFontSmall( void ) { return fontSystemSmall; }
		inline static struct mufont_s *getFontMedium( void ) { return fontSystemMedium; }
		inline static struct mufont_s *getFontBig( void ) { return fontSystemBig; }
	};

	void UI_ForceMenuOff( void );

	char *UI_CopyString( const char *in );
	void UI_Error( const char *format, ... );
	void UI_Printf( const char *format, ... );

#define GAMETYPE_NB		6

	// TODO : this should be completely global (shared with game)
	extern const char *gametype_names[GAMETYPE_NB][2];

	/** A common class to manage memory pools
		@Remark : this class can be moved to a common file and used
		for any pool of the game if sometime we port the whole of it to c++ */
	class Mem
	{
	protected:
		inline static void *MemAlloc( size_t size) {
			return Trap::Mem_Alloc( size, __FILE__, __LINE__);
		}

		inline static void MemFree( void *mem )	{
			Trap::Mem_Free(mem, __FILE__, __LINE__);
		}



	};

	/** A class for UI memory pool management */
	class UIMem : private Mem
	{

	public:
		inline static void *Malloc( size_t size ) {
			return MemAlloc( size );
		}

		inline static void Free( void *mem ) {
			return MemFree( mem );
		}



	};
}

#endif
