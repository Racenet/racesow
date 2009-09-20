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
#include <string.h>
#include <ctype.h>

#include "ui_local.h"

static void  Field_Init( menucommon_t *f );
static void  Action_DoEnter( menucommon_t *a );
static void  Action_Draw( menucommon_t *a );
static void  Menu_DrawStatusBar( char *string );
static void Menu_AdjustRectangle( int *mins, int *maxs );
static void  Separator_Draw( menucommon_t *s );
static void  Slider_DoSlide( menucommon_t *s, int dir );
static void  Slider_Draw( menucommon_t *s );
//static void	 SpinControl_DoEnter( menulist_s *s );
static void  SpinControl_Draw( menucommon_t *s );
static void  SpinControl_DoSlide( menucommon_t *s, int dir );
static void  Scrollbar_DoSlide( menucommon_t *s, int dir );
static void  Scrollbar_Draw( menucommon_t *s );

#define RCOLUMN_OFFSET  16
#define LCOLUMN_OFFSET -16

vec4_t colorWarsowOrange = { 0.95f, 0.33f, 0.1f, 1.0f };
vec4_t colorWarsowOrangeBright = { 1.0f, 0.6f, 0.1f, 1.0f };
vec4_t colorWarsowPurpleBright = { 0.66f, 0.66f, 1.0f, 1.0f };
vec4_t colorWarsowPurple = { 0.3843f, 0.2902f, 0.5843f, 1.0f };

/*
   ===============================================================================

   STRINGS DRAWING

   ===============================================================================
 */

//=============
//UI_FillRect
//=============
void UI_FillRect( int x, int y, int w, int h, vec4_t color )
{
	trap_R_DrawStretchPic( x, y, w, h, 0, 0, 1, 1, color, uis.whiteShader );
}

//=============
//UI_StringWidth
//=============
int UI_StringWidth( char *s, struct mufont_s *font )
{
	if( !font )
		font = uis.fontSystemSmall;
	return trap_SCR_strWidth( s, font, 0 );
}

//=============
//UI_StringHeight
//=============
int UI_StringHeight( struct mufont_s *font )
{
	if( !font )
		font = uis.fontSystemSmall;
	return trap_SCR_strHeight( font );
}

//=============
//UISCR_HorizontalAlignOffset
//=============
int UISCR_HorizontalAlignOffset( int align, int width )
{
	int nx = 0;

	if( align % 3 == 0 )  // left
		nx = 0;
	if( align % 3 == 1 )  // center
		nx = -( width / 2 );
	if( align % 3 == 2 )  // right
		nx = -width;

	return nx;
}

//=============
//UISCR_VerticalAlignOffset
//=============
int UISCR_VerticalAlignOffset( int align, int height )
{
	int ny = 0;

	if( align / 3 == 0 )  // top
		ny = 0;
	else if( align / 3 == 1 )  // middle
		ny = -( height / 2 );
	else if( align / 3 == 2 )  // bottom
		ny = -height;

	return ny;
}

/*
* UI_DrawBoxLayer
*/
static void UI_DrawBoxLayer( int x, int y, int width, int height, vec4_t color, qboolean border )
{
#define UI_BOX_IMAGE_SIZE 16
	int i, columns, rows, xoffset, yoffset;
	int extraHeight, extraWidth;
	float fracHeight = 0, fracWidth = 0;

	if( width < UI_BOX_IMAGE_SIZE * 2 )
		return;

	if( height < UI_BOX_IMAGE_SIZE * 2 )
		return;

	if( width == UI_BOX_IMAGE_SIZE * 2 )
		columns = 0;
	else
		columns = ( width - ( UI_BOX_IMAGE_SIZE * 2 ) ) / UI_BOX_IMAGE_SIZE;

	if( height == UI_BOX_IMAGE_SIZE * 2 )
		rows = 0;
	else
		rows = ( height - ( UI_BOX_IMAGE_SIZE * 2 ) ) / UI_BOX_IMAGE_SIZE;

	extraHeight = ( height - 2 * UI_BOX_IMAGE_SIZE ) - ( rows * UI_BOX_IMAGE_SIZE );
	clamp_low( extraHeight, 0 );
	if( extraHeight )
		fracHeight = (float)extraHeight / (float)UI_BOX_IMAGE_SIZE;

	extraWidth = ( width - 2 * UI_BOX_IMAGE_SIZE ) - ( columns * UI_BOX_IMAGE_SIZE );
	clamp_low( extraWidth, 0 );
	if( extraWidth )
		fracWidth = (float)extraWidth / (float)UI_BOX_IMAGE_SIZE;

	// upleft corner
	if( !border )
		trap_R_DrawStretchPic( x, y, UI_BOX_IMAGE_SIZE, UI_BOX_IMAGE_SIZE, 0, 0, 1, 1, color, uis.gfxBoxUpLeft );
	else
		trap_R_DrawStretchPic( x, y, UI_BOX_IMAGE_SIZE, UI_BOX_IMAGE_SIZE, 0, 0, 1, 1, color, uis.gfxBoxBorderUpLeft );

	// left side
	yoffset = UI_BOX_IMAGE_SIZE;
	for( i = 0; i < rows; i++, yoffset += UI_BOX_IMAGE_SIZE )
	{
		if( border )
			trap_R_DrawStretchPic( x, y + yoffset, UI_BOX_IMAGE_SIZE, UI_BOX_IMAGE_SIZE, 0, 0, 1, 1, color, uis.gfxBoxBorderLeft );
		else
			trap_R_DrawStretchPic( x, y + yoffset, UI_BOX_IMAGE_SIZE, UI_BOX_IMAGE_SIZE, 0, 0, 1, 1, color, uis.gfxBoxLeft );
	}

	if( extraHeight )
	{
		if( border )
			trap_R_DrawStretchPic( x, y + yoffset, UI_BOX_IMAGE_SIZE, extraHeight, 0, 0, 1, fracHeight, color, uis.gfxBoxBorderLeft );
		else
			trap_R_DrawStretchPic( x, y + yoffset, UI_BOX_IMAGE_SIZE, extraHeight, 0, 0, 1, fracHeight, color, uis.gfxBoxLeft );
	
		yoffset += extraHeight;
	}

	// left bottom corner
	if( border )
		trap_R_DrawStretchPic( x, y + ( height - UI_BOX_IMAGE_SIZE ), UI_BOX_IMAGE_SIZE, UI_BOX_IMAGE_SIZE, 0, 0, 1, 1, color, uis.gfxBoxBorderBottomLeft );
	else
		trap_R_DrawStretchPic( x, y + ( height - UI_BOX_IMAGE_SIZE ), UI_BOX_IMAGE_SIZE, UI_BOX_IMAGE_SIZE, 0, 0, 1, 1, color, uis.gfxBoxBottomLeft );

	// top and bottom sides
	xoffset = UI_BOX_IMAGE_SIZE;
	for( i = 0; i < columns; i++, xoffset += UI_BOX_IMAGE_SIZE )
	{
		if( border )
			trap_R_DrawStretchPic( x + xoffset, y, UI_BOX_IMAGE_SIZE, UI_BOX_IMAGE_SIZE, 0, 0, 1, 1, color, uis.gfxBoxBorderUp );
		else
			trap_R_DrawStretchPic( x + xoffset, y, UI_BOX_IMAGE_SIZE, UI_BOX_IMAGE_SIZE, 0, 0, 1, 1, color, uis.gfxBoxUp );
	
		if( border )
			trap_R_DrawStretchPic( x + xoffset, y + ( height - UI_BOX_IMAGE_SIZE ), UI_BOX_IMAGE_SIZE, UI_BOX_IMAGE_SIZE, 0, 0, 1, 1, color, uis.gfxBoxBorderBottom );
		else
			trap_R_DrawStretchPic( x + xoffset, y + ( height - UI_BOX_IMAGE_SIZE ), UI_BOX_IMAGE_SIZE, UI_BOX_IMAGE_SIZE, 0, 0, 1, 1, color, uis.gfxBoxBottom );
	
	}

	if( extraWidth )
	{
		if( border )
			trap_R_DrawStretchPic( x + xoffset, y, extraWidth, UI_BOX_IMAGE_SIZE, 0, 0, fracWidth, 1, color, uis.gfxBoxBorderUp );
		else
			trap_R_DrawStretchPic( x + xoffset, y, extraWidth, UI_BOX_IMAGE_SIZE, 0, 0, fracWidth, 1, color, uis.gfxBoxUp );

		if( border )
			trap_R_DrawStretchPic( x + xoffset, y + ( height - UI_BOX_IMAGE_SIZE ), extraWidth, UI_BOX_IMAGE_SIZE, 0, 0, fracWidth, 1, color, uis.gfxBoxBorderBottom );
		else
			trap_R_DrawStretchPic( x + xoffset, y + ( height - UI_BOX_IMAGE_SIZE ), extraWidth, UI_BOX_IMAGE_SIZE, 0, 0, fracWidth, 1, color, uis.gfxBoxBottom );
	}

	// top right corner
	if( border )
		trap_R_DrawStretchPic( x + ( width - UI_BOX_IMAGE_SIZE ), y, UI_BOX_IMAGE_SIZE, UI_BOX_IMAGE_SIZE, 0, 0, 1, 1, color, uis.gfxBoxBorderUpRight );
	else
		trap_R_DrawStretchPic( x + ( width - UI_BOX_IMAGE_SIZE ), y, UI_BOX_IMAGE_SIZE, UI_BOX_IMAGE_SIZE, 0, 0, 1, 1, color, uis.gfxBoxUpRight );

	// right side
	yoffset = UI_BOX_IMAGE_SIZE;
	for( i = 0; i < rows; i++, yoffset += UI_BOX_IMAGE_SIZE )
	{
		if( border )
			trap_R_DrawStretchPic( x + ( width - UI_BOX_IMAGE_SIZE ), y + yoffset, UI_BOX_IMAGE_SIZE, UI_BOX_IMAGE_SIZE, 0, 0, 1, 1, color, uis.gfxBoxBorderRight );
		else
			trap_R_DrawStretchPic( x + ( width - UI_BOX_IMAGE_SIZE ), y + yoffset, UI_BOX_IMAGE_SIZE, UI_BOX_IMAGE_SIZE, 0, 0, 1, 1, color, uis.gfxBoxRight );
	}

	if( extraHeight )
	{
		if( border )
			trap_R_DrawStretchPic( x + ( width - UI_BOX_IMAGE_SIZE ), y + yoffset, UI_BOX_IMAGE_SIZE, extraHeight, 0, 0, 1, fracHeight, color, uis.gfxBoxBorderRight );
		else
			trap_R_DrawStretchPic( x + ( width - UI_BOX_IMAGE_SIZE ), y + yoffset, UI_BOX_IMAGE_SIZE, extraHeight, 0, 0, 1, fracHeight, color, uis.gfxBoxRight );
	}

	// bottom right corner
	if( border )
		trap_R_DrawStretchPic( x + ( width - UI_BOX_IMAGE_SIZE ), y + ( height - UI_BOX_IMAGE_SIZE ), UI_BOX_IMAGE_SIZE, UI_BOX_IMAGE_SIZE, 0, 0, 1, 1, color, uis.gfxBoxBorderBottomRight );
	else
		trap_R_DrawStretchPic( x + ( width - UI_BOX_IMAGE_SIZE ), y + ( height - UI_BOX_IMAGE_SIZE ), UI_BOX_IMAGE_SIZE, UI_BOX_IMAGE_SIZE, 0, 0, 1, 1, color, uis.gfxBoxBottomRight );

	// fill
	if( !border )
		trap_R_DrawStretchPic( x + UI_BOX_IMAGE_SIZE, y + UI_BOX_IMAGE_SIZE, width - ( 2 * UI_BOX_IMAGE_SIZE ), height - ( 2 * UI_BOX_IMAGE_SIZE ), 0, 0, 1, 1, color, uis.whiteShader );
#undef UI_BOX_IMAGE_SIZE
}

