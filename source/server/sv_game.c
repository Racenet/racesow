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
// sv_game.c -- interface to the game dll

#include "server.h"

game_export_t *ge;

EXTERN_API_FUNC void *GetGameAPI( void * );

mempool_t *sv_gameprogspool;
static void *module_handle;

//======================================================================

// PF versions of the CM functions passed to the game module
// they only add svs.cms as the first parameter

//======================================================================

static inline int PF_CM_TransformedPointContents( vec3_t p, struct cmodel_s *cmodel, vec3_t origin, vec3_t angles ) {
	return CM_TransformedPointContents( svs.cms, p, cmodel, origin, angles );
}

static inline void PF_CM_TransformedBoxTrace( trace_t *tr, vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs,
	struct cmodel_s *cmodel, int brushmask, vec3_t origin, vec3_t angles ) {
	CM_TransformedBoxTrace( svs.cms, tr, start, end, mins, maxs, cmodel, brushmask, origin, angles );
}

static inline void PF_CM_RoundUpToHullSize( vec3_t mins, vec3_t maxs, struct cmodel_s *cmodel ) {
	CM_RoundUpToHullSize( svs.cms, mins, maxs, cmodel );
}

static inline int PF_CM_NumInlineModels( void ) {
	return CM_NumInlineModels( svs.cms );
}

static inline struct cmodel_s *PF_CM_InlineModel( int num ) {
	return CM_InlineModel( svs.cms, num );
}

static inline void PF_CM_InlineModelBounds( struct cmodel_s *cmodel, vec3_t mins, vec3_t maxs ) {
	CM_InlineModelBounds( svs.cms, cmodel, mins, maxs );
}

static inline struct cmodel_s *PF_CM_ModelForBBox( vec3_t mins, vec3_t maxs ) {
	return CM_ModelForBBox( svs.cms, mins, maxs );
}

static inline qboolean PF_CM_AreasConnected( int area1, int area2 ) {
	return CM_AreasConnected( svs.cms, area1, area2 );
}

static inline void PF_CM_SetAreaPortalState( int area, int otherarea, qboolean open ) {
	CM_SetAreaPortalState( svs.cms, area, otherarea, open );
}

static inline int PF_CM_BoxLeafnums( vec3_t mins, vec3_t maxs, int *list, int listsize, int *topnode ) {
	return CM_BoxLeafnums( svs.cms, mins, maxs, list, listsize, topnode );
}

static inline int PF_CM_LeafCluster( int leafnum ) {
	return CM_LeafCluster( svs.cms, leafnum );
}

static inline int PF_CM_LeafArea( int leafnum ) {
	return CM_LeafArea( svs.cms, leafnum );
}

//======================================================================

/*
* PF_DropClient
*/
static void PF_DropClient( edict_t *ent, int type, char *message )
{
	int p;
	client_t *drop;

	if( !ent )
		return;

	p = NUM_FOR_EDICT( ent );
	if( p < 1 || p > sv_maxclients->integer )
		return;

	drop = svs.clients + ( p-1 );
	if( message )
		SV_DropClient( drop, type, "%s", message );
	else
		SV_DropClient( drop, type, NULL );
}

/*
* PF_GetClientState
*
* Game code asks for the state of this client
*/
static int PF_GetClientState( int numClient )
{
	if( numClient < 0 || numClient >= sv_maxclients->integer )
		return -1;
	return svs.clients[numClient].state;
}

/*
* PF_GameCmd
*
* Sends the server command to clients.
* If ent is NULL the command will be sent to all connected clients
*/
static void PF_GameCmd( edict_t *ent, const char *cmd )
{
	int i;
	client_t *client;

	if( !cmd || !cmd[0] )
		return;

	if( !ent )
	{
		for( i = 0, client = svs.clients; i < sv_maxclients->integer; i++, client++ )
		{
			if( client->state < CS_SPAWNED )
				continue;
			SV_AddGameCommand( client, cmd );
		}
	}
	else
	{
		i = NUM_FOR_EDICT( ent );
		if( i < 1 || i > sv_maxclients->integer )
			return;

		client = svs.clients + ( i - 1 );
		if( client->state < CS_SPAWNED )
			return;

		SV_AddGameCommand( client, cmd );
	}
}

