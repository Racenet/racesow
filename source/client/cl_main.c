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
// cl_main.c  -- client main loop

#include "client.h"
#include "../qcommon/asyncstream.h"

cvar_t *cl_stereo_separation;
cvar_t *cl_stereo;

cvar_t *rcon_client_password;
cvar_t *rcon_address;

cvar_t *cl_timeout;
cvar_t *cl_maxfps;
#ifdef PUTCPU2SLEEP
cvar_t *cl_sleep;
#endif
cvar_t *cl_pps;
cvar_t *cl_compresspackets;
cvar_t *cl_shownet;

cvar_t *cl_extrapolationTime;
cvar_t *cl_extrapolate;

cvar_t *cl_timedemo;
cvar_t *cl_demoavi_video;
cvar_t *cl_demoavi_audio;
cvar_t *cl_demoavi_fps;
cvar_t *cl_demoavi_scissor;

cvar_t *sensitivity;
cvar_t *m_accel;
cvar_t *m_accelStyle;
cvar_t *m_accelOffset;
cvar_t *m_accelPow;
cvar_t *m_filter;
cvar_t *m_filterStrength;
cvar_t *m_sensCap;

cvar_t *m_pitch;
cvar_t *m_yaw;

//
// userinfo
//
cvar_t *info_password;
cvar_t *rate;

cvar_t *cl_masterservers;

// wsw : debug netcode
cvar_t *cl_debug_serverCmd;
cvar_t *cl_debug_timeDelta;

cvar_t *cl_downloads;
cvar_t *cl_downloads_from_web;
cvar_t *cl_downloads_from_web_timeout;
cvar_t *cl_download_allow_modules;
cvar_t *cl_checkForUpdate;

client_static_t	cls;
client_state_t cl;

entity_state_t cl_baselines[MAX_EDICTS];

static qboolean	cl_initialized = qfalse;

static async_stream_module_t *cl_async_stream;

//======================================================================


/*
=======================================================================

CLIENT RELIABLE COMMAND COMMUNICATION

=======================================================================
*/

/*
* CL_AddReliableCommand
* 
* The given command will be transmitted to the server, and is gauranteed to
* not have future usercmd_t executed before it is executed
*/
void CL_AddReliableCommand( /*const*/ char *cmd )
{
	int index;

	if( !cmd || !strlen( cmd ) )
		return;

	// if we would be losing an old command that hasn't been acknowledged,
	// we must drop the connection
	if( cls.reliableSequence > cls.reliableAcknowledge + MAX_RELIABLE_COMMANDS )
	{
		cls.reliableAcknowledge = cls.reliableSequence; // try to avoid loops
		Com_Error( ERR_DROP, "Client command overflow %i %i", cls.reliableAcknowledge, cls.reliableSequence );
	}
	cls.reliableSequence++;
	index = cls.reliableSequence & ( MAX_RELIABLE_COMMANDS - 1 );
	Q_strncpyz( cls.reliableCommands[index], cmd, sizeof( cls.reliableCommands[index] ) );
}

/*
* CL_UpdateClientCommandsToServer
* 
* Add the pending commands to the message
*/
void CL_UpdateClientCommandsToServer( msg_t *msg )
{
	unsigned int i;

	// write any unacknowledged clientCommands
	for( i = cls.reliableAcknowledge + 1; i <= cls.reliableSequence; i++ )
	{
		if( !strlen( cls.reliableCommands[i & ( MAX_RELIABLE_COMMANDS-1 )] ) )
			continue;

		MSG_WriteByte( msg, clc_clientcommand );
		if( !cls.reliable )
			MSG_WriteLong( msg, i );
		MSG_WriteString( msg, cls.reliableCommands[i & ( MAX_RELIABLE_COMMANDS-1 )] );
	}

	cls.reliableSent = cls.reliableSequence;
	if( cls.reliable )
		cls.reliableAcknowledge = cls.reliableSent;
}

/*
* CL_ForwardToServer_f
*/
void CL_ForwardToServer_f( void )
{
	if( cls.demo.playing )
		return;

	if( cls.state != CA_CONNECTED && cls.state != CA_ACTIVE )
	{
		Com_Printf( "Can't \"%s\", not connected\n", Cmd_Argv( 0 ) );
		return;
	}

	// don't forward the first argument
	if( Cmd_Argc() > 1 )
	{
		CL_AddReliableCommand( Cmd_Args() );
	}
}

/*
* CL_ServerDisconnect_f
*/
void CL_ServerDisconnect_f( void )
{
	char menuparms[MAX_STRING_CHARS];
	int type;
	char reason[MAX_STRING_CHARS];

	type = atoi( Cmd_Argv( 1 ) );
	if( type < 0 || type >= DROP_TYPE_TOTAL )
		type = DROP_TYPE_GENERAL;

	Q_strncpyz( reason, Cmd_Argv( 2 ), sizeof( reason ) );

	CL_Disconnect_f();

	Com_Printf( "Connection was closed by server: %s\n", reason );

	Q_snprintfz( menuparms, sizeof( menuparms ), "menu_open connfailed dropreason %i servername \"%s\" droptype %i rejectmessage \"%s\"", 
		DROP_REASON_CONNTERMINATED, cls.servername, type, reason );

	Cbuf_ExecuteText( EXEC_NOW, menuparms );
}

/*
* CL_Quit
*/
void CL_Quit( void )
{
	CL_Disconnect( NULL );
	Com_Quit();
}

/*
* CL_Quit_f
*/
static void CL_Quit_f( void )
{
	CL_Quit();
}

/*
* CL_SendConnectPacket
* 
* We have gotten a challenge from the server, so try and
* connect.
*/
static void CL_SendConnectPacket( void )
{
	userinfo_modified = qfalse;

	Com_DPrintf("CL_MM_Initialized: %d, cls.mm_ticket: %u\n", CL_MM_Initialized(), cls.mm_ticket );
	if( CL_MM_Initialized() && cls.mm_ticket != 0 )
		Netchan_OutOfBandPrint( cls.socket, &cls.serveraddress, "connect %i %i %i \"%s\" %i %u\n",
				APP_PROTOCOL_VERSION, Netchan_GamePort(), cls.challenge, Cvar_Userinfo(), 0, cls.mm_ticket );
	else
		Netchan_OutOfBandPrint( cls.socket, &cls.serveraddress, "connect %i %i %i \"%s\" %i\n",
				APP_PROTOCOL_VERSION, Netchan_GamePort(), cls.challenge, Cvar_Userinfo(), 0 );
}

/*
* CL_CheckForResend
* 
* Resend a connect message if the last one has timed out
*/
static void CL_CheckForResend( void )
{
	// FIXME: should use cls.realtime, but it can be old here after starting a server
	unsigned int realtime = Sys_Milliseconds();

	if( cls.demo.playing )
		return;

	// if the local server is running and we aren't then connect
	if( cls.state == CA_DISCONNECTED && Com_ServerState() )
	{
		CL_SetClientState( CA_CONNECTING );
		if( cls.servername )
			Mem_ZoneFree( cls.servername );
		cls.servername = ZoneCopyString( "localhost" );
		cls.servertype = SOCKET_LOOPBACK;
		NET_InitAddress( &cls.serveraddress, NA_LOOPBACK );
		if( !NET_OpenSocket( &cls.socket_loopback, cls.servertype, &cls.serveraddress, qfalse ) )
		{
			Com_Error( ERR_FATAL, "Couldn't open the loopback socket\n" );
			return;
		}
		cls.socket = &cls.socket_loopback;
	}

	// resend if we haven't gotten a reply yet
	if( cls.state == CA_CONNECTING && !cls.reliable )
	{
		if( realtime - cls.connect_time < 3000 )
			return;
		cls.connect_count++;
		cls.connect_time = realtime; // for retransmit requests

		Com_Printf( "Connecting to %s...\n", cls.servername );

		Netchan_OutOfBandPrint( cls.socket, &cls.serveraddress, "getchallenge\n" );
	}

	if( cls.state == CA_CONNECTING && cls.reliable )
	{
		if( realtime - cls.connect_time < 3000 )
			return;

#ifdef TCP_SUPPORT
		if( cls.socket->type == SOCKET_TCP && !cls.socket->connected )
		{
			connection_status_t status;

			if( !cls.connect_count )
			{
				Com_Printf( "Connecting to %s...\n", cls.servername );

				status = NET_Connect( cls.socket, &cls.serveraddress );
			}
			else
			{
				Com_Printf( "Checking connection to %s...\n", cls.servername );

				status = NET_CheckConnect( cls.socket );
			}

			cls.connect_count++;
			cls.connect_time = realtime;

			if( status == CONNECTION_FAILED )
			{
				CL_Disconnect( va( "TCP connection failed: %s", NET_ErrorString() ) );
				return;
			}

			if( status == CONNECTION_INPROGRESS )
			{
				return;
			}

			Com_Printf( "Connection made, asking for challenge %s...\n", cls.servername );
			Netchan_OutOfBandPrint( cls.socket, &cls.serveraddress, "getchallenge\n" );
			return;
		}
#endif

		if( realtime - cls.connect_time < 10000 )
			return;

		CL_Disconnect( "Connection timed out" );
	}
}

/*
* CL_Connect
*/
static void CL_Connect( const char *servername, socket_type_t type, netadr_t *address )
{
	netadr_t socketaddress;
	connstate_t newstate;

	CL_Disconnect( NULL );

	switch( type )
	{
	case SOCKET_LOOPBACK:
		NET_InitAddress( &socketaddress, NA_LOOPBACK );
		if( !NET_OpenSocket( &cls.socket_loopback, SOCKET_LOOPBACK, &socketaddress, qfalse ) )
		{
			Com_Error( ERR_FATAL, "Couldn't open the loopback socket: %s\n", NET_ErrorString() ); // FIXME
			return;
		}
		cls.socket = &cls.socket_loopback;
		cls.reliable = qfalse;
		break;

	case SOCKET_UDP:
		cls.socket = ( address->type == NA_IP6 ?  &cls.socket_udp6 :  &cls.socket_udp );
		cls.reliable = qfalse;
		break;

#ifdef TCP_SUPPORT
	case SOCKET_TCP:
		socketaddress.type = NA_IP;
		socketaddress.ip[0] = socketaddress.ip[1] = socketaddress.ip[2] = socketaddress.ip[3] = 0;
		socketaddress.port = 0;
		if( !NET_OpenSocket( &cls.socket_tcp, SOCKET_TCP, &socketaddress, qfalse ) )
		{
			Com_Error( ERR_FATAL, "Couldn't open the TCP socket\n" ); // FIXME
			return;
		}
		cls.socket = &cls.socket_tcp;
		cls.reliable = qtrue;
		break;
#endif

	default:
		assert( qfalse );
		return;
	}

	cls.servertype = type;
	cls.serveraddress = *address;
	if( NET_GetAddressPort( &cls.serveraddress ) == 0 )
		NET_SetAddressPort( &cls.serveraddress, PORT_SERVER );

	if( cls.servername )
		Mem_ZoneFree( cls.servername );
	cls.servername = ZoneCopyString( servername );

	memset( cl.configstrings, 0, sizeof( cl.configstrings ) );

	// If the server supports matchmaking and that we are authenticated, try getting a matchmaking ticket before joining the server
	newstate = CA_CONNECTING;
	if( CL_MM_Initialized() )
	{
		// if( MM_GetStatus() == MM_STATUS_AUTHENTICATED && CL_MM_GetTicket( serversession ) )
		if( CL_MM_Connect( &cls.serveraddress ) )
			newstate = CA_GETTING_TICKET;
	}
	CL_SetClientState( newstate );

	cls.connect_time = -99999; // CL_CheckForResend() will fire immediately
	cls.connect_count = 0;
	cls.rejected = qfalse;
	cls.lastPacketReceivedTime = cls.realtime; // reset the timeout limit
	cls.mv = qfalse;
}

