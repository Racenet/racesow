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

#include "client.h"
#include "../matchmaker/mm_ui.h"
#include "../matchmaker/mm_common.h"

#ifdef MATCHMAKER_SUPPORT

#ifndef TCP_SUPPORT
#	ifdef _MSC_VER
#		pragma message( "TCP support needed for matchmaker" )
#	else
#		warning TCP support needed for matchmaker
#	endif
#endif

static void CL_MM_ReadPackets( void );
static void CL_MM_CheckConnection( void );
static void CL_MM_ProcessQueue( void );
static void CL_MM_StayAlive( void );
static void CL_MM_FreePacketQueue( void );

static void CL_MM_Join( const char *gametype );
static void CL_MM_Drop( void );
static void CL_MM_Disconnect( void );
static void CL_MM_Chat( int type, char *msg );
static void CL_MM_Connect( void );
static void CL_MM_AddMatch( const char *gametype, int maxclients, int scorelimit, float timelimit, const char *skilltype );

static void CL_MMC_Add( msg_t *msg );
static void CL_MMC_Chat( msg_t *msg );
static void CL_MMC_Drop( msg_t *msg );
static void CL_MMC_PingServer( msg_t *msg );
static void CL_MMC_Joined( msg_t *msg );
static void CL_MMC_Shutdown( msg_t *msg );
static void CL_MMC_Start( msg_t *msg );
static void CL_MMC_Status( msg_t *msg );

//================
// private vars
//================
static struct
{
	netadr_t address;
	unsigned int pinged;
} cl_mm_pingserver;

static qboolean cl_mm_initialized = qfalse;
static qboolean cl_mm_starting = qfalse;

static mm_packet_t *cl_mm_packetqueue;
static mm_status_t cl_mm_status = STATUS_DISCONNECTED;

static socket_t cl_mm_socket;
static netadr_t cl_mm_address;

//================
// public vars
//================
cvar_t *cl_mmserver;

//================
// CL_MM_Init
// Initialize client matchmaking components
//================
void CL_MM_Init( void )
{
	if( cl_mm_initialized )
		return;

	cl_mmserver = Cvar_Get( "mmserver", MM_SERVER_IP, CVAR_ARCHIVE );

	cl_mm_initialized = qtrue;
}

//================
// CL_MM_Shutdown
// Shutdown client matchmaking components
//================
void CL_MM_Shutdown( void )
{
	if( !cl_mm_initialized )
		return;

	cl_mm_initialized = qfalse;
}

//================
// CL_MM_Frame
// Called every client frame
//================
void CL_MM_Frame( void )
{
	if( cl_mm_starting )
	{
		CL_MM_Disconnect();
		cl_mm_starting = qfalse;
	}

	CL_MM_CheckConnection();
	CL_MM_StayAlive();
	CL_MM_ReadPackets();
	CL_MM_ProcessQueue();
}

//================
// CL_MM_SendMsgToServer
// Send a message to matchmaker. Handles packet queuing if connection isn't established
//================
void CL_MM_SendMsgToServer( const char *format, ... )
{
	va_list argptr;
	char msg[MAX_PACKETLEN];

	if( !format || !*format )
		return;

	va_start( argptr, format );
	Q_vsnprintfz( msg, sizeof( msg ), format, argptr );
	va_end( argptr );

	if( cl_mm_status == STATUS_DISCONNECTED )
	{
		CL_MM_Connect();
		// if status is still disconnected
		if( cl_mm_status == STATUS_DISCONNECTED )
		{
			Com_Printf( "Could not send data to matchmaker: %s\n", msg );
			return;
		}
	}

	if( cl_mm_status == STATUS_CONNECTING )
	{
		mm_packet_t *packet, *end;
		int size;

		// Create packet
		size = strlen( msg ) + 1;
		packet = ( mm_packet_t * )Mem_TempMalloc( sizeof( mm_packet_t ) );
		MSG_Init( &packet->msg, ( qbyte * )Mem_TempMalloc( size ), size );
		MSG_WriteData( &packet->msg, msg, size );

		// Add to packet queue
		if( !cl_mm_packetqueue )
		{
			cl_mm_packetqueue = packet;
			return;
		}

		// Find the end of the queue, and add new packet there
		for( end = cl_mm_packetqueue ; end->next ; end = end->next );
		end->next = packet;

		return;
	}

	if( !NET_SendPacket( &cl_mm_socket, msg, strlen( msg ) + 1, &cl_mm_address ) )
		CL_MM_Disconnect();
}

