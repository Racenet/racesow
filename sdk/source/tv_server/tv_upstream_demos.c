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

#include "tv_local.h"
#include "tv_upstream.h"
#include "tv_upstream_demos.h"

/*
============================================================================

DEMO RECORDING

============================================================================
*/

/*
* TV_Upstream_IsAutoRecordable
*/
qboolean TV_Upstream_IsAutoRecordable( upstream_t *upstream )
{
	char *s, *t;
	static const char *seps = ";";
	qboolean match = qfalse;
	upstream_t *tupstream;

	assert( upstream );

	if( upstream->demoplaying )
		return qfalse;
	if( !Q_stricmp( tv_autorecord->string, "*" ) )
		return qtrue;

	// search for the given upstream in record list (semicolon separated)
	s = TempCopyString( tv_autorecord->string );

	t = strtok( s, seps );
	while( t != NULL )
	{
		qboolean res = TV_UpstreamForText( t, &tupstream );
		if( res && (tupstream == upstream) )
		{
			match = qtrue;
			break; // found a match
		}

		t = strtok( NULL, seps );
	}

	Mem_TempFree( s );
	if( match )
		return qtrue;

	return qfalse;
}

/*
* TV_Upstream_AutoRecordName
*/
static const char *TV_Upstream_AutoRecordName( upstream_t *upstream, char *name, size_t name_size )
{
	const char *gametype;
	char datetime[32];
	char matchname[MAX_CONFIGSTRING_CHARS];
	time_t long_time;
	struct tm *newtime;

	// date & time
	time( &long_time );
	newtime = localtime( &long_time );

	Q_snprintfz( datetime, sizeof( datetime ), "%04d-%02d-%02d_%02d-%02d", newtime->tm_year + 1900,
		newtime->tm_mon+1, newtime->tm_mday, newtime->tm_hour, newtime->tm_min );

	Q_strncpyz( matchname, upstream->configstrings[CS_MATCHNAME], sizeof( matchname ) );
	if( matchname[0] != '\0')
	{
		char *t = strstr( matchname, " vs " );
		if( t )
			memcpy( t, "_vs_", strlen( "_vs_" ) );
		Q_strncpyz( matchname, COM_RemoveJunkChars( COM_RemoveColorTokens( matchname ) ), sizeof( matchname ) );
	}

	// combine
	gametype = upstream->configstrings[CS_GAMETYPENAME];
	Q_snprintfz( name, name_size, "%s_%s_%s%s%s_auto%04i", datetime, gametype,
		upstream->levelname, matchname[0] ? "_" : "", matchname, (int)brandom( 0, 9999 ) );

	return name;
}

/*
* TV_Upstream_AutoRecordAction
*/
void TV_Upstream_AutoRecordAction( upstream_t *upstream, const char *action )
{
	char name[MAX_STRING_CHARS];
	const char *gametype;

	if( !action[0] )
		return;

	// filter out autorecord commands when playing a demo
	if( upstream->demoplaying )
		return;

	gametype = upstream->configstrings[CS_GAMETYPENAME];
	TV_Upstream_AutoRecordName( upstream, name, sizeof( name ) );

	if( !Q_stricmp( action, "start" ) )
	{
		if( upstream->demorecording )
			return;

		if( !TV_Upstream_IsAutoRecordable( upstream ) )
			return;

		TV_Upstream_StopDemoRecord( upstream, qtrue, qfalse );
		TV_Upstream_StartDemoRecord( upstream, name, qfalse );

		upstream->demoautorecording = qtrue;
	}
	else if( !Q_stricmp( action, "altstart" ) )
	{
		if( !TV_Upstream_IsAutoRecordable( upstream ) )
			return;

		TV_Upstream_StartDemoRecord( upstream, name, qtrue );

		upstream->demoautorecording = qtrue;
	}
	else if( !Q_stricmp( action, "stop" ) )
	{
		if( upstream->demoautorecording )
		{
			TV_Upstream_StopDemoRecord( upstream, qfalse, qfalse );
			upstream->demoautorecording = qfalse;
		}
	}
	else if( !Q_stricmp( action, "cancel" ) )
	{
		if( upstream->demoautorecording )
		{
			TV_Upstream_StopDemoRecord( upstream, qtrue, qtrue );
			upstream->demoautorecording = qfalse;
		}
	}
}

