#include "g_local.h"


// Functions

//================================================
//rs_SplashFrac - racesow version of G_SplashFrac
//================================================
void rs_SplashFrac( const vec3_t origin, const vec3_t mins, const vec3_t maxs, const vec3_t point, float maxradius, vec3_t pushdir, float *kickFrac, float *dmgFrac )
{
	vec3_t boxcenter = { 0, 0, 0 };
	float distance =0;
	int i;
	float innerradius;
	float outerradius;
	float g_distance;
	float h_distance;

	if( maxradius <= 0 )
	{
		if( kickFrac )
			*kickFrac = 0;
		if( dmgFrac )
			*dmgFrac = 0;
		return;
	}

	innerradius = ( maxs[0] + maxs[1] - mins[0] - mins[1] ) * 0.25;
	outerradius = ( maxs[2] - mins[2] ); //cylinder height

	// find center of the box
	for( i = 0; i < 3; i++ )
		boxcenter[i] = origin[i] + maxs[i] + mins[i];

	// find box radius to explosion origin direction
	VectorSubtract( boxcenter, point, pushdir );

	g_distance = sqrt( pushdir[0]*pushdir[0] + pushdir[1]*pushdir[1] ); // distance on the virtual ground
	h_distance = fabs( pushdir[2] );                                // corrected distance in height

	if( ( h_distance <= outerradius/2 ) || ( g_distance > innerradius ) )
	{
		distance = g_distance - innerradius;
	}
	if( ( h_distance > outerradius/2 ) || ( g_distance <= innerradius ) )
	{
		distance = h_distance - outerradius/2;
	}
	if( ( h_distance > outerradius/2 ) || ( g_distance > innerradius ) )
	{
		distance = sqrt( ( g_distance - innerradius )*( g_distance - innerradius ) + ( h_distance - outerradius/2 )*( h_distance - outerradius/2 ) );
	}

	if( dmgFrac )
	{
		// soft sin curve
		*dmgFrac = sin( DEG2RAD( ( distance / maxradius ) * 80 ) );
		clamp( *dmgFrac, 0.0f, 1.0f );
	}

	if( kickFrac )
	{
		*kickFrac = 1.0 - fabs( distance / maxradius );
		clamp( *kickFrac, 0.0f, 1.0f );
	}

	VectorSubtract( boxcenter, point, pushdir );
	VectorNormalizeFast( pushdir );
}

//==============
//RS_UseShooter
//==============
void RS_UseShooter( edict_t *self, edict_t *other, edict_t *activator ) {

	vec3_t      dir;
	vec3_t      angles;
    gs_weapon_definition_t *weapondef = NULL;

    if ( self->enemy ) {
        VectorSubtract( self->enemy->s.origin, self->s.origin, dir );
        VectorNormalize( dir );
    } else {
        VectorCopy( self->moveinfo.movedir, dir );
        VectorNormalize( dir );
    }
    VecToAngles( dir, angles );
	switch ( self->s.weapon ) {
        case WEAP_GRENADELAUNCHER:
        	weapondef = GS_GetWeaponDef( WEAP_GRENADELAUNCHER );
            W_Fire_Grenade( activator, self->s.origin, angles, weapondef->firedef.speed, weapondef->firedef.damage, weapondef->firedef.minknockback, weapondef->firedef.knockback, weapondef->firedef.stun, weapondef->firedef.mindamage, weapondef->firedef.splash_radius, weapondef->firedef.timeout, MOD_GRENADE_W, 0, qfalse );
            break;
        case WEAP_ROCKETLAUNCHER:
			weapondef = GS_GetWeaponDef( WEAP_ROCKETLAUNCHER );
			W_Fire_Rocket( activator, self->s.origin, angles, weapondef->firedef.speed, weapondef->firedef.damage, weapondef->firedef.minknockback, weapondef->firedef.knockback, weapondef->firedef.stun, weapondef->firedef.mindamage, weapondef->firedef.splash_radius, weapondef->firedef.timeout, MOD_ROCKET_W, 0 );
            break;
        case WEAP_PLASMAGUN:
        	weapondef = GS_GetWeaponDef( WEAP_PLASMAGUN );
            W_Fire_Plasma( activator, self->s.origin, angles, weapondef->firedef.speed, weapondef->firedef.damage, weapondef->firedef.minknockback, weapondef->firedef.knockback, weapondef->firedef.stun, weapondef->firedef.mindamage, weapondef->firedef.splash_radius, weapondef->firedef.timeout, MOD_PLASMA_W, 0 );
            break;
    }

    //G_AddEvent( self, EV_MUZZLEFLASH, FIRE_MODE_WEAK, qtrue );

}

