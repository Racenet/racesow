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
#include "cg_local.h"

/*
* CG_TouchJumpPad
*/
static void CG_TouchJumpPad( int entNum )
{
	CG_SexedSound( entNum, CHAN_BODY, va( S_PLAYER_JUMP_1_to_2, ( rand()&1 )+1 ), cg_volume_players->value );
	CG_PModel_AddAnimation( entNum, LEGS_JUMP_NEUTRAL, 0, 0, EVENT_CHANNEL );
}

/*
* CG_Event_WeaponBeam
*/
static void CG_Event_WeaponBeam( vec3_t origin, vec3_t dir, int ownerNum, int weapon, int firemode )
{
	gs_weapon_definition_t *weapondef;
	int range;
	vec3_t end;
	trace_t trace;

	switch( weapon )
	{
	case WEAP_ELECTROBOLT:
		weapondef = GS_GetWeaponDef( WEAP_ELECTROBOLT );
		range = ELECTROBOLT_RANGE;
		break;

	case WEAP_INSTAGUN:
		weapondef = GS_GetWeaponDef( WEAP_INSTAGUN );
		range = weapondef->firedef.timeout;
		break;

	default:
		return;
	}

	VectorNormalizeFast( dir );

	VectorMA( origin, range, dir, end );

	// retrace to spawn wall impact
	CG_Trace( &trace, origin, vec3_origin, vec3_origin, end, cg.view.POVent, MASK_SOLID );
	if( trace.ent != -1 && !(trace.surfFlags & (SURF_SKY|SURF_NOMARKS|SURF_NOIMPACT)) )
	{
		if( weapondef->weapon_id == WEAP_ELECTROBOLT )
			CG_BoltExplosionMode( trace.endpos, trace.plane.normal, FIRE_MODE_STRONG );
		else if( weapondef->weapon_id == WEAP_INSTAGUN )
			CG_InstaExplosionMode( trace.endpos, trace.plane.normal, FIRE_MODE_STRONG );
	}

	// when it's predicted we have to delay the drawing until the view weapon is calculated
	cg_entities[ownerNum].localEffects[LOCALEFFECT_EV_WEAPONBEAM] = weapon;
	VectorCopy( origin, cg_entities[ownerNum].laserOrigin );
	VectorCopy( trace.endpos, cg_entities[ownerNum].laserPoint );
}

void CG_WeaponBeamEffect( centity_t *cent )
{
	orientation_t projection;

	if( !cent->localEffects[LOCALEFFECT_EV_WEAPONBEAM] )
		return;

	// now find the projection source for the beam we will draw
	if( !CG_PModel_GetProjectionSource( cent->current.number, &projection ) )
		VectorCopy( cent->laserOrigin, projection.origin );

	if( cent->localEffects[LOCALEFFECT_EV_WEAPONBEAM] == WEAP_ELECTROBOLT )
		CG_ElectroTrail2( projection.origin, cent->laserPoint, cent->current.team );
	else
		CG_InstaPolyBeam( projection.origin, cent->laserPoint, cent->current.team );

	cent->localEffects[LOCALEFFECT_EV_WEAPONBEAM] = 0;
}

static centity_t *laserOwner = NULL;

static void _LaserImpact( trace_t *trace, vec3_t dir )
{
	if( !trace || trace->ent < 0 )
		return;

	if( cg_particles->integer && laserOwner )
	{
#define TRAILTIME ( (int)( 1000.0f / 20.0f ) ) // density as quantity per second

		if( laserOwner->localEffects[LOCALEFFECT_LASERBEAM_SMOKE_TRAIL] + TRAILTIME < cg.time )
		{
			laserOwner->localEffects[LOCALEFFECT_LASERBEAM_SMOKE_TRAIL] = cg.time;
			CG_ImpactSmokePuff( trace->endpos, trace->plane.normal, 3, 1.0f, 8, 12 );
		}
#undef TRAILTIME
	}

	// it's a brush model
	if( trace->ent == 0 || !( cg_entities[trace->ent].current.effects & EF_TAKEDAMAGE ) )
	{
		CG_AddLightToScene( trace->endpos, 100, 0.75f, 0.75f, 0.375f, NULL );

		// TODO: add impact model
		return;
	}

	// it's a player

	// TODO: add player-impact model
}

void CG_LaserBeamEffect( centity_t *cent )
{
	struct sfx_s *sound = NULL;
	float range;
	trace_t trace;
	orientation_t projectsource;
	vec4_t color;
	vec3_t laserOrigin, laserAngles, laserPoint;
	int i, j;

	if( cent->localEffects[LOCALEFFECT_LASERBEAM] <= cg.time )
		return;

	laserOwner = cent;

	if( cg_teamColoredBeams->integer && ( cent->current.team == TEAM_ALPHA || cent->current.team == TEAM_BETA ) )
		CG_TeamColor( cent->current.team, color );
	else
		Vector4Set( color, 1, 1, 1, 1 );

	// interpolate the positions

	if( ISVIEWERENTITY( cent->current.number ) && !cg.view.thirdperson )
	{
		VectorCopy( cg.predictedPlayerState.pmove.origin, laserOrigin );
		laserOrigin[2] += cg.predictedPlayerState.viewheight;
		VectorCopy( cg.predictedPlayerState.viewangles, laserAngles );

		VectorLerp( cent->laserPointOld, cg.lerpfrac, cent->laserPoint, laserPoint );
	}
	else
	{
		VectorLerp( cent->laserOriginOld, cg.lerpfrac, cent->laserOrigin, laserOrigin );
		VectorLerp( cent->laserPointOld, cg.lerpfrac, cent->laserPoint, laserPoint );
		if( !cent->laserCurved )
		{
			vec3_t dir;

			// make up the angles from the start and end points (s->angles is not so precise)
			VectorSubtract( laserPoint, laserOrigin, dir );
			VecToAngles( dir, laserAngles );
		}
		else // use player entity angles
		{
			for( i = 0; i < 3; i++ )
				laserAngles[i] = LerpAngle( cent->prev.angles[i], cent->current.angles[i], cg.lerpfrac );
		}
	}

	if( !cent->laserCurved )
	{
		range = GS_GetWeaponDef( WEAP_LASERGUN )->firedef.timeout;

		if( cent->current.effects & EF_QUAD )
			sound = CG_MediaSfx( cgs.media.sfxLasergunStrongQuadHum );
		else
			sound = CG_MediaSfx( cgs.media.sfxLasergunStrongHum );

		// trace the beam: for tracing we use the real beam origin
		GS_TraceLaserBeam( &trace, laserOrigin, laserAngles, range, cent->current.number, 0, _LaserImpact );
		
		// draw the beam: for drawing we use the weapon projection source (already handles the case of viewer entity)
		if( !CG_PModel_GetProjectionSource( cent->current.number, &projectsource ) )
			VectorCopy( laserOrigin, projectsource.origin );

		CG_KillPolyBeamsByTag( cent->current.number );
		CG_LaserGunPolyBeam( projectsource.origin, trace.endpos, color, cent->current.number );
	}
	else
	{
		float frac, subdivisions = cg_laserBeamSubdivisions->integer;
		vec3_t from, dir, end, blendPoint;
		int passthrough = cent->current.number;
		vec3_t tmpangles, blendAngles;

		range = GS_GetWeaponDef( WEAP_LASERGUN )->firedef_weak.timeout;

		if( cent->current.effects & EF_QUAD )
			sound = CG_MediaSfx( cgs.media.sfxLasergunWeakQuadHum );
		else
			sound = CG_MediaSfx( cgs.media.sfxLasergunWeakHum );

		// trace the beam: for tracing we use the real beam origin
		GS_TraceCurveLaserBeam( &trace, laserOrigin, laserAngles, laserPoint, cent->current.number, 0, _LaserImpact );

		// draw the beam: for drawing we use the weapon projection source (already handles the case of viewer entity)
		if( !CG_PModel_GetProjectionSource( cent->current.number, &projectsource ) )
			VectorCopy( laserOrigin, projectsource.origin );

		if( subdivisions < CURVELASERBEAM_SUBDIVISIONS )
			subdivisions = CURVELASERBEAM_SUBDIVISIONS;

		CG_KillPolyBeamsByTag( cent->current.number );

		// we redraw the full beam again, and trace each segment for stop dead impact
		VectorCopy( laserPoint, blendPoint );
		VectorCopy( projectsource.origin, from );
		VectorSubtract( blendPoint, projectsource.origin, dir );
		VecToAngles( dir, blendAngles );

		for( i = 1; i <= (int)subdivisions; i++ )
		{
			frac = ( ( range/subdivisions )*(float)i ) / (float)range;

			for( j = 0; j < 3; j++ )
				tmpangles[j] = LerpAngle( laserAngles[j], blendAngles[j], frac );

			AngleVectors( tmpangles, dir, NULL, NULL );
			VectorMA( projectsource.origin, range * frac, dir, end );

			GS_TraceLaserBeam( &trace, from, tmpangles, DistanceFast( from, end ), passthrough, 0, NULL );
			CG_LaserGunPolyBeam( from, trace.endpos, color, cent->current.number );
			if( trace.fraction != 1.0f )
				break;

			passthrough = trace.ent;
			VectorCopy( trace.endpos, from );
		}
	}

	// enable continuous flash on the weapon owner
	if( cg_weaponFlashes->integer )
		cg_entPModels[cent->current.number].flash_time = cg.time + CG_GetWeaponInfo( WEAP_LASERGUN )->flashTime;

	if( sound )
	{
		if( ISVIEWERENTITY( cent->current.number ) )
			trap_S_AddLoopSound( sound, cent->current.number, 1.0, ATTN_NONE );
		else
			trap_S_AddLoopSound( sound, cent->current.number, 1.0, ATTN_STATIC );
	}

	laserOwner = NULL;
}

