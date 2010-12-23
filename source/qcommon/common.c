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
// common.c -- misc functions used in client and server
#include "qcommon.h"
#include <setjmp.h>
#include "md5.h"
#include "glob.h"

#define MAX_NUM_ARGVS	50

static qboolean	dynvars_initialized = qfalse;
static qboolean	commands_intialized = qfalse;

static int com_argc;
static char *com_argv[MAX_NUM_ARGVS+1];
static char com_errormsg[MAX_PRINTMSG];

static jmp_buf abortframe;     // an ERR_DROP occured, exit the entire frame

cvar_t *host_speeds;
cvar_t *log_stats;
cvar_t *developer;
cvar_t *timescale;
cvar_t *dedicated;
cvar_t *versioncvar;
cvar_t *revisioncvar;
cvar_t *tv_server;
cvar_t *mm_server;

static cvar_t *fixedtime;
static cvar_t *logconsole = NULL;
static cvar_t *logconsole_append;
static cvar_t *logconsole_flush;
static cvar_t *logconsole_timestamp;
static cvar_t *com_showtrace;
static cvar_t *com_introPlayed;

int log_stats_file = 0;
static int log_file = 0;

static int server_state = CA_UNINITIALIZED;
static int client_state = CA_UNINITIALIZED;
static qboolean	demo_playing = qfalse;

// host_speeds times
unsigned int time_before_game;
unsigned int time_after_game;
unsigned int time_before_ref;
unsigned int time_after_ref;

/*
   ==============================================================

   DYNVARS

   ==============================================================
 */

static dynvar_get_status_t Com_Sys_Uptime_f( void **val )
{
	static char buf[32];
	const quint64 us = Sys_Microseconds();
	const unsigned int h = us / 3600000000u;
	const unsigned int min = ( us / 60000000 ) % 60;
	const unsigned int sec = ( us / 1000000 ) % 60;
	const unsigned int usec = us % 1000000;
	sprintf( buf, "%02d:%02d:%02d.%06d", h, min, sec, usec );
	*val = buf;
	return DYNVAR_GET_OK;
}

/*
   ==============================================================

   Commands

   ==============================================================
 */

#ifdef SYS_SYMBOL
static void Com_Sys_Symbol_f( void )
{
	const int argc = Cmd_Argc();
	if( argc == 2 || argc == 3 )
	{
		const char *const symbolName = Cmd_Argv( 1 );
		const char *const moduleName = Cmd_Argc() == 3
		                               ? Cmd_Argv( 2 )
					       : NULL;
		void *symbolAddr = Sys_GetSymbol( moduleName, symbolName );
		if( symbolAddr )
		{
			Com_Printf( "\"%s\" is \"0x%p\"\n", symbolName, symbolAddr );
		}
		else
			Com_Printf( "Error: Symbol not found\n" );
	}
	else
		Com_Printf( "usage: sys_symbol <symbolName> [moduleName]\n" );
}
#endif // SYS_SYMBOL

/*
==============================================================

BSP FORMATS

==============================================================
*/

static const int mod_IBSPQ1Versions[] = { Q1_BSPVERSION, 0 };

const bspFormatDesc_t q1BSPFormats[] =
{
	{ "", mod_IBSPQ1Versions, 0, 0, 0, Q1_LUMP_ENTITIES },

	// trailing NULL
	{ NULL, NULL, 0, 0, 0 }

};

static const int mod_IBSPQ2Versions[] = { Q2_BSPVERSION, 0 };

const bspFormatDesc_t q2BSPFormats[] =
{
	{ IDBSPHEADER, mod_IBSPQ2Versions, 0, 0, 0, Q2_LUMP_ENTITIES },

	// trailing NULL
	{ NULL, NULL, 0, 0, 0 }

};

static const int mod_IBSPQ3Versions[] = { Q3BSPVERSION, RTCWBSPVERSION, 0 };
static const int mod_RBSPQ3Versions[] = { RBSPVERSION, 0 };
static const int mod_FBSPQ3Versions[] = { QFBSPVERSION, 0 };

const bspFormatDesc_t q3BSPFormats[] =
{
	{ QFBSPHEADER, mod_FBSPQ3Versions, QF_LIGHTMAP_WIDTH, QF_LIGHTMAP_HEIGHT, BSP_RAVEN, LUMP_ENTITIES },
	{ IDBSPHEADER, mod_IBSPQ3Versions, LIGHTMAP_WIDTH, LIGHTMAP_HEIGHT, BSP_NONE, LUMP_ENTITIES },
	{ RBSPHEADER, mod_RBSPQ3Versions, LIGHTMAP_WIDTH, LIGHTMAP_HEIGHT, BSP_RAVEN, LUMP_ENTITIES },

	// trailing NULL
	{ NULL, NULL, 0, 0, 0 }
};

/*
* Com_FindBSPFormat
*/
const bspFormatDesc_t *Com_FindBSPFormat( const bspFormatDesc_t *formats, const char *header, int version )
{
	int j;
	const bspFormatDesc_t *bspFormat;

	// check whether any of passed formats matches the header/version combo
	for( bspFormat = formats; bspFormat->header; bspFormat++ )
	{
		if( strlen( bspFormat->header ) && strncmp( header, bspFormat->header, strlen( bspFormat->header ) ) )
			continue;

		// check versions listed for this header
		for( j = 0; bspFormat->versions[j]; j++ )
		{
			if( version == bspFormat->versions[j] )
				break;
		}

		// found a match
		if( bspFormat->versions[j] )
			return bspFormat;
	}

	return NULL;
}

/*
==================
Com_FindFormatDescriptor
==================
*/
const modelFormatDescr_t *Com_FindFormatDescriptor( const modelFormatDescr_t *formats, const qbyte *buf, const bspFormatDesc_t **bspFormat )
{
	int i;
	const modelFormatDescr_t *descr;

	// search for a matching header
	for( i = 0, descr = formats; descr->header; i++, descr++ )
	{
		if( descr->header[0] == '*' )
		{
			const char *header;
			int version;

			header = ( const char * )buf;
			version = LittleLong( *((int *)((qbyte *)buf + descr->headerLen)) );

			// check whether any of specified formats matches the header/version combo
			*bspFormat = Com_FindBSPFormat( descr->bspFormats, header, version );
			if( *bspFormat )
				return descr;
		}
		else
		{
			if( !strncmp( (const char *)buf, descr->header, descr->headerLen ) )
				return descr;
		}
	}

	return NULL;
}

/*
   ============================================================================

   CLIENT / SERVER interactions

   ============================================================================
 */

static int rd_target;
static char *rd_buffer;
static int rd_buffersize;
static void ( *rd_flush )( int target, char *buffer, const void *extra );
static const void *rd_extra;

void Com_BeginRedirect( int target, char *buffer, int buffersize, void ( *flush ), const void *extra )
{
	if( !target || !buffer || !buffersize || !flush )
		return;
	rd_target = target;
	rd_buffer = buffer;
	rd_buffersize = buffersize;
	rd_flush = flush;
	rd_extra = extra;

	*rd_buffer = 0;
}

void Com_EndRedirect( void )
{
	rd_flush( rd_target, rd_buffer, rd_extra );

	rd_target = 0;
	rd_buffer = NULL;
	rd_buffersize = 0;
	rd_flush = NULL;
	rd_extra = NULL;
}

