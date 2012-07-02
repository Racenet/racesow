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

/*
* G_Chase_SetChaseActive
*/
static void G_Chase_SetChaseActive( edict_t *ent, qboolean active )
{
	ent->r.client->resp.chase.active = active;
	G_UpdatePlayerMatchMsg( ent );
}

/*
* G_Chase_IsValidTarget
*/
static qboolean G_Chase_IsValidTarget( edict_t *ent, edict_t *target, qboolean teamonly )
{
	if( !ent || !target )
		return qfalse;

	if( !target->r.inuse || !target->r.client || trap_GetClientState( PLAYERNUM( target ) ) < CS_SPAWNED )
		return qfalse;

	if( target->s.team < TEAM_PLAYERS || target->s.team >= GS_MAX_TEAMS || target == ent )
		return qfalse;

	if( teamonly && !ent->r.client->teamstate.is_coach && G_ISGHOSTING( target ) )
		return qfalse;

	if( teamonly && target->s.team != ent->s.team )
		return qfalse;

	return qtrue;
}

/*
* G_Chase_FindFollowPOV
*/
static int G_Chase_FindFollowPOV( edict_t *ent )
{
	int i, j;
	int quad, warshell, regen, scorelead;
	int maxteam;
	int flags[GS_MAX_TEAMS];
	int newctfpov, newpoweruppov;
	int score_max;
	int newpov = -1;
	edict_t *target;
	static int ctfpov = -1, poweruppov = -1;
	static unsigned int flagswitchTime = 0;
	static unsigned int pwupswitchTime = 0;
#define CARRIERSWITCHDELAY 8000

	if( !ent->r.client || !ent->r.client->resp.chase.active || !ent->r.client->resp.chase.followmode )
		return newpov;

	// follow killer, with a small delay
	if( ( ent->r.client->resp.chase.followmode & 8 ) ) {
		edict_t *targ;

		targ = &game.edicts[ent->r.client->resp.chase.target];
		if( G_IsDead( targ ) && targ->deathTimeStamp + 400 < level.time ) {
			edict_t *attacker = targ->r.client->teamstate.last_killer;

			// ignore world and team kills
			if( attacker && attacker->r.client && !GS_IsTeamDamage( &targ->s, &attacker->s ) ) {
				newpov = ENTNUM( attacker );
			}
		}

		return newpov;
	}

	// find what players have what
	score_max = -999999999;
	quad = warshell = regen = scorelead = -1;
	memset( flags, -1, sizeof( flags ) );
	newctfpov = newpoweruppov = -1;
	maxteam = 0;

	for( i = 1; PLAYERNUM( (game.edicts+i) ) < gs.maxclients; i++ )
	{
		target = game.edicts + i;

		if( !target->r.inuse || trap_GetClientState( PLAYERNUM( target ) ) < CS_SPAWNED )
		{
			// check if old targets are still valid
			if( ctfpov == ENTNUM( target ) )
				ctfpov = -1;
			if( poweruppov == ENTNUM( target ) )
				poweruppov = -1;
			continue;
		}
		if( target->s.team <= 0 || target->s.team >= sizeof( flags ) / sizeof( flags[0] ) )
			continue;
		if( ent->r.client->resp.chase.teamonly && ent->s.team != target->s.team )
			continue;

		if( target->s.effects & EF_QUAD )
			quad = ENTNUM( target );
		if( target->s.effects & EF_SHELL )
			warshell = ENTNUM( target );
		if( target->s.effects & EF_REGEN )
			regen = ENTNUM( target );

		if( target->s.team && (target->s.effects & EF_CARRIER) )
		{
			if( target->s.team > maxteam )
				maxteam = target->s.team;
			flags[target->s.team-1] = ENTNUM( target );
		}

		// find the scoring leader
		if( target->r.client->ps.stats[STAT_SCORE] > score_max )
		{
			score_max = target->r.client->ps.stats[STAT_SCORE];
			scorelead = ENTNUM( target );
		}
	}

	// do some categorization

	for( i = 0; i < maxteam; i++ )
	{
		if( flags[i] == -1 )
			continue;

		// default new ctfpov to the first flag carrier
		if( newctfpov == -1 )
			newctfpov = flags[i];
		else
			break;
	}

	// do we have more than one flag carrier?
	if( i < maxteam )
	{
		// default to old ctfpov
		if( ctfpov >= 0 )
			newctfpov = ctfpov;
		if( ctfpov < 0 || level.time > flagswitchTime )
		{
			// alternate between flag carriers
			for( i = 0; i < maxteam; i++ )
			{
				if( flags[i] != ctfpov )
					continue;

				for( j = 0; j < maxteam-1; j++ )
				{
					if( flags[(i+j+1)%maxteam] != -1 )
					{
						newctfpov = flags[(i+j+1)%maxteam];
						break;
					}
				}
				break;
			}
		}

		if( newctfpov != ctfpov )
		{
			ctfpov = newctfpov;
			flagswitchTime = level.time + CARRIERSWITCHDELAY;
		}
	}
	else
	{
		ctfpov = newctfpov;
		flagswitchTime = 0;
	}

	if( quad != -1 && warshell != -1 && quad != warshell )
	{
		// default to old powerup
		if( poweruppov >= 0 )
			newpoweruppov = poweruppov;
		if( poweruppov < 0 || level.time > pwupswitchTime )
		{
			if( poweruppov == quad )
				newpoweruppov = warshell;
			else if( poweruppov == warshell )
				newpoweruppov = quad;
			else 
				newpoweruppov = ( rand() & 1 ) ? quad : warshell;
		}

		if( poweruppov != newpoweruppov )
		{
			poweruppov = newpoweruppov;
			pwupswitchTime = level.time + CARRIERSWITCHDELAY;
		}
	}
	else
	{
		if( quad != -1 )
			newpoweruppov = quad;
		else if( warshell != -1 )
			newpoweruppov = warshell;
		else if( regen != -1 )
			newpoweruppov = regen;

		poweruppov = newpoweruppov;
		pwupswitchTime = 0;
	}

	// so, we got all, select what we prefer to show
	if( ctfpov != -1 && ( ent->r.client->resp.chase.followmode & 4 ) )
		newpov = ctfpov;
	else if( poweruppov != -1 && ( ent->r.client->resp.chase.followmode & 2 ) )
		newpov = poweruppov;
	else if( scorelead != -1 && ( ent->r.client->resp.chase.followmode & 1 ) )
		newpov = scorelead;

	return newpov;
#undef CARRIERSWITCHDELAY
}

