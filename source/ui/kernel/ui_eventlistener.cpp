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
#include <Rocket/Controls.h>

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
			Com_Printf( "EventListener: Event %s, target %s, phase %i\n",
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
		else if( event.GetType() == "change" && ( event.GetPhase() == Rocket::Core::Event::PHASE_BUBBLE ) ) {
			bool linebreak = event.GetParameter<bool>( "linebreak", false );
			if( linebreak ) {
				// form submission
				String inpuType;
				Element *target = event.GetTargetElement();
				Rocket::Controls::ElementFormControl *input = dynamic_cast<Rocket::Controls::ElementFormControl *>(target);

				if( event.GetPhase() != Rocket::Core::Event::PHASE_BUBBLE ) {
					return;
				}
				if( input == NULL ) {
					// not a form control
					return;
				}
				if( input->IsDisabled() ) {
					// skip disabled inputs
					return;
				}
				if( !input->IsSubmitted() ) {
					// this input field isn't submitted with the form
					return;
				}

				inpuType = input->GetAttribute<String>( "type", "text" );
				if( inpuType != "text" && inpuType != "password" ) {
					// not a text field
					return;
				}

				// find the parent form element
				Element *parent = target->GetParentNode();
				Rocket::Controls::ElementForm *form = NULL;
				while( parent ) {
					form = dynamic_cast<Rocket::Controls::ElementForm *>(parent);
					if( form != NULL ) {
						// not a form, go up the tree
						break;
					}
					parent = parent->GetParentNode();
				}

				if( form == NULL ) {
					if( UI_Main::Get()->debugOn() ) {
						Com_Printf( "EventListener: input element outside the form, what should I do?\n" );
					}
					return;
				}

				// find the submit element
				Element *submit = NULL;

				ElementList controls;
				parent->GetElementsByTagName( controls, "input" );
				for( size_t i = 0; i < controls.size(); i++ ) {
					Rocket::Controls::ElementFormControl *control =
						dynamic_cast< Rocket::Controls::ElementFormControl* >(controls[i]);

					if( control->IsDisabled() ) {
						// skip disabled inputs
						continue;
					}

					String controlType = control->GetAttribute<String>( "type", "" );
					if( controlType != "submit" ) {
						// not a text field
						continue;
					}

					submit = controls[i];
					break;
				}

				if( submit == NULL ) {
					if( UI_Main::Get()->debugOn() ) {
						Com_Printf( "EventListener: form with no submit element, what should I do?\n" );
					}
					return;
				}

				// finally submit the form
				form->Submit( submit->GetAttribute< Rocket::Core::String >("name", ""), submit->GetAttribute< Rocket::Core::String >("value", "") );
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
