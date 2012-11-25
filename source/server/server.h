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
// server.h

#include "../qcommon/qcommon.h"
#include "../game/g_public.h"

//=============================================================================

#define	MAX_MASTERS 8               // max recipients for heartbeat packets
#define	HEARTBEAT_SECONDS   300

typedef enum
{
	ss_dead,        // no map loaded
	ss_loading,     // spawning level edicts
	ss_game         // actively running
} server_state_t;

// some commands are only valid before the server has finished
// initializing (precache commands, static sounds / objects, etc)

typedef struct ginfo_s
{
	struct edict_s *edicts;
	struct client_s *clients;

	int edict_size;
	int num_edicts;         // current number, <= max_edicts
	int max_edicts;
	int max_clients;		// <= sv_maxclients, <= max_edicts
} ginfo_t;

#define MAX_FRAME_SOUNDS 256
typedef struct
{
	server_state_t state;       // precache commands are only valid during load

	unsigned nextSnapTime;              // always sv.framenum * svc.snapFrameTime msec
	unsigned framenum;

	char mapname[MAX_QPATH];               // map name

	char configstrings[MAX_CONFIGSTRINGS][MAX_CONFIGSTRING_CHARS];
	entity_state_t baselines[MAX_EDICTS];
	int num_mv_clients;     // current number, <= sv_maxmvclients

	//
	// global variables shared between game and server
	//
	ginfo_t gi;
} server_t;

struct gclient_s
{
	player_state_t ps;  // communicated by server to clients
	client_shared_t	r;
};

struct edict_s
{
	entity_state_t s;
	entity_shared_t	r;
};

#define EDICT_NUM( n ) ( (edict_t *)( (qbyte *)sv.gi.edicts + sv.gi.edict_size*( n ) ) )
#define NUM_FOR_EDICT( e ) ( ( (qbyte *)( e )-(qbyte *)sv.gi.edicts ) / sv.gi.edict_size )

typedef struct
{
	qboolean allentities;
	qboolean multipov;
	qboolean relay;
	int clientarea;
	int numareas;
	int areabytes;
	qbyte *areabits;					// portalarea visibility bits
	int numplayers;
	int ps_size;
	player_state_t *ps;                 // [numplayers]
	int num_entities;
	int first_entity;                   // into the circular sv.client_entities[]
	unsigned int sentTimeStamp;         // time at what this frame snap was sent to the clients
	unsigned int UcmdExecuted;
	game_state_t gameState;
} client_snapshot_t;

typedef struct
{
	char *name;
	qbyte *data;            // file being downloaded
	int size;               // total bytes (can't use EOF because of paks)
	unsigned int timeout;   // so we can free the file being downloaded
	                        // if client omits sending success or failure message
} client_download_t;

typedef struct
{
	unsigned int framenum;
	char command[MAX_STRING_CHARS];
} game_command_t;

#define	LATENCY_COUNTS	16
#define	RATE_MESSAGES	25  // wsw : jal : was 10: I think it must fit sv_pps, I have to calculate it

