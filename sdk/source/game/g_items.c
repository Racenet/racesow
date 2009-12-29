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

#define HEALTH_IGNORE_MAX   1
#define HEALTH_TIMED	    2

#define SHELL_TIMEOUT	30000
#define QUAD_TIMEOUT	30000

//======================================================================

void DoRespawn( edict_t *ent )
{
	if( ent->team )
	{
		edict_t	*master;
		int count;
		int choice;

		master = ent->teammaster;

		for( count = 0, ent = master; ent; ent = ent->chain, count++ );

		choice = rand() % count;

		for( count = 0, ent = master; count < choice; ent = ent->chain, count++ );
	}

	ent->r.solid = SOLID_TRIGGER;
	ent->r.svflags &= ~SVF_NOCLIENT;

	GClip_LinkEntity( ent );

	// send an effect
	G_AddEvent( ent, EV_ITEM_RESPAWN, ent->item ? ent->item->tag : 0, qtrue );

	// powerups announce their presence with a global sound
	if( ent->item && ( ent->item->type & IT_POWERUP ) )
	{
		if( ent->item->tag == POWERUP_QUAD )
			G_GlobalSound( CHAN_AUTO, trap_SoundIndex( S_ITEM_QUAD_RESPAWN ) );
		if( ent->item->tag == POWERUP_SHELL )
			G_GlobalSound( CHAN_AUTO, trap_SoundIndex( S_ITEM_WARSHELL_RESPAWN ) );
	}
}

void SetRespawn( edict_t *ent, int delay )
{
	if( !ent->item )
		return;

	if( delay < 0 )
	{
		G_FreeEdict( ent );
		return;
	}

	ent->r.svflags |= SVF_NOCLIENT;
	ent->r.solid = SOLID_NOT;
	ent->nextThink = level.time + delay;
	ent->think = DoRespawn;

	// megahealth is different
	if( ( ent->style & HEALTH_TIMED ) && ent->r.owner )
	{
		ent->think = MegaHealth_think;
		ent->nextThink = level.time + 1;
	}

	GClip_LinkEntity( ent );
}

void G_Items_RespawnByType( unsigned int typeMask, int item_tag, float delay )
{
	edict_t *ent;
	int msecs;

	for( ent = game.edicts + gs.maxclients + BODY_QUEUE_SIZE; ENTNUM( ent ) < game.maxentities; ent++ )
	{
		if( !ent->r.inuse || !ent->item )
			continue;

		if( typeMask && !( ent->item->type & typeMask ) )
			continue;

		if( ent->spawnflags & ( DROPPED_ITEM|DROPPED_PLAYER_ITEM ) )
		{
			G_FreeEdict( ent );
			continue;
		}

		if( !G_Gametype_CanRespawnItem( ent->item ) )
			continue;

		// if a tag is specified, ignore others of the same type
		if( item_tag > 0 && ( ent->item->tag != item_tag ) )
			continue;

		msecs = (int)( delay * 1000 );
		if( msecs >= 0 )
			clamp_low( msecs, 1 );

		// megahealth is different
		if( ( ent->style & HEALTH_TIMED ) && ent->r.owner )
			ent->r.owner = NULL;

		SetRespawn( ent, msecs );
	}
}

//======================================================================

static qboolean Pickup_Powerup( edict_t *ent, edict_t *other )
{
	if( !ent->item || !ent->item->tag )
		return qfalse;

	if( ent->item->quantity )
	{
		int timeout;

		if( ent->spawnflags & ( DROPPED_ITEM | DROPPED_PLAYER_ITEM ) )
			timeout = ent->count + 1;
		else
			timeout = ent->item->quantity + 1;

		other->r.client->ps.inventory[ent->item->tag] += timeout;
	}
	else
	{
		other->r.client->ps.inventory[ent->item->tag]++;
	}

	return qtrue;
}

//======================================================================

