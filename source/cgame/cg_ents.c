/*
Copyright (C) 2002-2003 Victor Luchits

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

static void CG_UpdateEntities( void );

/*
* CG_FixVolumeCvars
* Don't let the user go too far away with volumes
*/
static void CG_FixVolumeCvars( void )
{
	if( developer->integer )
		return;

	if( cg_volume_players->value < 0.0f )
		trap_Cvar_SetValue( "cg_volume_players", 0.0f );
	else if( cg_volume_players->value > 2.0f )
		trap_Cvar_SetValue( "cg_volume_players", 2.0f );

	if( cg_volume_effects->value < 0.0f )
		trap_Cvar_SetValue( "cg_volume_effects", 0.0f );
	else if( cg_volume_effects->value > 2.0f )
		trap_Cvar_SetValue( "cg_volume_effects", 2.0f );

	if( cg_volume_announcer->value < 0.0f )
		trap_Cvar_SetValue( "cg_volume_announcer", 0.0f );
	else if( cg_volume_announcer->value > 2.0f )
		trap_Cvar_SetValue( "cg_volume_announcer", 2.0f );

	if( cg_volume_voicechats->value < 0.0f )
		trap_Cvar_SetValue( "cg_volume_voicechats", 0.0f );
	else if( cg_volume_voicechats->value > 2.0f )
		trap_Cvar_SetValue( "cg_volume_voicechats", 2.0f );

	if( cg_volume_hitsound->value < 0.0f )
		trap_Cvar_SetValue( "cg_volume_hitsound", 0.0f );
	else if( cg_volume_hitsound->value > 10.0f )
		trap_Cvar_SetValue( "cg_volume_hitsound", 10.0f );
}

//#define DRAWFROMWEAPON

static qboolean CG_UpdateLinearProjectilePosition( centity_t *cent )
{
	entity_state_t *state;
	float flyTime;
	unsigned int serverTime;
#define MIN_DRAWDISTANCE_FIRSTPERSON 86
#define MIN_DRAWDISTANCE_THIRDPERSON 52

	state = &cent->current;

	if( !state->linearProjectile )
		return -1;

	if( GS_MatchPaused() )
		serverTime = cg.frame.serverTime;
	else
		serverTime = cg.time;

	// add a time offset to counter antilag visualization
	if( !cgs.demoPlaying && cg_projectileAntilagOffset->value > 0.0f &&
		!ISVIEWERENTITY( state->ownerNum ) && ( cgs.playerNum + 1 != cg.predictedPlayerState.POVnum ) )
	{
		serverTime += state->modelindex2 * cg_projectileAntilagOffset->value;
	}

	if( serverTime > state->linearProjectileTimeStamp )
		flyTime = (float)( serverTime - state->linearProjectileTimeStamp ) * 0.001f;
	else if( serverTime < state->linearProjectileTimeStamp )
		flyTime = (float)( state->linearProjectileTimeStamp - serverTime ) * -0.001f;
	else
		flyTime = 0;

	VectorMA( state->origin2, flyTime, state->linearProjectileVelocity, state->origin );
#ifdef DRAWFROMWEAPON
	if( ISVIEWERENTITY( state->ownerNum ) ) // experimental. Draw linear projectiles as if coming from the weapon
	{
		VectorMA( cent->linearProjectileViewerSource, flyTime, cent->linearProjectileViewerVelocity, state->origin );
	}
#endif

	// when flyTime is negative don't offset it backwards more than PROJECTILE_PRESTEP value
	if( flyTime < 0 )
	{
		float maxBackOffset;

		if( ISVIEWERENTITY( state->ownerNum ) )
			maxBackOffset = ( PROJECTILE_PRESTEP - MIN_DRAWDISTANCE_FIRSTPERSON );
		else
			maxBackOffset = ( PROJECTILE_PRESTEP - MIN_DRAWDISTANCE_THIRDPERSON );

		if( DistanceFast( state->origin2, state->origin ) > maxBackOffset )
		{
			return qfalse;
		}
	}

	return qtrue;
#undef MIN_DRAWDISTANCE_FIRSTPERSON
#undef MIN_DRAWDISTANCE_THIRDPERSON
}

/*
* CG_NewPacketEntityState
*/
static void CG_NewPacketEntityState( entity_state_t *state )
{
	centity_t *cent;
	qboolean hasVelocity;

	cent = &cg_entities[state->number];

	hasVelocity = qfalse;

	VectorClear( cent->prevVelocity );
	cent->canExtrapolatePrev = qfalse;

	if( ISEVENTENTITY( state ) )
	{
		cent->prev = cent->current;
		cent->current = *state;
		cent->serverFrame = cg.frame.serverFrame;

		VectorClear( cent->velocity );
		cent->canExtrapolate = qfalse;
	}
	else if( state->linearProjectile )
	{
		qboolean firstTime = qfalse;

		if( ( cent->serverFrame != cg.oldFrame.serverFrame ) || state->teleported )
		{
			firstTime = qtrue;
			cent->prev = *state;
		}
		else
			cent->prev = cent->current;

		cent->current = *state;
		cent->serverFrame = cg.frame.serverFrame;

		VectorClear( cent->velocity );
		cent->canExtrapolate = qfalse;

#ifdef DRAWFROMWEAPON
		if( firstTime && ISVIEWERENTITY( state->ownerNum ) ) // experimental. Draw linear projectiles as if coming from the weapon
		{
			orientation_t projection;
			vec3_t dir, end;

			if( CG_PModel_GetProjectionSource( state->ownerNum, &projection ) )
			{
				VectorCopy( projection.origin, cent->linearProjectileViewerSource );

				// recalculate the direction from the new projection source
				VectorNormalize2( state->linearProjectileVelocity, dir );
				VectorMA( state->origin2, 16000, dir, end );
				VectorSubtract( end, cent->linearProjectileViewerSource, dir );
				VectorNormalize( dir );
				VectorScale( dir, VectorLength( state->linearProjectileVelocity ), cent->linearProjectileViewerVelocity );
			}
			else
			{
				//CG_Printf( "Couldn't get projection source\n" );
				VectorCopy( state->linearProjectileVelocity, cent->linearProjectileViewerVelocity );
				VectorCopy( state->origin2, cent->linearProjectileViewerSource );
			}
		}
#endif
		cent->linearProjectileCanDraw = CG_UpdateLinearProjectilePosition( cent );

		VectorCopy( cent->current.linearProjectileVelocity, cent->velocity );
		VectorCopy( cent->current.origin, cent->trailOrigin );
	}
	else
	{
		// if it moved too much force the teleported bit
		if(  abs( cent->current.origin[0] - state->origin[0] ) > 512
			|| abs( cent->current.origin[1] - state->origin[1] ) > 512
			|| abs( cent->current.origin[2] - state->origin[2] ) > 512 )
		{
			cent->serverFrame = -99;
		}

		// some data changes will force no lerping
		if( ( state->modelindex != cent->current.modelindex ) || state->teleported )
			cent->serverFrame = -99;

		if( cent->serverFrame != cg.oldFrame.serverFrame )
		{
			// wasn't in last update, so initialize some things
			// duplicate the current state so lerping doesn't hurt anything
			cent->prev = *state;

			memset( cent->localEffects, 0, sizeof( cent->localEffects ) );

			// new entities may have old_origin
			if( state->svflags & SVF_TRANSMITORIGIN2 &&
				( state->type == ET_GENERIC || state->type == ET_GIB
				|| state->type == ET_GRENADE || state->type == ET_SPRITE
				|| state->type == ET_ITEM || state->type == ET_FLAG_BASE
				|| state->type == ET_DECAL || state->type == ET_PARTICLES ) )
			{
				VectorCopy( state->old_origin, cent->prev.origin );
			}

			// Init the animation when new into PVS
			if( cg.frame.valid && ( state->type == ET_PLAYER || state->type == ET_CORPSE ) )
			{
				cent->lastAnims = 0;
				memset( cent->lastVelocities, 0, sizeof( cent->lastVelocities ) );
				memset( cent->lastVelocitiesFrames, 0, sizeof( cent->lastVelocitiesFrames ) );
				CG_PModel_ClearEventAnimations( state->number );
				memset( &cg_entPModels[state->number].animState, 0, sizeof( cg_entPModels[state->number].animState ) );

				// if it's a player and new in PVS, remove the old power time
				// This is way far from being the right thing. But will make it less bad for now
				cg_entPModels[state->number].flash_time = cg.time;
				cg_entPModels[state->number].barrel_time = cg.time;
			}
		}
		else // shuffle the last state to previous
		{
			cent->prev = cent->current;
		}

		if( cent->serverFrame != cg.oldFrame.serverFrame )
			cent->microSmooth = 0;

		cent->current = *state;
		cent->serverFrame = cg.frame.serverFrame;
		VectorCopy( state->origin, cent->trailOrigin );

		VectorCopy( cent->velocity, cent->prevVelocity );
		//VectorCopy( cent->extrapolatedOrigin, cent->prevExtrapolatedOrigin );
		cent->canExtrapolatePrev = cent->canExtrapolate;
		cent->canExtrapolate = qfalse;
		VectorClear( cent->velocity );

		// set up velocities for this entity
		if( cgs.extrapolationTime && ( state->svflags & SVF_TRANSMITORIGIN2 ) &&
			( cent->current.type == ET_PLAYER || cent->current.type == ET_CORPSE ) )
		{
			hasVelocity = qtrue;
			VectorCopy( cent->current.origin2, cent->velocity );
			VectorCopy( cent->prev.origin2, cent->prevVelocity );
			cent->canExtrapolate = cent->canExtrapolatePrev = qtrue;
		}
		else if( !VectorCompare( cent->prev.origin, cent->current.origin ) )
		{
			float snapTime = ( cg.frame.serverTime - cg.oldFrame.serverTime );

			if( !snapTime )
				snapTime = cgs.snapFrameTime;

			VectorSubtract( cent->current.origin, cent->prev.origin, cent->velocity );
			VectorScale( cent->velocity, 1000.0f/snapTime, cent->velocity );
			hasVelocity = qtrue;
		}

		if( ( cent->current.type == ET_GENERIC || cent->current.type == ET_PLAYER
			|| cent->current.type == ET_GIB || cent->current.type == ET_GRENADE
			|| cent->current.type == ET_ITEM || cent->current.type == ET_CORPSE ) )
			cent->canExtrapolate = qtrue;

		if( ISBRUSHMODEL( cent->current.modelindex ) ) // disable extrapolation on movers
			cent->canExtrapolate = qfalse;

		//if( cent->canExtrapolate )
		//	VectorMA( cent->current.origin, 0.001f * cgs.extrapolationTime, cent->velocity, cent->extrapolatedOrigin );
	}
}

