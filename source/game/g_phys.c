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
// g_phys.c

#include "g_local.h"

//================================================================================

/*
* SV_AddGravity
* 
*/
static void SV_AddGravity( edict_t *ent )
{
	ent->velocity[2] -= ent->gravity * g_gravity->value * FRAMETIME;
}
/*
typedef struct
{
vec3_t velocity;
vec3_t origin;
vec3_t mins, maxs;
float remainingTime;

vec3_t gravityDir;
float slideBounce;
int groundEntity;

int passent, contentmask;

int numClipPlanes;
vec3_t clipPlaneNormals[MAX_SLIDEMOVE_CLIP_PLANES];

int numtouch;
int touchents[MAXTOUCH];
} move_t;
*/

void G_AddGroundFriction( edict_t *ent, float friction )
{
	vec3_t v, frictionVec;
	float speed, fspeed;

	VectorSet( v, ent->velocity[0], ent->velocity[1], 0 );
	speed = VectorNormalize2( v, frictionVec );
	if( speed ) 
	{
		fspeed = friction * FRAMETIME;
		if( fspeed > speed )
			fspeed = speed;

		VectorMA( ent->velocity, -fspeed, frictionVec, ent->velocity );
	}
}

/*
* G_BoxSlideMove
* calls GS_SlideMove for edict_t and triggers touch functions of touched objects
*/
int G_BoxSlideMove( edict_t *ent, int contentmask, float slideBounce, float friction )
{
	int i;
	move_t entMove;
	int blockedmask = 0;
	float oldVelocity;
	memset( &entMove, 0, sizeof( move_t ) );

	oldVelocity = VectorLength( ent->velocity );
	if( !ent->groundentity )
		SV_AddGravity( ent );
	else // horizontal friction
		G_AddGroundFriction( ent, friction );

	entMove.numClipPlanes = 0;
	entMove.numtouch = 0;

	if( oldVelocity > 0 )
	{
		VectorCopy( ent->s.origin, entMove.origin );
		VectorCopy( ent->velocity, entMove.velocity );
		VectorCopy( ent->r.mins, entMove.mins );
		VectorCopy( ent->r.maxs, entMove.maxs );

		entMove.remainingTime = FRAMETIME;

		VectorSet( entMove.gravityDir, 0, 0, -1 );
		entMove.slideBounce = slideBounce;
		entMove.groundEntity = ( ent->groundentity == NULL ) ? -1 : ENTNUM( ent->groundentity );

		entMove.passent = ENTNUM( ent );
		entMove.contentmask = contentmask;

		blockedmask = GS_SlideMove( &entMove );

		// update with the new values
		VectorCopy( entMove.origin, ent->s.origin );
		VectorCopy( entMove.velocity, ent->velocity );
		ent->groundentity = ( entMove.groundEntity == -1 ) ? NULL : &game.edicts[entMove.groundEntity];

		GClip_LinkEntity( ent );
	}

	// call touches
	if( contentmask != 0 )
	{
		edict_t *other;
		GClip_TouchTriggers( ent );

		// touch other objects
		for( i = 0; i < entMove.numtouch; i++ )
		{
			other = &game.edicts[entMove.touchents[i]];
			if( other->r.svflags & SVF_PROJECTILE )
				continue;

			G_CallTouch( other, ent, NULL, 0 );

			// if self touch function, fire up touch and if freed stop
			G_CallTouch( ent, other, NULL, 0 );
			if( !ent->r.inuse )  // it may have been freed by the touch function
				break;
		}
	}

	if( ent->r.inuse )
	{
		G_CheckGround( ent );
		if( ent->groundentity && VectorLength( ent->velocity ) <= 1 && oldVelocity > 1 )
		{
			VectorClear( ent->velocity );
			VectorClear( ent->avelocity );
			G_CallStop( ent );
		}
	}

	return blockedmask;
}



//================================================================================

