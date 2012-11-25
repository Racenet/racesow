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

cvar_t *sv_botpersonality;

ai_weapon_t AIWeapons[WEAP_TOTAL];

//==========================================
// AI_InitLevel
// Inits Map local parameters
//==========================================
void AI_InitLevel( void )
{
	edict_t	*ent;

	//Init developer mode
	bot_showpath = trap_Cvar_Get( "bot_showpath", "0", 0 );
	bot_showcombat = trap_Cvar_Get( "bot_showcombat", "0", 0 );
	bot_showsrgoal = trap_Cvar_Get( "bot_showsrgoal", "0", 0 );
	bot_showlrgoal = trap_Cvar_Get( "bot_showlrgoal", "0", 0 );
	bot_dummy = trap_Cvar_Get( "bot_dummy", "0", 0 );
	sv_botpersonality =	    trap_Cvar_Get( "sv_botpersonality", "0", CVAR_ARCHIVE );

	nav.debugMode = qfalse;

	AI_InitNavigationData( qfalse );

	// count bots
	game.numBots = 0;
	for( ent = game.edicts + 1; PLAYERNUM( ent ) < gs.maxclients; ent++ )
	{
		if( !ent->r.inuse || !ent->ai.type ) continue;
		if( ent->r.svflags & SVF_FAKECLIENT && ent->ai.type == AI_ISBOT )
			game.numBots++;
	}

	// set up weapon usage weights

	memset( &AIWeapons, 0, sizeof( ai_weapon_t )*WEAP_TOTAL );

	//WEAP_GUNBLADE
	AIWeapons[WEAP_GUNBLADE].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIWeapons[WEAP_GUNBLADE].RangeWeight[AIWEAP_LONG_RANGE] = 0.1f;
	AIWeapons[WEAP_GUNBLADE].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.2f;
	AIWeapons[WEAP_GUNBLADE].RangeWeight[AIWEAP_SHORT_RANGE] = 0.3f;
	AIWeapons[WEAP_GUNBLADE].RangeWeight[AIWEAP_MELEE_RANGE] = 0.4f;

	//WEAP_MACHINEGUN
	AIWeapons[WEAP_MACHINEGUN].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIWeapons[WEAP_MACHINEGUN].RangeWeight[AIWEAP_LONG_RANGE] = 0.8f;
	AIWeapons[WEAP_MACHINEGUN].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.7f;
	AIWeapons[WEAP_MACHINEGUN].RangeWeight[AIWEAP_SHORT_RANGE] = 0.4f;
	AIWeapons[WEAP_MACHINEGUN].RangeWeight[AIWEAP_MELEE_RANGE] = 0.1f;

	//WEAP_RIOTGUN
	AIWeapons[WEAP_RIOTGUN].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIWeapons[WEAP_RIOTGUN].RangeWeight[AIWEAP_LONG_RANGE] = 0.1f;
	AIWeapons[WEAP_RIOTGUN].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.5f;
	AIWeapons[WEAP_RIOTGUN].RangeWeight[AIWEAP_SHORT_RANGE] = 0.8f;
	AIWeapons[WEAP_RIOTGUN].RangeWeight[AIWEAP_MELEE_RANGE] = 0.5f;

	//ROCKETLAUNCHER
	AIWeapons[WEAP_ROCKETLAUNCHER].aimType = AI_AIMSTYLE_PREDICTION_EXPLOSIVE;
	AIWeapons[WEAP_ROCKETLAUNCHER].RangeWeight[AIWEAP_LONG_RANGE] = 0.2f;
	AIWeapons[WEAP_ROCKETLAUNCHER].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.5f;
	AIWeapons[WEAP_ROCKETLAUNCHER].RangeWeight[AIWEAP_SHORT_RANGE] = 0.9f;
	AIWeapons[WEAP_ROCKETLAUNCHER].RangeWeight[AIWEAP_MELEE_RANGE] = 0.6f;

	//WEAP_GRENADELAUNCHER
	AIWeapons[WEAP_GRENADELAUNCHER].aimType = AI_AIMSTYLE_DROP;
	AIWeapons[WEAP_GRENADELAUNCHER].RangeWeight[AIWEAP_LONG_RANGE] = 0.0f;
	AIWeapons[WEAP_GRENADELAUNCHER].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.1f;
	AIWeapons[WEAP_GRENADELAUNCHER].RangeWeight[AIWEAP_SHORT_RANGE] = 0.4f;
	AIWeapons[WEAP_GRENADELAUNCHER].RangeWeight[AIWEAP_MELEE_RANGE] = 0.3f;

	//WEAP_PLASMAGUN
	AIWeapons[WEAP_PLASMAGUN].aimType = AI_AIMSTYLE_PREDICTION;
	AIWeapons[WEAP_PLASMAGUN].RangeWeight[AIWEAP_LONG_RANGE] = 0.1f;
	AIWeapons[WEAP_PLASMAGUN].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.5f;
	AIWeapons[WEAP_PLASMAGUN].RangeWeight[AIWEAP_SHORT_RANGE] = 0.7f;
	AIWeapons[WEAP_PLASMAGUN].RangeWeight[AIWEAP_MELEE_RANGE] = 0.4f;

	//WEAP_ELECTROBOLT
	AIWeapons[WEAP_ELECTROBOLT].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIWeapons[WEAP_ELECTROBOLT].RangeWeight[AIWEAP_LONG_RANGE] = 0.9f;
	AIWeapons[WEAP_ELECTROBOLT].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.7f;
	AIWeapons[WEAP_ELECTROBOLT].RangeWeight[AIWEAP_SHORT_RANGE] = 0.4f;
	AIWeapons[WEAP_ELECTROBOLT].RangeWeight[AIWEAP_MELEE_RANGE] = 0.3f;

	//WEAP_LASERGUN
	AIWeapons[WEAP_LASERGUN].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIWeapons[WEAP_LASERGUN].RangeWeight[AIWEAP_LONG_RANGE] = 0.0f;
	AIWeapons[WEAP_LASERGUN].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.0f;
	AIWeapons[WEAP_LASERGUN].RangeWeight[AIWEAP_SHORT_RANGE] = 0.7f;
	AIWeapons[WEAP_LASERGUN].RangeWeight[AIWEAP_MELEE_RANGE] = 0.6f;

	//WEAP_INSTAGUN
	AIWeapons[WEAP_INSTAGUN].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIWeapons[WEAP_INSTAGUN].RangeWeight[AIWEAP_LONG_RANGE] = 0.9f;
	AIWeapons[WEAP_INSTAGUN].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.9f;
	AIWeapons[WEAP_INSTAGUN].RangeWeight[AIWEAP_SHORT_RANGE] = 0.9f;
	AIWeapons[WEAP_INSTAGUN].RangeWeight[AIWEAP_MELEE_RANGE] = 0.9f;
}

