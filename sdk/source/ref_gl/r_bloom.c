/*
Copyright (C) Forest Hale
Copyright (C) 2006-2007 German Garcia

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

// r_bloom.c: 2D lighting post process effect

#include "r_local.h"

/*
==============================================================================

LIGHT BLOOMS

==============================================================================
*/

static float Diamond8x[8][8] =
{
	{ 0.0f, 0.0f, 0.0f, 0.1f, 0.1f, 0.0f, 0.0f, 0.0f, },
	{ 0.0f, 0.0f, 0.2f, 0.3f, 0.3f, 0.2f, 0.0f, 0.0f, },
	{ 0.0f, 0.2f, 0.4f, 0.6f, 0.6f, 0.4f, 0.2f, 0.0f, },
	{ 0.1f, 0.3f, 0.6f, 0.9f, 0.9f, 0.6f, 0.3f, 0.1f, },
	{ 0.1f, 0.3f, 0.6f, 0.9f, 0.9f, 0.6f, 0.3f, 0.1f, },
	{ 0.0f, 0.2f, 0.4f, 0.6f, 0.6f, 0.4f, 0.2f, 0.0f, },
	{ 0.0f, 0.0f, 0.2f, 0.3f, 0.3f, 0.2f, 0.0f, 0.0f, },
	{ 0.0f, 0.0f, 0.0f, 0.1f, 0.1f, 0.0f, 0.0f, 0.0f }
};

static float Diamond6x[6][6] =
{
	{ 0.0f, 0.0f, 0.1f, 0.1f, 0.0f, 0.0f, },
	{ 0.0f, 0.3f, 0.5f, 0.5f, 0.3f, 0.0f, },
	{ 0.1f, 0.5f, 0.9f, 0.9f, 0.5f, 0.1f, },
	{ 0.1f, 0.5f, 0.9f, 0.9f, 0.5f, 0.1f, },
	{ 0.0f, 0.3f, 0.5f, 0.5f, 0.3f, 0.0f, },
	{ 0.0f, 0.0f, 0.1f, 0.1f, 0.0f, 0.0f }
};

static float Diamond4x[4][4] =
{
	{ 0.3f, 0.4f, 0.4f, 0.3f, },
	{ 0.4f, 0.9f, 0.9f, 0.4f, },
	{ 0.4f, 0.9f, 0.9f, 0.4f, },
	{ 0.3f, 0.4f, 0.4f, 0.3f }
};

static int BLOOM_SIZE;

static image_t *r_bloomscreentexture;
static image_t *r_bloomeffecttexture;
static image_t *r_bloombackuptexture;
static image_t *r_bloomdownsamplingtexture;

static int r_screendownsamplingtexture_size;
static int screen_texture_width, screen_texture_height;
static int r_screenbackuptexture_width, r_screenbackuptexture_height;

// current refdef size:
static int curView_x;
static int curView_y;
static int curView_width;
static int curView_height;

// texture coordinates of screen data inside screentexture
static float screenTex_tcw;
static float screenTex_tch;

static int sample_width;
static int sample_height;

// texture coordinates of adjusted textures
static float sampleText_tcw;
static float sampleText_tch;

/*
=================
R_Bloom_InitBackUpTexture
=================
*/
static void R_Bloom_InitBackUpTexture( int width, int height )
{
	qbyte *data;

	data = Mem_TempMalloc( width * height * 4 );
	memset( data, 0, width * height * 4 );

	r_screenbackuptexture_width = width;
	r_screenbackuptexture_height = height;

	r_bloombackuptexture = R_LoadPic( "***r_bloombackuptexture***", &data, width, height, IT_CLAMP|IT_NOMIPMAP|IT_NOCOMPRESS|IT_NOPICMIP, 3 );

	Mem_TempFree( data );
}

/*
=================
R_Bloom_InitEffectTexture
=================
*/
static void R_Bloom_InitEffectTexture( void )
{
	qbyte *data;
	int		limit;

	if( r_bloom_sample_size->integer < 32 )
		Cvar_ForceSet( "r_bloom_sample_size", "32" );

	// make sure bloom size doesn't have stupid values
	limit = min( r_bloom_sample_size->integer, min( screen_texture_width, screen_texture_height ) );

	if( glConfig.ext.texture_non_power_of_two )
		BLOOM_SIZE = limit;
	else	// make sure bloom size is a power of 2
		for( BLOOM_SIZE = 32; (BLOOM_SIZE<<1) <= limit; BLOOM_SIZE <<= 1 );

	if( BLOOM_SIZE != r_bloom_sample_size->integer )
		Cvar_ForceSet( "r_bloom_sample_size", va( "%i", BLOOM_SIZE ) );

	data = Mem_TempMalloc( BLOOM_SIZE * BLOOM_SIZE * 4 );
	memset( data, 0, BLOOM_SIZE * BLOOM_SIZE * 4 );

	r_bloomeffecttexture = R_LoadPic( "***r_bloomeffecttexture***", &data, BLOOM_SIZE, BLOOM_SIZE, IT_CLAMP|IT_NOMIPMAP|IT_NOCOMPRESS|IT_NOPICMIP, 3 );

	Mem_TempFree( data );
}

