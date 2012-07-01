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

// snd_public.h -- sound dll information visible to engine

#define	SOUND_API_VERSION   32

#define	ATTN_NONE 0

//===============================================================

struct sfx_s;

//
// functions provided by the main engine
//
typedef struct
{
	// drops to console a client game error
	void ( *Error )( const char *msg );

	// console messages
	void ( *Print )( const char *msg );

	// console variable interaction
	cvar_t *( *Cvar_Get )( const char *name, const char *value, int flags );
	cvar_t *( *Cvar_Set )( const char *name, const char *value );
	void ( *Cvar_SetValue )( const char *name, float value );
	cvar_t *( *Cvar_ForceSet )( const char *name, const char *value );      // will return 0 0 if not found
	float ( *Cvar_Value )( const char *name );
	const char *( *Cvar_String )( const char *name );

	int ( *Cmd_Argc )( void );
	char *( *Cmd_Argv )( int arg );
	char *( *Cmd_Args )( void );        // concatenation of all argv >= 1

	void ( *Cmd_AddCommand )( const char *name, void ( *cmd )( void ) );
	void ( *Cmd_RemoveCommand )( const char *cmd_name );
	void ( *Cmd_ExecuteText )( int exec_when, const char *text );
	void ( *Cmd_Execute )( void );
	void ( *Cmd_SetCompletionFunc )( const char *cmd_name, char **( *completion_func )( const char *partial ) );

	// files will be memory mapped read only
	// the returned buffer may be part of a larger pak file,
	// or a discrete file from anywhere in the quake search path
	// a -1 return means the file does not exist
	// NULL can be passed for buf to just determine existance
	int ( *FS_FOpenFile )( const char *filename, int *filenum, int mode );
	int ( *FS_Read )( void *buffer, size_t len, int file );
	int ( *FS_Write )( const void *buffer, size_t len, int file );
	int ( *FS_Print )( int file, const char *msg );
	int ( *FS_Tell )( int file );
	int ( *FS_Seek )( int file, int offset, int whence );
	int ( *FS_Eof )( int file );
	int ( *FS_Flush )( int file );
	void ( *FS_FCloseFile )( int file );
	qboolean ( *FS_RemoveFile )( const char *filename );
	int ( *FS_GetFileList )( const char *dir, const char *extension, char *buf, size_t bufsize, int start, int end );
	qboolean ( *FS_IsUrl )( const char *url );

	unsigned int ( *Milliseconds )( void );
	void ( *PageInMemory )( qbyte *buffer, int size );

	// managed memory allocation
	struct mempool_s *( *Mem_AllocPool )( const char *name, const char *filename, int fileline );
	void *( *Mem_Alloc )( struct mempool_s *pool, int size, const char *filename, int fileline );
	void ( *Mem_Free )( void *data, const char *filename, int fileline );
	void ( *Mem_FreePool )( struct mempool_s **pool, const char *filename, int fileline );
	void ( *Mem_EmptyPool )( struct mempool_s *pool, const char *filename, int fileline );

	void ( *GetEntitySpatilization )( int entnum, vec3_t origin, vec3_t velocity );

	void *( *LoadLibrary )( const char *name, dllfunc_t *funcs );
	void ( *UnloadLibrary )( void **lib );
} sound_import_t;

//
// functions exported by the sound subsystem
//
typedef struct
{
	// if API is different, the dll cannot be used
	int ( *API )( void );

	// the init function will be called at each restart
	qboolean ( *Init )( void *hwnd, int maxEntities, qboolean verbose );
	void ( *Shutdown )( qboolean verbose );

	void ( *BeginRegistration )( void );
	void ( *EndRegistration )( void );

	void ( *StopAllSounds )( void );

	void ( *Clear )( void );
	void ( *Update )( const vec3_t origin, const vec3_t velocity, const vec3_t v_forward, const vec3_t v_right, const vec3_t v_up, qboolean avidump );
	void ( *Activate )( qboolean active );

	void ( *SetAttenuationModel )( int model, float maxdistance, float refdistance );

	// playing
	struct sfx_s *( *RegisterSound )( const char *sample );
	void ( *StartFixedSound )( struct sfx_s *sfx, const vec3_t origin, int channel, float fvol, float attenuation );
	void ( *StartRelativeSound )( struct sfx_s *sfx, int entnum, int channel, float fvol, float attenuation );
	void ( *StartGlobalSound )( struct sfx_s *sfx, int channel, float fvol );

	void ( *StartLocalSound )( const char *s );

	void ( *AddLoopSound )( struct sfx_s *sfx, int entnum, float fvol, float attenuation );

	// cinema
	void ( *RawSamples )( unsigned int samples, unsigned int rate, unsigned short width, unsigned short channels, const qbyte *data, qboolean music );
	unsigned int ( *GetRawSamplesTime )( void ); // Mixing position in milliseconds for A/V sync

	// music
	void ( *StartBackgroundTrack )( const char *intro, const char *loop );
	void ( *StopBackgroundTrack )( void );

	// avi dump
	void ( *BeginAviDemo )( void );
	void ( *StopAviDemo )( void );
} sound_export_t;
