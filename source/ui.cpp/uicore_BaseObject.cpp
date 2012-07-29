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


#include "uicore_BaseObject.h"
#include "uiwsw_Utils.h"
#include "uicore_Global.h"

namespace UICore
{

	ALLOCATOR_DEFINITION(BaseObject)
	DELETER_DEFINITION(BaseObject)

	/** Needed to properly draw borders */
	extern float scaleX, scaleY;

	bool BaseObject::shiftPressed = false;

	void BaseObject::BaseMouseDown( float x, float y, int button )
	{
		Rect r;

		sortChildren();

		for ( std::list<BaseObject*>::reverse_iterator child = sorted_children.rbegin() ; child != sorted_children.rend() ; child++ )
		{
			if ( (*child)->isVisible() && (*child)->isEnabled() && (*child)->isClickable() )
			{
				r = (*child)->position;
				r.w = max( 0.0f, min( (*child)->position.w, position.w - (*child)->position.x ) );
				r.h = max( 0.0f, min( (*child)->position.h, position.h - (*child)->position.y ) );
				if ( r.isPointInside( x, y ) )
				{
					(*child)->BaseMouseDown( x - (*child)->position.x, y - (*child)->position.y, button );
					return;
				}
			}
		}

		if ( draggable && button == Importer::clickButton )
			draggingObject = this;

		if ( focusedObject != this && button != Importer::mouseWheelUp && button != Importer::mouseWheelDown )
		{
			if ( focusedObject && !focusable )
			{
				BaseObject *oldFocused;

				// lose focus to nothing
				oldFocused = focusedObject;
				focusedObject = NULL;
				oldFocused->BaseLostFocus();
			}
			else if ( focusable )
			{
				setFocus();
			}
		}
		if ( MouseDown )
			MouseDown( this, x, y, button );
	}

	void BaseObject::BaseMouseUp( float x, float y, int button )
	{
		Rect r;

		sortChildren();

		for ( std::list<BaseObject*>::reverse_iterator child = sorted_children.rbegin() ; child != sorted_children.rend() ; child++ )
		{
			if ( (*child)->isVisible() && (*child)->isEnabled() && (*child)->isClickable() )
			{
				r =(*child)->position;
				r.w = max( 0.0f, min( (*child)->position.w, position.w - (*child)->position.x ) );
				r.h = max( 0.0f, min( (*child)->position.h, position.h - (*child)->position.y ) );
				if ( r.isPointInside( x, y ) )
				{
					(*child)->BaseMouseUp( x - (*child)->position.x, y - (*child)->position.y, button );
					return;
				}
			}
		}

		draggingObject = NULL;

		if ( MouseUp )
			MouseUp( this, x, y, button );
		if ( button == Importer::clickButton )
			DoClick();
	}

	void BaseObject::DoClick()
	{
		if ( ( lastclick >= (UIWsw::Local::getTime() - 500)) && (DoubleClick) ) {
			DoubleClick( this );
		} else if ( Click ) {
			Click( this );
		}
		lastclick = UIWsw::Local::getTime();
		if (this->clickthrough) {
			parent->DoClick();
		}
	}

	void BaseObject::BaseMouseMove( float x, float y, float xrel, float yrel )
	{
		if ( draggingObject )
		{
			draggingObject->position.x += xrel;
			draggingObject->position.y += yrel;
			if ( draggingObject->Dragging )
				draggingObject->Dragging( draggingObject, xrel, yrel );
			return;
		}

		Rect r;

		sortChildren();

		for ( std::list<BaseObject*>::reverse_iterator child = sorted_children.rbegin() ; child != sorted_children.rend() ; child++ )
		{
			if ( (*child)->isVisible() && (*child)->isEnabled() && (*child)->isClickable() )
			{
				r = (*child)->position;
				r.w = max( 0.0f, min( (*child)->position.w, position.w - (*child)->position.x ) );
				r.h = max( 0.0f, min( (*child)->position.h, position.h - (*child)->position.y ) );
				if ( r.isPointInside( x, y ) )
				{
					(*child)->BaseMouseMove( x - (*child)->position.x, y - (*child)->position.y, xrel, yrel );
					return;
				}
			}
		}

		if ( mouseOverObject && mouseOverObject != this )
		{
			BaseMouseIn( mouseOverObject );
			mouseOverObject->BaseMouseOut( this );
		}
		mouseOverObject = this;

		if ( MouseMove )
			MouseMove( this, x, y, xrel, yrel );
	}

