/*
Copyright (C) 2006 Pekka Lampila ("Medar"), Damien Deville ("Pb")
and German Garcia Fernandez ("Jal") for Chasseur de bots association.


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

//===================================================================

int clientVoted[MAX_CLIENTS];

cvar_t *g_callvote_electpercentage;
cvar_t *g_callvote_electtime;          // in seconds
cvar_t *g_callvote_enabled;

enum
{
	VOTED_NOTHING = 0,
	VOTED_YES,
	VOTED_NO
};

// Data that can be used by the vote specific functions
typedef struct
{
	edict_t	*caller;
	qboolean operatorcall;
	struct callvotetype_s *callvote;
	int argc;
	char *argv[MAX_STRING_TOKENS];
	char *string;               // can be used to overwrite the displayed vote string
	void *data;                 // any data vote wants to carry over multiple calls of validate and to execute
} callvotedata_t;

typedef struct callvotetype_s
{
	char *name;
	int expectedargs;               // -1 = any amount, -2 = any amount except 0
	qboolean ( *validate )( callvotedata_t *data, qboolean first );
	void ( *execute )( callvotedata_t *vote );
	char *( *current )( void );
	void ( *extraHelp )( edict_t *ent );
	char *argument_format;
	char *help;
	struct callvotetype_s *next;
} callvotetype_t;

// Data that will only be used by the common callvote functions
typedef struct
{
	unsigned int timeout;           // time to finish
	callvotedata_t vote;
} callvotestate_t;

callvotestate_t callvoteState;

callvotetype_t *callvotesHeadNode = NULL;

//==============================================
//		Vote specifics
//==============================================

//====================
// map
//====================

static void G_VoteMapExtraHelp( edict_t *ent )
{
	char *s;
	char buffer[MAX_STRING_CHARS];
	char message[MAX_STRING_CHARS / 4 * 3];    // use buffer to send only one print message
	int nummaps, i, start;
	size_t length, msglength;

	// update the maplist
	trap_ML_Update ();

	if( g_enforce_map_pool->integer && strlen( g_map_pool->string ) > 2 )
	{
		G_PrintMsg( ent, "Maps available [map pool enforced]:\n %s\n", g_map_pool->string );
		return;
	}

	// don't use Q_strncatz and Q_strncpyz below because we
	// check length of the message string manually

	memset( message, 0, sizeof( message ) );
	strcpy( message, "- Available maps:" );

	for( nummaps = 0; trap_ML_GetMapByNum( nummaps, NULL, 0 ); nummaps++ )
		;

	if( trap_Cmd_Argc() > 2 )
	{
		start = atoi( trap_Cmd_Argv( 2 ) ) - 1;
		if( start < 0 )
			start = 0;
	}
	else
	{
		start = 0;
	}

	i = start;
	msglength = strlen( message );
	while( trap_ML_GetMapByNum( i, buffer, sizeof( buffer ) ) )
	{
		i++;
		s = buffer;
		length = strlen( s );
		if( msglength + length + 3 >= sizeof( message ) )
			break;

		strcat( message, " " );
		strcat( message, s );

		msglength += length + 1;
	}

	if( i == start )
		strcat( message, "\nNone" );

	G_PrintMsg( ent, "%s", message );
	G_PrintMsg( ent, "\n", message );

	if( i < nummaps )
		G_PrintMsg( ent, "Type 'callvote map %i' for more maps\n", i+1 );
}

static qboolean G_VoteMapValidate( callvotedata_t *data, qboolean first )
{
	char mapname[MAX_CONFIGSTRING_CHARS];
    char *map;
    int mapnumber;

	if( !first )  // map can't become invalid while voting
		return qtrue;

	// racesow : vote a map number
    mapnumber = atoi( data->argv[0] );

    if( !Q_stricmp( data->argv[0], va( "%i", mapnumber ) ) && ( mapnumber <= mapcount ) )
    {
        map = RS_GetMapByNum(mapnumber);

        if ( map != NULL )
        {
            G_Free( data->argv[0] );
            data->argv[0] = G_Malloc( strlen (map) + 1 );
            Q_strncpyz( data->argv[0], map, strlen (map) + 1 );
        }

        free(map);
    }
    // !racesow

	if( strlen( "maps/" ) + strlen( data->argv[0] ) + strlen( ".bsp" ) >= MAX_CONFIGSTRING_CHARS )
	{
		G_PrintMsg( data->caller, "%sToo long map name\n", S_COLOR_RED );
		return qfalse;
	}

	Q_strncpyz( mapname, data->argv[0], sizeof( mapname ) );
	COM_SanitizeFilePath( mapname );

	if( !Q_stricmp( level.mapname, mapname ) )
	{
		G_PrintMsg( data->caller, "%sYou are already on that map\n", S_COLOR_RED );
		return qfalse;
	}

	if( !COM_ValidateRelativeFilename( mapname ) || strchr( mapname, '/' ) || strchr( mapname, '.' ) )
	{
		G_PrintMsg( data->caller, "%sInvalid map name\n", S_COLOR_RED );
		return qfalse;
	}

	if( trap_ML_FilenameExists( mapname ) )
	{
		// check if valid map is in map pool when on
		if( g_enforce_map_pool->integer )
		{
			char *s, *tok;
			static const char *seps = " ,";

			// if map pool is empty, basically turn it off
			if( strlen(maplist) < 2 ) //racesow : use maplist
				return qtrue;

			s = G_CopyString(maplist); //racesow : use maplist
			tok = strtok( s, seps );
			while ( tok != NULL )
			{
				if ( !Q_stricmp( tok, mapname ) )
				{
					G_Free( s );
					return qtrue;
				}
				else
					tok = strtok( NULL, seps );
			}
			G_Free( s );
			G_PrintMsg( data->caller, "%sMap is not in map pool.\n", S_COLOR_RED );
			return qfalse;
		}
		else
			return qtrue;
	}

	G_PrintMsg( data->caller, "%sNo such map available on this server\n", S_COLOR_RED );

	return qfalse;
}

static void G_VoteMapPassed( callvotedata_t *vote )
{
	Q_strncpyz( level.forcemap, Q_strlwr( vote->argv[0] ), sizeof( level.forcemap ) );
	G_EndMatch();
}

static char *G_VoteMapCurrent( void )
{
	return level.mapname;
}


//====================
// restart
//====================

static void G_VoteRestartPassed( callvotedata_t *vote )
{
	G_RestartLevel();
}


//====================
// nextmap
//====================

static void G_VoteNextMapPassed( callvotedata_t *vote )
{
	level.forcemap[0] = 0;
	G_EndMatch();
}


//====================
// scorelimit
//====================

static qboolean G_VoteScorelimitValidate( callvotedata_t *vote, qboolean first )
{
	int scorelimit = atoi( vote->argv[0] );

	if( scorelimit < 0 )
	{
		if( first ) G_PrintMsg( vote->caller, "%sCan't set negative scorelimit\n", S_COLOR_RED );
		return qfalse;
	}

	if( scorelimit == g_scorelimit->integer )
	{
		if( first )
		{
			G_PrintMsg( vote->caller, "%sScorelimit is already set to %i\n", S_COLOR_RED, scorelimit );
		}
		return qfalse;
	}

	return qtrue;
}

static void G_VoteScorelimitPassed( callvotedata_t *vote )
{
	trap_Cvar_Set( "g_scorelimit", va( "%i", atoi( vote->argv[0] ) ) );
}

static char *G_VoteScorelimitCurrent( void )
{
	return va( "%i", g_scorelimit->integer );
}

//====================
// timelimit
//====================

static qboolean G_VoteTimelimitValidate( callvotedata_t *vote, qboolean first )
{
	int timelimit = atoi( vote->argv[0] );

	if( timelimit < 0 )
	{
		if( first ) G_PrintMsg( vote->caller, "%sCan't set negative timelimit\n", S_COLOR_RED );
		return qfalse;
	}

	if( timelimit == g_timelimit->integer )
	{
		if( first ) G_PrintMsg( vote->caller, "%sTimelimit is already set to %i\n", S_COLOR_RED, timelimit );
		return qfalse;
	}

	return qtrue;
}

static void G_VoteTimelimitPassed( callvotedata_t *vote )
{
	trap_Cvar_Set( "g_timelimit", va( "%i", atoi( vote->argv[0] ) ) );
}

static char *G_VoteTimelimitCurrent( void )
{
	return va( "%i", g_timelimit->integer );
}


//====================
// gametype
//====================

static void G_VoteGametypeExtraHelp( edict_t *ent )
{
	char message[2048], *name; // use buffer to send only one print message
	int count;

	message[0] = 0;

	if( ( g_gametype->latched_string && strlen( g_gametype->latched_string ) > 0 ) &&
		( G_Gametype_Exists( g_gametype->latched_string ) ) )
	{
		Q_strncatz( message, "- Will be changed to: ", sizeof( message ) );
		Q_strncatz( message, g_gametype->latched_string, sizeof( message ) );
		Q_strncatz( message, "\n", sizeof( message ) );
	}

	Q_strncatz( message, "- Available gametypes:", sizeof( message ) );

	for( count = 0; ( name = G_ListNameForPosition( g_gametypes_list->string, count, CHAR_GAMETYPE_SEPARATOR ) ) != NULL; count++ )
	{
		if( G_Gametype_IsVotable( name ) )
		{
			Q_strncatz( message, " ", sizeof( message ) );
			Q_strncatz( message, name, sizeof( message ) );
		}
	}

	G_PrintMsg( ent, "%s\n", message );
}


static qboolean G_VoteGametypeValidate( callvotedata_t *vote, qboolean first )
{
	if( !G_Gametype_Exists( vote->argv[0] ) )
	{
		if( first ) G_PrintMsg( vote->caller, "%sgametype %s is not available\n", S_COLOR_RED, vote->argv[0] );
		return qfalse;
	}

	if( g_gametype->latched_string && G_Gametype_Exists( g_gametype->latched_string ) )
	{
		if( ( GS_MatchState() > MATCH_STATE_PLAYTIME ) &&
			!Q_stricmp( vote->argv[0], g_gametype->latched_string ) )
		{
			if( first )
				G_PrintMsg( vote->caller, "%s%s is already the next gametype\n", S_COLOR_RED, vote->argv[0] );
			return qfalse;
		}
	}

	if( ( GS_MatchState() <= MATCH_STATE_PLAYTIME || g_gametype->latched_string == NULL )
		&& !Q_stricmp( gs.gametypeName, vote->argv[0]) )
	{
		if( first )
			G_PrintMsg( vote->caller, "%s%s is the current gametype\n", S_COLOR_RED, vote->argv[0] );
		return qfalse;
	}

	// if the g_votable_gametypes is empty, allow all gametypes
	if( !G_Gametype_IsVotable( vote->argv[0] ) )
	{
		if( first )
		{
			G_PrintMsg( vote->caller, "%sVoting gametype %s is not allowed on this server\n",
				S_COLOR_RED, vote->argv[0] );
		}
		return qfalse;
	}

	return qtrue;
}

static void G_VoteGametypePassed( callvotedata_t *vote )
{
	char *gametype_string;
	char next_gametype_string[MAX_STRING_TOKENS];

	gametype_string = vote->argv[0];
	Q_strncpyz( next_gametype_string, gametype_string, sizeof( next_gametype_string ) );

	trap_Cvar_Set( "g_gametype", gametype_string );

	if( GS_MatchState() == MATCH_STATE_COUNTDOWN ||
		GS_MatchState() == MATCH_STATE_PLAYTIME || !G_RespawnLevel() )
	{
		// go thought scoreboard if in game
		Q_strncpyz( level.forcemap, level.mapname, sizeof( level.forcemap ) );
		G_EndMatch();
	}

	// we can't use gametype_string here, because there's a big chance it has just been freed after G_EndMatch
	G_PrintMsg( NULL, "Gametype changed to %s\n", next_gametype_string );
}

static char *G_VoteGametypeCurrent( void )
{
	return gs.gametypeName;
}


//====================
// warmup_timelimit
//====================

static qboolean G_VoteWarmupTimelimitValidate( callvotedata_t *vote, qboolean first )
{
	int warmup_timelimit = atoi( vote->argv[0] );

	if( warmup_timelimit < 0 )
	{
		if( first ) G_PrintMsg( vote->caller, "%sCan't set negative warmup timelimit\n", S_COLOR_RED );
		return qfalse;
	}

	if( warmup_timelimit == g_warmup_timelimit->integer )
	{
		if( first )
			G_PrintMsg( vote->caller, "%sWarmup timelimit is already set to %i\n", S_COLOR_RED, warmup_timelimit );
		return qfalse;
	}

	return qtrue;
}

static void G_VoteWarmupTimelimitPassed( callvotedata_t *vote )
{
	trap_Cvar_Set( "g_warmup_timelimit", va( "%i", atoi( vote->argv[0] ) ) );
}

static char *G_VoteWarmupTimelimitCurrent( void )
{
	return va( "%i", g_warmup_timelimit->integer );
}


//====================
// extended_time
//====================

static qboolean G_VoteExtendedTimeValidate( callvotedata_t *vote, qboolean first )
{
	int extended_time = atoi( vote->argv[0] );

	if( extended_time < 0 )
	{
		if( first ) G_PrintMsg( vote->caller, "%sCan't set negative extended time\n", S_COLOR_RED );
		return qfalse;
	}

	if( extended_time == g_match_extendedtime->integer )
	{
		if( first )
			G_PrintMsg( vote->caller, "%sExtended time is already set to %i\n", S_COLOR_RED, extended_time );
		return qfalse;
	}

	return qtrue;
}

static void G_VoteExtendedTimePassed( callvotedata_t *vote )
{
	trap_Cvar_Set( "g_match_extendedtime", va( "%i", atoi( vote->argv[0] ) ) );
}

static char *G_VoteExtendedTimeCurrent( void )
{
	return va( "%i", g_match_extendedtime->integer );
}

//====================
// allready
//====================

static qboolean G_VoteAllreadyValidate( callvotedata_t *vote, qboolean first )
{
	int notreadys = 0;
	edict_t	*ent;

	if( GS_MatchState() >= MATCH_STATE_COUNTDOWN )
	{
		if( first ) G_PrintMsg( vote->caller, "%sThe game is not in warmup mode\n", S_COLOR_RED );
		return qfalse;
	}

	for( ent = game.edicts+1; PLAYERNUM( ent ) < gs.maxclients; ent++ )
	{
		if( trap_GetClientState( PLAYERNUM( ent ) ) < CS_SPAWNED )
			continue;

		if( ent->s.team > TEAM_SPECTATOR && !level.ready[PLAYERNUM( ent )] )
			notreadys++;
	}

	if( !notreadys )
	{
		if( first ) G_PrintMsg( vote->caller, "%sEveryone is already ready\n", S_COLOR_RED );
		return qfalse;
	}

	return qtrue;
}

static void G_VoteAllreadyPassed( callvotedata_t *vote )
{
	edict_t	*ent;

	for( ent = game.edicts+1; PLAYERNUM( ent ) < gs.maxclients; ent++ )
	{
		if( trap_GetClientState( PLAYERNUM( ent ) ) < CS_SPAWNED )
			continue;

		if( ent->s.team > TEAM_SPECTATOR && !level.ready[PLAYERNUM( ent )] )
		{
			level.ready[PLAYERNUM( ent )] = qtrue;
			G_UpdatePlayerMatchMsg( ent );
			G_Match_CheckReadys();
		}
	}
}

//====================
// maxteamplayers
//====================

static qboolean G_VoteMaxTeamplayersValidate( callvotedata_t *vote, qboolean first )
{
	int maxteamplayers = atoi( vote->argv[0] );

	if( maxteamplayers < 1 )
	{
		if( first )
		{
			G_PrintMsg( vote->caller, "%sThe maximum number of players in team can't be less than 1\n",
				S_COLOR_RED );
		}
		return qfalse;
	}

	if( g_teams_maxplayers->integer == maxteamplayers )
	{
		if( first )
		{
			G_PrintMsg( vote->caller, "%sMaximum number of players in team is already %i\n",
				S_COLOR_RED, maxteamplayers );
		}
		return qfalse;
	}

	return qtrue;
}

static void G_VoteMaxTeamplayersPassed( callvotedata_t *vote )
{
	trap_Cvar_Set( "g_teams_maxplayers", va( "%i", atoi( vote->argv[0] ) ) );
}

static char *G_VoteMaxTeamplayersCurrent( void )
{
	return va( "%i", g_teams_maxplayers->integer );
}

//====================
// lock
//====================

static qboolean G_VoteLockValidate( callvotedata_t *vote, qboolean first )
{
	if( GS_MatchState() > MATCH_STATE_PLAYTIME )
	{
		if( first ) G_PrintMsg( vote->caller, "%sCan't lock teams after the match\n", S_COLOR_RED );
		return qfalse;
	}

	if( level.teamlock )
	{
		if( GS_MatchState() < MATCH_STATE_COUNTDOWN && first )
		{
			G_PrintMsg( vote->caller, "%sTeams are already set to be locked on match start\n", S_COLOR_RED );
		}
		else if( first )
		{
			G_PrintMsg( vote->caller, "%sTeams are already locked\n", S_COLOR_RED );
		}
		return qfalse;
	}

	return qtrue;
}

static void G_VoteLockPassed( callvotedata_t *vote )
{
	int team;

	level.teamlock = qtrue;

	// if we are inside a match, update the teams state
	if( GS_MatchState() >= MATCH_STATE_COUNTDOWN && GS_MatchState() <= MATCH_STATE_PLAYTIME )
	{
		if( GS_TeamBasedGametype() )
		{
			for( team = TEAM_ALPHA; team < GS_MAX_TEAMS; team++ )
				G_Teams_LockTeam( team );
		}
		else
		{
			G_Teams_LockTeam( TEAM_PLAYERS );
		}
		G_PrintMsg( NULL, "Teams locked\n" );
	}
	else
	{
		G_PrintMsg( NULL, "Teams will be locked when the match starts\n" );
	}
}

//====================
// unlock
//====================

static qboolean G_VoteUnlockValidate( callvotedata_t *vote, qboolean first )
{
	if( GS_MatchState() > MATCH_STATE_PLAYTIME )
	{
		if( first ) G_PrintMsg( vote->caller, "%sCan't unlock teams after the match\n", S_COLOR_RED );
		return qfalse;
	}

	if( !level.teamlock )
	{
		if( GS_MatchState() < MATCH_STATE_COUNTDOWN && first )
		{
			G_PrintMsg( vote->caller, "%sTeams are not set to be locked\n", S_COLOR_RED );
		}
		else if( first )
		{
			G_PrintMsg( vote->caller, "%sTeams are not locked\n", S_COLOR_RED );
		}
		return qfalse;
	}

	return qtrue;
}

static void G_VoteUnlockPassed( callvotedata_t *vote )
{
	int team;

	level.teamlock = qfalse;

	// if we are inside a match, update the teams state
	if( GS_MatchState() >= MATCH_STATE_COUNTDOWN && GS_MatchState() <= MATCH_STATE_PLAYTIME )
	{
		if( GS_TeamBasedGametype() )
		{
			for( team = TEAM_ALPHA; team < GS_MAX_TEAMS; team++ )
				G_Teams_UnLockTeam( team );
		}
		else
		{
			G_Teams_UnLockTeam( TEAM_PLAYERS );
		}
		G_PrintMsg( NULL, "Teams unlocked\n" );
	}
	else
	{
		G_PrintMsg( NULL, "Teams will no longer be locked when the match starts\n" );
	}
}

//====================
// remove
//====================

static void G_VoteRemoveExtraHelp( edict_t *ent )
{
	int i;
	edict_t *e;
	char msg[1024];

	msg[0] = 0;
	Q_strncatz( msg, "- List of players in game:\n", sizeof( msg ) );

	if( GS_TeamBasedGametype() )
	{
		int team;

		for( team = TEAM_ALPHA; team < GS_MAX_TEAMS; team++ )
		{
			Q_strncatz( msg, va( "%s:\n", GS_TeamName( team ) ), sizeof( msg ) );
			for( i = 0, e = game.edicts+1; i < gs.maxclients; i++, e++ )
			{
				if( !e->r.inuse || e->s.team != team )
					continue;

				Q_strncatz( msg, va( "%3i: %s\n", PLAYERNUM( e ), e->r.client->netname ), sizeof( msg ) );
			}
		}
	}
	else
	{
		for( i = 0, e = game.edicts+1; i < gs.maxclients; i++, e++ )
		{
			if( !e->r.inuse || e->s.team != TEAM_PLAYERS )
				continue;

			Q_strncatz( msg, va( "%3i: %s\n", PLAYERNUM( e ), e->r.client->netname ), sizeof( msg ) );
		}
	}

	G_PrintMsg( ent, "%s", msg );
}

static qboolean G_VoteRemoveValidate( callvotedata_t *vote, qboolean first )
{
	int who = -1;

	if( first )
	{
		edict_t *tokick = G_PlayerForText( vote->argv[0] );

		if( tokick )
			who = PLAYERNUM( tokick );
		else
			who = -1;

		if( who == -1 )
		{
			G_PrintMsg( vote->caller, "%sNo such player\n", S_COLOR_RED );
			return qfalse;
		}
		else if( tokick->s.team == TEAM_SPECTATOR )
		{
			G_PrintMsg( vote->caller, "Player %s%s%s is already spectator.\n", S_COLOR_WHITE,
				tokick->r.client->netname, S_COLOR_RED );

			return qfalse;
		}
		else
		{
			// we save the player id to be removed, so we don't later get confused by new ids or players changing names
			vote->data = G_Malloc( sizeof( int ) );
			memcpy( vote->data, &who, sizeof( int ) );
		}
	}
	else
	{
		memcpy( &who, vote->data, sizeof( int ) );
	}

	if( !game.edicts[who+1].r.inuse || game.edicts[who+1].s.team == TEAM_SPECTATOR )
	{
		return qfalse;
	}
	else
	{
		if( !vote->string || Q_stricmp( vote->string, game.edicts[who+1].r.client->netname ) )
		{
			if( vote->string ) G_Free( vote->string );
			vote->string = G_CopyString( game.edicts[who+1].r.client->netname );
		}

		return qtrue;
	}
}

static void G_VoteRemovePassed( callvotedata_t *vote )
{
	int who;
	edict_t *ent;

	memcpy( &who, vote->data, sizeof( int ) );
	ent = &game.edicts[who+1];

	// may have disconnect along the callvote time
	if( !ent->r.inuse || !ent->r.client || ent->s.team == TEAM_SPECTATOR )
		return;

	G_PrintMsg( NULL, "Player %s%s removed from team %s%s.\n", ent->r.client->netname, S_COLOR_WHITE,
		GS_TeamName( ent->s.team ), S_COLOR_WHITE );

	G_Teams_SetTeam( ent, TEAM_SPECTATOR );
	ent->r.client->queueTimeStamp = 0;
}


//====================
// kick
//====================

static void G_VoteKickExtraHelp( edict_t *ent )
{
	int i;
	edict_t *e;
	char msg[1024];

	msg[0] = 0;
	Q_strncatz( msg, "- List of current players:\n", sizeof( msg ) );

	for( i = 0, e = game.edicts+1; i < gs.maxclients; i++, e++ )
	{
		if( !e->r.inuse )
			continue;

		Q_strncatz( msg, va( "%3i: %s\n", PLAYERNUM( e ), e->r.client->netname ), sizeof( msg ) );
	}

	G_PrintMsg( ent, "%s", msg );
}

static qboolean G_VoteKickValidate( callvotedata_t *vote, qboolean first )
{
	int who = -1;

	if( first )
	{
		edict_t *tokick = G_PlayerForText( vote->argv[0] );

		if( tokick )
			who = PLAYERNUM( tokick );
		else
			who = -1;

		if( who != -1 )
		{
			if( game.edicts[who+1].r.client->isoperator )
			{
				G_PrintMsg( vote->caller, S_COLOR_RED"%s is a game operator.\n", game.edicts[who+1].r.client->netname );
				return qfalse;
			}

			// we save the player id to be kicked, so we don't later get
			//confused by new ids or players changing names

			vote->data = G_Malloc( sizeof( int ) );
			memcpy( vote->data, &who, sizeof( int ) );
		}
		else
		{
			G_PrintMsg( vote->caller, "%sNo such player\n", S_COLOR_RED );
			return qfalse;
		}
	}
	else
	{
		memcpy( &who, vote->data, sizeof( int ) );
	}

	if( !game.edicts[who+1].r.inuse )
	{
		return qfalse;
	}
	else
	{
		if( !vote->string || Q_stricmp( vote->string, game.edicts[who+1].r.client->netname ) )
		{
			if( vote->string ) 
				G_Free( vote->string );

			vote->string = G_CopyString( game.edicts[who+1].r.client->netname );
		}

		return qtrue;
	}
}

static void G_VoteKickPassed( callvotedata_t *vote )
{
	int who;
	edict_t *ent;

	memcpy( &who, vote->data, sizeof( int ) );
	ent = &game.edicts[who+1];
	if( !ent->r.inuse || !ent->r.client )  // may have disconnected along the callvote time
		return;

	trap_DropClient( ent, DROP_TYPE_NORECONNECT, "Kicked" );
}


//====================
// kickban
//====================

static void G_VoteKickBanExtraHelp( edict_t *ent )
{
	int i;
	edict_t *e;
	char msg[1024];

	msg[0] = 0;
	Q_strncatz( msg, "- List of current players:\n", sizeof( msg ) );

	for( i = 0, e = game.edicts+1; i < gs.maxclients; i++, e++ )
	{
		if( !e->r.inuse )
			continue;

		Q_strncatz( msg, va( "%3i: %s\n", PLAYERNUM( e ), e->r.client->netname ), sizeof( msg ) );
	}

	G_PrintMsg( ent, "%s", msg );
}

static qboolean G_VoteKickBanValidate( callvotedata_t *vote, qboolean first )
{
	int who = -1;

	if( !filterban->integer )
	{
		G_PrintMsg( vote->caller, "%sFilterban is disabled on this server\n", S_COLOR_RED );
		return qfalse;
	}

	if( first )
	{
		edict_t *tokick = G_PlayerForText( vote->argv[0] );

		if( tokick )
			who = PLAYERNUM( tokick );
		else
			who = -1;

		if( who != -1 )
		{
			if( game.edicts[who+1].r.client->isoperator )
			{
				G_PrintMsg( vote->caller, S_COLOR_RED"%s is a game operator.\n", game.edicts[who+1].r.client->netname );
				return qfalse;
			}

			// we save the player id to be kicked, so we don't later get
			// confused by new ids or players changing names

			vote->data = G_Malloc( sizeof( int ) );
			memcpy( vote->data, &who, sizeof( int ) );
		}
		else
		{
			G_PrintMsg( vote->caller, "%sNo such player\n", S_COLOR_RED );
			return qfalse;
		}
	}
	else
	{
		memcpy( &who, vote->data, sizeof( int ) );
	}

	if( !game.edicts[who+1].r.inuse )
	{
		return qfalse;
	}
	else
	{
		if( !vote->string || Q_stricmp( vote->string, game.edicts[who+1].r.client->netname ) )
		{
			if( vote->string ) 
				G_Free( vote->string );

			vote->string = G_CopyString( game.edicts[who+1].r.client->netname );
		}

		return qtrue;
	}
}

static void G_VoteKickBanPassed( callvotedata_t *vote )
{
	int who;
	edict_t *ent;

	memcpy( &who, vote->data, sizeof( int ) );
	ent = &game.edicts[who+1];
	if( !ent->r.inuse || !ent->r.client )  // may have disconnected along the callvote time
		return;

	trap_Cmd_ExecuteText( EXEC_APPEND, va( "addip %s %i\n", ent->r.client->ip, 15 ) );
	trap_DropClient( ent, DROP_TYPE_NORECONNECT, "Kicked" );
}

//====================
// mute
//====================

static void G_VoteMuteExtraHelp( edict_t *ent )
{
	int i;
	edict_t *e;
	char msg[1024];

	msg[0] = 0;
	Q_strncatz( msg, "- List of current players:\n", sizeof( msg ) );

	for( i = 0, e = game.edicts+1; i < gs.maxclients; i++, e++ )
	{
		if( !e->r.inuse )
			continue;

		Q_strncatz( msg, va( "%3i: %s\n", PLAYERNUM( e ), e->r.client->netname ), sizeof( msg ) );
	}

	G_PrintMsg( ent, "%s", msg );
}

static qboolean G_VoteMuteValidate( callvotedata_t *vote, qboolean first )
{
	int who = -1;

	if( first )
	{
		edict_t *tomute = G_PlayerForText( vote->argv[0] );

		if( tomute )
			who = PLAYERNUM( tomute );
		else
			who = -1;

		if( who != -1 )
		{
			// we save the player id to be kicked, so we don't later get confused by new ids or players changing names
			vote->data = G_Malloc( sizeof( int ) );
			memcpy( vote->data, &who, sizeof( int ) );
		}
		else
		{
			G_PrintMsg( vote->caller, "%sNo such player\n", S_COLOR_RED );
			return qfalse;
		}
	}
	else
	{
		memcpy( &who, vote->data, sizeof( int ) );
	}

	if( !game.edicts[who+1].r.inuse )
	{
		return qfalse;
	}
	else
	{
		if( !vote->string || Q_stricmp( vote->string, game.edicts[who+1].r.client->netname ) )
		{
			if( vote->string ) G_Free( vote->string );
			vote->string = G_CopyString( game.edicts[who+1].r.client->netname );
		}

		return qtrue;
	}
}

// chat mute
static void G_VoteMutePassed( callvotedata_t *vote )
{
	int who;
	edict_t *ent;

	memcpy( &who, vote->data, sizeof( int ) );
	ent = &game.edicts[who+1];
	if( !ent->r.inuse || !ent->r.client )  // may have disconnect along the callvote time
		return;

	ent->r.client->muted |= 1;
}

// vsay mute
static void G_VoteVMutePassed( callvotedata_t *vote )
{
	int who;
	edict_t *ent;

	memcpy( &who, vote->data, sizeof( int ) );
	ent = &game.edicts[who+1];
	if( !ent->r.inuse || !ent->r.client )  // may have disconnect along the callvote time
		return;

	ent->r.client->muted |= 2;
}

//====================
// unmute
//====================

static void G_VoteUnmuteExtraHelp( edict_t *ent )
{
	int i;
	edict_t *e;
	char msg[1024];

	msg[0] = 0;
	Q_strncatz( msg, "- List of current players:\n", sizeof( msg ) );

	for( i = 0, e = game.edicts+1; i < gs.maxclients; i++, e++ )
	{
		if( !e->r.inuse )
			continue;

		Q_strncatz( msg, va( "%3i: %s\n", PLAYERNUM( e ), e->r.client->netname ), sizeof( msg ) );
	}

	G_PrintMsg( ent, "%s", msg );
}

static qboolean G_VoteUnmuteValidate( callvotedata_t *vote, qboolean first )
{
	int who = -1;

	if( first )
	{
		edict_t *tomute = G_PlayerForText( vote->argv[0] );

		if( tomute )
			who = PLAYERNUM( tomute );
		else
			who = -1;

		if( who != -1 )
		{
			// we save the player id to be kicked, so we don't later get confused by new ids or players changing names
			vote->data = G_Malloc( sizeof( int ) );
			memcpy( vote->data, &who, sizeof( int ) );
		}
		else
		{
			G_PrintMsg( vote->caller, "%sNo such player\n", S_COLOR_RED );
			return qfalse;
		}
	}
	else
	{
		memcpy( &who, vote->data, sizeof( int ) );
	}

	if( !game.edicts[who+1].r.inuse )
	{
		return qfalse;
	}
	else
	{
		if( !vote->string || Q_stricmp( vote->string, game.edicts[who+1].r.client->netname ) )
		{
			if( vote->string ) G_Free( vote->string );
			vote->string = G_CopyString( game.edicts[who+1].r.client->netname );
		}

		return qtrue;
	}
}

// chat unmute
static void G_VoteUnmutePassed( callvotedata_t *vote )
{
	int who;
	edict_t *ent;

	memcpy( &who, vote->data, sizeof( int ) );
	ent = &game.edicts[who+1];
	if( !ent->r.inuse || !ent->r.client )  // may have disconnect along the callvote time
		return;

	ent->r.client->muted &= ~1;
}

// vsay unmute
static void G_VoteVUnmutePassed( callvotedata_t *vote )
{
	int who;
	edict_t *ent;

	memcpy( &who, vote->data, sizeof( int ) );
	ent = &game.edicts[who+1];
	if( !ent->r.inuse || !ent->r.client )  // may have disconnect along the callvote time
		return;

	ent->r.client->muted &= ~2;
}

//====================
// addbots
//====================

static qboolean G_VoteNumBotsValidate( callvotedata_t *vote, qboolean first )
{
	int numbots = atoi( vote->argv[0] );

	if( g_numbots->integer == numbots )
	{
		if( first ) G_PrintMsg( vote->caller, "%sNumber of bots is already %i\n", S_COLOR_RED, numbots );
		return qfalse;
	}

	if( numbots < 0 )
	{
		if( first ) G_PrintMsg( vote->caller, "%sNegative number of bots is not allowed\n", S_COLOR_RED );
		return qfalse;
	}

	if( numbots > gs.maxclients )
	{
		if( first )
		{
			G_PrintMsg( vote->caller, "%sNumber of bots can't be higher than the number of client spots (%i)\n",
				S_COLOR_RED, gs.maxclients );
		}
		return qfalse;
	}

	return qtrue;
}

static void G_VoteNumBotsPassed( callvotedata_t *vote )
{
	trap_Cvar_Set( "g_numbots", vote->argv[0] );
}

static char *G_VoteNumBotsCurrent( void )
{
	return va( "%i", g_numbots->integer );
}

//====================
// allow_teamdamage
//====================

static qboolean G_VoteAllowTeamDamageValidate( callvotedata_t *vote, qboolean first )
{
	int teamdamage = atoi( vote->argv[0] );

	if( teamdamage != 0 && teamdamage != 1 )
		return qfalse;

	if( teamdamage && g_allow_teamdamage->integer )
	{
		if( first ) G_PrintMsg( vote->caller, "%sTeam damage is already allowed\n", S_COLOR_RED );
		return qfalse;
	}

	if( !teamdamage && !g_allow_teamdamage->integer )
	{
		if( first ) G_PrintMsg( vote->caller, "%sTeam damage is already disabled\n", S_COLOR_RED );
		return qfalse;
	}

	return qtrue;
}

static void G_VoteAllowTeamDamagePassed( callvotedata_t *vote )
{
	trap_Cvar_Set( "g_allow_teamdamage", va( "%i", atoi( vote->argv[0] ) ) );
}

static char *G_VoteAllowTeamDamageCurrent( void )
{
	if( g_allow_teamdamage->integer )
		return "1";
	else
		return "0";
}

//====================
// instajump
//====================

static qboolean G_VoteAllowInstajumpValidate( callvotedata_t *vote, qboolean first )
{
	int instajump = atoi( vote->argv[0] );

	if( instajump != 0 && instajump != 1 )
		return qfalse;

	if( instajump && g_instajump->integer )
	{
		if( first ) 
			G_PrintMsg( vote->caller, "%sInstajump is already allowed\n", S_COLOR_RED );
		return qfalse;
	}

	if( !instajump && !g_instajump->integer )
	{
		if( first ) 
			G_PrintMsg( vote->caller, "%sInstajump is already disabled\n", S_COLOR_RED );
		return qfalse;
	}

	return qtrue;
}

static void G_VoteAllowInstajumpPassed( callvotedata_t *vote )
{
	trap_Cvar_Set( "g_instajump", va( "%i", atoi( vote->argv[0] ) ) );
}

static char *G_VoteAllowInstajumpCurrent( void )
{
	if( g_instajump->integer )
		return "1";
	else
		return "0";
}

//====================
// instashield
//====================

static qboolean G_VoteAllowInstashieldValidate( callvotedata_t *vote, qboolean first )
{
	int instashield = atoi( vote->argv[0] );

	if( instashield != 0 && instashield != 1 )
		return qfalse;

	if( instashield && g_instashield->integer )
	{
		if( first ) 
			G_PrintMsg( vote->caller, "%sInstashield is already allowed\n", S_COLOR_RED );
		return qfalse;
	}

	if( !instashield && !g_instashield->integer )
	{
		if( first ) 
			G_PrintMsg( vote->caller, "%sInstashield is already disabled\n", S_COLOR_RED );
		return qfalse;
	}

	return qtrue;
}

static void G_VoteAllowInstashieldPassed( callvotedata_t *vote )
{
	trap_Cvar_Set( "g_instashield", va( "%i", atoi( vote->argv[0] ) ) );

	// remove the shield from all players
	if( !g_instashield->integer )
	{
		int i;

		for( i = 0; i < gs.maxclients; i++ )
		{
			if( trap_GetClientState( i ) < CS_SPAWNED )
				continue;

			game.clients[i].ps.inventory[POWERUP_SHELL] = 0;
		}
	}
}

static char *G_VoteAllowInstashieldCurrent( void )
{
	if( g_instashield->integer )
		return "1";
	else
		return "0";
}

//====================
// allow_falldamage
//====================

static qboolean G_VoteAllowFallDamageValidate( callvotedata_t *vote, qboolean first )
{
	int falldamage = atoi( vote->argv[0] );

	if( falldamage != 0 && falldamage != 1 )
		return qfalse;

	if( falldamage && GS_FallDamage() )
	{
		if( first ) G_PrintMsg( vote->caller, "%sFall damage is already allowed\n", S_COLOR_RED );
		return qfalse;
	}

	if( !falldamage && !GS_FallDamage() )
	{
		if( first ) G_PrintMsg( vote->caller, "%sFall damage is already disabled\n", S_COLOR_RED );
		return qfalse;
	}

	return qtrue;
}

static void G_VoteAllowFallDamagePassed( callvotedata_t *vote )
{
	trap_Cvar_Set( "g_allow_falldamage", va( "%i", atoi( vote->argv[0] ) ) );
}

static char *G_VoteAllowFallDamageCurrent( void )
{
	if( GS_FallDamage() )
		return "1";
	else
		return "0";
}

//====================
// allow_selfdamage
//====================

static qboolean G_VoteAllowSelfDamageValidate( callvotedata_t *vote, qboolean first )
{
	int selfdamage = atoi( vote->argv[0] );

	if( selfdamage != 0 && selfdamage != 1 )
		return qfalse;

	if( selfdamage && GS_SelfDamage() )
	{
		if( first ) G_PrintMsg( vote->caller, "%sSelf damage is already allowed\n", S_COLOR_RED );
		return qfalse;
	}

	if( !selfdamage && !GS_SelfDamage() )
	{
		if( first ) G_PrintMsg( vote->caller, "%sSelf damage is already disabled\n", S_COLOR_RED );
		return qfalse;
	}

	return qtrue;
}

static void G_VoteAllowSelfDamagePassed( callvotedata_t *vote )
{
	trap_Cvar_Set( "g_allow_selfdamage", va( "%i", atoi( vote->argv[0] ) ) );
}

static char *G_VoteAllowSelfDamageCurrent( void )
{
	if( GS_SelfDamage() )
		return "1";
	else
		return "0";
}

//====================
// timeout
//====================
static qboolean G_VoteTimeoutValidate( callvotedata_t *vote, qboolean first )
{
	if( GS_MatchPaused() && ( level.timeout.endtime - level.timeout.time ) >= 2 * TIMEIN_TIME )
	{
		if( first ) G_PrintMsg( vote->caller, "%sTimeout already in progress\n", S_COLOR_RED );
		return qfalse;
	}

	return qtrue;
}

static void G_VoteTimeoutPassed( callvotedata_t *vote )
{
	if( !GS_MatchPaused() )
		G_AnnouncerSound( NULL, trap_SoundIndex( va( S_ANNOUNCER_TIMEOUT_TIMEOUT_1_to_2, ( rand()&1 )+1 ) ), GS_MAX_TEAMS, qtrue, NULL );

	GS_GamestatSetFlag( GAMESTAT_FLAG_PAUSED, qtrue );
	level.timeout.caller = 0;
	level.timeout.endtime = level.timeout.time + TIMEOUT_TIME + FRAMETIME;
}

//====================
// timein
//====================
static qboolean G_VoteTimeinValidate( callvotedata_t *vote, qboolean first )
{
	if( !GS_MatchPaused() )
	{
		if( first ) G_PrintMsg( vote->caller, "%sNo timeout in progress\n", S_COLOR_RED );
		return qfalse;
	}

	if( level.timeout.endtime - level.timeout.time <= 2 * TIMEIN_TIME )
	{
		if( first ) G_PrintMsg( vote->caller, "%sTimeout is about to end already\n", S_COLOR_RED );
		return qfalse;
	}

	return qtrue;
}

static void G_VoteTimeinPassed( callvotedata_t *vote )
{
	G_AnnouncerSound( NULL, trap_SoundIndex( va( S_ANNOUNCER_TIMEOUT_TIMEIN_1_to_2, ( rand()&1 )+1 ) ), GS_MAX_TEAMS, qtrue, NULL );
	level.timeout.endtime = level.timeout.time + TIMEIN_TIME + FRAMETIME;
}

//====================
// challengers_queue
//====================
static qboolean G_VoteChallengersValidate( callvotedata_t *vote, qboolean first )
{
	int challengers = atoi( vote->argv[0] );

	if( challengers != 0 && challengers != 1 )
		return qfalse;

	if( challengers && g_challengers_queue->integer )
	{
		if( first ) G_PrintMsg( vote->caller, "%sChallengers queue is already enabled\n", S_COLOR_RED );
		return qfalse;
	}

	if( !challengers && !g_challengers_queue->integer )
	{
		if( first ) G_PrintMsg( vote->caller, "%sChallengers queue is already disabled\n", S_COLOR_RED );
		return qfalse;
	}

	return qtrue;
}

static void G_VoteChallengersPassed( callvotedata_t *vote )
{
	trap_Cvar_Set( "g_challengers_queue", va( "%i", atoi( vote->argv[0] ) ) );
}

static char *G_VoteChallengersCurrent( void )
{
	if( g_challengers_queue->integer )
		return "1";
	else
		return "0";
}

//====================
// allow_uneven
//====================
static qboolean G_VoteAllowUnevenValidate( callvotedata_t *vote, qboolean first )
{
	int allow_uneven = atoi( vote->argv[0] );

	if( allow_uneven != 0 && allow_uneven != 1 )
		return qfalse;

	if( allow_uneven && g_teams_allow_uneven->integer )
	{
		if( first ) G_PrintMsg( vote->caller, "%sUneven teams is already allowed.\n", S_COLOR_RED );
		return qfalse;
	}

	if( !allow_uneven && !g_teams_allow_uneven->integer )
	{
		if( first ) G_PrintMsg( vote->caller, "%sUneven teams is already disallowed\n", S_COLOR_RED );
		return qfalse;
	}

	return qtrue;
}

static void G_VoteAllowUnevenPassed( callvotedata_t *vote )
{
	trap_Cvar_Set( "g_teams_allow_uneven", va( "%i", atoi( vote->argv[0] ) ) );
}

static char *G_VoteAllowUnevenCurrent( void )
{
	if( g_teams_allow_uneven->integer )
		return "1";
	else
		return "0";
}

#ifdef ALLOWBYNNY_VOTE
static qboolean G_VoteAllowBunnyValidate( callvotedata_t *vote, qboolean first )
{
	int allow_bunny = atoi( vote->argv[0] );

	if( allow_bunny != 0 && allow_bunny != 1 )
		return qfalse;

	if( allow_bunny && g_allow_bunny->integer )
	{
		if( first ) G_PrintMsg( vote->caller, "%sBunnyhopping already allowed.\n", S_COLOR_RED );
		return qfalse;
	}

	if( !allow_bunny && !g_allow_bunny->integer )
	{
		if( first ) G_PrintMsg( vote->caller, "%sBunnyhopping already disallowed\n", S_COLOR_RED );
		return qfalse;
	}

	return qtrue;
}

static void G_VoteAllowBunnyPassed( callvotedata_t *vote )
{
	trap_Cvar_Set( "g_allow_bunny", va( "%i", atoi( vote->argv[0] ) ) );
}

static char *G_VoteAllowBunnyCurrent( void )
{
	if( g_allow_bunny->integer )
		return "1";
	else
		return "0";
}
#endif

//================================================
//
//================================================


callvotetype_t *G_RegisterCallvote( const char *name )
{
	callvotetype_t *callvote;

	for( callvote = callvotesHeadNode; callvote != NULL; callvote = callvote->next )
	{
		if( !Q_stricmp( callvote->name, name ) )
			return callvote;
	}

	// create a new callvote
	callvote = G_LevelMalloc( sizeof( callvotetype_t ) );
	memset( callvote, 0, sizeof( callvotetype_t ) );
	callvote->next = callvotesHeadNode;
	callvotesHeadNode = callvote;

	callvote->name = G_LevelCopyString( name );
	return callvote;
}

void G_FreeCallvotes( void )
{
	callvotetype_t *callvote;

	while( callvotesHeadNode )
	{
		callvote = callvotesHeadNode->next;

		if( callvotesHeadNode->name )
			G_LevelFree( callvotesHeadNode->name );
		if( callvotesHeadNode->argument_format )
			G_LevelFree( callvotesHeadNode->argument_format );
		if( callvotesHeadNode->help )
			G_LevelFree( callvotesHeadNode->help );

		G_LevelFree( callvotesHeadNode );
		callvotesHeadNode = callvote;
	}

	callvotesHeadNode = NULL;
}

//===================================================================
// Common functions
//===================================================================

/*
* G_CallVotes_Reset
*/
void G_CallVotes_Reset( void )
{
	int i;

	callvoteState.vote.callvote = NULL;
	memset( clientVoted, VOTED_NOTHING, sizeof( clientVoted ) );
	callvoteState.timeout = 0;

	callvoteState.vote.caller = NULL;
	if( callvoteState.vote.string )
	{
		G_Free( callvoteState.vote.string );
	}
	if( callvoteState.vote.data )
	{
		G_Free( callvoteState.vote.data );
	}
	for( i = 0; i < callvoteState.vote.argc; i++ )
	{
		if( callvoteState.vote.argv[i] )
			G_Free( callvoteState.vote.argv[i] );
	}

	memset( &callvoteState, 0, sizeof( callvoteState ) );
}

