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
	VectorSubtract( boxcenter, origin, pushdir );

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
    gs_weapon_definition_t *weapondef = NULL;

    if ( self->enemy ) {
        VectorSubtract( self->enemy->s.origin, self->s.origin, dir );
        VectorNormalize( dir );
    } else {
        VectorCopy( self->moveinfo.movedir, dir );
        VectorNormalize( dir );
    }

	switch ( self->s.weapon ) {
        case WEAP_GRENADELAUNCHER:
            //W_Fire_Grenade( activator, self->s.origin, dir, firedef->speed, damage, knockback, mindmg, firedef->splash_radius, firedef->timeout, MOD_GRENADE_W, timeDelta );
            break;
        case WEAP_ROCKETLAUNCHER:
			weapondef = GS_GetWeaponDef( WEAP_ROCKETLAUNCHER );
            //W_Fire_Rocket( activator, self->s.origin, dir, speed, damage, knockback, mindmg, firedef->splash_radius, firedef->timeout, MOD_ROCKET_W, 0 );
			W_Fire_Rocket( activator, self->s.origin, dir, weapondef->firedef.speed, weapondef->firedef.damage, weapondef->firedef.minknockback, weapondef->firedef.knockback, weapondef->firedef.stun, weapondef->firedef.mindamage, weapondef->firedef.splash_radius, weapondef->firedef.timeout, MOD_ROCKET_W, 0 );
            break;
        case WEAP_PLASMAGUN:
            // W_Fire_Plasma( activator, self->s.origin, dir, damage, knockback, mindmg, firedef->splash_radius, firedef->speed, firedef->timeout, MOD_PLASMA_W, timeDelta );
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