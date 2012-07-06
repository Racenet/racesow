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
// TEAM CONFIG MENU
//
//=============================================================================

static menuframework_s s_team_config_menu;

static int currentTEAM = TEAM_ALPHA;
static menucommon_t *hasForcedModelMenuItem;
static menucommon_t *hasForcedColorMenuItem;

static cvar_t *color;
static cvar_t *model;
static cvar_t *skin;

static void UpdateTeamCvars( void )
{
	switch( currentTEAM )
	{
	case TEAM_ALPHA:
		model = trap_Cvar_Get( "cg_teamALPHAmodel", "", CVAR_ARCHIVE );
		skin = trap_Cvar_Get( "cg_teamALPHAskin", "", CVAR_ARCHIVE );
		color = trap_Cvar_Get( "cg_teamALPHAcolor", DEFAULT_TEAMALPHA_COLOR, CVAR_ARCHIVE );
		break;
	case TEAM_BETA:
		model = trap_Cvar_Get( "cg_teamBETAmodel", "", CVAR_ARCHIVE );
		skin = trap_Cvar_Get( "cg_teamBETAskin", "", CVAR_ARCHIVE );
		color = trap_Cvar_Get( "cg_teamBETAcolor", DEFAULT_TEAMBETA_COLOR, CVAR_ARCHIVE );
		break;
	case TEAM_PLAYERS:
	default:
		model = trap_Cvar_Get( "cg_teamPLAYERSmodel", "", CVAR_ARCHIVE );
		skin = trap_Cvar_Get( "cg_teamPLAYERSskin", "", CVAR_ARCHIVE );
		color = trap_Cvar_Get( "cg_teamPLAYERScolor", "", CVAR_ARCHIVE );
		break;
	}
}

/*
* M_TeamConfig_ApplyChanges
*/
static void M_TeamConfig_ApplyChanges( struct menucommon_s *unused )
{
	menucommon_t *modelitem = UI_MenuItemByName( "m_TeamConfig_model" );
	menucommon_t *skinitem = UI_MenuItemByName( "m_TeamConfig_skin" );

	UpdateTeamCvars();
	if( hasForcedModelMenuItem->curvalue )
	{
		trap_Cvar_Set( model->name, modelitem->itemnames[modelitem->curvalue] );
		trap_Cvar_Set( skin->name, skinitem->itemnames[skinitem->curvalue] );
	}
	else
	{
		trap_Cvar_Set( model->name, "" );
		trap_Cvar_Set( skin->name, "" );
	}

	if( hasForcedColorMenuItem->curvalue )
	{
		trap_Cvar_Set( color->name, va( "%i %i %i", (int)playerColor[0], (int)playerColor[1], (int)playerColor[2] ) );
	}
	else
	{
		trap_Cvar_Set( color->name, "" );
	}
}

static void M_GetTeamColor( void )
{
	int rgbcolor;
	menucommon_t *menuitem;

	rgbcolor = COM_ReadColorRGBString( color->string );
	if( rgbcolor == -1 )
	{
		rgbcolor = COM_ReadColorRGBString( color->dvalue );
	}
	if( rgbcolor != -1 )
	{
		Vector4Set( playerColor, COLOR_R( rgbcolor ), COLOR_G( rgbcolor ), COLOR_B( rgbcolor ), 255 );
	}
	else
	{
		Vector4Set( playerColor, 255, 255, 255, 255 ); // start white
	}

	// update the bars
	menuitem = UI_MenuItemByName( "m_TeamConfig_colorred" );
	menuitem->curvalue = playerColor[0];
	menuitem = UI_MenuItemByName( "m_TeamConfig_colorgreen" );
	menuitem->curvalue = playerColor[1];
	menuitem = UI_MenuItemByName( "m_TeamConfig_colorblue" );
	menuitem->curvalue = playerColor[2];
}

