/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2005 Stuart Dalton (badcdev@gmail.com)

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

/*
* snd_decoder.h
* Sound decoder management
* It only decodes things, so technically it's not a COdec, but who cares?
*
* 2005-08-31
*  Started
*/

#include "snd_local.h"

// Codec functions
typedef void *( *DECODER_LOAD )( const char *filename, snd_info_t *info );
typedef snd_stream_t *( *DECODER_OPEN )( const char *filename );
typedef int ( *DECODER_READ )( snd_stream_t *stream, int bytes, void *buffer, qboolean loop );
typedef void ( *DECODER_RESTART )( snd_stream_t *stream );
typedef void ( *DECODER_CLOSE )( snd_stream_t *stream );

// Codec data structure
struct snd_decoder_s
{
	char *ext;
	DECODER_LOAD load;
	DECODER_OPEN open;
	DECODER_READ read;
	DECODER_CLOSE close;
	snd_decoder_t *next;
};

/**
* Util functions used by decoders
*/
snd_stream_t *decoder_stream_init( snd_decoder_t *decoder );
void decoder_stream_shutdown( snd_stream_t *stream );

/**
* WAV Codec
*/
extern snd_decoder_t wav_decoder;
void *decoder_wav_load( const char *filename, snd_info_t *info );
snd_stream_t *decoder_wav_open( const char *filename );
int decoder_wav_read( snd_stream_t *stream, int bytes, void *buffer, qboolean loop );
void decoder_wav_close( snd_stream_t *stream );

/**
* Ogg Vorbis decoder
*/
extern snd_decoder_t ogg_decoder;
void *decoder_ogg_load( const char *filename, snd_info_t *info );
snd_stream_t *decoder_ogg_open( const char *filename );
int decoder_ogg_read( snd_stream_t *stream, int bytes, void *buffer, qboolean loop );
void decoder_ogg_close( snd_stream_t *stream );
qboolean SNDOGG_Init( qboolean verbose );
void SNDOGG_Shutdown( qboolean verbose );
