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

// commented out to make gcc happy
#if 0
//==================
//W_Fire_Lead
//the seed is important to be as pointer for cgame prediction accuracy
//==================
static void W_Fire_Lead( edict_t *self, vec3_t start, vec3_t aimdir, vec3_t axis[3], int damage, 
						int knockback, int stun, int hspread, int vspread, int *seed, int dflags,
						int mod, int timeDelta )
{
	trace_t	tr;
	vec3_t dir;
	vec3_t end;
	float r;
	float u;
	vec3_t water_start;
	int content_mask = MASK_SHOT | MASK_WATER;

	G_Trace4D( &tr, self->s.origin, NULL, NULL, start, self, MASK_SHOT, timeDelta );
	if( !( tr.fraction < 1.0 ) )
	{
#if 1
		// circle
		double alpha = M_PI * Q_crandom( seed ); // [-PI ..+PI]
		double s = fabs( Q_crandom( seed ) ); // [0..1]
		r = s * cos( alpha ) * hspread;
		u = s * sin( alpha ) * vspread;
#else
		// square
		r = Q_crandom( seed ) * hspread;
		u = Q_crandom( seed ) * vspread;
#endif
		VectorMA( start, 8192, axis[0], end );
		VectorMA( end, r, axis[1], end );
		VectorMA( end, u, axis[2], end );

		if( G_PointContents4D( start, timeDelta ) & MASK_WATER )
		{
			VectorCopy( start, water_start );
			content_mask &= ~MASK_WATER;
		}

		G_Trace4D( &tr, start, NULL, NULL, end, self, content_mask, timeDelta );

		// see if we hit water
		if( tr.contents & MASK_WATER )
		{
			VectorCopy( tr.endpos, water_start );

			if( !VectorCompare( start, tr.endpos ) )
			{
				vec3_t forward, right, up;

				// change bullet's course when it enters water
				VectorSubtract( end, start, dir );
				VecToAngles( dir, dir );
				AngleVectors( dir, forward, right, up );
#if 1
				// circle
				alpha = M_PI *Q_crandom( seed ); // [-PI ..+PI]
				s = fabs( Q_crandom( seed ) ); // [0..1]
				r = s *cos( alpha )*hspread*1.5;
				u = s *sin( alpha )*vspread*1.5;
#else
				r = Q_crandom( seed ) * hspread * 2;
				u = Q_crandom( seed ) * vspread * 2;
#endif
				VectorMA( water_start, 8192, forward, end );
				VectorMA( end, r, right, end );
				VectorMA( end, u, up, end );
			}

			// re-trace ignoring water this time
			G_Trace4D( &tr, water_start, NULL, NULL, end, self, MASK_SHOT, timeDelta );
		}
	}

	// send gun puff / flash
	if( tr.fraction < 1.0 && tr.ent != -1 )
	{
		if( game.edicts[tr.ent].takedamage )
		{
			G_TakeDamage( &game.edicts[tr.ent], self, self, aimdir, aimdir, tr.endpos, tr.plane.normal, damage, knockback, stun, dflags, mod );
		}
		else
		{
			if( !( tr.surfFlags & SURF_NOIMPACT ) )
			{
			}
		}
	}
}
#endif

#define DIRECAIRTHIT_DAMAGE_BONUS 5
#define DIRECTHIT_DAMAGE_BONUS 0

enum {
	PROJECTILE_TOUCH_NOT = 0,
	PROJECTILE_TOUCH_DIRECTHIT,
	PROJECTILE_TOUCH_DIRECTAIRHIT,
	PROJECTILE_TOUCH_DIRECTSPLASH // treat direct hits as pseudo-splash impacts
};

/*
* 
* - We will consider direct impacts as splash when the player is on the ground and the hit very close to the ground
*/
int G_Projectile_HitStyle( edict_t *projectile, edict_t *target )
{
	trace_t trace;
	vec3_t end;
	qboolean atGround = qfalse;
	edict_t *attacker;
#define AIRHIT_MINHEIGHT 64

	// don't hurt owner for the first second
	if( target == projectile->r.owner && target != world )
	{
		if( !g_projectile_touch_owner->integer ||
			( g_projectile_touch_owner->integer && projectile->timeStamp + 1000 > level.time ) )
			return PROJECTILE_TOUCH_NOT;
	}

	if( !target->takedamage || ISBRUSHMODEL( target->s.modelindex ) )
		return PROJECTILE_TOUCH_DIRECTHIT;

	if( target->waterlevel > 1 )
		return PROJECTILE_TOUCH_DIRECTHIT; // water hits are direct but don't count for awards

	//racesow
	    if( projectile->r.owner->r.client && target->r.client )
	    {
	        if( GS_RaceGametype() && target->team == projectile->r.owner->team && !level.gametype.playerInteraction )
	            return PROJECTILE_TOUCH_NOT;
	    }
	//!racesow

	attacker = ( projectile->r.owner && projectile->r.owner->r.client ) ? projectile->r.owner : NULL;

	// see if the target is at ground or a less than a step of height
	if( target->groundentity )
		atGround = qtrue;
	else
	{
		VectorCopy( target->s.origin, end );
		end[2] -= STEPSIZE;

		G_Trace4D( &trace, target->s.origin, target->r.mins, target->r.maxs, end, target, MASK_DEADSOLID, 0 );
		if( ( trace.ent != -1 || trace.startsolid ) && ISWALKABLEPLANE( &trace.plane ) )
			atGround = qtrue;
	}

	if( atGround )
	{
		// when the player is at ground we will consider a direct hit only when
		// the hit is 16 units above the feet
		if( projectile->s.origin[2] <= 16 + target->s.origin[2] + target->r.mins[2] )
			return PROJECTILE_TOUCH_DIRECTSPLASH;
	}
	else
	{
		// it's direct hit, but let's see if it's airhit
		VectorCopy( target->s.origin, end );
		end[2] -= AIRHIT_MINHEIGHT;

		G_Trace4D( &trace, target->s.origin, target->r.mins, target->r.maxs, end, target, MASK_DEADSOLID, 0 );
		if( ( trace.ent != -1 || trace.startsolid ) && ISWALKABLEPLANE( &trace.plane ) )
		{
			// add directhit and airhit to awards counter
			if( attacker && !GS_IsTeamDamage( &attacker->s, &target->s ) && G_ModToAmmo( projectile->style ) != AMMO_NONE )
			{
				projectile->r.owner->r.client->level.stats.accuracy_hits_direct[G_ModToAmmo( projectile->style )-AMMO_GUNBLADE]++;
				teamlist[projectile->r.owner->s.team].stats.accuracy_hits_direct[G_ModToAmmo( projectile->style )-AMMO_GUNBLADE]++;

				projectile->r.owner->r.client->level.stats.accuracy_hits_air[G_ModToAmmo( projectile->style )-AMMO_GUNBLADE]++;
				teamlist[projectile->r.owner->s.team].stats.accuracy_hits_air[G_ModToAmmo( projectile->style )-AMMO_GUNBLADE]++;
			}

			return PROJECTILE_TOUCH_DIRECTAIRHIT;
		}
	}

	// add directhit to awards counter
	if( attacker && !GS_IsTeamDamage( &attacker->s, &target->s ) && G_ModToAmmo( projectile->style ) != AMMO_NONE )
	{
		projectile->r.owner->r.client->level.stats.accuracy_hits_direct[G_ModToAmmo( projectile->style )-AMMO_GUNBLADE]++;
		teamlist[projectile->r.owner->s.team].stats.accuracy_hits_direct[G_ModToAmmo( projectile->style )-AMMO_GUNBLADE]++;
	}

	return PROJECTILE_TOUCH_DIRECTHIT;

#undef AIRHIT_MINHEIGHT
}

