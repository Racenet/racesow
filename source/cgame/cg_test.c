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

#ifndef PUBLIC_BUILD
#include "cg_local.h"

// cg_test.c -- test crap

cvar_t *cg_testEntities;
cvar_t *cg_testLights;

void CG_DrawTestLine( vec3_t start, vec3_t end )
{
	CG_QuickPolyBeam( start, end, 6, CG_MediaShader( cgs.media.shaderLaser ) );
}

void CG_DrawTestBox( vec3_t origin, vec3_t mins, vec3_t maxs, vec3_t angles )
{
	vec3_t start, end, vec;
	float linewidth = 6;
	vec3_t localAxis[3];
#if 1
	vec3_t ax[3];
	AnglesToAxis( angles, ax );
	Matrix_Transpose( ax, localAxis );
#else
	Matrix_Copy( axis_identity, localAxis );
	if( angles[YAW] ) Matrix_Rotate( localAxis, -angles[YAW], 0, 0, 1 );
	if( angles[PITCH] ) Matrix_Rotate( localAxis, -angles[PITCH], 0, 1, 0 );
	if( angles[ROLL] ) Matrix_Rotate( localAxis, -angles[ROLL], 1, 0, 0 );
#endif

	//horizontal projection
	start[0] = mins[0];
	start[1] = mins[1];
	start[2] = mins[2];

	end[0] = mins[0];
	end[1] = mins[1];
	end[2] = maxs[2];

	// convert to local axis space
	VectorCopy( start, vec );
	Matrix_TransformVector( localAxis, vec, start );
	VectorCopy( end, vec );
	Matrix_TransformVector( localAxis, vec, end );

	VectorAdd( origin, start, start );
	VectorAdd( origin, end, end );

	CG_QuickPolyBeam( start, end, linewidth, NULL );

	start[0] = mins[0];
	start[1] = maxs[1];
	start[2] = mins[2];

	end[0] = mins[0];
	end[1] = maxs[1];
	end[2] = maxs[2];

	// convert to local axis space
	VectorCopy( start, vec );
	Matrix_TransformVector( localAxis, vec, start );
	VectorCopy( end, vec );
	Matrix_TransformVector( localAxis, vec, end );

	VectorAdd( origin, start, start );
	VectorAdd( origin, end, end );

	CG_QuickPolyBeam( start, end, linewidth, NULL );

	start[0] = maxs[0];
	start[1] = mins[1];
	start[2] = mins[2];

	end[0] = maxs[0];
	end[1] = mins[1];
	end[2] = maxs[2];

	// convert to local axis space
	VectorCopy( start, vec );
	Matrix_TransformVector( localAxis, vec, start );
	VectorCopy( end, vec );
	Matrix_TransformVector( localAxis, vec, end );

	VectorAdd( origin, start, start );
	VectorAdd( origin, end, end );

	CG_QuickPolyBeam( start, end, linewidth, NULL );

	start[0] = maxs[0];
	start[1] = maxs[1];
	start[2] = mins[2];

	end[0] = maxs[0];
	end[1] = maxs[1];
	end[2] = maxs[2];

	// convert to local axis space
	VectorCopy( start, vec );
	Matrix_TransformVector( localAxis, vec, start );
	VectorCopy( end, vec );
	Matrix_TransformVector( localAxis, vec, end );

	VectorAdd( origin, start, start );
	VectorAdd( origin, end, end );

	CG_QuickPolyBeam( start, end, linewidth, NULL );

	//x projection
	start[0] = mins[0];
	start[1] = mins[1];
	start[2] = mins[2];

	end[0] = maxs[0];
	end[1] = mins[1];
	end[2] = mins[2];

	// convert to local axis space
	VectorCopy( start, vec );
	Matrix_TransformVector( localAxis, vec, start );
	VectorCopy( end, vec );
	Matrix_TransformVector( localAxis, vec, end );

	VectorAdd( origin, start, start );
	VectorAdd( origin, end, end );

	CG_QuickPolyBeam( start, end, linewidth, NULL );

	start[0] = mins[0];
	start[1] = maxs[1];
	start[2] = maxs[2];

	end[0] = maxs[0];
	end[1] = maxs[1];
	end[2] = maxs[2];

	// convert to local axis space
	VectorCopy( start, vec );
	Matrix_TransformVector( localAxis, vec, start );
	VectorCopy( end, vec );
	Matrix_TransformVector( localAxis, vec, end );

	VectorAdd( origin, start, start );
	VectorAdd( origin, end, end );

	CG_QuickPolyBeam( start, end, linewidth, NULL );

	start[0] = mins[0];
	start[1] = maxs[1];
	start[2] = mins[2];

	end[0] = maxs[0];
	end[1] = maxs[1];
	end[2] = mins[2];

	// convert to local axis space
	VectorCopy( start, vec );
	Matrix_TransformVector( localAxis, vec, start );
	VectorCopy( end, vec );
	Matrix_TransformVector( localAxis, vec, end );

	VectorAdd( origin, start, start );
	VectorAdd( origin, end, end );

	CG_QuickPolyBeam( start, end, linewidth, NULL );

	start[0] = mins[0];
	start[1] = mins[1];
	start[2] = maxs[2];

	end[0] = maxs[0];
	end[1] = mins[1];
	end[2] = maxs[2];

	// convert to local axis space
	VectorCopy( start, vec );
	Matrix_TransformVector( localAxis, vec, start );
	VectorCopy( end, vec );
	Matrix_TransformVector( localAxis, vec, end );

	VectorAdd( origin, start, start );
	VectorAdd( origin, end, end );

	CG_QuickPolyBeam( start, end, linewidth, NULL );

	//z projection
	start[0] = mins[0];
	start[1] = mins[1];
	start[2] = mins[2];

	end[0] = mins[0];
	end[1] = maxs[1];
	end[2] = mins[2];

	// convert to local axis space
	VectorCopy( start, vec );
	Matrix_TransformVector( localAxis, vec, start );
	VectorCopy( end, vec );
	Matrix_TransformVector( localAxis, vec, end );

	VectorAdd( origin, start, start );
	VectorAdd( origin, end, end );

	CG_QuickPolyBeam( start, end, linewidth, NULL );

	start[0] = maxs[0];
	start[1] = mins[1];
	start[2] = maxs[2];

	end[0] = maxs[0];
	end[1] = maxs[1];
	end[2] = maxs[2];

	// convert to local axis space
	VectorCopy( start, vec );
	Matrix_TransformVector( localAxis, vec, start );
	VectorCopy( end, vec );
	Matrix_TransformVector( localAxis, vec, end );

	VectorAdd( origin, start, start );
	VectorAdd( origin, end, end );

	CG_QuickPolyBeam( start, end, linewidth, NULL );

	start[0] = maxs[0];
	start[1] = mins[1];
	start[2] = mins[2];

	end[0] = maxs[0];
	end[1] = maxs[1];
	end[2] = mins[2];

	// convert to local axis space
	VectorCopy( start, vec );
	Matrix_TransformVector( localAxis, vec, start );
	VectorCopy( end, vec );
	Matrix_TransformVector( localAxis, vec, end );

	VectorAdd( origin, start, start );
	VectorAdd( origin, end, end );

	CG_QuickPolyBeam( start, end, linewidth, NULL );

	start[0] = mins[0];
	start[1] = mins[1];
	start[2] = maxs[2];

	end[0] = mins[0];
	end[1] = maxs[1];
	end[2] = maxs[2];

	// convert to local axis space
	VectorCopy( start, vec );
	Matrix_TransformVector( localAxis, vec, start );
	VectorCopy( end, vec );
	Matrix_TransformVector( localAxis, vec, end );

	VectorAdd( origin, start, start );
	VectorAdd( origin, end, end );

	CG_QuickPolyBeam( start, end, linewidth, NULL );
}

