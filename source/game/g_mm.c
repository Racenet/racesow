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
#include "g_local.h"
#include "../matchmaker/mm_query.h"

// number of raceruns to send in one batch
#define RACERUN_BATCH_SIZE	256

stat_query_api_t *sq_api;

static void G_Match_RaceReport( void );

//====================================================

static clientRating_t *g_ratingAlloc( const char *gametype, float rating, float deviation, int uuid )
{
	clientRating_t *cr;

	cr = (clientRating_t*)G_Malloc( sizeof(*cr) );
	if( !cr )
		return NULL;

	Q_strncpyz( cr->gametype, gametype, sizeof( cr->gametype ) - 1 );
	cr->rating = rating;
	cr->deviation = deviation;
	cr->next = 0;
	cr->uuid = uuid;

	return cr;
}

static clientRating_t *g_ratingCopy( clientRating_t *other )
{
	return g_ratingAlloc( other->gametype, other->rating, other->deviation, other->uuid );
}

// free the list of clientRatings
static void g_ratingsFree( clientRating_t *list )
{
	clientRating_t *next;

	while( list )
	{
		next = list->next;
		G_Free( list );
		list = next;
	}
}

// update the current servers rating
static void g_serverRating( void )
{
	clientRating_t avg;

	if( !game.ratings )
	{
		avg.rating = MM_RATING_DEFAULT;
		avg.deviation = MM_DEVIATION_DEFAULT;
	}
	else
	{
		Rating_AverageRating( &avg, game.ratings );
	}

	// Com_Printf("g_serverRating: Updated server's skillrating to %f\n", avg.rating );

	trap_Cvar_ForceSet( "sv_skillRating", va( "%.0f", avg.rating ) );
}

/*
* G_TransferRatings
*/
void G_TransferRatings( void )
{
	clientRating_t *cr, *found;
	edict_t *ent;
	gclient_t *client;

	// shuffle the ratings back from game.ratings to clients->ratings and back
	// based on current gametype
	g_ratingsFree( game.ratings );
	game.ratings = 0;

	for( ent = game.edicts + 1; PLAYERNUM( ent ) < gs.maxclients; ent++ )
	{
		client = ent->r.client;

		if( !client )
			continue;
		if( !ent->r.inuse )
			continue;

		// temphack for duplicate client entries
		found = Rating_FindId( game.ratings, client->mm_session );
		if( found )
			continue;

		found = Rating_Find( client->ratings, gs.gametypeName );

		// create a new default rating if this doesnt exist
		// DONT USE G_AddDefaultRating cause this will cause double entries in game.ratings
		if( !found )
		{
			found = g_ratingAlloc( gs.gametypeName, MM_RATING_DEFAULT, MM_DEVIATION_DEFAULT, client->mm_session );
			if( !found )
				continue;	// ??

			found->next = client->ratings;
			client->ratings = found;
		}

		// add it to the games list
		cr = g_ratingCopy( found );
		cr->next = game.ratings;
		game.ratings = cr;
	}

	g_serverRating();
}

// This doesnt update ratings, only inserts new default rating if it doesnt exist
// if gametype is NULL, use current gametype
clientRating_t *G_AddDefaultRating( edict_t *ent, const char *gametype )
{
	clientRating_t *cr;
	gclient_t *client;

	if ( gametype == NULL )
		gametype = gs.gametypeName;

	client = ent->r.client;
	if( ! ent->r.inuse )
		return NULL;

	cr = Rating_Find( client->ratings, gametype );
	if( cr == NULL )
	{
		cr = g_ratingAlloc( gametype, MM_RATING_DEFAULT, MM_DEVIATION_DEFAULT, ent->r.client->mm_session );
		if( !cr )
			return NULL;

		cr->next = client->ratings;
		client->ratings = cr;
	}

	if( !strcmp( gametype, gs.gametypeName) )
	{
		clientRating_t *found;

		// add this rating to current game-ratings
		found = Rating_FindId( game.ratings, client->mm_session );
		if( !found )
		{
			found = g_ratingCopy( cr );
			if( found )
			{
				found->next = game.ratings;
				game.ratings = found;
			}
		}
		else
		{
			// update rating
			found->rating = cr->rating;
			found->deviation = cr->deviation;
		}
		g_serverRating();
	}

	return cr;
}