/*
* UI_DrawBox
*/
void UI_DrawBox( int x, int y, int width, int height, vec4_t color, vec4_t lineColor, vec4_t shadowColor, vec4_t lineShadowColor )
{
	int shadowOffset = 2;

	if( !color )
		shadowColor = NULL;

	if( !lineColor )
		lineShadowColor = NULL;

	if( lineShadowColor )
		shadowColor = NULL;

	if( shadowColor )
		UI_DrawBoxLayer( x + shadowOffset, y + shadowOffset, width, height, shadowColor, qfalse );

	if( color )
		UI_DrawBoxLayer( x, y, width, height, color, qfalse );

	if( lineShadowColor )
		UI_DrawBoxLayer( x + shadowOffset, y + shadowOffset, width, height, lineShadowColor, qtrue );

	if( lineColor )
		UI_DrawBoxLayer( x, y, width, height, lineColor, qtrue );
}

/*
* UI_DrawBar
*/
void UI_DrawPicBar( int x, int y, int width, int height, int align, float percent, struct shader_s *shader, vec4_t backColor, vec4_t color )
{
	float widthFrac, heightFrac;
	x += UISCR_HorizontalAlignOffset( align, width );
	y += UISCR_VerticalAlignOffset( align, height );

	if( !shader )
		shader = uis.whiteShader;

	if( backColor )
		trap_R_DrawStretchPic( x, y, width, height, 0, 0, 1, 1, backColor, shader );

	if( !color )
		color = colorWhite;

	clamp( percent, 0, 100 );
	if( !percent )
		return;

	if( height > width )
	{
		widthFrac = 1.0f;
		heightFrac = percent / 100.0f;
	}
	else
	{
		widthFrac = percent / 100.0f;
		heightFrac = 1.0f;
	}

	trap_R_DrawStretchPic( x, y, (int)(width * widthFrac), (int)(height * heightFrac), 0, 0, widthFrac, heightFrac, color, shader );
}

/*
* UI_DrawBar
*/
void UI_DrawBar( int x, int y, int width, int height, int align, float percent, vec4_t backColor, vec4_t color )
{
	UI_DrawPicBar( x, y, width, height, align, percent, uis.whiteShader, backColor, color );
}

//=============
//UI_DrawStringHigh
//This string is highlighted by the mouse
//=============
void UI_DrawStringHigh( int x, int y, int align, const char *str, int maxwidth, struct mufont_s *font, vec4_t color )
{
	int shadowoffset = 1;
	if( !font )
		font = uis.fontSystemSmall;

	shadowoffset += ( trap_SCR_strHeight( font ) >= trap_SCR_strHeight( uis.fontSystemBig ) );

	if( maxwidth > 0 )
	{
		//trap_SCR_DrawStringWidth( x+shadowoffset, y+shadowoffset, align, COM_RemoveColorTokens( str ), maxwidth, font, colorBlack );
		//trap_SCR_DrawStringWidth( x, y, align, COM_RemoveColorTokens( str ), maxwidth, font, UI_COLOR_HIGHLIGHT );
		x += UISCR_HorizontalAlignOffset( align, maxwidth );
		y += UISCR_VerticalAlignOffset( align, trap_SCR_strHeight( font ) );
		trap_SCR_DrawClampString( x+shadowoffset, y+shadowoffset, COM_RemoveColorTokens( str ), x+shadowoffset, y+shadowoffset, x+shadowoffset+maxwidth, y+shadowoffset+trap_SCR_strHeight( font ), font, colorBlack );
		trap_SCR_DrawClampString( x, y, COM_RemoveColorTokens( str ), x, y, x+maxwidth, y+trap_SCR_strHeight( font ), font, UI_COLOR_HIGHLIGHT );
	}
	else
	{
		trap_SCR_DrawString( x+shadowoffset, y+shadowoffset, align, str, font, colorBlack );
		trap_SCR_DrawString( x, y, align, str, font, UI_COLOR_HIGHLIGHT );
	}
}

//=============
//UI_DrawString
//=============
void UI_DrawString( int x, int y, int align, const char *str, int maxwidth, struct mufont_s *font, vec4_t color )
{
	if( !font )
		font = uis.fontSystemSmall;

	if( maxwidth > 0 )
	{
		//trap_SCR_DrawStringWidth( x, y, align, str, maxwidth, font, color );
		x += UISCR_HorizontalAlignOffset( align, maxwidth );
		y += UISCR_VerticalAlignOffset( align, trap_SCR_strHeight( font ) );
		trap_SCR_DrawClampString( x, y, str, x, y, x + maxwidth, y + trap_SCR_strHeight( font ), font, color );
	}
	else
		trap_SCR_DrawString( x, y, align, str, font, color );
}

//=============
//UI_DrawStringRow_
//=============
static void UI_DrawStringRow_( int x, int y, int align, const char *str, int maxwidth, struct mufont_s *font, vec4_t color, 
	void ( *draw )( int, int, int, const char *, int, struct mufont_s *, vec4_t ) )
{
	int curwidth, width, totalwidth;
	static char stritem[MAX_STRING_CHARS];
	const char *pos, *lastpos;

	pos = lastpos = str;
	Q_strncpyz( stritem, str, sizeof( stritem ) );

	// scan for \\w:123\\ sequences, taking 123 as the maximum width for the next string
	totalwidth = width = 0;
	while( (pos = strstr( lastpos, "\\w:" )) )
	{
		stritem[pos - str] = '\0';
		draw( x + totalwidth, y, align, stritem + (lastpos - str), width ? width : max( maxwidth - totalwidth, 0 ), font, color );

		curwidth = UI_StringWidth( stritem + (lastpos - str), font );
		totalwidth += width ? width : curwidth;

		width = 0;
		lastpos = pos + 3;
		pos = strstr( lastpos, "\\" );
		if( pos )
		{
			stritem[pos - str] = '\0';
			width = atoi( stritem + (lastpos - str) );
			lastpos = pos + 1; 
		}
	}

	if( !maxwidth || maxwidth > totalwidth )
		draw( x + totalwidth, y, align, stritem + (lastpos - str), max( maxwidth - totalwidth, 0 ), font, color );
}