int CG_LostMultiviewPOV( void )
{
	int best, value, fallback;
	int i, index;

	best = gs.maxclients;
	index = fallback = -1;

	for( i = 0; i < cg.frame.numplayers; i++ )
	{
		value = abs( (int)cg.frame.playerStates[i].playerNum - (int)cg.multiviewPlayerNum );
		if( value == best && i > index )
			continue;

		if( value < best )
		{
			if( cg.frame.playerStates[i].pmove.pm_type == PM_SPECTATOR )
			{
				fallback = i;
				continue;
			}

			best = value;
			index = i;
		}
	}

	return index > -1 ? index : fallback;
}

static void CG_SetFramePlayerState( snapshot_t *frame, int index )
{
	frame->playerState = frame->playerStates[index];
	if( cgs.demoPlaying || cg.frame.multipov )
	{
		frame->playerState.pmove.pm_flags |= PMF_NO_PREDICTION;
		if( frame->playerState.pmove.pm_type != PM_SPECTATOR )
			frame->playerState.pmove.pm_type = PM_CHASECAM;
	}

	if( ( cgs.tv || frame->playerState.POVnum != cgs.playerNum + 1 ) &&
		!frame->playerState.pmove.stats[PM_STAT_ZOOMTIME] )
	{
		frame->playerState.fov = cg_fov->integer;
		clamp( frame->playerState.fov, MIN_FOV, MAX_FOV );
	}

	if( cgs.tv )
		frame->playerState.stats[STAT_REALTEAM] = TEAM_SPECTATOR;
}

static void CG_UpdatePlayerState( void )
{
	int i;
	int index;

	// just one POV
	if( !cg.frame.multipov )
	{
		index = 0;
		cg.multiviewPlayerNum = cg.frame.playerStates[index].playerNum;
	}
	else
	{
		// find the playerState containing our current POV, then cycle playerStates
		index = -1;
		for( i = 0; i < cg.frame.numplayers; i++ )
		{
			if( cg.frame.playerStates[i].playerNum < (unsigned)gs.maxclients && cg.frame.playerStates[i].playerNum == cg.multiviewPlayerNum )
			{
				index = i;
				break;
			}
		}

		// the POV was lost, find the closer one (may go up or down, but who cares)
		if( index == -1 || cg.frame.playerStates[index].pmove.pm_type == PM_SPECTATOR )
			index = CG_LostMultiviewPOV();

		cg.multiviewPlayerNum = cg.frame.playerStates[index].playerNum;
	}

	// set up the playerstates

	// current
	index = index;
	CG_SetFramePlayerState( &cg.frame, index );

	// old
	index = -1;
	for( i = 0; i < cg.oldFrame.numplayers; i++ )
	{
		if( (int) cg.oldFrame.playerStates[i].playerNum == cg.multiviewPlayerNum )
		{
			index = i;
			break;
		}
	}

	// use the current one for old frame too, if correct POV wasn't found
	if( index == -1 )
		cg.oldFrame.playerState = cg.frame.playerState;
	else
		CG_SetFramePlayerState( &cg.oldFrame, index );

	if( cg.frame.playerState.pmove.pm_type == PM_SPECTATOR || // force clear if spectator
		cg.frame.playerState.pmove.pm_type == PM_CHASECAM )
	{
		if( !( cg.oldFrame.playerState.pmove.pm_type == PM_SPECTATOR || cg.oldFrame.playerState.pmove.pm_type == PM_CHASECAM )
			|| cg.oldFrame.serverFrame == cg.frame.serverFrame )
			trap_Cvar_ForceSet( "scr_forceclear", "1" );
	}
	else
	{
		if( ( cg.oldFrame.playerState.pmove.pm_type == PM_SPECTATOR || cg.oldFrame.playerState.pmove.pm_type == PM_CHASECAM )
			|| cg.oldFrame.serverFrame == cg.frame.serverFrame )
			trap_Cvar_ForceSet( "scr_forceclear", "0" );
	}

	cg.predictedPlayerState = cg.frame.playerState;
}

/*
* CG_NewFrameSnap
* a new frame snap has been received from the server
*/
void CG_NewFrameSnap( snapshot_t *frame, snapshot_t *lerpframe )
{
	int i;

	assert( frame );

	if( lerpframe )
		cg.oldFrame = *lerpframe;
	else
		cg.oldFrame = *frame;

	cg.frame = *frame;
	gs.gameState = frame->gameState;

	cg.portalInView = qfalse;

	if( cg_projectileAntilagOffset->value > 1.0f || cg_projectileAntilagOffset->value < 0.0f )
		trap_Cvar_ForceSet( "cg_projectileAntilagOffset", cg_projectileAntilagOffset->dvalue );

	CG_UpdatePlayerState();

	for( i = 0; i < frame->numEntities; i++ )
		CG_NewPacketEntityState( &frame->parsedEntities[i & ( MAX_PARSE_ENTITIES-1 )] );

	if( lerpframe && ( memcmp( cg.oldFrame.areabits, cg.frame.areabits, cg.frame.areabytes ) == 0 ) )
		cg.oldAreabits = qtrue;
	else
		cg.oldAreabits = qfalse;

	// request TV-channels if we haven't done so already
	if( cgs.tv && !cgs.tvRequested )
	{
		const char *autowatch;

		trap_Cmd_ExecuteText( EXEC_APPEND, "cmd channels\n" );

		autowatch = trap_Cvar_String( "autowatch" );
		if( autowatch && autowatch[0] )
		{
			trap_Cmd_ExecuteText( EXEC_APPEND, va( "cmd watch \"%s\"\n", autowatch ) ); // automatically join a channel
			trap_Cvar_ForceSet( "autowatch", "" );
		}
		else
		{
			if( !cgs.configStrings[CS_WORLDMODEL][0] )
				trap_Cmd_ExecuteText( EXEC_APPEND, "menu_open tv\n" ); // bring up the channels menu if in lobby
		}
		cgs.tvRequested = qtrue;
	}

	// a new server frame begins now
	CG_FixVolumeCvars();

	CG_BuildSolidList();
	CG_UpdateEntities();
	CG_CheckPredictionError();
	cg.predictFrom = 0; // force the prediction to be restarted from the new snapshot
	cg.fireEvents = qtrue;

	for( i = 0; i < cg.frame.numgamecommands; i++ )
	{
		int target = cg.frame.playerState.POVnum-1;
		if( cg.frame.gamecommands[i].all || cg.frame.gamecommands[i].targets[target>>3] & ( 1<<( target&7 ) ) )
			CG_GameCommand( cg.frame.gamecommandsData + cg.frame.gamecommands[i].commandOffset );
	}

	CG_FireEvents( qtrue );
}


//=============================================================


/*
==========================================================================

ADD INTERPOLATED ENTITIES TO RENDERING LIST

==========================================================================
*/

/*
* CG_CModelForEntity
*  get the collision model for the given entity, no matter if box or brush-model.
*/
struct cmodel_s *CG_CModelForEntity( int entNum )
{
	int x, zd, zu;
	centity_t *cent;
	struct cmodel_s	*cmodel = NULL;
	vec3_t bmins, bmaxs;

	if( entNum < 0 || entNum >= MAX_EDICTS )
		return NULL;

	cent = &cg_entities[entNum];

	if( cent->serverFrame != cg.frame.serverFrame )  // not present in current frame
		return NULL;

	// find the cmodel
	if( cent->current.solid == SOLID_BMODEL )
	{                                       // special value for bmodel
		cmodel = trap_CM_InlineModel( cent->current.modelindex );
	}
	else if( cent->current.solid )
	{                               // encoded bbox
		x = 8 * ( cent->current.solid & 31 );
		zd = 8 * ( ( cent->current.solid>>5 ) & 31 );
		zu = 8 * ( ( cent->current.solid>>10 ) & 63 ) - 32;

		bmins[0] = bmins[1] = -x;
		bmaxs[0] = bmaxs[1] = x;
		bmins[2] = -zd;
		bmaxs[2] = zu;
		if( cent->type == ET_PLAYER || cent->type == ET_CORPSE )
			cmodel = trap_CM_OctagonModelForBBox( bmins, bmaxs );
		else
			cmodel = trap_CM_ModelForBBox( bmins, bmaxs );
	}

	return cmodel;
}

