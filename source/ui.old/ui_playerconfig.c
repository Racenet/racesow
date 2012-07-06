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

//=============================================================================
//
//PLAYER CONFIG MENU
//
//=============================================================================

static menuframework_s s_player_config_menu;

//==============================
//	CrossHair
//==============================

static void *s_crosshair_pic;
static int menu_crosshair_id;
static void CrosshairFunc( struct menucommon_s *unused )
{
	menu_crosshair_id++;
	if( menu_crosshair_id < 0 || menu_crosshair_id >= NUM_CROSSHAIRS )
		menu_crosshair_id = 0;

	s_crosshair_pic = trap_R_RegisterPic( va( "gfx/hud/crosshair%i", menu_crosshair_id ) );
	trap_Cvar_SetValue( "cg_crosshair", menu_crosshair_id );
}

static void *s_crosshair_strong_pic;
static int menu_crosshair_strong_id;
static void CrosshairStrongFunc( struct menucommon_s *unused )
{
	menu_crosshair_strong_id++;
	if( menu_crosshair_strong_id < 0 || menu_crosshair_strong_id >= NUM_CROSSHAIRS )
		menu_crosshair_strong_id = 0;

	s_crosshair_strong_pic = trap_R_RegisterPic( va( "gfx/hud/crosshair%i", menu_crosshair_strong_id ) );
	trap_Cvar_SetValue( "cg_crosshair_strong", menu_crosshair_strong_id );
}

cvar_t *pcolor;


//==============================
//	autoaction
//==============================
#define NUM_AUTOACTION 4

static char *autoaction[] =
{
	"none",
	"demo",
	"screenshot",
	"demo screenshot",
	"stats",
	"demo stats",
	"screenshot stats",
	"demo screenshot stats",
	0
};

static void ClientHUDCallBack( menucommon_t *menuitem )
{
	trap_Cvar_Set( "cg_clientHUD", menuitem->itemnames[menuitem->curvalue] );
}

static void UpdateFOVFunc( menucommon_t *menuitem )
{
	char *buf;

	buf = UI_GetMenuitemFieldBuffer( menuitem );
	if( buf )
	{
		int newfov = atoi( buf );
		clamp( newfov, 1, 160 );
		trap_Cvar_SetValue( "fov", newfov );
	}
}

static void UpdateZoomFOVFunc( menucommon_t *menuitem )
{
	char *buf;

	buf = UI_GetMenuitemFieldBuffer( menuitem );
	if( buf )
	{
		int newfov = atoi( buf );
		clamp( newfov, 1, 90 );
		trap_Cvar_SetValue( "zoomfov", newfov );
	}
}

static void UpdateShowHelpFunc( menucommon_t *menuitem )
{
	trap_Cvar_SetValue( "cg_showHelp", menuitem->curvalue );
}

static void UpdateShowMinimap( menucommon_t *menuitem )
{
	trap_Cvar_SetValue( "cg_showMinimap", menuitem->curvalue );
}

static void UpdateShowItemTimers( menucommon_t *menuitem )
{
	trap_Cvar_SetValue( "cg_showItemTimers", menuitem->curvalue );
}

static void UpdateShowFPSFunc( menucommon_t *menuitem )
{
	trap_Cvar_SetValue( "cg_showFPS", menuitem->curvalue );
}

static void UpdateSpeedMeterFunc( menucommon_t *menuitem )
{
	trap_Cvar_SetValue( "cg_showSpeed", menuitem->curvalue );
}

static void UpdateWeaponlistFunc( menucommon_t *menuitem )
{
	trap_Cvar_SetValue( "cg_weaponlist", menuitem->curvalue );
}

