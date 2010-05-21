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

#include "tvm_local.h"
#include "tvm_misc.h"
#include "tvm_chase.h"
#include "tvm_clip.h"
#include "tvm_client.h"

static void TVM_SpectatorMode( edict_t *ent );

//====================
//TVM_CanChase
//====================
static qboolean TVM_Chase_IsValidTarget( edict_t *ent, edict_t *target )
{
	assert( ent && ent->local && ent->r.client );

	if( !target->r.inuse || target->local || !target->r.client )
		return qfalse;
	
	if( PLAYERNUM( target ) == ent->relay->playernum )
		return qfalse;

	if( target->r.client->ps.pmove.pm_type == PM_SPECTATOR )
		return qfalse;

	return qtrue;
}

static int TVM_Chase_FindFollowPOV( edict_t *ent )
{
	int i, j;
	int quad, warshell, scorelead;
	int maxteam;
	int flags[8];
	int newctfpov, newpoweruppov;
	int score_max;
	int newpov = -1;
	tvm_relay_t *relay = ent->relay;
	edict_t *target;
	static int ctfpov = -1, poweruppov = -1;
	static unsigned int flagswitchTime = 0;
	static unsigned int pwupswitchTime = 0;
#define CARRIERSWITCHDELAY 8000

	if( !ent->r.client || !ent->r.client->chase.active || !ent->r.client->chase.followmode )
		return newpov;

	// find what players have what
	score_max = -999999999;
	quad = warshell = scorelead = -1;
	memset( flags, -1, sizeof( flags ) );
	newctfpov = newpoweruppov = -1;
	maxteam = 0;

	for( i = 1; PLAYERNUM( (relay->edicts + i) ) < relay->maxclients; i++ )
	{
		target = relay->edicts + i;
		if( !TVM_Chase_IsValidTarget( ent, target ) )
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
		if( ent->r.client->chase.teamonly && ent->s.team != target->s.team )
			continue;

		if( target->s.effects & relay->effects.quad )
			quad = ENTNUM( target );
		if( target->s.effects & relay->effects.shell )
			warshell = ENTNUM( target );

		if( target->s.team && (target->s.effects & relay->effects.enemy_flag) )
		{
			if( target->s.team > maxteam )
				maxteam = target->s.team;
			flags[target->s.team-1] = ENTNUM( target );
		}

		// find the scoring leader
		if( target->r.client->ps.stats[relay->stats.frags] > score_max )
		{
			score_max = target->r.client->ps.stats[relay->stats.frags];
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
		if( ctfpov < 0 || relay->serverTime > flagswitchTime )
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
			flagswitchTime = relay->serverTime + CARRIERSWITCHDELAY;
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
		if( poweruppov < 0 || relay->serverTime > pwupswitchTime )
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
			pwupswitchTime = relay->serverTime + CARRIERSWITCHDELAY;
		}
	}
	else
	{
		if( quad != -1 )
			newpoweruppov = quad;
		else if( warshell != -1 )
			newpoweruppov = warshell;

		poweruppov = newpoweruppov;
		pwupswitchTime = 0;
	}

	// so, we got all, select what we prefer to show
	if( ctfpov != -1 && ( ent->r.client->chase.followmode & 4 ) )
		newpov = ctfpov;
	else if( poweruppov != -1 && ( ent->r.client->chase.followmode & 2 ) )
		newpov = poweruppov;
	else if( scorelead != -1 && ( ent->r.client->chase.followmode & 1 ) )
		newpov = scorelead;

	return newpov;
#undef CARRIERSWITCHDELAY
}

