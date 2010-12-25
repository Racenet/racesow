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

#include "../gameshared/q_arch.h"
#include "../gameshared/q_math.h"
#include "../gameshared/q_shared.h"
#include "../gameshared/q_cvar.h"
#include "../gameshared/q_dynvar.h"
#include "../gameshared/q_comref.h"
#include "../gameshared/q_collision.h"

#include "../gameshared/gs_public.h"
#include "ui_public.h"
#include "../cgame/ref.h"
#include "ui_syscalls.h"
#include "ui_atoms.h"
#include "../gameshared/q_keycodes.h"
#include "ui_boneposes.h"
#include "../matchmaker/auth_public.h"

//#define AUTH_CODE

extern char *noyes_names[];
extern char *offon_names[];

typedef struct
{
	int vidWidth;
	int vidHeight;
	int gameProtocol;
	unsigned int time;
	float frameTime;

	int initialSharedSeed;
	int sharedSeed;

	float scaleX;
	float scaleY;

	int cursorX;
	int cursorY;

	int clientState;
	int serverState;

	qboolean forceUI;

	int backgroundNum;

	struct shader_s *whiteShader;

	struct mufont_s *fontSystemSmall;
	struct mufont_s *fontSystemMedium;
	struct mufont_s *fontSystemBig;

	struct shader_s *gfxBoxUpLeft;
	struct shader_s *gfxBoxBorderUpLeft;
	struct shader_s *gfxBoxLeft;
	struct shader_s *gfxBoxBorderLeft;
	struct shader_s *gfxBoxBottomLeft;
	struct shader_s *gfxBoxBorderBottomLeft;
	struct shader_s *gfxBoxUp;
	struct shader_s *gfxBoxBorderUp;
	struct shader_s *gfxBoxBottom;
	struct shader_s *gfxBoxBorderBottom;
	struct shader_s *gfxBoxUpRight;
	struct shader_s *gfxBoxBorderUpRight;
	struct shader_s *gfxBoxRight;
	struct shader_s *gfxBoxBorderRight;
	struct shader_s *gfxBoxBottomRight;
	struct shader_s *gfxBoxBorderBottomRight;

	struct shader_s *gfxSlidebar_1, *gfxSlidebar_2, *gfxSlidebar_3, *gfxSlidebar_4;
	struct shader_s *gfxScrollbar_1, *gfxScrollbar_2, *gfxScrollbar_3, *gfxScrollbar_4;

	struct shader_s *gfxArrowUp, *gfxArrowDown, *gfxArrowRight;

	qboolean backGround; // has to draw the ui background
	qboolean backGroundTrackStarted;
	qboolean demoplaying;
	int bind_grab;
} ui_local_t;

extern ui_local_t uis;

extern cvar_t *developer;

#define MENU_DEFAULT_WIDTH	640
#define MENU_DEFAULT_HEIGHT	480
#define UI_WIDTHSCALE ( (float)uis.vidWidth / (float)MENU_DEFAULT_WIDTH )
#define UI_HEIGHTSCALE ( (float)uis.vidHeight / (float)MENU_DEFAULT_HEIGHT )
#define UI_SCALED_WIDTH( w ) ( (float)w * UI_WIDTHSCALE )
#define UI_SCALED_HEIGHT( h ) ( (float)h * UI_HEIGHTSCALE )
#define UI_BUTTONBOX_VERTICAL_SPACE 2

void UI_Error( const char *format, ... );
void UI_Printf( const char *format, ... );
void UI_FillRect( int x, int y, int w, int h, vec4_t color );

#define UI_Malloc( size ) trap_Mem_Alloc( size, __FILE__, __LINE__ )
#define UI_Free( data ) trap_Mem_Free( data, __FILE__, __LINE__ )

char *_UI_CopyString( const char *in, const char *filename, int fileline );
#define UI_CopyString( in ) _UI_CopyString( in, __FILE__, __LINE__ )

#define NUM_CURSOR_FRAMES 15

const char *Default_MenuKey( menuframework_s *m, int key );
const char *Default_MenuCharEvent( menuframework_s *m, qwchar key );

extern char *menu_in_sound;
extern char *menu_move_sound;
extern char *menu_out_sound;

extern qboolean	m_entersound;

// callback functions
extern void ( *M_Login_Callback )( qboolean status );
extern void ( *UI_AuthReply_Callback )( auth_reply_t reply );
void UI_AuthReply( auth_reply_t reply );

