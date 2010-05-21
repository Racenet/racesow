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

#include "qcommon.h"

#include "sys_net.h"

#define	MAX_LOOPBACK	4

typedef struct
{
	qbyte data[MAX_MSGLEN];
	int datalen;
} loopmsg_t;

typedef struct
{
	qboolean open;
	loopmsg_t msgs[MAX_LOOPBACK];
	int get, send;
} loopback_t;

static loopback_t loopbacks[2];
static char *errorstring = NULL;
static size_t errorstring_size = 0;
static qboolean	net_initialized = qfalse;

/*
   =============================================================================
   PRIVATE FUNCTIONS
   =============================================================================
 */

/*
   ==================
   NET_UDP_GetPacket
   ==================
 */
static int NET_UDP_GetPacket( const socket_t *socket, netadr_t *address, msg_t *message )
{
	int ret;

	assert( socket && socket->open && socket->type == SOCKET_UDP );
	assert( address );
	assert( message );

	ret = Sys_NET_RecvFrom( socket->handle, message->data, message->maxsize, address );
	if( ret == -2 )  // would block
		return 0;
	if( ret == -1 )
		return -1;

	if( ret == (int)message->maxsize )
	{
		NET_SetErrorString( "Oversized packet" );
		return -1;
	}

	message->readcount = 0;
	message->cursize = ret;

	return 1;
}

/*
   =====================
   NET_UDP_SendPacket
   =====================
 */
static qboolean NET_UDP_SendPacket( const socket_t *socket, const void *data, size_t length, const netadr_t *address )
{
	assert( socket && socket->open && socket->type == SOCKET_UDP );
	assert( address );
	assert( length > 0 );

	if( !Sys_NET_SendTo( socket->handle, data, length, address ) )
		return qfalse;

	return qtrue;
}

/*
   ====================
   NET_UDP_OpenSocket
   ====================
 */
static qboolean NET_UDP_OpenSocket( socket_t *sock, const netadr_t *address, qboolean server )
{
	int newsocket;

	assert( sock && !sock->open );
	assert( address );

	if( address->type != NA_IP )
	{
		NET_SetErrorString( "Invalid address type" );
		return qfalse;
	}

	if( address->ip[0] == 0 && address->ip[1] == 0 && address->ip[2] == 0 && address->ip[3] == 0 )
	{
		Com_Printf( "Opening UDP/IP socket: *:%hu\n", BigShort( address->port ) );
	}
	else
	{
		Com_Printf( "Opening UDP/IP socket: %s\n", NET_AddressToString( address ) );
	}

	if( ( newsocket = Sys_NET_SocketOpen( SOCKET_UDP ) ) == -1 )
		return qfalse;

	// make it non-blocking
	if( !Sys_NET_SocketMakeNonBlocking( newsocket ) )
	{
		Sys_NET_SocketClose( newsocket );
		return qfalse;
	}

	// make it broadcast capable
	if( !Sys_NET_SocketMakeBroadcastCapable( newsocket ) )
	{
		Sys_NET_SocketClose( newsocket );
		return qfalse;
	}

	// wsw : pb : make it reusable (fast release of port when quit)
	/*if( setsockopt(newsocket, SOL_SOCKET, SO_REUSEADDR, (char *)&i, sizeof(i)) == -1 ) {
	    SetErrorStringFromErrno( "setsockopt" );
	    return 0;
	   }*/

	if( !Sys_NET_Bind( newsocket, address ) )
	{
		Sys_NET_SocketClose( newsocket );
		return qfalse;
	}

	sock->open = qtrue;
	sock->type = SOCKET_UDP;
	sock->address = *address;
	sock->server = server;
	sock->handle = newsocket;

	return qtrue;
}

/*
   ====================
   NET_UDP_CloseSocket
   ====================
 */
static void NET_UDP_CloseSocket( socket_t *socket )
{
	assert( socket && socket->type == SOCKET_UDP );

	if( !socket->open )
		return;

	Sys_NET_SocketClose( socket->handle );
	socket->handle = 0;
	socket->open = qfalse;
}

//=============================================================================

#ifdef TCP_SUPPORT
/*
   ==================
   NET_TCP_Get
   ==================
 */
