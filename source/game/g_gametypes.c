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

// Generic functions used for all gametypes when don't require any special setting

void G_Gametype_GENERIC_SetUpWarmup( void )
{
	level.gametype.readyAnnouncementEnabled = qtrue;
	level.gametype.scoreAnnouncementEnabled = qfalse;
	level.gametype.countdownEnabled = qfalse;
	level.gametype.pickableItemsMask = ( level.gametype.spawnableItemsMask|level.gametype.dropableItemsMask );
	if( GS_Instagib() )
		level.gametype.pickableItemsMask &= ~G_INSTAGIB_NEGATE_ITEMMASK;

	if( GS_TeamBasedGametype() )
	{
		qboolean any = qfalse;
		int team;
		for( team = TEAM_ALPHA; team < GS_MAX_TEAMS; team++ )
		{
			if( G_Teams_TeamIsLocked( team ) )
			{
				G_Teams_UnLockTeam( team );
				any = qtrue;
			}
		}
		if( any )
			G_PrintMsg( NULL, "Teams unlocked.\n" );
	}
	else
	{
		if( G_Teams_TeamIsLocked( TEAM_PLAYERS ) )
		{
			G_Teams_UnLockTeam( TEAM_PLAYERS );
			G_PrintMsg( NULL, "Teams unlocked.\n" );
		}
	}
	G_Teams_RemoveInvites();
}

void G_Gametype_GENERIC_SetUpCountdown( void )
{
	qboolean any = qfalse;
	int team;

	G_Match_RemoveAllProjectiles();
	G_Items_RespawnByType( 0, 0, 0 ); // respawn all items

	level.gametype.readyAnnouncementEnabled = qfalse;
	level.gametype.scoreAnnouncementEnabled = qfalse;
	level.gametype.countdownEnabled = qtrue;
	level.gametype.pickableItemsMask = 0; // disallow item pickup

	if( GS_TeamBasedGametype() )
	{
		for( team = TEAM_ALPHA; team < GS_MAX_TEAMS; team++ )
			if( G_Teams_LockTeam( team ) )
				any = qtrue;
	}
	else
	{
		if( G_Teams_LockTeam( TEAM_PLAYERS ) )
			any = qtrue;
	}

	if( any )
		G_PrintMsg( NULL, "Teams locked.\n" );

	G_AnnouncerSound( NULL, trap_SoundIndex( va( S_ANNOUNCER_COUNTDOWN_GET_READY_TO_FIGHT_1_to_2, ( rand()&1 )+1 ) ),
		GS_MAX_TEAMS, qtrue, NULL );
}

void G_Gametype_GENERIC_SetUpMatch( void )
{
	int i;

	level.gametype.readyAnnouncementEnabled = qfalse;
	level.gametype.scoreAnnouncementEnabled = qtrue;
	level.gametype.countdownEnabled = qtrue;
	level.gametype.pickableItemsMask = ( level.gametype.spawnableItemsMask|level.gametype.dropableItemsMask );
	if( GS_Instagib() )
		level.gametype.pickableItemsMask &= ~G_INSTAGIB_NEGATE_ITEMMASK;

	// clear player stats and scores, team scores and respawn clients in team lists
	for( i = TEAM_PLAYERS; i < GS_MAX_TEAMS; i++ )
	{
		int j;
		g_teamlist_t *team = &teamlist[i];
		memset( &team->stats, 0, sizeof( team->stats ) );

		// respawn all clients inside the playing teams
		for( j = 0; team->playerIndices[j] != -1; j++ )
		{
			edict_t *ent = &game.edicts[ team->playerIndices[j] ];
			G_ClientClearStats( ent );
			G_ClientRespawn( ent, qfalse );
		}
	}

	// set items to be spawned with a delay
	G_Items_RespawnByType( IT_ARMOR, ARMOR_RA, 15 );
	G_Items_RespawnByType( IT_ARMOR, ARMOR_RA, 15 );
	G_Items_RespawnByType( IT_HEALTH, HEALTH_MEGA, 15 );
	G_Items_RespawnByType( IT_HEALTH, HEALTH_ULTRA, 15 );
	G_Items_RespawnByType( IT_POWERUP, 0, brandom( 20, 40 ) );
	G_Match_FreeBodyQueue();

	G_AnnouncerSound( NULL, trap_SoundIndex( va( S_ANNOUNCER_COUNTDOWN_FIGHT_1_to_2, ( rand()&1 )+1 ) ), GS_MAX_TEAMS, qfalse, NULL );
	G_CenterPrintMsg( NULL, "FIGHT!\n" );
}

void G_Gametype_GENERIC_SetUpEndMatch( void )
{
	edict_t *ent;

	level.gametype.readyAnnouncementEnabled = qfalse;
	level.gametype.scoreAnnouncementEnabled = qfalse;
	level.gametype.pickableItemsMask = 0; // disallow item pickup
	level.gametype.countdownEnabled = qfalse;

	for( ent = game.edicts + 1; PLAYERNUM( ent ) < gs.maxclients; ent++ )
	{
		if( ent->r.inuse && trap_GetClientState( PLAYERNUM( ent ) ) >= CS_SPAWNED )
			G_ClientRespawn( ent, qtrue );
	}

	G_AnnouncerSound( NULL, trap_SoundIndex( va( S_ANNOUNCER_POSTMATCH_GAMEOVER_1_to_2, ( rand()&1 )+1 ) ), GS_MAX_TEAMS, qtrue, NULL );
}

static qboolean G_Gametype_GENERIC_MatchStateFinished( int incomingMatchState )
{
	if( GS_MatchState() <= MATCH_STATE_WARMUP && incomingMatchState > MATCH_STATE_WARMUP 
		&& incomingMatchState < MATCH_STATE_POSTMATCH )
		G_Match_Autorecord_Start();

	if( GS_MatchState() == MATCH_STATE_POSTMATCH )
		G_Match_Autorecord_Stop();

	return qtrue;
}

static void G_Gametype_GENERIC_MatchStateStarted( void )
{
	switch( GS_MatchState() )
	{
//	case MATCH_STATE_WAITING:
	case MATCH_STATE_WARMUP:
		G_Gametype_GENERIC_SetUpWarmup();
		break;
	case MATCH_STATE_COUNTDOWN:
		G_Gametype_GENERIC_SetUpCountdown();
		break;
	case MATCH_STATE_PLAYTIME:
		G_Gametype_GENERIC_SetUpMatch();
		break;
	case MATCH_STATE_POSTMATCH:
		G_Gametype_GENERIC_SetUpEndMatch();
		break;
	default:
		break;
	}
}

static void G_Gametype_GENERIC_ThinkRules( void )
{
	if( G_Match_ScorelimitHit() || G_Match_TimelimitHit() || G_Match_SuddenDeathFinished() )
		G_Match_LaunchState( GS_MatchState() + 1 );

	if( GS_MatchState() >= MATCH_STATE_POSTMATCH )
		return;
}

char *G_Gametype_GENERIC_ScoreboardMessage( void )
{
	char entry[MAX_TOKEN_CHARS];
	size_t len;
	int i;
	edict_t	*e;
	int carrierIcon;

	*scoreboardString = 0;
	len = 0;

	Q_snprintfz( entry, sizeof(entry), "&t %i 0 0 ", TEAM_PLAYERS );
	if( SCOREBOARD_MSG_MAXSIZE - len > strlen( entry ) )
	{
		Q_strncatz( scoreboardString, entry, sizeof( scoreboardString ) );
		len = strlen( scoreboardString );
	}

	// players
	for( i = 0; teamlist[TEAM_PLAYERS].playerIndices[i] != -1; i++ )
	{
		e = game.edicts + teamlist[TEAM_PLAYERS].playerIndices[i];

		if( e->s.effects & EF_CARRIER )
			carrierIcon = trap_ImageIndex( ( e->s.team == TEAM_BETA ) ? PATH_ALPHAFLAG_ICON : PATH_BETAFLAG_ICON );
		else if( e->s.effects & EF_QUAD )
			carrierIcon = trap_ImageIndex( PATH_QUAD_ICON );
		else if( e->s.effects & EF_SHELL )
			carrierIcon = trap_ImageIndex( PATH_SHELL_ICON );
		else
			carrierIcon = 0;

		Q_snprintfz( entry, sizeof( entry ), "&p %i %i %i %i %i ",
			PLAYERNUM( e ),
			e->r.client->level.stats.score,
			e->r.client->r.ping > 999 ? 999 : e->r.client->r.ping,
			carrierIcon,
			( level.ready[PLAYERNUM( e )] || GS_MatchState() >= MATCH_STATE_PLAYTIME ) ? trap_ImageIndex( PATH_VSAY_YES_ICON ) : 0
			);

		if( SCOREBOARD_MSG_MAXSIZE - len > strlen( entry ) )
		{
			Q_strncatz( scoreboardString, entry, sizeof( scoreboardString ) );
			len = strlen( scoreboardString );
		}
	}

	return scoreboardString;
}

