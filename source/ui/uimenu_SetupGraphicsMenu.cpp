/*
Copyright (C) 2007 Benjamin Litzelmann, Dexter Haslem et al.

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

#include "../game/q_shared.h"
#include "uicore_Global.h"
#include "uimenu_SetupGraphicsMenu.h"
#include "uimenu_SetupMenu.h"
#include "uiwsw_SysCalls.h"
#include "uiwsw_Utils.h"

using namespace UIWsw;
using namespace UICore;
using namespace UIMenu;

#define NEARESTEXPOF2(x)  ((int)floor( ( log( max( (x), 1 ) ) - log( 1.5 ) ) / log( 2 ) + 1 ))

extern SetupMenu *setupmenu;

SetupGraphicsMenu *setupgraphics = NULL;

void UIMenu::M_Menu_SetupGraphics_f( void )
{
	if ( !setupmenu )
		setupmenu = new SetupMenu();

	if ( !setupgraphics )
		setupgraphics = new SetupGraphicsMenu();

	setupgraphics->Show();
}

ALLOCATOR_DEFINITION(SetupGraphicsMenu)
DELETER_DEFINITION(SetupGraphicsMenu)

SetupGraphicsMenu::SetupGraphicsMenu()
{
	int yoffset[2] = {10 , 10};	
	panel = Factory::newPanel( setupmenu->panel, 200, 20, 540, 520 );

	tabs[0] = Factory::newSwitchButton( panel, 0, 0, 270, 30, "Basic" );
	tabs[1] = Factory::newSwitchButton( panel, 270, 0, 270, 30, "Advanced" );
	tabs[0]->setSwitchHandler( tabsHandler );
	tabs[1]->setSwitchHandler( tabsHandler );
	tabs[0]->setPressed( true );

	basic = Factory::newPanel( panel, 0, 30, 540, 490 );
	advanced = Factory::newPanel( panel, 0, 30, 540, 490 );

	basic->setBackColor( transparent );
	advanced->setBackColor( transparent );
	
	basic->setVisible( true );
	advanced->setVisible( false );

	resolutionListLabel = Factory::newLabel( basic, 20, yoffset[0], 100, 20, "Resolution" );
	resolutionList = Factory::newListBox( basic, 20, yoffset[0] += 25, 125, 100, false );
	resolutionListLabel->setAlign( ALIGN_MIDDLE_CENTER );

	fullscreen = Factory::newCheckBox( basic, 155, yoffset[0], 125, 20, "Fullscreen" );
	vsync = Factory::newCheckBox( basic, 155, yoffset[0] += 35, 140, 20, "Vertical sync" );
	
	colorDepthLabel = Factory::newLabel( basic, 155, yoffset[0] += 40, 120, 20, "Color Depth" );
	colorDepthLabel->setAlign( ALIGN_MIDDLE_LEFT );
	colorDepth = Factory::newListBox( basic, 235, yoffset[0], 150, 20, true );
	colorDepth->setItemSize( colorDepth->getWidth() / 3 );
	colorDepth->setScrollbarSize( 0 );
	Factory::newListItem( colorDepth, "desktop", 0 );
	Factory::newListItem( colorDepth, "16 bit", 1 );
	Factory::newListItem( colorDepth, "32 bit", 2 );


	brightnessLabel = Factory::newLabel( basic, 20, yoffset[0] += 55, 175, 20, "Brightness" );
	brightnessLabel->setAlign( ALIGN_MIDDLE_LEFT );
	brightness = Factory::newSlider( basic, 180, yoffset[0], 325, 20, true );
	brightness->setBoundValues( 0, 10 );
	brightness->setStepValues( 1, 1 );	

	texQualityLabel = Factory::newLabel( basic, 20, yoffset[0] += 30, 175, 20, "Texture quality" );
	texQualityLabel->setAlign( ALIGN_MIDDLE_LEFT  );
	texQuality = Factory::newSlider( basic, 180, yoffset[0], 325, 20, true );
	texQuality->setBoundValues( 0, 16 );
	texQuality->setStepValues( 1, 1 );
	
	skyQualityLabel = Factory::newLabel( basic, 20, yoffset[0] += 30, 175, 20, "Sky quality" );
	skyQualityLabel->setAlign( ALIGN_MIDDLE_LEFT  );
	skyQuality = Factory::newSlider( basic, 180, yoffset[0], 325, 20, true );
	skyQuality->setBoundValues( 0, 4 );
	skyQuality->setStepValues( 1, 1 );

	geometryLODLabel = Factory::newLabel( basic, 20, yoffset[0] += 30, 175, 20, "Geometry level of detail" );
	geometryLODLabel->setAlign( ALIGN_MIDDLE_LEFT );
	geometryLOD = Factory::newSlider( basic, 180, yoffset[0], 325, 20, true );
	geometryLOD->setBoundValues( 0, 4 );

	/*
	pplLabel = Factory::newLabel( basic, 20, yoffset[0] += 30, 175, 20, "Per-pixel lighting" );
	perPixelLighting = Factory::newListBox( basic, 180, yoffset[0], 325, 20, true ) ;
	perPixelLighting->setItemSize( perPixelLighting->getWidth() / 3 );
	perPixelLighting->setScrollbarSize( 0 );
	Factory::newListItem( perPixelLighting, "off", 0 );
	Factory::newListItem( perPixelLighting, "on", 1 );
	Factory::newListItem( perPixelLighting, "no specular", 2 );
	*/	
	texFilterLabel = Factory::newLabel( basic, 20, yoffset[0] += 30, 175, 20, "Texture Filter" );
	texFilterLabel->setAlign( ALIGN_MIDDLE_LEFT );
	texFilter =  Factory::newListBox( basic, 180, yoffset[0], 325, 20, true );
	texFilter->setItemSize( texFilter->getWidth() / 6 );
	texFilter->setScrollbarSize( 0 );
	Factory::newListItem( texFilter, "bilinear", 0 );
	Factory::newListItem( texFilter, "trilinear", 1 );
	Factory::newListItem( texFilter, "2xAF", 2 );
	Factory::newListItem( texFilter, "4xAF", 3 );
	Factory::newListItem( texFilter, "8xAF", 4 );
	Factory::newListItem( texFilter, "16xAF", 5 );

	perPixelLighting = Factory::newCheckBox( basic, 20, yoffset[0] += 30, 150, 20, "Per pixel lighting" );

	static char **resolutions;
	char custom_resolution[64];

	if ( ! resolutions )
	{
		int i, width, height;
        qboolean wideScreen;

        for( i = 0; Trap::VID_GetModeInfo( &width, &height, &wideScreen, i ); i++ ) ;
		
        resolutions = (char **)malloc( sizeof( char * ) * ( i + 1 ) );

		for( i = 0; Trap::VID_GetModeInfo( &width, &height, &wideScreen, i ); i++ )
        {
            Q_snprintfz( custom_resolution, sizeof( custom_resolution ), "%s%i x %i", ( wideScreen ? "W " : "" ), width, height );
            resolutions[i] = UI_CopyString( custom_resolution );
			Factory::newListItem( resolutionList, resolutions[i], resolutions[i][i] );		
        }
        resolutions[i] = NULL;
		free(resolutions);
    }

	profileListLabel = Factory::newLabel( basic, 20, yoffset[0] += 100, 100, 20, "Profiles" );
	profileListLabel->setAlign( ALIGN_MIDDLE_CENTER );
	profileList = Factory::newListBox( basic, 20, yoffset[0] += 20, 325, 25, true );
	profileList->setItemSize( profileList->getWidth() / 5 );
	profileList->setScrollbarSize( 0 );
	loadProfile = Factory::newButton( basic, 355, yoffset[0], 50, 25, "Load" );
	// FIXME: just doin for alignment
	Factory::newListItem( profileList, "Low", 0 );
	Factory::newListItem( profileList, "Medium", 1 );
	Factory::newListItem( profileList, "High", 2 );
	Factory::newListItem( profileList, "High+", 3 );
	Factory::newListItem( profileList, "Contrast", 4 );


	maxFPSLabel = Factory::newLabel( advanced, 20, yoffset[1] += 25, 100, 20, "Max FPS:" );
	maxFPSLabel->setAlign( ALIGN_MIDDLE_LEFT );
	maxFPS = Factory::newTextBox( advanced, 125, yoffset[1], 65, 20, 8 );

	simpleItems = Factory::newCheckBox( advanced, 20, yoffset[1] += 25, 155, 20, "Simple Items" );
	simpleItems->setAlign( ALIGN_MIDDLE_LEFT );

	wallMarks = Factory::newCheckBox( advanced, 20, yoffset[1] += 25, 155, 20, "Marks on walls" );
	wallMarks->setAlign( ALIGN_MIDDLE_LEFT );

	bloom = Factory::newCheckBox( advanced, 20, yoffset[1] += 25, 155, 20, "Light bloom" );
	bloom->setAlign( ALIGN_MIDDLE_LEFT );

	selfShadow = Factory::newCheckBox( advanced, 20, yoffset[1] += 25, 155, 20, "Show self shadow" );
	selfShadow->setAlign( ALIGN_MIDDLE_LEFT );

	reliefMapping = Factory::newCheckBox( advanced, 20, yoffset[1] += 25, 155, 20, "Relief mapping" );
	reliefMapping->setAlign( ALIGN_MIDDLE_LEFT );

	portalMaps = Factory::newCheckBox( advanced, 20, yoffset[1] += 25, 155, 20, "Portalmaps" );
	portalMaps->setAlign( ALIGN_MIDDLE_LEFT );

	specularLighting = Factory::newCheckBox( advanced, 20, yoffset[1] += 25, 155, 20, "Specular lighting" );
	specularLighting->setAlign( ALIGN_MIDDLE_LEFT );

	fastSky = Factory::newCheckBox( advanced, 20, yoffset[1] += 25, 155, 20, "Fast sky" );
	fastSky->setAlign( ALIGN_MIDDLE_LEFT );
}

