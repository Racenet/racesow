/*

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

#define PREDEFINES_PROFILES

static menuframework_s s_performance_menu;

#ifdef PREDEFINES_PROFILES
static char *gfx_profiles[] = { "low", "medium", "high", "high+", "contrast", 0 };
#else
static char **gfx_profiles;
#endif

static void AdvancedButton( menucommon_t *unused )
{
	trap_Cmd_ExecuteText( EXEC_APPEND, "menu_performanceadv\n" );
}

static void ApplyButton( menucommon_t *unused )
{
	menucommon_t *menuitem;

	menuitem = UI_MenuItemByName( "m_performance_resolution" );
	trap_Cvar_SetValue( "r_mode", menuitem->curvalue - 1 );

	menuitem = UI_MenuItemByName( "m_performance_fullscreen" );
	trap_Cvar_SetValue( "vid_fullscreen", menuitem->curvalue );

	menuitem = UI_MenuItemByName( "m_performance_gamma" );
	trap_Cvar_SetValue( "r_gamma", menuitem->curvalue/10.0 );

	menuitem = UI_MenuItemByName( "m_performance_colorbits" );
	trap_Cvar_SetValue( "r_colorbits", 16 * menuitem->curvalue );

	menuitem = UI_MenuItemByName( "m_performance_picmip" );
	trap_Cvar_SetValue( "r_picmip", 6 - menuitem->curvalue );

	menuitem = UI_MenuItemByName( "m_performance_filter" );
	if( menuitem->curvalue >= 2 )
	{
		trap_Cvar_Set( "r_texturemode", "GL_LINEAR_MIPMAP_LINEAR" );
		trap_Cvar_SetValue( "r_texturefilter", (1<<(menuitem->curvalue-1) ));
	}
	else
	{
		trap_Cvar_Set( "r_texturemode", menuitem->curvalue ? "GL_LINEAR_MIPMAP_LINEAR" : "GL_LINEAR_MIPMAP_NEAREST" );
		trap_Cvar_SetValue( "r_texturefilter", 1 );
	}

	menuitem = UI_MenuItemByName( "m_performance_skymip" );
	if( menuitem->curvalue )
	{
		trap_Cvar_SetValue( "r_fastsky", 0 );
		trap_Cvar_SetValue( "r_skymip", 4 - menuitem->curvalue );
	}
	else
	{
		trap_Cvar_SetValue( "r_fastsky", 1 );
	}

	menuitem = UI_MenuItemByName( "m_performance_swapinterval" );
	trap_Cvar_SetValue( "r_swapinterval", menuitem->curvalue );

	menuitem = UI_MenuItemByName( "m_performance_LOD_slider" );
	trap_Cvar_SetValue( "r_subdivisions", (1<<(4 - menuitem->curvalue)) );
	trap_Cvar_SetValue( "r_lodbias", (menuitem->curvalue >= 2 ? 0 : 2-menuitem->curvalue) );

#if 0
	menuitem = UI_MenuItemByName( "m_performance_glsl" );
	trap_Cvar_SetValue( "gl_ext_GLSL", menuitem->curvalue );
#endif

	menuitem = UI_MenuItemByName( "m_performance_pplighting" );

	if( menuitem->curvalue )
		trap_Cvar_SetValue( "r_lighting_vertexlight", 0 );
	switch( menuitem->curvalue )
	{
		case 0:
			trap_Cvar_SetValue( "r_lighting_vertexlight", 1 );
			break;
		case 1:
			trap_Cvar_SetValue( "r_lighting_deluxemapping", 0 );
			break;
		case 2:
			trap_Cvar_SetValue( "r_lighting_deluxemapping", 1 );
			trap_Cvar_SetValue( "r_lighting_specular", 1 );
			break;
		case 3:
			trap_Cvar_SetValue( "r_lighting_deluxemapping", 1 );
			trap_Cvar_SetValue( "r_lighting_specular", 0 );
	}

	trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart\n" );
}

static void ApplyProfileButton( menucommon_t *unused )
{
	menucommon_t *menuitem;
	char cmd[MAX_QPATH + 5 + 1];

	// store profile choice for UI
	menuitem = UI_MenuItemByName( "m_performance_profile" );
	trap_Cvar_ForceSet( "ui_gfxprofile", va( "%i", menuitem->curvalue ) );

	// execute profile
	Q_snprintfz( cmd, sizeof( cmd ), "exec profiles/gfx_%s.cfg\n", gfx_profiles[menuitem->curvalue] );
	trap_Cmd_ExecuteText( EXEC_APPEND, cmd );
}

static void Performance_Init( void )
{
	menucommon_t *menuitem;
	int yoffset = 0;

	char custom_resolution[64];
	static char **resolutions;

	static char *colordepth_names[] = { "desktop", "16 bits", "32 bits", 0 };

	static char *plighting_names[] = { "vertex (fast)", "lightmaps (normal)", "per pixel (quality)", "per pixel (no specular)", 0 };

	static char **texfilter_names;
	int anisotropic, spinindex;

#ifndef PREDEFINES_PROFILES
	if( !gfx_profiles )
	{
		int i = 0, num, total, len;
		char *current, buffer[1024];
		total = trap_FS_GetFileList( "profiles", ".cfg", NULL, 0, 0, 0 );

		if( total )
			gfx_profiles = UI_Malloc( sizeof( char * ) * ( total + 1 ) );

		while( i < total )
		{
			if( ( num = trap_FS_GetFileList( "profiles", ".cfg", buffer, sizeof( buffer ), i, total ) ) == 0 )
			{
				i++; // can happen if the filename is too long to fit into the buffer or we're done
				continue;
			}

			// add profiles to profiles list
			for( current = buffer ; num ; i++, num--, current += len )
			{
				len = strlen( current ) + 1;
				if( strncmp( current, "gfx_", 4 ) )
					continue;

				COM_StripExtension( current );

				gfx_profiles[i] = UI_Malloc( strlen( current + 4 ) + 1 );
				strcpy( gfx_profiles[i], current + 4 );
			}
		}
	}
#endif

	if( !resolutions )
	{                // count video modes
		int i, width, height;
		qboolean wideScreen;

		for( i = 0; trap_VID_GetModeInfo( &width, &height, &wideScreen, i - 1 ); i++ ) ;

		resolutions = (char **)UI_Malloc( sizeof( char * ) * ( i + 1 ) );

		for( i = 0; trap_VID_GetModeInfo( &width, &height, &wideScreen, i - 1 ); i++ )
		{
			Q_snprintfz( custom_resolution, sizeof( custom_resolution ), "%s%s%i x %i", i ? "" : "custom: ", ( wideScreen ? "W " : "" ), width, height );
			resolutions[i] = UI_CopyString( custom_resolution );
		}
		resolutions[i] = NULL;
	}

	if( !texfilter_names )
	{
		int i, count;

		for( count = 0; ; count++ )
		{
			if( trap_Cvar_Value( "gl_ext_texture_filter_anisotropic_max" ) <= (1<<count) )
				break;
		}

		texfilter_names = (char **)UI_Malloc( sizeof( char * ) * ( count + 1 + 1 ) );

		texfilter_names[0] = UI_CopyString( "bilinear" );
		for( i = 0; i < count; i++ )
			texfilter_names[i+1] = UI_CopyString( va( "trilinear %ixAF", (1<<i) ) );
		texfilter_names[i+1] = NULL;
	}

	menuitem = UI_InitMenuItem( "m_performance_title1", "GRAPHICS OPTIONS", 0, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemBig, NULL );
	Menu_AddItem( &s_performance_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );
	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_performance_profile", "profile", 0, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_performance_menu, menuitem );
	UI_SetupSpinControl( menuitem, gfx_profiles, trap_Cvar_Value( "ui_gfxprofile" ) );
	yoffset += trap_SCR_strHeight( menuitem->font );
	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_performance_applyprofile", "apply profile", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, ApplyProfileButton );
	Menu_AddItem( &s_performance_menu, menuitem );
	yoffset += 1.5 * UI_SetupButton( menuitem, qtrue );

	menuitem = UI_InitMenuItem( "m_performance_resolution", "resolution", 0, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_performance_menu, menuitem );
	UI_SetupSpinControl( menuitem, resolutions, max( trap_Cvar_Value( "r_mode" ), -1 ) + 1 );
	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_performance_fullscreen", "fullscreen", 0, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_performance_menu, menuitem );
	UI_SetupSpinControl( menuitem, noyes_names, trap_Cvar_Value( "vid_fullscreen" ) != 0 );
	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_performance_swapinterval", "vertical sync", 0, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_performance_menu, menuitem );
	UI_SetupSpinControl( menuitem, noyes_names, trap_Cvar_Value( "r_swapinterval" ) );
	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_performance_gamma", "brightness", 0, yoffset, MTYPE_SLIDER, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_performance_menu, menuitem );
	UI_SetupSlider( menuitem, 12, bound( (int)( trap_Cvar_Value( "r_gamma" ) * 10.0f ), 5, 13 ), 5, 13 );
	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_performance_colorbits", "color quality", 0, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_performance_menu, menuitem );
	if( !Q_stricmp( trap_Cvar_String( "r_colorbits" ), "16" ) )
		UI_SetupSpinControl( menuitem, colordepth_names, 1 );
	else if( !Q_stricmp( trap_Cvar_String( "r_colorbits" ), "32" ) )
		UI_SetupSpinControl( menuitem, colordepth_names, 2 );
	else
		UI_SetupSpinControl( menuitem, colordepth_names, 0 );
	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_performance_picmip", "texture quality", 0, yoffset, MTYPE_SLIDER, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_performance_menu, menuitem );
	UI_SetupSlider( menuitem, 12, 6-trap_Cvar_Value( "r_picmip" ), 0, 6 );
	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_performance_filter", "texture filter", 0, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_performance_menu, menuitem );
	anisotropic = trap_Cvar_Value( "r_texturefilter" );
	if( anisotropic >= 2 )
		spinindex = NEARESTEXPOF2( anisotropic ) + 1;
	else if( !Q_stricmp( trap_Cvar_String( "r_texturemode" ), "GL_LINEAR_MIPMAP_NEAREST" ) )
		spinindex = 0;
	else
		spinindex = 1;
	UI_SetupSpinControl( menuitem, texfilter_names, spinindex );
	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_performance_skymip", "sky quality", 0, yoffset, MTYPE_SLIDER, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_performance_menu, menuitem );
	UI_SetupSlider( menuitem, 12, (trap_Cvar_Value( "r_fastsky" ) ? 0 : 4-trap_Cvar_Value( "r_skymip" )), 0, 4 );
	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_performance_LOD_slider", "geometry level of detail", 0, yoffset, MTYPE_SLIDER, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_performance_menu, menuitem );
	UI_SetupSlider( menuitem, 12, 4-max( trap_Cvar_Value( "r_lodbias" ), NEARESTEXPOF2( trap_Cvar_Value( "r_subdivisions" ) ) ), 0, 4 );
	yoffset += trap_SCR_strHeight( menuitem->font );

#if 0
	menuitem = UI_InitMenuItem( "m_performance_glsl", "opengl shaders", 0, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_performance_menu, menuitem );
	UI_SetupSpinControl( menuitem, offon_names, trap_Cvar_Value( "gl_ext_GLSL" ) );
	yoffset += trap_SCR_strHeight( menuitem->font );
#endif

	if( trap_Cvar_Value( "r_lighting_vertexlight" ) )
		spinindex = 0;
	else if( !trap_Cvar_Value( "r_lighting_deluxemapping" ) )
		spinindex = 1;
	else if( trap_Cvar_Value( "r_lighting_specular" ) )
		spinindex = 2;
	else
		spinindex = 3;

	menuitem = UI_InitMenuItem( "m_performance_pplighting", "lighting", 0, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_performance_menu, menuitem );
	UI_SetupSpinControl( menuitem, plighting_names, spinindex );
	yoffset += trap_SCR_strHeight( menuitem->font );

	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_performance_back", "back", -16, yoffset, MTYPE_ACTION, ALIGN_RIGHT_TOP, uis.fontSystemBig, M_genericBackFunc );
	Menu_AddItem( &s_performance_menu, menuitem );
	UI_SetupButton( menuitem, qtrue );
	menuitem = UI_InitMenuItem( "m_performance_apply", "apply", 16, yoffset, MTYPE_ACTION, ALIGN_LEFT_TOP, uis.fontSystemBig, ApplyButton );
	Menu_AddItem( &s_performance_menu, menuitem );
	yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;;

	yoffset += trap_SCR_strHeight( uis.fontSystemSmall );

	menuitem = UI_InitMenuItem( "m_performance_advanced", "advanced options", 0, yoffset, MTYPE_ACTION, ALIGN_CENTER_TOP, uis.fontSystemBig, AdvancedButton );
	Menu_AddItem( &s_performance_menu, menuitem );
	UI_SetupButton( menuitem, qtrue );

	Menu_Center( &s_performance_menu );
	Menu_Init( &s_performance_menu, qfalse );
}

static void Performance_MenuDraw( void )
{
	Menu_AdjustCursor( &s_performance_menu, 1 );
	Menu_Draw( &s_performance_menu );
}

static const char *Performance_MenuKey( int key )
{
	return Default_MenuKey( &s_performance_menu, key );
}

static const char *Performance_MenuCharEvent( qwchar key )
{
	return Default_MenuCharEvent( &s_performance_menu, key );
}

void M_Menu_Performance_f( void )
{
	Performance_Init();
	M_PushMenu( &s_performance_menu, Performance_MenuDraw, Performance_MenuKey, Performance_MenuCharEvent );
}