/*
* G_CallVotes_PrintUsagesToPlayer
*/
static void G_CallVotes_PrintUsagesToPlayer( edict_t *ent )
{
	callvotetype_t *callvote;

	G_PrintMsg( ent, "Available votes:\n" );
	for( callvote = callvotesHeadNode; callvote != NULL; callvote = callvote->next )
	{
		if( trap_Cvar_Value( va( "g_disable_vote_%s", callvote->name ) ) == 1 )
			continue;

		if( callvote->argument_format )
			G_PrintMsg( ent, " %s %s\n", callvote->name, callvote->argument_format );
		else
			G_PrintMsg( ent, " %s\n", callvote->name );
	}
}

/*
* G_CallVotes_PrintHelpToPlayer
*/
static void G_CallVotes_PrintHelpToPlayer( edict_t *ent, callvotetype_t *callvote )
{

	if( !callvote )
		return;

	G_PrintMsg( ent, "Usage: %s %s\n%s%s\n", callvote->name,
		( callvote->argument_format ? callvote->argument_format : "" ),
		( callvote->current ? va( "Current: %s\n", callvote->current() ) : "" ),
		( callvote->help ? callvote->help : "" ) );
	if( callvote->extraHelp != NULL ) callvote->extraHelp( ent );
}

/*
* G_CallVotes_ArgsToString
*/
static char *G_CallVotes_ArgsToString( callvotedata_t *vote )
{
	static char argstring[MAX_STRING_CHARS];
	int i;

	argstring[0] = 0;

	if( vote->argc > 0 ) Q_strncatz( argstring, vote->argv[0], sizeof( argstring ) );
	for( i = 1; i < vote->argc; i++ )
	{
		Q_strncatz( argstring, " ", sizeof( argstring ) );
		Q_strncatz( argstring, vote->argv[i], sizeof( argstring ) );
	}

	return argstring;
}