typedef struct client_s
{
	sv_client_state_t state;

	char userinfo[MAX_INFO_STRING];             // name, etc

	qboolean reliable;                  // no need for acks, connection is reliable
	qboolean mv;                        // send multiview data to the client
	qboolean individual_socket;         // client has it's own socket that has to be checked separately

	socket_t socket;

	char reliableCommands[MAX_RELIABLE_COMMANDS][MAX_STRING_CHARS];
	unsigned int reliableSequence;      // last added reliable message, not necesarily sent or acknowledged yet
	unsigned int reliableAcknowledge;   // last acknowledged reliable message
	unsigned int reliableSent;          // last sent reliable message, not necesarily acknowledged yet

	game_command_t gameCommands[MAX_RELIABLE_COMMANDS];
	int gameCommandCurrent;             // position in the gameCommands table

	unsigned int clientCommandExecuted; // last client-command we received

	unsigned int UcmdTime;
	unsigned int UcmdExecuted;          // last client-command we executed
	unsigned int UcmdReceived;          // last client-command we received
	usercmd_t ucmds[CMD_BACKUP];        // each message will send several old cmds

	unsigned int lastPacketSentTime;    // time when we sent the last message to this client
	unsigned int lastPacketReceivedTime; // time when we received the last message from this client
	unsigned lastconnect;

	int lastframe;                  // used for delta compression etc.
	qboolean nodelta;               // send one non delta compressed frame trough
	int nodelta_frame;              // when we get confirmation of this frame, the non-delta frame is trough
	unsigned int lastSentFrameNum;  // for knowing which was last frame we sent

	int frame_latency[LATENCY_COUNTS];
	int ping;
#ifndef RATEKILLED
	//int				message_size[RATE_MESSAGES];	// used to rate drop packets
	int rate;
	int suppressCount;              // number of messages rate suppressed
#endif
	edict_t	*edict;                     // EDICT_NUM(clientnum+1)
	char name[MAX_INFO_VALUE];          // extracted from userinfo, high bits masked

	client_snapshot_t snapShots[UPDATE_BACKUP]; // updates can be delta'd from here

	client_download_t download;

	int challenge;                  // challenge of this user, randomly generated

	netchan_t netchan;

	qboolean tvclient;

	int mm_session;
	unsigned int mm_ticket;
	char mm_login[MAX_INFO_VALUE];
} client_t;

// a client can leave the server in one of four ways:
// dropping properly by quiting or disconnecting
// timing out if no valid messages are received for timeout.value seconds
// getting kicked off by the server operator
// a program error, like an overflowed reliable buffer

//=============================================================================

// MAX_CHALLENGES is made large to prevent a denial
// of service attack that could cycle all of them
// out before legitimate users connected
#define	MAX_CHALLENGES	1024

// MAX_SNAP_ENTITIES is the guess of what we consider maximum amount of entities
// to be sent to a client into a snap. It's used for finding size of the backup storage
#define MAX_SNAP_ENTITIES 64

typedef struct
{
	netadr_t adr;
	int challenge;
	int time;
} challenge_t;

// for server side demo recording
typedef struct
{
	int file;
	char *filename;
	char *tempname;
	time_t localtime;
	unsigned int basetime, duration;
	client_t client;                // special client for writing the messages
	char meta_data[SNAP_MAX_DEMO_META_DATA_SIZE];
	size_t meta_data_realsize;
} server_static_demo_t;

typedef server_static_demo_t demorec_t;

#ifdef TCP_SUPPORT
#define MAX_INCOMING_CONNECTIONS 256
typedef struct
{
	qboolean active;
	unsigned int time;      // for timeout
	socket_t socket;
	netadr_t address;
} incoming_t;
#endif

#define MAX_MOTD_LEN 1024

typedef struct client_entities_s
{
	unsigned num_entities;				// maxclients->integer*UPDATE_BACKUP*MAX_PACKET_ENTITIES
	unsigned next_entities;				// next client_entity to use
	entity_state_t *entities;			// [num_entities]
} client_entities_t;

typedef struct fatvis_s
{
	vec_t *skyorg;
	qbyte pvs[MAX_MAP_LEAFS/8];
	qbyte phs[MAX_MAP_LEAFS/8];
} fatvis_t;

typedef struct
{
	qboolean initialized;               // sv_init has completed
	unsigned int realtime;                  // real world time - always increasing, no clamping, etc
	unsigned int gametime;                  // game world time - always increasing, no clamping, etc

	socket_t socket_udp;
	socket_t socket_udp6;
	socket_t socket_loopback;
#ifdef TCP_SUPPORT
	socket_t socket_tcp;
#endif

	char mapcmd[MAX_TOKEN_CHARS];       // ie: *intro.cin+base

	int spawncount;                     // incremented each server start
	                                    // used to check late spawns

	client_t *clients;                  // [sv_maxclients->integer];
	client_entities_t client_entities;

	challenge_t challenges[MAX_CHALLENGES]; // to prevent invalid IPs from connecting
#ifdef TCP_SUPPORT
	incoming_t incoming[MAX_INCOMING_CONNECTIONS]; // holds socket while tcp client is connecting
#endif

	server_static_demo_t demo;

	purelist_t *purelist;				// pure file support

	cmodel_state_t *cms;                // passed to CM-functions

	fatvis_t fatvis;

	char *motd;
} server_static_t;