/*
* PF_dprint
*
* Debug print to server console
*/
static void PF_dprint( const char *msg )
{
	char copy[MAX_PRINTMSG], *end = copy + sizeof( copy );
	char *out = copy;
	const char *in = msg;

	if( !msg )
		return;

	// don't allow control chars except for \n
	if( sv_highchars && sv_highchars->integer )
	{
		for( ; *in && out < end - 1; in++ )
			if ( ( unsigned char )*in >= ' ' || *in == '\n' )
				*out++ = *in;
	}
	else
	{	// also convert highchars to ascii by stripping high bit
		for( ; *in && out < end - 1; in++ )
			if ( ( signed char )*in >= ' ' || *in == '\n' )
				*out++ = *in;
			else if( ( unsigned char )*in > 128 ) // 128 is not allowed
				*out++ = *in & 127;
			else if( ( unsigned char )*in == 128 )
				*out++ = '?';
	}
	*out = '\0';

	Com_Printf( "%s", copy );
}

/*
* PF_error
*
* Abort the server with a game error
*/
static void PF_error( const char *msg )
{
	int i;
	char copy[MAX_PRINTMSG];

	if( !msg )
	{
		Com_Error( ERR_DROP, "Game Error: unknown error" );
		return;
	}

	// mask off high bits and colored strings
	for( i = 0; i < sizeof( copy )-1 && msg[i]; i++ )
		copy[i] = msg[i]&127;
	copy[i] = 0;

	Com_Error( ERR_DROP, "Game Error: %s", copy );
}

/*
* PF_Configstring
*/
static void PF_ConfigString( int index, const char *val )
{
	size_t len;

	if( !val )
		return;

	if( index < 0 || index >= MAX_CONFIGSTRINGS )
		Com_Error( ERR_DROP, "configstring: bad index %i", index );

	if( index < SERVER_PROTECTED_CONFIGSTRINGS )
	{
		Com_Printf( "WARNING: 'PF_Configstring', configstring %i is server protected\n", index );
		return;
	}

	len = strlen( val );
	if( len >= sizeof( sv.configstrings[0] ) )
	{
		Com_Printf( "WARNING: 'PF_Configstring', configstring %i overflowed (%i)\n", index, strlen( val ) );
		len = sizeof( sv.configstrings[0] ) - 1;
	}

	if( !COM_ValidateConfigstring( val ) )
	{
		Com_Printf( "WARNING: 'PF_Configstring' invalid configstring %i: %s\n", index, val );
		return;
	}

	// ignore if no changes
	if( !strncmp( sv.configstrings[index], val, len ) && sv.configstrings[index][len] == '\0' )
		return;

	// change the string in sv
	Q_strncpyz( sv.configstrings[index], val, sizeof( sv.configstrings[index] ) );

	if( sv.state != ss_loading )
	{
		// We have to manually broadcast this one.
		int i;
		client_t *client;

		for( i = 0, client = svs.clients; i < sv_maxclients->integer; i++, client++ )
		{
			if( client->state < CS_CONNECTED )
				continue;
			SV_SendServerCommand( client, "cs %i \"%s\"", index, val );
		}

		if( svs.demo.file )
			SV_AddServerCommand( &svs.demo.client, va( "cs %i \"%s\"", index, val ) );
	}
}

static const char *PF_GetConfigString( int index )
{
	if( index < 0 || index >= MAX_CONFIGSTRINGS )
		return NULL;

	return sv.configstrings[ index ];
}

