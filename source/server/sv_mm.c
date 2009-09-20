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
#include "../qcommon/rsa.h"
#include "../qcommon/sha1.h"

#ifdef MATCHMAKER_SUPPORT

#ifndef TCP_SUPPORT
#	ifdef _MSC_VER
#		pragma message( "TCP support needed for matchmaker" )
#	else
#		warning TCP support needed for matchmaker
#	endif
#endif

#define MM_LOCK_TIMEOUT 40000

static void SV_MM_GenerateSalt( void );
static void SV_MM_Status( void );
static void SV_MM_LoadPublicKey( void );
static void SV_MM_FreePublicKey( void );
static qboolean SV_MM_SendMsgToServer( const char *format, ... );

static void SV_MMC_Lock( msg_t *msg );
static void SV_MMC_Setup( msg_t *msg );

//================
// sv_mm_locked_t
// Contains all the data necessary for server locking
//================
typedef struct
{
	unsigned int locked;

	int clientcount;
	qbyte clientips[MAX_CLIENTS][4];
} sv_mm_locked_t;

//================
// private vars
//================
static qboolean sv_mm_initialized = qfalse;

static sv_mm_locked_t sv_mm_locked;
static rsa_context sv_mm_ctx;

static char sv_mm_salt[SALT_LEN];
static netadr_t sv_mm_address;

#ifdef TCP_SUPPORT
static incoming_t *sv_mm_connection;
#endif

//================
// public vars
//================
cvar_t *sv_mmserver;
cvar_t *sv_allowmm;

//================
// SV_MM_Init
// Initialize matchmaking components
//================
void SV_MM_Init( void )
{
	if( sv_mm_initialized )
		return;

	sv_mmserver = Cvar_Get( "mmserver", MM_SERVER_IP, CVAR_ARCHIVE );
	sv_allowmm = Cvar_Get( "sv_allowmm", "1", CVAR_ARCHIVE );

	if( !sv_tcp->integer )
	{
		Com_Printf( "TCP must be enabled for matchmaker.\n" );
		Cvar_FullSet( "sv_allowmm", "0", CVAR_READONLY, qtrue );
	}

	NET_StringToAddress( sv_mmserver->string, &sv_mm_address );
	if( sv_mm_address.type == NA_NOTRANSMIT )
	{
		Com_Printf( "Invalid matchmaker server address.\n" );
		Cvar_FullSet( "sv_allowmm", "0", CVAR_READONLY, qtrue );
	}

	if( !sv_allowmm->integer )
		return;

	Cmd_AddCommand( "mmstatus", SV_MM_Status );

	SV_MM_GenerateSalt();

	SV_MM_LoadPublicKey();

	svc.last_mmheartbeat = MM_HEARTBEAT_SECONDS * 1000;

	sv_mm_initialized = qtrue;
}

//================
// SV_MM_Shutdown
// Shutdown matchmaking stuff
//================
void SV_MM_Shutdown( void )
{
	if( !sv_mm_initialized )
		return;

	Cmd_RemoveCommand( "mmstatus" );

	// tell mm we cant host anymore because we are shutting down
	if( ( sv_mm_address.type != NA_NOTRANSMIT ) && svs.socket_udp.open )
		Netchan_OutOfBandPrint( &svs.socket_udp, &sv_mm_address, "heartbeat no " APP_PROTOCOL_VERSION_STR );

	SV_MM_FreePublicKey();

	sv_mm_initialized = qfalse;
}

//================
// SV_MM_Frame
// Called every game frame
//================
void SV_MM_Frame( void )
{
	if( !sv_mm_initialized )
		return;

	if( !sv_mm_locked.locked )
		return;

	// if all clients have quit, then we can release the server again
	// make sure mm clients have had time to connect first
	if( SV_MM_CanHost()	&& sv_mm_locked.locked + MM_LOCK_TIMEOUT < svs.realtime	)
	{
		// reset game environment to what it was before
		Com_DPrintf( "Resetting settings after matchmaker\n" );
		ge->MM_Reset();
		memset( &sv_mm_locked, 0, sizeof( sv_mm_locked ) );

		SV_MM_GenerateSalt();
	}
}

//================
// SV_MM_GetSlotCount
// Returns the number of slots that can be used for clients
//================
int SV_MM_GetSlotCount( void )
{
	int i, tvclients = 0;

	// count tv clients
	for( i = 0 ; i < sv_maxclients->integer ; i++ )
	{
		if( svs.clients[i].tvclient )
			tvclients++;
	}

	return sv_maxclients->integer - tvclients;
}

//================
// SV_MM_CanHostMM
// Returns whether server is capable of hosting a match
//================
qboolean SV_MM_CanHost( void )
{
	int i;

	if( !sv_mm_initialized )
		return qfalse;

	for( i = 0 ; i < sv_maxclients->integer ; i++ )
	{
		if( svs.clients[i].edict->r.svflags & SVF_FAKECLIENT || svs.clients[i].tvclient )
			continue;

		if( svs.clients[i].state >= CS_CONNECTING )
			return qfalse;
	}

	return qtrue;
}