//================
// Accepted packets
//================
typedef struct
{
  char *name;
  void ( *func )( msg_t *msg );
} mm_cmd_t;

static mm_cmd_t mm_cmds[] =
{
  { "add", CL_MMC_Add },
  { "chat", CL_MMC_Chat },
  { "drop", CL_MMC_Drop },
	{ "joined", CL_MMC_Joined },
	{ "pingserver", CL_MMC_PingServer },
  { "shutdown", CL_MMC_Shutdown },
  { "start", CL_MMC_Start },
  { "status", CL_MMC_Status },

  { NULL, NULL }
};

//================
// CL_MM_Packet
//================
static void CL_MM_Packet( msg_t *msg )
{
	mm_cmd_t *cmd;
	char *c;

	Cmd_TokenizeString( ( char * )msg->data );

	c = Cmd_Argv( 0 );
	if( !c || !*c )
		return;

	for( cmd = mm_cmds ; cmd->name ; cmd++ )
	{
		if( !strcmp( c, cmd->name ) )
		{
			cmd->func( msg );
			return;
		}
	}
}

//================
// CL_MM_ReadPackets
// Read packets sent by the matchmaker
//================
static void CL_MM_ReadPackets( void )
{
	msg_t msg;
	qbyte msgData[MAX_MSGLEN];
	int ret;
	netadr_t address;

	if( cl_mm_status < STATUS_CONNECTED )
		return;

	MSG_Init( &msg, msgData, sizeof( msgData ) );
	MSG_Clear( &msg );

	while( ( ret = NET_GetPacket( &cl_mm_socket, &address, &msg ) ) )
	{
		if( ret == -1 )
		{
			NET_CloseSocket( &cl_mm_socket );
			return;
		}

		if( !NET_CompareAddress( &address, &cl_mm_address ) )
			continue;

		CL_MM_Packet( &msg );
	}
}

//================
// CL_MM_CheckConnection
// Checks if the client is still connected to the matchmaker
//================
static void CL_MM_CheckConnection( void )
{
#ifdef TCP_SUPPORT
	connection_status_t status;

	if( !cl_mm_socket.open )
	{
		cl_mm_status = STATUS_DISCONNECTED;
		return;
	}

	status = NET_CheckConnect( &cl_mm_socket );
	switch( status )
	{
	case CONNECTION_FAILED:
		CL_UIModule_MM_UIReply( ACTION_JOIN, "failed" );
		CL_MM_Disconnect();
		break;
	case CONNECTION_INPROGRESS:
		cl_mm_status = STATUS_CONNECTING;
		break;
	case CONNECTION_SUCCEEDED:
		if( cl_mm_status < STATUS_CONNECTED )
			cl_mm_status = STATUS_CONNECTED;
		break;
	}
#endif
}

//================
// CL_MM_ProcessQueue
// Send any unsent packets to matchmaker (if possible)
//================
static void CL_MM_ProcessQueue( void )
{
	mm_packet_t *packet;

	if( !cl_mm_socket.open )
		return;

	if( cl_mm_status < STATUS_CONNECTED )
		return;

	// send off all outstanding packets
	for( packet = cl_mm_packetqueue ; packet ; packet = packet->next )
	{
		if( !NET_SendPacket( &cl_mm_socket, packet->msg.data, packet->msg.cursize, &cl_mm_address ) )
		{
			CL_MM_Disconnect();
			return;
		}
	}

	CL_MM_FreePacketQueue();
}