/*
* M_PlayerConfig_ApplyChanges
*/
static void M_PlayerConfig_ApplyChanges( void )
{
	char cleanClanName[MAX_CLANNAME_BYTES];

	menucommon_t *playerbox = UI_MenuItemByName( "m_playerconfig_model" );
	menucommon_t *skinbox = UI_MenuItemByName( "m_playerconfig_skin" );
	menucommon_t *namebox = UI_MenuItemByName( "m_playerconfig_name" );
	menucommon_t *clanbox = UI_MenuItemByName( "m_playerconfig_clan" );
	menucommon_t *hudbox = UI_MenuItemByName( "m_playerconfig_clienthud" );
	menucommon_t *handbox = UI_MenuItemByName( "m_playerconfig_handedness" );
	menucommon_t *autoactionbox = UI_MenuItemByName( "m_playerconfig_autoaction" );
	m_listitem_t *item = UI_FindItemInScrollListWithId( &playermodelsItemsList, playerbox->curvalue );
	playermodelinfo_s *playermodel = (playermodelinfo_s *)item->data;

	trap_Cvar_Set( "name", ( (menufield_t *)namebox->itemlocal )->buffer );
	COM_SanitizeColorString( va( "%s", ((menufield_t *)clanbox->itemlocal )->buffer ), cleanClanName, sizeof( cleanClanName ), MAX_CLANNAME_CHARS, COLOR_WHITE );
	trap_Cvar_Set( "clan", cleanClanName );
	trap_Cvar_Set( "skin", playermodel->skinnames[skinbox->curvalue] );
	trap_Cvar_Set( "model", playermodel->directory );
	trap_Cvar_Set( "color", va( "%i %i %i", (int)playerColor[0], (int)playerColor[1], (int)playerColor[2] ) );
	trap_Cvar_Set( "cg_clientHUD", hudbox->itemnames[hudbox->curvalue] );
	trap_Cvar_SetValue( "hand", handbox->curvalue );

	if( strstr( autoaction[autoactionbox->curvalue], "demo" ) )
		trap_Cvar_Set( "cg_autoaction_demo", "1" );
	else
		trap_Cvar_Set( "cg_autoaction_demo", "0" );

	if( strstr( autoaction[autoactionbox->curvalue], "screenshot" ) )
		trap_Cvar_Set( "cg_autoaction_screenshot", "1" );
	else
		trap_Cvar_Set( "cg_autoaction_screenshot", "0" );

	if( strstr( autoaction[autoactionbox->curvalue], "stats" ) )
		trap_Cvar_Set( "cg_autoaction_stats", "1" );
	else
		trap_Cvar_Set( "cg_autoaction_stats", "0" );

	// fov
	UpdateFOVFunc( UI_MenuItemByName( "m_playerconfig_fov" ) );
	UpdateZoomFOVFunc( UI_MenuItemByName( "m_playerconfig_zoomfov" ) );
	UpdateShowHelpFunc( UI_MenuItemByName( "m_playerconfig_showhelp" ) );
	UpdateShowMinimap( UI_MenuItemByName( "m_playerconfig_showminimap" ) );
	UpdateShowItemTimers( UI_MenuItemByName( "m_playerconfig_showitemtimers" ) );
	UpdateShowFPSFunc( UI_MenuItemByName( "m_playerconfig_showfps" ) );
	UpdateSpeedMeterFunc( UI_MenuItemByName( "m_playerconfig_showspeed" ) );
	UpdateWeaponlistFunc( UI_MenuItemByName( "m_playerconfig_weaponlist" ) );
}

static void M_PlayerConfig_Close( menucommon_t *menuitem )
{
	M_genericBackFunc( menuitem );
}

static void M_PlayerConfig_SaveAndClose( menucommon_t *menuitem )
{
	M_PlayerConfig_ApplyChanges();
	M_PlayerConfig_Close( menuitem );
}

static void ModelCallback( menucommon_t *menuitem )
{
	menucommon_t *playerbox, *skinbox;
	m_listitem_t *item;
	playermodelinfo_s *playermodel;

	playerbox = UI_MenuItemByName( "m_playerconfig_model" );
	skinbox = UI_MenuItemByName( "m_playerconfig_skin" );
	if( !playerbox || !skinbox )
		return;

	item = UI_FindItemInScrollListWithId( &playermodelsItemsList, playerbox->curvalue );
	playermodel = (playermodelinfo_s *)item->data;
	UI_SetupSpinControl( skinbox, playermodel->skinnames, 0 );
	//skinbox->itemnames = playermodel->skinnames;

	//skinbox->curvalue = 0;
}

static void M_GetPlayerColor( void )
{
	int rgbcolor;
	pcolor = trap_Cvar_Get( "color", "", CVAR_ARCHIVE|CVAR_USERINFO );
	rgbcolor = COM_ReadColorRGBString( pcolor->string );
	if( rgbcolor == -1 )
		rgbcolor = COM_ReadColorRGBString( pcolor->dvalue );
	if( rgbcolor != -1 )
		Vector4Set( playerColor, COLOR_R( rgbcolor ), COLOR_G( rgbcolor ), COLOR_B( rgbcolor ), 255 );
	else
		Vector4Set( playerColor, 255, 255, 255, 255 ); // start white
	pcolor->modified = qfalse;
}


