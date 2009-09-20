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
// MM_CheckGameServers
// Delete old servers
//================
void MM_CheckGameServers( void )
{
	mm_server_t *server = mms.gameservers, *next;

	while( server )
	{
		next = server->next;

		server->killtime -= mms.frametime;
		// delete servers that have timed out
		if( server->killtime < 0 && !server->match )
			MM_FreeGameServer( server );

		server = next;
	}
}

//================
// MM_ServerList
// Ouput serverlist to console
//================
void MM_ServerList( void )
{
	int count;
	mm_server_t *server;

	Com_Printf( "List of gameservers:\n" );

	for ( server = mms.gameservers, count = 0 ; server ; server = server->next, count++ )
		Com_Printf( "%s\n", NET_AddressToString( &server->address ) );

	Com_Printf( "%i gameserver(s)\n", count );
}

//================
// MM_UpdateGameServer
// Updates/creates a gameserver
//================
mm_server_t *MM_UpdateGameServer( const netadr_t *address, int maxclients, char *salt )
{
	mm_server_t *server, *tmp;

	if( !address || !salt || !*salt )
		return NULL;

	server = MM_GetGameServer( address );
	if( !server )
	{
		server = MM_Malloc( sizeof( mm_server_t ) );
		server->address = *address;

		if( mms.gameservers )
		{
			// add it to the end of the list
			for( tmp = mms.gameservers ; tmp->next ; tmp = tmp->next );
			tmp->next = server;
		}
		else
			mms.gameservers = server;
	}

	server->maxclients = maxclients;
	server->killtime = MM_HEARTBEAT_SECONDS * 1000 * 2;
	memcpy( server->salt, salt, sizeof( server->salt ) );

	return server;
}

//================
// MM_FreeGameServer
// Free the memory occupied by the server, and remove it from the list
//================
void MM_FreeGameServer( mm_server_t *server )
{
	mm_server_t *current, *prev = NULL;

	if( !server )
		return;

	// find in the list
	for( current = mms.gameservers ; current ; current = current->next )
	{
		if( current == server )
			break;

		prev = current;
	}

	if( !current )
		return;

	// remove from the list
	if( prev )
		prev->next = server->next;
	else
		mms.gameservers = server->next;

	if( server->connection )
		server->connection->killtime = 0;

	// reassign any match attached to this server
	if( server->match )
	{
		if( server->match->bestserver == server )
			server->match->bestserver = NULL;

		if( server->match->server == server )
			server->match->server = server->next;

		MM_AssignGameServer( server->match );
	}

	Com_DPrintf( "Freeing gameserver %s\n", NET_AddressToString( &server->address ) );

	MM_Free( server );
}

//================
// MM_ServerConnectionFailed
// Connection to server returned an error, clean up
//================
void MM_ServerConnectionFailed( mm_connection_t *conn )
{
	mm_server_t *server;

	for( server = mms.gameservers ; server ; server = server->next )
	{
		if( server->connection == conn )
			break;
	}

	if( !server )
		return;

	Com_DPrintf( "Server connection failed for %s\n", NET_AddressToString( &server->address ) );

	MM_FreeGameServer( server );
}

//================
// MM_SendMsgToGameServer
// Sends a message to the specified gameserver
//================
void MM_SendMsgToGameServer( mm_server_t *server, const void *msg, size_t len )
{
	msg_t packet;
	unsigned char *data, sha1sum[20];
	int datalen;

	if( !server || !msg || !len )
		return;

	if( !*server->salt )
		return;

	server->connection = MM_CreateConnection( NULL, &server->address, MM_ServerConnectionFailed );
	if( !MM_OpenConnectionSocket( server->connection ) )
	{
		server->connection->killtime = 0;
		return;
	}

	datalen = mms.ctx.len + SALT_LEN + len;
	data = ( unsigned char * )MM_Malloc( datalen );

	MSG_Init( &packet, data, datalen );
	// leave space for the signature (keylen bytes)
	MSG_GetSpace( &packet, mms.ctx.len );
	// write salt (SALT_LEN bytes)
	MSG_WriteData( &packet, server->salt, SALT_LEN );
	// write message (len bytes)
	MSG_WriteData( &packet, msg, len );

	// create sha1 sum and sign
	sha1( data + mms.ctx.len, SALT_LEN + len, sha1sum );
	rsa_pkcs1_sign( &mms.ctx, RSA_PRIVATE, RSA_SHA1, 0, sha1sum, data );

	MM_SendConnectionPacket( server->connection, data, datalen );

	MM_Free( data );

	return;
}

//================
// MM_GetGameServer
// Return the server with the given address
//================
mm_server_t *MM_GetGameServer( const netadr_t *address )
{
	mm_server_t *server;

	for( server = mms.gameservers; server; server = server->next )
	{
		if( NET_CompareAddress( &server->address, address ) )
			return server;
	}

	return NULL;
}

//================
// MM_AssignGameServer
// Assign a gameserver to a match
//================
void MM_AssignGameServer( mm_match_t *match )
{
	mm_server_t *server;

	// get the next server
	if( !match->server )
		server = mms.gameservers;
	else
		server = match->server->next;

	// make sure the server has enough slots and isn't already assigned to a match
	while( server && ( server->match || server->maxclients < match->maxclients ) )
		server = server->next;

	Com_DPrintf( "Assigning server to match %u: %s", match->id, server ? NET_AddressToString( &server->address ) : "none found" );

	if( !server )
	{
		if( !match->bestserver )
			return;

		MM_SetupGameServer( match->bestserver );
		return;
	}

	MM_SendMsgToGameServer( server, "lock", 4 );

	server->match = match;
	match->server = server;
}

//================
// MM_SetupGameServer
//================
void MM_SetupGameServer( mm_server_t *server )
{
	mm_match_t *match;
	msg_t msg;
	qbyte msgData[MAX_PACKETLEN];
	int i, len;

	match = server->match;

	if( !match )
		return;

	Com_Printf( "Setting up gameserver for match %u: %s\n", match->id, NET_AddressToString( &server->address ) );

	MSG_Init( &msg, msgData, sizeof( msgData ) );

	MSG_WriteData( &msg, "setup", 5 );
	len = strlen( match->gametype );
	MSG_WriteLong( &msg, len );
	MSG_WriteData( &msg, match->gametype, len );
	MSG_WriteFloat( &msg, match->timelimit );
	MSG_WriteLong( &msg, match->scorelimit );

	for( i = 0 ; i < match->maxclients ; i++ )
	{
		if( match->clients[i] )
			MSG_WriteData( &msg, match->clients[i]->connection->address.ip, 4 );
	}

	MM_SendMsgToGameServer( server, msg.data, msg.cursize );
}
