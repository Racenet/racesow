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

#include "r_local.h"

#if defined ( __MACOSX__ )
#include "libjpeg/jpeglib.h"
#include "png/png.h"
#else
#include "jpeglib.h"
#include "png.h"
#endif


#include <setjmp.h>

#define	MAX_GLIMAGES	    4096
#define IMAGES_HASH_SIZE    64

static image_t images[MAX_GLIMAGES];
static image_t images_hash_headnode[IMAGES_HASH_SIZE], *free_images;
static unsigned int image_cur_hash;

static int *r_8to24table;

static mempool_t *r_texturesPool;
static char *r_imagePathBuf, *r_imagePathBuf2;
static size_t r_sizeof_imagePathBuf, r_sizeof_imagePathBuf2;

#undef ENSUREBUFSIZE
#define ENSUREBUFSIZE(buf,need) \
	if( r_sizeof_ ##buf < need ) \
	{ \
		if( r_ ##buf ) \
			Mem_Free( r_ ##buf ); \
		r_sizeof_ ##buf += (((need) & (MAX_QPATH-1))+1) * MAX_QPATH; \
		r_ ##buf = Mem_Alloc( r_texturesPool, r_sizeof_ ##buf ); \
	}

int gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
int gl_filter_max = GL_LINEAR;

int gl_filter_depth = GL_LINEAR;

int gl_anisotropic_filter = 0;

void GL_SelectTexture( int tmu )
{
	if( !glConfig.ext.multitexture )
		return;
	if( tmu == glState.currentTMU )
		return;

	glState.currentTMU = tmu;

	if( qglActiveTextureARB )
	{
		qglActiveTextureARB( tmu + GL_TEXTURE0_ARB );
		qglClientActiveTextureARB( tmu + GL_TEXTURE0_ARB );
	}
	else if( qglSelectTextureSGIS )
	{
		qglSelectTextureSGIS( tmu + GL_TEXTURE0_SGIS );
	}
}

void GL_TexEnv( GLenum mode )
{
	if( mode != ( GLenum )glState.currentEnvModes[glState.currentTMU] )
	{
		qglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode );
		glState.currentEnvModes[glState.currentTMU] = ( int )mode;
	}
}

void GL_Bind( int tmu, image_t *tex )
{
	GL_SelectTexture( tmu );

	if( r_nobind->integer )  // performance evaluation option
		tex = r_notexture;
	if( glState.currentTextures[tmu] == tex->texnum )
		return;

	glState.currentTextures[tmu] = tex->texnum;
	if( tex->flags & IT_CUBEMAP )
		qglBindTexture( GL_TEXTURE_CUBE_MAP_ARB, tex->texnum );
	else if( tex->depth != 1 )
		qglBindTexture( GL_TEXTURE_3D, tex->texnum );
	else
		qglBindTexture( GL_TEXTURE_2D, tex->texnum );
}

void GL_LoadTexMatrix( const mat4x4_t m )
{
	qglMatrixMode( GL_TEXTURE );
	qglLoadMatrixf( m );
	glState.texIdentityMatrix[glState.currentTMU] = qfalse;
}

void GL_LoadIdentityTexMatrix( void )
{
	if( !glState.texIdentityMatrix[glState.currentTMU] )
	{
		qglMatrixMode( GL_TEXTURE );
		qglLoadIdentity();
		glState.texIdentityMatrix[glState.currentTMU] = qtrue;
	}
}

void GL_EnableTexGen( int coord, int mode )
{
	int tmu = glState.currentTMU;
	int bit, gen;

	bit = 1 << (coord - GL_S);
	gen = GL_TEXTURE_GEN_S + (coord - GL_S);

	assert( gen == bound( GL_TEXTURE_GEN_S, gen, GL_TEXTURE_GEN_Q ) );

	if( mode )
	{
		if( !( glState.genSTEnabled[tmu] & bit ) )
		{
			qglEnable( gen );
			glState.genSTEnabled[tmu] |= bit;
		}
		qglTexGeni( coord, GL_TEXTURE_GEN_MODE, mode );
	}
	else
	{
		if( glState.genSTEnabled[tmu] & bit )
		{
			qglDisable( gen );
			glState.genSTEnabled[tmu] &= ~bit;
		}
	}
}

void GL_SetTexCoordArrayMode( int mode )
{
	int tmu = glState.currentTMU;
	int cmode = glState.texCoordArrayMode[tmu];

	if( cmode != mode )
	{
		if( cmode == GL_TEXTURE_COORD_ARRAY )
			qglDisableClientState( GL_TEXTURE_COORD_ARRAY );
		else if( cmode == GL_TEXTURE_CUBE_MAP_ARB )
			qglDisable( GL_TEXTURE_CUBE_MAP_ARB );

		if( mode == GL_TEXTURE_COORD_ARRAY )
			qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
		else if( mode == GL_TEXTURE_CUBE_MAP_ARB )
			qglEnable( GL_TEXTURE_CUBE_MAP_ARB );

		glState.texCoordArrayMode[tmu] = mode;
	}
}

typedef struct
{
	char *name;
	int minimize, maximize;
} glmode_t;

glmode_t modes[] = {
	{ "GL_NEAREST", GL_NEAREST, GL_NEAREST },
	{ "GL_LINEAR", GL_LINEAR, GL_LINEAR },
	{ "GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST },
	{ "GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR },
	{ "GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST },
	{ "GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR }
};

#define NUM_GL_MODES ( sizeof( modes ) / sizeof( glmode_t ) )

/*
* R_TextureMode
*/
void R_TextureMode( char *string )
{
	int i;
	image_t	*glt;

	for( i = 0; i < NUM_GL_MODES; i++ )
	{
		if( !Q_stricmp( modes[i].name, string ) )
			break;
	}

	if( i == NUM_GL_MODES )
	{
		Com_Printf( "R_TextureMode: bad filter name\n" );
		return;
	}

	gl_filter_min = modes[i].minimize;
	gl_filter_max = modes[i].maximize;

	// change all the existing mipmap texture objects
	for( i = 1, glt = images; i < MAX_GLIMAGES; i++, glt++ )
	{
		if( !glt->texnum ) {
			continue;
		}
		if( glt->flags & (IT_NOFILTERING|IT_DEPTH) ) {
			continue;
		}

		GL_Bind( 0, glt );

		if( !( glt->flags & IT_NOMIPMAP ) )
		{
			qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min );
			qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max );
		}
		else 
		{
			qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max );
			qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max );
		}
	}
}

/*
* R_AnisotropicFilter
*/
void R_AnisotropicFilter( int value )
{
	int i, old;
	image_t	*glt;

	if( !glConfig.ext.texture_filter_anisotropic )
		return;

	old = gl_anisotropic_filter;
	gl_anisotropic_filter = bound( 1, value, glConfig.maxTextureFilterAnisotropic );
	if( gl_anisotropic_filter == old )
		return;

	// change all the existing mipmap texture objects
	for( i = 1, glt = images; i < MAX_GLIMAGES; i++, glt++ )
	{
		if( !glt->texnum ) {
			continue;
		}
		if( (glt->flags & (IT_NOFILTERING|IT_DEPTH|IT_NOMIPMAP)) ) {
			continue;
		}

		GL_Bind( 0, glt );
		if( glt->upload_depth != 1 )
			qglTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_MAX_ANISOTROPY_EXT, gl_anisotropic_filter );
		else
			qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, gl_anisotropic_filter );
	}
}

/*
* R_ImageList_f
*/
void R_ImageList_f( void )
{
	int i, num_depth = 0, bytes;
	int numImages;
	image_t	*image;
	double texels = 0, add, total_bytes = 0;
	double depth_texels = 0;

	Com_Printf( "------------------\n" );

	numImages = 0;
	for( i = 0, image = images; i < MAX_GLIMAGES; i++, image++ )
	{
		if( !image->texnum ) {
			continue;
		}
		if( !image->upload_width || !image->upload_height || !image->upload_depth ) {
			continue;
		}

		add = image->upload_width * image->upload_height * image->upload_depth;
		if( !(image->flags & (IT_DEPTH|IT_NOFILTERING|IT_NOMIPMAP)) )
			add = (unsigned)floor( add / 0.75 );
		if( image->flags & IT_CUBEMAP )
			add *= 6;

		if( image->flags & IT_DEPTH )
		{
			num_depth++;
			depth_texels += image->upload_width * image->upload_height * image->upload_depth;
			Com_Printf( " %4i %4i %4i: %s\n", image->upload_width, image->upload_height, image->upload_depth, 
				image->name );
		}
		else
		{
			texels += add;
			bytes = add * (image->flags & IT_LUMINANCE ? 1 : 4);
			total_bytes += bytes;

			Com_Printf( " %4i %4i%s: %s%s%s %.1f KB\n", image->upload_width, image->upload_height,
				image->upload_depth > 1 ? va( " %4i", image->upload_depth ) : "",
				image->name, image->extension, ((image->flags & (IT_NOMIPMAP|IT_NOFILTERING)) ? "" : " (mip)"), bytes / 1024.0 );
		}
		numImages++;
	}

	Com_Printf( "Total texels count (counting mipmaps, approx): %.0f\n", texels );
	Com_Printf( "%i RGBA images, totalling %.3f megabytes\n", numImages, total_bytes / 1048576.0 );
	if( num_depth )
		Com_Printf( "%i depth images, totalling %.0f texels\n", num_depth, depth_texels );
}

/*
=================================================================

TEMPORARY IMAGE BUFFERS

=================================================================
*/

enum
{
	TEXTURE_LOADING_BUF0,TEXTURE_LOADING_BUF1,TEXTURE_LOADING_BUF2,TEXTURE_LOADING_BUF3,TEXTURE_LOADING_BUF4,TEXTURE_LOADING_BUF5,
	TEXTURE_RESAMPLING_BUF,
	TEXTURE_LINE_BUF,
	TEXTURE_CUT_BUF,
	TEXTURE_FLIPPING_BUF0,TEXTURE_FLIPPING_BUF1,TEXTURE_FLIPPING_BUF2,TEXTURE_FLIPPING_BUF3,TEXTURE_FLIPPING_BUF4,TEXTURE_FLIPPING_BUF5,

	NUM_IMAGE_BUFFERS
};

static qbyte *r_aviBuffer;

static qbyte *r_imageBuffers[NUM_IMAGE_BUFFERS];
static size_t r_imageBufSize[NUM_IMAGE_BUFFERS];

#define R_PrepareImageBuffer(buffer,size) _R_PrepareImageBuffer(buffer,size,__FILE__,__LINE__)

/*
* R_PrepareImageBuffer
*/
static qbyte *_R_PrepareImageBuffer( int buffer, size_t size, const char *filename, int fileline )
{
	if( r_imageBufSize[buffer] < size )
	{
		r_imageBufSize[buffer] = size;
		if( r_imageBuffers[buffer] )
			Mem_Free( r_imageBuffers[buffer] );
		r_imageBuffers[buffer] = _Mem_Alloc( r_texturesPool, size, 0, 0, filename, fileline );
	}

	memset( r_imageBuffers[buffer], 255, size );

	return r_imageBuffers[buffer];
}

/*
* R_FreeImageBuffers
*/
void R_FreeImageBuffers( void )
{
	int i;

	for( i = 0; i < NUM_IMAGE_BUFFERS; i++ )
	{
		if( r_imageBuffers[i] )
		{
			Mem_Free( r_imageBuffers[i] );
			r_imageBuffers[i] = NULL;
		}
		r_imageBufSize[i] = 0;
	}
}

/*
* R_SwapBlueRed
*/
static void R_SwapBlueRed( qbyte *data, int width, int height, int samples )
{
	int i, j, size;

	size = width * height;
	for( i = 0; i < size; i++, data += samples )
	{
		j = data[0];
		data[0] = data[2];
		data[2] = j;
		// data[0] ^= data[2];
		// data[2] = data[0] ^ data[2];
		// data[0] ^= data[2];
	}
}

/*
=================================================================

PCX LOADING

=================================================================
*/

typedef struct
{
	char manufacturer;
	char version;
	char encoding;
	char bits_per_pixel;
	unsigned short xmin, ymin, xmax, ymax;
	unsigned short hres, vres;
	unsigned char palette[48];
	char reserved;
	char color_planes;
	unsigned short bytes_per_line;
	unsigned short palette_type;
	char filler[58];
	unsigned char data;         // unbounded
} pcx_t;

static qbyte pcx_pal[768];

/*
* LoadPCX
*/
static int LoadPCX( const char *filename, qbyte **pic, int *width, int *height, int *flags, int side )
{
	qbyte *raw;
	pcx_t *pcx;
	int x, y, samples = 3;
	int len, columns, rows;
	int dataByte, runLength;
	qbyte *pal = pcx_pal, *pix;
	qbyte stack[0x4000];

	*pic = NULL;

	//
	// load the file
	//
	len = FS_LoadFile( filename, (void **)&raw, stack, sizeof( stack ) );
	if( !raw )
		return 0;

	//
	// parse the PCX file
	//
	pcx = (pcx_t *)raw;

	pcx->xmin = LittleShort( pcx->xmin );
	pcx->ymin = LittleShort( pcx->ymin );
	pcx->xmax = LittleShort( pcx->xmax );
	pcx->ymax = LittleShort( pcx->ymax );
	pcx->hres = LittleShort( pcx->hres );
	pcx->vres = LittleShort( pcx->vres );
	pcx->bytes_per_line = LittleShort( pcx->bytes_per_line );
	pcx->palette_type = LittleShort( pcx->palette_type );

	raw = &pcx->data;

	if( pcx->manufacturer != 0x0a
		|| pcx->version != 5
		|| pcx->encoding != 1
		|| pcx->bits_per_pixel != 8 )
	{
		Com_DPrintf( S_COLOR_YELLOW "Bad pcx file %s\n", filename );
		if( ( qbyte *)pcx != stack )
			FS_FreeFile( pcx );
		return 0;
	}

	columns = pcx->xmax + 1;
	rows = pcx->ymax + 1;
	pix = *pic = R_PrepareImageBuffer( TEXTURE_LOADING_BUF0+side, columns * rows * 3 );
	memcpy( pal, (qbyte *)pcx + len - 768, 768 );

	if( width )
		*width = columns;
	if( height )
		*height = rows;

	for( y = 0; y < rows; y++ )
	{
		for( x = 0; x < columns; )
		{
			dataByte = *raw++;

			if( ( dataByte & 0xC0 ) == 0xC0 )
			{
				runLength = dataByte & 0x3F;
				dataByte = *raw++;
			}
			else
				runLength = 1;

			while( runLength-- > 0 )
			{
				pix[0] = pal[dataByte*3+0];
				pix[1] = pal[dataByte*3+1];
				pix[2] = pal[dataByte*3+2];
				x++; pix += 3;
			}
		}
	}

	if( raw - (qbyte *)pcx > len )
	{
		Com_DPrintf( S_COLOR_YELLOW "PCX file %s was malformed", filename );
		*pic = NULL;
	}

	if( (qbyte *)pcx != stack )
		FS_FreeFile( pcx );

	return samples;
}