//==================
//W_Touch_Projectile - Generic projectile touch func. Only for replacement in tests
//==================
static void W_Touch_Projectile( edict_t *ent, edict_t *other, cplane_t *plane, int surfFlags )
{
	vec3_t dir, normal;
	int hitType;

	if( surfFlags & SURF_NOIMPACT )
	{
		G_FreeEdict( ent );
		return;
	}

	hitType = G_Projectile_HitStyle( ent, other );
	if( hitType == PROJECTILE_TOUCH_NOT )
		return;

	if( other->takedamage )
	{
		VectorNormalize2( ent->velocity, dir );

		if( hitType == PROJECTILE_TOUCH_DIRECTSPLASH ) // use hybrid direction from splash and projectile
		{
			G_SplashFrac4D( ENTNUM( other ), ent->s.origin, ent->projectileInfo.radius, dir, NULL, NULL, ent->timeDelta );
		}
		else
		{
			VectorNormalize2( ent->velocity, dir );
		}

		G_TakeDamage( other, ent, ent->r.owner, dir, ent->velocity, ent->s.origin, ent->projectileInfo.maxDamage, ent->projectileInfo.maxKnockback, ent->projectileInfo.stun, 0, ent->style );
	}

	G_TakeRadiusDamage( ent, ent->r.owner, plane, other, MOD_EXPLOSIVE );

	if( !plane->normal )
		VectorSet( normal, 0, 0, 1 );
	else
		VectorCopy( plane->normal, normal );
	
	G_Gametype_ScoreEvent( NULL, "projectilehit", va( "%i %i %f %f %f", ent->s.number, surfFlags, normal[0], normal[1], normal[2] ) );
}

//==================
//W_Fire_LinearProjectile - Spawn a generic linear projectile without a model, touch func, sound nor mod
//==================
static edict_t *W_Fire_LinearProjectile( edict_t *self, vec3_t start, vec3_t angles, int speed,
										float damage, int minKnockback, int maxKnockback, int stun, int minDamage, int radius, int timeout, int timeDelta )
{
	edict_t	*projectile;
	vec3_t dir;

	projectile = G_Spawn();
	VectorCopy( start, projectile->s.origin );
	VectorCopy( start, projectile->s.old_origin );
	VectorCopy( start, projectile->olds.origin );

	VectorCopy( angles, projectile->s.angles );
	AngleVectors( angles, dir, NULL, NULL );
	VectorScale( dir, speed, projectile->velocity );
	GS_SnapVelocity( projectile->velocity );

	projectile->movetype = MOVETYPE_LINEARPROJECTILE;
	projectile->s.linearProjectile = qtrue;

	projectile->r.solid = SOLID_YES;
	projectile->r.clipmask = ( GS_RaceGametype() && !level.gametype.playerInteraction ) ? MASK_SOLID : MASK_SHOT; //racesow projectiles interact with others

	projectile->r.svflags = SVF_PROJECTILE;
	// enable me when drawing exception is added to cgame
	projectile->r.svflags |= SVF_TRANSMITORIGIN2|SVF_NOCULLATORIGIN2;
	VectorClear( projectile->r.mins );
	VectorClear( projectile->r.maxs );
	projectile->s.modelindex = 0;
	projectile->r.owner = self;
	projectile->s.ownerNum = ENTNUM( self );
	projectile->touch = W_Touch_Projectile; //generic one. Should be replaced after calling this func
	projectile->nextThink = level.time + timeout;
	projectile->think = G_FreeEdict;
	projectile->classname = NULL; // should be replaced after calling this func.
	projectile->style = 0;
	projectile->s.sound = 0;
	projectile->timeStamp = level.time;
	projectile->s.linearProjectileTimeStamp = game.serverTime;
	projectile->timeDelta = timeDelta;

	projectile->projectileInfo.minDamage = min( minDamage, damage );
	projectile->projectileInfo.maxDamage = damage;
	projectile->projectileInfo.minKnockback = min( minKnockback, maxKnockback );
	projectile->projectileInfo.maxKnockback = maxKnockback;
	projectile->projectileInfo.stun = stun;
	projectile->projectileInfo.radius = radius;

	GClip_LinkEntity( projectile );

	// update some data required for the transmission
	VectorCopy( projectile->velocity, projectile->s.linearProjectileVelocity );
	projectile->s.team = self->s.team;
	projectile->s.modelindex2 = ( abs( timeDelta ) > 255 ) ? 255 : (unsigned int)abs( timeDelta );
	return projectile;
}

//==================
//W_Fire_TossProjectile - Spawn a generic projectile without a model, touch func, sound nor mod
//==================
static edict_t *W_Fire_TossProjectile( edict_t *self, vec3_t start, vec3_t angles, int speed,
									  float damage, int minKnockback, int maxKnockback, int stun, int minDamage, int radius, int timeout, int timeDelta )
{
	edict_t	*projectile;
	vec3_t dir;

	projectile = G_Spawn();
	VectorCopy( start, projectile->s.origin );
	VectorCopy( start, projectile->s.old_origin );
	VectorCopy( start, projectile->olds.origin );

	VectorCopy( angles, projectile->s.angles );
	AngleVectors( angles, dir, NULL, NULL );
	VectorScale( dir, speed, projectile->velocity );
	GS_SnapVelocity( projectile->velocity );

	projectile->movetype = MOVETYPE_BOUNCEGRENADE;

	// make missile fly through players in race
	if( GS_RaceGametype() && !level.gametype.playerInteraction )
		projectile->r.clipmask = MASK_SOLID;
	else
		projectile->r.clipmask = MASK_SHOT;

	projectile->r.solid = SOLID_YES;
	projectile->r.svflags = SVF_PROJECTILE;
	VectorClear( projectile->r.mins );
	VectorClear( projectile->r.maxs );
	//projectile->s.modelindex = trap_ModelIndex ("models/objects/projectile/plasmagun/proj_plasmagun2.md3");
	projectile->s.modelindex = 0;
	projectile->r.owner = self;
	projectile->touch = W_Touch_Projectile; //generic one. Should be replaced after calling this func
	projectile->nextThink = level.time + timeout;
	projectile->think = G_FreeEdict;
	projectile->classname = NULL; // should be replaced after calling this func.
	projectile->style = 0;
	projectile->s.sound = 0;
	projectile->timeStamp = level.time;
	projectile->timeDelta = timeDelta;
	projectile->s.team = self->s.team;

	projectile->projectileInfo.minDamage = min( minDamage, damage );
	projectile->projectileInfo.maxDamage = damage;
	projectile->projectileInfo.minKnockback = min( minKnockback, maxKnockback );
	projectile->projectileInfo.maxKnockback = maxKnockback;
	projectile->projectileInfo.stun = stun;
	projectile->projectileInfo.radius = radius;

	GClip_LinkEntity( projectile );

	return projectile;
}


//	------------ the actual weapons --------------


//==================
//W_Fire_Blade
//==================
void W_Fire_Blade( edict_t *self, int range, vec3_t start, vec3_t angles, float damage, int knockback, int stun, int mod, int timeDelta )
{
	edict_t *event, *other = NULL;
	vec3_t end;
	trace_t	trace;
	int mask = MASK_SHOT;
	vec3_t dir;
	int dmgflags = 0;

	if( GS_Instagib() )
		damage = 9999;

	AngleVectors( angles, dir, NULL, NULL );
	VectorMA( start, range, dir, end );

	if( GS_RaceGametype() && !level.gametype.playerInteraction )//racesow make projectile interact with others
		mask = MASK_SOLID;

	G_Trace4D( &trace, start, NULL, NULL, end, self, MASK_SHOT, timeDelta );
	if( trace.ent == -1 )  //didn't touch anything
		return;

	// find out what touched
	other = &game.edicts[trace.ent];
	if( !other->takedamage ) // it was the world
	{
		// wall impact
		VectorMA( trace.endpos, -0.02, dir, end );
		event = G_SpawnEvent( EV_BLADE_IMPACT, 0, end );
		event->s.ownerNum = ENTNUM( self );
		VectorScale( trace.plane.normal, 1024, event->s.origin2 );
		event->r.svflags = SVF_TRANSMITORIGIN2|SVF_NOCULLATORIGIN2;
		return;
	}

	// it was a player
	G_TakeDamage( other, self, self, dir, dir, other->s.origin, damage, knockback, stun, dmgflags, mod );
}