//================
// CL_MM_StayAlive
// Makes sure connection to matchmaker is kept open
//================
static void CL_MM_StayAlive( void )
{
	static unsigned int lastping = 0;

	if( cl_mm_status < STATUS_CONNECTED )
		return;

	if( lastping + 15000 > cls.realtime )
		return;

	CL_MM_SendMsgToServer( "stayalive" );
	lastping = cls.realtime;
}

//================
// CL_MM_FreePacketQueue
// Free all memory held by packet queue
//================
static void CL_MM_FreePacketQueue( void )
{
	mm_packet_t *packet, *next;

	packet = cl_mm_packetqueue;
	while( packet )
	{
		next = packet->next;

		Mem_TempFree( packet->msg.data );
		Mem_TempFree( packet );

		packet = next;
	}
	cl_mm_packetqueue = NULL;
}

//================
// CL_MM_Connect
// Connects to matchmaker
//================
static void CL_MM_Connect( void )
{
#ifdef TCP_SUPPORT
	netadr_t address;

	memset( &address, 0, sizeof( netadr_t ) );
	address.type = NA_IP;

	CL_MM_Disconnect();

	if( !NET_StringToAddress( cl_mmserver->string, &cl_mm_address ) )
	{
		Com_Printf( "Invalid address for matchmaker: %s\n", NET_ErrorString() );
		CL_UIModule_MM_UIReply( ACTION_CONNECT, "failed" );
		return;
	}

	if( !NET_OpenSocket( &cl_mm_socket, SOCKET_TCP, &address, qfalse ) )
	{
		Com_Printf( "Unable to open matchmaker socket: %s\n", NET_ErrorString() );
		CL_UIModule_MM_UIReply( ACTION_CONNECT, "failed" );
		return;
	}

	if( NET_Connect( &cl_mm_socket, &cl_mm_address ) == CONNECTION_FAILED )
	{
		Com_Printf( "Unable to connect to matchmaker: %s\n", NET_ErrorString() );
		CL_UIModule_MM_UIReply( ACTION_CONNECT, "failed" );
		NET_CloseSocket( &cl_mm_socket );
		return;
	}

	CL_MM_CheckConnection();
#endif
}

//================
// CL_MM_Disconnect
// Disconnects from matchmaker
//================
static void CL_MM_Disconnect( void )
{
	if( !cl_mm_socket.open )
		return;

	NET_CloseSocket( &cl_mm_socket );
	CL_MM_CheckConnection();
}

//================
// CL_MM_Join
// Join a match
//================
static void CL_MM_Join( const char *gametype )
{
	CL_MM_Connect();

	CL_MM_SendMsgToServer( "join 0 \"%s\" %s", Cvar_String( "name" ), gametype );

	CL_UIModule_MM_UIReply( ACTION_JOIN, "joining" );
}

//================
// CL_MM_Drop
// Drops the client from their current match
//================
static void CL_MM_Drop( void )
{
	if( cl_mm_status < STATUS_CONNECTED )
		return;

	// Tell matchmaker we are dropping
	CL_MM_SendMsgToServer( "drop" );

	CL_UIModule_MM_UIReply( ACTION_DROP, "" );

	cl_mm_status = STATUS_CONNECTED;
}

//================
// CL_MM_Chat
// Send a chat message to the matchmaker
//================
static void CL_MM_Chat( int type, char *msg )
{
	char *chr;

	if( cl_mm_status < STATUS_CONNECTED )
		return;

	if( !msg || !*msg )
		return;

	// Convert " to '
	while( ( chr = strchr( msg, '"' ) ) )
		*chr = '\'';

	CL_MM_SendMsgToServer( "chat %d \"%s\"", type, msg );
}