/*
* G_CallVotes_String
*/
static char *G_CallVotes_String( callvotedata_t *vote )
{
	if( vote->string )
		return vote->string;
	else
		return G_CallVotes_ArgsToString( vote );
}

/*
* G_CallVotes_CheckState
*/
static void G_CallVotes_CheckState( void )
{
	edict_t	*ent;
	int needvotes, yeses = 0, voters = 0, noes = 0;
	static unsigned int warntimer;

	if( !callvoteState.vote.callvote )
	{
		warntimer = 0;
		return;
	}

	if( callvoteState.vote.callvote->validate != NULL &&
		!callvoteState.vote.callvote->validate( &callvoteState.vote, qfalse ) )
	{
		// fixme: should be vote cancelled or something
		G_AnnouncerSound( NULL, trap_SoundIndex( va( S_ANNOUNCER_CALLVOTE_FAILED_1_to_2, ( rand()&1 )+1 ) ), GS_MAX_TEAMS, qtrue, NULL );
		G_PrintMsg( NULL, "Vote is no longer valid\nVote %s%s %s%s canceled\n", S_COLOR_YELLOW,
			callvoteState.vote.callvote->name, G_CallVotes_String( &callvoteState.vote ), S_COLOR_WHITE );
		G_CallVotes_Reset();
		return;
	}

	//analize votation state
	for( ent = game.edicts + 1; PLAYERNUM( ent ) < gs.maxclients; ent++ )
	{
		if( !ent->r.inuse || trap_GetClientState( PLAYERNUM( ent ) ) < CS_SPAWNED )
			continue;

		if( ( ent->r.svflags & SVF_FAKECLIENT ) || ent->r.client->tv )
			continue;

		voters++;
		if( clientVoted[PLAYERNUM( ent )] == VOTED_YES )
			yeses++;
		else if( clientVoted[PLAYERNUM( ent )] == VOTED_NO )
			noes++;
	}

	// passed?
	needvotes = (int)( ( voters * g_callvote_electpercentage->value ) / 100 );
	if( yeses > needvotes || callvoteState.vote.operatorcall )
	{
		G_AnnouncerSound( NULL, trap_SoundIndex( va( S_ANNOUNCER_CALLVOTE_PASSED_1_to_2, ( rand()&1 )+1 ) ), GS_MAX_TEAMS, qtrue, NULL );
		G_PrintMsg( NULL, "Vote %s%s %s%s passed\n", S_COLOR_YELLOW, callvoteState.vote.callvote->name,
			G_CallVotes_String( &callvoteState.vote ), S_COLOR_WHITE );
		if( callvoteState.vote.callvote->execute != NULL )
			callvoteState.vote.callvote->execute( &callvoteState.vote );
		G_CallVotes_Reset();
		return;
	}

	// failed?
	if( game.realtime > callvoteState.timeout || voters-noes <= needvotes ) // no change to pass anymore
	{
		G_AnnouncerSound( NULL, trap_SoundIndex( va( S_ANNOUNCER_CALLVOTE_FAILED_1_to_2, ( rand()&1 )+1 ) ), GS_MAX_TEAMS, qtrue, NULL );
		G_PrintMsg( NULL, "Vote %s%s %s%s failed\n", S_COLOR_YELLOW, callvoteState.vote.callvote->name,
			G_CallVotes_String( &callvoteState.vote ), S_COLOR_WHITE );
		G_CallVotes_Reset();
		return;
	}

	if( warntimer < game.realtime )
	{
		if( callvoteState.timeout - game.realtime <= 7.5 && callvoteState.timeout - game.realtime > 2.5 )
			G_AnnouncerSound( NULL, trap_SoundIndex( S_ANNOUNCER_CALLVOTE_VOTE_NOW ), GS_MAX_TEAMS, qtrue, NULL );
		G_PrintMsg( NULL, "Vote in progress: %s%s %s%s, %i voted yes, %i voted no. %i required\n", S_COLOR_YELLOW,
			callvoteState.vote.callvote->name, G_CallVotes_String( &callvoteState.vote ), S_COLOR_WHITE, yeses, noes,
			needvotes + 1 );
		warntimer = game.realtime + 5 * 1000;
	}
}