void M_MatchMaker_UpdateSlot( int slotno, const char *playername );
void M_MatchMaker_UpdateStatus( const char *status, qboolean showchat );
void M_MatchMaker_AddChatMsg( const char *msg );
void M_MatchMaker_Update( void );

float M_ClampCvar( float min, float max, float value );

void M_PopMenu( void );
void M_PushMenu( menuframework_s *m, void ( *draw )(void), const char *( *key )(int k), const char *( *charevent )(qwchar k) );
void M_ForceMenuOff( void );
void M_SetupPoppedCallback( void ( *closing )( void ) );
void M_genericBackFunc( menucommon_t *menuitem );

void M_Menu_Main_f( void );
void M_Menu_Main_Statusbar_f( void );
void M_AddToServerList( char *adr, char *info );
void M_AddToFavorites( menucommon_t *menuitem );
void M_RemoveFromFavorites( menucommon_t *menuitem );
void M_ForceMenuOff( void );

void M_Menu_Failed_f( void );
void M_Menu_Setup_f( void );
void M_Menu_JoinServer_f( void );
void M_Menu_MatchMaker_f( void );
void M_Menu_Login_f( void );
void M_Menu_Register_f( void );
void M_Menu_PlayerConfig_f( void );
void M_Menu_StartServer_f( void );
void M_Menu_Sound_f( void );
void M_Menu_Options_f( void );
void M_Menu_Performance_f( void );
void M_Menu_PerformanceAdv_f( void );
void M_Menu_Keys_f( void );
void M_Menu_Vsays_f( void );
void M_Menu_Quit_f( void );
void M_Menu_Reset_f( void );
void M_Menu_Demos_f( void );
void M_Menu_Mods_f( void );
void M_Menu_MsgBox_f( void );
void M_Menu_Custom_f( void );
void M_Menu_Chasecam_f( void );
void M_Menu_TeamConfig_f( void );
void M_Menu_Game_f( void );
void M_Menu_TV_f( void );
void M_Menu_TV_ChannelAdd_f( void );
void M_Menu_TV_ChannelRemove_f( void );
void M_Menu_Tutorials_f( void );
void M_Menu_Demoplay_f( void );

int UI_API( void );
void UI_Init( int vidWidth, int vidHeight, int protocol, int sharedSeed );
void UI_Shutdown( void );
void UI_Refresh( unsigned int time, int clientState, int serverState, qboolean demoplaying, qboolean backGround );
void UI_DrawConnectScreen( const char *serverName, const char *rejectmessage, int downloadType, const char *downloadfilename, 
						  float downloadPercent, int downloadSpeed, int connectCount, qboolean demoplaying, qboolean backGround );
void UI_Keydown( int key );
void UI_Keyup( int key );
void UI_CharEvent( qwchar key );
void UI_MouseMove( int dx, int dy );

// ui_playermodels.c
extern cvar_t *ui_playermodel_firstframe;
extern cvar_t *ui_playermodel_lastframe;
extern cvar_t *ui_playermodel_fps;

typedef struct
{
	int nskins;
	char **skinnames;
	char directory[MAX_QPATH];
} playermodelinfo_s;

extern m_itemslisthead_t playermodelsItemsList;
extern byte_vec4_t playerColor;

void UI_ColorRedCallback( menucommon_t *menuitem );
void UI_ColorGreenCallback( menucommon_t *menuitem );
void UI_ColorBlueCallback( menucommon_t *menuitem );
int UISCR_HorizontalAlignOffset( int align, int width );
int UISCR_VerticalAlignOffset( int align, int height );
void UI_Playermodel_Init( void );
void UI_FindIndexForModelAndSkin( const char *model, const char *skin, int *modelindex, int *skinindex );
void UI_DrawPlayerModel( char *model, char *skin, byte_vec4_t color, int xpos, int ypos, int width, int height, int frame, int oldframe );
qboolean UI_PlayerModelNextFrameTime( void );
void UI_DrawBox( int x, int y, int width, int height, vec4_t color, vec4_t lineColor, vec4_t shadowColor, vec4_t lineShadowColor );
void UI_DrawPicBar( int x, int y, int width, int height, int align, float percent, struct shader_s *shader, vec4_t backColor, vec4_t color );
void UI_DrawBar( int x, int y, int width, int height, int align, float percent, vec4_t backColor, vec4_t color );
void UI_DrawStringHigh( int x, int y, int align, const char *str, int maxwidth, struct mufont_s *font, vec4_t color );
void UI_DrawString( int x, int y, int align, const char *str, int maxwidth, struct mufont_s *font, vec4_t color );