// this inserts a new one, or updates the ratings if it exists
clientRating_t *G_AddRating( edict_t *ent, const char *gametype, float rating, float deviation )
{
	clientRating_t *cr;
	gclient_t *client;

	if ( gametype == NULL )
		gametype = gs.gametypeName;

	client = ent->r.client;
	if( ! ent->r.inuse )
		return NULL;

	cr = Rating_Find( client->ratings, gametype );
	if( cr != NULL )
	{
		cr->rating = rating;
		cr->deviation = deviation;
	}
	else
	{
		cr = g_ratingAlloc( gametype, rating, deviation, ent->r.client->mm_session );
		if( !cr )
			return NULL;

		cr->next = client->ratings;
		client->ratings = cr;
	}

	if( !strcmp( gametype, gs.gametypeName) )
	{
		clientRating_t *found;

		// add this rating to current game-ratings
		found = Rating_FindId( game.ratings, client->mm_session );
		if( !found )
		{
			found = g_ratingCopy( cr );
			if( found )
			{
				found->next = game.ratings;
				game.ratings = found;
			}
		}
		else
		{
			// update values
			found->rating = rating;
			found->deviation = deviation;
		}

		g_serverRating();
	}

	return cr;
}

// removes all references for given entity
void G_RemoveRating( edict_t *ent )
{
	gclient_t *client;
	clientRating_t *cr;

	client = ent->r.client;

	// first from the game
	cr = Rating_DetachId( &game.ratings, client->mm_session );
	if( cr )
		G_Free( cr );

	// then the clients own list
	g_ratingsFree( client->ratings );
	client->ratings = 0;

	g_serverRating();
}

// debug purposes
void G_ListRatings_f( void )
{
	clientRating_t *cr;
	gclient_t *cl;
	edict_t *ent;

	Com_Printf("Listing ratings by gametype:\n");
	for( cr = game.ratings; cr ; cr = cr->next )
		Com_Printf("  %s %d %f %f\n", cr->gametype, cr->uuid, cr->rating, cr->deviation );

	Com_Printf("Listing ratings by player\n");
	for( ent = game.edicts + 1; PLAYERNUM( ent ) < gs.maxclients; ent++ )
	{
		cl = ent->r.client;

		if( ! ent->r.inuse )
			continue;

		Com_Printf("%s:\n", cl->netname );
		for( cr = cl->ratings; cr ; cr = cr->next )
			Com_Printf("  %s %d %f %f\n", cr->gametype, cr->uuid, cr->rating, cr->deviation );
	}
}

//==========================================================

// race records from MM
void G_AddRaceRecords( edict_t *ent, int numSectors, unsigned int *records )
{
	gclient_t *cl;
	raceRun_t *rr;
	size_t size;

	cl = ent->r.client;

	if( ! ent->r.inuse || cl == NULL )
		return;

	rr = &cl->level.stats.raceRecords;
	if( rr->times )
		G_LevelFree( rr->times );

	size = ( numSectors + 1 ) * sizeof( unsigned int );
	rr->times = G_LevelMalloc( size );

	memcpy( rr->times, records, size );
	rr->numSectors = numSectors;
}

// race records to AS (TODO: export this to AS)
unsigned int G_GetRaceRecord( edict_t *ent, int sector )
{
	gclient_t *cl;
	raceRun_t *rr;

	cl = ent->r.client;

	if( ! ent->r.inuse || cl == NULL )
		return 0;

	rr = &cl->level.stats.raceRecords;
	if( !rr->times )
		return 0;

	// sector = -1 means final sector
	if( sector < -1 || sector >= rr->numSectors )
		return 0;

	if( sector < 0 )
		return rr->times[rr->numSectors];	// SAFE!
	// else
	return rr->times[sector];
}

// from AS
raceRun_t *G_NewRaceRun( edict_t *ent, int numSectors )
{
	gclient_t *cl;
	raceRun_t *rr;

	cl = ent->r.client;

	if( ! ent->r.inuse || cl == NULL  )
		return 0;

	rr = &cl->level.stats.currentRun;
	if( rr->times != NULL )
		G_LevelFree( rr->times );

	rr->times = G_LevelMalloc( (numSectors + 1) * sizeof( unsigned int ) );
	rr->numSectors = numSectors;
	rr->owner = cl->mm_session;

	return rr;
}