//======================
//RS_InitShooter_Finish
//======================
void RS_InitShooter_Finish( edict_t *self ) {

	self->enemy = G_PickTarget( self->target );
    self->think = 0;
    self->nextThink = 0;
}

//===============
//RS_InitShooter
//===============
void RS_InitShooter( edict_t *self, int weapon ) {

	self->use = RS_UseShooter;
    self->s.weapon = weapon;
    G_SetMovedir( self->s.angles, self->moveinfo.movedir );
    // target might be a moving object, so we can't set movedir for it
    if ( self->target ) {
        self->think = RS_InitShooter_Finish;
        self->nextThink = level.time + 500;
    }
    GClip_LinkEntity( self );

}

//=================
//RS_shooter_rocket
//===============
void RS_shooter_rocket( edict_t *self ) {
    RS_InitShooter( self, WEAP_ROCKETLAUNCHER );
}

//=================
//RS_shooter_plasma
//===============
void RS_shooter_plasma( edict_t *self ) {
    RS_InitShooter( self, WEAP_PLASMAGUN);
}

//=================
//RS_shooter_grenade
//===============
void RS_shooter_grenade( edict_t *self ) {
    RS_InitShooter( self, WEAP_GRENADELAUNCHER);
}

//=================
//QUAKED target_delay (1 0 0) (-8 -8 -8) (8 8 8)
//"wait" seconds to pause before firing targets.
//=================
void Think_Target_Delay( edict_t *self ) {

    G_UseTargets( self, self->activator );
}

//=================
//Use_Target_Delay
//=================
void Use_Target_Delay( edict_t *self, edict_t *other, edict_t *activator ) {
    self->nextThink = level.time + self->wait * 1000;
    self->think = Think_Target_Delay;
    self->activator = activator;
}

//=================
//RS_target_delay
//=================
void RS_target_delay( edict_t *self ) {

    if ( !self->wait ) {
        self->wait = 1;
    }
    self->use = Use_Target_Delay;
}


//==========================================================
//QUAKED target_relay (0 .7 .7) (-8 -8 -8) (8 8 8) RED_ONLY BLUE_ONLY RANDOM
//This can only be activated by other triggers which will cause it in turn to activate its own targets.
//-------- KEYS --------
//targetname : activating trigger points to this.
//target : this points to entities to activate when this entity is triggered.
//notfree : when set to 1, entity will not spawn in "Free for all" and "Tournament" modes.
//notteam : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes.
//notsingle : when set to 1, entity will not spawn in Single Player mode (bot play mode).
//-------- SPAWNFLAGS --------
//RED_ONLY : only red team players can activate trigger. <- WRONG
//BLUE_ONLY : only red team players can activate trigger. <- WRONG
//RANDOM : one one of the targeted entities will be triggered at random.
void target_relay_use (edict_t *self, edict_t *other, edict_t *activator) {
    if ( ( self->spawnflags & 1 ) && activator->s.team && activator->s.team != TEAM_ALPHA ) {
        return;
    }
    if ( ( self->spawnflags & 2 ) && activator->s.team && activator->s.team != TEAM_BETA ) {
        return;
    }
    if ( self->spawnflags & 4 ) {
        edict_t *ent;
        ent = G_PickTarget( self->target );
        if ( ent && ent->use ) {
            ent->use( ent, self, activator );
        }
        return;
    }
    G_UseTargets (self, activator);
}

//=================
//RS_target_relay
//=================
void RS_target_relay (edict_t *self) {
    self->use = target_relay_use;
}

/*
* RS_removeProjectiles
*/
void RS_removeProjectiles( edict_t *owner )
{
	edict_t *ent;

	for( ent = game.edicts + gs.maxclients; ENTNUM( ent ) < game.numentities; ent++ )
	{
		if( ent->r.inuse && !ent->r.client && ent->r.svflags & SVF_PROJECTILE && ent->r.solid != SOLID_NOT
				&& ent->r.owner == owner )
		{
			G_FreeEdict( ent );
		}
	}
}
