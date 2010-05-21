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

#ifndef _MATCHMAKER_H
#define _MATCHMAKER_H

#include "../qcommon/qcommon.h"
#include "../qcommon/sys_net.h"
#include "../qcommon/rsa.h"
#include "../qcommon/sha1.h"
#include "../../tools/db/db_public.h"
#include "mm_common.h"
#include "auth_public.h"

#ifndef TCP_SUPPORT
#error TCP support required to build
#endif

#define CONNECTION_KILL_TIME 30000

#define MAX_PING 120
#define MAX_PING_DEVIATION 100

#define SKILL_LEVEL_TOLERANCE 5

#define MM_Malloc( size ) Mem_Alloc( mm_mempool, size )
#define MM_Free( data ) Mem_Free( data )
#define Auth_Malloc( size ) Mem_Alloc( auth_mempool, size )
#define Auth_Free( data ) Mem_Free( data )

//================
// mm_connection_t
//================
typedef struct mm_connection_s
{
	socket_t socket;
	netadr_t address;

	int killtime;
	void ( *killcb )( struct mm_connection_s *conn );

	mm_packet_t *packets;

	struct mm_connection_s *prev;
	struct mm_connection_s *next;
} mm_connection_t;

//===============
// mm_client_t
//===============
typedef struct
{
	char *nickname;
	int userid;
	int skill;

	int ping;

	mm_connection_t *connection;
} mm_client_t;

//===============
// mm_match_t
//===============
struct mm_server_s;
typedef struct mm_match_s
{
	struct mm_match_s *next;

	unsigned int id;

	mm_client_t **clients;
	int maxclients;

	// match settings
	mm_type_t pingtype;
	char *gametype;
	int skilllevel;
	mm_type_t skilltype;
	int scorelimit;
	float timelimit;

	int bestdeviation, bestping;
	struct mm_server_s *bestserver, *server;
} mm_match_t;

//================
// mm_server_t
//================
struct mm_server_s
{
	netadr_t address;

	mm_match_t *match;
	int maxclients;

	mm_connection_t *connection;
	char salt[SALT_LEN];

	int killtime;

	struct mm_server_s *next;
};

typedef struct mm_server_s mm_server_t;

//================
// mm_static_t
//================
typedef struct
{
	unsigned int frametime;

	socket_t socket_udp;
	socket_t socket_tcp;

	mm_connection_t *connections;

	mm_match_t *matches;
	mm_server_t *gameservers;

	rsa_context ctx;
} mm_static_t;

//================
// mm_main.c
//================
extern mm_static_t mms;
extern mempool_t *mm_mempool;

void MM_Init( void );
void MM_Shutdown( void );
void MM_Frame( const int realmsec );

//================
// mm_connections.c
//================
void             MM_CheckConnections( void );
void             MM_ShutdownConnections( void );
void             MM_SendConnectionPackets( void );
mm_connection_t *MM_CreateConnection( socket_t *socket, netadr_t *address, void ( *killcb )( mm_connection_t *conn ) );
qboolean         MM_OpenConnectionSocket( mm_connection_t *conn );
qboolean         MM_SendConnectionPacket( mm_connection_t *conn, const void *data, size_t len );
void             MM_ConnectionList( void );

//================
// mm_db.c
//================
#define DBTABLE_USERS       "punbb_users"
#define DBTABLE_USERSTATS   "mm_userstats"

extern db_handle_t db_handle;

void        DB_Init( void );
void        DB_Shutdown( void );

db_status_t DB_Connect( db_handle_t *handle, const char *host, const char *user, const char *pass, const char *db );
void        DB_Disconnect( db_handle_t handle );

const db_error_t *DB_GetError( db_handle_t handle );

void        DB_EscapeString( char *out, const char *in, size_t size );
db_status_t DB_Query( db_handle_t handle, const char *format, ... );
db_status_t DB_FetchResult( db_handle_t handle, db_result_t *result );
void        DB_FreeResult( db_result_t *result );

//================
// mm_gameservers.c
//================
void         MM_CheckGameServers( void );
void         MM_ServerList( void );
mm_server_t *MM_UpdateGameServer( const netadr_t *address, int maxclients, char *salt );
void         MM_FreeGameServer( mm_server_t *server );
void         MM_SendMsgToGameServer( mm_server_t *server, const void *msg, size_t len );
mm_server_t *MM_GetGameServer( const netadr_t *address );
void         MM_AssignGameServer( mm_match_t *match );
void         MM_SetupGameServer( mm_server_t *server );

//================
// mm_net.c
//================
void MM_ConnectionPacket( mm_connection_t *conn, msg_t *msg );
void MM_ConnectionlessPacket( const socket_t *socket, const netadr_t *address, msg_t *msg );

//================
// mm_matches.c
//================
void        MM_MatchList( void );
void        MM_CheckMatches( void );
void        MM_AddClientToMatch( mm_match_t *match, mm_client_t *client );
int         MM_GetSkillLevelByStats( const int uid );
mm_match_t *MM_FindMatchById( unsigned int id );
void        MM_ClearMatches( void );
mm_match_t *MM_CreateMatch( int matchid, char *gametype, int maxclients, int scorelimit, float timelimit, mm_type_t skilltype, int skilllevel );
void        MM_FreeMatch( mm_match_t *match, qboolean freeconns );

//================
// mm_reply.c
//================
void MM_GameServerReply( mm_server_t *server );

//================
// mm_clients.c
//================
void		      MM_SendMsgToClient( mm_client_t *client, const char *format, ... );
void          MM_SendMsgToClients( const mm_match_t *match, const char *format, ... );
mm_client_t  *MM_CreateClient( int userid, const char *nickname, mm_connection_t *conn );
void          MM_FreeClient( mm_client_t *client, qboolean freeconns );
int           MM_ClientCount( const mm_match_t *match );
mm_client_t **MM_FindClientByUserId( mm_match_t *match, int uid );
mm_client_t **MM_FindClientByConnection( mm_match_t *match, const mm_connection_t *conn );

//================
// auth_main.c
//================
void     Auth_Init( void );
void     Auth_Shutdown( void );

qboolean Auth_GetUserInfo( int *id, char *user, int ulen, char *pass, int plen );

//================
// auth_oob.c
//================
void AuthC_Auth( const socket_t *socket, const netadr_t *address );

#endif
