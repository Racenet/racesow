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
/*
=======================================================================

SOUND MENU

=======================================================================
*/

static menuframework_s s_sound_menu;


static void UpdateSfxVolumeFunc( menucommon_t *menuitem )
{
	trap_Cvar_SetValue( "s_volume", menuitem->curvalue * 0.1f );
}

static void UpdateMusicVolumeFunc( menucommon_t *menuitem )
{
	trap_Cvar_SetValue( "s_musicvolume", menuitem->curvalue * 0.1f );
}

static void UpdateVolumePlayersFunc( menucommon_t *menuitem )
{
	trap_Cvar_SetValue( "cg_volume_players", menuitem->curvalue * 0.1f );
}

static void UpdateVolumeEffectsFunc( menucommon_t *menuitem )
{
	trap_Cvar_SetValue( "cg_volume_effects", menuitem->curvalue * 0.1f );
}

static void UpdateVolumeAnnouncerFunc( menucommon_t *menuitem )
{
	trap_Cvar_SetValue( "cg_volume_announcer", menuitem->curvalue * 0.1f );
}

static void UpdateVolumeVoiceChatFunc( menucommon_t *menuitem )
{
	trap_Cvar_SetValue( "cg_volume_voicechats", menuitem->curvalue * 0.1f );
}


static void UpdateVolumeHitSoundFunc( menucommon_t *menuitem )
{
	trap_Cvar_SetValue( "cg_volume_hitsound", menuitem->curvalue * 0.1f );
}

static void ApplySoundModuleFunc( menucommon_t *menuitem )
{
	menucommon_t *moduleitem = UI_MenuItemByName( "m_sound_module" );
	if( trap_Cvar_Value( "s_module" ) != moduleitem->curvalue )
	{
		trap_Cvar_SetValue( "s_module", moduleitem->curvalue );
		trap_Cmd_ExecuteText( EXEC_APPEND, "s_restart\n" );
	}

	M_PopMenu();
}