qboolean Add_Ammo( gclient_t *client, gsitem_t *item, int count, qboolean add_it )
{
	int max;

	if( !client || !item )
		return qfalse;

	max = item->inventory_max;

	if( max <= 0 )
		max = 255;

	if( (int)client->ps.inventory[item->tag] >= max )
		return qfalse;

	if( add_it )
	{
		client->ps.inventory[item->tag] += count;

		if( (int)client->ps.inventory[item->tag] > max )
			client->ps.inventory[item->tag] = max;
	}

	return qtrue;
}

static qboolean Pickup_AmmoPack( edict_t *ent, edict_t *other )
{
	gsitem_t *item;
	int i;

	if( !other->r.client )
		return qfalse;

	for( i = AMMO_GUNBLADE; i < AMMO_TOTAL; i++ )
	{
		item = GS_FindItemByTag( i );
		if( item )
			Add_Ammo( other->r.client, item, ent->invpak[i], qtrue );
	}

	return qtrue;
}

static qboolean Pickup_Ammo( edict_t *ent, edict_t *other )
{
	int oldcount;
	int count;
	qboolean weapon;

	// ammo packs are special
	if( ent->item->tag == AMMO_PACK || ent->item->tag == AMMO_PACK_WEAK || ent->item->tag == AMMO_PACK_STRONG )
		return Pickup_AmmoPack( ent, other );

	weapon = ( ent->item->type & IT_WEAPON );

	if( ent->count )
		count = ent->count;
	else
		count = ent->item->quantity;

	oldcount = other->r.client->ps.inventory[ent->item->tag];

	if( !Add_Ammo( other->r.client, ent->item, count, qtrue ) )
		return qfalse;

	return qtrue;
}

static void Drop_Ammo( edict_t *ent, gsitem_t *item )
{
	edict_t	*dropped;
	int index;

	index = item->tag;
	dropped = Drop_Item( ent, item );
	if( dropped )
	{
		if( ent->r.client->ps.inventory[index] >= item->quantity )
			dropped->count = item->quantity;
		else
			dropped->count = ent->r.client->ps.inventory[index];

		ent->r.client->ps.inventory[index] -= dropped->count;
	}
}


//======================================================================

void MegaHealth_think( edict_t *self )
{
	self->nextThink = level.time + 1;

	if( self->r.owner )
	{
		if( self->r.owner->r.inuse && self->r.owner->s.team != TEAM_SPECTATOR &&
			HEALTH_TO_INT( self->r.owner->health ) > self->r.owner->max_health )
		{
			return;
		}

		// disable the link to the owner
		self->r.owner = NULL;
	}

	// player is back under max health so we can set respawn time for next MH
	if( !( self->spawnflags & ( DROPPED_ITEM | DROPPED_PLAYER_ITEM ) ) && G_Gametype_CanRespawnItem( self->item ) )
		SetRespawn( self, G_Gametype_RespawnTimeForItem( self->item ) );
	else
		G_FreeEdict( self );
}

static qboolean Pickup_Health( edict_t *ent, edict_t *other )
{
	if( !( ent->style & HEALTH_IGNORE_MAX ) )
		if( HEALTH_TO_INT( other->health ) >= other->max_health )
			return qfalse;

	// start from at least 0.5, so the player sees his health increase the correct amount
	if( other->health < 0.5 )
		other->health = 0.5;

	other->health += ent->item->quantity;

	if( other->r.client )
	{
		other->r.client->level.stats.health_taken += ent->item->quantity;
		teamlist[other->s.team].stats.health_taken += ent->item->quantity;
	}

	if( !( ent->style & HEALTH_IGNORE_MAX ) )
	{
		if( other->health > other->max_health )
			other->health = other->max_health;
	}
	else
	{
		if( other->health > 200 )
			other->health = 200;
	}

	if( ent->style & HEALTH_TIMED )
		ent->r.owner = other;

	return qtrue;
}

//======================================================================

