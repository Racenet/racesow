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
// snd_mem.c: sound caching

#include "snd_local.h"

/*
================
ResampleSfx
================
*/
void ResampleSfx( sfxcache_t *sc, qbyte *data, char *name )
{
	int i, srclength, outcount, fracstep, chancount;
	int samplefrac, srcsample, srcnextsample;

	// this is usually 0.5 (128), 1 (256), or 2 (512)
	fracstep = ( (double) sc->speed / (double) dma.speed ) * 256.0;

	chancount = sc->channels - 1;
	srclength = sc->length / sc->channels;
	outcount = (double) sc->length * (double) dma.speed / (double) sc->speed;

	sc->length = outcount;
	if( sc->loopstart != -1 )
		sc->loopstart = (double) sc->loopstart * (double) dma.speed / (double) sc->speed;

	sc->speed = dma.speed;

	// resample / decimate to the current source rate
	if( fracstep == 256 )
	{
		if( sc->width == 2 )
			for( i = 0; i < srclength; i++ )
				( (short *)sc->data )[i] = LittleShort( ( (short *)data )[i] );
		else  // 8bit
			for( i = 0; i < srclength; i++ )
				( (signed char *)sc->data )[i] = ( (unsigned char *)data )[i] - 128;
	}
	else
	{
		int j, a, b, sample;

		// general case
		//Com_DPrintf("ResampleSfx: resampling sound %s\n", name);
		samplefrac = 0;
		srcsample = 0;
		srcnextsample = sc->channels;
		outcount *= sc->channels;

#define RESAMPLE_AND_ADVANCE    \
	sample = ( ( ( b - a ) * ( samplefrac & 255 ) ) >> 8 ) + a; \
	if( j == chancount ) \
		{ \
		samplefrac += fracstep; \
		srcsample = ( samplefrac >> 8 ) << chancount; \
		srcnextsample = srcsample + sc->channels; \
		}

		if( sc->width == 2 )
		{
			short *out = (void *)sc->data, *in = (void *)data;

			for( i = 0, j = 0; i < outcount; i++, j = i & chancount )
			{
				a = LittleShort( in[srcsample + j] );
				b = ( ( srcnextsample < srclength ) ? LittleShort( in[srcnextsample + j] ) : 0 );
				RESAMPLE_AND_ADVANCE;
				*out++ = (short)sample;
			}
		}
		else
		{
			signed char *out = (void *)sc->data;
			unsigned char *in = (void *)data;

			for( i = 0, j = 0; i < outcount; i++, j = i & chancount )
			{
				a = (int)in[srcsample + j] - 128;
				b = ( ( srcnextsample < srclength ) ? (int)in[srcnextsample + j] - 128 : 0 );
				RESAMPLE_AND_ADVANCE;
				*out++ = (signed char)sample;
			}
		}
	}
}


//=============================================================================

/*
==============
S_LoadSound_Wav
==============
*/
sfxcache_t *S_LoadSound_Wav( sfx_t *s )
{
	char namebuffer[MAX_QPATH];
	qbyte *data;
	wavinfo_t info;
	int len, file;
	sfxcache_t *sc;
	int size;

	assert( s && s->name[0] );
	assert( !s->cache );

	// load it in
	Q_strncpyz( namebuffer, s->name, sizeof( namebuffer ) );
	size = trap_FS_FOpenFile( namebuffer, &file, FS_READ );

	if( !file )
	{
		//Com_DPrintf ("Couldn't load %s\n", namebuffer);
		return NULL;
	}

	data = S_Malloc( size );
	trap_FS_Read( data, size, file );
	trap_FS_FCloseFile( file );

	info = GetWavinfo( s->name, data, size );
	if( info.channels < 1 || info.channels > 2 )
	{
		Com_Printf( "%s has an invalid number of channels\n", s->name );
		S_Free( data );
		return NULL;
	}

	// calculate resampled length
	len = (int) ( (double) info.samples * (double) dma.speed / (double) info.rate );
	len = len * info.width * info.channels;

	sc = s->cache = S_Malloc( len + sizeof( sfxcache_t ) );
	if( !sc )
	{
		S_Free( data );
		return NULL;
	}

	sc->length = info.samples;
	sc->loopstart = info.loopstart;
	sc->speed = info.rate;
	sc->channels = info.channels;
	sc->width = info.width;

	ResampleSfx( sc, data + info.dataofs, s->name );

	S_Free( data );

	return sc;
}