// from AS
void G_SetRaceTime( edict_t *ent, int sector, unsigned int time )
{
	gclient_t *cl;
	raceRun_t *rr;

	cl = ent->r.client;

	if( ! ent->r.inuse || cl == NULL )
		return;

	rr = &cl->level.stats.currentRun;
	if( sector < -1 || sector >= rr->numSectors )
		return;

	// normal sector
	if( sector >= 0 )
		rr->times[sector] = time;
	else if (rr->numSectors > 0)
	{
		raceRun_t *nrr;	// new global racerun

		rr->times[rr->numSectors] = time;
		rr->timestamp = trap_Milliseconds();

		// validate the client
		// no bots for race, at all
		if( ent->r.svflags & SVF_FAKECLIENT /* && mm_debug_reportbots->value == 0 */ )
		{
			G_Printf("G_SetRaceTime: not reporting fakeclients\n");
			return;
		}

		if( cl->mm_session <= 0 )
		{
			G_Printf("G_SetRaceTime: not reporting non-registered clients\n");
			return;
		}

		if( !game.raceruns )
			game.raceruns = LinearAllocator( sizeof( raceRun_t ), 0, _G_LevelMalloc, _G_LevelFree );

		// push new run
		nrr = LA_Alloc( game.raceruns );
		memcpy( nrr, rr, sizeof( raceRun_t ) );
		// reuse this one in nrr
		rr->times = 0;

		// see if we have to push intermediate result
		if( LA_Size( game.raceruns ) >= RACERUN_BATCH_SIZE )
		{
			G_Match_SendReport();
			// double-check this for memory-leaks
			if( game.raceruns != 0 )
				LinearAllocator_Free( game.raceruns );
			game.raceruns = 0;
		}
	}
}

void G_ListRaces_f( void )
{
	int i, j, size;
	raceRun_t *run;

	if( !game.raceruns || !LA_Size( game.raceruns ) )
	{
		G_Printf("No races to report\n");
		return;
	}

	G_Printf(S_COLOR_RED "  session    " S_COLOR_YELLOW "times\n");
	size = LA_Size( game.raceruns );
	for( i = 0; i < size; i++ )
	{
		run = (raceRun_t*)LA_Pointer( game.raceruns, i );
		G_Printf(S_COLOR_RED "  %d    " S_COLOR_YELLOW, run->owner );
		for( j = 0; j < run->numSectors; j++ )
			G_Printf("%d ", run->times[j] );
		G_Printf(S_COLOR_GREEN "%d\n", run->times[run->numSectors]);	// SAFE!
	}
}

//==========================================================

// free the collected statistics memory
void G_ClearStatistics( void )
{

}

//==========================================================
//		MM Reporting
//==========================================================