//==========================================
// G_FreeAI
// removes the AI handle from memory
//==========================================
void G_FreeAI( edict_t *ent )
{
	if( !ent->ai.type ) return;
	if( ent->ai.type == AI_ISBOT )
	{
		game.numBots--;
	}

	ent->ai.type = AI_INACTIVE;
}

//==========================================
// G_SpawnAI
// allocate ai_handle_t for this entity
//==========================================
void G_SpawnAI( edict_t *ent )
{
	memset( &ent->ai, 0, sizeof( ai_handle_t ) );
	if( ent->r.svflags & SVF_FAKECLIENT )
		ent->ai.type = AI_ISBOT;
	else
		ent->ai.type = AI_ISMONSTER;
}

// FIXME: All this is too slow
nav_ents_t *AI_GetGoalentForEnt( edict_t *target )
{
	int i;

	if( !target )
		return NULL;

	for( i = 0; i < nav.num_goalEnts; i++ )
	{
		if( target == nav.goalEnts[i].ent )
			return &nav.goalEnts[i];
	}

	return NULL;
}

edict_t *AI_GetGoalentAtNode( int node ) 
{
	int i;

	for( i = 0; i < nav.num_goalEnts; i++ )
	{
		if( nav.goalEnts[i].node == node )
			return nav.goalEnts[i].ent;
	}

	return NULL;
}