//pushmove objects do not obey gravity, and do not interact with each other or trigger fields, but block normal movement and push normal objects when they move.
//
//onground is set for toss objects when they come to a complete rest.  it is set for steping or walking objects
//
//doors, plats, etc are SOLID_BSP, and MOVETYPE_PUSH
//bonus items are SOLID_TRIGGER touch, and MOVETYPE_TOSS
//corpses are SOLID_NOT and MOVETYPE_TOSS
//crates are SOLID_BBOX and MOVETYPE_TOSS
//walking monsters are SOLID_SLIDEBOX and MOVETYPE_STEP
//flying/floating monsters are SOLID_SLIDEBOX and MOVETYPE_FLY
//
//solid_edge items only clip against bsp models.



/*
* SV_TestEntityPosition
* 
*/
static edict_t *SV_TestEntityPosition( edict_t *ent )
{
	trace_t	trace;
	int mask;


	if( ent->r.clipmask )
		mask = ent->r.clipmask;
	else
		mask = MASK_SOLID;

	G_Trace4D( &trace, ent->s.origin, ent->r.mins, ent->r.maxs, ent->s.origin, ent, mask, ent->timeDelta );
	if( trace.startsolid )
		return game.edicts;

	return NULL;
}

/*
* SV_CheckVelocity
*/
static void SV_CheckVelocity( edict_t *ent )
{
	float scale;

	//
	// bound velocity
	//
	scale = VectorLength( ent->velocity );
	if( ( scale > g_maxvelocity->value ) && ( scale ) )
	{
		scale = g_maxvelocity->value / scale;
		VectorScale( ent->velocity, scale, ent->velocity );
	}
}

/*
* SV_RunThink
* 
* Runs thinking code for this frame if necessary
*/
static void SV_RunThink( edict_t *ent )
{
	unsigned int thinktime;

	thinktime = ent->nextThink;
	if( thinktime <= 0 )
		return;
	if( thinktime > level.time )
		return;

	ent->nextThink = 0;

	if( ISEVENTENTITY( &ent->s ) )  // events do not think
		return;

	G_CallThink( ent );
}

/*
* SV_Impact
* 
* Two entities have touched, so run their touch functions
*/
void SV_Impact( edict_t *e1, trace_t *trace )
{
	edict_t	*e2;

	if( trace->ent != -1 )
	{
		e2 = &game.edicts[trace->ent];

		if( e1->r.solid != SOLID_NOT )
			G_CallTouch( e1, e2, &trace->plane, trace->surfFlags );

		if( e2->r.solid != SOLID_NOT )
			G_CallTouch( e2, e1, NULL, 0 );
	}
}