/*
* R_GetQ1Palette
*/
static int *R_GetQ1Palette( void )
{
	qbyte *raw;
	int r, g, b, v;
	int i, len;
	qbyte stack[0x4000];
	int	*out;
	const qbyte *pal;
	static const qbyte host_quakepal[768] =
#include "../qcommon/quake1pal.h"
	;

	// get the palette
	len = FS_LoadFile( "gfx/palette.lmp", (void **)&raw, stack, sizeof( stack ) );
	pal = ( raw && len >= 768 ) ? raw : host_quakepal;
	out = Mem_Alloc( r_texturesPool, sizeof( *out ) * 256 );

	for( i = 0; i < 256; i++ )
	{
		r = pal[i*3 + 0];
		g = pal[i*3 + 1];
		b = pal[i*3 + 2];

		v = COLOR_RGBA( r, g, b, 255 );
		out[i] = LittleLong( v );
	}

	if( (qbyte *)raw != stack )
		FS_FreeFile( raw );

	out[255] = 0;	// 255 is transparent

	return out;
}

/*
* R_GetQ2Palette
*/
static int *R_GetQ2Palette( void )
{
	int			i;
	int			*out;
	qbyte		*pic, *pal = pcx_pal;
	int			r, g, b;
	unsigned	v;
	int			width, height, flags = 0;

	// get the palette
	LoadPCX( "pics/colormap.pcx", &pic, &width, &height, &flags, 0 );
	if( !pic )
		return NULL;

	out = Mem_Alloc( r_texturesPool, sizeof( *out ) * 256 );
	for( i = 0; i < 256; i++ )
	{
		r = pal[i*3 + 0];
		g = pal[i*3 + 1];
		b = pal[i*3 + 2];

		v = COLOR_RGBA( r, g, b, 255 );
		out[i] = LittleLong( v );
	}

	out[255] &= LittleLong( 0xffffff );	// 255 is transparent
	return out;
}

/*
* R_GetPalette
* 
* Loads Q1 or Q2 palette from disk if not already loaded
*/
static int *R_GetPalette( int flags )
{
	int i;

	if( !r_8to24table )
	{
		if( flags & IT_MIPTEX )
			r_8to24table = R_GetQ1Palette();
		else if( flags & IT_WAL )
			r_8to24table = R_GetQ2Palette();

		if( !r_8to24table )
		{
			r_8to24table = Mem_Alloc( r_texturesPool, sizeof( *r_8to24table ) * 256 );

			// whatever...
			for( i = 0; i < 256; i++ )
				r_8to24table[i] = LittleLong( i );
		}
	}
	return r_8to24table;
}

/*
=========================================================

TARGA LOADING

=========================================================
*/

typedef struct _TargaHeader
{
	unsigned char id_length, colormap_type, image_type;
	unsigned short colormap_index, colormap_length;
	unsigned char colormap_size;
	unsigned short x_origin, y_origin, width, height;
	unsigned char pixel_size, attributes;
} TargaHeader;

// basic readpixel/writepixel loop for RLE runs
#define WRITELOOP_COMP_1				\
		for( i = 0; i < size; )		\
		{								\
			header = *pin++;			\
			pixelcount = (header & 0x7f) + 1;	\
			if( header & 0x80 )			\
			{							\
				READPIXEL(pin);			\
				for( j = 0; j < pixelcount; j++ )	\
				{						\
					WRITEPIXEL( pout );	\
				}						\
				i += pixelcount;		\
			}							\
			else						\
			{							\
				for( j = 0; j < pixelcount; j++ )	\
				{						\
					READPIXEL( pin );	\
					WRITEPIXEL( pout );	\
				}						\
				i += pixelcount;		\
			}							\
		}

// readpixel/writepixel loop for RLE runs with memcpy on uncomp run
#define WRITELOOP_COMP_2(chan)			\
		for( i = 0; i < size; )		\
		{								\
			header = *pin++;			\
			pixelcount = (header & 0x7f) + 1;	\
			if( header & 0x80 )			\
			{							\
				READPIXEL(pin);			\
				if( chan == 4 )			\
				{						\
					Q_memset32( pout, blue|(green<<8)|(red<<16)|(alpha<<24), pixelcount );	\
					pout += pixelcount * chan;		\
					i += pixelcount;		\
				}						\
				else					\
				{						\
					for( j = 0; j < pixelcount; j++ )	\
						WRITEPIXEL( pout );				\
					i += pixelcount;					\
				}						\
			}							\
			else						\
			{							\
				memcpy( pout, pin, pixelcount * chan );	\
				pin += pixelcount * chan;		\
				pout += pixelcount * chan;		\
				i += pixelcount;		\
			}							\
		}

#define WRITEPIXEL24(a)			\
		*a++ = blue;				\
		*a++ = green;				\
		*a++ = red;

#define WRITEPIXEL32(a)			\
		WRITEPIXEL24(a);			\
		*a++ = alpha;

#define READPIXEL(a)				\
		blue = *a++;				\
		red = palette[blue][0];		\
		green = palette[blue][1];	\
		blue = palette[blue][2];

#undef WRITEPIXEL
#define WRITEPIXEL	WRITEPIXEL24
static void tga_comp_cm24( qbyte *pout, qbyte *pin, qbyte palette[256][4], int size )
{
	int header, pixelcount;
	int blue, green, red;
	int i, j;

	WRITELOOP_COMP_1;
}

static void tga_cm24( qbyte *pout, qbyte *pin, qbyte palette[256][4], int size )
{
	int blue, green, red;
	int i;

	for( i = 0; i < size; i++ )
	{
		READPIXEL(pin);
		WRITEPIXEL(pout);
	}
}

#undef READPIXEL
#define READPIXEL(a)				\
		blue = *a++;				\
		red = palette[blue][0];		\
		green = palette[blue][1];	\
		blue = palette[blue][2];	\
		alpha = palette[blue][3];

#undef WRITEPIXEL
#define WRITEPIXEL	WRITEPIXEL32
static void tga_comp_cm32( qbyte *pout, qbyte *pin, qbyte palette[256][4], int size )
{
	int header, pixelcount;
	int blue, green, red, alpha;
	int i, j;

	WRITELOOP_COMP_1;
}

static void tga_cm32( qbyte *pout, qbyte *pin, qbyte palette[256][4], int size )
{
	int blue, green, red, alpha;
	int i;

	for( i = 0; i < size; i++ )
	{
		READPIXEL(pin);
		WRITEPIXEL(pout);
	}
}

#undef READPIXEL
#define READPIXEL(a)			\
		blue = *a++;			\
		green = *a++;			\
		red = *a++;

#undef WRITEPIXEL
#define WRITEPIXEL	WRITEPIXEL24
static void tga_comp_rgb24( qbyte *pout, qbyte *pin, int size )
{
	int header, pixelcount;
	int blue, green, red;
	int i, j;

	for( i = 0; i < size; )
	{
		header = *pin++;
		pixelcount = (header & 0x7f) + 1;
		if( header & 0x80 )
		{
			READPIXEL(pin);
			for( j = 0; j < pixelcount; j++ )
			{
				WRITEPIXEL( pout );
			}
			i += pixelcount;
		}
		else
		{
			memcpy( pout, pin, pixelcount * 3 );
			pin += pixelcount * 3;
			pout += pixelcount * 3;
			i += pixelcount;
		}
	}
}

static void tga_rgb24( qbyte *pout, qbyte *pin, int size )
{
	memcpy( pout, pin, size * 3 );
}

#undef READPIXEL
#define READPIXEL(a)			\
		blue = *a++;			\
		green = *a++;			\
		red = *a++;				\
		alpha = *a++;			\
		pix = blue | ( green << 8 ) | ( red << 16 ) | ( alpha << 24 );

#undef WRITEPIXEL
#define WRITEPIXEL(a)			\
		*((int*)a) = pix;	\
		a += 4;

static void tga_comp_rgb32( qbyte *pout, qbyte *pin, int size )
{
	int header, pixelcount;
	int blue, green, red, alpha;
	int i, pix;

	for( i = 0; i < size; )
	{
		header = *pin++;
		pixelcount = (header & 0x7f) + 1;
		if( header & 0x80 )
		{
			READPIXEL(pin);
			Q_memset32( pout, pix, pixelcount );
			pout += pixelcount * 4;
			i += pixelcount;
		}
		else
		{
			memcpy( pout, pin, pixelcount * 4 );
			pin += pixelcount * 4;
			pout += pixelcount * 4;
			i += pixelcount;
		}
	}
}

static void tga_rgb32( qbyte *pout, qbyte *pin, int size )
{
	memcpy( pout, pin, size * 4 );
}


#undef READPIXEL
#define READPIXEL(a)			\
	blue = green = red = *a++;

#undef WRITEPIXEL
#define WRITEPIXEL	WRITEPIXEL24
static void tga_comp_grey( qbyte *pout, qbyte *pin, int size )
{
	int header, pixelcount;
	int blue, green, red;
	int i, j;

	WRITELOOP_COMP_1;
}

static void tga_grey( qbyte *pout, qbyte *pin, int size )
{
	int i, a;

	for( i = 0; i < size; i++ )
	{
		a = *pin++;
		*pout++ = a;
		*pout++ = a;
		*pout++ = a;
	}
}


/*
* LoadTGA
*/
static int LoadTGA( const char *name, qbyte **pic, int *width, int *height, int *flags, int side )
{
	int i, j, columns, rows, samples;
	qbyte *buf_p, *buffer, *pixbuf, *targa_rgba;
	qbyte palette[256][4];
	TargaHeader targa_header;
	qbyte stack[0x4000];

	*pic = NULL;

	//
	// load the file
	//
	FS_LoadFile( name, (void **)&buffer, stack, sizeof( stack ) );
	if( !buffer )
		return 0;

	buf_p = buffer;
	targa_header.id_length = *buf_p++;
	targa_header.colormap_type = *buf_p++;
	targa_header.image_type = *buf_p++;

	targa_header.colormap_index = buf_p[0] + buf_p[1] * 256;
	buf_p += 2;
	targa_header.colormap_length = buf_p[0] + buf_p[1] * 256;
	buf_p += 2;
	targa_header.colormap_size = *buf_p++;
	targa_header.x_origin = LittleShort( *( (short *)buf_p ) );
	buf_p += 2;
	targa_header.y_origin = LittleShort( *( (short *)buf_p ) );
	buf_p += 2;
	targa_header.width = LittleShort( *( (short *)buf_p ) );
	buf_p += 2;
	targa_header.height = LittleShort( *( (short *)buf_p ) );
	buf_p += 2;
	targa_header.pixel_size = *buf_p++;
	targa_header.attributes = *buf_p++;
	if( targa_header.id_length != 0 )
		buf_p += targa_header.id_length; // skip TARGA image comment

	samples = 3;
	if( targa_header.image_type == 1 || targa_header.image_type == 9 )
	{
		// uncompressed colormapped image
		if( targa_header.pixel_size != 8 )
		{
			Com_DPrintf( S_COLOR_YELLOW "LoadTGA: Only 8 bit images supported for type 1 and 9" );
			if( buffer != stack )
				FS_FreeFile( buffer );
			return 0;
		}
		if( targa_header.colormap_length != 256 )
		{
			Com_DPrintf( S_COLOR_YELLOW "LoadTGA: Only 8 bit colormaps are supported for type 1 and 9" );
			if( buffer != stack )
				FS_FreeFile( buffer );
			return 0;
		}
		if( targa_header.colormap_index )
		{
			Com_DPrintf( S_COLOR_YELLOW "LoadTGA: colormap_index is not supported for type 1 and 9" );
			if( buffer != stack )
				FS_FreeFile( buffer );
			return 0;
		}
		if( targa_header.colormap_size == 24 )
		{
			for( i = 0; i < targa_header.colormap_length; i++ )
			{
				palette[i][2] = *buf_p++;
				palette[i][1] = *buf_p++;
				palette[i][0] = *buf_p++;
				palette[i][3] = 255;
			}
		}
		else if( targa_header.colormap_size == 32 )
		{
			samples = 4;

			for( i = 0; i < targa_header.colormap_length; i++ )
			{
				palette[i][2] = *buf_p++;
				palette[i][1] = *buf_p++;
				palette[i][0] = *buf_p++;
				palette[i][3] = *buf_p++;
			}
		}
		else
		{
			Com_DPrintf( S_COLOR_YELLOW "LoadTGA: only 24 and 32 bit colormaps are supported for type 1 and 9" );
			if( buffer != stack )
				FS_FreeFile( buffer );
			return 0;
		}
	}
	else if( targa_header.image_type == 2 || targa_header.image_type == 10 )
	{
		// uncompressed or RLE compressed RGB
		if( targa_header.pixel_size != 32 && targa_header.pixel_size != 24 )
		{
			Com_DPrintf( S_COLOR_YELLOW "LoadTGA: Only 32 or 24 bit images supported for type 2 and 10" );
			if( buffer != stack )
				FS_FreeFile( buffer );
			return 0;
		}

		samples = targa_header.pixel_size >> 3;
	}
	else if( targa_header.image_type == 3 || targa_header.image_type == 11 )
	{
		// uncompressed grayscale
		if( targa_header.pixel_size != 8 )
		{
			Com_DPrintf( S_COLOR_YELLOW "LoadTGA: Only 8 bit images supported for type 3 and 11" );
			if( buffer != stack )
				FS_FreeFile( buffer );
			return 0;
		}
	}

	columns = targa_header.width;
	if( width )
		*width = columns;

	rows = targa_header.height;
	if( height )
		*height = rows;

	targa_rgba = R_PrepareImageBuffer( TEXTURE_LOADING_BUF0+side, columns * rows * samples );
	*pic = targa_rgba;
	pixbuf = targa_rgba;

	switch( targa_header.image_type )
	{
		case 1:
			// uncompressed colourmapped
			if( targa_header.colormap_size == 24 )
				tga_cm24( pixbuf, buf_p, palette, rows * columns );
			else
				tga_cm32( pixbuf, buf_p, palette, rows * columns );
			break;

		case 9:
			// compressed colourmapped
			if( targa_header.colormap_size == 24 )
				tga_comp_cm24( pixbuf, buf_p, palette, rows * columns );
			else
				tga_comp_cm32( pixbuf, buf_p, palette, rows * columns );
			break;

		case 2:
			// uncompressed RGB
			if( targa_header.pixel_size == 24 )
				tga_rgb24( pixbuf, buf_p, rows * columns );
			else
				tga_rgb32( pixbuf, buf_p, rows * columns );
			break;

		case 10:
			// compressed RGB
			if( targa_header.pixel_size == 24 )
				tga_comp_rgb24( pixbuf, buf_p, rows * columns );
			else
				tga_comp_rgb32( pixbuf, buf_p, rows * columns );
			break;

		case 3:
			// uncompressed grayscale
			tga_grey( pixbuf, buf_p, rows * columns );
			break;

		case 11:
			// compressed grayscale
			tga_comp_grey( pixbuf, buf_p, rows * columns );
			break;
	}

	// this could be optimized out easily in uncompressed versions
	if( ! (targa_header.attributes & 0x20) )
	{
		// Flip the image vertically
		int rowsize = columns * samples;
		qbyte *row1, *row2;
		qbyte *tmpLine = Mem_TempMalloc( rowsize );

		for( i = 0, j = rows - 1; i < j; i++, j-- )
		{
			row1 = targa_rgba + i * rowsize;
			row2 = targa_rgba + j * rowsize;
			memcpy( tmpLine, row1, rowsize );
			memcpy( row1, row2, rowsize );
			memcpy( row2, tmpLine, rowsize );
		}

		Mem_TempFree( tmpLine );
	}

	if( buffer != stack )
		FS_FreeFile( buffer );

	if( flags )
		*flags |= IT_BGRA;
	return samples;
}