/*
* CG_DrawEntityBox
* draw the bounding box (in brush models case the box containing the model)
*/
void CG_DrawEntityBox( centity_t *cent )
{
#ifndef PUBLIC_BUILD
	struct cmodel_s *cmodel;
	vec3_t mins, maxs;

	if( cent->ent.renderfx & RF_VIEWERMODEL )
		return;

	cmodel = CG_CModelForEntity( cent->current.number );
	if( cmodel )
	{
		trap_CM_InlineModelBounds( cmodel, mins, maxs );
		if( cg_drawEntityBoxes->integer < 2 && cent->current.solid == SOLID_BMODEL )
			return;

		// push triggers don't move so aren't interpolated
		if( cent->current.type == ET_PUSH_TRIGGER )
			CG_DrawTestBox( cent->current.origin, mins, maxs, vec3_origin );
		else
		{
			vec3_t origin;
			VectorLerp( cent->prev.origin, cg.lerpfrac, cent->current.origin, origin );
			CG_DrawTestBox( origin, mins, maxs, vec3_origin );
		}
	}
#endif
}

/*
* CG_EntAddBobEffect
*/
static void CG_EntAddBobEffect( centity_t *cent )
{
	static float scale;
	static float bob;

	scale = 0.005f + cent->current.number * 0.00001f;
	bob = 4 + cos( ( cg.time + 1000 ) * scale ) * 4;
	cent->ent.origin2[2] += bob;
	cent->ent.origin[2] += bob;
	cent->ent.lightingOrigin[2] += bob;
}

/*
* CG_EntAddTeamColorTransitionEffect
*/
static void CG_EntAddTeamColorTransitionEffect( centity_t *cent )
{
	float f;
	qbyte *currentcolor;
	vec4_t scaledcolor, newcolor;
	const vec4_t neutralcolor = { 1.0f, 1.0f, 1.0f, 1.0f };

	f = (float)cent->current.counterNum / 255.0f;
	clamp( f, 0.0f, 1.0f );

	if( cent->current.type == ET_PLAYER || cent->current.type == ET_CORPSE )
		currentcolor = CG_PlayerColorForEntity( cent->current.number, cent->ent.shaderRGBA );
	else
		currentcolor = CG_TeamColorForEntity( cent->current.number, cent->ent.shaderRGBA );

	Vector4Scale( currentcolor, 1.0/255.0, scaledcolor );
	VectorLerp( neutralcolor, f, scaledcolor, newcolor );

	cent->ent.shaderRGBA[0] = (qbyte)( newcolor[0] * 255 );
	cent->ent.shaderRGBA[1] = (qbyte)( newcolor[1] * 255 );
	cent->ent.shaderRGBA[2] = (qbyte)( newcolor[2] * 255 );
	cent->ent.shaderRGBA[3] = (qbyte)255;
}

/*
* CG_AddLinkedModel
*/
static void CG_AddLinkedModel( centity_t *cent )
{
	static entity_t	ent;
	orientation_t tag;
	struct model_s *model;

	// linear projectiles can never have a linked model. Modelindex2 is used for a different purpose
	if( cent->current.linearProjectile )
		return;

	model = cgs.modelDraw[cent->current.modelindex2];
	if( !model )
		return;

	memset( &ent, 0, sizeof( entity_t ) );
	ent.rtype = RT_MODEL;
	ent.scale = cent->ent.scale;
	ent.renderfx = cent->ent.renderfx;
	Vector4Set( ent.shaderRGBA, 255, 255, 255, 255 );
	ent.model = model;
	ent.customShader = NULL;
	ent.customSkin = NULL;
	VectorCopy( cent->ent.origin, ent.origin );
	VectorCopy( cent->ent.origin, ent.origin2 );
	VectorCopy( cent->ent.lightingOrigin, ent.lightingOrigin );
	Matrix_Copy( cent->ent.axis, ent.axis );

	if( cent->effects & EF_AMMOBOX )  // ammobox icon hack
		ent.customShader = trap_R_RegisterPic( cent->item->icon );

	if( cent->item && ( cent->item->type & IT_WEAPON ) )
	{
		if( CG_GrabTag( &tag, &cent->ent, "tag_barrel" ) )
			CG_PlaceModelOnTag( &ent, &cent->ent, &tag );
	}
	else
	{
		if( CG_GrabTag( &tag, &cent->ent, "tag_linked" ) )
			CG_PlaceModelOnTag( &ent, &cent->ent, &tag );
	}

	CG_AddColoredOutLineEffect( &ent, cent->effects, 0, 0, 0, 255 );
	CG_AddEntityToScene( &ent );
	CG_AddShellEffects( &ent, cent->effects );
}

//==========================================================================
//		ET_GENERIC
//==========================================================================

/*
* CG_UpdateGenericEnt
*/
static void CG_UpdateGenericEnt( centity_t *cent )
{
	// start from clean
	memset( &cent->ent, 0, sizeof( cent->ent ) );
	cent->ent.scale = 1.0f;
	cent->ent.renderfx = cent->renderfx;

	// set entity color based on team
	CG_TeamColorForEntity( cent->current.number, cent->ent.shaderRGBA );
	if( cent->effects & EF_OUTLINE )
		Vector4Set( cent->outlineColor, 0, 0, 0, 255 );

	// set frame
	cent->ent.frame = cent->current.frame;
	cent->ent.oldframe = cent->prev.frame;

	// set up the model
	cent->ent.rtype = RT_MODEL;
	if( cent->current.modelindex < 255 )
		cent->ent.model = cgs.modelDraw[cent->current.modelindex];

	cent->skel = CG_SkeletonForModel( cent->ent.model );
}

/*
* CG_ExtrapolateLinearProjectile
*/
void CG_ExtrapolateLinearProjectile( centity_t *cent )
{
	int i;

	cent->linearProjectileCanDraw = CG_UpdateLinearProjectilePosition( cent );

	cent->ent.backlerp = 1.0f;

	for( i = 0; i < 3; i++ )
		cent->ent.origin[i] = cent->ent.origin2[i] = cent->ent.lightingOrigin[i] = cent->current.origin[i];

	AnglesToAxis( cent->current.angles, cent->ent.axis );
}

/*
* CG_LerpGenericEnt
*/
void CG_LerpGenericEnt( centity_t *cent )
{
	int i;
	vec3_t ent_angles = { 0, 0, 0 };

	cent->ent.backlerp = 1.0f - cg.lerpfrac;

	if( ISVIEWERENTITY( cent->current.number ) || cg.view.POVent == cent->current.number )
		VectorCopy( cg.predictedPlayerState.viewangles, ent_angles );
	else
	{
		// interpolate angles
		for( i = 0; i < 3; i++ )
			ent_angles[i] = LerpAngle( cent->prev.angles[i], cent->current.angles[i], cg.lerpfrac );
	}

	if( ent_angles[0] || ent_angles[1] || ent_angles[2] )
		AnglesToAxis( ent_angles, cent->ent.axis );
	else
		Matrix_Copy( axis_identity, cent->ent.axis );

	if( cent->renderfx & RF_FRAMELERP )
	{
		// step origin discretely, because the frames
		// do the animation properly
		vec3_t delta, move;

		VectorSubtract( cent->current.old_origin, cent->current.origin, move );
		Matrix_TransformVector( cent->ent.axis, move, delta );
		VectorMA( cent->current.origin, cent->ent.backlerp, delta, cent->ent.origin );
	}
	else if( ISVIEWERENTITY( cent->current.number ) || cg.view.POVent == cent->current.number )
	{
		VectorCopy( cg.predictedPlayerState.pmove.origin, cent->ent.origin );
		VectorCopy( cent->ent.origin, cent->ent.origin2 );
	}
	else
	{
		if( cgs.extrapolationTime && cent->canExtrapolate ) // extrapolation
		{
			vec3_t origin, xorigin1, xorigin2;

			float lerpfrac = cg.lerpfrac;
			clamp( lerpfrac, 0.0f, 1.0f );

			// extrapolation with half-snapshot smoothing
			if( cg.xerpTime >= 0 || !cent->canExtrapolatePrev )
			{
				VectorMA( cent->current.origin, cg.xerpTime, cent->velocity, xorigin1 );
			}
			else
			{
				VectorMA( cent->current.origin, cg.xerpTime, cent->velocity, xorigin1 );
				if( cent->canExtrapolatePrev )
				{
					vec3_t oldPosition;

					VectorMA( cent->prev.origin, cg.oldXerpTime, cent->prevVelocity, oldPosition );
					VectorLerp( oldPosition, cg.xerpSmoothFrac, xorigin1, xorigin1 );
				}
			}


			// extrapolation with full-snapshot smoothing
			VectorMA( cent->current.origin, cg.xerpTime, cent->velocity, xorigin2 );
			if( cent->canExtrapolatePrev )
			{
				vec3_t oldPosition;

				VectorMA( cent->prev.origin, cg.oldXerpTime, cent->prevVelocity, oldPosition );
				VectorLerp( oldPosition, lerpfrac, xorigin2, xorigin2 );
			}

			VectorLerp( xorigin1, 0.5f, xorigin2, origin );


			/*
			// Interpolation between 2 extrapolated positions
			if( !cent->canExtrapolatePrev )
				VectorMA( cent->current.origin, cg.xerpTime, cent->velocity, xorigin2 );
			else
			{
				float frac = cg.lerpfrac;
				clamp( frac, 0.0f, 1.0f );
				VectorLerp( cent->prevExtrapolatedOrigin, frac, cent->extrapolatedOrigin, xorigin2 );
			}
			*/

			if( cent->microSmooth == 2 )
			{
				vec3_t oldsmoothorigin;

				VectorLerp( cent->microSmoothOrigin2, 0.65f, cent->microSmoothOrigin, oldsmoothorigin );
				VectorLerp( origin, 0.5f, oldsmoothorigin, cent->ent.origin );
			}
			else if( cent->microSmooth == 1 )
				VectorLerp( origin, 0.5f, cent->microSmoothOrigin, cent->ent.origin );
			else
				VectorCopy( origin, cent->ent.origin );

			if( cent->microSmooth )
				VectorCopy( cent->microSmoothOrigin, cent->microSmoothOrigin2 );

			VectorCopy( origin, cent->microSmoothOrigin );
			cent->microSmooth++;
			clamp_high( cent->microSmooth, 2 );

			VectorCopy( cent->ent.origin, cent->ent.origin2 );
		}
		else // plain interpolation
		{
			for( i = 0; i < 3; i++ )
				cent->ent.origin[i] = cent->ent.origin2[i] = cent->prev.origin[i] + cg.lerpfrac *
				( cent->current.origin[i] - cent->prev.origin[i] );
		}
	}

	VectorCopy( cent->ent.origin, cent->ent.lightingOrigin );
}