/*
* CL_Connect_Cmd_f
*/
static void CL_Connect_Cmd_f( int socket )
{
	netadr_t serveraddress;
	char *servername, password[64], autowatch[64] = { 0 };
	const char *extension;
	char *connectstring, *connectstring_base;
	const char *tmp, *scheme = APP_URI_SCHEME, *proto_scheme = APP_URI_PROTO_SCHEME;

	if( Cmd_Argc() != 2 )
	{
		Com_Printf( "Usage: %s <server>\n", Cmd_Argv(0) );
		return;
	}

	connectstring_base = TempCopyString( Cmd_Argv( 1 ) );
	connectstring = connectstring_base;

	if( !Q_strnicmp( connectstring, proto_scheme, strlen( proto_scheme ) ) )
		connectstring += strlen( proto_scheme );
	else if( !Q_strnicmp( connectstring, scheme, strlen( scheme ) ) )
		connectstring += strlen( scheme );

	extension = COM_FileExtension( connectstring );
	if( extension && !Q_stricmp( extension, APP_DEMO_EXTENSION_STR ) )
	{
		char *temp;
		size_t temp_size;
		const char *http_scheme = "http://";

		if( !Q_strnicmp( connectstring, http_scheme, strlen( http_scheme ) ) )
			connectstring += strlen( http_scheme );

		temp_size = strlen( "demo " ) + strlen( http_scheme ) + strlen( connectstring ) + 1;
		temp = Mem_TempMalloc( temp_size );
		Q_snprintfz( temp, temp_size, "demo %s%s", http_scheme, connectstring );

		Cbuf_ExecuteText( EXEC_NOW, temp );

		Mem_TempFree( temp );
		Mem_TempFree( connectstring_base );
		return;
	}

	if ( ( tmp = Q_strrstr(connectstring, "@") ) != NULL ) {
		Q_strncpyz( password, connectstring, min(sizeof(password),( tmp - connectstring + 1)) );
		Cvar_Set( "password", password );
		connectstring = connectstring + (tmp - connectstring) + 1;
	}

	if ( ( tmp = Q_strrstr(connectstring, "#") ) != NULL ) {
		Q_strncpyz( autowatch, COM_RemoveColorTokens( tmp + 1 ), sizeof( autowatch ) );
		connectstring[tmp - connectstring] = '\0';
	}

	if ( ( tmp = Q_strrstr(connectstring, "/") ) != NULL ) {
		connectstring[tmp - connectstring] = '\0';
	}

	Cvar_ForceSet( "autowatch", autowatch );

	if( !NET_StringToAddress( connectstring, &serveraddress ) )
	{
		Mem_TempFree( connectstring_base );
		Com_Printf( "Bad server address\n" );
		return;
	}

	// wait until MM allows us to connect to a server
	// (not in a middle of login process or anything)
	CL_MM_WaitForLogin();

	servername = TempCopyString( connectstring );
	CL_Connect( servername, ( serveraddress.type == NA_LOOPBACK ? SOCKET_LOOPBACK : socket ), &serveraddress );

	Mem_TempFree( servername );
	Mem_TempFree( connectstring_base );
}

/*
* CL_Connect_f
*/
static void CL_Connect_f( void )
{
	CL_Connect_Cmd_f( SOCKET_UDP );
}

/*
* CL_TCPConnect_f
*/
#if defined(TCP_SUPPORT) && defined(TCP_ALLOW_CONNECT)
static void CL_TCPConnect_f( void )
{
	CL_Connect_Cmd_f( SOCKET_TCP );
}
#endif


/*
* CL_Rcon_f
* 
* Send the rest of the command line over as
* an unconnected command.
*/
static void CL_Rcon_f( void )
{
	char message[1024];
	int i;
	const socket_t *socket;
	const netadr_t *address;

	if( cls.demo.playing )
		return;

	if( rcon_client_password->string[0] == '\0' )
	{
		Com_Printf( "You must set 'rcon_password' before issuing an rcon command.\n" );
		return;
	}

	// wsw : jal : check for msg len abuse (thx to r1Q2)
	if( strlen( Cmd_Args() ) + strlen( rcon_client_password->string ) + 16 >= sizeof( message ) )
	{
		Com_Printf( "Length of password + command exceeds maximum allowed length.\n" );
		return;
	}

	message[0] = (qbyte)255;
	message[1] = (qbyte)255;
	message[2] = (qbyte)255;
	message[3] = (qbyte)255;
	message[4] = 0;

	Q_strncatz( message, "rcon ", sizeof( message ) );

	Q_strncatz( message, rcon_client_password->string, sizeof( message ) );
	Q_strncatz( message, " ", sizeof( message ) );

	for( i = 1; i < Cmd_Argc(); i++ )
	{
		Q_strncatz( message, "\"", sizeof( message ) );
		Q_strncatz( message, Cmd_Argv( i ), sizeof( message ) );
		Q_strncatz( message, "\" ", sizeof( message ) );
	}

	if( cls.state >= CA_CONNECTED )
	{
		socket = cls.netchan.socket;
		address = &cls.netchan.remoteAddress;
	}
	else
	{
		if( !strlen( rcon_address->string ) )
		{
			Com_Printf( "You must be connected, or set the 'rcon_address' cvar to issue rcon commands\n" );
			return;
		}

		if( rcon_address->modified )
		{
			if( !NET_StringToAddress( rcon_address->string, &cls.rconaddress ) )
			{
				Com_Printf( "Bad rcon_address.\n" );
				return; // we don't clear modified, so it will whine the next time too
			}
			if( NET_GetAddressPort( &cls.rconaddress ) == 0 )
				NET_SetAddressPort( &cls.rconaddress, PORT_SERVER );

			rcon_address->modified = qfalse;
		}

		socket = ( cls.rconaddress.type == NA_IP6 ? &cls.socket_udp6 : &cls.socket_udp );
		address = &cls.rconaddress;
	}

	NET_SendPacket( socket, message, (int)strlen( message )+1, address );
}

/*
* CL_GetClipboardData
*/
char *CL_GetClipboardData( qboolean primary )
{
	return Sys_GetClipboardData( primary );
}

/*
* CL_SetClipboardData
*/
qboolean CL_SetClipboardData( char *data )
{
	return Sys_SetClipboardData( data );
}

/*
* CL_FreeClipboardData
*/
void CL_FreeClipboardData( char *data )
{
	Sys_FreeClipboardData( data );
}

/*
* CL_OpenURLInBrowser
*/
void CL_OpenURLInBrowser( const char *url )
{
	Sys_OpenURLInBrowser( url );
}

/*
* CL_GetKeyDest
*/
int CL_GetKeyDest( void )
{
	return cls.key_dest;
}

/*
* CL_SetKeyDest
*/
void CL_SetKeyDest( int key_dest )
{
	if( key_dest < key_game || key_dest > key_delegate )
		Com_Error( ERR_DROP, "CL_SetKeyDest: invalid key_dest" );

	if( cls.key_dest != key_dest )
	{
		Key_ClearStates();
		cls.key_dest = key_dest;
		Con_SetMessageModeCvar();
	}
}


/*
* CL_SetOldKeyDest
*/
void CL_SetOldKeyDest( int key_dest )
{
	if( key_dest < key_game || key_dest > key_delegate )
		Com_Error( ERR_DROP, "CL_SetKeyDest: invalid key_dest" );
	cls.old_key_dest = key_dest;
}

/*
* CL_ResetServerCount
*/
void CL_ResetServerCount( void )
{
	cl.servercount = -1;
}

/*
* CL_ClearState
*/
void CL_ClearState( void )
{
	if( cl.cms )
	{
		CM_Free( cl.cms );
		cl.cms = NULL;
	}

	if( cl.frames_areabits )
	{
		Mem_Free( cl.frames_areabits );
		cl.frames_areabits = NULL;
	}

	if( cl.cmds )
	{
	    Mem_Free( cl.cmds );
	    cl.cmds = NULL;
	}

	if( cl.cmd_time )
	{
	    Mem_Free( cl.cmd_time );
	    cl.cmd_time = NULL;
	}

	if( cl.snapShots )
	{
	    Mem_Free( cl.snapShots );
	    cl.snapShots = NULL;
	}

	// wipe the entire cl structure
	memset( &cl, 0, sizeof( client_state_t ) );
	memset( cl_baselines, 0, sizeof( cl_baselines ) );

	cl.cmds = Mem_ZoneMalloc( sizeof( *cl.cmds ) * CMD_BACKUP );
	cl.cmd_time = Mem_ZoneMalloc( sizeof( *cl.cmd_time ) * CMD_BACKUP );
	cl.snapShots = Mem_ZoneMalloc( sizeof( *cl.snapShots ) * CMD_BACKUP );

	//userinfo_modified = qtrue;
	cls.lastExecutedServerCommand = 0;
	cls.reliableAcknowledge = 0;
	cls.reliableSequence = 0;
	cls.reliableSent = 0;
	memset( cls.reliableCommands, 0, sizeof( cls.reliableCommands ) );
	// reset ucmds buffer
	cls.ucmdHead = 0;
	cls.ucmdSent = 0;
	cls.ucmdAcknowledged = 0;

	//restart realtime and lastPacket times
	cls.realtime = 0;
	cls.gametime = 0;
	cls.lastPacketSentTime = 0;
	cls.lastPacketReceivedTime = 0;

	cls.sv_pure = qfalse;
}


/*
* CL_SetNext_f
* 
* Next is used to set an action which is executed at disconnecting.
*/
char cl_NextString[MAX_STRING_CHARS];
static void CL_SetNext_f( void )
{
	if( Cmd_Argc() < 2 )
	{
		Com_Printf( "USAGE: next <commands>\n" );
		return;
	}

	// jalfixme: I'm afraid of this being too powerful, since it basically
	// is allowed to execute everything. Shall we check for something?
	Q_strncpyz( cl_NextString, Cmd_Args(), sizeof( cl_NextString ) );
	Com_Printf( "NEXT: %s\n", cl_NextString );
}


/*
* CL_ExecuteNext
*/
static void CL_ExecuteNext( void )
{
	if( !strlen( cl_NextString ) )
		return;

	Cbuf_ExecuteText( EXEC_APPEND, cl_NextString );
	memset( cl_NextString, 0, sizeof( cl_NextString ) );
}

/*
* CL_Disconnect_SendCommand
* 
* Sends a disconnect message to the server
*/
static void CL_Disconnect_SendCommand( void )
{
	// wsw : jal : send the packet 3 times to make sure isn't lost
	CL_AddReliableCommand( "disconnect" );
	CL_SendMessagesToServer( qtrue );
	CL_AddReliableCommand( "disconnect" );
	CL_SendMessagesToServer( qtrue );
	CL_AddReliableCommand( "disconnect" );
	CL_SendMessagesToServer( qtrue );
}

