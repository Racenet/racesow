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
--------------------------------------------------------------
The ACE Bot is a product of Steve Yeager, and is available from
the ACE Bot homepage, at http://www.axionfx.com/ace.

This program is a modification of the ACE Bot, and is therefore
in NO WAY supported by Steve Yeager.
*/

#include "../g_local.h"
#include "ai_local.h"

//==========================================
// AI_CanMove
// Checks if bot can move (really just checking the ground)
// Also, this is not a real accurate check, but does a
// pretty good job and looks for lava/slime.
//==========================================
qboolean AI_CanMove( edict_t *self, int direction )
{
	vec3_t forward, right;
	vec3_t offset, start, end;
	vec3_t angles;
	trace_t tr;

	// Now check to see if move will move us off an edge
	VectorCopy( self->s.angles, angles );

	if( direction == BOT_MOVE_LEFT )
		angles[1] += 90;
	else if( direction == BOT_MOVE_RIGHT )
		angles[1] -= 90;
	else if( direction == BOT_MOVE_BACK )
		angles[1] -= 180;


	// Set up the vectors
	AngleVectors( angles, forward, right, NULL );

	VectorSet( offset, 36, 0, 24 );
	G_ProjectSource( self->s.origin, offset, forward, right, start );

	VectorSet( offset, 36, 0, -100 );
	G_ProjectSource( self->s.origin, offset, forward, right, end );

	G_Trace( &tr, start, NULL, NULL, end, self, MASK_AISOLID );

	if( tr.fraction == 1.0 || tr.contents & ( CONTENTS_LAVA|CONTENTS_SLIME ) )
		return qfalse;

	return qtrue; // yup, can move
}


//===================
//  AI_IsStep
//  Checks the floor one step below the player. Used to detect
//  if the player is really falling or just walking down a stair.
//===================
qboolean AI_IsStep( edict_t *ent )
{
	vec3_t point;
	trace_t	trace;

	//determine a point below
	point[0] = ent->s.origin[0];
	point[1] = ent->s.origin[1];
	point[2] = ent->s.origin[2] - ( 1.6*AI_STEPSIZE );

	//trace to point
	G_Trace( &trace, ent->s.origin, ent->r.mins, ent->r.maxs, point, ent, MASK_PLAYERSOLID );

	if( !ISWALKABLEPLANE( &trace.plane ) && !trace.startsolid )
		return qfalse;

	//found solid.
	return qtrue;
}


//==========================================
// AI_IsLadder
// check if entity is touching in front of a ladder
//==========================================
qboolean AI_IsLadder( vec3_t origin, vec3_t v_angle, vec3_t mins, vec3_t maxs, edict_t *passent )
{
	vec3_t spot;
	vec3_t flatforward, zforward;
	trace_t	trace;

	AngleVectors( v_angle, zforward, NULL, NULL );

	// check for ladder
	flatforward[0] = zforward[0];
	flatforward[1] = zforward[1];
	flatforward[2] = 0;
	VectorNormalize( flatforward );

	VectorMA( origin, 1, flatforward, spot );

	G_Trace( &trace, origin, mins, maxs, spot, passent, MASK_AISOLID );

	if( ( trace.fraction < 1 ) && ( trace.surfFlags & SURF_LADDER ) )
		return qtrue;

	return qfalse;
}


//==========================================
// AI_CheckEyes
// Helper for ACEMV_SpecialMove.
// Tries to turn when in front of obstacle
//==========================================
static qboolean AI_CheckEyes( edict_t *self, usercmd_t *ucmd )
{
	vec3_t forward, right;
	vec3_t leftstart, rightstart, focalpoint;
	vec3_t dir, offset;
	trace_t traceRight;
	trace_t traceLeft;

	// Get current angle and set up "eyes"
	VectorCopy( self->s.angles, dir );
	AngleVectors( dir, forward, right, NULL );

	if( !self->movetarget )
		VectorSet( offset, 200, 0, self->r.maxs[2]*0.5 ); // focalpoint
	else
		VectorSet( offset, 64, 0, self->r.maxs[2]*0.5 ); // wander focalpoint

	G_ProjectSource( self->s.origin, offset, forward, right, focalpoint );

	VectorSet( offset, 0, 18, self->r.maxs[2]*0.5 );
	G_ProjectSource( self->s.origin, offset, forward, right, leftstart );
	offset[1] -= 36;
	G_ProjectSource( self->s.origin, offset, forward, right, rightstart );

	G_Trace( &traceRight, rightstart, NULL, NULL, focalpoint, self, MASK_AISOLID );
	G_Trace( &traceLeft, leftstart, NULL, NULL, focalpoint, self, MASK_AISOLID );

	// Find the side with more open space and turn
	if( traceRight.fraction != 1 || traceLeft.fraction != 1 )
	{
		if( traceRight.fraction > traceLeft.fraction )
			self->s.angles[YAW] += ( 1.0 - traceLeft.fraction ) * 45.0;
		else
			self->s.angles[YAW] += -( 1.0 - traceRight.fraction ) * 45.0;

		ucmd->forwardmove = 1;
		return qtrue;
	}

	return qfalse;
}

