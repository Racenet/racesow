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
// g_weapon.c

#include "g_local.h"

void SV_Physics_LinearProjectile( edict_t *ent );

static qboolean	is_quad;

#define NO_ROCKET_ANTILAG
#define PLASMAHACK // ffs : hack for the plasmagun

#ifdef PLASMAHACK
void W_Plasma_Backtrace( edict_t *ent, const vec3_t start );
#endif

/*
* Use_Weapon
*/
void Use_Weapon( edict_t *ent, gsitem_t *item )
{
	int ammocount, weakammocount;
	gs_weapon_definition_t *weapondef;

	//invalid weapon item
	if( item->tag < WEAP_NONE || item->tag >= WEAP_TOTAL )
		return;

	// see if we're already changing to it
	if( ent->r.client->ps.stats[STAT_PENDING_WEAPON] == item->tag )
		return;

	weapondef = GS_GetWeaponDef( item->tag );

	if( !g_select_empty->integer && !( item->type & IT_AMMO ) )
	{
		if( weapondef->firedef.usage_count )
		{
			if( weapondef->firedef.ammo_id )
				ammocount = ent->r.client->ps.inventory[weapondef->firedef.ammo_id];
			else
				ammocount = weapondef->firedef.usage_count;
		}
		else
			ammocount = 1; // can change weapon

		if( weapondef->firedef_weak.usage_count )
		{
			if( weapondef->firedef_weak.ammo_id )
				weakammocount = ent->r.client->ps.inventory[weapondef->firedef_weak.ammo_id];
			else
				weakammocount = weapondef->firedef_weak.usage_count;
		}
		else
			weakammocount = 1; // can change weapon

		if( ammocount < weapondef->firedef.usage_count &&
			weakammocount < weapondef->firedef_weak.usage_count )
		{
			return;
		}
	}

	// change to this weapon when down
	ent->r.client->ps.stats[STAT_PENDING_WEAPON] = item->tag;
}

/*
* Pickup_Weapon
*/
qboolean Pickup_Weapon( edict_t *ent, edict_t *other )
{
	int ammo_tag;

	other->r.client->ps.inventory[ent->item->tag]++;

	// never allow the player to carry more than 2 copies of the same weapon
	if( other->r.client->ps.inventory[ent->item->tag] > ent->item->inventory_max )
		other->r.client->ps.inventory[ent->item->tag] = ent->item->inventory_max;

	if( !( ent->spawnflags & DROPPED_ITEM ) )
	{
		// give them some ammo with it
		ammo_tag = ent->item->weakammo_tag;

		if( ammo_tag )
			Add_Ammo( other->r.client, GS_FindItemByTag( ammo_tag ), GS_FindItemByTag( ammo_tag )->quantity, qtrue );
	}
	else
	{    //it's a dropped weapon
		ammo_tag = ent->item->weakammo_tag;
		if( ent->count && ammo_tag )
			Add_Ammo( other->r.client, GS_FindItemByTag( ammo_tag ), ent->count, qtrue );
	}

	return qtrue;
}

/*
* Drop_Weapon
*/
void Drop_Weapon( edict_t *ent, gsitem_t *item )
{
	int otherweapon;
	edict_t *drop;
	int ammodrop = 0;

	if( item->tag < 1 || item->tag >= WEAP_TOTAL )
	{
		G_PrintMsg( ent, "Can't drop unknown weapon\n" );
		return;
	}

	// find out the amount of ammo to drop
	if( ent->r.client->ps.inventory[item->tag] > 1 && ent->r.client->ps.inventory[item->weakammo_tag] > 5 )
	{
		ammodrop = ent->r.client->ps.inventory[item->weakammo_tag] / 2;
	}
	else // drop all
	{
		ammodrop = ent->r.client->ps.inventory[item->weakammo_tag];
	}

	drop = Drop_Item( ent, item );
	if( drop )
	{
		ent->r.client->ps.inventory[item->weakammo_tag] -= ammodrop;
		drop->count = ammodrop;
		drop->spawnflags |= DROPPED_PLAYER_ITEM;
		ent->r.client->ps.inventory[item->tag]--;

		if( !ent->r.client->ps.inventory[item->tag] )
		{
			otherweapon = GS_SelectBestWeapon( &ent->r.client->ps );
			Use_Weapon( ent, GS_FindItemByTag( otherweapon ) );
		}
	}
}