void G_Gametype_GENERIC_ClientRespawn( edict_t *self, int old_team, int new_team )
{
	int i;
	gclient_t *client = self->r.client;
	gs_weapon_definition_t *weapondef;

	if( G_ISGHOSTING( self ) )
		return;

	//give default items
	if( self->s.team != TEAM_SPECTATOR )
	{
		if( GS_Instagib() )
		{
			client->ps.inventory[WEAP_INSTAGUN] = 1;
			client->ps.inventory[AMMO_INSTAS] = 1;
			client->ps.inventory[AMMO_WEAK_INSTAS] = 1;
		}
		else
		{
			if( GS_MatchState() <= MATCH_STATE_WARMUP )
			{
				for( i = WEAP_GUNBLADE; i < WEAP_TOTAL; i++ )
				{
					if( i == WEAP_INSTAGUN )  // dont add instagun...
						continue;

					weapondef = GS_GetWeaponDef( i );
					client->ps.inventory[i] = 1;
					if( weapondef->firedef_weak.ammo_id )
						client->ps.inventory[weapondef->firedef_weak.ammo_id] = weapondef->firedef_weak.ammo_max;
					if( weapondef->firedef.ammo_id )
						client->ps.inventory[weapondef->firedef.ammo_id] = weapondef->firedef.ammo_max;
				}

				client->resp.armor = GS_Armor_MaxCountForTag( ARMOR_YA );
			}
			else
			{
				weapondef = GS_GetWeaponDef( WEAP_GUNBLADE );
				client->ps.inventory[WEAP_GUNBLADE] = 1;
				client->ps.inventory[AMMO_GUNBLADE] = weapondef->firedef.ammo_max;;
				client->ps.inventory[AMMO_WEAK_GUNBLADE] = 0;
			}
		}
	}

	// select rocket launcher if available
	if( GS_CheckAmmoInWeapon( &client->ps, WEAP_ROCKETLAUNCHER ) )
		client->ps.stats[STAT_PENDING_WEAPON] = WEAP_ROCKETLAUNCHER;
	else
		client->ps.stats[STAT_PENDING_WEAPON] = GS_SelectBestWeapon( &client->ps );

	// add a teleportation effect
	if( self->r.solid != SOLID_NOT )
		G_RespawnEffect( self );
}

void G_Gametype_GENERIC_PlayerKilled( edict_t *targ, edict_t *attacker, edict_t *inflictor )
{
	if( !attacker || GS_MatchState() != MATCH_STATE_PLAYTIME || ( targ->r.svflags & SVF_CORPSE ) )
		return;

	if( !attacker || !attacker->r.client || attacker == targ || attacker == world )
		teamlist[targ->s.team].stats.score--;
	else
	{
		if( GS_InvidualGameType() )
			teamlist[attacker->s.team].stats.score = attacker->r.client->level.stats.score;
		if( GS_IsTeamDamage( &targ->s, &attacker->s ) )
			teamlist[attacker->s.team].stats.score--;
		else
			teamlist[attacker->s.team].stats.score++;
	}

	// drop items
	if( targ->r.client && !( G_PointContents( targ->s.origin ) & CONTENTS_NODROP ) )
	{
		// drop the weapon
		if ( targ->r.client->ps.stats[STAT_WEAPON] > WEAP_GUNBLADE )
		{
			gsitem_t *weaponItem = GS_FindItemByTag( targ->r.client->ps.stats[STAT_WEAPON] );
			if( weaponItem )
			{
				edict_t *drop = Drop_Item( targ, weaponItem );
				if( drop )
				{
					drop->count = targ->r.client->ps.inventory[ weaponItem->weakammo_tag ];
					targ->r.client->ps.inventory[ weaponItem->weakammo_tag ] = 0;
				}
			}
		}

		// drop ammo pack (won't drop anything if player doesn't have any strong ammo)
		Drop_Item( targ, GS_FindItemByTag( AMMO_PACK_WEAK ) );
	}
}

static void G_Gametype_GENERIC_PlayerDamaged( edict_t *targ, edict_t *attacker, int damage )
{
}

void G_Gametype_GENERIC_ScoreEvent( gclient_t *client, const char *score_event, const char *args )
{
	edict_t *attacker = NULL;
	int arg1, arg2;

	if( !score_event || !score_event[0] )
		return;

	if( !client )
		return;

	if( !Q_stricmp( score_event, "dmg" ) )
	{
		if( args )
		{
			if( client )
				attacker = &game.edicts[ client - game.clients + 1 ];

			arg1 = atoi( COM_Parse( &args ) );
			arg2 = atoi( COM_Parse( &args ) );

			G_Gametype_GENERIC_PlayerDamaged( &game.edicts[arg1], attacker, arg2 );
		}
	}
	else if( !Q_stricmp( score_event, "kill" ) )
	{
		if( args )
		{
			if( client )
				attacker = &game.edicts[ client - game.clients + 1 ];

			arg1 = atoi( COM_Parse( &args ) );
			arg2 = atoi( COM_Parse( &args ) );

			G_Gametype_GENERIC_PlayerKilled( &game.edicts[arg1], attacker, arg2 != -1 ? &game.edicts[arg2] : NULL );
		}
	}
}

static void G_Gametype_GENERIC_Init( void )
{
	trap_ConfigString( CS_GAMETYPETITLE, "Gametype failed to load" );
	trap_ConfigString( CS_GAMETYPEVERSION, "0.0" );
	trap_ConfigString( CS_GAMETYPEAUTHOR, "Warsow Development Team" );
	trap_Cvar_ForceSet( "g_gametype", "error" );

	level.gametype.spawnableItemsMask = ( IT_WEAPON|IT_AMMO|IT_ARMOR|IT_POWERUP|IT_HEALTH );
	level.gametype.respawnableItemsMask = ( IT_WEAPON|IT_AMMO|IT_ARMOR|IT_POWERUP|IT_HEALTH );
	level.gametype.dropableItemsMask = ( IT_WEAPON|IT_AMMO|IT_ARMOR|IT_POWERUP|IT_HEALTH );
	level.gametype.pickableItemsMask = ( level.gametype.spawnableItemsMask|level.gametype.dropableItemsMask );
	if( GS_Instagib() )
		level.gametype.pickableItemsMask &= ~G_INSTAGIB_NEGATE_ITEMMASK;

	level.gametype.isTeamBased = qfalse;
	level.gametype.isRace = qfalse;
	level.gametype.hasChallengersQueue = qfalse;
	level.gametype.maxPlayersPerTeam = 0;

	level.gametype.ammo_respawn = 20;
	level.gametype.armor_respawn = 25;
	level.gametype.weapon_respawn = 5;
	level.gametype.health_respawn = 25;
	level.gametype.powerup_respawn = 90;
	level.gametype.megahealth_respawn = 20;
	level.gametype.ultrahealth_respawn = 60;

	level.gametype.countdownEnabled = qfalse;
	level.gametype.mathAbortDisabled = qfalse;
	level.gametype.canForceModels = qtrue;
	level.gametype.canShowMinimap = qfalse;
	level.gametype.teamOnlyMinimap = qtrue;
	level.gametype.spawnpoint_radius = 256;

	level.gametype.canShowMinimap = qfalse;
	level.gametype.teamOnlyMinimap = qtrue;

	if( GS_Instagib() )
		level.gametype.spawnpoint_radius *= 2;

	trap_ConfigString( CS_SCB_PLAYERTAB_LAYOUT, "%n 164 %i 64 %l 48 %p 18 %p 18" );
	trap_ConfigString( CS_SCB_PLAYERTAB_TITLES, "Name Score Ping C R" );
}

//==========================================================
//					Matches
//==========================================================

