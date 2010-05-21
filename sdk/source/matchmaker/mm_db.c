/*
   Copyright (C) 2007 Will Franklin

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

#include "matchmaker.h"

db_handle_t db_handle;

static cvar_t *db_host;
static cvar_t *db_user;
static cvar_t *db_pass;
static cvar_t *db_name;
static cvar_t *db_showQueries;
static cvar_t *db_showQueryErrors;

static db_export_t *dbe;
static void *module_handle;

//================
// DB_LoadLibrary
// Load dll and get function pointers
//================
static void DB_LoadLibrary( void )
{
	int apiversion;
	size_t file_size;
	char *file;
	GetDBAPI_t func;
	dllfunc_t funcs[2];

	if( dbe )
		return;

	file_size = strlen( LIB_DIRECTORY ) + 1 + strlen( "dbmysql" ) + 1 + strlen( ARCH ) + strlen( LIB_SUFFIX ) + 1;
	file = Mem_TempMalloc( file_size );
	Q_strncpyz( file, LIB_DIRECTORY "/dbmysql_" ARCH LIB_SUFFIX, file_size );

	funcs[0].name = "GetDBAPI";
	funcs[0].funcPointer = ( void ** )&func;
	funcs[1].name = NULL;
	module_handle = Com_LoadLibrary( file, funcs );

	Mem_TempFree( file );

	if( !module_handle )
		Com_Error( ERR_FATAL, "Failed to load DB DLL" );

	dbe = func();
	if( !dbe )
	{
		Com_Error( ERR_FATAL, "Failed to load DB function pointers" );
		Com_UnloadLibrary( &module_handle );
	}

	apiversion = dbe->API();

	if( apiversion != DB_API_VERSION )
	{
		Com_UnloadLibrary( &module_handle );
		dbe = NULL;
		Com_Error( ERR_FATAL, "DB is version %i, not %i", apiversion, DB_API_VERSION );
	}
}

//================
// DB_UnloadLibrary
// Unload dll and clear function pointers
//================
static void DB_UnloadLibrary( void )
{
	Com_UnloadLibrary( &module_handle );
	dbe = NULL;
}

//================
// DB_Init
// Initialize database connection
//================
void DB_Init( void )
{
	db_status_t status;

	// load dll
	DB_LoadLibrary();

	// db cvars
	db_host = Cvar_Get( "db_host", "localhost", CVAR_ARCHIVE );
	db_user = Cvar_Get( "db_user", "", CVAR_ARCHIVE );
	db_pass = Cvar_Get( "db_pass", "", CVAR_ARCHIVE );
	db_name = Cvar_Get( "db_name", "", CVAR_ARCHIVE );
	db_showQueries = Cvar_Get( "db_showQueries", "0", CVAR_ARCHIVE );
	db_showQueryErrors = Cvar_Get( "db_showQueryErrors", "1", CVAR_ARCHIVE );

	// db connection
	Com_Printf( "Opening connection to database '%s':\n  '%s'@'%s' (using password: %s)\n",
	            db_name->string,
	            db_user->string,
	            db_host->string,
	            strlen( db_pass->string ) ? "YES" : "NO" );
	status = DB_Connect( &db_handle, db_host->string, db_user->string, db_pass->string, db_name->string );
	if( status == DB_ERROR )
		Com_Error( ERR_FATAL, "Couldn't open connection to database: %s\n", DB_GetError( GLOBAL_ERROR ) );
}

//================
// DB_Shutdown
// Shutdown database connection
//================
void DB_Shutdown( void )
{
	if( !dbe )
		return;

	if( db_handle > -1 )
		dbe->Disconnect( db_handle );

	db_handle = -1;

	DB_UnloadLibrary();
}

//================
// DB LIBRARY FUNCTIONS
//================

db_status_t DB_Connect( db_handle_t *handle, const char *host, const char *user, const char *pass, const char *db )
{
	if( dbe )
		return dbe->Connect( handle, host, user, pass, db );

	return DB_ERROR;
}

void DB_Disconnect( db_handle_t handle )
{
	if( dbe )
		dbe->Disconnect( handle );
}

const db_error_t *DB_GetError( db_handle_t handle )
{
	if( dbe )
		return dbe->GetError( handle );

	return NULL;
}

void DB_EscapeString( char *out, const char *in, size_t size )
{
	if( dbe )
		dbe->EscapeString( out, in, size );
}

db_status_t DB_Query( db_handle_t handle, const char *format, ... )
{
	va_list argptr;
	char query[1024];

	if( !dbe )
		return DB_ERROR;

	va_start( argptr, format );
	Q_vsnprintfz( query, sizeof( query ), format, argptr );
	va_end( argptr );
		
	if( db_showQueries->integer )
		Com_Printf( "DB query executed:\n%s\n", query );

	if( db_showQueryErrors->integer )
	{
		db_status_t status = dbe->Query( handle, query );
		if( status == DB_ERROR )
			Com_Printf( "DB query failed:\n%s\n", DB_GetError( handle ) );

		return status;
	}

	return dbe->Query( handle, query );
}

db_status_t DB_FetchResult( db_handle_t handle, db_result_t *result )
{
	if( dbe )
		return dbe->FetchResult( handle, result );

	return DB_ERROR;
}

void DB_FreeResult( db_result_t *result )
{
	if( dbe )
		dbe->FreeResult( result );
}