/*
* M_Demos_CreateDemosList
*/
char **HUDnames = NULL;
static void M_PlayerConfig_CreateHUDsList( void )
{
	int i, j, k;
	char listbuf[1024], scratch[MAX_CONFIGSTRING_CHARS];
	char *ptr;
	int numHUDs, count;

	// get the list of skins
	numHUDs = trap_FS_GetFileList( "huds", ".hud", NULL, 0, 0, 0 );
	if( !numHUDs )
	{
		HUDnames = (char **)UI_Malloc( sizeof( char * ) * 2 );
		HUDnames[0] = UI_CopyString( "default" );
		HUDnames[1] = NULL;
		return;
	}

	// there are valid huds. So:
	HUDnames = (char **)UI_Malloc( sizeof( char * ) * ( numHUDs + 1 ) );
	count = 0;

	i = 0;
	do
	{
		if( ( k = trap_FS_GetFileList( "huds", ".hud", listbuf, sizeof( listbuf ), i, numHUDs ) ) == 0 )
		{
			i++; // can happen if the filename is too long to fit into the buffer or we're done
			continue;
		}
		i += k;

		// copy unique huds
		for( ptr = listbuf; k > 0; k--, ptr += strlen( ptr )+1 )
		{
			Q_strncpyz( scratch, ptr, sizeof( scratch ) );
			COM_StripExtension( scratch );

			// see if this hud is already in the list
			for( j = 0; j < count && Q_stricmp( scratch, HUDnames[j] ); j++ ) ;
			if( j == count )
				HUDnames[count++] = UI_CopyString( scratch );
		}
	}
	while( i < numHUDs );

	HUDnames[count] = NULL;
}