cvar_t *g_warmup_timelimit;
cvar_t *g_postmatch_timelimit;
cvar_t *g_match_extendedtime;
cvar_t *g_countdown_time;
cvar_t *g_votable_gametypes;
cvar_t *g_scorelimit;
cvar_t *g_timelimit;
cvar_t *g_gametype;
cvar_t *g_gametypes_list;

//==========================================================
//					Matches
//==========================================================

/*
* G_GetGameState
*/
game_state_t *G_GetGameState( void )
{
	return &gs.gameState;
}

/*
* G_Match_Tied
*/
qboolean G_Match_Tied( void )
{
	int team, total, numteams;

	total = 0; numteams = 0;
	for( team = TEAM_ALPHA; team < GS_MAX_TEAMS; team++ )
	{
		if( !teamlist[team].numplayers )
			continue;

		numteams++;
		total += teamlist[team].stats.score;
	}

	if( numteams < 2 )
		return qfalse;
	else
	{

		// total / numteams = averaged score
		for( team = TEAM_ALPHA; team < GS_MAX_TEAMS; team++ )
		{
			if( !teamlist[team].numplayers )
				continue;

			if( teamlist[team].stats.score != total / numteams )
			{
				return qfalse;
			}
		}
	}

	return qtrue;
}

/*
* G_Match_CheckExtendPlayTime
*/
qboolean G_Match_CheckExtendPlayTime( void )
{
	// check for extended time/sudden death
	if( GS_MatchState() != MATCH_STATE_PLAYTIME )
		return qfalse;

	if( GS_TeamBasedGametype() && !level.forceExit )
	{
		if( G_Match_Tied() )
		{
			GS_GamestatSetFlag( GAMESTAT_FLAG_MATCHEXTENDED, qtrue );
			gs.gameState.stats[GAMESTAT_MATCHSTATE] = MATCH_STATE_PLAYTIME;
			gs.gameState.longstats[GAMELONG_MATCHSTART] = game.serverTime;

			if( g_match_extendedtime->value )
			{
				if( !GS_MatchExtended() )  // first one
					G_AnnouncerSound( NULL, trap_SoundIndex( S_ANNOUNCER_OVERTIME_GOING_TO_OVERTIME ), GS_MAX_TEAMS, qtrue, NULL );
				else
					G_AnnouncerSound( NULL, trap_SoundIndex( S_ANNOUNCER_OVERTIME_OVERTIME ), GS_MAX_TEAMS, qtrue, NULL );

				G_PrintMsg( NULL, "Match tied. Timelimit extended by %i minutes!\n", g_match_extendedtime->integer );
				G_CenterPrintMsg( NULL, "%i MINUTE OVERTIME\n", g_match_extendedtime->integer );
				gs.gameState.longstats[GAMELONG_MATCHDURATION] = (unsigned int)( ( fabs( g_match_extendedtime->value ) * 60 ) * 1000 );
			}
			else
			{
				G_AnnouncerSound( NULL, trap_SoundIndex( va( S_ANNOUNCER_OVERTIME_SUDDENDEATH_1_to_2, ( rand()&1 )+1 ) ), GS_MAX_TEAMS, qtrue, NULL );
				G_PrintMsg( NULL, "Match tied. Sudden death!\n" );
				G_CenterPrintMsg( NULL, "SUDDEN DEATH\n" );
				gs.gameState.longstats[GAMELONG_MATCHDURATION] = 0;
			}

			return qtrue;
		}
	}

	return qfalse;
}

/*
* G_Match_SetAutorecordState
*/
static void G_Match_SetAutorecordState( const char *state )
{
	trap_ConfigString( CS_AUTORECORDSTATE, state );
}

/*
* G_Match_Autorecord_Start
*/
void G_Match_Autorecord_Start( void )
{
	int team, i, playerCount;

	G_Match_SetAutorecordState( "start" );

	// do not start autorecording if all playing clients are bots
	for( playerCount = 0, team = TEAM_PLAYERS; team < GS_MAX_TEAMS; team++ )
	{
		// add our team info to the string
		for( i = 0; teamlist[team].playerIndices[i] != -1; i++ )
		{
			if( game.edicts[ teamlist[team].playerIndices[i] ].r.svflags & SVF_FAKECLIENT )
				continue;

			playerCount++;
			break; // we only need one for this check
		}
	}

	if( playerCount && g_autorecord->integer )
	{
		char name[MAX_STRING_CHARS], datetime[17], players[MAX_STRING_CHARS];
		time_t long_time;
		struct tm *newtime;

		// date & time
		time( &long_time );
		newtime = localtime( &long_time );

		Q_snprintfz( datetime, sizeof( datetime ), "%04d-%02d-%02d_%02d-%02d", newtime->tm_year + 1900,
			newtime->tm_mon+1, newtime->tm_mday, newtime->tm_hour, newtime->tm_min );

		// list of players
		Q_strncpyz( players, trap_GetConfigString( CS_MATCHNAME ), sizeof( players ) );
		if( players[0] == '\0' )
		{
			if( GS_InvidualGameType() )
			{
				const char *netname;
				int team;
				edict_t *ent;

				for( team = TEAM_ALPHA; team < GS_MAX_TEAMS; team++ )
				{
					if( !teamlist[team].numplayers )
						continue;
					ent = game.edicts + teamlist[team].playerIndices[0];
					netname = ent->r.client->netname;
					Q_strncatz( players, netname, sizeof( players ) );
					if( team != GS_MAX_TEAMS - 1 )
						Q_strncatz( players, " vs ", sizeof( players ) );
				}
			}
		}

		if( players[0] != '\0' )
		{
			char *t = strstr( players, " vs " );
			if( t )
				memcpy( t, "_vs_", strlen( "_vs_" ) );
			Q_strncpyz( players, COM_RemoveJunkChars( COM_RemoveColorTokens( players ) ), sizeof( players ) );
		}

		// combine
		Q_snprintfz( name, sizeof( name ), "%s_%s_%s%s%s", datetime, gs.gametypeName,
			level.mapname, players[0] == '\0' ? "" : "_", players );

		trap_Cmd_ExecuteText( EXEC_APPEND, va( "serverrecord \"%s\" %i\n", name, (int)brandom( 1, 9999 ) ) );
	}
}

/*
* G_Match_Autorecord_AltStart
*/
void G_Match_Autorecord_AltStart( void )
{
	G_Match_SetAutorecordState( "altstart" );
}

/*
* G_Match_Autorecord_Stats
*/
void G_Match_Autorecord_Stats( void )
{
	edict_t *ent;

	for( ent = game.edicts + 1; PLAYERNUM( ent ) < gs.maxclients; ent++ )
	{
		if( !ent->r.inuse || ent->s.team == TEAM_SPECTATOR || ( ent->r.svflags & SVF_FAKECLIENT ) )
			continue;
		trap_GameCmd( ent, va( "plstats 2 \"%s\"", G_StatsMessage( ent ) ) );
	}
}

/*
* G_Match_Autorecord_Stop
*/
void G_Match_Autorecord_Stop( void )
{
	G_Match_SetAutorecordState( "stop" );

	if( g_autorecord->integer )
	{
		// stop it
		trap_Cmd_ExecuteText( EXEC_APPEND, "serverrecordstop\n" );

		// check if we wanna delete some
		if( g_autorecord_maxdemos->integer > 0 )
			trap_Cmd_ExecuteText( EXEC_APPEND, va( "serverrecordpurge %i\n", g_autorecord_maxdemos->integer ) );
	}
}

/*
* G_Match_Autorecord_Cancel
*/
void G_Match_Autorecord_Cancel( void )
{
	G_Match_SetAutorecordState( "cancel" );

	if( g_autorecord->integer )
		trap_Cmd_ExecuteText( EXEC_APPEND, "serverrecordcancel\n" );
}