#undef WRITEPIXEL24
#undef WRITEPIXEL32
#undef WRITEPIXEL
#undef READPIXEL
#undef WRITELOOP_COMP_1
#undef WRITELOOP_COMP_2

/*
* WriteTGA
*/
static qboolean WriteTGA( const char *name, qbyte *buffer, int width, int height, qboolean bgr )
{
	int file, i, c, temp;

	if( FS_FOpenFile( name, &file, FS_WRITE ) == -1 )
	{
		Com_Printf( "WriteTGA: Couldn't create %s\n", name );
		return qfalse;
	}

	buffer[2] = 2;  // uncompressed type
	buffer[12] = width&255;
	buffer[13] = width>>8;
	buffer[14] = height&255;
	buffer[15] = height>>8;
	buffer[16] = 24; // pixel size

	// swap rgb to bgr
	c = 18+width*height*3;
	if( !bgr )
	{
		for( i = 18; i < c; i += 3 )
		{
			temp = buffer[i];
			buffer[i] = buffer[i+2];
			buffer[i+2] = temp;
		}
	}
	FS_Write( buffer, c, file );
	FS_FCloseFile( file );

	return qtrue;
}

/*
=========================================================

JPEG LOADING

=========================================================
*/

struct q_jpeg_error_mgr {
	struct jpeg_error_mgr pub;		// "public" fields
	jmp_buf setjmp_buffer;			// for return to caller
};

static void q_jpg_error_exit(j_common_ptr cinfo)
{
	char buffer[JMSG_LENGTH_MAX];

	// cinfo->err really points to a my_error_mgr struct, so coerce pointer
	struct q_jpeg_error_mgr *qerr = (struct q_jpeg_error_mgr *) cinfo->err;

    // create the message
	qerr->pub.format_message( cinfo, buffer );
	Com_DPrintf( "q_jpg_error_exit: %s\n", buffer );

	// Return control to the setjmp point
	longjmp(qerr->setjmp_buffer, 1);
}

static void q_jpg_noop( j_decompress_ptr cinfo )
{
}

static boolean q_jpg_fill_input_buffer( j_decompress_ptr cinfo )
{
	Com_DPrintf( "Premature end of jpeg file\n" );
	return 1;
}

static void q_jpg_skip_input_data( j_decompress_ptr cinfo, long num_bytes )
{
	cinfo->src->next_input_byte += (size_t) num_bytes;
	cinfo->src->bytes_in_buffer -= (size_t) num_bytes;
}

void q_jpeg_mem_src( j_decompress_ptr cinfo, unsigned char *mem, unsigned long len )
{
	cinfo->src = (struct jpeg_source_mgr *)
		( *cinfo->mem->alloc_small )( (j_common_ptr) cinfo,
		JPOOL_PERMANENT,
		sizeof( struct jpeg_source_mgr ) );
	cinfo->src->init_source = q_jpg_noop;
	cinfo->src->fill_input_buffer = q_jpg_fill_input_buffer;
	cinfo->src->skip_input_data = q_jpg_skip_input_data;
	cinfo->src->resync_to_restart = jpeg_resync_to_restart;
	cinfo->src->term_source = q_jpg_noop;
	cinfo->src->bytes_in_buffer = len;
	cinfo->src->next_input_byte = mem;
}

/*
* LoadJPG
*/
static int LoadJPG( const char *name, qbyte **pic, int *width, int *height, int *flags, int side )
{
	unsigned int i, length, samples, widthXsamples;
	qbyte *img, *scan, *buffer, *line;
	struct q_jpeg_error_mgr jerr;
	struct jpeg_decompress_struct cinfo;
	qbyte stack[0x4000];

	*pic = NULL;

	// load the file
	length = FS_LoadFile( name, (void **)&buffer, stack, sizeof( stack ) );
	if( !buffer )
		return 0;

	cinfo.err = jpeg_std_error( &jerr.pub );
	jerr.pub.error_exit = q_jpg_error_exit;

	// establish the setjmp return context for q_jpg_error_exit to use.
	if( setjmp( jerr.setjmp_buffer ) ) {
		// if we get here, the JPEG code has signaled an error
		goto error;
	}

	jpeg_create_decompress( &cinfo );
	q_jpeg_mem_src( &cinfo, buffer, length );
	jpeg_read_header( &cinfo, TRUE );
	jpeg_start_decompress( &cinfo );
	samples = cinfo.output_components;

	if( samples != 3 && samples != 1 )
	{
error:
		Com_DPrintf( S_COLOR_YELLOW "Bad jpeg file %s\n", name );
		jpeg_destroy_decompress( &cinfo );
		if( buffer != stack )
			FS_FreeFile( buffer );
		return 0;
	}

	if( width )
		*width = cinfo.output_width;
	if( height )
		*height = cinfo.output_height;

	img = *pic = R_PrepareImageBuffer( TEXTURE_LOADING_BUF0+side, cinfo.output_width * cinfo.output_height * 3 );
	widthXsamples = cinfo.output_width * samples;
	if( sizeof( stack ) >= widthXsamples + length )
		line = stack + length;
	else
		line = R_PrepareImageBuffer( TEXTURE_LINE_BUF, widthXsamples );

	while( cinfo.output_scanline < cinfo.output_height )
	{
		scan = line;
		if( !jpeg_read_scanlines( &cinfo, &scan, 1 ) )
		{
			Com_Printf( S_COLOR_YELLOW "Bad jpeg file %s\n", name );
			jpeg_destroy_decompress( &cinfo );
			if( buffer != stack )
				FS_FreeFile( buffer );
			return 0;
		}

		if( samples == 1 )
		{
			for( i = 0; i < cinfo.output_width; i++, img += 3 )
				img[0] = img[1] = img[2] = *scan++;
		}
		else
		{
			memcpy( img, scan, widthXsamples );
			img += widthXsamples;
		}
	}

	jpeg_finish_decompress( &cinfo );
	jpeg_destroy_decompress( &cinfo );

	if( buffer != stack )
		FS_FreeFile( buffer );

	return 3;
}

/*
* WriteJPG
*/
static qboolean WriteJPG( const char *name, qbyte *buffer, int width, int height, int quality )
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	char *fullname;
	int fullname_size;
	FILE *f;
	JSAMPROW s[1];
	int offset, w3;

	// We can't use FS-functions with libjpeg
	fullname_size =
		sizeof( char ) * ( strlen( FS_WriteDirectory() ) + 1 + strlen( FS_GameDirectory() ) + 1 + strlen( name ) + 1 );
	fullname = Mem_TempMalloc( fullname_size );
	Q_snprintfz( fullname, fullname_size, "%s/%s/%s", FS_WriteDirectory(), FS_GameDirectory(), name );
	FS_CreateAbsolutePath( fullname );

	if( !( f = fopen( fullname, "wb" ) ) )
	{
		Com_Printf( "WriteJPG: Couldn't create %s\n", fullname );
		Mem_TempFree( fullname );
		return qfalse;
	}

	// initialize the JPEG compression object
	cinfo.err = jpeg_std_error( &jerr );
	jpeg_create_compress( &cinfo );
	jpeg_stdio_dest( &cinfo, f );

	// setup JPEG parameters
	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.in_color_space = JCS_RGB;
	cinfo.input_components = 3;

	jpeg_set_defaults( &cinfo );

	if( ( quality > 100 ) || ( quality <= 0 ) )
		quality = 85;

	jpeg_set_quality( &cinfo, quality, TRUE );

	// If quality is set high, disable chroma subsampling 
	if( quality >= 85 )
	{
		cinfo.comp_info[0].h_samp_factor = 1;
		cinfo.comp_info[0].v_samp_factor = 1;
	}

	// start compression
	jpeg_start_compress( &cinfo, qtrue );

	// feed scanline data
	w3 = cinfo.image_width * 3;
	offset = w3 * cinfo.image_height - w3;
	while( cinfo.next_scanline < cinfo.image_height )
	{
		s[0] = &buffer[offset - cinfo.next_scanline * w3];
		jpeg_write_scanlines( &cinfo, s, 1 );
	}

	// finish compression
	jpeg_finish_compress( &cinfo );
	jpeg_destroy_compress( &cinfo );

	fclose( f );
	Mem_TempFree( fullname );

	return qtrue;
}

/*
=========================================================

PNG LOADING

=========================================================
*/

typedef struct {
	qbyte *data;
	size_t size;
	size_t curptr;
} q_png_iobuf_t;

static void q_png_error_fn( png_structp png_ptr, const char *message )
{
    Com_DPrintf( "q_png_error_fn: error: %s\n", message );
}

static void q_png_warning_fn( png_structp png_ptr, const char *message )
{
    Com_DPrintf( "q_png_warning_fn: warning: %s\n", message );
}

//LordHavoc: removed __cdecl prefix, added overrun protection, and rewrote this to be more efficient
static void q_png_user_read_fn( png_structp png_ptr, unsigned char *data, size_t length )
{
	q_png_iobuf_t *io = (q_png_iobuf_t *)png_get_io_ptr( png_ptr );
	size_t rem = io->size - io->curptr;

	if( length > rem ) {
        Com_DPrintf( "q_png_user_read_fn: overrun by %i bytes\n", (int)(length - rem) );

        // a read going past the end of the file, fill in the remaining bytes
        // with 0 just to be consistent
        memset( data + rem, 0, length - rem );
        length = rem;
    }

	memcpy( data, io->data + io->curptr, length );
    io->curptr += length;
}

