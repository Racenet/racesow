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

static void MMC_Drop( mm_connection_t *conn );

//================
// MMC_Data
// Updates the users data in the database
// Cmd_Argv reference
// 1 = user id
// 2+ = stats in the format:
//      <stattag>=<stat>
//================
static void MMC_Data( mm_connection_t *conn )
{
	char query[1024];
	char *weap, *arg;
	int uid, dmg, i;

	if( !db_handle )
		Com_Error( ERR_DROP, "MMC_Data: No database connection\n" );

	// playerid must be provided, and must be the first argument
	uid = atoi( Cmd_Argv( 1 ) );
	if( !uid )
		return;

	Q_snprintfz( query, sizeof( query ), "UPDATE `%s` SET ", DBTABLE_USERSTATS );

	for( i = 2; i < Cmd_Argc(); i++ )
	{
		arg = Cmd_Argv( i );
		if( !arg || !*arg )
			continue;

		weap = strtok( arg, "=" );
		dmg = atoi( strtok( NULL, "=" ) );
		if( !dmg || !weap || !*weap )
			continue;

		//if( MM_CheckItemExists( weap ) )
			//Q_strncatz( query, va( "`%s`=`%s`+'%d',", weap, weap, dmg ), sizeof( query ) );
	}

	// remove trailing comma
	query[strlen( query ) - 1] = '\0';
	Q_strncatz( query, va( " WHERE `id`='%d'", uid ), sizeof( query ) );

	if( DB_Query( db_handle, query ) != DB_SUCCESS )
		Com_Printf( "DB_Query error:\n%s\n", DB_GetError( db_handle ) );

}

//================
// MMC_Join
// Adds the client to a match and tells all existing clients
// Cmd_Argv reference
// 1 - user id
// 2 - user nickname
// 3 - gametype
//================
static void MMC_Join( mm_connection_t *conn )
{
	int userid;
	char *nickname, *gametype;
	mm_match_t *current, *match = NULL;
	mm_client_t *client;

	if( Cmd_Argc() != 4 )
		return;

	userid = atoi( Cmd_Argv( 1 ) );
	if( userid < 0 )
		return;

	nickname = Cmd_Argv( 2 );
	if( !nickname || !*nickname )
		return;

	gametype = Cmd_Argv( 3 );
	if( !gametype || !*gametype )
		return;

	// find the client a match to play in
	for( current = mms.matches ; current ; current = current->next )
	{
		if( strcmp( current->gametype, gametype ) || MM_ClientCount( current ) == current->maxclients )
			continue;

		if( !match )
		{
			match = current;
			continue;
		}

		// make the client join the most full match
		if( current->maxclients > match->maxclients )
			match = current;
	}

	// no match found
	if( !match )
	{
		MM_SendConnectionPacket( conn, "joined nomatches", 17 );
		return;
	}

	client = MM_CreateClient( userid, nickname, conn );
	MM_AddClientToMatch( match, client );

	conn->killcb = MMC_Drop;
}

//================
// MMC_Drop
// drop client from match and tell other clients about the drop
//================
static void MMC_Drop( mm_connection_t *conn )
{
	mm_match_t *match;
	mm_client_t *client;
	int i;

	for( match = mms.matches ; match ; match = match->next )
	{
		for( i = 0 ; i < match->maxclients ; i++ )
		{
			client = match->clients[i];
			if( client && client->connection == conn )
			{
				MM_FreeClient( client, qtrue );
				match->clients[i] = NULL;

				if( MM_ClientCount( match ) )
					MM_SendMsgToClients( match, "drop %d", i );
				else
					MM_FreeMatch( match, qtrue );

				return;
			}
		}
	}
}

//================
// MMC_Reply
// Gameserver is replying to our cmd request
//================
static void MMC_Reply( mm_connection_t *conn )
{
	mm_server_t *server;

	server = MM_GetGameServer( &conn->address );
	if( !server )
		return;

	if( Cmd_Argc() != 3 )
		return;

	MM_GameServerReply( server );
	conn->killtime = 0;
	server->connection = NULL;
}

