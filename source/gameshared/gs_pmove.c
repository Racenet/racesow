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

#include "q_arch.h"
#include "q_math.h"
#include "q_shared.h"
#include "q_comref.h"
#include "q_collision.h"
#include "gs_public.h"

//===============================================================
//		WARSOW player AAboxes sizes

vec3_t playerbox_stand_mins = { -16, -16, -24 };
vec3_t playerbox_stand_maxs = { 16, 16, 40 };
int playerbox_stand_viewheight = 30;

vec3_t playerbox_crouch_mins = { -16, -16, -24 };
vec3_t playerbox_crouch_maxs = { 16, 16, 16 };
int playerbox_crouch_viewheight = 12;

vec3_t playerbox_gib_mins = { -16, -16, 0 };
vec3_t playerbox_gib_maxs = { 16, 16, 16 };
int playerbox_gib_viewheight = 8;

#define SPEEDKEY    500

#define PM_DASHJUMP_TIMEDELAY 1000 // delay in milliseconds
#define PM_WALLJUMP_TIMEDELAY	1300
#define PM_WALLJUMP_FAILED_TIMEDELAY	700
#define PM_SPECIAL_CROUCH_INHIBIT 400
#define PM_AIRCONTROL_BOUNCE_DELAY 200
#define PM_OVERBOUNCE		1.01f

//===============================================================

// all of the locals will be zeroed before each
// pmove, just to make damn sure we don't have
// any differences when running on client or server

typedef struct
{
	vec3_t origin;          // full float precision
	vec3_t velocity;        // full float precision

	vec3_t forward, right, up;
	vec3_t flatforward;     // normalized forward without z component, saved here because it needs
	// special handling for looking straight up or down
	float frametime;

	int groundsurfFlags;
	cplane_t groundplane;
	int groundcontents;

	vec3_t previous_origin;
	qboolean ladder;

	float forwardPush, sidePush, upPush;

	float maxPlayerSpeed;
	float maxWalkSpeed;
	float maxCrouchedSpeed;
	float jumpPlayerSpeed;
	float dashPlayerSpeed;
} pml_t;

pmove_t	*pm;
pml_t pml;

// movement parameters

#define DEFAULT_WALKSPEED 160.0f
#define DEFAULT_CROUCHEDSPEED 100.0f
#define DEFAULT_LADDERSPEED 250.0f

const float pm_friction = 8; //  ( initially 6 )
const float pm_waterfriction = 1;
const float pm_wateraccelerate = 10; // user intended acceleration when swimming ( initially 6 )

const float pm_accelerate = 12; // user intended acceleration when on ground or fly movement ( initially 10 )
const float pm_decelerate = 12; // user intended deceleration when on ground

const float pm_airaccelerate = 1; // user intended aceleration when on air
const float pm_airdecelerate = 2.0f; // air deceleration (not +strafe one, just at normal moving).

// special movement parameters

const float pm_aircontrol = 150.0f; // aircontrol multiplier (intertia velocity to forward velocity conversion)
const float pm_strafebunnyaccel = 70; // forward acceleration when strafe bunny hopping
const float pm_wishspeed = 30;

const float pm_dashupspeed = ( 174.0f * GRAVITY_COMPENSATE );

#ifdef OLDWALLJUMP
const float pm_wjupspeed = 370;
const float pm_wjbouncefactor = 0.5f;
#define pm_wjminspeed pm_maxspeed
#else
const float pm_wjupspeed = ( 330.0f * GRAVITY_COMPENSATE );
const float pm_failedwjupspeed = ( 50.0f * GRAVITY_COMPENSATE );
const float pm_wjbouncefactor = 0.3f;
const float pm_failedwjbouncefactor = 0.1f;
#define pm_wjminspeed ( ( pml.maxWalkSpeed + pml.maxPlayerSpeed ) * 0.5f )
#endif

//
// Kurim : some functions/defines that can be useful to work on the horizontal movement of player :
//
#define VectorScale2D( in, scale, out ) ( ( out )[0] = ( in )[0]*( scale ), ( out )[1] = ( in )[1]*( scale ) )
#define DotProduct2D( x, y )	       ( ( x )[0]*( y )[0]+( x )[1]*( y )[1] )

static vec_t VectorNormalize2D( vec3_t v ) // ByMiK : normalize horizontally (don't affect Z value)
{
	float length, ilength;
	length = v[0]*v[0] + v[1]*v[1];
	if( length )
	{
		length = sqrt( length ); // FIXME
		ilength = 1.0f/length;
		v[0] *= ilength;
		v[1] *= ilength;
	}
	return length;
}

// Could be used to test if player walk touching a wall, if not used in any other part of pm code i'll integrate
// this function to the walljumpcheck function.
// usage : nbTestDir = nb of direction to test around the player
// maxZnormal is the Z value of the normal of a poly to considere it as a wall
// normal is a pointer to the normal of the nearest wall

static void PlayerTouchWall( int nbTestDir, float maxZnormal, vec3_t *normal )
{
	vec3_t min, max, dir;
	int i, j;
	trace_t trace;
	float dist = 1.0;
	entity_state_t *state;

	for( i = 0; i < nbTestDir; i++ )
	{
		dir[0] = pml.origin[0] + ( pm->maxs[0]*cos( ( 360/nbTestDir )*i ) + pml.velocity[0] * 0.015f );
		dir[1] = pml.origin[1] + ( pm->maxs[1]*sin( ( 360/nbTestDir )*i ) + pml.velocity[1] * 0.015f );
		dir[2] = pml.origin[2];

		for( j = 0; j < 2; j++ )
		{
			min[j] = pm->mins[j];
			max[j] = pm->maxs[j];
		}
		min[2] = max[2] = 0;
		VectorScale( dir, 1.002, dir );

		module_Trace( &trace, pml.origin, min, max, dir, pm->playerState->POVnum, pm->contentmask, 0 );

		if( trace.allsolid ) return;

		if( trace.fraction == 1 )
			continue; // no wall in this direction

		if( trace.surfFlags & (SURF_SKY|SURF_NOWALLJUMP) )
			continue;

		if( trace.ent > 0 )
		{
			state = module_GetEntityState( trace.ent, 0 );
			if( state->type == ET_PLAYER )
				continue;
		}

		if( trace.fraction > 0 )
		{
			if( dist > trace.fraction && abs( trace.plane.normal[2] ) < maxZnormal )
			{
				dist = trace.fraction;
				VectorCopy( trace.plane.normal, *normal );
			}
		}
	}
}

//
//  walking up a step should kill some velocity
//

//==================
//PM_SlideMove
//
//Returns a new origin, velocity, and contact entity
//Does not modify any world state?
//==================

#define	MAX_CLIP_PLANES	5

static void PM_AddTouchEnt( int entNum )
{
	int i;

	if( pm->numtouch >= MAXTOUCH || entNum < 0 )
		return;

	// see if it is already added
	for( i = 0; i < pm->numtouch; i++ )
	{
		if( pm->touchents[i] == entNum )
			return;
	}

	// add it
	pm->touchents[pm->numtouch] = entNum;
	pm->numtouch++;
}