/*
* G_Match_CheckStateAbort
*/
static void G_Match_CheckStateAbort( void )
{
	qboolean any = qfalse;
	qboolean enough;

	if( GS_MatchState() <= MATCH_STATE_NONE || GS_MatchState() >= MATCH_STATE_POSTMATCH
		|| level.gametype.mathAbortDisabled )
	{
		GS_GamestatSetFlag( GAMESTAT_FLAG_WAITING, qfalse );
		return;
	}

	if( GS_TeamBasedGametype() )
	{
		int team, emptyteams = 0;

		for( team = TEAM_ALPHA; team < GS_MAX_TEAMS; team++ )
		{
			if( !teamlist[team].numplayers )
				emptyteams++;
			else
				any = qtrue;
		}

		enough = ( emptyteams == 0 );
	}
	else
	{
		enough = ( teamlist[TEAM_PLAYERS].numplayers > 1 );
		any = ( teamlist[TEAM_PLAYERS].numplayers > 0 );
	}

	// if waiting, turn on match states when enough players joined
	if( GS_MatchWaiting() && enough )
	{
		GS_GamestatSetFlag( GAMESTAT_FLAG_WAITING, qfalse );
		G_UpdatePlayersMatchMsgs();
	}
	// turn off active match states if not enough players left
	else if( GS_MatchState() == MATCH_STATE_WARMUP && !enough && GS_MatchDuration() )
	{
		GS_GamestatSetFlag( GAMESTAT_FLAG_WAITING, qtrue );
		G_UpdatePlayersMatchMsgs();
	}
	else if( GS_MatchState() == MATCH_STATE_COUNTDOWN && !enough )
	{
		G_PrintMsg( NULL, "Not enough players left. Countdown aborted.\n" );
		G_CenterPrintMsg( NULL, "COUNTDOWN ABORTED\n" );
		G_Match_Autorecord_Cancel();
		G_Match_LaunchState( MATCH_STATE_WARMUP );
		GS_GamestatSetFlag( GAMESTAT_FLAG_WAITING, qtrue );
		G_UpdatePlayersMatchMsgs();
	}
	// match running, but not enough players left
	else if( GS_MatchState() == MATCH_STATE_PLAYTIME && !enough )
	{
		G_PrintMsg( NULL, "Not enough players left. Match aborted.\n" );
		G_CenterPrintMsg( NULL, "MATCH ABORTED\n" );
		G_EndMatch();
	}
}

/*
* G_Match_LaunchState
*/
void G_Match_LaunchState( int matchState )
{
	static qboolean advance_queue = qfalse;

	// There isn't any script, run generic fuction
	if( level.gametype.asEngineHandle < 0 )
	{
		if( !G_Gametype_GENERIC_MatchStateFinished( matchState ) )
			return;
	}
	else
	{
		// give the gametype a chance to refuse the state change, or to set up things for it
		if( !G_asCallMatchStateFinishedScript( matchState ) )
			return;
	}

	GS_GamestatSetFlag( GAMESTAT_FLAG_MATCHEXTENDED, qfalse );
	GS_GamestatSetFlag( GAMESTAT_FLAG_WAITING, qfalse );

	switch( matchState )
	{
	default:
	case MATCH_STATE_WARMUP:
		{
			advance_queue = qfalse;
			level.forceStart = qfalse;

			gs.gameState.stats[GAMESTAT_MATCHSTATE] = MATCH_STATE_WARMUP;
			gs.gameState.longstats[GAMELONG_MATCHDURATION] = (unsigned int)( fabs( g_warmup_timelimit->value * 60 ) * 1000 );
			gs.gameState.longstats[GAMELONG_MATCHSTART] = game.serverTime;

			break;
		}

	case MATCH_STATE_COUNTDOWN:
		{
			advance_queue = qtrue;

			gs.gameState.stats[GAMESTAT_MATCHSTATE] = MATCH_STATE_COUNTDOWN;
			gs.gameState.longstats[GAMELONG_MATCHDURATION] = (unsigned int)( fabs( g_countdown_time->value ) * 1000 );
			gs.gameState.longstats[GAMELONG_MATCHSTART] = game.serverTime;

			break;
		}

	case MATCH_STATE_PLAYTIME:
		{
			advance_queue = qtrue; // shouldn't be needed here
			level.forceStart = qfalse;

			gs.gameState.stats[GAMESTAT_MATCHSTATE] = MATCH_STATE_PLAYTIME;
			gs.gameState.longstats[GAMELONG_MATCHDURATION] = (unsigned int)( fabs( 60 * g_timelimit->value )*1000 );
			gs.gameState.longstats[GAMELONG_MATCHSTART] = game.serverTime;
		}
		break;

	case MATCH_STATE_POSTMATCH:
		{
			gs.gameState.stats[GAMESTAT_MATCHSTATE] = MATCH_STATE_POSTMATCH;
			gs.gameState.longstats[GAMELONG_MATCHDURATION] = (unsigned int)fabs( g_postmatch_timelimit->value * 1000 ); // postmatch time in seconds
			gs.gameState.longstats[GAMELONG_MATCHSTART] = game.serverTime;

			G_Timeout_Reset();
			level.teamlock = qfalse;
			level.forceExit = qfalse;

			G_Match_Autorecord_Stats();
		}
		break;

	case MATCH_STATE_WAITEXIT:
		{
			if( advance_queue )
			{
				G_Teams_AdvanceChallengersQueue();
				advance_queue = qtrue;
			}

			gs.gameState.stats[GAMESTAT_MATCHSTATE] = MATCH_STATE_WAITEXIT;
			gs.gameState.longstats[GAMELONG_MATCHDURATION] = (unsigned int)fabs( 25000 );
			gs.gameState.longstats[GAMELONG_MATCHSTART] = game.serverTime;

			level.exitNow = qfalse;
		}
		break;
	}

	// give the gametype the chance to setup for the new state

	// There isn't any script, run generic fuction
	if( level.gametype.asEngineHandle < 0 )
		G_Gametype_GENERIC_MatchStateStarted();
	else
		G_asCallMatchStateStartedScript();

	G_UpdatePlayersMatchMsgs();
}

/*
* G_Match_ScorelimitHit
*/
qboolean G_Match_ScorelimitHit( void )
{
	edict_t	*e;

	if( GS_MatchState() != MATCH_STATE_PLAYTIME )
		return qfalse;

	if( g_scorelimit->integer )
	{
		if( !GS_TeamBasedGametype() )
		{
			for( e = game.edicts+1; PLAYERNUM( e ) < gs.maxclients; e++ )
			{
				if( !e->r.inuse )
					continue;

				if( e->r.client->level.stats.score >= g_scorelimit->integer )
					return qtrue;
			}
		}
		else
		{
			int team;

			for( team = TEAM_ALPHA; team < GS_MAX_TEAMS; team++ )
			{
				if( teamlist[team].stats.score >= g_scorelimit->integer )
					return qtrue;
			}
		}
	}

	return qfalse;
}

/*
* G_Match_SuddenDeathFinished
*/
qboolean G_Match_SuddenDeathFinished( void )
{
	if( GS_MatchState() != MATCH_STATE_PLAYTIME )
		return qfalse;

	if( !GS_MatchExtended() || GS_MatchDuration() )
		return qfalse;

	return !G_Match_Tied();
}

/*
* G_Match_TimelimitHit
*/
qboolean G_Match_TimelimitHit( void )
{
	// check for timelimit hit
	if( !GS_MatchDuration() || game.serverTime < GS_MatchEndTime() )
		return qfalse;

	if( GS_MatchState() == MATCH_STATE_WARMUP )
		level.forceStart = qtrue; // force match starting when timelimit is up, even if someone goes unready

	if( GS_MatchState() == MATCH_STATE_WAITEXIT )
	{
		level.exitNow = qtrue;
		return qfalse; // don't advance into next state. The match will be restarted
	}

	return qtrue;
}


static qboolean score_announcement_init = qfalse;
static int last_leaders[MAX_CLIENTS];
static int leaders[MAX_CLIENTS];
#define G_ANNOUNCER_READYUP_DELAY 20000; // milliseconds

/*
* G_IsLeading
*/
static qboolean G_IsLeading( edict_t *ent )
{
	int num, i;

	if( GS_TeamBasedGametype() )
		num = ent->s.team;
	else
		num = PLAYERNUM( ent ) + 1;

	for( i = 0; i < MAX_CLIENTS && leaders[i] != 0; i++ )
	{
		if( leaders[i] == num )
			return qtrue;
	}

	return qfalse;
}

/*
* G_WasLeading
*/
static qboolean G_WasLeading( edict_t *ent )
{
	int num, i;

	if( GS_TeamBasedGametype() )
		num = ent->s.team;
	else
		num = PLAYERNUM( ent ) + 1;

	for( i = 0; i < MAX_CLIENTS && last_leaders[i] != 0; i++ )
	{
		if( last_leaders[i] == num )
			return qtrue;
	}

	return qfalse;
}

