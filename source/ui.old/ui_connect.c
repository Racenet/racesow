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

#include "ui_local.h"

static void M_RefreshScrollWindowList( void );

//pinging
#define PING_ONCE
static unsigned int nextServerTime = 0;
static unsigned int nextPingTime = 0;

//sorting
#define	SORT_PING				1
#define	SORT_PLAYERS			2
#define	SORT_GAMETYPE			3
#define	SORT_MAP				4
#define SORT_MOD				5
#define	SORT_NAME				6

#define COLUMN_WIDTH_PING		55
#define COLUMN_WIDTH_PLAYERS	76
#define COLUMN_WIDTH_GAMETYPE	(uis.vidWidth <= 1024 ? 82 : 92)
#define COLUMN_WIDTH_MAP		(uis.vidWidth <= 1024 ? 94 : 124)
#define COLUMN_WIDTH_MOD		(uis.vidWidth <= 1024 ? 68 : 78)
#define COLUMN_WIDTH_NAME		0

cvar_t *ui_filter_gametype_names;
static char **gametype_filternames;

cvar_t *ui_filter_mod_names;
static char **mod_filternames;

#define ALLOW_DOWNLOAD_MODULES va( "cl_download_allow_modules_%02d", (int)(trap_Cvar_Value( "version" ) * 10) )

/*
   =============================================================================

   JOIN SERVER MENU

   =============================================================================
 */

static menuframework_s s_joinserver_menu;

static menucommon_t *menuitem_fullfilter;
static menucommon_t *menuitem_emptyfilter;
static menucommon_t *menuitem_skillfilter;
static menucommon_t *menuitem_passwordfilter;
static menucommon_t *menuitem_gametypefilter;
static menucommon_t *menuitem_modfilter;
static menucommon_t *menuitem_maxpingfilter;
static menucommon_t *menuitem_namematchfilter;
static menucommon_t *menuitem_instagibfilter;
static menucommon_t *menuitem_tvfilter;

typedef struct server_s
{
	char hostname[80];
	char map[80];
	int curuser;
	int maxuser;
	int bots;
	char gametype[16];
	char mod[16];
	qboolean instagib;
	qboolean tv;
	int skilllevel;
	int id;
	qboolean password;
	unsigned int ping;
	unsigned int ping_retries;
	qboolean displayed;
	char address[80];

	struct server_s *pnext;

} server_t;

static server_t	*servers;
static int numServers;
static server_t *pingingServer = NULL;

char ui_responseToken[MAX_TOKEN_CHARS];

/*
* UI_GetResponseToken
*/
static char *UI_GetResponseToken( char **data_p )
{
	int c;
	int len;
	unsigned backlen;
	char *data;

	data = *data_p;
	len = 0;
	ui_responseToken[0] = 0;

	if( !data )
	{
		*data_p = NULL;
		return "";
	}

	backlen = strlen( data );
	if( backlen < strlen( "\\EOT" ) )
	{
		*data_p = NULL;
		return "";
	}

skipbackslash:
	c = *data;
	if( c == '\\' )
	{
		if( data[1] == '\\' )
		{
			data += 2;
			goto skipbackslash;
		}
	}

	if( !c )
	{
		*data_p = NULL;
		return "";
	}

	do
	{
		if( len < MAX_TOKEN_CHARS )
		{
			ui_responseToken[len] = c;
			len++;
		}
		data++;
		c = *data;
	}
	while( c && c != '\\' );

	if( len == MAX_TOKEN_CHARS )
	{
		len = 0;
	}
	ui_responseToken[len] = 0;

	*data_p = data;
	return ui_responseToken;
}

/*
* FreeServerlist
*/
static void M_FreeServerlist( void )
{
	server_t *ptr;

	while( servers )
	{
		ptr = servers;
		servers = ptr->pnext;
		UI_Free( ptr );
	}

	servers = NULL;
	numServers = 0;
	pingingServer = NULL;

	M_RefreshScrollWindowList();
}

void M_Browser_FreeGametypesList( void )
{
	int i;

	if( !gametype_filternames )
		return;

	for( i = 0; gametype_filternames[i] != NULL; i++ )
	{
		if( gametype_filternames[i] )
		{
			UI_Free( gametype_filternames[i] );
			gametype_filternames[i] = NULL;
		}
	}

	UI_Free( gametype_filternames );
	gametype_filternames = NULL;
}

void M_Browser_UpdateGametypesList( char *list )
{
	char *s, **ptr;
	int count, i;
	size_t size;

	M_Browser_FreeGametypesList();

	for( count = 0; UI_ListNameForPosition( list, count, ';' ) != NULL; count++ );


	gametype_filternames = UI_Malloc( sizeof( ptr ) * ( count + 1 ) );

	for( i = 0; i < count; i++ )
	{
		s = UI_ListNameForPosition( list, i, ';' );

		size = strlen( s ) + 1;
		gametype_filternames[i] = UI_Malloc( size );
		Q_strncpyz( gametype_filternames[i], s, size );
	}

	gametype_filternames[i] = NULL;
}

void M_Browser_FreeModsList( void )
{
	int i;

	if( !mod_filternames )
		return;

	for( i = 0; mod_filternames[i] != NULL; i++ )
	{
		if( mod_filternames[i] )
		{
			UI_Free( mod_filternames[i] );
			mod_filternames[i] = NULL;
		}
	}

	UI_Free( mod_filternames );
	mod_filternames = NULL;
}

void M_Browser_UpdateModsList( char *list )
{
	char *s, **ptr;
	int count, i;
	size_t size;

	M_Browser_FreeModsList();

	for( count = 0; UI_ListNameForPosition( list, count, ';' ) != NULL; count++ );

	mod_filternames = UI_Malloc( sizeof( ptr ) * ( count + 1 ) );

	for( i = 0; i < count; i++ )
	{
		s = UI_ListNameForPosition( list, i, ';' );

		size = strlen( s ) + 1;
		mod_filternames[i] = UI_Malloc( size );
		Q_strncpyz( mod_filternames[i], s, size );
	}

	mod_filternames[i] = NULL;
}

/*
* M_RegisterServerItem
*/
static server_t *M_RegisterServerItem( char *adr, char *info )
{
	server_t *newserv, *checkserv;

	//check for the address being already in the list
	checkserv = servers;
	while( checkserv )
	{
		if( !Q_stricmp( adr, checkserv->address ) )
			return checkserv;
		checkserv = checkserv->pnext;
	}

	newserv = (server_t *)UI_Malloc( sizeof( server_t ) );

	Q_strncpyz( newserv->address, adr, sizeof( newserv->address ) );

	// fill in fields with empty msgs, in case any of them isn't received
	Q_snprintfz( newserv->hostname, sizeof( newserv->hostname ), "Unnamed Server" );
	Q_snprintfz( newserv->map, sizeof( newserv->map ), "Unknown" );
	Q_snprintfz( newserv->gametype, sizeof( newserv->gametype ), "Unknown" );
	newserv->mod[0] = 0;
	newserv->instagib = qfalse;
	newserv->tv = qfalse;
	newserv->curuser = -1; newserv->maxuser = -1;
	newserv->skilllevel = 1;
	newserv->password = qfalse;
	newserv->bots = 0;
	newserv->ping = 9999;
	newserv->ping_retries = 0;

	// set up the actual new item
	newserv->pnext = servers;
	servers = newserv;
	newserv->id = numServers;
	numServers++;

	return newserv;
}