//=============
//UI_DrawStringRow
//=============
static void UI_DrawStringRow( int x, int y, int align, const char *str, int maxwidth, struct mufont_s *font, vec4_t color )
{
	UI_DrawStringRow_( x, y, align, str, maxwidth, font, color, UI_DrawString );
}

//=============
//UI_DrawStringRowHigh
//=============
static void UI_DrawStringRowHigh( int x, int y, int align, const char *str, int maxwidth, struct mufont_s *font, vec4_t color )
{
	UI_DrawStringRow_( x, y, align, str, maxwidth, font, color, UI_DrawStringHigh );
}

//=================================================================
//
//		MENUITEMS
//
//=================================================================

//=================================================================
// ACTION
//=================================================================

static void Action_UpdateBox( menucommon_t *a )
{
	// horizontal mins
	if( a->active_width > 0 )
		a->mins[0] = a->x + a->parent->x + UISCR_HorizontalAlignOffset( a->align, a->active_width );
	else if( a->width )
		a->mins[0] = a->x + a->parent->x + UISCR_HorizontalAlignOffset( a->align, a->width );
	else if( a->font )
		a->mins[0] = a->x + a->parent->x + UISCR_HorizontalAlignOffset( a->align, UI_StringWidth( a->title, a->font ) );
	else
		a->mins[0] = a->x + a->parent->x + UISCR_HorizontalAlignOffset( a->align, 128 );

	// horizontal maxs
	if( a->active_width > 0 )
		a->maxs[0] = a->mins[0] + a->active_width;
	else if( a->width > 0 )
		a->maxs[0] = a->mins[0] + a->width;
	else if( a->font )
		a->maxs[0] = a->mins[0] + UI_StringWidth( a->title, a->font );
	else
		a->maxs[0] = a->mins[0] + 128;

	// vertical mins

	if( a->height )
		a->mins[1] = a->y + a->parent->y + UISCR_VerticalAlignOffset( a->align, a->height );
	else if( a->font )
		a->mins[1] = a->y + a->parent->y + UISCR_VerticalAlignOffset( a->align, UI_StringHeight( a->font ) );
	else
		a->mins[1] = a->y + a->parent->y + UISCR_VerticalAlignOffset( a->align, 32 );

	// vertical maxs
	if( a->height > 0 )
		a->maxs[1] = a->mins[1] + a->height;
	else if( a->font )
		a->maxs[1] = a->mins[1] + UI_StringHeight( a->font );
	else
		a->maxs[1] = a->mins[1] + 64;

}

static void Action_Init( menucommon_t *a )
{
	Action_UpdateBox( a );
}

void Action_DoEnter( menucommon_t *menuitem )
{
	if( menuitem->disabled )
		return;

	if( menuitem->callback )
		menuitem->callback( menuitem );
}

void Action_Draw( menucommon_t *menuitem )
{
	int x, y, textxoffset, textyoffset;
	int width, height;

	x = menuitem->x + menuitem->parent->x;
	y = menuitem->y + menuitem->parent->y;
	
	height = menuitem->height;
	width = menuitem->width;

	if( menuitem->height )
		textyoffset = (menuitem->height - UI_StringHeight( menuitem->font )) / 2;
	else
		textyoffset = 0;

	if( menuitem->box && menuitem->width )
		textxoffset = (menuitem->width - UI_StringWidth( menuitem->title, menuitem->font )) / 2;
	else
		textxoffset = 0;

	// update box for string size
	Action_UpdateBox( menuitem );

	if( menuitem->box )
	{
		float *color;
		int xoffset, yoffset;

		xoffset = UISCR_HorizontalAlignOffset( menuitem->align, width );
		yoffset = UISCR_VerticalAlignOffset( menuitem->align, menuitem->height );

		if( !menuitem->disabled && Menu_ItemAtCursor( menuitem->parent ) == menuitem )
			color = UI_COLOR_HIGHLIGHT;
		else
			color = colorWarsowPurple;

		if( menuitem->disabled )
			UI_DrawBox( x + xoffset, y + yoffset, width, menuitem->height, color, NULL, NULL, colorDkGrey );
		else if( Menu_ItemAtCursor( menuitem->parent ) == menuitem )
			UI_DrawBox( x + xoffset, y + yoffset, width, menuitem->height, color, colorWhite, NULL, colorDkGrey );
		else
			UI_DrawBox( x + xoffset, y + yoffset, width, menuitem->height, color, UI_COLOR_HIGHLIGHT, NULL, colorDkGrey );

		if( menuitem->ownerdraw )
			menuitem->ownerdraw( menuitem );

		if( menuitem->disabled )
		{
			UI_DrawStringRow( x + textxoffset, y + textyoffset, menuitem->align, menuitem->title, width, menuitem->font, colorMdGrey );
		}
		else
		{
			UI_DrawStringRow( x + textxoffset, y + textyoffset, menuitem->align, menuitem->title, width, menuitem->font, colorWhite );
		}
	}
	else
	{
		if( menuitem->ownerdraw )
			menuitem->ownerdraw( menuitem );

		if( menuitem->disabled )
		{
			UI_DrawStringRow( x + textxoffset, y + textyoffset, menuitem->align, menuitem->title, width, menuitem->font, colorMdGrey );
		}
		else if( Menu_ItemAtCursor( menuitem->parent ) == menuitem )
		{
			UI_DrawStringRowHigh( x + textxoffset, y + textyoffset, menuitem->align, menuitem->title, width, menuitem->font, colorWhite );
		}
		else
		{
			UI_DrawStringRow( x + textxoffset, y + textyoffset, menuitem->align, menuitem->title, width, menuitem->font, UI_COLOR_LIVETEXT );
		}
	}
}

//=================================================================
// FIELD
//=================================================================
static void Field_ResetCursor( menucommon_t *f )
{
	menufield_t *itemlocal;
	itemlocal = (menufield_t *)f->itemlocal;

	itemlocal->cursor = strlen( itemlocal->buffer );
	if( itemlocal->cursor > itemlocal->length )
		itemlocal->cursor = itemlocal->length;
}

static void Field_SetupBox( menucommon_t *f )
{
	menufield_t *itemlocal;
	itemlocal = (menufield_t *)f->itemlocal;

	f->mins[0] = f->x + f->parent->x + 16;
	f->maxs[0] = f->mins[0] + itemlocal->width;

	f->mins[1] = f->y + f->parent->y;
	f->maxs[1] = f->mins[1] + UI_StringHeight( f->font );
}

static void Field_Init( menucommon_t *f )
{
	Field_SetupBox( f );
}

static qboolean Field_DoEnter( menucommon_t *f )
{
	if( f->disabled )
		return qtrue;

	if( f->callback )
	{
		f->callback( f );
		return qtrue;
	}
	return qfalse;
}

static void Field_Draw( menucommon_t *f )
{
	int x, y;
	char tempbuffer[128] = "";
	menufield_t *itemlocal;

	int offset, xcursor;
	char *str;

	itemlocal = (menufield_t *)f->itemlocal;
	if( !itemlocal )
		return;

	Field_SetupBox( f ); // update box

	x = f->x + f->parent->x + LCOLUMN_OFFSET;
	y = f->y + f->parent->y;

	if( f->title[0] )
	{
		UI_DrawString( x, y, f->align, f->title, 0, f->font, UI_COLOR_DEADTEXT );
	}

	x = f->x + f->parent->x + 16;
	y = f->y + f->parent->y;

	{
		float color[4] = { 0.5, 0.5, 0.5, 0.5 };
		UI_FillRect( x, y, itemlocal->width, trap_SCR_strHeight( f->font ), color );
	}

	if( f->disabled )
		return;

	if( Menu_ItemAtCursor( f->parent ) == f )
	{

		Q_strncpyz( tempbuffer, itemlocal->buffer, sizeof( tempbuffer ) );

		// password protect fields, replace with '*'
		if( f->flags & F_PASSWORD )
		{
			str = tempbuffer;
			while( *str )
				*str++ = '*';
		}

		str = tempbuffer;
		while( *str && ( (int)trap_SCR_strWidth( str, f->font, 0 ) > itemlocal->width - 16 ) )
		{
			str++;
		}

		offset = str - tempbuffer;
		if( itemlocal->cursor < offset )
			Field_ResetCursor( f ); // force the cursor to be always at the end of the string

		UI_DrawString( x, y, ALIGN_LEFT_TOP, str, 0, f->font, UI_COLOR_LIVETEXT );
		//draw cursor
		if( ( int ) ( uis.time / 250 ) & 1 )
		{
			xcursor = trap_SCR_strWidth( str, f->font, itemlocal->cursor );
			UI_DrawString( x+xcursor, y, ALIGN_LEFT_TOP, "_", 0, f->font, UI_COLOR_LIVETEXT );
		}
	}
	else
	{
		x = f->x + f->parent->x + 16;
		y = f->y + f->parent->y;

		Q_strncpyz( tempbuffer, itemlocal->buffer, sizeof( tempbuffer ) );
		// password protect fields, replace with '*'
		if( f->flags & F_PASSWORD )
		{
			str = tempbuffer;
			while( *str )
				*str++ = '*';
		}
		UI_DrawString( x, y, ALIGN_LEFT_TOP, tempbuffer, 0, f->font, colorLtGrey );
	}
}