/*
* G_EndFrame_UpdateChaseCam
*/
static void G_EndFrame_UpdateChaseCam( edict_t *ent )
{
	edict_t *targ;
	int followpov;

	// not in chasecam
	if( !ent->r.client->resp.chase.active )
		return;

	if( ( followpov = G_Chase_FindFollowPOV( ent ) ) != -1 )
		ent->r.client->resp.chase.target = followpov;

	// is our chase target gone?
	targ = &game.edicts[ent->r.client->resp.chase.target];

	if( !G_Chase_IsValidTarget( ent, targ, ent->r.client->resp.chase.teamonly ) )
	{
		if( game.realtime < ent->r.client->resp.chase.timeout ) // wait for timeout
			return;

		ent->r.client->resp.chase.timeout = game.realtime + 1500; // update timeout

		G_ChasePlayer( ent, NULL, ent->r.client->resp.chase.teamonly, ent->r.client->resp.chase.followmode );
		targ = &game.edicts[ent->r.client->resp.chase.target];
		if( !G_Chase_IsValidTarget( ent, targ, ent->r.client->resp.chase.teamonly ) )
			return;
	}

	ent->r.client->resp.chase.timeout = game.realtime + 1500; // update timeout

	if( targ == ent )
		return;

	// free our psev buffer when in chasecam
	G_ClearPlayerStateEvents( ent->r.client );

	// copy target playerState to me
	ent->r.client->ps = targ->r.client->ps;

	// fix some stats we don't want copied from the target
	ent->r.client->ps.stats[STAT_REALTEAM] = ent->s.team;
	ent->r.client->ps.stats[STAT_LAYOUTS] &= ~STAT_LAYOUT_SCOREBOARD;
	ent->r.client->ps.stats[STAT_LAYOUTS] &= ~STAT_LAYOUT_CHALLENGER;
	ent->r.client->ps.stats[STAT_LAYOUTS] &= ~STAT_LAYOUT_READY;
	ent->r.client->ps.stats[STAT_LAYOUTS] &= ~STAT_LAYOUT_SPECTEAMONLY;

	if( ent->r.client->resp.chase.teamonly )
	{
		ent->r.client->ps.stats[STAT_LAYOUTS] |= STAT_LAYOUT_SPECTEAMONLY;
		if( !ent->r.client->teamstate.is_coach )
			ent->r.client->ps.stats[STAT_LAYOUTS] |= STAT_LAYOUT_SPECDEAD; // show deadcam effect
	}

	if( ent->r.client->level.showscores || GS_MatchState() >= MATCH_STATE_POSTMATCH )
		ent->r.client->ps.stats[STAT_LAYOUTS] |= STAT_LAYOUT_SCOREBOARD; // show the scoreboard

	if( GS_HasChallengers() && ent->r.client->queueTimeStamp )
		ent->r.client->ps.stats[STAT_LAYOUTS] |= STAT_LAYOUT_CHALLENGER;

	if( GS_MatchState() <= MATCH_STATE_WARMUP && level.ready[PLAYERNUM( ent )] )
		ent->r.client->ps.stats[STAT_LAYOUTS] |= STAT_LAYOUT_READY;


	// cgame will override the fov if not zooming
	ent->r.client->ps.fov = targ->r.client->zoomfov;

	// chasecam uses PM_CHASECAM
	ent->r.client->ps.pmove.pm_type = PM_CHASECAM;
	ent->r.client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;

	VectorCopy( targ->s.origin, ent->s.origin );
	VectorCopy( targ->s.angles, ent->s.angles );
	GClip_LinkEntity( ent );
}