//================
// MMC_Heartbeat
// Deals with the heartbeat sent by a gameserver
//================
static void MMC_Heartbeat( const socket_t *socket, const netadr_t *address )
{
	qboolean mm;
	int protocol, maxclients = 0;
	char *salt = NULL;

	if( Cmd_Argc() < 3 )
		return;

	mm = strcmp( Cmd_Argv( 1 ), "yes" ) == 0;
	if( mm && Cmd_Argc() != 5 )
		return;

	protocol = atoi( Cmd_Argv( 2 ) );
	if( protocol != APP_PROTOCOL_VERSION )
		return;

	if( mm )
	{
		maxclients = atoi( Cmd_Argv( 3 ) );
		if( maxclients < 1 )
			return;

		salt = Cmd_Argv( 4 );
		if( !*salt )
			return;
	}

	Com_DPrintf( "Accepted heartbeat from %s\n", NET_AddressToString( address ) );

	if(	mm )
		MM_UpdateGameServer( address, maxclients, salt );
	else
		MM_FreeGameServer( MM_GetGameServer( address ) );
}

//================
// MMC_Chat
// relays a chat message to clients
//================
static void MMC_Chat( mm_connection_t *conn )
{
	mm_match_t *match;
	mm_client_t **client = NULL;
	int type = atoi( Cmd_Argv( 1 ) );
	char *msg = Cmd_Argv( 2 );

	if( Cmd_Argc() != 3 )
		return;

	if( !msg || !*msg )
		return;

	for( match = mms.matches ; match ; match = match->next )
	{
		if( ( client = MM_FindClientByConnection( match, conn ) ) )
			break;
	}

	if( !client )
		return;

	// broadcast this message just to the people in this player's match
	if( type )
	{
		MM_SendMsgToClients( match, "chat \"%s%s:\" \"%s%s\"", S_COLOR_YELLOW, (*client)->nickname, S_COLOR_YELLOW, msg );
		return;
	}

	// broadcast this message to everyone in a match
	for( match = mms.matches ; match ; match = match->next )
		MM_SendMsgToClients( match, "chat \"%s%s:\" \"%s%s\"", S_COLOR_WHITE, (*client)->nickname, S_COLOR_GREEN, msg );
}

//================
// MMC_AddMatch
// Adds a match to the list of matches
//================
static void MMC_AddMatch( mm_connection_t *conn )
{
	static unsigned short matchid = 0;

	int userid, maxclients, scorelimit;
	float timelimit;
	char *nickname, *gametype;
	mm_type_t skilltype;

	mm_match_t *match;
	mm_client_t *client;

	if( Cmd_Argc() != 8 )
		return;

	userid = atoi( Cmd_Argv( 1 ) );
	nickname = Cmd_Argv( 2 );
	if( !*nickname )
		return;

	gametype = Cmd_Argv( 3 );
	if( !gametype || !*gametype )
		return;

	maxclients = atoi( Cmd_Argv( 4 ) );
	if( maxclients < 1 || maxclients > MAX_MAXCLIENTS )
		return;

	scorelimit = atoi( Cmd_Argv( 5 ) );
	if( scorelimit < 0 || scorelimit > MAX_SCORELIMIT )
		return;

	timelimit = atof( Cmd_Argv( 6 ) );
	if( timelimit < 0 || timelimit > MAX_TIMELIMIT );

	if( !scorelimit && !timelimit )
		return;

	skilltype = !strcmp( Cmd_Argv( 7 ), "dependent" ) ? TYPE_DEPENDENT : TYPE_ANY;

	match = MM_CreateMatch( matchid, gametype, maxclients, scorelimit, timelimit, skilltype, 0 );
	client = MM_CreateClient( userid, nickname, conn );
	MM_AddClientToMatch( match, client );

	matchid++;
}