qboolean Field_Key( menucommon_t *f, int key )
{
	menufield_t *itemlocal;
	itemlocal = (menufield_t *)f->itemlocal;
	if( !itemlocal )
		return qfalse;

	if( f->disabled )
		return qfalse;

	/*
	** support pasting from the clipboard
	*/
	if( ( toupper( key ) == 'V' && trap_Key_IsDown( K_CTRL ) ) ||
	   ( ( ( key == K_INS ) || ( key == KP_INS ) ) && trap_Key_IsDown( K_SHIFT ) ) )
	{
		char *cbd, *p;

		cbd = trap_CL_GetClipboardData( (qboolean)( key == K_INS || key == KP_INS ) );

		if( cbd )
		{
			p = strpbrk( cbd, "\n\r\b" );
			if( p )
				*p = 0;

			Q_strncpyz( itemlocal->buffer, cbd, sizeof( itemlocal->buffer ) );
			Field_ResetCursor( f );
			//Field_DoEnter( f ); // exec callback

			trap_CL_FreeClipboardData( cbd );
		}
		return qtrue;
	}

	switch( key )
	{
	case K_LEFTARROW:
	case K_BACKSPACE:
		if( itemlocal->cursor > 0 )
		{
			memmove( &itemlocal->buffer[itemlocal->cursor-1], &itemlocal->buffer[itemlocal->cursor], strlen( &itemlocal->buffer[itemlocal->cursor] ) + 1 );
			Field_ResetCursor( f );
			//Field_DoEnter( f ); // exec callback
		}
		return qtrue;

	case KP_DEL:
	case K_DEL:
	{
		memmove( &itemlocal->buffer[itemlocal->cursor], &itemlocal->buffer[itemlocal->cursor+1], strlen( &itemlocal->buffer[itemlocal->cursor+1] ) + 1 );
		Field_ResetCursor( f );
		//Field_DoEnter( f ); // exec callback
	}
		return qtrue;

	case KP_HOME:
	case KP_UPARROW:
	case KP_PGUP:
	case KP_LEFTARROW:
	case KP_5:
	case KP_RIGHTARROW:
	case KP_END:
	case KP_DOWNARROW:
	case KP_PGDN:
		return qtrue; // ignore keypad stuff, so it can be used for writing numbers (little ugly)
	}

	return qfalse;
}

qboolean Field_CharEvent( menucommon_t *f, qwchar key )
{
	menufield_t *itemlocal;
	itemlocal = (menufield_t *)f->itemlocal;
	if( !itemlocal )
		return qfalse;

	if( f->disabled )
		return qfalse;

	if( key < 32 || key > 126 )
		return qfalse; // non printable

	// jalfixme: reimplement numbers only option?
	//if ( !isdigit( key ) && ( f->flags & QMF_NUMBERSONLY ) )
	//	return qfalse;
	if( !isdigit( key ) && f->flags & F_NUMBERSONLY )
		return qfalse;

	if( itemlocal->cursor < itemlocal->length )
	{
		itemlocal->buffer[itemlocal->cursor++] = key;
		itemlocal->buffer[itemlocal->cursor] = 0;
		Field_SetupBox( f );
		//Field_DoEnter( f ); // exec callback
	}

	return qtrue;
}

//=================================================================
// SEPARATOR
//=================================================================
static void Separator_Init( menucommon_t *s )
{
}

void Separator_Draw( menucommon_t *s )
{
	int x, y;

	if( s->ownerdraw )
		s->ownerdraw( s );

	if( !s->title[0] )
	{
		return;
	}

	x = s->x + s->parent->x;
	y = s->y + s->parent->y;

	//UI_DrawString( x, y, s->align, s->title, s->width, s->font, UI_COLOR_DEADTEXT );
	UI_DrawStringRow( x, y, s->align, s->title, s->width, s->font, UI_COLOR_DEADTEXT );
}

//=================================================================
// SLIDER
//=================================================================
#define SCROLLBAR_PIC_SIZE 16

static void Slider_Init( menucommon_t *s )
{
	s->align = ALIGN_LEFT_TOP;

	s->mins[0] = s->x + s->parent->x + RCOLUMN_OFFSET;
	s->maxs[0] = s->mins[0] + s->width * SCROLLBAR_PIC_SIZE + SCROLLBAR_PIC_SIZE;

	s->mins[1] = s->y + s->parent->y;
	s->maxs[1] = s->mins[1] + SCROLLBAR_PIC_SIZE;
}

void Slider_Draw( menucommon_t *s )
{
	int i, x, y;
	vec4_t buttoncolor, color;
	int cursorxpos;

	Slider_Init( s ); //update box size

	x = s->x + s->parent->x;
	y = s->y + s->parent->y;

	//draw slider title, if any
	if( s->title[0] )
		UI_DrawString( x + LCOLUMN_OFFSET, y, ALIGN_RIGHT_TOP, s->title, 0, s->font, UI_COLOR_DEADTEXT );

	//The title is offset LCOLUMN_OFFSET, then everything else is offset RCOLUMN_OFFSET
	x += RCOLUMN_OFFSET; //move the whole thing to the right 16 units to right-align

	if( s->maxvalue > s->minvalue )
	{
		s->range = (float)( s->curvalue - s->minvalue ); // reserve space for the buttons
		s->range /= (float)( s->maxvalue - s->minvalue );
		clamp( s->range, 0, 1 );
	}
	else
		s->range = 0;

	if( s->disabled )
		Vector4Copy( colorMdGrey, color );
	else
		Vector4Copy( colorWhite, color );

	trap_R_DrawStretchPic( x, y, SCROLLBAR_PIC_SIZE, SCROLLBAR_PIC_SIZE, 0, 0, 1, 1,
	                      color, uis.gfxSlidebar_1 );

	for( i = 1; i < s->width-1; i++ )
		trap_R_DrawStretchPic( x + i*SCROLLBAR_PIC_SIZE, y, SCROLLBAR_PIC_SIZE, SCROLLBAR_PIC_SIZE, 0, 0, 1, 1,
		                      color, uis.gfxSlidebar_2 );

	trap_R_DrawStretchPic( x + s->width*SCROLLBAR_PIC_SIZE - SCROLLBAR_PIC_SIZE, y, SCROLLBAR_PIC_SIZE, SCROLLBAR_PIC_SIZE, 0, 0, 1, 1,
	                      color, uis.gfxSlidebar_3 );

	if( s->disabled )
		Vector4Copy( colorMdGrey, buttoncolor );
	else if( Menu_ItemAtCursor( s->parent ) == s )
		Vector4Copy( UI_COLOR_HIGHLIGHT, buttoncolor );
	else
		Vector4Copy( colorWhite, buttoncolor );

	cursorxpos = x + SCROLLBAR_PIC_SIZE // start with offset for the button
	             + ( ( s->width-3 ) * SCROLLBAR_PIC_SIZE * s->range );

	trap_R_DrawStretchPic( cursorxpos, y, SCROLLBAR_PIC_SIZE, SCROLLBAR_PIC_SIZE, 0, 0, 1, 1,
	                      buttoncolor, uis.gfxSlidebar_4 );
}

void Slider_DoSlide( menucommon_t *s, int dir )
{
	int min, max;
	float newrange;
	float value;

	//empty or erroneous
	if( s->width <= 0 )
		return;

	if( s->disabled )
		return;

	//offset 16 to match spincontrol alignment
	min = s->x + s->parent->x + SCROLLBAR_PIC_SIZE + RCOLUMN_OFFSET;
	max = min + ( ( s->width-1 ) * SCROLLBAR_PIC_SIZE ) - SCROLLBAR_PIC_SIZE;
	//empty or erroneous
	if( max < min )
		UI_Error( "Invalid slidebar range: 'min < max'" );

	// see if the mouse is touching the step buttons
	if( uis.cursorX < min || dir < 0 )
	{
		s->curvalue--;
	}
	else if( uis.cursorX > max || dir > 0 )
	{
		s->curvalue++;
	}
	else
	{
		newrange = uis.cursorX - min;
		newrange /= (float)( max - min );

		clamp( newrange, 0.0, 1.0 );

		value = newrange * (float)( s->maxvalue - s->minvalue ) + s->minvalue;
		if( value - (int)value > 0.5 )
			value = (int)value + 1;
		else
			value = (int)value;

		s->curvalue = (int)value;
	}

	if( s->curvalue > s->maxvalue )
		s->curvalue = s->maxvalue;
	else if( s->curvalue < s->minvalue )
		s->curvalue = s->minvalue;

	if( s->callback )
		s->callback( s );

}

//=================================================================
// SCROLLBAR
//=================================================================

static void Scrollbar_Init( menucommon_t *s )
{
	int y_size;

	y_size = trap_SCR_strHeight( s->font ); //y_size is used to stretch the graphic to the current font size's height

	s->align = ALIGN_LEFT_TOP;

	s->mins[0] = s->x + s->parent->x;
	s->maxs[0] = s->mins[0] + SCROLLBAR_PIC_SIZE;

	s->mins[1] = s->y + s->parent->y;
	s->maxs[1] = s->mins[1] + s->height * y_size;
}