//==================
//W_Touch_GunbladeBlast
//==================
static void W_Touch_GunbladeBlast( edict_t *ent, edict_t *other, cplane_t *plane, int surfFlags )
{
	vec3_t dir;
	int hitType;

	if( surfFlags & SURF_NOIMPACT )
	{
		G_FreeEdict( ent );
		return;
	}

	hitType = G_Projectile_HitStyle( ent, other );
	if( hitType == PROJECTILE_TOUCH_NOT )
		return;

	if( other->takedamage )
	{
		VectorNormalize2( ent->velocity, dir );

		if( hitType == PROJECTILE_TOUCH_DIRECTSPLASH ) // use hybrid direction from splash and projectile
		{
			G_SplashFrac4D( ENTNUM( other ), ent->s.origin, ent->projectileInfo.radius, dir, NULL, NULL, ent->timeDelta );
		}
		else
		{
			VectorNormalize2( ent->velocity, dir );
		}

		G_TakeDamage( other, ent, ent->r.owner, dir, ent->velocity, ent->s.origin, ent->projectileInfo.maxDamage, ent->projectileInfo.maxKnockback, ent->projectileInfo.stun, 0, ent->style );
	}

	G_TakeRadiusDamage( ent, ent->r.owner, plane, other, MOD_GUNBLADE_S );

	// add explosion event
	if( ( !other->takedamage || ISBRUSHMODEL( other->s.modelindex ) ) )
	{
		edict_t *event;

		event = G_SpawnEvent( EV_GUNBLADEBLAST_IMPACT, DirToByte( plane ? plane->normal : NULL ), ent->s.origin );
		event->s.weapon = ( ( ent->projectileInfo.radius * 1/8 ) > 127 ) ? 127 : ( ent->projectileInfo.radius * 1/8 );
		event->s.skinnum = ( ( ent->projectileInfo.maxKnockback * 1/8 ) > 255 ) ? 255 : ( ent->projectileInfo.maxKnockback * 1/8 );
		event->r.svflags |= SVF_NOORIGIN2;
	}

	// free at next frame
	G_FreeEdict( ent );
}

//==================
//W_Fire_GunbladeBlast
//==================
edict_t *W_Fire_GunbladeBlast( edict_t *self, vec3_t start, vec3_t angles, float damage, int minKnockback, int maxKnockback, int stun, int minDamage, int radius, int speed, int timeout, int mod, int timeDelta )
{
	edict_t	*blast;

	if( GS_Instagib() )
		damage = 9999;

	blast = W_Fire_LinearProjectile( self, start, angles, speed, damage, minKnockback, maxKnockback, stun, minDamage, radius, timeout, timeDelta );
	blast->s.modelindex = trap_ModelIndex( PATH_GUNBLADEBLAST_STRONG_MODEL );
	blast->s.type = ET_BLASTER;
	blast->s.effects |= EF_STRONG_WEAPON;
	blast->touch = W_Touch_GunbladeBlast;
	blast->classname = "gunblade_blast";
	blast->style = mod;

	blast->s.sound = trap_SoundIndex( S_WEAPON_PLASMAGUN_S_FLY );

	return blast;
}

//==================
//W_Fire_Bullet
//==================
void W_Fire_Bullet( edict_t *self, vec3_t start, vec3_t angles, int seed, int range, int spread, float damage, int knockback, int stun, int mod, int timeDelta )
{
	vec3_t dir;
	edict_t *event;
	float r, u;
	double alpha, s;
	trace_t trace;
	int dmgflags = DAMAGE_STUN_CLAMP|DAMAGE_KNOCKBACK_SOFT;

	if( GS_Instagib() )
		damage = 9999;

	AngleVectors( angles, dir, NULL, NULL );

	// send the event
	event = G_SpawnEvent( EV_FIRE_BULLET, seed, start );
	event->s.ownerNum = ENTNUM( self );
	event->r.svflags = SVF_TRANSMITORIGIN2|SVF_NOCULLATORIGIN2;
	VectorScale( dir, 4096, event->s.origin2 ); // DirToByte is too inaccurate
	event->s.weapon = WEAP_MACHINEGUN;
	if( mod == MOD_MACHINEGUN_S )
		event->s.weapon |= EV_INVERSE;

	// circle shape
	alpha = M_PI * Q_crandom( &seed ); // [-PI ..+PI]
	s = fabs( Q_crandom( &seed ) ); // [0..1]
	r = s * cos( alpha ) * spread;
	u = s * sin( alpha ) * spread;

	GS_TraceBullet( &trace, start, dir, r, u, range, ENTNUM( self ), timeDelta );
	if( trace.ent != -1 )
	{
		if( game.edicts[trace.ent].takedamage )
		{
			G_TakeDamage( &game.edicts[trace.ent], self, self, dir, dir, trace.endpos, damage, knockback, stun, dmgflags, mod );
		}
		else
		{
			if( !( trace.surfFlags & SURF_NOIMPACT ) )
			{
			}
		}
	}
}

void G_Fire_SpiralPattern( edict_t *self, vec3_t start, vec3_t dir, int *seed, int count, int spread, int range, float damage, int kick, int stun, int dflags, int mod, int timeDelta )
{
	int i;
	float r;
	float u;
	trace_t trace;

	for( i = 0; i < count; i++ )
	{
		r = cos( *seed + i ) * spread * i;
		u = sin( *seed + i ) * spread * i;

		GS_TraceBullet( &trace, start, dir, r, u, range, ENTNUM( self ), timeDelta );
		if( trace.ent != -1 )
		{
			if( game.edicts[trace.ent].takedamage )
			{
				G_TakeDamage( &game.edicts[trace.ent], self, self, dir, dir, trace.endpos, damage, kick, stun, dflags, mod );
			}
			else
			{
				if( !( trace.surfFlags & SURF_NOIMPACT ) )
				{
				}
			}
		}
	}
}

void W_Fire_Riotgun( edict_t *self, vec3_t start, vec3_t angles, int seed, int range, int spread,
					int count, float damage, int knockback, int stun, int mod, int timeDelta )
{
	vec3_t dir;
	edict_t *event;
	int dmgflags = 0;

	if( GS_Instagib() )
		damage = 9999;

	AngleVectors( angles, dir, NULL, NULL );

	// send the event
	event = G_SpawnEvent( EV_FIRE_RIOTGUN, seed, start );
	event->s.ownerNum = ENTNUM( self );
	event->r.svflags = SVF_TRANSMITORIGIN2|SVF_NOCULLATORIGIN2;
	VectorScale( dir, 4096, event->s.origin2 ); // DirToByte is too inaccurate
	event->s.weapon = WEAP_RIOTGUN;
	if( mod == MOD_RIOTGUN_S )
		event->s.weapon |= EV_INVERSE;

	G_Fire_SpiralPattern( self, start, dir, &seed, count, spread, range, damage, knockback, stun, dmgflags, mod, timeDelta );
}

//==================
//W_Grenade_ExplodeDir
//==================
static void W_Grenade_ExplodeDir( edict_t *ent, vec3_t normal )
{
	vec3_t origin;
	int radius;
	edict_t	*event;
	vec3_t up = { 0, 0, 1 };
	vec_t *dir = normal ? normal : up;

	G_TakeRadiusDamage( ent,
		ent->r.owner,
		NULL,
		ent->enemy,
		( ent->s.effects & EF_STRONG_WEAPON ) ? MOD_GRENADE_SPLASH_S : MOD_GRENADE_SPLASH_W
		);

	radius = ( ( ent->projectileInfo.radius * 1/8 ) > 127 ) ? 127 : ( ent->projectileInfo.radius * 1/8 );
	VectorMA( ent->s.origin, -0.02, ent->velocity, origin );
	event = G_SpawnEvent( EV_GRENADE_EXPLOSION, ( dir ? DirToByte( dir ) : 0 ), ent->s.origin );
	event->s.firemode = ( ent->s.effects & EF_STRONG_WEAPON ) ? FIRE_MODE_STRONG : FIRE_MODE_WEAK;
	event->s.weapon = radius;
	event->r.svflags |= SVF_NOORIGIN2;

	G_FreeEdict( ent );
}

//==================
//W_Grenade_Explode
//==================
static void W_Grenade_Explode( edict_t *ent )
{
	W_Grenade_ExplodeDir( ent, NULL );
}

