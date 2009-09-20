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

typedef struct
{
	vec3_t origin;          // full float precision
	vec3_t velocity;        // full float precision

	vec3_t forward, right, up;
	vec3_t flatforward;     // normalized forward without z component, saved here because it needs
	                        // special handling for looking straight up or down
	float frametime;
	float maxPlayerSpeed;

	vec3_t previous_origin;

	float forwardPush, sidePush, upPush;
} pml_t;

pmove_t	*pm;
pml_t pml;

vec3_t playerbox_stand_mins = { -16, -16, -24 };
vec3_t playerbox_stand_maxs = { 16, 16, 40 };
int playerbox_stand_viewheight = 30;

// movement parameters

#define DEFAULT_PLAYERSPEED 320.0f

const float pm_friction = 8; // initially 6

const float pm_accelerate = 12; // user intended acceleration when on ground or fly movement ( initially 10 )
const float pm_decelerate = 12; // user intended deceleration when on ground

#define SPEEDKEY    500

//================
//TVM_PM_ClampAngles
//================
static void TVM_PM_FlyMove( void )
{
	float speed, drop, friction, control, newspeed;
	float currentspeed, addspeed, accelspeed, maxspeed;
	int i;
	vec3_t wishvel;
	float fmove, smove;
	vec3_t wishdir;
	float wishspeed;

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

		friction = pm_friction*1.5; // extra friction
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
			pml.velocity[i] += accelspeed * wishdir[i];
	}

	// move
	VectorMA( pml.origin, pml.frametime, pml.velocity, pml.origin );
}

//================
//TVM_PM_ClampAngles
//================
#if defined ( _WIN32 ) && ( _MSC_VER >= 1400 )
#pragma warning( push )
#pragma warning( disable : 4310 )   // cast truncates constant value
#endif
static void TVM_PM_ClampAngles( void )
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
//TVM_PM_SnapPosition
//
//On exit, the origin will have a value that is pre-quantized to the (1.0/16.0)
//precision of the network channel and in a valid position.
//================
static void TVM_PM_SnapPosition( void )
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

		//		if( TVM_PM_GoodPosition(origint) ) {
		if( 1 )
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
//TVM_Pmove
//
//Can be called by either the server or the client
//================
void TVM_Pmove( pmove_t *pmove )
{
	pm = pmove;

	if( !pmove->playerState )
		return;

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

	// save old org in case we get stuck
	VectorCopy( pm->playerState->pmove.origin, pml.previous_origin );

	pml.frametime = pm->cmd.msec * 0.001;

	pml.maxPlayerSpeed = pm->playerState->pmove.stats[PM_STAT_MAXSPEED];
	if( pml.maxPlayerSpeed < 0 )
		pml.maxPlayerSpeed = DEFAULT_PLAYERSPEED;

	// drop timing counters
	if( pm->playerState->pmove.pm_time )
	{
		int msec;

		msec = pm->cmd.msec >> 3;
		if( !msec )
			msec = 1;
		if( msec >= pm->playerState->pmove.pm_time )
		{
			//pm->playerState->pmove.pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND | PMF_TIME_TELEPORT);
			pm->playerState->pmove.pm_time = 0;
		}
		else
			pm->playerState->pmove.pm_time -= msec;
	}

	if( pm->playerState->pmove.stats[PM_STAT_NOUSERCONTROL] > 0 )
		pm->playerState->pmove.stats[PM_STAT_NOUSERCONTROL] -= pm->cmd.msec;
	if( pm->playerState->pmove.stats[PM_STAT_KNOCKBACK] > 0 )
		pm->playerState->pmove.stats[PM_STAT_KNOCKBACK] -= pm->cmd.msec;
	if( pm->playerState->pmove.stats[PM_STAT_DASHTIME] > 0 )
		pm->playerState->pmove.stats[PM_STAT_DASHTIME] -= pm->cmd.msec;
	if( pm->playerState->pmove.stats[PM_STAT_WJTIME] > 0 )
		pm->playerState->pmove.stats[PM_STAT_WJTIME] -= pm->cmd.msec;

	pm->playerState->viewheight = playerbox_stand_viewheight;

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

	TVM_PM_ClampAngles();

	TVM_PM_FlyMove();
	TVM_PM_SnapPosition();
}
