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

#ifndef _UICORE_BASEOBJECT_H_
#define _UICORE_BASEOBJECT_H_

#include "uicore_Types.h"

namespace UICore
{
	typedef enum
	{
		BORDER_TOP_LEFT,
		BORDER_TOP,
		BORDER_TOP_RIGHT,
		BORDER_RIGHT,
		BORDER_BOTTOM_RIGHT,
		BORDER_BOTTOM,
		BORDER_BOTTOM_LEFT,
		BORDER_LEFT,

		BORDER_MAX_BORDERS
	} BorderImage;

	class BaseObject
	{
	protected:
		Rect position;
		bool visible;
		bool enabled;
		bool draggable;
		bool focusable;
		bool clickable;
		bool clickthrough;
		unsigned int lastclick;
		Color backColor;
		Color borderColor;
		Color disabledColor;
		Image backgroundImage;
		Image disabledImage;
		float borderWidth;
		void *userData;
		float rotation;
		Image borderImages[BORDER_MAX_BORDERS]; // border images starting from top left and going clockwise
		int index, zIndex;
		bool sort_children;

		BaseObject *parent;
		BaseObject *prevObject;
		BaseObject *nextObject;
		std::list<BaseObject*> children;
		std::list<BaseObject*> sorted_children;

		/** shift state is globally stored in order to properly do the backward keyboard nav (shift + tab) */
		static bool shiftPressed;

		virtual void BaseMouseDown( float x, float y, int button );
		virtual void BaseMouseUp( float x, float y, int button );
		virtual void BaseMouseMove( float x, float y, float xrel, float yrel );
		virtual void BaseMouseIn( BaseObject *oldObject );
		virtual void BaseMouseOut( BaseObject *newObject );
		virtual void BaseLostFocus( void );
		virtual void BaseKeyDown( int key, int charVal );
		virtual void BaseKeyUp( int key, int charVal );
		virtual void DoClick( void );

		void (*AfterDraw)( BaseObject *target, unsigned int deltatime, const Rect *parentPos, const Rect *parentPosSrc, bool enabled );
		void (*Click)( BaseObject *target );
		void (*DoubleClick)( BaseObject *target );
		void (*MouseDown)( BaseObject *target, float x, float y, int button );
		void (*MouseUp)( BaseObject *target, float x, float y, int button );
		void (*MouseMove)( BaseObject *target, float x, float y, float xrel, float yrel );
		void (*MouseIn)( BaseObject *target, BaseObject *oldObject );
		void (*MouseOut)( BaseObject *target, BaseObject *newObject );
		void (*GotFocus) (BaseObject *target );
		void (*LostFocus) (BaseObject *target );
		void (*Dragging)( BaseObject *target, float xrel, float yrel );
		void (*KeyDown)( BaseObject *target, int key, int charVal );
		void (*KeyUp)( BaseObject *target, int key, int charVal );

		bool RemoveChild( BaseObject *child );
		virtual void AddChild( BaseObject *child );
		void setFocusable( bool v );
		virtual void setSortChildren( bool r );
		virtual void sortChildren( void );

		virtual void Initialize();

		/** Set the actual position of this object according to its parent's one
			@param parentPos a rectangle representing actual parent position and size in absolute screen coordinates.
			This parameter can be NULL, and then posSrc is set to 0, 0, w, h and posDest to x, y, w, h
			@param parentPosSrc a rectangle representing the part of the parent object, in its local coordinates,
			that is going to be displayed. If parentPos is NULL, this parameter is not used. If this parameter is NULL,
			it is considered that the whole parent object will be displayed
			@param posSrc a rectangle instance on which will be written the part of the object that is going to be displayed,
			in local object	coordinates (if posSrc is 0, 0, w, h, then the whole object is going to be displayed)
			@param posDest a rectangle instance on which will be written the position and size in absolute screen coordinates
			on which the object will be displayed */
		void FitPosition( const Rect *parentPos, const Rect *parentPosSrc, Rect &posSrc, Rect &posDest );
		virtual void DrawBackground( const Rect &posSrc, const Rect &posDest, bool enabled = true );
		virtual void DrawBorder( const Rect &posSrc, const Rect &posDest, bool enabled = true );
		virtual void DrawChildren( unsigned int deltatime, const Rect &posSrc, const Rect &posDest, bool enabled = true );

