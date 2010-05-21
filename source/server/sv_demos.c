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

#include "server.h"

#define SV_DEMO_DIR va( "demos/server%s%s", sv_demodir->string[0] ? "/" : "", sv_demodir->string[0] ? sv_demodir->string : "" )

/*
==================
SV_Demo_WriteMessage

Writes given message to the demofile
==================
*/
static void SV_Demo_WriteMessage( msg_t *msg, int client_num )
{
	assert( svs.demos[client_num].file );
	if( !svs.demos[client_num].file )
		return;

	SNAP_RecordDemoMessage( svs.demos[client_num].file, msg, 0 );
}

//================
//SV_Demo_WriteStartMessages
//================
static void SV_Demo_WriteStartMessages( int client_num )
{
	SNAP_BeginDemoRecording( svs.demos[client_num].file, svs.spawncount, svc.snapFrameTime, sv.mapname, SV_BITFLAGS_RELIABLE, 
		svs.purelist, sv.configstrings[0], sv.baselines );
}

//================
//SV_Demo_WriteSnap
//================
void SV_Demo_WriteSnap( void )
{
	int i;
	msg_t msg;
	qbyte msg_buffer[MAX_MSGLEN];

	if( !svs.demos[MAX_CLIENTS].file )
		return;

	for( i = 0; i < sv_maxclients->integer; i++ )
	{
		if( svs.clients[i].state >= CS_SPAWNED && svs.clients[i].edict &&
			!( svs.clients[i].edict->r.svflags & SVF_NOCLIENT ) )
			break;
	}
	if( i == sv_maxclients->integer )
	{                               // FIXME
		Com_Printf( "No players left, stopping server side demo recording\n" );
		SV_Demo_Stop_f();
		return;
	}

	MSG_Init( &msg, msg_buffer, sizeof( msg_buffer ) );

	SV_BuildClientFrameSnap( &svs.demos[MAX_CLIENTS].client );

	SV_WriteFrameSnapToClient( &svs.demos[MAX_CLIENTS].client, &msg );

	SV_AddReliableCommandsToMessage( &svs.demos[MAX_CLIENTS].client, &msg );

	SV_Demo_WriteMessage( &msg, MAX_CLIENTS );

	svs.demos[MAX_CLIENTS].duration = svs.gametime - svs.demos[MAX_CLIENTS].basetime;
	svs.demos[MAX_CLIENTS].client.lastframe = sv.framenum; // FIXME: is this needed?
}

//================
//SV_Demo_WriteSingleClientsSnap
//================
void SV_Demo_WriteSingleClientsSnap( void )
{
	int client_num;
	for( client_num = 0; client_num < sv_maxclients->integer; client_num++ )
	{
		SV_Demo_WriteSingleClientSnap( client_num );
	}
}

//================
//SV_Demo_WriteSingleClientSnap
//================
void SV_Demo_WriteSingleClientSnap( int client_num )
{
	msg_t msg;
	qbyte msg_buffer[MAX_MSGLEN];

	if( !svs.demos[client_num].file )
		return;

	if( !( svs.clients[client_num].state >= CS_SPAWNED && svs.clients[client_num].edict &&
		!( svs.clients[client_num].edict->r.svflags & SVF_NOCLIENT ) ) ) {

		Com_Printf( "Player has left, stopping client demo recording\n" );
		SV_Demo_StopSingleClient_f(client_num); // TODO
		return;
	}
	MSG_Init( &msg, msg_buffer, sizeof( msg_buffer ) );

	SV_BuildClientFrameSnap( &svs.demos[client_num].client );

	SV_WriteFrameSnapToClient( &svs.demos[client_num].client, &msg );

	SV_AddReliableCommandsToMessage( &svs.demos[client_num].client, &msg );

	SV_Demo_WriteMessage( &msg, client_num );

	svs.demos[client_num].duration = svs.gametime - svs.demos[client_num].basetime;
	svs.demos[client_num].client.lastframe = sv.framenum; // FIXME: is this needed?
}

