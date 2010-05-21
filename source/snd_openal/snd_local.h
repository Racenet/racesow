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
// snd_local.h -- private OpenAL sound functions

#define OPENAL_RUNTIME
//#define VORBISLIB_RUNTIME // enable this define for dynamic linked vorbis libraries

// it's in qcommon.h too, but we don't include it for modules
typedef struct { char *name; void **funcPointer; } dllfunc_t;

#include "../gameshared/q_arch.h"
#include "../gameshared/q_math.h"
#include "../gameshared/q_shared.h"
#include "../gameshared/q_cvar.h"

#include "../client/snd_public.h"
#include "snd_syscalls.h"

#include "qal.h"

extern struct mempool_s *soundpool;

#define S_MemAlloc( pool, size ) trap_MemAlloc( pool, size, __FILE__, __LINE__ )
#define S_MemFree( mem ) trap_MemFree( mem, __FILE__, __LINE__ )
#define S_MemAllocPool( name ) trap_MemAllocPool( name, __FILE__, __LINE__ )
#define S_MemFreePool( pool ) trap_MemFreePool( pool, __FILE__, __LINE__ )
#define S_MemEmptyPool( pool ) trap_MemEmptyPool( pool, __FILE__, __LINE__ )

#define S_Malloc( size ) S_MemAlloc( soundpool, size )
#define S_Free( data ) S_MemFree( data )

typedef struct sfx_s
{
	char filename[MAX_QPATH];
	ALuint buffer;      // OpenAL buffer
	qboolean inMemory;
	qboolean isLocked;
	int used;           // Time last used
} sfx_t;

extern cvar_t *s_volume;
extern cvar_t *s_musicvolume;
extern cvar_t *s_sources;

extern int s_attenuation_model;
extern float s_attenuation_maxdistance;
extern float s_attenuation_refdistance;

#define SRCPRI_AMBIENT	0   // Ambient sound effects
#define SRCPRI_LOOP	1   // Looping (not ambient) sound effects
#define SRCPRI_ONESHOT	2   // One-shot sounds
#define SRCPRI_LOCAL	3   // Local sounds
#define SRCPRI_STREAM	4   // Streams (music, cutscenes)

/*
* Exported functions
*/
int S_API( void );
void S_Error( const char *format, ... );

qboolean S_Init( void *hwnd, int maxEntities, qboolean verbose );
void S_Shutdown( qboolean verbose );

void S_SoundsInMemory( void );
void S_FreeSounds( void );
void S_StopAllSounds( void );

void S_Clear( void );
void S_Update( const vec3_t origin, const vec3_t velocity, const vec3_t v_forward, const vec3_t v_right,
			  const vec3_t v_up, qboolean avidump );
void S_Activate( qboolean active );

void S_SetAttenuationModel( int model, float maxdistance, float refdistance );

// playing
struct sfx_s *S_RegisterSound( const char *sample );

void S_StartFixedSound( struct sfx_s *sfx, const vec3_t origin, int channel, float fvol, float attenuation );
void S_StartRelativeSound( struct sfx_s *sfx, int entnum, int channel, float fvol, float attenuation );
void S_StartGlobalSound( struct sfx_s *sfx, int channel, float fvol );

void S_StartLocalSound( const char *s );

void S_AddLoopSound( struct sfx_s *sfx, int entnum, float fvol, float attenuation );

// cinema
void S_RawSamples( int samples, int rate, int width, int channels, const qbyte *data, qboolean music );

// music
void S_StartBackgroundTrack( const char *intro, const char *loop );
void S_StopBackgroundTrack( void );

/*
* Util (snd_main.c)
*/
ALuint S_SoundFormat( int width, int channels );
const char *S_ErrorMessage( ALenum error );

/*
* Buffer management
*/
qboolean S_InitBuffers( void );
void S_ShutdownBuffers( void );
void S_SoundList( void );
void S_UseBuffer( sfx_t *sfx );
ALuint S_GetALBuffer( const sfx_t *sfx );

/*
* Source management
*/
typedef struct src_s
{
	ALuint source;
	sfx_t *sfx;

	int lastUse;    // Last time used
	int priority;
	int entNum;
	int channel;

	float fvol; // volume modifier, for s_volume updating
	float attenuation;

	qboolean isActive;
	qboolean isLocked;
	qboolean isLooping;
	qboolean isTracking;

	vec3_t origin, velocity; // for local culling
} src_t;

qboolean S_InitSources( int maxEntities, qboolean verbose );
void S_ShutdownSources( void );
void S_UpdateSources( void );
src_t *S_AllocSource( int priority, int entnum, int channel );
src_t *S_FindSource( int entnum, int channel );
void S_LockSource( src_t *src );
void S_UnlockSource( src_t *src );
void S_StopAllSources( void );
ALuint S_GetALSource( const src_t *src );

/*
* Music
*/
void S_UpdateMusic( void );

/*
* Stream
*/
void S_UpdateStream( void );
void S_StopStream( void );

/*
* Decoder
*/
typedef struct snd_info_s
{
	int rate;
	int width;
	int channels;
	int samples;
	int size;
} snd_info_t;

typedef struct snd_decoder_s snd_decoder_t;
typedef struct snd_stream_s
{
	snd_decoder_t *decoder;
	snd_info_t info; // TODO: Change to AL_FORMAT?
	void *ptr; // decoder specific stuff
} snd_stream_t;

qboolean S_InitDecoders( qboolean verbose );
void S_ShutdownDecoders( qboolean verbose );
void *S_LoadSound( const char *filename, snd_info_t *info );
snd_stream_t *S_OpenStream( const char *filename );
int S_ReadStream( snd_stream_t *stream, int bytes, void *buffer, qboolean loop );
void S_CloseStream( snd_stream_t *stream );

void S_BeginAviDemo( void );
void S_StopAviDemo( void );
