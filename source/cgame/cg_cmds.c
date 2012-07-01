/*
Copyright (C) 2002-2003 Victor Luchits

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

#include "cg_local.h"

/*
==========================================================================

SERVER COMMANDS

==========================================================================
*/

static void CG_SC_AutoRecordAction( const char *action );

/*
* CG_SC_Print
*/
static void CG_SC_Print( void )
{
	CG_LocalPrint( qfalse, "%s", trap_Cmd_Argv( 1 ) );
}

/*
* CG_SC_ChatPrint
*/
static void CG_SC_ChatPrint( void )
{
	const qboolean teamonly = (!Q_stricmp( trap_Cmd_Argv( 0 ), "tch" ) ? qtrue : qfalse);
	const int who = atoi( trap_Cmd_Argv( 1 ) );
	const char *name = (who && who == bound(1, who, MAX_CLIENTS) ? cgs.clientInfo[who-1].name : NULL);
	const char *text = trap_Cmd_Argv( 2 );
	const cvar_t *filter = (cgs.tv ? cg_chatFilterTV : cg_chatFilter);

	if( filter->integer & (teamonly ? 2 : 1) )
		return;

	if( !name )
		CG_LocalPrint( qfalse, S_COLOR_GREEN "console: %s\n", text );
	else if( teamonly )
		CG_LocalPrint( qtrue, S_COLOR_YELLOW "[%s]" S_COLOR_WHITE "%s:" S_COLOR_YELLOW " %s\n",
			cg.frame.playerState.stats[STAT_REALTEAM] == TEAM_SPECTATOR ? "SPEC" : "TEAM", name, text );
	else
		CG_LocalPrint( qfalse, "%s" S_COLOR_GREEN ": %s\n", name, text );

	if( cg_chatBeep->integer )
		trap_S_StartGlobalSound( CG_MediaSfx( cgs.media.sfxChat ), CHAN_AUTO, 1.0f );
}

/*
* CG_SC_TVChatPrint
*/
static void CG_SC_TVChatPrint( void )
{
	const char *name = trap_Cmd_Argv( 1 );
	const char *text = trap_Cmd_Argv( 2 );
	const cvar_t *filter = (cgs.tv ? cg_chatFilterTV : cg_chatFilter);

	if( filter->integer & 4 )
		return;

	CG_LocalPrint( qfalse, S_COLOR_RED "[TV]" S_COLOR_WHITE "%s" S_COLOR_GREEN ": %s", name, text );
	if( cg_chatBeep->integer )
		trap_S_StartGlobalSound( CG_MediaSfx( cgs.media.sfxChat ), CHAN_AUTO, 1.0f );
}

/*
* CG_SC_CenterPrint
*/
static void CG_SC_CenterPrint( void )
{
	CG_CenterPrint( trap_Cmd_Argv( 1 ) );
}

