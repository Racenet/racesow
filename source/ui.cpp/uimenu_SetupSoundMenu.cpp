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

#include "../gameshared/q_shared.h"
#include "../gameshared/q_math.h"
#include "../gameshared/q_cvar.h"
#include "../gameshared/q_dynvar.h"
#include "../gameshared/gs_ref.h"
#include "uicore_Global.h"
#include "uimenu_SetupSoundMenu.h"
#include "uimenu_SetupMenu.h"
#include "uiwsw_SysCalls.h"

using namespace UIWsw;
using namespace UICore;
using namespace UIMenu;

extern SetupMenu *setupmenu;

SetupSoundMenu *setupsounds = NULL;

void UIMenu::M_Menu_SetupSound_f( void )
{
	if ( !setupmenu )
		setupmenu = new SetupMenu();

	if ( !setupsounds )
		setupsounds = new SetupSoundMenu();

	setupsounds->Show();
}

ALLOCATOR_DEFINITION(SetupSoundMenu)
DELETER_DEFINITION(SetupSoundMenu)

SetupSoundMenu::SetupSoundMenu()
{
	int yoffset = 40;

	panel = Factory::newPanel( setupmenu->panel, 200, 20, 540, 520 );

	lmodule = Factory::newLabel( panel, 20, yoffset, 200, 20, "Sound module:" );
	module = Factory::newListBox( panel, 250, yoffset, 200, 60, false );

	lsound = Factory::newLabel( panel, 20, yoffset += 100, 200, 20, "Sound volume:" );
	sound = Factory::newSlider( panel, 250, yoffset, 200, 20, true );
	lmusic = Factory::newLabel( panel, 20, yoffset += 40, 200, 20, "Music volume:" );
	music = Factory::newSlider( panel, 250, yoffset, 200, 20, true );

	lplayers = Factory::newLabel( panel, 20, yoffset += 80, 200, 20, "Players volume:" );
	players = Factory::newSlider( panel, 250, yoffset, 200, 20, true );
	leffects = Factory::newLabel( panel, 20, yoffset += 40, 200, 20, "Effects volume:" );
	effects = Factory::newSlider( panel, 250, yoffset, 200, 20, true );
	lannouncer = Factory::newLabel( panel, 20, yoffset += 40, 200, 20, "Announcer volume:" );
	announcer = Factory::newSlider( panel, 250, yoffset, 200, 20, true );
	lvoicechats = Factory::newLabel( panel, 20, yoffset += 40, 200, 20, "Voice chats volume:" );
	voicechats = Factory::newSlider( panel, 250, yoffset, 200, 20, true );
	lhit = Factory::newLabel( panel, 20, yoffset += 40, 200, 20, "Hitsounds volume:" );
	hit = Factory::newSlider( panel, 250, yoffset, 200, 20, true );

	Factory::newListItem( module, "No sound" );
	Factory::newListItem( module, "Qfusion sound module" );
	Factory::newListItem( module, "OpenAL sound module" );

	lmodule->setAlign( ALIGN_MIDDLE_RIGHT );
	lsound->setAlign( ALIGN_MIDDLE_RIGHT );
	lmusic->setAlign( ALIGN_MIDDLE_RIGHT );
	lplayers->setAlign( ALIGN_MIDDLE_RIGHT );
	leffects->setAlign( ALIGN_MIDDLE_RIGHT );
	lannouncer->setAlign( ALIGN_MIDDLE_RIGHT );
	lvoicechats->setAlign( ALIGN_MIDDLE_RIGHT );
	lhit->setAlign( ALIGN_MIDDLE_RIGHT );

	sound->setBoundValues( 0, 10 );
	sound->setStepValues( 1, 2 );
	music->setBoundValues( 0, 10 );
	music->setStepValues( 1, 2 );
	players->setBoundValues( 0, 10 );
	players->setStepValues( 1, 2 );
	effects->setBoundValues( 0, 10 );
	effects->setStepValues( 1, 2 );
	announcer->setBoundValues( 0, 10 );
	announcer->setStepValues( 1, 2 );
	voicechats->setBoundValues( 0, 10 );
	voicechats->setStepValues( 1, 2 );
	hit->setBoundValues( 0, 10 );
	hit->setStepValues( 1, 2 );
}

void SetupSoundMenu::UpdateSoundFields( void )
{
	int iModule = Trap::Cvar_Int( "s_module" );
	if ( iModule < 0 || iModule > 2 )
		iModule = 1; // default module : qf

	module->selectItem( iModule );
	sound->setCurValue( (int)round(Trap::Cvar_Value( "s_volume" ) * 10.0f) );
	music->setCurValue( (int)round(Trap::Cvar_Value( "s_musicvolume" ) * 10.0f) );

	players->setCurValue( (int)round(Trap::Cvar_Value( "cg_volume_players" ) * 5.0f) );
	effects->setCurValue( (int)round(Trap::Cvar_Value( "cg_volume_effects" ) * 5.0f) );
	announcer->setCurValue( (int)round(Trap::Cvar_Value( "cg_volume_announcer" ) * 5.0f) );
	voicechats->setCurValue( (int)round(Trap::Cvar_Value( "cg_volume_voicechats" ) * 5.0f) );
	hit->setCurValue( (int)round(Trap::Cvar_Value( "cg_volume_hitsound" ) * 5.0f) );
}

void SetupSoundMenu::UpdateSoundConfig( void )
{
	bool needRestart = false;
	
	int selectedModule = module->getItemPosition( module->getSelectedItem() );

	if ( selectedModule != Trap::Cvar_Int( "s_module" ) )
	{
		Trap::Cvar_SetValue( "s_module", selectedModule );
		needRestart = true;
	}

	Trap::Cvar_SetValue( "s_volume", float(sound->getCurValue()) / 10.0f );
	Trap::Cvar_SetValue( "s_musicvolume", float(music->getCurValue()) / 10.0f );

	Trap::Cvar_SetValue( "cg_volume_players", float(players->getCurValue()) / 5.0f );
	Trap::Cvar_SetValue( "cg_volume_effects", float(effects->getCurValue()) / 5.0f );
	Trap::Cvar_SetValue( "cg_volume_announcer", float(announcer->getCurValue()) / 5.0f );
	Trap::Cvar_SetValue( "cg_volume_voicechats", float(voicechats->getCurValue()) / 5.0f );
	Trap::Cvar_SetValue( "cg_volume_hitsound", float(hit->getCurValue()) / 5.0f );

	if ( needRestart )
		Trap::Cmd_ExecuteText( EXEC_APPEND, "s_restart\n" );
}

void SetupSoundMenu::Show( void )
{
	if ( !setupmenu->isActive() )
		setupmenu->Show();

	panel->setVisible( true );
	setupmenu->currentSubPanel = panel;
	UpdateSoundFields();
}

void SetupSoundMenu::Hide( void )
{
	UpdateSoundConfig();
	setupmenu->currentSubPanel = setupmenu->subpanel;
	panel->setVisible( false );
}
