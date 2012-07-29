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

#include "uiwsw_Utils.h"
#include "uimenu_Global.h"
#include "uiwsw_Export.h"
#include "uiwsw_MapList.h"
#include "uiwsw_PlayerModels.h"
#include "../gameshared/gs_qrespath.h"

#include "../client/keys.h"

using namespace UIWsw;

int UIWsw::UI_API (void)
{
	return UI_API_VERSION;
}

void UIWsw::UI_Init( int vidWidth, int vidHeight, int protocol, int sharedSeed, qboolean demoPlaying, const char *demoName )
{
	Local::vidWidth = vidWidth;
	Local::vidHeight = vidHeight;
	Local::gameProtocol = protocol;

#if 0
	Local::scaleX = float(vidWidth) / REL_WIDTH;
	Local::scaleY = float(vidHeight) / REL_HEIGHT;
#else
	Local::scaleX = 1;
	Local::scaleY = 1;
#endif

	Local::cursorX = Local::vidWidth / 2;
	Local::cursorY = Local::vidHeight / 2;

	// wsw/Mokshu : test svn addin for VS 2005 and what about a "help/news" menu (latest news from website for example)
	Trap::Cmd_AddCommand( "menu_main", UIMenu::M_Menu_Main_f );
	Trap::Cmd_AddCommand( "menu_force", UIMenu::M_Menu_Force_f );
	Trap::Cmd_AddCommand( "menu_joinserver", UIMenu::M_Menu_JoinServer_f );
	Trap::Cmd_AddCommand( "menu_startserver", UIMenu::M_Menu_StartServer_f );
	Trap::Cmd_AddCommand( "menu_setup", UIMenu::M_Menu_Setup_f );
	Trap::Cmd_AddCommand( "menu_playersetup", UIMenu::M_Menu_SetupPlayer_f );
	Trap::Cmd_AddCommand( "menu_sound", UIMenu::M_Menu_SetupSound_f );
	Trap::Cmd_AddCommand( "menu_graphics", UIMenu::M_Menu_SetupGraphics_f );
	Trap::Cmd_AddCommand( "menu_quit", UIMenu::M_Menu_Quit_f );
	Trap::Cmd_AddCommand( "menu_login", UIMenu::M_Menu_Login_f );
	Trap::Cmd_AddCommand( "menu_custom", UIMenu::M_Menu_Custom_f );

	// precache sounds
	Trap::S_RegisterSound( S_UI_MENU_IN_SOUND );
	Trap::S_RegisterSound( S_UI_MENU_MOVE_SOUND );
	Trap::S_RegisterSound( S_UI_MENU_OUT_SOUND );

	// precache shaders
	Local::whiteShader = Trap::R_RegisterPic( "gfx/ui/white" );
	Trap::R_RegisterPic( UI_SHADER_FXBACK );
	Trap::R_RegisterPic( UI_SHADER_BIGLOGO );
	Trap::R_RegisterPic( UI_SHADER_CURSOR );

	// precache fonts
	cvar_t	*con_fontSystemSmall = Trap::Cvar_Get( "con_fontSystemSmall", DEFAULT_FONT_SMALL, CVAR_ARCHIVE );
	cvar_t	*con_fontSystemMedium = Trap::Cvar_Get( "con_fontSystemMedium", DEFAULT_FONT_MEDIUM, CVAR_ARCHIVE );
	cvar_t	*con_fontSystemBig = Trap::Cvar_Get( "con_fontSystemBig", DEFAULT_FONT_BIG, CVAR_ARCHIVE );

	Local::fontSystemSmall = Trap::SCR_RegisterFont( con_fontSystemSmall->string );
	if( !Local::fontSystemSmall ) {
		Local::fontSystemSmall = Trap::SCR_RegisterFont( DEFAULT_FONT_SMALL );
		if( !Local::fontSystemSmall )
			UI_Error( "Couldn't load default font \"%s\"", DEFAULT_FONT_SMALL );
	}
	Local::fontSystemMedium = Trap::SCR_RegisterFont( con_fontSystemMedium->string );
	if( !Local::fontSystemMedium )
		Local::fontSystemMedium = Trap::SCR_RegisterFont( DEFAULT_FONT_MEDIUM );

	Local::fontSystemBig = Trap::SCR_RegisterFont( con_fontSystemBig->string );
	if( !Local::fontSystemBig )
		Local::fontSystemBig = Trap::SCR_RegisterFont( DEFAULT_FONT_BIG );

	UICore::setScale( Local::scaleX, Local::scaleY );
	UIMenu::BuildTemplates();

#if 0
	UICore::rootPanel = new UICore::Panel( NULL, 0, 0, REL_WIDTH, REL_HEIGHT );
#else
	UICore::rootPanel = new UICore::Panel( NULL, 0, 0, Local::vidWidth, Local::vidHeight );
#endif

	UICore::rootPanel->setClickable( true );

	PlayerModels::Init(); // create a list with the available player models

	MapList::Precache();

	Local::backgroundTrackStarted = false;
}