void TVM_ChasePlayer( edict_t *ent, char *name, int followmode )
{
	int i;
	edict_t *e;
	gclient_t *client;
	int targetNum = -1;
	int oldTarget;
	qboolean can_follow = qtrue;
	char colorlessname[MAX_NAME_BYTES];

	client = ent->r.client;

	oldTarget = client->chase.target;
	if( oldTarget < 0 )
		oldTarget = 0;

	if( !can_follow && followmode )
	{
		TVM_PrintMsg( ent->relay, ent, "Chasecam follow mode unavailable\n" );
		followmode = qfalse;
	}

	if( ent->r.client->chase.followmode && !followmode )
		TVM_PrintMsg( ent->relay, ent, "Disabling chasecam follow mode\n" );

	// always disable chasing as a start
	memset( &client->chase, 0, sizeof( chasecam_t ) );

	// locate the requested target
	if( name && name[0] )
	{
		// find it by player names
		for( e = ent->relay->edicts + 1; PLAYERNUM( e ) < ent->relay->maxclients; e++ )
		{
			if( !TVM_Chase_IsValidTarget( ent, e ) )
				continue;

			Q_strncpyz( colorlessname, COM_RemoveColorTokens( ent->r.client->pers.netname ), sizeof(colorlessname) );
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
			if( i >= 0 && i < ent->relay->maxclients )
			{
				e = ent->relay->edicts + 1 + i;
				if( TVM_Chase_IsValidTarget( ent, e ) )
					targetNum = PLAYERNUM( e );
			}
		}

		if( targetNum == -1 )
			TVM_PrintMsg( ent->relay, ent, "Requested chasecam target is not available\n" );
	}

	// try to reuse old target if we didn't find a valid one
	if( targetNum == -1 && oldTarget > 0 && oldTarget <= ent->relay->maxclients )
	{
		e = ent->relay->edicts + oldTarget;
		if( TVM_Chase_IsValidTarget( ent, e ) )
			targetNum = PLAYERNUM( e );
	}

	// if we still don't have a target, just pick the first valid one
	if( targetNum == -1 )
	{
		for( e = ent->relay->edicts + 1; PLAYERNUM( e ) < ent->relay->maxclients; e++ )
		{
			if( !TVM_Chase_IsValidTarget( ent, e ) )
				continue;

			targetNum = PLAYERNUM( e );
			break;
		}
	}

	if( targetNum != -1 )
	{
		// we found a target, set up the chasecam
		client->chase.target = targetNum + 1;
		client->chase.active = qtrue;
		client->chase.followmode = followmode;
	}
	else
	{
		// stay as observer
		TVM_SpectatorMode( ent );
		TVM_CenterPrintMsg( ent->relay, ent, "No one to chase" );
	}
}

//====================
//TVM_ChaseChange
// Can be called when no chase target set, will then find from beginning or end (depending on the step)
//====================
static void TVM_ChaseChange( edict_t *ent, int step )
{
	int i;
	edict_t	*target;
	int start;

	assert( ent && ent->local && ent->r.client );
	assert( step != 0 );

	if( !ent->relay->maxclients )
		return;

	if( ent->r.client->chase.target )
	{
		start = ent->r.client->chase.target;
	}
	else
	{
		if( step > 0 )
			start = ent->relay->maxclients; // so we find first
		else
			start = 1; // so we find last
	}

	i = start;

	do
	{
		i += step;
		if( i < 1 )
			i = ent->relay->maxclients;
		else if( i > ent->relay->maxclients )
			i = 1;

		target = ent->relay->edicts + i;
		if( TVM_Chase_IsValidTarget( ent, target ) )
		{
			TVM_ChasePlayer( ent, va( "%i", PLAYERNUM(target) ), ent->r.client->chase.followmode );
			return;
		}
	}
	while( i != start );

	TVM_SpectatorMode( ent );
}

//====================
//TVM_Cmd_ChaseNext
//====================
void TVM_Cmd_ChaseNext( edict_t *ent )
{
	assert( ent && ent->local && ent->r.client );

	if( !ent->r.client->chase.active )
		return;

	TVM_ChaseChange( ent, 1 );
}

//====================
//TVM_Cmd_ChasePrev
//====================
void TVM_Cmd_ChasePrev( edict_t *ent )
{
	assert( ent && ent->local && ent->r.client );

	if( !ent->r.client->chase.active )
		return;

	TVM_ChaseChange( ent, -1 );
}

//====================
//TVM_SpectatorMode
//====================
static void TVM_SpectatorMode( edict_t *ent )
{
	assert( ent && ent->local && ent->r.client );

	// was in chasecam
	ent->r.client->chase.active = qfalse;
	ent->r.client->ps.pmove.pm_type = PM_SPECTATOR;
	ent->r.client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
	ent->r.client->ps.POVnum = ent->relay->playernum + 1;
}