/*
* CG_ConfigString
*/
void CG_ConfigString( int i, const char *s )
{
	size_t len;

	// wsw : jal : warn if configstring overflow
	len = strlen( s );
	if( len >= MAX_CONFIGSTRING_CHARS )
		CG_Printf( "%sWARNING:%s Configstring %i overflowed\n", S_COLOR_YELLOW, S_COLOR_WHITE, i );

	if( i < 0 || i >= MAX_CONFIGSTRINGS )
		CG_Error( "configstring > MAX_CONFIGSTRINGS" );

	Q_strncpyz( cgs.configStrings[i], s, sizeof( cgs.configStrings[i] ) );

	// do something apropriate
	if( i == CS_MAPNAME )
	{
		CG_RegisterLevelMinimap();
	}
	else if( i == CS_TVSERVER )
	{
		CG_UpdateTVServerString();
	}
	else if( i == CS_GAMETYPETITLE )
	{
	}
	else if( i == CS_GAMETYPENAME )
	{
		GS_SetGametypeName( cgs.configStrings[CS_GAMETYPENAME] );
	}
	else if( i == CS_AUTORECORDSTATE )
	{
		CG_SC_AutoRecordAction( cgs.configStrings[i] );
	}
	else if( i >= CS_MODELS && i < CS_MODELS+MAX_MODELS )
	{
		if( cgs.configStrings[i][0] == '$' )	// indexed pmodel
			cgs.pModelsIndex[i-CS_MODELS] = CG_RegisterPlayerModel( cgs.configStrings[i]+1 );
		else
			cgs.modelDraw[i-CS_MODELS] = CG_RegisterModel( cgs.configStrings[i] );
	}
	else if( i >= CS_SOUNDS && i < CS_SOUNDS+MAX_SOUNDS )
	{
		if( cgs.configStrings[i][0] != '*' )
			cgs.soundPrecache[i-CS_SOUNDS] = trap_S_RegisterSound( cgs.configStrings[i] );
	}
	else if( i >= CS_IMAGES && i < CS_IMAGES+MAX_IMAGES )
	{
		cgs.imagePrecache[i-CS_IMAGES] = trap_R_RegisterPic( cgs.configStrings[i] );
	}
	else if( i >= CS_SKINFILES && i < CS_SKINFILES+MAX_SKINFILES )
	{
		cgs.skinPrecache[i-CS_SKINFILES] = trap_R_RegisterSkinFile( cgs.configStrings[i] );
	}
	else if( i >= CS_LIGHTS && i < CS_LIGHTS+MAX_LIGHTSTYLES )
	{
		CG_SetLightStyle( i - CS_LIGHTS );
	}
	else if( i >= CS_ITEMS && i < CS_ITEMS+MAX_ITEMS )
	{
		CG_ValidateItemDef( i - CS_ITEMS, cgs.configStrings[i] );
	}
	else if( i >= CS_PLAYERINFOS && i < CS_PLAYERINFOS+MAX_CLIENTS )
	{
		CG_LoadClientInfo( &cgs.clientInfo[i-CS_PLAYERINFOS], cgs.configStrings[i], i-CS_PLAYERINFOS );
	}
	else if( i >= CS_GAMECOMMANDS && i < CS_GAMECOMMANDS+MAX_GAMECOMMANDS )
	{
		if( !cgs.demoPlaying )
		{
			trap_Cmd_AddCommand( cgs.configStrings[i], NULL );
			if( !Q_stricmp( cgs.configStrings[i], "gametypemenu" ) ) {
				cgs.hasGametypeMenu = qtrue;
			}
		}
	}
	else if( i >= CS_WEAPONDEFS && i < CS_WEAPONDEFS + MAX_WEAPONDEFS )
	{
		CG_OverrideWeapondef( i - CS_WEAPONDEFS, cgs.configStrings[i] );
	}
}

/*
* CG_SC_Scoreboard
*/
static void CG_SC_Scoreboard( void )
{
	SCR_UpdateScoreboardMessage( trap_Cmd_Argv( 1 ) );
}

/*
* CG_SC_PrintPlayerStats
*/
static void CG_SC_PrintPlayerStats( const char *s, void ( *pp ) )
{
	int playerNum;
	int i, shot_weak, hit_weak, shot_strong, hit_strong, hit_total, shot_total;
	int total_damage_given, total_damage_received, health_taken, armor_taken;
	gsitem_t *item;
	void ( *print )( const char *format, ... ) = pp;

	playerNum = CG_ParseValue( &s );
	if( playerNum < 0 || playerNum >= gs.maxclients )
		return;

	// print stats to console/file
	print( "Stats for %s" S_COLOR_WHITE ":\r\n\r\n", cgs.clientInfo[playerNum].name );
	print( "   Weapon             Weak               Strong\r\n" );
	print( "    hit/shot percent   hit/shot percent   hit/shot percent\r\n" );

	for( i = WEAP_GUNBLADE; i < WEAP_TOTAL; i++ )
	{
		item = GS_FindItemByTag( i );
		assert( item );

		shot_total = CG_ParseValue( &s );
		if( shot_total < 1 )  // only continue with registered shots
			continue;
		hit_total = CG_ParseValue( &s );

		shot_strong = CG_ParseValue( &s );
		hit_strong = (shot_strong != shot_total ? CG_ParseValue( &s ) : hit_total);

		shot_weak = shot_total - shot_strong;
		hit_weak = hit_total - hit_strong;

		// name
		print( "%s%2s" S_COLOR_WHITE ": ", item->color, item->shortname );

#define STATS_PERCENT(hit,total) ((total) == 0 ? 0 : ((hit) == (total) ? 100 : (float)(hit) * 100.0f / (float)(total)))

		// total
		print( S_COLOR_GREEN "%3i" S_COLOR_WHITE "/" S_COLOR_CYAN "%3i      " S_COLOR_YELLOW "%2.1f",
			hit_total, shot_total, STATS_PERCENT( hit_total, shot_total ) );

		// weak
		print( "    " S_COLOR_GREEN "%3i" S_COLOR_WHITE "/" S_COLOR_CYAN "%3i      " S_COLOR_YELLOW "%2.1f",
			hit_weak, shot_weak,  STATS_PERCENT( hit_weak, shot_weak ) );

		// strong
		print( "   " S_COLOR_GREEN "%3i" S_COLOR_WHITE "/" S_COLOR_CYAN "%3i      " S_COLOR_YELLOW "%2.1f",
			hit_strong, shot_strong, STATS_PERCENT( hit_strong, shot_strong ) );

		print( "\r\n" );
	}

	print( "\r\n" );

	total_damage_given = CG_ParseValue( &s );
	total_damage_received = CG_ParseValue( &s );

	print( S_COLOR_YELLOW "Damage given/received: " S_COLOR_WHITE "%i/%i " S_COLOR_YELLOW "ratio: %s%3.2f\r\n",
		total_damage_given, total_damage_received,
		( total_damage_given > total_damage_received ? S_COLOR_GREEN : S_COLOR_RED ),
		STATS_PERCENT( total_damage_given, total_damage_given + total_damage_received ) );

	health_taken = CG_ParseValue( &s );
	armor_taken = CG_ParseValue( &s );

	print( S_COLOR_YELLOW "Health/Armor taken : " S_COLOR_CYAN "%i" S_COLOR_WHITE "/" S_COLOR_CYAN "%i\r\n",
		health_taken, armor_taken );

#undef STATS_PERCENT
}