//================
//SV_Demo_InitClient
//================
static void SV_Demo_InitClient( int client_num )
{
	memset( &svs.demos[client_num].client, 0, sizeof( svs.demos[client_num].client ) );

	svs.demos[client_num].client.mv = qtrue;
	svs.demos[client_num].client.reliable = qtrue;

	svs.demos[client_num].client.reliableAcknowledge = 0;
	svs.demos[client_num].client.reliableSequence = 0;
	svs.demos[client_num].client.reliableSent = 0;
	memset( svs.demos[client_num].client.reliableCommands, 0, sizeof( svs.demos[client_num].client.reliableCommands ) );

	svs.demos[client_num].client.lastframe = sv.framenum - 1;
	svs.demos[client_num].client.nodelta = qfalse;
}

/*
==============
SV_Demo_Start_f

Begins server demo recording.
==============
*/
void SV_Demo_Start_f( void )
{
	int demofilename_size, i;

	if( Cmd_Argc() < 2 )
	{
		Com_Printf( "Usage: serverrecord <demoname>\n" );
		return;
	}

	if( svs.demos[MAX_CLIENTS].file )
	{
		Com_Printf( "Already recording\n" );
		return;
	}

	if( sv.state != ss_game )
	{
		Com_Printf( "Must be in a level to record\n" );
		return;
	}

	for( i = 0; i < sv_maxclients->integer; i++ )
	{
		if( svs.clients[i].state >= CS_SPAWNED && svs.clients[i].edict &&
			!( svs.clients[i].edict->r.svflags & SVF_NOCLIENT ) )
			break;
	}
	if( i == sv_maxclients->integer )
	{
		Com_Printf( "No players in game, can't record a demo\n" );
		return;
	}

	//
	// open the demo file
	//

	// real name
	demofilename_size =
		sizeof( char ) * ( strlen( SV_DEMO_DIR ) + 1 + strlen( Cmd_Argv( 1 ) ) + strlen( "_auto" ) + 4 + strlen( APP_DEMO_EXTENSION_STR ) + 1 );
	svs.demos[MAX_CLIENTS].filename = Mem_ZoneMalloc( demofilename_size );

	Q_snprintfz( svs.demos[MAX_CLIENTS].filename, demofilename_size, "%s/%s", SV_DEMO_DIR, Cmd_Argv( 1 ) );

	if( Cmd_Argc() == 3 && atoi( Cmd_Argv( 2 ) ) > 0 )
		Q_strncatz( svs.demos[MAX_CLIENTS].filename, va( "_auto%4i", atoi( Cmd_Argv( 2 ) ) ), demofilename_size );

	COM_SanitizeFilePath( svs.demos[MAX_CLIENTS].filename );

	if( !COM_ValidateRelativeFilename( svs.demos[MAX_CLIENTS].filename ) )
	{
		Mem_ZoneFree( svs.demos[MAX_CLIENTS].filename );
		svs.demos[MAX_CLIENTS].filename = NULL;
		Com_Printf( "Invalid filename.\n" );
		return;
	}

	COM_DefaultExtension( svs.demos[MAX_CLIENTS].filename, APP_DEMO_EXTENSION_STR, demofilename_size );

	// temp name
	demofilename_size = sizeof( char ) * ( strlen( svs.demos[MAX_CLIENTS].filename ) + strlen( ".rec" ) + 1 );
	svs.demos[MAX_CLIENTS].tempname = Mem_ZoneMalloc( demofilename_size );
	Q_snprintfz( svs.demos[MAX_CLIENTS].tempname, demofilename_size, "%s.rec", svs.demos[MAX_CLIENTS].filename );

	// open it
	if( FS_FOpenFile( svs.demos[MAX_CLIENTS].tempname, &svs.demos[MAX_CLIENTS].file, FS_WRITE ) == -1 )
	{
		Com_Printf( "Error: Couldn't open file: %s\n", svs.demos[MAX_CLIENTS].tempname );
		Mem_ZoneFree( svs.demos[MAX_CLIENTS].filename );
		svs.demos[MAX_CLIENTS].filename = NULL;
		Mem_ZoneFree( svs.demos[MAX_CLIENTS].tempname );
		svs.demos[MAX_CLIENTS].tempname = NULL;
		return;
	}

	Com_Printf( "Recording server demo: %s\n", svs.demos[MAX_CLIENTS].filename );

	SV_Demo_InitClient(MAX_CLIENTS);

	// write serverdata, configstrings and baselines
	svs.demos[MAX_CLIENTS].duration = 0;
	svs.demos[MAX_CLIENTS].basetime = svs.gametime;
	SV_Demo_WriteStartMessages(MAX_CLIENTS);

	// write one nodelta frame
	svs.demos[MAX_CLIENTS].client.nodelta = qtrue;
	SV_Demo_WriteSnap();
	svs.demos[MAX_CLIENTS].client.nodelta = qfalse;
}