void UIWsw::UI_Shutdown( void )
{
	Trap::S_StopBackgroundTrack();

	Trap::Cmd_RemoveCommand( "menu_main" );
	Trap::Cmd_RemoveCommand( "menu_setup" );
	Trap::Cmd_RemoveCommand( "menu_joinserver" );
	Trap::Cmd_RemoveCommand( "menu_login" );
	Trap::Cmd_RemoveCommand( "menu_register" );
	Trap::Cmd_RemoveCommand( "menu_playersetup" );
	Trap::Cmd_RemoveCommand( "menu_startserver" );
	Trap::Cmd_RemoveCommand( "menu_graphics" );
	Trap::Cmd_RemoveCommand( "menu_sound" );
	Trap::Cmd_RemoveCommand( "menu_video" );
	Trap::Cmd_RemoveCommand( "menu_options" );
	Trap::Cmd_RemoveCommand( "menu_keys" );
	Trap::Cmd_RemoveCommand( "menu_vsays" );
	Trap::Cmd_RemoveCommand( "menu_quit" );
	Trap::Cmd_RemoveCommand( "menu_demos" );
	Trap::Cmd_RemoveCommand( "menu_mods" );
	Trap::Cmd_RemoveCommand( "menu_game" );
	Trap::Cmd_RemoveCommand( "menu_tv" );
	Trap::Cmd_RemoveCommand( "menu_tv_channel_add" );
	Trap::Cmd_RemoveCommand( "menu_tv_channel_remove" );
	Trap::Cmd_RemoveCommand( "menu_failed" );
	Trap::Cmd_RemoveCommand( "menu_teamconfig" );
	Trap::Cmd_RemoveCommand( "menu_force" );
	Trap::Cmd_RemoveCommand( "menu_custom" );
}

void UIWsw::UI_Refresh( unsigned int time, int clientState, int serverState, qboolean demoPaused, unsigned int demoTime, qboolean backGround )
{
	Local::frameTime = (time - Local::time) * 0.001f;
	Local::time = time;
	Local::clientState = clientState;
	Local::serverState = serverState;
	Local::background = (backGround == qtrue);

	if ( !UIMenu::activeMenu ) // ui is inactive
		return;

	// draw background
	if( Local::background ) {
		Trap::R_DrawStretchPic( 0, 0, Local::vidWidth, Local::vidHeight, 
			0, 0, 1, 1, colorWhite, Trap::R_RegisterPic( UI_SHADER_VIDEOBACK ) );

		Trap::R_DrawStretchPic ( 0, 0, Local::vidWidth, Local::vidHeight, 
			0, 0, 1, 1, colorWhite, Trap::R_RegisterPic( UI_SHADER_FXBACK ) );
/*
		Trap::R_DrawStretchPic ( 0, 0, Local::vidWidth, Local::vidHeight, 
			0, 0, 1, 1, colorWhite, Trap::R_RegisterPic( UI_SHADER_BIGLOGO ) );
*/
		if( !Local::backgroundTrackStarted ) {
			Trap::S_StartBackgroundTrack( S_PLAYLIST_MENU, "3" ); // shuffle and loop
			Local::backgroundTrackStarted = true;
		}
	} else {
		Local::backgroundTrackStarted = false;
	}

	UICore::rootPanel->Draw( time );

	// draw cursor
	if ( !Local::bind_grab )
		Trap::R_DrawStretchPic( Local::cursorX - 16, Local::cursorY - 16, 32, 32, 
			0, 0, 1, 1, colorWhite, Trap::R_RegisterPic( UI_SHADER_CURSOR ) );

	// TODO : fix enter sound
	// delay playing the enter sound until after the
	// menu has been drawn, to avoid delay while
	// caching images
	//if ( m_entersound )
	//{
	//	Trap::S_StartLocalSound( menu_in_sound );
	//	m_entersound = qfalse;
	//}
}