/*
=================
R_Bloom_InitTextures
=================
*/
static void R_Bloom_InitTextures( void )
{
	qbyte *data;
	int size;

	if( glConfig.ext.texture_non_power_of_two )
	{
		screen_texture_width = glState.width;
		screen_texture_height = glState.height;
	}
	else
	{
		// find closer power of 2 to screen size 
		for (screen_texture_width = 1;screen_texture_width < glState.width;screen_texture_width <<= 1);
		for (screen_texture_height = 1;screen_texture_height < glState.height;screen_texture_height <<= 1);
	}

	// disable blooms if we can't handle a texture of that size
	if( screen_texture_width > glConfig.maxTextureSize || screen_texture_height > glConfig.maxTextureSize )
	{
		screen_texture_width = screen_texture_height = 0;
		Cvar_ForceSet( "r_bloom", "0" );
		Com_Printf( S_COLOR_YELLOW "WARNING: 'R_InitBloomScreenTexture' too high resolution for light bloom, effect disabled\n" );
		return;
	}

	// init the screen texture
	size = screen_texture_width * screen_texture_height * 4;
	data = Mem_TempMalloc( size );
	memset( data, 255, size );
	r_bloomscreentexture = R_LoadPic( "***r_bloomscreentexture***", &data, screen_texture_width, screen_texture_height, IT_CLAMP|IT_NOMIPMAP|IT_NOCOMPRESS|IT_NOPICMIP, 3 );
	Mem_TempFree( data );

	// validate bloom size and init the bloom effect texture
	R_Bloom_InitEffectTexture();

	// if screensize is more than 2x the bloom effect texture, set up for stepped downsampling
	r_bloomdownsamplingtexture = NULL;
	r_screendownsamplingtexture_size = 0;

	if( (glState.width > (BLOOM_SIZE * 2) || glState.height > (BLOOM_SIZE * 2)) && !r_bloom_fast_sample->integer )
	{
		r_screendownsamplingtexture_size = (int)( BLOOM_SIZE * 2 );
		data = Mem_TempMalloc( r_screendownsamplingtexture_size * r_screendownsamplingtexture_size * 4 );
		memset( data, 0, r_screendownsamplingtexture_size * r_screendownsamplingtexture_size * 4 );
		r_bloomdownsamplingtexture = R_LoadPic( "***r_bloomdownsamplingtexture***", &data, r_screendownsamplingtexture_size, r_screendownsamplingtexture_size, IT_CLAMP|IT_NOMIPMAP|IT_NOCOMPRESS|IT_NOPICMIP, 3 );
		Mem_TempFree( data );
	}

	// init the screen backup texture
	if( r_screendownsamplingtexture_size )
		R_Bloom_InitBackUpTexture( r_screendownsamplingtexture_size, r_screendownsamplingtexture_size );
	else
		R_Bloom_InitBackUpTexture( BLOOM_SIZE, BLOOM_SIZE );
}

/*
=================
R_InitBloomTextures
=================
*/
void R_InitBloomTextures( void )
{
	BLOOM_SIZE = 0;
	if( !r_bloom->integer )
		return;
	R_Bloom_InitTextures();
}

/*
=================
R_Bloom_SamplePass
=================
*/
static inline void R_Bloom_SamplePass( int xpos, int ypos )
{
	qglBegin( GL_QUADS );
	qglTexCoord2f( 0, sampleText_tch );
	qglVertex2f( xpos, ypos );
	qglTexCoord2f( 0, 0 );
	qglVertex2f( xpos, ypos+sample_height );
	qglTexCoord2f( sampleText_tcw, 0 );
	qglVertex2f( xpos+sample_width, ypos+sample_height );
	qglTexCoord2f( sampleText_tcw, sampleText_tch );
	qglVertex2f( xpos+sample_width, ypos );
	qglEnd();
}

/*
=================
R_Bloom_Quad
=================
*/
static inline void R_Bloom_Quad( int x, int y, int w, int h, float texwidth, float texheight )
{
	qglBegin( GL_QUADS );
	qglTexCoord2f( 0, texheight );
	qglVertex2f( x, glState.height-h-y );
	qglTexCoord2f( 0, 0 );
	qglVertex2f( x, glState.height-y );
	qglTexCoord2f( texwidth, 0 );
	qglVertex2f( x+w, glState.height-y );
	qglTexCoord2f( texwidth, texheight );
	qglVertex2f( x+w, glState.height-h );
	qglEnd();
}