/*
==============
SV_Demo_Stop
==============
*/
void SV_Demo_Stop( qboolean cancel )
{
	if( !svs.demos[MAX_CLIENTS].file )
	{
		Com_Printf( "No server demo recording in progress\n" );
		return;
	}

	if( cancel )
	{
		Com_Printf( "Canceled server demo recording: %s\n", svs.demos[MAX_CLIENTS].filename );
	}
	else
	{
		SNAP_StopDemoRecording( svs.demos[MAX_CLIENTS].file, svs.demos[MAX_CLIENTS].basetime, svs.demos[MAX_CLIENTS].duration );
		Com_Printf( "Stopped server demo recording: %s\n", svs.demos[MAX_CLIENTS].filename );
	}

	FS_FCloseFile( svs.demos[MAX_CLIENTS].file );
	svs.demos[MAX_CLIENTS].file = 0;
	svs.demos[MAX_CLIENTS].basetime = svs.demos[MAX_CLIENTS].duration = 0;

	if( cancel )
	{
		if( !FS_RemoveFile( svs.demos[MAX_CLIENTS].tempname ) )
			Com_Printf( "Error: Failed to delete the temporary server demo file\n" );
	}
	else
	{
		if( !FS_MoveFile( svs.demos[MAX_CLIENTS].tempname, svs.demos[MAX_CLIENTS].filename ) )
			Com_Printf( "Error: Failed to rename the server demo file\n" );
	}

	SNAP_FreeClientFrames( &svs.demos[MAX_CLIENTS].client );

	Mem_ZoneFree( svs.demos[MAX_CLIENTS].filename );
	svs.demos[MAX_CLIENTS].filename = NULL;
	Mem_ZoneFree( svs.demos[MAX_CLIENTS].tempname );
	svs.demos[MAX_CLIENTS].tempname = NULL;
}

/*
==============
SV_Demo_Stop
==============
*/
void SV_Demo_StopSingleClient( int client_num, qboolean cancel )
{
	if( !svs.demos[client_num].file )
	{
		Com_Printf( "No client demo recording in progress\n" );
		return;
	}

	if( cancel )
	{
		Com_Printf( "Canceled client demo recording: %s\n", svs.demos[client_num].filename );
	}
	else
	{
		SNAP_StopDemoRecording( svs.demos[client_num].file, svs.demos[client_num].basetime, svs.demos[client_num].duration );
		Com_Printf( "Stopped client demo recording: %s\n", svs.demos[client_num].filename );
	}

	FS_FCloseFile( svs.demos[client_num].file );
	svs.demos[client_num].file = 0;
	svs.demos[client_num].basetime = svs.demos[client_num].duration = 0;

	if( cancel )
	{
		if( !FS_RemoveFile( svs.demos[client_num].tempname ) )
			Com_Printf( "Error: Failed to delete the temporary client demo file\n" );
	}
	else
	{
		if( !FS_MoveFile( svs.demos[client_num].tempname, svs.demos[client_num].filename ) )
			Com_Printf( "Error: Failed to rename the client demo file\n" );
	}

	SNAP_FreeClientFrames( &svs.demos[client_num].client );

	Mem_ZoneFree( svs.demos[client_num].filename );
	svs.demos[client_num].filename = NULL;
	Mem_ZoneFree( svs.demos[client_num].tempname );
	svs.demos[client_num].tempname = NULL;
}

/*
==============
SV_Demo_Stop_f

Console command for stopping server demo recording.
==============
*/
void SV_Demo_Stop_f( void )
{
	SV_Demo_Stop( qfalse );
}