//======================================================================
//
// WEAPON FIRING
//
//======================================================================

/*
* G_ProjectileDistancePrestep
*/
static void G_ProjectileDistancePrestep( edict_t *projectile, float distance )
{
	float speed;
	vec3_t dir, dest;
	int mask, i;
	trace_t	trace;
#ifdef PLASMAHACK
	vec3_t plasma_hack_start;
#endif

	if( projectile->movetype != MOVETYPE_TOSS 
		&& projectile->movetype != MOVETYPE_LINEARPROJECTILE
		&& projectile->movetype != MOVETYPE_BOUNCE 
		&& projectile->movetype != MOVETYPE_BOUNCEGRENADE )
		return;

	if( !distance )
		return;

	if( ( speed = VectorNormalize2( projectile->velocity, dir ) ) == 0.0f )
		return;

	mask = ( projectile->r.clipmask ) ? projectile->r.clipmask : MASK_SHOT; // race trick should come set up inside clipmask

	if( projectile->movetype == MOVETYPE_LINEARPROJECTILE )
		VectorCopy( projectile->s.origin2, projectile->s.origin );

#ifdef PLASMAHACK
	VectorCopy( projectile->s.origin, plasma_hack_start );
#endif

	VectorMA( projectile->s.origin, distance, dir, dest );
	G_Trace4D( &trace, projectile->s.origin, projectile->r.mins, projectile->r.maxs, dest, projectile->r.owner, mask, projectile->timeDelta );

	for( i = 0; i < 3; i++ )
		projectile->s.origin[i] = projectile->s.origin2[i] = projectile->olds.origin[i] = projectile->olds.origin2[i] = trace.endpos[i];

	GClip_LinkEntity( projectile );
	SV_Impact( projectile, &trace );

	// set initial water state
	if( !projectile->r.inuse )
		return;

	projectile->waterlevel = ( G_PointContents4D( projectile->s.origin, projectile->timeDelta ) & MASK_WATER ) ? qtrue : qfalse;

	// ffs : hack for the plasmagun
#ifdef PLASMAHACK
	if( projectile->s.type == ET_PLASMA )
		W_Plasma_Backtrace( projectile, plasma_hack_start );
#endif
}

/*
* G_ProjectileTimePrestep
*/
static void G_ProjectileTimePrestep( edict_t *projectile, int timeOffset )
{
	if( projectile->movetype != MOVETYPE_TOSS && projectile->movetype != MOVETYPE_LINEARPROJECTILE
		&& projectile->movetype != MOVETYPE_BOUNCE && projectile->movetype != MOVETYPE_BOUNCEGRENADE )
		return;

	if( timeOffset <= 0 )
		return;

	if( projectile->movetype != MOVETYPE_LINEARPROJECTILE )
	{
		vec3_t distVec;

		VectorScale( projectile->velocity, (float)timeOffset * 0.001f, distVec );
		G_ProjectileDistancePrestep( projectile, VectorLength( distVec ) );
		return;
	}

	projectile->s.linearProjectileTimeStamp -= timeOffset;
	SV_Physics_LinearProjectile( projectile );
}

/*
* G_CheckBladeAutoAttack
*/
qboolean G_CheckBladeAutoAttack( player_state_t *playerState )
{
	vec3_t origin, dir, end;
	trace_t trace;
	edict_t *targ, *player;
	gs_weapon_definition_t *weapondef = GS_GetWeaponDef( WEAP_GUNBLADE );

	if( playerState->POVnum <= 0 || (int)playerState->POVnum > gs.maxclients )
		return qfalse;

	VectorCopy( playerState->pmove.origin, origin );
	origin[2] += playerState->viewheight;
	AngleVectors( playerState->viewangles, dir, NULL, NULL );
	VectorMA( origin, weapondef->firedef_weak.timeout, dir, end );

	player = game.edicts + playerState->POVnum;
	if( !player->r.client )
		return qfalse;

	// check for a player to touch
	G_Trace4D( &trace, origin, vec3_origin, vec3_origin, end, player, CONTENTS_BODY, player->r.client->timeDelta );
	if( trace.ent == -1 )
		return qfalse;

	targ = game.edicts + trace.ent;
	if( !targ->takedamage || targ->s.type != ET_PLAYER )
		return qfalse;

	if( GS_RaceGametype() && targ->team == player->team )
		return qfalse;

	if( GS_IsTeamDamage( &targ->s, &player->s ) )
		return qfalse;

	return qtrue;
}