/*
* G_EndServerFrames_UpdateChaseCam
*/
void G_EndServerFrames_UpdateChaseCam( void )
{
	int i, team;
	edict_t	*ent;

	// do it by teams, so spectators can copy the chasecam information from players
	for( team = TEAM_PLAYERS; team < GS_MAX_TEAMS; team++ )
	{
		for( i = 0; teamlist[team].playerIndices[i] != -1; i++ )
		{
			ent = game.edicts + teamlist[team].playerIndices[i];
			if( trap_GetClientState( PLAYERNUM(ent) ) < CS_SPAWNED )
			{
				G_Chase_SetChaseActive( ent, qfalse );
				continue;
			}

			G_EndFrame_UpdateChaseCam( ent );
		}
	}

	// Do spectators last
	team = TEAM_SPECTATOR;
	for( i = 0; teamlist[team].playerIndices[i] != -1; i++ )
	{
		ent = game.edicts + teamlist[team].playerIndices[i];
		if( trap_GetClientState( PLAYERNUM(ent) ) < CS_SPAWNED )
		{
			G_Chase_SetChaseActive( ent, qfalse );
			continue;
		}

		G_EndFrame_UpdateChaseCam( ent );
	}
}

/*
* G_ChasePlayer
*/
void G_ChasePlayer( edict_t *ent, const char *name, qboolean teamonly, int followmode )
{
	int i;
	edict_t *e;
	gclient_t *client;
	int targetNum = -1;
	int oldTarget;
	qboolean can_follow = qtrue;
	char colorlessname[MAX_NAME_BYTES];

	client = ent->r.client;

	oldTarget = client->resp.chase.target;

	if( teamonly && !client->teamstate.is_coach )
		can_follow = qfalse;

	if( !can_follow && followmode )
	{
		G_PrintMsg( ent, "Chasecam follow mode unavailable\n" );
		followmode = qfalse;
	}

	if( ent->r.client->resp.chase.followmode && !followmode )
		G_PrintMsg( ent, "Disabling chasecam follow mode\n" );

	// always disable chasing as a start
	memset( &client->resp.chase, 0, sizeof( chasecam_t ) );

	// locate the requested target
	if( name && name[0] )
	{
		// find it by player names
		for( e = game.edicts + 1; PLAYERNUM( e ) < gs.maxclients; e++ )
		{
			if( !G_Chase_IsValidTarget( ent, e, teamonly ) )
				continue;

			Q_strncpyz( colorlessname, COM_RemoveColorTokens( e->r.client->netname ), sizeof(colorlessname) );

			if( !Q_stricmp( COM_RemoveColorTokens( name ), colorlessname ) )
			{
				targetNum = PLAYERNUM( e );
				break;
			}
		}

		// didn't find it by name, try by numbers
		if( targetNum == -1 )
		{
			i = atoi( name );
			if( i >= 0 && i < gs.maxclients )
			{
				e = game.edicts + 1 + i;
				if( G_Chase_IsValidTarget( ent, e, teamonly ) )
					targetNum = PLAYERNUM( e );
			}
		}

		if( targetNum == -1 )
			G_PrintMsg( ent, "Requested chasecam target is not available\n" );
	}

	// try to reuse old target if we didn't find a valid one
	if( targetNum == -1 && oldTarget > 0 && oldTarget < gs.maxclients )
	{
		e = game.edicts + 1 + oldTarget;
		if( G_Chase_IsValidTarget( ent, e, teamonly ) )
			targetNum = PLAYERNUM( e );
	}

	// if we still don't have a target, just pick the first valid one
	if( targetNum == -1 )
	{
		for( e = game.edicts + 1; PLAYERNUM( e ) < gs.maxclients; e++ )
		{
			if( !G_Chase_IsValidTarget( ent, e, teamonly ) )
				continue;

			targetNum = PLAYERNUM( e );
			break;
		}
	}

	// make the client a ghost
	G_GhostClient( ent );
	if( targetNum != -1 )
	{
		// we found a target, set up the chasecam
		client->resp.chase.target = targetNum + 1;
		client->resp.chase.teamonly = teamonly;
		client->resp.chase.followmode = followmode;
		G_Chase_SetChaseActive( ent, qtrue );
	}
	else
	{
		// stay as observer
		if( !teamonly )
			ent->movetype = MOVETYPE_NOCLIP;
		client->level.showscores = qfalse;
		G_Chase_SetChaseActive( ent, qfalse );
		G_CenterPrintMsg( ent, "No one to chase" );
	}
}

