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

// MAPLIST FUNCTIONS

#include "qcommon.h"

#define MLIST_CACHE "mapcache.txt"
#define MLIST_NULL  ""
#define MLIST_HASH_SIZE 128

#define MLIST_UNKNOWN_MAPNAME	"@#$"

#define MLIST_CACHE_EXISTS ( FS_FOpenFile( MLIST_CACHE, NULL, FS_READ ) != -1 )

typedef struct mapinfo_s
{
	char *filename;
	char *fullname;

	struct mapinfo_s *next;
	struct mapinfo_s *nextfilename; // for hashes
	struct mapinfo_s *nextfullname;
} mapinfo_t;

static mapinfo_t *maplist;
static mapinfo_t *filename_hash[MLIST_HASH_SIZE];
static mapinfo_t *fullname_hash[MLIST_HASH_SIZE];

static qboolean ml_flush = qtrue;
static qboolean ml_initialized = qfalse;

static void ML_BuildCache( void );
static void ML_InitFromCache( void );
static void ML_InitFromMaps( void );
static void ML_GetFullnameFromMap( const char *filename, char *fullname, size_t len );
static qboolean ML_FilenameExistsExt( const char *filename, qboolean quick );

//=================
// ML_AddMap
// Handles assigning memory for map and adding it to the list
// in alphabetical order
//=================
static void ML_AddMap( const char *filename, const char *fullname )
{
	mapinfo_t *map, *current, *prev;
	int key;
	char *buffer;
	char fullname_[MAX_CONFIGSTRING_CHARS];

	if( !ML_ValidateFilename( filename ) )
		return;

	if( !fullname )
	{
		ML_GetFullnameFromMap( filename, fullname_, sizeof( fullname_ ) );
		fullname = fullname_;
	}
		
	if( !ML_ValidateFullname( fullname ) && *fullname )	// allow empty fullnames
		return;

	ml_flush = qtrue;	// tell everyone that maplist has changed 
	buffer = Mem_ZoneMalloc( sizeof( mapinfo_t ) + strlen( filename ) + 1 + strlen( fullname ) + 1 );

	map = ( mapinfo_t * )buffer;
	buffer += sizeof( mapinfo_t );

	map->filename = buffer;
	strcpy( map->filename, filename );
	COM_StripExtension( map->filename );
	buffer += strlen( filename ) + 1;

	map->fullname = buffer;
	strcpy( map->fullname, fullname );
	Q_strlwr( map->fullname );

	key = Com_HashKey( map->filename, MLIST_HASH_SIZE );
	map->nextfilename = filename_hash[key];
	filename_hash[key] = map;

	key = Com_HashKey( map->fullname, MLIST_HASH_SIZE );
	map->nextfullname = fullname_hash[key];
	fullname_hash[key] = map;

	// keep the list in alphabetical order (case insensitive)
	prev = NULL;
	for( current = maplist; current; current = current->next )
	{
		if( Q_stricmp( filename, current->filename ) < 1 )
			break;
		prev = current;
	}

	// beginning of the list
	if( !prev )
	{
		map->next = maplist;
		maplist = map;
	}
	else
	{
		prev->next = map;
		map->next = current;
	}
}

//=================
// ML_BuildCache
// Write the map data to a cache file
//=================
static void ML_BuildCache( void )
{
	int filenum;
	mapinfo_t *map;

	if( !ml_initialized )
		return;

	if( FS_FOpenFile( MLIST_CACHE, &filenum, FS_WRITE ) != -1 )
	{
		for( map = maplist; map; map = map->next )
			FS_Printf( filenum, "%s\r\n%s\r\n", map->filename, map->fullname );
		FS_FCloseFile( filenum );
	}
}

typedef struct mapdir_s
{
	char *filename;

	struct mapdir_s *prev;
	struct mapdir_s *next;
} mapdir_t;

