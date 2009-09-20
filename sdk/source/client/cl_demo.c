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
// cl_demo.c  -- demo recording

#include "client.h"

static void CL_PauseDemo( qboolean paused );

/*
====================
CL_WriteDemoMessage

Dumps the current net message, prefixed by the length
====================
*/
void CL_WriteDemoMessage( msg_t *msg )
{
	if( cls.demo.file <= 0 )
	{
		cls.demo.recording = qfalse;
		return;
	}

	// the first eight bytes are just packet sequencing stuff
	SNAP_RecordDemoMessage( cls.demo.file, msg, 8 );
}

/*
====================
CL_Stop_f

stop recording a demo
====================
*/
void CL_Stop_f( void )
{
	int arg;
	qboolean silent, cancel;

	// look through all the args
	silent = qfalse;
	cancel = qfalse;
	for( arg = 1; arg < Cmd_Argc(); arg++ )
	{
		if( !Q_stricmp( Cmd_Argv( arg ), "silent" ) )
			silent = qtrue;
		else if( !Q_stricmp( Cmd_Argv( arg ), "cancel" ) )
			cancel = qtrue;
	}

	if( !cls.demo.recording )
	{
		if( !silent )
			Com_Printf( "Not recording a demo.\n" );
		return;
	}

	// finish up
	SNAP_StopDemoRecording( cls.demo.file, cls.demo.basetime, cls.demo.duration );

	FS_FCloseFile( cls.demo.file );

	// cancel the demos
	if( cancel )
	{
		// remove the file that correspond to cls.demo.file
		if( !silent )
			Com_Printf( "Canceling demo: %s\n", cls.demo.filename );
		if( !FS_RemoveFile( cls.demo.filename ) && !silent )
			Com_Printf( "Error canceling demo." );
	}

	if( !silent )
		Com_Printf( "Stopped demo: %s\n", cls.demo.filename );

	cls.demo.file = 0; // file id
	Mem_ZoneFree( cls.demo.filename );
	cls.demo.filename = NULL;
	cls.demo.recording = qfalse;
}

/*
====================
CL_Record_f

record <demoname>

Begins recording a demo from the current position
====================
*/
void CL_Record_f( void )
{
	char *name;
	size_t name_size;
	qboolean silent;

	if( cls.state != CA_ACTIVE )
	{
		Com_Printf( "You must be in a level to record.\n" );
		return;
	}

	if( Cmd_Argc() < 2 )
	{
		Com_Printf( "record <demoname>\n" );
		return;
	}

	if( Cmd_Argc() > 2 && !Q_stricmp( Cmd_Argv( 2 ), "silent" ) )
		silent = qtrue;
	else
		silent = qfalse;

	if( cls.demo.playing )
	{
		if( !silent )
			Com_Printf( "You can't record from another demo.\n" );
		return;
	}

	if( cls.demo.recording )
	{
		if( !silent )
			Com_Printf( "Already recording.\n" );
		return;
	}

	//
	// open the demo file
	//
	name_size = sizeof( char ) * ( strlen( "demos/" ) + strlen( Cmd_Argv( 1 ) ) + strlen( APP_DEMO_EXTENSION_STR ) + 1 );
	name = Mem_ZoneMalloc( name_size );

	Q_snprintfz( name, name_size, "demos/%s", Cmd_Argv( 1 ) );
	COM_SanitizeFilePath( name );
	COM_DefaultExtension( name, APP_DEMO_EXTENSION_STR, name_size );

	if( !COM_ValidateRelativeFilename( name ) )
	{
		if( !silent )
			Com_Printf( "Invalid filename.\n" );
		Mem_ZoneFree( name );
		return;
	}

	if( FS_FOpenFile( name, &cls.demo.file, FS_WRITE ) == -1 )
	{
		Com_Printf( "Error: Couldn't create the demo file.\n" );
		Mem_ZoneFree( name );
		return;
	}

	if( !silent )
		Com_Printf( "Recording demo: %s\n", name );

	// store the name in case we need it later
	cls.demo.filename = name;
	cls.demo.recording = qtrue;
	cls.demo.basetime = cls.demo.duration = 0;

	// don't start saving messages until a non-delta compressed message is received
	CL_AddReliableCommand( "nodelta" ); // request non delta compressed frame from server
	cls.demo.waiting = qtrue;

	// write out messages to hold the startup information
	SNAP_BeginDemoRecording( cls.demo.file, 0x10000 + cl.servercount, cl.snapFrameTime, cl.servermessage, cls.reliable ? SV_BITFLAGS_RELIABLE : 0, cls.purelist, cl.configstrings[0], cl_baselines );

	// the rest of the demo file will be individual frames
}