//==========================================
// AI_SpecialMove
// Handle special cases of crouch/jump
// If the move is resolved here, this function returns qtrue.
//==========================================
qboolean AI_SpecialMove( edict_t *self, usercmd_t *ucmd )
{
	vec3_t forward;
	trace_t tr;
	vec3_t boxmins, boxmaxs, boxorigin;

	// Get current direction
	AngleVectors( tv( 0, self->s.angles[YAW], 0 ), forward, NULL, NULL );

	// make sure we are blocked
	VectorCopy( self->s.origin, boxorigin );
	VectorMA( boxorigin, 8, forward, boxorigin ); //move box by 8 to front
	G_Trace( &tr, self->s.origin, self->r.mins, self->r.maxs, boxorigin, self, MASK_AISOLID );
	if( !tr.startsolid && tr.fraction == 1.0 )  // not blocked
		return qfalse;

	//ramps
	if( ISWALKABLEPLANE( &tr.plane ) )
		return qfalse;

	if( self->ai.status.moveTypesMask & LINK_CROUCH || self->is_swim )
	{
		// crouch box
		VectorCopy( self->s.origin, boxorigin );
		VectorCopy( self->r.mins, boxmins );
		VectorCopy( self->r.maxs, boxmaxs );
		boxmaxs[2] = 14; // crouched size
		VectorMA( boxorigin, 8, forward, boxorigin ); // move box by 8 to front
		// see if blocked
		G_Trace( &tr, boxorigin, boxmins, boxmaxs, boxorigin, self, MASK_AISOLID );
		if( !tr.startsolid ) // can move by crouching
		{
			ucmd->forwardmove = 1;
			ucmd->upmove = -1;
			return qtrue;
		}
	}

	if( self->ai.status.moveTypesMask & LINK_JUMP && self->groundentity )
	{
		// jump box
		VectorCopy( self->s.origin, boxorigin );
		VectorCopy( self->r.mins, boxmins );
		VectorCopy( self->r.maxs, boxmaxs );
		VectorMA( boxorigin, 8, forward, boxorigin ); // move box by 8 to front
		//
		boxorigin[2] += ( boxmins[2] + AI_JUMPABLE_HEIGHT ); // put at bottom + jumpable height
		boxmaxs[2] = boxmaxs[2] - boxmins[2]; // total player box height in boxmaxs
		boxmins[2] = 0;

		G_Trace( &tr, boxorigin, boxmins, boxmaxs, boxorigin, self, MASK_AISOLID );
		if( !tr.startsolid ) // can move by jumping
		{
			ucmd->forwardmove = 1;
			ucmd->upmove = 1;

			return qtrue;
		}
	}

	// nothing worked, check for turning
	return AI_CheckEyes( self, ucmd );
}