/*
* G_Match_ScoreAnnouncement
*/
static void G_Match_ScoreAnnouncement( void )
{
	int i;
	edict_t *e, *chased;
	int num_leaders, team;

	if( !level.gametype.scoreAnnouncementEnabled )
		return;

	num_leaders = 0;
	memset( leaders, 0, sizeof( leaders ) );

	if( GS_TeamBasedGametype() )
	{
		int score_max = -999999999;

		for( team = TEAM_ALPHA; team < GS_MAX_TEAMS; team++ )
		{
			if( !teamlist[team].numplayers )
				continue;

			if( teamlist[team].stats.score > score_max )
			{
				score_max = teamlist[team].stats.score;
				leaders[0] = team;
				num_leaders = 1;
			}
			else if( teamlist[team].stats.score == score_max )
			{
				leaders[num_leaders++] = team;
			}
		}
		leaders[num_leaders] = 0;
	}
	else
	{
		int score_max = -999999999;

		for( i = 0; i < MAX_CLIENTS && teamlist[TEAM_PLAYERS].playerIndices[i] != -1; i++ )
		{
			if( game.clients[teamlist[TEAM_PLAYERS].playerIndices[i]-1].level.stats.score > score_max )
			{
				score_max = game.clients[teamlist[TEAM_PLAYERS].playerIndices[i]-1].level.stats.score;
				leaders[0] = teamlist[TEAM_PLAYERS].playerIndices[i];
				num_leaders = 1;
			}
			else if( game.clients[teamlist[TEAM_PLAYERS].playerIndices[i]-1].level.stats.score == score_max )
			{
				leaders[num_leaders++] = teamlist[TEAM_PLAYERS].playerIndices[i];
			}
		}
		leaders[num_leaders] = 0;
	}

	if( !score_announcement_init )
	{
		// copy over to last_leaders
		memcpy( last_leaders, leaders, sizeof( leaders ) );
		score_announcement_init = qtrue;
		return;
	}

	for( e = game.edicts + 1; PLAYERNUM( e ) < gs.maxclients; e++ )
	{
		if( !e->r.client || trap_GetClientState( PLAYERNUM( e ) ) < CS_SPAWNED )
			continue;

		if( e->r.client->resp.chase.active )
			chased = &game.edicts[e->r.client->resp.chase.target];
		else
			chased = e;

		// floating spectator
		if( chased->s.team == TEAM_SPECTATOR )
		{
			if( !GS_TeamBasedGametype() )
				continue;

			if( last_leaders[1] == 0 && leaders[1] != 0 )
			{
				G_AnnouncerSound( e, trap_SoundIndex( va( S_ANNOUNCER_SCORE_TEAM_TIED_LEAD_1_to_2, ( rand()&1 )+1 ) ),
					GS_MAX_TEAMS, qtrue, NULL );
			}
			else if( leaders[1] == 0 && ( last_leaders[0] != leaders[0] || last_leaders[1] != 0 ) )
			{
				//G_AnnouncerSound( e, trap_SoundIndex( va( S_ANNOUNCER_SCORE_TEAM_1_to_4_TAKEN_LEAD_1_to_2,
				//	leaders[0]-1, ( rand()&1 )+1 ) ), GS_MAX_TEAMS, qtrue, NULL );
			}
			continue;
		}

		// in the game or chasing someone who is
		if( G_WasLeading( chased ) && !G_IsLeading( chased ) )
		{
			if( GS_TeamBasedGametype() && !GS_InvidualGameType() )
				G_AnnouncerSound( e, trap_SoundIndex( va( S_ANNOUNCER_SCORE_TEAM_LOST_LEAD_1_to_2, ( rand()&1 )+1 ) ),
				GS_MAX_TEAMS, qtrue, NULL );
			else
				G_AnnouncerSound( e, trap_SoundIndex( va( S_ANNOUNCER_SCORE_LOST_LEAD_1_to_2, ( rand()&1 )+1 ) ),
				GS_MAX_TEAMS, qtrue, NULL );
		}
		else if( ( !G_WasLeading( chased ) || ( last_leaders[1] != 0 ) ) && G_IsLeading( chased ) && ( leaders[1] == 0 ) )
		{
			if( GS_TeamBasedGametype() && !GS_InvidualGameType() )
				G_AnnouncerSound( e, trap_SoundIndex( va( S_ANNOUNCER_SCORE_TEAM_TAKEN_LEAD_1_to_2, ( rand()&1 )+1 ) ),
				GS_MAX_TEAMS, qtrue, NULL );
			else
				G_AnnouncerSound( e, trap_SoundIndex( va( S_ANNOUNCER_SCORE_TAKEN_LEAD_1_to_2, ( rand()&1 )+1 ) ),
				GS_MAX_TEAMS, qtrue, NULL );
		}
		else if( ( !G_WasLeading( chased ) || ( last_leaders[1] == 0 ) ) && G_IsLeading( chased ) && ( leaders[1] != 0 ) )
		{
			if( GS_TeamBasedGametype() && !GS_InvidualGameType() )
				G_AnnouncerSound( e, trap_SoundIndex( va( S_ANNOUNCER_SCORE_TEAM_TIED_LEAD_1_to_2, ( rand()&1 )+1 ) ),
				GS_MAX_TEAMS, qtrue, NULL );
			else
				G_AnnouncerSound( e, trap_SoundIndex( va( S_ANNOUNCER_SCORE_TIED_LEAD_1_to_2, ( rand()&1 )+1 ) ),
				GS_MAX_TEAMS, qtrue, NULL );
		}
	}

	// copy over to last_leaders
	memcpy( last_leaders, leaders, sizeof( leaders ) );
}

/*
* G_Match_ReadyAnnouncement
*/
static void G_Match_ReadyAnnouncement( void )
{
	int i;
	edict_t *e;
	int team;
	qboolean readyupwarnings = qfalse;
	int START_TEAM, END_TEAM;

	if( !level.gametype.readyAnnouncementEnabled )
		return;

	// ready up announcements

	if( GS_TeamBasedGametype() )
	{
		START_TEAM = TEAM_ALPHA;
		END_TEAM = GS_MAX_TEAMS;
	}
	else
	{
		START_TEAM = TEAM_PLAYERS;
		END_TEAM = TEAM_PLAYERS+1;
	}

	for( team = START_TEAM; team < END_TEAM; team++ )
	{
		if( !teamlist[team].numplayers )
			continue;

		for( i = 0; i < teamlist[team].numplayers; i++ )
		{
			e = game.edicts + teamlist[team].playerIndices[i];
			if( e->r.svflags & SVF_FAKECLIENT )
				continue;

			if( level.ready[teamlist[team].playerIndices[i]-1] )
			{
				readyupwarnings = qtrue;
			}
		}
	}

	if( !readyupwarnings )
		return;

	// now let's repeat and warn
	for( team = START_TEAM; team < END_TEAM; team++ )
	{
		if( !teamlist[team].numplayers )
			continue;
		for( i = 0; i < teamlist[team].numplayers; i++ )
		{
			if( !level.ready[teamlist[team].playerIndices[i]-1] )
			{
				e = game.edicts + teamlist[team].playerIndices[i];
				if( !e->r.client || trap_GetClientState( PLAYERNUM( e ) ) != CS_SPAWNED )
					continue;

				if( e->r.client->teamstate.readyUpWarningNext < game.realtime )
				{
					e->r.client->teamstate.readyUpWarningNext = game.realtime + G_ANNOUNCER_READYUP_DELAY;
					e->r.client->teamstate.readyUpWarningCount++;
					if( e->r.client->teamstate.readyUpWarningCount > 3 )
					{
						G_AnnouncerSound( e, trap_SoundIndex( S_ANNOUNCER_READY_UP_PISSEDOFF ), GS_MAX_TEAMS, qtrue, NULL );
						e->r.client->teamstate.readyUpWarningCount = 0;
					}
					else
					{
						G_AnnouncerSound( e, trap_SoundIndex( S_ANNOUNCER_READY_UP_POLITE ), GS_MAX_TEAMS, qtrue, NULL );
					}
				}
			}
		}
	}
}

/*
* G_EndMatch
*/
void G_EndMatch( void )
{
	level.forceExit = qtrue;
	G_Match_LaunchState( MATCH_STATE_POSTMATCH );
}