/*
* PF_PureSound
*/
static void PF_PureSound( const char *name )
{
	const char *extension;
	char tempname[MAX_CONFIGSTRING_CHARS];

	if( sv.state != ss_loading )
		return;

	if( !name || !name[0] || strlen( name ) >= MAX_CONFIGSTRING_CHARS )
		return;

	Q_strncpyz( tempname, name, sizeof( tempname ) );

	if( !COM_FileExtension( tempname ) )
	{
		extension = FS_FirstExtension( tempname, SOUND_EXTENSIONS, NUM_SOUND_EXTENSIONS );
		if( !extension )
			return;

		COM_ReplaceExtension( tempname, extension, sizeof( tempname ) );
	}

	SV_AddPureFile( tempname );
}

/*
* SV_AddPureShader
*
* FIXME: For now we don't parse shaders, but simply assume that it uses the same name .tga or .jpg
*/
static void SV_AddPureShader( const char *name )
{
	const char *extension;
	char tempname[MAX_CONFIGSTRING_CHARS];

	if( !name || !name[0] )
		return;

	assert( name && name[0] && strlen( name ) < MAX_CONFIGSTRING_CHARS );

	if( !Q_strnicmp( name, "textures/common/", strlen( "textures/common/" ) ) )
		return;

	Q_strncpyz( tempname, name, sizeof( tempname ) );

	if( !COM_FileExtension( tempname ) )
	{
		extension = FS_FirstExtension( tempname, IMAGE_EXTENSIONS, NUM_IMAGE_EXTENSIONS );
		if( !extension )
			return;

		COM_ReplaceExtension( tempname, extension, sizeof( tempname ) );
	}

	SV_AddPureFile( tempname );
}

/*
* SV_AddPureBSP
*/
static void SV_AddPureBSP( void )
{
	int i;
	const char *shader;

	SV_AddPureFile( sv.configstrings[CS_WORLDMODEL] );
	for( i = 0; ( shader = CM_ShaderrefName( svs.cms, i ) ); i++ )
		SV_AddPureShader( shader );
}

/*
* PF_PureModel
*/
static void PF_PureModel( const char *name )
{
	if( sv.state != ss_loading )
		return;
	if( !name || !name[0] || strlen( name ) >= MAX_CONFIGSTRING_CHARS )
		return;

	if( name[0] == '*' )
	{                       // inline model
		if( !strcmp( name, "*0" ) )
			SV_AddPureBSP(); // world
	}
	else
	{
		SV_AddPureFile( name );
	}
}

/*
* PF_inVisSet
*
* Also checks portalareas so that doors block sight
*/
static qboolean PF_inVisSet( vec3_t p1, vec3_t p2, qbyte *( *vis )( cmodel_state_t *, int ) )
{
	int leafnum;
	int cluster;
	int area1, area2;
	qbyte *mask;

	leafnum = CM_PointLeafnum( svs.cms, p1 );
	cluster = CM_LeafCluster( svs.cms, leafnum );
	area1 = CM_LeafArea( svs.cms, leafnum );
	mask = vis( svs.cms, cluster );

	leafnum = CM_PointLeafnum( svs.cms, p2 );
	cluster = CM_LeafCluster( svs.cms, leafnum );
	area2 = CM_LeafArea( svs.cms, leafnum );

	if( ( !( mask[cluster>>3] & ( 1<<( cluster&7 ) ) ) ) )
		return qfalse;

	if( !CM_AreasConnected( svs.cms, area1, area2 ) )
		return qfalse; // a door blocks sight

	return qtrue;
}

/*
* PF_inPVS
*/
static qboolean PF_inPVS( vec3_t p1, vec3_t p2 ) {
	return PF_inVisSet( p1, p2, CM_ClusterPVS );
}

/*
* PF_inPHS
*/
static qboolean PF_inPHS( vec3_t p1, vec3_t p2 ) {
	return PF_inVisSet( p1, p2, CM_ClusterPHS );
}