/*
* CL_Disconnect
* 
* Goes from a connected state to full screen console state
* Sends a disconnect message to the server
* This is also called on Com_Error, so it shouldn't cause any errors
*/
void CL_Disconnect( const char *message )
{
	char menuparms[MAX_STRING_CHARS];
	qboolean wasconnecting;

	// We have to shut down webdownloading first
	if( cls.download.web )
	{
		cls.download.disconnect = qtrue;
		return;
	}

	if( cls.state == CA_UNINITIALIZED )
		return;
	if( cls.state == CA_DISCONNECTED )
		goto done;

	if( cls.state < CA_LOADING )
		wasconnecting = qtrue;
	else
		wasconnecting = qfalse;

	SV_ShutdownGame( "Owner left the listen server", qfalse );

	if( cl_timedemo && cl_timedemo->integer )
	{
		unsigned int time;
		int i;

		Com_Printf( "\n" );
		for( i = 1; i < 100; i++ )
		{
			if( cl.timedemo.counts[i] > 0 )
			{
				Com_Printf( "%2ims - %7.2ffps: %6.2f%c\n", i, 1000.0/i,
					( cl.timedemo.counts[i]*1.0/cl.timedemo.frames )*100.0, '%' );
			}
		}

		Com_Printf( "\n" );
		time = Sys_Milliseconds() - cl.timedemo.start;
		if( time > 0 )
			Com_Printf( "%i frames, %3.1f seconds: %3.1f fps\n", cl.timedemo.frames,
			time/1000.0, cl.timedemo.frames*1000.0 / time );
	}

	cls.connect_time = 0;
	cls.connect_count = 0;
	cls.rejected = 0;

	if( cls.demo.recording )
		CL_Stop_f();

	if( cls.state == CA_CINEMATIC )
		SCR_StopCinematic();
	else if( cls.demo.playing )
		CL_DemoCompleted();
	else
		CL_Disconnect_SendCommand(); // send a disconnect message to the server

	FS_RemovePurePaks();

	Com_FreePureList( &cls.purelist );

	cls.sv_pure = qfalse;

	// udp is kept open all the time, for connectionless messages
	if( cls.socket && cls.socket->type != SOCKET_UDP )
		NET_CloseSocket( cls.socket );

	cls.socket = NULL;
	cls.reliable = qfalse;
	cls.mv = qfalse;

	CL_RestartMedia( qfalse );

	CL_Mumble_Unlink();

	CL_ClearState();
	CL_SetClientState( CA_DISCONNECTED );

	if( cls.download.requestname )
	{
		if( cls.download.name )
		{
			assert( !cls.download.web );

			cls.download.cancelled = qtrue;
			CL_StopServerDownload();
		}
		CL_DownloadDone();
	}

	if( message != NULL )
	{
		Q_snprintfz( menuparms, sizeof( menuparms ), "menu_open connfailed dropreason %i servername \"%s\" droptype %i rejectmessage \"%s\"",
			( wasconnecting ? DROP_REASON_CONNFAILED : DROP_REASON_CONNERROR ), cls.servername, DROP_TYPE_GENERAL, message );

		Cbuf_ExecuteText( EXEC_NOW, menuparms );
	}

done:
	SCR_EndLoadingPlaque(); // get rid of loading plaque

	// in case we disconnect while in download phase
	CL_FreeDownloadList();

	CL_ExecuteNext(); // start next action if any is defined
}

void CL_Disconnect_f( void )
{
	// We have to shut down webdownloading first
	if( cls.download.web )
	{
		cls.download.disconnect = qtrue;
		return;
	}

	CL_Disconnect( NULL );
}

/*
* CL_Changing_f
* 
* Just sent as a hint to the client that they should
* drop to full console
*/
void CL_Changing_f( void )
{
	//ZOID
	//if we are downloading, we don't change!  This so we don't suddenly stop downloading a map
	if( cls.download.filenum || cls.download.web )
		return;

	if( cls.demo.recording )
		CL_Stop_f();

	Com_Printf( "CL:Changing\n" );

	memset( cl.configstrings, 0, sizeof( cl.configstrings ) );
	CL_SetClientState( CA_CONNECTED ); // not active anymore, but not disconnected
}

/*
* CL_ServerReconnect_f
* 
* The server is changing levels
*/
void CL_ServerReconnect_f( void )
{
	if( cls.demo.playing )
		return;

	//if we are downloading, we don't change!  This so we don't suddenly stop downloading a map
	if( cls.download.filenum || cls.download.web )
	{
		cls.download.pending_reconnect = qtrue;
		return;
	}

	if( cls.state < CA_CONNECTED )
	{
		Com_Printf( "Error: CL_ServerReconnect_f while not connected\n" );
		return;
	}

	if( cls.demo.recording )
		CL_Stop_f();

	cls.connect_count = 0;
	cls.rejected = 0;

	CL_SoundModule_StopAllSounds();

	Com_Printf( "Reconnecting...\n" );

#ifdef TCP_SUPPORT
	cls.connect_time = Sys_Milliseconds();
#else
	cls.connect_time = Sys_Milliseconds() - 1500;
#endif

	memset( cl.configstrings, 0, sizeof( cl.configstrings ) );
	CL_SetClientState( CA_HANDSHAKE );
	CL_AddReliableCommand( "new" );
}

/*
* CL_Reconnect_f
* 
* User reconnect command.
*/
void CL_Reconnect_f( void )
{
	char *servername;
	socket_type_t servertype;
	netadr_t serveraddress;

	if( !cls.servername )
	{
		Com_Printf( "Can't reconnect, never connected\n" );
		return;
	}

	servername = TempCopyString( cls.servername );
	servertype = cls.servertype;
	serveraddress = cls.serveraddress;
	CL_Disconnect( NULL );
	CL_Connect( servername, servertype, &serveraddress );
	Mem_TempFree( servername );
}

/*
* CL_ConnectionlessPacket
* 
* Responses to broadcasts, etc
*/
static void CL_ConnectionlessPacket( const socket_t *socket, const netadr_t *address, msg_t *msg )
{
	char *s;
	char *c;

	MSG_BeginReading( msg );
	MSG_ReadLong( msg ); // skip the -1

	s = MSG_ReadStringLine( msg );

	if( !strncmp( s, "getserversResponse\\", 19 ) )
	{
		Com_DPrintf( "%s: %s\n", NET_AddressToString( address ), "getserversResponse" );
		CL_ParseGetServersResponse( socket, address, msg, qfalse );
		return;
	}

	if( !strncmp( s, "getserversExtResponse", 21 ) )
	{
		Com_DPrintf( "%s: %s\n", NET_AddressToString( address ), "getserversExtResponse" );
		CL_ParseGetServersResponse( socket, address, msg, qtrue );
		return;
	}

	Cmd_TokenizeString( s );
	c = Cmd_Argv( 0 );

	Com_DPrintf( "%s: %s\n", NET_AddressToString( address ), s );

	// server responding to a status broadcast
	if( !strcmp( c, "info" ) )
	{
		CL_ParseStatusMessage( socket, address, msg );
		return;
	}

	// jal : wsw
	// server responding to a detailed info broadcast
	if( !strcmp( c, "infoResponse" ) )
	{
		CL_ParseGetInfoResponse( socket, address, msg );
		return;
	}
	if( !strcmp( c, "statusResponse" ) )
	{
		CL_ParseGetStatusResponse( socket, address, msg );
		return;
	}

	if( cls.demo.playing )
	{
		Com_DPrintf( "Received connectionless cmd \"%s\" from %s while playing a demo\n", s, NET_AddressToString( address ) );
		return;
	}

	// server connection
	if( !strcmp( c, "client_connect" ) )
	{
		if( cls.state == CA_CONNECTED )
		{
			Com_Printf( "Dup connect received.  Ignored.\n" );
			return;
		}
		// these two are from Q3
		if( cls.state != CA_CONNECTING )
		{
			Com_Printf( "client_connect packet while not connecting.  Ignored.\n" );
			return;
		}
		if( !NET_CompareAddress( address, &cls.serveraddress ) )
		{
			Com_Printf( "client_connect from a different address.  Ignored.\n" );
			Com_Printf( "Was %s should have been %s\n", NET_AddressToString( address ),
				NET_AddressToString( &cls.serveraddress ) );
			return;
		}

		cls.rejected = qfalse;

		Netchan_Setup( &cls.netchan, socket, address, Netchan_GamePort() );
		memset( cl.configstrings, 0, sizeof( cl.configstrings ) );
		CL_SetClientState( CA_HANDSHAKE );
		CL_AddReliableCommand( "new" );
		return;
	}

	// reject packet, used to inform the client that connection attemp didn't succeed
	if( !strcmp( c, "reject" ) )
	{
		int rejectflag;

		if( cls.state != CA_CONNECTING )
		{
			Com_Printf( "reject packet while not connecting, ignored\n" );
			return;
		}
		if( !NET_CompareAddress( address, &cls.serveraddress ) )
		{
			Com_Printf( "reject from a different address, ignored\n" );
			Com_Printf( "Was %s should have been %s\n", NET_AddressToString( address ),
				NET_AddressToString( &cls.serveraddress ) );
			return;
		}

		cls.rejected = qtrue;

		cls.rejecttype = atoi( MSG_ReadStringLine( msg ) );
		if( cls.rejecttype < 0 || cls.rejecttype >= DROP_TYPE_TOTAL ) cls.rejecttype = DROP_TYPE_GENERAL;

		rejectflag = atoi( MSG_ReadStringLine( msg ) );

		Q_strncpyz( cls.rejectmessage, MSG_ReadStringLine( msg ), sizeof( cls.rejectmessage ) );
		if( strlen( cls.rejectmessage ) > sizeof( cls.rejectmessage )-2 )
		{
			cls.rejectmessage[strlen( cls.rejectmessage )-2] = '.';
			cls.rejectmessage[strlen( cls.rejectmessage )-1] = '.';
			cls.rejectmessage[strlen( cls.rejectmessage )] = '.';
		}

		Com_Printf( "Connection refused: %s\n", cls.rejectmessage );
		if( rejectflag & DROP_FLAG_AUTORECONNECT )
		{
			Com_Printf( "Automatic reconnecting allowed.\n" );
		}
		else
		{
			char menuparms[MAX_STRING_CHARS];

			Com_Printf( "Automatic reconnecting not allowed.\n" );

			CL_Disconnect( NULL );
			Q_snprintfz( menuparms, sizeof( menuparms ), "menu_open connfailed dropreason %i servername \"%s\" droptype %i rejectmessage \"%s\"",
				DROP_REASON_CONNFAILED, cls.servername, cls.rejecttype, cls.rejectmessage );

			Cbuf_ExecuteText( EXEC_NOW, menuparms );
		}

		return;
	}

	// remote command from gui front end
	if( !strcmp( c, "cmd" ) )
	{
		if( !NET_IsLocalAddress( address ) )
		{
			Com_Printf( "Command packet from remote host, ignored\n" );
			return;
		}
		Sys_AppActivate();
		s = MSG_ReadString( msg );
		Cbuf_AddText( s );
		Cbuf_AddText( "\n" );
		return;
	}
	// print command from somewhere
	if( !strcmp( c, "print" ) )
	{
		// CA_CONNECTING is allowed, because old servers send protocol mismatch connection error message with it
		if( ( ( cls.state != CA_UNINITIALIZED && cls.state != CA_DISCONNECTED ) &&
			NET_CompareAddress( address, &cls.serveraddress ) ) ||
			( rcon_address->string[0] != '\0' && NET_CompareAddress( address, &cls.rconaddress ) ) )
		{
			s = MSG_ReadString( msg );
			Com_Printf( "%s", s );
			return;
		}
		else
		{
			Com_Printf( "Print packet from unknown host, ignored\n" );
			return;
		}
	}

	// ping from somewhere
	if( !strcmp( c, "ping" ) )
	{
		// send any args back with the acknowledgement
		Netchan_OutOfBandPrint( socket, address, "ack %s", Cmd_Args() );
		return;
	}

	// ack from somewhere
	if( !strcmp( c, "ack" ) )
	{
		return;
	}

	// challenge from the server we are connecting to
	if( !strcmp( c, "challenge" ) )
	{
		// these two are from Q3
		if( cls.state != CA_CONNECTING )
		{
			Com_Printf( "challenge packet while not connecting, ignored\n" );
			return;
		}
		if( !NET_CompareAddress( address, &cls.serveraddress ) )
		{
			Com_Printf( "challenge from a different address, ignored\n" );
			Com_Printf( "Was %s", NET_AddressToString( address ) );
			Com_Printf( " should have been %s\n", NET_AddressToString( &cls.serveraddress ) );
			return;
		}

		cls.challenge = atoi( Cmd_Argv( 1 ) );
		//wsw : r1q2[start]
		//r1: reset the timer so we don't send dup. getchallenges
		cls.connect_time = Sys_Milliseconds();
		//wsw : r1q2[end]
		CL_SendConnectPacket();
		return;
	}

	// echo request from server
	if( !strcmp( c, "echo" ) )
	{
		Netchan_OutOfBandPrint( socket, address, "%s", Cmd_Argv( 1 ) );
		return;
	}

	Com_Printf( "Unknown connectionless packet from %s\n%s\n", NET_AddressToString( address ), c );
}