void CG_Event_LaserBeam( int entNum, int weapon, int fireMode )
{
	centity_t *cent = &cg_entities[entNum];
	unsigned int timeout;
	vec3_t dir;

	if( !cg_predictLaserBeam->integer )
		return;

	// lasergun's smooth refire
	if( fireMode == FIRE_MODE_STRONG )
	{
		cent->laserCurved = qfalse;
		timeout = GS_GetWeaponDef( WEAP_LASERGUN )->firedef.reload_time + 10;

		// find destiny point
		VectorCopy( cg.predictedPlayerState.pmove.origin, cent->laserOrigin );
		cent->laserOrigin[2] += cg.predictedPlayerState.viewheight;
		AngleVectors( cg.predictedPlayerState.viewangles, dir, NULL, NULL );
		VectorMA( cent->laserOrigin, GS_GetWeaponDef( WEAP_LASERGUN )->firedef.timeout, dir, cent->laserPoint );
	}
	else
	{
		cent->laserCurved = qtrue;
		timeout = GS_GetWeaponDef( WEAP_LASERGUN )->firedef_weak.reload_time + 10;

		// find destiny point
		VectorCopy( cg.predictedPlayerState.pmove.origin, cent->laserOrigin );
		cent->laserOrigin[2] += cg.predictedPlayerState.viewheight;
		if( !G_GetLaserbeamPoint( &cg.weaklaserTrail, &cg.predictedPlayerState, cg.predictingTimeStamp, cent->laserPoint ) )
		{
			AngleVectors( cg.predictedPlayerState.viewangles, dir, NULL, NULL );
			VectorMA( cent->laserOrigin, GS_GetWeaponDef( WEAP_LASERGUN )->firedef.timeout, dir, cent->laserPoint );
		}
	}

	VectorCopy( cent->laserOrigin, cent->laserOriginOld );
	VectorCopy( cent->laserPoint, cent->laserPointOld );
	cent->localEffects[LOCALEFFECT_LASERBEAM] = cg.time + timeout;
}

/*
* CG_FireWeaponEvent
*/
static void CG_FireWeaponEvent( int entNum, int weapon, int fireMode )
{
	float attenuation;
	struct sfx_s *sound = NULL;
	weaponinfo_t *weaponInfo;

	if( !weapon )
		return;

	// hack idle attenuation on the plasmagun to reduce sound flood on the scene
	if( weapon == WEAP_PLASMAGUN )
		attenuation = ATTN_IDLE;
	else
		attenuation = ATTN_NORM;

	weaponInfo = CG_GetWeaponInfo( weapon );

	// sound
	if( fireMode == FIRE_MODE_STRONG )
	{
		if( weaponInfo->num_strongfire_sounds )
			sound = weaponInfo->sound_strongfire[(int)brandom( 0, weaponInfo->num_strongfire_sounds )];
	}
	else
	{
		if( weaponInfo->num_fire_sounds )
			sound = weaponInfo->sound_fire[(int)brandom( 0, weaponInfo->num_fire_sounds )];
	}

	if( sound )
	{
		if( ISVIEWERENTITY( entNum ) )
			trap_S_StartGlobalSound( sound, CHAN_MUZZLEFLASH, cg_volume_effects->value );
		else
			// fixed position is better for location, but the channels are used from worldspawn
			// and openal runs out of channels quick on cheap cards. Relative sound uses per-entity channels.
			trap_S_StartRelativeSound( sound, entNum, CHAN_MUZZLEFLASH, cg_volume_effects->value, attenuation );

		if( ( cg_entities[entNum].current.effects & EF_QUAD ) && ( weapon != WEAP_LASERGUN ) )
		{
			struct sfx_s *quadSfx = CG_MediaSfx( cgs.media.sfxQuadFireSound );
			if( ISVIEWERENTITY( entNum ) )
				trap_S_StartGlobalSound( quadSfx, CHAN_AUTO, cg_volume_effects->value );
			else
				trap_S_StartRelativeSound( quadSfx, entNum, CHAN_AUTO, cg_volume_effects->value, attenuation );
		}
	}

	// flash and barrel effects
	
	if( weapon == WEAP_GUNBLADE ) // gunblade is special
	{
		if( fireMode == FIRE_MODE_STRONG )
		{
			// light flash
			if( cg_weaponFlashes->integer && weaponInfo->flashTime )
				cg_entPModels[entNum].flash_time = cg.time + weaponInfo->flashTime;
		}
		else
		{
			// start barrel rotation or offsetting
			if( weaponInfo->barrelTime )
				cg_entPModels[entNum].barrel_time = cg.time + weaponInfo->barrelTime;
		}
	}
	else
	{
		// light flash
		if( cg_weaponFlashes->integer && weaponInfo->flashTime )
			cg_entPModels[entNum].flash_time = cg.time + weaponInfo->flashTime;

		// start barrel rotation or offsetting
		if( weaponInfo->barrelTime )
			cg_entPModels[entNum].barrel_time = cg.time + weaponInfo->barrelTime;
	}

	// add animation to the player model
	switch( weapon )
	{
	case WEAP_NONE:
		break;

	case WEAP_GUNBLADE:
		if( fireMode == FIRE_MODE_WEAK )
			CG_PModel_AddAnimation( entNum, 0, TORSO_SHOOT_BLADE, 0, EVENT_CHANNEL );
		else
			CG_PModel_AddAnimation( entNum, 0, TORSO_SHOOT_PISTOL, 0, EVENT_CHANNEL );
		break;

	case WEAP_LASERGUN:
		CG_PModel_AddAnimation( entNum, 0, TORSO_SHOOT_PISTOL, 0, EVENT_CHANNEL );
		break;

	default:
	case WEAP_RIOTGUN:
	case WEAP_PLASMAGUN:
		CG_PModel_AddAnimation( entNum, 0, TORSO_SHOOT_LIGHTWEAPON, 0, EVENT_CHANNEL );
		break;

	case WEAP_ROCKETLAUNCHER:
	case WEAP_GRENADELAUNCHER:
		CG_PModel_AddAnimation( entNum, 0, TORSO_SHOOT_HEAVYWEAPON, 0, EVENT_CHANNEL );
		break;

	case WEAP_ELECTROBOLT:
		CG_PModel_AddAnimation( entNum, 0, TORSO_SHOOT_AIMWEAPON, 0, EVENT_CHANNEL );
		break;
	}

	// add animation to the view weapon model
	if( ISVIEWERENTITY( entNum ) && !cg.view.thirdperson )
		CG_ViewWeapon_StartAnimationEvent( fireMode == FIRE_MODE_STRONG ? WEAPMODEL_ATTACK_STRONG : WEAPMODEL_ATTACK_WEAK );
}

