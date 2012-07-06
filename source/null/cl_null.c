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

// cl_null.c -- this file can stub out the entire client system
// for pure dedicated servers

#include "../qcommon/qcommon.h"
#include "../client/keys.h"
#include "../client/vid.h"
#include "../client/console.h"
#include "../ref_gl/r_public.h"

static void Key_Bind_Null_f( void )
{
}

void CL_Init( void )
{
}

void CL_InitDynvars( void )
{
}

void CL_Disconnect( const char *message )
{
}

void CL_Shutdown( void )
{
}

void CL_Frame( int realmsec, int gamemsec )
{
}

void CL_ParseServerMessage( msg_t *msg )
{
}

void CL_Netchan_Transmit( msg_t *msg )
{
}

void Con_Print( const char *text )
{
}

int CL_GetKeyDest( void )
{
	return -1;
}

int CL_GetClientState( void )
{
	return 0; // CA_UNINITIALIZED
}

void SCR_BeginLoadingPlaque( void )
{
}

void SCR_EndLoadingPlaque( void )
{
}

void Key_Init( void )
{
	Cmd_AddCommand( "bind", Key_Bind_Null_f );
}

void Key_Shutdown( void )
{
	Cmd_RemoveCommand( "bind" );
}

keydest_t Key_DelegatePush( key_delegate_f keydel, key_char_delegate_f chardel )
{
	return key_console;
}

void Key_DelegatePop( keydest_t next_dest )
{
}

struct mufont_s;

struct mufont_s *SCR_RegisterFont( const char *name )
{
	return NULL;
}

void SCR_DrawString( int x, int y, int align, const char *str, struct mufont_s *font, vec4_t color )
{
}

int SCR_DrawStringWidth( int x, int y, int align, const char *str, int maxwidth, struct mufont_s *font, vec4_t color )
{
	return 0;
}

void SCR_DrawClampString( int x, int y, const char *str, int xmin, int ymin, int xmax, int ymax, struct mufont_s *font, vec4_t color )
{
}

void SCR_DrawRawChar( int x, int y, qwchar num, struct mufont_s *font, vec4_t color )
{
}

size_t SCR_strHeight( struct mufont_s *font )
{
	return 0;
}

size_t SCR_strWidth( const char *str, struct mufont_s *font, int maxlen )
{
	return 0;
}

size_t SCR_StrlenForWidth( const char *str, struct mufont_s *font, size_t maxwidth )
{
	return 0;
}

struct shader_s *R_RegisterPic( const char *name )
{
	return NULL;
}

void R_DrawStretchPic( int x, int y, int w, int h, float s1, float t1, float s2, float t2, const float *color, const struct shader_s *shader )
{
}

void R_DrawRotatedStretchPic( int x, int y, int w, int h, float s1, float t1, float s2, float t2, float angle, const vec4_t color, const struct shader_s *shader )
{
}

struct poly_s;

void R_DrawStretchPoly( const struct poly_s *p, float x_offset, float y_offset )
{
}

viddef_t viddef;

qboolean con_initialized;