qboolean Add_Armor( edict_t *ent, edict_t *other, qboolean pick_it )
{
	gclient_t *client = other->r.client;
	float maxarmorcount = 0.0f, newarmorcount;
	float pickupitem_maxcount;

	if( !client )
		return qfalse;

	if( !ent->item || !( ent->item->type & IT_ARMOR ) )
		return qfalse;

	pickupitem_maxcount = GS_Armor_MaxCountForTag( ent->item->tag );

	// can't pick if surpassed the max armor count of that type
	if( pickupitem_maxcount && ( client->resp.armor >= pickupitem_maxcount ) )
		return qfalse;

	if( GS_Armor_TagForCount( client->resp.armor ) == ARMOR_NONE )
		maxarmorcount = pickupitem_maxcount;
	else
		maxarmorcount = max( GS_Armor_MaxCountForTag( GS_Armor_TagForCount( client->resp.armor ) ), pickupitem_maxcount );

	if( !pickupitem_maxcount )
		newarmorcount = client->resp.armor + GS_Armor_PickupCountForTag( ent->item->tag );
	else
		newarmorcount = min( client->resp.armor + GS_Armor_PickupCountForTag( ent->item->tag ), maxarmorcount );

	// it can't be picked up if it doesn't add any armor
	if( newarmorcount <= client->resp.armor )
		return qfalse;

	if( pick_it )
	{
		client->resp.armor = newarmorcount;
		client->ps.stats[STAT_ARMOR] = ARMOR_TO_INT( client->resp.armor );
		client->level.stats.armor_taken += ent->item->quantity;
		teamlist[other->s.team].stats.armor_taken += ent->item->quantity;
	}
	
	return qtrue;
}

static qboolean Pickup_Armor( edict_t *ent, edict_t *other )
{
	return Add_Armor( ent, other, qtrue );
}

//======================================================================

//======================================================================

void Touch_ItemSound( edict_t *other, gsitem_t *item )
{
	if( item->pickup_sound )
	{
		if( item->type & IT_POWERUP )
			G_Sound( other, CHAN_ITEM, trap_SoundIndex( item->pickup_sound ), ATTN_NORM );
		else
			G_Sound( other, CHAN_AUTO, trap_SoundIndex( item->pickup_sound ), ATTN_NORM );
	}
}

//===============
//Touch_Item
//===============
void Touch_Item( edict_t *ent, edict_t *other, cplane_t *plane, int surfFlags )
{
	qboolean taken;

	if( !other->r.client || G_ISGHOSTING( other ) )
		return;

	if( !( other->r.client->ps.pmove.stats[PM_STAT_FEATURES] & PMFEAT_ITEMPICK ) )
		return;

	if( !ent->item || !( ent->item->flags & ITFLAG_PICKABLE ) )
		return; // not a grabbable item

	if( !G_Gametype_CanPickUpItem( ent->item ) )
		return;

	taken = G_PickupItem( ent, other );

	if( !( ent->spawnflags & ITEM_TARGETS_USED ) )
	{
		G_UseTargets( ent, other );
		ent->spawnflags |= ITEM_TARGETS_USED;
	}

	if( !taken )
		return;

	// flash the screen
	G_AddPlayerStateEvent( other->r.client, PSEV_PICKUP, ( ent->item->flags & IT_WEAPON ? ent->item->tag : 0 ) );

	G_AwardPlayerPickup( other, ent );

	// for messages
	other->r.client->teamstate.last_pickup = ent;

	// show icon and name on status bar
	other->r.client->ps.stats[STAT_PICKUP_ITEM] = ent->item->tag;
	other->r.client->resp.pickup_msg_time = level.time + 3000;

	if( ent->attenuation )
		Touch_ItemSound( other, ent->item );

	if( !( ent->spawnflags & ( DROPPED_ITEM | DROPPED_PLAYER_ITEM ) ) && G_Gametype_CanRespawnItem( ent->item ) )
		SetRespawn( ent, G_Gametype_RespawnTimeForItem( ent->item ) );
	else
		G_FreeEdict( ent );
}

//======================================================================