//==================
//W_Touch_Grenade
//==================
static void W_Touch_Grenade( edict_t *ent, edict_t *other, cplane_t *plane, int surfFlags )
{
	int hitType;
	vec3_t dir;

	if( surfFlags & SURF_NOIMPACT )
	{
		G_FreeEdict( ent );
		return;
	}

	hitType = G_Projectile_HitStyle( ent, other );
	if( hitType == PROJECTILE_TOUCH_NOT )
		return;

	// don't explode on doors and plats that take damage (removed in racesow: || ISBRUSHMODEL( other->s.modelindex ))
	if( !other->takedamage )
	{
	    if( ent->s.effects & EF_STRONG_WEAPON )//racesow: make grenades bounce twice code
            ent->health -= 1;

        if( !( ent->s.effects & EF_STRONG_WEAPON )
                || ( ( VectorLength( ent->velocity ) && Q_rint( ent->health ) > 0 ) || ent->timeStamp + 350 > level.time ) )//racesow: make grenades bounce twice code
        {
            // kill some velocity on each bounce
            float fric;
            static cvar_t *g_grenade_friction = NULL;

            // racesow: used rs_grenade_friction instead of g_grenade_friction
            g_grenade_friction = trap_Cvar_Get( "rs_grenade_friction", "0.85", CVAR_ARCHIVE );
            // !racesow

            fric = bound( 0, g_grenade_friction->value, 1 );
            VectorScale( ent->velocity, fric, ent->velocity );

            G_AddEvent( ent, EV_GRENADE_BOUNCE, ( ent->s.effects & EF_STRONG_WEAPON ) ? FIRE_MODE_STRONG : FIRE_MODE_WEAK, qtrue );
            return;
        }
	}

	if( other->takedamage )
	{
		int directHitDamage = ent->projectileInfo.maxDamage;

		VectorNormalize2( ent->velocity, dir );

		if( hitType == PROJECTILE_TOUCH_DIRECTSPLASH ) // use hybrid direction from splash and projectile
		{
			G_SplashFrac4D( ENTNUM( other ), ent->s.origin, ent->projectileInfo.radius, dir, NULL, NULL, ent->timeDelta );
		}
		else
		{
			VectorNormalize2( ent->velocity, dir );

			// no direct hit bonuses for grenades
			/*
			if( hitType == PROJECTILE_TOUCH_DIRECTAIRHIT )
				directHitDamage += DIRECAIRTHIT_DAMAGE_BONUS;
			else if( hitType == PROJECTILE_TOUCH_DIRECTHIT )
				directHitDamage += DIRECTHIT_DAMAGE_BONUS;
			*/
		}

		G_TakeDamage( other, ent, ent->r.owner, dir, ent->velocity, ent->s.origin, directHitDamage, ent->projectileInfo.maxKnockback, ent->projectileInfo.stun, 0, ent->style );
	}

	ent->enemy = other;
	W_Grenade_ExplodeDir( ent, plane ? plane->normal : NULL );
}

//==================
//W_Fire_Grenade
//==================
edict_t *W_Fire_Grenade( edict_t *self, vec3_t start, vec3_t angles, int speed, float damage,
						int minKnockback, int maxKnockback, int stun, int minDamage, float radius,
						int timeout, int mod, int timeDelta, qboolean aim_up )
{
	edict_t	*grenade;
	static cvar_t *g_grenade_gravity = NULL;

   /* racesow: custom parameters*/
    float rs_radius;
    int rs_minKnockback, rs_maxKnockback, rs_speed, rs_timeout;
    /* !racesow*/

    // racesow: used rs_grenade_gravity instead of g_grenade_gravity
    g_grenade_gravity = trap_Cvar_Get( "rs_grenade_gravity", "1.3", CVAR_ARCHIVE );
    // !racesow

	if( aim_up )
	{

        if( !GS_RaceGametype() )// racesow
            angles[PITCH] -= 10; // aim some degrees upwards from view dir

		// clamp to front side of the player
		angles[PITCH] += -90; // rotate to make easier the check
		while( angles[PITCH] < -360 ) angles[PITCH] += 360;
		clamp( angles[PITCH], -180, 0 );
		angles[PITCH] += 90;
		while( angles[PITCH] > 360 ) angles[PITCH] -= 360;
	}

   /* racesow: cvars for other parameters */
    if (mod==MOD_GRENADE_W)
    {
        rs_timeout= trap_Cvar_Get( "rs_grenadeweak_timeout", "1250", CVAR_ARCHIVE )->integer;
        rs_maxKnockback= trap_Cvar_Get( "rs_grenadeweak_knockback", "100", CVAR_ARCHIVE )->integer;
        rs_radius= trap_Cvar_Get( "rs_grenadeweak_splash", "170", CVAR_ARCHIVE )->value;
        rs_minKnockback= trap_Cvar_Get( "rs_grenadeweak_minknockback", "10", CVAR_ARCHIVE)->integer;
        rs_speed= trap_Cvar_Get( "rs_grenadeweak_speed", "900", CVAR_ARCHIVE )->integer;
    }
    else
    {
        rs_timeout= trap_Cvar_Get( "rs_grenade_timeout", "1250", CVAR_ARCHIVE )->integer;
        rs_maxKnockback= trap_Cvar_Get( "rs_grenade_knockback", "90", CVAR_ARCHIVE )->integer;
        rs_radius= trap_Cvar_Get( "rs_grenade_splash", "160", CVAR_ARCHIVE )->value;
        rs_minKnockback= trap_Cvar_Get( "rs_grenade_minknockback", "5", CVAR_ARCHIVE )->integer;
        rs_speed= trap_Cvar_Get( "rs_grenade_speed", "900", CVAR_ARCHIVE )->integer;
    }
    /* !racesow */

    if( GS_Instagib() )
        damage = 9999;

    /* racesow */
    grenade = W_Fire_TossProjectile( self, start, angles, rs_speed, damage, rs_minKnockback, rs_maxKnockback, stun, minDamage, rs_radius, rs_timeout, timeDelta );
    /* !racesow */
    VectorClear( grenade->s.angles );
	grenade->style = mod;
	grenade->s.type = ET_GRENADE;
	grenade->movetype = MOVETYPE_BOUNCEGRENADE;
	grenade->touch = W_Touch_Grenade;
	grenade->use = NULL;
	grenade->think = W_Grenade_Explode;
	grenade->classname = "grenade";
	grenade->gravity = g_grenade_gravity->value;
	grenade->enemy = NULL;

	if( mod == MOD_GRENADE_S )
	{
		grenade->s.modelindex = trap_ModelIndex( PATH_GRENADE_STRONG_MODEL );
		grenade->s.effects |= EF_STRONG_WEAPON;
		grenade->health = 2;//racesow: make grenades bounce twice code
	}
	else
	{
		grenade->s.modelindex = trap_ModelIndex( PATH_GRENADE_WEAK_MODEL );
		grenade->s.effects &= ~EF_STRONG_WEAPON;
	}

	GClip_LinkEntity( grenade );

	return grenade;
}