	void BaseObject::BaseMouseIn( BaseObject *oldObject )
	{
		if ( MouseIn )
			MouseIn( this, oldObject );
	}

	void BaseObject::BaseMouseOut( BaseObject *newObject )
	{
		if ( MouseOut )
			MouseOut( this, newObject );
	}

	void BaseObject::BaseLostFocus( void )
	{
		if ( LostFocus )
			LostFocus( this );
	}

	void BaseObject::BaseKeyDown( int key, int charVal )
	{
		if ( key == Importer::keyShift )
			shiftPressed = true;

		if ( focusedObject && focusedObject != this )
		{
			focusedObject->BaseKeyDown( key, charVal );
			return;
		}
		else 
		{
			BaseObject *oldFocus = focusedObject;
			BaseObject *p = this;

			if ( charVal == Importer::keyTab )
			{
				if ( !focusedObject )
				{
					setFocus();
					return;
				}

				if ( !shiftPressed )
				{
					while ( oldFocus == focusedObject )
					{
						if ( p->nextObject )
						{
							if ( p->nextObject->isVisible() )
								p->nextObject->setFocus();
							p = p->nextObject;
						}
						else if ( p->parent )
							p = p->parent;
						else // stop case : no next and no parent, so we take first of the list
						{
							while ( p->prevObject ) // first object...
								p = p->prevObject;
							while ( !p->isVisible() ) // ...then first visible object
								p = p->nextObject;
							p->setFocus();
							break;
						}
						if ( p == this ) // this is the only selectable item
							break;
					}
				}
				else
				{
					while ( oldFocus == focusedObject )
					{
						if ( p->prevObject )
						{
							if ( p->prevObject->isVisible() )
								p->prevObject->setFocus();
							p = p->prevObject;
							// if it's not p but something inside p who got focus
							if ( oldFocus != focusedObject && !p->focusable )
							{
								// so we must take the last object inside p
								do
								{
									if ( !p->children.empty() )
									{
										p = p->children.back();
										while ( p->prevObject && p->focusable && !p->isVisible() ) // last visible object
											p = p->prevObject;
										p->setFocus();
									}
								// if p is a container, then we go again to take last item of p
								} while ( p != focusedObject );
							}
						}
						else if ( p->parent )
						{
							p = p->parent;
						}
						else // stop case : no prev and no parent, so we take last of the list
						{
							// we must take the last object inside the last object of the list
							do
							{
								p = focusedObject;
								while ( p->nextObject ) // last object...
									p = p->nextObject;
								while ( !p->isVisible() ) // ...then last visible object
									p = p->prevObject;
								p->setFocus();
							// if p is a container, then we go again to take last item of p
							} while ( p != focusedObject );
							break;
						}
						if ( p == this ) // this is the only selectable item
							break;
					}
				}
			}
		}

		if ( KeyDown )
			KeyDown( this, key, charVal );
	}

	void BaseObject::BaseKeyUp( int key, int charVal )
	{
		if ( key == Importer::keyShift )
			shiftPressed = false;

		if ( focusedObject && focusedObject != this )
		{
			focusedObject->BaseKeyUp( key, charVal );
			return;
		}

		if ( KeyUp )
			KeyUp( this, key, charVal );
	}

	bool BaseObject::RemoveChild( BaseObject *child )
	{
		for ( std::list<BaseObject*>::iterator it = children.begin() ; it != children.end() ; it++ )
			if ( *it == child )
			{
				if ( child->prevObject )
					child->prevObject->nextObject = nextObject;
				if ( child->nextObject )
					child->nextObject->prevObject = prevObject;	
				child->prevObject = child->nextObject = NULL;
				children.erase( it );
				child->parent = NULL;
				setSortChildren( true );
				return true;
			}
		return false;
	}

	void BaseObject::AddChild( BaseObject *obj )
	{
		if (!obj) return;
		assert(obj != this);
		/*
		if (obj == this) {
			obj->nextObject = NULL;
			obj->prevObject = NULL;
			return;
		}
		*/
		if ( !this->children.empty() ) {
			obj->prevObject = this->children.back();
		} else {
			obj->prevObject = NULL;
		}
		if (obj->prevObject) {
			obj->prevObject->nextObject = obj;
		}
		obj->nextObject = NULL;
		obj->index = this->children.size();
		this->children.push_back(obj);
		this->setSortChildren( true );
		obj->parent = this;
	}