int AI_ChangeAngle( edict_t *ent )
{
	float ideal_yaw;
	float ideal_pitch;
	float current_yaw;
	float current_pitch;
	float move, yawmove = 0;
	float speed;
	float speed_yaw, speed_pitch;
	vec3_t ideal_angle;
	edict_t *self = ent;

	// Normalize the move angle first
	VectorNormalize( ent->ai.move_vector );

	current_yaw = anglemod( ent->s.angles[YAW] );
	current_pitch = anglemod( ent->s.angles[PITCH] );

	VecToAngles( ent->ai.move_vector, ideal_angle );

	ideal_yaw = anglemod( ideal_angle[YAW] );
	ideal_pitch = anglemod( ideal_angle[PITCH] );

	speed_yaw = ent->ai.speed_yaw;
	speed_pitch = ent->ai.speed_pitch;

	// Yaw
	if( fabs( current_yaw - ideal_yaw ) < 10 )
	{
		speed_yaw *= 0.5;
	}
	if( fabs( current_pitch - ideal_pitch ) < 10 )
	{
		speed_pitch *= 0.5;
	}

	if( current_yaw != ideal_yaw )
	{
		yawmove = ideal_yaw - current_yaw;
		speed = ent->yaw_speed * FRAMETIME;
		if( ideal_yaw > current_yaw )
		{
			if( yawmove >= 180 )
				yawmove = yawmove - 360;
		}
		else
		{
			if( yawmove <= -180 )
				yawmove = yawmove + 360;
		}
		if( yawmove > 0 )
		{
			if( speed_yaw > speed )
				speed_yaw = speed;
			if( yawmove < 3 )
				speed_yaw += AI_YAW_ACCEL/4.0;
			else
				speed_yaw += AI_YAW_ACCEL;
		}
		else
		{
			if( speed_yaw < -speed )
				speed_yaw = -speed;

			if( yawmove > -3 )
				speed_yaw -= AI_YAW_ACCEL/4.0;
			else
				speed_yaw -= AI_YAW_ACCEL;
		}

		yawmove = speed_yaw;

		ent->s.angles[YAW] = anglemod( current_yaw + yawmove );
	}


	// Pitch
	if( current_pitch != ideal_pitch )
	{
		move = ideal_pitch - current_pitch;
		speed = ent->yaw_speed * FRAMETIME;
		if( ideal_pitch > current_pitch )
		{
			if( move >= 180 )
				move = move - 360;
		}
		else
		{
			if( move <= -180 )
				move = move + 360;
		}
		if( move > 0 )
		{
			if( speed_pitch > speed )
				speed_pitch = speed;
			if( move < 3 )
				speed_pitch += AI_YAW_ACCEL/4.0;
			else
				speed_pitch += AI_YAW_ACCEL;
		}
		else
		{
			if( speed_pitch < -speed )
				speed_pitch = -speed;
			if( move > -3 )
				speed_pitch -= AI_YAW_ACCEL/4.0;
			else
				speed_pitch -= AI_YAW_ACCEL;
		}
		move = speed_pitch;
		ent->s.angles[PITCH] = anglemod( current_pitch + move );
	}
	ent->ai.speed_yaw = speed_yaw;
	ent->ai.speed_pitch = speed_pitch;
	return yawmove > 0 ? 1 : -1;
}

/*
* AI_MoveToShortRangeGoalEntity
* A.K.A Item pick magnet
*/
qboolean AI_MoveToShortRangeGoalEntity( edict_t *self, usercmd_t *ucmd )
{
	if( !self->movetarget || !self->r.client )
		return qfalse;

	if( self->ai.goalEnt && ( self->ai.goalEnt->ent == self->movetarget )
		&& ( AI_GetNodeFlags( self->ai.goal_node ) & NODEFLAGS_ENTITYREACH ) )
	{
		// wait
		VectorSubtract( self->movetarget->s.origin, self->s.origin, self->ai.move_vector );
		if( VectorLength( self->ai.move_vector ) < 72 )
			ucmd->buttons |= BUTTON_WALK;

		if( BoundsIntersect( self->movetarget->r.absmin, self->movetarget->r.absmax, self->r.absmin, self->r.absmax ) )
		{
			ucmd->forwardmove = 0;
			ucmd->sidemove = 0;
			ucmd->upmove = 0;
			self->ai.node_timeout = 0;
			return qtrue;
		}
	}

	if( self->movetarget->r.solid == SOLID_NOT || DistanceFast( self->movetarget->s.origin, self->s.origin ) > AI_GOAL_SR_RADIUS + 72 )
	{
		self->movetarget = NULL;
		self->ai.shortRangeGoalTimeout = level.time;
		return qfalse;
	}

	// Force movement direction to reach the goal entity
	VectorSubtract( self->movetarget->s.origin, self->s.origin, self->ai.move_vector );

	return qtrue;
}