//==================
//W_Touch_Rocket
//==================
static void W_Touch_Rocket( edict_t *ent, edict_t *other, cplane_t *plane, int surfFlags )
{
	int mod_splash;
	vec3_t dir;
	int hitType;

	if( surfFlags & SURF_NOIMPACT )
	{
		G_FreeEdict( ent );
		return;
	}

	hitType = G_Projectile_HitStyle( ent, other );
	if( hitType == PROJECTILE_TOUCH_NOT )
		return;

	if( other->takedamage )
	{
		int directHitDamage = ent->projectileInfo.maxDamage;

		VectorNormalize2( ent->velocity, dir );

		if( hitType == PROJECTILE_TOUCH_DIRECTSPLASH ) // use hybrid direction from splash and projectile
		{
			
			G_SplashFrac4D( ENTNUM( other ), ent->s.origin, ent->projectileInfo.radius, dir, NULL, NULL, ent->timeDelta );
		}
		else
		{
			VectorNormalize2( ent->velocity, dir );

			if( hitType == PROJECTILE_TOUCH_DIRECTAIRHIT )
				directHitDamage += DIRECAIRTHIT_DAMAGE_BONUS;
			else if( hitType == PROJECTILE_TOUCH_DIRECTHIT )
				directHitDamage += DIRECTHIT_DAMAGE_BONUS;
		}

		G_TakeDamage( other, ent, ent->r.owner, dir, ent->velocity, ent->s.origin, directHitDamage, ent->projectileInfo.maxKnockback, ent->projectileInfo.stun, 0, ent->style );
	}

	if( ent->s.effects & EF_STRONG_WEAPON )
		mod_splash = MOD_ROCKET_SPLASH_S;
	else
		mod_splash = MOD_ROCKET_SPLASH_W;

	G_TakeRadiusDamage( ent, ent->r.owner, plane, other, mod_splash );

	// spawn the explosion
	if( !( surfFlags & SURF_NOIMPACT ) )
	{
		edict_t *event;
		vec3_t explosion_origin;

		VectorMA( ent->s.origin, -0.02, ent->velocity, explosion_origin );
		event = G_SpawnEvent( EV_ROCKET_EXPLOSION, DirToByte( plane ? plane->normal : NULL ), explosion_origin );
		event->s.firemode = ( ent->s.effects & EF_STRONG_WEAPON ) ? FIRE_MODE_STRONG : FIRE_MODE_WEAK;
		event->s.weapon = ( ( ent->projectileInfo.radius * 1/8 ) > 255 ) ? 255 : ( ent->projectileInfo.radius * 1/8 );
		event->r.svflags |= SVF_NOORIGIN2;
	}

	// free the rocket at next frame
	G_FreeEdict( ent );
}

//==================
//W_Fire_Rocket
//==================
edict_t *W_Fire_Rocket( edict_t *self, vec3_t start, vec3_t angles, int speed, float damage, int minKnockback, int maxKnockback, int stun, int minDamage, int radius, int timeout, int mod, int timeDelta )
{
	edict_t	*rocket;
    /* racesow: custom parameters*/
    int new_speed;
    int rs_minKnockback, rs_maxKnockback, rs_radius;
    /* !racesow*/

    /* racesow: added water rockets */
    new_speed=speed;
    if( self->waterlevel > 1 )
        new_speed*=0.5;
    /* !racesow */

    /* racesow: cvars for other parameters */
    if (mod==MOD_ROCKET_W)
    {
        rs_maxKnockback= trap_Cvar_Get( "rs_rocketweak_knockback", "95", CVAR_ARCHIVE )->integer;
        rs_radius= trap_Cvar_Get( "rs_rocketweak_splash", "140", CVAR_ARCHIVE )->integer;
        rs_minKnockback= trap_Cvar_Get( "rs_rocketweak_minknockback", "5", CVAR_ARCHIVE)->integer;
    }
    else
    {
        rs_maxKnockback= trap_Cvar_Get( "rs_rocket_knockback", "100", CVAR_ARCHIVE )->integer;
        rs_radius= trap_Cvar_Get( "rs_rocket_splash", "140", CVAR_ARCHIVE )->integer;
        rs_minKnockback= trap_Cvar_Get( "rs_rocket_minknockback", "10", CVAR_ARCHIVE )->integer;
    }
    /* !racesow */

    if( GS_Instagib() )
        damage = 9999;

    /* racesow: custom arguments */
    rocket = W_Fire_LinearProjectile( self, start, angles, new_speed, damage, rs_minKnockback, rs_maxKnockback, stun, minDamage, rs_radius, timeout, timeDelta );
    /* !racesow */

	rocket->s.type = ET_ROCKET; //rocket trail sfx
	if( mod == MOD_ROCKET_S )
	{
		rocket->s.modelindex = trap_ModelIndex( PATH_ROCKET_STRONG_MODEL );
		rocket->s.effects |= EF_STRONG_WEAPON;
		rocket->s.sound = trap_SoundIndex( S_WEAPON_ROCKET_S_FLY );
	}
	else
	{
		rocket->s.modelindex = trap_ModelIndex( PATH_ROCKET_WEAK_MODEL );
		rocket->s.effects &= ~EF_STRONG_WEAPON;
		rocket->s.sound = trap_SoundIndex( S_WEAPON_ROCKET_W_FLY );
	}
	rocket->touch = W_Touch_Rocket;
	rocket->think = G_FreeEdict;
	rocket->classname = "rocket";
	rocket->style = mod;

	return rocket;
}

static void W_Plasma_Explosion( edict_t *ent, edict_t *ignore, cplane_t *plane, int surfFlags )
{
	edict_t *event;
	int radius = ( ( ent->projectileInfo.radius * 1/8 ) > 127 ) ? 127 : ( ent->projectileInfo.radius * 1/8 );

	event = G_SpawnEvent( EV_PLASMA_EXPLOSION, DirToByte( plane ? plane->normal : NULL ), ent->s.origin );
	event->s.firemode = ( ent->s.effects & EF_STRONG_WEAPON ) ? FIRE_MODE_STRONG : FIRE_MODE_WEAK;
	event->s.weapon = radius & 127;
	event->r.svflags |= SVF_NOORIGIN2;

	G_TakeRadiusDamage( ent, ent->r.owner, plane, ignore, ent->style );

	G_FreeEdict( ent );
}

//==================
//W_Touch_Plasma
//==================
static void W_Touch_Plasma( edict_t *ent, edict_t *other, cplane_t *plane, int surfFlags )
{
	int hitType;
	vec3_t dir;
	/*if( surfFlags & SURF_NOIMPACT ) //<- Warsow0.6 code
	{
		G_FreeEdict( ent );
		return;
	}*/

	hitType = G_Projectile_HitStyle( ent, other );
	if( hitType == PROJECTILE_TOUCH_NOT )
		return;

	if( other->takedamage )
	{
		VectorNormalize2( ent->velocity, dir );

		if( hitType == PROJECTILE_TOUCH_DIRECTSPLASH ) // use hybrid direction from splash and projectile
		{
			G_SplashFrac4D( ENTNUM( other ), ent->s.origin, ent->projectileInfo.radius, dir, NULL, NULL, ent->timeDelta );
		}
		else
		{
			VectorNormalize2( ent->velocity, dir );
		}
		//racesow
		if( surfFlags & SURF_NOIMPACT ) //hack for plasma shooters which shoot on buttons with SURF_NOIMPACT
		{
			G_TakeDamage( other, ent, ent->r.owner, dir, ent->velocity, ent->s.origin, ent->projectileInfo.maxDamage, 0, 0, DAMAGE_NO_KNOCKBACK, ent->style );
			G_FreeEdict( ent );
			return;
		}
		//!racesow
		else
		{
			G_TakeDamage( other, ent, ent->r.owner, dir, ent->velocity, ent->s.origin, ent->projectileInfo.maxDamage, ent->projectileInfo.maxKnockback, ent->projectileInfo.stun, DAMAGE_KNOCKBACK_SOFT, ent->style );
		}
	}

	W_Plasma_Explosion( ent, other, plane, surfFlags );
}

//==================
//W_Plasma_Backtrace
//==================
void W_Plasma_Backtrace( edict_t *ent, const vec3_t start )
{
	trace_t	tr;
	vec3_t oldorigin;
	vec3_t mins = { -2, -2, -2 }, maxs = { 2, 2, 2 };

	if( GS_RaceGametype() && !level.gametype.playerInteraction )//racesow make projectiles interact with others in freestyle
		return;

	VectorCopy( ent->s.origin, oldorigin );
	VectorCopy( start, ent->s.origin );

	do
	{
		G_Trace4D( &tr, ent->s.origin, mins, maxs, oldorigin, ent, ( CONTENTS_BODY|CONTENTS_CORPSE ), ent->timeDelta );

		VectorCopy( tr.endpos, ent->s.origin );

		if( tr.ent == -1 )
			break;
		if( tr.allsolid || tr.startsolid )
			W_Touch_Plasma( ent, &game.edicts[tr.ent], NULL, 0 );
		else if( tr.fraction != 1.0 )
			W_Touch_Plasma( ent, &game.edicts[tr.ent], &tr.plane, tr.surfFlags );
		else
			break;
	} while( ent->r.inuse && ent->s.type == ET_PLASMA && !VectorCompare( ent->s.origin, oldorigin ) );

	if( ent->r.inuse && ent->s.type == ET_PLASMA )
		VectorCopy( oldorigin, ent->s.origin );
}