//=================
// ML_InitFromCache
// Fills map list array from cache, much faster
//=================
static void ML_InitFromCache( void )
{
	int count, i, total, len;
	size_t size = 0;
	char *buffer, *chr, *current, *curend;
	char *temp, *maps, *map;
	mapdir_t *dir, *curmap, *prev;

	if( ml_initialized )
		return;

	total = FS_GetFileListExt( "maps", ".bsp", NULL, &size, 0, 0 );
	if( !total )
		return;

	// load maps from directory reading into a list
	maps = temp = Mem_TempMalloc( size + sizeof( mapdir_t ) * total );
	temp += size;
	FS_GetFileList( "maps", ".bsp", maps, size, 0, 0 );
	len = 0;
	prev = NULL;
	dir = NULL;
	for( i = 0; i < total; i++ )
	{
		map = maps + len;
		len += strlen( map ) + 1;

		curmap = ( mapdir_t * )temp;
		temp += sizeof( mapdir_t );

		COM_StripExtension( map );

		if( !i )
			dir = curmap;
		else
		{
			prev->next = curmap;
			curmap->prev = prev;
		}

		curmap->filename = map;
		prev = curmap;
	}

	FS_LoadFile( MLIST_CACHE, (void **)&buffer, NULL, 0 );
	if( !buffer )
	{
		Mem_TempFree( maps );
		return;
	}

	current = curend = buffer;
	count = 0;

	for( chr = buffer; *chr; chr++ )
	{
		// current character is a delimiter
		if( *chr == '\n' )
		{
			if( *(chr-1) == '\r' )
				*(chr-1) = '\0';	// clear the CR too
			*chr = '\0';			// clear the LF

			// if we have got both params
			if( !( ++count & 1 ) )
			{
				// check if its in the maps directory
				for( curmap = dir; curmap; curmap = curmap->next )
				{
					if( !Q_stricmp( curmap->filename, current ) )
					{
						if( curmap->prev )
							curmap->prev->next = curmap->next;
						else
							dir = curmap->next;

						if( curmap->next )
							curmap->next->prev = curmap->prev;
						break;
					}
				}

				// if we found it in the maps directory
				if( curmap )
				{
					COM_SanitizeFilePath( current );

					// well, if we've got a map with an unknown fullname, load it from map
					if( !strcmp( curend + 1, MLIST_UNKNOWN_MAPNAME ) )
						ML_AddMap( current, NULL );
					else
						ML_AddMap( current, curend + 1 );
				}
				current = chr + 1;
			}
			else
				curend = chr;
		}
	}

	// we've now loaded the mapcache, but there may be files which
	// have been added to maps directory, and the mapcache isnt aware
	// these will be left over in our directory list
	for( curmap = dir; curmap; curmap = curmap->next )
		ML_AddMap( curmap->filename, NULL );

	Mem_TempFree( maps );
	FS_FreeFile( buffer );
}

//=================
// ML_InitFromMaps
// Fills map list array from each map file. Very slow
// and should only be called if cache doesnt exist
//=================
static void ML_InitFromMaps( void )
{
	int i, j, total, len;
	char maps[2048];
	char *filename;

	if( ml_initialized )
		return;

	total = FS_GetFileList( "maps", ".bsp", NULL, 0, 0, 0 );
	if( !total )
		return;

	i = 0;
	while( i < total )
	{
		memset( maps, 0, sizeof( maps ) );
		j = FS_GetFileList( "maps", ".bsp", maps, sizeof( maps ), i, total );

		// no maps returned, map name is too big or end of list?
		if( !j )
		{
			i++;
			continue;
		}
		i += j;

		// split the maps up and find their fullnames
		len = 0;
		while( j-- )
		{
			filename = maps + len;
			if( !*filename )
				continue;

			len += strlen( filename ) + 1;
			
			COM_SanitizeFilePath( filename );
			COM_StripExtension( filename );

			ML_AddMap( filename, NULL );
		}
	}
}

//=================
// ML_MapListCmd
// Handler for console command "maplist"
//=================
static void ML_MapListCmd( void )
{
	char *pattern;
	mapinfo_t *map;
	int argc = Cmd_Argc();
	int count = 0;

	if( argc > 2 )
	{
		Com_Printf( "Usage: %s [rebuild]\n", Cmd_Argv(0) );
		return;
	}

	pattern = (argc == 2 ? Cmd_Argv( 1 ) : NULL);
	if( pattern && !*pattern )
		pattern = NULL;

	if( pattern )
	{
		if( !strcmp( pattern, "rebuild" ) )
		{
			Com_Printf( "Rebuilding map list...\n" );
			ML_Restart( qtrue );
			return;
		}

		if( !strcmp( pattern, "update" ) )
		{
			Com_Printf( "Updating map list...\n" );
			ML_Update();
			return;
		}
	}

	for( map = maplist ; map ; map = map->next )
	{
		if( pattern && !Com_GlobMatch( pattern, map->filename, qfalse ) )
			continue;

		Com_Printf( "%s: %s\n", map->filename, map->fullname );
		count++;
	}

	Com_Printf( "%d map(s) %s\n", count, pattern ? "matching" : "total" );
}