/*
* LoadPNG
*/
static int LoadPNG( const char *name, qbyte **pic, int *width, int *height, int *flags, int side )
{
	qbyte *img;
	qbyte *png_data;
	size_t png_datasize;
	qbyte stack[0x4000];
	q_png_iobuf_t io;
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	png_uint_32 p_width, p_height;
	int p_bit_depth, p_color_type, p_interlace_type;
	int samples;
	size_t y, row_bytes;
	unsigned char **row_pointers;

	*pic = NULL;

	// load the file
	png_datasize = FS_LoadFile( name, (void **)&png_data, stack, sizeof( stack ) );
	if( !png_data )
		return 0;

	if( png_sig_cmp( png_data, 0, png_datasize ) ) {
error:
		Com_DPrintf( S_COLOR_YELLOW "Bad png file %s\n", name );

		if( png_ptr != NULL ) {
			png_destroy_write_struct( &png_ptr, NULL );
		}
		if( png_data != stack ) {
			FS_FreeFile( png_data );
		}
        return 0;
	}
	
	// create and initialize the png_struct with the desired error handler
	// functions. We also supply the  the compiler header file version, so 
	// that we know if the application was compiled with a compatible 
	// version of the library. REQUIRED
	png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, 0, q_png_error_fn, q_png_warning_fn );
	if( png_ptr == NULL ) {
		goto error;
	}

	// allocate/initialize the image information data. REQUIRED
	info_ptr = png_create_info_struct( png_ptr );
	if( info_ptr == NULL ) {
		goto error;
	}

	// set error handling if you are using the setjmp/longjmp method (this is
	// the normal method of doing things with libpng). REQUIRED unless you
	// set up your own error handlers in the png_create_read_struct() earlier.
	if( setjmp( png_jmpbuf( png_ptr ) ) ) {
		goto error;
	}

	io.curptr = 0;
	io.data = png_data;
	io.size = png_datasize;

	// if you are using replacement read functions, instead of calling
	// png_init_io() here you would call:
	png_set_read_fn( png_ptr, (void *)&io, q_png_user_read_fn );
	// where user_io_ptr is a structure you want available to the callbacks

	png_set_sig_bytes( png_ptr, 0 );

	// the call to png_read_info() gives us all of the information from the
	// PNG file before the first IDAT (image data chunk). REQUIRED
	png_read_info( png_ptr, info_ptr );

	png_get_IHDR( png_ptr, info_ptr, &p_width, &p_height, &p_bit_depth, &p_color_type,
		&p_interlace_type, NULL, NULL );

	if( p_color_type & PNG_COLOR_MASK_ALPHA ) {
		samples = 4;
	}
	else {
		samples = 3;
		// add filler (or alpha) byte (before/after each RGB triplet)
		// png_set_filler( png_ptr, 255, 1 );
	}
	*width = p_width;
	*height = p_height;

	// expand paletted colors into true RGB triplets
	if( p_color_type == PNG_COLOR_TYPE_PALETTE ) {
        png_set_palette_to_rgb( png_ptr );
	}

	// expand grayscale images to RGB triplets
	if( p_color_type == PNG_COLOR_TYPE_GRAY || p_color_type == PNG_COLOR_TYPE_GRAY_ALPHA ) {
        png_set_gray_to_rgb( png_ptr );
	}

	// expand paletted or RGB images with transparency to full alpha channels
	// so the data will be available as RGBA quartets.
	if( png_get_valid( png_ptr, info_ptr, PNG_INFO_tRNS ) ) {
        png_set_tRNS_to_alpha( png_ptr );
	}

	// expand grayscale images to the full 8 bits
	if( p_bit_depth < 8 ) {
        png_set_expand( png_ptr );
	}

	// optional call to gamma correct and add the background to the palette
	// and update info structure. REQUIRED if you are expecting libpng to
	// update the palette for you (ie you selected such a transform above).
    png_read_update_info( png_ptr, info_ptr );

	// allocate the memory to hold the image using the fields of info_ptr

	row_bytes = png_get_rowbytes( png_ptr, info_ptr );
    row_pointers = (unsigned char **)Mem_TempMalloc( p_height * sizeof( *row_pointers ) );

	img = *pic = R_PrepareImageBuffer( TEXTURE_LOADING_BUF0+side, p_height * row_bytes );

	for( y = 0; y < p_height; y++ ) {
		row_pointers[y] = img + y * row_bytes;
	}

	// now it's time to read the image
	png_read_image( png_ptr, row_pointers );

	// read rest of file, and get additional chunks in info_ptr - REQUIRED
    png_read_end( png_ptr, info_ptr );

	// clean up after the read, and free any memory allocated - REQUIRED
    png_destroy_read_struct( &png_ptr, &info_ptr, 0 );

	Mem_TempFree( row_pointers );

	if( png_data != stack ) {
		FS_FreeFile( png_data );
	}

	return samples;
}

/*
=========================================================

MIPTEX LOADING

=========================================================
*/

/*
* LoadMipTex
*/
static int LoadMipTex( qbyte **pic, int width, int height, int flags )
{
	unsigned int i;
	int side = 0;
	unsigned int p, s, *trans;
	qbyte *imgbuf, *data;
	int *d_8to24table;

	data = *pic;

	// load palette from disk
	d_8to24table = R_GetPalette( IT_MIPTEX );

	s = width * height;
	imgbuf = R_PrepareImageBuffer( TEXTURE_LOADING_BUF0+side, s * 4 );
	*pic = imgbuf;
	trans = ( unsigned int * )imgbuf;

	if( flags & IT_SKY )
	{
		unsigned j;
		unsigned r, g, b, p;
		unsigned transpix;
		unsigned rgba;
		int halfwidth = width >> 1;

		// a sky texture is 256*128, with the right side being a masked overlay
		r = g = b = 0;
		for( i = 0; i < (unsigned)height; i++ )
		{
			for( j = 0; j < (unsigned)halfwidth; j++ )
			{
				p = data[i*width + halfwidth + j];
				rgba = d_8to24table[p];
				trans[i*width + halfwidth + j] = rgba;
				r += COLOR_R( rgba );
				g += COLOR_G( rgba );
				b += COLOR_B( rgba );
			}
		}

		// make an average value for the back to avoid
		// a fringe on the top level
		transpix = COLOR_RGBA( r/(halfwidth*height), g/(halfwidth*height), b/(halfwidth*height), 0 );

		for( i = 0; i < (unsigned)height; i++ )
		{
			for( j = 0; j < (unsigned)halfwidth; j++ )
			{
				p = data[i*width + j];
				trans[i*width + j] = p ? d_8to24table[p] : transpix;
			}
		}
		return 4;
	}
	else if( flags & IT_MIPTEX_FULLBRIGHT )
	{
		// this is a fullbright mask, so make all non-fullbright
		// colors transparent
		for( i = 0; i < s; i++ )
		{
			p = data[i];
			if( p < 224 )
				trans[i] = 0; // transparent
			else
				trans[i] = d_8to24table[p];	// fullbright
		}
		return 4;
	}
	else
	{
		// copy rgb components
		for( i = 0; i < s; i++ )
		{
			p = data[i];
			*imgbuf++ = ((qbyte *)&d_8to24table[p])[0];
			*imgbuf++ = ((qbyte *)&d_8to24table[p])[1];
			*imgbuf++ = ((qbyte *)&d_8to24table[p])[2];
		}
		return 3;
	}
}

/*
* R_MiptexHasFullbrights
*/
qboolean R_MiptexHasFullbrights( qbyte *pixels, int width, int height )
{
	int i;
	int size = width * height;

	for( i = 0; i < size; i++ )
	{
		if( pixels[i] >= 224 )
			return qtrue;
	}

	return qfalse;
}

/*
=========================================================

WAL LOADING

=========================================================
*/

/*
* LoadWAL
*/
static int LoadWAL( const char *name, qbyte **pic, int *width, int *height, int *flags, int side )
{
	unsigned int i;
	unsigned int p, s, *trans;
	unsigned int rows, columns;
	int samples;
	qbyte *buffer, *data, *imgbuf;
	q2miptex_t *mt;
	qbyte stack[0x4000];
	int *d_8to24table;

	*pic = NULL;

	// load palette from disk
	d_8to24table = R_GetPalette( IT_WAL );

	// load the file
	FS_LoadFile( name, (void **)&buffer, stack, sizeof( stack ) );
	if( !buffer )
		return 0;

	mt = ( q2miptex_t * )buffer;
	rows = LittleLong( mt->width );
	columns = LittleLong( mt->height );
	data = buffer + LittleLong( mt->offsets[0] );

	if( width )
		*width = ( int )rows;
	if( height )
		*height = ( int )columns;

	s = LittleLong( mt->width ) * LittleLong( mt->height );
	imgbuf = R_PrepareImageBuffer( TEXTURE_LOADING_BUF0+side, s * 4 );
	*pic = imgbuf;
	trans = ( unsigned int * )imgbuf;

	// determine the number of channels
	for( i = 0; i < s && data[i] != 255; i++ );
	samples = (i < s) ? 4 : 3;

	if( samples == 4 )
	{
		for( i = 0; i < s; i++ )
		{
			p = data[i];
			trans[i] = d_8to24table[p];

			if( p == 255 )
			{	
				// transparent, so scan around for another color
				// to avoid alpha fringes
				// FIXME: do a full flood fill so mips work...
				if( i > rows && data[i-rows] != 255 )
					p = data[i-rows];
				else if( i < s-rows && data[i+rows] != 255 )
					p = data[i+rows];
				else if( i > 0 && data[i-1] != 255 )
					p = data[i-1];
				else if( i < s-1 && data[i+1] != 255 )
					p = data[i+1];
				else
					p = 0;

				// copy rgb components
				((qbyte *)&trans[i])[0] = ((qbyte *)&d_8to24table[p])[0];
				((qbyte *)&trans[i])[1] = ((qbyte *)&d_8to24table[p])[1];
				((qbyte *)&trans[i])[2] = ((qbyte *)&d_8to24table[p])[2];
			}
		}
	}
	else
	{
		// copy rgb components
		for( i = 0; i < s; i++ )
		{
			p = data[i];
			*imgbuf++ = ((qbyte *)&d_8to24table[p])[0];
			*imgbuf++ = ((qbyte *)&d_8to24table[p])[1];
			*imgbuf++ = ((qbyte *)&d_8to24table[p])[2];
		}
	}

	if( ( qbyte * )mt != stack )
		FS_FreeFile( mt );

	if( flags )
		*flags |= IT_WAL;
	return samples;
}

//=======================================================

/*
* R_LoadImageFromDisk
*/
static int R_LoadImageFromDisk( char *pathname, size_t pathname_size, qbyte **pic, int *width, int *height, int *flags, int side )
{
	const char *extension;
	int samples;

	*pic = NULL;
	*width = *height = 0;
	samples = 0;

	extension = FS_FirstExtension( pathname, IMAGE_EXTENSIONS, NUM_IMAGE_EXTENSIONS );
	if( extension )
	{
		int format_flags = 0;

		COM_ReplaceExtension( pathname, extension, pathname_size );

		if( !Q_stricmp( extension, ".jpg" ) )
			samples = LoadJPG( pathname, pic, width, height, &format_flags, side );
		else if( !Q_stricmp( extension, ".tga" ) )
			samples = LoadTGA( pathname, pic, width, height, &format_flags, side );
		else if( !Q_stricmp( extension, ".png" ) )
			samples = LoadPNG( pathname, pic, width, height, &format_flags, side );
		else if( !Q_stricmp( extension, ".pcx" ) )
			samples = LoadPCX( pathname, pic, width, height, &format_flags, side );
		else if( !Q_stricmp( extension, ".wal" ) )
			samples = LoadWAL( pathname, pic, width, height, &format_flags, side );

		if( samples )
		{
			if( ( format_flags & IT_BGRA ) && ( !glConfig.ext.bgra || !flags ) )
			{
				R_SwapBlueRed( *pic, *width, *height, samples );
				format_flags &= ~IT_BGRA;
			}
		}

		if( flags )
			*flags |= format_flags;
	}

	return samples;
}

/*
* R_FlipTexture
*/
static void R_FlipTexture( const qbyte *in, qbyte *out, int width, int height, int samples, qboolean flipx, qboolean flipy, qboolean flipdiagonal )
{
	int i, x, y;
	const qbyte *p, *line;
	int row_inc = ( flipy ? -samples : samples ) * width, col_inc = ( flipx ? -samples : samples );
	int row_ofs = ( flipy ? ( height - 1 ) * width * samples : 0 ), col_ofs = ( flipx ? ( width - 1 ) * samples : 0 );

	if( flipdiagonal )
	{
		for( x = 0, line = in + col_ofs; x < width; x++, line += col_inc )
			for( y = 0, p = line + row_ofs; y < height; y++, p += row_inc, out += samples )
				for( i = 0; i < samples; i++ )
					out[i] = p[i];
	}
	else
	{
		for( y = 0, line = in + row_ofs; y < height; y++, line += row_inc )
			for( x = 0, p = line + col_ofs; x < width; x++, p += col_inc, out += samples )
				for( i = 0; i < samples; i++ )
					out[i] = p[i];
	}
}

/*
* R_ResampleTexture
*/
static void R_ResampleTexture( const qbyte *in, int inwidth, int inheight, qbyte *out, int outwidth, int outheight, int samples )
{
	int i, j, k;
	int inwidthS, outwidthS;
	unsigned int frac, fracstep;
	const qbyte *inrow, *inrow2, *pix1, *pix2, *pix3, *pix4;
	unsigned *p1, *p2;
	qbyte *opix;

	if( inwidth == outwidth && inheight == outheight )
	{
		memcpy( out, in, inwidth * inheight * samples );
		return;
	}

	p1 = ( unsigned * )R_PrepareImageBuffer( TEXTURE_LINE_BUF, outwidth * sizeof( *p1 ) * 2 );
	p2 = p1 + outwidth;

	fracstep = inwidth * 0x10000 / outwidth;

	frac = fracstep >> 2;
	for( i = 0; i < outwidth; i++ )
	{
		p1[i] = samples * ( frac >> 16 );
		frac += fracstep;
	}

	frac = 3 * ( fracstep >> 2 );
	for( i = 0; i < outwidth; i++ )
	{
		p2[i] = samples * ( frac >> 16 );
		frac += fracstep;
	}

	inwidthS = inwidth * samples;
	outwidthS = outwidth * samples;
	for( i = 0; i < outheight; i++, out += outwidthS )
	{
		inrow = in + inwidthS * (int)( ( i + 0.25 ) * inheight / outheight );
		inrow2 = in + inwidthS * (int)( ( i + 0.75 ) * inheight / outheight );
		for( j = 0; j < outwidth; j++ )
		{
			pix1 = inrow + p1[j];
			pix2 = inrow + p2[j];
			pix3 = inrow2 + p1[j];
			pix4 = inrow2 + p2[j];
			opix = out + j * samples;

			for( k = 0; k < samples; k++ )
				opix[k] = ( pix1[k] + pix2[k] + pix3[k] + pix4[k] ) >> 2;
		}
	}
}

