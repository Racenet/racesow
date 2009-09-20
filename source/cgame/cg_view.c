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

// cg_view.c -- player rendering positioning

//======================================================================
//					ChaseHack (In Eyes Chasecam)
//======================================================================

cg_chasecam_t chaseCam;

int CG_LostMultiviewPOV( void );

/*
* CG_ChaseStep
*/
void CG_ChaseStep( int step )
{
	int index, checkPlayer, i;

	if( chaseCam.mode < 0 || chaseCam.mode >= CAM_MODES )
		return;

	if( cg.frame.multipov )
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
		if( index == -1 )
		{
			index = CG_LostMultiviewPOV();
			cg.multiviewPlayerNum = cg.frame.playerStates[index].playerNum;
		}
		else
		{
			checkPlayer = index;
			for( i = 0; i < cg.frame.numplayers; i++ )
			{
				checkPlayer += step;
				if( checkPlayer < 0 )
					checkPlayer = cg.frame.numplayers - 1;
				else if( checkPlayer >= cg.frame.numplayers )
					checkPlayer = 0;

				if( ( checkPlayer != index ) && cg.frame.playerStates[checkPlayer].stats[STAT_REALTEAM] == TEAM_SPECTATOR )
					continue;
				break;
			}

			cg.multiviewPlayerNum = cg.frame.playerStates[checkPlayer].playerNum;
		}
	}
	else if( !cgs.demoPlaying )
	{
		trap_Cmd_ExecuteText( EXEC_NOW, step > 0 ? "chasenext" : "chaseprev" );
	}
}

/*
* CG_AddLocalSounds
*/
static void CG_AddLocalSounds( void )
{
	static qboolean postmatchsound_set = qfalse, demostream = qfalse;
	static unsigned int lastSecond = 0;

	// add local announces
	if( GS_Countdown() )
	{
		if( GS_MatchDuration() )
		{
			unsigned int duration, curtime, remainingSeconds;
			float seconds;

			curtime = GS_MatchPaused() ? cg.frame.serverTime : cg.time;
			duration = GS_MatchDuration();

			if( duration + GS_MatchStartTime() < curtime ) 
				duration = curtime - GS_MatchStartTime(); // avoid negative results

			seconds = (float)( GS_MatchStartTime() + duration - curtime ) * 0.001f;
			remainingSeconds = (unsigned int)seconds;

			if( remainingSeconds != lastSecond )
			{
				if( 1 + remainingSeconds < 4 )
				{
					struct sfx_s *sound = trap_S_RegisterSound( va( S_ANNOUNCER_COUNTDOWN_COUNT_1_to_3_SET_1_to_2, 1 + remainingSeconds, 1 ) );
					CG_AddAnnouncerEvent( sound, qfalse );
				}

				lastSecond = remainingSeconds;
			}
		}
	}
	else
		lastSecond = 0;

	// add sounds from announcer
	CG_ReleaseAnnouncerEvents();

	// if in postmatch, play postmatch song
	if( GS_MatchState() >= MATCH_STATE_POSTMATCH )
	{
		if( !postmatchsound_set && !demostream )
		{
			trap_S_StopBackgroundTrack();
			trap_S_StartBackgroundTrack( va( S_MUSIC_POSTMATCH_1_to_7, (int)brandom( 1, 7 ) ), NULL );
			postmatchsound_set = qtrue;
		}
	}
	else
	{
		if( cgs.demoPlaying && cgs.demoAudioStream && !demostream )
		{
			trap_S_StopBackgroundTrack();
			trap_S_StartBackgroundTrack( cgs.demoAudioStream, NULL );
			demostream = qtrue;
		}

		if( postmatchsound_set )
		{
			trap_S_StopBackgroundTrack();
			postmatchsound_set = qfalse;
		}

		// notice: these 2 sound files aren't used anymore
		//cgs.media.sfxTimerBipBip
		//cgs.media.sfxTimerPloink
	}
}