/*
* M_AddToServerList
* The client sent us a new server adress for our list
*/
void M_AddToServerList( char *adr, char *info )
{
	server_t *newserv;
	char *pntr;
	char *tok;
	qboolean need_refresh = qfalse;

	if( !nextServerTime && !nextPingTime )  //User stopped the listing
		return;

	newserv = M_RegisterServerItem( adr, info );

	// parse the info string (new formatting since 0.21)
	pntr = info;

	while( pntr )
	{
		tok = UI_GetResponseToken( &pntr );
		if( !tok || !strlen( tok ) || !Q_stricmp( tok, "EOT" ) )
			break;

		// got cmd
		if( !Q_stricmp( tok, "n" ) )
		{
			tok = UI_GetResponseToken( &pntr );
			if( !tok || !strlen( tok ) || !Q_stricmp( tok, "EOT" ) )
				break;

			if( Q_stricmp( newserv->hostname, tok ) )
			{
				need_refresh = qtrue;
				Q_snprintfz( newserv->hostname, sizeof( newserv->hostname ), tok );
			}
			continue;
		}

		// got cmd
		if( !Q_stricmp( tok, "m" ) )
		{
			tok = UI_GetResponseToken( &pntr );
			if( !tok || !strlen( tok ) || !Q_stricmp( tok, "EOT" ) )
				break;

			while( *tok == ' ' ) tok++; // remove spaces in front of the gametype name

			if( Q_stricmp( newserv->map, tok ) )
			{
				need_refresh = qtrue;
				Q_snprintfz( newserv->map, sizeof( newserv->map ), tok );
				Q_strlwr( newserv->map );
			}
			continue;
		}

		// got cmd
		if( !Q_stricmp( tok, "u" ) )
		{
			int curuser, maxuser;
			tok = UI_GetResponseToken( &pntr );
			if( !tok || !strlen( tok ) || !Q_stricmp( tok, "EOT" ) )
				break;

			sscanf( tok, "%d/%d", &curuser, &maxuser );
			if( curuser != newserv->curuser || maxuser != newserv->maxuser )
			{
				need_refresh = qtrue;
				newserv->curuser = curuser;
				newserv->maxuser = maxuser;
			}
			continue;
		}

		// got cmd
		if( !Q_stricmp( tok, "b" ) )
		{
			int bots;
			tok = UI_GetResponseToken( &pntr );
			if( !tok || !strlen( tok ) || !Q_stricmp( tok, "EOT" ) )
				break;
			bots = atoi( tok );
			if( bots != newserv->bots )
			{
				need_refresh = qtrue;
				newserv->bots = bots;
			}
			continue;
		}

		// got cmd
		if( !Q_stricmp( tok, "g" ) )
		{
			tok = UI_GetResponseToken( &pntr );
			if( !tok || !strlen( tok ) || !Q_stricmp( tok, "EOT" ) )
				break;

			if( newserv->tv )
			{
				Q_strncpyz( newserv->gametype, "tv", sizeof( newserv->gametype ) );
				continue;
			}

			while( *tok == ' ' ) tok++; // remove spaces in front of the gametype name

			if( Q_stricmp( newserv->gametype, tok ) )
			{
				need_refresh = qtrue;
				Q_snprintfz( newserv->gametype, sizeof( newserv->gametype ), tok );
				Q_strlwr( newserv->gametype );

				// add to list of known gametypes
				if( UI_NamesListCvarAddName( ui_filter_gametype_names, newserv->gametype, ';' ) )
				{
					M_Browser_UpdateGametypesList( ui_filter_gametype_names->string );
					UI_SetupSpinControl( menuitem_gametypefilter, gametype_filternames, menuitem_gametypefilter->curvalue );
				}
			}
			continue;
		}

		// got cmd
		if( !Q_stricmp( tok, "mo" ) )
		{
			tok = UI_GetResponseToken( &pntr );
			if( !tok || !strlen( tok ) || !Q_stricmp( tok, "EOT" ) )
				break;

			while( *tok == ' ' ) tok++; // remove spaces in front of the mod name

			if( Q_stricmp( newserv->mod, tok ) )
			{
				need_refresh = qtrue;
				Q_snprintfz( newserv->mod, sizeof( newserv->mod ), tok );
				Q_strlwr( newserv->mod );

				// add to list of known mods
				if( UI_NamesListCvarAddName( ui_filter_mod_names, newserv->mod, ';' ) )
				{
					M_Browser_UpdateModsList( ui_filter_mod_names->string );
					UI_SetupSpinControl( menuitem_modfilter, mod_filternames, menuitem_modfilter->curvalue );
				}
			}
			continue;
		}

		// got cmd
		if( !Q_stricmp( tok, "ig" ) )
		{
			qboolean instagib;

			tok = UI_GetResponseToken( &pntr );
			if( !tok || !strlen( tok ) || !Q_stricmp( tok, "EOT" ) )
				break;

			instagib = (qboolean)( atoi( tok ) != 0 );

			if( newserv->instagib != instagib )
			{
				need_refresh = qtrue;
				newserv->instagib = instagib;
			}
			continue;
		}

		// got cmd
		if( !Q_stricmp( tok, "s" ) )
		{
			int skilllevel;
			tok = UI_GetResponseToken( &pntr );
			if( !tok || !strlen( tok ) || !Q_stricmp( tok, "EOT" ) )
				break;
			skilllevel = atoi( tok );
			if( skilllevel != newserv->skilllevel )
			{
				need_refresh = qtrue;
				newserv->skilllevel = skilllevel;
			}
			continue;
		}

		// got cmd
		if( !Q_stricmp( tok, "p" ) )
		{
			int password;
			tok = UI_GetResponseToken( &pntr );
			if( !tok || !strlen( tok ) || !Q_stricmp( tok, "EOT" ) )
				break;
			password = ( atoi( tok ) );
			if( password != newserv->password )
			{
				need_refresh = qtrue;
				newserv->password = (qboolean)password;
			}
			continue;
		}

		// got cmd
		if( !Q_stricmp( tok, "ping" ) )
		{
			int ping;
			tok = UI_GetResponseToken( &pntr );
			if( !tok || !strlen( tok ) || !Q_stricmp( tok, "EOT" ) )
				break;
			ping = ( atoi( tok ) );
			if( ping != (int) newserv->ping || newserv->ping_retries == 0u )
			{
				need_refresh = qtrue;
#ifdef PING_ONCE
				newserv->ping = ping;
				nextServerTime = uis.time;
#else
				if( newserv->ping < 999 && ping < 999 )
				{
					newserv->ping = ( newserv->ping + ping ) * 0.5;
				}
				else
				{
					newserv->ping = ping;
				}
#endif
				if( newserv->ping > 999 )
					newserv->ping = 999;
			}
			continue;
		}

		// ignore mm flag
		if( !Q_stricmp( tok, "mm" ) )
		{
			// get rid of the token
			UI_GetResponseToken( &pntr );
			continue;
		}

		// ignore tv flag
		if( !Q_stricmp( tok, "tv" ) )
		{
			qboolean tv;

			// get rid of the token
			tok = UI_GetResponseToken( &pntr );
			if( !tok || !strlen( tok ) || !Q_stricmp( tok, "EOT" ) )
				break;

			tv = (qboolean)( atoi( tok ) != 0 );

			if( newserv->tv != tv )
			{
				need_refresh = qtrue;
				newserv->tv = tv;
			}

			if( tv )
				Q_strncpyz( newserv->gametype, "tv", sizeof( newserv->gametype ) );
			continue;
		}

		Com_Printf( "UI:M_AddToServerList(%s): Unknown token:\"%s\"\n", adr, tok );
	}

	if( need_refresh )
		M_RefreshScrollWindowList();
}
#define SERVER_PINGING_MINRETRY 500
#define SERVER_PINGING_MAXRETRYTIMEOUTS 1

/*
* GetNextServerToPing - Get a non yet pinged server
*/
static server_t *GetNextServerToPing( void )
{
	int count, stop;
	server_t *server;

	count = 0;
	server = servers;
	while( server )
	{
		if( server->ping > 999 && server->ping_retries < SERVER_PINGING_MAXRETRYTIMEOUTS )
			count++;
		server = server->pnext;
	}

	if( !count )
		return NULL;

	stop = brandom( 1, count );

	server = servers;
	while( server )
	{
		if( server->ping > 999 && server->ping_retries < SERVER_PINGING_MAXRETRYTIMEOUTS )
		{
			if( --stop == 0 )
				return server;
		}
		server = server->pnext;
	}

	assert( qfalse );

	return NULL;
}

/*
* PingServers
*/
static void PingServers( void )
{
	if( !nextServerTime && !nextPingTime ) //User stopped the listing
	{
		pingingServer = NULL;
		return;
	}

	if( nextServerTime > uis.time )
	{
		// it's not time to move into the next server yet, keep trying current
		if( pingingServer )
		{
			if( nextPingTime > uis.time )
				return;

			nextPingTime = uis.time + SERVER_PINGING_MINRETRY;
			trap_Cmd_ExecuteText( EXEC_APPEND, va( "pingserver %s", pingingServer->address ) );
		}
		return;
	}

	if( pingingServer && pingingServer->ping > SERVER_PINGING_TIMEOUT )
	{
		if( developer->integer )
			UI_Printf( "Server %s timed out\n", pingingServer ? pingingServer->address : "" );
	}

	nextServerTime = uis.time + SERVER_PINGING_TIMEOUT;
	pingingServer = GetNextServerToPing();
	if( !pingingServer )
		return;

	pingingServer->ping_retries++;

#ifdef PING_ONCE
	trap_Cmd_ExecuteText( EXEC_APPEND, va( "pingserver %s", pingingServer->address ) );
	nextPingTime = nextServerTime;
#else
	nextPingTime = uis.time + ( SERVER_PINGING_MINRETRY/2 );
#endif
}