/*
   =============
   Com_Printf

   Both client and server can use this, and it will output
   to the apropriate place.
   =============
 */
void Com_Printf( const char *format, ... )
{
	va_list	argptr;
	char msg[MAX_PRINTMSG];

	time_t timestamp;
	char timestamp_str[MAX_PRINTMSG];
	struct tm *timestampptr;
	timestamp = time( NULL );
	timestampptr = gmtime( &timestamp );
	strftime( timestamp_str, MAX_PRINTMSG, "%Y-%m-%dT%H:%M:%SZ ", timestampptr );

	va_start( argptr, format );
	Q_vsnprintfz( msg, sizeof( msg ), format, argptr );
	va_end( argptr );

	if( rd_target )
	{
		if( (int)( strlen( msg ) + strlen( rd_buffer ) ) > ( rd_buffersize - 1 ) )
		{
			rd_flush( rd_target, rd_buffer, rd_extra );
			*rd_buffer = 0;
		}
		strcat( rd_buffer, msg );
		return;
	}

	Con_Print( msg );

	// also echo to debugging console
	Sys_ConsoleOutput( msg );

	// logconsole
	if( logconsole && logconsole->modified )
	{
		logconsole->modified = qfalse;

		if( log_file )
		{
			FS_FCloseFile( log_file );
			log_file = 0;
		}

		if( logconsole->string && logconsole->string[0] )
		{
			size_t name_size;
			char *name;

			name_size = strlen( logconsole->string ) + strlen( ".log" ) + 1;
			name = Mem_TempMalloc( name_size );
			Q_strncpyz( name, logconsole->string, name_size );
			COM_DefaultExtension( name, ".log", name_size );

			if( FS_FOpenFile( name, &log_file, ( logconsole_append && logconsole_append->integer ? FS_APPEND : FS_WRITE ) ) == -1 )
			{
				log_file = 0;
				Com_Printf( "Couldn't open: %s\n", name );
			}

			Mem_TempFree( name );
		}
	}

	if( log_file )
	{
		if( logconsole_timestamp && logconsole_timestamp->integer )
			FS_Printf( log_file, "%s", timestamp_str );
		FS_Printf( log_file, "%s", msg );
		if( logconsole_flush && logconsole_flush->integer )
			FS_Flush( log_file ); // force it to save every time
	}
}


/*
   ================
   Com_DPrintf

   A Com_Printf that only shows up if the "developer" cvar is set
   ================
 */
void Com_DPrintf( const char *format, ... )
{
	va_list	argptr;
	char msg[MAX_PRINTMSG];

	if( !developer || !developer->integer )
		return; // don't confuse non-developers with techie stuff...

	va_start( argptr, format );
	Q_vsnprintfz( msg, sizeof( msg ), format, argptr );
	va_end( argptr );

	Com_Printf( "%s", msg );
}


/*
   =============
   Com_Error

   Both client and server can use this, and it will
   do the apropriate things.
   =============
 */
void Com_Error( int code, const char *format, ... )
{
	va_list	argptr;
	char *msg = com_errormsg;
	const size_t sizeof_msg = sizeof( com_errormsg );
	static qboolean	recursive = qfalse;

	if( recursive )
	{
		Com_Printf( "recursive error after: %s", msg ); // wsw : jal : log it
		Sys_Error( "recursive error after: %s", msg );
	}
	recursive = qtrue;

	va_start( argptr, format );
	Q_vsnprintfz( msg, sizeof_msg, format, argptr );
	va_end( argptr );

	if( code == ERR_DROP )
	{
		Com_Printf( "********************\nERROR: %s\n********************\n", msg );
		SV_ShutdownGame( va( "Server crashed: %s\n", msg ), qfalse );
		CL_Disconnect( msg );
		recursive = qfalse;
		longjmp( abortframe, -1 );
	}
	else
	{
		Com_Printf( "********************\nERROR: %s\n********************\n", msg );
		SV_Shutdown( va( "Server fatal crashed: %s\n", msg ) );
		CL_Shutdown();
		MM_Shutdown();
	}

	if( log_file )
	{
		FS_FCloseFile( log_file );
		log_file = 0;
	}

	Sys_Error( "%s", msg );
}


/*
   =============
   Com_Quit

   Both client and server can use this, and it will
   do the apropriate things.
   =============
 */
void Com_Quit( void )
{
	if( dynvars_initialized )
	{
		dynvar_t *quit = Dynvar_Lookup( "quit" );
		if( quit )
		{
			// wsw : aiwa : added "quit" event for pluggable clean-up (e.g. IRC shutdown)
			Dynvar_CallListeners( quit, NULL );
		}
		Dynvar_Destroy( quit );
	}

	SV_Shutdown( "Server quit\n" );
	CL_Shutdown();
	MM_Shutdown();

	if( log_file )
	{
		FS_FCloseFile( log_file );
		log_file = 0;
	}

	Sys_Quit();
}


/*
   ==================
   Com_ServerState
   ==================
 */
int Com_ServerState( void )
{
	return server_state;
}

/*
   ==================
   Com_SetServerState
   ==================
 */
void Com_SetServerState( int state )
{
	server_state = state;
}

int Com_ClientState( void )
{
	return client_state;
}

void Com_SetClientState( int state )
{
	client_state = state;
}

qboolean Com_DemoPlaying( void )
{
	return demo_playing;
}

void Com_SetDemoPlaying( qboolean state )
{
	demo_playing = state;
}


/*
   ==========
   Com_HashKey

   Returns hash key for a string
   ==========
 */
unsigned int Com_HashKey( const char *name, int hashsize )
{
	int i;
	unsigned int v;
	unsigned int c;

	v = 0;
	for( i = 0; name[i]; i++ )
	{
		c = name[i];
		if( c == '\\' )
			c = '/';
		v = ( v + i ) * 37 + tolower( c ); // case insensitivity
	}

	return v % hashsize;
}

unsigned int Com_DaysSince1900( void )
{
	time_t long_time;
	struct tm *newtime;

	// get date from system
	time( &long_time );
	newtime = localtime( &long_time );

	return ( newtime->tm_year * 365 ) + newtime->tm_yday;
}

//============================================================================

/*
   ================
   COM_CheckParm

   Returns the position (1 to argc-1) in the program's argument list
   where the given parameter apears, or 0 if not present
   ================
 */
int COM_CheckParm( char *parm )
{
	int i;

	for( i = 1; i < com_argc; i++ )
	{
		if( !strcmp( parm, com_argv[i] ) )
			return i;
	}

	return 0;
}

int COM_Argc( void )
{
	return com_argc;
}

char *COM_Argv( int arg )
{
	if( arg < 0 || arg >= com_argc || !com_argv[arg] )
		return "";
	return com_argv[arg];
}

void COM_ClearArgv( int arg )
{
	if( arg < 0 || arg >= com_argc || !com_argv[arg] )
		return;
	com_argv[arg] = "";
}


/*
   ================
   COM_InitArgv
   ================
 */
void COM_InitArgv( int argc, char **argv )
{
	int i;

	if( argc > MAX_NUM_ARGVS )
		Com_Error( ERR_FATAL, "argc > MAX_NUM_ARGVS" );
	com_argc = argc;
	for( i = 0; i < argc; i++ )
	{
		if( !argv[i] || strlen( argv[i] ) >= MAX_TOKEN_CHARS )
			com_argv[i] = "";
		else
			com_argv[i] = argv[i];
	}
}