/*
* R_HeightmapToNormalmap
*/
static int R_HeightmapToNormalmap( const qbyte *in, qbyte *out, int width, int height, float bumpScale, int samples )
{
	int x, y;
	vec3_t n;
	float ibumpScale;
	const qbyte *p0, *p1, *p2;

	if( !bumpScale )
		bumpScale = 1.0f;
	bumpScale *= max( 0, r_lighting_bumpscale->value );
	ibumpScale = ( 255.0 * 3.0 ) / bumpScale;

	memset( out, 255, width * height * 4 );
	for( y = 0; y < height; y++ )
	{
		for( x = 0; x < width; x++, out += 4 )
		{
			p0 = in + ( y * width + x ) * samples;
			p1 = ( x == width - 1 ) ? p0 - x * samples : p0 + samples;
			p2 = ( y == height - 1 ) ? in + x * samples : p0 + width * samples;

			n[0] = ( p0[0] + p0[1] + p0[2] ) - ( p1[0] + p1[1] + p1[2] );
			n[1] = ( p2[0] + p2[1] + p2[2] ) - ( p0[0] + p0[1] + p0[2] );
			n[2] = ibumpScale;
			VectorNormalize( n );

			out[0] = ( n[0] + 1 ) * 127.5f;
			out[1] = ( n[1] + 1 ) * 127.5f;
			out[2] = ( n[2] + 1 ) * 127.5f;
			out[3] = ( p0[0] + p0[1] + p0[2] ) / 3;
		}
	}

	return 4;
}

/*
* R_CutImage
*/
static void R_CutImage( qbyte *in, int inwidth, int height, qbyte *out, int x, int y, int outwidth, int outheight, int samples )
{
	int i;
	qbyte *iin, *iout;

	if( x + outwidth > inwidth )
		outwidth = inwidth - x;
	if( y + outheight > height )
		outheight = height - y;

	x *= samples;
	inwidth *= samples;
	outwidth *= samples;

	for( i = 0, iout = (qbyte *)out; i < outheight; i++, iout += outwidth )
	{
		iin = (qbyte *)in + (y + i) * inwidth + x;
		memcpy( iout, iin, outwidth );
	}
}

/*
* R_MipMap
* 
* Operates in place, quartering the size of the texture
* note: if given odd width/height this discards the last row/column of
* pixels, rather than doing a proper box-filter scale down (LordHavoc)
*/
static void R_MipMap( qbyte *in, int width, int height, int samples )
{
	int i, j, k, samples2;
	qbyte *out;

	// width <<= 2;
	width *= samples;
	height >>= 1;
	samples2 = samples << 1;

	out = in;
	for( i = 0; i < height; i++, in += width )
	{
		for( j = 0; j < width; j += samples2, out += samples, in += samples2 )
		{
			for( k = 0; k < samples; k++ )
				out[k] = ( in[k] + in[k+samples] + in[width+k] + in[width+k+samples] )>>2;
		}
	}
}

/*
* R_TextureFormat
*/
static int R_TextureFormat( int samples, qboolean noCompress )
{
	int bits = r_texturebits->integer;

	if( glConfig.ext.texture_compression && !noCompress )
	{
		if( samples == 3 )
			return GL_COMPRESSED_RGB_ARB;
		return GL_COMPRESSED_RGBA_ARB;
	}

	if( samples == 3 )
	{
		if( bits == 16 )
			return GL_RGB5;
		else if( bits == 32 )
			return GL_RGB8;
		return GL_RGB;
	}

	if( bits == 16 )
		return GL_RGBA4;
	else if( bits == 32 )
		return GL_RGBA8;
	return GL_RGBA;
}

/*
* R_Upload32
*/
void R_Upload32( qbyte **data, int width, int height, int flags, int *upload_width, int *upload_height, int *samples, int inSamples, qboolean subImage )
{
	int i, comp, format;
	int target, target2;
	int numTextures;
	qbyte *scaled = NULL;
	int scaledWidth, scaledHeight;

	assert( samples );

	// we can't properly mipmap a NPT-texture in software
	if( glConfig.ext.texture_non_power_of_two && ( flags & IT_NOMIPMAP ) )
	{
		scaledWidth = width;
		scaledHeight = height;
	}
	else
	{
		for( scaledWidth = 1; scaledWidth < width; scaledWidth <<= 1 );
		for( scaledHeight = 1; scaledHeight < height; scaledHeight <<= 1 );
	}

	if( !( flags & IT_NOPICMIP ) ) {
		if( flags & IT_SKY ) {
			// let people sample down the sky textures for speed
			scaledWidth >>= r_skymip->integer;
			scaledHeight >>= r_skymip->integer;
		}
		else {
			// let people sample down the world textures for speed
			scaledWidth >>= r_picmip->integer;
			scaledHeight >>= r_picmip->integer;
		}
	}

	// don't ever bother with > maxSize textures
	if( flags & IT_CUBEMAP )
	{
		numTextures = 6;
		target = GL_TEXTURE_CUBE_MAP_ARB;
		target2 = GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB;
		clamp( scaledWidth, 1, glConfig.maxTextureCubemapSize );
		clamp( scaledHeight, 1, glConfig.maxTextureCubemapSize );
	}
	else
	{
		numTextures = 1;
		target = GL_TEXTURE_2D;
		target2 = GL_TEXTURE_2D;
		clamp( scaledWidth, 1, glConfig.maxTextureSize );
		clamp( scaledHeight, 1, glConfig.maxTextureSize );
	}

	if( upload_width )
		*upload_width = scaledWidth;
	if( upload_height )
		*upload_height = scaledHeight;

	// scan the texture for any non-255 alpha
	if( flags & IT_LUMINANCE )
	{
		*samples = 1;
	}

	if( flags & IT_DEPTH )
	{
		comp = GL_DEPTH_COMPONENT;
		format = GL_DEPTH_COMPONENT;
	}
	else if( flags & IT_LUMINANCE )
	{
		comp = GL_LUMINANCE;
		format = GL_LUMINANCE;
	}
	else
	{
		comp = R_TextureFormat( *samples, flags & IT_NOCOMPRESS );
		if( inSamples == 4 )
			format = ( flags & IT_BGRA ? GL_BGRA_EXT : GL_RGBA );
		else
			format = ( flags & IT_BGRA ? GL_BGR_EXT : GL_RGB );
	}

	if( flags & IT_NOFILTERING )
	{
		qglTexParameteri( target, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		qglTexParameteri( target, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	}
	else if( flags & IT_DEPTH )
	{
		qglTexParameteri( target, GL_TEXTURE_MIN_FILTER, gl_filter_depth );
		qglTexParameteri( target, GL_TEXTURE_MAG_FILTER, gl_filter_depth );

		if( glConfig.ext.texture_filter_anisotropic )
			qglTexParameteri( target, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1 );
	}
	else if( !( flags & IT_NOMIPMAP ) )
	{
		qglTexParameteri( target, GL_TEXTURE_MIN_FILTER, gl_filter_min );
		qglTexParameteri( target, GL_TEXTURE_MAG_FILTER, gl_filter_max );

		if( glConfig.ext.texture_filter_anisotropic )
			qglTexParameteri( target, GL_TEXTURE_MAX_ANISOTROPY_EXT, gl_anisotropic_filter );
	}
	else
	{
		qglTexParameteri( target, GL_TEXTURE_MIN_FILTER, gl_filter_max );
		qglTexParameteri( target, GL_TEXTURE_MAG_FILTER, gl_filter_max );

		if( glConfig.ext.texture_filter_anisotropic )
			qglTexParameteri( target, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1 );
	}

	// clamp if required
	if( !( flags & IT_CLAMP ) )
	{
		qglTexParameteri( target, GL_TEXTURE_WRAP_S, GL_REPEAT );
		qglTexParameteri( target, GL_TEXTURE_WRAP_T, GL_REPEAT );
	}
	else if( glConfig.ext.texture_edge_clamp )
	{
		qglTexParameteri( target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		qglTexParameteri( target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	}
	else
	{
		qglTexParameteri( target, GL_TEXTURE_WRAP_S, GL_CLAMP );
		qglTexParameteri( target, GL_TEXTURE_WRAP_T, GL_CLAMP );
	}

	if( ( scaledWidth == width ) && ( scaledHeight == height ) && ( flags & IT_NOMIPMAP ) )
	{
		if( subImage )
		{
			for( i = 0; i < numTextures; i++, target2++ )
				qglTexSubImage2D( target2, 0, 0, 0, scaledWidth, scaledHeight, format, GL_UNSIGNED_BYTE, data[i] );
		}
		else
		{
			for( i = 0; i < numTextures; i++, target2++ )
				qglTexImage2D( target2, 0, comp, scaledWidth, scaledHeight, 0, format, GL_UNSIGNED_BYTE, data[i] );
		}
	}
	else
	{
		for( i = 0; i < numTextures; i++, target2++ )
		{
			qbyte *mip;

			if( !scaled )
				scaled = R_PrepareImageBuffer( TEXTURE_RESAMPLING_BUF, scaledWidth * scaledHeight * inSamples );

			// resample the texture
			mip = scaled;
			if( data[i] )
				R_ResampleTexture( data[i], width, height, (qbyte*)mip, scaledWidth, scaledHeight, inSamples );
			else
				mip = NULL;

			if( subImage )
				qglTexSubImage2D( target2, 0, 0, 0, scaledWidth, scaledHeight, format, GL_UNSIGNED_BYTE, mip );
			else
				qglTexImage2D( target2, 0, comp, scaledWidth, scaledHeight, 0, format, GL_UNSIGNED_BYTE, mip );

			// mipmaps generation
			if( !( flags & IT_NOMIPMAP ) && mip )
			{
				int w, h;
				int miplevel = 0;

				w = scaledWidth;
				h = scaledHeight;
				while( w > 1 || h > 1 )
				{
					R_MipMap( mip, w, h, inSamples );

					w >>= 1;
					h >>= 1;
					if( w < 1 )
						w = 1;
					if( h < 1 )
						h = 1;
					miplevel++;

					if( subImage )
						qglTexSubImage2D( target2, miplevel, 0, 0, w, h, format, GL_UNSIGNED_BYTE, mip );
					else
						qglTexImage2D( target2, miplevel, comp, w, h, 0, format, GL_UNSIGNED_BYTE, mip );
				}
			}
		}
	}
}

/*
* R_Upload32_3D
* 
* No resampling, scaling, mipmapping. Just to make 3D attenuation work ;)
*/
void R_Upload32_3D_Fast( qbyte **data, int width, int height, int depth, int flags, int *upload_width, int *upload_height, int *upload_depth, int *samples, qboolean subImage )
{
	int comp, format;
	int scaledWidth, scaledHeight, scaledDepth;

	assert( samples );
	assert( ( flags & (IT_NOMIPMAP|IT_NOPICMIP) ) );

	for( scaledWidth = 1; scaledWidth < width; scaledWidth <<= 1 ) ;
	for( scaledHeight = 1; scaledHeight < height; scaledHeight <<= 1 ) ;
	for( scaledDepth = 1; scaledDepth < depth; scaledDepth <<= 1 ) ;

	if( width != scaledWidth || height != scaledHeight || depth != scaledDepth )
		Com_Error( ERR_DROP, "R_Upload32_3D: bad texture dimensions (not a power of 2)" );
	if( scaledWidth > glConfig.maxTextureSize3D || scaledHeight > glConfig.maxTextureSize3D || scaledDepth > glConfig.maxTextureSize3D )
		Com_Error( ERR_DROP, "R_Upload32_3D: texture is too large (resizing is not supported)" );

	if( upload_width )
		*upload_width = scaledWidth;
	if( upload_height )
		*upload_height = scaledHeight;
	if( upload_depth )
		*upload_depth = scaledDepth;

	if( flags & IT_LUMINANCE )
	{
		comp = GL_LUMINANCE;
		format = GL_LUMINANCE;
	}
	else
	{
		comp = R_TextureFormat( *samples, flags & IT_NOCOMPRESS );
		if( *samples == 4 )
			format = ( flags & IT_BGRA ? GL_BGRA_EXT : GL_RGBA );
		else
			format = ( flags & IT_BGRA ? GL_BGR_EXT : GL_RGB );
	}

	if( flags & IT_NOFILTERING )
	{
		qglTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		qglTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	}
	else if( !( flags & IT_NOMIPMAP ) )
	{
		qglTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, gl_filter_min );
		qglTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, gl_filter_max );

		if( glConfig.ext.texture_filter_anisotropic )
			qglTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_MAX_ANISOTROPY_EXT, gl_anisotropic_filter );
	}
	else
	{
		qglTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, gl_filter_max );
		qglTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, gl_filter_max );

		if( glConfig.ext.texture_filter_anisotropic )
			qglTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1 );
	}

	// clamp if required
	if( !( flags & IT_CLAMP ) )
	{
		qglTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT );
		qglTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT );
		qglTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT );
	}
	else if( glConfig.ext.texture_edge_clamp )
	{
		qglTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		qglTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		qglTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );
	}
	else
	{
		qglTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP );
		qglTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP );
		qglTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP );
	}

	if( subImage )
		qglTexSubImage3D( GL_TEXTURE_3D, 0, 0, 0, 0, scaledWidth, scaledHeight, scaledDepth, format, GL_UNSIGNED_BYTE, data[0] );
	else
		qglTexImage3D( GL_TEXTURE_3D, 0, comp, scaledWidth, scaledHeight, scaledDepth, 0, format, GL_UNSIGNED_BYTE, data[0] );
}

/*
* R_AllocPic
*/
static image_t *R_AllocPic( void )
{
	image_t *image;
	unsigned int hash = image_cur_hash;

	if( !free_images ) {
		return NULL;
	}

	image = free_images;
	free_images = image->next;

	// link to the list of active images
	image->prev = &images_hash_headnode[hash];
	image->next = images_hash_headnode[hash].next;
	image->next->prev = image;
	image->prev->next = image;
	return image;
}

