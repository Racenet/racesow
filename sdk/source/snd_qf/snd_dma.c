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
// snd_dma.c -- main control for any streaming sound output device


#include "snd_local.h"

// =======================================================================
// Internal sound data & structures
// =======================================================================

#define FORWARD 0
#define RIGHT   1
#define UP      2

channel_t channels[MAX_CHANNELS];

qboolean snd_initialized = qfalse;

dma_t dma;

vec3_t listenerOrigin;
vec3_t listenerVelocity;
vec3_t listenerAxis[3];

int soundtime;      // sample PAIRS
int paintedtime;    // sample PAIRS

// during registration it is possible to have more sounds
// than could actually be referenced during gameplay,
// because we don't want to free anything until we are
// sure we won't need it.
#define	    MAX_SFX	512
sfx_t known_sfx[MAX_SFX];
int num_sfx;

#define	    MAX_LOOPSFX	128
loopsfx_t loop_sfx[MAX_LOOPSFX];
int num_loopsfx;

#define	    MAX_PLAYSOUNDS  128
playsound_t s_playsounds[MAX_PLAYSOUNDS];
playsound_t s_freeplays;
playsound_t s_pendingplays;

cvar_t *developer;

cvar_t *s_volume;
cvar_t *s_musicvolume;
cvar_t *s_testsound;
cvar_t *s_khz;
cvar_t *s_show;
cvar_t *s_mixahead;
cvar_t *s_swapstereo;
cvar_t *s_vorbis;

static int s_attenuation_model = 0;
static float s_attenuation_maxdistance = 0;
static float s_attenuation_refdistance = 0;

struct mempool_s *soundpool;

int s_rawend;
portable_samplepair_t s_rawsamples[MAX_RAW_SAMPLES];

bgTrack_t *s_bgTrack;
bgTrack_t s_bgTrackIntro;
bgTrack_t s_bgTrackLoop;

static int s_aviNumSamples;
static int s_aviDumpFile;
static char *s_aviDumpFileName;

/*
===============================================================================

console functions

===============================================================================
*/

#ifdef ENABLE_PLAY
static void S_Play( void )
{
	int i;
	sfx_t *sfx;

	for( i = 1; i < trap_Cmd_Argc(); i++ )
	{
		sfx = S_RegisterSound( trap_Cmd_Argv( i ) );
		if( !sfx )
		{
			Com_Printf( "Couldn't play: %s\n", trap_Cmd_Argv( i ) );
			continue;
		}
		S_StartGlobalSound( sfx, CHAN_AUTO, 1.0 );
	}
}
#endif // ENABLE_PLAY

/*
* S_SoundList
*/
void S_SoundList( void )
{
	int i;
	sfx_t *sfx;
	sfxcache_t *sc;
	int size, total;

	total = 0;
	for( sfx = known_sfx, i = 0; i < num_sfx; i++, sfx++ )
	{
		if( !sfx->name[0] )
			continue;
		sc = sfx->cache;
		if( sc )
		{
			size = sc->length*sc->width*sc->channels;
			total += size;
			if( sc->loopstart >= 0 )
				Com_Printf( "L" );
			else
				Com_Printf( " " );
			Com_Printf( "(%2db) %6i : %s\n", sc->width*8, size, sfx->name );
		}
		else
		{
			if( sfx->name[0] == '*' )
				Com_Printf( "  placeholder : %s\n", sfx->name );
			else
				Com_Printf( "  not loaded  : %s\n", sfx->name );
		}
	}
	Com_Printf( "Total resident: %i\n", total );
}

/*
* S_Music
*/
static void S_Music( void )
{
	if( trap_Cmd_Argc() < 2 )
	{
		Com_Printf( "music: <introfile> [loopfile]\n" );
		return;
	}

	S_StartBackgroundTrack( trap_Cmd_Argv( 1 ), trap_Cmd_Argv( 2 ) );
}

// ====================================================================
// User-setable variables
// ====================================================================

/*
* S_SoundInfo_f
*/
static void S_SoundInfo_f( void )
{
	Com_Printf( "%5d stereo\n", dma.channels - 1 );
	Com_Printf( "%5d samples\n", dma.samples );
	Com_Printf( "%5d samplepos\n", dma.samplepos );
	Com_Printf( "%5d samplebits\n", dma.samplebits );
	Com_Printf( "%5d submission_chunk\n", dma.submission_chunk );
	Com_Printf( "%5d speed\n", dma.speed );
	Com_Printf( "0x%x dma buffer\n", dma.buffer );
}