static int PM_SlideMove( void )
{
	vec3_t end, dir;
	vec3_t old_velocity, last_valid_origin;
	float value;
	vec3_t planes[MAX_CLIP_PLANES];
	int numplanes = 0;
	trace_t	trace;
	int moves, i, j, k;
	int maxmoves = 4;
	float remainingTime = pml.frametime;
	int blockedmask = 0;

	VectorCopy( pml.velocity, old_velocity );
	VectorCopy( pml.origin, last_valid_origin );

	if( pm->groundentity != -1 )
	{                          // clip velocity to ground, no need to wait
		// if the ground is not horizontal (a ramp) clipping will slow the player down
		if( pml.groundplane.normal[2] == 1.0f && pml.velocity[2] < 0.0f )
			pml.velocity[2] = 0.0f;
	}

	numplanes = 0; // clean up planes count for checking

	for( moves = 0; moves < maxmoves; moves++ )
	{
		VectorMA( pml.origin, remainingTime, pml.velocity, end );
		module_Trace( &trace, pml.origin, pm->mins, pm->maxs, end, pm->playerState->POVnum, pm->contentmask, 0 );
		if( trace.allsolid )
		{               // trapped into a solid
			VectorCopy( last_valid_origin, pml.origin );
			return SLIDEMOVEFLAG_TRAPPED;
		}

		if( trace.fraction > 0 )
		{                   // actually covered some distance
			VectorCopy( trace.endpos, pml.origin );
			VectorCopy( trace.endpos, last_valid_origin );
		}

		if( trace.fraction == 1 )
			break; // move done

		// save touched entity for return output
		PM_AddTouchEnt( trace.ent );

		// at this point we are blocked but not trapped.

		blockedmask |= SLIDEMOVEFLAG_BLOCKED;
		if( trace.plane.normal[2] < SLIDEMOVE_PLANEINTERACT_EPSILON )
		{                                                       // is it a vertical wall?
			blockedmask |= SLIDEMOVEFLAG_WALL_BLOCKED;
		}

		remainingTime -= ( trace.fraction * remainingTime );

		// we got blocked, add the plane for sliding along it

		// if this is a plane we have touched before, try clipping
		// the velocity along it's normal and repeat.
		for( i = 0; i < numplanes; i++ )
		{
			if( DotProduct( trace.plane.normal, planes[i] ) > ( 1.0f - SLIDEMOVE_PLANEINTERACT_EPSILON ) )
			{
				VectorAdd( trace.plane.normal, pml.velocity, pml.velocity );
				break;
			}
		}
		if( i < numplanes )  // found a repeated plane, so don't add it, just repeat the trace
			continue;

		// security check: we can't store more planes
		if( numplanes >= MAX_CLIP_PLANES )
		{
			VectorClear( pml.velocity );
			return SLIDEMOVEFLAG_TRAPPED;
		}

		// put the actual plane in the list
		VectorCopy( trace.plane.normal, planes[numplanes] );
		numplanes++;

		//
		// modify original_velocity so it parallels all of the clip planes
		//

		for( i = 0; i < numplanes; i++ )
		{
			if( DotProduct( pml.velocity, planes[i] ) >= SLIDEMOVE_PLANEINTERACT_EPSILON )  // would not touch it
				continue;

			GS_ClipVelocity( pml.velocity, planes[i], pml.velocity, PM_OVERBOUNCE );
			// see if we enter a second plane
			for( j = 0; j < numplanes; j++ )
			{
				if( j == i )  // it's the same plane
					continue;
				if( DotProduct( pml.velocity, planes[j] ) >= SLIDEMOVE_PLANEINTERACT_EPSILON )
					continue; // not with this one

				//there was a second one. Try to slide along it too
				GS_ClipVelocity( pml.velocity, planes[j], pml.velocity, PM_OVERBOUNCE );

				// check if the slide sent it back to the first plane
				if( DotProduct( pml.velocity, planes[i] ) >= SLIDEMOVE_PLANEINTERACT_EPSILON )
					continue;

				// bad luck: slide the original velocity along the crease
				CrossProduct( planes[i], planes[j], dir );
				VectorNormalize( dir );
				value = DotProduct( dir, pml.velocity );
				VectorScale( dir, value, pml.velocity );

				// check if there is a third plane, in that case we're trapped
				for( k = 0; k < numplanes; k++ )
				{
					if( j == k || i == k )  // it's the same plane
						continue;
					if( DotProduct( pml.velocity, planes[k] ) >= SLIDEMOVE_PLANEINTERACT_EPSILON )
						continue; // not with this one
					VectorClear( pml.velocity );
					break;
				}
			}
		}
	}

	if( pm->playerState->pmove.pm_time )
	{
		VectorCopy( old_velocity, pml.velocity );
	}

	return blockedmask;
}

//==================
//PM_StepSlideMove
//
//Each intersection will try to step over the obstruction instead of
//sliding along it.
//==================
static void PM_StepSlideMove( void )
{
	vec3_t start_o, start_v;
	vec3_t down_o, down_v;
	trace_t	trace;
	float down_dist, up_dist;
	vec3_t up, down;
	int blocked;

	VectorCopy( pml.origin, start_o );
	VectorCopy( pml.velocity, start_v );

	blocked = PM_SlideMove();

	VectorCopy( pml.origin, down_o );
	VectorCopy( pml.velocity, down_v );

	VectorCopy( start_o, up );
	up[2] += STEPSIZE;

	module_Trace( &trace, up, pm->mins, pm->maxs, up, pm->playerState->POVnum, pm->contentmask, 0 );
	if( trace.allsolid )
		return; // can't step up

	// try sliding above
	VectorCopy( up, pml.origin );
	VectorCopy( start_v, pml.velocity );

	PM_SlideMove();

	// push down the final amount
	VectorCopy( pml.origin, down );
	down[2] -= STEPSIZE;
	module_Trace( &trace, pml.origin, pm->mins, pm->maxs, down, pm->playerState->POVnum, pm->contentmask, 0 );
	if( !trace.allsolid )
	{
		VectorCopy( trace.endpos, pml.origin );
	}

	VectorCopy( pml.origin, up );

	// decide which one went farther
	down_dist = ( down_o[0] - start_o[0] )*( down_o[0] - start_o[0] )
		+ ( down_o[1] - start_o[1] )*( down_o[1] - start_o[1] );
	up_dist = ( up[0] - start_o[0] )*( up[0] - start_o[0] )
		+ ( up[1] - start_o[1] )*( up[1] - start_o[1] );

	if( down_dist >= up_dist || trace.allsolid || ( trace.fraction != 1.0 && !ISWALKABLEPLANE( &trace.plane ) ) )
	{
		VectorCopy( down_o, pml.origin );
		VectorCopy( down_v, pml.velocity );
		return;
	}

	// only add the stepping output when it was a vertical step (second case is at the exit of a ramp)
	if( ( blocked & SLIDEMOVEFLAG_WALL_BLOCKED ) || trace.plane.normal[2] == 1.0f - SLIDEMOVE_PLANEINTERACT_EPSILON )
	{
		pm->step = ( pml.origin[2] - pml.previous_origin[2] );
	}

	// wsw : jal : The following line is what produces the ramp sliding.

	//!! Special case
	// if we were walking along a plane, then we need to copy the Z over
	pml.velocity[2] = down_v[2];
}

//==================
//PM_Friction -- Modified for wsw
//
//Handles both ground friction and water friction
//==================
static void PM_Friction( void )
{
	float *vel;
	float speed, newspeed, control;
	float friction;
	float drop;

	vel = pml.velocity;

	speed = vel[0]*vel[0] +vel[1]*vel[1] + vel[2]*vel[2];
	if( speed < 1 )
	{
		vel[0] = 0;
		vel[1] = 0;
		return;
	}

	speed = sqrt( speed );
	drop = 0;

	// apply ground friction
	if( ( ( ( ( pm->groundentity != -1 ) && !( pml.groundsurfFlags & SURF_SLICK ) ) ) && ( pm->waterlevel < 2 ) ) || ( pml.ladder ) )
	{
		if( pm->playerState->pmove.stats[PM_STAT_KNOCKBACK] <= 0 )
		{
			friction = pm_friction;
			control = speed < pm_decelerate ? pm_decelerate : speed;
			drop += control * friction * pml.frametime;
		}
	}

	// apply water friction
	if( ( pm->waterlevel >= 2 ) && !pml.ladder )
		drop += speed * pm_waterfriction * pm->waterlevel * pml.frametime;

	// scale the velocity
	newspeed = speed - drop;
	if( newspeed <= 0 )
	{
		newspeed = 0;
		VectorClear( vel );
	}
	else
	{
		newspeed /= speed;
		VectorScale( vel, newspeed, vel );
	}
}

//==============
//PM_Accelerate
//
//Handles user intended acceleration
//==============
static void PM_Accelerate( vec3_t wishdir, float wishspeed, float accel )
{
	int i;
	float addspeed, accelspeed, currentspeed;

	currentspeed = DotProduct( pml.velocity, wishdir );
	addspeed = wishspeed - currentspeed;
	if( addspeed <= 0 )
		return;
	accelspeed = accel*pml.frametime*wishspeed;
	if( accelspeed > addspeed )
		accelspeed = addspeed;

	for( i = 0; i < 3; i++ )
		pml.velocity[i] += accelspeed*wishdir[i];
}

static void PM_AirAccelerate( vec3_t wishdir, float wishspeed )
{
	vec3_t curvel, wishvel, acceldir, curdir;
	float addspeed, accelspeed, curspeed;
	float dot;
	// air movement parameters:
	float airforwardaccel = 1.00001f; // Default: 1.0f : how fast you accelerate until you reach pm_maxspeed
	float bunnyaccel = 0.1586f; // (0.42 0.1593f) Default: 0.1585f how fast you accelerate after reaching pm_maxspeed
	// (it gets harder as you near bunnytopspeed)
	float bunnytopspeed = 900; // (0.42: 925) soft speed limit (can get faster with rjs and on ramps)
	float turnaccel = 9.0f;    // (0.42: 9.0) Default: 7 max sharpness of turns
	float backtosideratio = 0.9f; // (0.42: 0.8) Default: 0.8f lower values make it easier to change direction without
	// losing speed; the drawback is "understeering" in sharp turns

	if( !wishspeed )
		return;

	VectorCopy( pml.velocity, curvel );
	curvel[2] = 0;
	curspeed = VectorLength( curvel );

	if( wishspeed > curspeed * 1.01f ) // moving below pm_maxspeed
	{
		float accelspeed = curspeed + airforwardaccel * pml.maxPlayerSpeed * pml.frametime;
		if( accelspeed < wishspeed )
			wishspeed = accelspeed;
	}
	else
	{
		float f = ( bunnytopspeed - curspeed ) / ( bunnytopspeed - pml.maxPlayerSpeed );
		if( f < 0 )
			f = 0;
		wishspeed = max( curspeed, pml.maxPlayerSpeed ) + bunnyaccel * f * pml.maxPlayerSpeed * pml.frametime;
	}
	VectorScale( wishdir, wishspeed, wishvel );
	VectorSubtract( wishvel, curvel, acceldir );
	addspeed = VectorNormalize( acceldir );

	accelspeed = turnaccel * pml.maxPlayerSpeed * pml.frametime;
	if( accelspeed > addspeed )
		accelspeed = addspeed;

	if( backtosideratio < 1.0f )
	{
		VectorNormalize2( curvel, curdir );
		dot = DotProduct( acceldir, curdir );
		if( dot < 0 )
			VectorMA( acceldir, -( 1.0f - backtosideratio ) * dot, curdir, acceldir );
	}

	VectorMA( pml.velocity, accelspeed, acceldir, pml.velocity );
}