//================
// MMC_PingServer
// Decides which server the clients will play on
//================
static void MMC_PingServer( mm_connection_t *conn )
{
	mm_match_t *match;
	mm_client_t **client = NULL;
	mm_server_t *server;
	netadr_t address;
	int ping, i, lower, upper;

	if( Cmd_Argc() != 3 )
		return;

	for( match = mms.matches ; match ; match = match->next )
	{
		if( ( client = MM_FindClientByConnection( match, conn ) ) )
			break;
	}

	if( !client )
		return;

	if( !NET_StringToAddress( Cmd_Argv( 1 ), &address ) )
		return;

	server = MM_GetGameServer( &address );
	if( !server )
		return;

	if( server->match != match )
		return;

	ping = atoi( Cmd_Argv( 2 ) );
	if( ping < 0 )
		return;

	(*client)->ping = ping;

	lower = 99999999; // probably high enough :P
	upper = 0;
	for( i = 0 ; i < match->maxclients ; i++ )
	{
		// this client has not returned a ping, we need to wait for them
		if( match->clients[i] && !match->clients[i]->ping )
			return;

		lower = min( lower, match->clients[i]->ping );
		upper = max( upper, match->clients[i]->ping );
	}

	// use this server!
	if( upper - lower < MAX_PING_DEVIATION && upper < MAX_PING )
	{
		// free the best alternative (as we have something better)
		if( match->bestserver )
			match->bestserver->match = NULL;

		MM_SetupGameServer( server );
		return;
	}

	// this server we have found here is the best we have found so far
	if( !match->bestserver || ( upper - lower < match->bestdeviation && upper < match->bestping ) )
	{
		// free the old best alternative
		if( match->bestserver )
			match->bestserver->match = NULL;

		match->bestserver = server;
		match->bestdeviation = upper - lower;
		match->bestping = upper;
	}
	// free this server as it will not be used
	else
		server->match = NULL;

	// look for a better server
	MM_AssignGameServer( match );
}

//================
// Supported connetion-orientated packets
//================
typedef struct
{
	char *name;
	void ( *func )( mm_connection_t *conn );
} mm_cmd_t;

mm_cmd_t mm_cmds[] =
{
	// matchmaking packets
	// client
	{ "addmatch", MMC_AddMatch },
	{ "chat", MMC_Chat },
	{ "data", MMC_Data },
	{ "drop", MMC_Drop },
	{ "join", MMC_Join },
	{ "pingserver", MMC_PingServer },
	{ "stayalive", NULL },
	// server
	{ "reply", MMC_Reply },

	{ NULL, NULL }
};

//================
// MM_ConnectionPacket
// Handles a connection-orientated packet
//================
void MM_ConnectionPacket( mm_connection_t *conn, msg_t *msg )
{
	mm_cmd_t *cmd;
	char *s, *c;

	if( !conn || !msg )
		return;

	MSG_BeginReading( msg );
	s = MSG_ReadStringLine( msg );

	Cmd_TokenizeString( s );
	c = Cmd_Argv( 0 );

	for( cmd = mm_cmds; cmd->name; cmd++ )
	{
		if( !strcmp( cmd->name, c ) )
		{
			if( cmd->func )
				cmd->func( conn );

			return;
		}
	}

	Com_DPrintf( "Bad packet from %s:\n%s\n", NET_AddressToString( &conn->address ), s );
}

//================
// MM_ConnectionlessPacket
// Handles a connectionless packet
//================
void MM_ConnectionlessPacket( const socket_t *socket, const netadr_t *address, msg_t *msg )
{
	char *s, *c;

	if( !socket || !address || !msg )
		return;

	if( socket->type != SOCKET_UDP )
		return;

	MSG_BeginReading( msg );
	MSG_ReadLong( msg ); // ignore -1

	s = MSG_ReadStringLine( msg );

	Cmd_TokenizeString( s );
	c = Cmd_Argv( 0 );

	if( !strcmp( c, "ping" ) )
	{
		Netchan_OutOfBandPrint( socket, address, "ack %s", Cmd_Args() );
		return;
	}

	if( !strcmp( c, "heartbeat" ) )
	{
		MMC_Heartbeat( socket, address );
		return;
	}

	Com_DPrintf( "Bad connectionless packet from %s:\n%s\n", NET_AddressToString( address ), s );
}