/*
* S_Init
*/
qboolean S_Init( void *hwnd, int maxEntities, qboolean verbose )
{
	developer = trap_Cvar_Get( "developer", "0", 0 );

	s_volume = trap_Cvar_Get( "s_volume", "0.8", CVAR_ARCHIVE );
	s_musicvolume = trap_Cvar_Get( "s_musicvolume", "0.8", CVAR_ARCHIVE );
	s_khz = trap_Cvar_Get( "s_khz", "44", CVAR_ARCHIVE );
	s_mixahead = trap_Cvar_Get( "s_mixahead", "0.2", CVAR_ARCHIVE );
	s_show = trap_Cvar_Get( "s_show", "0", CVAR_CHEAT );
	s_testsound = trap_Cvar_Get( "s_testsound", "0", 0 );
	s_swapstereo = trap_Cvar_Get( "s_swapstereo", "0", CVAR_ARCHIVE );
	s_vorbis = trap_Cvar_Get( "s_vorbis", "1", CVAR_ARCHIVE );

#ifdef ENABLE_PLAY
	trap_Cmd_AddCommand( "play", S_Play );
#endif
	trap_Cmd_AddCommand( "music", S_Music );
	trap_Cmd_AddCommand( "stopsound", S_StopAllSounds );
	trap_Cmd_AddCommand( "stopmusic", S_StopBackgroundTrack );
	trap_Cmd_AddCommand( "soundlist", S_SoundList );
	trap_Cmd_AddCommand( "soundinfo", S_SoundInfo_f );

	if( !SNDDMA_Init( hwnd, verbose ) )
		return qfalse;

	SNDOGG_Init( verbose );

	S_InitScaletable();

	S_SetAttenuationModel( S_DEFAULT_ATTENUATION_MODEL, S_DEFAULT_ATTENUATION_MAXDISTANCE, S_DEFAULT_ATTENUATION_REFDISTANCE );

	num_sfx = 0;
	num_loopsfx = 0;

	S_ClearSoundTime();

	if( verbose )
		Com_Printf( "Sound sampling rate: %i\n", dma.speed );

	soundpool = S_MemAllocPool( "QF Sound Module" );

	S_StopAllSounds();

	return qtrue;
}


// =======================================================================
// Shutdown sound engine
// =======================================================================

/*
* S_Shutdown
*/
void S_Shutdown( qboolean verbose )
{
	S_StopAviDemo();

	// free all sounds
	S_FreeSounds();

	SNDDMA_Shutdown( verbose );
	SNDOGG_Shutdown( verbose );

#ifdef ENABLE_PLAY
	trap_Cmd_RemoveCommand( "play" );
#endif
	trap_Cmd_RemoveCommand( "music" );
	trap_Cmd_RemoveCommand( "stopsound" );
	trap_Cmd_RemoveCommand( "stopmusic" );
	trap_Cmd_RemoveCommand( "soundlist" );
	trap_Cmd_RemoveCommand( "soundinfo" );

	S_MemFreePool( &soundpool );

	num_sfx = 0;
	num_loopsfx = 0;
}


// =======================================================================
// Load a sound
// =======================================================================

/*
* S_FindName
*/
static sfx_t *S_FindName( const char *name, qboolean create )
{
	int i;
	sfx_t *sfx;

	if( !name )
		S_Error( "S_FindName: NULL" );
	if( !name[0] )
	{
		*( (int *)0 ) = -1;
		S_Error( "S_FindName: empty name" );
	}

	if( strlen( name ) >= MAX_QPATH )
		S_Error( "Sound name too long: %s", name );

	// see if already loaded
	for( i = 0; i < num_sfx; i++ )
	{
		if( !strcmp( known_sfx[i].name, name ) )
			return &known_sfx[i];
	}

	if( !create )
		return NULL;

	// find a free sfx
	for( i = 0; i < num_sfx; i++ )
	{
		if( !known_sfx[i].name[0] )
			break;
	}

	if( i == num_sfx )
	{
		if( num_sfx == MAX_SFX )
			S_Error( "S_FindName: out of sfx_t" );
		num_sfx++;
	}

	sfx = &known_sfx[i];
	memset( sfx, 0, sizeof( *sfx ) );
	Q_strncpyz( sfx->name, name, sizeof( sfx->name ) );

	return sfx;
}

/*
* S_RegisterSound
*/
sfx_t *S_RegisterSound( const char *name )
{
	sfx_t *sfx;

	assert( name );

	sfx = S_FindName( name, qtrue );
	S_LoadSound( sfx );

	return sfx;
}

/*
* S_FreeSounds
*/
void S_FreeSounds( void )
{
	int i;
	sfx_t *sfx;

	// free all sounds
	for( i = 0, sfx = known_sfx; i < num_sfx; i++, sfx++ )
	{
		if( !sfx->name[0] )
			continue;
		if( sfx->cache )
			S_Free( sfx->cache );
		memset( sfx, 0, sizeof( *sfx ) );
	}

	S_StopBackgroundTrack();
}

/*
* S_SoundsInMemory
*/
void S_SoundsInMemory( void )
{
	int i, size;
	sfx_t *sfx;

	// free any sounds not from this registration sequence
	for( i = 0, sfx = known_sfx; i < num_sfx; i++, sfx++ )
	{
		if( !sfx->name[0] )
			continue;

		// make sure it is paged in
		if( sfx->cache )
		{
			size = sfx->cache->length * sfx->cache->width;
			trap_PageInMemory( (qbyte *)sfx->cache, size );
		}
	}
}