	public:
		BaseObject();
		BaseObject( BaseObject *parent, float x, float y, float w, float h );
		virtual ~BaseObject();
		void deleteChildren( void );

		MEMORY_OPERATORS

		virtual void Draw( unsigned int deltatime, const Rect *parentPos = NULL, const Rect *parentPosSrc = NULL, bool enabled = true );

		/** Returns true if this object contains the passed object */
		bool Contains( BaseObject *obj );
		/** Returns an iterator set at the first child of the object. @Remark Using this function is UNSAFE */
		inline std::list<BaseObject*>::iterator getFirstChildIterator( void );
		/** Returns an iterator set just after the last child of the object. @Remark Using this function is UNSAFE */
		inline std::list<BaseObject*>::iterator getEndChildIterator( void );

		static bool compareChildren( BaseObject *c1, BaseObject *c2 );

		void setParent( BaseObject *obj );
		BaseObject *getParent( void );
#if 0
		void PutOnTop( BaseObject *obj );
		void PutOnBottom( BaseObject *obj );
#endif
		virtual void setFocus( void );

		inline virtual void setVisible( bool v );
		inline virtual void setEnabled( bool v );
		inline virtual void setDraggable( bool v );
		inline virtual void setClickable( bool v );
		inline virtual void setClickThrough( bool v );
		inline virtual void setPosition( const Rect &pos );
		inline virtual void setRotation( float angle );
		inline virtual void setPosition( float x, float y );
		inline virtual void setSize( float w, float h );
		inline virtual void setBackColor( const Color &color );
		inline virtual void setBorderColor( const Color &color );
		inline virtual void setDisabledColor( const Color &color );
		inline virtual void setBackgroundImage( Image image );
		inline virtual void setDisabledImage( Image image );
		inline virtual void setBorderWidth( const float width );
		inline virtual void setBorderImages( const Image *images );
		inline virtual void setZIndex( int z );
		inline virtual void setUserData(void *data);

		inline virtual bool isVisible( void ) const;
		inline virtual bool isEnabled( void ) const;
		inline virtual bool isDraggable( void ) const;
		inline virtual bool isClickable( void ) const;
		inline virtual Rect getPosition( void ) const;
		inline virtual void getPosition( float &x, float &y ) const;
		inline virtual float getPositionX( void ) const;
		inline virtual float getPositionY( void ) const;
		inline virtual void getSize( float &w, float &h ) const;
		inline virtual float getWidth( void ) const;
		inline virtual float getHeight( void ) const;
		inline virtual Color getBackColor( void ) const;
		inline virtual Color getBorderColor( void ) const;
		inline virtual Color getDisabledColor( void ) const;
		inline virtual Image getBackgroundImage( void ) const;
		inline virtual Image getDisabledImage( void ) const;
		inline virtual float getBorderWidth( void ) const;
		inline virtual const Image *getBorderImages( void ) const;
		inline virtual int getZIndex( void ) const;
		inline virtual void *getUserData(void);

		virtual std::string getType( void ) const = 0;