/*
* G_Match_CheckReadys
*/
void G_Match_CheckReadys( void )
{
	edict_t *e;
	qboolean allready;
	int readys, notreadys, teamsready;
	int team, i;

	if( GS_MatchState() != MATCH_STATE_WARMUP && GS_MatchState() != MATCH_STATE_COUNTDOWN )
		return;

	if( GS_MatchState() == MATCH_STATE_COUNTDOWN && level.forceStart )
		return; // never stop countdown if we have run out of warmup_timelimit

	teamsready = 0;
	for( team = TEAM_PLAYERS; team < GS_MAX_TEAMS; team++ )
	{
		readys = notreadys = 0;
		for( i = 0; teamlist[team].playerIndices[i] != -1; i++ )
		{
			e = game.edicts + teamlist[team].playerIndices[i];

			if( !e->r.inuse )
				continue;
			if( e->s.team == TEAM_SPECTATOR )  //ignore spectators
				continue;

			if( level.ready[PLAYERNUM( e )] )
				readys++;
			else
				notreadys++;
		}
		if( !notreadys && readys )
			teamsready++;
	}

	// everyone has commited
	if( GS_TeamBasedGametype() )
	{
		if( teamsready == GS_MAX_TEAMS - TEAM_ALPHA )
			allready = qtrue;
		else
			allready = qfalse;
	}
	else
	{       //ffa
		if( teamsready && teamlist[TEAM_PLAYERS].numplayers > 1 )
			allready = qtrue;
		else
			allready = qfalse;
	}

	if( allready == qtrue && GS_MatchState() != MATCH_STATE_COUNTDOWN )
	{
		G_PrintMsg( NULL, "All players are ready.  Match starting!\n" );
		G_Match_LaunchState( MATCH_STATE_COUNTDOWN );
	}
	else if( allready == qfalse && GS_MatchState() == MATCH_STATE_COUNTDOWN )
	{
		G_PrintMsg( NULL, "Countdown aborted.\n" );
		G_CenterPrintMsg( NULL, "COUNTDOWN ABORTED\n" );
		G_Match_Autorecord_Cancel();
		G_Match_LaunchState( MATCH_STATE_WARMUP );
	}
}

/*
* G_Match_Ready
*/
void G_Match_Ready( edict_t *ent )
{
	if( ent->r.svflags & SVF_FAKECLIENT && level.ready[PLAYERNUM( ent )] == qtrue )
		return;

	if( ent->s.team == TEAM_SPECTATOR )
	{
		G_PrintMsg( ent, "Join the game first\n" );
		return;
	}

	if( GS_MatchState() != MATCH_STATE_WARMUP )
	{
		if( !( ent->r.svflags & SVF_FAKECLIENT ) )
			G_PrintMsg( ent, "We're not in warmup.\n" );
		return;
	}

	if( level.ready[PLAYERNUM( ent )] )
	{
		G_PrintMsg( ent, "You are already ready.\n" );
		return;
	}

	level.ready[PLAYERNUM( ent )] = qtrue;

	G_PrintMsg( NULL, "%s%s is ready!\n", ent->r.client->netname, S_COLOR_WHITE );

	G_UpdatePlayerMatchMsg( ent );

	G_Match_CheckReadys();
}

/*
* G_Match_NotReady
*/
void G_Match_NotReady( edict_t *ent )
{
	if( ent->s.team == TEAM_SPECTATOR )
	{
		G_PrintMsg( ent, "Join the game first\n" );
		return;
	}

	if( GS_MatchState() != MATCH_STATE_WARMUP && GS_MatchState() != MATCH_STATE_COUNTDOWN )
	{
		G_PrintMsg( ent, "A match is not being setup.\n" );
		return;
	}

	if( !level.ready[PLAYERNUM( ent )] )
	{
		G_PrintMsg( ent, "You weren't ready.\n" );
		return;
	}

	level.ready[PLAYERNUM( ent )] = qfalse;

	G_PrintMsg( NULL, "%s%s is no longer ready.\n", ent->r.client->netname, S_COLOR_WHITE );

	G_UpdatePlayerMatchMsg( ent );

	G_Match_CheckReadys();
}

/*
* G_Match_ToggleReady
*/
void G_Match_ToggleReady( edict_t *ent )
{
	if( !level.ready[PLAYERNUM( ent )] )
		G_Match_Ready( ent );
	else
		G_Match_NotReady( ent );
}

/*
* G_Match_RemoveAllProjectiles
*/
void G_Match_RemoveAllProjectiles( void )
{
	edict_t *ent;

	for( ent = game.edicts + gs.maxclients; ENTNUM( ent ) < game.numentities; ent++ )
	{
		if( ent->r.inuse && !ent->r.client && ent->r.svflags & SVF_PROJECTILE && ent->r.solid != SOLID_NOT )
		{
			G_FreeEdict( ent );
		}
	}
}

/*
* G_Match_FreeBodyQueue
*/
void G_Match_FreeBodyQueue( void )
{
	edict_t *ent;
	int i;

	ent = &game.edicts[gs.maxclients + 1];
	for( i = 0; i < BODY_QUEUE_SIZE; ent++, i++ )
	{
		if( !ent->r.inuse )
			continue;

		if( ent->classname && !Q_stricmp( ent->classname, "body" ) )
		{
			GClip_UnlinkEntity( ent );

			ent->deadflag = DEAD_NO;
			ent->movetype = MOVETYPE_NONE;
			ent->r.solid = SOLID_NOT;
			ent->r.svflags = SVF_NOCLIENT;

			ent->s.type = ET_GENERIC;
			ent->s.skinnum = 0;
			ent->s.frame = 0;
			ent->s.modelindex = 0;
			ent->s.sound = 0;
			ent->s.effects = 0;

			ent->takedamage = DAMAGE_NO;
			ent->flags |= FL_NO_KNOCKBACK;

			GClip_LinkEntity( ent );
		}
	}

	level.body_que = 0;
}

//======================================================
//		Game types
//======================================================

/*
* G_Gametype_IsVotable
*/
qboolean G_Gametype_IsVotable( const char *name )
{
	char *ptr = g_votable_gametypes->string;
	char *validname;

	if( !name )
		return qfalse;

	// if the votable gametypes list is empty, allow all but SP
	if( ptr == NULL || ptr[0] == 0 )
		return qtrue;

	// check for the gametype being in the votable gametypes list
	while( ptr && *ptr )
	{
		validname = COM_Parse( &ptr );
		if( !validname[0] )
			break;

		if( !Q_stricmp( validname, name ) )
			return qtrue;
	}

	return qfalse;
}

/*
* G_Gametype_CanPickUpItem
*/
qboolean G_Gametype_CanPickUpItem( gsitem_t *item )
{
	if( !item )
		return qfalse;

	return ( item->type & level.gametype.pickableItemsMask );
}

/*
* G_Gametype_CanSpawnItem
*/
qboolean G_Gametype_CanSpawnItem( gsitem_t *item )
{
	if( !item )
		return qfalse;

	return ( level.gametype.spawnableItemsMask & item->type );
}

/*
* G_Gametype_CanRespawnItem
*/
qboolean G_Gametype_CanRespawnItem( gsitem_t *item )
{
	int itemmask;

	if( !item )
		return qfalse;

	itemmask = level.gametype.respawnableItemsMask;
	if( GS_Instagib() )
	{
		itemmask &= ~G_INSTAGIB_NEGATE_ITEMMASK;
	}

	return ( ( itemmask & item->type ) != 0 );
}

/*
* G_Gametype_CanDropItem
*/
qboolean G_Gametype_CanDropItem( gsitem_t *item, qboolean ignoreMatchState )
{
	int itemmask;

	if( !item )
		return qfalse;

	if( !ignoreMatchState )
	{
		if( GS_MatchState() > MATCH_STATE_PLAYTIME )
			return qfalse;
	}

	itemmask = level.gametype.dropableItemsMask;
	if( GS_Instagib() )
	{
		itemmask &= ~G_INSTAGIB_NEGATE_ITEMMASK;
	}

	return ( itemmask & item->type );
}

/*
* G_Gametype_CanTeamDamage
*/
qboolean G_Gametype_CanTeamDamage( int damageflags )
{
	if( damageflags & DAMAGE_NO_PROTECTION )
		return qtrue;

	if( !GS_TeamBasedGametype() )
		return qtrue;

	return g_allow_teamdamage->integer;
}

