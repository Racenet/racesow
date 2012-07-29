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

int AI_FindCost( int from, int to, int movetypes )
{
	astarpath_t path;

	if( !AStar_GetPath( from, to, movetypes, &path ) )
		return -1;

	return path.totalDistance;
}

int AI_FindClosestReachableNode( vec3_t origin, edict_t *passent, int range, int flagsmask )
{
	int i;
	float closest;
	float dist;
	int node = -1;
	trace_t	tr;
	vec3_t maxs, mins;

	VectorSet( mins, -8, -8, -8 );
	VectorSet( maxs, 8, 8, 8 );

	// For Ladders, do not worry so much about reachability
	if( flagsmask & NODEFLAGS_LADDER )
	{
		VectorCopy( vec3_origin, maxs );
		VectorCopy( vec3_origin, mins );
	}

	closest = range;

	for( i = 0; i < nav.num_nodes; i++ )
	{
		if( flagsmask == NODE_ALL || nodes[i].flags & flagsmask )
		{
			dist = DistanceFast( nodes[i].origin, origin );

			if( dist < closest )
			{
				// make sure it is visible
				G_Trace( &tr, origin, mins, maxs, nodes[i].origin, passent, MASK_NODESOLID );
				if( tr.fraction == 1.0 )
				{
					node = i;
					closest = dist;
				}
			}
		}
	}
	return node;
}

int AI_FindClosestNode( vec3_t origin, float mindist, int range, int flagsmask )
{
	int i;
	float closest;
	float dist;
	int node = NODE_INVALID;

	if( mindist > range ) return -1;

	closest = range;

	for( i = 0; i < nav.num_nodes; i++ )
	{
		if( flagsmask == NODE_ALL || nodes[i].flags & flagsmask )
		{
			dist = DistanceFast( nodes[i].origin, origin );
			if( dist > mindist && dist < closest )
			{
				node = i;
				closest = dist;
			}
		}
	}
	return node;
}

void AI_ClearGoal( edict_t *self )
{
	self->ai.goal_node = NODE_INVALID;
	self->ai.current_node = NODE_INVALID;
	self->ai.next_node = NODE_INVALID;
	self->ai.goalEnt = NULL;
	self->ai.vsay_goalent = NULL;

	VectorSet( self->ai.move_vector, 0, 0, 0 );
}

void AI_SetGoal( edict_t *self, int goal_node )
{
	int node;

	self->ai.goal_node = goal_node;
	node = AI_FindClosestReachableNode( self->s.origin, self, NODE_DENSITY * 3, NODE_ALL );

	if( node == NODE_INVALID )
	{
		AI_ClearGoal( self );
		return;
	}

	// ASTAR 
	if( !AStar_GetPath( node, goal_node, self->ai.status.moveTypesMask, &self->ai.path ) )
	{
		AI_ClearGoal( self );
		return;
	}

	self->ai.current_node = self->ai.path.nodes[self->ai.path.numNodes];

	if( nav.debugMode && bot_showlrgoal->integer > 1 )
		G_PrintChasersf( self, "%s: GOAL: new START NODE selected %d goal %d\n", self->ai.pers.netname, node, self->ai.goal_node );

	self->ai.next_node = self->ai.current_node; // make sure we get to the nearest node first
	self->ai.node_timeout = 0;
	self->ai.longRangeGoalTimeout = 0;
	self->ai.tries = 0; // Reset the count of how many times we tried this goal
}

