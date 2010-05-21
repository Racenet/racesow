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
// net_wins.c

#include "../qcommon/qcommon.h"

#include "../qcommon/sys_net.h"

#include "winquake.h"

#define MAX_IPS 16
static int numIP;
static qbyte localIP[MAX_IPS][4];

//=============================================================================

static const char *LastError( void )
{
	switch( WSAGetLastError() )
	{
	case WSAEINTR: return "WSAEINTR";
	case WSAEBADF: return "WSAEBADF";
	case WSAEACCES: return "WSAEACCES";
	case WSAEDISCON: return "WSAEDISCON";
	case WSAEFAULT: return "WSAEFAULT";
	case WSAEINVAL: return "WSAEINVAL";
	case WSAEMFILE: return "WSAEMFILE";
	case WSAEWOULDBLOCK: return "WSAEWOULDBLOCK";
	case WSAEINPROGRESS: return "WSAEINPROGRESS";
	case WSAEALREADY: return "WSAEALREADY";
	case WSAENOTSOCK: return "WSAENOTSOCK";
	case WSAEDESTADDRREQ: return "WSAEDESTADDRREQ";
	case WSAEMSGSIZE: return "WSAEMSGSIZE";
	case WSAEPROTOTYPE: return "WSAEPROTOTYPE";
	case WSAENOPROTOOPT: return "WSAENOPROTOOPT";
	case WSAEPROTONOSUPPORT: return "WSAEPROTONOSUPPORT";
	case WSAESOCKTNOSUPPORT: return "WSAESOCKTNOSUPPORT";
	case WSAEOPNOTSUPP: return "WSAEOPNOTSUPP";
	case WSAEPFNOSUPPORT: return "WSAEPFNOSUPPORT";
	case WSAEAFNOSUPPORT: return "WSAEAFNOSUPPORT";
	case WSAEADDRINUSE: return "WSAEADDRINUSE";
	case WSAEADDRNOTAVAIL: return "WSAEADDRNOTAVAIL";
	case WSAENETDOWN: return "WSAENETDOWN";
	case WSAENETUNREACH: return "WSAENETUNREACH";
	case WSAENETRESET: return "WSAENETRESET";
	case WSAECONNABORTED: return "WSWSAECONNABORTEDAEINTR";
	case WSAECONNRESET: return "WSAECONNRESET";
	case WSAENOBUFS: return "WSAENOBUFS";
	case WSAEISCONN: return "WSAEISCONN";
	case WSAENOTCONN: return "WSAENOTCONN";
	case WSAESHUTDOWN: return "WSAESHUTDOWN";
	case WSAETOOMANYREFS: return "WSAETOOMANYREFS";
	case WSAETIMEDOUT: return "WSAETIMEDOUT";
	case WSAECONNREFUSED: return "WSAECONNREFUSED";
	case WSAELOOP: return "WSAELOOP";
	case WSAENAMETOOLONG: return "WSAENAMETOOLONG";
	case WSAEHOSTDOWN: return "WSAEHOSTDOWN";
	case WSASYSNOTREADY: return "WSASYSNOTREADY";
	case WSAVERNOTSUPPORTED: return "WSAVERNOTSUPPORTED";
	case WSANOTINITIALISED: return "WSANOTINITIALISED";
	case WSAHOST_NOT_FOUND: return "WSAHOST_NOT_FOUND";
	case WSATRY_AGAIN: return "WSATRY_AGAIN";
	case WSANO_RECOVERY: return "WSANO_RECOVERY";
	case WSANO_DATA: return "WSANO_DATA";
	default: return "NO ERROR";
	}
}

/*
   ====================
   SetErrorStringFromLastError
   ====================
 */
static void SetErrorStringFromLastError( const char *function )
{
	if( function )
	{
		NET_SetErrorString( "%s: %s", function, LastError() );
	}
	else
	{
		NET_SetErrorString( "%s", LastError() );
	}
}

/*
   =============
   SockaddressToAddress
   =============
 */
static qboolean SockaddressToAddress( const struct sockaddr *s, netadr_t *address )
{
	assert( s );
	assert( address );

	switch( s->sa_family )
	{
	case AF_INET:
		address->type = NA_IP;
		*(int *)&address->ip = ( (struct sockaddr_in *)s )->sin_addr.s_addr;
		address->port = ( (struct sockaddr_in *)s )->sin_port;
		return qtrue;

	default:
		NET_SetErrorString( "Unknown address family" );
		return qfalse;
	}
}

/*
   =============
   StringToSockaddress
   =============
 */