/*
* CG_SC_PrintStatsToFile
*/
static int cg_statsFileHandle;
void CG_SC_PrintStatsToFile( const char *format, ... )
{
	va_list	argptr;
	char msg[1024];

	va_start( argptr, format );
	Q_vsnprintfz( msg, sizeof( msg ), format, argptr );
	va_end( argptr );

	trap_FS_Print( cg_statsFileHandle, msg );
}

/*
* CG_SC_DumpPlayerStats
*/
static void CG_SC_DumpPlayerStats( const char *filename, const char *stats )
{
	if( cgs.demoPlaying )
		return;

	if( trap_FS_FOpenFile( filename, &cg_statsFileHandle, FS_APPEND ) == -1 )
	{
		CG_Printf( "Couldn't write autorecorded stats, error opening file %s\n", filename );
		return;
	}

	CG_SC_PrintPlayerStats( stats, CG_SC_PrintStatsToFile );

	trap_FS_FCloseFile( cg_statsFileHandle );
}

/*
* CG_SC_PlayerStats
*/
static void CG_SC_PlayerStats( void )
{
	const char *s;
	int print;

	print = atoi( trap_Cmd_Argv( 1 ) );
	s = trap_Cmd_Argv( 2 );

	if( !print )
	{	// scoreboard message update
		SCR_UpdatePlayerStatsMessage( s );
		return;
	}

	CG_SC_PrintPlayerStats( s, CG_Printf );

	if( print == 2 )
		CG_SC_AutoRecordAction( "stats" );
}

/*
* CG_SC_AutoRecordName
*/
static const char *CG_SC_AutoRecordName( void )
{
	time_t long_time;
	struct tm *newtime;
	static char name[MAX_STRING_CHARS];
	char mapname[MAX_CONFIGSTRING_CHARS];
	const char *cleanplayername, *cleanplayername2;

	// get date from system
	time( &long_time );
	newtime = localtime( &long_time );

	if( cg.view.POVent <= 0 )
	{
		cleanplayername2 = "";
	}
	else
	{
		// remove color tokens from player names (doh)
		cleanplayername = COM_RemoveColorTokens( cgs.clientInfo[cg.view.POVent-1].name );

		// remove junk chars from player names for files
		cleanplayername2 = COM_RemoveJunkChars( cleanplayername );
	}

	// lowercase mapname
	Q_strncpyz( mapname, cgs.configStrings[CS_MAPNAME], sizeof( mapname ) );
	Q_strlwr( mapname );

	// make file name
	// duel_year-month-day_hour-min_map_player
	Q_snprintfz( name, sizeof( name ), "%s_%04d-%02d-%02d_%02d-%02d_%s_%s_%04i",
		gs.gametypeName,
		newtime->tm_year + 1900, newtime->tm_mon+1, newtime->tm_mday,
		newtime->tm_hour, newtime->tm_min,
		mapname,
		cleanplayername2,
		(int)brandom( 0, 9999 )
		);

	return name;
}

//racesow
/*
* CG_SC_RaceDemoRename
*/
/* enable this, when trap_FS_MoveFile works! (tested)
static qboolean CG_SC_RaceDemoRename( const char *src, const char *dst )
{
	char *baseDirectory = "demos"; //hardcoded string
	char *ext = "wd"; //hardcoded string
	int protocol = cgs.gameProtocol;
	int file;

	if ( !trap_FS_MoveFile( va( "%s/%s", baseDirectory, src ), va( "%s/%s.%s%d", baseDirectory, dst, ext, protocol ) ) )
	{
		//workaround to create the path
		trap_FS_FOpenFile( va( "%s/%s.%s%d", baseDirectory, dst, ext, protocol ), &file, FS_WRITE );
		trap_FS_FCloseFile( file );

		if ( !trap_FS_MoveFile( va( "%s/%s", baseDirectory, src ), va( "%s/%s.%s%d", baseDirectory, dst, ext, protocol ) ) )
			return qfalse;
	}
	return qtrue;
}
*/

