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

#include "../qcommon/qcommon.h"

#include "../qcommon/sys_net.h"

#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <errno.h>
#include <arpa/inet.h>

#define MAX_IPS 16
static int numIP;
static qbyte localIP[MAX_IPS][4];

//=============================================================================

/*
   ====================
   SetErrorStringFromErrno
   ====================
 */
static void SetErrorStringFromErrno( const char *function )
{
	int code = errno;
	if( function )
	{
		NET_SetErrorString( "%s: %s", function, strerror( code ) );
	}
	else
	{
		NET_SetErrorString( "%s", strerror( code ) );
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
		*(int *)&( (struct sockaddr_in *)sadr )->sin_addr = -1;
		( (struct sockaddr_in *)sadr )->sin_port = address->port;
		return qtrue;

	case NA_IP:
		( (struct sockaddr_in *)sadr )->sin_family = AF_INET;
		*(int *)&( (struct sockaddr_in *)sadr )->sin_addr = *(int *)&address->ip;
		( (struct sockaddr_in *)sadr )->sin_port = address->port;
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
   ==================
   GetLocalAddress
   ==================
 */
static void GetLocalAddress( void )
{
	struct hostent *hostInfo;
	char hostname[256];
	char *p;
	int ip;
	int n;

	if( gethostname( hostname, 256 ) == -1 )
		return;

	hostInfo = gethostbyname( hostname );
	if( !hostInfo )
		return;

	Com_Printf( "Hostname: %s\n", hostInfo->h_name );

	n = 0;
	while( ( p = hostInfo->h_aliases[n++] ) != NULL )
	{
		Com_Printf( "Alias: %s\n", p );
	}

	if( hostInfo->h_addrtype != AF_INET )
		return;

	numIP = 0;
	while( ( p = hostInfo->h_addr_list[numIP++] ) != NULL && numIP < MAX_IPS )
	{
		ip = ntohl( *(int *)p );
		localIP[numIP][0] = p[0];
		localIP[numIP][1] = p[1];
		localIP[numIP][2] = p[2];
		localIP[numIP][3] = p[3];
		Com_Printf( "IP: %i.%i.%i.%i\n", ( ip >> 24 ) & 0xff, ( ip >> 16 ) & 0xff, ( ip >> 8 ) & 0xff, ip & 0xff );
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
	socklen_t fromlen;
	int ret;

	assert( data );
	assert( address );

	fromlen = sizeof( from );
	ret = recvfrom( handle, data, maxsize, 0, (struct sockaddr *)&from, &fromlen );
	if( ret == -1 )
	{
		SetErrorStringFromErrno( "recvfrom" );
		if( errno == EWOULDBLOCK || errno == ECONNREFUSED )
			return -2;
		return -1;
	}

	if( !SockaddressToAddress( &from, address ) )
		return -1;

	return ret;
}

/*
   =====================
   Sys_NET_SendTo
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

	if( sendto( handle, data, length, 0, (struct sockaddr *)&addr, sizeof( addr ) ) == -1 )
	{
		SetErrorStringFromErrno( "sendto" );
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
	int handle;

	switch( type )
	{
	case SOCKET_UDP:
		handle = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP );
		if( handle == -1 )
		{
			SetErrorStringFromErrno( "socket" );
			return -1;
		}
		return handle;

#ifdef TCP_SUPPORT
	case SOCKET_TCP:
		handle = socket( PF_INET, SOCK_STREAM, IPPROTO_TCP );
		if( handle == -1 )
		{
			SetErrorStringFromErrno( "socket" );
			return -1;
		}
		else
		{
			struct linger ling;

			ling.l_onoff = 1;
			ling.l_linger = 5;		// 0 for abortive disconnect

			if( setsockopt( handle, SOL_SOCKET, SO_LINGER, (char *)&ling, sizeof( ling ) ) < 0 )
			{
				SetErrorStringFromErrno( "socket" );
				return -1;
			}
		}
		return handle;
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
	shutdown(handle, SHUT_RDWR);
	close( handle );
}

/*
   ====================
   Sys_NET_SocketMakeNonBlocking
   ====================
 */
qboolean Sys_NET_SocketMakeNonBlocking( int handle )
{
	int _true = 1;

	if( ioctl( handle, FIONBIO, &_true ) == -1 )
	{
		SetErrorStringFromErrno( "ioctl" );
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

	if( setsockopt( handle, SOL_SOCKET, SO_BROADCAST, (char *)&num, sizeof( num ) ) == -1 )
	{
		SetErrorStringFromErrno( "setsockopt" );
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

	if( bind( handle, &sockaddress, sizeof( sockaddress ) ) == -1 )
	{
		SetErrorStringFromErrno( "bind" );
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

	ret = recv( handle, data, length, 0 );
	if( ret == -1 )
	{
		SetErrorStringFromErrno( "recv" );
		if( errno == EWOULDBLOCK )
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

	ret = recv( handle, data, length, MSG_PEEK );
	if( ret == -1 )
	{
		SetErrorStringFromErrno( "recv" );
		if( errno == EWOULDBLOCK )
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
#if ( defined ( __FreeBSD__ ) && ( __FreeBSD_version < 600020 ) || defined ( __APPLE__ ) )
	int opt_val = 1;
#endif

	assert( data );
	assert( length > 0 );

#if ( defined ( __FreeBSD__ ) && ( __FreeBSD_version < 600020 ) || defined ( __APPLE__ ) )
	// Currently ignore the return code from setsockopt
	// Disable SIGPIPE
	setsockopt( handle, SOL_SOCKET, SO_NOSIGPIPE, &opt_val, sizeof( opt_val ) );
	ret = send( handle, data, length, 0 );
	opt_val = 0;
	// Enable SIGPIPE
	setsockopt( handle, SOL_SOCKET, SO_NOSIGPIPE, &opt_val, sizeof( opt_val ) );
#else
	ret = send( handle, data, length, MSG_NOSIGNAL );
#endif
	if( ret == -1 )
	{
		SetErrorStringFromErrno( "send" );
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
		SetErrorStringFromErrno( "listen" );
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

	if( connect( handle, &sockaddress, sizeof( sockaddress ) ) == -1 )
	{
		if( errno == EINPROGRESS )
		{
			return CONNECTION_INPROGRESS;
		}
		else
		{
			SetErrorStringFromErrno( "connect" );
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
	struct timeval timeout = { 0, 0 };
	int result;
	fd_set set;

	assert( socket && socket->open && socket->type == SOCKET_TCP );

	if( socket->connected )
		return CONNECTION_SUCCEEDED;

	FD_ZERO( &set );
	FD_SET( socket->handle, &set );

	if( ( result = select( socket->handle + 1, NULL, &set, NULL, &timeout ) ) == -1 )
	{
		SetErrorStringFromErrno( "select" );
		return CONNECTION_FAILED;
	}
	else if( result )
	{
		struct sockaddr addr;
		socklen_t addr_size;

		if( !FD_ISSET( socket->handle, &set ) )
		{
			NET_SetErrorString( "Write fd not set" );
			return CONNECTION_FAILED;
		}

		// trick to check if we actually got connection succesfully
		// idea from http://cr.yp.to/docs/connect.html
		addr_size = sizeof( addr );
		if( getpeername( socket->handle, &addr, &addr_size ) != 0 )
		{
			char ch;
			read( socket->handle, &ch, 1 ); // produces right errno
			SetErrorStringFromErrno( "getpeername" );
			return CONNECTION_FAILED;
		}

		socket->connected = qtrue;

		return CONNECTION_SUCCEEDED;
	}
	else
	{
		return CONNECTION_INPROGRESS;
	}
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
	socklen_t sockaddress_size;
	int newhandle;

	assert( address );

	sockaddress_size = sizeof( sockaddress );
	newhandle = accept( handle, &sockaddress, &sockaddress_size );
	if( newhandle == -1 )
	{
		SetErrorStringFromErrno( "accept" );
		if( errno == EWOULDBLOCK )
			return -2;
		return -1;
	}
	if( !newhandle )
	{
		NET_SetErrorString( "Accept didn't return a socket" );
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

   sleeps msec or until one of the sockets is ready
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
			FD_SET(sockets[i]->handle, &fdsetr ); // network socket
			if (p_fdsete)
				FD_SET(sockets[i]->handle, p_fdsete );
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
				if ( (exception_cb) && (p_fdsete) && (FD_ISSET(sockets[i]->handle, p_fdsete )) ) {
					exception_cb(sockets[i]);
				}
				if ( (read_cb) && (FD_ISSET(sockets[i]->handle, &fdsetr )) ) {
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
	GetLocalAddress();
}

/*
   ====================
   Sys_NET_Shutdown
   ====================
 */
void Sys_NET_Shutdown( void )
{
}