/*
* S_ClearSoundTime
*/
void S_ClearSoundTime( void )
{
	soundtime = 0;
	paintedtime = 0;
}

//=============================================================================

/*
* S_PickChannel
*/
channel_t *S_PickChannel( int entnum, int entchannel )
{
	int ch_idx;
	int first_to_die;
	int life_left;
	channel_t *ch;

	if( entchannel < 0 )
		S_Error( "S_PickChannel: entchannel < 0" );

	// Check for replacement sound, or find the best one to replace
	first_to_die = -1;
	life_left = 0x7fffffff;
	for( ch_idx = 0; ch_idx < MAX_CHANNELS; ch_idx++ )
	{
		if( entchannel != 0 // channel 0 never overrides
			&& channels[ch_idx].entnum == entnum
			&& channels[ch_idx].entchannel == entchannel )
		{ // always override sound from same entity
			first_to_die = ch_idx;
			break;
		}

		// don't let monster sounds override player sounds
		// wsw: Medar: Disabled. Don't wanna use cl.playernum much in client code, because of chasecam and stuff
		//if (channels[ch_idx].entnum == cl.playernum+1 && entnum != cl.playernum+1 && channels[ch_idx].sfx)
		//	continue;

		if( channels[ch_idx].end - paintedtime < life_left )
		{
			life_left = channels[ch_idx].end - paintedtime;
			first_to_die = ch_idx;
		}
	}

	if( first_to_die == -1 )
		return NULL;

	ch = &channels[first_to_die];
	memset( ch, 0, sizeof( *ch ) );

	return ch;
}

/*
* S_SetAttenuationModel
*/
void S_SetAttenuationModel( int model, float maxdistance, float refdistance )
{
	s_attenuation_model = model;
	s_attenuation_maxdistance = maxdistance;
	s_attenuation_refdistance = refdistance;
}

/*
* S_GainForAttenuation
*/
static float S_GainForAttenuation( float dist, float attenuation )
{
	if( !attenuation )
		return 1.0f;
	return Q_GainForAttenuation( s_attenuation_model, s_attenuation_maxdistance, s_attenuation_refdistance, dist, attenuation );
}

/*
* S_SpatializeOrigin
*/
#define Q3STEREODIRECTION
static void S_SpatializeOrigin( vec3_t origin, float master_vol, float dist_mult, int *left_vol, int *right_vol )
{
	vec_t dot;
	vec_t dist;
	vec_t lscale, rscale, scale;
	vec3_t source_vec;

	// calculate stereo separation and distance attenuation
	VectorSubtract( origin, listenerOrigin, source_vec );

	dist = VectorNormalize( source_vec );

	if( dma.channels == 1 || !dist_mult )
	{ // no attenuation = no spatialization
		rscale = 1.0f;
		lscale = 1.0f;
	}
	else
	{
#ifdef Q3STEREODIRECTION
		vec3_t vec;
		Matrix_TransformVector( listenerAxis, source_vec, vec );
		dot = vec[1];
#else
		dot = DotProduct( source_vec, listenerAxis[RIGHT] );
#endif
		rscale = 0.5 * ( 1.0 + dot );
		lscale = 0.5 * ( 1.0 - dot );
		if( rscale < 0 )
		{
			rscale = 0;
		}
		if( lscale < 0 )
		{
			lscale = 0;
		}
	}

	dist = S_GainForAttenuation( dist, dist_mult );

	// add in distance effect
	scale = dist * rscale;
	*right_vol = (int) ( master_vol * scale );
	if( *right_vol < 0 )
		*right_vol = 0;

	scale = dist * lscale;
	*left_vol = (int) ( master_vol * scale );
	if( *left_vol < 0 )
		*left_vol = 0;
}

/*
* S_Spatialize
*/
void S_Spatialize( channel_t *ch )
{
	vec3_t origin, velocity;

	if( ch->fixed_origin )
		VectorCopy( ch->origin, origin );
	else
		trap_GetEntitySpatilization( ch->entnum, origin, velocity );

	S_SpatializeOrigin( origin, ch->master_vol, ch->dist_mult, &ch->leftvol, &ch->rightvol );
}


/*
* S_AllocPlaysound
*/
static playsound_t *S_AllocPlaysound( void )
{
	playsound_t *ps;

	ps = s_freeplays.next;
	if( ps == &s_freeplays )
		return NULL; // no free playsounds

	// unlink from freelist
	ps->prev->next = ps->next;
	ps->next->prev = ps->prev;

	return ps;
}

/*
* S_FreePlaysound
*/
static void S_FreePlaysound( playsound_t *ps )
{
	// unlink from channel
	ps->prev->next = ps->next;
	ps->next->prev = ps->prev;

	// add to free list
	ps->next = s_freeplays.next;
	s_freeplays.next->prev = ps;
	ps->prev = &s_freeplays;
	s_freeplays.next = ps;
}