typedef struct
{
	int	last_heartbeat;
	int last_mmheartbeat;
	unsigned int last_activity;
	unsigned int snapFrameTime;		// msecs between server packets
	unsigned int gameFrameTime;		// msecs between game code executions
	qboolean autostarted;
} server_constant_t;

//=============================================================================

// shared message buffer to be used for occasional messages
extern msg_t tmpMessage;
extern qbyte tmpMessageData[MAX_MSGLEN];

extern mempool_t *sv_mempool;

extern server_constant_t svc;              // constant server info (trully persistant since sv_init)
extern server_static_t svs;                // persistant server info
extern server_t	sv;                 // local server

extern cvar_t *sv_ip;
extern cvar_t *sv_port;
extern cvar_t *sv_ip6;
extern cvar_t *sv_port6;
extern cvar_t *sv_tcp;

extern cvar_t *sv_skilllevel;
extern cvar_t *sv_maxclients;
extern cvar_t *sv_maxmvclients;

extern cvar_t *sv_enforcetime;
extern cvar_t *sv_showRcon;
extern cvar_t *sv_showChallenge;
extern cvar_t *sv_showInfoQueries;
extern cvar_t *sv_highchars;

//wsw : jal
extern cvar_t *sv_maxrate;
extern cvar_t *sv_compresspackets;
extern cvar_t *sv_public;         // should heartbeats be sent

// wsw : debug netcode
extern cvar_t *sv_debug_serverCmd;

extern cvar_t *sv_uploads;
extern cvar_t *sv_uploads_from_server;
extern cvar_t *sv_uploads_baseurl;
extern cvar_t *sv_uploads_demos_baseurl;

extern cvar_t *sv_pure;
extern cvar_t *sv_pure_forcemodulepk3;

// MOTD: 0=disable MOTD
//       1=Enable MOTD
extern cvar_t *sv_MOTD;
// File to read MOTD from
extern cvar_t *sv_MOTDFile;
// String to display
extern cvar_t *sv_MOTDString;
extern cvar_t *sv_lastAutoUpdate;
extern cvar_t *sv_defaultmap;

extern cvar_t *sv_demodir;

extern cvar_t *sv_mm_authkey;
extern cvar_t *sv_mm_loginonly;
extern cvar_t *sv_mm_debug_reportbots;

//===========================================================

//
// sv_main.c
//
int SV_ModelIndex( const char *name );
int SV_SoundIndex( const char *name );
int SV_ImageIndex( const char *name );
int SV_SkinIndex( const char *name );

void SV_WriteClientdataToMessage( client_t *client, msg_t *msg );

void SV_AutoUpdateFromWeb( qboolean checkOnly );
void SV_InitOperatorCommands( void );
void SV_ShutdownOperatorCommands( void );

void SV_SendServerinfo( client_t *client );
void SV_UserinfoChanged( client_t *cl );

void SV_MasterHeartbeat( void );

void SVC_MasterInfoResponse( const socket_t *socket, const netadr_t *address );
int SVC_FakeConnect( char *fakeUserinfo, char *fakeSocketType, char *fakeIP );

void SV_UpdateActivity( void );

//
// sv_oob.c
//
void SV_ConnectionlessPacket( const socket_t *socket, const netadr_t *address, msg_t *msg );
void SV_InitMaster( void );

//
// sv_init.c
//
void SV_InitGame( void );
void SV_Map( const char *level, qboolean devmap );
void SV_SetServerConfigStrings( void );

void SV_AddPureFile( const char *filename );
void SV_PureList_f( void );

//
// sv_phys.c
//
void SV_PrepWorldFrame( void );

