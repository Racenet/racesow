/*
 * UI_EventListener.cpp
 *
 *  Created on: 27.6.2011
 *      Author: hc
 */

#include "ui_precompiled.h"
#include "kernel/ui_common.h"
#include "kernel/ui_main.h"
#include "kernel/ui_eventlistener.h"
#include <Rocket/Core/Input.h>

namespace WSWUI
{

static const String SOUND_HOVER = "sound-hover";
static const String SOUND_CLICK = "sound-click";

using namespace Rocket::Core;

BaseEventListener::BaseEventListener()
{
	// TODO Auto-generated constructor stub

}

BaseEventListener::~BaseEventListener()
{
	// TODO Auto-generated destructor stub
}

void BaseEventListener::ProcessEvent( Event &event )
{
	Element *target = event.GetTargetElement();

	if( event.GetPhase() != Rocket::Core::Event::PHASE_TARGET ) {
		return;
	}

	/* ch : CSS sound properties are handled here */
	if( event.GetType() == "keydown" ) {
	}
	else if( event.GetType() == "mouseover" ) {
		StartTargetPropertySound( target, SOUND_HOVER );
	}
	else if( event.GetType() == "click" ) {
		StartTargetPropertySound( target, SOUND_CLICK );
	}
}

void BaseEventListener::StartTargetPropertySound( Element *target, const String &property )
{
	String sound = target->GetProperty<String>( property );
	if( sound.Empty() ) {
		return;
	}

	// check relative url, and add documents path
	if( sound[0] != '/' ) {
		ElementDocument *doc = target->GetOwnerDocument();
		if( doc ) {
			URL documentURL( doc->GetSourceURL() );

			URL soundURL( sound );
			soundURL.PrefixPath( documentURL.GetPath() );

			sound = soundURL.GetPathedFileName();
		}
	}

	trap::S_StartLocalSound( sound.CString()+1 );
}

Rocket::Core::EventListener * GetBaseEventListener( void )
{
	return __new__(BaseEventListener)();
}

//===================================================

class UI_MainListener : public EventListener
{
public:
	virtual void ProcessEvent( Event &event )
	{
		if( UI_Main::Get()->debugOn() ) {
			Com_Printf( "eventlistener: Event %s, target %s, phase %i\n",
				event.GetType().CString(),
				event.GetTargetElement()->GetTagName().CString(),
				event.GetPhase() );
		}

		if( event.GetType() == "keydown" && 
			( event.GetPhase() == Rocket::Core::Event::PHASE_TARGET || event.GetPhase() == Rocket::Core::Event::PHASE_BUBBLE ) )
		{
			int key = event.GetParameter<int>( "key_identifier", 0 );

			if( key == Input::KI_ESCAPE ) {
				NavigationStack *stack = UI_Main::Get()->getNavigator();

				if( stack->isTopModal() ) {
					// pop the top document
					stack->popDocument();
				}
				else {
					// hide all documents
					UI_Main::Get()->showUI( false );
				}
				event.StopPropagation();
			}
			else if( key == Rocket::Core::Input::KI_BROWSER_BACK || key == Rocket::Core::Input::KI_BACK ) {
				NavigationStack *stack = UI_Main::Get()->getNavigator();

				// act as history.back()
				if( stack && stack->hasAtLeastTwoDocuments() ) {
					stack->popDocument();
					event.StopPropagation();
				}
			}
		}
	}
};

UI_MainListener ui_mainlistener;

EventListener * UI_GetMainListener( void )
{
	return &ui_mainlistener;
}

}