/*
* CG_SC_RaceDemoName
*/
static const char *CG_SC_RaceDemoName( unsigned int raceTime )
{
	unsigned int hour, min, sec, milli;

	static char name[MAX_STRING_CHARS];
	char mapname[MAX_CONFIGSTRING_CHARS];

	milli = raceTime;

	hour = milli/3600000;
	milli -= hour*3600000;
	min = milli/60000;
	milli -= min*60000;
	sec = milli/1000;
	milli -= sec*1000;

	// lowercase mapname
	Q_strncpyz( mapname, cgs.configStrings[CS_MAPNAME], sizeof( mapname ) );
	Q_strlwr( mapname );

	// make file path
	// "gametype/map/map_time_random"
	Q_snprintfz( name, sizeof( name ), "%s/%s/%s_%02u-%02u-%02u-%003u_%04i",
		gs.gametypeName,
		mapname,
		mapname, hour, min, sec, milli,	(int)brandom( 0, 9999 )
		);

	return name;
}

enum {
	RS_RACEDEMO_START = 0,
	RS_RACEDEMO_STOP,
	RS_RACEDEMO_CANCEL
};

/*
* CG_SC_RaceDemo
*/
static void CG_SC_RaceDemo( int action, unsigned int raceTime )
{
	//enable this, when trap_FS_MoveFile works!
//	char *demoname = "currentrace.rec";

	char *directory = "autorecord";
	const char *realname;
	static qboolean autorecording = qfalse;

	// filter out autorecord commands when playing a demo
	if( cgs.demoPlaying )
		return;


	switch( action )
	{
	case RS_RACEDEMO_START:
		if( rs_autoRaceDemo->integer )
		{
			//delete "cancel", when trap_FS_MoveFile works!
			trap_Cmd_ExecuteText( EXEC_NOW, "stop cancel silent" );

			//delete this, when trap_FS_MoveFile works!
			realname = CG_SC_AutoRecordName();
			trap_Cmd_ExecuteText( EXEC_NOW, va( "record %s/%s/%s silent",
				directory, gs.gametypeName, realname ) );

			//enable this, when trap_FS_MoveFile works!
//			trap_Cmd_ExecuteText( EXEC_NOW, va( "record %s/%s silent",
//					directory, demoname ) );

		}
		autorecording = qtrue;
		break;
	case RS_RACEDEMO_STOP:
		if( rs_autoRaceDemo->integer )
			trap_Cmd_ExecuteText( EXEC_NOW, "stop silent" );

		if( autorecording && raceTime > 0 )
		{
			realname = CG_SC_RaceDemoName( raceTime );

			if( rs_autoRaceScreenshot->integer )
				trap_Cmd_ExecuteText( EXEC_NOW, va( "screenshot %s/%s silent",
						directory, realname ) );

			//enable this, when trap_FS_MoveFile works!
//			if ( rs_autoRaceDemo->integer )
//				CG_SC_RaceDemoRename( va( "%s/%s", directory, demoname ),
//						va( "%s/%s", directory, realname ) );
		}
		autorecording = qfalse;
		break;
	case RS_RACEDEMO_CANCEL:
		if( rs_autoRaceDemo->integer )
			trap_Cmd_ExecuteText( EXEC_NOW, "stop cancel silent" );
		autorecording = qfalse;
		break;
	}
}

/*
* RaceDemo control functions
*/
static void CG_SC_RaceDemoStart( void )
{
	CG_SC_RaceDemo( RS_RACEDEMO_START, 0 );
}
static void CG_SC_RaceDemoStop( void )
{
	CG_SC_RaceDemo( RS_RACEDEMO_STOP, atoi( trap_Cmd_Argv( 1 ) ) );
}
static void CG_SC_RaceDemoCancel( void )
{
	CG_SC_RaceDemo( RS_RACEDEMO_CANCEL, 0 );
}
//!racesow