	void BaseObject::setSortChildren( bool r )
	{
		sort_children = r;
	}

	void BaseObject::sortChildren( void )
	{
		if( !sort_children )
			return;

		sorted_children.clear();
		for ( std::list<BaseObject*>::const_iterator it = children.begin() ; it != children.end() ; ++it )
			sorted_children.push_back( *it );
		sorted_children.sort( BaseObject::compareChildren );

		setSortChildren( false );
	}

	bool BaseObject::Contains( BaseObject *obj )
	{
		for ( std::list<BaseObject*>::const_iterator it = children.begin() ; it != children.end() ; ++it )
		{
			if ( obj == *it )
				return true;

			if ( (*it)->Contains( obj ) )
				return true;
		}
		return false;
	}

	BaseObject::BaseObject()
	{
		Initialize();
	}

	BaseObject::BaseObject( BaseObject *parent, float x, float y, float w, float h )
	{
		Initialize();
		setParent( parent );
		setPosition( x, y );
		setSize( w, h );
	}

	BaseObject::~BaseObject()
	{
		deleteChildren();

		if ( parent )
			parent->RemoveChild( this );

		setFocusable( false );

		if( focusedObject == this )
			focusedObject = NULL;
		if( mouseOverObject == this )
			mouseOverObject = NULL;
		if( draggingObject == this )
			draggingObject = NULL;
	}

	void BaseObject::deleteChildren( void )
	{
		BaseObject *child;

		for ( std::list<BaseObject*>::iterator it = children.begin() ; it != children.end() ; it++ )
			(*it)->deleteChildren();

		while ( children.begin() != children.end() )
		{
			child = *children.begin();
			children.pop_front();
			child->parent = NULL;
			delete child;
		}
	}

	void BaseObject::Initialize()
	{
		prevObject = nextObject = parent = NULL;
		lastclick = 0;
		setRotation( 0 );
		setVisible( true );
		setEnabled( true );
		setDraggable( false );
		setFocusable( false );
		setClickable( true );
		setClickThrough( false );
		setBackColor( Color( 0, 0, 0, 0 ) );
		setBorderColor( Color( 0, 0, 0, 0 ) );
		setDisabledColor( Color( 0, 0, 0, 0 ) );
		setBackgroundImage( NULL );
		setDisabledImage( NULL );
		setBorderWidth( 1 );
		setBorderImages( NULL );
		setAfterDrawHandler( NULL );
		setClickHandler( NULL );
		setDoubleClickHandler( NULL );
		setMouseDownHandler( NULL );
		setMouseUpHandler( NULL );
		setMouseMoveHandler( NULL );
		setMouseInHandler( NULL );
		setMouseOutHandler( NULL );
		setGotFocusHandler( NULL );
		setLostFocusHandler( NULL );
		setDraggingHandler( NULL );
		setKeyDownHandler( NULL );
		setKeyUpHandler( NULL );
		setUserData(NULL);
	}

	void BaseObject::FitPosition( const Rect *parentPos, const Rect *parentPosSrc, Rect &posSrc, Rect &posDest )
	{
		if ( !parentPos )
		{
			posSrc.x = posSrc.y = 0;
			posDest.x = position.x;
			posDest.y = position.y;
			posSrc.w = posDest.w = position.w;
			posSrc.h = posDest.h = position.h;
		}
		else
		{
			if ( parentPosSrc )
			{
				posSrc.x = max( 0.0f, parentPosSrc->x - position.x );
				posSrc.y = max( 0.0f, parentPosSrc->y - position.y );
				posSrc.w = min( min( position.w - posSrc.x, parentPosSrc->w ), (parentPosSrc->x - posSrc.x) + parentPosSrc->w - position.x );
				posSrc.h = min( min( position.h - posSrc.y, parentPosSrc->h ), (parentPosSrc->y - posSrc.y) + parentPosSrc->h - position.y );
			}
			else
			{
				posSrc.x = max( 0.0f, -position.x );
				posSrc.y = max( 0.0f, -position.y );
				posSrc.w = max( position.w - posSrc.x, parentPos->w - position.x );
				posSrc.h = max( position.h - posSrc.y, parentPos->h - position.y );
			}
			posDest.x = parentPos->x + position.x;
			posDest.y = parentPos->y + position.y;
			posDest.w = position.w;
			posDest.h = position.h;
		}
	}