/*
* SV_FlyMove
* 
* The basic solid body movement clip that slides along multiple planes
* Returns the clipflags if the velocity was modified (hit something solid)
* 1 = floor
* 2 = wall / step
* 4 = dead stop
*/
#if 0
#define	MAX_CLIP_PLANES	5
int SV_FlyMove( edict_t *ent, float time, int mask )
{
	edict_t	*hit;
	int bumpcount, numbumps;
	vec3_t dir;
	float d;
	int numplanes;
	vec3_t planes[MAX_CLIP_PLANES];
	vec3_t primal_velocity, original_velocity, new_velocity;
	int i, j;
	trace_t	trace;
	vec3_t end;
	float time_left;
	int blocked;

	numbumps = 4;

	blocked = 0;
	VectorCopy( ent->velocity, original_velocity );
	VectorCopy( ent->velocity, primal_velocity );
	numplanes = 0;

	time_left = time;

	ent->groundentity = NULL;
	for( bumpcount = 0; bumpcount < numbumps; bumpcount++ )
	{
		for( i = 0; i < 3; i++ )
			end[i] = ent->s.origin[i] + time_left * ent->velocity[i];

		G_Trace4D( &trace, ent->s.origin, ent->r.mins, ent->r.maxs, end, ent, mask, ent->timeDelta );
		if( trace.allsolid )
		{ // entity is trapped in another solid
			VectorClear( ent->velocity );
			return 3;
		}

		if( trace.fraction > 0 )
		{ // actually covered some distance
			VectorCopy( trace.endpos, ent->s.origin );
			VectorCopy( ent->velocity, original_velocity );
			numplanes = 0;
		}

		if( trace.fraction == 1 )
			break; // moved the entire distance

		hit = &game.edicts[trace.ent];

		if( ISWALKABLEPLANE( &trace.plane ) )
		{
			blocked |= 1; // floor
			if( hit->r.solid == SOLID_BSP )
			{
				ent->groundentity = hit;
				ent->groundentity_linkcount = hit->r.linkcount;
			}
		}
		if( !trace.plane.normal[2] )
		{
			blocked |= 2; // step
		}

		//
		// run the impact function
		//
		SV_Impact( ent, &trace );
		if( !ent->r.inuse )
			break; // removed by the impact function

		time_left -= time_left * trace.fraction;

		// cliped to another plane
		if( numplanes >= MAX_CLIP_PLANES )
		{ // this shouldn't really happen
			VectorClear( ent->velocity );
			return 3;
		}

		VectorCopy( trace.plane.normal, planes[numplanes] );
		numplanes++;

		//
		// modify original_velocity so it parallels all of the clip planes
		//
		for( i = 0; i < numplanes; i++ )
		{
			GS_ClipVelocity( original_velocity, planes[i], new_velocity, 1 );
			for( j = 0; j < numplanes; j++ )
				if( j != i )
				{
					if( DotProduct( new_velocity, planes[j] ) < 0 )
						break; // not ok
				}
				if( j == numplanes )
					break;
		}

		if( i != numplanes )
		{               // go along this plane
			VectorCopy( new_velocity, ent->velocity );
		}
		else
		{ // go along the crease
			if( numplanes != 2 )
			{
				VectorClear( ent->velocity );
				return 7;
			}
			CrossProduct( planes[0], planes[1], dir );
			VectorNormalize( dir );
			d = DotProduct( dir, ent->velocity );
			VectorScale( dir, d, ent->velocity );
		}

		//
		// if original velocity is against the original velocity, stop dead
		// to avoid tiny occilations in sloping corners
		//
		if( DotProduct( ent->velocity, primal_velocity ) <= 0 )
		{
			VectorClear( ent->velocity );
			return blocked;
		}
	}

	return blocked;
}
#endif

//===============================================================================
//
//PUSHMOVE
//
//===============================================================================


/*
* SV_PushEntity
* 
* Does not change the entities velocity at all
*/
static trace_t SV_PushEntity( edict_t *ent, vec3_t push )
{
	trace_t	trace;
	vec3_t start;
	vec3_t end;
	int mask;

	GS_SnapVelocity( push );
	VectorCopy( ent->s.origin, start );
	VectorAdd( start, push, end );

retry:
	if( ent->r.clipmask )
		mask = ent->r.clipmask;
	else
		mask = MASK_SOLID;

	G_Trace4D( &trace, start, ent->r.mins, ent->r.maxs, end, ent, mask, ent->timeDelta );
	if( ent->movetype == MOVETYPE_PUSH || !trace.startsolid )
		VectorCopy( trace.endpos, ent->s.origin );

	GClip_LinkEntity( ent );

	if( trace.fraction < 1.0 )
	{
		SV_Impact( ent, &trace );

		// if the pushed entity went away and the pusher is still there
		if( !game.edicts[trace.ent].r.inuse && ent->movetype == MOVETYPE_PUSH && ent->r.inuse )
		{
			// move the pusher back and try again
			VectorCopy( start, ent->s.origin );
			GClip_LinkEntity( ent );
			goto retry;
		}
	}

	if( ent->r.inuse )
		GClip_TouchTriggers( ent );

	return trace;
}


typedef struct
{
	edict_t	*ent;
	vec3_t origin;
	vec3_t angles;
	float deltayaw;
} pushed_t;
pushed_t pushed[MAX_EDICTS], *pushed_p;

edict_t	*obstacle;