/*
* CG_SC_AutoRecordAction
*/
static void CG_SC_AutoRecordAction( const char *action )
{
	static qboolean autorecording = qfalse;
	const char *name;
	qboolean spectator;

	if( !action[0] )
		return;

	// filter out autorecord commands when playing a demo
	if( cgs.demoPlaying )
		return;

	// let configstrings and other stuff arrive before taking any action
	if( !cgs.precacheDone )
		return;

	if( cg.frame.playerState.pmove.pm_type == PM_SPECTATOR || cg.frame.playerState.pmove.pm_type == PM_CHASECAM )
		spectator = qtrue;
	else
		spectator = qfalse;

	name = CG_SC_AutoRecordName();

	if( !Q_stricmp( action, "start" ) )
	{
		if( cg_autoaction_demo->integer && ( !spectator || cg_autoaction_spectator->integer ) )
		{
			trap_Cmd_ExecuteText( EXEC_NOW, "stop silent" );
			trap_Cmd_ExecuteText( EXEC_NOW, va( "record autorecord/%s/%s silent",
				gs.gametypeName, name ) );
			autorecording = qtrue;
		}
	}
	else if( !Q_stricmp( action, "altstart" ) )
	{
		if( cg_autoaction_demo->integer && ( !spectator || cg_autoaction_spectator->integer ) )
		{
			trap_Cmd_ExecuteText( EXEC_NOW, va( "record autorecord/%s/%s silent",
				gs.gametypeName, name ) );
			autorecording = qtrue;
		}
	}
	else if( !Q_stricmp( action, "stop" ) )
	{
		if( autorecording )
		{
			trap_Cmd_ExecuteText( EXEC_NOW, "stop silent" );
			autorecording = qfalse;
		}

		if( cg_autoaction_screenshot->integer && ( !spectator || cg_autoaction_spectator->integer ) )
		{
			trap_Cmd_ExecuteText( EXEC_NOW, va( "screenshot autorecord/%s/%s silent",
				gs.gametypeName, name ) );
		}
	}
	else if( !Q_stricmp( action, "cancel" ) )
	{
		if( autorecording )
		{
			trap_Cmd_ExecuteText( EXEC_NOW, "stop cancel silent" );
			autorecording = qfalse;
		}
	}
	else if( !Q_stricmp( action, "stats" ) )
	{
		if( cg_autoaction_stats->integer && ( !spectator || cg_autoaction_spectator->integer ) )
		{
			const char *filename = va( "stats/%s/%s.txt", gs.gametypeName, name );
			CG_SC_DumpPlayerStats( filename, trap_Cmd_Argv( 2 ) );
		}
	}
	else if( developer->integer )
	{
		CG_Printf( "CG_SC_AutoRecordAction: Unknown action: %s\n", action );
	}
}

/*
* CG_SC_ChannelAdd
*/
static void CG_SC_ChannelAdd( void )
{
	char menuparms[MAX_STRING_CHARS];

	Q_snprintfz( menuparms, sizeof( menuparms ), "menu_tvchannel_add %s\n", trap_Cmd_Args() );
	trap_Cmd_ExecuteText( EXEC_NOW, menuparms );
}

/*
* CG_SC_ChannelRemove
*/
static void CG_SC_ChannelRemove( void )
{
	int i, id;

	for( i = 1; i < trap_Cmd_Argc(); i++ )
	{
		id = atoi( trap_Cmd_Argv( i ) );
		if( id <= 0 )
			continue;
		trap_Cmd_ExecuteText( EXEC_NOW, va( "menu_tvchannel_remove %i\n", id ) );
	}
}

/*
* CG_SC_MatchMessage
*/
static void CG_SC_MatchMessage( void )
{
	matchmessage_t mm;

	cg.matchmessage = NULL;

	mm = atoi( trap_Cmd_Argv( 1 ) );
	cg.matchmessage = GS_MatchMessageString( mm );
	if( !cg.matchmessage[0] )
		cg.matchmessage = NULL;
}

/*
* CG_CS_UpdateTeamInfo
*/
static void CG_CS_UpdateTeamInfo( void )
{
	char *ti;

	ti = trap_Cmd_Argv( 1 );
	if( !ti[0] )
	{
		cg.teaminfo_size = 0;
		CG_Free( cg.teaminfo );
		cg.teaminfo = NULL;
		return;
	}

	if( strlen( ti ) + 1 > cg.teaminfo_size )
	{
		if( cg.teaminfo )
			CG_Free( cg.teaminfo );
		cg.teaminfo_size = strlen( ti ) + 1;
		cg.teaminfo = CG_Malloc( cg.teaminfo_size );
	}

	Q_strncpyz( cg.teaminfo, ti, cg.teaminfo_size );
}