/*
* G_AddPlayerReport
*/
void G_AddPlayerReport( edict_t *ent, qboolean final )
{
	gclient_t *cl;
	gclient_quit_t *quit;
	const char *mm_session_str;
	int i, mm_session;
	cvar_t *report_bots;

	// TODO: check if MM is enabled

	if( GS_RaceGametype() )
	{
		// force sending report when someone disconnects
		G_Match_SendReport();
		return;
	}

	// if( !g_isSupportedGametype( gs.gametypeName ) )
	if( !GS_MMCompatible() )
		return;

	cl = ent->r.client;

	if( ! ent->r.inuse )
		return;

	report_bots = trap_Cvar_Get( "sv_mm_debug_reportbots", "0", CVAR_CHEAT );

	if( ( ent->r.svflags & SVF_FAKECLIENT ) && !report_bots->integer )
		return;

	if( cl == NULL || cl->team == TEAM_SPECTATOR )
		return;

	// FIXME: mm_session now exists in gclient_t
	mm_session = 0;
	mm_session_str = Info_ValueForKey( cl->userinfo, "cl_mm_session" );

	if( mm_session_str != NULL )
		mm_session = atoi( mm_session_str );

	if( mm_session == 0 )
	{
		G_Printf( "G_AddPlayerReport: Client without session-id (%s) %d\n\t(%s)\n", cl->netname, mm_session, cl->userinfo );
		return;
	}

	// check merge situation
	for( quit = game.quits; quit; quit = quit->next )
	{
		if( quit->mm_session == mm_session )
			break;

		// ch : for unregistered players, merge stats by name
		if( mm_session < 0 && quit->mm_session < 0 && !strcmp( quit->netname, cl->netname ) )
			break;
	}

	// debug :
	G_Printf("G_AddPlayerReport %s, session %d\n", cl->netname, mm_session );

	if( quit )
	{
		gameaward_t *award1, *award2;
		loggedFrag_t *frag1, *frag2;
		int j, inSize, outSize;

		// we can merge
		Q_strncpyz( quit->netname, cl->netname, sizeof( quit->netname ) );
		quit->team = cl->team;
		quit->timePlayed += ( level.time - cl->teamstate.timeStamp ) / 1000;
		quit->final = final;

		quit->stats.armor_taken += cl->level.stats.armor_taken;
		quit->stats.deaths += cl->level.stats.deaths;
		quit->stats.awards += cl->level.stats.awards;
		quit->stats.frags += cl->level.stats.frags;
		quit->stats.health_taken += cl->level.stats.health_taken;
		quit->stats.score += cl->level.stats.score;
		quit->stats.suicides += cl->level.stats.suicides;
		quit->stats.teamfrags += cl->level.stats.teamfrags;
		quit->stats.total_damage_given += cl->level.stats.total_damage_given;
		quit->stats.total_damage_received += cl->level.stats.total_damage_received;
		quit->stats.total_teamdamage_given += cl->level.stats.total_teamdamage_given;
		quit->stats.total_teamdamage_received += cl->level.stats.total_teamdamage_received;
		quit->stats.ga_taken += cl->level.stats.ga_taken;
		quit->stats.ya_taken += cl->level.stats.ya_taken;
		quit->stats.ra_taken += cl->level.stats.ra_taken;
		quit->stats.mh_taken += cl->level.stats.mh_taken;
		quit->stats.uh_taken += cl->level.stats.uh_taken;
		quit->stats.quads_taken += cl->level.stats.quads_taken;
		quit->stats.shells_taken += cl->level.stats.shells_taken;
		quit->stats.regens_taken += cl->level.stats.regens_taken;
		quit->stats.bombs_planted += cl->level.stats.bombs_planted;
		quit->stats.bombs_defused += cl->level.stats.bombs_defused;
		quit->stats.flags_capped += cl->level.stats.flags_capped;

		for( i = 0; i < (AMMO_TOTAL-AMMO_GUNBLADE); i++ )
		{
			quit->stats.accuracy_damage[i] += cl->level.stats.accuracy_damage[i];
			quit->stats.accuracy_frags[i] += cl->level.stats.accuracy_frags[i];
			quit->stats.accuracy_hits[i] += cl->level.stats.accuracy_hits[i];
			quit->stats.accuracy_hits_air[i] += cl->level.stats.accuracy_hits_air[i];
			quit->stats.accuracy_hits_direct[i] += cl->level.stats.accuracy_hits_direct[i];
			quit->stats.accuracy_shots[i] += cl->level.stats.accuracy_shots[i];
		}

		// merge awards
		if( cl->level.stats.awardAllocator )
		{
			if( !quit->stats.awardAllocator )
				quit->stats.awardAllocator = LinearAllocator( sizeof( gameaward_t ), 0, _G_LevelMalloc, _G_LevelFree );

			inSize = LA_Size( cl->level.stats.awardAllocator );
			outSize = quit->stats.awardAllocator ? LA_Size( quit->stats.awardAllocator ) : 0;
			for( i = 0; i < inSize; i++ )
			{
				award1 = LA_Pointer( cl->level.stats.awardAllocator, i );
				// search for matching one
				for( j = 0; j < outSize; j++ )
				{
					award2 = LA_Pointer( quit->stats.awardAllocator, j );
					if( !strcmp( award1->name, award2->name ) )
					{
						award2->count += award1->count;
						break;
					}
				}
				if( j >= outSize )
				{
					award2 = LA_Alloc( quit->stats.awardAllocator );
					award2->name = award1->name;
					award2->count = award1->count;
				}
			}

			// we can free the old awards
			LinearAllocator_Free( cl->level.stats.awardAllocator );
			cl->level.stats.awardAllocator = 0;
		}

		// merge logged frags
		if( cl->level.stats.fragAllocator )
		{
			inSize = LA_Size( cl->level.stats.fragAllocator );
			if( !quit->stats.fragAllocator )
				quit->stats.fragAllocator = LinearAllocator( sizeof( loggedFrag_t ), 0, _G_LevelMalloc, _G_LevelFree );

			for( i = 0; i < inSize; i++ )
			{
				frag1 = LA_Pointer( cl->level.stats.fragAllocator, i );
				frag2 = LA_Alloc( quit->stats.fragAllocator );
				memcpy( frag2, frag1, sizeof( *frag1 ) );
			}

			// we can free the old frags
			LinearAllocator_Free( cl->level.stats.fragAllocator );
			cl->level.stats.fragAllocator = 0;
		}

	}
	else
	{
		// create a new quit structure
		quit = G_Malloc( sizeof( *quit ) );
		memset( quit, 0, sizeof( *quit ) );

		// fill in the data
		Q_strncpyz( quit->netname, cl->netname, sizeof( quit->netname ) );
		quit->team = cl->team;
		quit->timePlayed = ( level.time - cl->teamstate.timeStamp ) / 1000;
		quit->final = final;
		quit->mm_session = mm_session;

		memcpy( &quit->stats, &cl->level.stats, sizeof( quit->stats ) );

		// put it to the list
		quit->next = game.quits;
		game.quits = quit;
	}
}

