/*
Copyright (C) 2011 Victor Luchits

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

#include "ui_precompiled.h"
#include "kernel/ui_common.h"
#include "kernel/ui_main.h"

#include "as/asui.h"
#include "as/asui_local.h"
#include "as/asui_url.h"
#include "as/asui_scheduled.h"

namespace ASUI {

typedef WSWUI::RefreshState RefreshState;

class ASWindow : public EventListener
{
public:
	ASWindow( ASInterface *asmodule ) : 
		EventListener(), 
		suspendedContext( NULL ),
		attachedModalDocument( NULL ),
		modalValue( 0 ),
		backgroundTrackPlaying( false )
	{
		schedulers.clear();
	}

	~ASWindow()
	{
		// detatch itself from the possibly opened modal window
		detachAsEventListener();

		// remove schedulers for all documents we hold references to
		for( SchedulerMap::iterator it = schedulers.begin(); it != schedulers.end(); it++ ) {
			ElementDocument *doc = it->first;
			FunctionCallScheduler *scheduler = it->second;

			doc->RemoveReference();
			scheduler->shutdown();
			__delete__( scheduler );
		}
		schedulers.clear();
	}

	/// Loads document from the passed URL.
	void open( const ASURL &location )
	{
		WSWUI::NavigationStack *stack = UI_Main::Get()->getNavigator();
		if( stack == NULL ) {
			return;
		}

		stack->pushDocument( location.GetURL()->buffer );
	}

	/// Loads modal document from the URL.
	/// FIXME: move to window.
	void modal( const ASURL &location, int defaultCode = -1 )
	{
		WSWUI::NavigationStack *stack = UI_Main::Get()->getNavigator();

		// default return value when modal window is not closed via window.close()
		modalValue = defaultCode;
		if( stack == NULL || stack->isTopModal() ) {
			modalValue = defaultCode;
			return;
		}

		// suspend active context, we're going to resume it when 
		// the modal dialog is closed
		suspendActiveContext();

		if( suspendedContext ) {
			// attach itself as a listener of hide event so the context
			// can be resumed after the modal document is hidden
			WSWUI::Document *doc = stack->pushDocument( location.GetURL()->buffer, true, true );
			if( doc ) {
				attachedModalDocument = doc->getRocketDocument();
				attachedModalDocument->AddEventListener( "hide", this );
			}
		}
	}

	/// Returns exit code of the last opened modal document.
	int getModalValue( void ) const
	{
		return modalValue;
	}

	/// Closes the current document if there's more than one on the stack.
	/// Stores exit code to be passed to suspended context if modal.
	void close( int code = 0 )
	{
		WSWUI::NavigationStack *stack = UI_Main::Get()->getNavigator();
		if( stack == NULL ) {
			return;
		}

		ElementDocument *document = GetCurrentUIDocument();
		bool isModal = document->IsModal();

		// can't close if there's a modal dialog on top
		if( !isModal && stack->isTopModal() ) {
			return;
		}

		// so it's a top modal dialog or there's no modal dialog on stack at all
		if( isModal ) {
			modalValue = code;
			stack->popDocument();
		}
		else {
			// not really a modal window, clear the stack
			UI_Main::Get()->showUI( false );
		}
	}

	/// Run all currently active schedulers.
	/// If we're the only reference holder to a document, release the document and its scheduler
	void update( void )
	{
		SchedulerMap::iterator it = schedulers.begin();
		while( it != schedulers.end() ) {
			// grab the next pointer in case we erase the current one
			SchedulerMap::iterator next = it;
			next++;

			ElementDocument *doc = it->first;
			FunctionCallScheduler *scheduler = it->second;

			if( doc->GetReferenceCount() == 1 ) {
				scheduler->shutdown();
				__delete__( scheduler );

				doc->RemoveReference();

				schedulers.erase( it );
			}
			else {
				scheduler->update();
			}

			// advance
			it = next;
		}
	}

	ElementDocument *getDocument( void ) const
	{
		ElementDocument *document = GetCurrentUIDocument();
		document->AddReference();
		return document;
	}

	ASURL getLocation( void ) const
	{
		ElementDocument *document = GetCurrentUIDocument();
		return ASURL( document->GetSourceURL().CString() );
	}

	void setLocation( const ASURL &location )
	{
		open( location );
	}

	unsigned int getTime( void ) const
	{
		const RefreshState &state = UI_Main::Get()->getRefreshState();
		return state.time;
	}

	bool getDrawBackground( void ) const
	{
		const RefreshState &state = UI_Main::Get()->getRefreshState();
		return state.drawBackground;
	}

	int getWidth( void ) const
	{
		const RefreshState &state = UI_Main::Get()->getRefreshState();
		return state.width;
	}

	int getHeight( void ) const
	{
		const RefreshState &state = UI_Main::Get()->getRefreshState();
		return state.height;
	}

	unsigned int historySize( void ) const
	{
		WSWUI::NavigationStack *stack = UI_Main::Get()->getNavigator();
		if( stack != NULL ) {
			return stack->getStackSize();
		}
		return 0;
	}

	void historyBack( void ) const
	{
		WSWUI::NavigationStack *stack = UI_Main::Get()->getNavigator();
		if( stack != NULL && stack->hasAtLeastTwoDocuments() && !stack->isTopModal() ) {
			stack->popDocument();
		}
	}

	int setTimeout( asIScriptFunction *func, unsigned int ms )
	{
		return getSchedulerForCurrentUIDocument()->setTimeout( func, ms );
	}

	int setTimeout( asIScriptFunction *func, unsigned int ms, CScriptAnyInterface &any )
	{
		return getSchedulerForCurrentUIDocument()->setTimeout( func, ms, any );
	}

	int setInterval( asIScriptFunction *func, unsigned int ms )
	{
		return getSchedulerForCurrentUIDocument()->setInterval( func, ms );
	}

	int setInterval( asIScriptFunction *func, unsigned int ms, CScriptAnyInterface &any )
	{
		return getSchedulerForCurrentUIDocument()->setInterval( func, ms, any );
	}

	void clearTimeout( int id )
	{
		getSchedulerForCurrentUIDocument()->clearTimeout( id );
	}

	void clearInterval( int id )
	{
		getSchedulerForCurrentUIDocument()->clearInterval( id );
	}

	void ProcessEvent( Event &event )
	{
		if( suspendedContext && event.GetTargetElement() == attachedModalDocument ) {
			detachAsEventListener();
			resumeSuspendedContext();
		}
	}

	void startLocalSound( const asstring_t &s )
	{
		trap::S_StartLocalSound( s.buffer );
	}

	void startBackgroundTrack( const asstring_t &intro, const asstring_t &loop, bool stopIfPlaying )
	{
		if( stopIfPlaying || !backgroundTrackPlaying ) {
			trap::S_StartBackgroundTrack( intro.buffer, loop.buffer );
			backgroundTrackPlaying = true;
		}
	}

	void stopBackgroundTrack( void )
	{
		trap::S_StopBackgroundTrack();
		backgroundTrackPlaying = false;
	}

	void flash( unsigned int count )
	{
		trap::VID_FlashWindow( count );
	}

private:
	typedef std::map<ElementDocument *, FunctionCallScheduler *>  SchedulerMap;
	SchedulerMap schedulers;

	/// Suspend active Angelscript execution context
	void suspendActiveContext( void )
	{
		suspendedContext = UI_Main::Get()->getAS()->getActiveContext();
		suspendedContext->Suspend();
	}

	/// Resume previously suspended AngelScript execution context
	void resumeSuspendedContext( void )
	{
		suspendedContext->Execute();
		suspendedContext = NULL;
	}

	static ElementDocument *GetCurrentUIDocument( void )
	{
		// we assume we set the user data at document instancing in asui_scriptdocument.cpp
		// also note that this method can called outside of AS execution context!
		return static_cast<ElementDocument *>( UI_Main::Get()->getAS()->getActiveModule()->GetUserData() );
	}

	void detachAsEventListener( void )
	{
		if( attachedModalDocument ) {
			attachedModalDocument->RemoveEventListener( "hide", this );
			attachedModalDocument = NULL;
		}
	}

	/// finds or creates new scheduler for the document currently on AS-stack
	FunctionCallScheduler *getSchedulerForCurrentUIDocument( void )
	{
		ElementDocument *doc = GetCurrentUIDocument();
		SchedulerMap::iterator it = schedulers.find( doc );

		FunctionCallScheduler *scheduler;
		if( it == schedulers.end() ) {
			doc->AddReference();

			scheduler = __new__( FunctionCallScheduler )();
			scheduler->init( UI_Main::Get()->getAS() );

			schedulers[doc] = scheduler;
		}
		else {
			scheduler = it->second;
		}
		return scheduler;
	}

	// context we've suspended to popup the modal document
	// we're going to resume it as soon as the document 
	// is closed with document.close call in the script
	asIScriptContext *suspendedContext;

	// modal document we've attached to
	ElementDocument *attachedModalDocument;

	// exit code passed via document.close() of the modal document
	int modalValue;

	bool backgroundTrackPlaying;
};

// ====================================================================

static ASWindow *asWindow;

/// This makes AS aware of this class so other classes may reference
/// it in their properties and methods
void PrebindWindow( ASInterface *as )
{
	ASBind::Class<ASWindow, ASBind::class_singleref>( as->getEngine() );
}

void BindWindow( ASInterface *as )
{
	ASBind::Global( as->getEngine() )
		// setTimeout and setInterval callback funcdefs
		.funcdef( &FunctionCallScheduler::ASFuncdef, "TimerCallback" )
		.funcdef( &FunctionCallScheduler::ASFuncdef2, "TimerCallback2" )
	;

	ASBind::GetClass<ASWindow>( as->getEngine() )
		.method( &ASWindow::open, "open" )
		.method2( &ASWindow::close, "void close( int code = 0 )" )
		.method2( &ASWindow::modal, "void modal( const String &location, int defaultCode = -1 )" )
		.method( &ASWindow::getModalValue, "getModalValue" )

		.method( &ASWindow::getDocument, "get_document" )

		.method( &ASWindow::getLocation, "get_location" )
		.method( &ASWindow::setLocation, "set_location" )

		.method( &ASWindow::getTime, "get_time" )
		.method( &ASWindow::getDrawBackground, "get_drawBackground" )
		.method( &ASWindow::getWidth, "get_width" )
		.method( &ASWindow::getHeight, "get_height" )

		.method( &ASWindow::historySize, "history_size" )
		.method( &ASWindow::historyBack, "history_back" )

		.constmethod( &ASWindow::startLocalSound, "startLocalSound" )
		.method2( &ASWindow::startBackgroundTrack, "void startBackgroundTrack( String &in intro, String &in loop, bool stopIfPlaying = true ) const" )
		.constmethod( &ASWindow::stopBackgroundTrack, "stopBackgroundTrack" )

		.method2<int (ASWindow::*)(asIScriptFunction *, unsigned int)>
			( &ASWindow::setTimeout, "int setTimeout (TimerCallback @, uint)" )
		.method2<int (ASWindow::*)(asIScriptFunction *, unsigned int)>
			( &ASWindow::setInterval, "int setInterval (TimerCallback @, uint)" )

		.method2<int (ASWindow::*)(asIScriptFunction *, unsigned int, CScriptAnyInterface &)>
			( &ASWindow::setTimeout, "int setTimeout (TimerCallback2 @, uint, any &in)" )
		.method2<int (ASWindow::*)(asIScriptFunction *, unsigned int, CScriptAnyInterface &)>
			( &ASWindow::setInterval, "int setInterval (TimerCallback2 @, uint, any &in)" )

		.method( &ASWindow::clearTimeout, "clearTimeout" )
		.method( &ASWindow::clearInterval, "clearInterval" )

		.method( &ASWindow::flash, "flash" )
	;
}

void BindWindowGlobal( ASInterface *as )
{
	assert( asWindow == NULL );

	// set the AS module for scheduler
	asWindow = __new__( ASWindow )( as );

	ASBind::Global( as->getEngine() )
		// global variable
		.var( asWindow, "window" )
	;
}

void RunWindowFrame( void )
{
	assert( asWindow != NULL );

	asWindow->update();
}

void UnbindWindow( void )
{
	__delete__( asWindow );
	asWindow = NULL;
}

}

ASBIND_TYPE( ASUI::ASWindow, Window );