static qboolean StringToSockaddress( const char *s, struct sockaddr *sadr )
{
	struct hostent *h;
	char *p;
	char copy[128];

	assert( s );
	assert( sadr );

	if( strlen( s ) >= sizeof( copy ) / sizeof( char ) )
	{
		NET_SetErrorString( "String too long" );
		return qfalse;
	}

	if( !s[0] )
		Q_strncpyz( copy, "0.0.0.0", sizeof( copy ) );
	else
		Q_strncpyz( copy, s, sizeof( copy ) );

	memset( sadr, 0, sizeof( *sadr ) );
	( (struct sockaddr_in *)sadr )->sin_family = AF_INET;

	// port - read and strip
	if( ( p = strchr( copy, ':' ) ) )
	{
		*p = 0;
		( (struct sockaddr_in *)sadr )->sin_port = htons( (short)atoi( p + 1 ) );
	}
	else
	{
		( (struct sockaddr_in *)sadr )->sin_port = 0;
	}

	// address, host names can't start with a digit
	if( copy[0] >= '0' && copy[0] <= '9' )
	{
		*(int *)&( (struct sockaddr_in *)sadr )->sin_addr = inet_addr( copy );
	}
	else
	{
		if( !( h = gethostbyname( copy ) ) )
		{
			NET_SetErrorString( "Host not found" );
			return qfalse;
		}
		*(int *)&( (struct sockaddr_in *)sadr )->sin_addr = *(int *)h->h_addr_list[0];
	}

	return qtrue;
}

/*
   =============
   AddressToSockaddress
   =============
 */
static qboolean AddressToSockaddress( const netadr_t *address, struct sockaddr *sadr )
{
	assert( address );
	assert( sadr );

	switch( address->type )
	{
	case NA_BROADCAST:
		( (struct sockaddr_in *)sadr )->sin_family = AF_INET;
		( (struct sockaddr_in *)sadr )->sin_port = address->port;
		( (struct sockaddr_in *)sadr )->sin_addr.s_addr = INADDR_BROADCAST;
		return qtrue;

	case NA_IP:
		( (struct sockaddr_in *)sadr )->sin_family = AF_INET;
		( (struct sockaddr_in *)sadr )->sin_port = address->port;
		if( address->ip[0] == 0 && address->ip[1] == 0 && address->ip[2] == 0 && address->ip[3] == 0 )
		{
			( (struct sockaddr_in *)sadr )->sin_addr.s_addr = INADDR_ANY;
		}
		else
		{
			( (struct sockaddr_in *)sadr )->sin_addr.s_addr = *(int *)&address->ip;
		}
		return qtrue;

	default:
		NET_SetErrorString( "Unsupported address type" );
		return qfalse;
	}
}

/*
   =============
   Sys_NET_StringToAddress
   =============
 */
qboolean Sys_NET_StringToAddress( const char *s, netadr_t *address )
{
	struct sockaddr sadr;

	assert( s );
	assert( address );

	if( !StringToSockaddress( s, &sadr ) )
		return qfalse;

	SockaddressToAddress( &sadr, address );

	return qtrue;
}

/*
   ==================
   Sys_NET_AsyncResolveHostname
   ==================
 */
void Sys_NET_AsyncResolveHostname( const char *hostname )
{
#ifndef DEDICATED_ONLY
#define WM_ASYNC_LOOKUP_DONE WM_USER+1
	static char hostentbuf[MAXGETHOSTSTRUCT];

	WSAAsyncGetHostByName( cl_hwnd, WM_ASYNC_LOOKUP_DONE, hostname, hostentbuf, sizeof( hostentbuf ) );
#undef WM_ASYNC_LOOKUP_DONE
#endif
}

/*
   ==================
   Sys_NET_ShowIP
   ==================
 */
void Sys_NET_ShowIP( void )
{
	int i;

	for( i = 0; i < numIP; i++ )
		Com_Printf( "IP: %i.%i.%i.%i\n", localIP[i][0], localIP[i][1], localIP[i][2], localIP[i][3] );
}

/*
   =====================
   GetLocalAddress
   =====================
 */