/*
* CL_ProcessPacket
*/
static qboolean CL_ProcessPacket( netchan_t *netchan, msg_t *msg )
{
	int zerror;

	if( !Netchan_Process( netchan, msg ) )
		return qfalse; // wasn't accepted for some reason

	// now if compressed, expand it
	MSG_BeginReading( msg );
	MSG_ReadLong( msg ); // sequence
	MSG_ReadLong( msg ); // sequence_ack
	if( msg->compressed )
	{
		zerror = Netchan_DecompressMessage( msg );
		if( zerror < 0 )
		{
			// compression error. Drop the packet
			Com_Printf( "CL_ProcessPacket: Compression error %i. Dropping packet\n", zerror );
			return qfalse;
		}
	}

	return qtrue;
}

/*
* CL_ReadPackets
*/
void CL_ReadPackets( void )
{
	static msg_t msg;
	static qbyte msgData[MAX_MSGLEN];
	int socketind, ret;
	socket_t *socket;
	netadr_t address;

	socket_t* sockets [] =
	{
		&cls.socket_loopback,
		&cls.socket_udp,
		&cls.socket_udp6,
#ifdef TCP_SUPPORT
		&cls.socket_tcp
#endif
	};

	MSG_Init( &msg, msgData, sizeof( msgData ) );

	for( socketind = 0; socketind < sizeof( sockets ) / sizeof( sockets[0] ); socketind++ )
	{
		socket = sockets[socketind];

#ifdef TCP_SUPPORT
		if( socket->type == SOCKET_TCP && !socket->connected )
			continue;
#endif

		while( socket->open && ( ret = NET_GetPacket( socket, &address, &msg ) ) != 0 )
		{
			if( ret == -1 )
			{
				Com_Printf( "Error receiving packet with %s: %s\n", NET_SocketToString( socket ), NET_ErrorString() );
				if( cls.reliable && cls.socket == socket )
					CL_Disconnect( va( "Error receiving packet: %s\n", NET_ErrorString() ) );

				continue;
			}

			// remote command packet
			if( *(int *)msg.data == -1 )
			{
				CL_ConnectionlessPacket( socket, &address, &msg );
				continue;
			}

			if( cls.demo.playing ) {
				// only allow connectionless packets during demo playback
				continue;
			}

			if( cls.state == CA_DISCONNECTED || cls.state == CA_GETTING_TICKET || cls.state == CA_CONNECTING || cls.state == CA_CINEMATIC )
			{
				Com_DPrintf( "%s: Not connected\n", NET_AddressToString( &address ) );
				continue; // dump it if not connected
			}

			if( msg.cursize < 8 )
			{
				//wsw : r1q2[start]
				//r1: delegated to DPrintf (someone could spam your console with crap otherwise)
				Com_DPrintf( "%s: Runt packet\n", NET_AddressToString( &address ) );
				//wsw : r1q2[end]
				continue;
			}

			//
			// packet from server
			//
			if( !NET_CompareAddress( &address, &cls.netchan.remoteAddress ) )
			{
				Com_DPrintf( "%s: Sequenced packet without connection\n", NET_AddressToString( &address ) );
				continue;
			}
			if( !CL_ProcessPacket( &cls.netchan, &msg ) )
				continue; // wasn't accepted for some reason, like only one fragment of bigger message

			CL_ParseServerMessage( &msg );
			cls.lastPacketReceivedTime = cls.realtime;

#ifdef TCP_SUPPORT
			// we might have just been disconnected
			if( socket->type == SOCKET_TCP && !socket->connected )
				break;
#endif
		}
	}

	if( cls.demo.playing ) {
		return;
	}

	// not expected, but could happen if cls.realtime is cleared and lastPacketReceivedTime is not
	if( cls.lastPacketReceivedTime > cls.realtime )
		cls.lastPacketReceivedTime = cls.realtime;

	// check timeout
	if( cls.state >= CA_HANDSHAKE && cls.state != CA_CINEMATIC && cls.lastPacketReceivedTime )
	{
		if( cls.lastPacketReceivedTime + cl_timeout->value*1000 < cls.realtime )
		{
			if( ++cl.timeoutcount > 5 ) // timeoutcount saves debugger
			{
				Com_Printf( "\nServer connection timed out.\n" );
				CL_Disconnect( "Connection timed out" );
				return;
			}
		}
	}
	else
		cl.timeoutcount = 0;
}

//=============================================================================

/*
* CL_Userinfo_f
*/
static void CL_Userinfo_f( void )
{
	Com_Printf( "User info settings:\n" );
	Info_Print( Cvar_Userinfo() );
}

static int precache_check; // for autodownload of precache items
static int precache_spawncount;
static int precache_tex;
static int precache_pure;

#define PLAYER_MULT 5

// ENV_CNT is map load
#define ENV_CNT ( CS_PLAYERINFOS + MAX_CLIENTS * PLAYER_MULT )
#define TEXTURE_CNT ( ENV_CNT+1 )

static unsigned int CL_LoadMap( const char *name )
{
	int i;
	int areas;

	unsigned int map_checksum;

	assert( !cl.cms );
	cl.cms = CM_New( NULL );
	CM_LoadMap( cl.cms, name, qtrue, &map_checksum );

	assert( cl.cms );

	// allocate memory for areabits
	areas = CM_NumAreas( cl.cms );
	areas *= CM_AreaRowSize( cl.cms );

	cl.frames_areabits = Mem_ZoneMalloc( UPDATE_BACKUP * areas );
	for( i = 0; i < UPDATE_BACKUP; i++ )
	{
		cl.snapShots[i].areabytes = areas;
		cl.snapShots[i].areabits = cl.frames_areabits + i * areas;
	}

	// check memory integrity
	Mem_CheckSentinelsGlobal();

	return map_checksum;
}

void CL_RequestNextDownload( void )
{
	char tempname[MAX_CONFIGSTRING_CHARS + 4];
	purelist_t *purefile;
	int i;

	if( cls.state != CA_CONNECTED )
		return;

	// pure list
	if( cls.sv_pure )
	{
		// skip
		if( !cl_downloads->integer )
			precache_pure = -1;

		// try downloading
		if( precache_pure != -1 )
		{
			i = 0;
			purefile = cls.purelist;
			while( i < precache_pure && purefile )
			{
				purefile = purefile->next;
				i++;
			}

			while( purefile )
			{
				precache_pure++;
				if( !CL_CheckOrDownloadFile( purefile->filename ) )
					return;
				purefile = purefile->next;
			}
			precache_pure = -1;
		}

		if( precache_pure == -1 )
		{
			qboolean failed = qfalse;
			char message[MAX_STRING_CHARS];

			Q_snprintfz( message, sizeof( message ), "Pure check failed:" );

			purefile = cls.purelist;
			while( purefile )
			{
				Com_DPrintf( "Adding pure file: %s\n", purefile->filename );
				if( !FS_AddPurePak( purefile->checksum ) )
				{
					failed = qtrue;
					Q_strncatz( message, " ", sizeof( message ) );
					Q_strncatz( message, purefile->filename, sizeof( message ) );
				}
				purefile = purefile->next;
			}

			if( failed )
			{
				Com_Error( ERR_DROP, message );
				return;
			}
		}
	}

	// skip if download not allowed
	if( !cl_downloads->integer && precache_check < ENV_CNT )
		precache_check = ENV_CNT;

	//ZOID
	if( precache_check == CS_WORLDMODEL ) // confirm map
	{
		precache_check = CS_MODELS; // 0 isn't used

		if( !CL_CheckOrDownloadFile( cl.configstrings[CS_WORLDMODEL] ) )
			return; // started a download
	}

	if( precache_check >= CS_MODELS && precache_check < CS_MODELS+MAX_MODELS )
	{
		while( precache_check < CS_MODELS+MAX_MODELS && cl.configstrings[precache_check][0] )
		{
			if( cl.configstrings[precache_check][0] == '*' ||
				cl.configstrings[precache_check][0] == '$' || // disable playermodel downloading for now
				cl.configstrings[precache_check][0] == '#' )
			{
				precache_check++;
				continue;
			}

			if( !CL_CheckOrDownloadFile( cl.configstrings[precache_check++] ) )
				return; // started a download
		}
		precache_check = CS_SOUNDS;
	}

	if( precache_check >= CS_SOUNDS && precache_check < CS_SOUNDS+MAX_SOUNDS )
	{
		if( precache_check == CS_SOUNDS )
			precache_check++; // zero is blank

		while( precache_check < CS_SOUNDS+MAX_SOUNDS && cl.configstrings[precache_check][0] )
		{
			if( cl.configstrings[precache_check][0] == '*' ) // sexed sounds
			{
				precache_check++;
				continue;
			}
			Q_strncpyz( tempname, cl.configstrings[precache_check++], sizeof( tempname ) );
			if( !COM_FileExtension( tempname ) )
			{
				if( !FS_FirstExtension( tempname, SOUND_EXTENSIONS, NUM_SOUND_EXTENSIONS ) )
				{
					COM_DefaultExtension( tempname, ".wav", sizeof( tempname ) );
					if( !CL_CheckOrDownloadFile( tempname ) )
						return; // started a download
				}
			}
			else
			{
				if( !CL_CheckOrDownloadFile( tempname ) )
					return; // started a download
			}
		}
		precache_check = CS_IMAGES;
	}
	if( precache_check >= CS_IMAGES && precache_check < CS_IMAGES+MAX_IMAGES )
	{
		if( precache_check == CS_IMAGES )
			precache_check++; // zero is blank

		// precache phase completed
		precache_check = ENV_CNT;
	}

	if( precache_check == ENV_CNT )
	{
		qboolean restart = qfalse;
		qboolean vid_restart = qfalse;
		const char *restart_msg = "";
		unsigned map_checksum;

		// we're done with the download phase, so clear the list
		CL_FreeDownloadList();
		if( cls.sv_pure )
		{
			restart = qtrue;
			restart_msg = "Pure server. Restarting media...";
		}
		if( cls.download.successCount )
		{
			restart = qtrue;
			vid_restart = qtrue;
			restart_msg = "Files downloaded. Restarting media...";
		}

		R_BeginRegistration();
		CL_SoundModule_BeginRegistration();

		if( restart ) {
			Com_Printf( "%s\n", restart_msg );

			if( vid_restart ) {
				// no media is going to survive a vid_restart...
				Cbuf_ExecuteText( EXEC_NOW, "s_restart 1\n" );
			}
			else {
				// the following registration calls will ensure 
				// no media assets survives the restart
				R_EndRegistration();
				CL_SoundModule_EndRegistration();

				R_BeginRegistration();
				CL_SoundModule_BeginRegistration();
			}
		}

		if( !vid_restart ) {
			CL_RestartMedia( qfalse );
		}

		cls.download.successCount = 0;

		map_checksum = CL_LoadMap( cl.configstrings[CS_WORLDMODEL] );
		if( map_checksum != (unsigned)atoi( cl.configstrings[CS_MAPCHECKSUM] ) )
		{
			Com_Error( ERR_DROP, "Local map version differs from server: %u != '%u'",
				map_checksum, (unsigned)atoi(cl.configstrings[CS_MAPCHECKSUM]) );
			return;
		}

		precache_check = TEXTURE_CNT;
	}

	if( precache_check == TEXTURE_CNT )
	{
		precache_check = TEXTURE_CNT+1;
		precache_tex = 0;
	}

	// confirm existance of textures, download any that don't exist
	if( precache_check == TEXTURE_CNT+1 )
		precache_check = TEXTURE_CNT+999;

	// load client game module
	CL_GameModule_Init();
	CL_AddReliableCommand( va( "begin %i\n", precache_spawncount ) );

	CL_Mumble_Link();
}