	void BaseObject::DrawBackground( const Rect &posSrc, const Rect &posDest, bool enabled )
	{
		Rect rect;
		float bwX, bwY;

		bwX = max( 0.0f, min( posSrc.w, borderWidth / scaleX ) - posSrc.x );
		bwY = max( 0.0f, min( posSrc.h, borderWidth / scaleY ) - posSrc.y );

		rect.x = posSrc.x + bwX;
		rect.y = posSrc.y + bwY;
		rect.w = posSrc.w - bwX * 2;
		rect.h = posSrc.h - bwY * 2;
		if ( !backgroundImage )
		{
			rect.x += posDest.x;
			rect.y += posDest.y;
		}

		if ( enabled && this->enabled )
		{
			if ( backgroundImage )
				Importer::DrawImage( backgroundImage, rect, posDest, backColor, rotation );
			else
				Importer::FillRect( rect, backColor );
		}
		else
		{
			if ( disabledImage )
				Importer::DrawImage( disabledImage, rect, posDest, disabledColor, rotation );
			else
				Importer::FillRect( rect, disabledColor );
		}
	}

	void BaseObject::DrawBorder( const Rect &posSrc, const Rect &posDest, bool enabled )
	{
		if ( borderWidth <= 0.0f )
			return;

		if( borderImages[0] )
		{
			Rect s, d;
			float bwX, bwY;

			bwX = max( 0.0f, min( posSrc.w, borderWidth / scaleX ) - posSrc.x );
			bwY = max( 0.0f, min( posSrc.h, borderWidth / scaleY ) - posSrc.y );

			// Draw top border
			s = posSrc, s.x += bwX, s.w -= bwX * 2, s.h = bwY;
			d = posDest;
			Importer::DrawImage( borderImages[BORDER_TOP], s, d, borderColor, rotation );

			// Draw bottom border
			s = posSrc,  s.x += bwX, s.y += posSrc.h - bwY, s.w -= bwX * 2, s.h = bwY;
			d = posDest;
			Importer::DrawImage( borderImages[BORDER_BOTTOM], s, d, borderColor, rotation );

			// Draw left border
			s = posSrc,  s.y += bwY, s.w = bwX, s.h = posSrc.h - bwY * 2;
			d = posDest;
			Importer::DrawImage( borderImages[BORDER_LEFT], s, d, borderColor, rotation );

			// Draw right border
			s = posSrc,  s.x += posSrc.w - bwX, s.y += bwY, s.w = bwX, s.h = posSrc.h - bwY * 2;
			d = posDest;
			Importer::DrawImage( borderImages[BORDER_RIGHT], s, d, borderColor, rotation );

			// Draw top-left corner
			s = posSrc, s.w = bwX, s.h = bwY;
			d = posDest;
			Importer::DrawImage( borderImages[BORDER_TOP_LEFT], s, d, borderColor, rotation );

			// Draw top-right corner
			s = posSrc, s.x += posSrc.w - bwX, s.w = bwX, s.h = bwY;
			d = posDest;
			Importer::DrawImage( borderImages[BORDER_TOP_RIGHT], s, d, borderColor, rotation );

			// Draw bottom-right corner
			s = posSrc, s.x += posSrc.w - bwX, s.y += posSrc.h - bwY, s.w = bwX, s.h = bwY;
			d = posDest;
			Importer::DrawImage( borderImages[BORDER_BOTTOM_RIGHT], s, d, borderColor, rotation );

			// Draw bottom-left corner
			s = posSrc, s.y += posSrc.h - bwY, s.w = bwX, s.h = bwY;
			d = posDest;
			Importer::DrawImage( borderImages[BORDER_BOTTOM_LEFT], s, d, borderColor, rotation );
		}
		else
		{
			Rect r;
			float bwX, bwY;
			bwX = max( 0.0f, min( posSrc.w, borderWidth / scaleX ) - posSrc.x );
			bwY = max( 0.0f, min( posSrc.h, borderWidth / scaleY ) - posSrc.y );
			// Draw top
			r.x = posDest.x + posSrc.x;
			r.y = posDest.y + posSrc.y;
			r.w = posSrc.w;
			r.h = bwY;
			Importer::FillRect( r, borderColor );
			// Draw Bottom
			r.y = posDest.y + posSrc.y + posSrc.h - bwY;
			Importer::FillRect( r, borderColor );
			// Draw Left
			r.x = posDest.x + posSrc.x;
			r.y = posDest.y + posSrc.y + bwY;
			r.w = bwX;
			r.h = posSrc.h - bwY - bwY;
			Importer::FillRect( r, borderColor );
			// Draw Right
			r.x = posDest.x + posSrc.x + posSrc.w - bwX;
			Importer::FillRect( r, borderColor );
		}
	}