static void GetLocalAddress( void )
{
	struct hostent *hostInfo;
	char hostname[256];
	char *p;
	int error, ip, n;

	if( gethostname( hostname, 256 ) == SOCKET_ERROR )
	{
		error = WSAGetLastError();
		return;
	}

	hostInfo = gethostbyname( hostname );
	if( !hostInfo )
	{
		error = WSAGetLastError();
		return;
	}

	Com_Printf( "Hostname: %s\n", hostInfo->h_name );
	n = 0;
	while( ( p = hostInfo->h_aliases[n++] ) != NULL )
		Com_Printf( "Alias: %s\n", p );

	if( hostInfo->h_addrtype != AF_INET )
		return;

	numIP = 0;
	while( ( p = hostInfo->h_addr_list[numIP] ) != NULL && numIP < MAX_IPS )
	{
		ip = ntohl( *(int *)p );
		localIP[numIP][0] = p[0];
		localIP[numIP][1] = p[1];
		localIP[numIP][2] = p[2];
		localIP[numIP][3] = p[3];
		Com_Printf( "IP: %i.%i.%i.%i\n", ( ip >> 24 ) & 0xff, ( ip >> 16 ) & 0xff, ( ip >> 8 ) & 0xff, ip & 0xff );
		numIP++;
	}
}

//=============================================================================

/*
   ==================
   Sys_NET_RecvFrom

   returns size or -2 for blocking and -1 for other error
   ==================
 */
int Sys_NET_RecvFrom( int handle, void *data, size_t maxsize, netadr_t *address )
{
	struct sockaddr from;
	int fromlen, ret, err;

	assert( data );
	assert( maxsize > 0 );
	assert( address );

	fromlen = sizeof( from );
	ret = recvfrom( (SOCKET)handle, data, maxsize, 0, (struct sockaddr *)&from, &fromlen );
	if( ret == SOCKET_ERROR )
	{
		err = WSAGetLastError();
		if( err == WSAEMSGSIZE )
		{
			NET_SetErrorString( "Oversized packet" );
		}
		else
		{
			SetErrorStringFromLastError( "recvfrom" );
		}
		if( err == WSAEWOULDBLOCK || err == WSAECONNRESET )
			return -2;
		return -1;
	}

	if( !SockaddressToAddress( &from, address ) )
		return -1;

	return ret;
}

/*
   =====================
   Sys_NET_Send
   =====================
 */
qboolean Sys_NET_SendTo( int handle, const void *data, size_t length, const netadr_t *address )
{
	struct sockaddr addr;

	assert( data );
	assert( length > 0 );
	assert( address );

	if( !AddressToSockaddress( address, &addr ) )
		return qfalse;

	if( sendto( (SOCKET)handle, data, length, 0, &addr, sizeof( addr ) ) == SOCKET_ERROR )
	{
		SetErrorStringFromLastError( "sendto" );
		return qfalse;
	}

	return qtrue;
}

/*
   ====================
   Sys_NET_SocketOpen

   returns handle or -1 for error
   ====================
 */
int Sys_NET_SocketOpen( socket_type_t type )
{
	SOCKET handle;

	switch( type )
	{
	case SOCKET_UDP:
		handle = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
		if( handle == INVALID_SOCKET )
		{
			SetErrorStringFromLastError( "socket" );
			return -1;
		}
		return (int)handle;
#ifdef TCP_SUPPORT
	case SOCKET_TCP:
		handle = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
		if( handle == -1 )
		{
			SetErrorStringFromLastError( "socket" );
			return -1;
		}
		else
		{
			struct linger ling;

			ling.l_onoff = 1;
			ling.l_linger = 5;		// 0 for abortive disconnect

			if( setsockopt( handle, SOL_SOCKET, SO_LINGER, (char *)&ling, sizeof( ling ) ) < 0)
			{
				SetErrorStringFromLastError( "socket" );
				return -1;
			}
		}
		return (int)handle;
#endif
	default:
		NET_SetErrorString( "Unknown socket type" );
		return -1;
	}
}

/*
   ====================
   Sys_NET_SocketClose
   ====================
 */
void Sys_NET_SocketClose( int handle )
{
	closesocket( (SOCKET)handle );
}

/*
   ====================
   Sys_NET_SocketMakeNonBlocking
   ====================
 */
qboolean Sys_NET_SocketMakeNonBlocking( int handle )
{
	unsigned long _true = 1;

	if( ioctlsocket( (SOCKET)handle, FIONBIO, &_true ) == SOCKET_ERROR )
	{
		SetErrorStringFromLastError( "ioctlsocket" );
		return qfalse;
	}

	return qtrue;
}

/*
   ====================
   Sys_NET_SocketMakeBroadcastCapable
   ====================
 */
qboolean Sys_NET_SocketMakeBroadcastCapable( int handle )
{
	int num = 1;

	if( setsockopt( (SOCKET)handle, SOL_SOCKET, SO_BROADCAST, (char *)&num, sizeof( num ) ) == SOCKET_ERROR )
	{
		SetErrorStringFromLastError( "setsockopt" );
		return qfalse;
	}

	return qtrue;
}

/*
   ====================
   Sys_NET_Bind
   ====================
 */
