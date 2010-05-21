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

#include "snd_local.h"

#define MUSIC_BUFFERS		8
#define MUSIC_BUFFER_SIZE	8192

static qboolean music_playing = qfalse;
static qboolean loop_playing = qfalse;
static src_t *src = NULL;
static ALuint source;
static ALuint buffers[MUSIC_BUFFERS];

static snd_stream_t *music_stream;
static char s_backgroundLoop[MAX_QPATH];

static qbyte decode_buffer[MUSIC_BUFFER_SIZE];

/*
* Local helper functions
*/

static void music_source_get( void )
{
	// Allocate a source at high priority
	src = S_AllocSource( SRCPRI_STREAM, -2, 0 );
	if( !src )
		return;

	S_LockSource( src );
	source = S_GetALSource( src );

	qalSource3f( source, AL_POSITION, 0.0, 0.0, 0.0 );
	qalSource3f( source, AL_VELOCITY, 0.0, 0.0, 0.0 );
	qalSource3f( source, AL_DIRECTION, 0.0, 0.0, 0.0 );
	qalSourcef( source, AL_ROLLOFF_FACTOR, 0.0 );
	qalSourcei( source, AL_SOURCE_RELATIVE, AL_TRUE );
	qalSourcef( source, AL_GAIN, s_musicvolume->value );
}

static void music_source_free( void )
{
	S_UnlockSource( src );
	source = 0;
	src = NULL;
}

static qboolean music_process( ALuint b )
{
	int l = 0;
	ALuint format;
	ALenum error;

	if( !loop_playing ) // intro
	{
		l = S_ReadStream( music_stream, MUSIC_BUFFER_SIZE, decode_buffer, qfalse );

		if( !l )
		{
			S_CloseStream( music_stream );
			music_stream = S_OpenStream( s_backgroundLoop );
			if( !music_stream )
				return qfalse;
			loop_playing = qtrue;
		}
	}

	if( loop_playing )
	{
		l = S_ReadStream( music_stream, MUSIC_BUFFER_SIZE, decode_buffer, qtrue );

		if( !l )
			return qfalse;
	}

	format = S_SoundFormat( music_stream->info.width, music_stream->info.channels );
	qalBufferData( b, format, decode_buffer, l, music_stream->info.rate );
	if( ( error = qalGetError() ) != AL_NO_ERROR )
		return qfalse;

	return qtrue;
}

/*
* Sound system wide functions (snd_loc.h)
*/

void S_UpdateMusic( void )
{
	int processed;
	ALint state;
	ALenum error;
	ALuint b;

	if( !music_playing )
		return;

	qalGetSourcei( source, AL_BUFFERS_PROCESSED, &processed );
	while( processed-- )
	{
		qalSourceUnqueueBuffers( source, 1, &b );
		if( !music_process( b ) )
		{
			Com_Printf( "Error processing music data\n" );
			S_StopBackgroundTrack();
			return;
		}
		qalSourceQueueBuffers( source, 1, &b );
		if( ( error = qalGetError() ) != AL_NO_ERROR )
		{
			Com_Printf( "Couldn't queue music data (%s)\n", S_ErrorMessage( error ) );
			S_StopBackgroundTrack();
			return;
		}
	}

	// If it's not still playing, give it a kick
	qalGetSourcei( source, AL_SOURCE_STATE, &state );
	if( state == AL_STOPPED )
		qalSourcePlay( source );

	if( s_musicvolume->modified )
		qalSourcef( source, AL_GAIN, s_musicvolume->value );
}

/*
* Global functions (sound.h)
*/

void S_StartBackgroundTrack( const char *intro, const char *loop )
{
	int i;
	ALenum error;

	// Stop any existing music that might be playing
	S_StopBackgroundTrack();

	if( !intro || !intro[0] )
	{
		if( !loop || !loop[0] )
			return;
		intro = loop;
	}
	if( !loop || !loop[0] )
	{
		loop = intro;
	}

	Q_strncpyz( s_backgroundLoop, loop, sizeof( s_backgroundLoop ) );

	music_stream = S_OpenStream( intro );
	if( !music_stream )
		return;

	music_source_get();
	if( !src )
	{
		Com_Printf( "Error couldn't get source for music\n" );
		return;
	}

	qalGenBuffers( MUSIC_BUFFERS, buffers );
	if( ( error = qalGetError() ) != AL_NO_ERROR )
	{
		Com_Printf( "Error couldn't generate music buffers (%s)\n", S_ErrorMessage( error ) );
		music_source_free();
		return;
	}

	// Queue the buffers up
	for( i = 0; i < MUSIC_BUFFERS; i++ )
	{
		if( !music_process( buffers[i] ) )
		{
			Com_Printf( "Error processing music data\n" );
			qalDeleteBuffers( MUSIC_BUFFERS, buffers );
			music_source_free();
			return;
		}
	}

	qalSourceQueueBuffers( source, MUSIC_BUFFERS, buffers );
	if( ( error = qalGetError() ) != AL_NO_ERROR )
	{
		Com_Printf( "Couldn't queue music data (%s)\n", S_ErrorMessage( error ) );
		qalDeleteBuffers( MUSIC_BUFFERS, buffers );
		music_source_free();
		return;
	}

	qalSourcePlay( source );

	music_playing = qtrue;
	if( loop == intro )
		loop_playing = qtrue;
	else
		loop_playing = qfalse;
}

void S_StopBackgroundTrack( void )
{
	if( !music_playing )
		return;

	qalSourceStop( source );
	qalSourceUnqueueBuffers( source, MUSIC_BUFFERS, buffers );
	qalDeleteBuffers( MUSIC_BUFFERS, buffers );

	music_source_free();
	if( music_stream )
		S_CloseStream( music_stream );

	music_playing = qfalse;
}