/*
* G_CallVotes_CmdVote
*/
void G_CallVotes_CmdVote( edict_t *ent )
{
	char *vote;

	if( !ent->r.client )
		return;
	if( ( ent->r.svflags & SVF_FAKECLIENT ) || ent->r.client->tv )
		return;

	if( !callvoteState.vote.callvote )
	{
		G_PrintMsg( ent, "%sThere's no vote in progress\n", S_COLOR_RED );
		return;
	}

	if( clientVoted[PLAYERNUM( ent )] != VOTED_NOTHING )
	{
		G_PrintMsg( ent, "%sYou have already voted\n", S_COLOR_RED );
		return;
	}

	vote = trap_Cmd_Argv( 1 );
	if( !Q_stricmp( vote, "yes" ) )
	{
		clientVoted[PLAYERNUM( ent )] = VOTED_YES;
		G_CallVotes_CheckState();
		return;
	}
	else if( !Q_stricmp( vote, "no" ) )
	{
		clientVoted[PLAYERNUM( ent )] = VOTED_NO;
		G_CallVotes_CheckState();
		return;
	}

	G_PrintMsg( ent, "%sInvalid vote: %s%s%s. Use yes or no\n", S_COLOR_RED,
		S_COLOR_YELLOW, vote, S_COLOR_RED );
}