edict_t *AI_GetGoalEnt( int index )
{
	if( !nav.loaded || index < 0 || index >= nav.num_goalEnts )
		return NULL;

	return nav.goalEnts[index].ent;
}

int AI_GetGoalEntNode( int index )
{
	if( !nav.loaded || index < 0 || index >= nav.num_goalEnts )
		return NODE_INVALID;

	return nav.goalEnts[index].node;
}



//==========================================
// AI_ResetWeights
// Init bot weights from bot-class weights.
//==========================================
void AI_ResetWeights( ai_handle_t *ai )
{
	int i;

	// restore defaults from bot personality
	memset( ai->status.entityWeights, 0, sizeof( ai->status.entityWeights ) );

	for( i = 0; i < nav.num_goalEnts; i++ )
	{
		if( nav.goalEnts[i].ent->item )
			ai->status.entityWeights[i] = ai->pers.inventoryWeights[nav.goalEnts[i].ent->item->tag];
	}
}


//==========================================
// AI_ResetNavigation
// Init bot navigation. Called at first spawn & each respawn
//==========================================
void AI_ResetNavigation( edict_t *self )
{
	self->enemy = self->ai.latched_enemy = NULL;
	self->ai.state_combat_timeout = 0;

	self->ai.is_bunnyhop = qfalse;

	self->ai.nearest_node_tries = 0;
	self->ai.longRangeGoalTimeout = 0;

	self->ai.blocked_timeout = level.time + 15000;

	self->ai.shortRangeGoalTimeout = level.time + AI_SHORT_RANGE_GOAL_DELAY;
	self->movetarget = NULL;

	AI_ClearGoal( self );
}