/*
* S_IssuePlaysound
* Only called by the update loop
*/
void S_IssuePlaysound( playsound_t *ps )
{
	channel_t *ch;
	sfxcache_t *sc;

	if( s_show->integer )
		Com_Printf( "Issue %i\n", ps->begin );
	// pick a channel to play on
	ch = S_PickChannel( ps->entnum, ps->entchannel );
	if( !ch )
	{
		S_FreePlaysound( ps );
		return;
	}
	sc = S_LoadSound( ps->sfx );
	if( !sc )
	{
		S_FreePlaysound( ps );
		return;
	}

	// spatialize
	ch->dist_mult = ps->attenuation;
	ch->master_vol = ps->volume;
	ch->entnum = ps->entnum;
	ch->entchannel = ps->entchannel;
	ch->sfx = ps->sfx;
	VectorCopy( ps->origin, ch->origin );
	ch->fixed_origin = ps->fixed_origin;

	S_Spatialize( ch );

	ch->pos = 0;
	ch->end = paintedtime + sc->length;

	// free the playsound
	S_FreePlaysound( ps );
}

/*
* S_ClearPlaysounds
*/
void S_ClearPlaysounds( void )
{
	int i;

	memset( s_playsounds, 0, sizeof( s_playsounds ) );
	s_freeplays.next = s_freeplays.prev = &s_freeplays;
	s_pendingplays.next = s_pendingplays.prev = &s_pendingplays;

	for( i = 0; i < MAX_PLAYSOUNDS; i++ )
	{
		s_playsounds[i].prev = &s_freeplays;
		s_playsounds[i].next = s_freeplays.next;
		s_playsounds[i].prev->next = &s_playsounds[i];
		s_playsounds[i].next->prev = &s_playsounds[i];
	}

	memset( channels, 0, sizeof( channels ) );
}

// =======================================================================
// Start a sound effect
// =======================================================================