/*
* ChaseStep
*/
void G_ChaseStep( edict_t *ent, int step )
{
	int i, j;
	int start;
	edict_t *newtarget = NULL;

	if( !ent->r.client->resp.chase.active )
		return;

	i = start = ent->r.client->resp.chase.target;

	if( step == 0 )
	{
		if( G_Chase_IsValidTarget( ent, game.edicts + i, ent->r.client->resp.chase.teamonly ) )
			newtarget = game.edicts + i;
		else
			step = 1;
	}

	if( !newtarget )
	{
		for( j = 0; j < gs.maxclients; j++ )
		{
			i += step;
			if( i < 1 )
				i = gs.maxclients;
			else if( i > gs.maxclients )
				i = 1;
			if( i == start )
				break;
			if( G_Chase_IsValidTarget( ent, game.edicts + i, ent->r.client->resp.chase.teamonly ) )
			{	
				newtarget = game.edicts + i;
				break;
			}
		}
	}

	if( newtarget )
	{
		G_ChasePlayer( ent, va( "%i", PLAYERNUM( newtarget ) ), ent->r.client->resp.chase.teamonly, ent->r.client->resp.chase.followmode );
	}
}

/*
* Cmd_ChaseCam_f
*/
void Cmd_ChaseCam_f( edict_t *ent )
{
	qboolean team_only;
	const char *arg1;

	if( ent->s.team != TEAM_SPECTATOR && !ent->r.client->teamstate.is_coach )
	{
		G_Teams_JoinTeam( ent, TEAM_SPECTATOR );
		G_PrintMsg( NULL, "%s%s joined the %s%s team.\n", ent->r.client->netname,
			S_COLOR_WHITE, GS_TeamName( ent->s.team ), S_COLOR_WHITE );
	}

	// & 1 = scorelead
	// & 2 = powerups
	// & 4 = objectives
	// & 8 = fragger

	if( ent->r.client->teamstate.is_coach && GS_TeamBasedGametype() )
		team_only = qtrue;
	else 
		team_only = qfalse;

	arg1 = trap_Cmd_Argv( 1 );

	if( trap_Cmd_Argc() < 2 )
	{
		G_ChasePlayer( ent, NULL, team_only, 0 );
	}
	else if( !Q_stricmp( arg1, "auto" ) )
	{
		G_PrintMsg( ent, "Chasecam mode is 'auto'. It will follow the score leader when no powerup nor flag is carried.\n" );
		G_ChasePlayer( ent, NULL, team_only, 7 );
	}
	else if( !Q_stricmp( arg1, "carriers" ) )
	{
		G_PrintMsg( ent, "Chasecam mode is 'carriers'. It will switch to flag or powerup carriers when any of these items is picked up.\n" );
		G_ChasePlayer( ent, NULL, team_only, 6 );
	}
	else if( !Q_stricmp( arg1, "powerups" ) )
	{
		G_PrintMsg( ent, "Chasecam mode is 'powerups'. It will switch to powerup carriers when any of these items is picked up.\n" );
		G_ChasePlayer( ent, NULL, team_only, 2 );
	}
	else if( !Q_stricmp( arg1, "objectives" ) )
	{
		G_PrintMsg( ent, "Chasecam mode is 'objectives'. It will switch to objectives carriers when any of these items is picked up.\n" );
		G_ChasePlayer( ent, NULL, team_only, 4 );
	}
	else if( !Q_stricmp( arg1, "score" ) )
	{
		G_PrintMsg( ent, "Chasecam mode is 'score'. It will always follow the highest fragger.\n" );
		G_ChasePlayer( ent, NULL, team_only, 1 );
	}
	else if( !Q_stricmp( arg1, "fragger" ) )
	{
		G_PrintMsg( ent, "Chasecam mode is 'fragger'. The last fragging player will be followed.\n" );
		G_ChasePlayer( ent, NULL, team_only, 8 );
	}
	else if( !Q_stricmp( arg1, "help" ) )
	{
		G_PrintMsg( ent, "Chasecam modes:\n" );
		G_PrintMsg( ent, "- 'auto': Chase the score leader unless there's an objective carrier or a powerup carrier.\n" );
		G_PrintMsg( ent, "- 'carriers': User has pov control unless there's an objective carrier or a powerup carrier.\n" );
		G_PrintMsg( ent, "- 'objectives': User has pov control unless there's an objective carrier.\n" );
		G_PrintMsg( ent, "- 'powerups': User has pov control unless there's a flag carrier.\n" );
		G_PrintMsg( ent, "- 'score': Always follow the score leader. User has no pov control.\n" );
		G_PrintMsg( ent, "- 'none': Disable chasecam.\n" );
		return;
	}
	else
	{
		G_ChasePlayer( ent, arg1, team_only, 0 );
	}

	G_Teams_LeaveChallengersQueue( ent );
}