/*
* G_Fire_Gunblade_Knife
*/
static edict_t *G_Fire_Gunblade_Knife( vec3_t origin, vec3_t angles, firedef_t *firedef, edict_t *owner )
{
	int range, knockback, stun, mod;
	float damage;
	int timeDelta;

	timeDelta = 0;
	if( owner && owner->r.client )
		timeDelta = owner->r.client->timeDelta;

	mod = ( firedef->fire_mode == FIRE_MODE_STRONG ) ? MOD_GUNBLADE_S : MOD_GUNBLADE_W;
	range = firedef->timeout;
	damage = firedef->damage;
	knockback = firedef->knockback;
	stun = firedef->stun;

	if( is_quad )
	{
		damage *= QUAD_DAMAGE_SCALE;
		knockback *= QUAD_KNOCKBACK_SCALE;
	}

	W_Fire_Blade( owner, range, origin, angles, damage, knockback, stun, mod, timeDelta );

	return NULL;
}

/*
* G_LocalSpread
*/
static void G_LocalSpread( vec3_t angles, int spread, int seed )
{
	float r, u;
	vec3_t axis[3], dir;
	double alpha;
	double s;

	if( spread <= 0 )
		return;

	seed &= 255;

	alpha = M_PI * Q_crandom( &seed ); // [-PI ..+PI]
	s = fabs( Q_crandom( &seed ) ); // [0..1]

	r = s * cos( alpha ) * spread;
	u = s * sin( alpha ) * spread;

	AngleVectors( angles, axis[0], axis[1], axis[2] );

	VectorMA( vec3_origin, 8192, axis[0], dir );
	VectorMA( dir, r, axis[1], dir );
	VectorMA( dir, u, axis[2], dir );

	VecToAngles( dir, angles );
}

/*
* G_Fire_Gunblade_Blast
*/
static edict_t *G_Fire_Gunblade_Blast( vec3_t origin, vec3_t angles, firedef_t *firedef, edict_t *owner, int seed )
{
	int speed, knockback, stun, minDamage, minKnockback, radius, mod;
	float damage;
	int timeDelta;
	float power;

	timeDelta = 0;
	if( owner && owner->r.client )
		timeDelta = owner->r.client->timeDelta;

	if( firedef->spread )
		G_LocalSpread( angles, firedef->spread, seed );

	mod = ( firedef->fire_mode == FIRE_MODE_STRONG ) ? MOD_GUNBLADE_S : MOD_GUNBLADE_W;
	speed = firedef->speed;
	damage = firedef->damage;
	knockback = firedef->knockback;
	stun = firedef->stun;
	minDamage = firedef->mindamage;
	minKnockback = firedef->minknockback;
	radius = firedef->splash_radius;

	// hackish : scale by power fraction
	if( owner && owner->r.client )
	{
		power = (float)owner->r.client->ps.inventory[firedef->ammo_id] /(float)firedef->ammo_max;
		damage *= power;
		knockback *= power;
		radius *= power;
	}

	if( is_quad )
	{
		damage *= QUAD_DAMAGE_SCALE;
		knockback *= QUAD_KNOCKBACK_SCALE;
	}

	// scale by power fraction
	if( damage < firedef->mindamage )
		damage = firedef->mindamage;
	if( minDamage < firedef->mindamage )
		minDamage = firedef->mindamage;

	// hackish : every shot wastes all player power
	if( owner && owner->r.client && firedef->ammo_id )
	{
		owner->r.client->ps.inventory[firedef->ammo_id] = min( firedef->ammo_pickup * 2, firedef->ammo_max );
		owner->r.client->resp.gunbladeChargeTimeStamp = level.time;
	}

	return W_Fire_GunbladeBlast( owner, origin, angles, damage, minKnockback, knockback, stun, minDamage,
		radius, speed, firedef->timeout, mod, timeDelta );
}