/*
* S_StartSound
*/
static void S_StartSound( sfx_t *sfx, const vec3_t origin, int entnum, int entchannel, float fvol, float attenuation )
{
	sfxcache_t *sc;
	int vol;
	playsound_t *ps, *sort;

	if( !sfx )
		return;

	// make sure the sound is loaded
	sc = S_LoadSound( sfx );
	if( !sc )
		return; // couldn't load the sound's data

	vol = fvol*255;

	// make the playsound_t
	ps = S_AllocPlaysound();
	if( !ps )
		return;

	if( origin )
	{
		VectorCopy( origin, ps->origin );
		ps->fixed_origin = qtrue;
	}
	else
		ps->fixed_origin = qfalse;

	ps->entnum = entnum;
	ps->entchannel = entchannel;
	ps->attenuation = attenuation;
	ps->volume = vol;
	ps->sfx = sfx;

	ps->begin = paintedtime;

	// sort into the pending sound list
	for( sort = s_pendingplays.next;
		sort != &s_pendingplays && sort->begin < ps->begin;
		sort = sort->next )
		;

	ps->next = sort;
	ps->prev = sort->prev;

	ps->next->prev = ps;
	ps->prev->next = ps;
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
* S_StartLocalSound
*/
void S_StartLocalSound( const char *sound )
{
	sfx_t *sfx;

	sfx = S_RegisterSound( sound );
	if( !sfx )
	{
		Com_Printf( "S_StartLocalSound: can't cache %s\n", sound );
		return;
	}

	S_StartGlobalSound( sfx, CHAN_AUTO, 1 );
}

/*
* S_Clear
*/
void S_Clear( void )
{
	int clear;

	s_rawend = 0;

	if( dma.samplebits == 8 )
		clear = 0x80;
	else
		clear = 0;

	SNDDMA_BeginPainting();
	if( dma.buffer )
		memset( dma.buffer, clear, dma.samples * dma.samplebits/8 );
	SNDDMA_Submit();
}

/*
* S_StopAllSounds
*/
void S_StopAllSounds( void )
{
	// clear all the playsounds and channels
	S_ClearPlaysounds();

	S_Clear();

	S_StopBackgroundTrack();
}

/*
* S_AddLoopSound
*/
void S_AddLoopSound( sfx_t *sfx, int entnum, float fvol, float attenuation )
{
	if( !sfx || num_loopsfx >= MAX_LOOPSFX )
		return;

	loop_sfx[num_loopsfx].sfx = sfx;
	loop_sfx[num_loopsfx].volume = 255.0 * fvol;
	loop_sfx[num_loopsfx].attenuation = attenuation;

	trap_GetEntitySpatilization( entnum, loop_sfx[num_loopsfx].origin, NULL );

	num_loopsfx++;
}

/*
* S_AddLoopSounds
*/
static void S_AddLoopSounds( void )
{
	int i, j;
	int left, right, left_total, right_total;
	channel_t *ch;
	sfx_t *sfx;
	sfxcache_t *sc;

	for( i = 0; i < num_loopsfx; i++ )
	{
		if( !loop_sfx[i].sfx )
			continue;

		sfx = loop_sfx[i].sfx;
		sc = sfx->cache;
		if( !sc )
			continue;

		// find the total contribution of all sounds of this type
		if( loop_sfx[i].attenuation )
		{
			S_SpatializeOrigin( loop_sfx[i].origin, loop_sfx[i].volume, loop_sfx[i].attenuation, &left_total, &right_total );
			for( j = i+1; j < num_loopsfx; j++ )
			{
				if( loop_sfx[j].sfx != loop_sfx[i].sfx )
					continue;
				loop_sfx[j].sfx = NULL; // don't check this again later

				S_SpatializeOrigin( loop_sfx[j].origin, loop_sfx[i].volume, loop_sfx[i].attenuation, &left, &right );
				left_total += left;
				right_total += right;
			}

			if( left_total == 0 && right_total == 0 )
				continue; // not audible
		}
		else
		{
			left_total = loop_sfx[i].volume;
			right_total = loop_sfx[i].volume;
		}

		// allocate a channel
		ch = S_PickChannel( 0, 0 );
		if( !ch )
			return;

		if( left_total > 255 )
			left_total = 255;
		if( right_total > 255 )
			right_total = 255;
		ch->leftvol = left_total;
		ch->rightvol = right_total;
		ch->autosound = qtrue; // remove next frame
		ch->sfx = sfx;
		ch->pos = paintedtime % sc->length;
		ch->end = paintedtime + sc->length - ch->pos;
	}

	num_loopsfx = 0;
}

//=============================================================================

/*
* S_RawSamples
*/
void S_RawSamples( int samples, int rate, int width, int channels, const qbyte *data, qboolean music )
{
	int snd_vol;
	unsigned src, dst;
	unsigned fracstep, samplefrac;

	snd_vol = (int)( ( music ? s_musicvolume->value : s_volume->value ) * 256 );
	if( snd_vol < 0 )
		snd_vol = 0;

	if( s_rawend < paintedtime )
		s_rawend = paintedtime;

	fracstep = ( (unsigned)rate << 8 ) / (unsigned)dma.speed;
	samplefrac = 0;

	if( width == 2 )
	{
		short *in = (short *)data;

		if( channels == 2 )
		{
			for( src = 0; src < (unsigned)samples; samplefrac += fracstep, src = ( samplefrac >> 8 ) )
			{
				dst = s_rawend++ & ( MAX_RAW_SAMPLES - 1 );
				s_rawsamples[dst].left = in[src*2] * snd_vol;
				s_rawsamples[dst].right = in[src*2+1] * snd_vol;
			}
		}
		else
		{
			for( src = 0; src < (unsigned)samples; samplefrac += fracstep, src = ( samplefrac >> 8 ) )
			{
				dst = s_rawend++ & ( MAX_RAW_SAMPLES - 1 );
				s_rawsamples[dst].left = s_rawsamples[dst].right = in[src] * snd_vol;
			}
		}
	}
	else
	{
		if( channels == 2 )
		{
			char *in = (char *)data;

			for( src = 0; src < (unsigned)samples; samplefrac += fracstep, src = ( samplefrac >> 8 ) )
			{
				dst = s_rawend++ & ( MAX_RAW_SAMPLES - 1 );
				s_rawsamples[dst].left = in[src*2] << 8 * snd_vol;
				s_rawsamples[dst].right = in[src*2+1] << 8 * snd_vol;
			}
		}
		else
		{
			for( src = 0; src < (unsigned)samples; samplefrac += fracstep, src = ( samplefrac >> 8 ) )
			{
				dst = s_rawend++ & ( MAX_RAW_SAMPLES - 1 );
				s_rawsamples[dst].left = s_rawsamples[dst].right = ( data[src] - 128 ) << 8 * snd_vol;
			}
		}
	}
}

//=============================================================================

/*
* S_BackgroundTrack_FindNextChunk
*/
static qboolean S_BackgroundTrack_FindNextChunk( char *name, int *last_chunk, int file )
{
	char chunkName[4];
	int iff_chunk_len;

	while( 1 )
	{
		trap_FS_Seek( file, *last_chunk, FS_SEEK_SET );

		if( trap_FS_Eof( file ) )
			return qfalse; // didn't find the chunk

		trap_FS_Seek( file, 4, FS_SEEK_CUR );
		trap_FS_Read( &iff_chunk_len, sizeof( iff_chunk_len ), file );
		iff_chunk_len = LittleLong( iff_chunk_len );
		if( iff_chunk_len < 0 )
			return qfalse; // didn't find the chunk

		trap_FS_Seek( file, -8, FS_SEEK_CUR );
		*last_chunk = trap_FS_Tell( file ) + 8 + ( ( iff_chunk_len + 1 ) & ~1 );
		trap_FS_Read( chunkName, 4, file );
		if( !strncmp( chunkName, name, 4 ) )
			return qtrue;
	}
}

/*
* S_BackgroundTrack_GetWavinfo
*/
static int S_BackgroundTrack_GetWavinfo( const char *name, wavinfo_t *info )
{
	short t;
	int samples, file;
	int iff_data, last_chunk;
	char chunkName[4];

	last_chunk = 0;
	memset( info, 0, sizeof( wavinfo_t ) );

	if( trap_FS_FOpenFile( name, &file, FS_READ ) == -1 )
		return 0;

	// find "RIFF" chunk
	if( !S_BackgroundTrack_FindNextChunk( "RIFF", &last_chunk, file ) )
	{
		Com_Printf( "Missing RIFF chunk\n" );
		return 0;
	}

	trap_FS_Read( chunkName, 4, file );
	if( !strncmp( chunkName, "WAVE", 4 ) )
	{
		Com_Printf( "Missing WAVE chunk\n" );
		return 0;
	}

	// get "fmt " chunk
	iff_data = trap_FS_Tell( file ) + 4;
	last_chunk = iff_data;
	if( !S_BackgroundTrack_FindNextChunk( "fmt ", &last_chunk, file ) )
	{
		Com_Printf( "Missing fmt chunk\n" );
		return 0;
	}

	trap_FS_Read( chunkName, 4, file );

	trap_FS_Read( &t, sizeof( t ), file );
	if( LittleShort( t ) != 1 )
	{
		Com_Printf( "Microsoft PCM format only\n" );
		return 0;
	}

	trap_FS_Read( &t, sizeof( t ), file );
	info->channels = LittleShort( t );

	trap_FS_Read( &info->rate, sizeof( info->rate ), file );
	info->rate = LittleLong( info->rate );

	trap_FS_Seek( file, 4 + 2, FS_SEEK_CUR );

	trap_FS_Read( &t, sizeof( t ), file );
	info->width = LittleShort( t ) / 8;

	info->loopstart = 0;

	// find data chunk
	last_chunk = iff_data;
	if( !S_BackgroundTrack_FindNextChunk( "data", &last_chunk, file ) )
	{
		Com_Printf( "Missing data chunk\n" );
		return 0;
	}

	trap_FS_Read( &samples, sizeof( samples ), file );
	info->samples = LittleLong( samples ) / info->width / info->channels;

	info->dataofs = trap_FS_Tell( file );

	return file;
}

/*
* S_StartBackgroundTrack
*/
void S_StartBackgroundTrack( const char *intro, const char *loop )
{
	S_StopBackgroundTrack();

	if( !intro || !intro[0] )
		return;

	if( !SNDOGG_OpenTrack( intro, &s_bgTrackIntro ) )
	{
		s_bgTrackIntro.file = S_BackgroundTrack_GetWavinfo( intro, &s_bgTrackIntro.info );

		if( !s_bgTrackIntro.file || !s_bgTrackIntro.info.samples )
			return;
	}

	if( loop && loop[0] && Q_stricmp( intro, loop ) )
	{
		if( !SNDOGG_OpenTrack( loop, &s_bgTrackLoop ) )
			s_bgTrackLoop.file = S_BackgroundTrack_GetWavinfo( loop, &s_bgTrackLoop.info );
	}

	if( !s_bgTrackLoop.file || !s_bgTrackLoop.info.samples )
		s_bgTrackLoop = s_bgTrackIntro;
	s_bgTrack = &s_bgTrackIntro;
}

/*
* S_StopBackgroundTrack
*/
void S_StopBackgroundTrack( void )
{
	if( !s_bgTrack )
		return;

	if( s_bgTrackIntro.file != s_bgTrackLoop.file )
	{
		if( s_bgTrackIntro.close )
			s_bgTrackIntro.close( &s_bgTrackIntro );
		else
			trap_FS_FCloseFile( s_bgTrackIntro.file );
	}

	if( s_bgTrackLoop.close )
		s_bgTrackLoop.close( &s_bgTrackLoop );
	else
		trap_FS_FCloseFile( s_bgTrackLoop.file );

	s_bgTrack = NULL;
	memset( &s_bgTrackIntro, 0, sizeof( bgTrack_t ) );
	memset( &s_bgTrackLoop, 0, sizeof( bgTrack_t ) );
}

//=============================================================================

/*
* S_StopBackgroundTrack
* Medar: untested
*/
static void byteSwapRawSamples( int samples, int width, int channels, const qbyte *data )
{
	int i;

	if( LittleShort( 256 ) == 256 )
		return;

	if( width != 2 )
		return;

	if( channels == 2 )
		samples <<= 1;

	for( i = 0; i < samples; i++ )
		( (short *)data )[i] = BigShort( ( (short *)data )[i] );
}

/*
* S_UpdateBackgroundTrack
*/
static void S_UpdateBackgroundTrack( void )
{
	int samples, maxSamples;
	int read, maxRead, total;
	float scale;
	qbyte data[MAX_RAW_SAMPLES*4];

	if( !s_bgTrack || !s_musicvolume->value )
		return;

	if( s_rawend < paintedtime )
		s_rawend = paintedtime;
	scale = (float)s_bgTrack->info.rate / dma.speed;
	maxSamples = sizeof( data ) / s_bgTrack->info.channels / s_bgTrack->info.width;

	while( 1 )
	{
		samples = ( paintedtime + MAX_RAW_SAMPLES - s_rawend ) * scale;
		if( samples <= 0 )
			return;
		if( samples > maxSamples )
			samples = maxSamples;
		maxRead = samples * s_bgTrack->info.channels * s_bgTrack->info.width;

		total = 0;
		while( total < maxRead )
		{
			if( s_bgTrack->read )
				read = s_bgTrack->read( s_bgTrack, data + total, maxRead - total );
			else
				read = trap_FS_Read( data + total, maxRead - total, s_bgTrack->file );

			if( !read )
			{
				if( s_bgTrackIntro.file != s_bgTrackLoop.file )
				{
					if( s_bgTrackIntro.close )
						s_bgTrackIntro.close( &s_bgTrackIntro );
					else
						trap_FS_FCloseFile( s_bgTrackIntro.file );
					s_bgTrackIntro = s_bgTrackLoop;
				}

				s_bgTrack = &s_bgTrackLoop;
				if( s_bgTrack->seek )
					s_bgTrack->seek( s_bgTrack, s_bgTrack->info.dataofs );
				else
					trap_FS_Seek( s_bgTrack->file, s_bgTrack->info.dataofs, FS_SEEK_SET );
			}

			total += read;
		}

		byteSwapRawSamples( samples, s_bgTrack->info.width, s_bgTrack->info.channels, data );
		S_RawSamples( samples, s_bgTrack->info.rate, s_bgTrack->info.width, s_bgTrack->info.channels, data, qtrue );
	}
}

/*
* GetSoundtime
*/
static void GetSoundtime( void )
{
	int samplepos;
	static int buffers;
	static int oldsamplepos;
	int fullsamples;

	fullsamples = dma.samples / dma.channels;

	// it is possible to miscount buffers if it has wrapped twice between
	// calls to S_Update.  Oh well.
	samplepos = SNDDMA_GetDMAPos();

	if( samplepos < oldsamplepos )
	{
		buffers++;          // buffer wrapped

		if( paintedtime > 0x40000000 )
		{
			// time to chop things off to avoid 32 bit limits
			buffers = 0;
			paintedtime = fullsamples;
			S_StopAllSounds();
		}
	}
	oldsamplepos = samplepos;

	soundtime = buffers * fullsamples + samplepos / dma.channels;
}

/*
* S_Update_
*/
static void S_Update_( qboolean avidump )
{
	unsigned endtime;
	int samps;

	SNDDMA_BeginPainting();

	if( !dma.buffer )
		return;

	// Updates DMA time
	GetSoundtime();

	// check to make sure that we haven't overshot
	if( paintedtime < soundtime )
	{
		//Com_DPrintf( "S_Update_ : overflow\n" );
		paintedtime = soundtime;
	}

	// mix ahead of current position
	endtime = soundtime + s_mixahead->value * dma.speed;

	// mix to an even submission block size
	endtime = ( endtime + dma.submission_chunk-1 ) & ~( dma.submission_chunk-1 );
	samps = dma.samples >> ( dma.channels-1 );
	if( (int)( endtime - soundtime ) > samps )
		endtime = soundtime + samps;

	if( avidump && s_aviDumpFile )
		s_aviNumSamples += S_PaintChannels( endtime, s_aviDumpFile );
	else
		S_PaintChannels( endtime, 0 );

	SNDDMA_Submit();
}

/*
* S_Update
*/
void S_Update( const vec3_t origin, const vec3_t velocity, const vec3_t forward, const vec3_t right, const vec3_t up, qboolean avidump )
{
	int i;
	int total;
	channel_t *ch;
	channel_t *combine;

	// rebuild scale tables if volume is modified
	if( s_volume->modified )
		S_InitScaletable();

	VectorCopy( origin, listenerOrigin );
	VectorCopy( velocity, listenerVelocity );
	VectorCopy( forward, listenerAxis[FORWARD] );
	VectorCopy( right, listenerAxis[RIGHT] );
	VectorCopy( up, listenerAxis[UP] );

	combine = NULL;

	// update spatialization for dynamic sounds
	ch = channels;
	for( i = 0; i < MAX_CHANNELS; i++, ch++ )
	{
		if( !ch->sfx )
			continue;
		if( ch->autosound )
		{ // autosounds are regenerated fresh each frame
			memset( ch, 0, sizeof( *ch ) );
			continue;
		}
		S_Spatialize( ch ); // respatialize channel
		if( !ch->leftvol && !ch->rightvol )
		{
			memset( ch, 0, sizeof( *ch ) );
			continue;
		}
	}

	// add loopsounds
	S_AddLoopSounds();

	//
	// debugging output
	//
	if( s_show->integer )
	{
		total = 0;
		ch = channels;
		for( i = 0; i < MAX_CHANNELS; i++, ch++ )
			if( ch->sfx && ( ch->leftvol || ch->rightvol ) )
			{
				Com_Printf( "%3i %3i %s\n", ch->leftvol, ch->rightvol, ch->sfx->name );
				total++;
			}

			Com_Printf( "----(%i)---- painted: %i\n", total, paintedtime );
	}

	// mix some sound
	S_UpdateBackgroundTrack();

	S_Update_( avidump );
}

/*
* S_BeginAviDemo
*/
void S_BeginAviDemo( void )
{
	size_t checkname_size;
	char *checkname;
	const char *filename = "wavdump";

	if( s_aviDumpFile )
		S_StopAviDemo();

	checkname_size = sizeof( char ) * ( strlen( "avi/" ) + strlen( filename ) + 4 + 1 );
	checkname = S_Malloc( checkname_size );
	Q_snprintfz( checkname, checkname_size, "avi/%s.wav", filename );

	if( trap_FS_FOpenFile( checkname, &s_aviDumpFile, FS_WRITE ) == -1 )
	{
		Com_Printf( "S_BeginAviDemo: Failed to open %s for writing.\n", checkname );
	}
	else
	{
		int i;
		short s;

		// write the WAV header

		trap_FS_Write( "RIFF", 4, s_aviDumpFile );	// "RIFF"
		i = LittleLong( LONG_MAX );
		trap_FS_Write( &i, 4, s_aviDumpFile );		// WAVE chunk length
		trap_FS_Write( "WAVE", 4, s_aviDumpFile );	// "WAVE"

		trap_FS_Write( "fmt ", 4, s_aviDumpFile );	// "fmt "
		i = LittleLong( 16 );
		trap_FS_Write( &i, 4, s_aviDumpFile );		// fmt chunk size
		s = LittleShort( 1 );
		trap_FS_Write( &s, 2, s_aviDumpFile );		// audio format. 1 - PCM uncompressed
		s = LittleShort( dma.channels );
		trap_FS_Write( &s, 2, s_aviDumpFile );		// number of channels
		i = LittleLong( dma.speed );
		trap_FS_Write( &i, 4, s_aviDumpFile );		// sample rate
		i = LittleLong( dma.speed * dma.channels * (dma.samplebits/8) );
		trap_FS_Write( &i, 4, s_aviDumpFile );		// byte rate
		s = LittleShort( dma.channels * (dma.samplebits/8) );
		trap_FS_Write( &s, 2, s_aviDumpFile );		// block align
		s = LittleLong( dma.samplebits );
		trap_FS_Write( &s, 2, s_aviDumpFile );		// block align

		trap_FS_Write( "data", 4, s_aviDumpFile );	// "data"
		i = LittleLong( LONG_MAX-36 );
		trap_FS_Write( &i, 4, s_aviDumpFile );		// data chunk length

		s_aviDumpFileName = S_Malloc( checkname_size );
		memcpy( s_aviDumpFileName, checkname, checkname_size );
	}

	S_Free( checkname );
}

/*
* S_StopAviDemo
*/
void S_StopAviDemo( void )
{
	if( s_aviDumpFile )
	{
		// don't leave empty files
		if( !s_aviNumSamples )
		{
			trap_FS_FCloseFile( s_aviDumpFile );
			trap_FS_RemoveFile( s_aviDumpFileName );
		}
		else
		{
			int size;

			// fill in the missing values in RIFF header
			size = (s_aviNumSamples * dma.channels * (dma.samplebits/8)) + 36;
			trap_FS_Seek( s_aviDumpFile, 4, FS_SEEK_SET );
			trap_FS_Write( &size, 4, s_aviDumpFile );

			size -= 36;
			trap_FS_Seek( s_aviDumpFile, 40, FS_SEEK_SET );
			trap_FS_Write( &size, 4, s_aviDumpFile );

			trap_FS_FCloseFile( s_aviDumpFile );
		}

		s_aviDumpFile = 0;
	}

	s_aviNumSamples = 0;

	if( s_aviDumpFileName )
	{
		S_Free( s_aviDumpFileName );
		s_aviDumpFileName = NULL;
	}
}

/*
* S_API
*/
int S_API( void )
{
	return SOUND_API_VERSION;
}

/*
* S_Error
*/
void S_Error( const char *format, ... )
{
	va_list	argptr;
	char msg[1024];

	va_start( argptr, format );
	Q_vsnprintfz( msg, sizeof( msg ), format, argptr );
	va_end( argptr );

	trap_Error( msg );
}

#ifndef SOUND_HARD_LINKED
// this is only here so the functions in q_shared.c and q_math.c can link
void Sys_Error( const char *format, ... )
{
	va_list	argptr;
	char msg[1024];

	va_start( argptr, format );
	Q_vsnprintfz( msg, sizeof( msg ), format, argptr );
	va_end( argptr );

	trap_Error( msg );
}

void Com_Printf( const char *format, ... )
{
	va_list	argptr;
	char msg[1024];

	va_start( argptr, format );
	Q_vsnprintfz( msg, sizeof( msg ), format, argptr );
	va_end( argptr );

	trap_Print( msg );
}
#endif