/*
* ResetAllServers
*/
static void ResetAllServers( void )
{
	server_t *server;

	server = servers;
	while( server )
	{
		// fill in fields with empty msgs, in case any of them isn't received
		server->displayed = qfalse;
		Q_snprintfz( server->hostname, sizeof( server->hostname ), "Unnamed Server" );
		Q_snprintfz( server->map, sizeof( server->map ), "Unknown" );
		Q_snprintfz( server->gametype, sizeof( server->gametype ), "Unknown" );
		server->mod[0] = 0;
		server->instagib = qfalse;
		server->curuser = -1; server->maxuser = -1;
		server->skilllevel = 1;
		server->password = qfalse;
		server->bots = 0;
		server->ping = 9999;
		server->ping_retries = 0;
		server = server->pnext;
		M_RefreshScrollWindowList();
	}
	pingingServer = NULL;
	nextServerTime = uis.time;
}

/*
* SearchGames
*/
static void SearchGames( char *s )
{
	char *master;
	const char *mlist;
	int ignore_empty = 0;
	int ignore_full = 0;

#ifdef PING_ONCE
	// clear all ping values so they are retried
	ResetAllServers();
#endif

	//"show all", "is", "is not", 0
	if( menuitem_emptyfilter )
		ignore_empty = ( menuitem_emptyfilter->curvalue == 2 );
	if( menuitem_fullfilter )
		ignore_full = ( menuitem_fullfilter->curvalue == 2 );

	mlist = trap_Cvar_String( "masterservers" );
	if( !mlist || !( *mlist ) )
	{
		Menu_SetStatusBar( &s_joinserver_menu, "No master server defined" );
		return;
	}

	nextServerTime = nextPingTime = uis.time;
	pingingServer = NULL;

	if( Q_stricmp( s, "local" ) == 0 )
	{
		// send out request
		trap_Cmd_ExecuteText( EXEC_APPEND, va( "requestservers %s %s %s\n", s,
		                                       ignore_full ? "" : "full",
		                                       ignore_empty ? "" : "empty" ) );
		return;
	}
	else if( Q_stricmp( s, "favorites" ) == 0 )
	{
		int total_favs, i;
		nextServerTime = uis.time;
		//requestservers local is a hack since favorites is unsupported,
		//but allows ping responses into the menu.
		trap_Cmd_ExecuteText( EXEC_APPEND, va( "requestservers local %s %s\n",
		                                       ignore_full ? "" : "full",
		                                       ignore_empty ? "" : "empty" ) );
		total_favs = trap_Cvar_Value( "favorites" );
		for( i = 1; i <= total_favs; i++ )
		{
			trap_Cmd_ExecuteText( EXEC_APPEND, va( "pingserver %s\n", trap_Cvar_String( va( "favorite_%i", i ) ) ) );
		}
		return;
	}
	else
	{   //its global
		// request to each ip on the list
		while( mlist )
		{
			master = COM_Parse( &mlist );
			if( !master || !( *master ) )
				break;

			// send out request
			trap_Cmd_ExecuteText( EXEC_APPEND, va( "requestservers %s %s %s %s %s\n",
				s, master, trap_Cvar_String( "gamename" ),
				ignore_full ? "" : "full",
				ignore_empty ? "" : "empty" ) );
		}
	}
}

//=============================================================================
// scroll list
//=============================================================================

m_itemslisthead_t serversScrollList;

/*
* GetBestNextPingServer
*/
static server_t *GetBestNextPingServer( server_t *server )
{
	server_t *ptr = servers, *best = NULL;
	unsigned int minping = 0, bestping = 10000; // 9999 = not pinged or no ping response, < 1000 = pinged
	if( server )
		minping = server->ping;

	while( ptr )
	{
		if( Q_stricmp( ptr->gametype, "Unknown" ) != 0 )
		{                                         //hack to show servers that are up but don't respond to pings
			if( ptr->displayed == qfalse && ptr->ping >= minping )
			{
				if( ptr->ping < bestping )
				{
					best = ptr;
					bestping = ptr->ping;
				}
			}
		}
		ptr = ptr->pnext;
	}

	if( best != NULL )
		best->displayed = qtrue;

	return best;
}

/*
* GetWorstNextPingServer
*/
static server_t *GetWorstNextPingServer( server_t *server )
{
	server_t *ptr = servers, *worst = NULL;
	unsigned int minping = 10000, worstping = 0;
	if( server )
		minping = server->ping;

	while( ptr )
	{
		if( Q_stricmp( ptr->gametype, "Unknown" ) != 0 )
		{
			//hack to show servers that are up but don't respond to pings
			if( ptr->displayed == qfalse && ptr->ping <= minping )
			{
				if( ptr->ping > worstping )
				{
					worst = ptr;
					worstping = ptr->ping;
				}
			}
		}
		ptr = ptr->pnext;
	}

	if( worst != NULL )
		worst->displayed = qtrue;

	return worst;
}

/*
* GetMostPlayersServer
*/
static server_t *GetMostPlayersServer( server_t *server )
{
	server_t *ptr = servers, *most = NULL;
	int mostplayers = -1;
	unsigned int bestping = 10000;

	while( ptr )
	{
		if( ptr->curuser != -1 )
		{
			if( ptr->displayed == qfalse )
			{
				if( ptr->curuser > mostplayers || ( ptr->curuser == mostplayers && ptr->ping < bestping ) )
				{
					most = ptr;
					mostplayers = ptr->curuser;
					bestping = ptr->ping;
				}
			}
		}
		ptr = ptr->pnext;
	}

	if( most != NULL )
		most->displayed = qtrue;

	return most;
}

/*
* GetLeastPlayersServer
*/
static server_t *GetLeastPlayersServer( server_t *server )
{
	server_t *ptr = servers, *least = NULL;
	int leastplayers = MAX_CLIENTS+1;
	unsigned int bestping = 10000;

	while( ptr )
	{
		if( ptr->curuser != -1 )
		{
			if( ptr->displayed == qfalse )
			{
				if( ( ptr->curuser >= 0 && ptr->curuser < leastplayers ) || ( ptr->curuser == leastplayers && ptr->ping < bestping ) )
				{
					least = ptr;
					leastplayers = ptr->curuser;
					bestping = ptr->ping;
				}
			}
		}
		ptr = ptr->pnext;
	}

	if( least != NULL )
		least->displayed = qtrue;

	return least;
}

/*
* GetFirstGametypeServer
*/
static server_t *GetFirstGametypeServer( server_t *server )
{
	server_t *ptr = servers, *first = NULL;
	char firstname[80];
	unsigned int bestping = 10000;

	strcpy( firstname, "zzzzz" );

	while( ptr )
	{
		if( ptr->displayed == qfalse )
		{
			if( Q_stricmp( ptr->gametype, "Unknown" ) != 0 )
			{
				char gametype[80];

				Q_snprintfz( gametype, sizeof( gametype ), "%s%s", ( ptr->instagib ? "i" : "" ), ptr->gametype );
				if( Q_stricmp( gametype, firstname ) <= -1 || ( Q_stricmp( gametype, firstname ) == 0 && ptr->ping < bestping ) )
				{
					first = ptr;
					strcpy( firstname, gametype );
					bestping = ptr->ping;
				}
			}
		}
		ptr = ptr->pnext;
	}

	if( first != NULL )
		first->displayed = qtrue;

	return first;
}

/*
* GetLastGametypeServer
*/
static server_t *GetLastGametypeServer( server_t *server )
{
	server_t *ptr = servers, *last = NULL;
	char lastname[80];
	unsigned int bestping = 10000;

	strcpy( lastname, "" );

	while( ptr )
	{
		if( ptr->displayed == qfalse )
		{
			if( Q_stricmp( ptr->gametype, "Unknown" ) != 0 )
			{
				char gametype[80];

				Q_snprintfz( gametype, sizeof( gametype ), "%s%s", ( ptr->instagib ? "i" : "" ), ptr->gametype );
				if( Q_stricmp( gametype, lastname ) >= 1 || ( Q_stricmp( gametype, lastname ) == 0 && ptr->ping < bestping ) )
				{
					last = ptr;
					strcpy( lastname, gametype );
					bestping = ptr->ping;
				}
			}
		}
		ptr = ptr->pnext;
	}

	if( last != NULL )
		last->displayed = qtrue;

	return last;
}

/*
* GetFirstMapServer
*/
static server_t *GetFirstMapServer( server_t *server )
{
	server_t *ptr = servers, *first = NULL;
	char firstname[80];
	unsigned int bestping = 10000;

	strcpy( firstname, "zzzzz" );

	while( ptr )
	{
		if( ptr->displayed == qfalse )
		{
			if( Q_stricmp( ptr->map, "Unknown" ) != 0 )
			{
				if( Q_stricmp( ptr->map, firstname ) <= -1 || ( Q_stricmp( ptr->map, firstname ) == 0 && ptr->ping < bestping ) )
				{
					first = ptr;
					Q_strncpyz( firstname, ptr->map, sizeof( firstname ) );
					bestping = ptr->ping;
				}
			}
		}
		ptr = ptr->pnext;
	}

	if( first != NULL )
		first->displayed = qtrue;

	return first;
}