/*
* CG_Cmd_DemoGet_f
*/
static qboolean demo_requested = qfalse;
void CG_Cmd_DemoGet_f( void )
{
	if( demo_requested )
	{
		CG_Printf( "Already requesting a demo\n" );
		return;
	}

	if( trap_Cmd_Argc() != 2 || ( atoi( trap_Cmd_Argv( 1 ) ) <= 0 && trap_Cmd_Argv( 1 )[0] != '.' ) )
	{
		CG_Printf( "Usage: demoget <number>\n" );
		CG_Printf( "Donwloads a demo from the server\n" );
		CG_Printf( "Use the demolist command to see list of demos on the server\n" );
		return;
	}

	trap_Cmd_ExecuteText( EXEC_NOW, va( "cmd demoget %s", trap_Cmd_Argv( 1 ) ) );

	demo_requested = qtrue;
}

/*
* CG_SC_DemoGet
*/
static void CG_SC_DemoGet( void )
{
	if( cgs.demoPlaying )
	{
		// ignore download commands coming from demo files
		return;
	}

	if( !demo_requested )
	{
		CG_Printf( "Warning: demoget when not requested, ignored\n" );
		return;
	}

	demo_requested = qfalse;

	if( trap_Cmd_Argc() < 2 )
	{
		CG_Printf( "No such demo found\n" );
		return;
	}

	if( !COM_ValidateRelativeFilename( trap_Cmd_Argv( 1 ) ) )
	{
		CG_Printf( "Warning: demoget: Invalid filename, ignored\n" );
		return;
	}

	trap_DownloadRequest( trap_Cmd_Argv( 1 ), qfalse );
}

/*
* CG_SC_MOTD
*/
static void CG_SC_MOTD( void )
{
	char *motd;

	if( cg.motd )
		CG_Free( cg.motd );
	cg.motd = NULL;

	motd = trap_Cmd_Argv( 2 );
	if( !motd[0] )
		return;

	if( !strcmp( trap_Cmd_Argv( 1 ), "1" ) )
	{
		cg.motd = CG_CopyString( motd );
		cg.motd_time = cg.time + 50 *strlen( motd );
		if( cg.motd_time < cg.time + 5000 )
			cg.motd_time = cg.time + 5000;
	}

	CG_Printf( "\nMessage of the Day:\n%s", motd );
}

/*
* CG_SC_MenuCustom
*/
static void CG_SC_MenuCustom( void )
{
	char request[MAX_STRING_CHARS];
	int i, c;

	if( cgs.demoPlaying || cgs.tv )
		return;

	if( trap_Cmd_Argc() < 2 )
		return;

	Q_strncpyz( request, va( "menu_open custom title \"%s\" ", trap_Cmd_Argv( 1 ) ), sizeof( request ) );
	
	for( i = 2, c = 1; i < trap_Cmd_Argc() - 1; i += 2, c++ )
	{
		Q_strncatz( request, va( "btn%i \"%s\" ", c, trap_Cmd_Argv( i ) ), sizeof( request ) );
		Q_strncatz( request, va( "cmd%i \"%s\" ", c, trap_Cmd_Argv( i + 1 ) ), sizeof( request ) );
	}

	trap_Cmd_ExecuteText( EXEC_APPEND, va( "%s\n", request ) );
}

/*
* CG_AddAward
*/
void CG_AddAward( const char *str )
{
	if( !str || !str[0] )
		return;

	Q_strncpyz( cg.award_lines[cg.award_head % MAX_AWARD_LINES], str, MAX_CONFIGSTRING_CHARS );
	cg.award_times[cg.award_head % MAX_AWARD_LINES] = cg.time;
	cg.award_head++;
}

/*
* CG_SC_AddAward
*/
static void CG_SC_AddAward( void )
{
	CG_AddAward( trap_Cmd_Argv( 1 ) );
}

//racesow
/*
* CG_CheckpointsAdd
*/
void CG_CheckpointsAdd( int cpNum, int time )
{
	if ( cpNum >= 0 && cpNum < MAX_CHECKPOINTS )
		cg.checkpoints[cpNum] = time;
}

/*
* CG_SC_CheckpointsAdd
*
* 1: checkpoint number
* 2: checkpoint time in msec
*
*/
static void CG_SC_CheckpointsAdd( void )
{
	CG_CheckpointsAdd( atoi( trap_Cmd_Argv( 1 ) ), atoi( trap_Cmd_Argv( 2 ) ) );
}

/*
* CG_CheckpointsClear
*/
void CG_CheckpointsClear( void )
{
	int i;
	for ( i = 0; i < MAX_CHECKPOINTS; i++ ) {
		cg.checkpoints[i] = STAT_NOTSET;
	}
}

/*
* CG_SC_CheckpointsClear
*/
static void CG_SC_CheckpointsClear( void )
{
	CG_CheckpointsClear();
}
//!racesow