/*
* CG_SetSensitivityScale
* Scale sensitivity for different view effects
*/
float CG_SetSensitivityScale( const float sens )
{
	float sensScale = 1.0f;

	if( !cgs.demoPlaying && sens && ( cg.predictedPlayerState.pmove.stats[PM_STAT_ZOOMTIME] > 0 ) )
	{
		if( cg_zoomSens->value )
			return cg_zoomSens->value/sens;

		return ( cg.predictedPlayerState.fov / cgs.clientInfo[cgs.playerNum].fov );
	}

	return sensScale;
}

/*
* CG_AddKickAngles
*/
void CG_AddKickAngles( vec3_t viewangles )
{
	float time;
	float uptime;
	float delta;
	int i;

	for( i = 0; i < MAX_ANGLES_KICKS; i++ )
	{
		if( cg.time > cg.kickangles[i].timestamp + cg.kickangles[i].kicktime )
			continue;

		time = (float)( ( cg.kickangles[i].timestamp + cg.kickangles[i].kicktime ) - cg.time );
		uptime = ( (float)cg.kickangles[i].kicktime ) * 0.5f;
		delta = 1.0f - ( abs( time - uptime ) / uptime );
		//CG_Printf("Kick Delta:%f\n", delta );
		if( delta > 1.0f )
			delta = 1.0f;
		if( delta <= 0.0f )
			continue;

		viewangles[PITCH] += cg.kickangles[i].v_pitch * delta;
		viewangles[ROLL] += cg.kickangles[i].v_roll * delta;
	}
}

/*
* CG_CalcViewBob
*/
static void CG_CalcViewBob( void )
{
	float bobMove, bobTime, bobScale;

	if( !cg.view.drawWeapon )
		return;

	// calculate speed and cycle to be used for all cyclic walking effects
	cg.xyspeed = sqrt( cg.predictedPlayerState.pmove.velocity[0]*cg.predictedPlayerState.pmove.velocity[0] + cg.predictedPlayerState.pmove.velocity[1]*cg.predictedPlayerState.pmove.velocity[1] );

	bobScale = 0;
	if( cg.xyspeed < 5 )
		cg.oldBobTime = 0;  // start at beginning of cycle again
	else if( cg_gunbob->integer )
	{
		if( !ISVIEWERENTITY( cg.view.POVent ) )
			bobScale = 0.0f;
		else if( CG_PointContents( cg.view.origin ) & MASK_WATER )
			bobScale =  0.75f;
		else
		{
			centity_t *cent;
			vec3_t mins, maxs;
			trace_t	trace;

			cent = &cg_entities[cg.view.POVent];
			GS_BBoxForEntityState( &cent->current, mins, maxs );
			maxs[2] = mins[2];
			mins[2] -= ( 1.6f*STEPSIZE );

			CG_Trace( &trace, cg.predictedPlayerState.pmove.origin, mins, maxs, cg.predictedPlayerState.pmove.origin, cg.view.POVent, MASK_PLAYERSOLID );
			if( trace.startsolid || trace.allsolid )
			{
				if( cg.predictedPlayerState.pmove.stats[PM_STAT_CROUCHTIME] )
					bobScale = 1.5f;
				else
					bobScale = 2.5f;
			}
		}
	}

	bobMove = cg.frameTime * bobScale;
	bobTime = ( cg.oldBobTime += bobMove );

	cg.bobCycle = (int)bobTime;
	cg.bobFracSin = fabs( sin( bobTime*M_PI ) );
}

/*
* CG_ResetKickAngles
*/
void CG_ResetKickAngles( void )
{
	memset( cg.kickangles, 0, sizeof( cg.kickangles ) );
}

