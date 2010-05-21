/*
   Copyright (C) 2002-2003 Victor Luchits

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

#include "client.h"
#include "cin.h"

static short snd_sqr_arr[256];

/*
   ==================
   RoQ_Init
   ==================
 */
void RoQ_Init( void )
{
	int i;

	for( i = 0; i < 128; i++ )
	{
		snd_sqr_arr[i] = i * i;
		snd_sqr_arr[i + 128] = -( i * i );
	}
}

/*
   ==================
   RoQ_ReadChunk
   ==================
 */
void RoQ_ReadChunk( cinematics_t *cin )
{
	roq_chunk_t *chunk = &cin->chunk;

	FS_Read( &chunk->id, sizeof( short ), cin->file );
	FS_Read( &chunk->size, sizeof( int ), cin->file );
	FS_Read( &chunk->argument, sizeof( short ), cin->file );

	chunk->id = LittleShort( chunk->id );
	chunk->size = LittleLong( chunk->size );
	chunk->argument = LittleShort( chunk->argument );
}

/*
   ==================
   RoQ_SkipBlock
   ==================
 */
static inline void RoQ_SkipBlock( cinematics_t *cin, int size )
{
	FS_Seek( cin->file, size, FS_SEEK_CUR );
}

/*
   ==================
   RoQ_SkipChunk
   ==================
 */
void RoQ_SkipChunk( cinematics_t *cin )
{
	RoQ_SkipBlock( cin, cin->chunk.size );
}

/*
   ==================
   RoQ_ReadInfo
   ==================
 */
void RoQ_ReadInfo( cinematics_t *cin )
{
	short t[4];

	FS_Read( t, sizeof( short ) * 4, cin->file );

	if( cin->width != LittleShort( t[0] ) || cin->height != LittleShort( t[1] ) )
	{
		cin->width = LittleShort( t[0] );
		cin->height = LittleShort( t[1] );

		if( cin->vid_buffer )
			Mem_Free( cin->vid_buffer );

		// default to 255 for alpha
		if( cin->mempool )
			cin->vid_buffer = Mem_AllocExt( cin->mempool, cin->width * cin->height * 4 * 2, 0 );
		else
			cin->vid_buffer = Mem_ZoneMallocExt( cin->width * cin->height * 4 * 2, 0 );
		memset( cin->vid_buffer, 0xFF, cin->width * cin->height * 4 * 2 );

		cin->vid_pic[0] = cin->vid_buffer;
		cin->vid_pic[1] = cin->vid_buffer + cin->width * cin->height * 4;
	}
}

/*
   ==================
   RoQ_ReadCodebook
   ==================
 */
void RoQ_ReadCodebook( cinematics_t *cin )
{
	unsigned int nv1, nv2;
	roq_chunk_t *chunk = &cin->chunk;

	nv1 = ( chunk->argument >> 8 ) & 0xFF;
	if( !nv1 )
		nv1 = 256;

	nv2 = chunk->argument & 0xFF;
	if( !nv2 && ( nv1 * 6 < chunk->size ) )
		nv2 = 256;

	FS_Read( cin->cells, sizeof( roq_cell_t )*nv1, cin->file );
	FS_Read( cin->qcells, sizeof( roq_qcell_t )*nv2, cin->file );
}

/*
   ==================
   RoQ_ApplyVector2x2
   ==================
 */
static void RoQ_DecodeBlock( qbyte *dst0, qbyte *dst1, const qbyte *src0, const qbyte *src1, float u, float v )
{
	int c[3];

	// convert YCbCr to RGB
	VectorSet( c, 1.402f * v, -0.34414f * u - 0.71414f * v, 1.772f * u );

	// 1st pixel
	dst0[0] = bound( 0, c[0] + src0[0], 255 );
	dst0[1] = bound( 0, c[1] + src0[0], 255 );
	dst0[2] = bound( 0, c[2] + src0[0], 255 );

	// 2nd pixel
	dst0[4] = bound( 0, c[0] + src0[1], 255 );
	dst0[5] = bound( 0, c[1] + src0[1], 255 );
	dst0[6] = bound( 0, c[2] + src0[1], 255 );

	// 3rd pixel
	dst1[0] = bound( 0, c[0] + src1[0], 255 );
	dst1[1] = bound( 0, c[1] + src1[0], 255 );
	dst1[2] = bound( 0, c[2] + src1[0], 255 );

	// 4th pixel
	dst1[4] = bound( 0, c[0] + src1[1], 255 );
	dst1[5] = bound( 0, c[1] + src1[1], 255 );
	dst1[6] = bound( 0, c[2] + src1[1], 255 );
}

/*
   ==================
   RoQ_ApplyVector2x2
   ==================
 */
static void RoQ_ApplyVector2x2( cinematics_t *cin, int x, int y, const roq_cell_t *cell )
{
	qbyte *dst0, *dst1;

	dst0 = cin->vid_pic[0] + ( y * cin->width + x ) * 4;
	dst1 = dst0 + cin->width * 4;

	RoQ_DecodeBlock( dst0, dst1, cell->y, cell->y+2, (float)( (int)cell->u-128 ), (float)( (int)cell->v-128 ) );
}