/*
* G_CallVotes_Think
*/
void G_CallVotes_Think( void )
{
	static unsigned int callvotethinktimer = 0;

	if( !callvoteState.vote.callvote )
	{
		callvotethinktimer = 0;
		return;
	}

	if( callvotethinktimer < game.realtime )
	{
		G_CallVotes_CheckState();
		callvotethinktimer = game.realtime + 1000;
	}
}

/*
* G_CallVote
*/
static void G_CallVote( edict_t *ent, qboolean isopcall )
{
	int i;
	char *votename;
	callvotetype_t *callvote;

	if( !isopcall && ent->s.team == TEAM_SPECTATOR && GS_InvidualGameType()
		&& GS_MatchState() == MATCH_STATE_PLAYTIME && !GS_MatchPaused() )
	{
		int team, count;
		edict_t *e;

		for( count = 0, team = TEAM_ALPHA; team < GS_MAX_TEAMS; team++ )
		{
			if( !teamlist[team].numplayers )
				continue;

			for( i = 0; teamlist[team].playerIndices[i] != -1; i++ )
			{
				e = game.edicts + teamlist[team].playerIndices[i];
				if( e->r.inuse && e->r.svflags & SVF_FAKECLIENT )
					count++;
			}
		}

		if( !count )
		{
			G_PrintMsg( ent, "%sSpectators cannot start a vote while a match is in progress\n", S_COLOR_RED );
			return;
		}
	}

	if( !g_callvote_enabled->integer )
	{
		G_PrintMsg( ent, "%sCallvoting is disabled on this server\n", S_COLOR_RED );
		return;
	}

	if( callvoteState.vote.callvote )
	{
		G_PrintMsg( ent, "%sA vote is already in progress\n", S_COLOR_RED );
		return;
	}

	votename = trap_Cmd_Argv( 1 );
	if( !votename || !votename[0] )
	{
		G_CallVotes_PrintUsagesToPlayer( ent );
		return;
	}

	if( strlen( votename ) > MAX_QPATH )
	{
		G_PrintMsg( ent, "%sInvalid vote\n", S_COLOR_RED );
		G_CallVotes_PrintUsagesToPlayer( ent );
		return;
	}

	//find the actual callvote command
	for( callvote = callvotesHeadNode; callvote != NULL; callvote = callvote->next )
	{
		if( callvote->name && !Q_stricmp( callvote->name, votename ) )
			break;
	}

	//unrecognized callvote type
	if( callvote == NULL )
	{
		G_PrintMsg( ent, "%sUnrecognized vote: %s\n", S_COLOR_RED, votename );
		G_CallVotes_PrintUsagesToPlayer( ent );
		return;
	}

	// wsw : pb : server admin can now disable a specific vote command (g_disable_vote_<vote name>)
	// check if vote is disabled
	if( !isopcall && trap_Cvar_Value( va( "g_disable_vote_%s", callvote->name ) ) )
	{
		G_PrintMsg( ent, "%sCallvote %s is disabled on this server\n", S_COLOR_RED, callvote->name );
		return;
	}

	// allow a second cvar specific for opcall
	if( isopcall && trap_Cvar_Value( va( "g_disable_opcall_%s", callvote->name ) ) )
	{
		G_PrintMsg( ent, "%sOpcall %s is disabled on this server\n", S_COLOR_RED, callvote->name );
		return;
	}

	//we got a valid type. Get the parameters if any
	if( callvote->expectedargs != trap_Cmd_Argc()-2 )
	{
		if( callvote->expectedargs != -1 &&
			( callvote->expectedargs != -2 || trap_Cmd_Argc()-2 > 0 ) )
		{
			// wrong number of parametres
			G_CallVotes_PrintHelpToPlayer( ent, callvote );
			return;
		}
	}

	callvoteState.vote.argc = trap_Cmd_Argc()-2;
	for( i = 0; i < callvoteState.vote.argc; i++ )
		callvoteState.vote.argv[i] = G_CopyString( trap_Cmd_Argv( i+2 ) );

	callvoteState.vote.callvote = callvote;
	callvoteState.vote.caller = ent;
	callvoteState.vote.operatorcall = isopcall;

	//validate if there's a validation func
	if( callvote->validate != NULL && !callvote->validate( &callvoteState.vote, qtrue ) )
	{
		G_CallVotes_PrintHelpToPlayer( ent, callvote );
		G_CallVotes_Reset(); // free the args
		return;
	}

	//we're done. Proceed launching the election
	memset( clientVoted, VOTED_NOTHING, sizeof( clientVoted ) );
	callvoteState.timeout = game.realtime + ( g_callvote_electtime->integer * 1000 );

	//caller is assumed to vote YES
	clientVoted[PLAYERNUM( ent )] = VOTED_YES;

	G_AnnouncerSound( NULL, trap_SoundIndex( va( S_ANNOUNCER_CALLVOTE_CALLED_1_to_2, ( rand()&1 )+1 ) ), GS_MAX_TEAMS, qtrue, NULL );

	G_PrintMsg( NULL, "%s%s requested to vote %s%s %s%s\n", ent->r.client->netname, S_COLOR_WHITE, S_COLOR_YELLOW,
		callvoteState.vote.callvote->name, G_CallVotes_String( &callvoteState.vote ), S_COLOR_WHITE );

	G_PrintMsg( NULL, "%sPress %sF1 (\\vote yes)%s or %sF2 (\\vote no)%s\n", S_COLOR_WHITE, S_COLOR_YELLOW,
		S_COLOR_WHITE, S_COLOR_YELLOW, S_COLOR_WHITE );

	G_CallVotes_Think(); // make the first think
}