/*
* CL_Precache_f
* 
* The server will send this command right
* before allowing the client into the server
*/
void CL_Precache_f( void )
{
	FS_RemovePurePaks();

	if( cls.demo.playing )
	{
		if( !cls.demo.play_jump )
		{
			CL_LoadMap( cl.configstrings[CS_WORLDMODEL] );

			CL_GameModule_Init();
		}
		else
		{
			CL_GameModule_Reset();
		}

		cls.demo.play_ignore_next_frametime = qtrue;

		return;
	}

	precache_pure = 0;
	precache_check = CS_WORLDMODEL;
	precache_spawncount = atoi( Cmd_Argv( 1 ) );

	CL_RequestNextDownload();
}

/*
* CL_WriteConfiguration
* 
* Writes key bindings, archived cvars and aliases to a config file
*/
static void CL_WriteConfiguration( const char *name, qboolean warn )
{
	int file;

	if( FS_FOpenFile( name, &file, FS_WRITE ) == -1 )
	{
		Com_Printf( "Couldn't write %s.\n", name );
		return;
	}

	if( warn ) {
		// Write 'Warsow' with UTF-8 encoded section sign in place of the s to aid
		// text editors in recognizing the file's encoding
		FS_Printf( file, "// This file is automatically generated by " APPLICATION_UTF8 ", do not modify.\r\n" );
	}

	FS_Printf( file, "\r\n// key bindings\r\n" );
	Key_WriteBindings( file );

	FS_Printf( file, "\r\n// variables\r\n" );
	Cvar_WriteVariables( file );

	FS_Printf( file, "\r\n// aliases\r\n" );
	Cmd_WriteAliases( file );

	FS_FCloseFile( file );
}


/*
* CL_WriteConfig_f
*/
static void CL_WriteConfig_f( void )
{
	char *name;
	int name_size;

	if( Cmd_Argc() != 2 )
	{
		Com_Printf( "Usage: writeconfig <filename>\n" );
		return;
	}

	name_size = sizeof( char ) * ( strlen( Cmd_Argv( 1 ) ) + strlen( ".cfg" ) + 1 );
	name = Mem_TempMalloc( name_size );
	Q_strncpyz( name, Cmd_Argv( 1 ), name_size );
	COM_SanitizeFilePath( name );

	if( !COM_ValidateRelativeFilename( name ) )
	{
		Com_Printf( "Invalid filename" );
		Mem_TempFree( name );
		return;
	}

	COM_DefaultExtension( name, ".cfg", name_size );

	Com_Printf( "Writing: %s\n", name );
	CL_WriteConfiguration( name, qfalse );

	Mem_TempFree( name );
}

/*
* CL_SetClientState
*/
void CL_SetClientState( int state )
{
	cls.state = state;
	Com_SetClientState( state );

	switch( state )
	{
	case CA_DISCONNECTED:
		Con_Close();
		Cbuf_ExecuteText( EXEC_NOW, "menu_force 1" );
		//CL_UIModule_MenuMain ();
		CL_SetKeyDest( key_menu );
		//SCR_UpdateScreen();
		break;
	case CA_GETTING_TICKET:
	case CA_CONNECTING:
		Con_Close();
		CL_UIModule_ForceMenuOff();
		CL_SetKeyDest( key_game );
		//SCR_UpdateScreen();
		break;
	case CA_CONNECTED:
		Con_Close();
		Cvar_FixCheatVars();
		//SCR_UpdateScreen();
		break;
	case CA_ACTIVE:
	case CA_CINEMATIC:
		R_EndRegistration();
		CL_SoundModule_EndRegistration();
		Con_Close();
		CL_UIModule_ForceMenuOff();
		CL_SetKeyDest( key_game );
		//SCR_UpdateScreen();
		if( !cls.sv_tv )
			CL_AddReliableCommand( "svmotd 1" );
		CL_SoundModule_Clear();
		break;
	default:
		break;
	}
}

/*
* CL_GetClientState
*/
connstate_t CL_GetClientState( void )
{
	return cls.state;
}

/*
* CL_InitMedia
*/
void CL_InitMedia( qboolean verbose )
{
	if( cls.mediaInitialized )
		return;
	if( cls.state == CA_UNINITIALIZED )
		return;

	// random seed to be shared among game modules so pseudo-random stuff is in sync
	if ( cls.state != CA_CONNECTED )
	{
		srand( time( NULL ) );
		cls.mediaRandomSeed = rand();
	}

	cls.mediaInitialized = qtrue;

	// register console font and background
	SCR_RegisterConsoleMedia();

	// load user interface
	CL_UIModule_Init();

	// check memory integrity
	Mem_CheckSentinelsGlobal();

	CL_SoundModule_StopAllSounds();
}

/*
* CL_ShutdownMedia
*/
void CL_ShutdownMedia( qboolean verbose )
{
	if( !cls.mediaInitialized )
		return;

	cls.mediaInitialized = qfalse;

	// shutdown cgame
	CL_GameModule_Shutdown();

	// shutdown user interface
	CL_UIModule_Shutdown();

	// wsw : jalfonts
	SCR_ShutDownConsoleMedia();

	CL_SoundModule_StopAllSounds();
}

/*
* CL_RestartMedia
*/
void CL_RestartMedia( qboolean verbose )
{
	CL_ShutdownMedia( verbose );
	CL_InitMedia( verbose );
}

/*
* CL_S_Restart
* 
* Restart the sound subsystem so it can pick up new parameters and flush all sounds
*/
void CL_S_Restart( qboolean noVideo )
{
	qboolean verbose = (Cmd_Argc() >= 2 ? qtrue : qfalse);

	// The cgame and game must also be forced to restart because handles will become invalid
	// VID_Restart also forces an audio restart
	if( !noVideo )
	{
		VID_Restart( verbose, qtrue );
		VID_CheckChanges();
	}
	else
	{
		CL_SoundModule_Shutdown( verbose );
		CL_SoundModule_Init( verbose );
	}
}

/*
* CL_S_Restart_f
* 
* Restart the sound subsystem so it can pick up new parameters and flush all sounds
*/
static void CL_S_Restart_f( void )
{
	CL_S_Restart( qfalse );
}

/*
* CL_ShowIP_f - wsw : jal : taken from Q3 (it only shows the ip when server was started)
*/
static void CL_ShowIP_f( void )
{
	NET_ShowIP();
}

/*
* CL_ShowServerIP_f - wsw : pb : show the ip:port of the server the client is connected to
*/
static void CL_ShowServerIP_f( void )
{
	if( cls.state != CA_CONNECTED && cls.state != CA_ACTIVE )
	{
		Com_Printf( "Not connected to a server\n" );
		return;
	}

	Com_Printf( "Connected to server:\n" );
	Com_Printf( "Name: %s\n", cls.servername );
	Com_Printf( "Address: %s\n", NET_AddressToString( &cls.serveraddress ) );
}