//================================================================
//
//	WARSOW : CLIENT SIDE DEMO PLAYBACK
//
//================================================================


// demo file
static int demofilehandle;
static int demofilelen;

/*
=================
CL_BeginDemoAviDump
=================
*/
/*static*/ void CL_BeginDemoAviDump( void )
{
	if( cls.demo.avi )
		return;

	cls.demo.avi_video = (cl_demoavi_video->integer ? qtrue : qfalse);
	cls.demo.avi_audio = (cl_demoavi_audio->integer ? qtrue : qfalse);
	cls.demo.avi = (cls.demo.avi_video || cls.demo.avi_audio);
	cls.demo.avi_frame = 0;

	if( cls.demo.avi_video )
		R_BeginAviDemo();

	if( cls.demo.avi_audio )
		CL_SoundModule_BeginAviDemo();
}

/*
=================
CL_StopDemoAviDump
=================
*/
static void CL_StopDemoAviDump( void )
{
	if( !cls.demo.avi )
		return;

	if( cls.demo.avi_video )
	{
		R_StopAviDemo();
		cls.demo.avi_video = qfalse;
	}

	if( cls.demo.avi_audio )
	{
		CL_SoundModule_StopAviDemo();
		cls.demo.avi_audio = qfalse;
	}

	cls.demo.avi = qfalse;
	cls.demo.avi_frame = 0;
}

/*
=================
CL_DemoCompleted

Close the demo file and disable demo state. Called from disconnection proccess
=================
*/
void CL_DemoCompleted( void )
{
	if( cls.demo.avi )
		CL_StopDemoAviDump();

	if( demofilehandle )
	{
		FS_FCloseFile( demofilehandle );
		demofilehandle = 0;
		demofilelen = 0;
	}

	cls.demo.playing = qfalse;
	cls.demo.basetime = cls.demo.duration = 0;

	Com_SetDemoPlaying( qfalse );

	CL_PauseDemo( qfalse );

	Cvar_ForceSet( "demoname", "" ); // used by democams to load up the .cam file
	Cvar_ForceSet( "demotime", "0" );
	Cvar_ForceSet( "demoduration", "0" );

	Com_Printf( "Demo completed\n" );

	memset( &cls.demo, 0, sizeof( cls.demo ) );
}

/*
=================
CL_ReadDemoMessage

Read a packet from the demo file and send it to the messages parser
=================
*/
static void CL_ReadDemoMessage( void )
{
	static qbyte msgbuf[MAX_MSGLEN];
	static msg_t demomsg;
	static qboolean init = qtrue;
	int read;

	if( !demofilehandle )
	{
		CL_Disconnect( NULL );
		return;
	}

	if( demofilelen <= 0 )
	{
		CL_Disconnect( NULL );
		return;
	}

	if( init )
	{
		MSG_Init( &demomsg, msgbuf, sizeof( msgbuf ) );
		init = qfalse;
	}

	read = SNAP_ReadDemoMessage( demofilehandle, &demomsg );
	if( read == -1 )
	{
		CL_Disconnect( NULL );
		return;
	}

	demofilelen -= read;

	CL_ParseServerMessage( &demomsg );
}