static int NET_TCP_Get( const socket_t *socket, netadr_t *address, void *data, size_t length )
{
	int ret;

	assert( socket && socket->open && socket->type == SOCKET_TCP );
	assert( data );
	assert( length > 0 );

	ret = Sys_NET_Recv( socket->handle, data, length );
	if( ret == -2 )  // would block
		return 0;
	if( ret == -1 )
		return -1;

	if( address )
		*address = socket->remoteAddress;

	return ret;
}

/*
   ==================
   NET_TCP_GetPacket
   ==================
 */
static int NET_TCP_GetPacket( const socket_t *socket, netadr_t *address, msg_t *message )
{
	int ret;
	qbyte buffer[MAX_PACKETLEN + 4];
	size_t len;

	assert( socket && socket->open && socket->connected && socket->type == SOCKET_TCP );
	assert( address );
	assert( message );

	// peek the message to see if the whole packet is ready
	ret = Sys_NET_Peek( socket->handle, buffer, sizeof( buffer ) );
	if( ret == -2 )  // would block
		return 0;
	if( ret == -1 )
		return -1;

	if( ret < 4 )  // the length information is not yet received
		return 0;

	memcpy( &len, buffer, 4 );
	len = LittleLong( len );

	if( len > MAX_PACKETLEN || len > message->maxsize )
	{
		NET_SetErrorString( "Oversized packet" );
		return -1;
	}

	if( ret < (int)len + 4 )  // the whole packet is not yet ready
		return 0;

	// ok we have the whole packet ready, get it

	// read the 4 byte header
	ret = NET_TCP_Get( socket, NULL, buffer, 4 );
	if( ret == -1 )
		return -1;
	if( ret != 4 )
	{
		NET_SetErrorString( "Couldn't read the whole packet" );
		return -1;
	}

	ret = NET_TCP_Get( socket, NULL, message->data, len );
	if( ret == -1 )
		return -1;
	if( ret != (int)len )
	{
		NET_SetErrorString( "Couldn't read the whole packet" );
		return -1;
	}

	*address = socket->remoteAddress;

	message->readcount = 0;
	message->cursize = ret;

	return qtrue;
}

/*
   ====================
   NET_TCP_Send
   ====================
 */
static qboolean NET_TCP_Send( const socket_t *socket, const void *data, size_t length, const netadr_t *address )
{
	int ret;

	assert( socket && socket->open && socket->type == SOCKET_TCP );
	assert( data );
	assert( length > 0 );
	assert( address );

	ret = Sys_NET_Send( socket->handle, data, length );
	if( ret == -1 )
		return qfalse;
	if( ret != (int)length )
	{
		NET_SetErrorString( "Couldn't send all data" );
		return qfalse;
	}

	return qtrue;
}

/*
   ====================
   NET_TCP_Listen
   ====================
 */
static qboolean NET_TCP_Listen( const socket_t *socket )
{
	assert( socket && socket->open && socket->type == SOCKET_TCP && socket->handle );

	return Sys_NET_Listen( socket->handle, 8 );
}

/*
   ====================
   NET_TCP_Connect
   ====================
 */
static connection_status_t NET_TCP_Connect( socket_t *socket, const netadr_t *address )
{
	int ret;

	assert( socket && socket->open && socket->type == SOCKET_TCP && socket->handle && !socket->connected );
	assert( address );

	ret = Sys_NET_Connect( socket->handle, address );
	if( ret == CONNECTION_INPROGRESS )
	{
		socket->remoteAddress = *address;
		return CONNECTION_INPROGRESS;
	}
	if( ret == CONNECTION_FAILED )
		return CONNECTION_FAILED;

	socket->connected = qtrue;
	socket->remoteAddress = *address;

	return CONNECTION_SUCCEEDED;
}

/*
   ====================
   NET_TCP_Accept
   ====================
 */