/*
* CG_AddGenericEnt
*/
static void CG_AddGenericEnt( centity_t *cent )
{
	if( !cent->ent.scale )
		return;

	// if set to invisible, skip
	if( !cent->current.modelindex && !( cent->effects & EF_FLAG_TRAIL ) )
		return;

	// bobbing & auto-rotation
	if( cent->effects & EF_ROTATE_AND_BOB )
	{
		CG_EntAddBobEffect( cent );
		Matrix_Copy( cg.autorotateAxis, cent->ent.axis );
	}

	if( cent->effects & EF_TEAMCOLOR_TRANSITION )
		CG_EntAddTeamColorTransitionEffect( cent );

	// render effects
	cent->ent.renderfx = cent->renderfx;

	if( !( cent->ent.renderfx & RF_NOSHADOW ) )
		cent->ent.renderfx |= RF_PLANARSHADOW;

	if( cent->item )
	{
		gsitem_t *item = cent->item;

		if( item->type & (IT_HEALTH|IT_POWERUP) )
			cent->ent.renderfx |= RF_NOSHADOW;
		else if( item->type & (IT_WEAPON|IT_AMMO|IT_ARMOR) )
			cent->ent.renderfx |= RF_PLANARSHADOW;

		if( cent->effects & EF_AMMOBOX )
		{
#ifdef DOWNSCALE_ITEMS // Ugly hack for the release. Armor models are way too big
			cent->ent.scale *= 0.90f;
#endif

			// find out the ammo box color
			if( cent->item->color && strlen( cent->item->color ) > 1 )
			{
				vec4_t scolor;
				Vector4Copy( color_table[ColorIndex( cent->item->color[1] )], scolor );
				cent->ent.shaderRGBA[0] = ( qbyte )( 255 * scolor[0] );
				cent->ent.shaderRGBA[1] = ( qbyte )( 255 * scolor[1] );
				cent->ent.shaderRGBA[2] = ( qbyte )( 255 * scolor[2] );
				cent->ent.shaderRGBA[3] = ( qbyte )( 255 * scolor[3] );
			}
			else  // set white
				Vector4Set( cent->ent.shaderRGBA, 255, 255, 255, 255 );
		}

#ifdef CGAMEGETLIGHTORIGIN
		// add shadows for items (do it before offseting for weapons)
		if( !( cent->ent.renderfx & RF_NOSHADOW ) && ( cg_shadows->integer == 1 ) )
			CG_AllocShadeBox( cent->current.number, cent->ent.origin, item_box_mins, item_box_maxs, NULL );
#endif

		// offset weapon items by their special tag
		if( cent->item->type & IT_WEAPON )
		{
			cent->ent.renderfx |= RF_OCCLUSIONTEST;
			CG_PlaceModelOnTag( &cent->ent, &cent->ent, &cgs.weaponItemTag );
		}
	}

	if( cent->skel )
	{
		// get space in cache, interpolate, transform, link
		cent->ent.boneposes = cent->ent.oldboneposes = CG_RegisterTemporaryExternalBoneposes( cent->skel );
		CG_LerpSkeletonPoses( cent->skel, cent->ent.frame, cent->ent.oldframe, cent->ent.boneposes, 1.0 - cent->ent.backlerp );
		CG_TransformBoneposes( cent->skel, cent->ent.boneposes, cent->ent.boneposes );
	}

	// flags are special
	if( cent->effects & EF_FLAG_TRAIL )
	{
		CG_AddFlagModelOnTag( cent, cent->ent.shaderRGBA, "tag_linked" );
	}

	if( !cent->current.modelindex )
		return;

	// add to refresh list
	CG_AddCentityOutLineEffect( cent );
	CG_AddEntityToScene( &cent->ent );

	if( cent->current.modelindex2 )
		CG_AddLinkedModel( cent );
}

//==========================================================================
//		ET_FLAG_BASE
//==========================================================================

/*
* CG_AddFlagModelOnTag
*/
void CG_AddFlagModelOnTag( centity_t *cent, byte_vec4_t teamcolor, char *tagname )
{
	static entity_t	flag;
	orientation_t tag;

	if( !( cent->effects & EF_FLAG_TRAIL ) )
		return;

	memset( &flag, 0, sizeof( entity_t ) );
	flag.model = trap_R_RegisterModel( PATH_FLAG_MODEL );
	if( !flag.model )
		return;

	flag.rtype = RT_MODEL;
	flag.scale = 1.0f;
	flag.renderfx = cent->ent.renderfx & ~RF_PLANARSHADOW;
	flag.customShader = NULL;
	flag.customSkin = NULL;
	flag.shaderRGBA[0] = ( qbyte )teamcolor[0];
	flag.shaderRGBA[1] = ( qbyte )teamcolor[1];
	flag.shaderRGBA[2] = ( qbyte )teamcolor[2];
	flag.shaderRGBA[3] = ( qbyte )teamcolor[3];

	VectorCopy( cent->ent.origin, flag.origin );
	VectorCopy( cent->ent.origin, flag.origin2 );
	VectorCopy( cent->ent.lightingOrigin, flag.lightingOrigin );

	// place the flag on the tag if available
	if( tagname && CG_GrabTag( &tag, &cent->ent, tagname ) )
	{
		Matrix_Copy( cent->ent.axis, flag.axis );
		CG_PlaceModelOnTag( &flag, &cent->ent, &tag );
	}
	else // Flag dropped
	{
		vec3_t angles;

		// quick & dirty client-side rotation animation, rotate once every 2 seconds
		if( !cent->fly_stoptime )
			cent->fly_stoptime = cg.time;

		angles[0] = LerpAngle( cent->prev.angles[0], cent->current.angles[0], cg.lerpfrac ) - 75; // Let it stand up 75 degrees
		angles[1] = ( 360.0 * ( ( cent->fly_stoptime - cg.time ) % 2000 ) ) / 2000.0;
		angles[2] = LerpAngle( cent->prev.angles[2], cent->current.angles[2], cg.lerpfrac );

		AnglesToAxis( angles, flag.axis );
		VectorMA( flag.origin, 16, flag.axis[0], flag.origin ); // Move the flag up a bit
	}

	CG_AddColoredOutLineEffect( &flag, EF_OUTLINE,
		(qbyte)( teamcolor[0]*0.3 ),
		(qbyte)( teamcolor[1]*0.3 ),
		(qbyte)( teamcolor[2]*0.3 ),
		255 );

	CG_AddEntityToScene( &flag );

	// add the light & energy effects
	if( CG_GrabTag( &tag, &flag, "tag_color" ) )
		CG_PlaceModelOnTag( &flag, &flag, &tag );

	// FIXME: convert this to an autosprite mesh in the flag model
	if( !( cent->ent.renderfx & RF_VIEWERMODEL ) )
	{
		flag.rtype = RT_SPRITE;
		flag.model = NULL;
		flag.renderfx = RF_NOSHADOW|RF_FULLBRIGHT;
		flag.frame = flag.oldframe = 0;
		flag.radius = 32.0f;
		flag.customShader = CG_MediaShader( cgs.media.shaderFlagFlare );

		CG_AddEntityToScene( &flag );
	}

	// if on a player, flag drops colored particles and lights up
	if( cent->current.type == ET_PLAYER )
	{
		CG_AddLightToScene( flag.origin, 350, teamcolor[0]/255, teamcolor[1]/255, teamcolor[2]/255, NULL );

		if( cent->localEffects[LOCALEFFECT_FLAGTRAIL_LAST_DROP] + FLAG_TRAIL_DROP_DELAY < cg.time )
		{
			cent->localEffects[LOCALEFFECT_FLAGTRAIL_LAST_DROP] = cg.time;
			CG_FlagTrail( flag.origin, cent->trailOrigin, cent->ent.origin, teamcolor[0]/255, teamcolor[1]/255, teamcolor[2]/255 );
		}
	}
}

/*
* CG_UpdateFlagBaseEnt
*/
static void CG_UpdateFlagBaseEnt( centity_t *cent )
{
	// set entity color based on team
	CG_TeamColorForEntity( cent->current.number, cent->ent.shaderRGBA );
	if( cent->effects & EF_OUTLINE )
		CG_SetOutlineColor( cent->outlineColor, cent->ent.shaderRGBA );

	cent->ent.scale = 1.0f;
	cent->ent.renderfx = cent->renderfx;

	cent->item = GS_FindItemByTag( cent->current.itemNum );
	if( cent->item )
		cent->effects |= cent->item->effects;

	cent->ent.rtype = RT_MODEL;
	cent->ent.frame = cent->current.frame;
	cent->ent.oldframe = cent->prev.frame;

	// set up the model
	cent->ent.model = cgs.modelDraw[cent->current.modelindex];
	cent->skel = CG_SkeletonForModel( cent->ent.model );
}

