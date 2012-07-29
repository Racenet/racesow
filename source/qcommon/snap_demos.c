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

static size_t meta_data_ofs; // FIXME
static char dummy_meta_data[SNAP_MAX_DEMO_META_DATA_SIZE];

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

	read = FS_Read( msg->data, msglen, demofile );
	if( read != msglen )
		Com_Error( ERR_DROP, "Error reading demo file: End of file" );

	msg->cursize = msglen;
	msg->readcount = 0;

	return read;
}

/*
* SNAP_BeginDemoRecording
*/
void SNAP_BeginDemoRecording( int demofile, unsigned int spawncount, unsigned int snapFrameTime, 
	const char *sv_name, unsigned int sv_bitflags, purelist_t *purelist, char *configstrings, 
	entity_state_t *baselines, unsigned int baseTime )
{
	unsigned int i;
	msg_t msg;
	qbyte msg_buffer[MAX_MSGLEN];
	purelist_t *purefile;
	entity_state_t nullstate;
	entity_state_t *base;
	size_t demoinfo_len, demoinfo_len_pos, demoinfo_end;
	size_t meta_data_ofs_pos;

	MSG_Init( &msg, msg_buffer, sizeof( msg_buffer ) );

	// demoinfo message
	MSG_WriteByte( &msg, svc_demoinfo );

	demoinfo_len_pos = msg.cursize;
	MSG_WriteLong( &msg, 0 );	// svc_demoinfo length
	demoinfo_len = msg.cursize;

	meta_data_ofs_pos = msg.cursize;
	MSG_WriteLong( &msg, 0 );	// meta data start offset
	meta_data_ofs = msg.cursize;

	// internal data follows
	MSG_WriteLong( &msg, baseTime );	// initial server time

	// write zero-filled buffer now
	SNAP_ClearDemoMeta( dummy_meta_data, SNAP_MAX_DEMO_META_DATA_SIZE );

	meta_data_ofs = msg.cursize - meta_data_ofs;
	MSG_WriteLong( &msg, 0 );		// real size
	MSG_WriteLong( &msg, SNAP_MAX_DEMO_META_DATA_SIZE ); // max size
	MSG_WriteData( &msg, dummy_meta_data, SNAP_MAX_DEMO_META_DATA_SIZE );

	demoinfo_end = msg.cursize;
	demoinfo_len = msg.cursize - demoinfo_len;

	msg.cursize = demoinfo_len_pos;
	MSG_WriteLong( &msg, demoinfo_len );	// svc_demoinfo length
	msg.cursize = meta_data_ofs_pos;
	MSG_WriteLong( &msg, meta_data_ofs );	// meta data start offset

	msg.cursize = demoinfo_end;
	DEMO_SAFEWRITE( demofile, &msg, qtrue );

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
* SNAP_ClearDemoMeta
*/
size_t SNAP_ClearDemoMeta( char *meta_data, size_t meta_data_max_size )
{
	memset( meta_data, 0, meta_data_max_size );
	return 0;
}

/*
* SNAP_SetDemoMetaValue
*
* Stores a key-value pair of strings in a buffer in the following format:
* key1\0value1\0key2\0value2\0...keyN\0valueN\0
* The resulting string is ensured to be null-terminated.
*/
size_t SNAP_SetDemoMetaKeyValue( char *meta_data, size_t meta_data_max_size, size_t meta_data_realsize,
							  const char *key, const char *value )
{
	char *s;
	char *m_key, *m_val, *m_pastval;
	size_t key_size, value_size;
	const char *end = meta_data + meta_data_realsize;

	assert( key );
	assert( value );

	if( !key || !value ) {
		goto done;
	}
	if( !*key || !*value ) {
		goto done;
	}

	// find current key value and remove it
	for( s = meta_data; s < end && *s; ) {
		m_key = s;
		key_size = strlen( m_key ) + 1;
		m_val = m_key + key_size;
		if( m_val >= end ) {
			// key without the value pair, EOF
			goto done;
		}

		value_size = strlen( m_val ) + 1; 
		m_pastval = m_val + value_size;

		if( !Q_stricmp( m_key, key ) ) {
			if( !Q_stricmp( m_val, value ) ) {
				// unchanged
				goto done;
			}

			// key match, move everything past the key value
			// in place of the key
			memmove( m_key, m_pastval, end - m_pastval );
			meta_data_realsize -= (m_pastval - m_key);
			break;
		}

		// some other key, skip
		s = m_pastval;
	}

	key_size = strlen( key ) + 1;
	value_size = strlen( value ) + 1;
	if( meta_data_realsize + key_size + value_size > meta_data_max_size ) {
		// no space
		Com_Printf( "SNAP_SetDemoMetaValue: omitting value '%s' key '%s'\n", value, key );
		goto done;
	}

	memcpy( meta_data + meta_data_realsize, key, key_size ); meta_data_realsize += key_size;
	memcpy( meta_data + meta_data_realsize, value, value_size ); meta_data_realsize += value_size;

	// EOF
	meta_data[meta_data_max_size-1] = 0;

done:
	return meta_data_realsize;
}

/*
* SNAP_StopDemoRecording
*/
void SNAP_StopDemoRecording( int demofile, const char *meta_data, size_t meta_data_realsize )
{
	int i;
	const char e = '\0';

	// finishup
	i = LittleLong( -1 );
	FS_Write( &i, 4, demofile );

	if( !meta_data || !*meta_data || !meta_data_realsize ) {
		return;
	}

	// fseek to zero byte, skipping initial msg length + svc_demoinfo byte + svc_demoinfo length + meta data ofs + meta_data_len
	FS_Seek( demofile, 0 + sizeof(int) + 1 + sizeof(int) + meta_data_ofs + sizeof(int), FS_SEEK_SET );

	if( meta_data_realsize > SNAP_MAX_DEMO_META_DATA_SIZE ) {
		meta_data_realsize = SNAP_MAX_DEMO_META_DATA_SIZE;
	}

	i = LittleLong( meta_data_realsize );
	FS_Write( &i, sizeof( i ), demofile );

	i = LittleLong( SNAP_MAX_DEMO_META_DATA_SIZE );
	FS_Write( &i, sizeof( i ), demofile );

	FS_Write( meta_data, meta_data_realsize - 1, demofile );
	FS_Write( &e, 1, demofile );
}

/*
* SNAP_ReadDemoMetaData
*
* Reads null-terminated meta information from a demo file into a string
*/
size_t SNAP_ReadDemoMetaData( int demofile, char *meta_data, size_t meta_data_size )
{
	char demoinfo;
	int meta_data_ofs;
	unsigned int meta_data_realsize, meta_data_fullsize;

	if( !meta_data || !meta_data_size ) {
		return 0;
	}

	// fseek to zero byte, skipping initial msg length
	if( FS_Seek( demofile, 0 + sizeof(int), FS_SEEK_SET ) < 0 ) {
		return 0;
	}

	// read svc_demoinfo
	FS_Read( &demoinfo, 1, demofile );
	if( demoinfo != svc_demoinfo ) {
		return 0;
	}

	// skip demoinfo length
	FS_Seek( demofile, sizeof( int ), FS_SEEK_CUR );

	// read meta data offset
	FS_Read( ( void * )&meta_data_ofs, sizeof( int ), demofile );
	meta_data_ofs = LittleLong( meta_data_ofs );

	if( FS_Seek( demofile, meta_data_ofs, FS_SEEK_CUR ) < 0 ) {
		return 0;
	}

	FS_Read( ( void * )&meta_data_realsize, sizeof( int ), demofile );
	FS_Read( ( void * )&meta_data_fullsize, sizeof( int ), demofile );

	meta_data_realsize = LittleLong( meta_data_realsize );
	meta_data_fullsize = LittleLong( meta_data_fullsize );

	FS_Read( ( void * )meta_data, min( meta_data_size, meta_data_realsize ), demofile );
	meta_data[min(meta_data_realsize, meta_data_size-1)] = '\0'; // termination \0

	return meta_data_realsize;
}