/*
* G_Fire_Rocket
*/
static edict_t *G_Fire_Rocket( vec3_t origin, vec3_t angles, firedef_t *firedef, edict_t *owner, int seed )
{
	int speed, knockback, stun, minDamage, minKnockback, radius, mod;
	float damage;
	int timeDelta;

	// FIXME2: Rockets go slower underwater, do this at the actual rocket firing function

	timeDelta = 0;
	if( owner && owner->r.client )
		timeDelta = owner->r.client->timeDelta;

	if( firedef->spread )
		G_LocalSpread( angles, firedef->spread, seed );

	mod = ( firedef->fire_mode == FIRE_MODE_STRONG ) ? MOD_ROCKET_S : MOD_ROCKET_W;
	speed = firedef->speed;
	damage = firedef->damage;
	knockback = firedef->knockback;
	stun = firedef->stun;
	minDamage = firedef->mindamage;
	minKnockback = firedef->minknockback;
	radius = firedef->splash_radius;

	if( is_quad )
	{
		damage *= QUAD_DAMAGE_SCALE;
		knockback *= QUAD_KNOCKBACK_SCALE;
	}

	return W_Fire_Rocket( owner, origin, angles, speed, damage, minKnockback, knockback, stun, minDamage,
		radius, firedef->timeout, mod, timeDelta );
}

/*
* G_Fire_Machinegun
*/
static edict_t *G_Fire_Machinegun( vec3_t origin, vec3_t angles, firedef_t *firedef, edict_t *owner, int seed )
{
	int range, knockback, stun, mod;
	float damage;
	int timeDelta;

	if( owner && owner->r.client )
		timeDelta = owner->r.client->timeDelta;
	else
		timeDelta = 0;

	mod = ( firedef->fire_mode == FIRE_MODE_STRONG ) ? MOD_MACHINEGUN_S : MOD_MACHINEGUN_W;
	range = firedef->timeout;
	damage = firedef->damage;
	knockback = firedef->knockback;
	stun = firedef->stun;

	if( is_quad )
	{
		damage *= QUAD_DAMAGE_SCALE;
		knockback *= QUAD_KNOCKBACK_SCALE;
	}

	W_Fire_Bullet( owner, origin, angles, seed, range, firedef->spread,
		damage, knockback, stun, mod, timeDelta );

	return NULL;
}

/*
* G_Fire_Riotgun
*/
static edict_t *G_Fire_Riotgun( vec3_t origin, vec3_t angles, firedef_t *firedef, edict_t *owner, int seed )
{
	int range, knockback, stun, mod;
	float damage;
	int timeDelta;

	if( owner && owner->r.client )
		timeDelta = owner->r.client->timeDelta;
	else
		timeDelta = 0;

	mod = ( firedef->fire_mode == FIRE_MODE_STRONG ) ? MOD_RIOTGUN_S : MOD_RIOTGUN_W;
	range = firedef->timeout;
	damage = firedef->damage;
	knockback = firedef->knockback;
	stun = firedef->stun;

	if( is_quad )
	{
		damage *= QUAD_DAMAGE_SCALE;
		knockback *= QUAD_KNOCKBACK_SCALE;
	}

	W_Fire_Riotgun( owner, origin, angles, seed, range, firedef->spread, firedef->projectile_count, 
		damage, knockback, stun, mod, timeDelta );

	return NULL;
}

/*
* G_Fire_Grenade
*/
static edict_t *G_Fire_Grenade( vec3_t origin, vec3_t angles, firedef_t *firedef, edict_t *owner, int seed )
{
	int speed, minKnockback, knockback, stun, minDamage, radius, mod;
	float damage;
	int timeDelta;

	// FIXME2: projectiles go slower underwater, do this at the actual firing function

	timeDelta = 0;
	if( owner && owner->r.client )
		timeDelta = owner->r.client->timeDelta;

	if( firedef->spread )
		G_LocalSpread( angles, firedef->spread, seed );

	mod = ( firedef->fire_mode == FIRE_MODE_STRONG ) ? MOD_GRENADE_S : MOD_GRENADE_W;
	speed = firedef->speed;
	damage = firedef->damage;
	knockback = firedef->knockback;
	stun = firedef->stun;
	minDamage = firedef->mindamage;
	minKnockback = firedef->minknockback;
	radius = firedef->splash_radius;

	if( is_quad )
	{
		damage *= QUAD_DAMAGE_SCALE;
		minDamage *= QUAD_DAMAGE_SCALE;
		knockback *= QUAD_KNOCKBACK_SCALE;
	}

	return W_Fire_Grenade( owner, origin, angles, speed, damage, minKnockback, knockback, stun,
		minDamage, radius, firedef->timeout, mod, timeDelta, qtrue );
}

