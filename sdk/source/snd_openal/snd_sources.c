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

#ifdef __MACOSX__
#define MAX_SRC 64
#else
#define MAX_SRC 128
#endif
static src_t srclist[MAX_SRC];
static int src_count = 0;
static qboolean src_inited = qfalse;

typedef struct sentity_s
{
	src_t *src;
	int touched;    // Sound present this update?
} sentity_t;
static sentity_t *entlist = NULL; //[MAX_EDICTS];

/*
* source_setup
*/
static void source_setup( src_t *src, sfx_t *sfx, int priority, int entNum, int channel, float fvol, float attenuation )
{
	ALuint buffer;

	// Mark the SFX as used, and grab the raw AL buffer
	S_UseBuffer( sfx );
	buffer = S_GetALBuffer( sfx );

	clamp_low( attenuation, 0.0f );

	src->lastUse = trap_Milliseconds();
	src->sfx = sfx;
	src->priority = priority;
	src->entNum = entNum;
	src->channel = channel;
	src->fvol = fvol;
	src->attenuation = attenuation;
	src->isActive = qtrue;
	src->isLocked = qfalse;
	src->isLooping = qfalse;
	src->isTracking = qfalse;
	VectorClear( src->origin );
	VectorClear( src->velocity );

	qalSourcefv( src->source, AL_POSITION, vec3_origin );
	qalSourcefv( src->source, AL_VELOCITY, vec3_origin );
	qalSourcef( src->source, AL_GAIN, fvol * s_volume->value );
	qalSourcei( src->source, AL_SOURCE_RELATIVE, AL_FALSE );
	qalSourcei( src->source, AL_LOOPING, AL_FALSE );
	qalSourcei( src->source, AL_BUFFER, buffer );

	qalSourcef( src->source, AL_REFERENCE_DISTANCE, s_attenuation_refdistance );
	qalSourcef( src->source, AL_MAX_DISTANCE, s_attenuation_maxdistance );
	qalSourcef( src->source, AL_ROLLOFF_FACTOR, attenuation );
}

/*
* source_kill
*/
static void source_kill( src_t *src )
{
	if( src->isLocked )
		return;

	if( src->isActive )
		qalSourceStop( src->source );

	qalSourcei( src->source, AL_BUFFER, AL_NONE );

	src->sfx = 0;
	src->lastUse = 0;
	src->priority = 0;
	src->entNum = -1;
	src->channel = -1;
	src->fvol = 1;
	src->isActive = qfalse;
	src->isLocked = qfalse;
	src->isLooping = qfalse;
	src->isTracking = qfalse;
}

/*
* source_spatialize
*/
static void source_spatialize( src_t *src )
{
	if( !src->attenuation )
	{	
		qalSourcei( src->source, AL_SOURCE_RELATIVE, AL_TRUE );
		// this was set at source_setup, no need to redo every frame
		//qalSourcefv( src->source, AL_POSITION, vec3_origin );
		//qalSourcefv( src->source, AL_VELOCITY, vec3_origin );
		return;
	}

	if( src->isTracking )
		trap_GetEntitySpatilization( src->entNum, src->origin, src->velocity );

	qalSourcei( src->source, AL_SOURCE_RELATIVE, AL_FALSE );
	qalSourcefv( src->source, AL_POSITION, src->origin );
	qalSourcefv( src->source, AL_VELOCITY, src->velocity );
}

/*
* source_loop
*/
static void source_loop( int priority, sfx_t *sfx, int entNum, float fvol, float attenuation )
{
	src_t *src;
	qboolean new_source = qfalse;

	if( !sfx )
		return;

	if( entNum < 0 )
		return;

	// Do we need to start a new sound playing?
	if( !entlist[entNum].src )
	{
		src = S_AllocSource( priority, entNum, 0 );
		if( !src )
			return;
		new_source = qtrue;
	}
	else if( entlist[entNum].src->sfx != sfx )
	{
		// Need to restart. Just re-use this channel
		src = entlist[entNum].src;
		source_kill( src );
		new_source = qtrue;
	}
	else
	{
		src = entlist[entNum].src;
	}

	if( new_source )
	{
		source_setup( src, sfx, priority, entNum, -1, fvol, attenuation );
		qalSourcei( src->source, AL_LOOPING, AL_TRUE );
		src->isLooping = qtrue;

		entlist[entNum].src = src;
	}

	qalSourcef( src->source, AL_GAIN, src->fvol * s_volume->value );

	qalSourcef( src->source, AL_REFERENCE_DISTANCE, s_attenuation_refdistance );
	qalSourcef( src->source, AL_MAX_DISTANCE, s_attenuation_maxdistance );
	qalSourcef( src->source, AL_ROLLOFF_FACTOR, attenuation );

	if( new_source )
	{
		if( src->attenuation )
			src->isTracking = qtrue;	

		source_spatialize( src );

		qalSourcePlay( src->source );
	}

	entlist[entNum].touched = qtrue;
}

/*
* S_InitSources
*/
qboolean S_InitSources( int maxEntities, qboolean verbose )
{
	int i;

	memset( srclist, 0, sizeof( srclist ) );
	src_count = 0;

	// Allocate as many sources as possible
	for( i = 0; i < MAX_SRC; i++ )
	{
		qalGenSources( 1, &srclist[i].source );
		if( qalGetError() != AL_NO_ERROR )
			break;
		src_count++;
	}
	if( !src_count )
		return qfalse;

	if( verbose )
		Com_Printf( "allocated %d sources\n", src_count );

	if( maxEntities < 1 )
		return qfalse;

	entlist = ( sentity_t * )S_Malloc( sizeof( sentity_t ) * maxEntities );

	src_inited = qtrue;
	return qtrue;
}