// when using +strafe convert the inertia to forward speed.
static void PM_Aircontrol( pmove_t *pm, vec3_t wishdir, float wishspeed )
{
	float zspeed, speed, dot, k;
	int i;
	float fmove, smove;

	if( !pm_aircontrol )
		return;

	// accelerate
	fmove = pml.forwardPush;
	smove = pml.sidePush;

	if( ( smove > 0 || smove < 0 ) || ( wishspeed == 0.0 ) )
		return; // can't control movement if not moving forward or backward

	zspeed = pml.velocity[2];
	pml.velocity[2] = 0;
	speed = VectorNormalize( pml.velocity );


	dot = DotProduct( pml.velocity, wishdir );
	k = 32.0f * pm_aircontrol * dot * dot * pml.frametime;

	if( dot > 0 )
	{
		// we can't change direction while slowing down
		for( i = 0; i < 2; i++ )
			pml.velocity[i] = pml.velocity[i] * speed + wishdir[i] * k;

		VectorNormalize( pml.velocity );
	}

	for( i = 0; i < 2; i++ )
		pml.velocity[i] *= speed;

	pml.velocity[2] = zspeed;
}

#if 0 // never used
static void PM_AirAccelerate( vec3_t wishdir, float wishspeed, float accel )
{
	int i;
	float addspeed, accelspeed, currentspeed, wishspd = wishspeed;

	if( wishspd > 30 )
		wishspd = 30;
	currentspeed = DotProduct( pml.velocity, wishdir );
	addspeed = wishspd - currentspeed;
	if( addspeed <= 0 )
		return;
	accelspeed = accel * wishspeed * pml.frametime;
	if( accelspeed > addspeed )
		accelspeed = addspeed;

	for( i = 0; i < 3; i++ )
		pml.velocity[i] += accelspeed*wishdir[i];
}
#endif


//=============
//PM_AddCurrents
//=============
static void PM_AddCurrents( vec3_t wishvel )
{
	//
	// account for ladders
	//

	if( pml.ladder && fabs( pml.velocity[2] ) <= DEFAULT_LADDERSPEED )
	{
		if( ( pm->playerState->viewangles[PITCH] <= -15 ) && ( pml.forwardPush > 0 ) )
			wishvel[2] = DEFAULT_LADDERSPEED;
		else if( ( pm->playerState->viewangles[PITCH] >= 15 ) && ( pml.forwardPush > 0 ) )
			wishvel[2] = -DEFAULT_LADDERSPEED;
		else if( pml.upPush > 0 )
			wishvel[2] = DEFAULT_LADDERSPEED;
		else if( pml.upPush < 0 )
			wishvel[2] = -DEFAULT_LADDERSPEED;
		else
			wishvel[2] = 0;

		// limit horizontal speed when on a ladder
		clamp( wishvel[0], -25, 25 );
		clamp( wishvel[1], -25, 25 );
	}
}

//===================
//PM_WaterMove
//
//===================
static void PM_WaterMove( void )
{
	int i;
	vec3_t wishvel;
	float wishspeed;
	vec3_t wishdir;

	// user intentions
	for( i = 0; i < 3; i++ )
		wishvel[i] = pml.forward[i]*pml.forwardPush + pml.right[i]*pml.sidePush;

	if( !pml.forwardPush && !pml.sidePush && !pml.upPush )
		wishvel[2] -= 60; // drift towards bottom
	else
		wishvel[2] += pml.upPush;

	PM_AddCurrents( wishvel );

	VectorCopy( wishvel, wishdir );
	wishspeed = VectorNormalize( wishdir );

	if( wishspeed > pml.maxPlayerSpeed )
	{
		wishspeed = pml.maxPlayerSpeed / wishspeed;
		VectorScale( wishvel, wishspeed, wishvel );
		wishspeed = pml.maxPlayerSpeed;
	}
	wishspeed *= 0.5;

	PM_Accelerate( wishdir, wishspeed, pm_wateraccelerate );
	PM_StepSlideMove();
}

//===================
//PM_Move -- Kurim
//
//===================
static void PM_Move( void )
{
	int i;
	vec3_t wishvel;
	float fmove, smove;
	vec3_t wishdir;
	float wishspeed;
	float maxspeed;
	float accel;
	float wishspeed2;

	fmove = pml.forwardPush;
	smove = pml.sidePush;

	for( i = 0; i < 2; i++ )
		wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
	wishvel[2] = 0;

	PM_AddCurrents( wishvel );

	VectorCopy( wishvel, wishdir );
	wishspeed = VectorNormalize( wishdir );

	// clamp to server defined max speed

	if( pm->playerState->pmove.stats[PM_STAT_CROUCHTIME] )
	{
		maxspeed = pml.maxCrouchedSpeed;
	}
	else if( ( pm->cmd.buttons & BUTTON_WALK ) && ( pm->playerState->pmove.stats[PM_STAT_FEATURES] & PMFEAT_WALK ) )
	{
		maxspeed = pml.maxWalkSpeed;
	}
	else
		maxspeed = pml.maxPlayerSpeed;

	if( wishspeed > maxspeed )
	{
		wishspeed = maxspeed / wishspeed;
		VectorScale( wishvel, wishspeed, wishvel );
		wishspeed = maxspeed;
	}

	if( pml.ladder )
	{
		PM_Accelerate( wishdir, wishspeed, pm_accelerate );

		if( !wishvel[2] )
		{
			if( pml.velocity[2] > 0 )
			{
				pml.velocity[2] -= pm->playerState->pmove.gravity * pml.frametime;
				if( pml.velocity[2] < 0 )
					pml.velocity[2]  = 0;
			}
			else
			{
				pml.velocity[2] += pm->playerState->pmove.gravity * pml.frametime;
				if( pml.velocity[2] > 0 )
					pml.velocity[2]  = 0;
			}
		}

		PM_StepSlideMove();
	}
	else if( pm->groundentity != -1 )
	{ 
		// walking on ground
		if( pml.velocity[2] > 0 )
			pml.velocity[2] = 0; //!!! this is before the accel

		PM_Accelerate( wishdir, wishspeed, pm_accelerate );

		// fix for negative trigger_gravity fields
		if( pm->playerState->pmove.gravity > 0 )
		{
			if( pml.velocity[2] > 0 )
				pml.velocity[2] = 0;
		}
		else
			pml.velocity[2] -= pm->playerState->pmove.gravity * pml.frametime;

		if( !pml.velocity[0] && !pml.velocity[1] )
			return;

		PM_StepSlideMove();
	}
	else if( ( pm->playerState->pmove.stats[PM_STAT_FEATURES] & PMFEAT_AIRCONTROL ) 
		&& !( pm->playerState->pmove.stats[PM_STAT_FEATURES] & PMFEAT_FWDBUNNY ) )
	{
		// Air Control
		wishspeed2 = wishspeed;
		if( DotProduct( pml.velocity, wishdir ) < 0 
			&& !( pm->playerState->pmove.pm_flags & PMF_WALLJUMPING ) 
			&& ( pm->playerState->pmove.stats[PM_STAT_KNOCKBACK] <= 0 ) )
			accel = pm_airdecelerate;
		else
			accel = pm_airaccelerate;

		if( ( pm->playerState->pmove.pm_flags & PMF_WALLJUMPING )
			|| ( pm->playerState->pmove.stats[PM_STAT_KNOCKBACK] > 0 ) )
			accel = 0; // no stopmove while walljumping

		if( ( smove > 0 || smove < 0 ) && !fmove && ( pm->playerState->pmove.stats[PM_STAT_KNOCKBACK] <= 0 ) )
		{
			if( wishspeed > pm_wishspeed )
				wishspeed = pm_wishspeed;
			accel = pm_strafebunnyaccel;
		}

		// Air control
		PM_Accelerate( wishdir, wishspeed, accel );
		if( pm_aircontrol && !( pm->playerState->pmove.pm_flags & PMF_WALLJUMPING ) && ( pm->playerState->pmove.stats[PM_STAT_KNOCKBACK] <= 0 ) )  // no air ctrl while wjing
			PM_Aircontrol( pm, wishdir, wishspeed2 );

		// add gravity
		pml.velocity[2] -= pm->playerState->pmove.gravity * pml.frametime;
		PM_StepSlideMove();
	}
	else // air movement
	{
		qboolean inhibit = qfalse;
		qboolean accelerating, decelerating;

		accelerating = ( DotProduct( pml.velocity, wishdir ) > 0.0f );
		decelerating = ( DotProduct( pml.velocity, wishdir ) < -0.0f );
		
		if( ( pm->playerState->pmove.pm_flags & PMF_WALLJUMPING ) &&
			( pm->playerState->pmove.stats[PM_STAT_WJTIME] >= ( PM_WALLJUMP_TIMEDELAY - PM_AIRCONTROL_BOUNCE_DELAY ) ) )
			inhibit = qtrue;

		if( ( pm->playerState->pmove.pm_flags & PMF_DASHING ) &&
			( pm->playerState->pmove.stats[PM_STAT_DASHTIME] >= ( PM_DASHJUMP_TIMEDELAY - PM_AIRCONTROL_BOUNCE_DELAY ) ) )
			inhibit = qtrue;

		if( !( pm->playerState->pmove.stats[PM_STAT_FEATURES] & PMFEAT_FWDBUNNY ) )
			inhibit = qtrue;

		if( pm->playerState->pmove.stats[PM_STAT_KNOCKBACK] > 0 )
			inhibit = qtrue;

		// (aka +fwdbunny) pressing forward or backward but not pressing strafe and not dashing
		if( accelerating && !inhibit && !smove && fmove )
		{
			PM_AirAccelerate( wishdir, wishspeed );
		}
		else // strafe running
		{
			qboolean aircontrol = qtrue;

			wishspeed2 = wishspeed;
			if( decelerating && 
				!( pm->playerState->pmove.pm_flags & PMF_WALLJUMPING ) )
				accel = pm_airdecelerate;
			else
				accel = pm_airaccelerate;

			if( pm->playerState->pmove.pm_flags & PMF_WALLJUMPING 
				|| ( pm->playerState->pmove.stats[PM_STAT_KNOCKBACK] > 0 ) )
			{
				accel = 0; // no stop-move while wall-jumping
				aircontrol = qfalse;
			}

			if( ( pm->playerState->pmove.pm_flags & PMF_DASHING ) &&
				( pm->playerState->pmove.stats[PM_STAT_DASHTIME] >= ( PM_DASHJUMP_TIMEDELAY - PM_AIRCONTROL_BOUNCE_DELAY ) ) )
				aircontrol = qfalse;

			if( !( pm->playerState->pmove.stats[PM_STAT_FEATURES] & PMFEAT_AIRCONTROL ) )
				aircontrol = qfalse;

			// +strafe bunnyhopping
			if( aircontrol && smove && !fmove )
			{
				if( wishspeed > pm_wishspeed )
					wishspeed = pm_wishspeed;

				PM_Accelerate( wishdir, wishspeed, pm_strafebunnyaccel );
				PM_Aircontrol( pm, wishdir, wishspeed2 );
			}
			else // standard movement (includes strafejumping)
			{
				PM_Accelerate( wishdir, wishspeed, accel );
			}
		}

		// add gravity
		pml.velocity[2] -= pm->playerState->pmove.gravity * pml.frametime;
		PM_StepSlideMove();
	}
}


