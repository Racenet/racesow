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
// cl_scrn.c -- master for refresh, status bar, console, chat, notify, etc

/*

full screen console
put up loading plaque
blanked background with loading plaque
blanked background with menu
cinematics
full screen image for quit and victory

end of unit intermissions

*/

#include "client.h"

float scr_con_current;    // aproaches scr_conlines at scr_conspeed
float scr_con_previous;
float scr_conlines;       // 0.0 to 1.0 lines of console to display

qboolean scr_initialized;    // ready to draw

int scr_draw_loading;

static cvar_t *scr_consize;
static cvar_t *scr_conspeed;
static cvar_t *scr_netgraph;
static cvar_t *scr_timegraph;
static cvar_t *scr_debuggraph;
static cvar_t *scr_graphheight;
static cvar_t *scr_graphscale;
static cvar_t *scr_graphshift;
static cvar_t *scr_forceclear;

/*
===============================================================================

MUFONT STRINGS

===============================================================================
*/


//
//	Variable width (proportional) fonts
//

//===============================================================================
//FONT LOADING
//===============================================================================

mempool_t *fonts_mempool;

#define Font_Alloc( size ) Mem_Alloc( fonts_mempool, size )
#define Font_Free( size ) Mem_Free( size )

#define REPLACEMENT_CHAR 127

#define MIN_FONT_CHARS 128	// to fit REPLACEMENT_CHAR
#define MAX_FONT_CHARS	0x2200

typedef struct
{
	unsigned short x, y;
	qbyte width, height;
	float s1, t1, s2, t2;
} muchar_t;

typedef struct mufont_s
{
	char *name;
	char *shadername;
	int fontheight;
	float imagewidth, imageheight;
	struct shader_s	*shader;
	struct mufont_s	*next;
	unsigned int numchars;
	muchar_t *chars;
} mufont_t;

static mufont_t *gs_muFonts;