/*
   ==================
   RoQ_ApplyVector4x4
   ==================
 */
static void RoQ_ApplyVector4x4( cinematics_t *cin, int x, int y, const roq_cell_t *cell )
{
	qbyte *dst0, *dst1;
	qbyte p[4];
	float u, v;

	u = (float)( (int)cell->u - 128 );
	v = (float)( (int)cell->v - 128 );

	p[0] = p[1] = cell->y[0];
	p[2] = p[3] = cell->y[1];
	dst0 = cin->vid_pic[0] + ( y * cin->width + x ) * 4; dst1 = dst0 + cin->width * 4;
	RoQ_DecodeBlock( dst0, dst0+8, p, p+2, u, v );
	RoQ_DecodeBlock( dst1, dst1+8, p, p+2, u, v );

	p[0] = p[1] = cell->y[2];
	p[2] = p[3] = cell->y[3];
	dst0 += cin->width * 4 * 2; dst1 += cin->width * 4 * 2;
	RoQ_DecodeBlock( dst0, dst0+8, p, p+2, u, v );
	RoQ_DecodeBlock( dst1, dst1+8, p, p+2, u, v );
}

/*
   ==================
   RoQ_ApplyMotion4x4
   ==================
 */
static void RoQ_ApplyMotion4x4( cinematics_t *cin, int x, int y, qbyte mv, char mean_x, char mean_y )
{
	int x0, y0;
	qbyte *src, *dst;

	// calc source coords
	x0 = x + 8 - ( mv >> 4 ) - mean_x;
	y0 = y + 8 - ( mv & 0xF ) - mean_y;

	src = cin->vid_pic[1] + ( y0 * cin->width + x0 ) * 4;
	dst = cin->vid_pic[0] + ( y * cin->width + x ) * 4;

	for( y = 0; y < 4; y++, src += cin->width * 4, dst += cin->width * 4 )
		memcpy( dst, src, 4 * 4 );
}

/*
   ==================
   RoQ_ApplyMotion8x8
   ==================
 */
static void RoQ_ApplyMotion8x8( cinematics_t *cin, int x, int y, qbyte mv, char mean_x, char mean_y )
{
	int x0, y0;
	qbyte *src, *dst;

	// calc source coords
	x0 = x + 8 - ( mv >> 4 ) - mean_x;
	y0 = y + 8 - ( mv & 0xF ) - mean_y;

	src = cin->vid_pic[1] + ( y0 * cin->width + x0 ) * 4;
	dst = cin->vid_pic[0] + ( y * cin->width + x ) * 4;

	for( y = 0; y < 8; y++, src += cin->width * 4, dst += cin->width * 4 )
		memcpy( dst, src, 8 * 4 );
}

/*
   ==================
   RoQ_ReadVideo
   ==================
 */