//=============
//PM_CategorizePosition
//=============
static void PM_CategorizePosition( void )
{
	vec3_t point;
	int cont;
	trace_t	trace;
	int sample1;
	int sample2;

	// if the player hull point one-quarter unit down is solid, the player is on ground

	// see if standing on something solid
	point[0] = pml.origin[0];
	point[1] = pml.origin[1];
	point[2] = pml.origin[2] - 0.25;

	if( pml.velocity[2] > 180 ) // !!ZOID changed from 100 to 180 (ramp accel)
	{
		pm->playerState->pmove.pm_flags &= ~PMF_ON_GROUND;
		pm->groundentity = -1;
	}
	else
	{
		module_Trace( &trace, pml.origin, pm->mins, pm->maxs, point, pm->playerState->POVnum, pm->contentmask, 0 );
		pml.groundplane = trace.plane;
		pml.groundsurfFlags = trace.surfFlags;
		pml.groundcontents = trace.contents;

		if( ( trace.fraction == 1 ) || ( !ISWALKABLEPLANE( &trace.plane ) && !trace.startsolid ) )
		{
			pm->groundentity = -1;
			pm->playerState->pmove.pm_flags &= ~PMF_ON_GROUND;
		}
		else
		{
			pm->groundentity = trace.ent;

			// hitting solid ground will end a waterjump
			if( pm->playerState->pmove.pm_flags & PMF_TIME_WATERJUMP )
			{
				pm->playerState->pmove.pm_flags &= ~( PMF_TIME_WATERJUMP | PMF_TIME_LAND | PMF_TIME_TELEPORT );
				pm->playerState->pmove.pm_time = 0;
			}

			if( !( pm->playerState->pmove.pm_flags & PMF_ON_GROUND ) )
			{ // just hit the ground
				pm->playerState->pmove.pm_flags |= PMF_ON_GROUND;
			}
		}

		if( ( pm->numtouch < MAXTOUCH ) && ( trace.fraction < 1.0 ) )
		{
			pm->touchents[pm->numtouch] = trace.ent;
			pm->numtouch++;
		}
	}

	//
	// get waterlevel, accounting for ducking
	//
	pm->waterlevel = 0;
	pm->watertype = 0;

	sample2 = pm->playerState->viewheight - pm->mins[2];
	sample1 = sample2 / 2;

	point[2] = pml.origin[2] + pm->mins[2] + 1;
	cont = module_PointContents( point, 0 );

	if( cont & MASK_WATER )
	{
		pm->watertype = cont;
		pm->waterlevel = 1;
		point[2] = pml.origin[2] + pm->mins[2] + sample1;
		cont = module_PointContents( point, 0 );
		if( cont & MASK_WATER )
		{
			pm->waterlevel = 2;
			point[2] = pml.origin[2] + pm->mins[2] + sample2;
			cont = module_PointContents( point, 0 );
			if( cont & MASK_WATER )
				pm->waterlevel = 3;
		}
	}
}

static void PM_ClearDash( void )
{
	pm->playerState->pmove.pm_flags &= ~PMF_DASHING;
	pm->playerState->pmove.stats[PM_STAT_DASHTIME] = 0;
}

static void PM_ClearWallJump( void )
{
	pm->playerState->pmove.pm_flags &= ~PMF_WALLJUMPING;
	pm->playerState->pmove.pm_flags &= ~PMF_WALLJUMPCOUNT;
	pm->playerState->pmove.stats[PM_STAT_WJTIME] = 0;
}

static void PM_ClearStun( void )
{
	pm->playerState->pmove.stats[PM_STAT_STUN] = 0;
}

//=============
//PM_CheckJump
//=============
static void PM_CheckJump( void )
{
	if( pml.upPush < 10 )
	{ 
		// not holding jump
		if( !( pm->playerState->pmove.stats[PM_STAT_FEATURES] & PMFEAT_CONTINOUSJUMP ) )
			pm->playerState->pmove.pm_flags &= ~PMF_JUMP_HELD;

		return;
	}

	if( !( pm->playerState->pmove.stats[PM_STAT_FEATURES] & PMFEAT_CONTINOUSJUMP ) )
	{
		// must wait for jump to be released
		if( pm->playerState->pmove.pm_flags & PMF_JUMP_HELD )
			return;
	}

	if( pm->playerState->pmove.pm_type != PM_NORMAL )
		return;

	if( pm->waterlevel >= 2 )
	{ // swimming, not jumping
		pm->groundentity = -1;
		return;
	}

	if( pm->groundentity == -1 )
		return;

	if( !( pm->playerState->pmove.stats[PM_STAT_FEATURES] & PMFEAT_JUMP ) )
		return;

	if( !( pm->playerState->pmove.stats[PM_STAT_FEATURES] & PMFEAT_CONTINOUSJUMP ) )
		pm->playerState->pmove.pm_flags |= PMF_JUMP_HELD;

	pm->groundentity = -1;

	//if( gs.module == GS_MODULE_GAME ) GS_Printf( "upvel %f\n", pml.velocity[2] );
	if( pml.velocity[2] > 100 )
	{
		module_PredictedEvent( pm->playerState->POVnum, EV_DOUBLEJUMP, 0 );
		pml.velocity[2] += pml.jumpPlayerSpeed;
	}
	else if( pml.velocity[2] > 0 )
	{
		module_PredictedEvent( pm->playerState->POVnum, EV_JUMP, 0 );
		pml.velocity[2] += pml.jumpPlayerSpeed;
	}
	else
	{
		module_PredictedEvent( pm->playerState->POVnum, EV_JUMP, 0 );
		pml.velocity[2] = pml.jumpPlayerSpeed;
	}

	// remove wj count
	pm->playerState->pmove.pm_flags &= ~PMF_JUMPPAD_TIME;
	PM_ClearDash();
	PM_ClearWallJump();
}