static mufont_t *SCR_LoadMUFont( const char *name, size_t len )
{
	size_t filename_size;
	char *filename;
	qbyte *buf;
	char *ptr, *token, *start;
	int filenum;
	int length;
	mufont_t *font;
	struct shader_s *shader;
	int numchar;
	int x, y, w, h;

	filename_size = strlen( "fonts/" ) + len + strlen( ".tga" ) + 1;
	filename = Mem_TempMalloc( filename_size );
	Q_snprintfz( filename, filename_size, "fonts/%s", name );

	// load the shader
	COM_ReplaceExtension( filename, ".tga", filename_size );

	shader = R_RegisterPic( filename );
	if( !shader )
	{
		Mem_TempFree( filename );
		return NULL;
	}

	// load the font description
	COM_ReplaceExtension( filename, ".wfd", filename_size );

	// load the file
	length = FS_FOpenFile( filename, &filenum, FS_READ );
	if( length == -1 )
	{
		Mem_TempFree( filename );
		return NULL;
	}

	buf = Mem_TempMalloc( length + 1 );
	length = FS_Read( buf, length, filenum );
	FS_FCloseFile( filenum );
	if( !length )
	{
		Mem_TempFree( filename );
		Mem_TempFree( buf );
		return NULL;
	}

	// seems to be valid. Allocate it
	font = (mufont_t *)Font_Alloc( sizeof( mufont_t ) );
	font->shader = shader;

	font->name = Font_Alloc( len + 1 );
	Q_strncpyz( font->name, name, len + 1 );

	font->shadername = Font_Alloc( filename_size );
	Q_strncpyz( font->shadername, filename, filename_size );
	COM_ReplaceExtension( font->shadername, ".tga", filename_size );

	// proceed
	ptr = ( char * )buf;

	// get texture width and height
	token = COM_Parse( &ptr );
	if( !token[0] )
		goto error;
	font->imagewidth = atoi( token );

	token = COM_Parse( &ptr );
	if( !token[0] )
		goto error;
	font->imageheight = atoi( token );

	font->numchars = MIN_FONT_CHARS;

	// get the number of chars
	start = ptr;
	while( ptr )
	{
		// "<char>" "<x>" "<y>" "<width>" "<height>"
		token = COM_Parse( &ptr );
		if( !token[0] )
			break;
		numchar = atoi( token );
		if( numchar <= 0 )
			break;

		x = atoi( COM_Parse( &ptr ) ), y = atoi( COM_Parse( &ptr ) );
		w = atoi( COM_Parse( &ptr ) ), h = atoi( COM_Parse( &ptr ) );

		if( numchar < 32 || numchar >= MAX_FONT_CHARS )
			continue;
		if( ( unsigned int )( numchar + 1 ) > font->numchars )
			font->numchars = ( unsigned int )numchar + 1;
	}

	if( !font->numchars )
		goto error;

	font->chars = Font_Alloc( font->numchars * sizeof( muchar_t ) );

	// get the chars
	ptr = start;
	while( ptr )
	{
		token = COM_Parse( &ptr );
		if( !token[0] )
			break;
		numchar = atoi( token );
		if( numchar <= 0 )
			break;

		x = atoi( COM_Parse( &ptr ) ), y = atoi( COM_Parse( &ptr ) );
		w = atoi( COM_Parse( &ptr ) ), h = atoi( COM_Parse( &ptr ) );

		if( numchar < 32 || ( unsigned int )numchar >= font->numchars )
			continue;

		font->chars[numchar].x = x;
		font->chars[numchar].y = y;
		font->chars[numchar].width = w;
		font->chars[numchar].height = h;

		// create the texture coordinates
		font->chars[numchar].s1 = ( (float)x )/(float)font->imagewidth;
		font->chars[numchar].s2 = ( (float)( x + w ) )/(float)font->imagewidth;
		font->chars[numchar].t1 = ( (float)y )/(float)font->imageheight;
		font->chars[numchar].t2 = ( (float)( y + h ) )/(float)font->imageheight;
	}

	// mudFont is not always giving a proper size to the space character
	font->chars[' '].width = font->chars['-'].width;

	// height is the same for every character
	font->fontheight = font->chars['a'].height;

	// if REPLACEMENT_CHAR is not present in this font, copy '?' to that position
	if( !( font->chars[REPLACEMENT_CHAR].height ) )
		font->chars[REPLACEMENT_CHAR] = font->chars['?'];

	Mem_TempFree( filename );
	Mem_TempFree( buf );
	return font;

error:
	if( font->chars )
		Font_Free( font->chars );
	if( font->name )
		Font_Free( font->name );
	Font_Free( font );

	Mem_TempFree( filename );
	Mem_TempFree( buf );
	return NULL;
}

/*
* SCR_FreeMUFont
*/
void SCR_FreeMUFont( mufont_t *font )
{
	assert( font );

	Font_Free( font->chars );
	Font_Free( font->name );
	Font_Free( font->shadername );
	Font_Free( font );
}

/*
* SCR_RegisterFont
*/
struct mufont_s *SCR_RegisterFont( const char *name )
{
	mufont_t *font;
	const char *extension;
	size_t len;

	extension = COM_FileExtension( name );
	len = ( extension ? extension - name - 1 : strlen( name ) );

	for( font = gs_muFonts; font; font = font->next )
	{
		if( !Q_strnicmp( font->name, name, len ) ) {
			font->shader = R_RegisterPic( font->shadername );
			return font;
		}
	}

	font = SCR_LoadMUFont( name, len );
	if( !font )
	{
		return NULL;
	}

	font->next = gs_muFonts;
	gs_muFonts = font;

	return font;
}