static void M_TeamConfig_ColorRedCallback( menucommon_t *menuitem )
{
	UI_ColorRedCallback( menuitem );
	M_TeamConfig_ApplyChanges( NULL );
}
static void M_TeamConfig_ColorGreenCallback( menucommon_t *menuitem )
{
	UI_ColorGreenCallback( menuitem );
	M_TeamConfig_ApplyChanges( NULL );
}
static void M_TeamConfig_ColorBlueCallback( menucommon_t *menuitem )
{
	UI_ColorBlueCallback( menuitem );
	M_TeamConfig_ApplyChanges( NULL );
}

static qboolean M_TeamHasModel( void )
{
	UpdateTeamCvars();

	if( model->string[0] )
		return qtrue;

	return qfalse;
}

static void M_GetTeamModel( void )
{
	menucommon_t *menuitem;
	m_listitem_t *item;
	playermodelinfo_s *playermodel;
	int currentdirectoryindex = 0;
	int currentskinindex = 0;

	if( currentTEAM < TEAM_PLAYERS )
		currentTEAM = TEAM_PLAYERS;
	if( currentTEAM >= GS_MAX_TEAMS )
		currentTEAM = GS_MAX_TEAMS - 1;

	// find the skin and model matching our user settings (if any)
	UI_FindIndexForModelAndSkin( model->string, skin->string, &currentdirectoryindex, &currentskinindex );
	menuitem = UI_MenuItemByName( "m_TeamConfig_model" );
	menuitem->curvalue = currentdirectoryindex;

	item = UI_FindItemInScrollListWithId( &playermodelsItemsList, currentdirectoryindex );
	playermodel = (playermodelinfo_s *)item->data;

	menuitem = UI_MenuItemByName( "m_TeamConfig_skin" );
	menuitem->itemnames = playermodel->skinnames;
	menuitem->curvalue = currentskinindex;
}

static void ForceAColorCallback( menucommon_t *menuitem )
{
	UpdateTeamCvars();
	if( menuitem->curvalue )
	{
		trap_Cvar_Set( color->name, va( "%i %i %i", (int)playerColor[0], (int)playerColor[1], (int)playerColor[2] ) );
	}
	else
	{
		trap_Cvar_Set( color->name, "" );
	}
}

static void ChangeTeamCallback( menucommon_t *menuitem )
{
	M_TeamConfig_ApplyChanges( NULL );
	currentTEAM = menuitem->curvalue + TEAM_PLAYERS;
	UpdateTeamCvars();

	// see if this team has a forced model set up
	hasForcedModelMenuItem->curvalue = ( model->string[0] != 0 );
	hasForcedColorMenuItem->curvalue = ( color->string[0] != 0 );

	M_GetTeamColor();
	M_GetTeamModel();
}

static void M_TeamConfig_MyTeamAlpha( menucommon_t *menuitem )
{
	trap_Cvar_Set( "cg_forceMyTeamAlpha", ( menuitem->curvalue ? "1" : "0" ) );
}

static void M_TeamConfig_TeamPlayersTeamBeta( menucommon_t *menuitem )
{
	trap_Cvar_Set( "cg_forceTeamPlayersTeamBeta", ( menuitem->curvalue ? "1" : "0" ) );
}

static void M_TeamConfig_Close( menucommon_t *menuitem )
{
	M_genericBackFunc( menuitem );
}

static void M_TeamConfig_SaveAndClose( menucommon_t *menuitem )
{
	M_TeamConfig_ApplyChanges( NULL );
	M_TeamConfig_Close( menuitem );
}

static void TeamModelCallback( menucommon_t *menuitem )
{
	// todo, update the skin itemnames list
	M_TeamConfig_ApplyChanges( NULL );
}