//
// sv_send.c
//
qboolean SV_Netchan_Transmit( netchan_t *netchan, msg_t *msg );
void SV_AddServerCommand( client_t *client, const char *cmd );
void SV_SendServerCommand( client_t *cl, const char *format, ... );
void SV_AddGameCommand( client_t *client, const char *cmd );
void SV_AddReliableCommandsToMessage( client_t *client, msg_t *msg );
qboolean SV_SendClientsFragments( void );
void SV_InitClientMessage( client_t *client, msg_t *msg, qbyte *data, size_t size );
qboolean SV_SendMessageToClient( client_t *client, msg_t *msg );
void SV_ResetClientFrameCounters( void );

typedef enum { RD_NONE, RD_PACKET } redirect_t;

// destination class for SV_multicast
typedef enum
{
	MULTICAST_ALL,
	MULTICAST_PHS,
	MULTICAST_PVS
} multicast_t;

#define	SV_OUTPUTBUF_LENGTH ( MAX_MSGLEN - 16 )

extern char sv_outputbuf[SV_OUTPUTBUF_LENGTH];

typedef struct
{
	const socket_t *socket;
	const netadr_t *address;
} flush_params_t;

void SV_FlushRedirect( int sv_redirected, char *outputbuf, const void *extra );
void SV_SendClientMessages( void );

void SV_Multicast( vec3_t origin, multicast_t to );
void SV_BroadcastCommand( const char *format, ... );

//
// sv_client.c
//
void SV_ParseClientMessage( client_t *client, msg_t *msg );
qboolean SV_ClientConnect( const socket_t *socket, const netadr_t *address, client_t *client, char *userinfo,
                           int game_port, int challenge, qboolean fakeClient, qboolean tvClient,
                           unsigned int ticket_id, int session_id );
void SV_DropClient( client_t *drop, int type, const char *format, ... );
void SV_ExecuteClientThinks( int clientNum );
void SV_ClientResetCommandBuffers( client_t *client );

//
// sv_mv.c
//
qboolean SV_Multiview_CreateStartMessages( qboolean ( *callback )( msg_t *msg, void *extra ), void *extra );


//
// sv_ccmds.c
//
void SV_Status_f( void );

//
// sv_ents.c
//
void SV_WriteFrameSnapToClient( client_t *client, msg_t *msg );
void SV_BuildClientFrameSnap( client_t *client );


void SV_Error( char *error, ... );

//
// sv_game.c
//
extern game_export_t *ge;

void SV_InitGameProgs( void );
void SV_ShutdownGameProgs( void );
void SV_InitEdict( edict_t *e );


//============================================================

//
// sv_demos.c
//
void SV_Demo_WriteSnap( void );
void SV_Demo_Start_f( void );
void SV_Demo_Stop_f( void );
void SV_Demo_Cancel_f( void );
void SV_Demo_Purge_f( void );

void SV_DemoList_f( client_t *client );
void SV_DemoGet_f( client_t *client );

#define SV_SetDemoMetaKeyValue(k,v) svs.demo.meta_data_realsize = SNAP_SetDemoMetaKeyValue(svs.demo.meta_data, sizeof(svs.demo.meta_data), svs.demo.meta_data_realsize, k, v)

qboolean SV_IsDemoDownloadRequest( const char *request );

//
// sv_motd.c
//
void SV_MOTD_Update( void );
void SV_MOTD_Get_f( client_t *client );

//
// sv_mm.c
//
#ifdef TCP_SUPPORT
void SV_MM_SetConnection( incoming_t *connection );
#endif

void SV_MM_Init( void );
void SV_MM_Shutdown( qboolean logout );
void SV_MM_Frame( void );
qboolean SV_MM_Initialized( void );

int SV_MM_ClientConnect( const netadr_t *address, char *userinfo, unsigned int ticket, int session );
void SV_MM_ClientDisconnect( client_t *client );

int SV_MM_GenerateLocalSession( void );

// match report
#include "../matchmaker/mm_common.h"
void SV_MM_SendQuery( stat_query_t *query );
void SV_MM_GameState( qboolean state );
void SV_MM_GetMatchUUID( void (*callback_fn)( const char *uuid ) );