/*
* SV_Push
* 
* Objects need to be moved back on a failed push,
* otherwise riders would continue to slide.
*/
static qboolean SV_Push( edict_t *pusher, vec3_t move, vec3_t amove )
{
	int i, e;
	edict_t	*check, *block;
	vec3_t mins, maxs;
	pushed_t *p;
	vec3_t axis[3];
	vec3_t org, org2, move2;

	// clamp the move so the position will
	// be accurate for client side prediction
	GS_SnapVelocity( move );

	// find the bounding box
	for( i = 0; i < 3; i++ )
	{
		mins[i] = pusher->r.absmin[i] + move[i];
		maxs[i] = pusher->r.absmax[i] + move[i];
	}

	// we need this for pushing things later
	VectorNegate( amove, org );
	AnglesToAxis( org, axis );

	// save the pusher's original position
	pushed_p->ent = pusher;
	VectorCopy( pusher->s.origin, pushed_p->origin );
	VectorCopy( pusher->s.angles, pushed_p->angles );
	if( pusher->r.client )
		pushed_p->deltayaw = pusher->r.client->ps.pmove.delta_angles[YAW];
	pushed_p++;

	// move the pusher to its final position
	VectorAdd( pusher->s.origin, move, pusher->s.origin );
	VectorAdd( pusher->s.angles, amove, pusher->s.angles );
	GClip_LinkEntity( pusher );

	// see if any solid entities are inside the final position
	check = game.edicts + 1;
	for( e = 1; e < game.numentities; e++, check++ )
	{
		if( !check->r.inuse )
			continue;
		if( check->movetype == MOVETYPE_PUSH
			|| check->movetype == MOVETYPE_STOP
			|| check->movetype == MOVETYPE_NONE
			|| check->movetype == MOVETYPE_NOCLIP )
			continue;

		if( !check->r.area.prev )
			continue; // not linked in anywhere

		// if the entity is standing on the pusher, it will definitely be moved
		if( check->groundentity != pusher )
		{
			// see if the ent needs to be tested
			if( check->r.absmin[0] >= maxs[0]
			|| check->r.absmin[1] >= maxs[1]
			|| check->r.absmin[2] >= maxs[2]
			|| check->r.absmax[0] <= mins[0]
			|| check->r.absmax[1] <= mins[1]
			|| check->r.absmax[2] <= mins[2] )
				continue;

			// see if the ent's bbox is inside the pusher's final position
			if( !SV_TestEntityPosition( check ) )
				continue;
		}

		if( ( pusher->movetype == MOVETYPE_PUSH ) || ( check->groundentity == pusher ) )
		{
			// move this entity
			pushed_p->ent = check;
			VectorCopy( check->s.origin, pushed_p->origin );
			VectorCopy( check->s.angles, pushed_p->angles );
			pushed_p++;

			// try moving the contacted entity
			VectorAdd( check->s.origin, move, check->s.origin );
			if( check->r.client )
			{ // FIXME: doesn't rotate monsters?
				check->r.client->ps.pmove.delta_angles[YAW] += amove[YAW];
			}

			// figure movement due to the pusher's amove
			VectorSubtract( check->s.origin, pusher->s.origin, org );
			Matrix_TransformVector( axis, org, org2 );
			VectorSubtract( org2, org, move2 );
			VectorAdd( check->s.origin, move2, check->s.origin );

			if( check->movetype != MOVETYPE_BOUNCEGRENADE )
			{
				// may have pushed them off an edge
				if( check->groundentity != pusher )
					check->groundentity = NULL;
			}

			block = SV_TestEntityPosition( check );
			if( !block )
			{
				// pushed ok
				GClip_LinkEntity( check );
				// impact?
				continue;
			}
			else
			{
				// try to fix block
				// if it is ok to leave in the old position, do it
				// this is only relevant for riding entities, not pushed
				VectorSubtract( check->s.origin, move, check->s.origin );
				VectorSubtract( check->s.origin, move2, check->s.origin );
				block = SV_TestEntityPosition( check );
				if( !block )
				{
					pushed_p--;
					continue;
				}
			}
		}

		// save off the obstacle so we can call the block function
		obstacle = check;

		// move back any entities we already moved
		// go backwards, so if the same entity was pushed
		// twice, it goes back to the original position
		for( p = pushed_p-1; p >= pushed; p-- )
		{
			VectorCopy( p->origin, p->ent->s.origin );
			VectorCopy( p->angles, p->ent->s.angles );
			if( p->ent->r.client )
			{
				p->ent->r.client->ps.pmove.delta_angles[YAW] = p->deltayaw;
			}
			GClip_LinkEntity( p->ent );
		}
		return qfalse;
	}

	//FIXME: is there a better way to handle this?
	// see if anything we moved has touched a trigger
	for( p = pushed_p-1; p >= pushed; p-- )
		GClip_TouchTriggers( p->ent );

	return qtrue;
}