//=================
// ML_Init
// Initialize map list. Check if cache file exists, if not create it
//=================
void ML_Init( void )
{
	if( ml_initialized )
		return;

	maplist = NULL;

	Cmd_AddCommand( "maplist", ML_MapListCmd );

	if( MLIST_CACHE_EXISTS )
		ML_InitFromCache();
	else
		ML_InitFromMaps();

	ML_BuildCache();

	ml_initialized = qtrue;
	ml_flush = qtrue;
}

//=================
// ML_Shutdown
// Free map list memory
//=================
void ML_Shutdown( void )
{
	mapinfo_t *map, *next;

	if( !ml_initialized )
		return;

	Cmd_RemoveCommand( "maplist" );

	ML_BuildCache();

	map = maplist;
	while( map )
	{
		next = map->next;
		Mem_ZoneFree( map );
		map = next;
	}

	maplist = NULL;

	memset( filename_hash, 0, sizeof( filename_hash ) );
	memset( fullname_hash, 0, sizeof( fullname_hash ) );

	ml_initialized = qfalse;
	ml_flush = qtrue;
}

//=================
// ML_Restart
// Restart map list stuff
//=================
void ML_Restart( qboolean forcemaps )
{
	ML_Shutdown();
	if( forcemaps )
		FS_RemoveFile( MLIST_CACHE );
	FS_Rescan();
	ML_Init();
}

//=================
// ML_Update
//=================
qboolean ML_Update( void )
{
	int i, len, total, newpaks;
	size_t size;
	char *map, *maps, *filename;

	newpaks = FS_Rescan();
	if( !newpaks )
		return qfalse;

	total = FS_GetFileListExt( "maps", ".bsp", NULL, &size, 0, 0 );
	if( size )
	{
		maps = Mem_TempMalloc( size );
		FS_GetFileList( "maps", ".bsp", maps, size, 0, 0 );
		for( i = 0, len = 0; i < total; i++ )
		{
			map = maps + len;
			len += strlen( map ) + 1;

			filename = ( char * )COM_FileBase( map );
			COM_StripExtension( filename );

			// don't check for existance of each file itself, as we've just got the fresh list
			if( !ML_FilenameExistsExt( filename, qtrue ) )
				ML_AddMap( filename, MLIST_UNKNOWN_MAPNAME );
		}
		Mem_TempFree( maps );
	}

	return qtrue;
}

//=================
// ML_GetFilename
// Returns the filename for the map with the corresponding fullname
//=================
const char *ML_GetFilenameExt( const char *fullname, qboolean recursive )
{
	mapinfo_t *map;
	int key;
	char *filename, *fullname2;

	if( !ml_initialized )
		return MLIST_NULL;

	if( !ML_ValidateFullname( fullname ) )
		return MLIST_NULL;

	filename = NULL;
	fullname2 = Mem_TempMalloc( strlen( fullname ) + 1 );
	strcpy( fullname2, fullname );
	Q_strlwr( fullname2 );

	key = Com_HashKey( fullname, MLIST_HASH_SIZE );
	for( map = fullname_hash[key]; map; map = map->nextfullname )
	{
		if( !strcmp( fullname2, map->fullname ) )
		{
			filename = map->filename;
			break;
		}
	}

	Mem_Free( fullname2 );

	if( filename )
		return filename;

	// we should technically never get here, but
	// maybe the mapper has changed the fullname of the map
	// or the user has tampered with the mapcache
	// we need to reload the whole cache from file if we get here

	if( !recursive )
	{
		ML_Restart( qtrue );
		return ML_GetFilenameExt( fullname, qtrue );
	}

	return MLIST_NULL;
}

//=================
// ML_GetFilename
// Returns the filename for the map with the corresponding fullname
//=================
const char *ML_GetFilename( const char *fullname )
{
	return ML_GetFilenameExt( fullname, qfalse );
}

//=================
// ML_FilenameExists
// Checks to see if a filename is present in the map list
//=================
static qboolean ML_FilenameExistsExt( const char *filename, qboolean quick )
{
	mapinfo_t *map;
	int key;
	char *filepath;

	if( !ml_initialized )
		return qfalse;

	filepath = va( "maps/%s.bsp", filename );
	COM_SanitizeFilePath( filepath );

	if( !ML_ValidateFilename( filename ) )
		return qfalse;

	key = Com_HashKey( filename, MLIST_HASH_SIZE );
	for( map = filename_hash[key]; map; map = map->nextfilename )
	{
		if( !Q_stricmp( filename, map->filename ) )
		{
			if( !quick && FS_FOpenFile( filepath, NULL, FS_READ ) == -1 )
				return qfalse;
			return qtrue;
		}
	}

	return qfalse;
}