//==================
//W_Think_Plasma
//==================
static void W_Think_Plasma( edict_t *ent )
{
	vec3_t start;

	if( ent->timeout < level.time )
	{
		G_FreeEdict( ent );
		return;
	}

	if( ent->r.inuse )
		ent->nextThink = level.time + 1;

	VectorMA( ent->s.origin, -( game.frametime * 0.001 ), ent->velocity, start );

	W_Plasma_Backtrace( ent, start );
}

//==================
//W_AutoTouch_Plasma
//==================
static void W_AutoTouch_Plasma( edict_t *ent, edict_t *other, cplane_t *plane, int surfFlags )
{
	W_Think_Plasma( ent );
	if( !ent->r.inuse || ent->s.type != ET_PLASMA )
		return;

	W_Touch_Plasma( ent, other, plane, surfFlags );
}

//==================
//W_Fire_Plasma
//==================
edict_t *W_Fire_Plasma( edict_t *self, vec3_t start, vec3_t angles, float damage, int minKnockback, int maxKnockback, int stun, int minDamage, int radius, int speed, int timeout, int mod, int timeDelta )
{
	edict_t	*plasma;
    /* racesow: custom parameters*/
    int rs_minKnockback, rs_maxKnockback, rs_radius, rs_speed;
    /* !racesow*/

    /* racesow: cvars for other parameters */
    if (mod==MOD_PLASMA_W)
    {
        rs_maxKnockback= trap_Cvar_Get( "rs_plasmaweak_knockback", "14", CVAR_ARCHIVE )->integer;
        rs_radius= trap_Cvar_Get( "rs_plasmaweak_splash", "45", CVAR_ARCHIVE )->integer;
        rs_minKnockback= trap_Cvar_Get( "rs_plasmaweak_minknockback", "1", CVAR_ARCHIVE)->integer;
        rs_speed= trap_Cvar_Get( "rs_plasmaweak_speed", "2400", CVAR_ARCHIVE )->integer;
    }
    else
    {
        rs_maxKnockback= trap_Cvar_Get( "rs_plasma_knockback", "20", CVAR_ARCHIVE )->integer;
        rs_radius= trap_Cvar_Get( "rs_plasma_splash", "45", CVAR_ARCHIVE )->integer;
        rs_minKnockback= trap_Cvar_Get( "rs_plasma_minknockback", "1", CVAR_ARCHIVE )->integer;
        rs_speed= trap_Cvar_Get( "rs_plasma_speed", "2400", CVAR_ARCHIVE )->integer;
    }
    /* !racesow */

    if( GS_Instagib() )
        damage = 9999;

    /* racesow */
    plasma = W_Fire_LinearProjectile( self, start, angles, rs_speed, damage, rs_minKnockback, rs_maxKnockback, stun, minDamage, rs_radius, timeout, timeDelta );
    /* !racesow */
	plasma->s.type = ET_PLASMA;
	plasma->classname = "plasma";
	plasma->style = mod;

	plasma->think = W_Think_Plasma;
	plasma->touch = W_AutoTouch_Plasma;
	plasma->nextThink = level.time + 1;
	plasma->timeout = level.time + timeout;

	if( mod == MOD_PLASMA_S )
	{
		plasma->s.modelindex = trap_ModelIndex( PATH_PLASMA_STRONG_MODEL );
		plasma->s.sound = trap_SoundIndex( S_WEAPON_PLASMAGUN_S_FLY );
		plasma->s.effects |= EF_STRONG_WEAPON;
	}
	else
	{
		plasma->s.modelindex = trap_ModelIndex( PATH_PLASMA_WEAK_MODEL );
		plasma->s.sound = trap_SoundIndex( S_WEAPON_PLASMAGUN_W_FLY );
		plasma->s.effects &= ~EF_STRONG_WEAPON;
	}

	return plasma;
}

//==================
//W_Touch_Bolt
//==================
static void W_Touch_Bolt( edict_t *self, edict_t *other, cplane_t *plane, int surfFlags )
{
	edict_t *event;
	qboolean missed = qtrue;
	int hitType;

	if( surfFlags & SURF_NOIMPACT )
	{
		G_FreeEdict( self );
		return;
	}

	if( other == self->enemy )
		return;

	hitType = G_Projectile_HitStyle( self, other );
	if( hitType == PROJECTILE_TOUCH_NOT )
		return;

	if( other->takedamage )
	{
		vec3_t invdir;
		G_TakeDamage( other, self, self->r.owner, self->velocity, self->velocity, self->s.origin, self->projectileInfo.maxDamage, self->projectileInfo.maxKnockback, self->projectileInfo.stun, 0, MOD_ELECTROBOLT_W );
		VectorNormalize2( self->velocity, invdir );
		VectorScale( invdir, -1, invdir );
		event = G_SpawnEvent( EV_BOLT_EXPLOSION, DirToByte( invdir ), self->s.origin );
		event->s.firemode = FIRE_MODE_WEAK;
		event->r.svflags |= SVF_NOORIGIN2;
		if( other->r.client ) missed = qfalse;
	}
	else if( !( surfFlags & SURF_NOIMPACT ) )
	{   
		// add explosion event
		event = G_SpawnEvent( EV_BOLT_EXPLOSION, DirToByte( plane ? plane->normal : NULL ), self->s.origin );
		event->s.firemode = FIRE_MODE_WEAK;
		event->r.svflags |= SVF_NOORIGIN2;
	}

	if( missed && self->r.client )
		G_AwardPlayerMissedElectrobolt( self->r.owner, MOD_ELECTROBOLT_W ); // hit something that isnt a player

	G_FreeEdict( self );
}

//==================
//W_Fire_Electrobolt_Combined
//==================
void W_Fire_Electrobolt_Combined( edict_t *self, vec3_t start, vec3_t angles, float maxdamage, float mindamage, float maxknockback, float minknockback, int stun, int range, int mod, int timeDelta )
{
	vec3_t from, end, dir;
	trace_t	tr;
	edict_t	*ignore, *event, *hit, *damaged;
	int mask;
	qboolean missed = qtrue;
	int dmgflags = 0;
	int fireMode;

#ifdef ELECTROBOLT_TEST
	fireMode = FIRE_MODE_WEAK;
#else
	fireMode = FIRE_MODE_STRONG;
#endif

	if( GS_Instagib() )
		maxdamage = mindamage = 9999;

	AngleVectors( angles, dir, NULL, NULL );
	VectorMA( start, range, dir, end );
	VectorCopy( start, from );

	ignore = self;
	hit = damaged = NULL;

	mask = MASK_SHOT;
	if( GS_RaceGametype() && !level.gametype.playerInteraction )//racesow make projectiles interact with others in freestyle
		mask = MASK_SOLID;

	clamp_high( mindamage, maxdamage );
	clamp_high( minknockback, maxknockback );

	tr.ent = -1;
	while( ignore )
	{
		G_Trace4D( &tr, from, NULL, NULL, end, ignore, mask, timeDelta );

		VectorCopy( tr.endpos, from );
		ignore = NULL;

		if( tr.ent == -1 )
			break;

		// some entity was touched
		hit = &game.edicts[tr.ent];
		//racesow: do hit check later to activate shootable buttons

		// allow trail to go through BBOX entities (players, gibs, etc)
		if( !ISBRUSHMODEL( hit->s.modelindex ) )
			ignore = hit;

		if( ( hit != self ) && ( hit->takedamage ) )
		{
			float frac, damage, knockback;

			frac = DistanceFast( tr.endpos, start ) / (float)range;
			clamp( frac, 0.0f, 1.0f );

			damage = maxdamage - ( ( maxdamage - mindamage ) * frac );
			knockback = maxknockback - ( ( maxknockback - minknockback ) * frac );

			G_TakeDamage( hit, self, self, dir, dir, tr.endpos, damage, knockback, stun, dmgflags, mod );
            //racesow make shootable buttons work
            if( hit == world )  // stop dead if hit the world
                return;
            if( hit->movetype == MOVETYPE_NONE || hit->movetype == MOVETYPE_PUSH )
                return;
            //!racesow
			// spawn a impact event on each damaged ent
			event = G_SpawnEvent( EV_BOLT_EXPLOSION, DirToByte( tr.plane.normal ), tr.endpos );
			event->s.firemode = fireMode;
			if( hit->r.client )
				missed = qfalse;

			damaged = hit;
		}
	}

	if( missed && self->r.client )
		G_AwardPlayerMissedElectrobolt( self, mod );

	// send the weapon fire effect
	event = G_SpawnEvent( EV_ELECTROTRAIL, ENTNUM( self ), start );
	event->r.svflags = SVF_TRANSMITORIGIN2|SVF_NOCULLATORIGIN2;
	VectorScale( dir, 1024, event->s.origin2 );
	event->s.firemode = fireMode;

	if( !GS_Instagib() && tr.ent == -1 )	// didn't touch anything, not even a wall
	{
		edict_t *bolt;
		gs_weapon_definition_t *weapondef = GS_GetWeaponDef( self->s.weapon );

		// fire a weak EB from the end position
		bolt = W_Fire_Electrobolt_Weak( self, end, angles, weapondef->firedef_weak.speed, mindamage, minknockback, minknockback, stun, weapondef->firedef_weak.timeout, mod, timeDelta );
		bolt->enemy = damaged;
	}
}