void SetupGraphicsMenu::picmipSliderHandler(BaseObject* target, int v1, int v2)
{
	//
}

void SetupGraphicsMenu::UpdateGraphicsFields( void )
{
	resolutionList->selectItem( Trap::Cvar_Value( "r_mode" ) );
	fullscreen->setPressed( ( Trap::Cvar_Value( "vid_fullscreen" ) ? true : false ) );
	vsync->setPressed( ( Trap::Cvar_Value( "r_swapinterval" ) ? true : false ) );	
	maxFPS->setText( Trap::Cvar_String( "cl_maxfps" ) );
	simpleItems->setPressed( ( Trap::Cvar_Value( "cg_simpleItems" ) ? true : false ) );
	wallMarks->setPressed( ( Trap::Cvar_Value( "cg_decals" ) ? true : false ) );
	bloom->setPressed( ( Trap::Cvar_Value( "r_bloom" ) ? true : false ) );
	selfShadow->setPressed( ( Trap::Cvar_Value( "r_shadows_self_shadow" ) ? true : false ) );
	reliefMapping->setPressed( ( Trap::Cvar_Value( "r_offsetmapping_reliefmapping" ) ? true : false ) );
	portalMaps->setPressed( ( Trap::Cvar_Value( "r_portalMaps" ) ? true : false ) );
	fastSky->setPressed( ( Trap::Cvar_Value( "r_fastSky" ) ? true : false ) );
	brightness->setCurValue( (int)round(Trap::Cvar_Value( "r_gamma" ) * 5.0f) );
	texQuality->setCurValue( 16 - Trap::Cvar_Value( "r_picmip" ) ); 
	skyQuality->setCurValue( ( Trap::Cvar_Int( "r_fastsky" ) ? 0 : 4 - Trap::Cvar_Value( "r_skymip" ) ) );
	geometryLOD->setCurValue( 4 - max( Trap::Cvar_Value( "r_lodbias" ), Trap::Cvar_Value( "r_subdivisions" ) ) );
}