/*
* TeamConfig_MenuInit
*/
static qboolean TeamConfig_MenuInit( void )
{
	menucommon_t *menuitem;
	m_listitem_t *item;
	playermodelinfo_s *playermodel;
	int yoffset = 0;

	static char *team_names[] =
	{
		"PLAYERS",
		"ALPHA",
		"BETA",
		0
	};

	if( playermodelsItemsList.numItems == 0 )
		return qfalse;

	s_team_config_menu.nitems = 0;

	currentTEAM = TEAM_ALPHA;
	UpdateTeamCvars();

	// title
	menuitem = UI_InitMenuItem( "m_TeamConfig_title1", "TEAM ASPECT SETUP", 0, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemBig, NULL );
	Menu_AddItem( &s_team_config_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	yoffset += trap_SCR_strHeight( menuitem->font );

	// team
	menuitem = UI_InitMenuItem( "m_TeamConfig_team", "team", 0, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, ChangeTeamCallback );
	UI_SetupSpinControl( menuitem, team_names, TEAM_ALPHA - TEAM_PLAYERS );
	Menu_AddItem( &s_team_config_menu, menuitem );

	yoffset += trap_SCR_strHeight( menuitem->font );

	// padding
	yoffset += trap_SCR_strHeight( menuitem->font );

	// model
	menuitem = UI_InitMenuItem( "m_TeamConfig_forcemodel", "force a team model", 0, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, M_TeamConfig_ApplyChanges );
	UI_SetupSpinControl( menuitem, noyes_names, ( model->string[0] != 0 ) );
	Menu_AddItem( &s_team_config_menu, menuitem );
	hasForcedModelMenuItem = menuitem;

	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_TeamConfig_model", "model", 0, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, TeamModelCallback );
	UI_SetupSpinControl( menuitem, playermodelsItemsList.item_names, 0 );
	Menu_AddItem( &s_team_config_menu, menuitem );

	yoffset += trap_SCR_strHeight( menuitem->font );

	item = UI_FindItemInScrollListWithId( &playermodelsItemsList, 0 );
	playermodel = (playermodelinfo_s *)item->data;
	menuitem = UI_InitMenuItem( "m_TeamConfig_skin", "skin", 0, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, M_TeamConfig_ApplyChanges );
	UI_SetupSpinControl( menuitem, playermodel->skinnames, 0 );
	Menu_AddItem( &s_team_config_menu, menuitem );

	yoffset += trap_SCR_strHeight( menuitem->font );

	M_GetTeamModel();

	// padding
	yoffset += trap_SCR_strHeight( menuitem->font );

	// color
	menuitem = UI_InitMenuItem( "m_TeamConfig_forcecolor", "force a team color", 0, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, ForceAColorCallback );
	UI_SetupSpinControl( menuitem, noyes_names, ( color->string[0] != 0 ) );
	Menu_AddItem( &s_team_config_menu, menuitem );
	hasForcedColorMenuItem = menuitem;

	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_TeamConfig_colorred", "red", 0, yoffset, MTYPE_SLIDER, ALIGN_RIGHT_TOP, uis.fontSystemSmall, M_TeamConfig_ColorRedCallback );
	Menu_AddItem( &s_team_config_menu, menuitem );
	UI_SetupSlider( menuitem, 12, playerColor[0], 0, 255 );

	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_TeamConfig_colorgreen", "green", 0, yoffset, MTYPE_SLIDER, ALIGN_RIGHT_TOP, uis.fontSystemSmall, M_TeamConfig_ColorGreenCallback );
	Menu_AddItem( &s_team_config_menu, menuitem );
	UI_SetupSlider( menuitem, 12, playerColor[1], 0, 255 );

	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_TeamConfig_colorblue", "blue", 0, yoffset, MTYPE_SLIDER, ALIGN_RIGHT_TOP, uis.fontSystemSmall, M_TeamConfig_ColorBlueCallback );
	Menu_AddItem( &s_team_config_menu, menuitem );
	UI_SetupSlider( menuitem, 12, playerColor[2], 0, 255 );

	M_GetTeamColor();

	yoffset += trap_SCR_strHeight( menuitem->font );

	// padding
	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_TeamConfig_MyTeamAlpha", "switch my team to show as team ALPHA", 60, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, M_TeamConfig_MyTeamAlpha );
	UI_SetupSpinControl( menuitem, noyes_names, ( trap_Cvar_Value( "cg_forceMyTeamAlpha" ) ) );
	Menu_AddItem( &s_team_config_menu, menuitem );

	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_TeamConfig_TeamPlayersTeamBeta", "show teamless players as team BETA players", 60, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, M_TeamConfig_TeamPlayersTeamBeta );
	UI_SetupSpinControl( menuitem, noyes_names, ( trap_Cvar_Value( "cg_forceTeamPlayersTeamBeta" ) ) );
	Menu_AddItem( &s_team_config_menu, menuitem );

	yoffset += trap_SCR_strHeight( menuitem->font );

	// padding
	yoffset += trap_SCR_strHeight( menuitem->font );

	// help
	menuitem = UI_InitMenuItem( "m_TeamConfig_HelpText", "If you want to always force your teammates and your enemies to a specific look,", 10, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_team_config_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_TeamConfig_HelpText2", "set both of the above options and configure ALPHA and BETA teams", 10, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_team_config_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	// padding
	yoffset += trap_SCR_strHeight( menuitem->font );

	// back
	menuitem = UI_InitMenuItem( "m_TeamConfig_back", "back", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, M_TeamConfig_SaveAndClose );
	Menu_AddItem( &s_team_config_menu, menuitem );
	UI_SetupButton( menuitem, qtrue );

	Menu_Center( &s_team_config_menu );
	Menu_Init( &s_team_config_menu, qfalse );

	return qtrue;
}

/*
* TeamConfig_MenuDraw - skelmod
*/
static void TeamConfig_MenuDraw( void )
{
	menucommon_t *playerbox;
	m_listitem_t *item;
	playermodelinfo_s *playermodel;
	int x, y, width, height, i;
	vec4_t tmpcolor;
	static int pmod_frame = -1, pmod_oldframe;

	Menu_Draw( &s_team_config_menu ); // draw menu on back

	// go for the model

	// if the color cvar is modified from outside of the ui (console), update the color vector
	if( color && color->modified )
		M_GetTeamColor();

	x = ( uis.vidWidth / 2 )+64;
	y = ( uis.vidHeight/2 - 128 );
	width = 256;
	height = 256;

	// now on the model.
	// First of all, see if we have to draw any model
	if( M_TeamHasModel() )
	{
		if( ( model && model->modified ) || ( skin && skin->modified ) )
		{
			M_GetTeamModel();
		}

		playerbox = UI_MenuItemByName( "m_TeamConfig_model" );
		if( playerbox && model->string[0] )
		{
			item = UI_FindItemInScrollListWithId( &playermodelsItemsList, playerbox->curvalue );
			if( item && item->data )
			{
				menucommon_t *skinitem = UI_MenuItemByName( "m_TeamConfig_skin" );
				playermodel = (playermodelinfo_s *)item->data;

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
				UI_DrawPlayerModel( playermodel->directory, skinitem->itemnames[skinitem->curvalue], playerColor, x+80, y-52, width, height, pmod_frame, pmod_oldframe );
			}
		}

	}
	else
	{
		if( hasForcedColorMenuItem->curvalue )
		{
			// draw colored square
			for( i = 0; i < 3; i++ )
				tmpcolor[i] = playerColor[i] * ( 1.0/255.0 );
			tmpcolor[3] = 1;
			trap_R_DrawStretchPic( x+160, y+40, width-128, height-128, 0, 0, 1, 1, tmpcolor, uis.whiteShader );
		}
	}
}


static const char *TeamConfig_MenuKey( int key )
{
	menucommon_t *item;

	item = Menu_ItemAtCursor( &s_team_config_menu );

	if( key == K_ESCAPE || ( ( key == K_MOUSE2 ) && ( item->type != MTYPE_SPINCONTROL ) &&
		( item->type != MTYPE_SLIDER ) ) )
	{
		M_TeamConfig_SaveAndClose( NULL );
		return menu_out_sound;
	}

	return Default_MenuKey( &s_team_config_menu, key );
}

static const char *TeamConfig_MenuCharEvent( qwchar key )
{
	return Default_MenuCharEvent( &s_team_config_menu, key );
}

void M_Menu_TeamConfig_f( void )
{
	if( !TeamConfig_MenuInit() )
	{
		Menu_SetStatusBar( &s_team_config_menu, "No valid player models found" );
		return;
	}

	Menu_SetStatusBar( &s_team_config_menu, NULL );
	M_PushMenu( &s_team_config_menu, TeamConfig_MenuDraw, TeamConfig_MenuKey, TeamConfig_MenuCharEvent );
}