/*
==============
S_LoadSound
==============
*/
sfxcache_t *S_LoadSound( sfx_t *s )
{
	const char *extension;

	if( !s->name[0] )
		return NULL;

	// see if still in memory
	if( s->cache )
		return s->cache;

	extension = COM_FileExtension( s->name );
	if( extension )
	{
		if( !Q_stricmp( extension, ".wav" ) )
		{
			return S_LoadSound_Wav( s );
		}
		if( !Q_stricmp( extension, ".ogg" ) )
		{
			return SNDOGG_Load( s );
		}
	}

	return NULL;
}



/*
===============================================================================

WAV loading

===============================================================================
*/


qbyte *data_p;
qbyte *iff_end;
qbyte *last_chunk;
qbyte *iff_data;
int iff_chunk_len;


static short GetLittleShort( void )
{
	short val = 0;
	val = *data_p;
	val = val + ( *( data_p+1 )<<8 );
	data_p += 2;
	return val;
}

static int GetLittleLong( void )
{
	int val = 0;
	val = *data_p;
	val = val + ( *( data_p+1 )<<8 );
	val = val + ( *( data_p+2 )<<16 );
	val = val + ( *( data_p+3 )<<24 );
	data_p += 4;
	return val;
}

static void FindNextChunk( char *name )
{
	while( 1 )
	{
		data_p = last_chunk;

		if( data_p >= iff_end )
		{ // didn't find the chunk
			data_p = NULL;
			return;
		}

		data_p += 4;
		iff_chunk_len = GetLittleLong();
		if( iff_chunk_len < 0 )
		{
			data_p = NULL;
			return;
		}

		data_p -= 8;
		last_chunk = data_p + 8 + ( ( iff_chunk_len + 1 ) & ~1 );
		if( !strncmp( (char *) data_p, name, 4 ) )
			return;
	}
}

static void FindChunk( char *name )
{
	last_chunk = iff_data;
	FindNextChunk( name );
}

/*
============
GetWavinfo
============
*/
wavinfo_t GetWavinfo( const char *name, qbyte *wav, int wavlength )
{
	wavinfo_t info;
	int i;
	int format;
	int samples;

	memset( &info, 0, sizeof( info ) );

	if( !wav )
		return info;

	iff_data = wav;
	iff_end = wav + wavlength;

	// find "RIFF" chunk
	FindChunk( "RIFF" );
	if( !( data_p && !strncmp( (char *) data_p+8, "WAVE", 4 ) ) )
	{
		Com_Printf( "Missing RIFF/WAVE chunks\n" );
		return info;
	}

	// get "fmt " chunk
	iff_data = data_p + 12;

	FindChunk( "fmt " );
	if( !data_p )
	{
		Com_Printf( "Missing fmt chunk\n" );
		return info;
	}

	data_p += 8;
	format = GetLittleShort();
	if( format != 1 )
	{
		Com_Printf( "Microsoft PCM format only\n" );
		return info;
	}

	info.channels = GetLittleShort();
	info.rate = GetLittleLong();
	data_p += 4+2;
	info.width = GetLittleShort() / 8;

	// get cue chunk
	FindChunk( "cue " );
	if( data_p )
	{
		data_p += 32;
		info.loopstart = GetLittleLong();

		// if the next chunk is a LIST chunk, look for a cue length marker
		FindNextChunk( "LIST" );
		if( data_p )
		{
			if( !strncmp( (char *) data_p + 28, "mark", 4 ) )
			{ // this is not a proper parse, but it works with cooledit...
				data_p += 24;
				i = GetLittleLong(); // samples in loop
				info.samples = info.loopstart + i;
			}
		}
	}
	else
		info.loopstart = -1;

	// find data chunk
	FindChunk( "data" );
	if( !data_p )
	{
		Com_Printf( "Missing data chunk\n" );
		return info;
	}

	data_p += 4;
	samples = GetLittleLong() / info.width / info.channels;

	if( info.samples )
	{
		if( samples < info.samples )
			S_Error( "Sound %s has a bad loop length", name ); // FIXME: Medar: Should be ERR_DROP?
	}
	else
		info.samples = samples;

	info.dataofs = data_p - wav;

	return info;
}