//=================
// ML_FilenameExists
// Checks to see if a filename is present in the map list
//=================
qboolean ML_FilenameExists( const char *filename )
{
	return ML_FilenameExistsExt( filename, qfalse );
}

//=================
// ML_GetFullname
// Returns the fullname for the map with the corresponding filename
//=================
const char *ML_GetFullname( const char *filename )
{
	mapinfo_t *map;
	int key;
	char *filepath;

	if( !ml_initialized )
		return MLIST_NULL;

	filepath = va( "maps/%s.bsp", filename );
	COM_SanitizeFilePath( filepath );

	if( !ML_ValidateFilename( filepath ) )
		return MLIST_NULL;
/*
	if( FS_FOpenFile( filepath, NULL, FS_READ ) == -1 )
		return MLIST_NULL;
*/
	COM_StripExtension( filepath );

	key = Com_HashKey( filename, MLIST_HASH_SIZE );
	for( map = filename_hash[key]; map; map = map->nextfilename )
	{
		if( !Q_stricmp( filename, map->filename ) )
			return map->fullname;
	}

	// we should never get down here!
	assert( qfalse );

	return MLIST_NULL;
}

//=================
// ML_GetFullnameFromMap
// Get fullname of map from file or worldspawn (slow)
//=================
static void ML_GetFullnameFromMap( const char *filename, char *fullname, size_t len )
{
	char *buffer, *line;

	*fullname = '\0';

	// Try and load fullname from a file
	FS_LoadFile( va( "maps/%s.txt", filename ), ( void ** )&buffer, NULL, 0 );
	if( buffer )
	{
		line = strtok( buffer, "\n" );
		Q_strncpyz( fullname, buffer, len );
		FS_FreeFile( buffer );
		return;
	}

	// Try and load fullname from worldspawn
	CM_LoadMapMessage( va( "maps/%s.bsp", filename ), fullname, len );
	COM_RemoveColorTokens( fullname );
}

//=================
// ML_ValidateFilename
// Checks that the filename provided is valid
//=================
qboolean ML_ValidateFilename( const char *filename )
{
	if( !filename || !*filename )
		return qfalse;

	if( !COM_FileExtension( filename ) )
	{
		if( strlen( "maps/" ) + strlen( filename ) + strlen( ".bsp" ) >= MAX_CONFIGSTRING_CHARS )
			return qfalse;
	}
	else
	{
		if( Q_stricmp( COM_FileExtension( filename ), ".bsp" ) )
			return qfalse;
		if( strlen( "maps/" ) + strlen( filename ) >= MAX_CONFIGSTRING_CHARS )
			return qfalse;
	}

	if( !COM_ValidateRelativeFilename( filename ) || strchr( filename, '/' ) )
		return qfalse;

	return qtrue;
}

//=================
// ML_ValidateFullname
// Checks that the fullname provided is valid
//=================
qboolean ML_ValidateFullname( const char *fullname )
{
	if( !fullname || !*fullname )
		return qfalse;

	if( strlen( fullname ) >= MAX_CONFIGSTRING_CHARS )
		return qfalse;

	return qtrue;
}

//=================
// ML_GetMapByNum
// Prints map infostring in "mapname\0fullname" format into "out" string,
// returns fullsize (so that out can be reallocated if there's not enough space)
//=================
size_t ML_GetMapByNum( int num, char *out, size_t size )
{
	static int i = 0;
	size_t fsize;
	static mapinfo_t *map = NULL;

	if( !ml_initialized )
		return 0;

	if( ml_flush || i > num )
	{
		map = NULL;
		ml_flush = qfalse;
	}
	if( !map )
	{
		i = 0;
		map = maplist;
	}

	for( ; i < num && map; i++, map = map->next )
		;
	if( !map )
		return 0;

	fsize = strlen( map->filename ) + 1 + strlen( map->fullname ) + 1;
	if( out && (fsize <= size) )
	{
		Q_strncpyz( out, map->filename, size );
		Q_strncpyz( out + strlen( out ) + 1, 
			strcmp( map->fullname, MLIST_UNKNOWN_MAPNAME ) ? map->fullname : map->filename, size - (strlen( out ) + 1) );
	}

	return fsize;
}