/*
* CG_SC_ExecuteText
*/
static void CG_SC_ExecuteText( void )
{
	if( cgs.demoPlaying || cgs.tv )
		return;

	trap_Cmd_ExecuteText( EXEC_APPEND, trap_Cmd_Args() );
}

typedef struct
{
	char *name;
	void ( *func )( void );
} svcmd_t;

svcmd_t cg_svcmds[] =
{
	{ "pr", CG_SC_Print },
	{ "ch", CG_SC_ChatPrint },
	{ "tch", CG_SC_ChatPrint },
	{ "tvch", CG_SC_TVChatPrint },
	{ "cp", CG_SC_CenterPrint },
	{ "obry", CG_SC_Obituary },
	{ "scb", CG_SC_Scoreboard },
	{ "plstats", CG_SC_PlayerStats },
	{ "mm", CG_SC_MatchMessage },
	{ "ti", CG_CS_UpdateTeamInfo },
	{ "demoget", CG_SC_DemoGet },
	{ "cha", CG_SC_ChannelAdd },
	{ "chr", CG_SC_ChannelRemove },
	{ "mecu", CG_SC_MenuCustom },
	{ "motd", CG_SC_MOTD },
	{ "aw", CG_SC_AddAward },
	{ "cpa", CG_SC_CheckpointsAdd }, //racesow
	{ "cpc", CG_SC_CheckpointsClear }, //racesow
	{ "dstart", CG_SC_RaceDemoStart }, //racesow
	{ "dstop", CG_SC_RaceDemoStop }, //racesow
	{ "dcancel", CG_SC_RaceDemoCancel }, //racesow
	{ "cmd", CG_SC_ExecuteText },

	{ NULL }
};

/*
* CG_GameCommand
*/
void CG_GameCommand( const char *command )
{
	char *s;
	svcmd_t *cmd;

	trap_Cmd_TokenizeString( command );

	s = trap_Cmd_Argv( 0 );
	for( cmd = cg_svcmds; cmd->name; cmd++ )
	{
		if( !strcmp( s, cmd->name ) )
		{
			cmd->func();
			return;
		}
	}

	CG_Printf( "Unknown game command: %s\n", s );
}

/*
==========================================================================

CGAME COMMANDS

==========================================================================
*/

/*
* CG_UseItem
*/
void CG_UseItem( const char *name )
{
	gsitem_t *item;

	if( !cg.frame.valid || cgs.demoPlaying )
		return;

	if( !name )
		return;

	item = GS_Cmd_UseItem( &cg.frame.playerState, name, 0 );
	if( item )
	{
		if( item->type & IT_WEAPON )
		{
			CG_Predict_ChangeWeapon( item->tag );
			cg.lastWeapon = cg.predictedPlayerState.stats[STAT_PENDING_WEAPON];
		}

		trap_Cmd_ExecuteText( EXEC_NOW, va( "cmd use %i", item->tag ) );
	}
}

/*
* CG_Cmd_UseItem_f
*/
static void CG_Cmd_UseItem_f( void )
{
	if( !trap_Cmd_Argc() )
	{
		CG_Printf( "Usage: 'use <item name>' or 'use <item index>'\n" );
		return;
	}

	CG_UseItem( trap_Cmd_Args() );
}

/*
* CG_Cmd_NextWeapon_f
*/
static void CG_Cmd_NextWeapon_f( void )
{
	gsitem_t *item;

	if( !cg.frame.valid )
		return;

	if( cgs.demoPlaying || cg.predictedPlayerState.pmove.pm_type == PM_CHASECAM )
	{
		CG_ChaseStep( 1 );
		return;
	}

	item = GS_Cmd_NextWeapon_f( &cg.frame.playerState, cg.predictedWeaponSwitch );
	if( item )
	{
		CG_Predict_ChangeWeapon( item->tag );
		trap_Cmd_ExecuteText( EXEC_NOW, va( "cmd use %i", item->tag ) );
		cg.lastWeapon = cg.predictedPlayerState.stats[STAT_PENDING_WEAPON];
	}
}

/*
* CG_Cmd_PrevWeapon_f
*/
static void CG_Cmd_PrevWeapon_f( void )
{
	gsitem_t *item;

	if( !cg.frame.valid )
		return;

	if( cgs.demoPlaying || cg.predictedPlayerState.pmove.pm_type == PM_CHASECAM )
	{
		CG_ChaseStep( -1 );
		return;
	}

	item = GS_Cmd_PrevWeapon_f( &cg.frame.playerState, cg.predictedWeaponSwitch );
	if( item )
	{
		CG_Predict_ChangeWeapon( item->tag );
		trap_Cmd_ExecuteText( EXEC_NOW, va( "cmd use %i", item->tag ) );
		cg.lastWeapon = cg.predictedPlayerState.stats[STAT_PENDING_WEAPON];
	}
}