//==========================================
// AI_PickLongRangeGoal
//
// Evaluate the best long range goal and send the bot on
// its way. This is a good time waster, so use it sparingly.
// Do not call it for every think cycle.
//
// jal: I don't think there is any problem by calling it,
// now that we have stored the costs at the nav.costs table (I don't do it anyway)
//==========================================
void AI_PickLongRangeGoal( edict_t *self )
{
#define WEIGHT_MAXDISTANCE_FACTOR 20000.0f
#define COST_INFLUENCE	0.5f
	int i;
	float weight, bestWeight = 0.0;
	int current_node;
	float cost;
	float dist;
	nav_ents_t *goalEnt, *bestGoalEnt = NULL;

	AI_ClearGoal( self );

	if( G_ISGHOSTING( self ) )
		return;

	if( self->ai.longRangeGoalTimeout > level.time )
		return;

	if( !self->r.client->ps.pmove.stats[PM_STAT_MAXSPEED] ) {
		return;
	}

	self->ai.longRangeGoalTimeout = level.time + AI_LONG_RANGE_GOAL_DELAY + brandom( 0, 1000 );

	// look for a target
	current_node = AI_FindClosestReachableNode( self->s.origin, self, ( ( 1 + self->ai.nearest_node_tries ) * NODE_DENSITY ), NODE_ALL );
	self->ai.current_node = current_node;

	if( current_node == NODE_INVALID )
	{
		if( nav.debugMode && bot_showlrgoal->integer )
			G_PrintChasersf( self, "%s: LRGOAL: Closest node not found. Tries:%i\n", self->ai.pers.netname, self->ai.nearest_node_tries );

		self->ai.nearest_node_tries++; // extend search radius with each try
		return;
	}

	self->ai.nearest_node_tries = 0;

	// Run the list of potential goal entities
	for( i = 0; i < nav.num_goalEnts; i++ )
	{
		goalEnt = &nav.goalEnts[i];

		if( !goalEnt->ent )
			continue;

		if( !goalEnt->ent->r.inuse )
		{
			goalEnt->node = NODE_INVALID;
			continue;
		}

		if( goalEnt->ent->r.client )
		{
			if( G_ISGHOSTING( goalEnt->ent ) || goalEnt->ent->ai.notarget )
				goalEnt->node = NODE_INVALID;
			else
				goalEnt->node = AI_FindClosestReachableNode( goalEnt->ent->s.origin, goalEnt->ent, NODE_DENSITY, NODE_ALL );
		}

		if( goalEnt->ent->item )
		{
			if( !G_Gametype_CanPickUpItem( goalEnt->ent->item ) )
				continue;
		}

		if( goalEnt->node == NODE_INVALID )
			continue;

		weight = self->ai.status.entityWeights[i];

		if( weight <= 0.0f )
			continue;

		// don't try to find cost for too far away objects
		dist = DistanceFast( self->s.origin, goalEnt->ent->s.origin );
		if( dist > WEIGHT_MAXDISTANCE_FACTOR * weight/* || dist < AI_GOAL_SR_RADIUS*/ )
			continue;

		cost = AI_FindCost( current_node, goalEnt->node, self->ai.status.moveTypesMask );
		if( cost == NODE_INVALID )
			continue;

		cost -= brandom( 0, 2000 ); // allow random variations
		clamp_low( cost, 1 );
		weight = ( 1000 * weight ) / ( cost * COST_INFLUENCE ); // Check against cost of getting there

		if( weight > bestWeight )
		{
			bestWeight = weight;
			bestGoalEnt = goalEnt;
		}
	}

	if( bestGoalEnt )
	{
		self->ai.goalEnt = bestGoalEnt;
		AI_SetGoal( self, bestGoalEnt->node );

		if( self->ai.goalEnt != NULL && nav.debugMode && bot_showlrgoal->integer )
			G_PrintChasersf( self, "%s: selected a %s at node %d for LR goal. (weight %f)\n", self->ai.pers.netname, self->ai.goalEnt->ent->classname, self->ai.goalEnt->node, bestWeight );

		return;
	}

	if( nav.debugMode && bot_showlrgoal->integer )
		G_PrintChasersf( self, "%s: did not find a LR goal.\n", self->ai.pers.netname );

#undef WEIGHT_MAXDISTANCE_FACTOR
#undef COST_INFLUENCE
}

//==========================================
// AI_PickShortRangeGoal
// Pick best goal based on importance and range. This function
// overrides the long range goal selection for items that
// are very close to the bot and are reachable.
//==========================================
static void AI_PickShortRangeGoal( edict_t *self )
{
	edict_t *bestGoal = NULL;
	float bestWeight = 0;
	nav_ents_t *goalEnt;
	gsitem_t *item;
	qboolean canPickupItems;
	int i;

	if( !self->r.client || G_ISGHOSTING( self ) )
		return;

	if( self->ai.state_combat_timeout > level.time )
	{
		self->ai.shortRangeGoalTimeout = self->ai.state_combat_timeout;
		return;
	}

	if( self->ai.shortRangeGoalTimeout > level.time )
		return;

	canPickupItems = (self->r.client->ps.pmove.stats[PM_STAT_FEATURES] & PMFEAT_ITEMPICK) != 0;

	self->ai.shortRangeGoalTimeout = level.time + AI_SHORT_RANGE_GOAL_DELAY;

	self->movetarget = NULL;

	for( i = 0; i < nav.num_goalEnts; i++ )
	{
		goalEnt = &nav.goalEnts[i];
		if( !goalEnt->ent->r.inuse || goalEnt->ent->r.solid == SOLID_NOT )
			continue;

		if( goalEnt->ent->r.client )
			continue;

		if( self->ai.status.entityWeights[i] <= 0.0f )
			continue;

		item = goalEnt->ent->item;
		if( canPickupItems && item ) {
			if( !G_Gametype_CanPickUpItem( item ) || !( item->flags & ITFLAG_PICKABLE ) ) {
				continue;
			}
		}

		if( DistanceFast( self->s.origin, goalEnt->ent->s.origin ) > AI_GOAL_SR_RADIUS )
			continue;

		if( AI_ShortRangeReachable( self, goalEnt->ent->s.origin ) && G_InFront( self, goalEnt->ent ) )
		{
			// Long range goal gets top priority
			if( goalEnt == self->ai.goalEnt ) 
			{
				bestGoal = goalEnt->ent;
				break;
			}

			// get the one with the best weight
			if( self->ai.status.entityWeights[i] > bestWeight )
			{
				bestWeight = self->ai.status.entityWeights[i];
				bestGoal = goalEnt->ent;
			}
		}
	}

	if( bestGoal )
	{
		self->movetarget = bestGoal;
		if( nav.debugMode && bot_showsrgoal->integer )
			G_PrintChasersf( self, "%s: selected a %s for SR goal.\n", self->ai.pers.netname, self->movetarget->classname );
	}
}