/*
==============
SV_Demo_Cancel_f

Cancels the server demo recording (stop, remove file)
==============
*/
void SV_Demo_Cancel_f( void )
{
	SV_Demo_Stop( qtrue );
}

/*
==============
SV_Demo_Purge_f

Removes the server demo files
==============
*/
void SV_Demo_Purge_f( void )
{
	char *buffer;
	char *p, *s, num[8];
	char path[256];
	size_t extlen, length, bufSize;
	unsigned int i, numdemos, numautodemos, maxautodemos;

	if( Cmd_Argc() > 2 )
	{
		Com_Printf( "Usage: serverrecordpurge [maxautodemos]\n" );
		return;
	}

	maxautodemos = 0;
	if( Cmd_Argc() == 2 )
	{
		maxautodemos = atoi( Cmd_Argv( 1 ) );
		if( maxautodemos < 0 )
			maxautodemos = 0;
	}

	numdemos = FS_GetFileListExt( SV_DEMO_DIR, APP_DEMO_EXTENSION_STR, NULL, &bufSize, 0, 0 );
	if( !numdemos )
		return;

	extlen = strlen( APP_DEMO_EXTENSION_STR );
	buffer = Mem_TempMalloc( bufSize );
	FS_GetFileList( SV_DEMO_DIR, APP_DEMO_EXTENSION_STR, buffer, bufSize, 0, 0 );

	numautodemos = 0;
	s = buffer;
	for( i = 0; i < numdemos; i++, s += length + 1 )
	{
		length = strlen( s );
		if( length < strlen( "_auto9999" ) + extlen )
			continue;

		p = s + length - strlen( "_auto9999" ) - extlen;
		if( strncmp( p, "_auto", strlen( "_auto" ) ) )
			continue;

		p += strlen( "_auto" );
		Q_snprintfz( num, sizeof( num ), "%04i", atoi( p ) );
		if( strncmp( p, num, 4 ) )
			continue;

		numautodemos++;
	}

	if( numautodemos <= maxautodemos )
		return;

	s = buffer;
	for( i = 0; i < numdemos; i++, s += length + 1 )
	{
		length = strlen( s );
		if( length < strlen( "_auto9999" ) + extlen )
			continue;

		p = s + length - strlen( "_auto9999" ) - extlen;
		if( strncmp( p, "_auto", strlen( "_auto" ) ) )
			continue;

		p += strlen( "_auto" );
		Q_snprintfz( num, sizeof( num ), "%04i", atoi( p ) );
		if( strncmp( p, num, 4 ) )
			continue;

		Q_snprintfz( path, sizeof( path ), "%s/%s", SV_DEMO_DIR, s );
		Com_Printf( "Removing old autorecord demo: %s\n", path );
		if( !FS_RemoveFile( path ) )
		{
			Com_Printf( "Error, couldn't remove file: %s\n", path );
			continue;
		}

		if( --numautodemos == maxautodemos )
			break;
	}

	Mem_TempFree( buffer );
}