//================
// CL_MM_AddMatch
// Attempts to add a match to the matchmaker
//================
static void CL_MM_AddMatch( const char *gametype, int maxclients, int scorelimit, float timelimit, const char *skilltype )
{
	if ( cl_mm_status < STATUS_CONNECTED )
		return;

	if( maxclients < 1 || maxclients > MAX_MAXCLIENTS )
		return;

	if( scorelimit < 0 || scorelimit > MAX_SCORELIMIT )
		return;

	if( timelimit < 0 || timelimit > MAX_TIMELIMIT )
		return;

	if( !scorelimit && !timelimit )
		return;

	// drop from any existing match
	CL_MM_Drop();

	CL_MM_SendMsgToServer( "addmatch 0 \"%s\" %s %d %d %.2f %s", Cvar_String( "name" ),  gametype, maxclients, scorelimit, timelimit, skilltype );

	CL_UIModule_MM_UIReply( ACTION_ADDMATCH, "adding" );
}

//================
// CL_MMC_Acknowledge
// Ping acknowledgement from someone
//================
void CL_MMC_Acknowledge( const netadr_t *address )
{
	if( NET_CompareAddress( &cl_mm_pingserver.address, address ) )
		CL_MM_SendMsgToServer( "pingserver %s %u", NET_AddressToString( address ), cls.realtime - cl_mm_pingserver.pinged );
}

//================
// CL_MMC_Add
// Add a player to the client's current match
//================
static void CL_MMC_Add( msg_t *msg )
{
	int i, slot;
	char *nickname;

	// check we have the correct number of parameters
	if( ( ( Cmd_Argc() - 1 ) % 2 ) )
		return;

	for( i = 1 ; i < Cmd_Argc() ; i += 2 )
	{
		slot = atoi( Cmd_Argv( i ) );
		nickname = Cmd_Argv( i + 1 );

		if( !nickname || !*nickname )
			continue;

		CL_UIModule_MM_UIReply( ACTION_JOIN, va( "add %d \"%s\"", slot, nickname ) );
	}
}

//================
// CL_MMC_Chat
//================
static void CL_MMC_Chat( msg_t *msg )
{
	char *nickname, *chat;

	if( Cmd_Argc() != 3 )
		return;

	nickname = Cmd_Argv( 1 );
	chat = Cmd_Argv( 2 );
	if( !nickname || !*nickname || !chat || !*chat )
		return;

	// We format it like this so UI knows where to seperate the string for scrolling
	CL_UIModule_MM_UIReply( ACTION_CHAT, va( "%s%c%s", nickname, '\n', chat ) );
}

//================
// CL_MMC_Drop
// Remove a player from the client's current match
//================
static void CL_MMC_Drop( msg_t *msg )
{
	int slot;

	if( Cmd_Argc() != 2 ) 
		return;

	slot = atoi( Cmd_Argv( 1 ) );
	if( slot < 0 )
		return;

	CL_UIModule_MM_UIReply( ACTION_DROP, va( "%d", slot ) );
}

//================
// CL_MMC_Joined
// The client has successfully joined a match
//================
static void CL_MMC_Joined( msg_t *msg )
{
	int maxclients, scorelimit, skilllevel, slot;
	float timelimit;
	char *gametype, *skilltype;

	if( Cmd_Argc() == 2 && !strcmp( Cmd_Argv( 1 ), "nomatches" ) )
	{
		CL_UIModule_MM_UIReply( ACTION_JOIN, "nomatches" );
		return;
	}

	if( Cmd_Argc() != 8 )
		return;

	maxclients = atoi( Cmd_Argv( 1 ) );
	if( maxclients < 0 || maxclients >= MAX_CLIENTS )
		return;

	gametype = Cmd_Argv( 2 );

	scorelimit = atoi( Cmd_Argv( 3 ) );
	if( scorelimit < 0 || scorelimit > MAX_SCORELIMIT )
		return;

	timelimit = atof( Cmd_Argv( 4 ) );
	if( timelimit < 0 || timelimit > MAX_TIMELIMIT )
		return;

	if( !scorelimit && !timelimit )
		return;

	skilltype = Cmd_Argv( 5 );

	skilllevel = atoi( Cmd_Argv( 6 ) );
	if( !strcmp( skilltype, "dependent" ) && skilllevel < 0 )
		return;

	slot = atoi( Cmd_Argv( 7 ) );
	if( slot < 0 || slot >= maxclients )
		return;

	CL_UIModule_MM_UIReply( ACTION_JOIN, va( "joined %d %s %d %.2f %s %d %d", maxclients, gametype, scorelimit, timelimit, skilltype, skilllevel, slot ) );

	cl_mm_status = STATUS_MATCHMAKING;
}