/*
* G_Gametype_RespawnTimeForItem
*/
int G_Gametype_RespawnTimeForItem( gsitem_t *item )
{
	if( !item )
		return -1; // free the edict

	if( item->type & IT_AMMO )
	{
		if( g_ammo_respawn->value > 0.0f )
			return g_ammo_respawn->value * 1000;

		return level.gametype.ammo_respawn * 1000;
	}

	if( item->type & IT_WEAPON )
	{
		if( g_weapon_respawn->value > 0.0f )
			return g_weapon_respawn->value * 1000;

		return level.gametype.weapon_respawn * 1000;
	}

	if( item->tag == HEALTH_MEGA )
	{
		return level.gametype.megahealth_respawn * 1000;
	}

	if( item->tag == HEALTH_ULTRA )
	{
		return level.gametype.ultrahealth_respawn * 1000;
	}

	if( item->type & IT_HEALTH )
	{
		if( g_health_respawn->value > 0 )
			return g_health_respawn->value * 1000;

		return level.gametype.health_respawn * 1000;
	}

	if( item->type & IT_ARMOR )
	{
		if( g_armor_respawn->value > 0 )
			return g_armor_respawn->value * 1000;

		return level.gametype.armor_respawn * 1000;
	}

	if( item->type & IT_POWERUP )
	{
		return level.gametype.powerup_respawn * 1000;
	}

	return item->quantity * 1000;
}

/*
* G_Gametype_DroppedItemTimeout
*/
int G_Gametype_DroppedItemTimeout( gsitem_t *item )
{
	// to do: add cvar
	return 29;
}

/*
* G_EachNewSecond
*/
static qboolean G_EachNewSecond( void )
{
	static int lastsecond;
	static int second;

	second = (int)( level.time * 0.001 );
	if( lastsecond == second )
		return qfalse;

	lastsecond = second;
	return qtrue;
}

/*
* G_CheckNumBots
*/
static void G_CheckNumBots( void )
{
	edict_t	*ent;

	if( level.spawnedTimeStamp + 5000 > game.realtime )
		return;

	// check sanity of g_numbots
	if( g_numbots->integer < 0 )
		trap_Cvar_Set( "g_numbots", "0" );

	if( g_numbots->integer > gs.maxclients )
		trap_Cvar_Set( "g_numbots", va( "%i", gs.maxclients ) );

	if( g_numbots->integer < game.numBots )
	{   // kick one bot
		for( ent = game.edicts + gs.maxclients; PLAYERNUM( ent ) >= 0; ent-- )
		{
			if( !ent->r.inuse || !( ent->r.svflags & SVF_FAKECLIENT ) )
				continue;
			if( ent->ai.type == AI_ISBOT )
			{
				trap_DropClient( ent, DROP_TYPE_GENERAL, NULL );
				break;
			}
		}
		return;
	}

	if( g_numbots->integer > game.numBots )
	{                                     // add a bot if there is room
		for( ent = game.edicts + 1; PLAYERNUM( ent ) < gs.maxclients && game.numBots < g_numbots->integer; ent++ )
		{
			if( !ent->r.inuse && trap_GetClientState( PLAYERNUM( ent ) ) == CS_FREE )
				BOT_SpawnBot( NULL );
		}
	}
}

/*
* G_TickOutPowerUps
*/
static void G_TickOutPowerUps( void )
{
	edict_t *ent;
	gsitem_t *item;
	int i;

	for( ent = game.edicts + 1; PLAYERNUM( ent ) < gs.maxclients; ent++ )
	{
		if( ent->r.inuse && trap_GetClientState( PLAYERNUM( ent ) ) >= CS_SPAWNED )
		{
			for( i = POWERUP_QUAD; i < POWERUP_TOTAL; i++ )
			{
				item = GS_FindItemByTag( i );
				if( item && item->quantity && ent->r.client->ps.inventory[item->tag] > 0 )
				{
					ent->r.client->ps.inventory[item->tag]--;
				}
			}
		}
	}

	// also tick out dropped powerups
	for( ent = game.edicts + gs.maxclients + BODY_QUEUE_SIZE; ENTNUM( ent ) < game.numentities; ent++ )
	{
		if( !ent->r.inuse || !ent->item )
			continue;

		if( !( ent->item->type & IT_POWERUP ) )
			continue;

		if( ent->spawnflags & ( DROPPED_ITEM|DROPPED_PLAYER_ITEM ) )
		{
			ent->count--;
			if( ent->count <= 0 )
			{
				G_FreeEdict( ent );
				continue;
			}
		}
	}
}

/*
* G_EachNewMinute
*/
static qboolean G_EachNewMinute( void )
{
	static int lastminute;
	static int minute;

	minute = (int)( level.time*0.001 / 60.0f );
	if( lastminute == minute )
		return qfalse;

	lastminute = minute;
	return qtrue;
}

/*
* G_CheckEvenTeam
*/
static void G_CheckEvenTeam( void )
{
	int max = 0;
	int min = gs.maxclients + 1;
	int uneven_team = TEAM_SPECTATOR;
	int i;

	if( GS_MatchState() >= MATCH_STATE_POSTMATCH )
		return;

	if( !GS_TeamBasedGametype() )
		return;

	if( g_teams_allow_uneven->integer )
		return;

	for( i = TEAM_ALPHA; i < GS_MAX_TEAMS; i++ )
	{
		if( max < teamlist[i].numplayers )
		{
			max = teamlist[i].numplayers;
			uneven_team = i;
		}
		if( min > teamlist[i].numplayers )
			min = teamlist[i].numplayers;
	}

	if( max - min > 1 )
	{
		for( i = 0; teamlist[uneven_team].playerIndices[i] != -1; i++ )
		{
			edict_t	*e = game.edicts + teamlist[uneven_team].playerIndices[i];
			if( !e->r.inuse )
				continue;
			G_CenterPrintMsg( e, "Teams are uneven. Please switch into another team.\n" ); // FIXME: need more suitable message :P
			G_PrintMsg( e, "%sTeams are uneven. Please switch into another team.\n", S_COLOR_CYAN ); // FIXME: need more suitable message :P
		}
		// FIXME: switch team forcibly?
	}
}

/*
* G_Gametype_ScoreEvent
*/
void G_Gametype_ScoreEvent( gclient_t *client, const char *score_event, const char *args )
{
	if( !score_event || !score_event[0] )
		return;

	if( level.gametype.asEngineHandle < 0 )
		G_Gametype_GENERIC_ScoreEvent( client, score_event, args );
	else
		G_asCallScoreEventScript( client, score_event, args );
}

/*
* G_RunGametype
*/
void G_RunGametype( void )
{
	G_Teams_ExecuteChallengersQueue();
	G_Teams_UpdateMembersList();
	G_Match_CheckStateAbort();

	G_UpdateScoreBoardMessages();

	//check gametype specific rules
	if( level.gametype.asEngineHandle < 0 )
		G_Gametype_GENERIC_ThinkRules();
	else
		G_asCallThinkRulesScript();

	if( G_EachNewSecond() )
	{
		G_CheckNumBots();
		G_TickOutPowerUps();
	}

	if( G_EachNewMinute() )
		G_CheckEvenTeam();

	G_Match_ScoreAnnouncement();
	G_Match_ReadyAnnouncement();

	if( GS_TeamBasedGametype() )
		G_Teams_UpdateTeamInfoMessages();

	G_asGarbageCollect( qfalse );
}

//======================================================
//		Game type registration
//======================================================