//===================
//  AI_CategorizePosition
//  Categorize waterlevel and groundentity/stepping
//===================
void AI_CategorizePosition( edict_t *ent )
{
	qboolean stepping = AI_IsStep( ent );

	ent->was_swim = ent->is_swim;
	ent->was_step = ent->is_step;

	ent->is_ladder = AI_IsLadder( ent->s.origin, ent->s.angles, ent->r.mins, ent->r.maxs, ent );

	G_CategorizePosition( ent );
	if( ent->waterlevel > 2 || ( ent->waterlevel && !stepping ) )
	{
		ent->is_swim = qtrue;
		ent->is_step = qfalse;
		return;
	}

	ent->is_swim = qfalse;
	ent->is_step = stepping;
}

void AI_UpdateStatus( edict_t *self )
{
	if( !G_ISGHOSTING( self ) )
	{
		AI_ResetWeights( &self->ai );

		self->ai.status.moveTypesMask = self->ai.pers.moveTypesMask;

		if( !GT_asCallBotStatus( self ) )
			self->ai.pers.UpdateStatus( self );

		self->ai.statusUpdateTimeout = level.time + AI_STATUS_TIMEOUT;

		// no cheating with moveTypesMask
		self->ai.status.moveTypesMask &= self->ai.pers.moveTypesMask;
	}
}

//==========================================
// AI_Think
// think funtion for AIs
//==========================================
void AI_Think( edict_t *self )
{
	if( !self->ai.type )
		return;

	if( level.spawnedTimeStamp + 5000 > game.realtime || !level.canSpawnEntities )
	{
		self->nextThink = level.time + game.snapFrameTime;
		return;
	}

	AI_CategorizePosition( self );

	// check for being blocked
	if( !G_ISGHOSTING( self ) )
	{
		if( VectorLengthFast( self->velocity ) > 37 )
			self->ai.blocked_timeout = level.time + 10000;

		// if completely stuck somewhere
		if( self->ai.blocked_timeout < level.time )
		{
			self->ai.pers.blockedTimeout( self );
			return;
		}
	}

	//update status information to feed up ai
	if( self->ai.statusUpdateTimeout <= level.time )
		AI_UpdateStatus( self );

	if( AI_NodeHasTimedOut( self ) )
		AI_ClearGoal( self );

	if( self->ai.goal_node == NODE_INVALID )
		AI_PickLongRangeGoal( self );

	AI_PickShortRangeGoal( self );

	self->ai.pers.RunFrame( self );

	// Show the path
	if( nav.debugMode && bot_showpath->integer && self->ai.goal_node != NODE_INVALID )
	{
		// only draw the path of those bots which are being chased
		edict_t *chaser;
		qboolean chaserFound = qfalse;

		for( chaser = game.edicts + 1; ENTNUM( chaser ) < gs.maxclients; chaser++ )
		{
			if( chaser->r.client->resp.chase.active && chaser->r.client->resp.chase.target == ENTNUM( self ) )
			{
				AITools_DrawPath( self, self->ai.goal_node );
				chaserFound = qtrue;
			}
		}

		if( !chaserFound && game.numBots == 1 )
			AITools_DrawPath( self, self->ai.goal_node );
	}
}