/*
=================
CL_ReadDemoPackets

See if it's time to read a new demo packet
=================
*/
void CL_ReadDemoPackets( void )
{
	while( cls.demo.playing && ( cl.receivedSnapNum <= 0 || !cl.frames[cl.receivedSnapNum&UPDATE_MASK].valid || cl.frames[cl.receivedSnapNum&UPDATE_MASK].serverTime < cl.serverTime ) )
		CL_ReadDemoMessage();

	Cvar_ForceSet( "demoduration", va( "%i", (int)ceil( cls.demo.duration/1000.0f ) ) );
	Cvar_ForceSet( "demotime", va( "%i", (int)floor( max(cl.frames[cl.receivedSnapNum & UPDATE_MASK].serverTime - cls.demo.basetime,0)/1000.0f ) ) );

	if( cls.demo.play_jump )
		cls.demo.play_jump = qfalse;
}

/*
====================
CL_StartDemo
====================
*/
static void CL_StartDemo( const char *demoname )
{
	size_t name_size;
	char *name, *servername;
	int tempdemofilehandle = 0, tempdemofilelen = -1;

	// have to copy the argument now, since next actions will lose it
	servername = TempCopyString( demoname );
	COM_SanitizeFilePath( servername );

	name_size = sizeof( char ) * ( strlen( "demos/" ) + strlen( servername ) + strlen( APP_DEMO_EXTENSION_STR ) + 1 );
	name = Mem_TempMalloc( name_size );

	Q_snprintfz( name, name_size, "demos/%s", servername );
	COM_DefaultExtension( name, APP_DEMO_EXTENSION_STR, name_size );
	if( COM_ValidateRelativeFilename( name ) )
		tempdemofilelen = FS_FOpenFile( name, &tempdemofilehandle, FS_READ );	// open the demo file

	if( !tempdemofilehandle || tempdemofilelen < 1 )
	{
		// relative filename didn't work, try launching a demo from absolute path
		Q_snprintfz( name, name_size, "%s", servername );
		COM_DefaultExtension( name, APP_DEMO_EXTENSION_STR, name_size );
		tempdemofilelen = FS_FOpenAbsoluteFile( name, &tempdemofilehandle, FS_READ );
	}

	if( !tempdemofilehandle || tempdemofilelen < 1 )
	{
		Com_Printf( "No valid demo file found\n" );
		FS_FCloseFile( tempdemofilehandle );
		Mem_TempFree( name );
		Mem_TempFree( servername );
		return;
	}

	// make sure a local server is killed
	Cbuf_ExecuteText( EXEC_NOW, "killserver\n" );
	CL_Disconnect( NULL );
	// wsw: Medar: fix for menu getting stuck on screen when starting demo, but maybe there is better fix out there?
	CL_UIModule_ForceMenuOff();

	memset( &cls.demo, 0, sizeof( cls.demo ) );

	demofilehandle = tempdemofilehandle;
	demofilelen = tempdemofilelen;

	cls.servername = ZoneCopyString( COM_FileBase( servername ) );
	COM_StripExtension( cls.servername );

	CL_SetClientState( CA_HANDSHAKE );
	Com_SetDemoPlaying( qtrue );
	cls.demo.playing = qtrue;
	cls.demo.basetime = cls.demo.duration = 0;

	cls.demo.play_ignore_next_frametime = qfalse;
	cls.demo.play_jump = qfalse;

	CL_PauseDemo( qfalse );

	// hack the read-only cvars in (for UI)
	Cvar_Get( "demotime", "0", CVAR_READONLY );
	Cvar_Get( "demoduration", "0", CVAR_READONLY );
	Cvar_ForceSet( "demoname", name ); // used by democams to load up the .cam file

	// set up for timedemo settings
	memset( &cl.timedemo, 0, sizeof( cl.timedemo ) );

	Mem_TempFree( name );
	Mem_TempFree( servername );
}