/*
* GetLastMapServer
*/
static server_t *GetLastMapServer( server_t *server )
{
	server_t *ptr = servers, *last = NULL;
	char lastname[80];
	unsigned int bestping = 10000;

	strcpy( lastname, "" );

	while( ptr )
	{
		if( ptr->displayed == qfalse )
		{
			if( Q_stricmp( ptr->map, "Unknown" ) != 0 )
			{
				if( Q_stricmp( ptr->map, lastname ) >= 1 || ( Q_stricmp( ptr->map, lastname ) == 0 && ptr->ping < bestping ) )
				{
					last = ptr;
					Q_strncpyz( lastname, ptr->map, sizeof( lastname ) );
					bestping = ptr->ping;
				}
			}
		}
		ptr = ptr->pnext;
	}

	if( last != NULL )
		last->displayed = qtrue;

	return last;
}

/*
* GetFirstModServer
*/
static server_t *GetFirstModServer( server_t *server )
{
	server_t *ptr = servers, *first = NULL;
	char firstname[80];
	unsigned int bestping = 10000;

	strcpy( firstname, "zzzzz" );

	while( ptr )
	{
		if( ptr->displayed == qfalse )
		{
			if( ptr->maxuser > 0 )
			{
				char mod[80];

				Q_snprintfz( mod, sizeof( mod ), "%s", ptr->mod );
				if( Q_stricmp( mod, firstname ) <= -1 || ( Q_stricmp( mod, firstname ) == 0 && ptr->ping < bestping ) )
				{
					first = ptr;
					strcpy( firstname, mod );
					bestping = ptr->ping;
				}
			}
		}
		ptr = ptr->pnext;
	}

	if( first != NULL )
		first->displayed = qtrue;

	return first;
}

/*
* GetLastModServer
*/
static server_t *GetLastModServer( server_t *server )
{
	server_t *ptr = servers, *last = NULL;
	char lastname[80];
	unsigned int bestping = 10000;

	strcpy( lastname, "" );

	while( ptr )
	{
		if( ptr->displayed == qfalse )
		{
			if( ptr->maxuser > 0 )
			{
				char mod[80];

				Q_snprintfz( mod, sizeof( mod ), "%s", ptr->mod );
				if( Q_stricmp( mod, lastname ) >= 1 || ( Q_stricmp( mod, lastname ) == 0 && ptr->ping < bestping ) )
				{
					last = ptr;
					strcpy( lastname, mod );
					bestping = ptr->ping;
				}
			}
		}
		ptr = ptr->pnext;
	}

	if( last != NULL )
		last->displayed = qtrue;

	return last;
}

/*
* GetFirstNameServer
*/
static server_t *GetFirstNameServer( server_t *server )
{
	server_t *ptr = servers, *first = NULL;
	char firstname[80];
	unsigned int bestping = 10000;

	strcpy( firstname, "zzzzz" );

	while( ptr )
	{
		if( ptr->displayed == qfalse )
		{
			char hostname[80];

			Q_strncpyz( hostname, COM_RemoveColorTokens( ptr->hostname ), sizeof( hostname ) );
			if( Q_stricmp( hostname, "Unnamed Server" ) != 0 )
			{
				if( Q_stricmp( hostname, firstname ) <= -1 || ( Q_stricmp( hostname, firstname ) == 0 && ptr->ping < bestping ) )
				{
					first = ptr;
					strcpy( firstname, hostname );
					bestping = ptr->ping;
				}
			}
		}
		ptr = ptr->pnext;
	}

	if( first != NULL )
		first->displayed = qtrue;

	return first;
}

/*
* GetLastNameServer
*/
static server_t *GetLastNameServer( server_t *server )
{
	server_t *ptr = servers, *last = NULL;
	char lastname[80];
	unsigned int bestping = 10000;

	strcpy( lastname, "" );

	while( ptr )
	{
		if( ptr->displayed == qfalse )
		{
			char hostname[80];

			Q_strncpyz( hostname, COM_RemoveColorTokens( ptr->hostname ), sizeof( hostname ) );
			if( Q_stricmp( hostname, "Unnamed Server" ) != 0 )
			{
				if( Q_stricmp( hostname, lastname ) >= 1 || ( Q_stricmp( hostname, lastname ) == 0 && ptr->ping < bestping ) )
				{
					last = ptr;
					strcpy( lastname, hostname );
					bestping = ptr->ping;
				}
			}
		}
		ptr = ptr->pnext;
	}

	if( last != NULL )
		last->displayed = qtrue;

	return last;
}

/*
* SortServers
*/
static server_t *SortServers( server_t *server )
{
	server_t *answer = NULL;
	int sort_method;

	sort_method = UI_MenuItemByName( "m_connect_sortbar_ping" )->sort_active;

	if( sort_method == SORT_PING )
	{
		if( UI_MenuItemByName( "m_connect_sortbar_ping" )->curvalue == 0 )
			answer = GetBestNextPingServer( server );
		else
			answer = GetWorstNextPingServer( server );
	}
	else if( sort_method == SORT_PLAYERS )
	{
		if( UI_MenuItemByName( "m_connect_sortbar_players" )->curvalue == 0 )
			answer = GetMostPlayersServer( server );
		else
			answer = GetLeastPlayersServer( server );
	}
	else if( sort_method == SORT_GAMETYPE )
	{
		if( UI_MenuItemByName( "m_connect_sortbar_gametype" )->curvalue == 0 )
			answer = GetFirstGametypeServer( server );
		else
			answer = GetLastGametypeServer( server );
	}
	else if( sort_method == SORT_MAP )
	{
		if( UI_MenuItemByName( "m_connect_sortbar_map" )->curvalue == 0 )
			answer = GetFirstMapServer( server );
		else
			answer = GetLastMapServer( server );
	}
	else if( sort_method == SORT_MOD )
	{
		if( UI_MenuItemByName( "m_connect_sortbar_mod" )->curvalue == 0 )
			answer = GetFirstModServer( server );
		else
			answer = GetLastModServer( server );
	}
	else if( sort_method == SORT_NAME )
	{
		if( UI_MenuItemByName( "m_connect_sortbar_name" )->curvalue == 0 )
			answer = GetFirstNameServer( server );
		else
			answer = GetLastNameServer( server );
	}

	return answer;
}

/*
* M_RefreshScrollWindowList
*/
static void M_RefreshScrollWindowList( void )
{
	server_t *ptr = NULL;
	qboolean add;
	int addedItems = 0;

	UI_FreeScrollItemList( &serversScrollList );
	// mark all servers as not added
	ptr = servers;
	while( ptr )
	{
		ptr->displayed = qfalse;
		ptr = ptr->pnext;
	}

	while( ( ptr = SortServers( ptr ) ) != NULL )
	{
		add = qtrue;
		// ignore if full (local)
		if( menuitem_fullfilter && menuitem_fullfilter->curvalue )
		{
			int full = ( ptr->curuser && ( ptr->curuser == ptr->maxuser ) );
			if( menuitem_fullfilter->curvalue - 1 == full )
				add = qfalse;
		}
		// ignore if empty (local)
		if( menuitem_emptyfilter && menuitem_emptyfilter->curvalue )
		{
			int empty = ( ptr->curuser - ptr->bots != 0 );
			if( menuitem_emptyfilter->curvalue - 1 != empty )
				add = qfalse;
		}
		// ignore if it has password and we are filtering those
		if( menuitem_passwordfilter && menuitem_passwordfilter->curvalue )
		{
			if( menuitem_passwordfilter->curvalue - 1 == ptr->password )
				add = qfalse;
		}
		// ignore if it's of filtered ping
		if( menuitem_maxpingfilter && (int)trap_Cvar_Value( "ui_filter_ping" ) )
		{
			if( ptr->ping > trap_Cvar_Value( "ui_filter_ping" ) )
				add = qfalse;
		}

		// ignore if it's name does not match
		if( menuitem_namematchfilter && strlen( trap_Cvar_String( "ui_filter_name" ) ) )
		{
			char filter_name[80], host_name[80];

			//Remove color and convert to lower case
			Q_strncpyz( filter_name, COM_RemoveColorTokens( trap_Cvar_String( "ui_filter_name" ) ), sizeof( filter_name ) );
			Q_strlwr( filter_name );

			Q_strncpyz( host_name, COM_RemoveColorTokens( ptr->hostname ), sizeof( filter_name ) );
			Q_strlwr( host_name );

			if( !strstr( host_name, filter_name ) )
				add = qfalse;
		}

		// ignore if it's of filtered gametype
		if( menuitem_gametypefilter && menuitem_gametypefilter->itemnames &&
			Q_stricmp( menuitem_gametypefilter->itemnames[menuitem_gametypefilter->curvalue], "show all" ) )
		{
			if( Q_stricmp( menuitem_gametypefilter->itemnames[menuitem_gametypefilter->curvalue], ptr->gametype ) )
				add = qfalse;
		}

		// ignore if it's of filtered mod
		if( menuitem_modfilter && menuitem_modfilter->itemnames &&
			Q_stricmp( menuitem_modfilter->itemnames[menuitem_modfilter->curvalue], "show all" ) )
		{
			// if it's basewsw it doesn't send any mod name
			if( !Q_stricmp( menuitem_modfilter->itemnames[menuitem_modfilter->curvalue], "don't show" ) )
			{
				if( ptr->mod[0] )
					add = qfalse;
			}
			// if it's a different mod name, don't show
			else if( Q_stricmp( menuitem_modfilter->itemnames[menuitem_modfilter->curvalue], ptr->mod ) )
				add = qfalse;
		}

		// instagib filter
		if( menuitem_instagibfilter && menuitem_instagibfilter->curvalue )
		{
			if( menuitem_instagibfilter->curvalue - 1 == ptr->instagib )
				add = qfalse;
		}

		// tv filter
		if( menuitem_tvfilter )
		{
			if( menuitem_tvfilter->curvalue != ptr->tv )
				add = qfalse;
		}

		// ignore if it's of filtered skill
		if( menuitem_skillfilter && menuitem_skillfilter->curvalue )
		{
			if( menuitem_skillfilter->curvalue - 1 != ptr->skilllevel )
				add = qfalse;
		}

		if( add )
			UI_AddItemToScrollList( &serversScrollList, va( "%i", addedItems++ ), (void *)ptr );
	}
}