/*
   ================
   COM_AddParm

   Adds the given string at the end of the current argument list
   ================
 */
void COM_AddParm( char *parm )
{
	if( com_argc == MAX_NUM_ARGVS )
		Com_Error( ERR_FATAL, "COM_AddParm: MAX_NUM_ARGVS" );
	com_argv[com_argc++] = parm;
}

int Com_GlobMatch( const char *pattern, const char *text, const qboolean casecmp )
{
	return glob_match( pattern, text, casecmp );
}

char *_ZoneCopyString( const char *in, const char *filename, int fileline )
{
	char *out;

	out = _Mem_Alloc( zoneMemPool, sizeof( char ) * ( strlen( in ) + 1 ), 0, 0, filename, fileline );
	//out = Mem_ZoneMalloc( sizeof(char) * (strlen(in) + 1) );
	Q_strncpyz( out, in, sizeof( char ) * ( strlen( in ) + 1 ) );

	return out;
}

char *TempCopyString( const char *in )
{
	char *out;

	out = Mem_TempMalloc( sizeof( char ) * ( strlen( in ) + 1 ) );
	Q_strncpyz( out, in, sizeof( char ) * ( strlen( in ) + 1 ) );

	return out;
}

void Info_Print( char *s )
{
	char key[512];
	char value[512];
	char *o;
	int l;

	if( *s == '\\' )
		s++;
	while( *s )
	{
		o = key;
		while( *s && *s != '\\' )
			*o++ = *s++;

		l = o - key;
		if( l < 20 )
		{
			memset( o, ' ', 20-l );
			key[20] = 0;
		}
		else
			*o = 0;
		Com_Printf( "%s", key );

		if( !*s )
		{
			Com_Printf( "MISSING VALUE\n" );
			return;
		}

		o = value;
		s++;
		while( *s && *s != '\\' )
			*o++ = *s++;
		*o = 0;

		if( *s )
			s++;
		Com_Printf( "%s\n", value );
	}
}

//============================================================================

// this is a 16 bit, non-reflected CRC using the polynomial 0x1021
// and the initial and final xor values shown below...  in other words, the
// CCITT standard CRC used by XMODEM

#define CRC_INIT_VALUE	0xffff
#define CRC_XOR_VALUE	0x0000

static unsigned short crctable[256] =
{
	0x0000,	0x1021,	0x2042,	0x3063,	0x4084,	0x50a5,	0x60c6,	0x70e7,
	0x8108,	0x9129,	0xa14a,	0xb16b,	0xc18c,	0xd1ad,	0xe1ce,	0xf1ef,
	0x1231,	0x0210,	0x3273,	0x2252,	0x52b5,	0x4294,	0x72f7,	0x62d6,
	0x9339,	0x8318,	0xb37b,	0xa35a,	0xd3bd,	0xc39c,	0xf3ff,	0xe3de,
	0x2462,	0x3443,	0x0420,	0x1401,	0x64e6,	0x74c7,	0x44a4,	0x5485,
	0xa56a,	0xb54b,	0x8528,	0x9509,	0xe5ee,	0xf5cf,	0xc5ac,	0xd58d,
	0x3653,	0x2672,	0x1611,	0x0630,	0x76d7,	0x66f6,	0x5695,	0x46b4,
	0xb75b,	0xa77a,	0x9719,	0x8738,	0xf7df,	0xe7fe,	0xd79d,	0xc7bc,
	0x48c4,	0x58e5,	0x6886,	0x78a7,	0x0840,	0x1861,	0x2802,	0x3823,
	0xc9cc,	0xd9ed,	0xe98e,	0xf9af,	0x8948,	0x9969,	0xa90a,	0xb92b,
	0x5af5,	0x4ad4,	0x7ab7,	0x6a96,	0x1a71,	0x0a50,	0x3a33,	0x2a12,
	0xdbfd,	0xcbdc,	0xfbbf,	0xeb9e,	0x9b79,	0x8b58,	0xbb3b,	0xab1a,
	0x6ca6,	0x7c87,	0x4ce4,	0x5cc5,	0x2c22,	0x3c03,	0x0c60,	0x1c41,
	0xedae,	0xfd8f,	0xcdec,	0xddcd,	0xad2a,	0xbd0b,	0x8d68,	0x9d49,
	0x7e97,	0x6eb6,	0x5ed5,	0x4ef4,	0x3e13,	0x2e32,	0x1e51,	0x0e70,
	0xff9f,	0xefbe,	0xdfdd,	0xcffc,	0xbf1b,	0xaf3a,	0x9f59,	0x8f78,
	0x9188,	0x81a9,	0xb1ca,	0xa1eb,	0xd10c,	0xc12d,	0xf14e,	0xe16f,
	0x1080,	0x00a1,	0x30c2,	0x20e3,	0x5004,	0x4025,	0x7046,	0x6067,
	0x83b9,	0x9398,	0xa3fb,	0xb3da,	0xc33d,	0xd31c,	0xe37f,	0xf35e,
	0x02b1,	0x1290,	0x22f3,	0x32d2,	0x4235,	0x5214,	0x6277,	0x7256,
	0xb5ea,	0xa5cb,	0x95a8,	0x8589,	0xf56e,	0xe54f,	0xd52c,	0xc50d,
	0x34e2,	0x24c3,	0x14a0,	0x0481,	0x7466,	0x6447,	0x5424,	0x4405,
	0xa7db,	0xb7fa,	0x8799,	0x97b8,	0xe75f,	0xf77e,	0xc71d,	0xd73c,
	0x26d3,	0x36f2,	0x0691,	0x16b0,	0x6657,	0x7676,	0x4615,	0x5634,
	0xd94c,	0xc96d,	0xf90e,	0xe92f,	0x99c8,	0x89e9,	0xb98a,	0xa9ab,
	0x5844,	0x4865,	0x7806,	0x6827,	0x18c0,	0x08e1,	0x3882,	0x28a3,
	0xcb7d,	0xdb5c,	0xeb3f,	0xfb1e,	0x8bf9,	0x9bd8,	0xabbb,	0xbb9a,
	0x4a75,	0x5a54,	0x6a37,	0x7a16,	0x0af1,	0x1ad0,	0x2ab3,	0x3a92,
	0xfd2e,	0xed0f,	0xdd6c,	0xcd4d,	0xbdaa,	0xad8b,	0x9de8,	0x8dc9,
	0x7c26,	0x6c07,	0x5c64,	0x4c45,	0x3ca2,	0x2c83,	0x1ce0,	0x0cc1,
	0xef1f,	0xff3e,	0xcf5d,	0xdf7c,	0xaf9b,	0xbfba,	0x8fd9,	0x9ff8,
	0x6e17,	0x7e36,	0x4e55,	0x5e74,	0x2e93,	0x3eb2,	0x0ed1,	0x1ef0
};

unsigned short CRC_Block( qbyte *start, int count )
{
	unsigned short crc;

	crc = CRC_INIT_VALUE;
	while( count-- )
		crc = ( crc << 8 ) ^ crctable[( crc >> 8 ) ^ *start++];

	return crc ^ CRC_XOR_VALUE;
}