static void Sound_MenuInit( void )
{
	menucommon_t *menuitem;
	static char *module_items[] = { "no sound", "qf", "openal", 0 };
	int yoffset = 0;

	/*
	** configure controls menu and menu items
	*/
	s_sound_menu.x = uis.vidWidth / 2;
	s_sound_menu.nitems = 0;

	// title
	menuitem = UI_InitMenuItem( "m_sound_title1", "SOUND OPTIONS", 0, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemBig, NULL );
	Menu_AddItem( &s_sound_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	// padding
	yoffset += trap_SCR_strHeight( menuitem->font );

	// s_module
	menuitem = UI_InitMenuItem( "m_sound_module", "sound module", 0, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemMedium, NULL );
	Menu_AddItem( &s_sound_menu, menuitem );
	if( (int)trap_Cvar_Value( "s_module" ) == 2 )
		UI_SetupSpinControl( menuitem, module_items, 2 );
	else if( (int)trap_Cvar_Value( "s_module" ) == 1 )
		UI_SetupSpinControl( menuitem, module_items, 1 );
	else
		UI_SetupSpinControl( menuitem, module_items, 0 );
	yoffset += trap_SCR_strHeight( menuitem->font );

	// padding
	yoffset += trap_SCR_strHeight( menuitem->font );

	// s_volume
	menuitem = UI_InitMenuItem( "m_sound_gamevolume", "sound volume", 0, yoffset, MTYPE_SLIDER, ALIGN_RIGHT_TOP, uis.fontSystemSmall, UpdateSfxVolumeFunc );
	Menu_AddItem( &s_sound_menu, menuitem );
	UI_SetupSlider( menuitem, 12, trap_Cvar_Value( "s_volume" ) * 10, 0, 10 );
	yoffset += trap_SCR_strHeight( menuitem->font );

	// s_musicvolume
	menuitem = UI_InitMenuItem( "m_sound_musicvolume", "music volume", 0, yoffset, MTYPE_SLIDER, ALIGN_RIGHT_TOP, uis.fontSystemSmall, UpdateMusicVolumeFunc );
	Menu_AddItem( &s_sound_menu, menuitem );
	UI_SetupSlider( menuitem, 12, trap_Cvar_Value( "s_musicvolume" ) * 10, 0, 10 );
	yoffset += trap_SCR_strHeight( menuitem->font );

	// some padding
	yoffset += trap_SCR_strHeight( menuitem->font );

	// cg_volume_players
	menuitem = UI_InitMenuItem( "m_sound_playersvolume", "player sounds volume", 0, yoffset, MTYPE_SLIDER, ALIGN_RIGHT_TOP, uis.fontSystemSmall, UpdateVolumePlayersFunc );
	Menu_AddItem( &s_sound_menu, menuitem );
	UI_SetupSlider( menuitem, 12, trap_Cvar_Value( "cg_volume_players" ) * 10, 0, 20 );
	yoffset += trap_SCR_strHeight( menuitem->font );

	// cg_volume_effects
	menuitem = UI_InitMenuItem( "m_sound_effectsvolume", "effects sounds volume", 0, yoffset, MTYPE_SLIDER, ALIGN_RIGHT_TOP, uis.fontSystemSmall, UpdateVolumeEffectsFunc );
	Menu_AddItem( &s_sound_menu, menuitem );
	UI_SetupSlider( menuitem, 12, trap_Cvar_Value( "cg_volume_effects" ) * 10, 0, 20 );
	yoffset += trap_SCR_strHeight( menuitem->font );

	// cg_volume_announcer
	menuitem = UI_InitMenuItem( "m_sound_announcervolume", "announcer sounds volume", 0, yoffset, MTYPE_SLIDER, ALIGN_RIGHT_TOP, uis.fontSystemSmall, UpdateVolumeAnnouncerFunc );
	Menu_AddItem( &s_sound_menu, menuitem );
	UI_SetupSlider( menuitem, 12, trap_Cvar_Value( "cg_volume_announcer" ) * 10, 0, 20 );
	yoffset += trap_SCR_strHeight( menuitem->font );

	// cg_volume_voicechats
	menuitem = UI_InitMenuItem( "m_sound_voicechatvolume", "voice chats sounds volume", 0, yoffset, MTYPE_SLIDER, ALIGN_RIGHT_TOP, uis.fontSystemSmall, UpdateVolumeVoiceChatFunc );
	Menu_AddItem( &s_sound_menu, menuitem );
	UI_SetupSlider( menuitem, 12, trap_Cvar_Value( "cg_volume_voicechats" ) * 10, 0, 20 );
	yoffset += trap_SCR_strHeight( menuitem->font );

	// cg_volume_hitsound
	menuitem = UI_InitMenuItem( "m_sound_hitsoundsvolume", "hit sounds volume", 0, yoffset, MTYPE_SLIDER, ALIGN_RIGHT_TOP, uis.fontSystemSmall, UpdateVolumeHitSoundFunc );
	Menu_AddItem( &s_sound_menu, menuitem );
	UI_SetupSlider( menuitem, 12, trap_Cvar_Value( "cg_volume_hitsound" ) * 10, 0, 20 );
	yoffset += trap_SCR_strHeight( menuitem->font );

	yoffset += trap_SCR_strHeight( menuitem->font );

	// apply
	menuitem = UI_InitMenuItem( "m_sound_module_apply", "ok", 16, yoffset, MTYPE_ACTION, ALIGN_LEFT_TOP, uis.fontSystemBig, ApplySoundModuleFunc );
	Menu_AddItem( &s_sound_menu, menuitem );
	UI_SetupButton( menuitem, qtrue );

	menuitem = UI_InitMenuItem( "m_sound_back", "back", -16, yoffset, MTYPE_ACTION, ALIGN_RIGHT_TOP, uis.fontSystemBig, M_genericBackFunc );
	Menu_AddItem( &s_sound_menu, menuitem );
	yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;

	Menu_Center( &s_sound_menu );
	Menu_Init( &s_sound_menu, qfalse );
}

static void Sound_MenuDraw( void )
{
	Menu_AdjustCursor( &s_sound_menu, 1 );
	Menu_Draw( &s_sound_menu );
}

static const char *Sound_MenuKey( int key )
{
	return Default_MenuKey( &s_sound_menu, key );
}

static const char *Sound_MenuCharEvent( qwchar key )
{
	return Default_MenuCharEvent( &s_sound_menu, key );
}

void M_Menu_Sound_f( void )
{
	Sound_MenuInit();
	M_PushMenu( &s_sound_menu, Sound_MenuDraw, Sound_MenuKey, Sound_MenuCharEvent );
}