/*
* S_ShutdownSources
*/
void S_ShutdownSources( void )
{
	int i;

	if( !src_inited )
		return;

	// Destroy all the sources
	for( i = 0; i < src_count; i++ )
	{
		qalSourceStop( srclist[i].source );
		qalDeleteSources( 1, &srclist[i].source );
	}

	memset( srclist, 0, sizeof( srclist ) );

	S_Free( entlist );
	entlist = NULL;

	src_inited = qfalse;
}

/*
* S_UpdateSources
*/
void S_UpdateSources( void )
{
	int i, entNum;
	ALint state;

	for( i = 0; i < src_count; i++ )
	{
		if( srclist[i].isLocked )
			continue;
		if( !srclist[i].isActive )
			continue;

		if( s_volume->modified )
			qalSourcef( srclist[i].source, AL_GAIN, srclist[i].fvol * s_volume->value );

		// Check if it's done, and flag it
		qalGetSourcei( srclist[i].source, AL_SOURCE_STATE, &state );
		if( state == AL_STOPPED )
		{
			source_kill( &srclist[i] );
			continue;
		}

		entNum = srclist[i].entNum;

		if( srclist[i].isLooping )
		{
			// If a looping effect hasn't been touched this frame, kill it
			if( !entlist[entNum].touched )
			{
				source_kill( &srclist[i] );
				entlist[entNum].src = NULL;
			}
			else
			{
				entlist[entNum].touched = qfalse;
			}
		}

		source_spatialize( &srclist[i] );
	}
}

/*
* S_AllocSource
*/
src_t *S_AllocSource( int priority, int entNum, int channel )
{
	int i;
	int empty = -1;
	int weakest = -1;
	int weakest_time = trap_Milliseconds();
	int weakest_priority = priority;

	for( i = 0; i < src_count; i++ )
	{
		if( srclist[i].isLocked )
			continue;

		if( !srclist[i].isActive && ( empty == -1 ) )
			empty = i;

		if( srclist[i].priority < weakest_priority ||
			( srclist[i].priority == weakest_priority && srclist[i].lastUse < weakest_time ) )
		{
			weakest_priority = srclist[i].priority;
			weakest_time = srclist[i].lastUse;
			weakest = i;
		}

		// Is it an exact match, and not on channel 0?
		if( ( srclist[i].entNum == entNum ) && ( srclist[i].channel == channel ) && ( channel != 0 ) )
		{
			source_kill( &srclist[i] );
			return &srclist[i];
		}
	}

	if( empty != -1 )
	{
		return &srclist[empty];
	}

	if( weakest != -1 )
	{
		source_kill( &srclist[weakest] );
		return &srclist[weakest];
	}

	return NULL;
}

/*
* S_LockSource
*/
void S_LockSource( src_t *src )
{
	src->isLocked = qtrue;
}

/*
* S_UnlockSource
*/
void S_UnlockSource( src_t *src )
{
	src->isLocked = qfalse;
}

/*
* S_GetALSource
*/
ALuint S_GetALSource( const src_t *src )
{
	return src->source;
}

/*
* S_StartLocalSound
*/
void S_StartLocalSound( const char *name )
{
	sfx_t *sfx;
	src_t *src;

	src = S_AllocSource( SRCPRI_LOCAL, -1, CHAN_AUTO );
	if( !src )
		return;

	sfx = S_RegisterSound( name );
	if( !sfx )
		return;

	source_setup( src, sfx, SRCPRI_LOCAL, -1, CHAN_AUTO, 1.0, ATTN_NONE );
	qalSourcei( src->source, AL_SOURCE_RELATIVE, AL_TRUE );

	qalSourcePlay( src->source );
}

/*
* S_StartSound
*/
static void S_StartSound( sfx_t *sfx, const vec3_t origin, int entNum, int channel, float fvol, float attenuation )
{
	src_t *src;

	if( !sfx )
		return;

	src = S_AllocSource( SRCPRI_ONESHOT, entNum, channel );
	if( !src )
		return;

	source_setup( src, sfx, SRCPRI_ONESHOT, entNum, channel, fvol, attenuation );

	if( src->attenuation )
	{
		if( origin )
			VectorCopy( origin, src->origin );
		else
			src->isTracking = qtrue;	
	}

	source_spatialize( src );

	qalSourcePlay( src->source );
}

/*
* S_StartFixedSound
*/
void S_StartFixedSound( sfx_t *sfx, const vec3_t origin, int channel, float fvol, float attenuation )
{
	S_StartSound( sfx, origin, 0, channel, fvol, attenuation );
}

/*
* S_StartRelativeSound
*/
void S_StartRelativeSound( sfx_t *sfx, int entnum, int channel, float fvol, float attenuation )
{
	S_StartSound( sfx, NULL, entnum, channel, fvol, attenuation );
}

/*
* S_StartGlobalSound
*/
void S_StartGlobalSound( sfx_t *sfx, int channel, float fvol )
{
	S_StartSound( sfx, NULL, 0, channel, fvol, ATTN_NONE );
}

/*
* S_AddLoopSound
*/
void S_AddLoopSound( sfx_t *sfx, int entnum, float fvol, float attenuation )
{
	source_loop( SRCPRI_LOOP, sfx, entnum, fvol, attenuation );
}

/*
* S_StopAllSources
*/
void S_StopAllSources( void )
{
	int i;

	for( i = 0; i < src_count; i++ )
		source_kill( &srclist[i] );
}
