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

#include "../g_local.h"
#include "ai_local.h"

/*
* AI_DropNodeOriginToFloor
*/
qboolean AI_DropNodeOriginToFloor( vec3_t origin, edict_t *passent )
{
	trace_t	trace;

	G_Trace( &trace, origin, tv( item_box_mins[0], item_box_mins[1], 0 ), tv( item_box_maxs[0], item_box_maxs[1], 0 ), tv( origin[0], origin[1], world->r.mins[2] ), passent, MASK_NODESOLID );
	if( trace.startsolid )
		return qfalse;

	origin[0] = trace.endpos[0];
	origin[1] = trace.endpos[1];
	origin[2] = trace.endpos[2] + 2 + abs( playerbox_stand_mins[2] );

	return qtrue;
}

qboolean AI_visible( edict_t *self, edict_t *other )
{
	vec3_t spot1;
	vec3_t spot2;
	trace_t	trace;

	VectorCopy( self->s.origin, spot1 );
	spot1[2] += self->viewheight;
	VectorCopy( other->s.origin, spot2 );
	spot2[2] += other->viewheight;
	G_Trace( &trace, spot1, vec3_origin, vec3_origin, spot2, self, MASK_OPAQUE );

	if( trace.fraction == 1.0 )
		return qtrue;
	return qfalse;
}

qboolean AI_infront( edict_t *self, edict_t *other )
{
	vec3_t vec;
	float dot;
	vec3_t forward;

	AngleVectors( self->s.angles, forward, NULL, NULL );
	VectorSubtract( other->s.origin, self->s.origin, vec );
	VectorNormalizeFast( vec );
	dot = DotProduct( vec, forward );

	if( dot > 0.3 )
		return qtrue;
	return qfalse;
}

qboolean AI_infront2D( vec3_t lookDir, vec3_t origin, vec3_t point, float accuracy )
{
	vec3_t vec;
	float dot;
	vec3_t origin2D, point2D, lookDir2D;

	VectorSet( origin2D, origin[0], origin[1], 0 );
	VectorSet( point2D, point[0], point[1], 0 );
	VectorSet( lookDir2D, lookDir[0], lookDir[1], 0 );
	VectorNormalizeFast( lookDir2D );

	VectorSubtract( point2D, origin2D, vec );
	VectorNormalizeFast( vec );
	dot = DotProduct( vec, lookDir2D );

	clamp( accuracy, -1, 1 );

	return ( dot > accuracy );
}

void AI_NewEnemyInView( edict_t *self, edict_t *enemy )
{
	if( enemy == self )
		return;

	self->ai.latched_enemy = enemy;
	self->ai.enemyReactionDelay = ( 50 + ( AI_REACTION_TIME * ( 1.0f - self->ai.pers.skillLevel ) ) );
}

unsigned int AI_CurrentLinkType( edict_t *self )
{
	if( !AI_PlinkExists( self->ai.current_node, self->ai.next_node ) )
		return LINK_INVALID;

	return AI_PlinkMoveType( self->ai.current_node, self->ai.next_node );
}