static void drop_temp_touch( edict_t *ent, edict_t *other, cplane_t *plane, int surfFlags )
{
	if( other == ent->r.owner )
		return;
	Touch_Item( ent, other, plane, surfFlags );
}

static void drop_make_touchable( edict_t *ent )
{
	int timeout;
	ent->touch = Touch_Item;
	timeout = G_Gametype_DroppedItemTimeout( ent->item );
	if( timeout )
	{
		ent->nextThink = level.time + 1000 * timeout;
		ent->think = G_FreeEdict;
	}
}

edict_t *Drop_Item( edict_t *ent, gsitem_t *item )
{
	edict_t	*dropped;
	vec3_t forward, right;
	vec3_t offset;

	if( !G_Gametype_CanDropItem( item, qfalse ) )
		return NULL;

	dropped = G_Spawn();
	dropped->classname = item->classname;
	dropped->item = item;
	dropped->spawnflags = DROPPED_ITEM;
	VectorCopy( item_box_mins, dropped->r.mins );
	VectorCopy( item_box_maxs, dropped->r.maxs );
	dropped->r.solid = SOLID_TRIGGER;
	dropped->movetype = MOVETYPE_TOSS;
	dropped->touch = drop_temp_touch;
	dropped->stop = AI_AddGoalEntity;
	dropped->r.owner = ent;
	dropped->r.svflags &= ~SVF_NOCLIENT;
	dropped->s.team = ent->s.team;
	dropped->s.type = ET_ITEM;
	dropped->s.itemNum = item->tag;
	dropped->s.effects = 0; // default effects are applied client side
	dropped->s.modelindex = trap_ModelIndex( dropped->item->world_model[0] );
	dropped->s.modelindex2 = trap_ModelIndex( dropped->item->world_model[1] );
	dropped->attenuation = 1;

	if( ent->r.client )
	{
		trace_t	trace;

		AngleVectors( ent->r.client->ps.viewangles, forward, right, NULL );
		VectorSet( offset, 24, 0, -16 );
		G_ProjectSource( ent->s.origin, offset, forward, right, dropped->s.origin );
		G_Trace( &trace, ent->s.origin, dropped->r.mins, dropped->r.maxs,
		         dropped->s.origin, ent, CONTENTS_SOLID );
		VectorCopy( trace.endpos, dropped->s.origin );

		dropped->spawnflags |= DROPPED_PLAYER_ITEM;

		// ugly hack for dropping backpacks
		if( item->tag == AMMO_PACK_WEAK || item->tag == AMMO_PACK_STRONG || item->tag == AMMO_PACK )
		{
			int w;
			qboolean anything = qfalse;

			for( w = WEAP_GUNBLADE + 1; w < WEAP_TOTAL; w++ )
			{
				if( item->tag == AMMO_PACK_WEAK || item->tag == AMMO_PACK )
				{
					int weakTag = GS_FindItemByTag( w )->weakammo_tag;
					if( ent->r.client->ps.inventory[weakTag] > 0 )
					{
						dropped->invpak[weakTag] = ent->r.client->ps.inventory[weakTag];
						ent->r.client->ps.inventory[weakTag] = 0;
						anything = qtrue;
					}
				}
				
				if( item->tag == AMMO_PACK_STRONG || item->tag == AMMO_PACK )
				{
					int strongTag = GS_FindItemByTag( w )->ammo_tag;
					if( ent->r.client->ps.inventory[strongTag] )
					{
						dropped->invpak[strongTag] = ent->r.client->ps.inventory[strongTag];
						ent->r.client->ps.inventory[strongTag] = 0;
						anything = qtrue;
					}
				}
			}

			if( !anything ) // if nothing was added to the pack, don't bother spawning it
			{
				G_FreeEdict( dropped );
				return NULL;
			}
		}

		// power-ups are special
		if( ( item->type & IT_POWERUP ) && item->quantity )
		{
			if( ent->r.client->ps.inventory[item->tag] )
			{
				dropped->count = ent->r.client->ps.inventory[item->tag];
				ent->r.client->ps.inventory[item->tag] = 0;
			}
			else
			{
				dropped->count = item->quantity;
			}
		}
	}
	else
	{
		AngleVectors( ent->s.angles, forward, right, NULL );
		VectorCopy( ent->s.origin, dropped->s.origin );

		// ugly hack for dropping backpacks
		if( item->tag == AMMO_PACK_WEAK || item->tag == AMMO_PACK_STRONG || item->tag == AMMO_PACK )
		{
			int w;

			for( w = WEAP_GUNBLADE + 1; w < WEAP_TOTAL; w++ )
			{
				if( item->tag == AMMO_PACK_WEAK || item->tag == AMMO_PACK )
				{
					gsitem_t *ammo = GS_FindItemByTag( GS_FindItemByTag( w )->weakammo_tag );
					if( ammo )
						dropped->invpak[ammo->tag] = ammo->quantity;
				}

				if( item->tag == AMMO_PACK_STRONG || item->tag == AMMO_PACK )
				{
					gsitem_t *ammo = GS_FindItemByTag( GS_FindItemByTag( w )->ammo_tag );
					if( ammo )
						dropped->invpak[ammo->tag] = ammo->quantity;
				}
			}
		}

		// power-ups are special
		if( ( item->type & IT_POWERUP ) && item->quantity )
		{
			dropped->count = item->quantity;
		}
	}

	VectorScale( forward, 100, dropped->velocity );
	dropped->velocity[2] = 300;

	dropped->think = drop_make_touchable;
	dropped->nextThink = level.time + 1000;

	ent->r.client->teamstate.last_drop_item = item;
	VectorCopy( dropped->s.origin, ent->r.client->teamstate.last_drop_location );

	GClip_LinkEntity( dropped );

	return dropped;
}