#define RoQ_READ_BLOCK	0x4000
qbyte *RoQ_ReadVideo( cinematics_t *cin )
{
	roq_chunk_t *chunk = &cin->chunk;
	int i, vqflg, vqflg_pos, vqid;
	int xpos, ypos, x, y, xp, yp;
	qbyte c, *tp;
	roq_qcell_t *qcell;
	qbyte raw[RoQ_READ_BLOCK];
	unsigned remaining, bpos, read;

	vqflg = 0;
	vqflg_pos = -1;
	xpos = ypos = 0;

#define RoQ_ReadRaw() read = min( sizeof( raw ), remaining ); remaining -= read; FS_Read( raw, read, cin->file );
#define RoQ_ReadByte( x ) if( bpos >= read ) { RoQ_ReadRaw(); bpos = 0; } ( x ) = raw[bpos++];
#define RoQ_ReadShort( x ) if( bpos+1 == read ) { c = raw[bpos]; RoQ_ReadRaw(); ( x ) = ( raw[0] << 8 )|c; bpos = 1; } \
	else { if( bpos+1 > read ) { RoQ_ReadRaw(); bpos = 0; } ( x ) = ( raw[bpos+1] << 8 )|raw[bpos]; bpos += 2; }
#define RoQ_ReadFlag() if( vqflg_pos < 0 ) { RoQ_ReadShort( vqflg ); vqflg_pos = 7; } \
	vqid = ( vqflg >> ( vqflg_pos * 2 ) ) & 0x3; vqflg_pos--;

	for( bpos = read = 0, remaining = chunk->size; bpos < read || remaining; )
	{
		for( yp = ypos; yp < ypos + 16; yp += 8 )
			for( xp = xpos; xp < xpos + 16; xp += 8 )
			{
				RoQ_ReadFlag();

				switch( vqid )
				{
				case RoQ_ID_MOT:
					break;

				case RoQ_ID_FCC:
					RoQ_ReadByte( c );
					RoQ_ApplyMotion8x8( cin, xp, yp, c, ( char )( ( chunk->argument >> 8 ) & 0xff ), (char)( chunk->argument & 0xff ) );
					break;

				case RoQ_ID_SLD:
					RoQ_ReadByte( c );
					qcell = cin->qcells + c;
					RoQ_ApplyVector4x4( cin, xp, yp, cin->cells + qcell->idx[0] );
					RoQ_ApplyVector4x4( cin, xp+4, yp, cin->cells + qcell->idx[1] );
					RoQ_ApplyVector4x4( cin, xp, yp+4, cin->cells + qcell->idx[2] );
					RoQ_ApplyVector4x4( cin, xp+4, yp+4, cin->cells + qcell->idx[3] );
					break;

				case RoQ_ID_CCC:
					for( i = 0; i < 4; i++ )
					{
						x = xp; if( i & 0x01 ) x += 4;
						y = yp; if( i & 0x02 ) y += 4;

						RoQ_ReadFlag();

						switch( vqid )
						{
						case RoQ_ID_MOT:
							break;

						case RoQ_ID_FCC:
							RoQ_ReadByte( c );
							RoQ_ApplyMotion4x4( cin, x, y, c, ( char )( ( chunk->argument >> 8 ) & 0xff ), (char)( chunk->argument & 0xff ) );
							break;

						case RoQ_ID_SLD:
							RoQ_ReadByte( c );
							qcell = cin->qcells + c;
							RoQ_ApplyVector2x2( cin, x, y, cin->cells + qcell->idx[0] );
							RoQ_ApplyVector2x2( cin, x+2, y, cin->cells + qcell->idx[1] );
							RoQ_ApplyVector2x2( cin, x, y+2, cin->cells + qcell->idx[2] );
							RoQ_ApplyVector2x2( cin, x+2, y+2, cin->cells + qcell->idx[3] );
							break;

						case RoQ_ID_CCC:
							RoQ_ReadByte( c ); RoQ_ApplyVector2x2( cin, x, y, cin->cells + c );
							RoQ_ReadByte( c ); RoQ_ApplyVector2x2( cin, x+2, y, cin->cells + c );
							RoQ_ReadByte( c ); RoQ_ApplyVector2x2( cin, x, y+2, cin->cells + c );
							RoQ_ReadByte( c ); RoQ_ApplyVector2x2( cin, x+2, y+2, cin->cells + c );
							break;

						default:
							Com_DPrintf( "Unknown vq code: %d\n", vqid );
							break;
						}
					}
					break;

				default:
					Com_DPrintf( "Unknown vq code: %d\n", vqid );
					break;
				}
			}

		xpos += 16;
		if( xpos >= cin->width )
		{
			xpos -= cin->width;

			ypos += 16;
			if( ypos >= cin->height )
			{
				RoQ_SkipBlock( cin, remaining ); // ignore remaining trash
				break;
			}
		}
	}

	if( cin->frame++ == 0 )
	{                           // copy initial values to back buffer for motion
		memcpy( cin->vid_pic[1], cin->vid_pic[0], cin->width * cin->height * 4 );
	}
	else
	{       // swap buffers
		tp = cin->vid_pic[0]; cin->vid_pic[0] = cin->vid_pic[1]; cin->vid_pic[1] = tp;
	}

	return cin->vid_pic[1];
}

/*
   ==================
   RoQ_ReadAudio
   ==================
 */
void RoQ_ReadAudio( cinematics_t *cin )
{
	unsigned int i;
	int snd_left, snd_right;
	qbyte raw[RoQ_READ_BLOCK];
	short samples[RoQ_READ_BLOCK];
	roq_chunk_t *chunk = &cin->chunk;
	unsigned int remaining, read;

	if( chunk->id == RoQ_SOUND_MONO )
	{
		snd_left = chunk->argument;
		snd_right = 0;
	}
	else
	{
		snd_left = chunk->argument & 0xff00;
		snd_right = ( chunk->argument & 0xff ) << 8;
	}

	for( remaining = chunk->size; remaining > 0; remaining -= read )
	{
		read = min( sizeof( raw ), remaining );
		FS_Read( raw, read, cin->file );

		if( chunk->id == RoQ_SOUND_MONO )
		{
			for( i = 0; i < read; i++ )
			{
				snd_left += snd_sqr_arr[raw[i]];
				samples[i] = (short)snd_left;
				snd_left = (short)snd_left;
			}

			CL_SoundModule_RawSamples( read, cin->s_rate, 2, 1, (qbyte *)samples, qfalse );
		}
		else if( chunk->id == RoQ_SOUND_STEREO )
		{
			for( i = 0; i < read; i += 2 )
			{
				snd_left += snd_sqr_arr[raw[i]];
				samples[i+0] = (short)snd_left;
				snd_left = (short)snd_left;

				snd_right += snd_sqr_arr[raw[i+1]];
				samples[i+1] = (short)snd_right;
				snd_right = (short)snd_right;
			}

			CL_SoundModule_RawSamples( read / 2, cin->s_rate, 2, 2, (qbyte *)samples, qfalse );
		}
	}
}