//=============
//PM_CheckDash -- by Kurim
//=============
static void PM_CheckDash( void )
{
	float actual_velocity;
	float upspeed;
	vec3_t dashdir;

	if( !( pm->cmd.buttons & BUTTON_SPECIAL ) )
		pm->playerState->pmove.pm_flags &= ~PMF_SPECIAL_HELD;

	if( pm->playerState->pmove.pm_type != PM_NORMAL )
		return;

	if( pm->playerState->pmove.stats[PM_STAT_DASHTIME] > 0 )
		return;

	if( pm->playerState->pmove.stats[PM_STAT_KNOCKBACK] > 0 ) // can not start a new dash during knockback time
		return;

	if( ( pm->cmd.buttons & BUTTON_SPECIAL ) && pm->groundentity != -1 
		&& ( pm->playerState->pmove.stats[PM_STAT_FEATURES] & PMFEAT_DASH ) )
	{
		if( pm->playerState->pmove.pm_flags & PMF_SPECIAL_HELD )
			return;

		pm->playerState->pmove.pm_flags &= ~PMF_JUMPPAD_TIME;
		PM_ClearWallJump();

		pm->playerState->pmove.pm_flags |= PMF_DASHING;
		pm->playerState->pmove.pm_flags |= PMF_SPECIAL_HELD;
		pm->groundentity = -1;

		if( pml.velocity[2] <= 0.0f )
			upspeed = pm_dashupspeed;
		else
			upspeed = pm_dashupspeed + pml.velocity[2];

		VectorMA( vec3_origin, pml.forwardPush, pml.flatforward, dashdir );
		VectorMA( dashdir, pml.sidePush, pml.right, dashdir );
		dashdir[2] = 0.0;

		if( VectorLength( dashdir ) < 0.01f )  // if not moving, dash like a "forward dash"
			VectorCopy( pml.flatforward, dashdir );

		VectorNormalizeFast( dashdir );

		actual_velocity = VectorNormalize2D( pml.velocity );
		if( actual_velocity <= pml.dashPlayerSpeed )
			VectorScale( dashdir, pml.dashPlayerSpeed, dashdir );
		else
			VectorScale( dashdir, actual_velocity, dashdir );

		VectorCopy( dashdir, pml.velocity );
		pml.velocity[2] = upspeed;

		pm->playerState->pmove.stats[PM_STAT_DASHTIME] = PM_DASHJUMP_TIMEDELAY;

		// return sound events
		if( abs( pml.sidePush ) > 10 && abs( pml.sidePush ) >= abs( pml.forwardPush ) )
		{
			if( pml.sidePush > 0 )
			{
				module_PredictedEvent( pm->playerState->POVnum, EV_DASH, 2 );
			}
			else
			{
				module_PredictedEvent( pm->playerState->POVnum, EV_DASH, 1 );
			}
		}
		else if( pml.forwardPush < -10 )
		{
			module_PredictedEvent( pm->playerState->POVnum, EV_DASH, 3 );
		}
		else
		{
			module_PredictedEvent( pm->playerState->POVnum, EV_DASH, 0 );
		}
	}
	else if( pm->groundentity == -1 )
		pm->playerState->pmove.pm_flags &= ~PMF_DASHING;
}

//=================
//PM_CheckWallJump -- By Kurim
//=================
static void PM_CheckWallJump( void )
{
	vec3_t normal;
	float hspeed;

	if( !( pm->cmd.buttons & BUTTON_SPECIAL ) )
		pm->playerState->pmove.pm_flags &= ~PMF_SPECIAL_HELD;

	if( pm->groundentity != -1 )
	{
		pm->playerState->pmove.pm_flags &= ~PMF_WALLJUMPING;
		pm->playerState->pmove.pm_flags &= ~PMF_WALLJUMPCOUNT;
	}

	if( pm->playerState->pmove.pm_flags & PMF_WALLJUMPING && pml.velocity[2] < 0.0 )
		pm->playerState->pmove.pm_flags &= ~PMF_WALLJUMPING;

	if( pm->playerState->pmove.stats[PM_STAT_WJTIME] <= 0 )  // reset the wj count after wj delay
		pm->playerState->pmove.pm_flags &= ~PMF_WALLJUMPCOUNT;

	if( pm->playerState->pmove.pm_type != PM_NORMAL )
		return;

	// don't walljump in the first 100 milliseconds of a dash jump
	if( pm->playerState->pmove.pm_flags & PMF_DASHING 
		&& ( pm->playerState->pmove.stats[PM_STAT_DASHTIME] > ( PM_DASHJUMP_TIMEDELAY - 100 ) ) )
		return;

	
	// markthis

	if( pm->groundentity == -1 && ( pm->cmd.buttons & BUTTON_SPECIAL ) 
		&& ( pm->playerState->pmove.stats[PM_STAT_FEATURES] & PMFEAT_WALLJUMP ) &&
		( !( pm->playerState->pmove.pm_flags & PMF_WALLJUMPCOUNT ) )
		&& pm->playerState->pmove.stats[PM_STAT_WJTIME] <= 0
		)
	{
		trace_t trace;
		vec3_t point;

		point[0] = pml.origin[0];
		point[1] = pml.origin[1];
		point[2] = pml.origin[2] - STEPSIZE;

		// don't walljump if our height is smaller than a step 
		// unless the player is moving faster than dash speed and upwards
		hspeed = VectorLengthFast( tv( pml.velocity[0], pml.velocity[1], 0 ) );
		module_Trace( &trace, pml.origin, pm->mins, pm->maxs, point, pm->playerState->POVnum, pm->contentmask, 0 );
		
		if( ( hspeed > pm->playerState->pmove.stats[PM_STAT_DASHSPEED] && pml.velocity[2] > 8 ) 
			|| ( trace.fraction == 1 ) || ( !ISWALKABLEPLANE( &trace.plane ) && !trace.startsolid ) )
		{
			VectorClear( normal );
			PlayerTouchWall( 12, 0.3f, &normal );
			if( !VectorLength( normal ) )
				return;

			if( !( pm->playerState->pmove.pm_flags & PMF_SPECIAL_HELD ) 
				&& !( pm->playerState->pmove.pm_flags & PMF_WALLJUMPING ) )
			{
				float oldupvelocity = pml.velocity[2];
				pml.velocity[2] = 0.0;

				hspeed = VectorNormalize2D( pml.velocity );

				// if stunned almost do nothing
				if( pm->playerState->pmove.stats[PM_STAT_STUN] > 0 )
				{
					GS_ClipVelocity( pml.velocity, normal, pml.velocity, 1.0f );
					VectorMA( pml.velocity, pm_failedwjbouncefactor, normal, pml.velocity );

					VectorNormalize( pml.velocity );

					VectorScale( pml.velocity, hspeed, pml.velocity );
					pml.velocity[2] = ( oldupvelocity + pm_failedwjupspeed > pm_failedwjupspeed ) ? oldupvelocity : oldupvelocity + pm_failedwjupspeed;
				}
				else
				{
					GS_ClipVelocity( pml.velocity, normal, pml.velocity, 1.0005f );
					VectorMA( pml.velocity, pm_wjbouncefactor, normal, pml.velocity );

					if( hspeed < pm_wjminspeed )
						hspeed = pm_wjminspeed;

					VectorNormalize( pml.velocity );

					VectorScale( pml.velocity, hspeed, pml.velocity );
					pml.velocity[2] = ( oldupvelocity > pm_wjupspeed ) ? oldupvelocity : pm_wjupspeed; // jal: if we had a faster upwards speed, keep it
				}

				// set the walljumping state
				PM_ClearDash();
				pm->playerState->pmove.pm_flags &= ~PMF_JUMPPAD_TIME;

				pm->playerState->pmove.pm_flags |= PMF_WALLJUMPING;
				pm->playerState->pmove.pm_flags |= PMF_SPECIAL_HELD;

				pm->playerState->pmove.pm_flags |= PMF_WALLJUMPCOUNT;

				if( pm->playerState->pmove.stats[PM_STAT_STUN] > 0 )
				{
					pm->playerState->pmove.stats[PM_STAT_WJTIME] = PM_WALLJUMP_FAILED_TIMEDELAY;

					// Create the event
					module_PredictedEvent( pm->playerState->POVnum, EV_WALLJUMP_FAILED, DirToByte( normal ) );
				}
				else
				{
					pm->playerState->pmove.stats[PM_STAT_WJTIME] = PM_WALLJUMP_TIMEDELAY;

					// Create the event
					module_PredictedEvent( pm->playerState->POVnum, EV_WALLJUMP, DirToByte( normal ) );
				}
			}
		}
	}
	else
		pm->playerState->pmove.pm_flags &= ~PMF_WALLJUMPING;
}

//=============
//PM_CheckSpecialMovement
//=============
static void PM_CheckSpecialMovement( void )
{
	vec3_t spot;
	int cont;
	trace_t	trace;

	if( pm->playerState->pmove.pm_time )
		return;

	pml.ladder = qfalse;

	// check for ladder
	VectorMA( pml.origin, 1, pml.flatforward, spot );
	module_Trace( &trace, pml.origin, pm->mins, pm->maxs, spot, pm->playerState->POVnum, pm->contentmask, 0 );
	if( ( trace.fraction < 1 ) && ( trace.surfFlags & SURF_LADDER ) )
		pml.ladder = qtrue;

	// check for water jump
	if( pm->waterlevel != 2 )
		return;

	VectorMA( pml.origin, 30, pml.flatforward, spot );
	spot[2] += 4;
	cont = module_PointContents( spot, 0 );
	if( !( cont & CONTENTS_SOLID ) )
		return;

	spot[2] += 16;
	cont = module_PointContents( spot, 0 );
	if( cont )
		return;
	// jump out of water
	VectorScale( pml.flatforward, 50, pml.velocity );
	pml.velocity[2] = 350;

	pm->playerState->pmove.pm_flags |= PMF_TIME_WATERJUMP;
	pm->playerState->pmove.pm_time = 255;
}