static int NET_TCP_Accept( const socket_t *socket, socket_t *newsocket, netadr_t *address )
{
	int handle;

	assert( socket && socket->open && socket->type == SOCKET_TCP && socket->handle );
	assert( newsocket );
	assert( address );

	handle = Sys_NET_Accept( socket->handle, address );
	if( handle == -2 )  // would block
		return 0;
	if( handle == -1 )
		return -1;

	// make the new socket non-blocking
	if( !Sys_NET_SocketMakeNonBlocking( handle ) )
	{
		Sys_NET_SocketClose( handle );
		return -1;
	}

	newsocket->open = qtrue;
	newsocket->type = SOCKET_TCP;
	newsocket->server = socket->server;
	newsocket->address = socket->address;
	newsocket->remoteAddress = *address;
	newsocket->handle = handle;

	return 1;
}

/*
   ====================
   NET_TCP_OpenSocket
   ====================
 */
qboolean NET_TCP_OpenSocket( socket_t *sock, const netadr_t *address, qboolean server )
{
	int handle;

	assert( sock && !sock->open );
	assert( address );

	if( address->type != NA_IP )
	{
		NET_SetErrorString( "Invalid address" );
		return qfalse;
	}

	if( address->ip[0] == 0 && address->ip[1] == 0 && address->ip[2] == 0 && address->ip[3] == 0 )
	{
		Com_Printf( "Opening TCP/IP socket: *:%hu\n", BigShort( address->port ) );
	}
	else
	{
		Com_Printf( "Opening TCP/IP socket: %s\n", NET_AddressToString( address ) );
	}

	// create a TCP socket
	if( ( handle = Sys_NET_SocketOpen( SOCKET_TCP ) ) == -1 )
		return qfalse;

	// make it non-blocking
	if( !Sys_NET_SocketMakeNonBlocking( handle ) )
	{
		Sys_NET_SocketClose( handle );
		return qfalse;
	}

	if( !Sys_NET_Bind( handle, address ) )
	{
		Sys_NET_SocketClose( handle );
		return qfalse;
	}

	sock->open = qtrue;
	sock->type = SOCKET_TCP;
	sock->address = *address;
	sock->server = server;
	sock->handle = handle;

	return qtrue;
}

/*
   ====================
   NET_TCP_CloseSocket
   ====================
 */
static void NET_TCP_CloseSocket( socket_t *socket )
{
	assert( socket && socket->type == SOCKET_TCP );

	if( !socket->open )
		return;

	Sys_NET_SocketClose( socket->handle );
	socket->handle = 0;
	socket->open = qfalse;
	socket->connected = qfalse;
}
#endif // TCP_SUPPORT

//===================================================================


/*
   ===================
   NET_Loopback_GetPacket
   ===================
 */
static int NET_Loopback_GetPacket( const socket_t *socket, netadr_t *address, msg_t *net_message )
{
	int i;
	loopback_t *loop;

	assert( socket->type == SOCKET_LOOPBACK && socket->open );

	loop = &loopbacks[socket->handle];

	if( loop->send - loop->get > ( MAX_LOOPBACK - 1 ) )  // wsw : jal (from q2pro)
		loop->get = loop->send - MAX_LOOPBACK + 1; // wsw : jal (from q2pro)

	if( loop->get >= loop->send )
		return 0;

	i = loop->get & ( MAX_LOOPBACK-1 );
	loop->get++;

	memcpy( net_message->data, loop->msgs[i].data, loop->msgs[i].datalen );
	net_message->cursize = loop->msgs[i].datalen;
	memset( address, 0, sizeof( *address ) );
	address->type = NA_LOOPBACK;

	return 1;
}

/*
   ===================
   NET_SendLoopbackPacket
   ===================
 */
static qboolean NET_Loopback_SendPacket( const socket_t *socket, const void *data, size_t length,
                                         const netadr_t *address )
{
	int i;
	loopback_t *loop;

	assert( socket->open && socket->type == SOCKET_LOOPBACK );
	assert( data );
	assert( length > 0 );
	assert( address );

	if( address->type != NA_LOOPBACK )
	{
		NET_SetErrorString( "Invalid address" );
		return qfalse;
	}

	loop = &loopbacks[socket->handle^1];

	i = loop->send & ( MAX_LOOPBACK - 1 );
	loop->send++;

	memcpy( loop->msgs[i].data, data, length );
	loop->msgs[i].datalen = length;

	return qtrue;
}