//====================
//TVM_Cmd_ChaseCam
//====================
void TVM_Cmd_ChaseCam( edict_t *ent )
{
	assert( ent && ent->local && ent->r.client );

	// & 1 = scorelead
	// & 2 = powerups
	// & 4 = flags

	if( trap_Cmd_Argc() < 2 )
	{
		TVM_ChasePlayer( ent, NULL, 0 );
	}
	else if( !Q_stricmp( trap_Cmd_Argv( 1 ), "auto" ) )
	{
		TVM_PrintMsg( ent->relay, ent, "Chasecam mode is 'auto'. It will follow the score leader when no powerup nor flag is carried.\n" );
		TVM_ChasePlayer( ent, NULL, 7 );
	}
	else if( !Q_stricmp( trap_Cmd_Argv( 1 ), "carriers" ) )
	{
		TVM_PrintMsg( ent->relay, ent, "Chasecam mode is 'carriers'. It will switch to flag or powerup carriers when any of these items is picked up.\n" );
		TVM_ChasePlayer( ent, NULL, 6 );
	}
	else if( !Q_stricmp( trap_Cmd_Argv( 1 ), "powerups" ) )
	{
		TVM_PrintMsg( ent->relay, ent, "Chasecam mode is 'powerups'. It will switch to powerup carriers when any of these items is picked up.\n" );
		TVM_ChasePlayer( ent, NULL, 2 );
	}
	else if( !Q_stricmp( trap_Cmd_Argv( 1 ), "objectives" ) )
	{
		TVM_PrintMsg( ent->relay, ent, "Chasecam mode is 'objectives'. It will switch to flag carriers when any of these items is picked up.\n" );
		TVM_ChasePlayer( ent, NULL, 4 );
	}
	else if( !Q_stricmp( trap_Cmd_Argv( 1 ), "score" ) )
	{
		TVM_PrintMsg( ent->relay, ent, "Chasecam mode is 'score'. It will always follow the highest fragger.\n" );
		TVM_ChasePlayer( ent, NULL, 1 );
	}
	else if( !Q_stricmp( trap_Cmd_Argv( 1 ), "help" ) )
	{
		TVM_PrintMsg( ent->relay, ent, "Chasecam modes:\n" );
		TVM_PrintMsg( ent->relay, ent, "- 'auto': Chase the score leader unless there's an objective carrier or a powerup carrier.\n" );
		TVM_PrintMsg( ent->relay, ent, "- 'carriers': User has pov control unless there's an objective carrier or a powerup carrier.\n" );
		TVM_PrintMsg( ent->relay, ent, "- 'objectives': User has pov control unless there's a objective carrier.\n" );
		TVM_PrintMsg( ent->relay, ent, "- 'powerups': User has pov control unless there's a powerup carrier.\n" );
		TVM_PrintMsg( ent->relay, ent, "- 'score': Always follow the score leader. User has no pov control.\n" );
		TVM_PrintMsg( ent->relay, ent, "- 'none': Disable chasecam.\n" );
	}
	else
	{
		TVM_ChasePlayer( ent, trap_Cmd_Argv( 1 ), 0 );
	}
}

//====================
//TVM_Cmd_SwitchChaseCamMode - Used by cgame for switching mode when clicking the mouse button
//====================
void TVM_Cmd_SwitchChaseCamMode( edict_t *ent )
{
	assert( ent && ent->local && ent->r.client );

	if( ent->r.client->chase.active )
	{
		// avoid freefly during demo playback at all costs
		if( !(ent->relay->playernum < 0) )
			TVM_SpectatorMode( ent );
	}
	else
	{
		TVM_ChasePlayer( ent, NULL, ent->r.client->chase.followmode );
	}
}

//=================
//TVM_ChaseClientEndSnapFrame
//=================
void TVM_ChaseClientEndSnapFrame( edict_t *ent )
{
	edict_t *target;
	int i, followpov;

	assert( ent && ent->local && ent->r.client );
	assert( ent->r.client->chase.active );

	if( ( followpov = TVM_Chase_FindFollowPOV( ent ) ) != -1 )
		ent->r.client->chase.target = followpov;

	target = NULL;
	for( i = 0; i < 2 && !target; i++ )
	{
		if( i )
			TVM_ChasePlayer( ent, NULL, 0 );
		if( ent->r.client->chase.target > 0 && ent->r.client->chase.target <= ent->relay->maxclients )
			target = ent->relay->edicts + ent->r.client->chase.target;
		if( target && !TVM_Chase_IsValidTarget( ent, target ) )
			target = NULL;
	}

	if( !target )
		return;

	// copy target playerState to me
	ent->r.client->ps = target->r.client->ps;

	// chasecam uses PM_CHASECAM
	ent->r.client->ps.pmove.pm_type = PM_CHASECAM;
	ent->r.client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;
	ent->r.client->ps.POVnum = ENTNUM( target );

	VectorCopy( ent->r.client->ps.pmove.origin, ent->s.origin );
	VectorCopy( ent->r.client->ps.viewangles, ent->s.angles );

	GClip_LinkEntity( ent->relay, ent );
}