qboolean Sys_NET_Bind( int handle, const netadr_t *address )
{
	struct sockaddr sockaddress;

	if( !AddressToSockaddress( address, &sockaddress ) )
		return qfalse;

	if( bind( (SOCKET)handle, &sockaddress, sizeof( sockaddress ) ) == SOCKET_ERROR )
	{
		SetErrorStringFromLastError( "bind" );
		return qfalse;
	}

	return qtrue;
}

//=============================================================================

#ifdef TCP_SUPPORT
/*
   ==================
   Sys_NET_Recv

   returns size or -2 for blocking and -1 for other error
   ==================
 */
int Sys_NET_Recv( int handle, void *data, size_t length )
{
	int ret;

	assert( data );
	assert( length > 0 );

	ret = recv( (SOCKET)handle, data, length, 0 );
	if( ret == SOCKET_ERROR )
	{
		SetErrorStringFromLastError( "recv" );
		if( WSAGetLastError() == WSAEWOULDBLOCK )
			return -2;
		return -1;
	}

	return ret;
}

/*
   ==================
   Sys_NET_Peek

   returns size or -2 for blocking and -1 for other error
   ==================
 */
int Sys_NET_Peek( int handle, void *data, size_t length )
{
	int ret;

	assert( data );
	assert( length > 0 );

	ret = recv( (SOCKET)handle, data, length, MSG_PEEK );
	if( ret == SOCKET_ERROR )
	{
		SetErrorStringFromLastError( "recv" );
		if( WSAGetLastError() == WSAEWOULDBLOCK )
			return -2;
		return -1;
	}

	return ret;
}

/*
   ==================
   Sys_NET_Send
   ==================
 */
qboolean Sys_NET_Send( int handle, const void *data, size_t length )
{
	int ret;

	assert( data );
	assert( length > 0 );

	ret = send( (SOCKET)handle, data, length, 0 );
	if( ret == SOCKET_ERROR )
	{
		SetErrorStringFromLastError( "send" );
		return qfalse;
	}

	return qtrue;
}

/*
   ====================
   Sys_NET_Listen
   ====================
 */
qboolean Sys_NET_Listen( int handle, int queue_size )
{
	assert( queue_size > 0 );

	if( listen( handle, queue_size ) == -1 )
	{
		SetErrorStringFromLastError( "listen" );
		return qfalse;
	}

	return qtrue;
}

/*
   ====================
   Sys_NET_Connect
   ====================
 */
connection_status_t Sys_NET_Connect( int handle, const netadr_t *address )
{
	struct sockaddr sockaddress;

	assert( address );

	if( !AddressToSockaddress( address, &sockaddress ) )
		return CONNECTION_FAILED;

	if( connect( handle, &sockaddress, sizeof( sockaddress ) ) == SOCKET_ERROR )
	{
		if( WSAGetLastError() == WSAEWOULDBLOCK )
		{
			return CONNECTION_INPROGRESS;
		}
		else
		{
			SetErrorStringFromLastError( "connect" );
			return CONNECTION_FAILED;
		}
	}

	return CONNECTION_SUCCEEDED;
}

/*
   ====================
   Sys_NET_TCP_CheckConnect
   ====================
 */
connection_status_t Sys_NET_TCP_CheckConnect( socket_t *socket )
{
	TIMEVAL timeout = { 0, 0 };
	fd_set success_set, failure_set;

	assert( socket && socket->open && socket->type == SOCKET_TCP );

	if( socket->connected )
		return CONNECTION_SUCCEEDED;

	FD_ZERO( &success_set );
	FD_SET( (SOCKET)socket->handle, &success_set );
	FD_ZERO( &failure_set );
	FD_SET( (SOCKET)socket->handle, &failure_set );

	if( select( 0, NULL, &success_set, &failure_set, &timeout ) == SOCKET_ERROR )
	{
		SetErrorStringFromLastError( "select" );
		return CONNECTION_FAILED;
	}
	if( FD_ISSET( (SOCKET)socket->handle, &success_set ) )
	{
		socket->connected = qtrue;
		return CONNECTION_SUCCEEDED;
	}

	if( FD_ISSET( (SOCKET)socket->handle, &failure_set ) )
	{
		int error;
		int errlen = sizeof( error );
		getsockopt( (SOCKET)socket->handle, SOL_SOCKET, SO_ERROR, (char *)&error, &errlen );
		WSASetLastError( error );
		SetErrorStringFromLastError( NULL );
		return CONNECTION_FAILED;
	}

	return CONNECTION_INPROGRESS;
}