// common header
static void g_mm_writeHeader( stat_query_t *query, qboolean teamGame )
{
	stat_query_section_t *matchsection = sq_api->CreateSection( query, 0, "match" );

	// Write match properties
	// sq_api->SetNumber( matchsection, "final", (target_cl==NULL) ? 1 : 0 );
	sq_api->SetString( matchsection, "gametype", gs.gametypeName );
	sq_api->SetString( matchsection, "map", level.mapname );
	sq_api->SetString( matchsection, "hostname", trap_Cvar_String( "sv_hostname" ) );
	sq_api->SetNumber( matchsection, "timeplayed", ( game.serverTime - GS_MatchStartTime() ) / 1000 );
	sq_api->SetNumber( matchsection, "timelimit", GS_MatchDuration() / 1000 );
	sq_api->SetNumber( matchsection, "scorelimit", g_scorelimit->integer );
	sq_api->SetNumber( matchsection, "instagib", ( GS_Instagib() ? 1 : 0 ));
	sq_api->SetNumber( matchsection, "teamgame", teamGame );
	sq_api->SetNumber( matchsection, "racegame", ( GS_RaceGametype() ? 1 : 0 ));
	sq_api->SetString( matchsection, "gamedir", trap_Cvar_String( "fs_game" ) );
	sq_api->SetNumber( matchsection, "timestamp", trap_Milliseconds() );
	if( g_autorecord->integer ) {
		sq_api->SetString( matchsection, "demo_filename", va( "%s.wd%i", level.autorecord_name, game.protocol ) );
	}
}