qboolean AI_NewNextNode( edict_t *self ) 
{
	// reset timeout
	self->ai.node_timeout = 0;

	if( self->ai.next_node == self->ai.goal_node )
	{
		if( nav.debugMode && bot_showlrgoal->integer > 1 )
			G_PrintChasersf( self, "%s: GOAL REACHED!\n", self->ai.pers.netname );

		//if botroam, setup a timeout for it
		/*
		if( nodes[self->ai.goal_node].flags & NODEFLAGS_BOTROAM )
		{
		int i;
		for( i = 0; i < nav.num_broams; i++ ) //find the broam
		{
		if( nav.broams[i].node != self->ai.goal_node )
		continue;

		//if(AIDevel.debugChased && bot_showlrgoal->integer)
		//	G_PrintMsg (AIDevel.chaseguy, "%s: BotRoam Time Out set up for node %i\n", self->ai.pers.netname, nav.broams[i].node);
		//Com_Printf( "%s: BotRoam Time Out set up for node %i\n", self->ai.pers.netname, nav.broams[i].node);
		self->ai.status.broam_timeouts[i] = level.time + 15000;
		break;
		}
		}
		*/

		// don't let it wait too long to weight the inventory again
		AI_ClearGoal( self );

		return qfalse; // force checking for a new long range goal
	}

	// we did not reach our goal yet. just setup next node...
	self->ai.current_node = self->ai.next_node;
	if( self->ai.path.numNodes )
		self->ai.path.numNodes--;
	self->ai.next_node = self->ai.path.nodes[self->ai.path.numNodes];

	return qtrue;
}

void AI_NodeReached( edict_t *self )
{
	if( !AI_NewNextNode( self ) )
	{
		AI_ClearGoal( self );
	}
}

qboolean AI_NodeHasTimedOut( edict_t *self )
{
	if( self->ai.goal_node == NODE_INVALID )
		return qtrue;

	if( !GS_MatchPaused() )
		self->ai.node_timeout += game.frametime;

	// Try again?
	if( self->ai.node_timeout > NODE_TIMEOUT || self->ai.next_node == NODE_INVALID )
	{
		if( self->ai.tries++ > 3 )
			return qtrue;
		else
			AI_SetGoal( self, self->ai.goal_node );
	}

	if( self->ai.current_node == NODE_INVALID || self->ai.next_node == NODE_INVALID )
		return qtrue;

	return qfalse;
}


/*
* AI_ReachedEntity
* Some nodes are declared so they are never reached until the entity says so.
* This is a entity saying so.
*/
void AI_ReachedEntity( edict_t *self )
{
	nav_ents_t *goalEnt;
	edict_t *ent;

	if( ( goalEnt = AI_GetGoalentForEnt( self ) ) == NULL )
		return;

	// find all bots which have this node as goal and tell them their goal is reached
	for( ent = game.edicts + 1; PLAYERNUM( ent ) < gs.maxclients; ent++ )
	{
		if( !ent->ai.type )
			continue;

		if( ent->ai.goal_node == goalEnt->node )
			AI_ClearGoal( ent );
	}
}

/*
* AI_TouchedEntity
* Some AI has touched some entity. Some entities are declared to never be reached until touched.
* See if it's one of them and declare it reached
*/
void AI_TouchedEntity( edict_t *self, edict_t *ent )
{
	int i;

	// right now we only support this on a few trigger entities (jumpads, teleporters)
	if( ent->r.solid != SOLID_TRIGGER )
		return;

	if( self->ai.next_node != NODE_INVALID &&
		( nodes[self->ai.next_node].flags & NODEFLAGS_REACHATTOUCH ) )
	{
		for( i = 0; i < nav.num_navigableEnts; i++ )
		{
			if( nav.navigableEnts[i].node == self->ai.next_node && nav.navigableEnts[i].ent == ent )
			{
				if( nav.debugMode && bot_showlrgoal->integer > 1 )
					G_PrintChasersf( self, "REACHED touch node %i with entity %s\n", self->ai.next_node, ent->classname ? ent->classname : "no classname" );

				AI_NodeReached( self );
				return;
			}
		}

		for( i = 0; i < nav.num_goalEnts; i++ )
		{
			if( nav.goalEnts[i].node == self->ai.next_node && nav.goalEnts[i].ent == ent )
			{
				if( nav.debugMode && bot_showlrgoal->integer > 1 )
					G_PrintChasersf( self, "REACHED touch node %i with entity %s\n", self->ai.next_node, ent->classname ? ent->classname : "no classname" );

				AI_NodeReached( self );
				return;
			}
		}
	}
}

void AI_GetNodeOrigin( int node, vec3_t origin ) 
{
	if( node == NODE_INVALID )
		VectorCopy( vec3_origin, origin );
	else
		VectorCopy( nodes[node].origin, origin );
}

int AI_GetNodeFlags( int node ) 
{
	if( node == NODE_INVALID )
		return 0;

	return nodes[node].flags;
}