void Scrollbar_DoSlide( menucommon_t *s, int dir )
{
	int min, max, y_size, active_scroll, cv_pos;

	//This is pretty ugly, but it is all correct, modify with caution
	y_size = s->vspacing ? s->vspacing : trap_SCR_strHeight( s->font ); //y_size is used to stretch the graphic to the current font size's height
	active_scroll = s->maxvalue < 1 ? ( s->height - 2 ) * y_size : ( ( s->height - 2 ) * y_size ) * ( (float)( s->height - 2 ) / (float)( s->height - 2 + s->maxvalue ) );
	cv_pos = s->y + s->parent->y + y_size // start with offset for the button
	         + s->range * ( ( s->height - 2 ) * y_size - active_scroll ); //limit the range to the space that "active area" isn't using

	//empty or erroneous
	if( s->height <= 0 )
		return;

	min = s->y + s->parent->y + y_size; // extra pics reserved for the buttons
	max = min + ( ( s->height-1 ) * y_size ) - y_size;

	//empty or erroneous
	if( max < min )
		UI_Error( "Invalid scrollbar range: 'min < max'" );

	if( dir < 3 && dir > -3 && Menu_ItemAtCursor( s->parent ) == s && ( uis.cursorY > min && uis.cursorY < max ) )
	{
		if( uis.cursorY < cv_pos )
			s->curvalue = s->curvalue - s->height + 1; //first line gets moved to the last
		else if( uis.cursorY > cv_pos + active_scroll )
			s->curvalue = s->curvalue + s->height - 1; //last line gets moved to the first
	}
	else
	{
		if( uis.cursorY < min && Menu_ItemAtCursor( s->parent ) == s && dir == 1 )
			dir = -1;
		s->curvalue += dir;
	}

	if( s->curvalue > s->maxvalue )
		s->curvalue = s->maxvalue;
	else if( s->curvalue < s->minvalue )
		s->curvalue = s->minvalue;

	if( s->callback )
		s->callback( s );
}

void Scrollbar_Draw( menucommon_t *s )
{
	int i, x, y, y_size, active_scroll, cv_pos;
	vec4_t buttoncolor;

	Scrollbar_Init( s ); //update box size

	x = s->x + s->parent->x;
	y = s->y + s->parent->y;
	y_size = s->vspacing ? s->vspacing : trap_SCR_strHeight( s->font ); //y_size is used to stretch the graphic to the current font size's height

	//scrollbar title is for identification only, never draw it
	s->curvalue = trap_Cvar_Value( s->title );

	if( s->maxvalue > s->minvalue )
	{
		s->range = (float)( s->curvalue - s->minvalue ); // reserve space for the buttons
		s->range /= (float)( s->maxvalue - s->minvalue );
		clamp( s->range, 0, 1 );
	}
	else
		s->range = 0;

	trap_R_DrawStretchPic( x, y, SCROLLBAR_PIC_SIZE, y_size, 0, 0, 1, 1,
	                      colorWhite, uis.gfxScrollbar_1 );

	for( i = 1; i < s->height-1; i += 1 )
		trap_R_DrawStretchPic( x, y + i*y_size, SCROLLBAR_PIC_SIZE, y_size, 0, 0, 1, 1,
		                      colorWhite, uis.gfxScrollbar_2 );

	trap_R_DrawStretchPic( x, y + s->height*y_size - y_size, SCROLLBAR_PIC_SIZE, y_size, 0, 0, 1, 1,
	                      colorWhite, uis.gfxScrollbar_3 );

	if( Menu_ItemAtCursor( s->parent ) == s )
	{
		Vector4Copy( UI_COLOR_HIGHLIGHT, buttoncolor );
	}
	else
	{
		Vector4Copy( colorWhite, buttoncolor );
	}

	//This is pretty ugly, but it is all correct, modify with caution
	active_scroll = s->maxvalue < 1 ? ( s->height - 2 ) * y_size : ( ( s->height - 2 ) * y_size ) * ( (float)( s->height - 2 ) / (float)( s->height - 2 + s->maxvalue ) );
	cv_pos = s->y + s->parent->y + y_size // start with offset for the button
	         + s->range * ( ( s->height - 2 ) * y_size - active_scroll ); //limit the range to the space that "active area" isn't using

	trap_R_DrawStretchPic( x, cv_pos, SCROLLBAR_PIC_SIZE, active_scroll, 0, 0, 1, 1,
	                      buttoncolor, uis.gfxScrollbar_4 );

	if( s->callback )
		s->callback( s );
}

//=================================================================
// SPINCONTROL
//=================================================================
static void SpinControl_Init( menucommon_t *s )
{
	char buffer[100] = { 0 };
	int ysize, xsize, spacing, len;
	char **n;

	//set the curvalue of all non-selected sorts to 1 so the first click sorts down
	if( s->sort_active && s->sort_type )
	{
		int i;
		if( s->sort_active == s->sort_type )
		{
			for( i = 0; i < s->parent->nitems; i++ )
			{
				if( s->parent->items[i]->sort_active )  // only do this against spincontrols that are using sorting
					s->parent->items[i]->sort_active = s->sort_type;
				if( s->parent->items[i]->sort_active && s->parent->items[i]->sort_type != s->sort_type )
					s->parent->items[i]->curvalue = 1; // always make the first sort click switch to sort down
			}
		}
	}

	n = s->itemnames;

	if( !n )
		return;

	if( s->title[0] != 0 )
		s->mins[0] = s->x + s->parent->x + RCOLUMN_OFFSET;
	else
		s->mins[0] = s->x + s->parent->x;

	s->mins[1] = s->y + s->parent->y;

	ysize = UI_StringHeight( s->font );
	spacing = UI_StringHeight( s->font );

	xsize = 0;

	while( *n )
	{
		if( !strchr( *n, '\n' ) )
		{
			len = UI_StringWidth( *n, s->font );
			xsize = max( xsize, len );
		}
		else
		{

			Q_strncpyz( buffer, *n, sizeof( buffer ) );
			*strchr( buffer, '\n' ) = 0;
			len = UI_StringWidth( buffer, s->font );
			xsize = max( xsize, len );

			ysize = ysize + spacing;
			Q_strncpyz( buffer, strchr( *n, '\n' ) + 1, sizeof( buffer ) );
			len = UI_StringWidth( buffer, s->font );
			xsize = max( xsize, len );
		}

		n++;
	}

	if( s->align == ALIGN_CENTER_BOTTOM || s->align == ALIGN_CENTER_TOP || s->align == ALIGN_CENTER_MIDDLE )
		s->mins[0] -= xsize / 2;

	s->maxs[0] = s->mins[0] + xsize;
	s->maxs[1] = s->mins[1] + ysize;

	if( s->title != 0 )
	{                  //sort spincontrols have graphics which can be clicked on
		s->maxs[0] += s->pict.width * 2; //*2 to account for the space between the pict and the text
	}
}

/*
   void SpinControl_DoEnter( menulist_s *s )
   {
   s->curvalue++;
   if ( s->itemnames[s->curvalue] == 0 )
   s->curvalue = 0;

   if ( s->generic.callback )
   s->generic.callback( s );
   }
 */

void SpinControl_DoSlide( menucommon_t *s, int dir )
{
	if( s->disabled )
		return;

	s->curvalue += dir;

	if( s->maxvalue == 0 )
		s->curvalue = 0;
	else if( s->curvalue < 0 )
		s->curvalue = s->maxvalue;
	else if( s->itemnames[s->curvalue] == 0 )
		s->curvalue = 0;

	//set the curvalue of all non-selected sorts to 1 so the first click sorts down
	if( s->sort_active && s->sort_type )
	{
		int i;
		s->sort_active = s->sort_type;
		for( i = 0; i < s->parent->nitems; i++ )
		{
			if( s->parent->items[i]->sort_active )  // only do this against spincontrols that are using sorting
				s->parent->items[i]->sort_active = s->sort_type;
			if( s->parent->items[i]->sort_active && s->parent->items[i]->sort_type != s->sort_type )
				s->parent->items[i]->curvalue = 1; // always make the first sort click switch to sort down
		}
	}

	if( s->callback )
		s->callback( s );
}