/*
* PF_MemAlloc
*/
static void *PF_MemAlloc( size_t size, const char *filename, int fileline ) {
	return _Mem_Alloc( sv_gameprogspool, size, MEMPOOL_GAMEPROGS, 0, filename, fileline );
}

/*
* PF_MemFree
*/
static void PF_MemFree( void *data, const char *filename, int fileline ) {
	_Mem_Free( data, MEMPOOL_GAMEPROGS, 0, filename, fileline );
}

//==============================================

/*
* SV_ShutdownGameProgs
*
* Called when either the entire server is being killed, or
* it is changing to a different game directory.
*/
void SV_ShutdownGameProgs( void )
{
	if( !ge )
		return;

	ge->Shutdown();
	Mem_FreePool( &sv_gameprogspool );
	Com_UnloadGameLibrary( &module_handle );
	ge = NULL;
}

/*
* SV_LocateEntities
*/
static void SV_LocateEntities( struct edict_s *edicts, int edict_size, int num_edicts, int max_edicts )
{
	if( !edicts || edict_size < sizeof( entity_shared_t ) )
		Com_Error( ERR_DROP, "SV_LocateEntities: bad edicts" );

	sv.gi.edicts = edicts;
	sv.gi.clients = svs.clients;
	sv.gi.edict_size = edict_size;
	sv.gi.num_edicts = num_edicts;
	sv.gi.max_edicts = max_edicts;
	sv.gi.max_clients = min( num_edicts, sv_maxclients->integer );
}

/*
* SV_InitGameProgsImportStruct
*/
#define SV_InitGameProgsImportStruct( import ) \
	( \
	import.Print = PF_dprint, \
	import.Error = PF_error, \
	import.GameCmd = PF_GameCmd, \
	\
	import.inPVS = PF_inPVS, \
	import.inPHS = PF_inPHS, \
	\
	import.CM_TransformedPointContents = PF_CM_TransformedPointContents, \
	import.CM_TransformedBoxTrace = PF_CM_TransformedBoxTrace, \
	import.CM_RoundUpToHullSize = PF_CM_RoundUpToHullSize, \
	import.CM_NumInlineModels = PF_CM_NumInlineModels, \
	import.CM_InlineModel = PF_CM_InlineModel, \
	import.CM_InlineModelBounds = PF_CM_InlineModelBounds, \
	import.CM_ModelForBBox = PF_CM_ModelForBBox, \
	import.CM_AreasConnected = PF_CM_AreasConnected, \
	import.CM_SetAreaPortalState = PF_CM_SetAreaPortalState, \
	import.CM_BoxLeafnums = PF_CM_BoxLeafnums, \
	import.CM_LeafCluster = PF_CM_LeafCluster, \
	import.CM_LeafArea = PF_CM_LeafArea, \
	\
	import.Milliseconds = Sys_Milliseconds, \
	\
	import.ModelIndex = SV_ModelIndex, \
	import.SoundIndex = SV_SoundIndex, \
	import.ImageIndex = SV_ImageIndex, \
	import.SkinIndex = SV_SkinIndex, \
	\
	import.ConfigString = PF_ConfigString, \
	import.GetConfigString = PF_GetConfigString, \
	import.PureSound = PF_PureSound, \
	import.PureModel = PF_PureModel, \
	\
	import.FS_FOpenFile = FS_FOpenFile, \
	import.FS_Read = FS_Read, \
	import.FS_Write = FS_Write, \
	import.FS_Print = FS_Print, \
	import.FS_Tell = FS_Tell, \
	import.FS_Seek = FS_Seek, \
	import.FS_Eof = FS_Eof, \
	import.FS_Flush = FS_Flush, \
	import.FS_FCloseFile = FS_FCloseFile, \
	import.FS_RemoveFile = FS_RemoveFile, \
	import.FS_GetFileList = FS_GetFileList, \
	import.FS_FirstExtension = FS_FirstExtension, \
	\
	import.Mem_Alloc = PF_MemAlloc, \
	import.Mem_Free = PF_MemFree, \
	\
	import.Dynvar_Create = Dynvar_Create, \
	import.Dynvar_Destroy = Dynvar_Destroy, \
	import.Dynvar_Lookup = Dynvar_Lookup, \
	import.Dynvar_GetName = Dynvar_GetName, \
	import.Dynvar_GetValue = Dynvar_GetValue, \
	import.Dynvar_SetValue = Dynvar_SetValue, \
	import.Dynvar_AddListener = Dynvar_AddListener, \
	import.Dynvar_RemoveListener = Dynvar_RemoveListener, \
	\
	import.Cvar_Get = Cvar_Get, \
	import.Cvar_Set = Cvar_Set, \
	import.Cvar_SetValue = Cvar_SetValue, \
	import.Cvar_ForceSet = Cvar_ForceSet, \
	import.Cvar_Value = Cvar_Value, \
	import.Cvar_String = Cvar_String, \
	\
	import.Cmd_Argc = Cmd_Argc, \
	import.Cmd_Argv = Cmd_Argv, \
	import.Cmd_Args = Cmd_Args, \
	import.Cmd_AddCommand = Cmd_AddCommand, \
	import.Cmd_RemoveCommand = Cmd_RemoveCommand, \
	\
	import.ML_Update = ML_Update, \
	import.ML_GetMapByNum = ML_GetMapByNum, \
	import.ML_FilenameExists = ML_FilenameExists, \
	\
	import.Cmd_ExecuteText = Cbuf_ExecuteText, \
	import.Cbuf_Execute = Cbuf_Execute, \
	\
	import.FakeClientConnect = SVC_FakeConnect, \
	import.DropClient = PF_DropClient, \
	import.GetClientState = PF_GetClientState, \
	import.ExecuteClientThinks = SV_ExecuteClientThinks, \
	\
	import.LocateEntities = SV_LocateEntities, \
	\
	import.asGetAngelExport = Com_asGetAngelExport \
	)