void SetupGraphicsMenu::UpdateGraphicsConfig()
{
	bool needRestart = false;
	int currentMode = Trap::Cvar_Int( "r_mode" );
	bool fullScreen = ( Trap::Cvar_Int( "vid_fullscreen" ) ? true : false );
	bool vsyncEnabled = ( Trap::Cvar_Int( "r_swapinterval" ) ? true : false );
	bool bloomEnabled = ( Trap::Cvar_Int( "r_bloom" ) ? true : false );	
	bool fastskyEnabled = ( Trap::Cvar_Int( "r_fastsky" ) ? true : false );

	int picMip = Trap::Cvar_Int( "r_picmip" );
	int skyMip = Trap::Cvar_Int( "r_skymip" );

	int selectedMode = resolutionList->getItemPosition( resolutionList->getSelectedItem( ) );
	bool selectedFullscreen = fullscreen->isPressed( );
	bool selectedVsync = vsync->isPressed( ) ;
	bool selectedBloom = bloom->isPressed( );
	bool selectedFastSky = fastSky->isPressed( );
	int selectedPicmip = ( 16 - texQuality->getCurValue() );
	int selectedSkymip = ( 4 - skyQuality->getCurValue() );

	if( currentMode != selectedMode || fullScreen != selectedFullscreen || picMip != selectedPicmip || 
		bloomEnabled != selectedBloom || vsyncEnabled != selectedVsync || 
		( skyMip != selectedSkymip && ( !fastskyEnabled && !selectedFastSky ) ) )
		needRestart = true;
	
	Trap::Cvar_SetValue( "r_mode", selectedMode );
	Trap::Cvar_SetValue( "vid_fullscreen", selectedFullscreen );
	Trap::Cvar_SetValue( "r_swapinterval", selectedVsync );
	Trap::Cvar_SetValue( "r_gamma", float( brightness->getCurValue()) / 5.0f );
	Trap::Cvar_SetValue( "r_picmip", ( texQuality->getCurValue() ) ); 
	Trap::Cvar_SetValue( "cl_maxfps", atoi( maxFPS->getText( ) ) );
	Trap::Cvar_SetValue( "cg_simpleItems", simpleItems->isPressed( ) );
	Trap::Cvar_SetValue( "cg_decals", wallMarks->isPressed( ) );
	Trap::Cvar_SetValue( "r_bloom", selectedBloom );
	Trap::Cvar_SetValue( "r_shadows_self_shadow", selfShadow->isPressed( ) );
	Trap::Cvar_SetValue( "r_picmip", selectedPicmip ); 
	Trap::Cvar_SetValue( "r_fastsky", selectedFastSky );

	if( !fastskyEnabled && !selectedFastSky )
		Trap::Cvar_SetValue( "r_skymip", selectedSkymip );
	else 
		Trap::Cvar_SetValue( "r_skymip", 0 );

	if( needRestart )
		Trap::Cmd_ExecuteText( EXEC_APPEND, "vid_restart\n" );
}

void SetupGraphicsMenu::Show( void )
{
	// todo
	if( !setupmenu->isActive( ) )
		setupmenu->Show( );
	panel->setVisible( true );
	setupmenu->currentSubPanel = panel;
	UpdateGraphicsFields( );
}

void SetupGraphicsMenu::Hide( void )
{
	UpdateGraphicsConfig( );
	setupmenu->currentSubPanel = setupmenu->subpanel;
	panel->setVisible( false );
}

void SetupGraphicsMenu::tabsHandler( BaseObject* target, bool )
{	
	if( target == setupgraphics->tabs[0] )
	{
		setupgraphics->tabs[0]->setPressed(true);
		setupgraphics->tabs[1]->setPressed(false);

		setupgraphics->basic->setVisible( true );
		setupgraphics->advanced->setVisible( false );
	}
	else
	{
		setupgraphics->tabs[0]->setPressed(false);
		setupgraphics->tabs[1]->setPressed(true);

		setupgraphics->basic->setVisible( false );
		setupgraphics->advanced->setVisible( true );
	}

}