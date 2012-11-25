/*
Copyright (C) 2012 Chasseur de bots

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

/*
* G_ResetMapScriptData
*/
static void G_ResetMapScriptData( void )
{
	memset( &level.mapscript, 0, sizeof( level.mapscript ) );
}

/*
* G_asInitializeMapScript
*/
static qboolean G_asInitializeMapScript( const char *moduleName )
{
	const char *fdeclstr;
	int asEngineHandle = level.asEngineHandle;

	fdeclstr = "void MAP_Init()";
	level.mapscript.initFunc = angelExport->asGetFunctionByDecl( asEngineHandle, moduleName, fdeclstr );
	if( !level.mapscript.initFunc )
	{
		if( developer->integer || sv_cheats->integer )
			G_Printf( "* The function '%s' was not present in the map script.\n", fdeclstr );
	}

	fdeclstr = "void MAP_PreThink()";
	level.mapscript.preThinkFunc = angelExport->asGetFunctionByDecl( asEngineHandle, moduleName, fdeclstr );
	if( !level.mapscript.preThinkFunc )
	{
		if( developer->integer || sv_cheats->integer )
			G_Printf( "* The function '%s' was not present in the map script.\n", fdeclstr );
	}

	fdeclstr = "void MAP_PostThink()";
	level.mapscript.postThinkFunc = angelExport->asGetFunctionByDecl( asEngineHandle, moduleName, fdeclstr );
	if( !level.mapscript.postThinkFunc )
	{
		if( developer->integer || sv_cheats->integer )
			G_Printf( "* The function '%s' was not present in the map script.\n", fdeclstr );
	}

	fdeclstr = "void MAP_Exit()";
	level.mapscript.exitFunc = angelExport->asGetFunctionByDecl( asEngineHandle, moduleName, fdeclstr );
	if( !level.mapscript.exitFunc )
	{
		if( developer->integer || sv_cheats->integer )
			G_Printf( "* The function '%s' was not present in the map script.\n", fdeclstr );
	}

	return qtrue;
}

/*
* G_asCallMapFunction
*/
static void G_asCallMapFunction( void *func )
{
	int error, asContextHandle;

	if( !func || !angelExport )
		return;

	asContextHandle = angelExport->asAdquireContext( level.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, func );
	if( error < 0 ) 
		return;

	inMapFuncCall = qtrue;

	error = angelExport->asExecute( asContextHandle );
	if( G_ExecutionErrorReport( level.asEngineHandle, asContextHandle, error ) )
		G_asShutdownMapScript();

	inMapFuncCall = qfalse;
}

/*
* G_asCallMapInit
*/
void G_asCallMapInit( void )
{
	G_asCallMapFunction( level.mapscript.initFunc );
}

/*
* G_asCallMapPreThink
*/
void G_asCallMapPreThink( void )
{
	G_asCallMapFunction( level.mapscript.preThinkFunc );
}

/*
* G_asCallMapPostThink
*/
void G_asCallMapPostThink( void )
{
	G_asCallMapFunction( level.mapscript.postThinkFunc );
}

/*
* G_asCallMapExit
*/
void G_asCallMapExit( void )
{
	G_asCallMapFunction( level.mapscript.exitFunc );
}

/*
* G_asLoadMapScript
*/
qboolean G_asLoadMapScript( const char *mapName )
{
	const char *moduleName = MAP_SCRIPTS_MODULE_NAME;

	G_ResetMapScriptData();

	// Load the script
	if( !G_LoadGameScript( moduleName, MAP_SCRIPTS_DIRECTORY, mapName, MAP_SCRIPTS_PROJECT_EXTENSION ) ) {
		return qfalse;
	}

	// Initialize the script
	if( !G_asInitializeMapScript( moduleName ) ) {
		G_asShutdownMapScript();
		return qfalse;
	}

	return qtrue;
}

/*
* G_asShutdownMapScript
*/
void G_asShutdownMapScript( void )
{
	int i;
	edict_t *e;

	if( level.asEngineHandle < 0 ) {
		return;
	}

	// release the callback and any other objects obtained from the script engine before releasing the engine
	for( i = 0; i < game.numentities; i++ ) {
		e = &game.edicts[i];

		if( e->scriptSpawned && !strcmp( e->scriptModule, MAP_SCRIPTS_MODULE_NAME ) ) {
			G_asReleaseEntityBehaviors( e );
		}
	}

	G_ResetMapScriptData();
}