/*
* SV_Physics_Pusher
* 
* Bmodel objects don't interact with each other, but
* push all box objects
*/
static void SV_Physics_Pusher( edict_t *ent )
{
	vec3_t move, amove;
	edict_t	*part, *mover;

	// if not a team captain, so movement will be handled elsewhere
	if( ent->flags & FL_TEAMSLAVE )
		return;

	// make sure all team slaves can move before commiting
	// any moves or calling any think functions
	// if the move is blocked, all moved objects will be backed out
	//retry:
	pushed_p = pushed;
	for( part = ent; part; part = part->teamchain )
	{
		if( part->velocity[0] || part->velocity[1] || part->velocity[2] ||
			part->avelocity[0] || part->avelocity[1] || part->avelocity[2] )
		{
			// object is moving
			VectorScale( part->velocity, FRAMETIME, move );
			VectorScale( part->avelocity, FRAMETIME, amove );

			if( !SV_Push( part, move, amove ) )
				break; // move was blocked
		}
	}
	if( pushed_p > &pushed[MAX_EDICTS] )
		G_Error( "pushed_p > &pushed[MAX_EDICTS], memory corrupted" );

	if( part )
	{
		// the move failed, bump all nextthink times and back out moves
		for( mover = ent; mover; mover = mover->teamchain )
		{
			if( mover->nextThink > 0 )
				mover->nextThink += game.frametime;
		}

		// if the pusher has a "blocked" function, call it
		// otherwise, just stay in place until the obstacle is gone
		if( part->moveinfo.blocked )
			part->moveinfo.blocked( part, obstacle );
#if 0
		// if the obstacle went away and the pusher is still there
		if( !obstacle->r.inuse && part->r.inuse )
			goto retry;
#endif
	}
}

//==================================================================

/*
* SV_Physics_None
* only think
*/
static void SV_Physics_None( edict_t *ent )
{
}

/*
* SV_Physics_Noclip
* 
* A moving object that doesn't obey physics
*/
#if 0
static void SV_Physics_Noclip( edict_t *ent )
{
	VectorMA( ent->s.angles, FRAMETIME, ent->avelocity, ent->s.angles );
	VectorMA( ent->s.origin, FRAMETIME, ent->velocity, ent->s.origin );

	GClip_LinkEntity( ent );
}
#endif

//==============================================================================
//
//TOSS / BOUNCE
//
//==============================================================================

