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
// MM_MatchesList
// Prints the list of matches to console
//================
void MM_MatchList( void )
{
	int i = 0, count = 0;
	mm_match_t *match;

	for( match = mms.matches ; match ; match = match->next )
	{
		Com_Printf( "Match id: %d\n ping type: %s\n gametype: %s\n skill type: %s\n clients:\n",
		            match->id,
		            match->pingtype == TYPE_ANY ? "any" : "dependent",
		            match->gametype,
		            match->skilltype == TYPE_ANY ? "any" : "dependent" );

		for( i = 0; i < match->maxclients; i++ )
		{
			if( match->clients[i] )
				Com_Printf( "  %s %s (%d): %s\n",
				           match->clients[i]->userid < 1 ? "U" : "R", // registered or not
				           match->clients[i]->nickname,
				           match->clients[i]->userid,
				           NET_AddressToString( &match->clients[i]->connection->address ) );
		}

		count++;
	}

	Com_Printf( "%d match(s)\n", count );
}

void MM_CheckMatches( void )
{
	mm_match_t *match;

	for( match = mms.matches ; match ; match = match->next )
	{
		if( !match->server && MM_ClientCount( match ) == match->maxclients )
			MM_AssignGameServer( match );
	}
}

//================
// MM_AddClientToMatch
// Adds a client to a match and updates all clients
//================
void MM_AddClientToMatch( mm_match_t *match, mm_client_t *client )
{
	int i, slot;
	char addstr[MAX_PACKETLEN];

	if( !match || !client )
		return;

	if( MM_ClientCount( match ) == match->maxclients )
		return;

	// find the client a slot
	for( slot = 0 ; slot < match->maxclients ; slot++ )
	{
		if( !match->clients[slot] )
		{
			match->clients[slot] = client;
			break;
		}
	}

	// send the client information about the match
	MM_SendMsgToClient( client, "joined %d %s %d %.2f %s %d %d",
		match->maxclients, match->gametype, match->scorelimit, match->timelimit,
		match->skilltype == TYPE_DEPENDENT ? "dependent" : "all",
		match->skilllevel, slot );

	// add packet for new client
	Q_strncpyz( addstr, "add", sizeof( addstr ) );
	for( i = 0 ; i < match->maxclients; i++ )
	{
		if( !match->clients[i] )
			continue;

		Q_strncatz( addstr, va( " %d \"%s\"", i,
		                        match->clients[i]->nickname ), sizeof( addstr ) );

		// tell other clients about new client
		if( i != slot )
			MM_SendMsgToClient( match->clients[i], "add %d \"%s\"", slot, client->nickname );
	}

	// tell new client about existing clients
	MM_SendMsgToClient( client, addstr );

	// now we want to check if all the client slots have been taken
	// if so, now we need to find a suitable server for the players to play on
	if( MM_ClientCount( match ) == match->maxclients )
		MM_AssignGameServer( match );
}

//================
// MM_GetMatchById
// Returns the match with the given id
//================
mm_match_t *MM_FindMatchById( unsigned int id )
{
	mm_match_t *match;

	for( match = mms.matches ; match ; match = match->next )
	{
		if( match->id == id )
			return match;
	}

	return NULL;
}

//================
// MM_GetSkillLevelByStats
// Calculates skill level for userid by retrieving weapstats from the database
// returns -1 on error
//================
int MM_GetSkillLevelByStats( const int uid )
{
	/*int i;
	MYSQL_RES *res;
	MYSQL_ROW row;
	char query[MAX_QUERY_SIZE];
	float level = 0;
	mm_supported_items_t *item;

	if( !db_handle )
	{
		Com_Printf( "MM_GetSkillLevelByStats: No database connection\n" );
		return -1;
	}

	if( !uid )
		return 0;

	Q_snprintfz( query, sizeof( query ), "SELECT " );

	for( item = supported_items; item->short_name; item++ )
		Q_strncatz( query, va( "`%s`,", item->short_name ), sizeof( query ) );

	// remove trailing comma
	query[strlen( query ) - 1] = '\0';

	Q_strncatz( query, va( " FROM `%s` WHERE `id`='%d'", DBTABLE_USERSTATS, uid ), sizeof( query ) );

	if( DB_Query( db_handle, query ) != DB_SUCCESS )
		return -1;

	if( DB_FetchResult( db_handle, &res ) != DB_SUCCESS )
		return -1;

	// no data found in the database
	if( !DB_NumRows( res ) )
		return 0;

	if( DB_FetchRow( res, &row, 0 ) != DB_SUCCESS )
	{
		DB_FreeResult( &res );
		return -1;
	}
	for( i = 0, item = supported_items; item->short_name; i++, item++ )
		level += atoi( row[i] ) * item->multiplier;

	DB_FreeResult( &res );
	return ( int ) ceil( level );*/

	return -1;
}

//================
// MM_ClearMatches
// Delete all the matches in the matches list
//================
void MM_ClearMatches( void )
{
	mm_match_t *match, *next;

	match = mms.matches;

	while( match )
	{
		next = match->next;
		MM_FreeMatch( match, qtrue );
		match = next;
	}
}

//================
// MM_CreateMatch
// Creates a match in memory
//================
mm_match_t *MM_CreateMatch( int matchid, char *gametype, int maxclients, int scorelimit, float timelimit, mm_type_t skilltype, int skilllevel )
{
	mm_match_t *match;

	if( matchid < 0  || !maxclients )
		return NULL;

	if( !gametype || !*gametype )

	if( skilltype == TYPE_DEPENDENT && skilllevel < 0 )
		return NULL;

	match = ( mm_match_t * )MM_Malloc( sizeof( mm_match_t ) );

	match->id = matchid;
	match->clients = ( mm_client_t ** )MM_Malloc( sizeof( mm_client_t * ) * maxclients );
	match->maxclients = maxclients;
	match->gametype = ( char * )MM_Malloc( strlen( gametype ) + 1 );
	strcpy( match->gametype, gametype );
	match->skilltype = skilltype;
	match->skilllevel = skilllevel;
	match->pingtype = TYPE_ANY; // for now
	match->scorelimit = scorelimit;
	match->timelimit = timelimit;

	match->next = mms.matches;
	mms.matches = match;

	return match;
}

//================
// MM_FreeMatch
// Free match memory and remove it from list
//================
void MM_FreeMatch( mm_match_t *match, qboolean freeconns )
{
	int i;
	mm_server_t *server;
	mm_match_t *current, *prev = NULL;

	// find the match in the list
	for( current = mms.matches ; current ; current = current->next )
	{
		if( current == match )
			break;

		prev = current;
	}

	if( !current )
		return;

	// free clients
	for( i = 0; i < match->maxclients; i++ )
	{
		if( !match->clients[i] )
			continue;

		MM_FreeClient( match->clients[i], freeconns );
	}

	// remove from the list
	if( prev )
		prev->next = match->next;
	else
		mms.matches = match->next;

	// free any servers possibly attached to this match
	for( server = mms.gameservers ; server ; server = server->next )
	{
		if( server->match == match )
			server->match = NULL;
	}

	MM_Free( match->clients );
	MM_Free( match->gametype );
	MM_Free( match );
}