/*
   ===================
   NET_Loopback_OpenSocket
   ===================
 */
static qboolean NET_Loopback_OpenSocket( socket_t *socket, const netadr_t *address, qboolean server )
{
	int i;

	assert( address );

	if( address->type != NA_LOOPBACK )
	{
		NET_SetErrorString( "Invalid address" );
		return qfalse;
	}

	for( i = 0; i < 2; i++ )
	{
		if( !loopbacks[i].open )
			break;
	}
	if( i == 2 )
	{
		NET_SetErrorString( "Both loopback sockets already open" );
		return qfalse;
	}

	memset( &loopbacks[i], 0, sizeof( loopbacks[i] ) );
	loopbacks[i].open = qtrue;

	socket->open = qtrue;
	socket->handle = i;

	socket->type = SOCKET_LOOPBACK;
	socket->address = *address;
	socket->server = server;

	return qtrue;
}

/*
   ===================
   NET_Loopback_CloseSocket
   ===================
 */
static void NET_Loopback_CloseSocket( socket_t *socket )
{
	assert( socket->type == SOCKET_LOOPBACK );

	if( !socket->open )
		return;

	assert( socket->handle >= 0 && socket->handle < 2 );

	loopbacks[socket->handle].open = qfalse;
	socket->open = qfalse;
	socket->handle = 0;
}

#ifdef TCP_SUPPORT
/*
   ===================
   NET_TCP_SendPacket
   ===================
 */
static qboolean NET_TCP_SendPacket( const socket_t *socket, const void *data, size_t length, const netadr_t *address )
{
	int len;

	assert( socket && socket->open && socket->type == SOCKET_TCP );
	assert( data );
	assert( address );

	// we send the length of the packet first
	len = LittleLong( length );
	if( !Sys_NET_Send( socket->handle, &len, 4 ) )
		return qfalse;

	if( !Sys_NET_Send( socket->handle, data, length ) )
		return qfalse;

	return qtrue;
}
#endif

/*
   =============================================================================
   PUBLIC FUNCTIONS
   =============================================================================
 */

/*
   ===================
   NET_GetPacket

   1	ok
   0	not ready
   -1	error
   ===================
 */
int NET_GetPacket( const socket_t *socket, netadr_t *address, msg_t *message )
{
	assert( socket->open );

	switch( socket->type )
	{
	case SOCKET_LOOPBACK:
		return NET_Loopback_GetPacket( socket, address, message );

	case SOCKET_UDP:
		return NET_UDP_GetPacket( socket, address, message );

#ifdef TCP_SUPPORT
	case SOCKET_TCP:
		return NET_TCP_GetPacket( socket, address, message );
#endif

	default:
		assert( qfalse );
		NET_SetErrorString( "Unknown socket type" );
		return -1;
	}
}

/*
   ===================
   NET_Get

   1	ok
   0	no data ready
   -1	error
   ===================
 */
int NET_Get( const socket_t *socket, netadr_t *address, void *data, size_t length )
{
	assert( socket->open );

	switch( socket->type )
	{
	case SOCKET_LOOPBACK:
	case SOCKET_UDP:
		NET_SetErrorString( "Operation not supported by the socket type" );
		return -1;

#ifdef TCP_SUPPORT
	case SOCKET_TCP:
		return NET_TCP_Get( socket, address, data, length );
#endif

	default:
		assert( qfalse );
		NET_SetErrorString( "Unknown socket type" );
		return -1;
	}
}

/*
   ===================
   NET_SendPacket
   ===================
 */
qboolean NET_SendPacket( const socket_t *socket, const void *data, size_t length, const netadr_t *address )
{
	assert( socket->open );

	if( address->type == NA_NOTRANSMIT )
		return qtrue;

	switch( socket->type )
	{
	case SOCKET_LOOPBACK:
		return NET_Loopback_SendPacket( socket, data, length, address );

	case SOCKET_UDP:
		return NET_UDP_SendPacket( socket, data, length, address );

#ifdef TCP_SUPPORT
	case SOCKET_TCP:
		return NET_TCP_SendPacket( socket, data, length, address );
#endif

	default:
		assert( qfalse );
		NET_SetErrorString( "Unknown socket type" );
		return qfalse;
	}
}

