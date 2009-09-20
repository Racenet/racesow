/*
   Copyright (C) 2007 Will Franklin.

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

mm_static_t mms;

static qboolean mm_initialized = qfalse;

static cvar_t *mm_ip;
static cvar_t *mm_port;

mempool_t *mm_mempool;

static void MM_LoadPrivateKey( void );
static void MM_FreePrivateKey( void );

static void MM_CheckSockets( void );
static void MM_ReadPacket( socket_t *socket );

//================
// MM_Init
// Initialize matchmaking components
//================
void MM_Init( void )
{
	netadr_t address;
	
	if( mm_initialized )
		return;

	Cmd_AddCommand( "matchlist", MM_MatchList );
	Cmd_AddCommand( "serverlist", MM_ServerList );
	Cmd_AddCommand( "clearmatches", MM_ClearMatches );
	Cmd_AddCommand( "connectionlist", MM_ConnectionList );

	mm_ip = Cvar_Get( "mm_ip", "", CVAR_ARCHIVE );
	mm_port = Cvar_Get( "mm_port", va( "%d", PORT_MATCHMAKER ), CVAR_ARCHIVE );

	mm_mempool = Mem_AllocPool( NULL, "Matchmaker" );

	memset( &mms, 0, sizeof( mm_static_t ) );

	NET_StringToAddress( mm_ip->string, &address );
	address.port = BigShort( mm_port->integer );
	if( !NET_OpenSocket( &mms.socket_udp, SOCKET_UDP, &address, qtrue ) )
		Com_Error( ERR_FATAL, "Couldn't open UDP socket: %s\n", NET_ErrorString() );

  if( !NET_OpenSocket( &mms.socket_tcp, SOCKET_TCP, &address, qtrue ) )
    Com_Error( ERR_FATAL, "Couldn't open TCP socket: %s\n", NET_ErrorString() );
  else if( !NET_Listen( &mms.socket_tcp ) )
    Com_Error( ERR_FATAL, "Couldn't listen to TCP socket: %s\n", NET_ErrorString() );

	MM_LoadPrivateKey();

	DB_Init();
	Auth_Init();

	mm_initialized = qtrue;
}

//================
// MM_Shutdown
// Shutdown matchmaking components
//================
void MM_Shutdown( void )
{
	if( !mm_initialized )
		return;

	Cmd_RemoveCommand( "matchlist" );
	Cmd_RemoveCommand( "serverlist" );
	Cmd_RemoveCommand( "clearmatches" );
	Cmd_RemoveCommand( "connectionlist" );

	DB_Shutdown();
	Auth_Shutdown();

	MM_FreePrivateKey();

	MM_ShutdownConnections();
	NET_CloseSocket( &mms.socket_udp );
	NET_CloseSocket( &mms.socket_tcp );

	Mem_FreePool( &mm_mempool );

	memset( &mms, 0, sizeof( mm_static_t ) );

	mm_initialized = qfalse;
}

//================
// MM_Frame
// Runs a matchmaker frame
//================
void MM_Frame( const int realmsec )
{
	mms.frametime = realmsec;

	MM_CheckConnections();
	MM_CheckSockets();

	MM_CheckGameServers();
	MM_CheckMatches();

	MM_SendConnectionPackets();

	Sys_Sleep(5);
}

//================
// MM_LoadPrivateKey
// Loads the private key from file
//================
static void MM_LoadPrivateKey( void )
{
	qboolean success;
	size_t offsets[] = 
	{
		CTXOFS( N ),
		CTXOFS( E ),
		CTXOFS( D ),
		CTXOFS( P ),
		CTXOFS( Q ),
		CTXOFS( DP ),
		CTXOFS( DQ ),
		CTXOFS( QP ),
		0
	};

	Com_Printf( "Loading private key... " );

	success = MM_LoadKey( &mms.ctx, RSA_PRIVATE, offsets, "mm_privkey.txt" );
	if( success )
		Com_Printf( "success!\n" );
	else
		Com_Printf( "failed!\n%s\n", MM_LoadKeyError() );
}

//================
// MM_FreePrivateKey
// Frees the private key
//================
static void MM_FreePrivateKey( void )
{
	rsa_free( &mms.ctx );
}

//================
// MM_CheckSockets
// Check which sockets need action
//================
static void MM_CheckSockets( void )
{
	socket_t **sockets;
	int count;
	mm_connection_t *conn;

	count = 1; // for udp
	// count connections
	for( conn = mms.connections ; conn ; conn = conn->next )
		count++;

	sockets = ( socket_t ** )MM_Malloc( sizeof( socket_t * ) * ( count + 1 ) );

	sockets[0] = &mms.socket_udp;
	sockets[count] = NULL;
	// add connections
	for( conn = mms.connections ; conn ; conn = conn->next )
		sockets[--count] = &conn->socket;

	NET_Monitor( 10, sockets, MM_ReadPacket, NULL );
	MM_Free( sockets );
}

//================
// MM_ReadPackets
// Read an incoming packet from a socket
//================
static void MM_ReadPacket( socket_t *socket )
{
	int ret;
	netadr_t address;
	mm_connection_t *conn = NULL;

	msg_t msg;
	qbyte msgData[MAX_MSGLEN];

	memset( &address, 0, sizeof( address ) );
	memset( msgData, 0, sizeof( msgData ) );

	if( !socket->open )
		return;

	if( socket->type == SOCKET_TCP )
	{
		// check if this socket has a connection
		for( conn = mms.connections ; conn ; conn = conn->next )
		{
			if( socket == &conn->socket )
				break;
		}

		// no connection for a TCP socket
		if( !conn )
			return;
	}

	MSG_Init( &msg, msgData, sizeof( msgData ) );
	MSG_Clear( &msg );

	// read packet
	ret = NET_GetPacket( socket, &address, &msg );

	// error
	if( ret < 0 )
	{
		Com_DPrintf( "NET_GetPacket Error: %s\n", NET_ErrorString() );
		if( conn )
			conn->killtime = 0;
		return;
	}

	// not an error, but equally unwanted
	if( !ret )
		return;

	// keep the connection alive
	if( conn )
		conn->killtime = CONNECTION_KILL_TIME;

	Com_DPrintf( "Packet from %s: %s\n", NET_AddressToString( &address ), msg.data );

	if( socket->type == SOCKET_TCP )
		MM_ConnectionPacket( conn, &msg );
	else
		MM_ConnectionlessPacket( socket, &address, &msg );
}