void SpinControl_Draw( menucommon_t *s )
{
	char buffer[100];
	int x, y;
	qboolean font_highlighted = qfalse;

	SpinControl_Init( s ); //update box size

	x = s->x + s->parent->x;
	y = s->y + s->parent->y;

	if( s->title[0] )
	{               //only offset it if there is a title, allows for sorting
		x = s->x + s->parent->x + LCOLUMN_OFFSET;
		UI_DrawString( x, y, s->align, s->title, 0, s->font, UI_COLOR_DEADTEXT );
		x = s->x + s->parent->x + RCOLUMN_OFFSET;
	}

	if( Menu_ItemAtCursor( s->parent ) == s )
		font_highlighted = qtrue;

	if( !strchr( s->itemnames[s->curvalue], '\n' ) )
	{
		//start with sort spincontrols
		if( s->sort_active && s->sort_active == s->sort_type )
		{
			Vector4Copy( UI_COLOR_DEADTEXT, s->pict.color );
			Vector4Copy( UI_COLOR_HIGHLIGHT, s->pict.colorHigh );
			s->pict.yoffset = 2;
			s->pict.width = s->pict.height = 10;
			if( s->curvalue == 0 )
				s->pict.shader = s->pict.shaderHigh = uis.gfxArrowDown;
			else
				s->pict.shader = s->pict.shaderHigh = uis.gfxArrowUp;
		}
		else if( s->sort_active )
			s->pict.shader = s->pict.shaderHigh = NULL;

		if( s->disabled )
			UI_DrawString( x, y, ALIGN_LEFT_TOP, ( char *)s->itemnames[s->curvalue], 0, s->font, colorMdGrey );
		else if( font_highlighted )
			UI_DrawStringHigh( x, y, ALIGN_LEFT_TOP, ( char *)s->itemnames[s->curvalue], 0, s->font, colorWhite );
		else if( s->sort_active && s->sort_active == s->sort_type )  //sort spingcontrols get special color if active
			UI_DrawString( x, y, ALIGN_LEFT_TOP, ( char *)s->itemnames[s->curvalue], 0, s->font, UI_COLOR_DEADTEXT );
		else
			UI_DrawString( x, y, ALIGN_LEFT_TOP, ( char *)s->itemnames[s->curvalue], 0, s->font, UI_COLOR_LIVETEXT );
	}
	else
	{
		Q_strncpyz( buffer, s->itemnames[s->curvalue], sizeof( buffer ) );
		*strchr( buffer, '\n' ) = 0;
		if( font_highlighted )
			UI_DrawStringHigh( x, y, s->align, buffer, 0, s->font, colorWhite );
		else
			UI_DrawString( x, y, s->align, buffer, 0, s->font, UI_COLOR_LIVETEXT );

		Q_strncpyz( buffer, strchr( s->itemnames[s->curvalue], '\n' ) + 1, sizeof( buffer ) );
		if( font_highlighted )
			UI_DrawStringHigh( x, y + UI_StringHeight( s->font ), ALIGN_LEFT_TOP, buffer, 0, s->font, colorWhite );
		else
			UI_DrawString( x, y + UI_StringHeight( s->font ), ALIGN_LEFT_TOP, buffer, 0, s->font, UI_COLOR_LIVETEXT );
	}
}

//=================================================================
//
// MENUITEM ASSOCIATED ELEMENTS DRAWING (Generic to all types)
//
//=================================================================
static void MenuItem_DrawPict( menucommon_t *menuitem )
{
	int x, y;
	x = menuitem->x + menuitem->parent->x;
	y = menuitem->y + menuitem->parent->y;

	x += menuitem->pict.xoffset;
	y += menuitem->pict.yoffset;

	if( Menu_ItemAtCursor( menuitem->parent ) == menuitem )
	{
		if( menuitem->pict.shaderHigh )
		{
			trap_R_DrawStretchPic( x, y, menuitem->pict.width, menuitem->pict.height, 0, 0, 1, 1,
			                       menuitem->pict.colorHigh, menuitem->pict.shaderHigh );

			return;
		}
	}

	if( menuitem->pict.shader )
	{
		trap_R_DrawStretchPic( x, y, menuitem->pict.width, menuitem->pict.height, 0, 0, 1, 1,
		                       menuitem->pict.color, menuitem->pict.shader );
	}
}

static void Menu_DrawWindowedBackground( menuframework_s *menu )
{
	// it works, but needs to be improved
	int i;
	vec2_t mins, maxs;
	static vec4_t colorback = { .0f, .0f, .0f, 0.6f };

	mins[0] = uis.vidWidth;
	mins[1] = uis.vidHeight;
	maxs[0] = maxs[1] = 0;
	for( i = 0; i < menu->nitems; i++ )
	{
		if( menu->items[i]->mins[0] && menu->items[i]->mins[0] < mins[0] )
			mins[0] = menu->items[i]->mins[0];
		if( menu->items[i]->mins[1] && menu->items[i]->mins[1] < mins[1] )
			mins[1] = menu->items[i]->mins[1];
		if( menu->items[i]->maxs[0] && menu->items[i]->maxs[0] > maxs[0] )
			maxs[0] = menu->items[i]->maxs[0];
		if( menu->items[i]->maxs[1] && menu->items[i]->maxs[1] > maxs[1] )
			maxs[1] = menu->items[i]->maxs[1];
	}
	//extend it
	mins[0] -= 16;
	mins[1] -= 16;
	maxs[0] += 16;
	maxs[1] += 16;

	// add a background to the menu
	//trap_R_DrawStretchPic( mins[0], mins[1], maxs[0]-mins[0], maxs[1]-mins[1],
	//	0, 0, 1, 1, colorWhite, trap_R_RegisterPic( "gfx/ui/novideoback") );

	trap_R_DrawStretchPic( 0, mins[1], uis.vidWidth, maxs[1]-mins[1],
	                       0, 0, 1, 1, colorback, uis.whiteShader );
}

//=================================================================
//
//		MENU
//
//=================================================================

void Menu_AddItem( menuframework_s *menu, void *item )
{
	int i;
	qboolean found = qfalse;

	if( menu->nitems == 0 )
		menu->nslots = 0;

	for( i = 0; i < menu->nitems; i++ )
	{
		if( menu->items[i] == item )
		{
			found = qtrue;
			break;
		}
	}

	if( !found && menu->nitems < MAXMENUITEMS )
	{
		menu->items[menu->nitems] = (menucommon_t *)item;
		menu->items[menu->nitems]->parent = menu;
		menu->nitems++;
	}

	menu->nslots = Menu_TallySlots( menu );
}

/*
** Menu_AdjustCursor
**
** This function takes the given menu, the direction, and attempts
** to adjust the menu's cursor so that it's at the next available
** slot.
*/
void Menu_AdjustCursor( menuframework_s *m, int dir )
{
	menucommon_t *citem;
	int i;

	// see if it's in a valid spot
	if( m->cursor >= 0 && m->cursor < m->nitems )
	{
		if( ( citem = Menu_ItemAtCursor( m ) ) != 0 )
		{
			if( citem->type != MTYPE_SEPARATOR )
				return;
		}
	}

	// it's not in a valid spot, so crawl in the direction indicated until we
	// find a valid spot
	if( dir == 1 )
	{
		for( i = 0; i < m->nitems; i++ )
		{
			citem = Menu_ItemAtCursor( m );
			if( citem )
				if( citem->type != MTYPE_SEPARATOR )
					break;
			m->cursor += dir;
			if( m->cursor >= m->nitems )
				m->cursor = 0;
		}
	}
	else
	{
		for( i = 0; i < m->nitems; i++ )
		{
			citem = Menu_ItemAtCursor( m );
			if( citem )
				if( citem->type != MTYPE_SEPARATOR )
					break;
			m->cursor += dir;
			if( m->cursor < 0 )
				m->cursor = m->nitems - 1;
		}
	}
}

/*
** Menu_Center
**
** This function takes the given menu, the direction, and attempts
** to adjust its position to the center of the screen.
** Note: will now be ignored when a menu is already dynamically
**	set to scale the screen or if it is too big for the default
**	menu sizes.
*/
void Menu_Center( menuframework_s *menu )
{
	int i, height = 0;

	menu->x = uis.vidWidth / 2;

	// y center
	for( i = 0; i < menu->nitems; i++ )
	{
		if( menu->items[i]->y > height )
			height = menu->items[i]->y;
	}
	height += 10;
	menu->y = ( uis.vidHeight - height ) / 2;
}

void Menu_AdjustRectangle( int *mins, int *maxs )
{
	mins[0] *= UI_WIDTHSCALE;
	maxs[0] *= UI_HEIGHTSCALE;

	mins[1] *= UI_WIDTHSCALE;
	maxs[1] *= UI_HEIGHTSCALE;
}

void Menu_Init( menuframework_s *menu, qboolean justify_buttons )
{
	int i;
	int max_button_width = 0;

	// init items
	for( i = 0; i < menu->nitems; i++ )
	{
		switch( menu->items[i]->type )
		{
		case MTYPE_FIELD:
			Field_Init( menu->items[i] );
			break;
		case MTYPE_SLIDER:
			Slider_Init( menu->items[i] );
			break;
		case MTYPE_SPINCONTROL:
			SpinControl_Init( menu->items[i] );
			break;
		case MTYPE_ACTION:
			Action_Init( menu->items[i] );
			if( menu->items[i]->width > max_button_width )
				max_button_width = menu->items[i]->width;
			break;
		case MTYPE_SEPARATOR:
			Separator_Init( menu->items[i] );
			break;
		case MTYPE_SCROLLBAR:
			Scrollbar_Init( menu->items[i] );
			break;
		}
	}

	if( justify_buttons )
	{
		// justify items
		for( i = 0; i < menu->nitems; i++ )
		{
			switch( menu->items[i]->type )
			{
			case MTYPE_FIELD:
				break;
			case MTYPE_SLIDER:
				break;
			case MTYPE_SPINCONTROL:
				break;
			case MTYPE_ACTION:
				if( menu->items[i]->box )
					menu->items[i]->width = max_button_width;
				break;
			case MTYPE_SEPARATOR:
				break;
			case MTYPE_SCROLLBAR:
				break;
			}
		}
	}

	// init items
	for( i = 0; i < menu->nitems; i++ )
	{
		Menu_AdjustRectangle( menu->items[i]->mins, menu->items[i]->maxs );
	}
}

