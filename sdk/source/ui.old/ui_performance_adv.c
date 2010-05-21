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

static menuframework_s s_performanceadv_menu;

static void PortalmapsControl( menucommon_t *s )
{
	menucommon_t *menuitem, *slider;

	menuitem = UI_MenuItemByName( "m_performanceadv_portalmaps_maxtexsize" );
	menuitem->disabled = !s->curvalue;
	slider = UI_MenuItemByName( "m_performanceadv_portalmaps_maxtexsize_slider" );
	if( s->curvalue == 1 && menuitem->curvalue != 1 )
		slider->disabled = qtrue;
	else
		slider->disabled = !s->curvalue;
}

static void PortalMapsMaxTexSizeControl( menucommon_t *unused )
{
	menucommon_t *menuitem = UI_MenuItemByName( "m_performanceadv_portalmaps_maxtexsize_slider" );
	menuitem->disabled = !menuitem->disabled;
}

static void ShadowsControl( menucommon_t *s )
{
	menucommon_t *menuitem, *slider;
	
	menuitem = UI_MenuItemByName( "m_performanceadv_shadowmap_maxtexsize" );
	menuitem->disabled = s->curvalue != 3;

	slider = UI_MenuItemByName( "m_performanceadv_shadowmapsPCF_slider" );
	slider->disabled = s->curvalue != 3;

	slider = UI_MenuItemByName( "m_performanceadv_shadowmap_maxtexsize_slider" );
	if( s->curvalue != 3 || !menuitem->curvalue )
		slider->disabled = qtrue;
	else
		slider->disabled = s->curvalue != 3;
}

static void ShadowMapMaxTexSizeControl( menucommon_t *unused )
{
	menucommon_t *menuitem = UI_MenuItemByName( "m_performanceadv_shadowmap_maxtexsize_slider" );
	menuitem->disabled = !menuitem->disabled;
}

static void OffsetMappingControl( menucommon_t *s )
{
	menucommon_t *menuitem;
	
	menuitem = UI_MenuItemByName( "m_performanceadv_reliefmapping" );
	menuitem->disabled = (s->curvalue == 0);
}

