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

#include "tv_local.h"

#include "tv_relay_svcmd.h"

#include "tv_relay.h"
#include "tv_relay_module.h"
#include "tv_relay_client.h"
#include "tv_upstream.h"
#include "tv_downstream.h"

/*
   ==================
   TV_Relay_ParseConfigstringCommand_f
   ==================
 */
static void TV_Relay_ParseConfigstringCommand_f( relay_t *relay )
{
	int index;
	size_t len;
	char *val;

	if( Cmd_Argc() != 3 )
		return;

	index = atoi( Cmd_Argv( 1 ) );
	val = Cmd_Argv( 2 );

	if( index < 0 || index >= MAX_CONFIGSTRINGS )
		TV_Relay_Error( relay, "configstring > MAX_CONFIGSTRINGS" );

	// wsw : jal : warn if configstring overflow
	len = strlen( val );
	if( len >= MAX_CONFIGSTRING_CHARS )
	{
		Com_Printf( "Warning: Configstring %i overflowed: %s\n", index, val );
		len = MAX_CONFIGSTRING_CHARS - 1;
	}

	if( !COM_ValidateConfigstring( val ) )
	{
		Com_Printf( "Warning: Configstring %i invalid: %s\n", index, val );
		return;
	}

	// game module can prohibit changing this configstring
	if( relay->state == CA_ACTIVE && !relay->module_export->ConfigString( relay->module, index, val ) )
		return;

	// ignore if no changes
	if( !strncmp( relay->configstrings[index], val, len ) && relay->configstrings[index][len] == '\0' )
		return;

	Q_strncpyz( relay->configstrings[index], val, sizeof( relay->configstrings[index] ) );

	if( relay->state == CA_ACTIVE )
	{
		// We have to manually broadcast this one.
		client_t *client;
		int i;
		for( i = 0, client = tvs.clients; i < tv_maxclients->integer; i++, client++ )
		{
			if( client->state < CS_CONNECTED )
				continue;
			if( client->relay != relay )
				continue;
			TV_Downstream_SendServerCommand( client, "cs %i \"%s\"", index, val );
		}
	}

#if 0
	if( ( index == CS_HOSTNAME || index == CS_MATCHNAME ) 
		|| ( ( index == CS_MAPNAME || index == CS_GAMETYPENAME ) && val[0] != '\0' ) ) {
		TV_Relay_NameNotify( relay, NULL );
	}
#else
	if( ( index == CS_HOSTNAME ) 
		|| ( ( index == CS_MAPNAME ) && val[0] != '\0' ) ) {
		TV_Relay_NameNotify( relay, NULL );
	}
#endif
}

/*
   =================
   TV_Relay_Precache_f
   =================
 */
static void TV_Relay_Precache_f( relay_t *relay )
{
	int i;

	// TODO actually check that we have the stuff
	TV_Relay_InitModule( relay );
	TV_Relay_InitMap( relay );

	// update clients that have've been waiting for the module to go up
	for( i = 0; i < tv_maxclients->integer; i++ )
	{
		if( tvs.clients[i].relay == relay )
			TV_Relay_ClientConnect( relay, &tvs.clients[i] );
	}
}

/*
   =================
   TV_Relay_Multiview_f
   =================
 */
static void TV_Relay_Multiview_f( relay_t *relay )
{
	relay->multiview = ( atoi( Cmd_Argv( 1 ) ) != 0 );
}

/*
   =================
   TV_Relay_ServerReconnect_f

   The server is changing levels
   =================
 */
static void TV_Relay_ServerReconnect_f( relay_t *relay )
{
	TV_Relay_ReconnectClients( relay );
}

/*
   =================
   TV_Relay_ServerDisconnect_f

   The server is changing levels
   =================
 */
static void TV_Relay_ServerDisconnect_f( relay_t *relay )
{
	int type;

	type = atoi( Cmd_Argv( 1 ) );
	if( type < 0 || type >= DROP_TYPE_TOTAL )
		type = DROP_TYPE_GENERAL;

	TV_Relay_Error( relay, "Server disconnected: %s", Cmd_Argv( 2 ) );
}

// ---

typedef struct
{
	char *name;
	void ( *func )( relay_t *relay );
} svcmd_t;

static svcmd_t svcmds[] =
{
	{ "forcereconnect", TV_Relay_ServerReconnect_f },
	{ "reconnect", TV_Relay_ServerReconnect_f },
	{ "changing", NULL },
	{ "precache", TV_Relay_Precache_f },
	{ "cmd", NULL },
	{ "cs", TV_Relay_ParseConfigstringCommand_f },
	{ "disconnect", TV_Relay_ServerDisconnect_f },
	//{ "initdownload", CL_InitDownload_f },
	{ "multiview", TV_Relay_Multiview_f },

	{ NULL, NULL }
};

/*
   ==================
   TV_Relay_ParseServerCommand
   ==================
 */
void TV_Relay_ParseServerCommand( relay_t *relay, msg_t *msg )
{
	const char *s;
	char *text;
	svcmd_t *cmd;

	text = MSG_ReadString( msg );
	Cmd_TokenizeString( text );
	s = Cmd_Argv( 0 );

	// filter out these server commands to be called from the client
	for( cmd = svcmds; cmd->name; cmd++ )
	{
		if( !strcmp( s, cmd->name ) )
		{
			if( cmd->func )
				cmd->func( relay );
			return;
		}
	}

	Com_Printf( "Unknown server command: %s\n", s );
}