/*
* CG_StartKickAnglesEffect
*/
void CG_StartKickAnglesEffect( vec3_t source, float knockback, float radius, int time )
{
	float kick;
	float side;
	float dist;
	float delta;
	float ftime;
	vec3_t forward, right, v;
	int i, kicknum = -1;
	vec3_t playerorigin;

	if( knockback <= 0 || time <= 0 || radius <= 0.0f )
		return;

	// if spectator but not in chasecam, don't get any kick
	if( cg.frame.playerState.pmove.pm_type == PM_SPECTATOR )
		return;

	// not if dead
	if( cg_entities[cg.view.POVent].current.type == ET_CORPSE || cg_entities[cg.view.POVent].current.type == ET_GIB )
		return;

	// predictedPlayerState is predicted only when prediction is enabled, otherwise it is interpolated
	VectorCopy( cg.predictedPlayerState.pmove.origin, playerorigin );

	VectorSubtract( source, playerorigin, v );
	dist = VectorNormalize( v );
	if( dist > radius )
		return;

	delta = 1.0f - ( dist / radius );
	if( delta > 1.0f )
		delta = 1.0f;
	if( delta <= 0.0f )
		return;

	kick = abs( knockback ) * delta;
	if( kick ) // kick of 0 means no view adjust at all
	{
		//find first free kick spot, or the one closer to be finished
		for( i = 0; i < MAX_ANGLES_KICKS; i++ )
		{
			if( cg.time > cg.kickangles[i].timestamp + cg.kickangles[i].kicktime )
			{
				kicknum = i;
				break;
			}
		}

		// all in use. Choose the closer to be finished
		if( kicknum == -1 )
		{
			int remaintime;
			int best = ( cg.kickangles[0].timestamp + cg.kickangles[0].kicktime ) - cg.time;
			kicknum = 0;
			for( i = 1; i < MAX_ANGLES_KICKS; i++ )
			{
				remaintime = ( cg.kickangles[i].timestamp + cg.kickangles[i].kicktime ) - cg.time;
				if( remaintime < best )
				{
					best = remaintime;
					kicknum = i;
				}
			}
		}

		AngleVectors( cg.frame.playerState.viewangles, forward, right, NULL );

		if( kick < 1.0f )
			kick = 1.0f;

		side = DotProduct( v, right );
		cg.kickangles[kicknum].v_roll = kick*side*0.3;
		clamp( cg.kickangles[kicknum].v_roll, -20, 20 );

		side = -DotProduct( v, forward );
		cg.kickangles[kicknum].v_pitch = kick*side*0.3;
		clamp( cg.kickangles[kicknum].v_pitch, -20, 20 );

		cg.kickangles[kicknum].timestamp = cg.time;
		ftime = (float)time * delta;
		if( ftime < 100 )
			ftime = 100;
		cg.kickangles[kicknum].kicktime = ftime;
	}
}

/*
* CG_ResetColorBlend
*/
void CG_ResetColorBlend( void )
{
	memset( cg.colorblends, 0, sizeof( cg.colorblends ) );
}

/*
* CG_StartColorBlendEffect
*/
void CG_StartColorBlendEffect( float r, float g, float b, float a, int time )
{
	int i, bnum = -1;

	if( a <= 0.0f || time <= 0 )
		return;

	//find first free colorblend spot, or the one closer to be finished
	for( i = 0; i < MAX_COLORBLENDS; i++ )
	{
		if( cg.time > cg.colorblends[i].timestamp + cg.colorblends[i].blendtime )
		{
			bnum = i;
			break;
		}
	}

	// all in use. Choose the closer to be finished
	if( bnum == -1 )
	{
		int remaintime;
		int best = ( cg.colorblends[0].timestamp + cg.colorblends[0].blendtime ) - cg.time;
		bnum = 0;
		for( i = 1; i < MAX_COLORBLENDS; i++ )
		{
			remaintime = ( cg.colorblends[i].timestamp + cg.colorblends[i].blendtime ) - cg.time;
			if( remaintime < best )
			{
				best = remaintime;
				bnum = i;
			}
		}
	}

	// assign the color blend
	cg.colorblends[bnum].blend[0] = r;
	cg.colorblends[bnum].blend[1] = g;
	cg.colorblends[bnum].blend[2] = b;
	cg.colorblends[bnum].blend[3] = a;

	cg.colorblends[bnum].timestamp = cg.time;
	cg.colorblends[bnum].blendtime = time;
}