//================
// SV_MM_IsLocked
// Returns whether server is locked
//================
qboolean SV_MM_IsLocked( void )
{
	return sv_mm_locked.locked != 0;
}

//================
// SV_MM_Initialized
//================
qboolean SV_MM_Initialized( void )
{
	return sv_mm_initialized;
}

//================
// SV_MM_NetAddress
//================
qboolean SV_MM_NetAddress( netadr_t *addr )
{
	if( sv_mm_address.type == NA_NOTRANSMIT )
		return qfalse;

	assert( addr );
	*addr = sv_mm_address;
	return qtrue;
}

/*
* SV_MM_Salt
*/
const char *SV_MM_Salt( void )
{
	return sv_mm_salt;
}

/*
* SV_MM_SetConnection
*/
#ifdef TCP_SUPPORT
void SV_MM_SetConnection( incoming_t *connection )
{
	sv_mm_connection = connection;
}
#endif

//================
// SV_MM_GenerateSalt
// Generates the salt
//================
static void SV_MM_GenerateSalt( void )
{
	int i;
	srand( time( NULL ) );
	for( i = 0 ; i < sizeof( sv_mm_salt ) ; i++ )
		sv_mm_salt[i] = brandom( '0', 'z' );
}

//================
// SV_MM_Status
// Outputs server's matchmaking status to console
//================
static void SV_MM_Status( void )
{
	int i;

	if( !sv_mm_initialized )
	{
		Com_Printf( "Server does not have matchmaking enabled\n" );
		return;
	}

	if( SV_MM_IsLocked() )
	{
		Com_Printf( "Server is in matchmaking mode" );
		if( SV_MM_CanHost() )
			Com_Printf( " (timeout in %d seconds)", ( MM_LOCK_TIMEOUT - ( svs.realtime - sv_mm_locked.locked ) ) / 1000 );

		Com_Printf( "\nClients (%d total):\n", sv_mm_locked.clientcount );
		for( i = 0 ; i < sv_mm_locked.clientcount ; i++ )
			Com_Printf( "  %d.%d.%d.%d\n", sv_mm_locked.clientips[i][0], sv_mm_locked.clientips[i][1], sv_mm_locked.clientips[i][2], sv_mm_locked.clientips[i][3] );
	}
	else
		Com_Printf( "Server is not in matchmaking mode\n" );
}

//================
// SV_MM_LoadPublicKey
// Loads the public key from file
//================
static void SV_MM_LoadPublicKey( void )
{
	qboolean success;
	size_t offsets[] =
	{
		CTXOFS( N ),
		CTXOFS( E ),
		0
	};

	Com_Printf( "Loading matchmaker public key... " );

	success = MM_LoadKey( &sv_mm_ctx, RSA_PUBLIC, offsets, "mm_pubkey.txt" );
	if( success )
		Com_Printf( "success!\n" );
	else
	{
		Com_Printf( "failed!\n%s\n", MM_LoadKeyError() );
		Cvar_FullSet( "sv_allowmm", "0", CVAR_READONLY, qtrue );
	}
}

//================
// SV_MM_FreePublicKey
// Frees the public key
//================
static void SV_MM_FreePublicKey( void )
{
	rsa_free( &sv_mm_ctx );
}

//================
// SV_MM_SendMsgToServer
// Sends a msg to the matchmaker
//================
static qboolean SV_MM_SendMsgToServer( const char *format, ... )
{
#ifdef TCP_SUPPORT
	va_list argptr;
	char msg[1024];
	connection_status_t status;

	if( !sv_mm_connection || !sv_mm_connection->socket.open )
		return qfalse;

	status = NET_CheckConnect( &sv_mm_connection->socket );
	if( status != CONNECTION_SUCCEEDED )
		return qfalse;

	va_start( argptr, format );
	Q_vsnprintfz( msg, sizeof( msg ), format, argptr );
	va_end( argptr );

	if( !*msg )
		return qfalse;

	Com_DPrintf( "Sending packet to matchmaker: %s\n", msg );

	if( !NET_SendPacket( &sv_mm_connection->socket, msg, strlen( msg ) + 1, &sv_mm_connection->address ) )
	{
		NET_CloseSocket( &sv_mm_connection->socket );
		sv_mm_connection->active = qfalse;

		return qfalse;
	}

	return qtrue;
#else
	return qfalse;
#endif
}

//================
// Supported commands
//================
typedef struct
{
	char *name;
	void ( *func )( msg_t *msg );
} mm_cmd_t;

static mm_cmd_t mm_cmds[] =
{
	{ "lock", SV_MMC_Lock },
	{ "setup", SV_MMC_Setup },
	
	{ NULL, NULL }
};