/*
* G_Fire_Plasma
*/
static edict_t *G_Fire_Plasma( vec3_t origin, vec3_t angles, firedef_t *firedef, edict_t *owner, int seed )
{
	int speed, knockback, stun, minDamage, minKnockback, radius, mod;
	float damage;
	int timeDelta;

	// FIXME2: projectiles go slower underwater, do this at the actual firing function

	timeDelta = 0;
	if( owner && owner->r.client )
		timeDelta = owner->r.client->timeDelta;

	if( firedef->spread )
		G_LocalSpread( angles, firedef->spread, seed );

	mod = ( firedef->fire_mode == FIRE_MODE_STRONG ) ? MOD_PLASMA_S : MOD_PLASMA_W;
	speed = firedef->speed;
	damage = firedef->damage;
	knockback = firedef->knockback;
	stun = firedef->stun;
	minDamage = firedef->mindamage;
	minKnockback = firedef->minknockback;
	radius = firedef->splash_radius;

	if( is_quad )
	{
		damage *= QUAD_DAMAGE_SCALE;
		knockback *= QUAD_KNOCKBACK_SCALE;
	}

	return W_Fire_Plasma( owner, origin, angles, damage, minKnockback, knockback, stun, minDamage, radius,
		speed, firedef->timeout, mod, timeDelta );
}

/*
* G_Fire_Lasergun
*/
static edict_t *G_Fire_Lasergun( vec3_t origin, vec3_t angles, firedef_t *firedef, edict_t *owner, int seed )
{
	int range, knockback, stun, mod;
	float damage;
	int timeDelta;
	vec3_t end;

	timeDelta = 0;
	if( owner && owner->r.client )
		timeDelta = owner->r.client->timeDelta;

	if( firedef->spread )
		G_LocalSpread( angles, firedef->spread, seed );

	mod = ( firedef->fire_mode == FIRE_MODE_STRONG ) ? MOD_LASERGUN_S : MOD_LASERGUN_W;
	range = firedef->timeout;
	damage = firedef->damage;
	knockback = firedef->knockback;
	stun = firedef->stun;

	if( is_quad )
	{
		damage *= QUAD_DAMAGE_SCALE;
		knockback *= QUAD_KNOCKBACK_SCALE;
	}

	// no need to continue if strong mode
	if( firedef->fire_mode == FIRE_MODE_STRONG )
	{
		return W_Fire_Lasergun( owner, origin, angles, damage, knockback, stun, range, mod, timeDelta );
	}

	// find the endpoint into the ones in the backup trail
	if( !owner || !owner->r.client )
	{
		vec3_t dir;
		AngleVectors( angles, dir, NULL, NULL );
		VectorMA( origin, range, dir, end );
	}
	else if( !G_GetLaserbeamPoint( &owner->r.client->resp.trail, &owner->r.client->ps, owner->r.client->ucmd.serverTimeStamp, end ) )
	{
		vec3_t dir;
		AngleVectors( angles, dir, NULL, NULL );
		VectorMA( origin, range, dir, end );
	}

	return W_Fire_Lasergun_Weak( owner, origin, end, damage, knockback, stun, range,
		mod, timeDelta );
}

/*
* G_Fire_WeakBolt
*/
static edict_t *G_Fire_WeakBolt( vec3_t origin, vec3_t angles, firedef_t *firedef, edict_t *owner, int seed )
{
	int speed, maxknockback, minknockback, stun, mod;
	float maxdamage, mindamage;
	int timeDelta;

	// FIXME2: projectiles go slower underwater, do this at the actual firing function

	timeDelta = 0;
	if( owner && owner->r.client )
		timeDelta = owner->r.client->timeDelta;

	if( firedef->spread )
		G_LocalSpread( angles, firedef->spread, seed );

	mod = ( firedef->fire_mode == FIRE_MODE_STRONG ) ? MOD_ELECTROBOLT_S : MOD_ELECTROBOLT_W;
	speed = firedef->speed;
	maxdamage = firedef->damage;
	mindamage = firedef->mindamage;
	maxknockback = firedef->knockback;
	minknockback = firedef->minknockback;
	stun = firedef->stun;

	if( is_quad )
	{
		maxdamage *= QUAD_DAMAGE_SCALE;
		maxknockback *= QUAD_KNOCKBACK_SCALE;
	}
#ifdef ELECTROBOLT_TEST
	W_Fire_Electrobolt_Combined( owner, origin, angles, maxdamage, mindamage,
		maxknockback, minknockback, stun, firedef->timeout, mod, timeDelta );

	return NULL;
#else
	return W_Fire_Electrobolt_Weak( owner, origin, angles, speed, maxdamage, minknockback, maxknockback, stun,
		firedef->timeout, mod, timeDelta );
#endif
}