//===============
//PM_FlyMove
//===============
static void PM_FlyMove( qboolean doclip )
{
	float speed, drop, friction, control, newspeed;
	float currentspeed, addspeed, accelspeed, maxspeed;
	int i;
	vec3_t wishvel;
	float fmove, smove;
	vec3_t wishdir;
	float wishspeed;
	vec3_t end;
	trace_t	trace;

	maxspeed = pml.maxPlayerSpeed * 1.5;

	if( pm->cmd.buttons & BUTTON_SPECIAL )
		maxspeed *= 2;

	// friction
	speed = VectorLength( pml.velocity );
	if( speed < 1 )
	{
		VectorClear( pml.velocity );
	}
	else
	{
		drop = 0;

		friction = pm_friction * 1.5; // extra friction
		control = speed < pm_decelerate ? pm_decelerate : speed;
		drop += control * friction * pml.frametime;

		// scale the velocity
		newspeed = speed - drop;
		if( newspeed < 0 )
			newspeed = 0;
		newspeed /= speed;

		VectorScale( pml.velocity, newspeed, pml.velocity );
	}

	// accelerate
	fmove = pml.forwardPush;
	smove = pml.sidePush;

	if( pm->cmd.buttons & BUTTON_SPECIAL )
	{
		fmove *= 2;
		smove *= 2;
	}

	VectorNormalize( pml.forward );
	VectorNormalize( pml.right );

	for( i = 0; i < 3; i++ )
		wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
	wishvel[2] += pml.upPush;

	VectorCopy( wishvel, wishdir );
	wishspeed = VectorNormalize( wishdir );


	// clamp to server defined max speed
	//
	if( wishspeed > maxspeed )
	{
		wishspeed = maxspeed/wishspeed;
		VectorScale( wishvel, wishspeed, wishvel );
		wishspeed = maxspeed;
	}

	currentspeed = DotProduct( pml.velocity, wishdir );
	addspeed = wishspeed - currentspeed;
	if( addspeed > 0 )
	{
		accelspeed = pm_accelerate * pml.frametime * wishspeed;
		if( accelspeed > addspeed )
			accelspeed = addspeed;

		for( i = 0; i < 3; i++ )
			pml.velocity[i] += accelspeed*wishdir[i];
	}

	if( doclip )
	{
		for( i = 0; i < 3; i++ )
			end[i] = pml.origin[i] + pml.frametime * pml.velocity[i];

		module_Trace( &trace, pml.origin, pm->mins, pm->maxs, end, pm->playerState->POVnum, pm->contentmask, 0 );

		VectorCopy( trace.endpos, pml.origin );
	}
	else
	{
		// move
		VectorMA( pml.origin, pml.frametime, pml.velocity, pml.origin );
	}
}

static void PM_CheckZoom( void )
{
	if( pm->playerState->pmove.pm_type != PM_NORMAL )
	{
		pm->playerState->pmove.stats[PM_STAT_ZOOMTIME] = 0;
		return;
	}

	if( ( pm->cmd.buttons & BUTTON_ZOOM ) && ( pm->playerState->pmove.stats[PM_STAT_FEATURES] & PMFEAT_ZOOM ) )
	{
		pm->playerState->pmove.stats[PM_STAT_ZOOMTIME] += pm->cmd.msec;
		clamp( pm->playerState->pmove.stats[PM_STAT_ZOOMTIME], 0, ZOOMTIME );
	}
	else if( pm->playerState->pmove.stats[PM_STAT_ZOOMTIME] > 0 )
	{
		pm->playerState->pmove.stats[PM_STAT_ZOOMTIME] -= pm->cmd.msec;
		clamp( pm->playerState->pmove.stats[PM_STAT_ZOOMTIME], 0, ZOOMTIME );
	}
}

//==============
//PM_AdjustBBox
//
//Sets mins, maxs, and pm->viewheight
//==============
static void PM_AdjustBBox( void )
{
	float crouchFrac;
	trace_t	trace;

	if( pm->playerState->pmove.pm_type == PM_GIB )
	{
		pm->playerState->pmove.stats[PM_STAT_CROUCHTIME] = 0;
		VectorCopy( playerbox_gib_maxs, pm->maxs );
		VectorCopy( playerbox_gib_mins, pm->mins );
		pm->playerState->viewheight = playerbox_gib_viewheight;
		return;
	}

	if( pm->playerState->pmove.pm_type >= PM_FREEZE )
	{
		pm->playerState->pmove.stats[PM_STAT_CROUCHTIME] = 0;
		pm->playerState->viewheight = 0;
		return;
	}

	if( pm->playerState->pmove.pm_type == PM_SPECTATOR )
	{
		pm->playerState->pmove.stats[PM_STAT_CROUCHTIME] = 0;
		pm->playerState->viewheight = playerbox_stand_viewheight;
	}

	if( pml.upPush < 0 && ( pm->playerState->pmove.stats[PM_STAT_FEATURES] & PMFEAT_CROUCH ) && 
		pm->playerState->pmove.stats[PM_STAT_WJTIME] < ( PM_WALLJUMP_TIMEDELAY - PM_SPECIAL_CROUCH_INHIBIT ) &&
		pm->playerState->pmove.stats[PM_STAT_DASHTIME] < ( PM_DASHJUMP_TIMEDELAY - PM_SPECIAL_CROUCH_INHIBIT ) )
	{
		pm->playerState->pmove.stats[PM_STAT_CROUCHTIME] += pm->cmd.msec;
		clamp( pm->playerState->pmove.stats[PM_STAT_CROUCHTIME], 0, CROUCHTIME );

		crouchFrac = (float)pm->playerState->pmove.stats[PM_STAT_CROUCHTIME] / (float)CROUCHTIME;
		VectorLerp( playerbox_stand_mins, crouchFrac, playerbox_crouch_mins, pm->mins );
		VectorLerp( playerbox_stand_maxs, crouchFrac, playerbox_crouch_maxs, pm->maxs );
		pm->playerState->viewheight = playerbox_stand_viewheight - ( crouchFrac * ( playerbox_stand_viewheight - playerbox_crouch_viewheight ) );

		// it's going down, so, no need of checking for head-chomping
		return;
	}

	// it's crouched, but not pressing the crouch button anymore, try to stand up
	if( pm->playerState->pmove.stats[PM_STAT_CROUCHTIME] != 0 )
	{
		vec3_t curmins, curmaxs, wishmins, wishmaxs;
		float curviewheight, wishviewheight;
		int newcrouchtime;

		// find the current size
		crouchFrac = (float)pm->playerState->pmove.stats[PM_STAT_CROUCHTIME] / (float)CROUCHTIME;
		VectorLerp( playerbox_stand_mins, crouchFrac, playerbox_crouch_mins, curmins );
		VectorLerp( playerbox_stand_maxs, crouchFrac, playerbox_crouch_maxs, curmaxs );
		curviewheight = playerbox_stand_viewheight - ( crouchFrac * ( playerbox_stand_viewheight - playerbox_crouch_viewheight ) );

		if( !pm->cmd.msec ) // no need to continue
		{
			VectorCopy( curmins, pm->mins );
			VectorCopy( curmaxs, pm->maxs );
			pm->playerState->viewheight = curviewheight;
			return;
		}

		// find the desired size
		newcrouchtime = pm->playerState->pmove.stats[PM_STAT_CROUCHTIME] - pm->cmd.msec;
		clamp( newcrouchtime, 0, CROUCHTIME );
		crouchFrac = (float)newcrouchtime / (float)CROUCHTIME;
		VectorLerp( playerbox_stand_mins, crouchFrac, playerbox_crouch_mins, wishmins );
		VectorLerp( playerbox_stand_maxs, crouchFrac, playerbox_crouch_maxs, wishmaxs );
		wishviewheight = playerbox_stand_viewheight - ( crouchFrac * ( playerbox_stand_viewheight - playerbox_crouch_viewheight ) );

		// check that the head is not blocked
		module_Trace( &trace, pml.origin, wishmins, wishmaxs, pml.origin, pm->playerState->POVnum, pm->contentmask, 0 );
		if( trace.allsolid || trace.startsolid )
		{
			// can't do the uncrouching, let the time alone and use old position
			VectorCopy( curmins, pm->mins );
			VectorCopy( curmaxs, pm->maxs );
			pm->playerState->viewheight = curviewheight;
			return;
		}

		// can do the uncrouching, use new position and update the time
		pm->playerState->pmove.stats[PM_STAT_CROUCHTIME] = newcrouchtime;
		VectorCopy( wishmins, pm->mins );
		VectorCopy( wishmaxs, pm->maxs );
		pm->playerState->viewheight = wishviewheight;
		return;
	}

	// the player is not crouching at all
	VectorCopy( playerbox_stand_mins, pm->mins );
	VectorCopy( playerbox_stand_maxs, pm->maxs );
	pm->playerState->viewheight = playerbox_stand_viewheight;
}

//==============
//PM_AdjustViewheight
//==============
void PM_AdjustViewheight( void )
{
	float height;
	vec3_t pm_maxs, mins, maxs;

	if( pm->playerState->pmove.pm_type == PM_SPECTATOR )
	{
		VectorCopy( playerbox_stand_mins, mins );
		VectorCopy( playerbox_stand_maxs, maxs );
	}
	else
	{
		VectorCopy( pm->mins, mins );
		VectorCopy( pm->maxs, maxs );
	}

	VectorCopy( maxs, pm_maxs );
	module_RoundUpToHullSize( mins, maxs );

	height = pm_maxs[2] - maxs[2];
	if( height > 0 )
		pm->playerState->viewheight -= height;
}