//======================================================================

//================
//G_PickupItem
//================
qboolean G_PickupItem( edict_t *ent, edict_t *other )
{
	gsitem_t	*it;
	qboolean taken = qfalse;

	if( !ent || !other )
		return qfalse;

	if( other->r.client && G_ISGHOSTING( other ) )
		return qfalse;

	if( !ent->item || !( ent->item->flags & ITFLAG_PICKABLE ) )
		return qfalse;

	it = ent->item;

	if( it->type & IT_WEAPON )
	{
		taken = Pickup_Weapon( ent, other );
	}
	else if( it->type & IT_AMMO )
	{
		taken = Pickup_Ammo( ent, other );
	}
	else if( it->type & IT_ARMOR )
	{
		taken = Pickup_Armor( ent, other );
	}
	else if( it->type & IT_HEALTH )
	{
		taken = Pickup_Health( ent, other );
	}
	else if( it->type & IT_POWERUP )
	{
		taken = Pickup_Powerup( ent, other );
	}

	if( taken && other->r.client )
		G_Gametype_ScoreEvent( other->r.client, "pickup", it->classname );

	return taken;
}

static void Drop_General( edict_t *ent, gsitem_t *item )
{
	Drop_Item( ent, item );
	if( ent->r.client && ent->r.client->ps.inventory[item->tag] > 0 )
		ent->r.client->ps.inventory[item->tag]--;
}

//================
//G_DropItem
//================
void G_DropItem( edict_t *ent, gsitem_t *it )
{
	if( !it || !( it->flags & ITFLAG_DROPABLE ) )
		return;

	if( !G_Gametype_CanDropItem( it, qfalse ) )
		return;

	if( it->type & IT_WEAPON )
	{
		Drop_Weapon( ent, it );
	}
	else if( it->type & IT_AMMO )
	{
		Drop_Ammo( ent, it );
	}
	else
	{
		Drop_General( ent, it );
	}
}

//================
//G_UseItem
//================
void G_UseItem( edict_t *ent, gsitem_t *it )
{
	if( !it || !( it->flags & ITFLAG_USABLE ) )
		return;

	if( it->type & IT_WEAPON )
		Use_Weapon( ent, it );
}