void W_Fire_Electrobolt_FullInstant( edict_t *self, vec3_t start, vec3_t angles, float maxdamage, float mindamage, int maxknockback, int minknockback, int stun, int range, int minDamageRange, int mod, int timeDelta )
{
	vec3_t from, end, dir;
	trace_t	tr;
	edict_t	*ignore, *event, *hit, *damaged;
	int mask;
	qboolean missed = qtrue;
	int dmgflags = 0;

#define FULL_DAMAGE_RANGE g_projectile_prestep->value

	if( GS_Instagib() )
		maxdamage = mindamage = 9999;

	AngleVectors( angles, dir, NULL, NULL );
	VectorMA( start, range, dir, end );
	VectorCopy( start, from );

	ignore = self;
	hit = damaged = NULL;

	mask = MASK_SHOT;
	if( GS_RaceGametype() && !level.gametype.playerInteraction )//racesow make projectiles interact with others in freestyle
		mask = MASK_SOLID;

	clamp_high( mindamage, maxdamage );
	clamp_high( minknockback, maxknockback );
	clamp_high( minDamageRange, range );

	if( minDamageRange <= FULL_DAMAGE_RANGE )
		minDamageRange = FULL_DAMAGE_RANGE + 1;

	if( range <= FULL_DAMAGE_RANGE + 1 )
		range = FULL_DAMAGE_RANGE + 1;

	tr.ent = -1;
	while( ignore )
	{
		G_Trace4D( &tr, from, NULL, NULL, end, ignore, mask, timeDelta );

		VectorCopy( tr.endpos, from );
		ignore = NULL;

		if( tr.ent == -1 )
			break;

		// some entity was touched
		hit = &game.edicts[tr.ent];
        //racesow: do hit check later to activate shootable buttons

		// allow trail to go through BBOX entities (players, gibs, etc)
		if( !ISBRUSHMODEL( hit->s.modelindex ) )
			ignore = hit;

		if( ( hit != self ) && ( hit->takedamage ) )
		{
			float frac, damage, knockback, dist;

			dist = DistanceFast( tr.endpos, start );
			if( dist <= FULL_DAMAGE_RANGE )
				frac = 0.0f;
			else
			{
				frac = ( dist - FULL_DAMAGE_RANGE ) / (float)( minDamageRange - FULL_DAMAGE_RANGE );
				clamp( frac, 0.0f, 1.0f );
			}

			damage = maxdamage - ( ( maxdamage - mindamage ) * frac );
			knockback = maxknockback - ( ( maxknockback - minknockback ) * frac );

			//G_Printf( "mindamagerange %i frac %.1f damage %i\n", minDamageRange, 1.0f - frac, (int)damage );

			G_TakeDamage( hit, self, self, dir, dir, tr.endpos, damage, knockback, stun, dmgflags, mod );
            //racesow make shottable buttons work
            if( hit == world )  // stop dead if hit the world
                return;
            if( hit->movetype == MOVETYPE_NONE || hit->movetype == MOVETYPE_PUSH )
                return;
            //!racesow
			// spawn a impact event on each damaged ent
			event = G_SpawnEvent( EV_BOLT_EXPLOSION, DirToByte( tr.plane.normal ), tr.endpos );
			event->s.firemode = FIRE_MODE_STRONG;
			if( hit->r.client )
				missed = qfalse;

			damaged = hit;
		}
	}

	if( missed && self->r.client )
		G_AwardPlayerMissedElectrobolt( self, mod );

	// send the weapon fire effect
	event = G_SpawnEvent( EV_ELECTROTRAIL, ENTNUM( self ), start );
	event->r.svflags = SVF_TRANSMITORIGIN2|SVF_NOCULLATORIGIN2;
	VectorScale( dir, 1024, event->s.origin2 );
	event->s.firemode = FIRE_MODE_STRONG;

#undef FULL_DAMAGE_RANGE
}

//==================
//W_Fire_Electrobolt_Weak
//==================
edict_t *W_Fire_Electrobolt_Weak( edict_t *self, vec3_t start, vec3_t angles, float speed, float damage, int minKnockback, int maxKnockback, int stun, int timeout, int mod, int timeDelta )
{
	edict_t	*bolt;

	if( GS_Instagib() )
		damage = 9999;

	// projectile, weak mode
	bolt = W_Fire_LinearProjectile( self, start, angles, speed, damage, minKnockback, maxKnockback, stun, 0, 0, timeout, timeDelta );
	bolt->s.modelindex = trap_ModelIndex( PATH_ELECTROBOLT_WEAK_MODEL );
	bolt->s.type = ET_ELECTRO_WEAK; //add particle trail and light
	bolt->s.ownerNum = ENTNUM( self );
	bolt->touch = W_Touch_Bolt;
	bolt->classname = "bolt";
	bolt->style = mod;
	bolt->s.effects &= ~EF_STRONG_WEAPON;

	return bolt;
}

//==================
//W_Fire_Instagun_Strong
//==================
void W_Fire_Instagun( edict_t *self, vec3_t start, vec3_t angles, float damage, int knockback,
					 int stun, int radius, int range, int mod, int timeDelta )
{
	vec3_t from, end, dir;
	trace_t	tr;
	edict_t	*ignore, *event;
	int mask;
	qboolean missed = qtrue;
	int dmgflags = 0;

	if( GS_Instagib() )
		damage = 9999;

	AngleVectors( angles, dir, NULL, NULL );
	VectorMA( start, range, dir, end );
	VectorCopy( start, from );
	ignore = self;
	mask = MASK_SHOT;
	if( GS_RaceGametype() && !level.gametype.playerInteraction )//racesow: make projectiles interact with others in freestyle
		mask = MASK_SOLID;
	tr.ent = -1;
	while( ignore )
	{
		G_Trace4D( &tr, from, NULL, NULL, end, ignore, mask, timeDelta );
		VectorCopy( tr.endpos, from );
		ignore = NULL;
		if( tr.ent == -1 )
			break;

		// some entity was touched
		if( tr.ent == world->s.number 
			|| game.edicts[tr.ent].movetype == MOVETYPE_NONE 
			|| game.edicts[tr.ent].movetype == MOVETYPE_PUSH )
		{
			if( g_instajump->integer && self && self->r.client )
			{
				// create a temporary inflictor entity
				edict_t *inflictor;

				inflictor = G_Spawn();
				inflictor->s.solid = SOLID_NOT;
				inflictor->timeDelta = 0;
				VectorCopy( tr.endpos, inflictor->s.origin );
				inflictor->s.ownerNum = ENTNUM( self );
				inflictor->projectileInfo.maxDamage = 0;
				inflictor->projectileInfo.minDamage = 0;
				inflictor->projectileInfo.maxKnockback = knockback;
				inflictor->projectileInfo.minKnockback = 1;
				inflictor->projectileInfo.stun = 0;
				inflictor->projectileInfo.radius = radius;

				G_TakeRadiusDamage( inflictor, self, &tr.plane, NULL, mod );

				G_FreeEdict( inflictor );
			}
			break;
		}

		// allow trail to go through SOLID_BBOX entities (players, gibs, etc)
		if( !ISBRUSHMODEL( game.edicts[tr.ent].s.modelindex ) )
			ignore = &game.edicts[tr.ent];

		if( ( &game.edicts[tr.ent] != self ) && ( game.edicts[tr.ent].takedamage ) )
		{
			G_TakeDamage( &game.edicts[tr.ent], self, self, dir, dir, tr.endpos, damage, knockback, stun, dmgflags, mod );
			// spawn a impact event on each damaged ent
			event = G_SpawnEvent( EV_INSTA_EXPLOSION, DirToByte( tr.plane.normal ), tr.endpos );
			event->s.firemode = FIRE_MODE_STRONG;
			if( game.edicts[tr.ent].r.client )
				missed = qfalse;
		}
	}

	if( missed && self->r.client )
		G_AwardPlayerMissedElectrobolt( self, mod );

	// send the weapon fire effect
	event = G_SpawnEvent( EV_INSTATRAIL, ENTNUM( self ), start );
	event->r.svflags = SVF_TRANSMITORIGIN2|SVF_NOCULLATORIGIN2;
	VectorScale( dir, 1024, event->s.origin2 );
}