/*
   ===================
   NET_Send
   ===================
 */
qboolean NET_Send( const socket_t *socket, const void *data, size_t length, const netadr_t *address )
{
	assert( socket->open );

	if( address->type == NA_NOTRANSMIT )
		return qtrue;

	switch( socket->type )
	{
	case SOCKET_LOOPBACK:
	case SOCKET_UDP:
		NET_SetErrorString( "Operation not supported by the socket type" );
		return qfalse;

#ifdef TCP_SUPPORT
	case SOCKET_TCP:
		return NET_TCP_Send( socket, data, length, address );
#endif

	default:
		assert( qfalse );
		NET_SetErrorString( "Unknown socket type" );
		return qfalse;
	}
}

/*
   ===================
   NET_AddressToString
   ===================
 */
char *NET_AddressToString( const netadr_t *a )
{
	static char s[64];

	switch( a->type )
	{
	case NA_NOTRANSMIT:
		Q_strncpyz( s, "no-transmit", sizeof( s ) );
		break;
	case NA_LOOPBACK:
		Q_strncpyz( s, "loopback", sizeof( s ) );
		break;
	case NA_IP:
	case NA_BROADCAST:
		Q_snprintfz( s, sizeof( s ), "%i.%i.%i.%i:%hu", a->ip[0], a->ip[1], a->ip[2], a->ip[3], BigShort( a->port ) );
		break;
	default:
		assert( qfalse );
		Q_strncpyz( s, "unknown", sizeof( s ) );
		break;
	}

	return s;
}

/*
   ===================
   NET_CompareBaseAddress

   Compares without the port
   ===================
 */
qboolean NET_CompareBaseAddress( const netadr_t *a, const netadr_t *b )
{
	if( a->type != b->type )
		return qfalse;

	switch( a->type )
	{
	case NA_LOOPBACK:
		return qtrue;

	case NA_IP:
	case NA_BROADCAST:
		if( a->ip[0] == b->ip[0] && a->ip[1] == b->ip[1] && a->ip[2] == b->ip[2] && a->ip[3] == b->ip[3] )
			return qtrue;
		return qfalse;

	default:
		assert( qfalse );
		return qfalse;
	}
}

/*
   ===================
   NET_CompareAddress

   Compares with the port
   ===================
 */
qboolean NET_CompareAddress( const netadr_t *a, const netadr_t *b )
{
	if( a->type != b->type )
		return qfalse;

	switch( a->type )
	{
	case NA_LOOPBACK:
		return qtrue;

	case NA_IP:
	case NA_BROADCAST:
		if( a->ip[0] == b->ip[0] && a->ip[1] == b->ip[1] && a->ip[2] == b->ip[2] && a->ip[3] == b->ip[3] &&
		   BigShort( a->port ) == BigShort( b->port ) )
			return qtrue;
		return qfalse;

	default:
		assert( qfalse );
		return qfalse;
	}
}

/*
   ===================
   NET_NoTransmitAddress
   ===================
 */
void NET_NoTransmitAddress( netadr_t *address )
{
	memset( address, 0, sizeof( netadr_t ) );
	address->type = NA_NOTRANSMIT;
}

/*
   ===================
   NET_LoopbackAddress
   ===================
 */
void NET_LoopbackAddress( netadr_t *address )
{
	memset( address, 0, sizeof( netadr_t ) );
	address->type = NA_LOOPBACK;
}

/*
   ===================
   NET_BroadcastAddress
   ===================
 */
void NET_BroadcastAddress( netadr_t *address, int port )
{
	memset( address, 0, sizeof( netadr_t ) );
	address->type = NA_BROADCAST;
	address->port = BigShort( port );
}

/*
   =============
   NET_StringToAddress
   =============
 */
qboolean NET_StringToAddress( const char *s, netadr_t *address )
{
	memset( address, 0, sizeof( *address ) );

	if( !Sys_NET_StringToAddress( s, address ) )
	{
		address->type = NA_NOTRANSMIT;
		return qfalse;
	}

	return qtrue;
}

/*
   ===================
   NET_IsLocalAddress
   ===================
 */