static qbyte chktbl[1024] = {
	0x84, 0x47, 0x51, 0xc1, 0x93, 0x22, 0x21, 0x24, 0x2f, 0x66, 0x60, 0x4d, 0xb0, 0x7c, 0xda,
	0x88, 0x54, 0x15, 0x2b, 0xc6, 0x6c, 0x89, 0xc5, 0x9d, 0x48, 0xee, 0xe6, 0x8a, 0xb5, 0xf4,
	0xcb, 0xfb, 0xf1, 0x0c, 0x2e, 0xa0, 0xd7, 0xc9, 0x1f, 0xd6, 0x06, 0x9a, 0x09, 0x41, 0x54,
	0x67, 0x46, 0xc7, 0x74, 0xe3, 0xc8, 0xb6, 0x5d, 0xa6, 0x36, 0xc4, 0xab, 0x2c, 0x7e, 0x85,
	0xa8, 0xa4, 0xa6, 0x4d, 0x96, 0x19, 0x19, 0x9a, 0xcc, 0xd8, 0xac, 0x39, 0x5e, 0x3c, 0xf2,
	0xf5, 0x5a, 0x72, 0xe5, 0xa9, 0xd1, 0xb3, 0x23, 0x82, 0x6f, 0x29, 0xcb, 0xd1, 0xcc, 0x71,
	0xfb, 0xea, 0x92, 0xeb, 0x1c, 0xca, 0x4c, 0x70, 0xfe, 0x4d, 0xc9, 0x67, 0x43, 0x47, 0x94,
	0xb9, 0x47, 0xbc, 0x3f, 0x01, 0xab, 0x7b, 0xa6, 0xe2, 0x76, 0xef, 0x5a, 0x7a, 0x29, 0x0b,
	0x51, 0x54, 0x67, 0xd8, 0x1c, 0x14, 0x3e, 0x29, 0xec, 0xe9, 0x2d, 0x48, 0x67, 0xff, 0xed,
	0x54, 0x4f, 0x48, 0xc0, 0xaa, 0x61, 0xf7, 0x78, 0x12, 0x03, 0x7a, 0x9e, 0x8b, 0xcf, 0x83,
	0x7b, 0xae, 0xca, 0x7b, 0xd9, 0xe9, 0x53, 0x2a, 0xeb, 0xd2, 0xd8, 0xcd, 0xa3, 0x10, 0x25,
	0x78, 0x5a, 0xb5, 0x23, 0x06, 0x93, 0xb7, 0x84, 0xd2, 0xbd, 0x96, 0x75, 0xa5, 0x5e, 0xcf,
	0x4e, 0xe9, 0x50, 0xa1, 0xe6, 0x9d, 0xb1, 0xe3, 0x85, 0x66, 0x28, 0x4e, 0x43, 0xdc, 0x6e,
	0xbb, 0x33, 0x9e, 0xf3, 0x0d, 0x00, 0xc1, 0xcf, 0x67, 0x34, 0x06, 0x7c, 0x71, 0xe3, 0x63,
	0xb7, 0xb7, 0xdf, 0x92, 0xc4, 0xc2, 0x25, 0x5c, 0xff, 0xc3, 0x6e, 0xfc, 0xaa, 0x1e, 0x2a,
	0x48, 0x11, 0x1c, 0x36, 0x68, 0x78, 0x86, 0x79, 0x30, 0xc3, 0xd6, 0xde, 0xbc, 0x3a, 0x2a,
	0x6d, 0x1e, 0x46, 0xdd, 0xe0, 0x80, 0x1e, 0x44, 0x3b, 0x6f, 0xaf, 0x31, 0xda, 0xa2, 0xbd,
	0x77, 0x06, 0x56, 0xc0, 0xb7, 0x92, 0x4b, 0x37, 0xc0, 0xfc, 0xc2, 0xd5, 0xfb, 0xa8, 0xda,
	0xf5, 0x57, 0xa8, 0x18, 0xc0, 0xdf, 0xe7, 0xaa, 0x2a, 0xe0, 0x7c, 0x6f, 0x77, 0xb1, 0x26,
	0xba, 0xf9, 0x2e, 0x1d, 0x16, 0xcb, 0xb8, 0xa2, 0x44, 0xd5, 0x2f, 0x1a, 0x79, 0x74, 0x87,
	0x4b, 0x00, 0xc9, 0x4a, 0x3a, 0x65, 0x8f, 0xe6, 0x5d, 0xe5, 0x0a, 0x77, 0xd8, 0x1a, 0x14,
	0x41, 0x75, 0xb1, 0xe2, 0x50, 0x2c, 0x93, 0x38, 0x2b, 0x6d, 0xf3, 0xf6, 0xdb, 0x1f, 0xcd,
	0xff, 0x14, 0x70, 0xe7, 0x16, 0xe8, 0x3d, 0xf0, 0xe3, 0xbc, 0x5e, 0xb6, 0x3f, 0xcc, 0x81,
	0x24, 0x67, 0xf3, 0x97, 0x3b, 0xfe, 0x3a, 0x96, 0x85, 0xdf, 0xe4, 0x6e, 0x3c, 0x85, 0x05,
	0x0e, 0xa3, 0x2b, 0x07, 0xc8, 0xbf, 0xe5, 0x13, 0x82, 0x62, 0x08, 0x61, 0x69, 0x4b, 0x47,
	0x62, 0x73, 0x44, 0x64, 0x8e, 0xe2, 0x91, 0xa6, 0x9a, 0xb7, 0xe9, 0x04, 0xb6, 0x54, 0x0c,
	0xc5, 0xa9, 0x47, 0xa6, 0xc9, 0x08, 0xfe, 0x4e, 0xa6, 0xcc, 0x8a, 0x5b, 0x90, 0x6f, 0x2b,
	0x3f, 0xb6, 0x0a, 0x96, 0xc0, 0x78, 0x58, 0x3c, 0x76, 0x6d, 0x94, 0x1a, 0xe4, 0x4e, 0xb8,
	0x38, 0xbb, 0xf5, 0xeb, 0x29, 0xd8, 0xb0, 0xf3, 0x15, 0x1e, 0x99, 0x96, 0x3c, 0x5d, 0x63,
	0xd5, 0xb1, 0xad, 0x52, 0xb8, 0x55, 0x70, 0x75, 0x3e, 0x1a, 0xd5, 0xda, 0xf6, 0x7a, 0x48,
	0x7d, 0x44, 0x41, 0xf9, 0x11, 0xce, 0xd7, 0xca, 0xa5, 0x3d, 0x7a, 0x79, 0x7e, 0x7d, 0x25,
	0x1b, 0x77, 0xbc, 0xf7, 0xc7, 0x0f, 0x84, 0x95, 0x10, 0x92, 0x67, 0x15, 0x11, 0x5a, 0x5e,
	0x41, 0x66, 0x0f, 0x38, 0x03, 0xb2, 0xf1, 0x5d, 0xf8, 0xab, 0xc0, 0x02, 0x76, 0x84, 0x28,
	0xf4, 0x9d, 0x56, 0x46, 0x60, 0x20, 0xdb, 0x68, 0xa7, 0xbb, 0xee, 0xac, 0x15, 0x01, 0x2f,
	0x20, 0x09, 0xdb, 0xc0, 0x16, 0xa1, 0x89, 0xf9, 0x94, 0x59, 0x00, 0xc1, 0x76, 0xbf, 0xc1,
	0x4d, 0x5d, 0x2d, 0xa9, 0x85, 0x2c, 0xd6, 0xd3, 0x14, 0xcc, 0x02, 0xc3, 0xc2, 0xfa, 0x6b,
	0xb7, 0xa6, 0xef, 0xdd, 0x12, 0x26, 0xa4, 0x63, 0xe3, 0x62, 0xbd, 0x56, 0x8a, 0x52, 0x2b,
	0xb9, 0xdf, 0x09, 0xbc, 0x0e, 0x97, 0xa9, 0xb0, 0x82, 0x46, 0x08, 0xd5, 0x1a, 0x8e, 0x1b,
	0xa7, 0x90, 0x98, 0xb9, 0xbb, 0x3c, 0x17, 0x9a, 0xf2, 0x82, 0xba, 0x64, 0x0a, 0x7f, 0xca,
	0x5a, 0x8c, 0x7c, 0xd3, 0x79, 0x09, 0x5b, 0x26, 0xbb, 0xbd, 0x25, 0xdf, 0x3d, 0x6f, 0x9a,
	0x8f, 0xee, 0x21, 0x66, 0xb0, 0x8d, 0x84, 0x4c, 0x91, 0x45, 0xd4, 0x77, 0x4f, 0xb3, 0x8c,
	0xbc, 0xa8, 0x99, 0xaa, 0x19, 0x53, 0x7c, 0x02, 0x87, 0xbb, 0x0b, 0x7c, 0x1a, 0x2d, 0xdf,
	0x48, 0x44, 0x06, 0xd6, 0x7d, 0x0c, 0x2d, 0x35, 0x76, 0xae, 0xc4, 0x5f, 0x71, 0x85, 0x97,
	0xc4, 0x3d, 0xef, 0x52, 0xbe, 0x00, 0xe4, 0xcd, 0x49, 0xd1, 0xd1, 0x1c, 0x3c, 0xd0, 0x1c,
	0x42, 0xaf, 0xd4, 0xbd, 0x58, 0x34, 0x07, 0x32, 0xee, 0xb9, 0xb5, 0xea, 0xff, 0xd7, 0x8c,
	0x0d, 0x2e, 0x2f, 0xaf, 0x87, 0xbb, 0xe6, 0x52, 0x71, 0x22, 0xf5, 0x25, 0x17, 0xa1, 0x82,
	0x04, 0xc2, 0x4a, 0xbd, 0x57, 0xc6, 0xab, 0xc8, 0x35, 0x0c, 0x3c, 0xd9, 0xc2, 0x43, 0xdb,
	0x27, 0x92, 0xcf, 0xb8, 0x25, 0x60, 0xfa, 0x21, 0x3b, 0x04, 0x52, 0xc8, 0x96, 0xba, 0x74,
	0xe3, 0x67, 0x3e, 0x8e, 0x8d, 0x61, 0x90, 0x92, 0x59, 0xb6, 0x1a, 0x1c, 0x5e, 0x21, 0xc1,
	0x65, 0xe5, 0xa6, 0x34, 0x05, 0x6f, 0xc5, 0x60, 0xb1, 0x83, 0xc1, 0xd5, 0xd5, 0xed, 0xd9,
	0xc7, 0x11, 0x7b, 0x49, 0x7a, 0xf9, 0xf9, 0x84, 0x47, 0x9b, 0xe2, 0xa5, 0x82, 0xe0, 0xc2,
	0x88, 0xd0, 0xb2, 0x58, 0x88, 0x7f, 0x45, 0x09, 0x67, 0x74, 0x61, 0xbf, 0xe6, 0x40, 0xe2,
	0x9d, 0xc2, 0x47, 0x05, 0x89, 0xed, 0xcb, 0xbb, 0xb7, 0x27, 0xe7, 0xdc, 0x7a, 0xfd, 0xbf,
	0xa8, 0xd0, 0xaa, 0x10, 0x39, 0x3c, 0x20, 0xf0, 0xd3, 0x6e, 0xb1, 0x72, 0xf8, 0xe6, 0x0f,
	0xef, 0x37, 0xe5, 0x09, 0x33, 0x5a, 0x83, 0x43, 0x80, 0x4f, 0x65, 0x2f, 0x7c, 0x8c, 0x6a,
	0xa0, 0x82, 0x0c, 0xd4, 0xd4, 0xfa, 0x81, 0x60, 0x3d, 0xdf, 0x06, 0xf1, 0x5f, 0x08, 0x0d,
	0x6d, 0x43, 0xf2, 0xe3, 0x11, 0x7d, 0x80, 0x32, 0xc5, 0xfb, 0xc5, 0xd9, 0x27, 0xec, 0xc6,
	0x4e, 0x65, 0x27, 0x76, 0x87, 0xa6, 0xee, 0xee, 0xd7, 0x8b, 0xd1, 0xa0, 0x5c, 0xb0, 0x42,
	0x13, 0x0e, 0x95, 0x4a, 0xf2, 0x06, 0xc6, 0x43, 0x33, 0xf4, 0xc7, 0xf8, 0xe7, 0x1f, 0xdd,
	0xe4, 0x46, 0x4a, 0x70, 0x39, 0x6c, 0xd0, 0xed, 0xca, 0xbe, 0x60, 0x3b, 0xd1, 0x7b, 0x57,
	0x48, 0xe5, 0x3a, 0x79, 0xc1, 0x69, 0x33, 0x53, 0x1b, 0x80, 0xb8, 0x91, 0x7d, 0xb4, 0xf6,
	0x17, 0x1a, 0x1d, 0x5a, 0x32, 0xd6, 0xcc, 0x71, 0x29, 0x3f, 0x28, 0xbb, 0xf3, 0x5e, 0x71,
	0xb8, 0x43, 0xaf, 0xf8, 0xb9, 0x64, 0xef, 0xc4, 0xa5, 0x6c, 0x08, 0x53, 0xc7, 0x00, 0x10,
	0x39, 0x4f, 0xdd, 0xe4, 0xb6, 0x19, 0x27, 0xfb, 0xb8, 0xf5, 0x32, 0x73, 0xe5, 0xcb, 0x32
};