/*
* TV_Upstream_WriteDemoMessage
*
* Dumps the current net message, prefixed by the length
*/
void TV_Upstream_WriteDemoMessage( upstream_t *upstream, msg_t *msg )
{
	if( upstream->demofilehandle <= 0 )
	{
		upstream->demorecording = qfalse;
		return;
	}

	// the first eight bytes are just packet sequencing stuff
	SNAP_RecordDemoMessage( upstream->demofilehandle, msg, 8 );
}

/*
* TV_Upstream_StartDemoRecord
*/
void TV_Upstream_StartDemoRecord( upstream_t *upstream, const char *demoname, qboolean silent )
{
	char *servername, *temp;
	size_t name_size;

	assert( upstream );
	assert( demoname );

	if( upstream->demoplaying )
	{
		if( !silent )
			Com_Printf( "You can't record from another demo.\n" );
		return;
	}

	if( upstream->demorecording )
	{
		if( !silent )
			Com_Printf( "Already recording.\n" );
		return;
	}

	// strip the port number from servername
	servername = TempCopyString( upstream->servername );
	temp = strstr( servername, ":" );
	if( temp ) *temp = '\0';

	// store the name
	name_size = sizeof( char ) * ( strlen( "demos/tvserver" ) + 1 + strlen( servername ) + 1 + strlen( demoname ) + strlen( APP_DEMO_EXTENSION_STR ) + 1 );
	upstream->demofilename = Mem_ZoneMalloc( name_size );

	Q_snprintfz( upstream->demofilename, name_size, "demos/tvserver/%s/%s", servername, demoname );
	COM_SanitizeFilePath( upstream->demofilename );
	COM_DefaultExtension( upstream->demofilename, APP_DEMO_EXTENSION_STR, name_size );

	Mem_TempFree( servername );

	if( !COM_ValidateRelativeFilename( upstream->demofilename ) )
	{
		if( !silent )
			Com_Printf( "Invalid filename.\n" );
		Mem_ZoneFree( upstream->demofilename );
		return;
	}

	// temp name
	name_size = sizeof( char ) * ( strlen( upstream->demofilename ) + strlen( ".rec" ) + 1 );
	upstream->demotempname = Mem_ZoneMalloc( name_size );
	Q_snprintfz( upstream->demotempname, name_size, "%s.rec", upstream->demofilename );

	// open the demo file
	if( FS_FOpenFile( upstream->demotempname, &upstream->demofilehandle, FS_WRITE ) == -1 )
	{
		Com_Printf( "Error: Couldn't create the demo file.\n" );

		Mem_ZoneFree( upstream->demotempname );
		upstream->demotempname = NULL;
		Mem_ZoneFree( upstream->demofilename );
		upstream->demofilename = NULL;
		return;
	}

	if( !silent )
		Com_Printf( "Recording demo: %s\n", upstream->demofilename );

	upstream->demorecording = qtrue;
	upstream->demobasetime = upstream->demoduration = 0;

	// don't start saving messages until a non-delta compressed message is received
	TV_Upstream_AddReliableCommand( upstream, "nodelta" ); // request non delta compressed frame from server
	upstream->demowaiting = qtrue;

	// write out messages to hold the startup information
	SNAP_BeginDemoRecording( upstream->demofilehandle, 0x10000 + upstream->servercount, 
		upstream->snapFrameTime, upstream->levelname, upstream->reliable ? SV_BITFLAGS_RELIABLE : 0, 
		upstream->purelist, upstream->configstrings[0], upstream->baselines );

	// the rest of the demo file will be individual frames
}