void Menu_Draw( menuframework_s *menu )
{
	int i;
	menucommon_t *item;

	if( !uis.backGround )
		Menu_DrawWindowedBackground( menu );

	// draw contents
	for( i = 0; i < menu->nitems; i++ )
	{
		//draw associated picture
		MenuItem_DrawPict( menu->items[i] );

		switch( menu->items[i]->type )
		{
		case MTYPE_FIELD:
			Field_Draw( menu->items[i] );
			break;
		case MTYPE_SLIDER:
			Slider_Draw( menu->items[i] );
			break;
		case MTYPE_SPINCONTROL:
			SpinControl_Draw( menu->items[i] );
			break;
		case MTYPE_ACTION:
			Action_Draw( menu->items[i] );
			break;
		case MTYPE_SEPARATOR:
			Separator_Draw( menu->items[i] );
			break;
		case MTYPE_SCROLLBAR:
			Scrollbar_Draw( menu->items[i] );
			break;
		}
	}

	item = Menu_ItemAtCursor( menu );

	if( item && item->cursordraw )
	{
		item->cursordraw( item );
	}
	else if( menu->cursordraw )
	{
		menu->cursordraw( menu );
	}

	if( item )
	{
		if( item->statusbarfunc )
			item->statusbarfunc( item );
		else if( item->statusbar )
			Menu_DrawStatusBar( item->statusbar );
		else if( menu->statusbar )
			Menu_DrawStatusBar( menu->statusbar );
	}
	else if( menu->statusbar )
	{
		Menu_DrawStatusBar( menu->statusbar );
	}
}

void Menu_DrawStatusBar( char *string )
{
	struct mufont_s *font = uis.fontSystemSmall;

	UI_FillRect( 0, uis.vidHeight-trap_SCR_strHeight( font ), uis.vidWidth, trap_SCR_strHeight( font ), colorDkGrey );
	trap_SCR_DrawStringWidth( uis.vidWidth/2, uis.vidHeight, ALIGN_CENTER_BOTTOM, string, uis.vidWidth, font, colorWhite );
}

menucommon_t *Menu_ItemAtCursor( menuframework_s *m )
{
	if( m->cursor < 0 || m->cursor >= m->nitems )
		return NULL;

	return m->items[m->cursor];
}

qboolean Menu_SelectItem( menuframework_s *s )
{
	menucommon_t *item = Menu_ItemAtCursor( s );

	if( item )
	{
		switch( item->type )
		{
		case MTYPE_FIELD:
			return Field_DoEnter( item );
		case MTYPE_ACTION:
			Action_DoEnter( item );
			return qtrue;
		case MTYPE_SPINCONTROL:
			//			SpinControl_DoEnter( item );
			return qfalse;
		}
	}
	return qfalse;
}

void Menu_SetStatusBar( menuframework_s *m, char *string )
{
	m->statusbar = string;
}

qboolean Menu_SlideItem( menuframework_s *s, int dir, int key )
{
	menucommon_t *item = Menu_ItemAtCursor( s );

	if( !item )
		return qfalse;
	if( item->scrollbar_id )  //if we're hoving on a list, we want to move its scrollbar, not the list
		item = s->items[item->scrollbar_id];

	if( item )
	{
		switch( item->type )
		{
		case MTYPE_SLIDER:
			if( key == K_MOUSE2 )
				return qfalse;
			if( key == K_MOUSE1 )
				dir = 0; //we move to where the mouse is, not just one step
			Slider_DoSlide( item, dir );
			return qtrue;
		case MTYPE_SPINCONTROL:
			SpinControl_DoSlide( item, dir );
			return qtrue;
		case MTYPE_SCROLLBAR:
			if( key != K_MOUSE2 ) //K_MOUSE2 just goes back a menu, does not move the scrollbar
			{
				if( ( Menu_ItemAtCursor( s ) != item && key != K_MOUSE1 ) || ( Menu_ItemAtCursor( s ) == item ) )
				{
					if( dir > 3 || dir < -3 )
						dir = 0;
					Scrollbar_DoSlide( item, dir ); //only use K_MOUSE1 for scrolling if we're hovering the scrollbar
				}
			}
			if( Menu_ItemAtCursor( s )->scrollbar_id )  //if we're moving the scrollbar, and not the list, we have to return qfalse
				return qfalse;
			else
				return qtrue;
		}
	}

	return qfalse;
}

int Menu_TallySlots( menuframework_s *menu )
{
	int i;
	int total = 0;

	for( i = 0; i < menu->nitems; i++ )
	{
		total++;
	}

	return total;
}

/*
* UI_ListNameForPosition
*/
char *UI_ListNameForPosition( const char *namesList, int position, const char separator )
{
	static char buf[MAX_STRING_CHARS];
	const char *s, *t;
	char *b;
	int count, len;

	if( !namesList )
		return NULL;

	// set up the tittle from the spinner names
	s = namesList;
	t = s;
	count = 0;
	buf[0] = 0;
	b = buf;
	while( *s && ( s = strchr( s, separator ) ) )
	{
		if( count == position )
		{
			len = s - t;
			if( len <= 0 )
				UI_Error( "G_NameInStringList: empty name in list\n" );
			if( len > MAX_STRING_CHARS - 1 )
				UI_Printf( "WARNING: G_NameInStringList: name is too long\n" );
			while( t <= s )
			{
				if( *t == separator || t == s )
				{
					*b = 0;
					break;
				}
				
				*b = *t;
				t++;
				b++;
			}

			break;
		}

		count++;
		s++;
		t = s;
	}

	if( buf[0] == 0 )
		return NULL;

	return buf;
}

/*
* UI_CreateFileNamesListCvar
*/
qboolean UI_CreateFileNamesListCvar( cvar_t *cvar, const char *path, const char *extension, const char separator )
{
	char separators[2];
	char name[MAX_CONFIGSTRING_CHARS];
	char buffer[MAX_STRING_CHARS], *s, *list;
	int numfiles, i, j, found, length, fulllength;

	if( !cvar )
		return qfalse;

	trap_Cvar_ForceSet( cvar->name, ";" );

	if( !extension || !path )
		return qfalse;

	if( extension[0] != '.' || strlen( extension ) < 2 )
		return qfalse;

	if( ( numfiles = trap_FS_GetFileList( path, extension, NULL, 0, 0, 0 ) ) == 0 ) 
		return qfalse;

	separators[0] = separator;
	separators[1] = 0;

	//
	// do a first pass just for finding the full len of the list
	//

	i = 0;
	found = 0;
	length = 0;
	fulllength = 0;
	do 
	{
		if( ( j = trap_FS_GetFileList( path, extension, buffer, sizeof( buffer ), i, numfiles ) ) == 0 ) 
		{
			// can happen if the filename is too long to fit into the buffer or we're done
			i++;
			continue;
		}

		i += j;
		for( s = buffer; j > 0; j--, s += length + 1 ) 
		{
			length = strlen( s );

			if( strlen( path ) + 1 + length >= MAX_CONFIGSTRING_CHARS ) 
			{
				Com_Printf( "Warning: UI_CreateFileNamesListCvar :file name too long: %s\n", s );
				continue;
			}

			Q_strncpyz( name, s, sizeof( name ) );
			COM_StripExtension( name );

			fulllength += strlen( name ) + 1;
			found++;
		}
	} while( i < numfiles );

	if( !found )
		return qfalse;

	//
	// Allocate a string for the full list and do a second pass to copy them in there
	//

	fulllength += 1;
	list = UI_Malloc( fulllength );

	i = 0;
	length = 0;
	do 
	{
		if( ( j = trap_FS_GetFileList( path, extension, buffer, sizeof( buffer ), i, numfiles ) ) == 0 ) 
		{
			// can happen if the filename is too long to fit into the buffer or we're done
			i++;
			continue;
		}

		i += j;
		for( s = buffer; j > 0; j--, s += length + 1 ) 
		{
			length = strlen( s );

			if( strlen( path ) + 1 + length >= MAX_CONFIGSTRING_CHARS ) 
				continue;

			Q_strncpyz( name, s, sizeof( name ) );
			COM_StripExtension( name );

			Q_strncatz( list, name, fulllength );
			Q_strncatz( list, separators, fulllength );
		}
	} while( i < numfiles );

	trap_Cvar_ForceSet( cvar->name, list );
	UI_Free( list );

	return qtrue;
}