/*
   ====================
   COM_BlockSequenceCRCByte

   For proxy protecting
   ====================
 */
qbyte COM_BlockSequenceCRCByte( qbyte *base, size_t length, int sequence )
{
	qbyte *p;
	unsigned int x, n;
	qbyte chkb[60 + 4];
	unsigned short crc;

	if( sequence < 0 )
		Sys_Error( "sequence < 0, this shouldn't happen" );

	p = chktbl + ( sequence % ( sizeof( chktbl ) - 4 ) );

	if( length > 60 )
		length = 60;
	memcpy( chkb, base, length );

	chkb[length] = p[0];
	chkb[length+1] = p[1];
	chkb[length+2] = p[2];
	chkb[length+3] = p[3];

	length += 4;

	crc = CRC_Block( chkb, length );

	for( x = 0, n = 0; n < length; n++ )
		x += chkb[n];

	crc = ( crc ^ x ) & 0xff;

	return crc;
}


/**
 * Checksum utility function that uses libmd5-rfc in order to produce
 * correct results on little and big endian machines.
 * @author Michael P. Jung
 * @date: 2006-06-24
 */
unsigned int Com_BlockChecksum( void *buffer, size_t length )
{
	md5_byte_t digest[16];
	md5_state_t state;

	md5_init( &state );
	md5_append( &state, (md5_byte_t *)buffer, length );
	md5_finish( &state, digest );

	return
	       ( digest[0] << 24 | digest[1] << 16 | digest[2] << 8 | digest[3] ) ^
	       ( digest[4] << 24 | digest[5] << 16 | digest[6] << 8 | digest[7] ) ^
	       ( digest[8] << 24 | digest[9] << 16 | digest[10] << 8 | digest[11] ) ^
	       ( digest[12] << 24 | digest[13] << 16 | digest[14] << 8 | digest[15] );

}