//======================================================================

static edict_t *G_ClosestFlagBase( edict_t *ent )
{
	int i;
	edict_t *t, *best;
	float dist, best_dist;
	static qboolean firstTime = qtrue;
	static unsigned int lastLevelSpawnCount;
	static edict_t *flagBases[GS_MAX_TEAMS];

	// store pointers to flag bases if called for the first time in this level spawn
	if( firstTime || lastLevelSpawnCount != game.levelSpawnCount )
	{
		for( t = game.edicts + 1 + gs.maxclients; ENTNUM( t ) < game.numentities; t++ )
		{
			if( t->s.type != ET_FLAG_BASE )
				continue;
			flagBases[t->s.team] = t;
		}

		// ok, remember last time we were called
		firstTime = qfalse;
		lastLevelSpawnCount = game.levelSpawnCount;
	}

	best = NULL;
	best_dist = 9999999;

	// find the closest flag base starting from TEAM_ALPHA
	for( i = TEAM_ALPHA; i < GS_MAX_TEAMS; i++ )
	{
		t = flagBases[i];
		if( !t )
			continue;

		// if equally distant from two bases, consider this item neutral
		dist = Distance( ent->s.origin, t->s.origin );
		if( best && fabs( dist - best_dist ) < 10 )
		{
			best = NULL;
			break;
		}

		if( dist < best_dist )
		{
			best_dist = dist;
			best = t;
		}
	}

	return best;
}

static qboolean G_ItemNeedsTimer( gsitem_t *it )
{
	assert( it );

	if( it->type == IT_POWERUP )
		return qtrue;
	if( it->type == IT_ARMOR )
		return ( it->tag == ARMOR_YA || it->tag == ARMOR_RA );
	if( it->type == IT_HEALTH )
		return ( it->tag == HEALTH_MEGA || it->tag == HEALTH_ULTRA );

	return qfalse;
}

//================
//item_timer_think
//================
void item_timer_think( edict_t *ent )
{
	edict_t *owner;

	owner = ent->r.owner;
	if( !owner || !owner->r.inuse || owner->s.type != ET_ITEM )
	{
		G_FreeEdict( ent );
		return;
	}

	if( owner->think != DoRespawn )
	{
		// megahealth is special
		if( owner->style & HEALTH_TIMED && owner->r.owner )
		{
/*			if( owner->r.owner->r.inuse && owner->r.owner->s.team != TEAM_SPECTATOR &&
				HEALTH_TO_INT( owner->r.owner->health ) > owner->r.owner->max_health )
				ent->s.frame = HEALTH_TO_INT( owner->r.owner->health ) - owner->r.owner->max_health;
			else*/
				ent->s.frame = 0;
			ent->s.frame += G_Gametype_RespawnTimeForItem( owner->item ) / 1000;
		}
		else
		{
			ent->s.frame = 0;
		}
	}
	else
	{
		ent->s.frame = owner->nextThink - level.time;
		if( ent->s.frame < 0 )
			ent->s.frame = 0;
		else
			ent->s.frame = (int)((float)ent->s.frame / 1000.0 + 0.5);
	}
	ent->nextThink = level.time + 1000;
}

//================
//Spawn_ItemTimer
//================
static void Spawn_ItemTimer( edict_t *ent )
{
	edict_t *timer;
	gsitem_t *item = ent->item;
	char location[MAX_CONFIGSTRING_CHARS];

	if( !G_ItemNeedsTimer( item ) )
		return;

	// location tag
	G_LocationName( ent->s.origin, location, sizeof( location ) );

	// item timer is a special entity type, carrying information about its parent item entity
	// which is only visible to spectators
	timer = G_Spawn();
	timer->s.type = ET_ITEM_TIMER;
	timer->s.itemNum = ent->s.itemNum;
	timer->s.team = TEAM_SPECTATOR;
	timer->r.svflags = SVF_ONLYTEAM | SVF_BROADCAST;
	timer->r.owner = ent;
	timer->s.modelindex = 0;
	timer->s.modelindex2 = G_LocationTAG( location );
	timer->nextThink = level.time + 250;
	timer->think = item_timer_think;
	VectorCopy( ent->s.origin, timer->s.origin ); // for z-sorting

	if( ( item->type != IT_POWERUP ) && GS_TeamBasedGametype())
	{
		edict_t *base;

		// what follows is basically a hack that allows timers to be assigned
		// to different teams in CTF. Powerups remain unassigned though
		base = G_ClosestFlagBase( ent );
		if( base )
			timer->s.modelindex = base->s.team;
	}

	timer->s.modelindex++; // add + 1 so we're guaranteed to have modelindex > 0
}

