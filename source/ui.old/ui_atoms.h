/*
Copyright (C) 1997-2001 Id Software, Inc.

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
#ifndef __QMENU_H__
#define __QMENU_H__

extern vec4_t colorWarsowOrange;
extern vec4_t colorWarsowPurple;
extern vec4_t colorWarsowPurpleBright;
extern vec4_t colorWarsowPurple;

#define UI_COLOR_HIGHLIGHT  colorWarsowPurpleBright
#define UI_COLOR_LIVETEXT   colorWhite
#define UI_COLOR_DEADTEXT   colorWarsowOrangeBright

#define MAXMENUITEMS	    128

enum
{
	MTYPE_SLIDER
	, MTYPE_ACTION
	, MTYPE_SPINCONTROL
	, MTYPE_SEPARATOR
	, MTYPE_FIELD
	, MTYPE_SCROLLBAR

	, MTYPE_TOTAL
};

//wsw:will:field flags
#define F_NUMBERSONLY 1
#define F_PASSWORD 2
//#define F_NEXT 4

#define SLIDER_RANGE	    10

typedef struct
{
	char buffer[80];
	int cursor;
	int length;
	int width;
} menufield_t;

typedef struct
{
	struct shader_s *shader;
	struct shader_s *shaderHigh; // show when highlighed
	int xoffset;
	int yoffset;
	int width;
	int height;
	vec4_t color;
	vec4_t colorHigh;
} menupitcture_t;

typedef struct menucommon_s
{
	int type;
	const char *name; // string used as item id
	char title[MAX_STRING_CHARS]; // string print on screen
	int x, y;
	int mins[2], maxs[2];
	struct _tag_menuframework *parent;
	int cursor_offset;
	int localdata[4];
	int align;
	struct mufont_s *font;

	char *statusbar;

	menupitcture_t pict; // each menuitem can have a picture associated
	qboolean box;

	void ( *callback )( struct menucommon_s *self );
	void ( *callback_doubleclick )( struct menucommon_s *self );
	void ( *statusbarfunc )( struct menucommon_s *self );
	void ( *ownerdraw )( struct menucommon_s *self );
	void ( *cursordraw )( struct menucommon_s *self );

	int curvalue;
	int minvalue;
	int maxvalue;
	float range;
	int width;
	int height;
	int active_width;	// never used for drawing, only determines the clickable area
	int vspacing;
	char **itemnames;	// used by spincontrols
	void *itemlocal;	// for special expansions (field text buffer, etc)

	void *next;			// for registration
	int scrollbar_id;	// menuitem location of a corresponding scrollbar
	int sort_active;	// indicates the active sorting method
	int sort_type;		// the type of sorting the active instance uses

	//wsw:will:added for field flags
	int flags;

	qboolean disabled;
} menucommon_t;

typedef struct _tag_menuframework
{
	int x, y;
	int cursor;

	int nitems;
	int nslots;
	menucommon_t *items[MAXMENUITEMS];

	char *statusbar;

	void ( *cursordraw )( struct _tag_menuframework *m );

} menuframework_s;

//=======================================================

char *UI_ListNameForPosition( const char *namesList, int position, const char separator );
qboolean UI_CreateFileNamesListCvar( cvar_t *cvar, const char *path, const char *extension, const char separator );
qboolean UI_NamesListCvarAddName( const cvar_t *cvar, const char *name, const char separator );

//=======================================================
// scroll lists management
//=======================================================
typedef struct m_listitem_s
{
	char name[MAX_STRING_CHARS];
	struct m_listitem_s *pnext;
	int id;
	void *data;

} m_listitem_t;

typedef struct
{
	m_listitem_t *headNode;
	int numItems;
	char *item_names[32000];        //fixme
} m_itemslisthead_t;

void UI_FreeScrollItemList( m_itemslisthead_t *itemlist );
m_listitem_t *UI_FindItemInScrollListWithId( m_itemslisthead_t *itemlist, int itemid );
void UI_AddItemToScrollList( m_itemslisthead_t *itemlist, const char *name, void *data );

//=======================================================================
//=======================================================================
menucommon_t *UI_MenuItemByName( char *name );
char *UI_GetMenuitemFieldBuffer( menucommon_t *menuitem );
menucommon_t *UI_RegisterMenuItem( const char *name, int type );
menucommon_t *UI_InitMenuItem( const char *name, char *title, int x, int y, int type, int align, struct mufont_s *font, void ( *callback )(struct menucommon_s *) );
int UI_SetupButton( menucommon_t *menuitem, qboolean box );
menucommon_t *UI_SetupSlider( menucommon_t *menuitem, int width, int curvalue, int minvalue, int maxvalue );
menucommon_t *UI_SetupSpinControl( menucommon_t *menuitem, char **item_names, int curvalue );
menufield_t *UI_SetupField( menucommon_t *menuitem, const char *string, size_t length, int width );
void UI_SetupFlags( menucommon_t *menuitem, int flags );
menucommon_t *UI_SetupScrollbar( menucommon_t *menuitem, int height, int curvalue, int minvalue, int maxvalue );
//=======================================================================
//=======================================================================

qboolean Field_Key( menucommon_t *field, int key );
qboolean Field_CharEvent( menucommon_t *field, qwchar key );

int	UI_StringWidth( char *s, struct mufont_s *font );
int	UI_StringHeight( struct mufont_s *font );
void	UI_DrawString( int x, int y, int align, const char *str, int maxwidth, struct mufont_s *font, vec4_t color );

void	Menu_AddItem( menuframework_s *menu, void *item );
void	Menu_AdjustCursor( menuframework_s *menu, int dir );
void	Menu_Center( menuframework_s *menu );
void	Menu_Init( menuframework_s *menu, qboolean justify_buttons );
void	Menu_Draw( menuframework_s *menu );
menucommon_t *Menu_ItemAtCursor( menuframework_s *m );
qboolean Menu_SelectItem( menuframework_s *s );
void	Menu_SetStatusBar( menuframework_s *s, char *string );
qboolean Menu_SlideItem( menuframework_s *s, int dir, int key );
qboolean Menu_DragItem( menuframework_s *s, int dir, int key );
int	Menu_TallySlots( menuframework_s *menu );

#endif