/*
* CL_InitLocal
*/
static void CL_InitLocal( void )
{
	cvar_t *color;

	cls.state = CA_DISCONNECTED;
	Com_SetClientState( CA_DISCONNECTED );

	//
	// register our variables
	//
	cl_stereo_separation =	Cvar_Get( "cl_stereo_separation", "0.4", CVAR_ARCHIVE );
	cl_stereo =		Cvar_Get( "cl_stereo", "0", CVAR_ARCHIVE );

	cl_maxfps =		Cvar_Get( "cl_maxfps", "125", CVAR_ARCHIVE );
#ifdef PUTCPU2SLEEP
	cl_sleep =		Cvar_Get( "cl_sleep", "0", CVAR_ARCHIVE );
#endif
	cl_pps =		Cvar_Get( "cl_pps", "40", CVAR_ARCHIVE );
	cl_compresspackets =	Cvar_Get( "cl_compresspackets", "1", CVAR_ARCHIVE );

	cl_extrapolationTime =	Cvar_Get( "cl_extrapolationTime", "0", CVAR_DEVELOPER );
	cl_extrapolate = Cvar_Get( "cl_extrapolate", "1", CVAR_ARCHIVE );

	cl_yawspeed =		Cvar_Get( "cl_yawspeed", "140", 0 );
	cl_pitchspeed =		Cvar_Get( "cl_pitchspeed", "150", 0 );
	cl_anglespeedkey =	Cvar_Get( "cl_anglespeedkey", "1.5", 0 );

	cl_run =		Cvar_Get( "cl_run", "1", CVAR_ARCHIVE );
	sensitivity =		Cvar_Get( "sensitivity", "3", CVAR_ARCHIVE );
	m_accel =		Cvar_Get( "m_accel", "0", CVAR_ARCHIVE );
	m_accelStyle =	Cvar_Get( "m_accelStyle", "0", CVAR_ARCHIVE );
	m_accelOffset =	Cvar_Get( "m_accelOffset", "0", CVAR_ARCHIVE );
	m_accelPow =	Cvar_Get( "m_accelPow", "2", CVAR_ARCHIVE );
	m_filter =		Cvar_Get( "m_filter", "1", CVAR_ARCHIVE );
	m_filterStrength =	Cvar_Get( "m_filterStrength", "0.5", CVAR_ARCHIVE );
	m_pitch =		Cvar_Get( "m_pitch", "0.022", CVAR_ARCHIVE );
	m_yaw =			Cvar_Get( "m_yaw", "0.022", CVAR_ARCHIVE );
	m_sensCap =		Cvar_Get( "m_sensCap", "0", CVAR_ARCHIVE );

	cl_masterservers =	Cvar_Get( "masterservers", DEFAULT_MASTER_SERVERS_IPS, 0 );

	cl_shownet =		Cvar_Get( "cl_shownet", "0", 0 );
	cl_timeout =		Cvar_Get( "cl_timeout", "120", 0 );
	cl_timedemo =		Cvar_Get( "timedemo", "0", CVAR_CHEAT );
	cl_demoavi_video =	Cvar_Get( "cl_demoavi_video", "1", CVAR_ARCHIVE );
	cl_demoavi_audio =	Cvar_Get( "cl_demoavi_audio", "0", CVAR_ARCHIVE );
	cl_demoavi_fps =	Cvar_Get( "cl_demoavi_fps", "30.3", CVAR_ARCHIVE );
	cl_demoavi_fps->modified = qtrue;
	cl_demoavi_scissor =	Cvar_Get( "cl_demoavi_scissor", "0", CVAR_ARCHIVE );

	rcon_client_password =	Cvar_Get( "rcon_password", "", 0 );
	rcon_address =		Cvar_Get( "rcon_address", "", 0 );

	// wsw : debug netcode
	cl_debug_serverCmd =	Cvar_Get( "cl_debug_serverCmd", "0", CVAR_ARCHIVE|CVAR_CHEAT );
	cl_debug_timeDelta =	Cvar_Get( "cl_debug_timeDelta", "0", CVAR_ARCHIVE/*|CVAR_CHEAT*/ );

	cl_downloads =		Cvar_Get( "cl_downloads", "1", CVAR_ARCHIVE );
	cl_downloads_from_web =	Cvar_Get( "cl_downloads_from_web", "1", CVAR_ARCHIVE );
	cl_downloads_from_web_timeout = Cvar_Get( "cl_downloads_from_web_timeout", "600", CVAR_ARCHIVE );
	cl_download_allow_modules = Cvar_Get( "cl_download_allow_modules_" APP_VERSION_STR_MAJORMINOR, "1", CVAR_ARCHIVE );
	cl_checkForUpdate =	Cvar_Get( "cl_checkForUpdate", "1", CVAR_ARCHIVE );

	//
	// userinfo
	//
	info_password =		Cvar_Get( "password", "", CVAR_USERINFO );
	rate =			Cvar_Get( "rate", "60000", CVAR_DEVELOPER ); // FIXME

	Cvar_Get( "name", "player", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get( "clan", "", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get( "model", DEFAULT_PLAYERMODEL, CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get( "skin", DEFAULT_PLAYERSKIN, CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get( "hand", "0", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get( "handicap", "0", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get( "fov", "100", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get( "zoomfov", "30", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get( "zoomsens", "0", CVAR_ARCHIVE );

	Cvar_Get( "cl_download_name", "", CVAR_READONLY );
	Cvar_Get( "cl_download_percent", "0", CVAR_READONLY );

	color = Cvar_Get( "color", "", CVAR_ARCHIVE|CVAR_USERINFO );
	if( COM_ReadColorRGBString( color->string ) == -1 )
	{
		time_t long_time; // random isn't working fine at this point.
		struct tm *newtime; // so we get the user local time and use some values from there
		int rgbcolor;
		time( &long_time );
		newtime = localtime( &long_time );
		long_time *= newtime->tm_sec * newtime->tm_min * newtime->tm_wday;
		rgbcolor = COM_ValidatePlayerColor( COLOR_RGB( long_time&0xff, ( long_time>>8 )&0xff, ( long_time>>16 )&0xff ) );
		Cvar_Set( "color", va( "%i %i %i", COLOR_R( rgbcolor ), COLOR_G( rgbcolor ), COLOR_B( rgbcolor ) ) );
	}

	//
	// register our commands
	//
	Cmd_AddCommand( "s_restart", CL_S_Restart_f );
	Cmd_AddCommand( "cmd", CL_ForwardToServer_f );
	Cmd_AddCommand( "requestservers", CL_GetServers_f );
	Cmd_AddCommand( "getinfo", CL_QueryGetInfoMessage_f ); // wsw : jal : ask for server info
	Cmd_AddCommand( "getstatus", CL_QueryGetStatusMessage_f ); // wsw : jal : ask for server info
	Cmd_AddCommand( "userinfo", CL_Userinfo_f );
	Cmd_AddCommand( "disconnect", CL_Disconnect_f );
	Cmd_AddCommand( "record", CL_Record_f );
	Cmd_AddCommand( "stop", CL_Stop_f );
	Cmd_AddCommand( "quit", CL_Quit_f );
	Cmd_AddCommand( "connect", CL_Connect_f );
#if defined(TCP_SUPPORT) && defined(TCP_ALLOW_CONNECT)
	Cmd_AddCommand( "tcpconnect", CL_TCPConnect_f );
#endif
	Cmd_AddCommand( "reconnect", CL_Reconnect_f );
	Cmd_AddCommand( "rcon", CL_Rcon_f );
	Cmd_AddCommand( "writeconfig", CL_WriteConfig_f );
	Cmd_AddCommand( "showip", CL_ShowIP_f ); // jal : wsw : print our ip
	Cmd_AddCommand( "demo", CL_PlayDemo_f );
	Cmd_AddCommand( "demoavi", CL_PlayDemoToAvi_f );
	Cmd_AddCommand( "next", CL_SetNext_f );
	Cmd_AddCommand( "pingserver", CL_PingServer_f );
	Cmd_AddCommand( "demopause", CL_PauseDemo_f );
	Cmd_AddCommand( "demojump", CL_DemoJump_f );
	Cmd_AddCommand( "showserverip", CL_ShowServerIP_f );
	Cmd_AddCommand( "downloadstatus", CL_DownloadStatus_f );
	Cmd_AddCommand( "downloadcancel", CL_DownloadCancel_f );
}

/*
* CL_ShutdownLocal
*/
static void CL_ShutdownLocal( void )
{
	cls.state = CA_UNINITIALIZED;
	Com_SetClientState( CA_UNINITIALIZED );

	Cmd_RemoveCommand( "s_restart" );
	Cmd_RemoveCommand( "cmd" );
	Cmd_RemoveCommand( "requestservers" );
	Cmd_RemoveCommand( "getinfo" );
	Cmd_RemoveCommand( "getstatus" );
	Cmd_RemoveCommand( "userinfo" );
	Cmd_RemoveCommand( "disconnect" );
	Cmd_RemoveCommand( "record" );
	Cmd_RemoveCommand( "stop" );
	Cmd_RemoveCommand( "quit" );
	Cmd_RemoveCommand( "connect" );
#if defined(TCP_SUPPORT) && defined(TCP_ALLOW_CONNECT)
	Cmd_RemoveCommand( "tcpconnect" );
#endif
	Cmd_RemoveCommand( "reconnect" );
	Cmd_RemoveCommand( "rcon" );
	Cmd_RemoveCommand( "writeconfig" );
	Cmd_RemoveCommand( "showip" );
	Cmd_RemoveCommand( "demo" );
	Cmd_RemoveCommand( "demoavi" );
	Cmd_RemoveCommand( "next" );
	Cmd_RemoveCommand( "pingserver" );
	Cmd_RemoveCommand( "demopause" );
	Cmd_RemoveCommand( "demojump" );
	Cmd_RemoveCommand( "showserverip" );
	Cmd_RemoveCommand( "downloadstatus" );
	Cmd_RemoveCommand( "downloadcancel" );
}

//============================================================================


#ifdef _DEBUG
static void CL_LogStats( void )
{
	static unsigned int lasttimecalled = 0;
	if( log_stats->integer )
	{
		if( cls.state == CA_ACTIVE )
		{
			if( !lasttimecalled )
			{
				lasttimecalled = Sys_Milliseconds();
				if( log_stats_file )
					FS_Printf( log_stats_file, "0\n" );
			}
			else
			{
				unsigned int now = Sys_Milliseconds();

				if( log_stats_file )
					FS_Printf( log_stats_file, "%u\n", now - lasttimecalled );
				lasttimecalled = now;
			}
		}
	}
}
#endif

/*
* CL_TimedemoStats
*/
static void CL_TimedemoStats( void )
{
	if( cl_timedemo->integer )
	{
		static int lasttime = 0;
		int curtime = Sys_Milliseconds();
		if( lasttime != 0 )
		{
			if( curtime - lasttime >= 100 )
				cl.timedemo.counts[99]++;
			else
				cl.timedemo.counts[curtime-lasttime]++;
		}
		lasttime = curtime;
	}
}

/*
* CL_AdjustServerTime - adjust delta to new frame snap timestamp
*/
void CL_AdjustServerTime( unsigned int gamemsec )
{
	// hurry up if coming late (unless in demos)
	if( !cls.demo.playing )
	{
		if( ( cl.newServerTimeDelta < cl.serverTimeDelta ) && gamemsec > 1 )
			cl.serverTimeDelta--;
		if( cl.newServerTimeDelta > cl.serverTimeDelta )
			cl.serverTimeDelta++;
	}

	cl.serverTime = cls.gametime + cl.serverTimeDelta;

	// it launches a new snapshot when the timestamp of the CURRENT snap is reached.
	if( cl.pendingSnapNum && ( cl.serverTime >= cl.snapShots[cl.currentSnapNum & UPDATE_MASK].serverTime ) )
	{
		// fire next snapshot
		if( CL_GameModule_NewSnapshot( cl.pendingSnapNum ) )
		{
			cl.previousSnapNum = cl.currentSnapNum;
			cl.currentSnapNum = cl.pendingSnapNum;
			cl.pendingSnapNum = 0;

			// getting a valid snapshot ends the connection process
			if( cls.state == CA_CONNECTED )
				CL_SetClientState( CA_ACTIVE );
		}
	}
}

/*
* CL_RestartTimeDeltas
*/
void CL_RestartTimeDeltas( unsigned int newTimeDelta )
{
	int i;

	cl.serverTimeDelta = cl.newServerTimeDelta = newTimeDelta;
	for( i = 0; i < MAX_TIMEDELTAS_BACKUP; i++ )
		cl.serverTimeDeltas[i] = newTimeDelta;

	if( cl_debug_timeDelta->integer )
		Com_Printf( S_COLOR_CYAN"***** timeDelta restarted\n" );
}

/*
* CL_SmoothTimeDeltas
*/
int CL_SmoothTimeDeltas( void )
{
	int i, count;
	double delta;
	snapshot_t	*snap;

	if( cls.demo.playing )
	{
		if( cl.currentSnapNum <= 0 ) // if first snap
			return cl.serverTimeDeltas[cl.pendingSnapNum & MASK_TIMEDELTAS_BACKUP];

		return cl.serverTimeDeltas[cl.currentSnapNum & MASK_TIMEDELTAS_BACKUP];
	}

	i = cl.receivedSnapNum - min( MAX_TIMEDELTAS_BACKUP, 8 );
	if( i < 0 )
	{
		i = 0;
	}

	for( delta = 0, count = 0; i <= cl.receivedSnapNum; i++ )
	{
		snap = &cl.snapShots[i & UPDATE_MASK];
		if( snap->valid && snap->serverFrame == i )
		{
			delta += (double)cl.serverTimeDeltas[i & MASK_TIMEDELTAS_BACKUP];
			count++;
		}
	}

	if( !count )
		return 0;

	return (int)( delta / (double)count );
}

/*
* CL_UpdateSnapshot - Check for pending snapshots, and fire if needed
*/
void CL_UpdateSnapshot( void )
{
	snapshot_t	*snap;
	int i;

	// see if there is any pending snap to be fired
	if( !cl.pendingSnapNum && ( cl.currentSnapNum != cl.receivedSnapNum ) )
	{
		snap = NULL;
		for( i = cl.currentSnapNum + 1; i <= cl.receivedSnapNum; i++ )
		{
			if( cl.snapShots[i & UPDATE_MASK].valid && ( cl.snapShots[i & UPDATE_MASK].serverFrame > cl.currentSnapNum ) )
			{
				snap = &cl.snapShots[i & UPDATE_MASK];
				//torbenh: this break was the source of the lag bug at cl_fps < sv_pps
				//break;
			}
		}

		if( snap ) // valid pending snap found
		{
			cl.pendingSnapNum = snap->serverFrame;

			cl.newServerTimeDelta = CL_SmoothTimeDeltas();

			if( cl_extrapolationTime->modified )
			{
				if( cl_extrapolationTime->integer > (int)cl.snapFrameTime - 1 )
					Cvar_ForceSet( "cl_extrapolationTime", va( "%i", (int)cl.snapFrameTime - 1 ) );
				else if( cl_extrapolationTime->integer < 0 )
					Cvar_ForceSet( "cl_extrapolationTime", "0" );

				cl_extrapolationTime->modified = qfalse;
			}

			if( !cls.demo.playing && cl_extrapolate->integer )
				cl.newServerTimeDelta += cl_extrapolationTime->integer;

			// if we don't have current snap (or delay is too big) don't wait to fire the pending one
			if( ( !cls.demo.play_jump && cl.currentSnapNum <= 0 ) ||
				( !cls.demo.playing && abs( cl.newServerTimeDelta - cl.serverTimeDelta ) > 200 ) )
				cl.serverTimeDelta = cl.newServerTimeDelta;

			// don't either wait if in a timedemo
			if( cls.demo.playing && cl_timedemo->integer )
				cl.serverTimeDelta = cl.newServerTimeDelta;
		}
	}
}

/*
* CL_Netchan_Transmit
*/
void CL_Netchan_Transmit( msg_t *msg )
{
	int zerror;

	// if we got here with unsent fragments, fire them all now
	Netchan_PushAllFragments( &cls.netchan );

	// do not enable client compression until I fix the compression+fragmentation rare case bug
	if( ( cl_compresspackets->integer && msg->cursize > 60 ) || cl_compresspackets->integer > 1 )
	{
		zerror = Netchan_CompressMessage( msg );
		if( zerror < 0 ) // it's compression error, just send uncompressed
		{
			Com_DPrintf( "CL_Netchan_Transmit (ignoring compression): Compression error %i\n", zerror );
		}
	}

	Netchan_Transmit( &cls.netchan, msg );
	cls.lastPacketSentTime = cls.realtime;
}

/*
* CL_MaxPacketsReached
*/
static qboolean CL_MaxPacketsReached( void )
{
	static unsigned int lastPacketTime = 0;
	static float roundingMsec = 0.0f;
	int minpackettime;
	int elapsedTime;

	if( lastPacketTime > cls.realtime )
		lastPacketTime = cls.realtime;

	if( cl_pps->integer > 62 || cl_pps->integer < 20 )
	{
		Com_Printf( "'cl_pps' value is out of valid range, resetting to default\n" );
		Cvar_ForceSet( "cl_pps", va( "%s", cl_pps->dvalue ) );
	}

	elapsedTime = cls.realtime - lastPacketTime;
	if( cls.mv )
		minpackettime = ( 1000.0f / 2 );
	else
	{
		float minTime = ( 1000.0f / cl_pps->value );

		// don't let cl_pps be smaller than sv_pps
		if( cls.state == CA_ACTIVE && !cls.demo.playing && cl.snapFrameTime )
		{
			if( (unsigned int)minTime > cl.snapFrameTime )
				minTime = cl.snapFrameTime;
		}

		minpackettime = (int)minTime;
		roundingMsec += minTime - (int)minTime;
		if( roundingMsec >= 1.0f )
		{
			minpackettime += (int)roundingMsec;
			roundingMsec -= (int)roundingMsec;
		}
	}

	if( elapsedTime < minpackettime )
		return qfalse;

	lastPacketTime = cls.realtime;
	return qtrue;
}

/*
* CL_SendMessagesToServer
*/
void CL_SendMessagesToServer( qboolean sendNow )
{
	msg_t message;
	qbyte messageData[MAX_MSGLEN];

	if( cls.state == CA_DISCONNECTED || cls.state == CA_GETTING_TICKET || cls.state == CA_CONNECTING || cls.state == CA_CINEMATIC )
		return;

	if( cls.demo.playing )
		return;

	MSG_Init( &message, messageData, sizeof( messageData ) );
	MSG_Clear( &message );

	// send only reliable commands during connecting time
	if( cls.state < CA_ACTIVE )
	{
		if( sendNow || cls.realtime > 100 + cls.lastPacketSentTime )
		{
			// write the command ack
			if( !cls.reliable )
			{
				MSG_WriteByte( &message, clc_svcack );
				MSG_WriteLong( &message, (unsigned int)cls.lastExecutedServerCommand );
			}
			//write up the clc commands
			CL_UpdateClientCommandsToServer( &message );
			if( message.cursize > 0 )
				CL_Netchan_Transmit( &message );
		}
	}
	else if( sendNow || CL_MaxPacketsReached() )
	{
		// write the command ack
		if( !cls.reliable )
		{
			MSG_WriteByte( &message, clc_svcack );
			MSG_WriteLong( &message, (unsigned int)cls.lastExecutedServerCommand );
		}
		// send a userinfo update if needed
		if( userinfo_modified )
		{
			userinfo_modified = qfalse;
			CL_AddReliableCommand( va( "usri \"%s\"", Cvar_Userinfo() ) );
		}
		CL_UpdateClientCommandsToServer( &message );
		CL_WriteUcmdsToMessage( &message );
		if( message.cursize > 0 )
			CL_Netchan_Transmit( &message );
	}
}

/*
* CL_NetFrame
*/
static void CL_NetFrame( int realmsec, int gamemsec )
{
	// read packets from server
	if( realmsec > 5000 )  // if in the debugger last frame, don't timeout
		cls.lastPacketReceivedTime = cls.realtime;

	if( cls.demo.playing ) {
		CL_ReadDemoPackets(); // fetch results from demo file
	}
	CL_ReadPackets(); // fetch results from server

	// send packets to server
	if( cls.netchan.unsentFragments )
		Netchan_TransmitNextFragment( &cls.netchan );
	else
		CL_SendMessagesToServer( qfalse );

	// resend a connection request if necessary
	CL_CheckForResend();
	CL_CheckDownloadTimeout();
}

/*
* CL_Frame
*/
void CL_Frame( int realmsec, int gamemsec )
{
	static int allRealMsec = 0, allGameMsec = 0, extraMsec = 0;
	static float roundingMsec = 0.0f;
	int minMsec;

	if( dedicated->integer )
		return;

	cls.realtime += realmsec;

	if( cls.demo.playing && cls.demo.play_ignore_next_frametime )
	{
		gamemsec = 0;
		cls.demo.play_ignore_next_frametime = qfalse;
	}

	if( cl_demoavi_fps->modified )
	{
		float newvalue = 1000.0f / (int)( 1000.0f/cl_demoavi_fps->value );
		if( fabs( newvalue - cl_demoavi_fps->value ) > 0.001 )
			Com_Printf( "cl_demoavi_fps value has been adjusted to %.4f\n", newvalue );

		Cvar_SetValue( "cl_demoavi_fps", newvalue );
		cl_demoavi_fps->modified = qfalse;
	}

	// demoavi
	if( ( cls.demo.avi || cls.demo.pending_avi ) && cls.state == CA_ACTIVE )
	{
		if( cls.demo.pending_avi && !cls.demo.avi )
		{
			cls.demo.pending_avi = qfalse;
			CL_BeginDemoAviDump();
		}

		// fixed time for next frame
		if( cls.demo.avi_video )
		{
			gamemsec = ( 1000.0 / (double)cl_demoavi_fps->integer ) * Cvar_Value( "timescale" );
			if( gamemsec < 1 )
				gamemsec = 1;
		}
	}

	if( cls.demo.playing && cls.demo.paused )
		gamemsec = 0;

	cls.gametime += gamemsec;

	allRealMsec += realmsec;
	allGameMsec += gamemsec;

	CL_UpdateSnapshot();
	CL_AdjustServerTime( gamemsec );
	CL_UserInputFrame();
	CL_NetFrame( realmsec, gamemsec );
	CL_MM_Frame();

	if( cl_maxfps->integer > 0 && !cl_timedemo->integer && !( cls.demo.avi_video && cls.state == CA_ACTIVE ) )
	{
		// Vic: do not allow setting cl_maxfps to very low values to prevent cheating
		if( cl_maxfps->integer < 24 )
			Cvar_ForceSet( "cl_maxfps", "24" );

		minMsec = max( ( 1000.0f / cl_maxfps->value ), 1 );
		roundingMsec += max( ( 1000.0f / cl_maxfps->value ), 1.0f ) - minMsec;
	}
	else
	{
		minMsec = 1;
		roundingMsec = 0;
	}

	if( roundingMsec >= 1.0f )
	{
		minMsec += (int)roundingMsec;
		roundingMsec -= (int)roundingMsec;
	}

	if( allRealMsec + extraMsec < minMsec )
	{
#ifdef PUTCPU2SLEEP
		// let CPU sleep while playing fullscreen video or when explicitly told to do so by user
		if( ( cl_sleep->integer || cls.state == CA_CINEMATIC || cls.state == CA_DISCONNECTED ) && minMsec - extraMsec > 1 )
			Sys_Sleep( 1 );
#endif
		return;
	}

	cls.frametime = (float)allGameMsec * 0.001f;
	cls.realframetime = (float)allRealMsec * 0.001f;
#if 1
	if( allRealMsec < minMsec ) // is compensating for a too slow frame
	{
		extraMsec -= ( minMsec - allRealMsec );
		clamp( extraMsec, 0, 100 );
	}
	else // too slow, or exact frame
	{
		extraMsec = allRealMsec - minMsec;
		clamp( extraMsec, 0, 100 );
	}
#else
	extraMsec = allRealMsec - minMsec;
	clamp( extraMsec, 0, minMsec );
#endif

	CL_TimedemoStats();

	// allow rendering DLL change
	VID_CheckChanges();

	cl.inputRefreshed = qfalse;
	if( cls.state != CA_ACTIVE )
		CL_UpdateCommandInput();

	CL_NewUserCommand( allRealMsec );

	// update the screen
	if( host_speeds->integer )
		time_before_ref = Sys_Milliseconds();
	SCR_UpdateScreen();
	if( host_speeds->integer )
		time_after_ref = Sys_Milliseconds();

	// 0.50 doesn't refresh input from cgame if prediction is disabled.
	CL_UpdateCommandInput();

	if( CL_WriteAvi() )
	{
		int frame = ++cls.demo.avi_frame;
		if( cls.demo.avi_video )
			R_WriteAviFrame( frame, cl_demoavi_scissor->integer );
	}

	// update audio
	if( cls.state != CA_ACTIVE || cls.state == CA_CINEMATIC )
	{
		// if the loading plaque is up, clear everything out to make sure we aren't looping a dirty
		// dma buffer while loading
		if( cls.disable_screen )
			CL_SoundModule_Clear();
		else
			CL_SoundModule_Update( vec3_origin, vec3_origin, vec3_origin, vec3_origin, vec3_origin, NULL, qfalse );
	}

	// advance local effects for next frame
	SCR_RunCinematic();
	SCR_RunConsole( allRealMsec );

	allRealMsec = 0;
	allGameMsec = 0;

	cls.framecount++;
#ifdef _DEBUG
	CL_LogStats();
#endif
}


//============================================================================

static char *updateRemoteData;
static size_t updateRemoteDataSize;

/*
* CL_CheckForUpdateDoneCb
*/
static void CL_CheckForUpdateDoneCb( int status, const char *contentType, void *privatep )
{
	float local_version, net_version;

	if( status == 200 )
		goto done;

	// got the file
	// this look stupid but is the safe way to do it
	local_version = atof( va( "%4.3f", APP_VERSION ) );
	net_version = atof( updateRemoteData );

	// we have the version
	//Com_Printf("CheckForUpdate: local: %f net: %f\n", local_version, net_version);
	if( net_version > local_version )
	{
		char cmd[1024];
		char net_version_str[16], *s;

		Q_snprintfz( net_version_str, sizeof( net_version_str ), "%4.3f", net_version );
		s = net_version_str + strlen( net_version_str ) - 1;
		while( *s == '0' ) s--;
		net_version_str[s-net_version_str+1] = '\0';

		// you should update
		Com_Printf( APPLICATION " version %s is available.\nVisit " APP_URL " for more information\n", net_version_str );
		Q_snprintfz( cmd, sizeof( cmd ), "menu_msgbox \"Version %s of " APPLICATION " is available\" \"Visit " APP_URL " for more information\"", net_version_str );
		Cbuf_ExecuteText( EXEC_APPEND, cmd );
	}
	else if( net_version == local_version )
	{
		Com_Printf( "Your %s version is up-to-date.\n", APPLICATION );
	}

done:
	if( updateRemoteData )
	{
		Mem_Free( updateRemoteData );
		updateRemoteData = NULL;
		updateRemoteDataSize = 0;
	}
}

/*
* CL_CheckForUpdateReadCb
*/
static size_t CL_CheckForUpdateReadCb( const void *buf, size_t numb, float percentage, const char *contentType, void *privatep )
{
	char *newbuf;

	newbuf = Mem_ZoneMalloc( updateRemoteDataSize + numb + 1 );
	memcpy( newbuf, updateRemoteData, updateRemoteDataSize - 1 );
	memcpy( newbuf + updateRemoteDataSize - 1, buf, numb );
	newbuf[numb] = '\0'; // EOF

	Mem_Free( updateRemoteData );
	updateRemoteData = newbuf;

	return numb;
}

#define TRACKING_PROFILE_ID "profile.id"

/*
* CL_CheckForUpdateHeaderCb
*
* Read the Set-Profile-Id header which works as a cookie and
* store the read value in a file. All subsequent update requests
* will echo back this id.
*/
static void CL_CheckForUpdateHeaderCb( const char *buf, void *privatep )
{
	const char *str;

	if ( (str  = (char*)strstr(buf, "SET-PROFILE-ID:")) )
	{
		const char *val;
		size_t val_size;

		str += 15;
		while (*str && (*str == ' ')) { str++; }

		val = str;
		while (*str && (*str != '\r') && (*str != '\n') && (*str != '\n')) { str++; }
		val_size = str - val;

		if( val_size > 0 ) {
			int filenum; 
			
			if( FS_FOpenFile( TRACKING_PROFILE_ID, &filenum, FS_WRITE ) < 0 ) {
				return;
			}

			FS_Write( val, val_size - 1, filenum );
			FS_FCloseFile( filenum );
		}
	}
}

/*
* CL_CheckForUpdate
* 
* retrieve a file with the last version umber on a web server, compare with current version
* display a message box in case the user need to update
*/
static void CL_CheckForUpdate( void )
{
#ifdef PUBLIC_BUILD
#define HTTP_HEADER_SIZE	128

	char url[MAX_STRING_CHARS];
	char *resolution;
	char *campaign;
	int campaignSize;
	char *profileId;
	int profileIdSize;
	int headerNum = 0;
	const char *headers[7] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL };

	if( !cl_checkForUpdate->integer )
		return;

	if( updateRemoteData )
		return; // still not done with the previous iteration?..

	// step one get the last version file
	Com_Printf( "Checking for " APPLICATION " update.\n" );

	Q_snprintfz( url, sizeof( url ), "%s%s", APP_UPDATE_URL, APP_CLIENT_UPDATE_FILE );

	updateRemoteDataSize = 1;
	updateRemoteData = Mem_ZoneMalloc( 1 );
	*updateRemoteData = '\0';

	// send screen resolution in UA-pixels header
	resolution = Mem_TempMalloc( HTTP_HEADER_SIZE );
	Q_snprintfz( resolution, HTTP_HEADER_SIZE, "%ix%i", viddef.width, viddef.height );
	headers[headerNum++] = "UA-pixels";
	headers[headerNum++] = resolution;

	// send campaign ID
	campaign = NULL;
	campaignSize = FS_LoadBaseFile( "campaign.txt", &campaign, NULL, 0 );
	if( campaignSize > 0 ) {
		headers[headerNum++] = "X-Campaign";
		headers[headerNum++] = campaign;
	}

	// send profile ID
	profileId = NULL;
	profileIdSize = FS_LoadFile( TRACKING_PROFILE_ID, &profileId, NULL, 0 );
	if( profileIdSize > 0 ) {
		headers[headerNum++] = "X-Profile-Id";
		headers[headerNum++] = Q_strlwr( profileId );
	}

	CL_AsyncStreamRequest( url, headers, 15, 0, CL_CheckForUpdateReadCb, CL_CheckForUpdateDoneCb, 
		CL_CheckForUpdateHeaderCb, NULL, qfalse );

	Mem_TempFree( resolution );

	if( campaign ) {
		FS_FreeBaseFile( campaign );
	}
	if( profileId ) {
		FS_FreeBaseFile( profileId );
	}
#endif
}

//============================================================================

/*
* CL_AsyncStream_Alloc
*/
static void *CL_AsyncStream_Alloc( size_t size, const char *filename, int fileline )
{
	return _Mem_Alloc( zoneMemPool, size, 0, 0, filename, fileline );
}

/*
* CL_AsyncStream_Free
*/
static void CL_AsyncStream_Free( void *data, const char *filename, int fileline )
{
	_Mem_Free( data, 0, 0, filename, fileline );
}

/*
* CL_InitAsyncStream
*/
static void CL_InitAsyncStream( void )
{
	cl_async_stream = AsyncStream_InitModule( "Client", CL_AsyncStream_Alloc, CL_AsyncStream_Free );
}

/*
* CL_ShutdownAsyncStream
*/
static void CL_ShutdownAsyncStream( void )
{
	if( !cl_async_stream ) {
		return;
	}

	AsyncStream_ShutdownModule( cl_async_stream );
	cl_async_stream = NULL;
}

/*
* CL_AsyncStreamRequest
*/
void CL_AsyncStreamRequest( const char *url, const char **headers, int timeout, int resumeFrom,
	size_t (*read_cb)(const void *, size_t, float, const char *, void *), void (*done_cb)(int, const char *, void *), 
	void (*header_cb)(const char *, void *), void *privatep, qboolean urlencodeUnsafe )
{
	char *tmpUrl = NULL;
	const char *safeUrl;

	if( urlencodeUnsafe ) {
		// urlencode unsafe characters
		size_t allocSize = strlen( url ) * 3 + 1;
		tmpUrl = Mem_TempMalloc( allocSize );
		AsyncStream_UrlEncodeUnsafeChars( url, tmpUrl, allocSize );

		safeUrl = tmpUrl;
	}
	else {
		safeUrl = url;
	}

	AsyncStream_PerformRequestExt( cl_async_stream, safeUrl, "GET", NULL, headers, timeout, 
		resumeFrom, read_cb, done_cb, header_cb, NULL );

	if( urlencodeUnsafe ) {
		Mem_TempFree( tmpUrl );
	}
}

//============================================================================

/*
* CL_Init
*/
void CL_Init( void )
{
	netadr_t address;
	cvar_t *cl_port;
	cvar_t *cl_port6;

	assert( !cl_initialized );

	if( dedicated->integer )
		return; // nothing running on the client

	// all archived variables will now be loaded

	Con_Init();

	VID_Init();

	CL_ClearState();

	// IPv4
	NET_InitAddress( &address, NA_IP );
	cl_port = Cvar_Get( "cl_port", "0", CVAR_NOSET );
	NET_SetAddressPort( &address, cl_port->integer );
	if( !NET_OpenSocket( &cls.socket_udp, SOCKET_UDP, &address, qfalse ) )
		Com_Error( ERR_FATAL, "Couldn't open UDP socket: %s", NET_ErrorString() );

	// IPv6
	NET_InitAddress( &address, NA_IP6 );
	cl_port6 = Cvar_Get( "cl_port6", "0", CVAR_NOSET );
	NET_SetAddressPort( &address, cl_port6->integer );
	if( !NET_OpenSocket( &cls.socket_udp6, SOCKET_UDP, &address, qfalse ) )
		Com_Printf( "Error: Couldn't open UDP6 socket: %s", NET_ErrorString() );

	SCR_InitScreen();
	cls.disable_screen = qtrue; // don't draw yet

	CL_InitCinematics();

	CL_InitLocal();
	CL_InitInput();

	CL_InitAsyncStream();

	CL_SoundModule_Init( qtrue ); // sound must be initialized after window is created

	CL_InitMedia( qtrue );
	Cbuf_ExecuteText( EXEC_NOW, "menu_force 1" );

	// check for update
	CL_CheckForUpdate();

	CL_InitServerList();

	CL_MM_Init();

	ML_Init();

	CL_Mumble_Init();

	cl_initialized = qtrue;
}

/*
* CL_InitDynvars
*/
void CL_InitDynvars( void )
{
	CL_InitInputDynvars();
}

/*
* CL_Shutdown
* 
* FIXME: this is a callback from Sys_Quit and Com_Error.  It would be better
* to run quit through here before the final handoff to the sys code.
*/
void CL_Shutdown( void )
{
	if( !cl_initialized )
		return;

	CL_SoundModule_StopAllSounds();

	ML_Shutdown();
	CL_MM_Shutdown( qtrue );
	CL_ShutDownServerList();

	CL_WriteConfiguration( "config.cfg", qtrue );

	CL_Disconnect( NULL );
	NET_CloseSocket( &cls.socket_udp );
	NET_CloseSocket( &cls.socket_udp6 );
	// TOCHECK: Shouldn't we close the TCP socket too?
	if( cls.servername )
		Mem_ZoneFree( cls.servername );

	CL_UIModule_Shutdown();
	CL_GameModule_Shutdown();
	CL_SoundModule_Shutdown( qtrue );
	CL_ShutdownInput();
	CL_Mumble_Shutdown();
	VID_Shutdown();

	CL_ShutdownMedia( qtrue );

	CL_ShutdownCinematics();

	CL_ShutdownAsyncStream();

	CL_ShutdownLocal();

	Con_Shutdown();

	cls.state = CA_UNINITIALIZED;
	cl_initialized = qfalse;
}