/*
* G_SpectatorMode
*/
void G_SpectatorMode( edict_t *ent )
{
	// join spectator team
	if( ent->s.team != TEAM_SPECTATOR )
	{
		G_Teams_JoinTeam( ent, TEAM_SPECTATOR );
		G_PrintMsg( NULL, "%s%s joined the %s%s team.\n", ent->r.client->netname,
			S_COLOR_WHITE, GS_TeamName( ent->s.team ), S_COLOR_WHITE );
	}

	// was in chasecam
	if( ent->r.client->resp.chase.active )
	{
		ent->r.client->level.showscores = qfalse;
		G_Chase_SetChaseActive( ent, qfalse );

		// reset movement speeds
		ent->r.client->ps.pmove.stats[PM_STAT_MAXSPEED] = DEFAULT_PLAYERSPEED;
		ent->r.client->ps.pmove.stats[PM_STAT_JUMPSPEED] = DEFAULT_JUMPSPEED;
		ent->r.client->ps.pmove.stats[PM_STAT_DASHSPEED] = DEFAULT_DASHSPEED;
	}

	ent->movetype = MOVETYPE_NOCLIP;
}

/*
* Cmd_Spec_f
*/
void Cmd_Spec_f( edict_t *ent )
{
	if( ent->s.team == TEAM_SPECTATOR && !ent->r.client->queueTimeStamp )
	{
		G_PrintMsg( ent, "You are already a spectator.\n" );
		return;
	}

	G_SpectatorMode( ent );
	G_Teams_LeaveChallengersQueue( ent );
}

/*
* Cmd_SwitchChaseCamMode_f
*/
void Cmd_SwitchChaseCamMode_f( edict_t *ent )
{
	if( ent->s.team == TEAM_SPECTATOR )
	{
		if( ent->r.client->resp.chase.active )
			G_SpectatorMode( ent );
		else
		{
			G_Chase_SetChaseActive( ent, qtrue );
			G_ChaseStep( ent, 0 );
		}
	}
}