		inline virtual void setAfterDrawHandler( void (*AfterDraw)( BaseObject *target, unsigned int deltatime, const Rect *parentPos, const Rect *parentPosSrc, bool enabled ) );
		inline virtual void setClickHandler( void (*Click)( BaseObject *target ) );
		inline virtual void setDoubleClickHandler( void (*DoubleClick)( BaseObject *target ) );
		inline virtual void setMouseDownHandler( void (*MouseDown)( BaseObject *target, float x, float y, int button ) );
		inline virtual void setMouseUpHandler( void (*MouseUp)( BaseObject *target, float x, float y, int button ) );
		inline virtual void setMouseMoveHandler( void (*MouseMove)( BaseObject *target, float x, float y, float xrel, float yrel ) );
		inline virtual void setMouseInHandler( void (*MouseIn)( BaseObject *target, BaseObject *oldObject ) );
		inline virtual void setMouseOutHandler( void (*MouseOut)( BaseObject *target, BaseObject *newObject ) );
		inline virtual void setGotFocusHandler( void (*GotFocus)( BaseObject *target ) );
		inline virtual void setLostFocusHandler( void (*LostFocus)( BaseObject *target ) );
		inline virtual void setDraggingHandler( void (*Dragging)( BaseObject *target, float xrel, float yrel ) );
		inline virtual void setKeyDownHandler( void (*KeyDown)( BaseObject *target, int key, int charVal ) );
		inline virtual void setKeyUpHandler( void (*KeyUp)( BaseObject *target, int key, int charVal ) );
	};

	extern BaseObject *draggingObject;
	extern BaseObject *mouseOverObject;
	extern BaseObject *focusedObject;

	std::list<BaseObject*>::iterator BaseObject::getFirstChildIterator( void )
	{
		return children.begin();
	}

	std::list<BaseObject*>::iterator BaseObject::getEndChildIterator( void )
	{
		return children.end();
	}

	void BaseObject::setVisible( bool v )
	{
		visible = v;
		if ( !v && focusedObject && Contains( focusedObject ) )
			focusedObject = NULL;
	}

	void BaseObject::setEnabled( bool v )
	{
		enabled = v;
		if ( !v && focusedObject == this )
			focusedObject = NULL;
	}

	void BaseObject::setDraggable( bool v )
	{
		draggable = v;
	}

	void BaseObject::setClickable( bool v )
	{
		clickable = v;
	}

	void BaseObject::setClickThrough( bool v )
	{
		clickthrough = v;
	}

	void BaseObject::setPosition( const Rect &pos )
	{
		position = pos;
	}

	void BaseObject::setPosition( float x, float y )
	{
		position.x = x;
		position.y = y;
	}

	void BaseObject::setRotation( float angle )
	{
		rotation = angle;
	}

	void BaseObject::setSize( float w, float h )
	{
		position.w = w;
		position.h = h;
	}

	void BaseObject::setBackColor( const Color &color )
	{
		backColor = color;
	}

	void BaseObject::setBorderColor( const Color &color )
	{
		borderColor = color;
	}

	void BaseObject::setDisabledColor( const Color &color )
	{
		disabledColor = color;
	}

	void BaseObject::setBackgroundImage( Image image )
	{
		backgroundImage = image;
	}

	void BaseObject::setDisabledImage( Image image )
	{
		disabledImage = image;
	}

	void BaseObject::setBorderWidth( float width )
	{
		borderWidth = width;
	}

	void BaseObject::setBorderImages( const Image *images )
	{
		int i;

		for( i = 0; i < BORDER_MAX_BORDERS; i++ )
			borderImages[i] = NULL;

		if( !images )
			return;
		
		for( i = 0; i < BORDER_MAX_BORDERS; i++ )
			borderImages[i] = images[i];
	}

	void BaseObject::setZIndex( int z )
	{
		zIndex = z;
		if ( parent )
			parent->setSortChildren( true );
	}

	void BaseObject::setUserData(void *data)
	{
		userData = data;
	}

	void *BaseObject::getUserData(void)
	{
		return userData;
	}
	bool BaseObject::isVisible( void ) const
	{
		return visible;
	}

	bool BaseObject::isEnabled( void ) const
	{
		if( parent && !parent->isEnabled() )
			return false;
		return enabled;
	}