/*
* R_InitPic
*/
static image_t *R_InitPic( const char *name, qbyte **pic, int width, int height, int depth, int flags, int samples )
{
	image_t	*image;

	if( image_cur_hash >= IMAGES_HASH_SIZE )
		image_cur_hash = Com_HashKey( name, IMAGES_HASH_SIZE );

	image = R_AllocPic();
	if( !image ) {
		Com_Error( ERR_DROP, "R_InitPic: r_numImages == MAX_GLIMAGES" );
	}

	image->name = Mem_Alloc( r_texturesPool, strlen( name ) + 1 );
	strcpy( image->name, name );
	image->width = width;
	image->height = height;
	image->depth = image->upload_depth = depth;
	image->flags = flags;
	image->samples = samples;
	image->fbo = 0;
	image->registration_sequence = r_front.registration_sequence;

	qglGenTextures( 1, &image->texnum );
	GL_Bind( 0, image );

	if( depth == 1 )
		R_Upload32( pic, width, height, flags, &image->upload_width, &image->upload_height, &image->samples, image->samples, qfalse );
	else
		R_Upload32_3D_Fast( pic, width, height, depth, flags, &image->upload_width, &image->upload_height, &image->upload_depth, &image->samples, qfalse );

	if( flags & IT_FRAMEBUFFER )
	{
		image->fbo = R_RegisterFBObject ();
		if( image->fbo 
			&&! R_AttachTextureToFBOject( image->fbo, image, (image->flags & IT_DEPTH ? qtrue : qfalse) ) )
		{
			Com_Printf( S_COLOR_YELLOW "Warning: Error attaching texture to a FBO: %s\n", image->name );
			image->fbo = 0;
		}
	}

	image_cur_hash = IMAGES_HASH_SIZE+1;
	return image;
}

/*
* R_LoadPic
*/
image_t *R_LoadPic( const char *name, qbyte **pic, int width, int height, int flags, int samples )
{
	qbyte **data = pic;

	if( flags & IT_MIPTEX )
		samples = LoadMipTex( data, width, height, flags );

	if( !(flags & IT_CUBEMAP) )
	{
		qbyte *temp;

		if( flags & IT_LEFTHALF )
		{
			temp = R_PrepareImageBuffer( TEXTURE_CUT_BUF, width/2 * height * samples );
			R_CutImage( *data, width, height, temp, 0, 0, width/2, height, samples );
			data = &temp;
			width /= 2;
		}
		else if( flags & IT_RIGHTHALF )
		{
			temp = R_PrepareImageBuffer( TEXTURE_CUT_BUF, width/2 * height * samples );
			R_CutImage( *data, width, height, temp, width/2, 0, width/2, height, samples );
			data = &temp;
			width /= 2;
		}

		if( flags & ( IT_FLIPX|IT_FLIPY|IT_FLIPDIAGONAL ) )
		{
			temp = R_PrepareImageBuffer( TEXTURE_FLIPPING_BUF0, width * height * samples );
			R_FlipTexture( *data, temp, width, height, samples, (flags & IT_FLIPX), (flags & IT_FLIPY), (flags & IT_FLIPDIAGONAL) );
			data = &temp;
		}
	}

	return R_InitPic( name, data, width, height, 1, flags, samples );
}

/*
* R_Load3DPic
*/
image_t *R_Load3DPic( const char *name, qbyte **pic, int width, int height, int depth, int flags, int samples )
{
	return R_InitPic( name, pic, width, height, depth, flags, samples );
}

/*
* R_FindImage
* 
* Finds or loads the given image
*/
image_t	*R_FindImage( const char *name, const char *suffix, int flags, float bumpScale )
{
	int i, lastDot, lastSlash;
	unsigned int len, key;
	image_t	*image, *hnode;
	int width = 1, height = 1, samples = 0;
	char *pathname;
	const char *extension = "";
	size_t pathsize;

	if( !name || !name[0] )
		return NULL; //	Com_Error (ERR_DROP, "R_FindImage: NULL name");

	ENSUREBUFSIZE( imagePathBuf, strlen( name ) + (suffix ? strlen( suffix ) : 0) + 5 );
	pathname = r_imagePathBuf;
	pathsize = r_sizeof_imagePathBuf;

	lastDot = -1;
	lastSlash = -1;
	for( i = ( name[0] == '/' || name[0] == '\\' ), len = 0; name[i]; i++ )
	{
		if( name[i] == '.' )
			lastDot = len;
		if( name[i] == '\\' )
			pathname[len] = '/';
		else
			pathname[len] = tolower( name[i] );
		if( pathname[len] == '/' )
			lastSlash = len;
		len++;
	}

	if( len < 5 )
		return NULL;

	// don't confuse paths such as /ui/xyz.cache/123 with file extensions
	if( lastDot < lastSlash ) {
		lastDot = -1;
	}

	if( lastDot != -1 )
	{
		len = lastDot;
		extension = &name[len];
	}

	if( suffix )
	{
		for( i = 0; suffix[i]; i++ )
			pathname[len++] = tolower( suffix[i] );
	}

	pathname[len] = 0;

	// look for it
	key = image_cur_hash = Com_HashKey( pathname, IMAGES_HASH_SIZE );
	hnode = &images_hash_headnode[key];
	if( flags & IT_HEIGHTMAP )
	{
		for( image = hnode->prev; image != hnode; image = image->prev )
		{
			if( ( ( image->flags & flags ) == flags ) && ( image->bumpScale == bumpScale ) && !strcmp( image->name, pathname ) ) {
				R_TouchImage( image );
				return image;
			}
		}
	}
	else
	{
		for( image = hnode->prev; image != hnode; image = image->prev )
		{
			if( ( ( image->flags & flags ) == flags ) && !strcmp( image->name, pathname ) ) {
				R_TouchImage( image );
				return image;
			}
		}
	}

	pathname[len] = 0;
	image = NULL;

	//
	// load the pic from disk
	//
	if( flags & IT_CUBEMAP )
	{
		qbyte *pic[6];
		struct cubemapSufAndFlip
		{
			char *suf; int flags;
		} cubemapSides[2][6] = {
			{ 
				{ "px", 0 }, { "nx", 0 }, { "py", 0 },
				{ "ny", 0 }, { "pz", 0 }, { "nz", 0 } 
			},
			{
				{ "rt", IT_FLIPDIAGONAL }, { "lf", IT_FLIPX|IT_FLIPY|IT_FLIPDIAGONAL }, { "bk", IT_FLIPY },
				{ "ft", IT_FLIPX }, { "up", IT_FLIPDIAGONAL }, { "dn", IT_FLIPDIAGONAL }
			}
		};
		int j, lastSize = 0;

		pathname[len] = '_';
		for( i = 0; i < 2; i++ )
		{
			for( j = 0; j < 6; j++ )
			{
				pathname[len+1] = cubemapSides[i][j].suf[0];
				pathname[len+2] = cubemapSides[i][j].suf[1];
				pathname[len+3] = 0;

				Q_strncatz( pathname, extension, pathsize );
				samples = R_LoadImageFromDisk( pathname, pathsize, &(pic[j]), &width, &height, &flags, j );
				if( pic[j] )
				{
					if( width != height )
					{
						Com_Printf( "Not square cubemap image %s\n", pathname );
						break;
					}
					if( !j )
					{
						lastSize = width;
					}
					else if( lastSize != width )
					{
						Com_Printf( "Different cubemap image size: %s\n", pathname );
						break;
					}
					if( cubemapSides[i][j].flags & ( IT_FLIPX|IT_FLIPY|IT_FLIPDIAGONAL ) )
					{
						int flags = cubemapSides[i][j].flags;
						qbyte *temp = R_PrepareImageBuffer( TEXTURE_FLIPPING_BUF0+j, width * height * samples );
						R_FlipTexture( pic[j], temp, width, height, 4, (flags & IT_FLIPX), (flags & IT_FLIPY), (flags & IT_FLIPDIAGONAL) );
						pic[j] = temp;
					}
					continue;
				}
				break;
			}
			if( j == 6 )
				break;
		}

		if( i != 2 )
		{
			pathname[len] = 0;
			image = R_LoadPic( pathname, pic, width, height, flags, samples );
			image->extension[0] = '.';
			Q_strncpyz( &image->extension[1], &pathname[len+4], sizeof( image->extension )-1 );
		}
	}
	else
	{
		qbyte *pic;

		Q_strncatz( pathname, extension, pathsize );
		samples = R_LoadImageFromDisk( pathname, pathsize, &pic, &width, &height, &flags, 0 );

		if( pic )
		{
			qbyte *temp;

			if( flags & IT_HEIGHTMAP )
			{
				temp = R_PrepareImageBuffer( TEXTURE_FLIPPING_BUF0, width * height * 4 );
				samples = R_HeightmapToNormalmap( pic, temp, width, height, bumpScale, samples );
				pic = temp;
			}

			pathname[len] = 0;
			image = R_LoadPic( pathname, &pic, width, height, flags, samples );
			image->extension[0] = '.';
			Q_strncpyz( &image->extension[1], &pathname[len+1], sizeof( image->extension )-1 );
		}
	}

	return image;
}

/*
==============================================================================

SCREEN SHOTS

==============================================================================
*/

/*
* R_ScreenShot
*/
static void R_ScreenShot( const char *name, qboolean silent )
{
	char *checkname = NULL;
	size_t checkname_size = 0, gamepath_offset = 0;
	qbyte *buffer;

	if( name && name[0] && Q_stricmp(name, "*") )
	{
		checkname_size = sizeof( char ) * ( strlen( "screenshots/" ) + strlen( name ) + strlen( ".jpg" ) + 1 );
		checkname = Mem_TempMalloc( checkname_size );
		Q_snprintfz( checkname, checkname_size, "screenshots/%s", name );

		COM_SanitizeFilePath( checkname );

		if( !COM_ValidateRelativeFilename( checkname ) )
		{
			Com_Printf( "Invalid filename\n" );
			Mem_Free( checkname );
			return;
		}

		if( r_screenshot_jpeg->integer )
			COM_DefaultExtension( checkname, ".jpg", checkname_size );
		else
			COM_DefaultExtension( checkname, ".tga", checkname_size );
	}

	//
	// find a file name to save it to
	//
	if( !checkname )
	{
		int i;
		const int maxFiles = 100000;
		static int lastIndex = 0;
		qboolean addIndex = qfalse;
		time_t timestamp;
		char timestamp_str[MAX_QPATH];
		struct tm *timestampptr;

		timestamp = time( NULL );
		timestampptr = localtime( &timestamp );

		// validate timestamp string
		for( i = 0; i < 2; i++ )
		{
			strftime( timestamp_str, sizeof( timestamp_str ), r_screenshot_fmtstr->string, timestampptr );
			if( !COM_ValidateRelativeFilename( timestamp_str ) )
				Cvar_ForceSet( r_screenshot_fmtstr->name, r_screenshot_fmtstr->dvalue );
			else
				break;
		}

		// hm... shouldn't really happen, but check anyway
		if( i == 2 )
		{
			Q_strncpyz( timestamp_str, APP_SCREENSHOTS_PREFIX, sizeof( timestamp_str ) );
			Cvar_ForceSet( r_screenshot_fmtstr->name, APP_SCREENSHOTS_PREFIX );
		}

		gamepath_offset = strlen( FS_WriteDirectory() ) + 1 + strlen( FS_GameDirectory() ) + 1;

		checkname_size =
			sizeof( char ) * ( gamepath_offset + strlen( "screenshots/" ) + strlen( timestamp_str ) + 5 + strlen( ".jpg" ) + 1 );
		checkname = Mem_TempMalloc( checkname_size );

		// if the string format is a constant or file already exists then iterate
		if( !*timestamp_str || !strcmp( timestamp_str, r_screenshot_fmtstr->string ) )
		{
			addIndex = qtrue;

			// force a rescan in case some vars have changed..
			if( r_screenshot_fmtstr->modified )
			{
				lastIndex = 0;
				r_screenshot_fmtstr->modified = qtrue;
			}
			if( r_screenshot_jpeg->modified )
			{
				lastIndex = 0;
				r_screenshot_jpeg->modified = qfalse;
			}
		}
		else
		{
			Q_snprintfz( checkname, checkname_size, "%s/%s/screenshots/%s.%s", 
				FS_WriteDirectory(), FS_GameDirectory(), timestamp_str, r_screenshot_jpeg->integer ? "jpg" : "tga" );
			if( FS_FOpenAbsoluteFile( checkname, NULL, FS_READ ) != -1 )
			{
				lastIndex = 0;
				addIndex = qtrue;
			}
		}

		for( ; addIndex && lastIndex < maxFiles; lastIndex++ )
		{
			Q_snprintfz( checkname, checkname_size, "%s/%s/screenshots/%s%05i.%s", 
				FS_WriteDirectory(), FS_GameDirectory(), timestamp_str, lastIndex, r_screenshot_jpeg->integer ? "jpg" : "tga" );
			if( FS_FOpenAbsoluteFile( checkname, NULL, FS_READ ) == -1 )
				break; // file doesn't exist
		}

		if( lastIndex == maxFiles )
		{
			Com_Printf( "Couldn't create a file\n" );
			Mem_Free( checkname );
			return;
		}

		lastIndex++;
	}

	if( r_screenshot_jpeg->integer )
	{
		buffer = Mem_Alloc( r_texturesPool, glState.width * glState.height * 3 );
		qglReadPixels( 0, 0, glState.width, glState.height, GL_RGB, GL_UNSIGNED_BYTE, buffer );

		if( WriteJPG( checkname + gamepath_offset, buffer, glState.width, glState.height, r_screenshot_jpeg_quality->integer ) && !silent )
			Com_Printf( "Wrote %s\n", checkname );
	}
	else
	{
		buffer = Mem_Alloc( r_texturesPool, 18 + glState.width * glState.height * 3 );
		qglReadPixels( 0, 0, glState.width, glState.height, glConfig.ext.bgra ? GL_BGR_EXT : GL_RGB, GL_UNSIGNED_BYTE, buffer + 18 );

		if( WriteTGA( checkname + gamepath_offset, buffer, glState.width, glState.height, glConfig.ext.bgra ) && !silent )
			Com_Printf( "Wrote %s\n", checkname );
	}

	Mem_Free( buffer );
	Mem_Free( checkname );
}