/*
=================
R_Bloom_DrawEffect
=================
*/
static void R_Bloom_DrawEffect( void )
{
	GL_Bind( 0, r_bloomeffecttexture );
	GL_TexEnv( GL_MODULATE );
	GL_SetState( GLSTATE_NO_DEPTH_TEST|GLSTATE_SRCBLEND_ONE|GLSTATE_DSTBLEND_ONE );
	qglColor4f( r_bloom_alpha->value, r_bloom_alpha->value, r_bloom_alpha->value, 1.0f );

	qglBegin( GL_QUADS );
	qglTexCoord2f( 0, sampleText_tch );
	qglVertex2f( curView_x, curView_y );
	qglTexCoord2f( 0, 0 );
	qglVertex2f( curView_x, curView_y+curView_height );
	qglTexCoord2f( sampleText_tcw, 0 );
	qglVertex2f( curView_x+curView_width, curView_y+curView_height );
	qglTexCoord2f( sampleText_tcw, sampleText_tch );
	qglVertex2f( curView_x+curView_width, curView_y );
	qglEnd();
}

/*
=================
R_Bloom_GeneratexDiamonds
=================
*/
static void R_Bloom_GeneratexDiamonds( void )
{
	int i, j, k;
	float intensity, scale, *diamond;

	// set up sample size workspace
	qglScissor( 0, 0, sample_width, sample_height );
	qglViewport( 0, 0, sample_width, sample_height );
	qglMatrixMode( GL_PROJECTION );
	qglLoadIdentity();
	qglOrtho( 0, sample_width, sample_height, 0, -10, 100 );
	qglMatrixMode( GL_MODELVIEW );
	qglLoadIdentity();

	// copy small scene into r_bloomeffecttexture
	GL_Bind( 0, r_bloomeffecttexture );
	qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, sample_width, sample_height );

	// start modifying the small scene corner
	qglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

	// darkening passes
	if( r_bloom_darken->integer )
	{
		GL_TexEnv( GL_MODULATE );
		GL_SetState( GLSTATE_NO_DEPTH_TEST|GLSTATE_SRCBLEND_DST_COLOR|GLSTATE_DSTBLEND_ZERO );

		for( i = 0; i < r_bloom_darken->integer; i++ )
			R_Bloom_SamplePass( 0, 0 );
		qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, sample_width, sample_height );
	}

	// bluring passes
	GL_SetState( GLSTATE_NO_DEPTH_TEST|GLSTATE_SRCBLEND_ONE|GLSTATE_DSTBLEND_ONE_MINUS_SRC_COLOR );

	if( r_bloom_diamond_size->integer > 7 || r_bloom_diamond_size->integer <= 3 )
	{
		if( r_bloom_diamond_size->integer != 8 )
			Cvar_ForceSet( "r_bloom_diamond_size", "8" );
	}
	else if( r_bloom_diamond_size->integer > 5 )
	{
		if( r_bloom_diamond_size->integer != 6 )
			Cvar_ForceSet( "r_bloom_diamond_size", "6" );
	}
	else if( r_bloom_diamond_size->integer > 3 )
	{
		if( r_bloom_diamond_size->integer != 4 )
			Cvar_ForceSet( "r_bloom_diamond_size", "4" );
	}

	switch( r_bloom_diamond_size->integer )
	{
	case 4:
		k = 2;
		diamond = &Diamond4x[0][0];
		scale = r_bloom_intensity->value * 0.8f;
		break;
	case 6:
		k = 3;
		diamond = &Diamond6x[0][0];
		scale = r_bloom_intensity->value * 0.5f;
		break;
	default:
//	case 8:
		k = 4;
		diamond = &Diamond8x[0][0];
		scale = r_bloom_intensity->value * 0.3f;
		break;
	}

	for( i = 0; i < r_bloom_diamond_size->integer; i++ )
	{
		for( j = 0; j < r_bloom_diamond_size->integer; j++, diamond++ )
		{
			intensity =  *diamond * scale;
			if( intensity < 0.01f )
				continue;

			qglColor4f( intensity, intensity, intensity, 1.0 );
			R_Bloom_SamplePass( i - k, j - k );
		}
	}

	qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, sample_width, sample_height );

	// restore full screen workspace
	qglScissor( 0, 0, glState.width, glState.height );
	qglViewport( 0, 0, glState.width, glState.height );
	qglMatrixMode( GL_PROJECTION );
	qglLoadIdentity();
	qglOrtho( 0, glState.width, glState.height, 0, -10, 100 );
	qglMatrixMode( GL_MODELVIEW );
	qglLoadIdentity();
}