/*
* TV_Upstream_StopDemoRecord
*/
void TV_Upstream_StopDemoRecord( upstream_t *upstream, qboolean silent, qboolean cancel )
{
	assert( upstream );

	if( !upstream->demorecording )
	{
		if( !silent )
			Com_Printf( "Not recording a demo.\n" );
		return;
	}

	// finish up
	SNAP_StopDemoRecording( upstream->demofilehandle, upstream->demobasetime, upstream->demoduration );

	FS_FCloseFile( upstream->demofilehandle );

	// cancel the demos
	if( cancel )
	{
		// remove the file that correspond to cls.demo.file
		if( !silent )
			Com_Printf( "Canceling demo: %s\n", upstream->demofilename );
		if( !FS_RemoveFile( upstream->demotempname ) && !silent )
			Com_Printf( "Error canceling demo." );
	}
	else
	{
		if( !FS_MoveFile( upstream->demotempname, upstream->demofilename ) )
			Com_Printf( "Error: Failed to rename the demo file\n" );
	}

	if( !silent )
		Com_Printf( "Stopped demo: %s\n", upstream->demofilename );

	upstream->demofilehandle = 0; // file id

	Mem_ZoneFree( upstream->demofilename );
	upstream->demofilename = NULL;
	Mem_ZoneFree( upstream->demotempname );
	upstream->demotempname = NULL;

	upstream->demorecording = qfalse;
	upstream->demoautorecording = qfalse;
}

/*
============================================================================

DEMO PLAYBACK

============================================================================
*/

/*
* TV_Upstream_NextDemo
*/
void TV_Upstream_NextDemo( const char *demoname, const char *curdemo, qboolean randomize, char **name, char **filepath )
{
	int i, j, total;
	size_t bufsize, len, dir_size;
	char *file, *buf, **match, *dir;
	const char *extension, *pattern, *p;

	*name = *filepath = NULL;

	assert( demoname );
	assert( *demoname );

	buf = NULL;
	bufsize = 0;
	total = 0;

	// check if user specified a demo pattern (e.g. "tutorials/*.wd10") or a demolist filename
	extension = COM_FileExtension( demoname );
	if( extension && !Q_stricmp( extension, APP_DEMO_EXTENSION_STR ) )
		pattern = demoname;
	else
		pattern = "";

	dir_size = strlen( "demos" ) + strlen( pattern ) + 1;
	dir = Mem_TempMalloc( dir_size );
	strcpy( dir, "demos" );

	if( *pattern )
	{
		// find first character that looks like a wildcard
		const char *last_slash = NULL;

		p = pattern;
		do
		{
			if( *p == '/' )
				last_slash = p;
			else if( *p == '?' || *p == '*' || *p == '[' )
				break;
		} while( *++p );

		// append the path part of wildcard to dir and shift the pattern
		if( last_slash )
		{
			Q_strncatz( dir, "/", dir_size );
			Q_strncatz( dir, pattern, strlen( dir ) + (last_slash - pattern) + 1 );
			pattern = last_slash + 1;
		}

		bufsize = 0;
		total = FS_GetFileListExt( dir, APP_DEMO_EXTENSION_STR, NULL, &bufsize, 0, 0 );
		if( !total )
			bufsize = 0;

		if( bufsize )
		{
			buf = Mem_TempMalloc( bufsize );
			FS_GetFileList( dir, APP_DEMO_EXTENSION_STR, buf, bufsize, 0, 0 );
		}
	}
	else
	{
		// load demolist file and pick next available demo
		int filehandle = 0, filelen = -1;

		// load list from file
		filelen = FS_FOpenFile( demoname, &filehandle, FS_READ );
		if( filehandle && filelen > 0 )
		{
			bufsize = (size_t)(filelen + 1);
			buf = Mem_TempMalloc( bufsize );
			FS_Read( buf, filelen, filehandle );
			FS_FCloseFile( filehandle );
		}

		// parse the list stripping CRLF characters
		if( buf )
		{
			p = strtok( buf, "\r\n" );
			if( p )
			{
				char *newbuf;
				size_t newbufsize;

				newbufsize = 0;
				newbuf = Mem_TempMalloc( bufsize );
				while( p != NULL )
				{
					total++;

					Q_strncpyz( newbuf + newbufsize, p, bufsize - newbufsize );
					newbufsize += strlen( p ) + 1;
					p = strtok( NULL, "\r\n" );
				}

				Mem_TempFree( buf );
				buf = newbuf;
			}
		}
	}

	if( buf )
	{
		int next;

		// get the list of demo files
		match = Mem_TempMalloc( total * sizeof( char * ) );
		total = 0;

		next = (randomize ? -1 : 0);
		for( len = 0; buf[len]; )
		{
			file = buf + len;
			len += strlen( file ) + 1;

			if( *pattern )
			{
				if( !Com_GlobMatch( pattern, file, qfalse ) )
					continue;
			}

			// avoid replays
			if( curdemo && !Q_stricmp( file, curdemo ) )
			{
				// if ordered, schedule the next map
				if( !randomize )
					next = total;
				continue;
			}

			match[total++] = file;
		}

		// pick a new random demo if possible or otherwise try the old one
		if( total )
		{
			if( next < 0 )
				next = rand() % total;

			// walk the list until we find an existing file
			// in case of pattern match, the check is not necessary though
			for( i = 0; i < total; i++ )
			{
				j = (i + next) % total;
				if( !*pattern )
				{
					extension = COM_FileExtension( match[j] );
					if( FS_FOpenFile( va( "demos/%s%s", match[j], (extension ? "" : APP_DEMO_EXTENSION_STR) ), NULL, FS_READ ) == -1 )
						continue;
				}

				*name = TempCopyString( match[j] );
				break;
			}
		}

		// fallback to current demo
		if( !*name && curdemo )
			*name = TempCopyString( curdemo );

		Mem_TempFree( match );

		// append the dir to filename, sigh..
		if( *name )
		{
			size_t filepath_size;

			filepath_size = strlen( dir ) + strlen( "/" ) + strlen( *name ) + strlen( APP_DEMO_EXTENSION_STR ) + 1;
			*filepath = Mem_TempMalloc( filepath_size );
			strcpy( *filepath, dir );
			strcat( *filepath, "/" );
			strcat( *filepath, *name );
			COM_DefaultExtension( *filepath, APP_DEMO_EXTENSION_STR, filepath_size );
		}

		Mem_TempFree( buf );
	}

	Mem_TempFree( dir );
}