/*
* PlayerConfig_MenuInit
*/
static qboolean PlayerConfig_MenuInit( void )
{
	menucommon_t *menuitem;
	int currentdirectoryindex = 0;
	int currentskinindex = 0;
	int hand = trap_Cvar_Value( "hand" );
	const char *name = trap_Cvar_String( "name" );
	const char *clan = trap_Cvar_String( "clan" );
	const char *model = trap_Cvar_String( "model" );
	const char *skin = trap_Cvar_String( "skin" );
	cvar_t *cg_clientHUD = trap_Cvar_Get( "cg_clientHUD", "default", CVAR_ARCHIVE );
	int x = -150;
	int yoffset = 0;
	static char *handedness[] = { "right", "left", "center", 0 };
	static char *minimapmodes[] = { "off", "normal", "spec", "normal+spec", 0 };
	static char *itemtimersmodes[] = { "off", "on normal servers", "on tv servers", "on all servers", 0 };
	m_listitem_t *item;
	playermodelinfo_s *playermodel;
	int i, menu_autoaction_startid;

	M_GetPlayerColor();

	if( playermodelsItemsList.numItems == 0 )
		return qfalse;

	if( hand < 0 || hand > 2 )
		trap_Cvar_SetValue( "hand", 0 );

	// find the skin and model matching our user settings (if any)
	UI_FindIndexForModelAndSkin( model, skin, &currentdirectoryindex, &currentskinindex );

	s_player_config_menu.x = uis.vidWidth / 2;
	s_player_config_menu.y = uis.vidHeight / 2;
	s_player_config_menu.nitems = 0;

	menuitem = UI_InitMenuItem( "m_player_config_title1", "PLAYER SETUP", x, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemBig, NULL );
	Menu_AddItem( &s_player_config_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	yoffset += trap_SCR_strHeight( menuitem->font );

	// name
	menuitem = UI_InitMenuItem( "m_playerconfig_name", "name", x, yoffset, MTYPE_FIELD, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	UI_SetupField( menuitem, name, 20, -1 );
	Menu_AddItem( &s_player_config_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	// clan name
	menuitem = UI_InitMenuItem( "m_playerconfig_clan", "clan", x, yoffset, MTYPE_FIELD, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	UI_SetupField( menuitem, clan, MAX_CLANNAME_BYTES, -1 );
	Menu_AddItem( &s_player_config_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	// model
	menuitem = UI_InitMenuItem( "m_playerconfig_model", "model", x, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, ModelCallback );
	UI_SetupSpinControl( menuitem, playermodelsItemsList.item_names, currentdirectoryindex );
	Menu_AddItem( &s_player_config_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	// skin
	item = UI_FindItemInScrollListWithId( &playermodelsItemsList, currentdirectoryindex );
	playermodel = (playermodelinfo_s *)item->data;
	menuitem = UI_InitMenuItem( "m_playerconfig_skin", "skin", x, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	UI_SetupSpinControl( menuitem, playermodel->skinnames, currentskinindex );
	Menu_AddItem( &s_player_config_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_playerconfig_colorred", "red", x, yoffset, MTYPE_SLIDER, ALIGN_RIGHT_TOP, uis.fontSystemSmall, UI_ColorRedCallback );
	Menu_AddItem( &s_player_config_menu, menuitem );
	UI_SetupSlider( menuitem, 12, playerColor[0], 0, 255 );
	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_playerconfig_colorgreen", "green", x, yoffset, MTYPE_SLIDER, ALIGN_RIGHT_TOP, uis.fontSystemSmall, UI_ColorGreenCallback );
	Menu_AddItem( &s_player_config_menu, menuitem );
	UI_SetupSlider( menuitem, 12, playerColor[1], 0, 255 );
	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_playerconfig_colorblue", "blue", x, yoffset, MTYPE_SLIDER, ALIGN_RIGHT_TOP, uis.fontSystemSmall, UI_ColorBlueCallback );
	Menu_AddItem( &s_player_config_menu, menuitem );
	UI_SetupSlider( menuitem, 12, playerColor[2], 0, 255 );
	yoffset += trap_SCR_strHeight( menuitem->font );

	yoffset += trap_SCR_strHeight( menuitem->font );

	// cg_crosshair
	menuitem = UI_InitMenuItem( "m_playerconfig_crosshair", "crosshair", x-16, yoffset, MTYPE_ACTION, ALIGN_RIGHT_TOP, uis.fontSystemSmall, CrosshairFunc );
	Menu_AddItem( &s_player_config_menu, menuitem );
	menu_crosshair_id = trap_Cvar_Value( "cg_crosshair" );
	clamp( menu_crosshair_id, 0, NUM_CROSSHAIRS - 1 );
	s_crosshair_pic = trap_R_RegisterPic( va( "gfx/hud/crosshair%i", menu_crosshair_id ) );
	yoffset += trap_SCR_strHeight( menuitem->font );

	yoffset += 2 *trap_SCR_strHeight( menuitem->font );

	// cg_crosshair_strong
	menuitem = UI_InitMenuItem( "m_playerconfig_crosshair_strong", "strong crosshair", x-16, yoffset, MTYPE_ACTION, ALIGN_RIGHT_TOP, uis.fontSystemSmall, CrosshairStrongFunc );
	Menu_AddItem( &s_player_config_menu, menuitem );
	menu_crosshair_strong_id = trap_Cvar_Value( "cg_crosshair_strong" );
	clamp( menu_crosshair_strong_id, 0, NUM_CROSSHAIRS - 1 );
	s_crosshair_strong_pic = trap_R_RegisterPic( va( "gfx/hud/crosshair%i", menu_crosshair_strong_id ) );
	yoffset += trap_SCR_strHeight( menuitem->font );

	yoffset += trap_SCR_strHeight( menuitem->font );

	// fov
	menuitem = UI_InitMenuItem( "m_playerconfig_fov", "client FOV", x, yoffset, MTYPE_FIELD, ALIGN_RIGHT_TOP, uis.fontSystemSmall, UpdateFOVFunc );
	Menu_AddItem( &s_player_config_menu, menuitem );
	UI_SetupField( menuitem, trap_Cvar_String( "fov" ) ? trap_Cvar_String( "fov" ) : "90", 4, -1 );
	yoffset += trap_SCR_strHeight( menuitem->font );

	// zoomfov
	menuitem = UI_InitMenuItem( "m_playerconfig_zoomfov", "zoom FOV", x, yoffset, MTYPE_FIELD, ALIGN_RIGHT_TOP, uis.fontSystemSmall, UpdateZoomFOVFunc );
	Menu_AddItem( &s_player_config_menu, menuitem );
	UI_SetupField( menuitem, trap_Cvar_String( "zoomfov" ) ? trap_Cvar_String( "zoomfov" ) : "40", 4, -1 );
	yoffset += trap_SCR_strHeight( menuitem->font );

	// hand
	menuitem = UI_InitMenuItem( "m_playerconfig_handedness", "handedness", x, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	UI_SetupSpinControl( menuitem, handedness, trap_Cvar_Value( "hand" ) );
	Menu_AddItem( &s_player_config_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	// cg_clientHUD
	M_PlayerConfig_CreateHUDsList();
	if( HUDnames )
	{
		int startvalue;
		menuitem = UI_InitMenuItem( "m_playerconfig_clienthud", "client HUD", x, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, ClientHUDCallBack );
		Menu_AddItem( &s_player_config_menu, menuitem );
		for( i = 0, startvalue = 0; HUDnames[i] != NULL; i++ )
		{
			if( !Q_stricmp( HUDnames[i], cg_clientHUD->string ) )
			{
				startvalue = i;
				break;
			}
		}
		UI_SetupSpinControl( menuitem, HUDnames, startvalue );
	}
	yoffset += trap_SCR_strHeight( menuitem->font );

	// cg_showFPS
	menuitem = UI_InitMenuItem( "m_playerconfig_showhelp", "show help", x, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, UpdateShowHelpFunc );
	Menu_AddItem( &s_player_config_menu, menuitem );
	UI_SetupSpinControl( menuitem, offon_names, trap_Cvar_Value( "cg_showHelp" ) );
	yoffset += trap_SCR_strHeight( menuitem->font );

	// cg_showMinimap
	menuitem = UI_InitMenuItem( "m_playerconfig_showminimap", "show minimap", x, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, UpdateShowMinimap );
	Menu_AddItem( &s_player_config_menu, menuitem );
	UI_SetupSpinControl( menuitem, minimapmodes, trap_Cvar_Value( "cg_showMinimap" ) );
	yoffset += trap_SCR_strHeight( menuitem->font );

	// cg_showItemTimers
	menuitem = UI_InitMenuItem( "m_playerconfig_showitemtimers", "show item timers", x, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, UpdateShowItemTimers );
	Menu_AddItem( &s_player_config_menu, menuitem );
	UI_SetupSpinControl( menuitem, itemtimersmodes, trap_Cvar_Value( "cg_showItemTimers" ) );
	yoffset += trap_SCR_strHeight( menuitem->font );

	// cg_showFPS
	menuitem = UI_InitMenuItem( "m_playerconfig_showfps", "show fps", x, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, UpdateShowFPSFunc );
	Menu_AddItem( &s_player_config_menu, menuitem );
	UI_SetupSpinControl( menuitem, offon_names, trap_Cvar_Value( "cg_showFPS" ) );
	yoffset += trap_SCR_strHeight( menuitem->font );

	// cg_showspeedmeter
	menuitem = UI_InitMenuItem( "m_playerconfig_showspeed", "show speed meter", x, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, UpdateSpeedMeterFunc );
	Menu_AddItem( &s_player_config_menu, menuitem );
	UI_SetupSpinControl( menuitem, offon_names, trap_Cvar_Value( "cg_showSpeed" ) );
	yoffset += trap_SCR_strHeight( menuitem->font );

	// cg_weaponlist
	menuitem = UI_InitMenuItem( "m_playerconfig_weaponlist", "HUD weapon list", x, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, UpdateWeaponlistFunc );
	Menu_AddItem( &s_player_config_menu, menuitem );
	UI_SetupSpinControl( menuitem, offon_names, trap_Cvar_Value( "cg_weaponlist" ) );
	yoffset += trap_SCR_strHeight( menuitem->font );

	// cg_autoaction
	menuitem = UI_InitMenuItem( "m_playerconfig_autoaction", "autoaction", x, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_player_config_menu, menuitem );

	menu_autoaction_startid = 0;
	if( trap_Cvar_Value( "cg_autoaction_demo" ) )
		menu_autoaction_startid |= 1;
	if( trap_Cvar_Value( "cg_autoaction_screenshot" ) )
		menu_autoaction_startid |= 2;
	if( trap_Cvar_Value( "cg_autoaction_stats" ) )
		menu_autoaction_startid |= 4;

	UI_SetupSpinControl( menuitem, autoaction, menu_autoaction_startid );
	yoffset += trap_SCR_strHeight( menuitem->font );

	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_playerconfig_apply", "apply", x + 16, yoffset, MTYPE_ACTION, ALIGN_LEFT_TOP, uis.fontSystemBig, M_PlayerConfig_SaveAndClose );
	Menu_AddItem( &s_player_config_menu, menuitem );
	UI_SetupButton( menuitem, qtrue );

	menuitem = UI_InitMenuItem( "m_playerconfig_back", "back", x - 16, yoffset, MTYPE_ACTION, ALIGN_RIGHT_TOP, uis.fontSystemBig, M_PlayerConfig_Close );
	Menu_AddItem( &s_player_config_menu, menuitem );
	UI_SetupButton( menuitem, qtrue );

	Menu_Center( &s_player_config_menu );
	Menu_Init( &s_player_config_menu, qfalse );

	return qtrue;
}

/*
* PlayerConfig_MenuDraw - skelmod
*/
static void PlayerConfig_MenuDraw( void )
{
	menucommon_t *playerbox, *skinbox;
	m_listitem_t *item;
	playermodelinfo_s *playermodel;
	int x, y, width, height;
	static int pmod_frame = -1, pmod_oldframe;

	//draw crosshair
	if( s_crosshair_pic )
	{
		menucommon_t *crosshairbox = UI_MenuItemByName( "m_playerconfig_crosshair" );
		if( crosshairbox )
		{
			trap_R_DrawStretchPic( s_player_config_menu.x + crosshairbox->x + 32,
				s_player_config_menu.y + crosshairbox->y - 8, 32, 32,
				0, 0, 1, 1, colorWhite, (struct shader_s *)s_crosshair_pic );
		}
	}

	//draw strong crosshair
	if( s_crosshair_strong_pic )
	{
		menucommon_t *crosshairbox = UI_MenuItemByName( "m_playerconfig_crosshair_strong" );
		if( crosshairbox )
		{
			trap_R_DrawStretchPic( s_player_config_menu.x + crosshairbox->x + 32,
				s_player_config_menu.y + crosshairbox->y - 8, 32, 32,
				0, 0, 1, 1, colorWhite, (struct shader_s *)s_crosshair_strong_pic );
		}
	}

	// go for the model

	// if the color cvar is modified from outside of the ui (console), update the color vector
	if( pcolor && pcolor->modified )
	{
		M_GetPlayerColor();
	}

	playerbox = UI_MenuItemByName( "m_playerconfig_model" );
	skinbox = UI_MenuItemByName( "m_playerconfig_skin" );
	if( !playerbox || !skinbox )
		return;

	Menu_Draw( &s_player_config_menu ); // draw menu first

	item = UI_FindItemInScrollListWithId( &playermodelsItemsList, playerbox->curvalue );
	if( item && item->data )
	{
		playermodel = (playermodelinfo_s *)item->data;
		if( !playermodel->skinnames[skinbox->curvalue] )
			return;

		if( pmod_frame == -1 )
		{
			pmod_frame = pmod_oldframe = ui_playermodel_firstframe->integer;
		}
		if( UI_PlayerModelNextFrameTime() )
		{
			pmod_oldframe = pmod_frame;
			pmod_frame++;
			if( pmod_frame > ui_playermodel_lastframe->integer )
				pmod_frame = ui_playermodel_firstframe->integer;
		}

		// draw player model
		x = ( uis.vidWidth / 2 ) - ( 64*( uis.vidWidth/800 ) );
		y = 0;
		width = ( uis.vidWidth / 2 );
		height = uis.vidHeight;
		UI_DrawPlayerModel( playermodel->directory, playermodel->skinnames[skinbox->curvalue], playerColor, x, y, width, height, pmod_frame, pmod_oldframe );
	}
}


static const char *PlayerConfig_MenuKey( int key )
{
	menucommon_t *item;

	item = Menu_ItemAtCursor( &s_player_config_menu );

	if( key == K_ESCAPE || ( ( key == K_MOUSE2 ) && ( item->type != MTYPE_SPINCONTROL ) &&
		( item->type != MTYPE_SLIDER ) ) )
	{
		M_PlayerConfig_SaveAndClose( NULL );
		return menu_out_sound;
	}

	return Default_MenuKey( &s_player_config_menu, key );
}

static const char *PlayerConfig_MenuCharEvent( qwchar key )
{
	return Default_MenuCharEvent( &s_player_config_menu, key );
}

void M_Menu_PlayerConfig_f( void )
{
	if( !PlayerConfig_MenuInit() )
	{
		Menu_SetStatusBar( &s_player_config_menu, "No valid player models found" );
		return;
	}

	Menu_SetStatusBar( &s_player_config_menu, NULL );
	M_PushMenu( &s_player_config_menu, PlayerConfig_MenuDraw, PlayerConfig_MenuKey, PlayerConfig_MenuCharEvent );
}