/*
* CG_LeadWaterSplash
*/
static void CG_LeadWaterSplash( trace_t *tr )
{
	int contents;
	vec_t *dir, *pos;
	
	contents = tr->contents;
	pos = tr->endpos;
	dir = tr->plane.normal;

	if( contents & CONTENTS_WATER )
		CG_ParticleEffect( pos, dir, 0.47f, 0.48f, 0.8f, 8 );
	else if( contents & CONTENTS_SLIME )
		CG_ParticleEffect( pos, dir, 0.0f, 1.0f, 0.0f, 8 );
	else if( contents & CONTENTS_LAVA )
		CG_ParticleEffect( pos, dir, 1.0f, 0.67f, 0.0f, 8 );
}

/*
* CG_LeadBubbleTrail
*/
static void CG_LeadBubbleTrail( trace_t *tr, vec3_t water_start )
{
	// if went through water, determine where the end and make a bubble trail
	vec3_t dir, pos;

	VectorSubtract( tr->endpos, water_start, dir );
	VectorNormalize( dir );
	VectorMA( tr->endpos, -2, dir, pos );

	if( CG_PointContents( pos ) & MASK_WATER )
		VectorCopy( pos, tr->endpos );
	else
		CG_Trace( tr, pos, vec3_origin, vec3_origin, water_start, tr->ent ? cg_entities[tr->ent].current.number : 0, MASK_WATER );

	VectorAdd( water_start, tr->endpos, pos );
	VectorScale( pos, 0.5, pos );

	CG_BubbleTrail( water_start, tr->endpos, 32 );
}