qboolean NET_IsLocalAddress( const netadr_t *address )
{
	switch( address->type )
	{
	case NA_LOOPBACK:
		return qtrue;

	case NA_IP:
		if( address->ip[0] == 127 && address->ip[1] == 0 )
			return qtrue;
		// TODO: Check for own external IP address?
		return qfalse;

	default:
		return qfalse;
	}
}

/*
   ==================
   NET_IsLANAddress
   ==================
 */
qboolean NET_IsLANAddress( const netadr_t *address )
{
	if( NET_IsLocalAddress( address ) )
		return qtrue;

	switch( address->type )
	{
	case NA_IP:
		// RFC1918:
		// 10.0.0.0        -   10.255.255.255  (10/8 prefix)
		// 172.16.0.0      -   172.31.255.255  (172.16/12 prefix)
		// 192.168.0.0     -   192.168.255.255 (192.168/16 prefix)
		if( address->ip[0] == 10 )
			return qtrue;
		if( address->ip[0] == 172 && ( address->ip[1]&0xf0 ) == 16 )
			return qtrue;
		if( address->ip[0] == 192 && address->ip[1] == 168 )
			return qtrue;
		return qfalse;

	default:
		return qfalse;
	}
}

/*
   =====================
   NET_AsyncResolveHostname
   =====================
 */
void NET_AsyncResolveHostname( const char *hostname )
{
	Sys_NET_AsyncResolveHostname( hostname );
}

/*
   ===================
   NET_ShowIP
   ===================
 */
void NET_ShowIP( void )
{
	Sys_NET_ShowIP();
}

/*
   ===================
   NET_ErrorString
   ===================
 */
const char *NET_ErrorString( void )
{
	return errorstring;
}

/*
   ===================
   NET_SetErrorString
   ===================
 */
void NET_SetErrorString( const char *format, ... )
{
	va_list	argptr;
	char msg[MAX_PRINTMSG];

	va_start( argptr, format );
	Q_vsnprintfz( msg, sizeof( msg ), format, argptr );
	va_end( argptr );

	if( errorstring_size < strlen( msg ) + 1 )
	{
		if( errorstring )
			Mem_ZoneFree( errorstring );
		errorstring_size = strlen( msg ) + 1 + 64;
		errorstring = Mem_ZoneMalloc( errorstring_size );
	}

	Q_strncpyz( errorstring, msg, errorstring_size );
}

/*
   ===============
   NET_SocketTypeToString
   ================
 */
const char *NET_SocketTypeToString( socket_type_t type )
{
	switch( type )
	{
	case SOCKET_LOOPBACK:
		return "loopback";

	case SOCKET_UDP:
		return "UDP";

#ifdef TCP_SUPPORT
	case SOCKET_TCP:
		return "TCP";
#endif

	default:
		return "unknown";
	}
}

/*
   ===============
   NET_SocketToString
   ================
 */
const char *NET_SocketToString( const socket_t *socket )
{
	return va( "%s %s", NET_SocketTypeToString( socket->type ), ( socket->server ? "server" : "client" ) );
}

/*
   ===================
   NET_Listen
   ===================
 */
qboolean NET_Listen( const socket_t *socket )
{
	assert( socket->open );

	switch( socket->type )
	{
#ifdef TCP_SUPPORT
	case SOCKET_TCP:
		return NET_TCP_Listen( socket );
#endif

	case SOCKET_LOOPBACK:
	case SOCKET_UDP:
	default:
		assert( qfalse );
		NET_SetErrorString( "Unsupported socket type" );
		return qfalse;
	}
}

/*
   ===================
   NET_Connect
   ===================
 */
connection_status_t NET_Connect( socket_t *socket, const netadr_t *address )
{
	assert( socket->open && !socket->connected );
	assert( address );

	switch( socket->type )
	{
#ifdef TCP_SUPPORT
	case SOCKET_TCP:
		return NET_TCP_Connect( socket, address );
#endif

	case SOCKET_LOOPBACK:
	case SOCKET_UDP:
	default:
		assert( qfalse );
		NET_SetErrorString( "Unsupported socket type" );
		return CONNECTION_FAILED;
	}
}