	bool BaseObject::isDraggable( void ) const
	{
		return draggable;
	}

	bool BaseObject::isClickable( void ) const
	{
		if( parent && !parent->isClickable() )
			return false;
		return clickable;
	}

	Rect BaseObject::getPosition( void ) const
	{
		return position;
	}
	float BaseObject::getPositionX( void ) const
	{
		return position.x;
	}
	float BaseObject::getPositionY( void ) const
	{
		return position.y;
	}

	void BaseObject::getPosition( float &x, float &y ) const
	{
		x = position.x;
		y = position.y;
	}

	void BaseObject::getSize( float &w, float &h ) const
	{
		w = position.w;
		h = position.h;
	}

	float BaseObject::getWidth( void ) const
	{
		return position.w;
	}

	float BaseObject::getHeight( void ) const
	{
		return position.h;
	}

	Color BaseObject::getBackColor( void ) const
	{
		return backColor;
	}

	Color BaseObject::getBorderColor( void ) const
	{
		return borderColor;
	}

	Color BaseObject::getDisabledColor( void ) const
	{
		return disabledColor;
	}

	Image BaseObject::getBackgroundImage( void ) const
	{
		return backgroundImage;
	}

	Image BaseObject::getDisabledImage( void ) const
	{
		return disabledImage;
	}

	float BaseObject::getBorderWidth( void ) const
	{
		return borderWidth;
	}

	const Image *BaseObject::getBorderImages( void ) const
	{
		return borderImages;
	}

	int BaseObject::getZIndex( void ) const
	{
		return zIndex;
	}

	void BaseObject::setAfterDrawHandler( void (*AfterDraw)( BaseObject *target, unsigned int deltatime, const Rect *parentPos, const Rect *parentPosSrc, bool enabled ) )
	{
		this->AfterDraw = AfterDraw;
	}

	void BaseObject::setClickHandler( void (*Click)( BaseObject *target ) )
	{
		this->Click = Click;
	}

	void BaseObject::setDoubleClickHandler( void (*DoubleClick)( BaseObject *target ) )
	{
		this->DoubleClick = DoubleClick;
	}

	void BaseObject::setMouseDownHandler( void (*MouseDown)( BaseObject *target, float x, float y, int button ) )
	{
		this->MouseDown = MouseDown;
	}

	void BaseObject::setMouseUpHandler( void (*MouseUp)( BaseObject *target, float x, float y, int button ) )
	{
		this->MouseUp = MouseUp;
	}

	void BaseObject::setMouseMoveHandler( void (*MouseMove)( BaseObject *target, float x, float y, float xrel, float yrel ) )
	{
		this->MouseMove = MouseMove;
	}

	void BaseObject::setMouseInHandler( void (*MouseIn)( BaseObject *target, BaseObject *oldObject ) )
	{
		this->MouseIn = MouseIn;
	}

	void BaseObject::setMouseOutHandler( void (*MouseOut)( BaseObject *target, BaseObject *newObject ) )
	{
		this->MouseOut = MouseOut;
	}

	void BaseObject::setGotFocusHandler( void (*GotFocus)( BaseObject *target ) )
	{
		this->GotFocus = GotFocus;
	}

	void BaseObject::setLostFocusHandler( void (*LostFocus)( BaseObject *target ) )
	{
		this->LostFocus = LostFocus;
	}

	void BaseObject::setDraggingHandler( void (*Dragging)( BaseObject *target, float xrel, float yrel ) )
	{
		this->Dragging = Dragging;
	}

	void BaseObject::setKeyDownHandler( void (*KeyDown)( BaseObject *target, int key, int charVal ) )
	{
		this->KeyDown = KeyDown;
	}

	void BaseObject::setKeyUpHandler( void (*KeyUp)( BaseObject *target, int key, int charVal ) )
	{
		this->KeyUp = KeyUp;
	}
}

#endif
