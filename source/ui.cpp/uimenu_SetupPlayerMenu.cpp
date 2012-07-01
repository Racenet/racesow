/*
Copyright (C) 2007 Benjamin Litzelmann

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


#include "uiwsw_PlayerModels.h"
#include "uicore_Global.h"
#include "uimenu_SetupPlayerMenu.h"
#include "uimenu_SetupMenu.h"
#include "uiwsw_Export.h"
#include "uiwsw_SysCalls.h"
#include "uiwsw_Utils.h"

using namespace UIWsw;
using namespace UICore;
using namespace UIMenu;

extern SetupMenu *setupmenu;

SetupPlayerMenu *setupplayer = NULL;

// TODO : add crosshair sizes in this menu

static const char *teamnames[5] = {
	"PLAYERS",
	"ALPHA",
	"BETA"
};

void UIMenu::M_Menu_SetupPlayer_f( void )
{
	if ( !setupmenu )
		setupmenu = new SetupMenu();

	if ( !setupplayer )
		setupplayer = new SetupPlayerMenu();

	setupplayer->Show();
}

void SetupPlayerMenu::tabsHandler( BaseObject* target, bool )
{	
	int tab = 0;
	for ( int i = 0 ; i < 3 ; ++i )
	{
		if ( setupplayer->tabs[i] != target )
			setupplayer->tabs[i]->setPressed( false );
		else
		{
			setupplayer->tabs[i]->setPressed( true );
			tab = i;
		}
	}
	setupplayer->player->setVisible( false );
	setupplayer->hud->setVisible( false );
	setupplayer->team->setVisible( false );

	if ( tab == 1 )
		setupplayer->hud->setVisible( true );
	else if ( tab == 2 )
		setupplayer->team->setVisible( true );
	else
		setupplayer->player->setVisible( true );
}

void SetupPlayerMenu::modelSelectedHandler( ListItem *target, int, bool isSelected )
{
	setupplayer->skin->clear();

	if ( !isSelected )
		return;

	const playermodelinfo_t* pmodel = PlayerModels::getModel( target->getCaption().c_str() );

	if ( !pmodel )
		return;

	for ( int i = 0 ; i < pmodel->nskins ; ++i )
	{
		Factory::newListItem( setupplayer->skin, pmodel->skinnames[i] );
		// select default, or current skin
		if ( i == 0 || !Q_stricmp( pmodel->skinnames[i], Trap::Cvar_String( "skin" ) ) )
			setupplayer->skin->selectItem( i );
	}
}

void SetupPlayerMenu::handednessHandler( BaseObject* target, bool )
{
	for ( int i = 0 ; i < 3 ; ++i )
		if ( setupplayer->handedness[i] != target )
			setupplayer->handedness[i]->setPressed( false );
		else
			setupplayer->handedness[i]->setPressed( true );
}

void SetupPlayerMenu::playerDisplayHandler( BaseObject *target, unsigned int, const Rect *parentPos, const Rect *, bool )
{
	byte_vec4_t clr = { 255, 255, 255, 255 };
	ListItem *selItem;
	Rect r;
	ListBox *modellist = NULL, *skinlist = NULL;

	// This handler is used for each panel displaying model, so we need to check which
	// panel it is in order to get the model/skin/color for the corresponding lists
	if ( target == setupplayer->modeldisplay )
	{
		modellist = setupplayer->model;
		skinlist = setupplayer->skin;
		for ( int i = 0 ; i < 3 ; ++i )
			clr[i] = setupplayer->rgb[i]->getCurValue();
	}
	else
	{
		for ( int i = 0 ; i < 5 ; ++i )
		{
			if ( target == setupplayer->teampreview[i] )
			{
				modellist = setupplayer->teammodel[i];
				skinlist = setupplayer->teamskin[i];
				for ( int j = 0 ; j < 3 ; ++j )
					clr[j] = setupplayer->teamcolor[j][i]->getCurValue();
			}
		}
	}

	target->getPosition( r.x, r.y );
	target->getSize( r.w, r.h );
	r.x = (r.x + parentPos->x) * Local::getScaleX();
	r.y = (r.y + parentPos->y) * Local::getScaleY();
	r.w *= Local::getScaleX();
	r.h *= Local::getScaleY();

	selItem = modellist->getSelectedItem();
	if ( selItem )
	{

		if ( skinlist->getSelectedItem() )
			PlayerModels::DrawPlayerModel( modellist->getSelectedItem()->getCaption().c_str(),
					skinlist->getSelectedItem()->getCaption().c_str(), clr, (int)r.x, (int)r.y, (int)r.w, (int)r.h, 1, 1 );
		else
			PlayerModels::DrawPlayerModel( modellist->getSelectedItem()->getCaption().c_str(),
					"default", clr, (int)r.x, (int)r.y, (int)r.w, (int)r.h, 1, 1 );
	}
	else if ( target != setupplayer->modeldisplay )
	{ // for team color only, we need to display a colored rect if there's no forced model
		if ( target != setupplayer->teampreview[0] || setupplayer->forcecolor[0]->isPressed() )
		{ // also not for players team when there's no forced color
			vec4_t color;
			Vector4Set( color, float(clr[0])/255.0f, float(clr[1])/255.0f, float(clr[2])/255.0f, 1.0f );
			Trap::R_DrawStretchPic( (int)(r.x + r.w / 4), (int)(r.y + 3*r.h / 8), (int)(r.w / 2), (int)(r.h / 4), 0, 0, 1, 1, color, Local::getWhiteShader() );
		}
	}
}

void SetupPlayerMenu::teamTabsHandler( BaseObject* target, bool )
{
	for ( int i = 0 ; i < 3 ; ++i )
	{
		if ( setupplayer->teamselection[i] != target )
		{
			setupplayer->teamselection[i]->setPressed( false );
			setupplayer->teamsetup[i]->setVisible( false );
		}
		else
		{
			setupplayer->teamselection[i]->setPressed( true );
			setupplayer->teamsetup[i]->setVisible( true );
		}
	}
}

void SetupPlayerMenu::forceModelHandler( BaseObject *target, bool newVal )
{
	int team;
	for ( team = 0 ; team < 5 ; ++team )
		if ( target == setupplayer->forcemodel[team] )
			break;

	if ( team == 5 ) // Should never happen
		return;

	if ( newVal )
	{
		setupplayer->teammodel[team]->setEnabled( true );
		setupplayer->teamskin[team]->setEnabled( true );
		setupplayer->teammodel[team]->selectItem( 0 ); // We select first model
	}
	else
	{
		setupplayer->teammodel[team]->setEnabled( false );
		setupplayer->teamskin[team]->setEnabled( false );
		setupplayer->teammodel[team]->clearSelection();
	}
}

void SetupPlayerMenu::teamModelSelectedHandler( ListItem *target, int, bool isSelected )
{
	int team;
	char sz[128];

	if ( !isSelected )
		return;

	for ( team = 0 ; team < 5 ; ++team )
		if ( target->getParent() == setupplayer->teammodel[team] )
			break;

	if ( team == 5 ) // Should never happen
		return;

	setupplayer->teamskin[team]->clear();

	const playermodelinfo_t* pmodel = PlayerModels::getModel( target->getCaption().c_str() );

	if ( !pmodel )
		return;

	sprintf( sz, "cg_team%sskin", teamnames[team] );
	for ( int i = 0 ; i < pmodel->nskins ; ++i )
	{
		Factory::newListItem( setupplayer->teamskin[team], pmodel->skinnames[i] );
		// select default, or current skin
		if ( i == 0 || !Q_stricmp( pmodel->skinnames[i], Trap::Cvar_String( sz ) ) )
			setupplayer->teamskin[team]->selectItem( i );
	}
}

void SetupPlayerMenu::forceColorHandler( BaseObject *target, bool newVal )
{
	int team;
	int rgbcolor = 0;

	for ( team = 0 ; team < 5 ; ++team )
		if ( target == setupplayer->forcecolor[team] )
			break;

	if ( team == 5 ) // Should never happen
		return;

	if ( newVal )
	{
		for ( int i = 0 ; i < 3 ; ++i )
			setupplayer->teamcolor[i][team]->setEnabled( true );

		cvar_t *pcolor = NULL;
		// stupid default value...
		if ( team == 0 )
			pcolor = Trap::Cvar_Get( va( "cg_team%scolor", teamnames[team]), "", CVAR_ARCHIVE );
		else if ( team == 1 )
			pcolor = Trap::Cvar_Get( va( "cg_team%scolor", teamnames[team]), DEFAULT_TEAMALPHA_COLOR, CVAR_ARCHIVE );
		else if ( team == 2 )
			pcolor = Trap::Cvar_Get( va( "cg_team%scolor", teamnames[team]), DEFAULT_TEAMBETA_COLOR, CVAR_ARCHIVE );

		rgbcolor = COM_ReadColorRGBString( pcolor->string );
		if( rgbcolor == -1 )
			rgbcolor = COM_ReadColorRGBString( pcolor->dvalue );
		if( rgbcolor != -1 )
		{
			setupplayer->teamcolor[0][team]->setCurValue( COLOR_R(rgbcolor) );
			setupplayer->teamcolor[1][team]->setCurValue( COLOR_G(rgbcolor) );
			setupplayer->teamcolor[2][team]->setCurValue( COLOR_B(rgbcolor) );
		}
	}
	else
	{
		for ( int i = 0 ; i < 3 ; ++i )
			setupplayer->teamcolor[i][team]->setEnabled( false );

		// This is just for visual issue (seeing the actual color on disabled
		// sliders), cause the cvar for colors will be set to "" when default
		// colors are used.
		if ( team == 0 )
			rgbcolor = COM_ReadColorRGBString( "255 255 255" );
		else if ( team == 1 )
			rgbcolor = COM_ReadColorRGBString( DEFAULT_TEAMALPHA_COLOR );
		else if ( team == 2 )
			rgbcolor = COM_ReadColorRGBString( DEFAULT_TEAMBETA_COLOR );

		setupplayer->teamcolor[0][team]->setCurValue( COLOR_R(rgbcolor) );
		setupplayer->teamcolor[1][team]->setCurValue( COLOR_G(rgbcolor) );
		setupplayer->teamcolor[2][team]->setCurValue( COLOR_B(rgbcolor) );
	}
}

ALLOCATOR_DEFINITION(SetupPlayerMenu)
DELETER_DEFINITION(SetupPlayerMenu)

SetupPlayerMenu::SetupPlayerMenu()
{
	int i, j;
	int yoffset = 0, ysuboffset = 0;
	Color transparent( 0, 0, 0, 0 );

	panel = Factory::newPanel( setupmenu->panel, 200, 20, 540, 520 );

	tabs[0] = Factory::newSwitchButton( panel, 0, 0, 180, 30, "Player" );
	tabs[1] = Factory::newSwitchButton( panel, 180, 0, 180, 30, "Hud" );
	tabs[2] = Factory::newSwitchButton( panel, 360, 0, 180, 30, "Team" );
	player = Factory::newPanel( panel, 0, 30, 540, 490 );
	hud = Factory::newPanel( panel, 0, 30, 540, 490 );
	team = Factory::newPanel( panel, 0, 30, 540, 490 );

	player->setBackColor( transparent );
	hud->setBackColor( transparent );
	team->setBackColor( transparent );
	for ( i = 0 ; i < 3 ; ++i )
		tabs[i]->setSwitchHandler( tabsHandler );

	// Player tab
	lplrname = Factory::newLabel( player, 20, yoffset += 20, 120, 20, "Player name:" );
	plrname = Factory::newTextBox( player, 160, yoffset, 200, 20, 20 );

	lmodel = Factory::newLabel( player, 20, yoffset += 40, 120, 20, "Player model:" );
	model = Factory::newListBox( player, 160, yoffset, 180, 120, false );
	model->setItemSelectedHandler( modelSelectedHandler );
	modeldisplay = Factory::newPanel( player, 360, yoffset, 160, 200 );
	modeldisplay->setAfterDrawHandler( playerDisplayHandler );
	lskin = Factory::newLabel( player, 20, yoffset += 140, 120, 20, "Skin:" );
	skin = Factory::newListBox( player, 160, yoffset, 180, 60, false );
	for ( i = 0, yoffset += 45 ; i < 3 ; ++i )
	{
		lrgb[i] = Factory::newLabel( player, 20, yoffset += 35, 120, 20 );
		lrgb[i]->setAlign( ALIGN_MIDDLE_RIGHT );
		rgb[i] = Factory::newSlider( player, 160, yoffset, 120, 15, true );
		rgb[i]->setBoundValues( 0, 255 );
		rgb[i]->setStepValues( 8, 32 );
	}
	lhandedness = Factory::newLabel( player, 20, yoffset += 35, 120, 20, "Handedness:" );
	handedness[0] = Factory::newSwitchButton( player, 160, yoffset, 80, 20, "Left" );
	handedness[1] = Factory::newSwitchButton( player, 260, yoffset, 80, 20, "Middle" );
	handedness[2] = Factory::newSwitchButton( player, 360, yoffset, 80, 20, "Right" );
	for ( i = 0 ; i < 3 ; ++i )
		handedness[i]->setSwitchHandler( handednessHandler );

	lfov = Factory::newLabel( player, 20, yoffset += 40, 120, 20, "FOV:" );
	fov = Factory::newTextBox( player, 160, yoffset, 120, 20, 4 );

	lrgb[0]->setCaption( "Red:" );
	lrgb[1]->setCaption( "Green:" );
	lrgb[2]->setCaption( "Blue:" );
	lplrname->setAlign( ALIGN_MIDDLE_RIGHT );
	lmodel->setAlign( ALIGN_MIDDLE_RIGHT );
	lskin->setAlign( ALIGN_MIDDLE_RIGHT );
	lhandedness->setAlign( ALIGN_MIDDLE_RIGHT );
	lfov->setAlign( ALIGN_MIDDLE_RIGHT );

	// Hud tab
	yoffset = 0;

	lclienthud = Factory::newLabel( hud, 20, yoffset += 20, 120, 20, "Client HUD:" );
	clienthud = Factory::newListBox( hud, 160, yoffset, 210, 105, false );

	lwcrosshair = Factory::newLabel( hud, 20, yoffset += 120, 120, 20, "Weak crosshair:" );
	wcrosshair = Factory::newListBox( hud, 160, yoffset, 210, 50, true );
	wcrosshair->setItemSize( 50 - 15 ); // listheight - sliderheight => square items
	lscrosshair = Factory::newLabel( hud, 20, yoffset += 70, 120, 20, "Strong crosshair:" );
	scrosshair = Factory::newListBox( hud, 160, yoffset, 210, 50, true );
	scrosshair->setItemSize( 50 - 15 ); // listheight - sliderheight => square items
	weaponlist = Factory::newCheckBox( hud, 20, yoffset += 70, "Show weaponlist:", 160 );
	showfps = Factory::newCheckBox( hud, 200, yoffset, "Show FPS:", 160 );
	showspeed = Factory::newCheckBox( hud, 20, yoffset += 40, "Show speed meter:", 160 );
	showhelp = Factory::newCheckBox( hud, 200, yoffset, "Show help msg:", 160 );

	lautoaction = Factory::newLabel( hud, 20, yoffset += 40, 120, 20, "Auto actions:" );
	autoaction = Factory::newListBox( hud, 160, yoffset, 160, 60, false );
	autoaction->setMultipleSelection( true );
	Factory::newListItem( autoaction, "Demo" );
	Factory::newListItem( autoaction, "Screenshot" );
	Factory::newListItem( autoaction, "Stats" );
	Factory::newListItem( autoaction, "Spectator" );

	lclienthud->setAlign( ALIGN_MIDDLE_RIGHT );
	lwcrosshair->setAlign( ALIGN_MIDDLE_RIGHT );
	lscrosshair->setAlign( ALIGN_MIDDLE_RIGHT );
	weaponlist->setAlign( ALIGN_MIDDLE_RIGHT );
	showfps->setAlign( ALIGN_MIDDLE_RIGHT );
	showspeed->setAlign( ALIGN_MIDDLE_RIGHT );
	showhelp->setAlign( ALIGN_MIDDLE_RIGHT );
	lautoaction->setAlign( ALIGN_MIDDLE_RIGHT );

	// Team Tab
	for ( i = 0, yoffset = 20 ; i < 3 ; ++i )
	{
		ysuboffset = 0;
		teamselection[i] = Factory::newSwitchButton( team, 20 + 100 * i, yoffset, 100, 30, teamnames[i] );
		teamselection[i]->setSwitchHandler( teamTabsHandler );
		teamsetup[i] = Factory::newPanel( team, 20, yoffset + 30, 500, 350 );
		forcemodel[i] = Factory::newCheckBox( teamsetup[i], 20, ysuboffset += 20, "Force team model", 180 );
		forcemodel[i]->setSwitchHandler( forceModelHandler );
		lteammodel[i] = Factory::newLabel( teamsetup[i], 20, ysuboffset += 30, 180, 20, "Team model:" );
		teammodel[i] = Factory::newListBox( teamsetup[i], 20, ysuboffset += 20, 180, 120, false );
		teammodel[i]->setItemSelectedHandler( teamModelSelectedHandler );
		teammodel[i]->setEnabled( false );
		lteamskin[i] = Factory::newLabel( teamsetup[i], 20, ysuboffset += 150, 180, 20, "Team skin:" );
		teamskin[i] = Factory::newListBox( teamsetup[i], 20, ysuboffset += 20, 180, 60, false );
		teamskin[i]->setEnabled( false );
		teampreview[i] = Factory::newPanel( teamsetup[i], 250, 20, 170, 200 );
		teampreview[i]->setAfterDrawHandler( playerDisplayHandler );
		for ( j = 0 ; j < 3 ; ++j )
		{
			lteamcolor[j][i] = Factory::newLabel( teamsetup[i], 220, ysuboffset + 35 * j, 60, 20 );
			lteamcolor[j][i]->setAlign( ALIGN_MIDDLE_RIGHT );
			teamcolor[j][i] = Factory::newSlider( teamsetup[i], 300, ysuboffset + 35 * j, 120, 15, true );
			teamcolor[j][i]->setBoundValues( 0, 255 );
			teamcolor[j][i]->setStepValues( 8, 32 );
		}
		lteamcolor[0][i]->setCaption( "Red:" );
		lteamcolor[1][i]->setCaption( "Green:" );
		lteamcolor[2][i]->setCaption( "Blue:" );
		forcecolor[i] = Factory::newCheckBox( teamsetup[i], 20, ysuboffset += 80, "Force team color", 180 );
		forcecolor[i]->setAlign( ALIGN_MIDDLE_RIGHT );
		forcecolor[i]->setSwitchHandler( forceColorHandler );
	}
	playerasalpha = Factory::newCheckBox( team, 20, yoffset += 400, "Player team as ALPHA", 300 );
	teamlessasbeta = Factory::newCheckBox( team, 20, yoffset += 30, "Enemy teams/Teamless players as BETA", 300 );
}

void SetupPlayerMenu::UpdateHUDList( void )
{
	int		i, k;
	char	listbuf[1024], scratch[MAX_CONFIGSTRING_CHARS];
	char	*ptr;
	int		numHUDs;
	ListItem *item;

	clienthud->clear();

	// get the list of skins
	numHUDs = Trap::FS_GetFileList( "huds", ".hud", NULL, 0, 0, 0 );
	if( !numHUDs ) {
		Factory::newListItem( clienthud, "default" );
		return;
	}

	i = 0;
	do {
		if( (k = Trap::FS_GetFileList( "huds", ".hud", listbuf, sizeof( listbuf ), i, numHUDs )) == 0 )
		{
			i++; // can happen if the filename is too long to fit into the buffer or we're done
			continue;
		}
		i += k;

		for( ptr = listbuf; k > 0; k--, ptr += strlen( ptr )+1 )
		{
			Q_strncpyz( scratch, ptr, sizeof( scratch ) );
			COM_StripExtension( scratch );
			item = Factory::newListItem( clienthud, scratch );
			if ( !Q_stricmp( Trap::Cvar_String( "cg_clienthud" ), scratch ) )
				clienthud->selectItem( item );
		}
	} while( i < numHUDs );
}

void SetupPlayerMenu::UpdateCrosshairLists( void )
{
	ListItem *item;
	Color back( blue2 );
	Color high( blue1 );
	Color press( orange );
	back.setAlpha( 1 );
	high.setAlpha( 1 );
	press.setAlpha( 1 );

	wcrosshair->clear();
	scrosshair->clear();

	for ( int i = 0 ; i < NUM_CROSSHAIRS ; ++i )
	{
		item = Factory::newListItem( wcrosshair, "" );
		item->setBackgroundImage( Trap::R_RegisterPic( va("gfx/hud/crosshair%i", i) ) );
		item->setHighlightImage( Trap::R_RegisterPic( va("gfx/hud/crosshair%i", i) ) );
		item->setPressedImage( Trap::R_RegisterPic( va("gfx/hud/crosshair%i", i) ) );
		item->setBackColor( back );
		item->setHighlightColor( high );
		item->setPressedColor( press );
		if ( i == Trap::Cvar_Value( "cg_crosshair" ) )
			wcrosshair->selectItem( item );

		item = Factory::newListItem( scrosshair, "" );
		item->setBackgroundImage( Trap::R_RegisterPic( va("gfx/hud/crosshair%i", i) ) );
		item->setHighlightImage( Trap::R_RegisterPic( va("gfx/hud/crosshair%i", i) ) );
		item->setPressedImage( Trap::R_RegisterPic( va("gfx/hud/crosshair%i", i) ) );
		item->setBackColor( back );
		item->setHighlightColor( high );
		item->setPressedColor( press );
		if ( i == Trap::Cvar_Value( "cg_crosshair_strong" ) )
			scrosshair->selectItem( item );
	}
}

void SetupPlayerMenu::UpdatePlayerFields( void )
{
	const playermodelinfo_t *plrmodel;
	int value;
	ListItem *item;

	// Player
	plrname->setText( Trap::Cvar_String( "name" ) );

	model->clear();
	plrmodel = PlayerModels::getModelList();
	while ( plrmodel )
	{
		item = Factory::newListItem( model, plrmodel->directory );
		if ( !Q_stricmp( plrmodel->directory, Trap::Cvar_String( "model" ) ) )
			model->selectItem( item );
			
		plrmodel = plrmodel->next;
	}

	int rgbcolor;
	cvar_t *pcolor = Trap::Cvar_Get( "color", "", CVAR_ARCHIVE|CVAR_USERINFO );
	rgbcolor = COM_ReadColorRGBString( pcolor->string );
	if( rgbcolor == -1 )
		rgbcolor = COM_ReadColorRGBString( pcolor->dvalue );
	if( rgbcolor != -1 )
	{
		rgb[0]->setCurValue( COLOR_R(rgbcolor) );
		rgb[1]->setCurValue( COLOR_G(rgbcolor) );
		rgb[2]->setCurValue( COLOR_B(rgbcolor) );
	}
	else
	{
		rgb[0]->setCurValue( 255 );
		rgb[1]->setCurValue( 255 );
		rgb[2]->setCurValue( 255 );
	}	
	pcolor->modified = qfalse;

	value = bound( int( Trap::Cvar_Value( "hand" ) ), 0, 2 );
	setupplayer->handednessHandler( handedness[value], true );
	fov->setText( Trap::Cvar_String( "fov" ) );

	// Hud
	UpdateHUDList();
	UpdateCrosshairLists();

	weaponlist->setPressed( Trap::Cvar_Value( "cg_weaponlist" ) != 0 );
	showfps->setPressed( Trap::Cvar_Value( "cg_showfps" ) != 0 );
	showspeed->setPressed( Trap::Cvar_Value( "cg_showspeed" ) != 0 );
	showhelp->setPressed( Trap::Cvar_Value( "cg_showhelp" ) != 0 );

	if ( Trap::Cvar_Value( "cg_autoaction_demo" ) )
		autoaction->selectItem( 0 );
	if ( Trap::Cvar_Value( "cg_autoaction_screenshot" ) )
		autoaction->selectItem( 1 );
	if ( Trap::Cvar_Value( "cg_autoaction_stats" ) )
		autoaction->selectItem( 2 );
	if ( Trap::Cvar_Value( "cg_autoaction_spectator" ) )
		autoaction->selectItem( 3 );

	// Team
	for ( int i = 0 ; i < 3 ; ++i )
	{
		forcemodel[i]->setPressed( Trap::Cvar_String( va("cg_team%smodel", teamnames[i] ) )[0] != '\0' );
		forceModelHandler( forcemodel[i], forcemodel[i]->isPressed() );
		teammodel[i]->clear();
		plrmodel = PlayerModels::getModelList();
		while ( plrmodel )
		{
			item = Factory::newListItem( teammodel[i], plrmodel->directory );
			if ( !Q_stricmp( plrmodel->directory, Trap::Cvar_String( va("cg_team%smodel", teamnames[i]) ) ) )
				teammodel[i]->selectItem( item );
				
			plrmodel = plrmodel->next;
		}
		forcecolor[i]->setPressed( Trap::Cvar_String( va("cg_team%scolor", teamnames[i] ) )[0] != '\0' );
		forceColorHandler( forcecolor[i], forcecolor[i]->isPressed() );
	}
	playerasalpha->setPressed( Trap::Cvar_Int( "cg_forceMyTeamAlpha" ) ? qtrue : qfalse );
	teamlessasbeta->setPressed( Trap::Cvar_Int( "cg_forceTeamPlayersTeamBeta" ) ? qtrue : qfalse );
}

void SetupPlayerMenu::UpdatePlayerConfig( void )
{
	SwitchButton *listItem;
	char sz[128], sz2[128];
	float fVal;

	// Player
	Trap::Cvar_Set( "name", plrname->getText() );

	listItem = model->getSelectedItem();
	if ( listItem ) // should always be true
	{
		Trap::Cvar_Set( "model", listItem->getCaption().c_str() );
		if ( skin->getSelectedItem() ) // should always be true
			Trap::Cvar_Set( "skin", skin->getSelectedItem()->getCaption().c_str() );
	}
	sprintf( sz, "%d %d %d", rgb[0]->getCurValue(), rgb[1]->getCurValue(), rgb[2]->getCurValue() );
	Trap::Cvar_Set( "color", sz );

	for ( int i = 0 ; i < 3 ; ++i )
		if ( handedness[i]->isPressed() )
		{
			Trap::Cvar_SetValue( "hand", float(i) );
			break;
		}

	fVal = float( atoi( fov->getText() ) );
	if ( fVal <= 0 || fVal >= 180 )
		fVal = 100;

	Trap::Cvar_SetValue( "fov", fVal );

	// Hud
	if ( clienthud->getSelectedItem() ) // should always be true
		Trap::Cvar_Set( "cg_clientHUD", clienthud->getSelectedItem()->getCaption().c_str() );

	if ( wcrosshair->getSelectedItem() ) // should always be true
		Trap::Cvar_SetValue( "cg_crosshair", float(wcrosshair->getItemPosition(wcrosshair->getSelectedItem())) );

	if ( scrosshair->getSelectedItem() ) // should always be true
		Trap::Cvar_SetValue( "cg_crosshair_strong", float(scrosshair->getItemPosition(scrosshair->getSelectedItem())) );

	Trap::Cvar_SetValue( "cg_weaponlist", float(weaponlist->isPressed()) );
	Trap::Cvar_SetValue( "cg_showFPS", float(showfps->isPressed()) );
	Trap::Cvar_SetValue( "cg_showspeed", float(showspeed->isPressed()) );
	Trap::Cvar_SetValue( "cg_showhelp", float(showhelp->isPressed()) );

	Trap::Cvar_SetValue( "cg_autoaction_demo", float(autoaction->getItem(0)->isPressed()) );
	Trap::Cvar_SetValue( "cg_autoaction_screenshot", float(autoaction->getItem(1)->isPressed()) );
	Trap::Cvar_SetValue( "cg_autoaction_spectator", float(autoaction->getItem(2)->isPressed()) );
	Trap::Cvar_SetValue( "cg_autoaction_stats", float(autoaction->getItem(3)->isPressed()) );

	// Team
	for ( int i = 0 ; i < 3 ; ++i )
	{
		sprintf( sz, "cg_team%scolor", teamnames[i] );
		if ( i == 0 && !forcecolor[i]->isPressed() ) // players team must have a clear string to unforce color
			sz2[0] = '\0';
		else
			sprintf( sz2, "%d %d %d", teamcolor[0][i]->getCurValue(), teamcolor[1][i]->getCurValue(), teamcolor[2][i]->getCurValue() );
		Trap::Cvar_Set( sz, sz2 );

		sprintf( sz, "cg_team%smodel", teamnames[i] );
		if ( forcemodel[i]->isPressed() && teammodel[i]->getSelectedItem() ) // getSelectedItem should always be true
		{
			Trap::Cvar_Set( sz, teammodel[i]->getSelectedItem()->getCaption().c_str() );
			sprintf( sz, "cg_team%sskin", teamnames[i] );
			if ( teamskin[i]->getSelectedItem() ) // should always be true
				Trap::Cvar_Set( sz, teamskin[i]->getSelectedItem()->getCaption().c_str() );
		}
		else
		{
			Trap::Cvar_Set( sz, "" );
			sprintf( sz, "cg_team%sskin", teamnames[i] );
			Trap::Cvar_Set( sz, "" );
		}
	}
	Trap::Cvar_SetValue( "cg_forceMyTeamAlpha", float(playerasalpha->isPressed()) );
	Trap::Cvar_SetValue( "cg_forceTeamPlayersTeamBeta", float(teamlessasbeta->isPressed()) );
}

void SetupPlayerMenu::Show( void )
{
	if ( !setupmenu->isActive() )
		setupmenu->Show();

	panel->setVisible( true );
	tabsHandler( tabs[0], true );
	teamTabsHandler( teamselection[0], true );
	setupmenu->currentSubPanel = panel;
	UpdatePlayerFields();
}

void SetupPlayerMenu::Hide( void )
{
	UpdatePlayerConfig();
	setupmenu->currentSubPanel = setupmenu->subpanel;
	panel->setVisible( false );
}