/*
* SV_InitGameProgs
*
* Init the game subsystem for a new map
*/
void SV_InitGameProgs( void )
{
	int apiversion;
	game_import_t import;
	void *( *builtinAPIfunc )(void *) = NULL;
	char manifest[MAX_INFO_STRING];

#ifdef GAME_HARD_LINKED
	builtinAPIfunc = GetGameAPI;
#endif

	// unload anything we have now
	if( ge )
		SV_ShutdownGameProgs();

	sv_gameprogspool = _Mem_AllocPool( NULL, "Game Progs", MEMPOOL_GAMEPROGS, __FILE__, __LINE__ );

	// load a new game dll
	SV_InitGameProgsImportStruct( import );

	// clear module manifest string
	assert( sizeof( manifest ) >= MAX_INFO_STRING );
	memset( manifest, 0, sizeof( manifest ) );

	ge = (game_export_t *)Com_LoadGameLibrary( "game", "GetGameAPI", &module_handle, &import, builtinAPIfunc, qfalse, manifest );
	if( !ge )
		Com_Error( ERR_DROP, "Failed to load game DLL" );

	AC_LoadLibrary( (void *) &import, (void *) ge, ANTICHEAT_SERVER );	// impulZ: Refire AC Init

	apiversion = ge->API();
	if( apiversion != GAME_API_VERSION )
	{
		Com_UnloadGameLibrary( &module_handle );
		Mem_FreePool( &sv_gameprogspool );
		ge = NULL;
		Com_Error( ERR_DROP, "Game is version %i, not %i", apiversion, GAME_API_VERSION );
	}

	Cvar_ForceSet( "sv_modmanifest", manifest );

	SV_SetServerConfigStrings();

	ge->Init( time( NULL ), svc.snapFrameTime, APP_PROTOCOL_VERSION );
}