/*
   ===============
   Com_PageInMemory
   ===============
 */
int paged_total;

void Com_PageInMemory( qbyte *buffer, int size )
{
	int i;

	for( i = size-1; i > 0; i -= 4096 )
		paged_total += buffer[i];
}

//============================================================================

/*
* Com_AddPurePakFile
*/
void Com_AddPakToPureList( purelist_t **purelist, const char *pakname, const unsigned checksum, mempool_t *mempool )
{
	purelist_t *purefile;
	const size_t len = strlen( pakname ) + 1;

	purefile = Mem_Alloc( mempool ? mempool : zoneMemPool, sizeof( purelist_t ) + len );
	purefile->filename = ( char * )(( qbyte * )purefile + sizeof( *purefile ));
	memcpy( purefile->filename, pakname, len );
	purefile->checksum = checksum;
	purefile->next = *purelist;
	*purelist = purefile;
}

/*
* Com_CountPureListFiles
*/
unsigned Com_CountPureListFiles( purelist_t *purelist )
{
	unsigned numpure;
	purelist_t *iter;

	numpure = 0;
	iter = purelist;
	while( iter )
	{
		numpure++;
		iter = iter->next;
	}

	return numpure;
}

/*
* Com_FindPakInPureList
*/
purelist_t *Com_FindPakInPureList( purelist_t *purelist, const char *pakname )
{
	purelist_t *purefile = purelist;

	while( purefile )
	{
		if( !strcmp( purefile->filename, pakname ) )
			break;
		purefile = purefile->next;
	}

	return purefile;
}

/*
* Com_FreePureList
*/
void Com_FreePureList( purelist_t **purelist )
{
	purelist_t *purefile = *purelist;

	while( purefile )
	{
		purelist_t *next = purefile->next;
		Mem_Free( purefile );
		purefile = next;
	}

	*purelist = NULL;
}

//============================================================================

static unsigned int com_CPUFeatures = 0xFFFFFFFF;

static inline int CPU_haveCPUID()
{
	int has_CPUID = 0;
#if defined(__GNUC__) && defined(i386)
	__asm__ (
"        pushfl                      # Get original EFLAGS             \n"
"        popl    %%eax                                                 \n"
"        movl    %%eax,%%ecx                                           \n"
"        xorl    $0x200000,%%eax     # Flip ID bit in EFLAGS           \n"
"        pushl   %%eax               # Save new EFLAGS value on stack  \n"
"        popfl                       # Replace current EFLAGS value    \n"
"        pushfl                      # Get new EFLAGS                  \n"
"        popl    %%eax               # Store new EFLAGS in EAX         \n"
"        xorl    %%ecx,%%eax         # Can not toggle ID bit,          \n"
"        jz      1f                  # Processor=80486                 \n"
"        movl    $1,%0               # We have CPUID support           \n"
"1:                                                                    \n"
	: "=m" (has_CPUID)
	:
	: "%eax", "%ecx"
	);
#elif defined(_MSC_VER) && defined(_M_IX86)
	__asm {
        pushfd                      ; Get original EFLAGS
        pop     eax
        mov     ecx, eax
        xor     eax, 200000h        ; Flip ID bit in EFLAGS
        push    eax                 ; Save new EFLAGS value on stack
        popfd                       ; Replace current EFLAGS value
        pushfd                      ; Get new EFLAGS
        pop     eax                 ; Store new EFLAGS in EAX
        xor     eax, ecx            ; Can not toggle ID bit,
        jz      done                ; Processor=80486
        mov     has_CPUID,1         ; We have CPUID support
done:
	}
#endif
	return has_CPUID;
}

static inline int CPU_getCPUIDFeatures()
{
	int features = 0;
#if defined(__GNUC__) && defined(i386)
	__asm__ (
"        movl    %%ebx,%%edi\n"
"        xorl    %%eax,%%eax         # Set up for CPUID instruction    \n"
"        cpuid                       # Get and save vendor ID          \n"
"        cmpl    $1,%%eax            # Make sure 1 is valid input for CPUID\n"
"        jl      1f                  # We dont have the CPUID instruction\n"
"        xorl    %%eax,%%eax                                           \n"
"        incl    %%eax                                                 \n"
"        cpuid                       # Get family/model/stepping/features\n"
"        movl    %%edx,%0                                              \n"
"1:                                                                    \n"
"        movl    %%edi,%%ebx\n"
	: "=m" (features)
	:
	: "%eax", "%ebx", "%ecx", "%edx", "%edi"
	);
#elif defined(_MSC_VER) && defined(_M_IX86)
	__asm {
        xor     eax, eax            ; Set up for CPUID instruction
        cpuid                       ; Get and save vendor ID
        cmp     eax, 1              ; Make sure 1 is valid input for CPUID
        jl      done                ; We dont have the CPUID instruction
        xor     eax, eax
        inc     eax
        cpuid                       ; Get family/model/stepping/features
        mov     features, edx
done:
	}
#endif
	return features;
}

static inline int CPU_getCPUIDFeaturesExt()
{
	int features = 0;
#if defined(__GNUC__) && defined(i386)
	__asm__ (
"        movl    %%ebx,%%edi\n"
"        movl    $0x80000000,%%eax   # Query for extended functions    \n"
"        cpuid                       # Get extended function limit     \n"
"        cmpl    $0x80000001,%%eax                                     \n"
"        jl      1f                  # Nope, we dont have function 800000001h\n"
"        movl    $0x80000001,%%eax   # Setup extended function 800000001h\n"
"        cpuid                       # and get the information         \n"
"        movl    %%edx,%0                                              \n"
"1:                                                                    \n"
"        movl    %%edi,%%ebx\n"
	: "=m" (features)
	:
	: "%eax", "%ebx", "%ecx", "%edx", "%edi"
	);
#elif defined(_MSC_VER) && defined(_M_IX86)
	__asm {
        mov     eax,80000000h       ; Query for extended functions
        cpuid                       ; Get extended function limit
        cmp     eax,80000001h
        jl      done                ; Nope, we dont have function 800000001h
        mov     eax,80000001h       ; Setup extended function 800000001h
        cpuid                       ; and get the information
        mov     features,edx
done:
	}
#endif
	return features;
}