static qboolean PM_GoodPosition( int snaptorigin[3] )
{
	trace_t	trace;
	vec3_t origin, end;
	int i;

	if( pm->playerState->pmove.pm_type == PM_SPECTATOR )
		return qtrue;

	for( i = 0; i < 3; i++ )
		origin[i] = end[i] = snaptorigin[i]*( 1.0/PM_VECTOR_SNAP );
	module_Trace( &trace, origin, pm->mins, pm->maxs, end, pm->playerState->POVnum, pm->contentmask, 0 );

	return !trace.allsolid;
}

//================
//PM_SnapPosition
//
//On exit, the origin will have a value that is pre-quantized to the (1.0/16.0)
//precision of the network channel and in a valid position.
//================
static void PM_SnapPosition( void )
{
	int sign[3];
	int i, j, bits;
	int base[3];
	int velint[3], origint[3];
	// try all single bits first
	static const int jitterbits[8] = { 0, 4, 1, 2, 3, 5, 6, 7 };

	// snap velocity to sixteenths
	for( i = 0; i < 3; i++ )
	{
		velint[i] = (int)( pml.velocity[i]*PM_VECTOR_SNAP );
		pm->playerState->pmove.velocity[i] = velint[i]*( 1.0/PM_VECTOR_SNAP );
	}

	for( i = 0; i < 3; i++ )
	{
		if( pml.origin[i] >= 0 )
			sign[i] = 1;
		else
			sign[i] = -1;
		origint[i] = (int)( pml.origin[i]*PM_VECTOR_SNAP );
		if( origint[i]*( 1.0/PM_VECTOR_SNAP ) == pml.origin[i] )
			sign[i] = 0;
	}
	VectorCopy( origint, base );

	// try all combinations
	for( j = 0; j < 8; j++ )
	{
		bits = jitterbits[j];
		VectorCopy( base, origint );
		for( i = 0; i < 3; i++ )
			if( bits & ( 1<<i ) )
				origint[i] += sign[i];

		if( PM_GoodPosition( origint ) )
		{
			VectorScale( origint, ( 1.0/PM_VECTOR_SNAP ), pm->playerState->pmove.origin );
			return;
		}
	}

	// go back to the last position
	VectorCopy( pml.previous_origin, pm->playerState->pmove.origin );
	VectorClear( pm->playerState->pmove.velocity );
}


//================
//PM_InitialSnapPosition
//
//================
static void PM_InitialSnapPosition( void )
{
	int x, y, z;
	int base[3];
	static const int offset[3] = { 0, -1, 1 };
	int origint[3];

	VectorScale( pm->playerState->pmove.origin, PM_VECTOR_SNAP, origint );
	VectorCopy( origint, base );

	for( z = 0; z < 3; z++ )
	{
		origint[2] = base[2] + offset[z];
		for( y = 0; y < 3; y++ )
		{
			origint[1] = base[1] + offset[y];
			for( x = 0; x < 3; x++ )
			{
				origint[0] = base[0] + offset[x];
				if( PM_GoodPosition( origint ) )
				{
					pml.origin[0] = pm->playerState->pmove.origin[0] = origint[0]*( 1.0/PM_VECTOR_SNAP );
					pml.origin[1] = pm->playerState->pmove.origin[1] = origint[1]*( 1.0/PM_VECTOR_SNAP );
					pml.origin[2] = pm->playerState->pmove.origin[2] = origint[2]*( 1.0/PM_VECTOR_SNAP );
					VectorCopy( pm->playerState->pmove.origin, pml.previous_origin );
					return;
				}
			}
		}
	}
}

static void PM_UpdateDeltaAngles( void )
{
	int i;

	if( gs.module != GS_MODULE_GAME )
		return;

	for( i = 0; i < 3; i++ )
		pm->playerState->pmove.delta_angles[i] = ANGLE2SHORT( pm->playerState->viewangles[i] ) - pm->cmd.angles[i];
}

//================
//PM_ApplyMouseAnglesClamp
//
//================
#if defined ( _WIN32 ) && ( _MSC_VER >= 1400 )
#pragma warning( push )
#pragma warning( disable : 4310 )   // cast truncates constant value
#endif
static void PM_ApplyMouseAnglesClamp( void )
{
	int i;
	short temp;

	for( i = 0; i < 3; i++ )
	{
		temp = pm->cmd.angles[i] + pm->playerState->pmove.delta_angles[i];
		if( i == PITCH )
		{
			// don't let the player look up or down more than 90 degrees
			if( temp > (short)ANGLE2SHORT( 90 ) - 1 )
			{
				pm->playerState->pmove.delta_angles[i] = ( ANGLE2SHORT( 90 ) - 1 ) - pm->cmd.angles[i];
				temp = ANGLE2SHORT( 90 ) - 1;
			}
			else if( temp < (short)ANGLE2SHORT( -90 ) + 1 )
			{
				pm->playerState->pmove.delta_angles[i] = ( ANGLE2SHORT( -90 ) + 1 ) - pm->cmd.angles[i];
				temp = ANGLE2SHORT( -90 ) + 1;
			}
		}

		pm->playerState->viewangles[i] = SHORT2ANGLE( temp );
	}

	AngleVectors( pm->playerState->viewangles, pml.forward, pml.right, pml.up );

	VectorCopy( pml.forward, pml.flatforward );
	pml.flatforward[2] = 0.0f;
	VectorNormalize( pml.flatforward );
}
#if defined ( _WIN32 ) && ( _MSC_VER >= 1400 )
#pragma warning( pop )
#endif