/*
* G_CallVote_Cmd
*/
void G_CallVote_Cmd( edict_t *ent )
{
	if( ( ent->r.svflags & SVF_FAKECLIENT ) || ent->r.client->tv )
		return;
	G_CallVote( ent, qfalse );
}

/*
* G_OperatorVote_Cmd
*/
void G_OperatorVote_Cmd( edict_t *ent )
{
	edict_t *other;

	if( !ent->r.client )
		return;
	if( ( ent->r.svflags & SVF_FAKECLIENT ) || ent->r.client->tv )
		return;

	if( !ent->r.client->isoperator )
	{
		G_PrintMsg( ent, "You are not a game operator\n" );
		return;
	}

	if( !Q_stricmp( trap_Cmd_Argv( 1 ), "help" ) )
	{
		G_PrintMsg( ent, "Opcall can be used with all callvotes and the following commands:\n" );
		G_PrintMsg( ent, "-help\n - passvote\n- cancelvote\n" );
		return;
	}

	if( !Q_stricmp( trap_Cmd_Argv( 1 ), "cancelvote" ) )
	{
		if( !callvoteState.vote.callvote )
		{
			G_PrintMsg( ent, "There's no callvote to cancel.\n" );
			return;
		}

		for( other = game.edicts + 1; PLAYERNUM( other ) < gs.maxclients; other++ )
		{
			if( !other->r.inuse || trap_GetClientState( PLAYERNUM( other ) ) < CS_SPAWNED )
				continue;

			if( ( other->r.svflags & SVF_FAKECLIENT ) || other->r.client->tv )
				continue;

			clientVoted[PLAYERNUM( other )] = VOTED_NO;
		}

		G_PrintMsg( NULL, "Callvote has been cancelled by %s\n", ent->r.client->netname );
		return;
	}

	if( !Q_stricmp( trap_Cmd_Argv( 1 ), "passvote" ) )
	{
		if( !callvoteState.vote.callvote )
		{
			G_PrintMsg( ent, "There's no callvote to pass.\n" );
			return;
		}

		for( other = game.edicts + 1; PLAYERNUM( other ) < gs.maxclients; other++ )
		{
			if( !other->r.inuse || trap_GetClientState( PLAYERNUM( other ) ) < CS_SPAWNED )
				continue;

			if( ( other->r.svflags & SVF_FAKECLIENT ) || other->r.client->tv )
				continue;

			clientVoted[PLAYERNUM( other )] = VOTED_YES;
		}

		G_PrintMsg( NULL, "Callvote has been passed by %s\n", ent->r.client->netname );
		return;
	}

	if( !Q_stricmp( trap_Cmd_Argv( 1 ), "putteam" ) )
	{
		char *splayer = trap_Cmd_Argv( 2 );
		char *steam = trap_Cmd_Argv( 3 );
		edict_t *playerEnt;
		int newTeam;

		if( !steam || !steam[0] || !splayer || !splayer[0] )
		{
			G_PrintMsg( ent, "Usage 'putteam <player id > <team name>'.\n" );
			return;
		}

		if( ( newTeam = GS_Teams_TeamFromName( steam ) ) < 0 )
		{
			G_PrintMsg( ent, "The team '%s' doesn't exist.\n", steam );
			return;
		}

		if( ( playerEnt = G_PlayerForText( splayer ) ) == NULL )
		{
			G_PrintMsg( ent, "The player '%s' couldn't be found.\n", splayer );
			return;
		}

		G_Teams_SetTeam( playerEnt, newTeam );
		G_PrintMsg( NULL, "%s was moved to team %s by %s.\n", playerEnt->r.client->netname, GS_TeamName( newTeam ), ent->r.client->netname );

		return;
	}

	G_CallVote( ent, qtrue );
}
//racesow
void G_Cancelvote_f( void )
{
    edict_t *other;

    if( !callvoteState.vote.callvote )
    {
        Com_Printf( "There's no callvote to cancel.\n" );
        return;
    }

    for( other = game.edicts + 1; PLAYERNUM( other ) < gs.maxclients; other++ )
    {
        if( !other->r.inuse || trap_GetClientState( PLAYERNUM( other ) ) < CS_SPAWNED )
            continue;

        if( ( other->r.svflags & SVF_FAKECLIENT ) || other->r.client->tv )
            continue;

        clientVoted[PLAYERNUM( other )] = VOTED_NO;
    }

    G_PrintMsg( NULL, "Callvote has been canceled by an admin\n" );
    return;
}
//!racesow