/*
* SV_Physics_Toss
* 
* Toss, bounce, and fly movement.  When onground, do nothing.
* 
* FIXME: This function needs a serious rewrite
*/
static void SV_Physics_Toss( edict_t *ent )
{
	trace_t	trace;
	vec3_t move;
	float backoff;
	edict_t	*slave;
	qboolean wasinwater;
	qboolean isinwater;
	vec3_t old_origin;
	float oldSpeed;

	// if not a team captain, so movement will be handled elsewhere
	if( ent->flags & FL_TEAMSLAVE )
		return;

	// refresh the ground entity
	if( ent->movetype == MOVETYPE_BOUNCE || ent->movetype == MOVETYPE_BOUNCEGRENADE )
	{
		if( ent->velocity[2] > 0.1f )
			ent->groundentity = NULL;
	}

	if( ent->groundentity && ent->groundentity != world && !ent->groundentity->r.inuse )
		ent->groundentity = NULL;

	oldSpeed = VectorLength( ent->velocity );

	if( ent->groundentity )
	{
		if( !oldSpeed )
			return;

		if( ent->movetype == MOVETYPE_TOSS )
		{
			if( ent->velocity[2] >= 8 )
				ent->groundentity = NULL;
			else
			{
				VectorClear( ent->velocity );
				VectorClear( ent->avelocity );
				G_CallStop( ent );
				return;
			}
		}
	}

	VectorCopy( ent->s.origin, old_origin );

	if( ent->accel != 0 )
	{
		if( ent->accel < 0 && VectorLength( ent->velocity ) < 50 )
		{
			VectorClear( ent->velocity );
		}
		else
		{
			vec3_t acceldir;
			VectorNormalize2( ent->velocity, acceldir );
			VectorScale( acceldir, ent->accel * FRAMETIME, acceldir );
			VectorAdd( ent->velocity, acceldir, ent->velocity );
		}
	}

	SV_CheckVelocity( ent );

	// add gravity
	if( ent->movetype != MOVETYPE_FLY && !ent->groundentity )
		SV_AddGravity( ent );

	// move angles
	VectorMA( ent->s.angles, FRAMETIME, ent->avelocity, ent->s.angles );

	// move origin
	VectorScale( ent->velocity, FRAMETIME, move );

	trace = SV_PushEntity( ent, move );
	if( !ent->r.inuse )
		return;

	GS_SnapPosition( ent->s.origin, ent->r.mins, ent->r.maxs, ENTNUM( ent ), ent->r.clipmask ? ent->r.clipmask : MASK_SOLID );

	if( trace.fraction < 1.0f )
	{
		if( ent->movetype == MOVETYPE_BOUNCE )
			backoff = 1.5;
		else if( ent->movetype == MOVETYPE_BOUNCEGRENADE )
		{
			static cvar_t *g_grenade_backoff = NULL;
			if( !g_grenade_backoff )
				g_grenade_backoff = trap_Cvar_Get( "g_grenade_backoff", "1.5", CVAR_DEVELOPER );
			backoff = g_grenade_backoff->value;
		}
		else
			backoff = 1;

		GS_ClipVelocity( ent->velocity, trace.plane.normal, ent->velocity, backoff );

		// stop if on ground

		if( ent->movetype == MOVETYPE_BOUNCE || ent->movetype == MOVETYPE_BOUNCEGRENADE )
		{
			// stop dead on allsolid

			// LA: hopefully will fix grenades bouncing down slopes
			// method taken from Darkplaces sourcecode
			if( trace.allsolid || 
				( ISWALKABLEPLANE( &trace.plane ) && 
					fabs( DotProduct( trace.plane.normal, ent->velocity ) ) < 60 
				)
			)
			{
				ent->groundentity = &game.edicts[trace.ent];
				ent->groundentity_linkcount = ent->groundentity->r.linkcount;
				VectorClear( ent->velocity );
				VectorClear( ent->avelocity );
				G_CallStop( ent );
			}
		}
		else
		{
			// in movetype_toss things stop dead when touching ground
#if 0
			G_CheckGround( ent );

			if( ent->groundentity )
			{
#else
			// walkable or trapped inside solid brush
			if( trace.allsolid || ISWALKABLEPLANE( &trace.plane ) )
			{
				ent->groundentity = trace.ent < 0 ? world : &game.edicts[trace.ent];
				ent->groundentity_linkcount = ent->groundentity->r.linkcount;
#endif
				VectorClear( ent->velocity );
				VectorClear( ent->avelocity );
				G_CallStop( ent );
			}
		}
	}

	GS_SnapVelocity( ent->velocity );

	// check for water transition
	wasinwater = ( ent->watertype & MASK_WATER );
	ent->watertype = G_PointContents( ent->s.origin );
	isinwater = ent->watertype & MASK_WATER;

	// never allow items in CONTENTS_NODROP
	if( ent->item && ( ent->watertype & CONTENTS_NODROP ) )
	{
		G_FreeEdict( ent );
		return;
	}

	if( isinwater )
		ent->waterlevel = 1;
	else
		ent->waterlevel = 0;

	if( !wasinwater && isinwater )
		G_PositionedSound( old_origin, CHAN_AUTO, trap_SoundIndex( S_HIT_WATER ), ATTN_IDLE );
	else if( wasinwater && !isinwater )
		G_PositionedSound( ent->s.origin, CHAN_AUTO, trap_SoundIndex( S_HIT_WATER ), ATTN_IDLE );

	// move teamslaves
	for( slave = ent->teamchain; slave; slave = slave->teamchain )
	{
		VectorCopy( ent->s.origin, slave->s.origin );
		GClip_LinkEntity( slave );
	}
}

//============================================================================

void SV_Physics_LinearProjectile( edict_t *ent )
{
	vec3_t start, end;
	int mask;
	float flyTime;
	trace_t	trace;
	int old_waterLevel;

	// if not a team captain movement will be handled elsewhere
	if( ent->flags & FL_TEAMSLAVE )
		return;

	old_waterLevel = ent->waterlevel;

	mask = ( ent->r.clipmask ) ? ent->r.clipmask : MASK_SOLID;

	// find it's current position given the starting timeStamp
	flyTime = (float)( game.serverTime - ent->s.linearProjectileTimeStamp ) * 0.001f;

	VectorCopy( ent->s.origin, start );
	VectorMA( ent->s.origin2, flyTime, ent->velocity, end );

	G_Trace4D( &trace, start, ent->r.mins, ent->r.maxs, end, ent, mask, ent->timeDelta );
	VectorCopy( trace.endpos, ent->s.origin );
	GClip_LinkEntity( ent );
	SV_Impact( ent, &trace );

	if( !ent->r.inuse )  // the projectile may be freed if touched something
		return;

	// update some data required for the transmission
	VectorCopy( ent->velocity, ent->s.linearProjectileVelocity );

	GClip_TouchTriggers( ent );
	ent->groundentity = NULL; // projectiles never have ground entity
	ent->waterlevel = ( G_PointContents4D( ent->s.origin, ent->timeDelta ) & MASK_WATER ) ? qtrue : qfalse;

	if( !old_waterLevel && ent->waterlevel )
		G_PositionedSound( start, CHAN_AUTO, trap_SoundIndex( S_HIT_WATER ), ATTN_IDLE );
	else if( old_waterLevel && !ent->waterlevel )
		G_PositionedSound( ent->s.origin, CHAN_AUTO, trap_SoundIndex( S_HIT_WATER ), ATTN_IDLE );
}

//============================================================================

/*
* G_RunEntity
* 
*/
void G_RunEntity( edict_t *ent )
{
	edict_t	*part;

	if( !level.canSpawnEntities ) // don't try to think before map entities are spawned
		return;

	if( ISEVENTENTITY( &ent->s ) )  // events do not think
		return;

	if( ent->timeDelta && !( ent->r.svflags & SVF_PROJECTILE ) )
	{
		G_Printf( "Warning: G_RunEntity 'Fixing timeDelta on non projectile entity\n" );
		ent->timeDelta = 0;
	}

	// only team captains decide the think, and they make think their team members when they do
	if( !( ent->flags & FL_TEAMSLAVE ) )
	{
		for( part = ent; part; part = part->teamchain )
		{
			SV_RunThink( part );
		}
	}

	if( ent->s.linearProjectile && ( ent->movetype != MOVETYPE_LINEARPROJECTILE ) )
	{
		G_Printf( "WARNING: G_RunEntity: fixing entity type %i not having MOVETYPE_LINEARPROJECTILE assigned\n" );
		ent->movetype = MOVETYPE_LINEARPROJECTILE;
	}

	switch( (int)ent->movetype )
	{
	case MOVETYPE_NONE:
	case MOVETYPE_NOCLIP: // only used for clients, that use pmove
		SV_Physics_None( ent );
		break;
	case MOVETYPE_PLAYER:
		SV_Physics_None( ent );
		break;
	case MOVETYPE_PUSH:
	case MOVETYPE_STOP:
		SV_Physics_Pusher( ent );
		break;
	case MOVETYPE_BOUNCE:
	case MOVETYPE_BOUNCEGRENADE:
		SV_Physics_Toss( ent );
		break;
	case MOVETYPE_TOSS:
		SV_Physics_Toss( ent );
		break;
	case MOVETYPE_FLY:
		SV_Physics_Toss( ent );
		break;
	case MOVETYPE_LINEARPROJECTILE:
		SV_Physics_LinearProjectile( ent );
		break;
	case MOVETYPE_TOSSSLIDE:
		G_BoxSlideMove( ent, ent->r.clipmask ? ent->r.clipmask : MASK_PLAYERSOLID, 1.01f, 10 );
		break;
	default:
		G_Error( "SV_Physics: bad movetype %i", (int)ent->movetype );
	}
}
