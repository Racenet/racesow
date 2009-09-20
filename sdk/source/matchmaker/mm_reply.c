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

#include "matchmaker.h"

typedef enum
{
	REPLY_NOMM,
	REPLY_FAILED,
	REPLY_SUCCESS
} mm_reply_t;

//================
// MM_ReplyToLock
// Tell the match clients to ping the server
//================
static void MM_ReplyToLock( mm_server_t *server, mm_reply_t reply )
{
	if( !server->match )
		return;

	// choose another server to try
	switch( reply )
	{
	case REPLY_NOMM:
	case REPLY_FAILED:
		MM_AssignGameServer( server->match );
		break;
	case REPLY_SUCCESS:
		MM_SendMsgToClients( server->match, "pingserver %s", NET_AddressToString( &server->address ) );
		break;
	default:
		break;
	}
}

//================
// MM_ReplyToSetup
// Tell the match clients 
//================
static void MM_ReplyToSetup( mm_server_t *server, mm_reply_t reply )
{
	if( !server->match )
		return;

	switch( reply )
	{
	case REPLY_NOMM:
	case REPLY_FAILED:
		MM_AssignGameServer( server->match );
		break;
	case REPLY_SUCCESS:
		MM_SendMsgToClients( server->match, "start %s", NET_AddressToString( &server->address ) );
		MM_FreeMatch( server->match, qfalse );
		server->killtime = 0;
		break;
	default:
		break;
	}
}

//================
// Supported replies
//================
typedef struct
{
	char *name;
	void ( *func )( mm_server_t *server, mm_reply_t reply );
} mm_replycmd_t;

mm_replycmd_t mm_replies[] =
{
	{ "lock", MM_ReplyToLock },
	{ "setup", MM_ReplyToSetup },
	{ "unlock", NULL },

	{ NULL, NULL }
};

//================
// MM_GameServerReply
// Handles a reply from a gameserver
//================
void MM_GameServerReply( mm_server_t *server )
{
	mm_replycmd_t *cmd;
	char *name, *replystr;
	mm_reply_t reply;

	name = Cmd_Argv( 1 );
	if( !name || !*name )
		return;

	replystr = Cmd_Argv( 2 );
	if( !replystr || !*replystr )
		return;

	if( !strcmp( replystr, "nomm" ) )
		reply = REPLY_NOMM;
	else if( !strcmp( replystr, "failed" ) )
		reply = REPLY_FAILED;
	else if( !strcmp( replystr, "success" ) )
		reply = REPLY_SUCCESS;
	else return;

	for( cmd = mm_replies ; cmd->name ; cmd++ )
	{
		if( !strcmp( cmd->name, name ) )
		{
			if( cmd->func )
				cmd->func( server, reply );
			break;
		}
	}

	if( reply == REPLY_NOMM )
		MM_FreeGameServer( server );
}
