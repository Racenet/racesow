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

#include "qcommon.h"

#define DEMO_SAFEWRITE(demofile,msg,force) \
	if( force || (msg)->cursize > (msg)->maxsize / 2 ) \
	{ \
		SNAP_RecordDemoMessage( demofile, msg, 0 ); \
		MSG_Clear( msg ); \
	}

/*
* SNAP_RecordDemoMessage
*
* Writes given message to demofile
*/
void SNAP_RecordDemoMessage( int demofile, msg_t *msg, int offset )
{
	int len;

	if( !demofile )
		return;

	// now write the entire message to the file, prefixed by length
	len = LittleLong( msg->cursize ) - offset;
	if( len <= 0 )
		return;

	FS_Write( &len, 4, demofile );
	FS_Write( msg->data + offset, len, demofile );
}

/*
* SNAP_ReadDemoMessage
*/
int SNAP_ReadDemoMessage( int demofile, msg_t *msg )
{
	int read = 0, msglen = -1;

	read += FS_Read( &msglen, 4, demofile );

	msglen = LittleLong( msglen );
	if( msglen == -1 )
		return -1;

	if( msglen > MAX_MSGLEN )
		Com_Error( ERR_DROP, "Error reading demo file: msglen > MAX_MSGLEN" );
	if( (size_t )msglen > msg->maxsize )
		Com_Error( ERR_DROP, "Error reading demo file: msglen > msg->maxsize" );

	if( FS_Read( msg->data, msglen, demofile ) != msglen )
		Com_Error( ERR_DROP, "Error reading demo file: End of file" );
	read += msglen;

	msg->cursize = msglen;
	msg->readcount = 0;

	return read;
}

/*
* SNAP_BeginDemoRecording
*/
void SNAP_BeginDemoRecording( int demofile, unsigned int spawncount, unsigned int snapFrameTime, const char *sv_name, unsigned int sv_bitflags, purelist_t *purelist, char *configstrings, entity_state_t *baselines )
{
	unsigned int i;
	msg_t msg;
	qbyte msg_buffer[MAX_MSGLEN];
	purelist_t *purefile;
	entity_state_t nullstate;
	entity_state_t *base;

	MSG_Init( &msg, msg_buffer, sizeof( msg_buffer ) );

	// demoinfo message
	MSG_WriteByte( &msg, svc_demoinfo );
	MSG_WriteLong( &msg, 0 );	// initial server time
	MSG_WriteLong( &msg, 0 );	// demo duration in milliseconds
	MSG_WriteLong( &msg, 0 );	// file offset at which the final -1 was written

	// serverdata message
	MSG_WriteByte( &msg, svc_serverdata );
	MSG_WriteLong( &msg, APP_PROTOCOL_VERSION );
	MSG_WriteLong( &msg, spawncount );
	MSG_WriteShort( &msg, (unsigned short)snapFrameTime );
	MSG_WriteString( &msg, FS_BaseGameDirectory() );
	MSG_WriteString( &msg, FS_GameDirectory() );
	MSG_WriteShort( &msg, -1 ); // playernum
	MSG_WriteString( &msg, sv_name ); // level name
	MSG_WriteByte( &msg, sv_bitflags ); // sv_bitflags

	// pure files
	i = Com_CountPureListFiles( purelist );
	if( i > (short)0x7fff )
		Com_Error( ERR_DROP, "Error: Too many pure files." );

	MSG_WriteShort( &msg, i );

	purefile = purelist;
	while( purefile )
	{
		MSG_WriteString( &msg, purefile->filename );
		MSG_WriteLong( &msg, purefile->checksum );
		purefile = purefile->next;

		DEMO_SAFEWRITE( demofile, &msg, qfalse );
	}

	// config strings
	for( i = 0; i < MAX_CONFIGSTRINGS; i++ )
	{
		const char *configstring = configstrings + i * MAX_CONFIGSTRING_CHARS;
		if( configstring[0] )
		{
			MSG_WriteByte( &msg, svc_servercs );
			MSG_WriteString( &msg, va( "cs %i \"%s\"", i, configstring ) );

			DEMO_SAFEWRITE( demofile, &msg, qfalse );
		}
	}

	// baselines
	memset( &nullstate, 0, sizeof( nullstate ) );

	for( i = 0; i < MAX_EDICTS; i++ )
	{
		base = &baselines[i];
		if( base->modelindex || base->sound || base->effects )
		{
			MSG_WriteByte( &msg, svc_spawnbaseline );
			MSG_WriteDeltaEntity( &nullstate, base, &msg, qtrue, qtrue );

			DEMO_SAFEWRITE( demofile, &msg, qfalse );
		}
	}

	// client expects the server data to be in a separate packet
	DEMO_SAFEWRITE( demofile, &msg, qtrue );

	MSG_WriteByte( &msg, svc_servercs );
	MSG_WriteString( &msg, "precache" );

	DEMO_SAFEWRITE( demofile, &msg, qtrue );
}

/*
* SNAP_StopDemoRecording
*/
void SNAP_StopDemoRecording( int demofile, unsigned int basetime, unsigned int duration )
{
	int len;

	// finishup
	len = -1;
	FS_Write( &len, 4, demofile );

	// write the svc_demoinfo stuff
	len = FS_Tell( demofile );
	FS_Seek( demofile, 0+sizeof(int)+1, FS_SEEK_SET );

	FS_Write( &basetime, 4, demofile );
	FS_Write( &duration, 4, demofile );
	FS_Write( &len, 4, demofile );
}