/*
* G_VoteFromScriptValidate
*/
static qboolean G_VoteFromScriptValidate( callvotedata_t *vote, qboolean first )
{
	char argsString[MAX_STRING_CHARS];
	int i;

	if( !vote || !vote->callvote || !vote->caller )
		return qfalse;

	Q_snprintfz( argsString, MAX_STRING_CHARS, "\"%s\"", vote->callvote->name );
	for( i = 0; i < vote->argc; i++ )
	{
		Q_strncatz( argsString, " ", MAX_STRING_CHARS );
		Q_strncatz( argsString, va( " \"%s\"", vote->argv[i] ), MAX_STRING_CHARS );
	}

	return G_asCallGameCommandScript( vote->caller->r.client, "callvotevalidate", argsString, vote->argc + 1 );
}

/*
* G_VoteFromScriptPassed
*/
static void G_VoteFromScriptPassed( callvotedata_t *vote )
{
	char argsString[MAX_STRING_CHARS];
	int i;

	if( !vote || !vote->callvote || !vote->caller )
		return;

	Q_snprintfz( argsString, MAX_STRING_CHARS, "\"%s\"", vote->callvote->name );
	for( i = 0; i < vote->argc; i++ )
	{
		Q_strncatz( argsString, " ", MAX_STRING_CHARS );
		Q_strncatz( argsString, va( " \"%s\"", vote->argv[i] ), MAX_STRING_CHARS );
	}

	G_asCallGameCommandScript( vote->caller->r.client, "callvotepassed", argsString, vote->argc + 1 );
}

/*
* G_RegisterGametypeScriptCallvote
*/
void G_RegisterGametypeScriptCallvote( const char *name, const char *usage, const char *help )
{
	callvotetype_t *vote;

	if( !name )
		return;

	vote = G_RegisterCallvote( name );
	vote->expectedargs = 1;
	vote->validate = G_VoteFromScriptValidate;
	vote->execute = G_VoteFromScriptPassed;
	vote->current = NULL;
	vote->extraHelp = NULL;
	vote->argument_format = usage ? G_LevelCopyString( usage ) : NULL;
	vote->help = help ? G_LevelCopyString( va( "- %s", help ) ) : NULL;
}