/*
   ====================
   Sys_NET_Accept

   returns handle or -2 for blocking and -1 for other error
   ====================
 */
int Sys_NET_Accept( int handle, netadr_t *address )
{
	struct sockaddr sockaddress;
	int sockaddress_size, newhandle;

	assert( address );

	sockaddress_size = sizeof( sockaddress );
	newhandle = accept( handle, &sockaddress, &sockaddress_size );
	if( newhandle == SOCKET_ERROR )
	{
		SetErrorStringFromLastError( "accept" );
		if( WSAGetLastError() == WSAEWOULDBLOCK )
			return -2;
		return -1;
	}

	if( !SockaddressToAddress( &sockaddress, address ) )
		return -1;

	return newhandle;
}
#endif // TCP_SUPPORT

//===================================================================

/*
   ====================
   Sys_NET_Sleep
   ====================
 */
void Sys_NET_Sleep( int msec, socket_t *sockets[] )
{
	struct timeval timeout;
	fd_set fdset;
	int i;

	if( !sockets || !sockets[0] )
		return;

	FD_ZERO( &fdset );

	for( i = 0; sockets[i]; i++ )
	{
		assert( sockets[i]->open );

		switch( sockets[i]->type )
		{
		case SOCKET_UDP:
#ifdef TCP_SUPPORT
		case SOCKET_TCP:
#endif
			assert( sockets[i]->handle > 0 );
			FD_SET( (unsigned)sockets[i]->handle, &fdset ); // network socket
			break;

		default:
			Com_Printf( "Warning: Invalid socket type on Sys_NET_Sleep\n" );
			return;
		}
	}

	timeout.tv_sec = msec / 1000;
	timeout.tv_usec = ( msec % 1000 ) * 1000;
	select( FD_SETSIZE, &fdset, NULL, NULL, &timeout );
}
//===================================================================

/*
   ====================
   Sys_NET_Monitor
   ====================
 */
int Sys_NET_Monitor( int msec, socket_t *sockets[], void (*read_cb)(socket_t *socket), void (*exception_cb)(socket_t *socket) )
{
	struct timeval timeout;
	fd_set fdsetr, fdsete;
	fd_set *p_fdsete = NULL;
	int i, ret;

	if( !sockets || !sockets[0] )
		return 0;

	FD_ZERO( &fdsetr );
	if (exception_cb) {
		FD_ZERO( &fdsete );
		p_fdsete = &fdsete;
	}

	for( i = 0; sockets[i]; i++ )
	{
		if (!sockets[i]->open)
			continue;
		switch( sockets[i]->type )
		{
		case SOCKET_UDP:
#ifdef TCP_SUPPORT
		case SOCKET_TCP:
#endif
			assert( sockets[i]->handle > 0 );
			FD_SET((unsigned)sockets[i]->handle, &fdsetr ); // network socket
			if (p_fdsete)
				FD_SET((unsigned)sockets[i]->handle, p_fdsete );
			break;
		case SOCKET_LOOPBACK:
		default:
			continue;
		}
	}

	timeout.tv_sec = msec / 1000;
	timeout.tv_usec = ( msec % 1000 ) * 1000;
	ret = select( FD_SETSIZE, &fdsetr, NULL, p_fdsete, &timeout );
	if ( ( ret > 0) && ( (read_cb) || (exception_cb)) ) {
		// Launch callbacks
		for( i = 0; sockets[i]; i++ ) {
			if (!sockets[i]->open)
				continue;

			switch( sockets[i]->type ) {
			case SOCKET_UDP:
#ifdef TCP_SUPPORT
			case SOCKET_TCP:
#endif
				if ( (exception_cb) && (p_fdsete) && (FD_ISSET((unsigned)sockets[i]->handle, p_fdsete )) ) {
					exception_cb(sockets[i]);
				}
				if ( (read_cb) && (FD_ISSET((unsigned)sockets[i]->handle, &fdsetr )) ) {
					read_cb(sockets[i]);
				}
				break;
			case SOCKET_LOOPBACK:
			default:
				continue;
			}
		}
	}
	return ret;
}
//===================================================================

/*
   ====================
   Sys_NET_Init
   ====================
 */
void Sys_NET_Init( void )
{
	WSADATA	winsockdata;

	if( WSAStartup( MAKEWORD( 1, 1 ), &winsockdata ) )
		Com_Error( ERR_FATAL, "Winsock initialization failed" );

	Com_Printf( "Winsock initialized\n" );

	GetLocalAddress();
}

/*
   ====================
   Sys_NET_Shutdown
   ====================
 */
void Sys_NET_Shutdown( void )
{
	WSACleanup();
}