/*
====================
CL_PlayDemo_f

demo <demoname>
====================
*/
void CL_PlayDemo_f( void )
{
	if( Cmd_Argc() != 2 )
	{
		Com_Printf( "demo <demoname>\n" );
		return;
	}
	CL_StartDemo( Cmd_Argv( 1 ) );
}

/*
====================
CL_PauseDemo
====================
*/
static void CL_PauseDemo( qboolean paused )
{
	cls.demo.paused = paused;
	Cvar_ForceSet( "demopaused", va("%i", cls.demo.paused ) );
}

/*
====================
CL_PauseDemo_f
====================
*/
void CL_PauseDemo_f( void )
{
	if( !cls.demo.playing )
	{
		Com_Printf( "Can only demopause when playing a demo.\n" );
		return;
	}

	if( Cmd_Argc() > 1 )
	{
		if( !Q_stricmp( Cmd_Argv( 1 ), "on" ) )
			CL_PauseDemo( qtrue );
		else if( !Q_stricmp( Cmd_Argv( 1 ), "off" ) )
			CL_PauseDemo( qfalse );
		return;
	}

	CL_PauseDemo( !cls.demo.paused );
}

/*
====================
CL_DemoJump_f
====================
*/
void CL_DemoJump_f( void )
{
	qboolean relative;
	int time;
	char *p;

	if( !cls.demo.playing )
	{
		Com_Printf( "Can only demojump when playing a demo\n" );
		return;
	}

	if( Cmd_Argc() != 2 )
	{
		Com_Printf( "Usage: demojump <time>\n" );
		Com_Printf( "Time format is [minutes:]seconds\n" );
		Com_Printf( "Use '+' or '-' in front of the time to specify it in relation to current position\n" );
		return;
	}

	p = Cmd_Argv( 1 );

	if( Cmd_Argv( 1 )[0] == '+' || Cmd_Argv( 1 )[0] == '-' )
	{
		relative = qtrue;
		p++;
	}
	else
	{
		relative = qfalse;
	}

	if( strchr( p, ':' ) )
		time = ( atoi( p ) * 60 + atoi( strchr( p, ':' ) + 1 ) ) * 1000;
	else
		time = atoi( p ) * 1000;

	if( Cmd_Argv( 1 )[0] == '-' )
		time = -time;

	CL_SoundModule_StopAllSounds ();

	if( relative )
		cls.gametime += time;
	else
		cls.gametime = time; // gametime always starts from 0

	if( cl.serverTime < cl.frames[cl.receivedSnapNum&UPDATE_MASK].serverTime )
		cl.pendingSnapNum = 0;

	CL_AdjustServerTime( 1 );

	if( cl.serverTime < cl.frames[cl.receivedSnapNum&UPDATE_MASK].serverTime )
	{
		demofilelen += FS_Tell( demofilehandle );
		FS_Seek( demofilehandle, 0, FS_SEEK_SET );
		cl.currentSnapNum = cl.receivedSnapNum = 0;
	}

	cls.demo.play_jump = qtrue;
}

/*
====================
CL_PlayDemoToAvi_f

demoavi <demoname> (if no name suplied, toogles demoavi status)
====================
*/
void CL_PlayDemoToAvi_f( void )
{
	if( Cmd_Argc() == 1 && cls.demo.playing ) // toggle demoavi mode
	{
		if( !cls.demo.avi )
			CL_BeginDemoAviDump();
		else
			CL_StopDemoAviDump();
	}
	else if( Cmd_Argc() == 2 )
	{
		char *tempname = TempCopyString( Cmd_Argv( 1 ) );

		CL_StartDemo( tempname );

		if( cls.demo.playing )
			cls.demo.pending_avi = qtrue;

		Mem_TempFree( tempname );
	}
	else
	{
		Com_Printf( "Usage: %sdemoavi <demoname>%s or %sdemoavi%s while playing a demo\n",
			S_COLOR_YELLOW, S_COLOR_WHITE, S_COLOR_YELLOW, S_COLOR_WHITE );
	}
}