/*
* SCR_FreeFont
*/
static void SCR_FreeFont( struct mufont_s *font )
{
	mufont_t *prev, *iter;

	assert( font );

	prev = NULL;
	iter = gs_muFonts;
	while( iter )
	{
		if( iter == font )
			break;
		prev = iter;
		iter = iter->next;
	}
	assert( iter == font );
	if( prev )
	{
		prev->next = font->next;
	}
	else
	{
		gs_muFonts = font->next;
	}

	SCR_FreeMUFont( font );
}

static void SCR_InitFonts( void )
{
	cvar_t *con_fontSystemSmall = Cvar_Get( "con_fontSystemSmall", DEFAULT_FONT_SMALL, CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	cvar_t *con_fontSystemMedium = Cvar_Get( "con_fontSystemMedium", DEFAULT_FONT_MEDIUM, CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	cvar_t *con_fontSystemBig = Cvar_Get( "con_fontSystemBig", DEFAULT_FONT_BIG, CVAR_ARCHIVE|CVAR_LATCH_VIDEO );

	fonts_mempool = Mem_AllocPool( NULL, "Fonts" );

	// register system fonts
	cls.fontSystemSmall = SCR_RegisterFont( con_fontSystemSmall->string );
	if( !cls.fontSystemSmall )
	{
		cls.fontSystemSmall = SCR_RegisterFont( DEFAULT_FONT_SMALL );
		if( !cls.fontSystemSmall )
			Com_Error( ERR_FATAL, "Couldn't load default font \"%s\"", DEFAULT_FONT_SMALL );
	}

	cls.fontSystemMedium = SCR_RegisterFont( con_fontSystemMedium->string );
	if( !cls.fontSystemMedium )
		cls.fontSystemMedium = SCR_RegisterFont( DEFAULT_FONT_MEDIUM );

	cls.fontSystemBig = SCR_RegisterFont( con_fontSystemBig->string );
	if( !cls.fontSystemBig )
		cls.fontSystemBig = SCR_RegisterFont( DEFAULT_FONT_BIG );
}

static void SCR_ShutdownFonts( void )
{
	while( gs_muFonts )
	{
		SCR_FreeFont( gs_muFonts );
	}

	cls.fontSystemSmall = NULL;
	cls.fontSystemMedium = NULL;
	cls.fontSystemBig = NULL;

	Mem_FreePool( &fonts_mempool );
}

//===============================================================================
//STRINGS HELPERS
//===============================================================================


static int SCR_HorizontalAlignForString( const int x, int align, int width )
{
	int nx = x;

	if( align % 3 == 0 )  // left
		nx = x;
	if( align % 3 == 1 )  // center
		nx = x - width / 2;
	if( align % 3 == 2 )  // right
		nx = x - width;

	return nx;
}

static int SCR_VerticalAlignForString( const int y, int align, int height )
{
	int ny = y;

	if( align / 3 == 0 )  // top
		ny = y;
	else if( align / 3 == 1 )  // middle
		ny = y - height / 2;
	else if( align / 3 == 2 )  // bottom
		ny = y - height;

	return ny;
}

/*
* SCR_strHeight
* it's font height in fact, but for preserving simetry I call it str
*/
size_t SCR_strHeight( struct mufont_s *font )
{
	if( !font )
		font = cls.fontSystemSmall;

	return font->fontheight;
}

/*
* SCR_strWidth
* doesn't count invisible characters. Counts up to given length, if any.
*/
size_t SCR_strWidth( const char *str, struct mufont_s *font, int maxlen )
{
	const char *s = str;
	size_t width = 0;
	qwchar num;

	if( !str )
		return 0;

	if( !font )
		font = cls.fontSystemSmall;

	while( *s && *s != '\n' )
	{
		if( maxlen && ( s - str ) >= maxlen )  // stop counting at desired len
			return width;

		switch( Q_GrabWCharFromColorString( &s, &num, NULL ) )
		{
		case GRABCHAR_CHAR:
			if( num < ' ' )
				break;
			if( num >= font->numchars || !( font->chars[num].height ) )
				num = REPLACEMENT_CHAR;
			width += font->chars[num].width;
			break;

		case GRABCHAR_COLOR:
			break;

		case GRABCHAR_END:
			return width;

		default:
			assert( 0 );
		}
	}

	return width;
}

/*
* SCR_StrlenForWidth
* returns the len allowed for the string to fit inside a given width when using a given font.
*/
size_t SCR_StrlenForWidth( const char *str, struct mufont_s *font, size_t maxwidth )
{
	const char *s, *olds;
	size_t width = 0;
	int gc;
	qwchar num;

	if( !str )
		return 0;

	if( !font )
		font = cls.fontSystemSmall;

	s = str;

	while( s )
	{
		olds = s;
		gc = Q_GrabWCharFromColorString( &s, &num, NULL );
		if( gc == GRABCHAR_CHAR )
		{
			if( num == '\n' )
				break;
			if( num < ' ' )
				continue;
			if( num >= font->numchars || !( font->chars[num].height ) )
				num = REPLACEMENT_CHAR;

			if( maxwidth && ( ( width + font->chars[num].width ) > maxwidth ) )
			{
				s = olds;
				break;
			}

			width += font->chars[num].width;
		}
		else if( gc == GRABCHAR_COLOR )
			continue;
		else if( gc == GRABCHAR_END )
			break;
		else
			assert( 0 );
	}

	return (unsigned int)( s - str );
}


//===============================================================================
//STRINGS DRAWING
//===============================================================================

/*
* SCR_DrawRawChar
* 
* Draws one graphics character with 0 being transparent.
* It can be clipped to the top of the screen to allow the console to be
* smoothly scrolled off.
*/
void SCR_DrawRawChar( int x, int y, qwchar num, struct mufont_s *font, vec4_t color )
{
	if( !font )
		font = cls.fontSystemSmall;

	if( num <= ' ' )
		return;
	if( num >= font->numchars || !( font->chars[num].height ) )
		num = REPLACEMENT_CHAR;

	if( y <= -font->fontheight )
		return; // totally off screen

	R_DrawStretchPic( x, y, font->chars[num].width, font->fontheight,
		font->chars[num].s1, font->chars[num].t1, font->chars[num].s2, font->chars[num].t2,
		color, font->shader );
}

static void SCR_DrawClampChar( int x, int y, qwchar num, int xmin, int ymin, int xmax, int ymax, struct mufont_s *font, vec4_t color )
{
	float pixelsize;
	float s1, s2, t1, t2;
	int sx, sy, sw, sh;
	int offset;

	if( !font )
		font = cls.fontSystemSmall;

	if( num <= ' ' )
		return;
	if( num >= font->numchars || !( font->chars[num].height ) )
		num = REPLACEMENT_CHAR;

	// ignore if completely out of the drawing space
	if( y + font->fontheight <= ymin || y >= ymax ||
		x + font->chars[num].width <= xmin || x >= xmax )
		return;

	pixelsize = 1.0f / font->imageheight;

	s1 = font->chars[num].s1;
	t1 = font->chars[num].t1;
	s2 = font->chars[num].s2;
	t2 = font->chars[num].t2;

	sx = x;
	sy = y;
	sw = font->chars[num].width;
	sh = font->fontheight;

	// clamp xmin
	if( x < xmin && x + font->chars[num].width >= xmin )
	{
		offset = xmin - x;
		if( offset )
		{
			sx += offset;
			sw -= offset;
			s1 += ( pixelsize * offset );
		}
	}

	// clamp ymin
	if( y < ymin && y + font->chars[num].height >= ymin )
	{
		offset = ymin - y;
		if( offset )
		{
			sy += offset;
			sh -= offset;
			t1 += ( pixelsize * offset );
		}
	}

	// clamp xmax
	if( x < xmax && x + font->chars[num].width >= xmax )
	{
		offset = ( x + font->chars[num].width ) - xmax;
		if( offset != 0 )
		{
			sw -= offset;
			s2 -= ( pixelsize * offset );
		}
	}

	// clamp ymax
	if( y < ymax && y + font->chars[num].height >= ymax )
	{
		offset = ( y + font->chars[num].height ) - ymax;
		if( offset != 0 )
		{
			sh -= offset;
			t2 -= ( pixelsize * offset );
		}
	}

	R_DrawStretchPic( sx, sy, sw, sh, s1, t1, s2, t2, color, font->shader );
}

void SCR_DrawClampString( int x, int y, const char *str, int xmin, int ymin, int xmax, int ymax, struct mufont_s *font, vec4_t color )
{
	int xoffset = 0, yoffset = 0;
	vec4_t scolor;
	int colorindex;
	qwchar num;
	const char *s = str;
	int gc;

	if( !str )
		return;

	if( !font )
		font = cls.fontSystemSmall;

	// clamp mins and maxs to the screen space
	if( xmin < 0 )
		xmin = 0;
	if( xmax > (int)viddef.width )
		xmax = (int)viddef.width;
	if( ymin < 0 )
		ymin = 0;
	if( ymax > (int)viddef.height )
		ymax = (int)viddef.height;
	if( xmax <= xmin || ymax <= ymin || x > xmax || y > ymax )
		return;

	Vector4Copy( color, scolor );

	while( 1 )
	{
		gc = Q_GrabWCharFromColorString( &s, &num, &colorindex );
		if( gc == GRABCHAR_CHAR )
		{
			if( num == '\n' )
				break;
			if( num < ' ' )
				continue;
			if( num >= font->numchars || !( font->chars[num].height ) )
				num = REPLACEMENT_CHAR;

			if( num != ' ' )
				SCR_DrawClampChar( x + xoffset, y + yoffset, num, xmin, ymin, xmax, ymax, font, scolor );

			xoffset += font->chars[num].width;
			if( x + xoffset > xmax )
				break;
		}
		else if( gc == GRABCHAR_COLOR )
		{
			assert( ( unsigned )colorindex < MAX_S_COLORS );
			VectorCopy( color_table[colorindex], scolor );
		}
		else if( gc == GRABCHAR_END )
			break;
		else
			assert( 0 );
	}
}

/*
* SCR_DrawRawString - Doesn't care about aligning. Returns drawn len.
* It can stop when reaching maximum width when a value has been parsed.
*/
static int SCR_DrawRawString( int x, int y, const char *str, int maxwidth, struct mufont_s *font, vec4_t color )
{
	int xoffset = 0, yoffset = 0;
	vec4_t scolor;
	const char *s, *olds;
	int gc, colorindex;
	qwchar num;

	if( !str )
		return 0;

	if( !font )
		font = cls.fontSystemSmall;

	if( maxwidth < 0 )
		maxwidth = 0;

	Vector4Copy( color, scolor );

	s = str;

	while( s )
	{
		olds = s;
		gc = Q_GrabWCharFromColorString( &s, &num, &colorindex );
		if( gc == GRABCHAR_CHAR )
		{
			if( num == '\n' )
				break;
			if( num < ' ' )
				continue;
			if( num >= font->numchars || !( font->chars[num].height ) )
				num = REPLACEMENT_CHAR;

			if( maxwidth && ( ( xoffset + font->chars[num].width ) > maxwidth ) )
			{
				s = olds;
				break;
			}

			if( num != ' ' )
				R_DrawStretchPic( x+xoffset, y+yoffset, font->chars[num].width, font->chars[num].height,
				font->chars[num].s1, font->chars[num].t1, font->chars[num].s2, font->chars[num].t2,
				scolor, font->shader );

			xoffset += font->chars[num].width;
		}
		else if( gc == GRABCHAR_COLOR )
		{
			assert( ( unsigned )colorindex < MAX_S_COLORS );
			VectorCopy( color_table[colorindex], scolor );
		}
		else if( gc == GRABCHAR_END )
			break;
		else
			assert( 0 );
	}

	return ( s - str );
}

/*
* SCR_DrawString
*/
void SCR_DrawString( int x, int y, int align, const char *str, struct mufont_s *font, vec4_t color )
{
	int width;

	if( !str )
		return;

	if( !font )
		font = cls.fontSystemSmall;

	width = SCR_strWidth( str, font, 0 );
	if( width )
	{
		x = SCR_HorizontalAlignForString( x, align, width );
		y = SCR_VerticalAlignForString( y, align, font->fontheight );

		if( y <= -font->fontheight || y >= (int)viddef.height )
			return; // totally off screen

		if( x <= -width || x >= (int)viddef.width )
			return; // totally off screen

		SCR_DrawRawString( x, y, str, 0, font, color );
	}
}

/*
* SCR_DrawStringWidth - clamp to width in pixels. Returns drawn len
*/
int SCR_DrawStringWidth( int x, int y, int align, const char *str, int maxwidth, struct mufont_s *font, vec4_t color )
{
	int width;

	if( !str )
		return 0;

	if( !font )
		font = cls.fontSystemSmall;

	if( maxwidth < 0 )
		maxwidth = 0;

	width = SCR_strWidth( str, font, 0 );
	if( width )
	{
		if( maxwidth && width > maxwidth )
			width = maxwidth;

		x = SCR_HorizontalAlignForString( x, align, width );
		y = SCR_VerticalAlignForString( y, align, font->fontheight );

		return SCR_DrawRawString( x, y, str, maxwidth, font, color );
	}

	return 0;
}

/*
* SCR_DrawFillRect
* 
* Fills a box of pixels with a single color
*/
void SCR_DrawFillRect( int x, int y, int w, int h, vec4_t color )
{
	R_DrawStretchPic( x, y, w, h, 0, 0, 1, 1, color, cls.whiteShader );
}

/*
===============================================================================

BAR GRAPHS

===============================================================================
*/

/*
* CL_AddNetgraph
* 
* A new packet was just parsed
*/
void CL_AddNetgraph( void )
{
	int i;
	int ping;

	// if using the debuggraph for something else, don't
	// add the net lines
	if( scr_timegraph->integer )
		return;

	for( i = 0; i < cls.netchan.dropped; i++ )
		SCR_DebugGraph( 30.0f, 0.655f, 0.231f, 0.169f );

	for( i = 0; i < cl.suppressCount; i++ )
		SCR_DebugGraph( 30.0f, 0.0f, 1.0f, 0.0f );

	// see what the latency was on this packet
	ping = cls.realtime - cl.cmd_time[cls.ucmdAcknowledged & CMD_MASK];
	ping /= 30;
	if( ping > 30 )
		ping = 30;
	SCR_DebugGraph( ping, 1.0f, 0.75f, 0.06f );
}


typedef struct
{
	float value;
	vec4_t color;
} graphsamp_t;

static int current;
static graphsamp_t values[1024];

/*
* SCR_DebugGraph
*/
void SCR_DebugGraph( float value, float r, float g, float b )
{
	values[current].value = value;
	values[current].color[0] = r;
	values[current].color[1] = g;
	values[current].color[2] = b;
	values[current].color[3] = 1.0f;

	current++;
	current &= 1023;
}

/*
* SCR_DrawDebugGraph
*/
static void SCR_DrawDebugGraph( void )
{
	int a, x, y, w, i, h;
	float v;

	//
	// draw the graph
	//
	w = viddef.width;
	x = 0;
	y = 0+viddef.height;
	SCR_DrawFillRect( x, y-scr_graphheight->integer,
		w, scr_graphheight->integer, colorBlack );

	for( a = 0; a < w; a++ )
	{
		i = ( current-1-a+1024 ) & 1023;
		v = values[i].value;
		v = v*scr_graphscale->integer + scr_graphshift->integer;

		if( v < 0 )
			v += scr_graphheight->integer * ( 1+(int)( -v/scr_graphheight->integer ) );
		h = (int)v % scr_graphheight->integer;
		SCR_DrawFillRect( x+w-1-a, y - h, 1, h, values[i].color );
	}
}

//============================================================================

/*
* SCR_InitScreen
*/
void SCR_InitScreen( void )
{
	scr_consize = Cvar_Get( "scr_consize", "0.5", CVAR_ARCHIVE );
	scr_conspeed = Cvar_Get( "scr_conspeed", "3", CVAR_ARCHIVE );
	scr_netgraph = Cvar_Get( "netgraph", "0", 0 );
	scr_timegraph = Cvar_Get( "timegraph", "0", 0 );
	scr_debuggraph = Cvar_Get( "debuggraph", "0", 0 );
	scr_graphheight = Cvar_Get( "graphheight", "32", 0 );
	scr_graphscale = Cvar_Get( "graphscale", "1", 0 );
	scr_graphshift = Cvar_Get( "graphshift", "0", 0 );
	scr_forceclear = Cvar_Get( "scr_forceclear", "0", CVAR_READONLY );

	scr_initialized = qtrue;
}

//=============================================================================

/*
* SCR_RunConsole
* 
* Scroll it up or down
*/
void SCR_RunConsole( int msec )
{
	// decide on the height of the console
	if( cls.key_dest == key_console )
		scr_conlines = bound( 0.1f, scr_consize->value, 1.0f );
	else
		scr_conlines = 0;

	scr_con_previous = scr_con_current;
	if( scr_conlines < scr_con_current )
	{
		scr_con_current -= scr_conspeed->value * msec * 0.001f;
		if( scr_conlines > scr_con_current )
			scr_con_current = scr_conlines;

	}
	else if( scr_conlines > scr_con_current )
	{
		scr_con_current += scr_conspeed->value * msec * 0.001f;
		if( scr_conlines < scr_con_current )
			scr_con_current = scr_conlines;
	}
}

/*
* SCR_DrawConsole
*/
static void SCR_DrawConsole( void )
{
	if( scr_con_current )
	{
		Con_DrawConsole( scr_con_current );
		return;
	}

	if( cls.state == CA_ACTIVE && ( cls.key_dest == key_game || cls.key_dest == key_message ) )
	{
		Con_DrawNotify(); // only draw notify in game
	}
}

/*
* SCR_BeginLoadingPlaque
*/
void SCR_BeginLoadingPlaque( void )
{
	CL_SoundModule_StopAllSounds();

	memset( cl.configstrings, 0, sizeof( cl.configstrings ) );

	scr_conlines = 0;       // none visible
	scr_draw_loading = 2;   // clear to black first
	SCR_UpdateScreen();

	CL_ShutdownMedia( qfalse );
}

/*
* SCR_EndLoadingPlaque
*/
void SCR_EndLoadingPlaque( void )
{
	cls.disable_screen = 0;
	Con_ClearNotify();
	CL_InitMedia( qfalse );
}


//=======================================================

/*
* SCR_RegisterConsoleMedia
*/
void SCR_RegisterConsoleMedia( void )
{
	cls.whiteShader = R_RegisterPic( "$whiteimage" );
	cls.consoleShader = R_RegisterPic( "gfx/ui/console" );
	SCR_InitFonts();
}

/*
* SCR_ShutDownConsoleMedia
*/
void SCR_ShutDownConsoleMedia( void )
{
	SCR_ShutdownFonts();
}

//============================================================================

/*
* SCR_RenderView
*/
static void SCR_RenderView( float stereo_separation )
{
	if( cls.demo.playing )
	{
		if( cl_timedemo->integer )
		{
			if( !cl.timedemo.start )
				cl.timedemo.start = Sys_Milliseconds();
			cl.timedemo.frames++;
		}
	}

	// frame is not valid until we load the CM data
	if( CM_ClientLoad( cl.cms ) )
		CL_GameModule_RenderView( stereo_separation );
}

//============================================================================

/*
* SCR_UpdateScreen
* 
* This is called every frame, and can also be called explicitly to flush
* text to the screen.
*/
void SCR_UpdateScreen( void )
{
	static dynvar_t *updatescreen = NULL;
	int numframes;
	int i;
	float separation[2];
	qboolean scr_cinematic;

	if( !updatescreen )
		updatescreen = Dynvar_Create( "updatescreen", qfalse, DYNVAR_WRITEONLY, DYNVAR_READONLY );

	// if the screen is disabled (loading plaque is up, or vid mode changing)
	// do nothing at all
	if( cls.disable_screen )
	{
		if( Sys_Milliseconds() - cls.disable_screen > 120000 )
		{
			cls.disable_screen = 0;
			Com_Printf( "Loading plaque timed out.\n" );
		}
		return;
	}

	if( !scr_initialized || !con_initialized || !cls.mediaInitialized )
		return;     // not initialized yet

	Con_CheckResize();

	/*
	** range check cl_camera_separation so we don't inadvertently fry someone's
	** brain
	*/
	if( cl_stereo_separation->value > 1.0 )
		Cvar_SetValue( "cl_stereo_separation", 1.0 );
	else if( cl_stereo_separation->value < 0 )
		Cvar_SetValue( "cl_stereo_separation", 0.0 );

	if( cl_stereo->integer )
	{
		numframes = 2;
		separation[0] = -cl_stereo_separation->value / 2;
		separation[1] =  cl_stereo_separation->value / 2;
	}
	else
	{
		separation[0] = 0;
		separation[1] = 0;
		numframes = 1;
	}

	// avoid redrawing fullscreen cinematics unless damaged by console drawing
	scr_cinematic = cls.state == CA_CINEMATIC ? qtrue : qfalse;
//	if( scr_cinematic && !cl.cin.redraw && !scr_con_current && !scr_con_previous ) {
//		return;
//	}

	if( cls.cgameActive && cls.state < CA_LOADING ) {
		// this is when we've finished loading cgame media and are waiting
		// for the first valid snapshot to arrive. keep the loading screen untouched
		return;
	}

	for( i = 0; i < numframes; i++ )
	{
		R_BeginFrame( separation[i], scr_cinematic || scr_forceclear->integer ? qtrue : qfalse );

		if( scr_draw_loading == 2 )
		{ 
			// loading plaque over black screen
			scr_draw_loading = 0;
			CL_UIModule_DrawConnectScreen( qtrue );
		}
		// if a cinematic is supposed to be running, handle menus
		// and console specially
		else if( scr_cinematic )
		{
			SCR_DrawCinematic();
			SCR_DrawConsole();
		}
		else if( cls.state == CA_DISCONNECTED )
		{
			CL_UIModule_Refresh( qtrue );
			SCR_DrawConsole();
		}
		else if( cls.state == CA_GETTING_TICKET || cls.state == CA_CONNECTING || cls.state == CA_CONNECTED || cls.state == CA_HANDSHAKE )
		{
			CL_UIModule_DrawConnectScreen( qtrue );
		}
		else if( cls.state == CA_LOADING )
		{
			SCR_RenderView( separation[i] );
			CL_UIModule_DrawConnectScreen( qfalse );
		}
		else if( cls.state == CA_ACTIVE )
		{
			SCR_RenderView( separation[i] );

			CL_UIModule_Refresh( qfalse );

			if( scr_timegraph->integer )
				SCR_DebugGraph( cls.frametime*300, 1, 1, 1 );

			if( scr_debuggraph->integer || scr_timegraph->integer || scr_netgraph->integer )
				SCR_DrawDebugGraph();

			SCR_DrawConsole();
		}

		// wsw : aiwa : call any listeners so they can draw their stuff
		Dynvar_CallListeners( updatescreen, NULL );

		R_EndFrame();
	}
}