/*
* COM_CPUFeatures
*
* CPU features detection code, taken from SDL
*/
unsigned int COM_CPUFeatures( void )
{
	if( com_CPUFeatures == 0xFFFFFFFF )
	{
		com_CPUFeatures = 0;

		if( CPU_haveCPUID() )
		{
			int CPUIDFeatures = CPU_getCPUIDFeatures();
			int CPUIDFeaturesExt = CPU_getCPUIDFeaturesExt();

			if( CPUIDFeatures & 0x00000010 )
				com_CPUFeatures |= QCPU_HAS_RDTSC;
			if( CPUIDFeatures & 0x00800000 )
				com_CPUFeatures |= QCPU_HAS_MMX;
			if( CPUIDFeaturesExt & 0x00400000 )
				com_CPUFeatures |= QCPU_HAS_MMXEXT;
			if( CPUIDFeaturesExt & 0x80000000 )
				com_CPUFeatures |= QCPU_HAS_3DNOW;
			if( CPUIDFeaturesExt & 0x40000000 )
				com_CPUFeatures |= QCPU_HAS_3DNOWEXT;
			if( CPUIDFeatures & 0x02000000 )
				com_CPUFeatures |= QCPU_HAS_SSE;
			if( CPUIDFeatures & 0x04000000 )
				com_CPUFeatures |= QCPU_HAS_SSE2;
		}
	}

	return com_CPUFeatures;
}

//========================================================

void Key_Init( void );
void Key_Shutdown( void );
void SCR_EndLoadingPlaque( void );

/*
   =============
   Com_Error_f

   Just throw a fatal error to
   test error shutdown procedures
   =============
 */
#ifndef PUBLIC_BUILD
static void Com_Error_f( void )
{
	Com_Error( ERR_FATAL, "%s", Cmd_Argv( 1 ) );
}
#endif

/*
   =============
   Com_Lag_f
   =============
 */
#ifndef PUBLIC_BUILD
static void Com_Lag_f( void )
{
	int msecs;

	if( Cmd_Argc() != 2 || atoi( Cmd_Argv( 1 ) ) <= 0 )
	{
		Com_Printf( "Usage: %s <milliseconds>\n", Cmd_Argv( 0 ) );
	}

	msecs = atoi( Cmd_Argv( 1 ) );
	Sys_Sleep( msecs );
	Com_Printf( "Lagged %i milliseconds\n", msecs );
}
#endif

/*
   =================
   Q_malloc

   Just like malloc(), but die if allocation fails
   =================
 */
void *Q_malloc( size_t size )
{
	void *buf = malloc( size );

	if( !buf )
		Sys_Error( "Q_malloc: failed on allocation of %i bytes.\n", size );

	return buf;
}

/*
   =================
   Q_realloc

   Just like realloc(), but die if reallocation fails
   =================
 */
void *Q_realloc( void *buf, size_t newsize )
{
	void *newbuf = realloc( buf, newsize );

	if( !newbuf && newsize )
		Sys_Error( "Q_realloc: failed on allocation of %i bytes.\n", newsize );

	return newbuf;
}

/*
   =================
   Q_free
   =================
 */
void Q_free( void *buf )
{
	free( buf );
}

/*
   =================
   Qcommon_InitCommands
   =================
 */
void Qcommon_InitCommands( void )
{
	assert( !commands_intialized );

#ifdef SYS_SYMBOL
	Cmd_AddCommand( "sys_symbol", Com_Sys_Symbol_f );
#endif
#ifndef PUBLIC_BUILD
	Cmd_AddCommand( "error", Com_Error_f );
	Cmd_AddCommand( "lag", Com_Lag_f );
#endif

	Cmd_AddCommand( "irc_connect", Irc_Connect_f );
	Cmd_AddCommand( "irc_disconnect", Irc_Disconnect_f );

	if( dedicated->integer )
		Cmd_AddCommand( "quit", Com_Quit );

	commands_intialized = qtrue;
}

/*
   =================
   Qcommon_ShutdownCommands
   =================
 */
void Qcommon_ShutdownCommands( void )
{
	if( !commands_intialized )
		return;

#ifdef SYS_SYMBOL
	Cmd_RemoveCommand( "sys_symbol" );
#endif
#ifndef PUBLIC_BUILD
	Cmd_RemoveCommand( "error" );
	Cmd_RemoveCommand( "lag" );
#endif

	Cmd_RemoveCommand( "irc_connect" );
	Cmd_RemoveCommand( "irc_disconnect" );

	if( dedicated->integer )
		Cmd_RemoveCommand( "quit" );

	commands_intialized = qfalse;
}

/*
   =================
   Qcommon_Init
   =================
 */