//================
//Pmove
//
//Can be called by either the server or the client
//================
void Pmove( pmove_t *pmove )
{
	float fallvelocity, falldelta, damage;
	int oldGroundEntity;

	if( !pmove->playerState )
		return;

	pm = pmove;

	// clear results
	pm->numtouch = 0;
	pm->groundentity = -1;
	pm->watertype = 0;
	pm->waterlevel = 0;
	pm->step = qfalse;

	// clear all pmove local vars
	memset( &pml, 0, sizeof( pml ) );

	VectorCopy( pm->playerState->pmove.origin, pml.origin );
	VectorCopy( pm->playerState->pmove.velocity, pml.velocity );

	fallvelocity = ( ( pml.velocity[2] < 0.0f ) ? abs( pml.velocity[2] ) : 0.0f );

	// save old org in case we get stuck
	VectorCopy( pm->playerState->pmove.origin, pml.previous_origin );

	pml.frametime = pm->cmd.msec * 0.001;

	pml.maxPlayerSpeed = pm->playerState->pmove.stats[PM_STAT_MAXSPEED];
	if( pml.maxPlayerSpeed < 0 )
		pml.maxPlayerSpeed = DEFAULT_PLAYERSPEED;

	pml.jumpPlayerSpeed = (float)pm->playerState->pmove.stats[PM_STAT_JUMPSPEED] * GRAVITY_COMPENSATE;
	if( pml.jumpPlayerSpeed < 0 )
		pml.jumpPlayerSpeed = DEFAULT_JUMPSPEED * GRAVITY_COMPENSATE;

	pml.dashPlayerSpeed = pm->playerState->pmove.stats[PM_STAT_DASHSPEED];
	if( pml.dashPlayerSpeed < 0 )
		pml.dashPlayerSpeed = DEFAULT_DASHSPEED;

	pml.maxWalkSpeed = DEFAULT_WALKSPEED;
	if( pml.maxWalkSpeed > pml.maxPlayerSpeed * 0.66f )
		pml.maxWalkSpeed = pml.maxPlayerSpeed * 0.66f;

	pml.maxCrouchedSpeed = DEFAULT_CROUCHEDSPEED;
	if( pml.maxCrouchedSpeed > pml.maxPlayerSpeed * 0.5f )
		pml.maxCrouchedSpeed = pml.maxPlayerSpeed * 0.5f;

	// assign a contentmask for the movement type
	switch( pm->playerState->pmove.pm_type )
	{
	case PM_FREEZE:
	case PM_CHASECAM:
		if( gs.module == GS_MODULE_GAME )
			pm->playerState->pmove.pm_flags |= PMF_NO_PREDICTION;
		pm->contentmask = 0;
		break;

	case PM_GIB:
		if( gs.module == GS_MODULE_GAME )
			pm->playerState->pmove.pm_flags |= PMF_NO_PREDICTION;
		pm->contentmask = MASK_DEADSOLID;
		break;

	case PM_SPECTATOR:
		if( gs.module == GS_MODULE_GAME )
			pm->playerState->pmove.pm_flags &= ~PMF_NO_PREDICTION;
		pm->contentmask = MASK_DEADSOLID;
		break;

	default:
	case PM_NORMAL:
		if( gs.module == GS_MODULE_GAME )
			pm->playerState->pmove.pm_flags &= ~PMF_NO_PREDICTION;
		if( pm->playerState->pmove.stats[PM_STAT_FEATURES] & PMFEAT_GHOSTMOVE )
			pm->contentmask = MASK_DEADSOLID;
		else
			pm->contentmask = MASK_PLAYERSOLID;
		break;
	}

	if( ! GS_MatchPaused() )
	{
		// drop timing counters
		if( pm->playerState->pmove.pm_time )
		{
			int msec;

			msec = pm->cmd.msec >> 3;
			if( !msec )
				msec = 1;
			if( msec >= pm->playerState->pmove.pm_time )
			{
				pm->playerState->pmove.pm_flags &= ~( PMF_TIME_WATERJUMP | PMF_TIME_LAND | PMF_TIME_TELEPORT );
				pm->playerState->pmove.pm_time = 0;
			}
			else
				pm->playerState->pmove.pm_time -= msec;
		}

		if( pm->playerState->pmove.stats[PM_STAT_NOUSERCONTROL] > 0 )
			pm->playerState->pmove.stats[PM_STAT_NOUSERCONTROL] -= pm->cmd.msec;
		else if( pm->playerState->pmove.stats[PM_STAT_NOUSERCONTROL] < 0 )
			pm->playerState->pmove.stats[PM_STAT_NOUSERCONTROL] = 0;

		if( pm->playerState->pmove.stats[PM_STAT_KNOCKBACK] > 0 )
			pm->playerState->pmove.stats[PM_STAT_KNOCKBACK] -= pm->cmd.msec;
		else if( pm->playerState->pmove.stats[PM_STAT_KNOCKBACK] < 0 )
			pm->playerState->pmove.stats[PM_STAT_KNOCKBACK] = 0;

		// PM_STAT_CROUCHTIME is handled at PM_AdjustBBox
		// PM_STAT_ZOOMTIME is handled at PM_CheckZoom

		if( pm->playerState->pmove.stats[PM_STAT_DASHTIME] > 0 )
			pm->playerState->pmove.stats[PM_STAT_DASHTIME] -= pm->cmd.msec;
		else if( pm->playerState->pmove.stats[PM_STAT_DASHTIME] < 0 )
			pm->playerState->pmove.stats[PM_STAT_DASHTIME] = 0;

		if( pm->playerState->pmove.stats[PM_STAT_WJTIME] > 0 )
			pm->playerState->pmove.stats[PM_STAT_WJTIME] -= pm->cmd.msec;
		else if( pm->playerState->pmove.stats[PM_STAT_WJTIME] < 0 )
			pm->playerState->pmove.stats[PM_STAT_WJTIME] = 0;

		if( pm->playerState->pmove.stats[PM_STAT_NOAUTOATTACK] > 0 )
			pm->playerState->pmove.stats[PM_STAT_NOAUTOATTACK] -= pm->cmd.msec;
		else if( pm->playerState->pmove.stats[PM_STAT_NOAUTOATTACK] < 0 )
			pm->playerState->pmove.stats[PM_STAT_NOAUTOATTACK] = 0;

		if( pm->playerState->pmove.stats[PM_STAT_STUN] > 0 )
			pm->playerState->pmove.stats[PM_STAT_STUN] -= pm->cmd.msec;
		else if( pm->playerState->pmove.stats[PM_STAT_STUN] < 0 )
			pm->playerState->pmove.stats[PM_STAT_STUN] = 0;
	}

	pml.forwardPush = pm->cmd.forwardfrac * SPEEDKEY;
	pml.sidePush = pm->cmd.sidefrac * SPEEDKEY;
	pml.upPush = pm->cmd.upfrac * SPEEDKEY;

	if( pm->playerState->pmove.stats[PM_STAT_NOUSERCONTROL] > 0 )
	{
		pml.forwardPush = 0;
		pml.sidePush = 0;
		pml.upPush = 0;
		pm->cmd.buttons = 0;
	}

	if( pm->snapinitial )
		PM_InitialSnapPosition();

	if( pm->playerState->pmove.pm_type != PM_NORMAL ) // includes dead, freeze, chasecam...
	{
		if( !GS_MatchPaused() )
		{
			PM_ClearDash();
			PM_ClearWallJump();
			PM_ClearStun();
			pm->playerState->pmove.stats[PM_STAT_KNOCKBACK] = 0;
			pm->playerState->pmove.stats[PM_STAT_CROUCHTIME] = 0;
			pm->playerState->pmove.stats[PM_STAT_ZOOMTIME] = 0;
			pm->playerState->pmove.pm_flags &= ~(PMF_JUMPPAD_TIME|PMF_DOUBLEJUMPED|PMF_TIME_WATERJUMP|PMF_TIME_LAND|PMF_TIME_TELEPORT|PMF_SPECIAL_HELD);

			PM_AdjustBBox();
		}

		PM_AdjustViewheight();

		if( pm->playerState->pmove.pm_type == PM_SPECTATOR )
		{
			PM_ApplyMouseAnglesClamp();
			PM_FlyMove( qfalse );
		}
		else
		{
			pml.forwardPush = 0;
			pml.sidePush = 0;
			pml.upPush = 0;
		}
		
		PM_SnapPosition();
		return;
	}

	PM_ApplyMouseAnglesClamp();

	// set mins, maxs, viewheight amd fov
	PM_AdjustBBox();
	PM_CheckZoom();

	// round up mins/maxs to hull size and adjust the viewheight, if needed
	PM_AdjustViewheight();

	// set groundentity, watertype, and waterlevel
	PM_CategorizePosition();
	oldGroundEntity = pm->groundentity;

	PM_CheckSpecialMovement();

	if( pm->playerState->pmove.pm_flags & PMF_TIME_TELEPORT )
	{ // teleport pause stays exactly in place
	}
	else if( pm->playerState->pmove.pm_flags & PMF_TIME_WATERJUMP )
	{ // waterjump has no control, but falls
		pml.velocity[2] -= pm->playerState->pmove.gravity * pml.frametime;
		if( pml.velocity[2] < 0 )
		{ // cancel as soon as we are falling down again
			pm->playerState->pmove.pm_flags &= ~( PMF_TIME_WATERJUMP | PMF_TIME_LAND | PMF_TIME_TELEPORT );
			pm->playerState->pmove.pm_time = 0;
		}

		PM_StepSlideMove();
	}
	else
	{
		// Kurim
		// Keep this order !
		PM_CheckJump();
		PM_CheckDash();
		PM_CheckWallJump();

		PM_Friction();

		if( pm->waterlevel >= 2 )
		{
			PM_WaterMove();
		}
		else
		{
			vec3_t angles;

			VectorCopy( pm->playerState->viewangles, angles );
			if( angles[PITCH] > 180 )
				angles[PITCH] = angles[PITCH] - 360;
			angles[PITCH] /= 3;

			AngleVectors( angles, pml.forward, pml.right, pml.up );

			// hack to work when looking straight up and straight down
			if( pml.forward[2] == -1.0f )
			{
				VectorCopy( pml.up, pml.flatforward );
			}
			else if( pml.forward[2] == 1.0f )
			{
				VectorCopy( pml.up, pml.flatforward );
				VectorNegate( pml.flatforward, pml.flatforward );
			}
			else
			{
				VectorCopy( pml.forward, pml.flatforward );
			}
			pml.flatforward[2] = 0.0f;
			VectorNormalize( pml.flatforward );

			PM_Move();
		}
	}

	// set groundentity, watertype, and waterlevel for final spot
	PM_CategorizePosition();
	PM_SnapPosition();

	// falling event

#define FALL_DAMAGE_MIN_DELTA 675
#define FALL_STEP_MIN_DELTA 400
#define MAX_FALLING_DAMAGE 15
#define FALL_DAMAGE_SCALE 1.0

	// check for falling damage
	module_PMoveTouchTriggers( pm );

	PM_UpdateDeltaAngles(); // in case some trigger action has moved the view angles (like teleported).

	// touching triggers may force groundentity off
	if( !( pm->playerState->pmove.pm_flags & PMF_ON_GROUND ) && pm->groundentity != -1 )
	{
		pm->groundentity = -1;
		pml.velocity[2] = 0;
	}

	if( pm->groundentity != -1 ) // remove wall-jump and dash bits when touching ground
	{
		// always keep the dash flag 50 msecs at least (to prevent being removed at the start of the dash)
		if( pm->playerState->pmove.stats[PM_STAT_DASHTIME] < ( PM_DASHJUMP_TIMEDELAY - 50 ) )
			pm->playerState->pmove.pm_flags &= ~PMF_DASHING;

		if( pm->playerState->pmove.stats[PM_STAT_WJTIME] < ( PM_WALLJUMP_TIMEDELAY - 50 ) )
			PM_ClearWallJump();
	}

	if( oldGroundEntity == -1 )
	{
		falldelta = fallvelocity - ( ( pml.velocity[2] < 0.0f ) ? abs( pml.velocity[2] ) : 0.0f );

		// scale delta if in water
		if( pm->waterlevel == 3 )
			falldelta = 0;
		if( pm->waterlevel == 2 )
			falldelta *= 0.25;
		if( pm->waterlevel == 1 )
			falldelta *= 0.5;

		if( falldelta > FALL_STEP_MIN_DELTA )
		{
			if( !GS_FallDamage() || ( pml.groundsurfFlags & SURF_NODAMAGE ) || ( pm->playerState->pmove.pm_flags & PMF_JUMPPAD_TIME ) )
				damage = 0;
			else
			{
				damage = ( ( falldelta - FALL_DAMAGE_MIN_DELTA ) / 10 ) * FALL_DAMAGE_SCALE;
				clamp( damage, 0.0f, MAX_FALLING_DAMAGE );
			}

			module_PredictedEvent( pm->playerState->POVnum, EV_FALL, damage );
		}

		pm->playerState->pmove.pm_flags &= ~PMF_JUMPPAD_TIME;
	}
}