//============================================================================

/*
* CG_AddEntityToScene
*/
void CG_AddEntityToScene( entity_t *ent )
{
	if( ent->model && ( !ent->boneposes || !ent->oldboneposes ) )
	{
		if( trap_R_SkeletalGetNumBones( ent->model, NULL ) )
			CG_SetBoneposesForTemporaryEntity( ent );
	}

	trap_R_AddEntityToScene( ent );
}

//============================================================================

/*
* CG_SkyPortal
*/
int CG_SkyPortal( void )
{
	float fov = 0;
	float scale = 0;
	int noents = 0;
	float pitchspeed = 0, yawspeed = 0, rollspeed = 0;
	skyportal_t *sp = &cg.view.refdef.skyportal;

	if( cgs.configStrings[CS_SKYBOX][0] == '\0' )
		return 0;

	if( sscanf( cgs.configStrings[CS_SKYBOX], "%f %f %f %f %f %i %f %f %f",
		&sp->vieworg[0], &sp->vieworg[1], &sp->vieworg[2], &fov, &scale, 
		&noents,
		&pitchspeed, &yawspeed, &rollspeed ) >= 3 )
	{
		float off = cg.view.refdef.time * 0.001f;

		sp->fov = fov;
		sp->noEnts = (noents ? qtrue : qfalse);
		sp->scale = scale ? 1.0f / scale : 0;
		VectorSet( sp->viewanglesOffset, anglemod( off * pitchspeed ), anglemod( off * yawspeed ), anglemod( off * rollspeed ) );
		return RDF_SKYPORTALINVIEW;
	}

	return 0;
}

/*
* CG_RenderFlags
*/
static int CG_RenderFlags( void )
{
	int rdflags, contents;

	rdflags = 0;

	// set the RDF_UNDERWATER and RDF_CROSSINGWATER bitflags
	contents = CG_PointContents( cg.view.origin );
	if( contents & MASK_WATER )
	{
		rdflags |= RDF_UNDERWATER;

		// undewater, check above
		contents = CG_PointContents( tv( cg.view.origin[0], cg.view.origin[1], cg.view.origin[2] + 9 ) );
		if( !(contents & MASK_WATER) )
			rdflags |= RDF_CROSSINGWATER;
	}
	else
	{
		// look down a bit
		contents = CG_PointContents( tv( cg.view.origin[0], cg.view.origin[1], cg.view.origin[2] - 9 ) );
		if( contents & MASK_WATER )
			rdflags |= RDF_CROSSINGWATER;
	}

	if( cg.oldAreabits )
		rdflags |= RDF_OLDAREABITS;

	if( cg.portalInView )
		rdflags |= RDF_PORTALINVIEW;

	if( cg_outlineWorld->integer )
		rdflags |= RDF_WORLDOUTLINES;

	rdflags |= RDF_BLOOM;
	rdflags |= CG_SkyPortal();

	return rdflags;
}