//=============================================================================
// Servers window
//=============================================================================

static int MAX_MENU_SERVERS = 9;

static int scrollbar_curvalue = 0;
static int scrollbar_id = 0;

#define	NO_SERVER_STRING    ""
#define	NO_MAP_STRING	"..."
#define	NO_USERS_STRING	"..."
#define NO_GAMETYPE_STRING "..."


/*
* M_Connect_UpdateFiltersSettings
*/
static void M_Connect_UpdateFiltersSettings( menucommon_t *menuitem )
{
	// remember filter options by using cvars
	if( menuitem_fullfilter ) trap_Cvar_SetValue( "ui_filter_full", menuitem_fullfilter->curvalue );
	if( menuitem_emptyfilter ) trap_Cvar_SetValue( "ui_filter_empty", menuitem_emptyfilter->curvalue );
	if( menuitem_passwordfilter ) trap_Cvar_SetValue( "ui_filter_password", menuitem_passwordfilter->curvalue );
	if( menuitem_skillfilter ) trap_Cvar_SetValue( "ui_filter_skill", menuitem_skillfilter->curvalue );
	if( menuitem_instagibfilter ) trap_Cvar_SetValue( "ui_filter_instagib", menuitem_instagibfilter->curvalue );
	if( menuitem_gametypefilter ) trap_Cvar_SetValue( "ui_filter_gametype", menuitem_gametypefilter->curvalue );
	if( menuitem_modfilter ) trap_Cvar_SetValue( "ui_filter_mod", menuitem_modfilter->curvalue );
	if( menuitem_maxpingfilter ) trap_Cvar_Set( "ui_filter_ping", UI_GetMenuitemFieldBuffer( menuitem_maxpingfilter ) );
	if( menuitem_namematchfilter ) trap_Cvar_Set( "ui_filter_name", UI_GetMenuitemFieldBuffer( menuitem_namematchfilter ) );
	if( menuitem_tvfilter ) trap_Cvar_SetValue( "ui_filter_tv", menuitem_tvfilter->curvalue );

	M_RefreshScrollWindowList();
}

/*
* M_Connect_UpdateSortbar
*/
static void M_Connect_UpdateSortbar( menucommon_t *menuitem )
{
	trap_Cvar_SetValue( "ui_sortmethod", menuitem->sort_active );
	if( menuitem->sort_active == menuitem->sort_type )
		trap_Cvar_SetValue( "ui_sortmethod_direction", menuitem->curvalue );
	M_RefreshScrollWindowList();
}

/*
* M_Connect_UpdateScrollbar
*/
static void M_Connect_UpdateScrollbar( menucommon_t *menuitem )
{
	menuitem->maxvalue = max( 0, serversScrollList.numItems - MAX_MENU_SERVERS );
	if( menuitem->curvalue > menuitem->maxvalue )  //if filters shrunk the list size, shrink the scrollbar and its value
		menuitem->curvalue = menuitem->maxvalue;
	trap_Cvar_SetValue( menuitem->title, menuitem->curvalue );
	scrollbar_curvalue = menuitem->curvalue;
}

/*
* M_StopSearch
*/
void M_StopSearch( struct menucommon_s *unused )
{
	nextServerTime = nextPingTime = 0;
}

/*
* M_Connect_ChangeSearchType
*/
static void M_Connect_ChangeSearchType( menucommon_t *menuitem )
{
	int i;
	M_StopSearch( NULL );
	M_FreeServerlist();
	trap_Cmd_ExecuteText( EXEC_APPEND, va( "seta ui_searchtype %i\n", menuitem->curvalue ) );
	if( menuitem->curvalue == 2 )
	{
		for( i = 0; i < MAX_MENU_SERVERS; i++ )
		{
			UI_MenuItemByName( va( "m_connect_button_%i", i ) )->statusbar = "press ENTER to connect, r to remove from favorites";
		}
	}
	else
	{
		for( i = 0; i < MAX_MENU_SERVERS; i++ )
		{
			UI_MenuItemByName( va( "m_connect_button_%i", i ) )->statusbar = "press ENTER to connect, f to add to favorites";
		}
	}

}

static void M_Connect_ToggleAllowModules( menucommon_t *menuitem )
{
	trap_Cvar_SetValue( ALLOW_DOWNLOAD_MODULES, menuitem->curvalue );
}

/*
* GetInforServerFunc
*/
#if 0
static void GetInforServerFunc( menucommon_t *menuitem )
{
	char buffer[128];
	server_t *server = NULL;
	m_listitem_t *listitem;

	menuitem->localdata[1] = menuitem->localdata[0] + scrollbar_curvalue;
	listitem = UI_FindItemInScrollListWithId( &serversScrollList, menuitem->localdata[1] );
	if( listitem && listitem->data )
		server = (server_t *)listitem->data;
	if( server )
	{
		Q_snprintfz( buffer, sizeof( buffer ), "getinfo %s\n", server->address );
		trap_Cmd_ExecuteText( EXEC_APPEND, buffer );
	}
}
#endif

/*
* SearchGamesFunc
*/
static void SearchGamesFunc( menucommon_t *menuitem )
{
	menucommon_t *typemenuitem = UI_MenuItemByName( "m_connect_search_type" );
	if( !typemenuitem )
		return;

	clamp( typemenuitem->curvalue, 0, 2 );
	M_FreeServerlist();

	SearchGames( typemenuitem->itemnames[typemenuitem->curvalue] );
}

/*
* M_Connect_Joinserver
*/
static void M_Connect_Joinserver( menucommon_t *menuitem )
{
	char buffer[128];
	server_t *server = NULL;
	m_listitem_t *listitem;

	menuitem->localdata[1] = menuitem->localdata[0] + scrollbar_curvalue;
	listitem = UI_FindItemInScrollListWithId( &serversScrollList, menuitem->localdata[1] );
	if( listitem && listitem->data )
		server = (server_t *)listitem->data;
	if( server )
	{
		Q_snprintfz( buffer, sizeof( buffer ), "connect %s\n", server->address );
		trap_Cmd_ExecuteText( EXEC_APPEND, buffer );
	}
}

/*
* M_AddToFavorites
*/
void M_AddToFavorites( menucommon_t *menuitem )
{
	//	char	buffer[128];
	server_t *server = NULL;
	m_listitem_t *listitem;

	if( trap_Cvar_Value( "ui_searchtype" ) == 3 )
		return; //no adding favorites in the favorites menu

	menuitem->localdata[1] = menuitem->localdata[0] + scrollbar_curvalue;
	listitem = UI_FindItemInScrollListWithId( &serversScrollList, menuitem->localdata[1] );

	if( listitem && listitem->data )
		server = (server_t *)listitem->data;

	if( server )
	{
		int i, favorites = (int)trap_Cvar_Value( "favorites" );
		for( i = 1; i <= favorites; i++ )
		{
			if( strcmp( trap_Cvar_String( va( "favorite_%i", i ) ), server->address ) == 0 )
				return; //it already is in favorites
		}
		trap_Cmd_ExecuteText( EXEC_APPEND, va( "seta favorites %i\n", ++favorites ) );
		trap_Cmd_ExecuteText( EXEC_APPEND, va( "seta favorite_%i %s\n", favorites, server->address ) );
	}
}