/*
* CG_AddFlagBaseEnt
*/
static void CG_AddFlagBaseEnt( centity_t *cent )
{
	if( !cent->ent.scale )
		return;

	// if set to invisible, skip
	if( !cent->current.modelindex )
		return;

	// bobbing & auto-rotation
	if( cent->current.type != ET_PLAYER && cent->effects & EF_ROTATE_AND_BOB )
	{
		CG_EntAddBobEffect( cent );
		Matrix_Copy( cg.autorotateAxis, cent->ent.axis );
	}

	// render effects
	cent->ent.renderfx = cent->renderfx | RF_PLANARSHADOW;

	// let's see: We add first the modelindex 1 (the base)

	if( cent->skel )
	{
		// get space in cache, interpolate, transform, link
		cent->ent.boneposes = cent->ent.oldboneposes = CG_RegisterTemporaryExternalBoneposes( cent->skel );
		CG_LerpSkeletonPoses( cent->skel, cent->ent.frame, cent->ent.oldframe, cent->ent.boneposes, 1.0 - cent->ent.backlerp );
		CG_TransformBoneposes( cent->skel, cent->ent.boneposes, cent->ent.boneposes );
	}

	// add to refresh list
	CG_AddCentityOutLineEffect( cent );

	CG_AddEntityToScene( &cent->ent );

	//CG_DrawTestBox( cent->ent.origin, item_box_mins, item_box_maxs, vec3_origin );

	cent->ent.customSkin = NULL;
	cent->ent.customShader = NULL;  // never use a custom skin on others

	// see if we have to add a flag
	if( cent->effects & EF_FLAG_TRAIL )
	{
		byte_vec4_t teamcolor;

		CG_AddFlagModelOnTag( cent, CG_TeamColorForEntity( cent->current.number, teamcolor ), "tag_flag1" );
	}
	// modelindex2 can add a number from 0 to 9
	else if( cent->current.modelindex2 > 0 && cent->current.modelindex2 <= 10 )
	{
		static entity_t number;
		number = cent->ent;
		Vector4Set( number.shaderRGBA, 255, 255, 255, 255 );
		number.rtype = RT_SPRITE;
		number.origin[2] += 24;
		number.origin2[2] += 24;
		number.model = NULL;
		number.radius = 12;
		number.customShader = CG_MediaShader( cgs.media.sbNums[cent->current.modelindex2 - 1] );
		CG_AddEntityToScene( &number );
	}
}

//==========================================================================
//		ET_PLAYER
//==========================================================================

/*
* CG_AddPlayerEnt
*/
static void CG_AddPlayerEnt( centity_t *cent )
{
	// render effects
	cent->ent.renderfx = cent->renderfx | RF_OCCLUSIONTEST;
#ifndef CELLSHADEDMATERIAL
	cent->ent.renderfx |= RF_MINLIGHT;
#endif

	if( ISVIEWERENTITY( cent->current.number ) )
	{
		cg.effects = cent->effects;
		VectorCopy( cent->ent.lightingOrigin, cg.lightingOrigin );
		if( !cg.view.thirdperson && cent->current.modelindex )
			cent->ent.renderfx |= RF_VIEWERMODEL; // only draw from mirrors
		if( ( cent->ent.renderfx & RF_VIEWERMODEL ) && !cg_showSelfShadow->integer )
			cent->ent.renderfx |= RF_NOSHADOW;
	}

	// if set to invisible, skip
	if( !cent->current.modelindex || cent->current.team == TEAM_SPECTATOR )
		return;

	CG_AddPModel( cent );

	// corpses can never have a model in modelindex2
	if( cent->current.type == ET_CORPSE )
		return;

	if( cent->current.modelindex2 )
		CG_AddLinkedModel( cent );
}

//==========================================================================
//		ET_SPRITE
//==========================================================================

/*
* CG_AddSpriteEnt
*/
static void CG_AddSpriteEnt( centity_t *cent )
{
	if( !cent->ent.scale )
		return;

	// if set to invisible, skip
	if( !cent->current.modelindex )
		return;

	// bobbing & auto-rotation
	if( cent->effects & EF_ROTATE_AND_BOB )
		CG_EntAddBobEffect( cent );

	if( cent->effects & EF_TEAMCOLOR_TRANSITION )
		CG_EntAddTeamColorTransitionEffect( cent );

	// render effects
	cent->ent.renderfx = cent->renderfx;

	// add to refresh list
	CG_AddEntityToScene( &cent->ent );

	if( cent->current.modelindex2 )
		CG_AddLinkedModel( cent );
}

/*
* CG_LerpSpriteEnt
*/
static void CG_LerpSpriteEnt( centity_t *cent )
{
	int i;

	// interpolate origin
	for( i = 0; i < 3; i++ )
		cent->ent.origin[i] = cent->ent.origin2[i] = cent->ent.lightingOrigin[i] = cent->prev.origin[i] + cg.lerpfrac * ( cent->current.origin[i] - cent->prev.origin[i] );

	cent->ent.radius = cent->prev.frame + cg.lerpfrac * ( cent->current.frame - cent->prev.frame );
}

/*
* CG_UpdateSpriteEnt
*/
static void CG_UpdateSpriteEnt( centity_t *cent )
{
	// start from clean
	memset( &cent->ent, 0, sizeof( cent->ent ) );
	cent->ent.scale = 1.0f;
	cent->ent.renderfx = cent->renderfx;

	// set entity color based on team
	CG_TeamColorForEntity( cent->current.number, cent->ent.shaderRGBA );

	// set up the model
	cent->ent.rtype = RT_SPRITE;
	cent->ent.model = NULL;
	cent->ent.customShader = cgs.imagePrecache[ cent->current.modelindex ];
	cent->ent.radius = cent->prev.frame;
	VectorCopy( cent->prev.origin, cent->ent.origin );
	VectorCopy( cent->prev.origin, cent->ent.origin2 );
	VectorCopy( cent->prev.origin, cent->ent.lightingOrigin );
	Matrix_Identity( cent->ent.axis );
}

//==========================================================================
//		ET_DECAL
//==========================================================================

/*
* CG_AddDecalEnt
*/
static void CG_AddDecalEnt( centity_t *cent )
{
	// if set to invisible, skip
	if( !cent->current.modelindex )
		return;

	if( cent->effects & EF_TEAMCOLOR_TRANSITION )
		CG_EntAddTeamColorTransitionEffect( cent );

	CG_AddFragmentedDecal( cent->ent.origin, cent->ent.origin2,
		cent->ent.rotation, cent->ent.radius,
		cent->ent.shaderRGBA[0]*(1.0/255.0), cent->ent.shaderRGBA[1]*(1.0/255.0), cent->ent.shaderRGBA[2]*(1.0/255.0),
		cent->ent.shaderRGBA[3]*(1.0/255.0), cent->ent.customShader );
}

/*
* CG_LerpDecalEnt
*/
static void CG_LerpDecalEnt( centity_t *cent )
{
	int i;
	float a1, a2;

	// interpolate origin
	for( i = 0; i < 3; i++ )
		cent->ent.origin[i] = cent->prev.origin[i] + cg.lerpfrac * ( cent->current.origin[i] - cent->prev.origin[i] );

	cent->ent.radius = cent->prev.frame + cg.lerpfrac * ( cent->current.frame - cent->prev.frame );

	a1 = cent->prev.modelindex2 / 255.0 * 360;
	a2 = cent->current.modelindex2 / 255.0 * 360;
	cent->ent.rotation = LerpAngle( a1, a2, cg.lerpfrac );
}

/*
* CG_UpdateDecalEnt
*/
static void CG_UpdateDecalEnt( centity_t *cent )
{
	// set entity color based on team
	CG_TeamColorForEntity( cent->current.number, cent->ent.shaderRGBA );

	// set up the null model, may be potentially needed for linked model
	cent->ent.model = NULL;
	cent->ent.customShader = cgs.imagePrecache[ cent->current.modelindex ];
	cent->ent.radius = cent->prev.frame;
	cent->ent.rotation = cent->prev.modelindex2 / 255.0 * 360;
	VectorCopy( cent->prev.origin, cent->ent.origin );
	VectorCopy( cent->prev.origin2, cent->ent.origin2 );
}

//==========================================================================
//		ET_ITEM
//==========================================================================

/*
* CG_UpdateItemEnt
*/
static void CG_UpdateItemEnt( centity_t *cent )
{
	memset( &cent->ent, 0, sizeof( cent->ent ) );
	Vector4Set( cent->ent.shaderRGBA, 255, 255, 255, 255 );

	cent->item = GS_FindItemByTag( cent->current.itemNum );
	if( !cent->item )
		return;

	cent->effects |= cent->item->effects;

	if( cg_simpleItems->integer && cent->item->simpleitem )
	{
		cent->ent.rtype = RT_SPRITE;
		cent->ent.model = NULL;
		cent->skel = NULL;
		cent->ent.renderfx = RF_NOSHADOW|RF_FULLBRIGHT;
		cent->ent.frame = cent->ent.oldframe = 0;

		cent->ent.radius = cg_simpleItemsSize->value <= 32 ? cg_simpleItemsSize->value : 32;
		if( cent->ent.radius < 1.0f )
			cent->ent.radius = 1.0f;

		if( cg_simpleItems->integer == 2 )
			cent->effects &= ~EF_ROTATE_AND_BOB;

		cent->ent.customShader = NULL;
		cent->ent.customShader = trap_R_RegisterPic( cent->item->simpleitem );
	}
	else
	{
		cent->ent.rtype = RT_MODEL;
		cent->ent.frame = cent->current.frame;
		cent->ent.oldframe = cent->prev.frame;

		if( cent->effects & EF_OUTLINE )
			Vector4Set( cent->outlineColor, 0, 0, 0, 255 ); // black

		// set up the model
		cent->ent.model = cgs.modelDraw[cent->current.modelindex];
		cent->skel = CG_SkeletonForModel( cent->ent.model );
	}
}