//================
// CL_MMC_PingServer
//================
static void CL_MMC_PingServer( msg_t *msg )
{
	if( Cmd_Argc() != 2 )
		return;

	if( !NET_StringToAddress( Cmd_Argv( 1 ), &cl_mm_pingserver.address ) )
		return;

	Netchan_OutOfBandPrint( &cls.socket_udp, &cl_mm_pingserver.address, "ping" );
	cl_mm_pingserver.pinged = cls.realtime;
}

//================
// CL_MMC_Shutdown
// Matchmaker is shutting down
//================
static void CL_MMC_Shutdown( msg_t *msg )
{
	CL_UIModule_MM_UIReply( ACTION_STATUS, "matchmaker shutting down" );
}

//================
// CL_MMC_Start
// Connect the client to a gameserver to begin the match
//================
static void CL_MMC_Start( msg_t *msg )
{
	char *ip;
	netadr_t address;

	if( Cmd_Argc() != 2 )
		return;

	ip = Cmd_Argv( 1 );
	if( !ip || !*ip )
		return;

	// check ip is valid
	if( !NET_StringToAddress( ip, &address ) )
		return;

	cl_mm_starting = qtrue;

	Cbuf_ExecuteText( EXEC_APPEND, va( "connect %s\n", ip ) );
}

//================
// CL_MMC_Status
// Update the status bar message in the UI
//================
static void CL_MMC_Status( msg_t *msg )
{
	char *status;

	if( Cmd_Argc() < 2 )
		return;

	status = Cmd_Args();
	CL_UIModule_MM_UIReply( ACTION_CHAT, va( "%s%s", S_COLOR_WHITE, status ) );
}

//================
// CL_MM_UIRequest
// This is how the UI talks to the client
//================
void CL_MM_UIRequest( mm_action_t request, const char *data )
{
	Cmd_TokenizeString( data );

	switch( request )
	{
	case ACTION_ADDMATCH:
		CL_MM_AddMatch( Cmd_Argv( 0 ), atoi( Cmd_Argv( 1 ) ), atoi( Cmd_Argv( 2 ) ), atof( Cmd_Argv( 3 ) ), Cmd_Argv( 4 ) );
		break;
	case ACTION_JOIN:
		CL_MM_Join( data );
		break;
	case ACTION_DROP:
		CL_MM_Drop();
		break;
	case ACTION_DISCONNECT:
		CL_MM_Disconnect();
		break;
	case ACTION_STATUS:
	case ACTION_CONNECT:
	case ACTION_GETCLIENTS:
	case ACTION_GETCHANNELS:
	case ACTION_CHAT:
		break;
	}
}

//================
// CL_MM_GetStatus
// Returns client's current matchmaking status
//================
mm_status_t CL_MM_GetStatus( void )
{
	return cl_mm_status;
}

#else

void CL_MM_Init( void )
{
}

void CL_MM_Shutdown( void )
{
}

void CL_MM_Frame( void )
{
}

void CL_MMC_Acknowledge( const netadr_t *address )
{
	address = NULL;
}

void CL_MM_UIRequest( mm_action_t action, const char *data )
{
	data = NULL;
}

mm_status_t CL_MM_GetStatus( void )
{
	return STATUS_DISCONNECTED;
}

#endif