void Qcommon_Init( int argc, char **argv )
{
	if( setjmp( abortframe ) )
		Sys_Error( "Error during initialization: %s", com_errormsg );

	// initialize memory manager
	Memory_Init();

	// prepare enough of the subsystems to handle
	// cvar and command buffer management
	COM_InitArgv( argc, argv );

	Swap_Init();
	Cbuf_Init();

	// initialize cmd/cvar/dynvar tries
	Cmd_PreInit();
	Cvar_PreInit();
	Dynvar_PreInit();

	// create basic commands and cvars
	Cmd_Init();
	Cvar_Init();
	Dynvar_Init();
	dynvars_initialized = qtrue;

	Key_Init();

	// we need to add the early commands twice, because
	// a basepath or cdpath needs to be set before execing
	// config files, but we want other parms to override
	// the settings of the config files
	Cbuf_AddEarlyCommands( qfalse );
	Cbuf_Execute();

	// wsw : aiwa : create dynvars (needs to be completed before .cfg scripts are executed)
	Dynvar_Create( "sys_uptime", qtrue, Com_Sys_Uptime_f, DYNVAR_READONLY );
	Dynvar_Create( "frametick", qfalse, DYNVAR_WRITEONLY, DYNVAR_READONLY );
	Dynvar_Create( "quit", qfalse, DYNVAR_WRITEONLY, DYNVAR_READONLY );
	Dynvar_Create( "irc_connected", qfalse, Irc_GetConnected_f, Irc_SetConnected_f );

	Sys_InitDynvars();
	CL_InitDynvars();

#ifdef TV_SERVER_ONLY
	tv_server = Cvar_Get( "tv_server", "1", CVAR_NOSET );
	Cvar_ForceSet( "tv_server", "1" );
#else
	tv_server = Cvar_Get( "tv_server", "0", CVAR_NOSET );
#endif

#ifdef DEDICATED_ONLY
	dedicated =	    Cvar_Get( "dedicated", "1", CVAR_NOSET );
	Cvar_ForceSet( "dedicated", "1" );
#else
	dedicated =	    Cvar_Get( "dedicated", "0", CVAR_NOSET );
#endif

#ifdef MATCHMAKER
	mm_server =	    Cvar_Get( "mm_server", "1", CVAR_READONLY );
	Cvar_ForceSet( "mm_server", "1" );
#else
	mm_server =	    Cvar_Get( "mm_server", "0", CVAR_READONLY );
#endif

	FS_Init();

	Cbuf_AddText( "exec default.cfg\n" );
	if( !dedicated->integer )
	{
		Cbuf_AddText( "exec config.cfg\n" );
		Cbuf_AddText( "exec autoexec.cfg\n" );
	}
	else if( mm_server->integer )
	{
		Cbuf_AddText( "exec mmaker_autoexec.cfg\n" );
	}
	else if( tv_server->integer )
	{
		Cbuf_AddText( "exec tvserver_autoexec.cfg\n" );
	}
	else
	{
		Cbuf_AddText( "exec dedicated_autoexec.cfg\n" );
	}

	Cbuf_AddEarlyCommands( qtrue );
	Cbuf_Execute();

	//
	// init commands and vars
	//
	Memory_InitCommands();

	Qcommon_InitCommands();

	host_speeds =	    Cvar_Get( "host_speeds", "0", 0 );
	log_stats =	    Cvar_Get( "log_stats", "0", 0 );
	developer =	    Cvar_Get( "developer", "0", 0 );
	timescale =	    Cvar_Get( "timescale", "1.0", CVAR_CHEAT );
	fixedtime =	    Cvar_Get( "fixedtime", "0", CVAR_CHEAT );
	if( tv_server->integer )
		logconsole =	    Cvar_Get( "logconsole", "tvconsole.log", CVAR_ARCHIVE );
	else if( dedicated->integer )
		logconsole =	    Cvar_Get( "logconsole", "wswconsole.log", CVAR_ARCHIVE );
	else
		logconsole =	    Cvar_Get( "logconsole", "", CVAR_ARCHIVE );
	logconsole_append = Cvar_Get( "logconsole_append", "1", CVAR_ARCHIVE );
	logconsole_flush =  Cvar_Get( "logconsole_flush", "0", CVAR_ARCHIVE );
	logconsole_timestamp =	Cvar_Get( "logconsole_timestamp", "0", CVAR_ARCHIVE );

	com_showtrace =	    Cvar_Get( "com_showtrace", "0", 0 );
	com_introPlayed =   Cvar_Get( "com_introPlayed", "1", CVAR_ARCHIVE );

	Cvar_Get( "irc_server", "irc.quakenet.org", CVAR_ARCHIVE );
	Cvar_Get( "irc_port", "6667", CVAR_ARCHIVE );
	Cvar_Get( "irc_nick", APPLICATION "Player", CVAR_ARCHIVE );
	Cvar_Get( "irc_user", APPLICATION "User", CVAR_ARCHIVE );
	Cvar_Get( "irc_password", "", CVAR_ARCHIVE );

	Cvar_Get( "gamename", APPLICATION, CVAR_READONLY );
	versioncvar = Cvar_Get( "version", APP_VERSION_STR " " CPUSTRING " " __DATE__ " " BUILDSTRING, CVAR_SERVERINFO|CVAR_READONLY );
	revisioncvar = Cvar_Get( "revision", SVN_RevString(), CVAR_READONLY );

	Sys_Init();

	NET_Init();
	Netchan_Init();

	CM_Init();

	Com_ScriptModule_Init();

	SV_Init();
	CL_Init();

	MM_Init();

	SCR_EndLoadingPlaque();

	// add + commands from command line
	if( !Cbuf_AddLateCommands() )
	{
		// if the user didn't give any commands, run default action

		if( !dedicated->integer )
		{
			// only play the introduction sequence once
			if( !com_introPlayed->integer )
			{
				Cvar_ForceSet( com_introPlayed->name, "1" );
				Cbuf_AddText( "cinematic intro.roq\n" );
			}
		}
	}
	else
	{
		// the user asked for something explicit
		// so drop the loading plaque
		SCR_EndLoadingPlaque();
	}

	Com_Printf( "\n====== %s Initialized ======\n", APPLICATION );

	Cbuf_Execute();
}

/*
   =================
   Qcommon_Frame
   =================
 */
void Qcommon_Frame( unsigned int realmsec )
{
	static dynvar_t	*frametick = NULL;
	static quint64 fc = 0;
	char *s;
	int time_before = 0, time_between = 0, time_after = 0;
	static unsigned int gamemsec;

	if( setjmp( abortframe ) )
		return; // an ERR_DROP was thrown

	if( log_stats->modified )
	{
		log_stats->modified = qfalse;

		if( log_stats->integer && !log_stats_file )
		{
			if( FS_FOpenFile( "stats.log", &log_stats_file, FS_WRITE ) != -1 )
			{
				FS_Printf( log_stats_file, "entities,dlights,parts,frame time\n" );
			}
			else
			{
				log_stats_file = 0;
			}
		}
		else if( log_stats_file )
		{
			FS_FCloseFile( log_stats_file );
			log_stats_file = 0;
		}
	}

	if( fixedtime->integer > 0 )
	{
		gamemsec = fixedtime->integer;
	}
	else if( timescale->value >= 0 )
	{
		static float extratime = 0.0f;
		gamemsec = extratime + (float)realmsec * timescale->value;
		extratime = ( extratime + (float)realmsec * timescale->value ) - (float)gamemsec;
	}
	else
	{
		gamemsec = realmsec;
	}

	if( com_showtrace->integer )
	{
		Com_Printf( "%4i traces %4i brush traces %4i points\n",
		            c_traces, c_brush_traces, c_pointcontents );
		c_traces = 0;
		c_brush_traces = 0;
		c_pointcontents = 0;
	}

	FS_Frame();

	if( dedicated->integer )
	{
		do
		{
			s = Sys_ConsoleInput();
			if( s )
				Cbuf_AddText( va( "%s\n", s ) );
		}
		while( s );

		Cbuf_Execute();
	}

	// keep the random time dependent
	rand();

	if( host_speeds->integer )
		time_before = Sys_Milliseconds();

	SV_Frame( realmsec, gamemsec );

	if( host_speeds->integer )
		time_between = Sys_Milliseconds();

	CL_Frame( realmsec, gamemsec );

	if( host_speeds->integer )
		time_after = Sys_Milliseconds();

	if( host_speeds->integer )
	{
		int all, sv, gm, cl, rf;

		all = time_after - time_before;
		sv = time_between - time_before;
		cl = time_after - time_between;
		gm = time_after_game - time_before_game;
		rf = time_after_ref - time_before_ref;
		sv -= gm;
		cl -= rf;
		Com_Printf( "all:%3i sv:%3i gm:%3i cl:%3i rf:%3i\n",
		            all, sv, gm, cl, rf );
	}

	MM_Frame( realmsec );

	// wsw : aiwa : generic observer pattern to plug in arbitrary functionality
	if( !frametick )
		frametick = Dynvar_Lookup( "frametick" );
	Dynvar_CallListeners( frametick, &fc );
	++fc;
}

/*
   =================
   Qcommon_Shutdown
   =================
 */
void Qcommon_Shutdown( void )
{
	static qboolean isdown = qfalse;

	if( isdown )
	{
		printf( "Recursive shutdown\n" );
		return;
	}
	isdown = qtrue;

	Com_ScriptModule_Shutdown();
	CM_Shutdown();
	Netchan_Shutdown();
	NET_Shutdown();
	Key_Shutdown();

	Qcommon_ShutdownCommands();
	Memory_ShutdownCommands();

	if( log_stats_file )
	{
		FS_FCloseFile( log_stats_file );
		log_stats_file = 0;
	}
	if( log_file )
	{
		FS_FCloseFile( log_file );
		log_file = 0;
	}
	logconsole = NULL;
	FS_Shutdown();

	Dynvar_Shutdown();
	dynvars_initialized = qfalse;
	Cvar_Shutdown();
	Cmd_Shutdown();
	Cbuf_Shutdown();
	Memory_Shutdown();
}