static void ApplyButton( menucommon_t *unused )
{
	menucommon_t *menuitem;
	int maxfps;

	maxfps = atoi( UI_GetMenuitemFieldBuffer( UI_MenuItemByName( "m_performanceadv_maxfps" ) ) );
	trap_Cvar_SetValue( "cl_maxfps", (maxfps ? max( maxfps, 24 ) : 0) );

	menuitem = UI_MenuItemByName( "m_performanceadv_sleep" );
	trap_Cvar_SetValue( "cl_sleep", menuitem->curvalue );

	menuitem = UI_MenuItemByName( "m_performanceadv_portalmaps" );
	trap_Cvar_SetValue( "r_portalmaps", menuitem->curvalue );

	menuitem = UI_MenuItemByName( "m_performanceadv_portalmaps_maxtexsize" );
	if( !menuitem->curvalue )
		trap_Cvar_SetValue( "r_portalmaps_maxtexsize", 0 );
	else
	{
		menuitem = UI_MenuItemByName( "m_performanceadv_portalmaps_maxtexsize_slider" );
		trap_Cvar_SetValue( "r_portalmaps_maxtexsize", menuitem->curvalue );
	}

#ifdef CGAMEGETLIGHTORIGIN
	menuitem = UI_MenuItemByName( "m_performanceadv_shadows" );
	trap_Cvar_SetValue( "cg_shadows", menuitem->curvalue );
	trap_Cvar_SetValue( "r_stencilbits", ( menuitem->curvalue > 1 )*8 ); //  2 is SHADOWS_STENCIL

	menuitem = UI_MenuItemByName( "m_performanceadv_shadowmap_maxtexsize" );
	if( !menuitem->curvalue )
		trap_Cvar_SetValue( "r_shadows_maxtexsize", 0 );
	else
	{
		menuitem = UI_MenuItemByName( "m_performanceadv_shadowmap_maxtexsize_slider" );
		trap_Cvar_SetValue( "r_shadows_maxtexsize", menuitem->curvalue );
	}
#else
	menuitem = UI_MenuItemByName( "m_video_cg_shadows" );
	trap_Cvar_SetValue( "r_shadows", menuitem->curvalue );
	trap_Cvar_SetValue( "r_stencilbits", ( menuitem->curvalue != 0 ) * 8 ); //  2 is SHADOWS_STENCIL
#endif

	menuitem = UI_MenuItemByName( "m_performanceadv_shadowmapsPCF_slider" );
	trap_Cvar_SetValue( "r_shadows_pcf", (menuitem->curvalue ? (menuitem->curvalue+1) : 0) );

	menuitem = UI_MenuItemByName( "m_performanceadv_selfshadow" );
	trap_Cvar_SetValue( "cg_showSelfShadow", menuitem->curvalue );

	menuitem = UI_MenuItemByName( "m_performanceadv_simpleitems" );
	trap_Cvar_SetValue( "cg_simpleItems", menuitem->curvalue );

	menuitem = UI_MenuItemByName( "m_performanceadv_decals" );
	trap_Cvar_SetValue( "cg_decals", menuitem->curvalue );

	menuitem = UI_MenuItemByName( "m_performanceadv_outlines" );
	trap_Cvar_SetValue( "cg_outlineWorld", (menuitem->curvalue & 1 ? 1 : 0) );
	trap_Cvar_SetValue( "cg_outlineModels", (menuitem->curvalue & 2 ? 1 : 0) );

	menuitem = UI_MenuItemByName( "m_performanceadv_dynamiclight" );
	trap_Cvar_SetValue( "r_dynamiclight", menuitem->curvalue );

	menuitem = UI_MenuItemByName( "m_performanceadv_cartoonfx" );
	trap_Cvar_SetValue( "cg_cartoonRockets", (menuitem->curvalue & 1 ? 1 : 0) );
	trap_Cvar_SetValue( "cg_dashEffect", (menuitem->curvalue & 2 ? 1 : 0) );
	trap_Cvar_SetValue( "cg_fallEffect", (menuitem->curvalue & 2 ? 1 : 0) );

	menuitem = UI_MenuItemByName( "m_performanceadv_offsetmapping" );
	trap_Cvar_SetValue( "r_offsetmapping", menuitem->curvalue );

	menuitem = UI_MenuItemByName( "m_performanceadv_reliefmapping" );
	trap_Cvar_SetValue( "r_offsetmapping_reliefmapping", menuitem->curvalue );

	menuitem = UI_MenuItemByName( "m_performanceadv_bloom" );
	trap_Cvar_SetValue( "r_bloom", menuitem->curvalue );

	trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart\n" );
}

