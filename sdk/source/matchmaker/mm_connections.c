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

static void MM_FreeConnection( mm_connection_t *conn );
static qboolean MM_SendConnectionPacketEx( mm_connection_t *conn, const void *data, size_t len, qboolean usequeue );
static char *MM_ConnectionString( mm_connection_t *conn );

//================
// MM_CheckConnections
// Remove old TCP connections and accept new ones
//================
void MM_CheckConnections( void )
{
	int ret;
	socket_t socket;
	netadr_t address;
	mm_connection_t	*conn = mms.connections, *next;

	if( !mms.socket_tcp.open )
		return;

	// free any old connections
	while( conn )
	{
		conn->killtime -= mms.frametime;
		if( conn->killtime <= 0 )
		{
			next = conn->next;
			MM_FreeConnection( conn );
			conn = next;
			continue;
		}

		conn = conn->next;
	}

	// accept new connections
	while( ( ret = NET_Accept( &mms.socket_tcp, &socket, &address ) ) )
	{
		if( ret == -1 )
		{
			Com_Printf( "NET_Accept: Error: %s", NET_ErrorString() );
			continue;
		}

		Com_DPrintf( "Connection accepted from %s\n", NET_AddressToString( &address ) );

		MM_CreateConnection( &socket, &address, NULL );
	}

	for( conn = mms.connections; conn ; conn = conn->next )
	{
		// set socket->connected status
		if( conn->socket.open )
			NET_CheckConnect( &conn->socket ); 
	}
}

void MM_ShutdownConnections( void )
{
	mm_connection_t *conn = mms.connections, *next;

	while( conn )
	{
		next = conn->next;
		MM_FreeConnection( conn );
		conn = next;
	}
}

//================
// MM_CreateConnection
// Create a connection and add it to the list of connections
//================
mm_connection_t *MM_CreateConnection( socket_t *socket, netadr_t *address, void ( *killcb )( mm_connection_t *conn ) )
{
	mm_connection_t *conn;

	if( !address )
		return NULL;

	conn = MM_Malloc( sizeof( mm_connection_t ) );
	if( socket )
		conn->socket = *socket;
	conn->address = *address;
	conn->killtime = CONNECTION_KILL_TIME;
	conn->killcb = killcb;

	if( mms.connections )
		mms.connections->prev = conn;
	conn->next = mms.connections;
	mms.connections = conn;

	return conn;
}

//================
// MM_FreeConnection
// Frees the connection
//================
static void MM_FreeConnection( mm_connection_t *conn )
{
	mm_packet_t *packet, *next;

	if( !conn )
		return;
	
	Com_DPrintf( "Discarded connection with %s\n", MM_ConnectionString( conn ) );

	if( conn->killcb )
		conn->killcb( conn );

	NET_CloseSocket( &conn->socket );

	// remove from list of connections
	if( conn->prev )
		conn->prev->next = conn->next;
	else
		mms.connections = conn->next;
	if( conn->next )
		conn->next->prev = conn->prev;

	// free packets
	packet = conn->packets;
	while( packet )
	{
		next = packet->next;

		MM_Free( packet->msg.data );
		MM_Free( packet );

		packet = next;
	}

	MM_Free( conn );
}

//================
// MM_OpenConnectionSocket
// Opens a socket using the given connection
//================
qboolean MM_OpenConnectionSocket( mm_connection_t *conn )
{
	netadr_t address;

	if( !conn )
		return qfalse;

	memset( &address, 0, sizeof( netadr_t ) );
	address.type = NA_IP;

	if( conn->socket.open )
		NET_CloseSocket( &conn->socket );

	if( !NET_OpenSocket( &conn->socket, SOCKET_TCP, &address, qfalse ) )
		return qfalse;

	if( NET_Connect( &conn->socket, &conn->address ) == CONNECTION_FAILED )
		return qfalse;

	return qtrue;
}

//================
// MM_SendConnectionPacket
// Send a packet to a known connection, add it to the list of packets if packet won't send
//================
qboolean MM_SendConnectionPacket( mm_connection_t *conn, const void *data, size_t len )
{
	return MM_SendConnectionPacketEx( conn, data, len, qtrue );
}

//================
// MM_SendConnectionPacketEx
// Attempts to send a packet, adds it to the connection's packet list if
// sending failed (and usequeue is true).
// If connection fails it attempts to reopen the connection if it was
// created by the matchmaker
//================
static qboolean MM_SendConnectionPacketEx( mm_connection_t *conn, const void *data, size_t len, qboolean usequeue )
{
	mm_packet_t *packet, *end;
	connection_status_t status;

	if( !conn || !data || !len )
		return qfalse;

	if( !conn->socket.open )
		return qfalse;

	status = NET_CheckConnect( &conn->socket );
	if( status == CONNECTION_FAILED )
	{
		conn->killtime = 0;
		return qfalse;
	}

	if( status == CONNECTION_SUCCEEDED )
	{
		if( NET_SendPacket( &conn->socket, data, len, &conn->address ) )
		{
			Com_DPrintf( "Packet sent to %s\n", MM_ConnectionString( conn ) );
			return qtrue;
		}
		else
		{
			conn->killtime = 0;
			return qfalse;
		}
	}

	if( !usequeue )
		return qfalse;

	Com_DPrintf( "Packet queued for %s\n", MM_ConnectionString( conn ) );

	packet = ( mm_packet_t * )MM_Malloc( sizeof( mm_packet_t ) );
	MSG_Init( &packet->msg, ( qbyte * )MM_Malloc( len ), len );
	MSG_WriteData( &packet->msg, data, len );

	if( conn->packets )
	{
		// go to the end of the list so packets are in order
		for( end = conn->packets ; end->next ; end = end->next );
		end->next = packet;
	}
	else
		conn->packets = packet;

	return qtrue;
}

static char *MM_ConnectionString( mm_connection_t *conn )
{
	return va( "%s (%s)", NET_AddressToString( &conn->address ), &conn->socket.server ? "client" : "server" );
}

//================
// MM_SendConnectionPackets
// Sends any connection packets which need sending
//================
void MM_SendConnectionPackets( void )
{
	mm_connection_t *conn;
	mm_packet_t *packet, *next;

	for( conn = mms.connections ; conn ; conn = conn->next )
	{
		packet = conn->packets;
		next = NULL;

		// send queued packets
		while( packet )
		{
			next = packet->next;

			if( !MM_SendConnectionPacketEx( conn, packet->msg.data, packet->msg.cursize, qfalse ) )
				break;

			MM_Free( packet->msg.data );
			MM_Free( packet );

			packet = next;
		}
	}
}

//================
// MM_ConnectionList
// Prints the list of connections to the console
//================
void MM_ConnectionList( void )
{
	mm_connection_t *conn;
	int count = 0;

	Com_Printf( "List of connections:\n" );

	for( conn = mms.connections ; conn ; conn = conn->next )
	{
		Com_Printf( "%s, idle %.1fs\n", MM_ConnectionString( conn ), ( float )( CONNECTION_KILL_TIME - conn->killtime ) / 1000.0f );
		count++;
	}

	Com_Printf( "%d connection(s)\n", count );
}