/*
* CG_FireLead
*/
static void CG_FireLead( int self, vec3_t start, vec3_t axis[3], int hspread, int vspread, int *seed, trace_t *trace )
{
	trace_t	tr;
	vec3_t dir;
	vec3_t end;
	float r;
	float u;
	vec3_t water_start;
	qboolean water = qfalse;
	int content_mask = MASK_SHOT | MASK_WATER;

#if 1
	// circle
	double alpha = M_PI *Q_crandom( seed ); // [-PI ..+PI]
	double s = fabs( Q_crandom( seed ) ); // [0..1]
	r = s *cos( alpha )*hspread;
	u = s *sin( alpha )*vspread;
#else
	// square
	r = Q_crandom( seed ) * hspread;
	u = Q_crandom( seed ) * vspread;
#endif

	VectorMA( start, 8192, axis[0], end );
	VectorMA( end, r, axis[1], end );
	VectorMA( end, u, axis[2], end );

	if( CG_PointContents( start ) & MASK_WATER )
	{
		water = qtrue;
		VectorCopy( start, water_start );
		content_mask &= ~MASK_WATER;
	}

	CG_Trace( &tr, start, vec3_origin, vec3_origin, end, self, content_mask );

	// see if we hit water
	if( tr.contents & MASK_WATER )
	{
		water = qtrue;
		VectorCopy( tr.endpos, water_start );

		if( !VectorCompare( start, tr.endpos ) )
		{
			vec3_t forward, right, up;

			CG_LeadWaterSplash( &tr );

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
		CG_Trace( &tr, water_start, vec3_origin, vec3_origin, end, self, MASK_SHOT );
	}

	// save the final trace
	*trace = tr;

	// if went through water, determine where the end and make a bubble trail
	if( water )
		CG_LeadBubbleTrail( trace, water_start );
}

/*
* CG_BulletImpact
*/
/*
static void CG_BulletImpact( trace_t *tr )
{
	// bullet impact
	CG_BulletExplosion( tr->endpos, NULL, tr );

	// spawn decal
	CG_SpawnDecal( tr->endpos, tr->plane.normal, random()*360, 8, 1, 1, 1, 1, 8, 1, qfalse, CG_MediaShader( cgs.media.shaderBulletMark ) );

	// throw particles on dust
	if( tr->surfFlags & SURF_DUST )
		CG_ParticleEffect( tr->endpos, tr->plane.normal, 0.30f, 0.30f, 0.25f, 20 );

	// impact sound
	trap_S_StartFixedSound( CG_MediaSfx( cgs.media.sfxRic[rand()&2] ), tr->endpos, CHAN_AUTO, cg_volume_effects->value, ATTN_STATIC );
}
*/

/*
* CG_FireBullet
*/
static void CG_FireBullet( int self, vec3_t start, vec3_t forward, int count, int spread, int seed, void ( *impact )(trace_t *tr /*, int impactnum*/) )
{
	int i;
	trace_t tr;
	vec3_t dir, axis[3];
	qboolean takedamage;

	// calculate normal vectors
	VecToAngles( forward, dir );
	AngleVectors( dir, axis[0], axis[1], axis[2] );

	for( i = 0; i < count; i++ )
	{
		CG_FireLead( self, start, axis, spread, spread, &seed, &tr );
		takedamage = tr.ent && ( cg_entities[tr.ent].current.effects & EF_TAKEDAMAGE );

		if( tr.fraction < 1.0f && !takedamage && !( tr.surfFlags & SURF_NOIMPACT ) )
			impact( &tr );
	}
}

/*
* CG_BulletImpact
*/
static void CG_BulletImpact( trace_t *tr )
{
	// bullet impact
	CG_BulletExplosion( tr->endpos, NULL, tr );

	// throw particles on dust
	if( tr->surfFlags & SURF_DUST )
		CG_ParticleEffect( tr->endpos, tr->plane.normal, 0.30f, 0.30f, 0.25f, 20 );

	// spawn decal
	CG_SpawnDecal( tr->endpos, tr->plane.normal, random()*360, 8, 1, 1, 1, 1, 8, 1, qfalse, CG_MediaShader( cgs.media.shaderBulletMark ) );
}

static void CG_Event_FireMachinegun( vec3_t origin, vec3_t dir, int weapon, int firemode, int seed, int owner )
{
	float r, u;
	double alpha, s;
	trace_t trace, *water_trace;
	gs_weapon_definition_t *weapondef = GS_GetWeaponDef( weapon );
	firedef_t *firedef = ( firemode ) ? &weapondef->firedef : &weapondef->firedef_weak;
	int range = firedef->timeout, spread = firedef->spread;

	// circle shape
	alpha = M_PI * Q_crandom( &seed ); // [-PI ..+PI]
	s = fabs( Q_crandom( &seed ) ); // [0..1]
	r = s * cos( alpha ) * spread;
	u = s * sin( alpha ) * spread;

	water_trace = GS_TraceBullet( &trace, origin, dir, r, u, range, owner, 0 );
	if( water_trace )
	{
		if( !VectorCompare( water_trace->endpos, origin ) )
			CG_LeadWaterSplash( water_trace );
	}

	if( trace.ent != -1 && !( trace.surfFlags & SURF_NOIMPACT ) )
	{
		CG_BulletImpact( &trace );

		if( !water_trace )
		{
			if( trace.surfFlags & SURF_FLESH ||
				( trace.ent > 0 && cg_entities[trace.ent].current.type == ET_PLAYER ) ||
				( trace.ent > 0 && cg_entities[trace.ent].current.type == ET_CORPSE ) )
			{
				// flesh impact sound
			}
			else
				trap_S_StartFixedSound( CG_MediaSfx( cgs.media.sfxRic[ rand()&2 ] ), trace.endpos, CHAN_AUTO, cg_volume_effects->value, ATTN_STATIC );
		}
	}

	if( water_trace )
		CG_LeadBubbleTrail( &trace, water_trace->endpos );
}

/*
* CG_Fire_SpiralPattern
*/
static void CG_Fire_SpiralPattern( vec3_t start, vec3_t dir, int *seed, int ignore, int count, int spread, int range, void ( *impact )(trace_t *tr) )
{
	int i;
	float r;
	float u;
	trace_t trace, *water_trace;

	assert( seed );

	for( i = 0; i < count; i++ )
	{
		r = cos( *seed + i ) * spread * i;
		u = sin( *seed + i ) * spread * i;

		water_trace = GS_TraceBullet( &trace, start, dir, r, u, range, ignore, 0 );
		if( water_trace )
		{
			trace_t *tr = water_trace;
			if( !VectorCompare( tr->endpos, start ) )
				CG_LeadWaterSplash( tr );
		}

		if( trace.ent != -1 && !( trace.surfFlags & SURF_NOIMPACT ) )
			impact( &trace );

		if( water_trace )
			CG_LeadBubbleTrail( &trace, water_trace->endpos );
	}
}

/*
* CG_Event_FireRiotgun
*/
static void CG_Event_FireRiotgun( vec3_t origin, vec3_t dir, int weapon, int firemode, int seed, int owner ) 
{
	trace_t	trace;
	vec3_t end;
	gs_weapon_definition_t *weapondef = GS_GetWeaponDef( weapon );
	firedef_t *firedef = ( firemode ) ? &weapondef->firedef : &weapondef->firedef_weak;

	CG_Fire_SpiralPattern( origin, dir, &seed, owner, firedef->projectile_count, firedef->spread, firedef->timeout, CG_BulletImpact );

	// spawn a single sound at the impact
	VectorMA( origin, firedef->timeout, dir, end );
	CG_Trace( &trace, origin, vec3_origin, vec3_origin, end, owner, MASK_SHOT );

	if( trace.ent != -1 && !( trace.surfFlags & SURF_NOIMPACT ) )
	{
		if( firedef->fire_mode == FIRE_MODE_STRONG )
		{
			trap_S_StartFixedSound( CG_MediaSfx( cgs.media.sfxRiotgunStrongHit ), trace.endpos, CHAN_AUTO,
				cg_volume_effects->value, ATTN_IDLE );
		}
		else
		{
			trap_S_StartFixedSound( CG_MediaSfx( cgs.media.sfxRiotgunWeakHit ), trace.endpos, CHAN_AUTO,
				cg_volume_effects->value, ATTN_IDLE );
		}
	}
}


//==================================================================

//=========================================================
#define CG_MAX_ANNOUNCER_EVENTS	32
#define CG_MAX_ANNOUNCER_EVENTS_MASK ( CG_MAX_ANNOUNCER_EVENTS - 1 )
#define CG_ANNOUNCER_EVENTS_FRAMETIME 1.5f // the announcer will speak each 1.5 seconds
typedef struct cg_announcerevent_s
{
	struct sfx_s *sound;
} cg_announcerevent_t;
cg_announcerevent_t cg_announcerEvents[CG_MAX_ANNOUNCER_EVENTS];
static int cg_announcerEventsCurrent = 0;
static int cg_announcerEventsHead = 0;
static float cg_announcerEventsDelay = 0.0f;

/*
* CG_ClearAnnouncerEvents
*/
void CG_ClearAnnouncerEvents( void )
{
	cg_announcerEventsCurrent = cg_announcerEventsHead = 0;
}

/*
* CG_AddAnnouncerEvent
*/
void CG_AddAnnouncerEvent( struct sfx_s *sound, qboolean queued )
{
	if( !sound )
		return;

	if( !queued )
	{
		trap_S_StartGlobalSound( sound, CHAN_ANNOUNCER, cg_volume_announcer->value );
		cg_announcerEventsDelay = CG_ANNOUNCER_EVENTS_FRAMETIME; // wait
		return;
	}

	if( cg_announcerEventsCurrent + CG_MAX_ANNOUNCER_EVENTS >= cg_announcerEventsHead )
	{
		// full buffer (we do nothing, just let it overwrite the oldest
	}

	// add it
	cg_announcerEvents[cg_announcerEventsHead&CG_MAX_ANNOUNCER_EVENTS_MASK].sound = sound;
	cg_announcerEventsHead++;
}

/*
* CG_ReleaseAnnouncerEvents
*/
void CG_ReleaseAnnouncerEvents( void )
{
	// see if enough time has passed
	cg_announcerEventsDelay -= cg.realFrameTime;
	if( cg_announcerEventsDelay > 0.0f )
		return;

	if( cg_announcerEventsCurrent < cg_announcerEventsHead )
	{
		struct sfx_s *sound;

		// play the event
		sound = cg_announcerEvents[cg_announcerEventsCurrent & CG_MAX_ANNOUNCER_EVENTS_MASK].sound;
		if( sound )
		{
			trap_S_StartGlobalSound( sound, CHAN_ANNOUNCER, cg_volume_announcer->value );
			cg_announcerEventsDelay = CG_ANNOUNCER_EVENTS_FRAMETIME; // wait
		}
		cg_announcerEventsCurrent++;
	}
	else
	{
		cg_announcerEventsDelay = 0; // no wait
	}
}

/*
* CG_StartVoiceTokenEffect
*/
static void CG_StartVoiceTokenEffect( int entNum, int type, int vsay )
{
	centity_t *cent;
	cgs_media_handle_t *sound = NULL;

	if( !cg_voiceChats->integer || cg_volume_voicechats->value <= 0.0f )
		return;

	cent = &cg_entities[entNum];

	// set the icon effect
	cent->localEffects[LOCALEFFECT_VSAY_HEADICON] = vsay;
	cent->localEffects[LOCALEFFECT_VSAY_HEADICON_TIMEOUT] = cg.time + HEADICON_TIMEOUT;

	// play the sound
	sound = cgs.media.sfxVSaySounds[vsay];

	// played as it was made by the 1st person player
	trap_S_StartGlobalSound( CG_MediaSfx( sound ), CHAN_AUTO, cg_volume_voicechats->value );
}

//==================================================================

//==================================================================

/*
* CG_Event_Fall
*/
void CG_Event_Fall( entity_state_t *state, int parm )
{
	if( ISVIEWERENTITY( state->number ) )
	{
		if( cg.frame.playerState.pmove.pm_type != PM_NORMAL )
		{
			CG_SexedSound( state->number, CHAN_AUTO, "*fall_0", cg_volume_players->value );
			return;
		}

		CG_ViewWeapon_StartFallKickEff( parm );

		if( parm > 0 )
			CG_DamageIndicatorAdd( parm, tv( 0, 0, 1 ) );
	}

	if( parm > 10 )
	{
		CG_SexedSound( state->number, CHAN_PAIN, "*fall_2", cg_volume_players->value );
		switch( (int)brandom( 0, 3 ) )
		{
		case 0:
			CG_PModel_AddAnimation( state->number, 0, TORSO_PAIN1, 0, EVENT_CHANNEL );
			break;
		case 1:
			CG_PModel_AddAnimation( state->number, 0, TORSO_PAIN2, 0, EVENT_CHANNEL );
			break;
		case 2:
		default:
			CG_PModel_AddAnimation( state->number, 0, TORSO_PAIN3, 0, EVENT_CHANNEL );
			break;
		}
	}
	else if( parm > 0 )
	{
		CG_SexedSound( state->number, CHAN_PAIN, "*fall_1", cg_volume_players->value );
	}
	else
		CG_SexedSound( state->number, CHAN_PAIN, "*fall_0", cg_volume_players->value );

	// smoke effect
	if( parm > 0 && ( cg_cartoonEffects->integer & 2 ) )
	{
		vec3_t start, end;
		trace_t trace;

		if( ISVIEWERENTITY( state->number ) )
			VectorCopy( cg.predictedPlayerState.pmove.origin, start );
		else
			VectorCopy( state->origin, start );

		VectorCopy( start, end );
		end[2] += playerbox_stand_mins[2] - 48.0f;

		CG_Trace( &trace, start, vec3_origin, vec3_origin, end, state->number, MASK_PLAYERSOLID );
		if( trace.ent == -1 )
		{
			start[2] += playerbox_stand_mins[2] + 8;
			CG_DustCircle( start, tv( 0, 0, 1 ), 50, 12 );
		}
		else if( !(trace.surfFlags & SURF_NODAMAGE) )
		{
			VectorMA( trace.endpos, 8, trace.plane.normal, end );
			CG_DustCircle( end, trace.plane.normal, 50, 12 );
		}
	}
}

/*
* CG_Event_Pain
*/
void CG_Event_Pain( entity_state_t *state, int parm )
{
	if( parm == PAIN_WARSHELL )
	{
		if( ISVIEWERENTITY( state->number ) )
			trap_S_StartGlobalSound( CG_MediaSfx( cgs.media.sfxShellHit ), CHAN_PAIN, cg_volume_players->value );
		else
			trap_S_StartRelativeSound( CG_MediaSfx( cgs.media.sfxShellHit ), state->number, CHAN_PAIN, cg_volume_players->value, ATTN_NORM );
	}
	else
	{
		CG_SexedSound( state->number, CHAN_PAIN, va( S_PLAYER_PAINS, 25*( parm+1 ) ), cg_volume_players->value );
	}

	switch( (int)brandom( 0, 3 ) )
	{
	case 0:
		CG_PModel_AddAnimation( state->number, 0, TORSO_PAIN1, 0, EVENT_CHANNEL );
		break;
	case 1:
		CG_PModel_AddAnimation( state->number, 0, TORSO_PAIN2, 0, EVENT_CHANNEL );
		break;
	case 2:
	default:
		CG_PModel_AddAnimation( state->number, 0, TORSO_PAIN3, 0, EVENT_CHANNEL );
		break;
	}
}

/*
* CG_Event_Die
*/
void CG_Event_Die( entity_state_t *state, int parm )
{
	CG_SexedSound( state->number, CHAN_PAIN, S_PLAYER_DEATH, cg_volume_players->value );
	switch( parm )
	{
	case 0:
	default:
		CG_PModel_AddAnimation( state->number, BOTH_DEATH1, BOTH_DEATH1, ANIM_NONE, EVENT_CHANNEL );
		break;
	case 1:
		CG_PModel_AddAnimation( state->number, BOTH_DEATH2, BOTH_DEATH2, ANIM_NONE, EVENT_CHANNEL );
		break;
	case 2:
		CG_PModel_AddAnimation( state->number, BOTH_DEATH3, BOTH_DEATH3, ANIM_NONE, EVENT_CHANNEL );
		break;
	}
}

/*
* CG_Event_Dash
*/
void CG_Event_Dash( entity_state_t *state, int parm )
{
	switch( parm )
	{
	default:
		break;
	case 0: // dash front
		CG_PModel_AddAnimation( state->number, LEGS_DASH, 0, 0, EVENT_CHANNEL );
		CG_SexedSound( state->number, CHAN_BODY, va( S_PLAYER_DASH_1_to_2, ( rand()&1 )+1 ), cg_volume_players->value );
		break;
	case 1: // dash left
		CG_PModel_AddAnimation( state->number, LEGS_DASH_LEFT, 0, 0, EVENT_CHANNEL );
		CG_SexedSound( state->number, CHAN_BODY, va( S_PLAYER_DASH_1_to_2, ( rand()&1 )+1 ), cg_volume_players->value );
		break;
	case 2: // dash right
		CG_PModel_AddAnimation( state->number, LEGS_DASH_RIGHT, 0, 0, EVENT_CHANNEL );
		CG_SexedSound( state->number, CHAN_BODY, va( S_PLAYER_DASH_1_to_2, ( rand()&1 )+1 ), cg_volume_players->value );
		break;
	case 3: // dash back
		CG_PModel_AddAnimation( state->number, LEGS_DASH_BACK, 0, 0, EVENT_CHANNEL );
		CG_SexedSound( state->number, CHAN_BODY, va( S_PLAYER_DASH_1_to_2, ( rand()&1 )+1 ), cg_volume_players->value );
		break;
	}
	CG_Dash( state ); // Dash smoke effect
	// since most dash animations jump with right leg, reset the jump to start with left leg after a dash
	cg_entities[state->number].jumpedLeft = qtrue;

  //racesow - lm: filter out other players
  if( ISVIEWERENTITY( state->number ))
    CG_AddJumpspeed();
  //!racesow
}

/*
* CG_Event_WallJump
*/
void CG_Event_WallJump( entity_state_t *state, int parm, int ev )
{
	vec3_t normal, forward, right;

	ByteToDir( parm, normal );

	AngleVectors( tv( state->angles[0], state->angles[1], 0 ), forward, right, NULL );

	if( DotProduct( normal, right ) > 0.3 )
		CG_PModel_AddAnimation( state->number, LEGS_WALLJUMP_RIGHT, 0, 0, EVENT_CHANNEL );
	else if( -DotProduct( normal, right ) > 0.3 )
		CG_PModel_AddAnimation( state->number, LEGS_WALLJUMP_LEFT, 0, 0, EVENT_CHANNEL );
	else if( -DotProduct( normal, forward ) > 0.3 )
		CG_PModel_AddAnimation( state->number, LEGS_WALLJUMP_BACK, 0, 0, EVENT_CHANNEL );
	else
		CG_PModel_AddAnimation( state->number, LEGS_WALLJUMP, 0, 0, EVENT_CHANNEL );

	if( ev == EV_WALLJUMP_FAILED )
	{
		if( ISVIEWERENTITY( state->number ) )
			trap_S_StartGlobalSound( CG_MediaSfx( cgs.media.sfxWalljumpFailed ), CHAN_BODY, cg_volume_effects->value );
		else
			trap_S_StartRelativeSound( CG_MediaSfx( cgs.media.sfxWalljumpFailed ), state->number, CHAN_BODY, cg_volume_effects->value, ATTN_NORM );
	}
	else
	{
		CG_SexedSound( state->number, CHAN_BODY, va( S_PLAYER_WALLJUMP_1_to_2, ( rand()&1 )+1 ), cg_volume_players->value );
	
		// smoke effect
		if( cg_cartoonEffects->integer & 1 )
		{
			vec3_t pos;
			VectorCopy( state->origin, pos );
			pos[2] += 15;
			CG_DustCircle( pos, normal, 65, 12 );
		}
	}

  //racesow - lm: filter out other players
  if( ISVIEWERENTITY( state->number ))
    CG_AddJumpspeed();
  //!racesow
}

/*
* CG_Event_DoubleJump
*/
void CG_Event_DoubleJump( entity_state_t *state, int parm )
{
	CG_SexedSound( state->number, CHAN_BODY, va( S_PLAYER_JUMP_1_to_2, ( rand()&1 )+1 ), cg_volume_players->value );
}

/*
* CG_Event_Jump
*/
void CG_Event_Jump( entity_state_t *state, int parm )
{
#define MOVEDIREPSILON 0.25f
	centity_t *cent;
	int xyspeedcheck;

	cent = &cg_entities[state->number];
	xyspeedcheck = SQRTFAST( cent->animVelocity[0]*cent->animVelocity[0] + cent->animVelocity[1]*cent->animVelocity[1] );
	if( xyspeedcheck < 100 )
	{                      // the player is jumping on the same place, not running
		CG_PModel_AddAnimation( state->number, LEGS_JUMP_NEUTRAL, 0, 0, EVENT_CHANNEL );
		CG_SexedSound( state->number, CHAN_BODY, va( S_PLAYER_JUMP_1_to_2, ( rand()&1 )+1 ), cg_volume_players->value );
	}
	else
	{
		vec3_t movedir, viewaxis[3];
		movedir[0] = cent->animVelocity[0];
		movedir[1] = cent->animVelocity[1];
		movedir[2] = 0;
		VectorNormalizeFast( movedir );
		AngleVectors( tv( 0, cent->current.angles[YAW], 0 ), viewaxis[FORWARD], viewaxis[RIGHT], viewaxis[UP] );
		// see what's his relative movement direction
		if( DotProduct( movedir, viewaxis[FORWARD] ) > MOVEDIREPSILON )
		{
			cent->jumpedLeft = !cent->jumpedLeft;
			if( !cent->jumpedLeft )
			{
				CG_PModel_AddAnimation( state->number, LEGS_JUMP_LEG2, 0, 0, EVENT_CHANNEL );
				CG_SexedSound( state->number, CHAN_BODY, va( S_PLAYER_JUMP_1_to_2, ( rand()&1 )+1 ), cg_volume_players->value );
			}
			else
			{
				CG_PModel_AddAnimation( state->number, LEGS_JUMP_LEG1, 0, 0, EVENT_CHANNEL );
				CG_SexedSound( state->number, CHAN_BODY, va( S_PLAYER_JUMP_1_to_2, ( rand()&1 )+1 ), cg_volume_players->value );
			}
		}
		else
		{
			CG_PModel_AddAnimation( state->number, LEGS_JUMP_NEUTRAL, 0, 0, EVENT_CHANNEL );
			CG_SexedSound( state->number, CHAN_BODY, va( S_PLAYER_JUMP_1_to_2, ( rand()&1 )+1 ), cg_volume_players->value );
		}
	}
#undef MOVEDIREPSILON

  //racesow - lm: filter out other players
  if( ISVIEWERENTITY( state->number ))
    CG_AddJumpspeed();
  //!racesow
}

/*
* CG_EntityEvent
*/
void CG_EntityEvent( entity_state_t *ent, int ev, int parm, qboolean predicted )
{
	//static orientation_t projection;
	vec3_t dir;
	qboolean viewer = ISVIEWERENTITY( ent->number );
	vec4_t color;
	int weapon = 0, fireMode = 0, count = 0;

	if( viewer && ( ev < PREDICTABLE_EVENTS_MAX ) && ( predicted != cg.view.playerPrediction ) )
		return;

	switch( ev )
	{
	case EV_NONE:
	default:
		break;

	//  PREDICTABLE EVENTS

	case EV_WEAPONACTIVATE:
		CG_PModel_AddAnimation( ent->number, 0, TORSO_WEAPON_SWITCHIN, 0, EVENT_CHANNEL );
		weapon = ( parm & ~EV_INVERSE );
		fireMode = ( parm & EV_INVERSE ) ? FIRE_MODE_STRONG : FIRE_MODE_WEAK;
		if( predicted )
		{
			cg_entities[ent->number].current.weapon = weapon;
			if( fireMode == FIRE_MODE_STRONG )
				cg_entities[ent->number].current.effects |= EF_STRONG_WEAPON;

			CG_ViewWeapon_RefreshAnimation( &cg.weapon );
		}

		if( viewer )
			cg.predictedWeaponSwitch = 0;

		if( viewer )
			trap_S_StartGlobalSound( CG_MediaSfx( cgs.media.sfxWeaponUp ), CHAN_AUTO, cg_volume_effects->value );
		else
			trap_S_StartFixedSound( CG_MediaSfx( cgs.media.sfxWeaponUp ), ent->origin, CHAN_AUTO, cg_volume_effects->value, ATTN_NORM );
		break;

	case EV_SMOOTHREFIREWEAPON: // the server never sends this event
		if( predicted )
		{
			weapon = ( parm & ~EV_INVERSE );
			fireMode = ( parm & EV_INVERSE ) ? FIRE_MODE_STRONG : FIRE_MODE_WEAK;

			cg_entities[ent->number].current.weapon = weapon;
			if( fireMode == FIRE_MODE_STRONG )
				cg_entities[ent->number].current.effects |= EF_STRONG_WEAPON;

			CG_ViewWeapon_RefreshAnimation( &cg.weapon );

			if( weapon == WEAP_LASERGUN )
				CG_Event_LaserBeam( ent->number, weapon, fireMode );
		}
		break;

	case EV_FIREWEAPON:
		weapon = ( parm & ~EV_INVERSE );
		fireMode = ( parm & EV_INVERSE ) ? FIRE_MODE_STRONG : FIRE_MODE_WEAK;

		if( predicted )
		{
			cg_entities[ent->number].current.weapon = weapon;
			if( fireMode == FIRE_MODE_STRONG )
				cg_entities[ent->number].current.effects |= EF_STRONG_WEAPON;
		}

		CG_FireWeaponEvent( ent->number, weapon, fireMode );
		
		// riotgun bullets, electrobolt and instagun beams are predicted when the weapon is fired
		if( predicted )
		{
			vec3_t origin;

			if( ( weapon == WEAP_ELECTROBOLT 
#ifndef ELECTROBOLT_TEST
				&& fireMode == FIRE_MODE_STRONG
#endif
				) 
				|| weapon == WEAP_INSTAGUN )
			{
				VectorCopy( cg.predictedPlayerState.pmove.origin, origin );
				origin[2] += cg.predictedPlayerState.viewheight;
				AngleVectors( cg.predictedPlayerState.viewangles, dir, NULL, NULL );
				CG_Event_WeaponBeam( origin, dir, cg.predictedPlayerState.POVnum, weapon, fireMode );
			}
			else if( weapon == WEAP_RIOTGUN || weapon == WEAP_MACHINEGUN )
			{
				int seed = cg.predictedEventTimes[EV_FIREWEAPON] & 255;

				VectorCopy( cg.predictedPlayerState.pmove.origin, origin );
				origin[2] += cg.predictedPlayerState.viewheight;
				AngleVectors( cg.predictedPlayerState.viewangles, dir, NULL, NULL );

				if( weapon == WEAP_RIOTGUN )
					CG_Event_FireRiotgun( origin, dir, weapon, fireMode, seed, cg.predictedPlayerState.POVnum );
				else
					CG_Event_FireMachinegun( origin, dir, weapon, fireMode, seed, cg.predictedPlayerState.POVnum );
			}
			else if( weapon == WEAP_LASERGUN )
			{
				CG_Event_LaserBeam( ent->number, weapon, fireMode );
			}
		}
		break;

	case EV_ELECTROTRAIL:
		// check the owner for predicted case
		if( ISVIEWERENTITY( parm ) && ( ev < PREDICTABLE_EVENTS_MAX ) && ( predicted != cg.view.playerPrediction ) )
			return;
		CG_Event_WeaponBeam( ent->origin, ent->origin2, parm, WEAP_ELECTROBOLT, ent->firemode );
		break;

	case EV_INSTATRAIL:
		// check the owner for predicted case
		if( ISVIEWERENTITY( parm ) && ( ev < PREDICTABLE_EVENTS_MAX ) && ( predicted != cg.view.playerPrediction ) )
			return;
		CG_Event_WeaponBeam( ent->origin, ent->origin2, parm, WEAP_INSTAGUN, FIRE_MODE_STRONG );
		break;

	case EV_FIRE_RIOTGUN:
	case EV_FIRE_BULLET:
		{
			// check the owner for predicted case
			if( ISVIEWERENTITY( ent->ownerNum ) && ( ev < PREDICTABLE_EVENTS_MAX ) && ( predicted != cg.view.playerPrediction ) )
				return;

			if( ev == EV_FIRE_RIOTGUN )
				CG_Event_FireRiotgun( ent->origin, ent->origin2, ( ent->weapon & ~EV_INVERSE ),
					( ent->weapon & EV_INVERSE ), parm, ent->ownerNum );
			else
				CG_Event_FireMachinegun( ent->origin, ent->origin2, ( ent->weapon & ~EV_INVERSE ), 
					( ent->weapon & EV_INVERSE ), parm, ent->ownerNum );
		}
		break;

	case EV_NOAMMOCLICK:
		if( viewer )
			trap_S_StartGlobalSound( CG_MediaSfx( cgs.media.sfxWeaponUpNoAmmo ), CHAN_ITEM, cg_volume_effects->value );
		else
			trap_S_StartFixedSound( CG_MediaSfx( cgs.media.sfxWeaponUpNoAmmo ), ent->origin, CHAN_ITEM, cg_volume_effects->value, ATTN_IDLE );
		break;

	case EV_DASH:
		CG_Event_Dash( ent, parm );
		break;

	case EV_WALLJUMP:
	case EV_WALLJUMP_FAILED:
		CG_Event_WallJump( ent, parm, ev );
		break;

	case EV_DOUBLEJUMP:
		CG_Event_DoubleJump( ent, parm );
		break;

	case EV_JUMP:
		CG_Event_Jump( ent, parm );
		break;

	case EV_JUMP_PAD:
		CG_TouchJumpPad( ent->number );
		break;

	case EV_FALL:
		CG_Event_Fall( ent, parm );
		break;


		//  NON PREDICTABLE EVENTS

	case EV_WEAPONDROP: // deactivate is not predictable
		CG_PModel_AddAnimation( ent->number, 0, TORSO_WEAPON_SWITCHOUT, 0, EVENT_CHANNEL );
		break;

	case EV_SEXEDSOUND:
		if( parm == 2 )
			CG_SexedSound( ent->number, CHAN_AUTO, S_PLAYER_GASP, cg_volume_players->value );
		else if( parm == 1 )
			CG_SexedSound( ent->number, CHAN_AUTO, S_PLAYER_DROWN, cg_volume_players->value );
		break;

	case EV_PAIN:
		CG_Event_Pain( ent, parm );
		break;

	case EV_DIE:
		CG_Event_Die( ent, parm );
		break;

	case EV_GIB:
		break;

	case EV_EXPLOSION1:
		CG_GenericExplosion( ent->origin, vec3_origin, FIRE_MODE_WEAK, parm * 8 );
		break;

	case EV_EXPLOSION2:
		CG_GenericExplosion( ent->origin, vec3_origin, FIRE_MODE_STRONG, parm * 16 );
		break;

	case EV_GREEN_LASER:
		CG_GreenLaser( ent->origin, ent->origin2 );
		break;

	case EV_PNODE:
		color[0] = COLOR_R( ent->colorRGBA ) * ( 1.0 / 255.0 );
		color[1] = COLOR_G( ent->colorRGBA ) * ( 1.0 / 255.0 );
		color[2] = COLOR_B( ent->colorRGBA ) * ( 1.0 / 255.0 );
		color[3] = COLOR_A( ent->colorRGBA ) * ( 1.0 / 255.0 );
		CG_PLink( ent->origin, ent->origin2, color, parm );
		break;

	case EV_SPARKS:
		ByteToDir( parm, dir );
		if( ent->damage > 0 )
		{
			count = (int)( ent->damage * 0.25f );
			clamp( count, 1, 10 );
		}
		else
			count = 6;

		CG_ParticleEffect( ent->origin, dir, 1.0f, 0.67f, 0.0f, count );
		break;

	case EV_BULLET_SPARKS:
		ByteToDir( parm, dir );
		CG_BulletExplosion( ent->origin, dir, NULL );
		CG_ParticleEffect( ent->origin, dir, 1.0f, 0.67f, 0.0f, 6 );
		trap_S_StartFixedSound( CG_MediaSfx( cgs.media.sfxRic[rand()&2] ), ent->origin, CHAN_AUTO,
			cg_volume_effects->value, ATTN_STATIC );
		break;

	case EV_LASER_SPARKS:
		ByteToDir( parm, dir );
		CG_ParticleEffect2( ent->origin, dir,
			COLOR_R( ent->colorRGBA ) * ( 1.0 / 255.0 ),
			COLOR_G( ent->colorRGBA ) * ( 1.0 / 255.0 ),
			COLOR_B( ent->colorRGBA ) * ( 1.0 / 255.0 ),
			ent->eventCount );
		break;

	case EV_GESTURE:
		CG_SexedSound( ent->number, CHAN_BODY, "*taunt", cg_volume_players->value );
		break;

	case EV_DROP:
		CG_PModel_AddAnimation( ent->number, 0, TORSO_DROP, 0, EVENT_CHANNEL );
		break;

	case EV_SPOG:
		CG_SmallPileOfGibs( ent->origin, parm, ent->origin2 );
		break;

	case EV_ITEM_RESPAWN:
		cg_entities[ent->number].respawnTime = cg.time;
		trap_S_StartRelativeSound( CG_MediaSfx( cgs.media.sfxItemRespawn ), ent->number, CHAN_AUTO,
			cg_volume_effects->value, ATTN_IDLE );
		break;

	case EV_PLAYER_RESPAWN:
		if( (unsigned)ent->ownerNum == cgs.playerNum + 1 )
		{
			CG_ResetKickAngles();
			CG_ResetColorBlend();
			CG_ResetDamageIndicator();
		}
		// fallthrough
	case EV_PLAYER_TELEPORT_IN:
		if( ISVIEWERENTITY( ent->ownerNum ) )
		{
			trap_S_StartGlobalSound( CG_MediaSfx( cgs.media.sfxTeleportIn ), CHAN_AUTO,
				cg_volume_effects->value );
		}
		else
		{
			trap_S_StartFixedSound( CG_MediaSfx( cgs.media.sfxTeleportIn ), ent->origin, CHAN_AUTO,
				cg_volume_effects->value, ATTN_NORM );
		}

		if( ent->ownerNum && ent->ownerNum < gs.maxclients + 1 )
			cg_entities[ent->ownerNum].localEffects[LOCALEFFECT_EV_PLAYER_TELEPORT_IN] = cg.time;
		break;

	case EV_PLAYER_TELEPORT_OUT:
		if( ISVIEWERENTITY( ent->ownerNum ) )
		{
			trap_S_StartGlobalSound( CG_MediaSfx( cgs.media.sfxTeleportOut ), CHAN_AUTO,
				cg_volume_effects->value );
		}
		else
		{
			trap_S_StartFixedSound( CG_MediaSfx( cgs.media.sfxTeleportOut ), ent->origin, CHAN_AUTO,
				cg_volume_effects->value, ATTN_NORM );
		}
		if( ent->ownerNum && ent->ownerNum < gs.maxclients + 1 )
		{
			cg_entities[ent->ownerNum].localEffects[LOCALEFFECT_EV_PLAYER_TELEPORT_OUT] = cg.time;
			VectorCopy( ent->origin, cg_entities[ent->ownerNum].teleportedFrom );
		}
		break;

	case EV_PLASMA_EXPLOSION:
		ByteToDir( parm, dir );
		CG_PlasmaExplosion( ent->origin, dir, ent->firemode, (float)ent->weapon * 8.0f );
		if( ent->firemode == FIRE_MODE_STRONG )
		{
			trap_S_StartFixedSound( CG_MediaSfx( cgs.media.sfxPlasmaStrongHit ), ent->origin, CHAN_AUTO, cg_volume_effects->value, ATTN_IDLE );
			CG_StartKickAnglesEffect( ent->origin, 50, ent->weapon * 8, 100 );
		}
		else
		{
			trap_S_StartFixedSound( CG_MediaSfx( cgs.media.sfxPlasmaWeakHit ), ent->origin, CHAN_AUTO, cg_volume_effects->value, ATTN_IDLE );
			CG_StartKickAnglesEffect( ent->origin, 30, ent->weapon * 8, 75 );
		}
		break;

	case EV_BOLT_EXPLOSION:
		ByteToDir( parm, dir );
		CG_BoltExplosionMode( ent->origin, dir, ent->firemode );
		break;

	case EV_INSTA_EXPLOSION:
		ByteToDir( parm, dir );
		CG_InstaExplosionMode( ent->origin, dir, ent->firemode );
		break;

	case EV_GRENADE_EXPLOSION:
		if( parm )
		{    // we have a direction
			ByteToDir( parm, dir );
			CG_GrenadeExplosionMode( ent->origin, dir, ent->firemode, (float)ent->weapon*8.0f );
		}
		else
		{ // no direction
			CG_GrenadeExplosionMode( ent->origin, vec3_origin, ent->firemode, (float)ent->weapon*8.0f );
		}

		if( ent->firemode == FIRE_MODE_STRONG )
			CG_StartKickAnglesEffect( ent->origin, 135, ent->weapon*8, 325 );
		else
			CG_StartKickAnglesEffect( ent->origin, 125, ent->weapon*8, 300 );
		break;

	case EV_ROCKET_EXPLOSION:
		ByteToDir( parm, dir );
		CG_RocketExplosionMode( ent->origin, dir, ent->firemode, (float)ent->weapon * 8.0f );

		if( ent->firemode == FIRE_MODE_STRONG )
			CG_StartKickAnglesEffect( ent->origin, 135, ent->weapon * 8, 300 );
		else
			CG_StartKickAnglesEffect( ent->origin, 125, ent->weapon * 8, 275 );
		break;

	case EV_GRENADE_BOUNCE:
		if( parm == FIRE_MODE_STRONG )
			trap_S_StartRelativeSound( CG_MediaSfx( cgs.media.sfxGrenadeStrongBounce[rand()&1] ), ent->number, CHAN_AUTO, cg_volume_effects->value, ATTN_IDLE );
		else
			trap_S_StartRelativeSound( CG_MediaSfx( cgs.media.sfxGrenadeWeakBounce[rand()&1] ), ent->number, CHAN_AUTO, cg_volume_effects->value, ATTN_IDLE );
		break;

	case EV_BLADE_IMPACT:
		CG_BladeImpact( ent->origin, ent->origin2 );
		break;

	case EV_GUNBLADEBLAST_IMPACT:
		ByteToDir( parm, dir );
		CG_GunBladeBlastImpact( ent->origin, dir, (float)ent->weapon*8 );
		if( ent->skinnum > 64 )
		{
			trap_S_StartFixedSound( CG_MediaSfx( cgs.media.sfxGunbladeStrongHit[2] ), ent->origin, CHAN_AUTO,
				cg_volume_effects->value, ATTN_DISTANT );
		}
		else if( ent->skinnum > 34 )
		{
			trap_S_StartFixedSound( CG_MediaSfx( cgs.media.sfxGunbladeStrongHit[1] ), ent->origin, CHAN_AUTO,
				cg_volume_effects->value, ATTN_NORM );
		}
		else
		{
			trap_S_StartFixedSound( CG_MediaSfx( cgs.media.sfxGunbladeStrongHit[0] ), ent->origin, CHAN_AUTO,
				cg_volume_effects->value, ATTN_IDLE );
		}

		//ent->skinnum is knockback value
		CG_StartKickAnglesEffect( ent->origin, ent->skinnum*8, ent->weapon*8, 200 );
		break;

	case EV_BLOOD:
		if( cg_showBloodTrail->integer == 2 && ISVIEWERENTITY( ent->ownerNum ) )
			break;
		ByteToDir( parm, dir );
		CG_BloodDamageEffect( ent->origin, dir, ent->damage );
		CG_CartoonHitEffect( ent->origin, dir, ent->damage );
		break;

		// func movers
	case EV_PLAT_HIT_TOP:
	case EV_PLAT_HIT_BOTTOM:
	case EV_PLAT_START_MOVING:
	case EV_DOOR_HIT_TOP:
	case EV_DOOR_HIT_BOTTOM:
	case EV_DOOR_START_MOVING:
	case EV_BUTTON_FIRE:
	case EV_TRAIN_STOP:
	case EV_TRAIN_START:
		{
			vec3_t so;
			CG_GetEntitySpatilization( ent->number, so, NULL );
			trap_S_StartFixedSound( cgs.soundPrecache[parm], so, CHAN_AUTO, cg_volume_effects->value, ATTN_STATIC );
		}
		break;

	case EV_VSAY:
		CG_StartVoiceTokenEffect( ent->ownerNum, EV_VSAY, parm );
		break;
	}
}

#define ISEARLYEVENT( ev ) ( ev == EV_WEAPONDROP )

/*
* CG_FireEvents
*/
static void CG_FireEntityEvents( qboolean early )
{
	int pnum, j;
	entity_state_t *state;

	for( pnum = 0; pnum < cg.frame.numEntities; pnum++ )
	{
		state = &cg.frame.parsedEntities[pnum&( MAX_PARSE_ENTITIES-1 )];

		if( state->type == ET_SOUNDEVENT )
		{
			if( early )
				CG_SoundEntityNewState( &cg_entities[state->number] );
			continue;
		}

		for( j = 0; j < 2; j++ )
		{
			if( early == ISEARLYEVENT( state->events[j] ) )
				CG_EntityEvent( state, state->events[j], state->eventParms[j], qfalse );
		}
	}
}

/*
* CG_FirePlayerStateEvents
* This events are only received by this client, and only affect it.
*/
static void CG_FirePlayerStateEvents( void )
{
	unsigned int event, parm, i, count;
	vec3_t dir;

	if( !cg.frame.playerState.event )
		return;

	if( cg.view.POVent != (int)cg.frame.playerState.POVnum )
		return;

	for( count = 0; count < 2; count++ )
	{
		// first byte is event number, second is parm
		event = cg.frame.playerState.event[count] & 127;
		parm = cg.frame.playerState.eventParm[count] & 0xFF;

		switch( event )
		{
		case PSEV_HIT:
			if( parm > 6 )
				break;
			if( parm < 4 )
			{        // hit of some caliber
				trap_S_StartGlobalSound( CG_MediaSfx( cgs.media.sfxWeaponHit[parm] ), CHAN_AUTO, cg_volume_hitsound->value );
				CG_ScreenCrosshairDamageUpdate();
			}
			else if( parm == 4 ) // killed an enemy
			{
				trap_S_StartGlobalSound( CG_MediaSfx( cgs.media.sfxWeaponKill ), CHAN_AUTO, cg_volume_hitsound->value );
				CG_ScreenCrosshairDamageUpdate();
			}
			else
			{ // hit a teammate
				trap_S_StartGlobalSound( CG_MediaSfx( cgs.media.sfxWeaponHitTeam ), CHAN_AUTO, cg_volume_hitsound->value );
				if( cg_showhelp->integer )
				{
					if( random() <= 0.5f )
						CG_CenterPrint( "Don't shoot at members of your team!" );
					else
						CG_CenterPrint( "You are shooting at your team-mates!" );
				}
			}
			break;

		case PSEV_PICKUP:
			if( cg_pickup_flash->integer && !cg.view.thirdperson )
				CG_StartColorBlendEffect( 1.0f, 1.0f, 1.0f, 0.25f, 150 );

			// auto-switch
			if( cg_weaponAutoSwitch->integer && ( parm > WEAP_NONE && parm < WEAP_TOTAL ) )
			{
				if( !cgs.demoPlaying && cg.predictedPlayerState.pmove.pm_type == PM_NORMAL 
					&& cg.predictedPlayerState.POVnum == cgs.playerNum + 1 )
				{
					// auto-switch only works when the user didn't have the just-picked weapon
					if( !cg.oldFrame.playerState.inventory[parm] )
					{
						// switch when player's only weapon is gunblade
						if( cg_weaponAutoSwitch->integer == 2 )
						{
							for( i = WEAP_GUNBLADE + 1; i < WEAP_TOTAL; i++ )
							{
								if( i == parm )
									continue;
								if( cg.predictedPlayerState.inventory[i] )
									break;
							}

							if( i == WEAP_TOTAL ) // didn't have any weapon
								CG_UseItem( va( "%i", parm ) );

						}
						// switch when the new weapon improves player's selected weapon
						else if( cg_weaponAutoSwitch->integer == 1 )
						{
							unsigned int best = WEAP_GUNBLADE;
							for( i = WEAP_GUNBLADE + 1; i < WEAP_TOTAL; i++ )
							{
								if( i == parm )
									continue;
								if( cg.predictedPlayerState.inventory[i] )
									best = i;
							}

							if( best < parm )
								CG_UseItem( va( "%i", parm ) );
						}
					}
				}
			}
			break;

		case PSEV_DAMAGE_20:
			ByteToDir( parm, dir );
			CG_DamageIndicatorAdd( 20, dir );
			break;

		case PSEV_DAMAGE_40:
			ByteToDir( parm, dir );
			CG_DamageIndicatorAdd( 40, dir );
			break;

		case PSEV_DAMAGE_60:
			ByteToDir( parm, dir );
			CG_DamageIndicatorAdd( 60, dir );
			break;

		case PSEV_DAMAGE_80:
			ByteToDir( parm, dir );
			CG_DamageIndicatorAdd( 80, dir );
			break;

		case PSEV_INDEXEDSOUND:
			if( cgs.soundPrecache[parm] )
				trap_S_StartGlobalSound( cgs.soundPrecache[parm], CHAN_AUTO, cg_volume_effects->value );
			break;

		case PSEV_ANNOUNCER:
			CG_AddAnnouncerEvent( cgs.soundPrecache[parm], qfalse );
			break;

		case PSEV_ANNOUNCER_QUEUED:
			CG_AddAnnouncerEvent( cgs.soundPrecache[parm], qtrue );
			break;

		default:
			break;
		}
	}
}

/*
* CG_FireEvents
*/
void CG_FireEvents( qboolean early )
{
	if( !cg.fireEvents )
		return;

	CG_FireEntityEvents( early );

	if( early )
		return;

	CG_FirePlayerStateEvents();
	cg.fireEvents = qfalse;
}