/*
* CG_InterpolatePlayerState
*/
static void CG_InterpolatePlayerState( player_state_t *playerState )
{
	int i;
	player_state_t *ps, *ops;
	qboolean teleported;

	ps = &cg.frame.playerState;
	ops = &cg.oldFrame.playerState;

	*playerState = *ps;

	teleported = ( ps->pmove.pm_flags & PMF_TIME_TELEPORT ) ? qtrue : qfalse;

	// if the player entity was teleported this frame use the final position
	if( abs( ops->pmove.origin[0] - ps->pmove.origin[0] ) > 256
	    || abs( ops->pmove.origin[1] - ps->pmove.origin[1] ) > 256
	    || abs( ops->pmove.origin[2] - ps->pmove.origin[2] ) > 256
		|| teleported )
	{
		VectorCopy( ps->pmove.origin, playerState->pmove.origin );
		VectorCopy( ps->viewangles, playerState->viewangles );
	}
	else
	{
		for( i = 0; i < 3; i++ )
		{
			playerState->pmove.origin[i] = ops->pmove.origin[i] + cg.lerpfrac * ( ps->pmove.origin[i] - ops->pmove.origin[i] );
			playerState->viewangles[i] = LerpAngle( ops->viewangles[i], ps->viewangles[i], cg.lerpfrac );
		}
	}

	// interpolate fov and viewheight
	if( !teleported )
	{
		playerState->fov = ops->fov + cg.lerpfrac * ( ps->fov - ops->fov );
		playerState->viewheight = ops->viewheight + cg.lerpfrac * ( ps->viewheight - ops->viewheight );
	}
}

/*
* CG_ThirdPersonOffsetView
*/
static void CG_ThirdPersonOffsetView( cg_viewdef_t *view )
{
	float dist, f, r;
	vec3_t dest, stop;
	vec3_t chase_dest;
	trace_t	trace;
	vec3_t mins = { -4, -4, -4 };
	vec3_t maxs = { 4, 4, 4 };

	if( !cg_thirdPersonAngle || !cg_thirdPersonRange )
	{
		cg_thirdPersonAngle = trap_Cvar_Get( "cg_thirdPersonAngle", "0", CVAR_ARCHIVE );
		cg_thirdPersonRange = trap_Cvar_Get( "cg_thirdPersonRange", "70", CVAR_ARCHIVE );
	}

	// calc exact destination
	VectorCopy( view->origin, chase_dest );
	r = DEG2RAD( cg_thirdPersonAngle->value );
	f = -cos( r );
	r = -sin( r );
	VectorMA( chase_dest, cg_thirdPersonRange->value * f, view->axis[FORWARD], chase_dest );
	VectorMA( chase_dest, cg_thirdPersonRange->value * r, view->axis[RIGHT], chase_dest );
	chase_dest[2] += 8;

	// find the spot the player is looking at
	VectorMA( view->origin, 512, view->axis[FORWARD], dest );
	CG_Trace( &trace, view->origin, mins, maxs, dest, view->POVent, MASK_SOLID );

	// calculate pitch to look at the same spot from camera
	VectorSubtract( trace.endpos, view->origin, stop );
	dist = sqrt( stop[0] * stop[0] + stop[1] * stop[1] );
	if( dist < 1 )
		dist = 1;
	view->angles[PITCH] = RAD2DEG( -atan2( stop[2], dist ) );
	view->angles[YAW] -= cg_thirdPersonAngle->value;
	AngleVectors( view->angles, view->axis[FORWARD], view->axis[RIGHT], view->axis[UP] );

	// move towards destination
	CG_Trace( &trace, view->origin, mins, maxs, chase_dest, view->POVent, MASK_SOLID );

	if( trace.fraction != 1.0 )
	{
		VectorCopy( trace.endpos, stop );
		stop[2] += ( 1.0 - trace.fraction ) * 32;
		CG_Trace( &trace, view->origin, mins, maxs, stop, view->POVent, MASK_SOLID );
		VectorCopy( trace.endpos, chase_dest );
	}

	VectorCopy( chase_dest, view->origin );
}

/*
* CG_ViewSmoothPredictedSteps
*/
static void CG_ViewSmoothPredictedSteps( vec3_t vieworg )
{
	int timeDelta;

	// smooth out stair climbing
	timeDelta = cg.realTime - cg.predictedStepTime;
	if( timeDelta < PREDICTED_STEP_TIME )
		vieworg[2] -= cg.predictedStep * ( PREDICTED_STEP_TIME - timeDelta ) / PREDICTED_STEP_TIME;
}