//=================
//SV_DemoList_f
//=================
#define DEMOS_PER_VIEW	30
void SV_DemoList_f( client_t *client )
{
	char message[MAX_STRING_CHARS];
	char numpr[16];
	char buffer[MAX_STRING_CHARS];
	char *s, *p;
	size_t j, length, length_escaped, pos, extlen;
	int numdemos, i, start = -1, end, k;

	if( client->state < CS_SPAWNED )
		return;

	if( Cmd_Argc() > 2 )
	{
		SV_AddGameCommand( client, "pr \"Usage: demolist [starting position]\n\"" );
		return;
	}

	if( Cmd_Argc() == 2 )
	{
		start = atoi( Cmd_Argv( 1 ) ) - 1;
		if( start < 0 )
		{
			SV_AddGameCommand( client, "pr \"Usage: demolist [starting position]\n\"" );
			return;
		}
	}

	Q_strncpyz( message, "pr \"Available demos:\n----------------\n", sizeof( message ) );

	numdemos = FS_GetFileList( SV_DEMO_DIR, APP_DEMO_EXTENSION_STR, NULL, 0, 0, 0 );
	if( numdemos )
	{
		if( start < 0 )
			start = max( 0, numdemos - DEMOS_PER_VIEW );
		else if( start > numdemos - 1 )
			start = numdemos - 1;

		if( start > 0 )
			Q_strncatz( message, "...\n", sizeof( message ) );

		end = start + DEMOS_PER_VIEW;
		if( end > numdemos )
			end = numdemos;

		extlen = strlen( APP_DEMO_EXTENSION_STR );

		i = start;
		do
		{
			if( ( k = FS_GetFileList( SV_DEMO_DIR, APP_DEMO_EXTENSION_STR, buffer, sizeof( buffer ), i, end ) ) == 0 )
			{
				i++;
				continue;
			}

			for( s = buffer; k > 0; k--, s += length+1, i++ )
			{
				length = strlen( s );

				length_escaped = length;
				p = s;
				while( ( p = strchr( p, '\\' ) ) )
					length_escaped++;

				Q_snprintfz( numpr, sizeof( numpr ), "%i: ", i+1 );
				if( strlen( message ) + strlen( numpr ) + length_escaped - extlen + 1 + 5 >= sizeof( message ) )
				{
					Q_strncatz( message, "\"", sizeof( message ) );
					SV_AddGameCommand( client, message );

					Q_strncpyz( message, "pr \"", sizeof( message ) );
					if( strlen( "demoget " ) + strlen( numpr ) + length_escaped - extlen + 1 + 5 >= sizeof( message ) )
						continue;
				}

				Q_strncatz( message, numpr, sizeof( message ) );
				pos = strlen( message );
				for( j = 0; j < length - extlen; j++ )
				{
					assert( s[j] != '\\' );
					if( s[j] == '"' )
						message[pos++] = '\\';
					message[pos++] = s[j];
				}
				message[pos++] = '\n';
				message[pos] = '\0';
			}
		}
		while( i < end );

		if( end < numdemos )
			Q_strncatz( message, "...\n", sizeof( message ) );
	}
	else
	{
		Q_strncatz( message, "none\n", sizeof( message ) );
	}

	Q_strncatz( message, "\"", sizeof( message ) );

	SV_AddGameCommand( client, message );
}

//=================
//SV_DemoGet_f
//
// Responds to clients demoget request with: demoget "filename"
// If nothing is found, responds with demoget without filename, so client knowns it wasn't found
//=================
void SV_DemoGet_f( client_t *client )
{
	int num, numdemos;
	char message[MAX_STRING_CHARS];
	char buffer[MAX_STRING_CHARS];
	char *s, *p;
	size_t j, length, length_escaped, pos, pos_bak, msglen;

	if( client->state < CS_SPAWNED )
		return;
	if( Cmd_Argc() != 2 )
		return;

	Q_strncpyz( message, "demoget \"", sizeof( message ) );
	Q_strncatz( message, SV_DEMO_DIR, sizeof( message ) );
	msglen = strlen( message );
	message[msglen++] = '/';

	pos = pos_bak = msglen;

	numdemos = FS_GetFileList( SV_DEMO_DIR, APP_DEMO_EXTENSION_STR, NULL, 0, 0, 0 );
	if( numdemos )
	{
		if( Cmd_Argv( 1 )[0] == '.' )
			num = numdemos - strlen( Cmd_Argv( 1 ) );
		else
			num = atoi( Cmd_Argv( 1 ) ) - 1;
		clamp( num, 0, numdemos - 1 );

		numdemos = FS_GetFileList( SV_DEMO_DIR, APP_DEMO_EXTENSION_STR, buffer, sizeof( buffer ), num, num+1 );
		if( numdemos )
		{
			s = buffer;
			length = strlen( buffer );

			length_escaped = length;
			p = s;
			while( ( p = strchr( p, '\\' ) ) )
				length_escaped++;

			if( msglen + length_escaped + 1 + 5 < sizeof( message ) )
			{
				for( j = 0; j < length; j++ )
				{
					assert( s[j] != '\\' );
					if( s[j] == '"' )
						message[pos++] = '\\';
					message[pos++] = s[j];
				}
			}
		}
	}

	if( pos == pos_bak )
		return;

	message[pos++] = '"';
	message[pos] = '\0';

	SV_AddGameCommand( client, message );
}

