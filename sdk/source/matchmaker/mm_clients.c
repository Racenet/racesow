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

//================
// MM_SendMsgToClients
// Sends a message to a client
//================
void MM_SendMsgToClient( mm_client_t *client, const char *format, ... )
{
	va_list argptr;
	char msg[MAX_PACKETLEN];

	if( !client || !format || !*format )
		return;

	// should never happen
	if( !client->connection )
		return;

	va_start( argptr, format );
	Q_vsnprintfz( msg, sizeof( msg ), format, argptr );
	va_end( argptr );

	if( !MM_SendConnectionPacket( client->connection, msg, strlen( msg ) + 1 ) )
		client->connection->killtime = 0;
}

//================
// MM_SendMsgToClients
// Sends a common message to all clients connected to the match
//================
void MM_SendMsgToClients( const mm_match_t *match, const char *format, ... )
{
	int i;
	va_list	argptr;
	char msg[1024];

	va_start( argptr, format );
	Q_vsnprintfz( msg, sizeof( msg ), format, argptr );
	va_end( argptr );

	if( !match || !*msg )
		return;

	for( i = 0; i < match->maxclients; i++ )
		MM_SendMsgToClient( match->clients[i], "%s", msg );
}

//================
// MM_CreateClient
// Creates a mm_client_t in memory
//================
mm_client_t *MM_CreateClient( int userid, const char *nickname, mm_connection_t *conn )
{
	mm_client_t *client;

	client = MM_Malloc( sizeof( mm_client_t ) );

	client->userid = userid;
	client->connection = conn;

	client->nickname = MM_Malloc( strlen( nickname ) + 1 );
	strcpy( client->nickname, nickname );

	//client->skill = skill;

	return client;
}

//================
// MM_FreeClient
// Frees memory used by a client
//================
void MM_FreeClient( mm_client_t *client, qboolean freeconns )
{
	if( !client )
		return;

	if( freeconns )
		client->connection->killtime = 0;

	MM_Free( client->nickname );
	MM_Free( client );
}

//================
// MM_ClientCount
// returns client count for a match
//================
int MM_ClientCount( const mm_match_t *match )
{
	int i, count = 0;

	if( !match )
		return -1;

	for( i = 0; i < match->maxclients; i++ )
	{
		if( match->clients[i] )
			count++;
	}

	return count;
}

//================
// MM_FindClientByUserId
// Finds a client in the match with the specified userid
//================
mm_client_t **MM_FindClientByUserId( mm_match_t *match, int uid )
{
	int i;

	if( !match || !uid )
		return NULL;

	for( i = 0; i < match->maxclients; i++ )
	{
		if( !match->clients[i] )
			continue;

		if( match->clients[i]->userid == uid )
			return &match->clients[i];
	}

	return NULL;
}

//================
// MM_FindClientByAddress
//================
mm_client_t **MM_FindClientByConnection( mm_match_t *match, const mm_connection_t *conn )
{
	int i;

	if( !match || !conn )
		return NULL;

	for( i = 0; i < match->maxclients; i++ )
	{
		if( !match->clients[i] )
			continue;

		if( match->clients[i]->connection == conn )
			return &match->clients[i];
	}

	return NULL;
}