/*
* G_ChecksumGametypes_f
*/
void G_ChecksumGametypes_f( void )
{
	const char *basePath = "progs/gametypes";
	const char *gamedir = trap_Cvar_String( "fs_game" );
	char *name, *sectionName;
	char filename[MAX_STRING_CHARS];
	int count, sectionNum;
	int length, filenum;
	qbyte *data;

	if( !level.canSpawnEntities )
	{
		G_Printf( "The level is being reinitialized, retry in a second\n" );
		return;
	}

	for( count = 0; ( name = G_ListNameForPosition( g_gametypes_list->string, count, CHAR_GAMETYPE_SEPARATOR ) ) != NULL; count++ )
	{
		trap_Cmd_ExecuteText( EXEC_APPEND, va( "fs_checksum %s/%s/%s%s\n", gamedir, basePath, name, GAMETYPE_PROJECT_EXTENSION ) );

		Q_snprintfz( filename, sizeof( filename ), "progs/gametypes/%s%s", name, GAMETYPE_PROJECT_EXTENSION );
		Q_strlwr( filename );
		if( ( length = trap_FS_FOpenFile( filename, &filenum, FS_READ ) ) == -1 )
		{
			G_Printf( "WARNING: Couldn't find %s\n", filename );
			continue;
		}

		if( !length )
		{
			trap_FS_FCloseFile( filenum );
			continue;
		}

		// load the definition file into memory
		data = G_Malloc( length + 1 );
		trap_FS_Read( data, length, filenum );
		trap_FS_FCloseFile( filenum );

		// do it
		for( sectionNum = 0; ( sectionName = G_ListNameForPosition( ( const char * )data, sectionNum, CHAR_GAMETYPE_SEPARATOR ) ) != NULL; sectionNum++ )
		{
			COM_StripExtension( sectionName );
			while( *sectionName == '\n' || *sectionName == ' ' || *sectionName == '\r' )
				sectionName++;

			trap_Cmd_ExecuteText( EXEC_APPEND, va( "fs_checksum %s/%s/%s%s\n", gamedir, basePath, sectionName, GAMETYPE_SCRIPT_EXTENSION ) );
		}

		G_Free( data );
		trap_Cbuf_Execute();
	}
}

/*
* G_Gametype_Exists
*/
qboolean G_Gametype_Exists( const char *name )
{
	char *str;
	int count;

	if( !name )
		return qfalse;

	for( count = 0; ( str = G_ListNameForPosition( g_gametypes_list->string, count, CHAR_GAMETYPE_SEPARATOR ) ) != NULL; count++ )
	{
		if( !Q_stricmp( name, str ) )
			return qtrue;
	}

	return qfalse;
}

/*
* G_Gametype_GenerateGametypesList
*/
void G_Gametype_GenerateGametypesList( void )
{
	char *scriptsList;

	scriptsList = G_AllocCreateNamesList( "progs/gametypes", GAMETYPE_PROJECT_EXTENSION, CHAR_GAMETYPE_SEPARATOR );
	if( !scriptsList )
	{
		trap_Cvar_ForceSet( "g_gametypes_list", "dm;" );
		return;
	}

	trap_Cvar_ForceSet( "g_gametypes_list", scriptsList );
	G_Free( scriptsList );
}

/*
* G_Gametype_SetDefaults
*/
void G_Gametype_SetDefaults( void )
{
	level.gametype.spawnableItemsMask = ( IT_WEAPON | IT_AMMO | IT_ARMOR | IT_POWERUP | IT_HEALTH );
    level.gametype.respawnableItemsMask = level.gametype.spawnableItemsMask;
    level.gametype.dropableItemsMask = level.gametype.spawnableItemsMask;
    level.gametype.pickableItemsMask = level.gametype.spawnableItemsMask;

    level.gametype.isTeamBased = qfalse;
    level.gametype.isRace = qfalse;
    level.gametype.hasChallengersQueue = qfalse;
    level.gametype.maxPlayersPerTeam = 0;

    level.gametype.ammo_respawn = 20;
    level.gametype.armor_respawn = 25;
    level.gametype.weapon_respawn = 5;
    level.gametype.health_respawn = 15;
    level.gametype.powerup_respawn = 90;
    level.gametype.megahealth_respawn = 20;
    level.gametype.ultrahealth_respawn = 40;

    level.gametype.readyAnnouncementEnabled = qfalse;
    level.gametype.scoreAnnouncementEnabled = qfalse;
    level.gametype.countdownEnabled = qfalse;
    level.gametype.mathAbortDisabled = qfalse;
    level.gametype.shootingDisabled = qfalse;
    level.gametype.infiniteAmmo = qfalse;
    level.gametype.canForceModels = qtrue;
    level.gametype.canShowMinimap = qfalse;
    level.gametype.teamOnlyMinimap = qtrue;
	level.gametype.customDeadBodyCam = qfalse;

    level.gametype.spawnpoint_radius = 64;

    //racesow
    level.gametype.autoInactivityRemove = qtrue;
    level.gametype.playerInteraction = qfalse;
    level.gametype.freestyleMapFix = qfalse;
    level.gametype.enableDrowning = qtrue;
    //!racesow
}

/*
* G_Gametype_Init
*/
void G_Gametype_Init( void )
{
	qboolean changed = qfalse;

	g_gametypes_list = trap_Cvar_Get( "g_gametypes_list", "", CVAR_NOSET|CVAR_ARCHIVE );
	G_Gametype_GenerateGametypesList(); // fill the g_gametypes_list cvar

	// empty string to allow all
	g_votable_gametypes = trap_Cvar_Get( "g_votable_gametypes", "", CVAR_ARCHIVE );

	if( !g_gametype ) // first time initialized
		changed = qtrue;

	g_gametype = trap_Cvar_Get( "g_gametype", "dm", CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_LATCH );

	//get the match cvars too
	g_warmup_timelimit = trap_Cvar_Get( "g_warmup_timelimit", "5", CVAR_ARCHIVE );
	g_postmatch_timelimit = trap_Cvar_Get( "g_postmatch_timelimit", "4", CVAR_ARCHIVE );
	g_countdown_time = trap_Cvar_Get( "g_countdown_time", "5", CVAR_ARCHIVE );
	g_match_extendedtime = trap_Cvar_Get( "g_match_extendedtime", "2", CVAR_ARCHIVE );

	// game settings
	g_timelimit = trap_Cvar_Get( "g_timelimit", "10", CVAR_ARCHIVE );
	g_scorelimit = trap_Cvar_Get( "g_scorelimit", "0", CVAR_ARCHIVE );
	g_allow_falldamage = trap_Cvar_Get( "g_allow_falldamage", "1", CVAR_ARCHIVE );
	g_allow_selfdamage = trap_Cvar_Get( "g_allow_selfdamage", "1", CVAR_ARCHIVE );
	g_allow_teamdamage = trap_Cvar_Get( "g_allow_teamdamage", "1", CVAR_ARCHIVE );
	g_allow_bunny = trap_Cvar_Get( "g_allow_bunny", "1", CVAR_ARCHIVE
#ifndef ALLOWBYNNY_VOTE
		|CVAR_READONLY
#endif
	);

	// update latched gametype change
	if( g_gametype->latched_string )
	{
		if( G_Gametype_Exists( g_gametype->latched_string ) )
		{
			trap_Cvar_ForceSet( "g_gametype", va( "%s", g_gametype->latched_string ) );
			changed = qtrue;
		}
		else
		{
			G_Printf( "G_Gametype: Invalid new gametype, change ignored\n" );
			trap_Cvar_ForceSet( "g_gametype", va( "%s", g_gametype->string ) );
		}
	}
	
	if( !G_Gametype_Exists( g_gametype->string ) )
	{
		G_Printf( "G_Gametype: Wrong value: '%s'. Setting up with default (dm)\n", g_gametype->string );
		trap_Cvar_ForceSet( "g_gametype", "dm" );
		changed = qtrue;
	}

	G_Printf( "-------------------------------------\n" );
	G_Printf( "Initalizing '%s' gametype\n", g_gametype->string );

	if( changed )
	{
		const char *configs_path = "configs/server/gametypes/";

		G_InitChallengersQueue();

		// print a hint for admins so they know there's a chance to execute a 
		// config here, but don't show it as an error, because it isn't
		G_Printf( "loading %s%s.cfg\n", configs_path, g_gametype->string );
		trap_Cmd_ExecuteText( EXEC_NOW, va( "exec %s%s.cfg silent\n", configs_path, g_gametype->string ) );
		trap_Cbuf_Execute();

		// on a listen server, override gametype-specific settings in config
		trap_Cmd_ExecuteText( EXEC_NOW, "vstr ui_startservercmd\n" );
		trap_Cbuf_Execute();
	}

	GS_SetGametypeName( g_gametype->string );

	// fixme: we are doing this twice because the gametype may check for GS_Instagib
	G_CheckCvars(); // update GS_Instagib, GS_FallDamage, etc

	G_Gametype_SetDefaults();

	// Init the current gametype
	if( !G_asLoadGametypeScript( g_gametype->string ) )
		G_Gametype_GENERIC_Init();

	trap_ConfigString( CS_GAMETYPENAME, gs.gametypeName );

	G_CheckCvars(); // update GS_Instagib, GS_FallDamage, etc
}