static stat_query_t *G_Match_GenerateReport( void )
{
	stat_query_t *query;
	stat_query_section_t *playersarray;
	//stat_query_section_t *weapindexarray;
	gclient_quit_t *cl;
	int i, teamGame, duelGame;
	// FIXME: globalize this
	const char *weapnames[] = { "gb", "mg", "rg", "gl", "rl", "pg", "lg", "eb", "ig" };

	// Feature: do not report matches with duration less than 1 minute (actually 66 seconds)
	if( ( game.serverTime - GS_MatchStartTime() ) <= 66 * 1000 )
		return 0;

	query = sq_api->CreateQuery( "smr", qfalse );
	if( !query )
		return 0;

	// ch : race properties through GS_RaceGametype()

	// official duel frag support
	duelGame = GS_TeamBasedGametype() && GS_MaxPlayersInTeam() == 1 ? 1 : 0;

	// ch : fixme do mark duels as teamgames
	if( duelGame ) {
		teamGame = 0;
	} else if( !GS_TeamBasedGametype() ) {
		teamGame = 0;
	} else {
		teamGame = 1;
	}

	g_mm_writeHeader( query, teamGame );

	// Write team properties (if any)
	if( teamlist[TEAM_ALPHA].numplayers > 0 && teamGame != 0 )
	{
		stat_query_section_t *teamarray = sq_api->CreateArray( query, 0, "teams" );

		for( i = TEAM_ALPHA; i <= TEAM_BETA; i++ )
		{
			stat_query_section_t *team = sq_api->CreateSection( query, teamarray, 0 );
			sq_api->SetString( team, "name", trap_GetConfigString ( CS_TEAM_SPECTATOR_NAME + (i-TEAM_SPECTATOR) ) );
			sq_api->SetNumber( team, "index", i - TEAM_ALPHA );
			sq_api->SetNumber( team, "score", teamlist[i].stats.score );
		}
	}

	// TODO: write the weapon indexes
	// weapindexarray = sq_api->CreateSection( query, 0, "weapindices" );

	// Write player properties
	playersarray = sq_api->CreateArray( query, 0, "players" );
	for( cl = game.quits; cl; cl = cl->next )
	{
		stat_query_section_t *playersection, *accsection, *awardssection;
		gameaward_t *ga;
		score_stats_t *stats;

		stats = &cl->stats;

		playersection = sq_api->CreateSection( query, playersarray, 0 );

		// GENERAL INFO
		sq_api->SetString( playersection, "name", cl->netname );
		sq_api->SetNumber( playersection, "score", cl->stats.score );
		sq_api->SetNumber( playersection, "timeplayed", cl->timePlayed );
		sq_api->SetNumber( playersection, "final", cl->final );
		sq_api->SetNumber( playersection, "frags", cl->stats.frags );
		sq_api->SetNumber( playersection, "deaths", cl->stats.deaths );
		sq_api->SetNumber( playersection, "suicides", cl->stats.suicides );
		sq_api->SetNumber( playersection, "numrounds", cl->stats.numrounds );
		sq_api->SetNumber( playersection, "teamfrags", cl->stats.teamfrags );
		sq_api->SetNumber( playersection, "dmg_given", cl->stats.total_damage_given );
		sq_api->SetNumber( playersection, "dmg_taken", cl->stats.total_damage_received );
		sq_api->SetNumber( playersection, "health_taken", cl->stats.health_taken );
		sq_api->SetNumber( playersection, "armor_taken", cl->stats.armor_taken );
		sq_api->SetNumber( playersection, "ga_taken", cl->stats.ga_taken );
		sq_api->SetNumber( playersection, "ya_taken", cl->stats.ya_taken );
		sq_api->SetNumber( playersection, "ra_taken", cl->stats.ra_taken );
		sq_api->SetNumber( playersection, "mh_taken", cl->stats.mh_taken );
		sq_api->SetNumber( playersection, "uh_taken", cl->stats.uh_taken );
		sq_api->SetNumber( playersection, "quads_taken", cl->stats.quads_taken );
		sq_api->SetNumber( playersection, "shells_taken", cl->stats.shells_taken );
		sq_api->SetNumber( playersection, "regens_taken", cl->stats.regens_taken );
		sq_api->SetNumber( playersection, "bombs_planted", cl->stats.bombs_planted );
		sq_api->SetNumber( playersection, "bombs_defused", cl->stats.bombs_defused );
		sq_api->SetNumber( playersection, "flags_capped", cl->stats.flags_capped );

		if( teamGame != 0 )
			sq_api->SetNumber( playersection, "team", cl->team - TEAM_ALPHA );

		// AWARDS

		if ( stats->awardAllocator && LA_Size( stats->awardAllocator ) > 0 )
		{
			stat_query_section_t *gasection;
			int size;

			awardssection = sq_api->CreateArray( query, playersection, "awards" );
			size = LA_Size( stats->awardAllocator );

			for ( i = 0; i < size; i++ )
			{
				ga = (gameaward_t*)LA_Pointer( stats->awardAllocator, i );
				gasection = sq_api->CreateSection( query, awardssection, 0 );
				sq_api->SetString( gasection, "name", ga->name );
				sq_api->SetNumber( gasection, "count", ga->count );
			}

			// theoretically you could Free() the awards now, but they are from levelpool
			// so they are deallocated anyway after this
		}

		// WEAPONS

		// first pass calculate the number of weapons, see if we even need this section
		for( i = 0; i < (AMMO_TOTAL-WEAP_TOTAL); i++ )
		{
			if ( stats->accuracy_shots[i] > 0 )
				break;
		}
		if ( i < (AMMO_TOTAL-WEAP_TOTAL) )
		{
			int j;

			accsection = sq_api->CreateSection( query, playersection, "weapons" );

			// we only loop thru the lower section of weapons since we put both
			// weak and strong shots inside the same weapon
			for ( j = 0; j < AMMO_WEAK_GUNBLADE-WEAP_TOTAL; j++ )
			{
				int weak, hits, shots;
				double acc;
				void *weapsection;

				weak = j + (AMMO_WEAK_GUNBLADE-WEAP_TOTAL);
				if ( stats->accuracy_shots[j] == 0 && stats->accuracy_shots[weak] == 0)
					continue;

				weapsection = sq_api->CreateSection( query, accsection, weapnames[j] );

				// STRONG
				hits = stats->accuracy_hits[j];
				shots = stats->accuracy_shots[j];
				// copied from cg_scoreboard.c, but changed the last -1 to 0 (no hits is zero acc, right??)
				acc = (double) (hits > 0 ? ((hits) == (shots) ? 100 : (min( (int)( floor( ( 100.0f*(hits) ) / ( (float)(shots) ) + 0.5f ) ), 99 ))) : 0);

				sq_api->SetNumber( weapsection, "strong_hits", hits );
				sq_api->SetNumber( weapsection, "strong_shots", shots );
				sq_api->SetNumber( weapsection, "strong_acc", acc );
				sq_api->SetNumber( weapsection, "strong_dmg", stats->accuracy_damage[j] );
				sq_api->SetNumber( weapsection, "strong_frags", stats->accuracy_frags[j] );

				// WEAK
				hits = stats->accuracy_hits[weak];
				shots = stats->accuracy_shots[weak];
				// copied from cg_scoreboard.c, but changed the last -1 to 0 (no hits is zero acc, right??)
				acc = (double) (hits > 0 ? ((hits) == (shots) ? 100 : (min( (int)( floor( ( 100.0f*(hits) ) / ( (float)(shots) ) + 0.5f ) ), 99 ))) : 0);

				sq_api->SetNumber( weapsection, "weak_hits", hits );
				sq_api->SetNumber( weapsection, "weak_shots", shots );
				sq_api->SetNumber( weapsection, "weak_acc", acc );
				sq_api->SetNumber( weapsection, "weak_dmg", stats->accuracy_damage[weak] );
				sq_api->SetNumber( weapsection, "weak_frags", stats->accuracy_frags[weak] );
			}
		}

		// duel frags
		if( /* duelGame && */ stats->fragAllocator != 0 && LA_Size( stats->fragAllocator ) > 0 )
		{
			void *fragSection;
			loggedFrag_t *lfrag;
			int size;

			fragSection = sq_api->CreateArray( query, playersection, "log_frags" );
			size = LA_Size( stats->fragAllocator );
			for( i = 0; i < size; i++ )
			{
				stat_query_section_t *sect;

				lfrag = (loggedFrag_t*)LA_Pointer( stats->fragAllocator, i );

				sect = sq_api->CreateSection( query, fragSection, 0 );
				sq_api->SetNumber( sect, "victim", lfrag->mm_victim );
				sq_api->SetNumber( sect, "weapon", lfrag->weapon );
				sq_api->SetNumber( sect, "time", lfrag->time );
			}
		}

		sq_api->SetNumber( playersection, "sessionid", cl->mm_session );
	}

	return query;
}