//==================
//G_HideLaser
//==================
void G_HideLaser( edict_t *ent )
{
	int soundindex;

	ent->s.modelindex = 0;
	ent->s.sound = 0;
	ent->r.svflags = SVF_NOCLIENT;

	if( ent->s.type == ET_CURVELASERBEAM )
		soundindex = trap_SoundIndex( S_WEAPON_LASERGUN_W_STOP );
	else
		soundindex = trap_SoundIndex( S_WEAPON_LASERGUN_S_STOP );

	G_Sound( game.edicts + ent->s.ownerNum, CHAN_AUTO, soundindex, ATTN_NORM );

	// give it 100 msecs before freeing itself, so we can relink it if we start firing again
	ent->think = G_FreeEdict;
	ent->nextThink = level.time + 100;
}

//==================
//G_Laser_Think
//==================
static void G_Laser_Think( edict_t *ent )
{
	edict_t *owner;

	if( ent->s.ownerNum < 1 || ent->s.ownerNum > gs.maxclients )
	{
		G_FreeEdict( ent );
		return;
	}

	owner = &game.edicts[ent->s.ownerNum];

	if( G_ISGHOSTING( owner ) || owner->s.weapon != WEAP_LASERGUN ||
		trap_GetClientState( PLAYERNUM( owner ) ) < CS_SPAWNED ||
		( owner->r.client->ps.weaponState != WEAPON_STATE_REFIRESTRONG
		&& owner->r.client->ps.weaponState != WEAPON_STATE_REFIRE ) )
	{
		G_HideLaser( ent );
		return;
	}

	ent->nextThink = level.time + 1;
}

static float laser_damage;
static int laser_knockback;
static int laser_stun;
static int laser_attackerNum;
static int laser_mod;
static int laser_missed;

static void _LaserImpact( trace_t *trace, vec3_t dir )
{
	edict_t *attacker;

	if( !trace || trace->ent <= 0 )
		return;

	attacker = &game.edicts[laser_attackerNum];

	if( game.edicts[trace->ent].takedamage )
	{
		G_TakeDamage( &game.edicts[trace->ent], attacker, attacker, dir, dir, trace->endpos, laser_damage, laser_knockback, laser_stun, DAMAGE_STUN_CLAMP|DAMAGE_KNOCKBACK_SOFT, laser_mod );
		laser_missed = qfalse;
	}
}

static edict_t *_FindOrSpawnLaser( edict_t *owner, int entType, qboolean *newLaser )
{
	int i, ownerNum;
	edict_t	*e, *laser;

	// first of all, see if we already have a beam entity for this laser
	*newLaser = qfalse;
	laser = NULL;
	ownerNum = ENTNUM( owner );
	for( i = gs.maxclients+1; i < game.maxentities; i++ )
	{
		e = &game.edicts[i];
		if( !e->r.inuse )
			continue;

		if( e->s.ownerNum == ownerNum && ( e->s.type == ET_LASERBEAM || e->s.type == ET_CURVELASERBEAM ) )
		{
			laser = e;
			break;
		}
	}

	// if no ent was found we have to create one
	if( !laser || laser->s.type != entType || !laser->s.modelindex )
	{
		if( !laser )
		{
			*newLaser = qtrue;
			laser = G_Spawn();
		}

		laser->s.type = entType;
		laser->s.ownerNum = ownerNum;
		laser->movetype = MOVETYPE_NONE;
		laser->r.solid = SOLID_NOT;
		laser->r.svflags = SVF_TRANSMITORIGIN2; // force PHS culling
		laser->s.modelindex = 255; // needs to have some value so it isn't filtered by the server culling
	}

	return laser;
}

//==================
//W_Fire_Lasergun
//==================
edict_t	*W_Fire_Lasergun( edict_t *self, vec3_t start, vec3_t angles, float damage, int knockback, int stun, int range, int mod, int timeDelta )
{
	edict_t	*laser;
	qboolean newLaser;
	trace_t	tr;
	vec3_t dir;

	if( GS_Instagib() )
		damage = 9999;

	laser = _FindOrSpawnLaser( self, ET_LASERBEAM, &newLaser );
	if( newLaser )
	{
		// the quad start sound is added from the server
		if( self->r.client && self->r.client->ps.inventory[POWERUP_QUAD] > 0 )
			G_Sound( self, CHAN_AUTO, trap_SoundIndex( S_QUAD_FIRE ), ATTN_NORM );
	}

	laser_damage = damage;
	laser_knockback = knockback;
	laser_stun = stun;
	laser_attackerNum = ENTNUM( self );
	laser_mod = mod;
	laser_missed = qtrue;

	GS_TraceLaserBeam( &tr, start, angles, range, ENTNUM( self ), timeDelta, _LaserImpact );

	laser->r.svflags |= SVF_FORCEOWNER;
	VectorCopy( start, laser->s.origin );
	AngleVectors( angles, dir, NULL, NULL );
	VectorMA( laser->s.origin, range, dir, laser->s.origin2 );

	laser->think = G_Laser_Think;
	laser->nextThink = level.time + 100;

	if( laser_missed && self->r.client )
		G_AwardPlayerMissedLasergun( self, mod );

	GClip_LinkEntity( laser );

	return laser;
}

//void AITools_DrawLine(vec3_t origin, vec3_t dest);
edict_t	*W_Fire_Lasergun_Weak( edict_t *self, vec3_t start, vec3_t end, float damage, int knockback, int stun, int range, int mod, int timeDelta )
{
	edict_t	*laser;
	qboolean newLaser;
	trace_t	trace;

	if( GS_Instagib() )
		damage = 9999;

	laser = _FindOrSpawnLaser( self, ET_CURVELASERBEAM, &newLaser );
	if( newLaser )
	{
		// the quad start sound is added from the server
		if( self->r.client && self->r.client->ps.inventory[POWERUP_QUAD] > 0 )
			G_Sound( self, CHAN_AUTO, trap_SoundIndex( S_QUAD_FIRE ), ATTN_NORM );
	}

	laser_damage = damage;
	laser_knockback = knockback;
	laser_stun = stun;
	laser_attackerNum = ENTNUM( self );
	laser_mod = mod;
	laser_missed = qtrue;

	GS_TraceCurveLaserBeam( &trace, start, self->s.angles, end, ENTNUM( self ), timeDelta, _LaserImpact );

	laser->r.svflags |= SVF_FORCEOWNER;
	VectorCopy( start, laser->s.origin );
	VectorCopy( end, laser->s.origin2 );

	laser->think = G_Laser_Think;
	laser->nextThink = level.time + 100;

	if( laser_missed && self->r.client )
		G_AwardPlayerMissedLasergun( self, mod );

	GClip_LinkEntity( laser );

	return laser;
}



