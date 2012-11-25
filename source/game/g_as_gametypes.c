/*
Copyright (C) 2008 German Garcia

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

#include "g_local.h"
#include "g_as_local.h"

static void GT_ResetScriptData( void )
{
	level.gametype.initFunc = NULL;
	level.gametype.spawnFunc = NULL;
	level.gametype.matchStateStartedFunc = NULL;
	level.gametype.matchStateFinishedFunc = NULL;
	level.gametype.thinkRulesFunc = NULL;
	level.gametype.playerRespawnFunc = NULL;
	level.gametype.scoreEventFunc = NULL;
	level.gametype.scoreboardMessageFunc = NULL;
	level.gametype.selectSpawnPointFunc = NULL;
	level.gametype.clientCommandFunc = NULL;
	level.gametype.botStatusFunc = NULL;
	level.gametype.shutdownFunc = NULL;
}

void GT_asShutdownScript( void )
{
	int i;
	edict_t *e;

	if( level.asEngineHandle < 0 ) {
		return;
	}

	// release the callback and any other objects obtained from the script engine before releasing the engine
	for( i = 0; i < game.numentities; i++ ) {
		e = &game.edicts[i];

		if( e->scriptSpawned && !strcmp( e->scriptModule, GAMETYPE_SCRIPTS_MODULE_NAME ) ) {
			G_asReleaseEntityBehaviors( e );
		}
	}

	GT_ResetScriptData();
}

//"void GT_SpawnGametype()"
void GT_asCallSpawn( void )
{
	int error, asContextHandle;

	if( !level.gametype.spawnFunc )
		return;

	asContextHandle = angelExport->asAdquireContext( level.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.spawnFunc );
	if( error < 0 ) 
		return;

	error = angelExport->asExecute( asContextHandle );
	if( G_ExecutionErrorReport( level.asEngineHandle, asContextHandle, error ) )
		GT_asShutdownScript();
}

//"void GT_MatchStateStarted()"
void GT_asCallMatchStateStarted( void )
{
	int error, asContextHandle;

	if( !level.gametype.matchStateStartedFunc )
		return;

	asContextHandle = angelExport->asAdquireContext( level.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.matchStateStartedFunc );
	if( error < 0 ) 
		return;

	error = angelExport->asExecute( asContextHandle );
	if( G_ExecutionErrorReport( level.asEngineHandle, asContextHandle, error ) )
		GT_asShutdownScript();
}

//"bool GT_MatchStateFinished( int incomingMatchState )"
qboolean GT_asCallMatchStateFinished( int incomingMatchState )
{
	int error, asContextHandle;
	qboolean result;

	if( !level.gametype.matchStateFinishedFunc )
		return qtrue;

	asContextHandle = angelExport->asAdquireContext( level.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.matchStateFinishedFunc );
	if( error < 0 ) 
		return qtrue;

	// Now we need to pass the parameters to the script function.
	angelExport->asSetArgDWord( asContextHandle, 0, incomingMatchState );

	error = angelExport->asExecute( asContextHandle );
	if( G_ExecutionErrorReport( level.asEngineHandle, asContextHandle, error ) )
		GT_asShutdownScript();

	// Retrieve the return from the context
	result = angelExport->asGetReturnBool( asContextHandle );

	return result;
}

//"void GT_ThinkRules( void )"
void GT_asCallThinkRules( void )
{
	int error, asContextHandle;

	if( !level.gametype.thinkRulesFunc )
		return;

	asContextHandle = angelExport->asAdquireContext( level.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.thinkRulesFunc );
	if( error < 0 ) 
		return;

	error = angelExport->asExecute( asContextHandle );
	if( G_ExecutionErrorReport( level.asEngineHandle, asContextHandle, error ) )
		GT_asShutdownScript();
}

//"void GT_playerRespawn( cEntity @ent, int old_team, int new_team )"
void GT_asCallPlayerRespawn( edict_t *ent, int old_team, int new_team )
{
	int error, asContextHandle;

	if( !level.gametype.playerRespawnFunc )
		return;

	asContextHandle = angelExport->asAdquireContext( level.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.playerRespawnFunc );
	if( error < 0 ) 
		return;

	// Now we need to pass the parameters to the script function.
	angelExport->asSetArgObject( asContextHandle, 0, ent );
	angelExport->asSetArgDWord( asContextHandle, 1, old_team );
	angelExport->asSetArgDWord( asContextHandle, 2, new_team );

	error = angelExport->asExecute( asContextHandle );
	if( G_ExecutionErrorReport( level.asEngineHandle, asContextHandle, error ) )
		GT_asShutdownScript();
}

//"void GT_scoreEvent( cClient @client, String &score_event, String &args )"
void GT_asCallScoreEvent( gclient_t *client, const char *score_event, const char *args )
{
	int error, asContextHandle;
	asstring_t *s1, *s2;

	if( !level.gametype.scoreEventFunc )
		return;

	if( !score_event || !score_event[0] )
		return;

	if( !args )
		args = "";

	asContextHandle = angelExport->asAdquireContext( level.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.scoreEventFunc );
	if( error < 0 ) 
		return;

	// Now we need to pass the parameters to the script function.
	s1 = angelExport->asStringFactoryBuffer( score_event, strlen( score_event ) );
	s2 = angelExport->asStringFactoryBuffer( args, strlen( args ) );

	angelExport->asSetArgObject( asContextHandle, 0, client );
	angelExport->asSetArgObject( asContextHandle, 1, s1 );
	angelExport->asSetArgObject( asContextHandle, 2, s2 );

	error = angelExport->asExecute( asContextHandle );
	if( G_ExecutionErrorReport( level.asEngineHandle, asContextHandle, error ) )
		GT_asShutdownScript();

	angelExport->asStringRelease( s1 );
	angelExport->asStringRelease( s2 );
}

//"String @GT_ScoreboardMessage( uint maxlen )"
char *GT_asCallScoreboardMessage( unsigned int maxlen )
{
	asstring_t *string;
	int error, asContextHandle;

	scoreboardString[0] = 0;

	if( !level.gametype.scoreboardMessageFunc )
		return NULL;

	asContextHandle = angelExport->asAdquireContext( level.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.scoreboardMessageFunc );
	if( error < 0 ) 
		return NULL;

	// Now we need to pass the parameters to the script function.
	angelExport->asSetArgDWord( asContextHandle, 0, maxlen );

	error = angelExport->asExecute( asContextHandle );
	if( G_ExecutionErrorReport( level.asEngineHandle, asContextHandle, error ) )
		GT_asShutdownScript();

	string = (asstring_t *)angelExport->asGetReturnObject( asContextHandle );
	if( !string || !string->len || !string->buffer )
		return NULL;

	Q_strncpyz( scoreboardString, string->buffer, sizeof( scoreboardString ) );

	return scoreboardString;
}

//"cEntity @GT_SelectSpawnPoint( cEntity @ent )"
edict_t *GT_asCallSelectSpawnPoint( edict_t *ent )
{
	int error, asContextHandle;
	edict_t *spot;

	if( !level.gametype.selectSpawnPointFunc )
		return SelectDeathmatchSpawnPoint( ent ); // should have a hardcoded backup

	asContextHandle = angelExport->asAdquireContext( level.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.selectSpawnPointFunc );
	if( error < 0 ) 
		return SelectDeathmatchSpawnPoint( ent );

	// Now we need to pass the parameters to the script function.
	angelExport->asSetArgObject( asContextHandle, 0, ent );

	error = angelExport->asExecute( asContextHandle );
	if( G_ExecutionErrorReport( level.asEngineHandle, asContextHandle, error ) )
		GT_asShutdownScript();

	spot = (edict_t *)angelExport->asGetReturnObject( asContextHandle );
	if( !spot )
		spot = SelectDeathmatchSpawnPoint( ent );

	return spot;
}

//"bool GT_Command( cClient @client, String &cmdString, String &argsString, int argc )"
qboolean GT_asCallGameCommand( gclient_t *client, const char *cmd, const char *args, int argc )
{
	int error, asContextHandle;
	asstring_t *s1, *s2;

	if( !level.gametype.clientCommandFunc )
		return qfalse; // should have a hardcoded backup

	// check for having any command to parse
	if( !cmd || !cmd[0] )
		return qfalse;

	asContextHandle = angelExport->asAdquireContext( level.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.clientCommandFunc );
	if( error < 0 ) 
		return qfalse;

	// Now we need to pass the parameters to the script function.
	s1 = angelExport->asStringFactoryBuffer( cmd, strlen( cmd ) );
	s2 = angelExport->asStringFactoryBuffer( args, strlen( args ) );

	angelExport->asSetArgObject( asContextHandle, 0, client );
	angelExport->asSetArgObject( asContextHandle, 1, s1 );
	angelExport->asSetArgObject( asContextHandle, 2, s2 );
	angelExport->asSetArgDWord( asContextHandle, 3, argc );

	error = angelExport->asExecute( asContextHandle );
	if( G_ExecutionErrorReport( level.asEngineHandle, asContextHandle, error ) )
		GT_asShutdownScript();

	angelExport->asStringRelease( s1 );
	angelExport->asStringRelease( s2 );

	// Retrieve the return from the context
	return angelExport->asGetReturnBool( asContextHandle );
}

//"bool GT_UpdateBotStatus( cEntity @ent )"
qboolean GT_asCallBotStatus( edict_t *ent )
{
	int error, asContextHandle;

	if( !level.gametype.botStatusFunc )
		return qfalse; // should have a hardcoded backup

	asContextHandle = angelExport->asAdquireContext( level.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.botStatusFunc );
	if( error < 0 ) 
		return qfalse;

	// Now we need to pass the parameters to the script function.
	angelExport->asSetArgObject( asContextHandle, 0, ent );

	error = angelExport->asExecute( asContextHandle );
	if( G_ExecutionErrorReport( level.asEngineHandle, asContextHandle, error ) )
		GT_asShutdownScript();

	// Retrieve the return from the context
	return angelExport->asGetReturnBool( asContextHandle );
}

//"void GT_Shutdown()"
void GT_asCallShutdown( void )
{
	int error, asContextHandle;

	if( !level.gametype.shutdownFunc || !angelExport )
		return;

	asContextHandle = angelExport->asAdquireContext( level.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.shutdownFunc );
	if( error < 0 ) 
		return;

	error = angelExport->asExecute( asContextHandle );
	if( G_ExecutionErrorReport( level.asEngineHandle, asContextHandle, error ) )
		GT_asShutdownScript();
}

static qboolean G_asInitializeGametypeScript( const char *moduleName )
{
	int asEngineHandle, asContextHandle, error;
	int funcCount;
	const char *fdeclstr;

	// grab script function calls
	asEngineHandle = level.asEngineHandle;
	funcCount = 0;

	fdeclstr = "void GT_InitGametype()";
	level.gametype.initFunc = angelExport->asGetFunctionByDecl( asEngineHandle, moduleName, fdeclstr );
	if( !level.gametype.initFunc )
	{
		G_Printf( "* The function '%s' was not found. Can not continue.\n", fdeclstr );
		return qfalse;
	}
	else
		funcCount++;

	fdeclstr = "void GT_SpawnGametype()";
	level.gametype.spawnFunc = angelExport->asGetFunctionByDecl( asEngineHandle, moduleName, fdeclstr );
	if( !level.gametype.spawnFunc )
	{
		if( developer->integer || sv_cheats->integer )
			G_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	fdeclstr = "void GT_MatchStateStarted()";
	level.gametype.matchStateStartedFunc = angelExport->asGetFunctionByDecl( asEngineHandle, moduleName, fdeclstr );
	if( !level.gametype.matchStateStartedFunc )
	{
		if( developer->integer || sv_cheats->integer )
			G_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	fdeclstr = "bool GT_MatchStateFinished( int incomingMatchState )";
	level.gametype.matchStateFinishedFunc = angelExport->asGetFunctionByDecl( asEngineHandle, moduleName, fdeclstr );
	if( !level.gametype.matchStateFinishedFunc )
	{
		if( developer->integer || sv_cheats->integer )
			G_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	fdeclstr = "void GT_ThinkRules()";
	level.gametype.thinkRulesFunc = angelExport->asGetFunctionByDecl( asEngineHandle, moduleName, fdeclstr );
	if( !level.gametype.thinkRulesFunc )
	{
		if( developer->integer || sv_cheats->integer )
			G_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	fdeclstr = "void GT_playerRespawn( cEntity @ent, int old_team, int new_team )";
	level.gametype.playerRespawnFunc = angelExport->asGetFunctionByDecl( asEngineHandle, moduleName, fdeclstr );
	if( !level.gametype.playerRespawnFunc )
	{
		if( developer->integer || sv_cheats->integer )
			G_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	fdeclstr = "void GT_scoreEvent( cClient @client, String &score_event, String &args )";
	level.gametype.scoreEventFunc = angelExport->asGetFunctionByDecl( asEngineHandle, moduleName, fdeclstr );
	if( !level.gametype.scoreEventFunc )
	{
		if( developer->integer || sv_cheats->integer )
			G_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	fdeclstr = "String @GT_ScoreboardMessage( uint maxlen )";
	level.gametype.scoreboardMessageFunc = angelExport->asGetFunctionByDecl( asEngineHandle, moduleName, fdeclstr );
	if( !level.gametype.scoreboardMessageFunc )
	{
		if( developer->integer || sv_cheats->integer )
			G_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	fdeclstr = "cEntity @GT_SelectSpawnPoint( cEntity @ent )";
	level.gametype.selectSpawnPointFunc = angelExport->asGetFunctionByDecl( asEngineHandle, moduleName, fdeclstr );
	if( !level.gametype.selectSpawnPointFunc )
	{
		if( developer->integer || sv_cheats->integer )
			G_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	fdeclstr = "bool GT_Command( cClient @client, String &cmdString, String &argsString, int argc )";
	level.gametype.clientCommandFunc = angelExport->asGetFunctionByDecl( asEngineHandle, moduleName, fdeclstr );
	if( !level.gametype.clientCommandFunc )
	{
		if( developer->integer || sv_cheats->integer )
			G_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	fdeclstr = "bool GT_UpdateBotStatus( cEntity @ent )";
	level.gametype.botStatusFunc = angelExport->asGetFunctionByDecl( asEngineHandle, moduleName, fdeclstr );
	if( !level.gametype.botStatusFunc )
	{
		if( developer->integer || sv_cheats->integer )
			G_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	fdeclstr = "void GT_Shutdown()";
	level.gametype.shutdownFunc = angelExport->asGetFunctionByDecl( asEngineHandle, moduleName, fdeclstr );
	if( !level.gametype.shutdownFunc )
	{
		if( developer->integer || sv_cheats->integer )
			G_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	//
	// execute the GT_InitGametype function
	//

	asContextHandle = angelExport->asAdquireContext( level.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.initFunc );
	if( error < 0 ) 
		return qfalse;

	error = angelExport->asExecute( asContextHandle );
	if( G_ExecutionErrorReport( level.asEngineHandle, asContextHandle, error ) )
		return qfalse;

	return qtrue;
}

qboolean GT_asLoadScript( const char *gametypeName )
{
	const char *moduleName = GAMETYPE_SCRIPTS_MODULE_NAME;

	GT_ResetScriptData();

	// Load the script
	if( !G_LoadGameScript( moduleName, GAMETYPE_SCRIPTS_DIRECTORY, gametypeName, GAMETYPE_PROJECT_EXTENSION ) ) {
		return qfalse;
	}

	// Initialize the script
	if( !G_asInitializeGametypeScript( moduleName ) ) {
		GT_asShutdownScript();
		return qfalse;
	}

	return qtrue;
}