/*
* G_Match_SendReport
*/
void G_Match_SendReport( void )
{
	edict_t *ent;
	gclient_quit_t *qcl, *qnext;
	stat_query_t *query;
	int numPlayers;

	// TODO: check if MM is enabled

	sq_api = trap_GetStatQueryAPI();
	if( !sq_api )
		return;

	if( GS_RaceGametype() )
	{
		G_Match_RaceReport();
		return;
	}

	// if( g_isSupportedGametype( gs.gametypeName ) )
	if( GS_MMCompatible() )
	{
		// merge game.clients with game.quits
		for( ent = game.edicts + 1; PLAYERNUM( ent ) < gs.maxclients; ent++ )
			G_AddPlayerReport( ent, qtrue );

		// check if we have enough players to report (at least 2)
		numPlayers = 0;
		for( qcl = game.quits; qcl ; qcl = qcl->next )
			numPlayers++;

		if( numPlayers > 1 )
		{
			// emit them all
			query = G_Match_GenerateReport();
			if( query )
			{
				trap_MM_SendQuery( query );

				// this will be free'd by callbacks
				query = NULL;
			}
		}
	}

	// clear the quit-list
	qnext = NULL;
	for( qcl = game.quits; qcl ; qcl = qnext )
	{
		qnext = qcl->next;
		G_Free( qcl );
	}
	game.quits = NULL;
}