/*
* CG_ChaseCamButtons
*/
static void CG_ChaseCamButtons( void )
{
#define CHASECAMBUTTONSDELAY ( cg.time + 250 )
	usercmd_t cmd;
	qboolean chasecam = ( cg.frame.playerState.pmove.pm_type == PM_CHASECAM ) 
		&& ( cg.frame.playerState.POVnum != (unsigned)( cgs.playerNum + 1 ) );
	qboolean realSpec = cgs.demoPlaying || cg.frame.playerState.stats[STAT_REALTEAM] == TEAM_SPECTATOR;

	if( (cg.frame.multipov || chasecam) && !CG_DemoCam_IsFree() )
	{
		if( cg.time <= chaseCam.cmd_mode_delay )
			return;

		trap_NET_GetUserCmd( trap_NET_GetCurrentUserCmdNum() - 1, &cmd );

		if( ( cmd.buttons & BUTTON_ATTACK ) )
		{
			if( chasecam )
			{
				if( realSpec )
				{
					if( ++chaseCam.mode >= CAM_MODES )
					{
						// if exceeds the cycle, start free fly
						trap_Cmd_ExecuteText( EXEC_NOW, "camswitch" );
						chaseCam.mode = 0; // smallest, to start the new cycle
					}

					chaseCam.cmd_mode_delay = CHASECAMBUTTONSDELAY;
				}
			}
			else
			{
				chaseCam.mode = ( chaseCam.mode != CAM_THIRDPERSON );
				chaseCam.cmd_mode_delay = CHASECAMBUTTONSDELAY;
			}
		}

		if( cg.frame.multipov || chasecam )
		{
			int step = 0;

			if( cmd.upmove > 0 || cmd.buttons & BUTTON_SPECIAL )
				step = 1;
			else if( cmd.upmove < 0 )
				step = -1;

			if( step )
			{
				CG_ChaseStep( step );
				chaseCam.cmd_mode_delay = CHASECAMBUTTONSDELAY;
			}
		}
	}
	else if( CG_DemoCam_IsFree() || cg.frame.playerState.pmove.pm_type == PM_SPECTATOR )
	{
		chaseCam.mode = CAM_INEYES;

		if( realSpec )
		{
			trap_NET_GetUserCmd( trap_NET_GetCurrentUserCmdNum() - 1, &cmd );

			if( ( cmd.buttons & BUTTON_ATTACK ) && cg.time > chaseCam.cmd_mode_delay )
			{
				trap_Cmd_ExecuteText( EXEC_NOW, "camswitch" );
				chaseCam.cmd_mode_delay = CHASECAMBUTTONSDELAY;
			}
		}
	}
	else
	{
		chaseCam.mode = CAM_INEYES;
	}
#undef CHASECAMBUTTONSDELAY
}