/*
* TV_Upstream_StartDemo
*/
void TV_Upstream_StartDemo( upstream_t *upstream, const char *demoname, qboolean randomize )
{
	char *name, *filepath;
	int tempdemofilehandle, tempdemofilelen;

	name = filepath = NULL;
	tempdemofilehandle = 0;
	tempdemofilelen = -1;

	TV_Upstream_NextDemo( demoname, upstream->demofilename, randomize, &name, &filepath );

	if( filepath )
	{
		if( COM_ValidateRelativeFilename( filepath ) )
			tempdemofilelen = FS_FOpenFile( filepath, &tempdemofilehandle, FS_READ );	// open the demo file
	}

	TV_Upstream_StopDemo( upstream );

	if( name )
		Com_Printf( "Starting demo from %s\n", filepath );

	upstream->demoplaying = qtrue;
	upstream->demofilename = name ? TV_Upstream_CopyString( upstream, name ) : NULL;
	upstream->demofilehandle = tempdemofilehandle;
	upstream->demofilelen = tempdemofilelen;
	upstream->demorandom = randomize;
	upstream->state = CA_HANDSHAKE;
	upstream->reliable = qfalse;

	upstream->servername = TV_Upstream_CopyString( upstream, demoname );	// can be demo filename/pattern or demolist filename
	upstream->rejected = qfalse;
	upstream->lastPacketReceivedTime = tvs.realtime;	// reset the timeout limit
	upstream->multiview = qfalse;
	upstream->precacheDone = qfalse;

	if( name )
		Mem_TempFree( name );
	if( filepath )
		Mem_TempFree( filepath );
}

/*
* TV_Upstream_StopDemo
*/
void TV_Upstream_StopDemo( upstream_t *upstream )
{
	if( upstream->demofilehandle )
	{
		FS_FCloseFile( upstream->demofilehandle );
		upstream->demofilehandle = 0;
	}

	if( upstream->demofilename )
	{
		Mem_Free( upstream->demofilename );
		upstream->demofilename = NULL;
	}

	upstream->demofilelen = 0;
	upstream->demoplaying = qfalse;
}