/*
* G_Fire_StrongBolt
*/
static edict_t *G_Fire_StrongBolt( vec3_t origin, vec3_t angles, firedef_t *firedef, edict_t *owner, int seed )
{
	int minDamageRange, stun, mod;
	float maxdamage, mindamage, maxknockback, minknockback;
	int timeDelta;

	timeDelta = 0;
	if( owner && owner->r.client )
		timeDelta = owner->r.client->timeDelta;

	if( firedef->spread )
		G_LocalSpread( angles, firedef->spread, seed );

	mod = ( firedef->fire_mode == FIRE_MODE_STRONG ) ? MOD_ELECTROBOLT_S : MOD_ELECTROBOLT_W;
	minDamageRange = firedef->timeout;
	maxdamage = firedef->damage;
	mindamage = firedef->mindamage;
	maxknockback = firedef->knockback;
	minknockback = firedef->minknockback;
	stun = firedef->stun;

	if( is_quad )
	{
		maxdamage *= QUAD_DAMAGE_SCALE;
		maxknockback *= QUAD_KNOCKBACK_SCALE;
	}
#ifdef ELECTROBOLT_TEST
	W_Fire_Electrobolt_FullInstant( owner, origin, angles, maxdamage, mindamage,
		maxknockback, minknockback, stun, ELECTROBOLT_RANGE, minDamageRange, mod, timeDelta );
#else
	W_Fire_Electrobolt_Combined( owner, origin, angles, maxdamage, mindamage,
		maxknockback, minknockback, stun, range, mod, timeDelta );
#endif
	return NULL;
}

/*
* G_Fire_Instagun
*/
static edict_t *G_Fire_Instagun( vec3_t origin, vec3_t angles, firedef_t *firedef, edict_t *owner, int seed )
{
	int range, knockback, stun, radius, mod;
	float damage;
	int timeDelta;

	timeDelta = 0;
	if( owner && owner->r.client )
		timeDelta = owner->r.client->timeDelta;

	if( firedef->spread )
		G_LocalSpread( angles, firedef->spread, seed );

	mod = ( firedef->fire_mode == FIRE_MODE_STRONG ) ? MOD_INSTAGUN_S : MOD_INSTAGUN_W;
	range = firedef->timeout;
	damage = firedef->damage;
	knockback = firedef->knockback;
	stun = firedef->stun;
	radius = firedef->splash_radius;

	if( is_quad )
	{
		damage *= QUAD_DAMAGE_SCALE;
		knockback *= QUAD_KNOCKBACK_SCALE;
	}

	W_Fire_Instagun( owner, origin, angles, damage,
		knockback, stun, radius, range, MOD_INSTAGUN_S, timeDelta );

	return NULL;
}