/*
* CG_SetupViewDef
*/
void CG_SetupViewDef( cg_viewdef_t *view, int type )
{
	memset( view, 0, sizeof( cg_viewdef_t ) );

	//
	// VIEW SETTINGS
	//

	view->type = type;

	if( view->type == VIEWDEF_PLAYERVIEW )
	{
		view->POVent = cg.frame.playerState.POVnum;

		view->draw2D = qtrue;

		// set up third-person
		if( cgs.demoPlaying )
			view->thirdperson = CG_DemoCam_GetThirdPerson();
		else if( chaseCam.mode == CAM_THIRDPERSON )
			view->thirdperson = qtrue;
		else
			view->thirdperson = ( cg_thirdPerson->integer != 0 );

		if( cg_entities[view->POVent].serverFrame != cg.frame.serverFrame )
			view->thirdperson = qfalse;

		// check for drawing gun
		if( !view->thirdperson && view->POVent > 0 && view->POVent <= gs.maxclients )
		{
			if( ( cg_entities[view->POVent].serverFrame == cg.frame.serverFrame ) &&
				( cg_entities[view->POVent].current.weapon != 0 ) )
				view->drawWeapon = ( cg_gun->integer != 0 );
		}

		// check for chase cams
		if( !( cg.frame.playerState.pmove.pm_flags & PMF_NO_PREDICTION ) )
		{
			if( (unsigned)view->POVent == cgs.playerNum + 1 )
			{
				if( cg_predict->integer && !view->thirdperson && !cgs.demoPlaying )
				{
					view->playerPrediction = qtrue;
				}
			}
		}
	}
	else if( view->type == VIEWDEF_CAMERA )
	{
		CG_DemoCam_GetViewDef( view );
	}
	else
	{
		module_Error( "CG_SetupView: Invalid view type %i\n", view->type );
	}

	//
	// SETUP REFDEF FOR THE VIEW SETTINGS
	//

	if( view->type == VIEWDEF_PLAYERVIEW )
	{
		int i;
		vec3_t viewoffset;

		if( view->playerPrediction )
		{
			CG_PredictMovement();

			// fixme: crouching is predicted now, but it looks very ugly
			VectorSet( viewoffset, 0.0f, 0.0f, cg.predictedPlayerState.viewheight );

			for( i = 0; i < 3; i++ )
			{
				view->origin[i] = cg.predictedPlayerState.pmove.origin[i] + viewoffset[i] - ( 1.0f - cg.lerpfrac ) * cg.predictionError[i];
				view->angles[i] = cg.predictedPlayerState.viewangles[i];
			}
			CG_ViewSmoothPredictedSteps( view->origin ); // smooth out stair climbing
		}
		else
		{
			cg.predictingTimeStamp = cg.time;
			cg.predictFrom = 0;

			// we don't run prediction, but we still set cg.predictedPlayerState with the interpolation
			CG_InterpolatePlayerState( &cg.predictedPlayerState );

			VectorSet( viewoffset, 0.0f, 0.0f, cg.predictedPlayerState.viewheight );

			VectorAdd( cg.predictedPlayerState.pmove.origin, viewoffset, view->origin );
			VectorCopy( cg.predictedPlayerState.viewangles, view->angles );
		}

		view->refdef.fov_x = cg.predictedPlayerState.fov;

		CG_CalcViewBob();

		VectorCopy( cg.predictedPlayerState.pmove.velocity, view->velocity );
	}
	else if( view->type == VIEWDEF_CAMERA )
	{
		view->refdef.fov_x = CG_DemoCam_GetOrientation( view->origin, view->angles, view->velocity );
	}

	// view rectangle size
	view->refdef.x = scr_vrect.x;
	view->refdef.y = scr_vrect.y;
	view->refdef.width = scr_vrect.width;
	view->refdef.height = scr_vrect.height;
	view->refdef.time = cg.time;
	view->refdef.areabits = cg.frame.areabits;

	view->refdef.fov_y = CalcFov( view->refdef.fov_x, view->refdef.width, view->refdef.height );
	view->fracDistFOV = tan( view->refdef.fov_x * ( M_PI/180 ) * 0.5f );

	AngleVectors( view->angles, view->axis[FORWARD], view->axis[RIGHT], view->axis[UP] );

	if( view->thirdperson )
		CG_ThirdPersonOffsetView( view );

	if( !view->playerPrediction )
		cg.predictedWeaponSwitch = 0;

	VectorCopy( cg.view.origin, view->refdef.vieworg );
	Matrix_Copy( cg.view.axis, view->refdef.viewaxis );
	VectorInverse( view->refdef.viewaxis[1] );
}