/*
* M_RemoveFromFavorites
*/
void M_RemoveFromFavorites( menucommon_t *menuitem )
{
	//	char	buffer[128];
	server_t *server = NULL;
	m_listitem_t *listitem;

	menuitem->localdata[1] = menuitem->localdata[0] + scrollbar_curvalue;
	listitem = UI_FindItemInScrollListWithId( &serversScrollList, menuitem->localdata[1] );

	if( listitem && listitem->data )
		server = (server_t *)listitem->data;

	if( server )
	{
		int i, match = 0, favorites = (int)trap_Cvar_Value( "favorites" );
		for( i = 1; i <= favorites; i++ )
		{
			if( !strcmp( trap_Cvar_String( va( "favorite_%i", i ) ), server->address ) )
				match = i;
		}
		if( !match )
			return;
		else if( match < favorites )
		{
			for( i = match; i < favorites; i++ )
			{
				char next[80];

				Q_strncpyz( next, trap_Cvar_String( va( "favorite_%i", ( i+1 ) ) ), sizeof( next ) );
				trap_Cmd_ExecuteText( EXEC_NOW, va( "seta favorite_%i %s\n", i, next ) );
			}
		}
		trap_Cmd_ExecuteText( EXEC_NOW, va( "seta favorite_%i \"\"\n", favorites ) );
		trap_Cmd_ExecuteText( EXEC_NOW, va( "seta favorites %i\n", --favorites ) );
		SearchGamesFunc( UI_MenuItemByName( "m_connect_search" ) ); //refresh the list so they know it is removed.
	}
}

/*
* M_UpdateSeverButton
*/
static void M_UpdateSeverButton( menucommon_t *menuitem )
{
	server_t *server = NULL;
	m_listitem_t *listitem;

	menuitem->localdata[1] = menuitem->localdata[0] + scrollbar_curvalue;
	listitem = UI_FindItemInScrollListWithId( &serversScrollList, menuitem->localdata[1] );
	if( listitem && listitem->data )
		server = (server_t *)listitem->data;

	if( server )
	{
		char gametype[16];

		Q_snprintfz( gametype, sizeof( gametype ), "%s%s", ( server->instagib ? "i" : "" ), server->gametype );

		Q_snprintfz( menuitem->title, sizeof( menuitem->title ), "\\w:%i\\%s%s\\w:%i\\%s%s\\w:%i\\%s%s\\w:%i\\%s%s\\w:%i\\%s%s\\w:%i\\%s%s",
			COLUMN_WIDTH_PING, S_COLOR_WHITE, va( "%i", server->ping ),
			COLUMN_WIDTH_PLAYERS, ( server->curuser == server->maxuser ? S_COLOR_RED : S_COLOR_WHITE ), va( "%i/%i", server->curuser - server->bots, server->maxuser ),
			COLUMN_WIDTH_GAMETYPE, S_COLOR_YELLOW, gametype,
			COLUMN_WIDTH_MAP, S_COLOR_ORANGE, server->map,
			COLUMN_WIDTH_MOD, S_COLOR_YELLOW, server->mod,
			COLUMN_WIDTH_NAME, S_COLOR_WHITE, server->hostname
		);
	}
	else
		Q_snprintfz( menuitem->title, MAX_STRING_CHARS, NO_SERVER_STRING );
}

/*
* M_DrawFiltersBox
*/
static void M_DrawFiltersBox( menucommon_t *menuitem )
{
	int x, y;
	int width, height;

	x = menuitem->x + menuitem->parent->x;
	y = menuitem->y + menuitem->parent->y - trap_SCR_strHeight( menuitem->font );

	width = menuitem->pict.width;
	height = menuitem->pict.height + trap_SCR_strHeight( menuitem->font ) + 4;

	UI_DrawBox( x, y, width, height, colorWarsowPurple, colorWhite, NULL, colorDkGrey );
	UI_DrawString( x + 24, y + 10, ALIGN_LEFT_TOP, "Filters", 0, menuitem->font, colorWhite );
}