// racesow

/*
==============
SV_Demo_StartSingleClient_f

Begins client demo recording.
==============
*/
void SV_Demo_StartSingleClient_f( void )
{
	int demofilename_size, client_num;

	if (!sv_clientRecord->integer)
	{
		Com_Printf( "clientrecord is disabled\n" );
		return;
	}

	if( Cmd_Argc() < 3 )
	{
		Com_Printf( "Usage: clientrecord <demoname> <playernum>\n" );
		return;
	}

	client_num = atoi(Cmd_Argv(2));

	if( svs.demos[client_num].file )
	{
		Com_Printf( va("Already recording for client %d\n", client_num) );
		return;
	}

	if( sv.state != ss_game )
	{
		Com_Printf( "Client must be in a level to record\n" );
		return;
	}

	if( !( svs.clients[client_num].state >= CS_SPAWNED && svs.clients[client_num].edict &&
		!( svs.clients[client_num].edict->r.svflags & SVF_NOCLIENT ) ) ) {

		Com_Printf( "Player not in game, can't record a demo\n" );
		return;
	}

	//
	// open the demo file
	//

	// real name
	demofilename_size =
		sizeof( char ) * ( strlen( SV_DEMO_DIR ) + 1 + strlen( Cmd_Argv( 1 ) ) + 4 + strlen( APP_DEMO_EXTENSION_STR ) + 1 );
	svs.demos[client_num].filename = Mem_ZoneMalloc( demofilename_size );

	Q_snprintfz( svs.demos[client_num].filename, demofilename_size, "%s/%s", SV_DEMO_DIR, Cmd_Argv( 1 ) );

	COM_SanitizeFilePath( svs.demos[client_num].filename );

	if( !COM_ValidateRelativeFilename( svs.demos[client_num].filename ) )
	{
		Mem_ZoneFree( svs.demos[client_num].filename );
		svs.demos[client_num].filename = NULL;
		Com_Printf( "Invalid filename.\n" );
		return;
	}

	COM_DefaultExtension( svs.demos[client_num].filename, APP_DEMO_EXTENSION_STR, demofilename_size );

	// temp name
	demofilename_size = sizeof( char ) * ( strlen( svs.demos[client_num].filename ) + strlen( ".rec" ) + 1 );
	svs.demos[client_num].tempname = Mem_ZoneMalloc( demofilename_size );
	Q_snprintfz( svs.demos[client_num].tempname, demofilename_size, "%s.rec", svs.demos[client_num].filename );

	// open it
	if( FS_FOpenFile( svs.demos[client_num].tempname, &svs.demos[client_num].file, FS_WRITE ) == -1 )
	{
		Com_Printf( "Error: Couldn't open file: %s\n", svs.demos[client_num].tempname );
		Mem_ZoneFree( svs.demos[client_num].filename );
		svs.demos[client_num].filename = NULL;
		Mem_ZoneFree( svs.demos[client_num].tempname );
		svs.demos[client_num].tempname = NULL;
		return;
	}

	Com_Printf( "Recording client demo: %s\n", svs.demos[client_num].filename );

	SV_Demo_InitClient(client_num);
	svs.demos[client_num].client.mv = qtrue;
	svs.demos[client_num].client.reliable = qtrue;

	// write serverdata, configstrings and baselines
	svs.demos[client_num].duration = 0;
	svs.demos[client_num].basetime = svs.gametime;

	SV_Demo_WriteStartMessages(client_num);

	// write one nodelta frame
	svs.demos[client_num].client.nodelta = qtrue;
	SV_Demo_WriteSingleClientSnap( client_num );
	svs.demos[client_num].client.nodelta = qfalse;
}

/*
==============
SV_Demo_StopSingleClient_f

Console command for stopping client demo recording.
==============
*/
void SV_Demo_StopSingleClient_f( int client_num )
{
	if( Cmd_Argc() > 2 )
	{
		Com_Printf( "Usage: clientrecordstop <clientnum>\n" );
		return;
	}

	SV_Demo_StopSingleClient( atoi(Cmd_Argv(1)), qfalse );
}

// !racesow