/*
* CG_RenderView
*/
#define	WAVE_AMPLITUDE	0.015   // [0..1]
#define	WAVE_FREQUENCY	0.6     // [0..1]
void CG_RenderView( float frameTime, float realFrameTime, int realTime, unsigned int serverTime, float stereo_separation )
{
	unsigned int prevTime;
	refdef_t *rd = &cg.view.refdef;

	// update time
	cg.realTime = realTime;
	cg.frameTime = frameTime;
	cg.realFrameTime = realFrameTime;
	cg.frameCount++;
	prevTime = cg.time;
	cg.time = serverTime;

	if( !cgs.precacheDone || !cg.frame.valid )
	{
		CG_DrawLoading();
		return;
	}

	if( cg.oldFrame.serverTime == cg.frame.serverTime )
		cg.lerpfrac = 1.0f;
	else
		cg.lerpfrac = (double)( cg.time - cg.oldFrame.serverTime ) / (double)( cg.frame.serverTime - cg.oldFrame.serverTime );

	if( cg_showClamp->integer )
	{
		if( cg.lerpfrac > 1.0f )
			CG_Printf( "high clamp %f\n", cg.lerpfrac );
		else if( cg.lerpfrac < 0.0f )
			CG_Printf( "low clamp  %f\n", cg.lerpfrac );
	}

	clamp( cg.lerpfrac, 0.0f, 1.0f );

	if( !cgs.configStrings[CS_WORLDMODEL][0] )
	{
		trap_R_DrawStretchPic( 0, 0, cgs.vidWidth, cgs.vidHeight, 0, 0, 1, 1, colorBlack, cgs.shaderWhite );
		return;
	}

	CG_CalcVrect(); // find sizes of the 3d drawing screen
	CG_TileClear(); // clear any dirty part of the background

	CG_ChaseCamButtons();

	CG_RunLightStyles();

	CG_LerpEntities();  // interpolate packet entities positions

	CG_ClearFragmentedDecals();

	trap_R_ClearScene();

	if( CG_DemoCam_Update() )
		CG_SetupViewDef( &cg.view, CG_DemoCam_GetViewType() );
	else
		CG_SetupViewDef( &cg.view, VIEWDEF_PLAYERVIEW );

	CG_CalcViewWeapon( &cg.weapon );

	CG_FireEvents( qfalse );

	CG_AddEntities();
	CG_AddViewWeapon( &cg.weapon );
	CG_AddLocalEntities();
	CG_AddParticles();
	CG_AddDlights();
#ifdef CGAMEGETLIGHTORIGIN
	CG_AddShadeBoxes();
#endif
	CG_AddDecals();
	CG_AddPolys();
	CG_AddLightStyles();

#ifndef PUBLIC_BUILD
	CG_AddTest();
#endif

	// offset vieworg appropriately if we're doing stereo separation
	VectorMA( cg.view.origin, stereo_separation, cg.view.axis[RIGHT], rd->vieworg );

	// never let it sit exactly on a node line, because a water plane can
	// disappear when viewed with the eye exactly on it.
	// the server protocol only specifies to 1/16 pixel, so add 1/16 in each axis
	rd->vieworg[0] += 1.0/PM_VECTOR_SNAP;
	rd->vieworg[1] += 1.0/PM_VECTOR_SNAP;
	rd->vieworg[2] += 1.0/PM_VECTOR_SNAP;

	AnglesToAxis( cg.view.angles, rd->viewaxis );

	rd->rdflags = CG_RenderFlags();

	// warp if underwater
	if( rd->rdflags & RDF_UNDERWATER )
	{
		float phase = rd->time * 0.001 * WAVE_FREQUENCY * M_TWOPI;
		float v = WAVE_AMPLITUDE * ( sin( phase ) - 1.0 ) + 1;
		rd->fov_x *= v;
		rd->fov_y *= v;
	}

	CG_AddLocalSounds();
	CG_SetSceneTeamColors(); // update the team colors in the renderer

	trap_R_RenderScene( &cg.view.refdef );

	cg.oldAreabits = qtrue;

	trap_S_Update( cg.view.origin, cg.view.velocity, cg.view.axis[FORWARD], cg.view.axis[RIGHT], cg.view.axis[UP] );

	CG_Draw2D();

	CG_ResetTemporaryBoneposesCache(); // clear for next frame
}