/*
* G_CallVotes_Init
*/
void G_CallVotes_Init( void )
{
	callvotetype_t *callvote;

	g_callvote_electpercentage =	trap_Cvar_Get( "g_vote_percent", "55", CVAR_ARCHIVE );
	g_callvote_electtime =		trap_Cvar_Get( "g_vote_electtime", "40", CVAR_ARCHIVE );
	g_callvote_enabled =		trap_Cvar_Get( "g_vote_allowed", "1", CVAR_ARCHIVE );

	// register all callvotes

	callvote = G_RegisterCallvote( "map" );
	callvote->expectedargs = 1;
	callvote->validate = G_VoteMapValidate;
	callvote->execute = G_VoteMapPassed;
	callvote->current = G_VoteMapCurrent;
	callvote->extraHelp = G_VoteMapExtraHelp;
	callvote->argument_format = G_LevelCopyString( "<name/[startnum]>" );
	callvote->help = G_LevelCopyString( "- Changes map" );

	callvote = G_RegisterCallvote( "restart" );
	callvote->expectedargs = 0;
	callvote->validate = NULL;
	callvote->execute = G_VoteRestartPassed;
	callvote->current = NULL;
	callvote->extraHelp = NULL;
	callvote->argument_format = NULL;
	callvote->help = G_LevelCopyString( "- Restarts current map" );

	callvote = G_RegisterCallvote( "nextmap" );
	callvote->expectedargs = 0;
	callvote->validate = NULL;
	callvote->execute = G_VoteNextMapPassed;
	callvote->current = NULL;
	callvote->extraHelp = NULL;
	callvote->argument_format = NULL;
	callvote->help = G_LevelCopyString( "- Jumps to the next map" );

	callvote = G_RegisterCallvote( "scorelimit" );
	callvote->expectedargs = 1;
	callvote->validate = G_VoteScorelimitValidate;
	callvote->execute = G_VoteScorelimitPassed;
	callvote->current = G_VoteScorelimitCurrent;
	callvote->extraHelp = NULL;
	callvote->argument_format = G_LevelCopyString( "<number>" );
	callvote->help = G_LevelCopyString( "- Sets the number of frags or caps needed to win the match\n- Use 0 to disable" );

	callvote = G_RegisterCallvote( "timelimit" );
	callvote->expectedargs = 1;
	callvote->validate = G_VoteTimelimitValidate;
	callvote->execute = G_VoteTimelimitPassed;
	callvote->current = G_VoteTimelimitCurrent;
	callvote->extraHelp = NULL;
	callvote->argument_format = G_LevelCopyString( "<minutes>" );
	callvote->help = G_LevelCopyString( "- Sets number of minutes after which the match ends\n- Use 0 to disable" );

	callvote = G_RegisterCallvote( "gametype" );
	callvote->expectedargs = 1;
	callvote->validate = G_VoteGametypeValidate;
	callvote->execute = G_VoteGametypePassed;
	callvote->current = G_VoteGametypeCurrent;
	callvote->extraHelp = G_VoteGametypeExtraHelp;
	callvote->argument_format = G_LevelCopyString( "<name>" );
	callvote->help = G_LevelCopyString( "- Changes the gametype" );

	callvote = G_RegisterCallvote( "warmup_timelimit" );
	callvote->expectedargs = 1;
	callvote->validate = G_VoteWarmupTimelimitValidate;
	callvote->execute = G_VoteWarmupTimelimitPassed;
	callvote->current = G_VoteWarmupTimelimitCurrent;
	callvote->extraHelp = NULL;
	callvote->argument_format = G_LevelCopyString( "<minutes>" );
	callvote->help = G_LevelCopyString( "- Sets the number of minutes after which the warmup ends\n- Use 0 to disable" );

	callvote = G_RegisterCallvote( "extended_time" );
	callvote->expectedargs = 1;
	callvote->validate = G_VoteExtendedTimeValidate;
	callvote->execute = G_VoteExtendedTimePassed;
	callvote->current = G_VoteExtendedTimeCurrent;
	callvote->extraHelp = NULL;
	callvote->argument_format = G_LevelCopyString( "<minutes>" );
	callvote->help = G_LevelCopyString( "- Sets the length of the overtime\n- Use 0 to enable suddendeath mode" );

	callvote = G_RegisterCallvote( "maxteamplayers" );
	callvote->expectedargs = 1;
	callvote->validate = G_VoteMaxTeamplayersValidate;
	callvote->execute = G_VoteMaxTeamplayersPassed;
	callvote->current = G_VoteMaxTeamplayersCurrent;
	callvote->extraHelp = NULL;
	callvote->argument_format = G_LevelCopyString( "<number>" );
	callvote->help = G_LevelCopyString( "- Sets the maximum number of players in one team" );

	callvote = G_RegisterCallvote( "lock" );
	callvote->expectedargs = 0;
	callvote->validate = G_VoteLockValidate;
	callvote->execute = G_VoteLockPassed;
	callvote->current = NULL;
	callvote->extraHelp = NULL;
	callvote->argument_format = NULL;
	callvote->help = G_LevelCopyString( "- Locks teams to disallow players joining in mid-game" );

	callvote = G_RegisterCallvote( "unlock" );
	callvote->expectedargs = 0;
	callvote->validate = G_VoteUnlockValidate;
	callvote->execute = G_VoteUnlockPassed;
	callvote->current = NULL;
	callvote->extraHelp = NULL;
	callvote->argument_format = NULL;
	callvote->help = G_LevelCopyString( "- Unlocks teams to allow players joining in mid-game" );

	callvote = G_RegisterCallvote( "allready" );
	callvote->expectedargs = 0;
	callvote->validate = G_VoteAllreadyValidate;
	callvote->execute = G_VoteAllreadyPassed;
	callvote->current = NULL;
	callvote->extraHelp = NULL;
	callvote->argument_format = NULL;
	callvote->help = G_LevelCopyString( "- Sets all players as ready so the match can start" );

	callvote = G_RegisterCallvote( "remove" );
	callvote->expectedargs = 1;
	callvote->validate = G_VoteRemoveValidate;
	callvote->execute = G_VoteRemovePassed;
	callvote->current = NULL;
	callvote->extraHelp = G_VoteRemoveExtraHelp;
	callvote->argument_format = G_LevelCopyString( "<id or name>" );
	callvote->help = G_LevelCopyString( "- Forces player back to spectator mode" );

	callvote = G_RegisterCallvote( "kick" );
	callvote->expectedargs = 1;
	callvote->validate = G_VoteKickValidate;
	callvote->execute = G_VoteKickPassed;
	callvote->current = NULL;
	callvote->extraHelp = G_VoteKickExtraHelp;
	callvote->argument_format = G_LevelCopyString( "<id or name>" );
	callvote->help = G_LevelCopyString( "- Removes player from the server" );

	callvote = G_RegisterCallvote( "kickban" );
	callvote->expectedargs = 1;
	callvote->validate = G_VoteKickBanValidate;
	callvote->execute = G_VoteKickBanPassed;
	callvote->current = NULL;
	callvote->extraHelp = G_VoteKickBanExtraHelp;
	callvote->argument_format = G_LevelCopyString( "<id or name>" );
	callvote->help = G_LevelCopyString( "- Removes player from the server and bans his IP-address for 15 minutes" );

	callvote = G_RegisterCallvote( "mute" );
	callvote->expectedargs = 1;
	callvote->validate = G_VoteMuteValidate;
	callvote->execute = G_VoteMutePassed;
	callvote->current = NULL;
	callvote->extraHelp = G_VoteMuteExtraHelp;
	callvote->argument_format = G_LevelCopyString( "<id or name>" );
	callvote->help = G_LevelCopyString( "- Disallows chat messages from the muted player" );

	callvote = G_RegisterCallvote( "vmute" );
	callvote->expectedargs = 1;
	callvote->validate = G_VoteMuteValidate;
	callvote->execute = G_VoteVMutePassed;
	callvote->current = NULL;
	callvote->extraHelp = G_VoteMuteExtraHelp;
	callvote->argument_format = G_LevelCopyString( "<id or name>" );
	callvote->help = G_LevelCopyString( "- Disallows voice chat messages from the muted player" );

	callvote = G_RegisterCallvote( "unmute" );
	callvote->expectedargs = 1;
	callvote->validate = G_VoteUnmuteValidate;
	callvote->execute = G_VoteUnmutePassed;
	callvote->current = NULL;
	callvote->extraHelp = G_VoteUnmuteExtraHelp;
	callvote->argument_format = G_LevelCopyString( "<id or name>" );
	callvote->help = G_LevelCopyString( "- Reallows chat messages from the unmuted player" );

	callvote = G_RegisterCallvote( "vunmute" );
	callvote->expectedargs = 1;
	callvote->validate = G_VoteUnmuteValidate;
	callvote->execute = G_VoteVUnmutePassed;
	callvote->current = NULL;
	callvote->extraHelp = G_VoteUnmuteExtraHelp;
	callvote->argument_format = G_LevelCopyString( "<id or name>" );
	callvote->help = G_LevelCopyString( "- Reallows voice chat messages from the unmuted player" );

	callvote = G_RegisterCallvote( "numbots" );
	callvote->expectedargs = 1;
	callvote->validate = G_VoteNumBotsValidate;
	callvote->execute = G_VoteNumBotsPassed;
	callvote->current = G_VoteNumBotsCurrent;
	callvote->extraHelp = NULL;
	callvote->argument_format = G_LevelCopyString( "<count>" );
	callvote->help = G_LevelCopyString( "- Sets the number of bots to play on the server" );

	callvote = G_RegisterCallvote( "allow_teamdamage" );
	callvote->expectedargs = 1;
	callvote->validate = G_VoteAllowTeamDamageValidate;
	callvote->execute = G_VoteAllowTeamDamagePassed;
	callvote->current = G_VoteAllowTeamDamageCurrent;
	callvote->extraHelp = NULL;
	callvote->argument_format = G_LevelCopyString( "<1 or 0>" );
	callvote->help = G_LevelCopyString( "- Toggles whether shooting teammates will do damage to them" );

	callvote = G_RegisterCallvote( "instajump" );
	callvote->expectedargs = 1;
	callvote->validate = G_VoteAllowInstajumpValidate;
	callvote->execute = G_VoteAllowInstajumpPassed;
	callvote->current = G_VoteAllowInstajumpCurrent;
	callvote->extraHelp = NULL;
	callvote->argument_format = G_LevelCopyString( "<1 or 0>" );
	callvote->help = G_LevelCopyString( "- Toggles whether instagun can be used for weapon jumping" );

	callvote = G_RegisterCallvote( "instashield" );
	callvote->expectedargs = 1;
	callvote->validate = G_VoteAllowInstashieldValidate;
	callvote->execute = G_VoteAllowInstashieldPassed;
	callvote->current = G_VoteAllowInstashieldCurrent;
	callvote->extraHelp = NULL;
	callvote->argument_format = G_LevelCopyString( "<1 or 0>" );
	callvote->help = G_LevelCopyString( "- Toggles the availability of instashield in instagib" );

	callvote = G_RegisterCallvote( "allow_falldamage" );
	callvote->expectedargs = 1;
	callvote->validate = G_VoteAllowFallDamageValidate;
	callvote->execute = G_VoteAllowFallDamagePassed;
	callvote->current = G_VoteAllowFallDamageCurrent;
	callvote->extraHelp = NULL;
	callvote->argument_format = G_LevelCopyString( "<1 or 0>" );
	callvote->help = G_LevelCopyString( "- Toggles whether falling long distances deals damage" );

	callvote = G_RegisterCallvote( "allow_selfdamage" );
	callvote->expectedargs = 1;
	callvote->validate = G_VoteAllowSelfDamageValidate;
	callvote->execute = G_VoteAllowSelfDamagePassed;
	callvote->current = G_VoteAllowSelfDamageCurrent;
	callvote->extraHelp = NULL;
	callvote->argument_format = G_LevelCopyString( "<1 or 0>" );
	callvote->help = G_LevelCopyString( "- Toggles whether weapon splashes can damage self" );

	callvote = G_RegisterCallvote( "timeout" );
	callvote->expectedargs = 0;
	callvote->validate = G_VoteTimeoutValidate;
	callvote->execute = G_VoteTimeoutPassed;
	callvote->current = NULL;
	callvote->extraHelp = NULL;
	callvote->argument_format = NULL;
	callvote->help = G_LevelCopyString( "- Pauses the game" );

	callvote = G_RegisterCallvote( "timein" );
	callvote->expectedargs = 0;
	callvote->validate = G_VoteTimeinValidate;
	callvote->execute = G_VoteTimeinPassed;
	callvote->current = NULL;
	callvote->extraHelp = NULL;
	callvote->argument_format = NULL;
	callvote->help = G_LevelCopyString( "- Resumes the game if in timeout" );

	callvote = G_RegisterCallvote( "challengers_queue" );
	callvote->expectedargs = 1;
	callvote->validate = G_VoteChallengersValidate;
	callvote->execute = G_VoteChallengersPassed;
	callvote->current = G_VoteChallengersCurrent;
	callvote->extraHelp = NULL;
	callvote->argument_format = G_LevelCopyString( "<1 or 0>" );
	callvote->help = G_LevelCopyString( "- Toggles the challenging spectators queue line" );

	callvote = G_RegisterCallvote( "allow_uneven" );
	callvote->expectedargs = 1;
	callvote->validate = G_VoteAllowUnevenValidate;
	callvote->execute = G_VoteAllowUnevenPassed;
	callvote->current = G_VoteAllowUnevenCurrent;
	callvote->extraHelp = NULL;
	callvote->argument_format = G_LevelCopyString( "<1 or 0>" );
	callvote->help = G_LevelCopyString( "- Toggles whether uneven teams is allowed" );

#ifdef ALLOWBYNNY_VOTE
	callvote = G_RegisterCallvote( "allow_bunny" );
	callvote->expectedargs = 1;
	callvote->validate = G_VoteAllowBunnyValidate;
	callvote->execute = G_VoteAllowBunnyPassed;
	callvote->current = G_VoteAllowBunnyCurrent;
	callvote->extraHelp = NULL;
	callvote->argument_format = G_LevelCopyString( "<1 or 0>" );
	callvote->help = G_LevelCopyString( "- Toggles whether bunnyhopping is enabled" );
#endif


	// wsw : pb : server admin can now disable a specific callvote command (g_disable_vote_<callvote name>)
	for( callvote = callvotesHeadNode; callvote != NULL; callvote = callvote->next )
	{
		trap_Cvar_Get( va( "g_disable_vote_%s", callvote->name ), "0", CVAR_ARCHIVE );
	}

	G_CallVotes_Reset();
}