static void PerformanceAdv_Init( void )
{
	menucommon_t *menuitem, *shadows;
	int yoffset = 0;
	int spinindex;

	static char *maxtexsize_names[] = { "best", "custom", 0 };
	int maxtexsize = min( trap_Cvar_Value( "gl_max_texture_size" ), min( uis.vidWidth, uis.vidHeight ) );

	static char *dynamiclights_names[] = { "off", "best", "fast", 0 };

	static char *outlines_names[] = { "nothing", "world", "models", "world and models", 0 };
	static char *offsetmapping_names[] = { "off", "world", "marks on walls", "everything", 0 };

	static char *cartoonfx_names[] = { "off", "rockets", "dash and fall", "everything", 0 };

#ifdef CGAMEGETLIGHTORIGIN
	static char *shadows_names[] = { "off", "simple", "stencil", "shadow maps", 0 };
#endif

	menuitem = UI_InitMenuItem( "m_performanceadv_title1", "ADVANCED", 0, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemBig, NULL );
	Menu_AddItem( &s_performanceadv_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );
	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_performanceadv_maxfps", "max fps", 0, yoffset, MTYPE_FIELD, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_performanceadv_menu, menuitem );
	UI_SetupField( menuitem, trap_Cvar_String( "cl_maxfps" ), 4, -1 );
	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_performanceadv_sleep", "sleep state between frames", 0, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_performanceadv_menu, menuitem );
	UI_SetupSpinControl( menuitem, offon_names, trap_Cvar_Value( "cl_sleep" ) );
	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_performanceadv_simpleitems", "simple items", 0, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_performanceadv_menu, menuitem );
	UI_SetupSpinControl( menuitem, noyes_names, trap_Cvar_Value( "cg_simpleItems" ) );
	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_performanceadv_decals", "marks on walls", 0, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_performanceadv_menu, menuitem );
	UI_SetupSpinControl( menuitem, offon_names, trap_Cvar_Value( "cg_decals" ) );
	yoffset += trap_SCR_strHeight( menuitem->font );

	if( !trap_Cvar_Value( "gl_ext_GLSL" ) )
		trap_Cvar_ForceSet( "cg_outlineWorld", "0" );

	spinindex = 0;
	if( trap_Cvar_Value( "cg_outlineWorld" ) )
		spinindex |= 1;
	if( trap_Cvar_Value( "cg_outlineModels" ) )
		spinindex |= 2;

	menuitem = UI_InitMenuItem( "m_performanceadv_outlines", "draw outlines of", 0, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_performanceadv_menu, menuitem );
	UI_SetupSpinControl( menuitem, outlines_names, spinindex  );
	yoffset += trap_SCR_strHeight( menuitem->font );

	spinindex = 0;
	if( trap_Cvar_Value( "cg_cartoonRockets" ) )
		spinindex |= 1;
	if( trap_Cvar_Value( "cg_dashEffect" ) || trap_Cvar_Value( "cg_fallEffect" ) )
		spinindex |= 2;

	menuitem = UI_InitMenuItem( "m_performanceadv_cartoonfx", "cartoon effects", 0, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_performanceadv_menu, menuitem );
	UI_SetupSpinControl( menuitem, cartoonfx_names, spinindex );
	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_performanceadv_dynamiclight", "dynamic lighting", 0, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_performanceadv_menu, menuitem );
	UI_SetupSpinControl( menuitem, dynamiclights_names, trap_Cvar_Value( "r_dynamiclight" ) );
	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_performanceadv_offsetmapping", "offset mapping", 0, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, OffsetMappingControl );
	Menu_AddItem( &s_performanceadv_menu, menuitem );
	UI_SetupSpinControl( menuitem, offsetmapping_names, trap_Cvar_Value( "r_offsetmapping" ) );
	yoffset += trap_SCR_strHeight( menuitem->font );
	if( !trap_Cvar_Value( "gl_ext_GLSL" ) )
		menuitem->disabled = qtrue;
	else
		menuitem->disabled = qfalse;

	menuitem = UI_InitMenuItem( "m_performanceadv_reliefmapping", "relief mapping", 0, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_performanceadv_menu, menuitem );
	UI_SetupSpinControl( menuitem, offon_names, trap_Cvar_Value( "r_offsetmapping_reliefmapping" ) );
	yoffset += trap_SCR_strHeight( menuitem->font );
	if( !trap_Cvar_Value( "gl_ext_GLSL" ) || !trap_Cvar_Value( "r_offsetmapping" ) )
		menuitem->disabled = qtrue;
	else
		menuitem->disabled = qfalse;

	menuitem = UI_InitMenuItem( "m_performanceadv_bloom", "light bloom", 0, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_performanceadv_menu, menuitem );
	UI_SetupSpinControl( menuitem, offon_names, trap_Cvar_Value( "r_bloom" ) );
	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = shadows = UI_InitMenuItem( "m_performanceadv_shadows", "dynamic shadows", 0, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, ShadowsControl );
	Menu_AddItem( &s_performanceadv_menu, menuitem );
#ifdef CGAMEGETLIGHTORIGIN
	UI_SetupSpinControl( menuitem, shadows_names, trap_Cvar_Value( "cg_shadows" ) );
#else
	UI_SetupSpinControl( menuitem, offon_names, trap_Cvar_Value( "r_shadows" ) );
#endif
	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_performanceadv_selfshadow", "show self shadow", 0, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_performanceadv_menu, menuitem );
	UI_SetupSpinControl( menuitem, noyes_names, trap_Cvar_Value( "cg_showSelfShadow" ) );
	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_performanceadv_shadowmap_maxtexsize", "shadow maps texture size", 0, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, ShadowMapMaxTexSizeControl );
	Menu_AddItem( &s_performanceadv_menu, menuitem );
	UI_SetupSpinControl( menuitem, maxtexsize_names, trap_Cvar_Value( "r_shadows_maxtexsize" ) );
	yoffset += trap_SCR_strHeight( menuitem->font );
	if( shadows->curvalue != 3 )
		menuitem->disabled = qtrue;
	else
		menuitem->disabled = qfalse;

	menuitem = UI_InitMenuItem( "m_performanceadv_shadowmap_maxtexsize_slider", "", 0, yoffset, MTYPE_SLIDER, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_performanceadv_menu, menuitem );
	UI_SetupSlider( menuitem, 12, trap_Cvar_Value( "r_shadows_maxtexsize" ) == 0 ? maxtexsize : trap_Cvar_Value( "r_shadows_maxtexsize" ), 1, maxtexsize );
	if( shadows->curvalue != 3 || !trap_Cvar_Value( "r_shadows_maxtexsize" ) )
		menuitem->disabled = qtrue;
	else
		menuitem->disabled = qfalse;
	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_performanceadv_shadowmapsPCF_slider", "shadow maps smoothing factor", 0, yoffset, MTYPE_SLIDER, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_performanceadv_menu, menuitem );
	UI_SetupSlider( menuitem, 12, trap_Cvar_Value( "r_shadows_pcf" ), 0, 2 );
	yoffset += trap_SCR_strHeight( menuitem->font );
	if( shadows->curvalue != 3 )
		menuitem->disabled = qtrue;
	else
		menuitem->disabled = qfalse;

	menuitem = UI_InitMenuItem( "m_performanceadv_portalmaps", "portalmaps", 0, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, PortalmapsControl );
	Menu_AddItem( &s_performanceadv_menu, menuitem );
	UI_SetupSpinControl( menuitem, offon_names, trap_Cvar_Value( "r_portalmaps" ) );
	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_performanceadv_portalmaps_maxtexsize", "portalmaps texture size", 0, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, PortalMapsMaxTexSizeControl );
	Menu_AddItem( &s_performanceadv_menu, menuitem );
	UI_SetupSpinControl( menuitem, maxtexsize_names, trap_Cvar_Value( "r_portalmaps_maxtexsize" ) );
	yoffset += trap_SCR_strHeight( menuitem->font );
	menuitem = UI_InitMenuItem( "m_performanceadv_portalmaps_maxtexsize_slider", "", 0, yoffset, MTYPE_SLIDER, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_performanceadv_menu, menuitem );
	UI_SetupSlider( menuitem, 12, trap_Cvar_Value( "r_portalmaps_maxtexsize" ) == 0 ? maxtexsize : trap_Cvar_Value( "r_portalmaps_maxtexsize" ), 1, maxtexsize );
	if( !trap_Cvar_Value( "r_portalmaps_maxtexsize" ) )
		menuitem->disabled = qtrue;
	else
		menuitem->disabled = qfalse;
	yoffset += trap_SCR_strHeight( menuitem->font );

	yoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_performanceadv_back", "back", -16, yoffset, MTYPE_ACTION, ALIGN_RIGHT_TOP, uis.fontSystemBig, M_genericBackFunc );
	Menu_AddItem( &s_performanceadv_menu, menuitem );
	UI_SetupButton( menuitem, qtrue );
	menuitem = UI_InitMenuItem( "m_performanceadv_apply", "apply", 16, yoffset, MTYPE_ACTION, ALIGN_LEFT_TOP, uis.fontSystemBig, ApplyButton );
	Menu_AddItem( &s_performanceadv_menu, menuitem );
	UI_SetupButton( menuitem, qtrue );

	Menu_Center( &s_performanceadv_menu );
	Menu_Init( &s_performanceadv_menu, qfalse );
}

static void PerformanceAdv_MenuDraw( void )
{
	Menu_AdjustCursor( &s_performanceadv_menu, 1 );
	Menu_Draw( &s_performanceadv_menu );
}

static const char *PerformanceAdv_MenuKey( int key )
{
	return Default_MenuKey( &s_performanceadv_menu, key );
}

static const char *PerformanceAdv_MenuCharEvent( qwchar key )
{
	return Default_MenuCharEvent( &s_performanceadv_menu, key );
}

void M_Menu_PerformanceAdv_f( void )
{
	PerformanceAdv_Init();
	M_PushMenu( &s_performanceadv_menu, PerformanceAdv_MenuDraw, PerformanceAdv_MenuKey, PerformanceAdv_MenuCharEvent );
}