//================
//Finish_SpawningItem
//================
static void Finish_SpawningItem( edict_t *ent )
{
	trace_t	tr;
	vec3_t dest;
	gsitem_t *item = ent->item;

	assert( item );

	ent->s.itemNum = item->tag;
	VectorCopy( item_box_mins, ent->r.mins );
	VectorCopy( item_box_maxs, ent->r.maxs );

	if( ent->model )
	{
		ent->s.modelindex = trap_ModelIndex( ent->model );
	}
	else
	{
		if( item->world_model[0] )
			ent->s.modelindex = trap_ModelIndex( item->world_model[0] );
		if( item->world_model[1] )
			ent->s.modelindex2 = trap_ModelIndex( item->world_model[1] );
	}

	ent->r.solid = SOLID_TRIGGER;
	ent->r.svflags &= ~SVF_NOCLIENT;
	ent->movetype = MOVETYPE_TOSS;
	ent->touch = Touch_Item;
	ent->attenuation = 1;

	if( ent->spawnflags & 1 )
		ent->gravity = 0;

	G_Trace( &tr, ent->s.origin, ent->r.mins, ent->r.maxs, ent->s.origin, ent, MASK_SOLID );
	if( tr.startsolid )
	{
		vec3_t end;

		// move it 16 units up, cause it's typical they share the leaf with the floor
		VectorCopy( ent->s.origin, end );
		end[2] += 16;

		G_Trace( &tr, end, ent->r.mins, ent->r.maxs, ent->s.origin, ent, MASK_SOLID );
		if( tr.startsolid )
		{
			G_Printf( "Warning: %s %s spawns inside solid. Inhibited\n", ent->classname, vtos( ent->s.origin ) );
			G_FreeEdict( ent );
			return;
		}

		VectorCopy( tr.endpos, ent->s.origin );
	}

	// drop the item to floor
	if( ent->gravity )
	{
		VectorSet( dest, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2] - 128 );
		G_Trace( &tr, ent->s.origin, ent->r.mins, ent->r.maxs, dest, ent, MASK_SOLID );
		VectorCopy( tr.endpos, ent->s.origin );
	}

	if( item->type & IT_HEALTH )
	{
		if( item->tag == HEALTH_SMALL || item->tag == HEALTH_ULTRA )
			ent->style = HEALTH_IGNORE_MAX;
		else if( item->tag == HEALTH_MEGA )
			ent->style = HEALTH_IGNORE_MAX|HEALTH_TIMED;
	}

	if( ent->team )
	{
		ent->flags &= ~FL_TEAMSLAVE;
		ent->chain = ent->teamchain;
		ent->teamchain = NULL;

		ent->r.svflags |= SVF_NOCLIENT;
		ent->r.solid = SOLID_NOT;

		// team slaves and targeted items aren't present at start
		if( ent == ent->teammaster && !ent->targetname )
		{
			ent->nextThink = level.time + 1;
			ent->think = DoRespawn;
			GClip_LinkEntity( ent );
		}
	}
	else if( ent->targetname )
	{
		ent->r.svflags |= SVF_NOCLIENT;
		ent->r.solid = SOLID_NOT;
	}

	GClip_LinkEntity( ent );

	Spawn_ItemTimer( ent );

	AI_AddGoalEntity( ent );
}