/*
* CG_AddItemEnt
*/
static void CG_AddItemEnt( centity_t *cent )
{
	int msec;

	if( !cent->item )
		return;

	// respawning items
	if( cent->respawnTime )
		msec = cg.time - cent->respawnTime;
	else
		msec = ITEM_RESPAWN_TIME;

	if( msec >= 0 && msec < ITEM_RESPAWN_TIME )
		cent->ent.scale = (float)msec / ITEM_RESPAWN_TIME;
	else
		cent->ent.scale = 1.0f;

	if( cent->ent.rtype != RT_SPRITE )
	{
		// weapons are special
		if( cent->item && cent->item->type & IT_WEAPON )
			cent->ent.scale *= 1.40f;

#ifdef DOWNSCALE_ITEMS // Ugly hack for release. Armor models are way too big
		if( cent->item )
		{
			if( cent->item->type & IT_ARMOR )
				cent->ent.scale *= 0.85f;
			if( cent->item->tag == HEALTH_SMALL )
				cent->ent.scale *= 0.85f;
		}
#endif

		// flags are special
		if( cent->effects & EF_FLAG_TRAIL )
		{
			CG_AddFlagModelOnTag( cent, cent->ent.shaderRGBA, NULL );
			return;
		}

		CG_AddGenericEnt( cent );
		return;
	}

	// offset the item origin up
	cent->ent.origin[2] += cent->ent.radius + 2;
	cent->ent.origin2[2] += cent->ent.radius + 2;
	if( cent->effects & EF_ROTATE_AND_BOB )
		CG_EntAddBobEffect( cent );

	Matrix_Identity( cent->ent.axis );
	CG_AddEntityToScene( &cent->ent );
}

//==========================================================================
//		ET_ITEM_TIMER
//==========================================================================

#define MAX_ITEM_TIMERS 8

static int cg_num_item_timers = 0;
static centity_t *cg_item_timers[MAX_ITEM_TIMERS];

/*
* CG_ResetItemTimers
*/
void CG_ResetItemTimers( void )
{
	cg_num_item_timers = 0;
}

/*
* CG_UpdateItemTimerEnt
*/
static void CG_UpdateItemTimerEnt( centity_t *cent )
{
	if( GS_MatchState() >= MATCH_STATE_POSTMATCH )
		return;

	cent->item = GS_FindItemByTag( cent->current.itemNum );
	if( !cent->item )
		return;

	if( cg_num_item_timers == MAX_ITEM_TIMERS )
		return;

	cent->ent.frame = cent->current.frame;
	cg_item_timers[cg_num_item_timers++] = cent;
}

/*
* CG_CompareItemTimers
*/
static int CG_CompareItemTimers( const centity_t **first, const centity_t **second )
{
	const centity_t *e1 = *first, *e2 = *second;
	const entity_state_t *s1 = &(e1->current), *s2 = &(e2->current);
	const gsitem_t *i1 = e1->item, *i2 = e2->item;
	int t1 = s1->modelindex - 1, t2 = s2->modelindex - 1;

	// special hack to order teams like this: alpha -> neutral -> beta
	if( (!t1 || !t2) && (GS_MAX_TEAMS - TEAM_ALPHA) == 2 )
	{
		if( t2 == TEAM_ALPHA || t1 == TEAM_BETA )
			return 1;
		if( t2 == TEAM_BETA || t1 == TEAM_ALPHA )
			return -1;
	}

	if( t2 > t1 )
		return -11;
	if( t2 < t1 )
		return 1;

	if( s2->origin[2] > s1->origin[2] )
		return 1;
	if( s2->origin[2] < s1->origin[2] )
		return -1;

	if( i2->type > i1->type )
		return 1;
	if( i2->type < i1->type )
		return -1;

	if( s2->number > s1->number )
		return 1;
	if( s2->number < s1->number )
		return -1;

	return 0;
}

/*
* CG_SortItemTimers
*/
static void CG_SortItemTimers( void )
{
	qsort( cg_item_timers, cg_num_item_timers, sizeof( cg_item_timers[0] ), (int (*)(const void *, const void *))CG_CompareItemTimers );
}

/*
* CG_GetItemTimerEnt
*/
centity_t *CG_GetItemTimerEnt( int num )
{
	if( num < 0 || num >= cg_num_item_timers )
		return NULL;
	return cg_item_timers[num];
}

//==========================================================================
//		ET_BEAM
//==========================================================================

/*
* CG_AddBeamEnt
*/
static void CG_AddBeamEnt( centity_t *cent )
{
	CG_QuickPolyBeam( cent->current.origin, cent->current.origin2, cent->current.frame * 0.5f, CG_MediaShader( cgs.media.shaderLaser ) ); // wsw : jalfixme: missing the color (comes inside cent->current.colorRGBA)
}

//==========================================================================
//		ET_LASERBEAM
//==========================================================================

/*
* CG_UpdateLaserbeamEnt
*/
static void CG_UpdateLaserbeamEnt( centity_t *cent )
{
	centity_t *owner;

	if( cg.view.playerPrediction && cg_predictLaserBeam->integer
		&& ISVIEWERENTITY( cent->current.ownerNum ) )
		return;

	owner = &cg_entities[cent->current.ownerNum];
	if( owner->serverFrame != cg.frame.serverFrame )
		CG_Error( "CG_UpdateLaserbeamEnt: owner is not in the snapshot\n" );

	owner->localEffects[LOCALEFFECT_LASERBEAM] = cg.time + 10;
	owner->laserCurved = ( cent->current.type == ET_CURVELASERBEAM ) ? qtrue : qfalse;

	// laser->s.origin is beam start
	// laser->s.origin2 is beam end

	VectorCopy( cent->prev.origin, owner->laserOriginOld );
	VectorCopy( cent->prev.origin2, owner->laserPointOld );

	VectorCopy( cent->current.origin, owner->laserOrigin );
	VectorCopy( cent->current.origin2, owner->laserPoint );
}

/*
* CG_LerpLaserbeamEnt
*/
static void CG_LerpLaserbeamEnt( centity_t *cent )
{
	centity_t *owner = &cg_entities[cent->current.ownerNum];

	if( cg.view.playerPrediction && cg_predictLaserBeam->integer
		&& ISVIEWERENTITY( cent->current.ownerNum ) )
		return;

	owner->localEffects[LOCALEFFECT_LASERBEAM] = cg.time + 1;
	owner->laserCurved = ( cent->current.type == ET_CURVELASERBEAM ) ? qtrue : qfalse;
}

//==========================================================================
//		ET_PORTALSURFACE
//==========================================================================

/*
* CG_UpdatePortalSurfaceEnt
*/
static void CG_UpdatePortalSurfaceEnt( centity_t *cent )
{
	// start from clean
	memset( &cent->ent, 0, sizeof( cent->ent ) );

	cent->ent.rtype = RT_PORTALSURFACE;
	Matrix_Identity( cent->ent.axis );
	VectorCopy( cent->current.origin, cent->ent.origin );
	VectorCopy( cent->current.origin2, cent->ent.origin2 );

	if( !VectorCompare( cent->ent.origin, cent->ent.origin2 ) )
	{
		cg.portalInView = qtrue;
		cent->ent.frame = cent->current.skinnum;
	}

	if( cent->current.effects & EF_NOPORTALENTS )
		cent->ent.renderfx |= RF_NOPORTALENTS;
}

/*
* CG_AddPortalSurfaceEnt
*/
static void CG_AddPortalSurfaceEnt( centity_t *cent )
{
	if( !VectorCompare( cent->ent.origin, cent->ent.origin2 ) )
	{                                                           // construct the view matrix for portal view
		if( cent->current.effects & EF_ROTATE_AND_BOB )
		{
			float phase = cent->current.frame / 256.0f;
			float speed = cent->current.modelindex2 ? cent->current.modelindex2 : 50;

			Matrix_Identity( cent->ent.axis );
			Matrix_Rotate( cent->ent.axis, 5 * sin( ( phase + cg.time * 0.001 * speed * 0.01 ) * M_TWOPI ), 1, 0, 0 );
		}
	}

	CG_AddEntityToScene( &cent->ent );
}

//==================================================
// ET_PARTICLES
//==================================================