/*
=================
R_Bloom_DownsampleView
=================
*/
static void R_Bloom_DownsampleView( void )
{
	qglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

	if( r_screendownsamplingtexture_size )
	{
		// stepped downsample
		int midsample_width = ( r_screendownsamplingtexture_size * sampleText_tcw );
		int midsample_height = ( r_screendownsamplingtexture_size * sampleText_tch );

		// copy the screen and draw resized
		GL_Bind( 0, r_bloomscreentexture );
		qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, curView_x, glState.height - ( curView_y + curView_height ), curView_width, curView_height );
		R_Bloom_Quad( 0, 0, midsample_width, midsample_height, screenTex_tcw, screenTex_tch );

		// now copy into downsampling (mid-sized) texture
		GL_Bind( 0, r_bloomdownsamplingtexture );
		qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, midsample_width, midsample_height );

		// now draw again in bloom size
		qglColor4f( 0.5f, 0.5f, 0.5f, 1.0f );
		R_Bloom_Quad( 0, 0, sample_width, sample_height, sampleText_tcw, sampleText_tch );

		// now blend the big screen texture into the bloom generation space (hoping it adds some blur)
		GL_SetState( GLSTATE_NO_DEPTH_TEST|GLSTATE_SRCBLEND_ONE|GLSTATE_DSTBLEND_ONE );

		GL_Bind( 0, r_bloomscreentexture );
		R_Bloom_Quad( 0, 0, sample_width, sample_height, screenTex_tcw, screenTex_tch );
		qglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
	}
	else
	{
		// downsample simple
		GL_Bind( 0, r_bloomscreentexture );
		qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, curView_x, glState.height - ( curView_y + curView_height ), curView_width, curView_height );
		R_Bloom_Quad( 0, 0, sample_width, sample_height, screenTex_tcw, screenTex_tch );
	}
}

/*
=================
R_BloomBlend
=================
*/
void R_BloomBlend( const refdef_t *fd )
{
	if( !( fd->rdflags & RDF_BLOOM ) || !r_bloom->integer )
		return;

	if( !BLOOM_SIZE )
		R_Bloom_InitTextures();

	if( screen_texture_width < BLOOM_SIZE || screen_texture_height < BLOOM_SIZE )
		return;

	// set up full screen workspace
	qglScissor( 0, 0, glState.width, glState.height );
	qglViewport( 0, 0, glState.width, glState.height );
	qglMatrixMode( GL_PROJECTION );
	qglLoadIdentity();
	qglOrtho( 0, glState.width, glState.height, 0, -10, 100 );
	qglMatrixMode( GL_MODELVIEW );
	qglLoadIdentity();

	GL_Cull( 0 );
	GL_SetState( GLSTATE_NO_DEPTH_TEST );

	qglColor4f( 1, 1, 1, 1 );

	// set up current sizes
	curView_x = fd->x;
	curView_y = fd->y;
	curView_width = fd->width;
	curView_height = fd->height;

	screenTex_tcw = ( (float)curView_width / (float)screen_texture_width );
	screenTex_tch = ( (float)curView_height / (float)screen_texture_height );
	if( curView_height > curView_width )
	{
		sampleText_tcw = ( (float)curView_width / (float)curView_height );
		sampleText_tch = 1.0f;
	}
	else
	{
		sampleText_tcw = 1.0f;
		sampleText_tch = ( (float)curView_height / (float)curView_width );
	}

	sample_width = ( BLOOM_SIZE * sampleText_tcw );
	sample_height = ( BLOOM_SIZE * sampleText_tch );

	// copy the screen space we'll use to work into the backup texture
	GL_Bind( 0, r_bloombackuptexture );
	qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, r_screenbackuptexture_width, r_screenbackuptexture_height );

	// create the bloom image
	R_Bloom_DownsampleView();
	R_Bloom_GeneratexDiamonds();

	// restore the screen-backup to the screen
	GL_SetState( GLSTATE_NO_DEPTH_TEST );
	GL_Bind( 0, r_bloombackuptexture );

	qglColor4f( 1, 1, 1, 1 );

	R_Bloom_Quad( 0, 0, r_screenbackuptexture_width, r_screenbackuptexture_height, 1.0f, 1.0f );

	qglScissor( ri.scissor[0], ri.scissor[1], ri.scissor[2], ri.scissor[3] );

	R_Bloom_DrawEffect();

	qglViewport( ri.viewport[0], ri.viewport[1], ri.viewport[2], ri.viewport[3] );
}