	bool BaseObject::compareChildren( BaseObject *c1, BaseObject *c2 )
	{
		if( c1->zIndex < c2->zIndex )
			return true;
		if( c1->zIndex > c2->zIndex )
			return false;
		if( c1->index < c2->index )
			return true;
		if( c1->index > c2->index )
			return false;
		return false;
	}

	void BaseObject::DrawChildren( unsigned int deltatime, const Rect &posSrc, const Rect &posDest, bool enabled )
	{
		sortChildren();

		for ( std::list<BaseObject*>::iterator child = sorted_children.begin() ; child != sorted_children.end() ; child++ )
		{
			(*child)->Draw( deltatime, &posDest, &posSrc, (enabled && this->enabled) );
		}
	}

	void BaseObject::Draw( unsigned int deltatime, const Rect *parentPos, const Rect *parentPosSrc, bool enabled )
	{
		Rect posSrc, posDest;

		if ( !visible )
			return;

		FitPosition( parentPos, parentPosSrc, posSrc, posDest );
		DrawBackground( posSrc, posDest, enabled );
		DrawChildren( deltatime, posSrc, posDest, enabled );
		DrawBorder( posSrc, posDest, enabled );

		if ( AfterDraw )
			AfterDraw( this, deltatime, parentPos, parentPosSrc, enabled );
	}

	void BaseObject::setFocusable( bool v )
	{
		focusable = v;
		if ( !v )
		{
			if ( prevObject )
				prevObject->nextObject = nextObject;
			if ( nextObject )
				nextObject->prevObject = prevObject;
		}
	}

	void BaseObject::setParent( BaseObject *obj )
	{
		if ( parent ) {
			parent->RemoveChild( this );
		}
		if ( obj ) {
			obj->AddChild( this );
		} else {
			parent = NULL;
		}
	}

	BaseObject *BaseObject::getParent( void )
	{
		return parent;
	}
#if 0
	void BaseObject::PutOnTop( BaseObject *obj )
	{
		std::list<BaseObject *>::iterator it;
		BaseObject *next;

		for( it = children.begin() ; it != children.end() ; it++ )
		{
			if( (*it) == obj )
				break;
		}

		if( it == children.end() )
			return;

		children.erase( it );
		children.push_back( obj );

		if( obj->prevObject )
			obj->prevObject->nextObject = obj->nextObject;
		if( obj->nextObject )
			obj->nextObject->prevObject = obj->prevObject;

		// go to the back of the queue
		if( !obj->nextObject )
			next = obj;
		else
		{
			next = obj->nextObject;
			while( next->nextObject )
				next = next->nextObject;
		}

		// put object at the front of the queue (i.e. drawn first)
		obj->prevObject = next;
		obj->nextObject = NULL;
		next->nextObject = obj;
	}

	void BaseObject::PutOnBottom( BaseObject *obj )
	{
		std::list<BaseObject *>::iterator it;
		BaseObject *prev;

		for( it = children.begin() ; it != children.end() ; it++ )
		{
			if( (*it) == obj )
				break;
		}

		if( it == children.end() )
			return;

		children.erase( it );
		children.push_front( obj );

		if( obj->prevObject )
			obj->prevObject->nextObject = obj->nextObject;
		if( obj->nextObject )
			obj->nextObject->prevObject = obj->prevObject;

		// go to the front of the queue
		if( !obj->prevObject )
			prev = obj;
		else
		{
			prev = obj->prevObject;
			while( prev->prevObject )
				prev = prev->prevObject;
		}

		// put object at the front of the queue (i.e. drawn first)
		obj->nextObject = prev;
		obj->prevObject = NULL;
		prev->prevObject = obj;
	}
#endif

	void BaseObject::setFocus( void )
	{
		if ( focusedObject != this )
		{
			BaseObject *oldFocus = focusedObject;
			if ( focusable )
			{
				focusedObject = this;
				if ( oldFocus )
					oldFocus->BaseLostFocus();
				if ( GotFocus )
					GotFocus( this );
			}
			else for ( std::list<BaseObject*>::iterator it = children.begin() ; it != children.end() ; ++it )
			{
				(*it)->setFocus();
				if ( focusedObject != oldFocus )
					break;
			}
		}
	}
}