/*
* R_ScreenShot_f
*/
void R_ScreenShot_f( void )
{
	R_ScreenShot( Cmd_Argv( 1 ), Cmd_Argc() >= 3 && !Q_stricmp( Cmd_Argv( 2 ), "silent" ) );
}

/*
* R_EnvShot_f
*/
void R_EnvShot_f( void )
{
	int i;
	int size, maxSize;
	qbyte *buffer, *bufferFlipped;
	int checkname_size;
	char *checkname;
	struct	cubemapSufAndFlip
	{
		char *suf; vec3_t angles; int flags;
	} cubemapShots[6] = {
		{ "px", { 0, 0, 0 }, IT_FLIPX|IT_FLIPY|IT_FLIPDIAGONAL },
		{ "nx", { 0, 180, 0 }, IT_FLIPDIAGONAL },
		{ "py", { 0, 90, 0 }, IT_FLIPY },
		{ "ny", { 0, 270, 0 }, IT_FLIPX },
		{ "pz", { -90, 180, 0 }, IT_FLIPDIAGONAL },
		{ "nz", { 90, 180, 0 }, IT_FLIPDIAGONAL }
	};

	if( !r_worldmodel )
		return;

	if( Cmd_Argc() != 3 )
	{
		Com_Printf( "usage: envshot <name> <size>\n" );
		return;
	}

	maxSize = min( glState.width, glState.height );
	if( maxSize > atoi( Cmd_Argv( 2 ) ) )
		maxSize = atoi( Cmd_Argv( 2 ) );

	for( size = 1; size < maxSize; size <<= 1 ) ;
	if( size > maxSize )
		size >>= 1;

	// do not render non-bmodel entities
	ri.params |= RP_ENVVIEW;

	buffer = Mem_Alloc( r_texturesPool, (size * size * 3) * 2 + 18 );
	bufferFlipped = buffer + size * size * 3;

	checkname_size = sizeof( char ) * ( strlen( "env/" ) + strlen( Cmd_Argv( 1 ) ) + 1 + strlen( cubemapShots[0].suf ) + 4 + 1 );
	checkname = Mem_TempMalloc( checkname_size );

	for( i = 0; i < 6; i++ )
	{
		R_DrawCubemapView( r_lastRefdef.vieworg, cubemapShots[i].angles, size );

		qglReadPixels( 0, glState.height - size, size, size, glConfig.ext.bgra ? GL_BGR_EXT : GL_RGB, GL_UNSIGNED_BYTE, buffer );

		R_FlipTexture( buffer, bufferFlipped + 18, size, size, 3, ( cubemapShots[i].flags & IT_FLIPX ), ( cubemapShots[i].flags & IT_FLIPY ), ( cubemapShots[i].flags & IT_FLIPDIAGONAL ) );

		Q_snprintfz( checkname, checkname_size, "env/%s_%s", Cmd_Argv( 1 ), cubemapShots[i].suf );
		//if( r_screenshot_jpeg->integer ) {
		//	COM_DefaultExtension( checkname, ".jpg", sizeof(checkname) );
		//	if( WriteJPG( checkname, bufferFlipped, size, size, r_screenshot_jpeg_quality->integer ) )
		//		Com_Printf( "Wrote envshot %s\n", checkname );
		//} else {
		COM_DefaultExtension( checkname, ".tga", checkname_size );
		if( WriteTGA( checkname, bufferFlipped, size, size, glConfig.ext.bgra ) )
			Com_Printf( "Wrote envshot %s\n", checkname );
		//}
	}

	// render non-bmodel entities again
	ri.params &= ~RP_ENVVIEW;

	Mem_Free( checkname );
	Mem_Free( buffer );
}

/*
* R_BeginAviDemo
*/
void R_BeginAviDemo( void )
{
	if( r_aviBuffer )
		Mem_Free( r_aviBuffer );
	r_aviBuffer = Mem_Alloc( r_texturesPool, 18 + glState.width * glState.height * 3 );
}

/*
* R_WriteAviFrame
*/
void R_WriteAviFrame( int frame, qboolean scissor )
{
	int x, y, w, h;
	int checkname_size;
	char *checkname;

	if( !r_aviBuffer )
		return;

	if( scissor )
	{
		x = r_lastRefdef.x;
		y = glState.height - r_lastRefdef.height - r_lastRefdef.y;
		w = r_lastRefdef.width;
		h = r_lastRefdef.height;
	}
	else
	{
		x = 0;
		y = 0;
		w = glState.width;
		h = glState.height;
	}

	checkname_size = sizeof( char ) * ( strlen( "avi/avi" ) + 6 + 4 + 1 );
	checkname = Mem_TempMalloc( checkname_size );
	Q_snprintfz( checkname, checkname_size, "avi/avi%06i", frame );

	if( r_screenshot_jpeg->integer )
	{
		COM_DefaultExtension( checkname, ".jpg", checkname_size );
		qglReadPixels( x, y, w, h, GL_RGB, GL_UNSIGNED_BYTE, r_aviBuffer );
		WriteJPG( checkname, r_aviBuffer, w, h, r_screenshot_jpeg_quality->integer );
	}
	else
	{
		COM_DefaultExtension( checkname, ".tga", checkname_size );
		qglReadPixels( x, y, w, h, glConfig.ext.bgra ? GL_BGR_EXT : GL_RGB, GL_UNSIGNED_BYTE, r_aviBuffer + 18 );
		WriteTGA( checkname, r_aviBuffer, w, h, glConfig.ext.bgra );
	}

	Mem_Free( checkname );
}

/*
* R_StopAviDemo
*/
void R_StopAviDemo( void )
{
	if( r_aviBuffer )
	{
		Mem_Free( r_aviBuffer );
		r_aviBuffer = NULL;
	}
}

//=======================================================

/*
* R_InitNoTexture
*/
static void R_InitNoTexture( int *w, int *h, int *depth, int *flags, int *samples )
{
	int x, y;
	qbyte *data;
	qbyte dottexture[8][8] =
	{
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 1, 1, 0, 0, 0, 0 },
		{ 0, 1, 1, 1, 1, 0, 0, 0 },
		{ 0, 1, 1, 1, 1, 0, 0, 0 },
		{ 0, 0, 1, 1, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
	};

	//
	// also use this for bad textures, but without alpha
	//
	*w = *h = 8;
	*depth = 1;
	*flags = 0;
	*samples = 3;

	// ch : check samples
	data = R_PrepareImageBuffer( TEXTURE_LOADING_BUF0, 8 * 8 * 3 );
	for( x = 0; x < 8; x++ )
	{
		for( y = 0; y < 8; y++ )
		{
			data[( y*8 + x )*3+0] = dottexture[x&3][y&3]*127;
			data[( y*8 + x )*3+1] = dottexture[x&3][y&3]*127;
			data[( y*8 + x )*3+2] = dottexture[x&3][y&3]*127;
		}
	}
}

/*
* R_InitDynamicLightTexture
*/
static void R_InitDynamicLightTexture( int *w, int *h, int *depth, int *flags, int *samples )
{
	vec3_t v = { 0, 0, 0 };
	float intensity;
	int x, y, z, d, size;
	qbyte *data;

	//
	// dynamic light texture
	//
	if( glConfig.ext.texture3D )
	{
		size = 32;
		*depth = 32;
	}
	else
	{
		size = 64;
		*depth = 1;
	}

	*w = *h = size;
	*flags = IT_NOPICMIP|IT_NOMIPMAP|IT_CLAMP|IT_NOCOMPRESS|IT_LUMINANCE;
	*samples = 1;

	data = R_PrepareImageBuffer( TEXTURE_LOADING_BUF0, size * size * *depth );
	for( x = 0; x < size; x++ )
	{
		for( y = 0; y < size; y++ )
		{
			for( z = 0; z < *depth; z++ )
			{
				v[0] = ( ( x + 0.5f ) * ( 2.0f / (float)size ) - 1.0f );
				v[1] = ( ( y + 0.5f ) * ( 2.0f / (float)size ) - 1.0f );
				if( *depth > 1 )
					v[2] = ( ( z + 0.5f ) * ( 2.0f / (float)*depth ) - 1.0f );

				intensity = 1.0f - sqrt( DotProduct( v, v ) );
				if( intensity > 0 )
					intensity = intensity * intensity * 215.5f;
				d = bound( 0, intensity, 255 );

				data[( z*size+y )*size + x] = d;
			}
		}
	}
}

/*
* R_InitSolidColorTexture
*/
static qbyte *R_InitSolidColorTexture( int *w, int *h, int *depth, int *flags, int *samples, int color )
{
	qbyte *data;

	//
	// solid color texture
	//
	*w = *h = 1;
	*depth = 1;
	*flags = IT_NOPICMIP|IT_NOCOMPRESS;
	*samples = 3;

	// ch : check samples
	data = R_PrepareImageBuffer( TEXTURE_LOADING_BUF0, 1 * 1 * 3 );
	data[0] = data[1] = data[2] = color;
	return data;
}

/*
* R_InitParticleTexture
*/
static void R_InitParticleTexture( int *w, int *h, int *depth, int *flags, int *samples )
{
	int x, y;
	int dx2, dy, d;
	qbyte *data;

	//
	// particle texture
	//
	*w = *h = 16;
	*depth = 1;
	*flags = IT_NOPICMIP|IT_NOMIPMAP;
	*samples = 4;

	data = R_PrepareImageBuffer( TEXTURE_LOADING_BUF0, 16 * 16 * 4 );
	for( x = 0; x < 16; x++ )
	{
		dx2 = x - 8;
		dx2 = dx2 * dx2;

		for( y = 0; y < 16; y++ )
		{
			dy = y - 8;
			d = 255 - 35 *sqrt( dx2 + dy *dy );
			data[( y*16 + x ) * 4 + 3] = bound( 0, d, 255 );
		}
	}
}

/*
* R_InitWhiteTexture
*/
static void R_InitWhiteTexture( int *w, int *h, int *depth, int *flags, int *samples )
{
	R_InitSolidColorTexture( w, h, depth, flags, samples, 255 );
}

/*
* R_InitBlackTexture
*/
static void R_InitBlackTexture( int *w, int *h, int *depth, int *flags, int *samples )
{
	R_InitSolidColorTexture( w, h, depth, flags, samples, 0 );
}

/*
* R_InitGreyTexture
*/
static void R_InitGreyTexture( int *w, int *h, int *depth, int *flags, int *samples )
{
	R_InitSolidColorTexture( w, h, depth, flags, samples, 127 );
}

/*
* R_InitBlankBumpTexture
*/
static void R_InitBlankBumpTexture( int *w, int *h, int *depth, int *flags, int *samples )
{
	qbyte *data = R_InitSolidColorTexture( w, h, depth, flags, samples, 128 );

/*
	data[0] = 128;	// normal X
	data[1] = 128;	// normal Y
*/
	data[2] = 255;	// normal Z
	data[3] = 128;	// height
}

/*
* R_InitFogTexture
*/
static void R_InitFogTexture( int *w, int *h, int *depth, int *flags, int *samples )
{
	qbyte *data;
	int x, y;
	double tw = 1.0f / ( (float)FOG_TEXTURE_WIDTH - 1.0f );
	double th = 1.0f / ( (float)FOG_TEXTURE_HEIGHT - 1.0f );
	double tx, ty, t;

	//
	// fog texture
	//
	*w = FOG_TEXTURE_WIDTH;
	*h = FOG_TEXTURE_HEIGHT;
	*depth = 1;
	*flags = IT_NOPICMIP|IT_NOMIPMAP|IT_CLAMP;
	*samples = 4;

	data = R_PrepareImageBuffer( TEXTURE_LOADING_BUF0, FOG_TEXTURE_WIDTH*FOG_TEXTURE_HEIGHT*4 );
	for( y = 0, ty = 0.0f; y < FOG_TEXTURE_HEIGHT; y++, ty += th )
	{
		for( x = 0, tx = 0.0f; x < FOG_TEXTURE_WIDTH; x++, tx += tw )
		{
			t = sqrt( tx ) * 255.0;
			data[( x+y*FOG_TEXTURE_WIDTH )*4+3] = (qbyte)( min( t, 255.0 ) );
		}
		data[y*4+3] = 0;
	}
}

/*
* R_InitCoronaTexture
*/
static void R_InitCoronaTexture( int *w, int *h, int *depth, int *flags, int *samples )
{
	int x, y, a;
	float dx, dy;
	qbyte *data;

	//
	// light corona texture
	//
	*w = *h = 32;
	*depth = 1;
	*flags = IT_NOMIPMAP|IT_NOPICMIP|IT_NOCOMPRESS|IT_CLAMP;
	*samples = 4;

	data = R_PrepareImageBuffer( TEXTURE_LOADING_BUF0, 32 * 32 * 4 );
	for( y = 0; y < 32; y++ )
	{
		dy = ( y - 15.5f ) * ( 1.0f / 16.0f );
		for( x = 0; x < 32; x++ )
		{
			dx = ( x - 15.5f ) * ( 1.0f / 16.0f );
			a = (int)( ( ( 1.0f / ( dx * dx + dy * dy + 0.2f ) ) - ( 1.0f / ( 1.0f + 0.2 ) ) ) * 32.0f / ( 1.0f / ( 1.0f + 0.2 ) ) );
			clamp( a, 0, 255 );
			data[( y*32+x )*4+0] = data[( y*32+x )*4+1] = data[( y*32+x )*4+2] = a;
		}
	}
}