void UIWsw::UI_DrawConnectScreen( const char *serverName, const char *rejectmessage, int downloadType, const char *downloadFilename,
						  float downloadPercent, int downloadSpeed, int connectCount, qboolean backGround )
{
	// TODO : replace current connect screen by an interactive one with cancel button, etc...
	qboolean localhost;
	char str[MAX_QPATH];
	//char levelshot[MAX_QPATH];
	char mapname[MAX_QPATH], message[MAX_QPATH];

	Trap::S_StopBackgroundTrack();

	localhost = (qboolean)(!serverName || !serverName[0] || !Q_stricmp ( serverName, "localhost" ));

	Trap::GetConfigString ( CS_MAPNAME, mapname, sizeof(mapname) );
	if ( backGround ) {
/*		if ( mapname[0] ) {
			Q_snprintfz ( levelshot, sizeof(levelshot), "levelshots/%s.jpg", mapname );

			if ( Trap::FS_FOpenFile( levelshot, NULL, FS_READ ) == -1 ) 
				Q_snprintfz ( levelshot, sizeof(levelshot), "levelshots/%s.tga", mapname );

			if ( Trap::FS_FOpenFile( levelshot, NULL, FS_READ ) == -1 ) 
				Q_snprintfz ( levelshot, sizeof(levelshot), "gfx/ui/unknownmap" );

			Trap::R_DrawStretchPic ( 0, 0, Local::vidWidth, Local::vidHeight, 
				0, 0, 1, 1, colorWhite, Trap::R_RegisterPic ( levelshot ) );
		} else*/ {
			Trap::R_DrawStretchPic( 0, 0, Local::vidWidth, Local::vidHeight,
				0, 0, 1, 1, colorBlack, Local::getWhiteShader() );
		}
	}

	// draw server name if not local host
	if ( !localhost ) {
		Q_snprintfz ( str, sizeof(str), "Connecting to %s", serverName );
		Trap::SCR_DrawString( Local::vidWidth/2, 64, ALIGN_CENTER_TOP, str, Local::fontSystemBig, colorWhite );
	}

	if( rejectmessage ) {
		Q_snprintfz ( str, sizeof(str), "Refused: %s", rejectmessage );
		Trap::SCR_DrawString( Local::vidWidth/2, 86, ALIGN_CENTER_TOP, str, Local::fontSystemMedium, colorWhite );
	}

	if( downloadFilename ) {
		Q_snprintfz ( str, sizeof(str), "Downloading %s", downloadFilename );
		Trap::SCR_DrawString( Local::vidWidth/2, 86, ALIGN_CENTER_TOP, str, Local::fontSystemMedium, colorWhite );
	}

	if( mapname[0] ) {
		// draw Warsow logo at bottom
		Trap::R_DrawStretchPic( 0, (int)(Local::vidHeight - 90*Local::scaleY), Local::vidWidth, (int)(80*Local::scaleY), 
			0, 0, 1, 1, colorWhite, Trap::R_RegisterPic( "gfx/ui/loadscreen_logo" ) );

		Trap::GetConfigString( CS_MESSAGE, message, sizeof(message) );

		if( message[0] )	// level name ("message")
			Trap::SCR_DrawString( Local::vidWidth/2, 180, ALIGN_CENTER_TOP, message, Local::fontSystemBig, colorWhite );
	} else {
		if( !localhost ) {
			Q_snprintfz ( message, sizeof(message), "Awaiting connection ... %i", connectCount );
			Trap::SCR_DrawString( Local::vidWidth/2, 180, ALIGN_CENTER_TOP, message, Local::fontSystemBig, colorWhite );
		} else {
			Q_strncpyz ( message, "Loading ...", sizeof(message) );
			Trap::SCR_DrawString( Local::vidWidth/2, 180, ALIGN_CENTER_TOP, message, Local::fontSystemBig, colorWhite );
		}
	}
}

void UIWsw::UI_Keydown( int key )
{
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_MWHEELUP || key == K_MWHEELDOWN )
		UICore::EventMouseDown( float(Local::cursorX)/Local::scaleX, float(Local::cursorY)/Local::scaleY, key );
	else
		UICore::EventKeyDown( key, 0 );
}

void UIWsw::UI_Keyup( int key )
{
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_MWHEELUP || key == K_MWHEELDOWN )
		UICore::EventMouseUp( float(Local::cursorX)/Local::scaleX, float(Local::cursorY)/Local::scaleY, key );
	else
		UICore::EventKeyUp( key, 0 );
}

void UIWsw::UI_CharEvent( qwchar key )
{
	UICore::EventKeyDown( 0, key );
}

void UIWsw::UI_MouseMove( int dx, int dy )
{

	if( Local::bind_grab )
		return; //don't move the mouse if we're grabbing binds

	Local::cursorX += dx;
	Local::cursorY += dy;

	clamp( Local::cursorX, 0, Local::vidWidth );
	clamp( Local::cursorY, 0, Local::vidHeight );

	if( dx || dy )
		UICore::EventMouseMove( float(Local::cursorX) / Local::scaleX, float(Local::cursorY) / Local::scaleY,
				float(dx) / Local::scaleX, float(dy) / Local::scaleY );
}