/*
* G_Match_RaceReport
*/
static void G_Match_RaceReport( void )
{
	stat_query_t *query;
	stat_query_section_t *runsArray;
	stat_query_section_t *timesArray, *dummy;
	raceRun_t *prr;
	int i, j, size;

	if( !GS_RaceGametype() )
	{
		G_Printf("G_Match_RaceReport.. not race gametype\n");
		return;
	}

	if( !game.raceruns || !LA_Size( game.raceruns ) )
	{
		G_Printf("G_Match_RaceReport.. no runs to report\n");
		return;
	}

	/*
	* match : { .... }
	* runs :
	* [
	*   {
	*   	session_id : 3434343,
	*  	times : [ 12, 43, 56, 7878, 4 ]
	*    }
	* ]
	*  ( TODO: get the nickname there )
	*/
	query = sq_api->CreateQuery( "smr", qfalse );
	if( !query )
	{
		G_Printf("G_Match_RaceReport.. failed to create query object\n");
		return;
	}

	g_mm_writeHeader( query, qfalse );

	// Players array
	runsArray = sq_api->CreateArray( query, 0, "runs" );
	size = LA_Size( game.raceruns );
	for( i = 0; i < size; i++ )
	{
		prr = (raceRun_t*)LA_Pointer( game.raceruns, i );

		dummy = sq_api->CreateSection( query, runsArray, 0 );

		sq_api->SetNumber( dummy, "session_id", prr->owner );
		sq_api->SetNumber( dummy, "timestamp", prr->timestamp );
		timesArray = sq_api->CreateArray( query, dummy, "times" );

		for( j = 0; j < prr->numSectors; j++ )
			sq_api->AddArrayNumber( timesArray, prr->times[j] );

		// FINAL TIME
		sq_api->AddArrayNumber( timesArray, prr->times[prr->numSectors] );
	}

	trap_MM_SendQuery( query );

	query = NULL;

	// clear gameruns
	LinearAllocator_Free( game.raceruns );
	game.raceruns = NULL;
}

//===========================================================================

/*
* 	LEGACY
*/

static struct
{
	char *g_gametype;
	int g_numbots, g_scorelimit;
	float g_timelimit, g_warmup_timelimit;
	qboolean g_allow_falldamage;
} g_mm_old;

void G_MM_Setup( const char *gametype, int scorelimit, float timelimit, qboolean falldamage )
{
	// store old cvar values
	g_mm_old.g_gametype = G_CopyString( gs.gametypeName );
	g_mm_old.g_numbots = g_numbots->integer;
	g_mm_old.g_scorelimit = g_scorelimit->integer;
	g_mm_old.g_timelimit = g_timelimit->value;
	g_mm_old.g_warmup_timelimit = g_warmup_timelimit->value;
	g_mm_old.g_allow_falldamage = g_allow_falldamage->integer;

	// set new cvar values
	trap_Cvar_Set( "g_gametype", gametype ); // this needs some sort of checking
	trap_Cvar_Set( "g_numbots", "0" );
	trap_Cvar_Set( "g_scorelimit", va( "%d", scorelimit ) );
	trap_Cvar_Set( "g_timelimit", va( "%f", timelimit ) );
	trap_Cvar_Set( "g_warmup_timelimit", "5" );
	trap_Cvar_Set( "g_allow_falldamage", va( "%d", falldamage ) );

	G_RespawnLevel();
}

void G_MM_Reset( void )
{
	trap_Cvar_Set( "g_gametype", g_mm_old.g_gametype );
	trap_Cvar_Set( "g_numbots", va( "%d", g_mm_old.g_numbots ) );
	trap_Cvar_Set( "g_scorelimit", va( "%d", g_mm_old.g_scorelimit ) );
	trap_Cvar_Set( "g_timelimit", va( "%f", g_mm_old.g_timelimit ) );
	trap_Cvar_Set( "g_warmup_timelimit", va( "%d", g_mm_old.g_warmup_timelimit ) );
	trap_Cvar_Set( "g_allow_falldamage", va( "%d", g_mm_old.g_allow_falldamage ) );

	G_Free( g_mm_old.g_gametype );
	memset( &g_mm_old, 0, sizeof( g_mm_old ) );

	G_RespawnLevel();
}