qboolean UI_NamesListCvarAddName( const cvar_t *cvar, const char *name, const char separator )
{
	char separators[2];
	char *newString, *s;
	size_t size;
	int i;

	if( !cvar || !cvar->string || !name || !name[0] )
		return qfalse;

	separators[0] = separator;
	separators[1] = 0;

	if( strlen( cvar->string ) )
	{
		// validate the integrity of the cvar content
		if( !strchr( cvar->string, separator ) || cvar->string[strlen( cvar->string ) - 1] != separator )
			trap_Cvar_ForceSet( cvar->name, va( "%s%c", cvar->string, separator ) );
	}

	// skip if the name is already in the list
	for( i = 0; ;i++ )
	{
		s = UI_ListNameForPosition( cvar->string, i, separator );
		if( !s )
			break;

		if( !Q_stricmp( s, name ) )
			return qfalse;
	}

	// if the current cvar is missing a terminating separator, add it.

	size = strlen( cvar->string ) + strlen( name ) + strlen( separators ) + 1;
	newString = UI_Malloc( size );

	Q_snprintfz( newString, size, "%s%s%s", cvar->string, name, separators );
	trap_Cvar_ForceSet( cvar->name, newString );

	UI_Free( newString );

	return qtrue;
}

//=======================================================
// scroll lists management
//=======================================================

//==================
//UI_FreeScrollItemList
//==================
void UI_FreeScrollItemList( m_itemslisthead_t *itemlist )
{
	m_listitem_t *ptr;

	while( itemlist->headNode )
	{
		ptr = itemlist->headNode;
		itemlist->headNode = ptr->pnext;
		UI_Free( ptr );
	}

	itemlist->headNode = NULL;
	itemlist->numItems = 0;
}

//==================
//UI_FindItemInScrollListWithId
//==================
m_listitem_t *UI_FindItemInScrollListWithId( m_itemslisthead_t *itemlist, int itemid )
{
	m_listitem_t *item;

	if( !itemlist->headNode )
		return NULL;

	item = itemlist->headNode;
	while( item )
	{
		if( item->id == itemid )
			return item;
		item = item->pnext;
	}

	return NULL;
}

//==================
//UI_AddItemToScrollList
//==================
void UI_AddItemToScrollList( m_itemslisthead_t *itemlist, const char *name, void *data )
{
	m_listitem_t *newitem, *checkitem;

	//check for the address being already in the list
	checkitem = itemlist->headNode;
	while( checkitem )
	{
		if( !Q_stricmp( name, checkitem->name ) )
			return;
		checkitem = checkitem->pnext;
	}

	newitem = (m_listitem_t *)UI_Malloc( sizeof( m_listitem_t ) );
	Q_strncpyz( newitem->name, name, sizeof( newitem->name ) );
	newitem->pnext = itemlist->headNode;
	itemlist->headNode = newitem;
	newitem->id = itemlist->numItems;
	newitem->data = data;

	// update item names array
	itemlist->item_names[itemlist->numItems] = UI_CopyString( newitem->name );
	itemlist->item_names[itemlist->numItems+1] = NULL;

	itemlist->numItems++;
}

//=======================================================
// menu actions registration
//=======================================================
menucommon_t *ui_menuitems_headnode;

menucommon_t *UI_MenuItemByName( char *name )
{
	menucommon_t *menuitem;

	if( !name )
		return NULL;

	for( menuitem = ui_menuitems_headnode; menuitem; menuitem = (menucommon_t *)menuitem->next )
	{
		if( !Q_stricmp( menuitem->name, name ) )
			return menuitem;
	}

	return NULL;
}

char *UI_GetMenuitemFieldBuffer( menucommon_t *menuitem )
{
	menufield_t *field;

	if( !menuitem || menuitem->type != MTYPE_FIELD )
		return NULL;
	if( !menuitem->itemlocal )
		return NULL;

	field = (menufield_t *)menuitem->itemlocal;
	return field->buffer;
}

menucommon_t *UI_RegisterMenuItem( const char *name, int type )
{
	menucommon_t *menuitem;
	size_t size;

	if( !name )
		return NULL;

	for( menuitem = ui_menuitems_headnode; menuitem; menuitem = (menucommon_t *)menuitem->next )
	{
		if( !Q_stricmp( menuitem->name, name ) )
			return menuitem;
	}

	switch( type )
	{
	case MTYPE_SLIDER:
		size = 0;
		break;
	case MTYPE_ACTION:
		size = 0;
		break;
	case MTYPE_SPINCONTROL:
		size = 0;
		break;
	case MTYPE_SEPARATOR:
		size = 0;
		break;
	case MTYPE_FIELD:
		size = sizeof( menufield_t );
		break;
	case MTYPE_SCROLLBAR:
		size = 0;
		break;
	default:
		size = 0;
		break;
	}

	// allocate one huge array to hold our data
	menuitem = (menucommon_t *)UI_Malloc( sizeof( menucommon_t ) );
	if( size )
		menuitem->itemlocal = UI_Malloc( size );
	menuitem->name = UI_CopyString( name );
	menuitem->next = ui_menuitems_headnode;
	ui_menuitems_headnode = menuitem;

	return menuitem;
}

menucommon_t *UI_InitMenuItem( const char *name, char *title, int x, int y, int type, int align, struct mufont_s *font, void ( *callback )(struct menucommon_s *) )
{
	menucommon_t *menuitem;

	if( !name )
		return NULL;

	menuitem = UI_RegisterMenuItem( name, type );
	if( !menuitem )
		return NULL;

	menuitem->type = type;
	menuitem->align = align;
	menuitem->x = x;
	menuitem->y = y;
	menuitem->font = font;
	menuitem->callback = callback;
	if( title )
		Q_strncpyz( menuitem->title, title, MAX_STRING_CHARS );
	else
		menuitem->title[0] = 0;

	// clear up associated picture trash
	menuitem->pict.shader = NULL;
	menuitem->pict.shaderHigh = NULL;
	menuitem->box = qfalse;
	Vector4Copy( colorWhite, menuitem->pict.color );
	Vector4Copy( colorWhite, menuitem->pict.colorHigh );

	return menuitem;
}

int UI_SetupButton( menucommon_t *menuitem, qboolean box )
{
	int minheight, minwidth;

	if( !menuitem )
		return 0;

	menuitem->box = box;
	minheight = trap_SCR_strHeight( menuitem->font );
	minwidth = UI_StringWidth( menuitem->title, menuitem->font );

	if( box )
	{
		menuitem->width = minwidth + 32;
		menuitem->height = minheight + 18;
	}
	else
	{
		menuitem->width = minwidth;
		menuitem->height = minheight;
	}
	
	return menuitem->height;
}

menucommon_t *UI_SetupSlider( menucommon_t *menuitem, int width, int curvalue, int minvalue, int maxvalue )
{
	if( !menuitem )
		return NULL;

	menuitem->curvalue = curvalue;
	menuitem->minvalue = minvalue;
	menuitem->maxvalue = maxvalue;
	clamp( menuitem->curvalue, minvalue, maxvalue );
	menuitem->width = width;
	if( menuitem->width < 3 )
		menuitem->width = 3;

	return menuitem;
}

menucommon_t *UI_SetupScrollbar( menucommon_t *menuitem, int height, int curvalue, int minvalue, int maxvalue )
{
	if( !menuitem )
		return NULL;

	menuitem->minvalue = minvalue;
	if( !menuitem->maxvalue )
		menuitem->maxvalue = maxvalue;
	if( !menuitem->curvalue )
		menuitem->curvalue = curvalue;
	clamp( menuitem->curvalue, minvalue, maxvalue );
	menuitem->height = height;
	if( menuitem->height < 3 )
		menuitem->height = 3;
	return menuitem;
}

menucommon_t *UI_SetupSpinControl( menucommon_t *menuitem, char **item_names, int curvalue )
{
	int numitems;

	if( !menuitem || !item_names )
		return NULL;

	//count item names
	numitems = 0;
	while( item_names[numitems] )
		numitems++;

	menuitem->itemnames = item_names;
	menuitem->curvalue = curvalue;
	menuitem->minvalue = 0;
	menuitem->maxvalue = numitems-1;
	clamp( menuitem->curvalue, menuitem->minvalue, menuitem->maxvalue );

	return menuitem;
}

menufield_t *UI_SetupField( menucommon_t *menuitem, const char *string, size_t length, int width )
{
	menufield_t *field;

	if( !menuitem )
		return NULL;

	field = (menufield_t *)menuitem->itemlocal;

	field->length = length;
	if( length < 1 )
		length = 1;

	if( width >= ( 2*(int)trap_SCR_strWidth( "_", menuitem->font, 0 ) ) )
		field->width = width;
	else
		field->width = ( length+1 ) *trap_SCR_strWidth( "_", menuitem->font, 0 );

	if( string )
	{
		Q_strncpyz( field->buffer, string, sizeof( field->buffer ) );
		field->cursor = strlen( field->buffer );
	}
	else
	{
		memset( field->buffer, 0, sizeof( field->buffer ) );
		field->cursor = 0;
	}

	return field;
}

void UI_SetupFlags( menucommon_t *menuitem, int flags )
{
	menuitem->flags = flags;
}