//================
// SV_MM_Packet
// Handle matchmaking packets
//================
void SV_MM_Packet( msg_t *msg )
{
	mm_cmd_t *cmd;
	unsigned char sha1sum[20];
	char salt[SALT_LEN];

	if( !sv_mm_initialized )
		return;

	MSG_BeginReading( msg );
	MSG_SkipData( msg, sv_mm_ctx.len );

	memset( salt, 0, sizeof( salt ) );
	MSG_ReadData( msg, salt, strlen( sv_mm_salt ) );

	// salt must be correct
	if( memcmp( salt, sv_mm_salt, sizeof( salt ) ) )
		return;

	// get sha1 sum of data
	sha1( msg->data + sv_mm_ctx.len, msg->cursize - sv_mm_ctx.len, sha1sum );

	if( rsa_pkcs1_verify( &sv_mm_ctx, RSA_PUBLIC, RSA_SHA1, 0, sha1sum, msg->data ) )
		return;

	for( cmd = mm_cmds ; cmd->name ; cmd++ )
	{
		if( !memcmp( msg->data + sv_mm_ctx.len + sizeof( salt ), cmd->name, strlen( cmd->name ) ) )
		{
			// set msg read point after command name
			MSG_SkipData( msg, strlen( cmd->name ) );
			if( cmd->func )
				cmd->func( msg );
			break;
		}
	}
}

//================
// SV_MM_ClientConnect
// When server is locked, this checks if the connecting player is
// one of the players who can connect
//================
qboolean SV_MM_ClientConnect( const netadr_t *address, char *userinfo )
{
	int i;
	netadr_t client;

	if( !sv_mm_initialized )
		return qtrue;

	if( !sv_mm_locked.locked )
		return qtrue;

	for( i = 0 ; i < sv_mm_locked.clientcount ; i++ )
	{
		memcpy( client.ip, sv_mm_locked.clientips[i], 4 );
		client.type = NA_IP;

		if( !NET_CompareBaseAddress( &client, address ) )
			continue;

		// hack a password into the userinfo, so users can connect
		// even if there is a password
		Info_SetValueForKey( userinfo, "password", Cvar_String( "password" ) );
		return qtrue;
	}

	Info_SetValueForKey( userinfo, "rejtype", va( "%d", DROP_TYPE_GENERAL ) );
	Info_SetValueForKey( userinfo, "rejflag", "0" );
	Info_SetValueForKey( userinfo, "rejmsg", "Server locked for matchmaking" );

	return qfalse;
}

//================
// SV_MMC_Lock
// Lock the server ready for matchmaking
//================
static void SV_MMC_Lock( msg_t *msg )
{
	if( !SV_MM_CanHost() )
	{
		SV_MM_SendMsgToServer( "reply lock failed" );
		return;
	}

	if( SV_MM_SendMsgToServer( "reply lock success" ) )
		sv_mm_locked.locked = svs.realtime;
}

//================
// SV_MMC_Setup
// Change gameplay settings ready for matchmaking
//================
static void SV_MMC_Setup( msg_t *msg )
{
	int i = 0;
	unsigned int len;
	char *gametype;
	float timelimit;
	int scorelimit;

	if( !SV_MM_CanHost() || sv_mm_locked.clientcount )
	{
		SV_MM_SendMsgToServer( "reply setup failed" );
		return;
	}

	len = MSG_ReadLong( msg );
	if( len <= 0 )
		return;

	gametype = ( char * )Mem_TempMalloc( len + 1 );
	MSG_ReadData( msg, gametype, len );
	// we need to check if the gametype is supported here.
	if( !*gametype )
	{
		Mem_TempFree( gametype );
		SV_MM_SendMsgToServer( "reply setup failed" );
		return;
	}

	timelimit = MSG_ReadFloat( msg );
	scorelimit = MSG_ReadLong( msg );

	if( !SV_MM_SendMsgToServer( "reply setup success" ) )
	{
		Mem_TempFree( gametype );
		return;
	}

	while( msg->readcount < msg->cursize && i < MAX_CLIENTS )
		MSG_ReadData( msg, sv_mm_locked.clientips[i++], 4 );
	sv_mm_locked.clientcount = i;
	sv_mm_locked.locked = svs.realtime;

	Com_DPrintf( "Changing server settings for matchmaker:\n" );
	Com_DPrintf( "  gametype: %s\n", gametype );
	Com_DPrintf( "  timelimit: %f\n", timelimit );
	Com_DPrintf( "  scorelimit: %d\n", scorelimit );

	ge->MM_Setup( gametype, scorelimit, timelimit, qtrue );

	Mem_TempFree( gametype );
}

#else

void SV_MM_Init( void )
{
}

void SV_MM_Shutdown( void )
{
}

void SV_MM_Frame( void )
{
}

void SV_MM_Packet( msg_t *msg )
{
}

qboolean SV_MM_Initialized( void )
{
	return qfalse;
}

qboolean SV_MM_CanHost( void )
{
	return qfalse;
}

int SV_MM_GetSlotCount( void )
{
	return 0;
}

qboolean SV_MM_IsLocked( void )
{
	return qfalse;
}

qboolean SV_MM_ClientConnect( const netadr_t *address, char *userinfo )
{
	return qtrue;
}

qboolean SV_MM_NetAddress( netadr_t *addr )
{
	addr->type = NA_NOTRANSMIT;
	return qfalse;
}

const char *SV_MM_Salt( void )
{
	static const char salt[1] = { '\0' };
	return salt;
}

#ifdef TCP_SUPPORT
void SV_MM_SetConnection( incoming_t *connection )
{
	connection = NULL;
}
#endif

#endif