/*
* G_FireWeapon
*/
void G_FireWeapon( edict_t *ent, int parm )
{
	gs_weapon_definition_t *weapondef;
	firedef_t *firedef;
	edict_t *projectile;
	vec3_t origin, angles;
	vec3_t viewoffset = { 0, 0, 0 };
	int ucmdSeed;
	// racesow
	float prestep; 
	// !racesow

	weapondef = GS_GetWeaponDef( ( parm & ~EV_INVERSE ) );
	firedef = ( parm & EV_INVERSE ) ? &weapondef->firedef : &weapondef->firedef_weak;

	// find this shot projection source
	if( ent->r.client )
	{
		viewoffset[2] += ent->r.client->ps.viewheight;
		VectorCopy( ent->r.client->ps.viewangles, angles );
		is_quad = ( ent->r.client->ps.inventory[POWERUP_QUAD] > 0 );
		ucmdSeed = ent->r.client->ucmd.serverTimeStamp & 255;
	}
	else
	{
		VectorCopy( ent->s.angles, angles );
		is_quad = qfalse;
		ucmdSeed = rand() & 255;
	}

	VectorAdd( ent->s.origin, viewoffset, origin );

	// racesow
	prestep=g_projectile_prestep->value; 
	// !racesow
	

	// shoot 

	projectile = NULL;

	switch( weapondef->weapon_id )
	{
	default:
	case WEAP_NONE:
		break;

	case WEAP_GUNBLADE:
		if( firedef->fire_mode == FIRE_MODE_STRONG )
			projectile = G_Fire_Gunblade_Blast( origin, angles, firedef, ent, ucmdSeed );
		else
			projectile = G_Fire_Gunblade_Knife( origin, angles, firedef, ent );
		break;

	case WEAP_MACHINEGUN:
		projectile = G_Fire_Machinegun( origin, angles, firedef, ent, ucmdSeed );
		break;

	case WEAP_RIOTGUN:
		projectile = G_Fire_Riotgun( origin, angles, firedef, ent, ucmdSeed );
		break;

	case WEAP_GRENADELAUNCHER:
		projectile = G_Fire_Grenade( origin, angles, firedef, ent, ucmdSeed );
		// racesow
		if( GS_RaceGametype() )
			//prestep/=2; // racesow 0.42 had default prestep=48 and genade prestep=24
			prestep=trap_Cvar_Get( "rs_grenade_prestep", "90", CVAR_ARCHIVE )->integer;
		// !racesow
		break;

	case WEAP_ROCKETLAUNCHER:
		projectile = G_Fire_Rocket( origin, angles, firedef, ent, ucmdSeed );
		// racesow
		if( GS_RaceGametype() )
			//prestep=0; // racesow 0.42 had rocket prestep=0
			prestep=trap_Cvar_Get( "rs_rocket_prestep", "90", CVAR_ARCHIVE )->integer;
		// !racesow
		break;
	case WEAP_PLASMAGUN:
		projectile = G_Fire_Plasma( origin, angles, firedef, ent, ucmdSeed );
		// racesow
		if( GS_RaceGametype() )
			//prestep*=2/3; // racesow 0.42 had plasma prestep=32
			prestep=trap_Cvar_Get( "rs_plasma_prestep", "90", CVAR_ARCHIVE )->integer;
		// !racesow
		break;

	case WEAP_LASERGUN:
		projectile = G_Fire_Lasergun( origin, angles, firedef, ent, ucmdSeed );
		break;

	case WEAP_ELECTROBOLT:
		projectile = G_Fire_StrongBolt( origin, angles, firedef, ent, ucmdSeed );
		break;

	case WEAP_INSTAGUN:
		projectile = G_Fire_Instagun( origin, angles, firedef, ent, ucmdSeed );
		break;
	}

	// add stats
	if( ent->r.client && weapondef->weapon_id != WEAP_NONE ) 
		ent->r.client->level.stats.accuracy_shots[firedef->ammo_id - AMMO_GUNBLADE] += firedef->projectile_count;

	if( projectile )
	{
		//if( projectile->s.linearProjectile ) // convert distance to time for linear projectiles
		//	G_ProjectileTimePrestep( projectile, 1000.0f * ( g_projectile_prestep->value / VectorLengthFast( projectile->velocity ) ) );
		//else
			// racesow: modified prestep
			G_ProjectileDistancePrestep( projectile, prestep );
			// !racesow
	}

	// racesow: enable skipping no_antilag if rs_rocket_antilag is 1
		if ( GS_RaceGametype() && ((trap_Cvar_Get( "rs_rocket_antilag", "1", CVAR_ARCHIVE )->integer==1 && projectile->s.type == ET_ROCKET) \
								|| (trap_Cvar_Get( "rs_plasma_antilag", "1", CVAR_ARCHIVE )->integer==1 && projectile->s.type == ET_PLASMA) \
								|| (trap_Cvar_Get( "rs_grenade_antilag", "1", CVAR_ARCHIVE )->integer==1 && projectile->s.type == ET_GRENADE)))
			return;
	// !racesow 

#ifdef NO_ROCKET_ANTILAG
	// hack for disabling antilag on rockets
	// racesow: added plasma and grenades
	if( projectile && (projectile->s.type == ET_ROCKET || projectile->s.type == ET_PLASMA || projectile->s.type == ET_GRENADE) )
	// !racesow
	{
		int timeOffset;

		timeOffset = -projectile->timeDelta;
		projectile->timeDelta = 0;
		if( projectile->s.linearProjectile )
			projectile->s.modelindex2 = 0;

		G_ProjectileTimePrestep( projectile, timeOffset );
	}
#endif
}

