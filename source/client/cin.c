/*
Copyright (C) 2012 Victor Luchits

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

static cin_export_t *cin_export;
static void *cin_libhandle = NULL;
static mempool_t *cin_mempool;

/*
* CL_CinModule_Error
*/
static void CL_CinModule_Error( const char *msg )
{
	Com_Error( ERR_FATAL, "%s", msg );
}

/*
* CL_CinModule_Print
*/
static void CL_CinModule_Print( const char *msg )
{
	Com_Printf( "%s", msg );
}

/*
* CL_CinModule_MemAlloc
*/
static void *CL_CinModule_MemAlloc( mempool_t *pool, int size, const char *filename, int fileline )
{
	return _Mem_Alloc( pool, size, MEMPOOL_CINMODULE, 0, filename, fileline );
}

/*
* CL_CinModule_MemFree
*/
static void CL_CinModule_MemFree( void *data, const char *filename, int fileline )
{
	_Mem_Free( data, MEMPOOL_CINMODULE, 0, filename, fileline );
}

/*
* CL_CinModule_MemAllocPool
*/
static mempool_t *CL_CinModule_MemAllocPool( const char *name, const char *filename, int fileline )
{
	return _Mem_AllocPool( cin_mempool, name, MEMPOOL_CINMODULE, filename, fileline );
}

/*
* CL_CinModule_MemFreePool
*/
static void CL_CinModule_MemFreePool( mempool_t **pool, const char *filename, int fileline )
{
	_Mem_FreePool( pool, MEMPOOL_CINMODULE, 0, filename, fileline );
}

/*
* CL_CinModule_MemEmptyPool
*/
static void CL_CinModule_MemEmptyPool( mempool_t *pool, const char *filename, int fileline )
{
	_Mem_EmptyPool( pool, MEMPOOL_CINMODULE, 0, filename, fileline );
}

/*
* CIN_LoadLibrary
*/
void CIN_LoadLibrary( qboolean verbose )
{
	static cin_import_t import;
	dllfunc_t funcs[2];
	void *( *GetCinematicsAPI )(void *);

	assert( !cin_libhandle );

	import.Print = &CL_CinModule_Print;
	import.Error = &CL_CinModule_Error;

	import.Cvar_Get = &Cvar_Get;
	import.Cvar_Set = &Cvar_Set;
	import.Cvar_SetValue = &Cvar_SetValue;
	import.Cvar_ForceSet = &Cvar_ForceSet;
	import.Cvar_String = &Cvar_String;
	import.Cvar_Value = &Cvar_Value;

	import.Cmd_Argc = &Cmd_Argc;
	import.Cmd_Argv = &Cmd_Argv;
	import.Cmd_Args = &Cmd_Args;

	import.Cmd_AddCommand = &Cmd_AddCommand;
	import.Cmd_RemoveCommand = &Cmd_RemoveCommand;
	import.Cmd_ExecuteText = &Cbuf_ExecuteText;
	import.Cmd_Execute = &Cbuf_Execute;
	import.Cmd_SetCompletionFunc = &Cmd_SetCompletionFunc;

	import.FS_FOpenFile = &FS_FOpenFile;
	import.FS_Read = &FS_Read;
	import.FS_Write = &FS_Write;
	import.FS_Print = &FS_Print;
	import.FS_Tell = &FS_Tell;
	import.FS_Seek = &FS_Seek;
	import.FS_Eof = &FS_Eof;
	import.FS_Flush = &FS_Flush;
	import.FS_FCloseFile = &FS_FCloseFile;
	import.FS_RemoveFile = &FS_RemoveFile;
	import.FS_GetFileList = &FS_GetFileList;
	import.FS_IsUrl = &FS_IsUrl;

	import.Milliseconds = &Sys_Milliseconds;
	import.Microseconds = &Sys_Microseconds;

	import.Mem_AllocPool = &CL_CinModule_MemAllocPool;
	import.Mem_Alloc = &CL_CinModule_MemAlloc;
	import.Mem_Free = &CL_CinModule_MemFree;
	import.Mem_FreePool = &CL_CinModule_MemFreePool;
	import.Mem_EmptyPool = &CL_CinModule_MemEmptyPool;

	import.S_Clear = &CL_SoundModule_Clear;
	import.S_RawSamples = &CL_SoundModule_RawSamples;
	import.S_GetRawSamplesTime = &CL_SoundModule_GetRawSamplesTime;

	// load dynamic library
	cin_export = NULL;
	if( verbose ) {
		Com_Printf( "Loading CIN module... " );
	}

	funcs[0].name = "GetCinematicsAPI";
	funcs[0].funcPointer = ( void ** ) &GetCinematicsAPI;
	funcs[1].name = NULL;
	cin_libhandle = Com_LoadLibrary( LIB_DIRECTORY "/cin_" ARCH LIB_SUFFIX, funcs );

	if( cin_libhandle )
	{
		// load succeeded
		int api_version;

		cin_export = GetCinematicsAPI( &import );
		cin_mempool = Mem_AllocPool( NULL, "CIN Module" );

		api_version = cin_export->API();

		if( api_version == CIN_API_VERSION )
		{
			if( cin_export->Init( verbose ) )
			{
				if( verbose ) {
					Com_Printf( "Success.\n" );
				}
			}
			else
			{
				// initialization failed
				Mem_FreePool( &cin_mempool );
				if( verbose ) {
					Com_Printf( "Initialization failed.\n" );
				}
				CIN_UnloadLibrary( verbose );
			}
		}
		else
		{
			// wrong version
			Mem_FreePool( &cin_mempool );
			Com_Printf( "CIN_LoadLibrary: wrong version: %i, not %i.\n", api_version, CIN_API_VERSION );
			CIN_UnloadLibrary( verbose );
		}
	}
	else
	{
		if( verbose ) {
			Com_Printf( "Not found.\n" );
		}
	}

	Mem_CheckSentinelsGlobal();
}

/*
* CIN_UnloadLibrary
*/
void CIN_UnloadLibrary( qboolean verbose )
{
	if( cin_export != NULL ) {
		cin_export->Shutdown( verbose );
		cin_export = NULL;
	}

	if( !cin_libhandle ) {
		return;
	}

	assert( cin_libhandle != NULL );

	Com_UnloadLibrary( &cin_libhandle );

	assert( !cin_libhandle );

	if( verbose ) {
		Com_Printf( "CIN module unloaded.\n" );
	}
}

struct cinematics_s *CIN_Open( const char *name, unsigned int start_time, int flags )
{
	if( cin_export ) {
		return cin_export->Open( name, start_time, flags );
	}
	return NULL;
}

qboolean CIN_NeedNextFrame( struct cinematics_s *cin, unsigned int curtime )
{
	if( cin_export ) {
		return cin_export->NeedNextFrame( cin, curtime );
	}
	return qfalse;
}

qbyte *CIN_ReadNextFrame( struct cinematics_s *cin, int *width, int *height, int *aspect_numerator, int *aspect_denominator, qboolean *redraw )
{
	if( cin_export ) {
		return cin_export->ReadNextFrame( cin, width, height, aspect_numerator, aspect_denominator, redraw );
	}
	return NULL;
}

void CIN_Close( struct cinematics_s *cin )
{
	if( cin_export ) {
		cin_export->Close( cin );
	}
}