/*
* CG_Cmd_PrevWeapon_f
*/
static void CG_Cmd_LastWeapon_f( void )
{
	gsitem_t *item;

	if( !cg.frame.valid || cgs.demoPlaying )
		return;

	if( cg.lastWeapon != WEAP_NONE && cg.lastWeapon != cg.predictedPlayerState.stats[STAT_PENDING_WEAPON] )
	{
		item = GS_Cmd_UseItem( &cg.frame.playerState, va( "%i", cg.lastWeapon ), IT_WEAPON );
		if( item )
		{
			if( item->type & IT_WEAPON )
				CG_Predict_ChangeWeapon( item->tag );

			trap_Cmd_ExecuteText( EXEC_NOW, va( "cmd use %i", item->tag ) );
			cg.lastWeapon = cg.predictedPlayerState.stats[STAT_PENDING_WEAPON];
		}
	}
}

/*
* CG_Viewpos_f
*/
static void CG_Viewpos_f( void )
{
	CG_Printf( "\"origin\" \"%i %i %i\"\n", (int)cg.view.origin[0], (int)cg.view.origin[1], (int)cg.view.origin[2] );
	CG_Printf( "\"angles\" \"%i %i %i\"\n", (int)cg.view.angles[0], (int)cg.view.angles[1], (int)cg.view.angles[2] );
}

// local cgame commands
typedef struct
{
	char *name;
	void ( *func )( void );
	qboolean allowdemo;
} cgcmd_t;

static cgcmd_t cgcmds[] =
{
	{ "score", CG_ToggleScores_f, qtrue },
	{ "+scores", CG_ScoresOn_f, qtrue },
	{ "-scores", CG_ScoresOff_f, qtrue },
	{ "demoget", CG_Cmd_DemoGet_f, qfalse },
	{ "demolist", NULL, qfalse },
	{ "use", CG_Cmd_UseItem_f, qfalse },
	{ "weapnext", CG_Cmd_NextWeapon_f, qtrue },
	{ "weapprev", CG_Cmd_PrevWeapon_f, qtrue },
	{ "weaplast", CG_Cmd_LastWeapon_f, qtrue },
	{ "viewpos", CG_Viewpos_f, qtrue },
	{ "players", NULL, qfalse },
	{ "spectators", NULL, qfalse },

	{ NULL, NULL }
};

/*
* CG_RegisterCGameCommands
*/
void CG_RegisterCGameCommands( void )
{
	int i;
	char *name;
	cgcmd_t *cmd;

	CG_LoadingString( "commands" );

	if( !cgs.demoPlaying )
	{
		// add game side commands
		for( i = 0; i < MAX_GAMECOMMANDS; i++ )
		{
			name = cgs.configStrings[CS_GAMECOMMANDS+i];
			if( !name[0] )
				continue;

			CG_LoadingItemName( name );

			// check for local command overrides
			for( cmd = cgcmds; cmd->name; cmd++ )
			{
				if( !Q_stricmp( cmd->name, name ) )
					break;
			}
			if( cmd->name )
				continue;

			if( !cgs.hasGametypeMenu && !Q_stricmp( name, "gametypemenu" ) ) {
				cgs.hasGametypeMenu = qtrue;
			}

			trap_Cmd_AddCommand( name, NULL );
		}
	}

	// add local commands
	for( cmd = cgcmds; cmd->name; cmd++ )
	{
		if( cgs.demoPlaying && !cmd->allowdemo )
			continue;
		trap_Cmd_AddCommand( cmd->name, cmd->func );
	}
}

/*
* CG_UnregisterCGameCommands
*/
void CG_UnregisterCGameCommands( void )
{
	int i;
	char *name;
	cgcmd_t *cmd;

	if( !cgs.demoPlaying )
	{
		// remove game commands
		for( i = 0; i < MAX_GAMECOMMANDS; i++ )
		{
			name = cgs.configStrings[CS_GAMECOMMANDS+i];
			if( !name[0] )
				continue;

			// check for local command overrides so we don't try
			// to unregister them twice
			for( cmd = cgcmds; cmd->name; cmd++ )
			{
				if( !Q_stricmp( cmd->name, name ) )
					break;
			}
			if( cmd->name )
				continue;

			trap_Cmd_RemoveCommand( name );
		}

		cgs.hasGametypeMenu = qfalse;
	}

	// remove local commands
	for( cmd = cgcmds; cmd->name; cmd++ )
	{
		if( cgs.demoPlaying && !cmd->allowdemo )
			continue;
		trap_Cmd_RemoveCommand( cmd->name );
	}
}