/*
   ===================
   NET_CheckConnect
   ===================
 */
connection_status_t NET_CheckConnect( socket_t *socket )
{
	assert( socket->open );

	if( socket->connected )
		return CONNECTION_SUCCEEDED;

	switch( socket->type )
	{
#ifdef TCP_SUPPORT
	case SOCKET_TCP:
		return Sys_NET_TCP_CheckConnect( socket );
#endif

	case SOCKET_LOOPBACK:
	case SOCKET_UDP:
	default:
		assert( qfalse );
		NET_SetErrorString( "Unsupported socket type" );
		return CONNECTION_FAILED;
	}
}

/*
   ===================
   NET_Accept
   ===================
 */
int NET_Accept( const socket_t *socket, socket_t *newsocket, netadr_t *address )
{
	assert( socket && socket->open );
	assert( newsocket );
	assert( address );

	switch( socket->type )
	{
#ifdef TCP_SUPPORT
	case SOCKET_TCP:
		return NET_TCP_Accept( socket, newsocket, address );
#endif

	case SOCKET_LOOPBACK:
	case SOCKET_UDP:
	default:
		assert( qfalse );
		NET_SetErrorString( "Unsupported socket type" );
		return qfalse;
	}
}

/*
   ===================
   NET_OpenSocket
   ===================
 */
qboolean NET_OpenSocket( socket_t *socket, socket_type_t type, const netadr_t *address, qboolean server )
{
	assert( !socket->open );
	assert( address );

	switch( type )
	{
	case SOCKET_LOOPBACK:
		return NET_Loopback_OpenSocket( socket, address, server );

	case SOCKET_UDP:
		return NET_UDP_OpenSocket( socket, address, server );

#ifdef TCP_SUPPORT
	case SOCKET_TCP:
		return NET_TCP_OpenSocket( socket, address, server );
#endif

	default:
		assert( qfalse );
		NET_SetErrorString( "Unknown socket type" );
		return qfalse;
	}
}

/*
   ===================
   NET_CloseSocket
   ===================
 */
void NET_CloseSocket( socket_t *socket )
{
	if( !socket->open )
		return;

	switch( socket->type )
	{
	case SOCKET_LOOPBACK:
		NET_Loopback_CloseSocket( socket );
		break;

	case SOCKET_UDP:
		NET_UDP_CloseSocket( socket );
		break;

#ifdef TCP_SUPPORT
	case SOCKET_TCP:
		NET_TCP_CloseSocket( socket );
		break;
#endif

	default:
		assert( qfalse );
		NET_SetErrorString( "Unknown socket type" );
		break;
	}
}

/*
   ===================
   NET_Sleep
   ===================
 */
void NET_Sleep( int msec, socket_t *sockets[] )
{
	Sys_NET_Sleep( msec, sockets );
}

/*
   ===================
   NET_Monitor
   Monitors the given sockets with the given timeout in milliseconds
   It ignores closed and loopback sockets.
   Calls the callback function read_cb(socket_t *) with the socket as parameter the socket when incoming data was detected on it
   Calls the callback function exception_cb(socket_t *) with the socket as parameter when a socket exception was detected on that socket
   For both callbacks, NULL can be passed. When NULL is passed for the exception_cb, no exception detection is performed
   Incoming data is always detected, even if the 'read_cb' callback was NULL.
   ===================
 */
int NET_Monitor( int msec, socket_t *sockets[], void (*read_cb)(socket_t *socket), void (*exception_cb)(socket_t *socket) )
{
	return Sys_NET_Monitor(msec, sockets, read_cb, exception_cb);
}

/*
   ===================
   NET_Init
   ===================
 */
void NET_Init( void )
{
	assert( !net_initialized );

	Sys_NET_Init();

	net_initialized = qtrue;
}

/*
   ===================
   NET_Shutdown
   ===================
 */
void NET_Shutdown( void )
{
	if( !net_initialized )
		return;

	if( errorstring )
	{
		Mem_ZoneFree( errorstring );
		errorstring = NULL;
		errorstring_size = 0;
	}

	Sys_NET_Shutdown();

	net_initialized = qfalse;
}
