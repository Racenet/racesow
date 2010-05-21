/*
   Copyright (C) 2007 Pekka Lampila

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

#ifndef __SYS_NET_H
#define __SYS_NET_H

#include "../qcommon/qcommon.h"

void	    Sys_NET_Init( void );
void	    Sys_NET_Shutdown( void );

void	    Sys_NET_Sleep( int msec, socket_t *sockets[] );
int         Sys_NET_Monitor( int msec, socket_t *sockets[], void (*read_cb)(socket_t *socket), void (*exception_cb)(socket_t *socket) );
void	    Sys_NET_ShowIP( void );
char       *Sys_NET_ErrorString( void );
qboolean    Sys_NET_StringToAddress( const char *s, netadr_t *address );
void		Sys_NET_AsyncResolveHostname( const char *hostname );

int	    Sys_NET_SocketOpen( socket_type_t type );
void	    Sys_NET_SocketClose( int handle );
qboolean    Sys_NET_SocketMakeNonBlocking( int handle );
qboolean    Sys_NET_SocketMakeBroadcastCapable( int handle );

qboolean    Sys_NET_Bind( int handle, const netadr_t *address );
#ifdef TCP_SUPPORT
qboolean    Sys_NET_Listen( int handle, int queue_size );
int	    Sys_NET_Accept( int handle, netadr_t *address );
connection_status_t Sys_NET_Connect( int handle, const netadr_t *address );
connection_status_t Sys_NET_TCP_CheckConnect( socket_t *socket );
#endif

#ifdef TCP_SUPPORT
int	    Sys_NET_Recv( int handle, void *data, size_t length );
int	    Sys_NET_Peek( int handle, void *data, size_t length );
qboolean    Sys_NET_Send( int handle, const void *data, size_t length );
#endif
int	    Sys_NET_RecvFrom( int handle, void *data, size_t maxsize, netadr_t *address );
qboolean    Sys_NET_SendTo( int handle, const void *data, size_t length, const netadr_t *address );

#endif // __SYS_NET_H