/*
* CG_TestEntities
* 
* If cg_testEntities is set, create 32 player models
*/
static void CG_TestEntities( void )
{
	int i, j;
	float f, r;
	entity_t ent;

	memset( &ent, 0, sizeof( ent ) );

	trap_R_ClearScene();

	for( i = 0; i < 100; i++ )
	{
		r = 64 * ( ( i%4 ) - 1.5 );
		f = 64 * ( i/4 ) + 128;

		for( j = 0; j < 3; j++ )
			ent.origin[j] = ent.lightingOrigin[j] = cg.view.origin[j] + cg.view.axis[FORWARD][j]*f + cg.view.axis[RIGHT][j]*r;

		Matrix_Copy( cg.autorotateAxis, ent.axis );

		ent.scale = 1.0f;
		ent.rtype = RT_MODEL;
		// skelmod splitmodels
		ent.model = cgs.basePModelInfo->model;
		if( cgs.baseSkin )
			ent.customSkin = cgs.baseSkin;
		else
			ent.customSkin = NULL;

		CG_AddEntityToScene( &ent ); // skelmod
	}
}

/*
* CG_TestLights
* 
* If cg_testLights is set, create 32 lights models
*/
static void CG_TestLights( void )
{
	int i, j;
	float f, r;
	vec3_t origin;

	for( i = 0; i < min( cg_testLights->integer, 32 ); i++ )
	{
		r = 64 * ( ( i%4 ) - 1.5 );
		f = 64 * ( i/4 ) + 128;

		for( j = 0; j < 3; j++ )
			origin[j] = cg.view.origin[j] /* + cg.view.axis[FORWARD][j]*f + cg.view.axis[RIGHT][j]*r*/;
		CG_AddLightToScene( origin, 200, ( ( i%6 )+1 ) & 1, ( ( ( i%6 )+1 ) & 2 )>>1, ( ( ( i%6 )+1 ) & 4 )>>2, NULL );
	}
}

/*
* CG_TestBlend
*/
void CG_AddTest( void )
{
	if( !cg_testEntities || !cg_testLights )
	{
		cg_testEntities =	trap_Cvar_Get( "cg_testEntities", "0", CVAR_CHEAT );
		cg_testLights =		trap_Cvar_Get( "cg_testLights", "0", CVAR_CHEAT );
	}

	if( cg_testEntities->integer )
		CG_TestEntities();
	if( cg_testLights->integer )
		CG_TestLights();
}

#else

#ifdef _WIN32
#ifndef __MINGW32__
static void DisableLevel4Warning( void )
{
	// wsw : aiwa : added to shut up VC8 level 4 warning
}
#endif
#endif

#endif // _DEBUG