/*
* JoinServer_MenuInit
*/
static void JoinServer_MenuInit( void )
{
	menucommon_t *menuitem;
	menucommon_t *menuitem_filters_background;
	int i, scrollwindow_width, scrollwindow_height, xoffset, yoffset = 0, vspacing = 0;
	static char sbar[64];
	static char *filter_names[] =		{ "show all", "show only", "don't show", 0 };
	static char *search_names[] =		{ "global", "local", "favorites", 0 };
	static char *skill_filternames[] =	{ "show all", "beginner", "expert", "hardcore", 0 };
	//sorting spincontrols, only the pict will change (Down/Up)
	static char *sort_ping[] =		{ "Ping", "Ping", 0 };
	static char *sort_players[] =		{ "Players", "Players", 0 };
	static char *sort_gametype[] =		{ "Gametype", "Gametype", 0 };
	static char *sort_map[] =		{ "Map", "Map", 0 };
	static char *sort_mod[] =		{ "Mod", "Mod", 0 };
	static char *sort_name[] =		{ "Name", "Name", 0 };
	static char *tvfilter_names[] =		{ "don't show", "show only", 0 };

	// we have to open the cvars like this so they are created as CVAR_ARCHIVE
	trap_Cvar_Get( "ui_filter_full", "0", CVAR_ARCHIVE );
	trap_Cvar_Get( "ui_filter_empty", "0", CVAR_ARCHIVE );
	trap_Cvar_Get( "ui_filter_password", "0", CVAR_ARCHIVE );
	trap_Cvar_Get( "ui_filter_skill", "0", CVAR_ARCHIVE );
	trap_Cvar_Get( "ui_filter_gametype", "0", CVAR_ARCHIVE );
	trap_Cvar_Get( "ui_filter_mod", "0", CVAR_ARCHIVE );
	trap_Cvar_Get( "ui_filter_instagib", "0", CVAR_ARCHIVE );
	trap_Cvar_Get( "ui_filter_ping", "0", CVAR_ARCHIVE );
	trap_Cvar_Get( "ui_sortmethod", "1", CVAR_ARCHIVE );
	trap_Cvar_Get( "ui_sortmethod_direction", "0", CVAR_ARCHIVE );
	trap_Cvar_Get( "ui_filter_tv", "0", CVAR_ARCHIVE );

	ui_filter_gametype_names = trap_Cvar_Get( "ui_filter_gametype_names", "show all;dm;duel;tdm;ctf;race;ca;tdo;da;headhunt", CVAR_ARCHIVE|CVAR_NOSET );
	UI_NamesListCvarAddName( ui_filter_gametype_names, "show all", ';' );
	M_Browser_UpdateGametypesList( ui_filter_gametype_names->string );

	ui_filter_mod_names = trap_Cvar_Get( "ui_filter_mod_names", "", CVAR_ARCHIVE|CVAR_NOSET );
	UI_NamesListCvarAddName( ui_filter_mod_names, "show all", ';' );
	UI_NamesListCvarAddName( ui_filter_mod_names, "don't show", ';' );
	M_Browser_UpdateModsList( ui_filter_mod_names->string );

	if( uis.vidWidth < 800 )
		scrollwindow_width = uis.vidWidth * 0.95;
	else if( uis.vidWidth < 1024 )
		scrollwindow_width = uis.vidWidth * 0.90;
	else
		scrollwindow_width = uis.vidWidth * 0.70;

	s_joinserver_menu.nitems = 0;

	/*This is a hack, but utilizes the most room at low resolutions
	   while leaving room so that the logo does not interfere. */
	if( uis.vidHeight < 768 )
		yoffset = uis.vidHeight * 0.07 - 32;
	else
		yoffset = uis.vidHeight * 0.1125 - 32;
	xoffset = scrollwindow_width / 2;

	menuitem = UI_InitMenuItem( "m_connect_title1", "SERVER BROWSER", 0, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemBig, NULL );
	Menu_AddItem( &s_joinserver_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );

	yoffset += 16;

	menuitem_filters_background = UI_InitMenuItem( "m_connect_filters", "", -xoffset, yoffset, MTYPE_SEPARATOR, ALIGN_LEFT_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_joinserver_menu, menuitem_filters_background );
	// create an associated picture to the items to act as window background
	menuitem = menuitem_filters_background;
	menuitem->ownerdraw = M_DrawFiltersBox;
	menuitem->pict.shader = uis.whiteShader;
	menuitem->pict.shaderHigh = NULL;
	Vector4Copy( colorMdGrey, menuitem->pict.color );
	menuitem->pict.color[3] = 0;
	menuitem->pict.yoffset = 0;
	menuitem->pict.xoffset = 0;
	menuitem->pict.width = scrollwindow_width;
	menuitem->pict.height = yoffset + menuitem->pict.yoffset; // will be set later

	yoffset += 0.5 * trap_SCR_strHeight( menuitem_filters_background->font );

	menuitem_fullfilter = UI_InitMenuItem( "m_connect_filterfull", "full", UI_SCALED_WIDTH( -100 ), yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, M_Connect_UpdateFiltersSettings );
	Menu_AddItem( &s_joinserver_menu, menuitem_fullfilter );
	UI_SetupSpinControl( menuitem_fullfilter, filter_names, trap_Cvar_Value( "ui_filter_full" ) );
	//yoffset += trap_SCR_strHeight( menuitem_fullfilter->font );

	menuitem_gametypefilter = UI_InitMenuItem( "m_connect_filtergametype", "gametype", UI_SCALED_WIDTH( 120 ), yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, M_Connect_UpdateFiltersSettings );
	Menu_AddItem( &s_joinserver_menu, menuitem_gametypefilter );
	UI_SetupSpinControl( menuitem_gametypefilter, gametype_filternames, trap_Cvar_Value( "ui_filter_gametype" ) );
	yoffset += trap_SCR_strHeight( menuitem_gametypefilter->font );

	menuitem_emptyfilter = UI_InitMenuItem( "m_connect_filterempty", "empty", UI_SCALED_WIDTH( -100 ), yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, M_Connect_UpdateFiltersSettings );
	Menu_AddItem( &s_joinserver_menu, menuitem_emptyfilter );
	UI_SetupSpinControl( menuitem_emptyfilter, filter_names, trap_Cvar_Value( "ui_filter_empty" ) );
	//yoffset += trap_SCR_strHeight( menuitem_emptyfilter->font );

	menuitem_modfilter = UI_InitMenuItem( "m_connect_filtermod", "mod", UI_SCALED_WIDTH( 120 ), yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, M_Connect_UpdateFiltersSettings );
	Menu_AddItem( &s_joinserver_menu, menuitem_modfilter );
	UI_SetupSpinControl( menuitem_modfilter, mod_filternames, trap_Cvar_Value( "ui_filter_mod" ) );
	yoffset += trap_SCR_strHeight( menuitem_modfilter->font );

	

	menuitem_passwordfilter = UI_InitMenuItem( "m_connect_filterpassword", "password", UI_SCALED_WIDTH( -100 ), yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, M_Connect_UpdateFiltersSettings );
	Menu_AddItem( &s_joinserver_menu, menuitem_passwordfilter );
	UI_SetupSpinControl( menuitem_passwordfilter, filter_names, trap_Cvar_Value( "ui_filter_password" ) );
	//yoffset += trap_SCR_strHeight( menuitem_passwordfilter->font );

	menuitem_skillfilter = UI_InitMenuItem( "m_connect_filterskill", "skill", UI_SCALED_WIDTH( 120 ), yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, M_Connect_UpdateFiltersSettings );
	Menu_AddItem( &s_joinserver_menu, menuitem_skillfilter );
	UI_SetupSpinControl( menuitem_skillfilter, skill_filternames, trap_Cvar_Value( "ui_filter_skill" ) );
	yoffset += trap_SCR_strHeight( menuitem_skillfilter->font );

	menuitem_instagibfilter = UI_InitMenuItem( "m_connect_filterinstagib", "instagib", UI_SCALED_WIDTH( -100 ), yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, M_Connect_UpdateFiltersSettings );
	Menu_AddItem( &s_joinserver_menu, menuitem_instagibfilter );
	UI_SetupSpinControl( menuitem_instagibfilter, filter_names, trap_Cvar_Value( "ui_filter_instagib" ) );

	menuitem_maxpingfilter = UI_InitMenuItem( "m_connect_filterping", "maxping", UI_SCALED_WIDTH( 120 ), yoffset, MTYPE_FIELD, ALIGN_RIGHT_TOP, uis.fontSystemSmall, M_Connect_UpdateFiltersSettings );
	Menu_AddItem( &s_joinserver_menu, menuitem_maxpingfilter );
	UI_SetupField( menuitem_maxpingfilter, trap_Cvar_String( "ui_filter_ping" ), 4, -1 );
	UI_SetupFlags( menuitem_maxpingfilter, F_NUMBERSONLY );
	yoffset += trap_SCR_strHeight( menuitem_maxpingfilter->font );

	menuitem_tvfilter = UI_InitMenuItem( "m_connect_filtertv", "tv", UI_SCALED_WIDTH( -100 ), yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, M_Connect_UpdateFiltersSettings );
	Menu_AddItem( &s_joinserver_menu, menuitem_tvfilter );
	UI_SetupSpinControl( menuitem_tvfilter, tvfilter_names, trap_Cvar_Value( "ui_filter_tv" ) );
	yoffset += trap_SCR_strHeight( menuitem_tvfilter->font );

	menuitem_namematchfilter = UI_InitMenuItem( "m_connect_filternamematch", "matched name", UI_SCALED_WIDTH( 120 ), yoffset, MTYPE_FIELD, ALIGN_RIGHT_TOP, uis.fontSystemSmall, M_Connect_UpdateFiltersSettings );
	Menu_AddItem( &s_joinserver_menu, menuitem_namematchfilter );
	UI_SetupField( menuitem_namematchfilter, trap_Cvar_String( "ui_filter_name" ), 12, -1 );
	yoffset += trap_SCR_strHeight( menuitem_namematchfilter->font );

	// here ends the filters background, set it's image height now
	menuitem_filters_background->pict.height = yoffset - menuitem_filters_background->pict.height + ( 0.5 * trap_SCR_strHeight( menuitem_skillfilter->font ) );

	yoffset += trap_SCR_strHeight( menuitem_skillfilter->font );

	menuitem = UI_InitMenuItem( "m_connect_search_type", "search", UI_SCALED_WIDTH( 120 ), yoffset, MTYPE_SPINCONTROL, MTYPE_SPINCONTROL, uis.fontSystemSmall, M_Connect_ChangeSearchType );
	Menu_AddItem( &s_joinserver_menu, menuitem );
	UI_SetupSpinControl( menuitem, search_names, trap_Cvar_Value( "ui_searchtype" ) );

	menuitem = UI_InitMenuItem( "m_connect_allow_modules", "Download modules", UI_SCALED_WIDTH( -100 ), yoffset, MTYPE_SPINCONTROL, MTYPE_SPINCONTROL, uis.fontSystemSmall, M_Connect_ToggleAllowModules );
	Menu_AddItem( &s_joinserver_menu, menuitem );
	UI_SetupSpinControl( menuitem, noyes_names, trap_Cvar_Value( ALLOW_DOWNLOAD_MODULES ) );

	yoffset += trap_SCR_strHeight( menuitem->font );
	yoffset += trap_SCR_strHeight( menuitem->font );

	// sortbar
	menuitem = UI_InitMenuItem( "m_connect_sortbar_ping", NULL, -xoffset, yoffset, MTYPE_SPINCONTROL, ALIGN_LEFT_TOP, uis.fontSystemSmall, M_Connect_UpdateSortbar );

	UI_SetupSpinControl( menuitem, sort_ping, trap_Cvar_Value( "ui_sortmethod_direction" ) );
	menuitem->sort_active = trap_Cvar_Value( "ui_sortmethod" );
	menuitem->sort_type = SORT_PING;
	menuitem->pict.xoffset = 36;
	Menu_AddItem( &s_joinserver_menu, menuitem );

	menuitem = UI_InitMenuItem( "m_connect_sortbar_players", NULL, menuitem->x + COLUMN_WIDTH_PING, yoffset, MTYPE_SPINCONTROL, ALIGN_LEFT_TOP, uis.fontSystemSmall, M_Connect_UpdateSortbar );
	UI_SetupSpinControl( menuitem, sort_players, trap_Cvar_Value( "ui_sortmethod_direction" ) );
	menuitem->sort_active = trap_Cvar_Value( "ui_sortmethod" );
	menuitem->sort_type = SORT_PLAYERS;
	menuitem->pict.xoffset = 58;
	Menu_AddItem( &s_joinserver_menu, menuitem );

	menuitem = UI_InitMenuItem( "m_connect_sortbar_gametype", NULL, menuitem->x + COLUMN_WIDTH_PLAYERS, yoffset, MTYPE_SPINCONTROL, ALIGN_LEFT_TOP, uis.fontSystemSmall, M_Connect_UpdateSortbar );
	UI_SetupSpinControl( menuitem, sort_gametype, trap_Cvar_Value( "ui_sortmethod_direction" ) );
	menuitem->sort_active = trap_Cvar_Value( "ui_sortmethod" );
	menuitem->sort_type = SORT_GAMETYPE;
	menuitem->pict.xoffset = 70;
	Menu_AddItem( &s_joinserver_menu, menuitem );

	menuitem = UI_InitMenuItem( "m_connect_sortbar_map", NULL, menuitem->x + COLUMN_WIDTH_GAMETYPE, yoffset, MTYPE_SPINCONTROL, ALIGN_LEFT_TOP, uis.fontSystemSmall, M_Connect_UpdateSortbar );
	UI_SetupSpinControl( menuitem, sort_map, trap_Cvar_Value( "ui_sortmethod_direction" ) );
	menuitem->sort_active = trap_Cvar_Value( "ui_sortmethod" );
	menuitem->sort_type = SORT_MAP;
	menuitem->pict.xoffset = 28;
	Menu_AddItem( &s_joinserver_menu, menuitem );

	menuitem = UI_InitMenuItem( "m_connect_sortbar_mod", NULL, menuitem->x + COLUMN_WIDTH_MAP, yoffset, MTYPE_SPINCONTROL, ALIGN_LEFT_TOP, uis.fontSystemSmall, M_Connect_UpdateSortbar );
	UI_SetupSpinControl( menuitem, sort_mod, trap_Cvar_Value( "ui_sortmethod_direction" ) );
	menuitem->sort_active = trap_Cvar_Value( "ui_sortmethod" );
	menuitem->sort_type = SORT_MOD;
	menuitem->pict.xoffset = 28;
	Menu_AddItem( &s_joinserver_menu, menuitem );

	menuitem = UI_InitMenuItem( "m_connect_sortbar_name", NULL, menuitem->x + COLUMN_WIDTH_MOD, yoffset, MTYPE_SPINCONTROL, ALIGN_LEFT_TOP, uis.fontSystemSmall, M_Connect_UpdateSortbar );
	UI_SetupSpinControl( menuitem, sort_name, trap_Cvar_Value( "ui_sortmethod_direction" ) );
	menuitem->sort_active = trap_Cvar_Value( "ui_sortmethod" );
	menuitem->sort_type = SORT_NAME;
	menuitem->pict.xoffset = 36;
	Menu_AddItem( &s_joinserver_menu, menuitem );

	yoffset += trap_SCR_strHeight( menuitem->font );

	// scrollbar
	vspacing = trap_SCR_strHeight( uis.fontSystemSmall ) + 4;
	scrollwindow_height = uis.vidHeight - ( yoffset + ( 6 * trap_SCR_strHeight( uis.fontSystemBig ) ) );
	MAX_MENU_SERVERS = scrollwindow_height / vspacing;
	if( MAX_MENU_SERVERS < 5 ) MAX_MENU_SERVERS = 5;

	menuitem = UI_InitMenuItem( "m_connect_scrollbar", NULL, xoffset, yoffset, MTYPE_SCROLLBAR, ALIGN_LEFT_TOP, uis.fontSystemSmall, M_Connect_UpdateScrollbar );
	menuitem->vspacing = vspacing;
	menuitem->scrollbar_id = scrollbar_id = s_joinserver_menu.nitems; //give the scrollbar an id to pass onto its list
	Q_strncpyz( menuitem->title, va( "ui_connect_scrollbar%i_curvalue", scrollbar_id ), sizeof( menuitem->title ) );
	if( !trap_Cvar_Value( menuitem->title ) )
		trap_Cvar_SetValue( menuitem->title, 0 );
	UI_SetupScrollbar( menuitem, MAX_MENU_SERVERS, trap_Cvar_Value( menuitem->title ), 0, 0 );
	Menu_AddItem( &s_joinserver_menu, menuitem );

	for( i = 0; i < MAX_MENU_SERVERS; i++ )
	{
		menuitem = UI_InitMenuItem( va( "m_connect_button_%i", i ), NO_SERVER_STRING, -xoffset, yoffset, MTYPE_ACTION, ALIGN_LEFT_TOP, uis.fontSystemSmall, NULL );
		menuitem->callback_doubleclick = M_Connect_Joinserver;
		menuitem->box = qfalse;
		menuitem->scrollbar_id = scrollbar_id; //id of the scrollbar so that mwheelup/down can scroll from the list
		menuitem->height = vspacing;
		menuitem->statusbar = "press ENTER to connect, f to add to favorites";
		menuitem->ownerdraw = M_UpdateSeverButton;
		menuitem->localdata[0] = i; // line in the window
		menuitem->localdata[1] = i; // line in list
		menuitem->width = scrollwindow_width; // adjust strings to this width
		Menu_AddItem( &s_joinserver_menu, menuitem );

		// create an associated picture to the items to act as window background
		menuitem->pict.shader = uis.whiteShader;
		menuitem->pict.shaderHigh = uis.whiteShader;
		Vector4Copy( colorWhite, menuitem->pict.colorHigh );
		Vector4Copy( ( i & 1 ) ? colorDkGrey : colorMdGrey, menuitem->pict.color );
		menuitem->pict.color[3] = menuitem->pict.colorHigh[3] = 0.65f;
		menuitem->pict.yoffset = 0;
		menuitem->pict.xoffset = 0;
		menuitem->pict.width = scrollwindow_width;
		menuitem->pict.height = vspacing;

		yoffset += vspacing;
	}

	yoffset += trap_SCR_strHeight( menuitem->font );

	// search and stop - align to the left
	// search
	xoffset = -scrollwindow_width / 2;
	menuitem = UI_InitMenuItem( "m_connect_search", "refresh", xoffset, yoffset, MTYPE_ACTION, ALIGN_LEFT_TOP, uis.fontSystemBig, SearchGamesFunc );
	Menu_AddItem( &s_joinserver_menu, menuitem );
	UI_SetupButton( menuitem, qtrue );
	Q_snprintfz( sbar, sizeof( sbar ), "Master server at %s", trap_Cvar_String( "masterservers" ) );
	menuitem->statusbar = sbar;
	xoffset += ( menuitem->width + 16 );

	// stop
	menuitem = UI_InitMenuItem( "m_connect_search_stop", "stop", xoffset, yoffset, MTYPE_ACTION, ALIGN_LEFT_TOP, uis.fontSystemBig, M_StopSearch );
	Menu_AddItem( &s_joinserver_menu, menuitem );
	UI_SetupButton( menuitem, qtrue );

	// back and join - align to the right
	// join
	xoffset = scrollwindow_width / 2;
	menuitem = UI_InitMenuItem( "m_connect_join", "join", xoffset, yoffset, MTYPE_ACTION, ALIGN_RIGHT_TOP, uis.fontSystemBig, M_Connect_Joinserver );
	Menu_AddItem( &s_joinserver_menu, menuitem );
	UI_SetupButton( menuitem, qtrue );
	xoffset -= ( menuitem->width + 16 );

	// back
	menuitem = UI_InitMenuItem( "m_connect_back", "back", xoffset, yoffset, MTYPE_ACTION, ALIGN_RIGHT_TOP, uis.fontSystemBig, M_genericBackFunc );
	Menu_AddItem( &s_joinserver_menu, menuitem );
	yoffset += UI_SetupButton( menuitem, qtrue ) + UI_BUTTONBOX_VERTICAL_SPACE;

	Menu_Center( &s_joinserver_menu ); //this is effectless, but we'll leave it to keep consistant.
	Menu_Init( &s_joinserver_menu, qfalse );

	// start pinging servers
	SearchGamesFunc( UI_MenuItemByName( "m_connect_search" ) );
}


static void JoinServer_MenuDraw( void )
{
	PingServers();
	Menu_Draw( &s_joinserver_menu );
}

static const char *JoinServer_MenuKey( int key )
{
	menucommon_t *item;

	item = Menu_ItemAtCursor( &s_joinserver_menu );

	if( key == K_ESCAPE || ( ( key == K_MOUSE2 ) && ( item->type != MTYPE_SPINCONTROL ) &&
	                        ( item->type != MTYPE_SLIDER ) ) )
	{
	}

	return Default_MenuKey( &s_joinserver_menu, key );
}

static const char *JoinServer_MenuCharEvent( qwchar key )
{
	return Default_MenuCharEvent( &s_joinserver_menu, key );
}

void M_Menu_JoinServer_f( void )
{
	JoinServer_MenuInit();
	M_PushMenu( &s_joinserver_menu, JoinServer_MenuDraw, JoinServer_MenuKey, JoinServer_MenuCharEvent );
}