/*
* Items may be spawned above other entities and they need them spawned before
*/
void G_Items_FinishSpawningItems( void )
{
	edict_t *ent;

	for( ent = game.edicts + 1 + gs.maxclients; ENTNUM( ent ) < game.numentities; ent++ )
	{
		if( !ent->r.inuse || !ent->item || ent->s.type != ET_ITEM )
			continue;

		Finish_SpawningItem( ent );
	}
}

//============
//SpawnItem
//
//Sets the clipping size and plants the object on the floor.
//
//Items can't be immediately dropped to floor, because they might
//be on an entity that hasn't spawned yet.
//============
void SpawnItem( edict_t *ent, gsitem_t *item )
{
	// set items as ET_ITEM for simpleitems
	ent->s.type = ET_ITEM;
	ent->s.itemNum = item->tag;
	ent->item = item;
	ent->s.effects = 0; // default effects are applied client side
}


//===============
//PrecacheItem
//
//Precaches all data needed for a given item.
//This will be called for each item spawned in a level,
//and for each item in each client's inventory.
//===============
void PrecacheItem( gsitem_t *it )
{
	int i;
	const char *s, *start;
	char data[MAX_QPATH];
	int len;
	gsitem_t	*ammo;

	if( !it )
		return;

	if( it->pickup_sound )
		trap_SoundIndex( it->pickup_sound );
	for( i = 0; i < MAX_ITEM_MODELS; i++ )
	{
		if( it->world_model[i] )
			trap_ModelIndex( it->world_model[i] );
	}

	if( it->icon )
		trap_ImageIndex( it->icon );

	// parse everything for its ammo
	if( it->ammo_tag )
	{
		ammo = GS_FindItemByTag( it->ammo_tag );
		if( ammo != it )
			PrecacheItem( ammo );
	}

	// parse the space separated precache string for other items
	for( i = 0; i < 3; i++ )
	{
		if( i == 0 )
			s = it->precache_models;
		else if( i == 1 )
			s = it->precache_sounds;
		else
			s = it->precache_images;

		if( !s || !s[0] )
			continue;

		while( *s )
		{
			start = s;
			while( *s && *s != ' ' )
				s++;

			len = s-start;
			if( len >= MAX_QPATH || len < 5 )
				G_Error( "PrecacheItem: %s has bad precache string", it->classname );
			memcpy( data, start, len );
			data[len] = 0;
			if( *s )
				s++;

			if( i == 0 )
				trap_ModelIndex( data );
			else if( i == 1 )
				trap_SoundIndex( data );
			else
				trap_ImageIndex( data );
		}
	}
}

//======================================================================

//===============
//SetItemNames
//
//Called by worldspawn
//===============
void G_PrecacheItems( void )
{
	int i;
	gsitem_t *item;

	// precache item names and weapondefs
	for( i = 1; ( item = GS_FindItemByTag( i ) ) != NULL; i++ )
	{
		trap_ConfigString( CS_ITEMS + i, item->name );

		if( item->type & IT_WEAPON && GS_GetWeaponDef( item->tag ) )
		{
			G_PrecacheWeapondef( i, &GS_GetWeaponDef( item->tag )->firedef );
			G_PrecacheWeapondef( i, &GS_GetWeaponDef( item->tag )->firedef_weak );
		}
	}

	// precache items
	if( GS_Instagib() )
	{
		item = GS_FindItemByTag( WEAP_INSTAGUN );
		PrecacheItem( item );
	}
	else
	{
		for( i = WEAP_GUNBLADE; i < WEAP_TOTAL; i++ )
		{
			item = GS_FindItemByTag( i );
			PrecacheItem( item );
		}
	}

	// Vic: precache ammo pack if it's droppable
	item = GS_FindItemByClassname( "item_ammopack" );
	if( item && G_Gametype_CanDropItem( item, qtrue ) )
	{
		PrecacheItem( item );
	}
}