static void CG_AddParticlesEnt( centity_t *cent )
{
	// origin = origin
	// angles = angles
	// sound = sound
	// light = light color
	// frame = speed
	// team = RGBA
	// modelindex = shader
	// modelindex2 = radius (spread)
	// effects & 0xFF = size
	// skinNum/counterNum = time (fade in seconds);
	// effects = spherical, bounce, gravity,
	// weapon = frequency

	vec3_t dir;
	float speed;
	int spriteTime;
	int spriteRadius;
	int mintime;
	vec3_t accel;
	int bounce = 0;
	qboolean expandEffect = qfalse;
	qboolean shrinkEffect = qfalse;
	vec3_t angles;
	int i;

	// duration of each particle
	spriteTime = cent->current.counterNum;
	if( !spriteTime )
		return;

	spriteRadius = cent->current.effects & 0xFF;
	if( !spriteRadius )
		return;

	if( !cent->current.weapon ) // weapon is count per second
		return;

	mintime = 1000/cent->current.weapon;

	if( cent->localEffects[LOCALEFFECT_ROCKETTRAIL_LAST_DROP] + mintime > cg.time ) // just reusing a define
		return;

	cent->localEffects[LOCALEFFECT_ROCKETTRAIL_LAST_DROP] = cg.time;

	speed = cent->current.frame;

	if( ( cent->current.effects >> 8 ) & 1 ) // SPHERICAL DROP
	{
		angles[0] = brandom( 0, 360 );
		angles[1] = brandom( 0, 360 );
		angles[2] = brandom( 0, 360 );

		AngleVectors( angles, dir, NULL, NULL );
		VectorNormalizeFast( dir );
		VectorScale( dir, speed, dir );
	}
	else // DIRECTIONAL DROP
	{
		float r, u;
		double alpha;
		double s;
		int seed = cg.time % 255;
		int spread = (unsigned)cent->current.modelindex2 * 25;

		// interpolate dropping angles
		for( i = 0; i < 3; i++ )
			angles[i] = LerpAngle( cent->prev.angles[i], cent->current.angles[i], cg.lerpfrac );

		AngleVectors( angles, cent->ent.axis[0], cent->ent.axis[1], cent->ent.axis[2] );

		alpha = M_PI * Q_crandom( &seed ); // [-PI ..+PI]
		s = fabs( Q_crandom( &seed ) ); // [0..1]
		r = s * cos( alpha ) * spread;
		u = s * sin( alpha ) * spread;

		// apply spread on the direction
		VectorMA( vec3_origin, 1024, cent->ent.axis[0], dir );
		VectorMA( dir, r, cent->ent.axis[1], dir );
		VectorMA( dir, u, cent->ent.axis[2], dir );

		VectorNormalizeFast( dir );
		VectorScale( dir, speed, dir );
	}

	// interpolate origin
	for( i = 0; i < 3; i++ )
		cent->ent.origin[i] = cent->ent.origin2[i] = cent->prev.origin[i] + cg.lerpfrac * ( cent->current.origin[i] - cent->prev.origin[i] );

	if( ( cent->current.effects >> 9 ) & 1 ) // BOUNCES ON WALLS/FLOORS
		bounce = 35;

	VectorClear( accel );
	if( ( cent->current.effects >> 10 ) & 1 ) // GRAVITY
		VectorSet( accel, -0.2f, -0.2f, -175.0f );

	if( ( cent->current.effects >> 11 ) & 1 ) // EXPAND_EFFECT
		expandEffect = qtrue;

	if( ( cent->current.effects >> 12 ) & 1 ) // SHRINK_EFFECT
		shrinkEffect = qtrue;

	CG_SpawnSprite( cent->ent.origin, dir, accel,
		spriteRadius, spriteTime, bounce, expandEffect, shrinkEffect,
		cent->ent.shaderRGBA[0] / 255.0f,
		cent->ent.shaderRGBA[1] / 255.0f,
		cent->ent.shaderRGBA[2] / 255.0f,
		cent->ent.shaderRGBA[3] / 255.0f,
		cent->current.light ? spriteRadius * 4 : 0, // light radius
		COLOR_R( cent->current.light ) / 255.0f,
		COLOR_G( cent->current.light ) / 255.0f,
		COLOR_B( cent->current.light ) / 255.0f,
		cent->ent.customShader );
}

void CG_UpdateParticlesEnt( centity_t *cent )
{
	// set entity color based on team
	CG_TeamColorForEntity( cent->current.number, cent->ent.shaderRGBA );

	// set up the data in the old position
	cent->ent.model = NULL;
	cent->ent.customShader = cgs.imagePrecache[ cent->current.modelindex ];
	VectorCopy( cent->prev.origin, cent->ent.origin );
	VectorCopy( cent->prev.origin2, cent->ent.origin2 );
}

//==================================================
// ET_SOUNDEVENT
//==================================================

void CG_SoundEntityNewState( centity_t *cent )
{
	int channel, soundindex, owner;
	float attenuation;
	qboolean fixed;

	soundindex = cent->current.sound;
	owner = cent->current.ownerNum;
	channel = cent->current.channel & ~CHAN_FIXED;
	fixed = (cent->current.channel & CHAN_FIXED);
	attenuation = (float)cent->current.attenuation / 16.0f;

	if( attenuation == ATTN_NONE )
	{
		if( cgs.soundPrecache[soundindex] )
			trap_S_StartGlobalSound( cgs.soundPrecache[soundindex], channel & ~CHAN_FIXED, 1.0f );
		return;
	}

	if( owner )
	{
		if( owner < 0 || owner >= MAX_EDICTS )
		{
			CG_Printf( "CG_SoundEntityNewState: bad owner number" );
			return;
		}
		if( cg_entities[owner].serverFrame != cg.frame.serverFrame )
			owner = 0;
	}

	if( !owner )
		fixed = qtrue;

	// sexed sounds are not in the sound index and ignore attenuation
	if( !cgs.soundPrecache[soundindex] )
	{
		if( owner )
		{
			char *cstring = cgs.configStrings[CS_SOUNDS + soundindex];
			if( cstring && cstring[0] == '*' )
				CG_SexedSound( owner, channel | (fixed ? CHAN_FIXED : 0), cstring, 1.0f );
		}
		return;
	}

	if( fixed )
		trap_S_StartFixedSound( cgs.soundPrecache[soundindex], cent->current.origin, channel, 1.0f, attenuation );
	else if( ISVIEWERENTITY( owner ) )
		trap_S_StartGlobalSound( cgs.soundPrecache[soundindex], channel, 1.0f );
	else
		trap_S_StartRelativeSound( cgs.soundPrecache[soundindex], owner, channel, 1.0f, attenuation );
}

//==========================================================================
//		PACKET ENTITIES
//==========================================================================

void CG_EntityLoopSound( entity_state_t *state, float attenuation )
{
	if( !state->sound )
		return;

	trap_S_AddLoopSound( cgs.soundPrecache[state->sound], state->number, cg_volume_effects->value, ISVIEWERENTITY( state->number ) ? ATTN_NONE : ATTN_IDLE );
}

/*
* CG_AddPacketEntitiesToScene
* Add the entities to the rendering list
*/
void CG_AddEntities( void )
{
	entity_state_t *state;
	vec3_t autorotate;
	int pnum;
	centity_t *cent;
	unsigned int serverTime;
	qboolean canLight;

	if( GS_MatchPaused() )
		serverTime = cg.frame.serverTime;
	else
		serverTime = cg.time;

	// bonus items rotate at a fixed rate
	VectorSet( autorotate, 0, ( cg.time%3600 )*0.1, 0 );
	AnglesToAxis( autorotate, cg.autorotateAxis );

	for( pnum = 0; pnum < cg.frame.numEntities; pnum++ )
	{
		state = &cg.frame.parsedEntities[pnum & ( MAX_PARSE_ENTITIES-1 )];
		cent = &cg_entities[state->number];

		if( cent->current.linearProjectile )
		{
			if( !cent->linearProjectileCanDraw )
				continue;
		}

		canLight = !state->linearProjectile;

		switch( cent->type )
		{
		case ET_GENERIC:
			CG_AddGenericEnt( cent );
			if( cg_drawEntityBoxes->integer ) CG_DrawEntityBox( cent );
			CG_EntityLoopSound( state, ATTN_STATIC );
			canLight = qtrue;
			break;
		case ET_GIB:
			if( cg_gibs->integer )
			{
				CG_AddGenericEnt( cent );
				if( cg_gibs->integer != 1 )
					CG_NewBloodTrail( cent );
				CG_EntityLoopSound( state, ATTN_STATIC );
				canLight = qtrue;
			}
			break;
		case ET_BLASTER:
			CG_AddGenericEnt( cent );
			CG_BlasterTrail( cent->trailOrigin, cent->ent.origin );
			CG_EntityLoopSound( state, ATTN_STATIC );
			break;

		case ET_ELECTRO_WEAK:
			cent->current.frame = cent->prev.frame = 0;
			cent->ent.frame =  cent->ent.oldframe = 0;

			CG_AddGenericEnt( cent );
			CG_EntityLoopSound( state, ATTN_STATIC );
			CG_ElectroWeakTrail( cent->trailOrigin, cent->ent.origin, NULL );
			break;
		case ET_ROCKET:
			CG_AddGenericEnt( cent );
			CG_NewRocketTrail( cent );
			CG_EntityLoopSound( state, ATTN_NORM );
			CG_AddLightToScene( cent->ent.origin, 300, 1, 1, 0, NULL );
			break;
		case ET_GRENADE:
			CG_AddGenericEnt( cent );
			CG_EntityLoopSound( state, ATTN_STATIC );
			CG_NewGrenadeTrail( cent );
			canLight = qtrue;
			break;
		case ET_PLASMA:
			CG_AddGenericEnt( cent );
			CG_EntityLoopSound( state, ATTN_STATIC );
			break;

		case ET_SPRITE:
			CG_AddSpriteEnt( cent );
			CG_EntityLoopSound( state, ATTN_STATIC );
			canLight = qtrue;
			break;

		case ET_ITEM:
			CG_AddItemEnt( cent );
			if( cg_drawEntityBoxes->integer ) CG_DrawEntityBox( cent );
			CG_EntityLoopSound( state, ATTN_IDLE );
			canLight = qtrue;
			break;

		case ET_PLAYER:
			CG_AddPlayerEnt( cent );
			if( cg_drawEntityBoxes->integer ) CG_DrawEntityBox( cent );
			CG_EntityLoopSound( state, ATTN_IDLE );
			CG_LaserBeamEffect( cent );
			CG_WeaponBeamEffect( cent );
			canLight = qtrue;
			break;

		case ET_CORPSE:
			CG_AddPlayerEnt( cent );
			if( cg_drawEntityBoxes->integer ) CG_DrawEntityBox( cent );
			CG_EntityLoopSound( state, ATTN_IDLE );
			canLight = qtrue;
			break;

		case ET_BEAM:
			CG_AddBeamEnt( cent );
			CG_EntityLoopSound( state, ATTN_STATIC );
			break;

		case ET_LASERBEAM:
		case ET_CURVELASERBEAM:
			break;

		case ET_PORTALSURFACE:
			CG_AddPortalSurfaceEnt( cent );
			CG_EntityLoopSound( state, ATTN_STATIC );
			break;

		case ET_FLAG_BASE:
			CG_AddFlagBaseEnt( cent );
			CG_EntityLoopSound( state, ATTN_STATIC );
			canLight = qtrue;
			break;

		case ET_MINIMAP_ICON:
			if( cent->effects & EF_TEAMCOLOR_TRANSITION )
				CG_EntAddTeamColorTransitionEffect( cent );
			break;

		case ET_DECAL:
			CG_AddDecalEnt( cent );
			CG_EntityLoopSound( state, ATTN_STATIC );
			break;

		case ET_PUSH_TRIGGER:
			if( cg_drawEntityBoxes->integer ) CG_DrawEntityBox( cent );
			CG_EntityLoopSound( state, ATTN_STATIC );
			break;

		case ET_EVENT:
		case ET_SOUNDEVENT:
			break;

		case ET_ITEM_TIMER:
			break;

		case ET_PARTICLES:
			CG_AddParticlesEnt( cent );
			CG_EntityLoopSound( state, ATTN_STATIC );
			break;

		default:
			CG_Error( "CG_AddPacketEntities: unknown entity type" );
			break;
		}

		// glow if light is set
		if( canLight && state->light )
		{
			CG_AddLightToScene( cent->ent.origin,
				COLOR_A( state->light ) * 4.0,
				COLOR_R( state->light ) * ( 1.0/255.0 ),
				COLOR_G( state->light ) * ( 1.0/255.0 ),
				COLOR_B( state->light ) * ( 1.0/255.0 ), NULL );
		}

		VectorCopy( cent->ent.origin, cent->trailOrigin );
	}
}