/*
* R_InitScreenTexture
*/
static void R_InitScreenTexture( image_t **texture, const char *name, const char *key, int id, int screenWidth, int screenHeight, int size, int flags, int samples, qboolean screenLimit )
{
	int limit;
	int width, height;
	image_t *t;

	// limit the texture size to either screen resolution in case we can't use FBO
	// or hardware limits and ensure it's a POW2-texture if we don't support such textures
	limit = glConfig.maxTextureSize;
	if( size )
		limit = min( limit, size );
	if( limit < 1 )
		limit = 1;
	width = height = limit;

	if( glConfig.ext.texture_non_power_of_two )
	{
		if( screenLimit )
		{
			width = min( screenWidth, limit );
			height = min( screenHeight, limit );
		}
	}
	else
	{
		if( screenLimit )
			limit = min( limit, min( screenWidth, screenHeight ) );
		for( size = 2; size <= limit; size <<= 1 );
		width = height = size >> 1;
	}

	// create a new texture or update the old one
	if( !( *texture ) || ( *texture )->width != width || ( *texture )->height != height )
	{
		qbyte *data = NULL;

		if( !*texture )
		{
			char uploadName[128];

			if( key && *key )
				Q_snprintfz( uploadName, sizeof( uploadName ), "***%s_%s_%i***", name, key, id );
			else
				Q_snprintfz( uploadName, sizeof( uploadName ), "***%s_%i***", name, id );
			*texture = R_LoadPic( uploadName, &data, width, height, flags, samples );
			return;
		}

		t = *texture;
		GL_Bind( 0, t );
		t->width = width;
		t->height = height;
		R_Upload32( &data, width, height, flags, &t->upload_width, &t->upload_height, &t->samples, t->samples, qfalse );

		// update FBO if attached
		if( t->fbo )
		{
			if( !R_AttachTextureToFBOject( t->fbo, t, (flags & IT_DEPTH ? qtrue : qfalse) ) )
			{
				Com_Printf( S_COLOR_YELLOW "Warning: Error attaching texture to a FBO: %s\n", t->name );
				t->fbo = 0;
			}
		}
	}
}

/*
* R_InitPortalTexture
*/
void R_InitPortalTexture( image_t **texture, const char *key, int id, int screenWidth, int screenHeight, int flags )
{
	int size = r_portalmaps_maxtexsize->integer ? r_portalmaps_maxtexsize->integer : glConfig.maxTextureSize;
	qboolean screenLimit = qtrue;

//	if( size > glConfig.maxTextureSize / 2 ) size = glConfig.maxTextureSize / 2;
	if( size > 2048 ) size = 2048;

	R_InitScreenTexture( texture, "r_portaltexture", key, id, screenWidth, screenHeight, size, IT_PORTALMAP|IT_FRAMEBUFFER|flags, 3, screenLimit );
}

/*
* R_InitShadowmapTexture
*/
void R_InitShadowmapTexture( image_t **texture, int id, int screenWidth, int screenHeight, int flags )
{
	int size = r_shadows_maxtexsize->integer;
	qboolean screenLimit = (!glConfig.ext.framebuffer_object ? qtrue : qfalse);

	R_InitScreenTexture( texture, "r_shadowmap", NULL, id, screenWidth, screenHeight, size, IT_SHADOWMAP|IT_FRAMEBUFFER|flags, 1, screenLimit );
}


/*
* R_FindPortalTextureSlot
*/
int R_FindPortalTextureSlot( const char *key, int id )
{
	int i;
	image_t *image;
	char uploadName[128];
	const char *name = "r_portaltexture";

	if( key && *key )
		Q_snprintfz( uploadName, sizeof( uploadName ), "***%s_%s_%i***", name, key, id );
	else
		Q_snprintfz( uploadName, sizeof( uploadName ), "***%s_%i***", name, id );

	for( i = 0; ; i++ )
	{
		image = r_portaltextures[i];
		if( !image )
			break;

		if( !strcmp( image->name, uploadName ) )
			return i+1;
	}

	if( i == MAX_PORTAL_TEXTURES )
		return 0;

	return i+1;
}

/*
* R_InitCinematicTexture
*/
static void R_InitCinematicTexture( void )
{
	const char *name = "*** cinematic ***";

	// reserve a dummy texture slot
	image_cur_hash = Com_HashKey( name, IMAGES_HASH_SIZE );
	r_cintexture = R_AllocPic();

	assert( r_cintexture );
	if( !r_cintexture ) {
		Com_Error( ERR_FATAL, "Failed to register cinematic texture" );
	}

	r_cintexture->name = Mem_Alloc( r_texturesPool, strlen( name ) + 1 );
	strcpy( r_cintexture->name, name );
	qglGenTextures( 1, &r_cintexture->texnum );
	r_cintexture->depth = 1;
}

/*
* R_InitBuiltinTextures
*/
static void R_InitBuiltinTextures( void )
{
	int w, h, depth, flags, samples;
	image_t *image;
	const struct
	{
		char *name;
		image_t	**image;
		void ( *init )( int *w, int *h, int *depth, int *flags, int *samples );
	}
	textures[] =
	{
		{ "***r_notexture***", &r_notexture, &R_InitNoTexture },
		{ "***r_whitetexture***", &r_whitetexture, &R_InitWhiteTexture },
		{ "***r_blacktexture***", &r_blacktexture, &R_InitBlackTexture },
		{ "***r_greytexture***", &r_greytexture, &R_InitGreyTexture },
		{ "***r_blankbumptexture***", &r_blankbumptexture, &R_InitBlankBumpTexture },
		{ "***r_dlighttexture***", &r_dlighttexture, &R_InitDynamicLightTexture },
		{ "***r_particletexture***", &r_particletexture, &R_InitParticleTexture },
		{ "***r_fogtexture***", &r_fogtexture, &R_InitFogTexture },
		{ "***r_coronatexture***", &r_coronatexture, &R_InitCoronaTexture },

		{ NULL, NULL, NULL }
	};
	size_t i, num_builtin_textures = sizeof( textures ) / sizeof( textures[0] ) - 1;

	for( i = 0; i < num_builtin_textures; i++ )
	{
		textures[i].init( &w, &h, &depth, &flags, &samples );

		image = ( depth == 1 ?
			R_LoadPic( textures[i].name, r_imageBuffers, w, h, flags, samples ) :
		R_Load3DPic( textures[i].name, r_imageBuffers, w, h, depth, flags, samples )
			);

		if( textures[i].image )
			*( textures[i].image ) = image;
	}
}

/*
* R_FillDrawFlatTexture
*/
static void R_FillDrawFlatTexture( int *w, int *h, int *depth, int *flags, int *samples )
{
	int i, z;
	int wallIndex;
	qbyte wallColor[3], floorColor[3];
	qbyte *data;

	//
	// solid color texture
	//
	*w = *h = 1;
	*depth = DRAWFLAT_TEXTURE_DEPTH;
	*flags = IT_NOPICMIP|IT_NOCOMPRESS|IT_NOFILTERING|IT_NOMIPMAP|IT_CLAMP;
	*samples = 3;

	// pixel at which we consider wall to be either a ceiling or a floor
	wallIndex = (DRAWFLAT_NORMAL_STEP * DRAWFLAT_TEXTURE_DEPTH) * 0.5;

	// presetup some variables..
	for( i = 0; i < 3; i++ ) {
		wallColor[i] = (qbyte)bound( 0, (int)(r_front.wallColor[i] * 255.0), 255 );
		floorColor[i] = (qbyte)bound( 0, (int)(r_front.floorColor[i] * 255.0), 255 );
	}

	data = R_PrepareImageBuffer( TEXTURE_LOADING_BUF0, 1 * 1 * DRAWFLAT_TEXTURE_DEPTH * 3 );
	for( z = 0; z < DRAWFLAT_TEXTURE_DEPTH; z++, data += 3 ) {
		if( z < wallIndex || z > DRAWFLAT_TEXTURE_DEPTH - wallIndex ) {
			// floor or ceiling
			VectorCopy( floorColor, data );
		}
		else {
			// wall
			VectorCopy( wallColor, data );
		}
	}
}

/*
* R_InitDrawFlatTexture
*
* This is a special builtin texture since we need to rebuild it upon
* changes in wall/floor color cvars' values.
*/
void R_InitDrawFlatTexture( void )
{
	int w, h, d;
	int flags, samples;

	if( !glConfig.ext.texture3D ) {
		r_drawflattexture = NULL;
		return;
	}

	R_FillDrawFlatTexture( &w, &h, &d, &flags, &samples );

	if( r_drawflattexture ) {
		image_t *img = r_drawflattexture;

		// update an already existing texture
		GL_Bind( 0, img );

		R_Upload32_3D_Fast( r_imageBuffers, w, h, d, flags, 
			&img->upload_width, &img->upload_height, &img->upload_depth, 
			&img->samples, qfalse );
		return;
	}

	// fresh texture initialization
	r_drawflattexture = R_Load3DPic( "***drawflattexture***", r_imageBuffers, w, h, d, flags, samples );
}

/*
* R_TouchBuiltinTextures
*/
static void R_TouchBuiltinTextures( void )
{
	r_cintexture->registration_sequence =
	r_notexture->registration_sequence =
	r_whitetexture->registration_sequence =
	r_blacktexture->registration_sequence = 
	r_greytexture->registration_sequence =
	r_blankbumptexture->registration_sequence = 
	r_dlighttexture->registration_sequence = 
	r_particletexture->registration_sequence = 
	r_fogtexture->registration_sequence = 
	r_coronatexture->registration_sequence = r_front.registration_sequence;
	if( r_drawflattexture != NULL ) {
		r_drawflattexture->registration_sequence = r_front.registration_sequence;
	}
}

/*
* R_ReleaseBuiltinTextures
*/
static void R_ReleaseBuiltinTextures( void )
{
	r_cintexture = NULL;
	r_notexture = NULL;
	r_whitetexture = r_blacktexture = r_greytexture = NULL;
	r_blankbumptexture = NULL;
	r_dlighttexture = NULL;
	r_particletexture = NULL;
	r_fogtexture = NULL;
	r_coronatexture = NULL;
	r_drawflattexture = NULL;
}

//=======================================================

/*
* R_InitImages
*/
void R_InitImages( void )
{
	int i;

	// allow any alignment
	qglPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	qglPixelStorei(GL_PACK_ALIGNMENT, 1);

	r_texturesPool = Mem_AllocPool( NULL, "Textures" );
	image_cur_hash = IMAGES_HASH_SIZE+1;

	r_imagePathBuf = r_imagePathBuf2 = NULL;
	r_sizeof_imagePathBuf = r_sizeof_imagePathBuf2 = 0;

	r_8to24table = NULL;

	memset( images, 0, sizeof( images ) );

	// link images
	free_images = images;
	for( i = 0; i < IMAGES_HASH_SIZE; i++ ) {
		images_hash_headnode[i].prev = &images_hash_headnode[i];
		images_hash_headnode[i].next = &images_hash_headnode[i];
	}
	for( i = 0; i < MAX_GLIMAGES - 1; i++ ) {
		images[i].next = &images[i+1];
	}

	R_InitCinematicTexture();
	R_InitBuiltinTextures();
	R_InitDrawFlatTexture();
	R_InitBloomTextures();
}

/*
* R_TouchImage
*/
void R_TouchImage( image_t *image )
{
	if( image->registration_sequence == r_front.registration_sequence ) {
		return;
	}

	image->registration_sequence = r_front.registration_sequence;
	if( image->fbo ) {
		R_TouchFBObject( image->fbo );
	}
}

/*
* R_FreeImage
*/
void R_FreeImage( image_t *image )
{
	qglDeleteTextures( 1, &image->texnum );

	Mem_Free( image->name );
	image->name = NULL;
	image->texnum = 0;
	image->registration_sequence = 0;

	// remove from linked active list
	image->prev->next = image->next;
	image->next->prev = image->prev;

	// insert into linked free list
	image->next = free_images;
	free_images = image;
}


/*
* R_FreeUnusedImages
*/
void R_FreeUnusedImages( void )
{
	int i;
	image_t *image;

	R_TouchBuiltinTextures();
	R_TouchBloomTextures();

	for( i = 0, image = images; i < MAX_GLIMAGES; i++, image++ ) {
		if( !image->name ) {
			// free image
			continue;
		}
		if( image->registration_sequence == r_front.registration_sequence ) {
			// we need this image
			continue;
		}
		R_FreeImage( image );
	}

	for( i = 0; i < MAX_PORTAL_TEXTURES; i++ ) {
		if( r_portaltextures[i] && r_portaltextures[i]->registration_sequence != r_front.registration_sequence ) {
			r_portaltextures[i] = NULL;
		}
	}

	for( i = 0; i < MAX_SHADOWGROUPS; i++ ) {
		if( r_shadowmapTextures[i] && r_shadowmapTextures[i]->registration_sequence != r_front.registration_sequence ) {
			r_shadowmapTextures[i] = NULL;
		}
	}
}

/*
* R_ShutdownImages
*/
void R_ShutdownImages( void )
{
	int i;
	image_t *image;

	if( !r_texturesPool )
		return;

	R_ReleaseBuiltinTextures();

	for( i = 0, image = images; i < MAX_GLIMAGES; i++, image++ ) {
		if( !image->name ) {
			// free texture
			continue;
		}
		R_FreeImage( image );
	}

	R_FreeImageBuffers();

	if( r_imagePathBuf )
		Mem_Free( r_imagePathBuf );
	if( r_imagePathBuf2 )
		Mem_Free( r_imagePathBuf2 );

	if( r_8to24table )
	{
		Mem_Free( r_8to24table );
		r_8to24table = NULL;
	}

	Mem_FreePool( &r_texturesPool );

	memset( r_portaltextures, 0, sizeof( image_t * ) * MAX_PORTAL_TEXTURES );
	memset( r_shadowmapTextures, 0, sizeof( image_t * ) * MAX_SHADOWGROUPS );

	r_imagePathBuf = r_imagePathBuf2 = NULL;
	r_sizeof_imagePathBuf = r_sizeof_imagePathBuf2 = 0;
}