/*
* CG_LerpEntities
* Interpolate the entity states positions into the entity_t structs
*/
void CG_LerpEntities( void )
{
	entity_state_t *state;
	int pnum;
	centity_t *cent;

	for( pnum = 0; pnum < cg.frame.numEntities; pnum++ )
	{
		state = &cg.frame.parsedEntities[pnum & ( MAX_PARSE_ENTITIES-1 )];
		cent = &cg_entities[state->number];

		switch( cent->type )
		{
		case ET_GENERIC:
		case ET_GIB:
		case ET_BLASTER:
		case ET_ELECTRO_WEAK:
		case ET_ROCKET:
		case ET_PLASMA:
		case ET_GRENADE:
		case ET_ITEM:
		case ET_PLAYER:
		case ET_CORPSE:
		case ET_FLAG_BASE:
			if( state->linearProjectile )
				CG_ExtrapolateLinearProjectile( cent );
			else
				CG_LerpGenericEnt( cent );
			break;

		case ET_SPRITE:
			CG_LerpSpriteEnt( cent );
			break;

		case ET_DECAL:
			CG_LerpDecalEnt( cent );
			break;

		case ET_BEAM:
			// beams aren't interpolated
			break;

		case ET_LASERBEAM:
		case ET_CURVELASERBEAM:
			CG_LerpLaserbeamEnt( cent );
			break;

		case ET_MINIMAP_ICON:
			break;

		case ET_PORTALSURFACE:
			//portals aren't interpolated
			break;

		case ET_PUSH_TRIGGER:
			break;

		case ET_EVENT:
		case ET_SOUNDEVENT:
			break;

		case ET_ITEM_TIMER:
			break;

		case ET_PARTICLES:
			break;

		default:
			CG_Error( "CG_LerpEntities: unknown entity type" );
			break;
		}
	}
}

/*
* CG_UpdateEntities
* Called at receiving a new serverframe. Sets up the model, type, etc to be drawn later on
*/
void CG_UpdateEntities( void )
{
	entity_state_t *state;
	int pnum;
	centity_t *cent;

	CG_ResetItemTimers();

	for( pnum = 0; pnum < cg.frame.numEntities; pnum++ )
	{
		state = &cg.frame.parsedEntities[pnum & ( MAX_PARSE_ENTITIES-1 )];
		cent = &cg_entities[state->number];
		cent->type = state->type;
		cent->effects = state->effects;
		cent->item = NULL;
		cent->renderfx = 0;

		switch( cent->type )
		{
		case ET_GENERIC:
			CG_UpdateGenericEnt( cent );
			break;
		case ET_GIB:
			if( cg_gibs->integer )
			{
				cent->renderfx |= RF_NOSHADOW;
				CG_UpdateGenericEnt( cent );

				// set the gib model ignoring the modelindex one
				if( cg_gibs->integer == 1 )
				{
					cent->ent.model = CG_MediaModel( cgs.media.modTechyGibs[ 0 ] );
				}
				else
				{
					cent->ent.model = CG_MediaModel( cgs.media.modMeatyGibs[ 0 ] );
				}
			}
			break;

			// projectiles with linear trajectories
		case ET_BLASTER:
		case ET_ELECTRO_WEAK:
		case ET_ROCKET:
		case ET_PLASMA:
		case ET_GRENADE:
			cent->renderfx |= ( RF_NOSHADOW|RF_FULLBRIGHT );
			CG_UpdateGenericEnt( cent );
			break;

		case ET_SPRITE:
			cent->renderfx |= ( RF_NOSHADOW|RF_FULLBRIGHT );
			CG_UpdateSpriteEnt( cent );
			break;

		case ET_ITEM:
			CG_UpdateItemEnt( cent );
			break;
		case ET_PLAYER:
		case ET_CORPSE:
			CG_UpdatePlayerModelEnt( cent );
			break;

		case ET_BEAM:
			break;

		case ET_LASERBEAM:
		case ET_CURVELASERBEAM:
			CG_UpdateLaserbeamEnt( cent );
			break;

		case ET_FLAG_BASE:
			CG_UpdateFlagBaseEnt( cent );
			break;

		case ET_MINIMAP_ICON:
			{
				CG_TeamColorForEntity( cent->current.number, cent->ent.shaderRGBA );
				if( cent->current.modelindex > 0 && cent->current.modelindex < MAX_IMAGES )
					cent->ent.customShader = cgs.imagePrecache[ cent->current.modelindex ];
				else
					cent->ent.customShader = NULL;
			}
			break;

		case ET_DECAL:
			CG_UpdateDecalEnt( cent );
			break;

		case ET_PORTALSURFACE:
			CG_UpdatePortalSurfaceEnt( cent );
			break;

		case ET_PUSH_TRIGGER:
			break;

		case ET_EVENT:
		case ET_SOUNDEVENT:
			break;

		case ET_ITEM_TIMER:
			CG_UpdateItemTimerEnt( cent );
			break;

		case ET_PARTICLES:
			CG_UpdateParticlesEnt( cent );
			break;

		default:
			if( cent->type & ET_INVERSE )
				CG_Printf( "CG_UpdateEntities: entity type with ET_INVERSE. Stripped type %i", cent->type & 127 );

			CG_Error( "CG_UpdateEntities: unknown entity type %i", cent->type );
			break;
		}
	}

	CG_SortItemTimers();
}

//=============================================================

/*
* CG_GetEntitySpatilization
* 
* Called to get the sound spatialization origin and velocity
*/
void CG_GetEntitySpatilization( int entNum, vec3_t origin, vec3_t velocity )
{
	centity_t *cent;
	struct cmodel_s *cmodel;
	vec3_t mins, maxs;

	if( entNum < -1 || entNum >= MAX_EDICTS )
		CG_Error( "CG_GetEntitySoundOrigin: bad entnum" );

	// hack for client side floatcam
	if( entNum == -1 )
	{
		if( origin != NULL )
			VectorCopy( cg.frame.playerState.pmove.origin, origin );
		if( velocity != NULL )
			VectorCopy( cg.frame.playerState.pmove.velocity, velocity );
		return;
	}

	cent = &cg_entities[entNum];

	// normal
	if( cent->current.solid != SOLID_BMODEL )
	{
		if( origin != NULL )
			VectorCopy( cent->ent.origin, origin );
		if( velocity != NULL )
			VectorCopy( cent->velocity, velocity );
		return;
	}

	// bmodel
	if( origin != NULL )
	{
		cmodel = trap_CM_InlineModel( cent->current.modelindex );
		trap_CM_InlineModelBounds( cmodel, mins, maxs );
		VectorAdd( maxs, mins, origin );
		VectorMA( cent->ent.origin, 0.5f, origin, origin );
	}
	if( velocity != NULL )
		VectorCopy( cent->velocity, velocity );
}
