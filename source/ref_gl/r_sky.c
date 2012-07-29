/*
Copyright (C) 1999 Stephen C. Taylor
Copyright (C) 2002-2007 Victor Luchits

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

// r_sky.c

#include "r_local.h"


#define SIDE_SIZE   9
#define POINTS_LEN  ( SIDE_SIZE*SIDE_SIZE )
#define ELEM_LEN    ( ( SIDE_SIZE-1 )*( SIDE_SIZE-1 )*6 )

#define SPHERE_RAD  10.0
#define EYE_RAD     9.0

#define SCALE_S	    4.0  // Arbitrary (?) texture scaling factors
#define SCALE_T	    4.0

#define BOX_SIZE    1.0f
#define BOX_STEP    BOX_SIZE / ( SIDE_SIZE-1 ) * 2.0f

elem_t	r_skydome_elems[6][ELEM_LEN];
meshbuffer_t r_skydome_mbuffer;

static mfog_t *r_skyfog;
static msurface_t *r_warpface;
static qboolean	r_warpfacevis;

static void Gen_BoxSide( skydome_t *skydome, int side, vec3_t orig, vec3_t drow, vec3_t dcol, float skyheight );
static void Gen_Box( skydome_t *skydome, float skyheight );

void MakeSkyVec( float x, float y, float z, int axis, vec3_t v );

/*
* R_CreateSkydome
*/
static void R_AddSkydomeToFarclip( const skydome_t *skydome )
{
	const float skyheight = skydome->skyheight;

//	if( skyheight*sqrt(3) > r_farclip_min )
//		r_farclip_min = skyheight*sqrt(3);
	if( skyheight*2 > r_farclip_min )
		r_farclip_min = skyheight*2;
}

/*
* R_CreateSkydome
*/
skydome_t *R_CreateSkydome( float skyheight, shader_t **farboxShaders, shader_t	**nearboxShaders )
{
	int i, size;
	mesh_t *mesh;
	skydome_t *skydome;
	qbyte *buffer;

	size = sizeof( skydome_t ) + sizeof( mesh_t ) * 6 + sizeof( vec4_t ) * POINTS_LEN * 6 +
		sizeof( vec4_t ) * POINTS_LEN * 6 + sizeof( vec2_t ) * POINTS_LEN * 11;
	buffer = Shader_Malloc( size );

	skydome = ( skydome_t * )buffer;
	memcpy( skydome->farboxShaders, farboxShaders, sizeof( shader_t * ) * 6 );
	memcpy( skydome->nearboxShaders, nearboxShaders, sizeof( shader_t * ) * 6 );
	buffer += sizeof( skydome_t );

	skydome->skyheight = skyheight;
	skydome->meshes = ( mesh_t * )buffer;
	buffer += sizeof( mesh_t ) * 6;

	for( i = 0, mesh = skydome->meshes; i < 6; i++, mesh++ )
	{
		mesh->numVerts = POINTS_LEN;
		mesh->xyzArray = ( vec4_t * )buffer; buffer += sizeof( vec4_t ) * POINTS_LEN;
		mesh->normalsArray = ( vec4_t * )buffer; buffer += sizeof( vec4_t ) * POINTS_LEN;
		if( i != 5 )
		{
			skydome->sphereStCoords[i] = ( vec2_t * )buffer; buffer += sizeof( vec2_t ) * POINTS_LEN;
		}
		skydome->linearStCoords[i] = ( vec2_t * )buffer; buffer += sizeof( vec2_t ) * POINTS_LEN;

		mesh->numElems = ELEM_LEN;
		mesh->elems = r_skydome_elems[i];
	}

	Gen_Box( skydome, skyheight );

	R_AddSkydomeToFarclip( skydome );

	return skydome;
}

/*
* R_TouchSkydome
*/
void R_TouchSkydome( skydome_t *skydome )
{
	int i;

	for( i = 0; i < 6; i++ ) {
		if( skydome->farboxShaders[i] ) {
			R_TouchShader( skydome->farboxShaders[i] );
		}
		if( skydome->nearboxShaders[i] ) {
			R_TouchShader( skydome->nearboxShaders[i] );
		}
	}

	R_AddSkydomeToFarclip( skydome );
}

/*
* R_FreeSkydome
*/
void R_FreeSkydome( skydome_t *skydome )
{
	Shader_Free( skydome );
}

/*
* Gen_Box
*/
static void Gen_Box( skydome_t *skydome, float skyheight )
{
	int axis;
	vec3_t orig, drow, dcol;

	for( axis = 0; axis < 6; axis++ )
	{
		MakeSkyVec( -BOX_SIZE, -BOX_SIZE, BOX_SIZE, axis, orig );
		MakeSkyVec( 0, BOX_STEP, 0, axis, drow );
		MakeSkyVec( BOX_STEP, 0, 0, axis, dcol );

		Gen_BoxSide( skydome, axis, orig, drow, dcol, skyheight );
	}
}

/*
* Gen_BoxSide
* 
* I don't know exactly what Q3A does for skybox texturing, but
* this is at least fairly close.  We tile the texture onto the
* inside of a large sphere, and put the camera near the top of
* the sphere. We place the box around the camera, and cast rays
* through the box verts to the sphere to find the texture coordinates.
*/
static void Gen_BoxSide( skydome_t *skydome, int side, vec3_t orig, vec3_t drow, vec3_t dcol, float skyheight )
{
	vec3_t pos, w, row, norm;
	float *v, *n, *st = NULL, *st2;
	int r, c;
	float t, d, d2, b, b2, q[2], s;

	s = 1.0 / ( SIDE_SIZE-1 );
	d = EYE_RAD; // sphere center to camera distance
	d2 = d * d;
	b = SPHERE_RAD; // sphere radius
	b2 = b * b;
	q[0] = 1.0 / ( 2.0 * SCALE_S );
	q[1] = 1.0 / ( 2.0 * SCALE_T );

	v = skydome->meshes[side].xyzArray[0];
	n = skydome->meshes[side].normalsArray[0];
	if( side != 5 )
		st = skydome->sphereStCoords[side][0];
	st2 = skydome->linearStCoords[side][0];

	VectorCopy( orig, row );

//	CrossProduct( dcol, drow, norm );
//	VectorNormalize( norm );
	VectorClear( norm );

	for( r = 0; r < SIDE_SIZE; r++ )
	{
		VectorCopy( row, pos );
		for( c = 0; c < SIDE_SIZE; c++ )
		{
			// pos points from eye to vertex on box
			VectorScale( pos, skyheight, v );
			VectorCopy( pos, w );

			// Normalize pos -> w
			VectorNormalize( w );

			// Find distance along w to sphere
			t = sqrt( d2 * ( w[2] * w[2] - 1.0 ) + b2 ) - d * w[2];
			w[0] *= t;
			w[1] *= t;

			if( st )
			{
				// use x and y on sphere as s and t
				// minus is here so skies scoll in correct (Q3A's) direction
				st[0] = -w[0] * q[0];
				st[1] = -w[1] * q[1];

				// avoid bilerp seam
				st[0] = ( bound( -1, st[0], 1 ) + 1.0 ) * 0.5;
				st[1] = ( bound( -1, st[1], 1 ) + 1.0 ) * 0.5;
			}

			st2[0] = c * s;
			st2[1] = 1.0 - r * s;

			VectorAdd( pos, dcol, pos );
			VectorCopy( norm, n );

			v += 4;
			n += 4;
			if( st ) st += 2;
			st2 += 2;
		}

		VectorAdd( row, drow, row );
	}
}

/*
* R_DrawSkySide
*/
static void R_DrawSkySide( skydome_t *skydome, int side, shader_t *shader, int features )
{
	meshbuffer_t *mbuffer = &r_skydome_mbuffer;

	if( ri.skyMins[0][side] >= ri.skyMaxs[0][side] ||
		ri.skyMins[1][side] >= ri.skyMaxs[1][side] )
		return;

	mbuffer->shaderkey = shader->sortkey;
	mbuffer->dlightbits = 0;
	mbuffer->sortkey = MB_FOG2NUM( r_skyfog );

	skydome->meshes[side].stArray = skydome->linearStCoords[side];
	R_PushMesh( &skydome->meshes[side], qfalse, features );
	R_RenderMeshBuffer( mbuffer, NULL );
}

/*
* R_DrawSkyBox
*/
static void R_DrawSkyBox( skydome_t *skydome, shader_t **shaders )
{
	int i, features;
	const int skytexorder[6] = { SKYBOX_RIGHT, SKYBOX_FRONT, SKYBOX_LEFT, SKYBOX_BACK, SKYBOX_TOP, SKYBOX_BOTTOM };

	features = shaders[0]->features;
	for( i = 0; i < 6; i++ )
		R_DrawSkySide( skydome, i, shaders[skytexorder[i]], features );
}

/*
* R_DrawBlackBottom
* 
* Draw dummy skybox side to prevent the HOM effect
*/
static void R_DrawBlackBottom( skydome_t *skydome )
{
	int features;
	shader_t *shader;

	// FIXME: register another shader instead maybe?
	shader = R_OcclusionShader ();

	features = shader->features;

	// HACK HACK HACK
	// skies ought not to write to depth buffer
	shader->flags &= ~SHADER_DEPTHWRITE;
	shader->passes[0].flags &= ~GLSTATE_DEPTHWRITE;
	R_DrawSkySide( skydome, 5, shader, features );
	shader->passes[0].flags |= GLSTATE_DEPTHWRITE;
	shader->flags |= SHADER_DEPTHWRITE;
}

/*
* R_DrawSky
*/
qboolean R_DrawSky( shader_t *shader )
{
	int i;
	int numVisSides;
	vec3_t mins, maxs;
	mat4x4_t m, oldm;
	elem_t *elem;
	skydome_t *skydome = r_skydomes[shader-r_shaders] ? r_skydomes[shader-r_shaders] : NULL;
	meshbuffer_t *mbuffer = &r_skydome_mbuffer;
	int u, v, umin, umax, vmin, vmax;
	float depthmin = gldepthmin, depthmax = gldepthmax;

	if( !skydome )
		return qfalse;

	numVisSides = 0;
	ClearBounds( mins, maxs );
	for( i = 0; i < 6; i++ )
	{
		if( ri.skyMins[0][i] >= ri.skyMaxs[0][i] ||
			ri.skyMins[1][i] >= ri.skyMaxs[1][i] )
			continue;

		// increase the visible sides counter
		numVisSides++;

		umin = (int)( ( ri.skyMins[0][i]+1.0f )*0.5f*(float)( SIDE_SIZE-1 ) );
		umax = (int)( ( ri.skyMaxs[0][i]+1.0f )*0.5f*(float)( SIDE_SIZE-1 ) ) + 1;
		vmin = (int)( ( ri.skyMins[1][i]+1.0f )*0.5f*(float)( SIDE_SIZE-1 ) );
		vmax = (int)( ( ri.skyMaxs[1][i]+1.0f )*0.5f*(float)( SIDE_SIZE-1 ) ) + 1;

		clamp( umin, 0, SIDE_SIZE-1 );
		clamp( umax, 0, SIDE_SIZE-1 );
		clamp( vmin, 0, SIDE_SIZE-1 );
		clamp( vmax, 0, SIDE_SIZE-1 );

		// Box elems in tristrip order
		elem = skydome->meshes[i].elems;
		for( v = vmin; v < vmax; v++ )
		{
			for( u = umin; u < umax; u++ )
			{
				elem[0] = v * SIDE_SIZE + u;
				elem[1] = elem[4] = elem[0] + SIDE_SIZE;
				elem[2] = elem[3] = elem[0] + 1;
				elem[5] = elem[1] + 1;
				elem += 6;
			}
		}

		AddPointToBounds( skydome->meshes[i].xyzArray[vmin*SIDE_SIZE+umin], mins, maxs );
		AddPointToBounds( skydome->meshes[i].xyzArray[vmax*SIDE_SIZE+umax], mins, maxs );

		skydome->meshes[i].numElems = ( vmax-vmin )*( umax-umin )*6;
	}

	// no sides are truly visible, ignore
	if( !numVisSides )
		return qfalse;

	VectorAdd( mins, ri.viewOrigin, mins );
	VectorAdd( maxs, ri.viewOrigin, maxs );

	if( ri.refdef.rdflags & RDF_SKYPORTALINVIEW )
		return R_DrawSkyPortal( &ri.refdef.skyportal, mins, maxs );

	GL_DepthRange( 1, 1 );

	// center skydome on camera to give the illusion of a larger space
	Matrix4_Copy( ri.modelviewMatrix, oldm );
	Matrix4_Copy( ri.worldviewMatrix, ri.modelviewMatrix );
	Matrix4_Copy( ri.worldviewMatrix, m );
	m[12] = 0;
	m[13] = 0;
	m[14] = 0;
	m[15] = 1.0;
	qglLoadMatrixf( m );

	if( ri.params & RP_CLIPPLANE )
		qglDisable( GL_CLIP_PLANE0 );

	// it can happen that sky surfaces have no fog hull specified
	// yet there's a global fog hull (see wvwq3dm7)
	if( !r_skyfog )
		r_skyfog = r_worldbrushmodel->globalfog;

	if( skydome->farboxShaders[0] )
		R_DrawSkyBox( skydome, skydome->farboxShaders );
	else
		R_DrawBlackBottom( skydome );

	if( shader->numpasses )
	{
		qboolean flush = qfalse;
		int features = shader->features;

		for( i = 0; i < 5; i++ )
		{
			if( ri.skyMins[0][i] >= ri.skyMaxs[0][i] ||
				ri.skyMins[1][i] >= ri.skyMaxs[1][i] )
				continue;

			flush = qtrue;
			mbuffer->shaderkey = shader->sortkey;
			mbuffer->dlightbits = 0;
			mbuffer->sortkey = MB_FOG2NUM( r_skyfog );

			skydome->meshes[i].stArray = skydome->sphereStCoords[i];
			R_PushMesh( &skydome->meshes[i], qfalse, features );
		}

		if( flush )
			R_RenderMeshBuffer( mbuffer, NULL );
	}

	if( skydome->nearboxShaders[0] )
		R_DrawSkyBox( skydome, skydome->nearboxShaders );

	if( ri.params & RP_CLIPPLANE )
		qglEnable( GL_CLIP_PLANE0 );

	GL_DepthRange( depthmin, depthmax );

	Matrix4_Copy( oldm, ri.modelviewMatrix );
	qglLoadMatrixf( ri.worldviewMatrix );

	r_skyfog = NULL;

	return qtrue;
}

//===================================================================

vec3_t skyclip[6] = {
	{ 1, 1, 0 },
	{ 1, -1, 0 },
	{ 0, -1, 1 },
	{ 0, 1, 1 },
	{ 1, 0, 1 },
	{ -1, 0, 1 }
};

// 1 = s, 2 = t, 3 = 2048
int st_to_vec[6][3] =
{
	{ 3, -1, 2 },
	{ -3, 1, 2 },

	{ 1, 3, 2 },
	{ -1, -3, 2 },

	{ -2, -1, 3 },  // 0 degrees yaw, look straight up
	{ 2, -1, -3 }   // look straight down
};

// s = [0]/[2], t = [1]/[2]
int vec_to_st[6][3] =
{
	{ -2, 3, 1 },
	{ 2, 3, -1 },

	{ 1, 3, 2 },
	{ -1, 3, -2 },

	{ -2, -1, 3 },
	{ -2, 1, -3 }
};

/*
* DrawSkyPolygon
*/
void DrawSkyPolygon( int nump, vec3_t vecs )
{
	int i, j;
	vec3_t v, av;
	float s, t, dv;
	int axis;
	float *vp;

	// decide which face it maps to
	VectorClear( v );

	for( i = 0, vp = vecs; i < nump; i++, vp += 3 )
		VectorAdd( vp, v, v );

	av[0] = fabs( v[0] );
	av[1] = fabs( v[1] );
	av[2] = fabs( v[2] );

	if( ( av[0] > av[1] ) && ( av[0] > av[2] ) )
		axis = ( v[0] < 0 ) ? 1 : 0;
	else if( ( av[1] > av[2] ) && ( av[1] > av[0] ) )
		axis = ( v[1] < 0 ) ? 3 : 2;
	else
		axis = ( v[2] < 0 ) ? 5 : 4;

	if( !r_skyfog )
		r_skyfog = r_warpface->fog;
	r_warpfacevis = qtrue;

	// project new texture coords
	for( i = 0; i < nump; i++, vecs += 3 )
	{
		j = vec_to_st[axis][2];
		dv = ( j > 0 ) ? vecs[j - 1] : -vecs[-j - 1];

		if( dv < 0.001 )
			continue; // don't divide by zero

		dv = 1.0f / dv;

		j = vec_to_st[axis][0];
		s = ( j < 0 ) ? -vecs[-j -1] * dv : vecs[j-1] * dv;

		j = vec_to_st[axis][1];
		t = ( j < 0 ) ? -vecs[-j -1] * dv : vecs[j-1] * dv;

		if( s < ri.skyMins[0][axis] )
			ri.skyMins[0][axis] = s;
		if( t < ri.skyMins[1][axis] )
			ri.skyMins[1][axis] = t;
		if( s > ri.skyMaxs[0][axis] )
			ri.skyMaxs[0][axis] = s;
		if( t > ri.skyMaxs[1][axis] )
			ri.skyMaxs[1][axis] = t;
	}
}

#define	MAX_CLIP_VERTS	64

/*
* ClipSkyPolygon
*/
void ClipSkyPolygon( int nump, vec3_t vecs, int stage )
{
	float *norm;
	float *v;
	qboolean front, back;
	float d, e;
	float dists[MAX_CLIP_VERTS + 1];
	int sides[MAX_CLIP_VERTS + 1];
	vec3_t newv[2][MAX_CLIP_VERTS + 1];
	int newc[2];
	int i, j;

	if( nump > MAX_CLIP_VERTS )
		Com_Error( ERR_DROP, "ClipSkyPolygon: MAX_CLIP_VERTS" );

loc1:
	if( stage == 6 )
	{	// fully clipped, so draw it
		DrawSkyPolygon( nump, vecs );
		return;
	}

	front = back = qfalse;
	norm = skyclip[stage];
	for( i = 0, v = vecs; i < nump; i++, v += 3 )
	{
		d = DotProduct( v, norm );
		if( d > ON_EPSILON )
		{
			front = qtrue;
			sides[i] = SIDE_FRONT;
		}
		else if( d < -ON_EPSILON )
		{
			back = qtrue;
			sides[i] = SIDE_BACK;
		}
		else
		{
			sides[i] = SIDE_ON;
		}
		dists[i] = d;
	}

	if( !front || !back )
	{	// not clipped
		stage++;
		goto loc1;
	}

	// clip it
	sides[i] = sides[0];
	dists[i] = dists[0];
	VectorCopy( vecs, ( vecs+( i*3 ) ) );
	newc[0] = newc[1] = 0;

	for( i = 0, v = vecs; i < nump; i++, v += 3 )
	{
		switch( sides[i] )
		{
		case SIDE_FRONT:
			VectorCopy( v, newv[0][newc[0]] );
			newc[0]++;
			break;
		case SIDE_BACK:
			VectorCopy( v, newv[1][newc[1]] );
			newc[1]++;
			break;
		case SIDE_ON:
			VectorCopy( v, newv[0][newc[0]] );
			newc[0]++;
			VectorCopy( v, newv[1][newc[1]] );
			newc[1]++;
			break;
		}

		if( sides[i] == SIDE_ON || sides[i+1] == SIDE_ON || sides[i+1] == sides[i] )
			continue;

		d = dists[i] / ( dists[i] - dists[i+1] );
		for( j = 0; j < 3; j++ )
		{
			e = v[j] + d * ( v[j+3] - v[j] );
			newv[0][newc[0]][j] = e;
			newv[1][newc[1]][j] = e;
		}
		newc[0]++;
		newc[1]++;
	}

	// continue
	ClipSkyPolygon( newc[0], newv[0][0], stage + 1 );
	ClipSkyPolygon( newc[1], newv[1][0], stage + 1 );
}

/*
* R_AddSkySurface
*/
qboolean R_AddSkySurface( msurface_t *fa )
{
	int i;
	vec4_t *vert;
	elem_t	*elem;
	mesh_t *mesh;
	vec3_t verts[4];

	// calculate vertex values for sky box
	r_warpface = fa;
	r_warpfacevis = qfalse;

	mesh = fa->mesh;
	elem = mesh->elems;
	vert = mesh->xyzArray;
	for( i = 0; i < mesh->numElems; i += 3, elem += 3 )
	{
		VectorSubtract( vert[elem[0]], ri.viewOrigin, verts[0] );
		VectorSubtract( vert[elem[1]], ri.viewOrigin, verts[1] );
		VectorSubtract( vert[elem[2]], ri.viewOrigin, verts[2] );
		ClipSkyPolygon( 3, verts[0], 0 );
	}

	return r_warpfacevis;
}

/*
* R_ClearSky
*/
void R_ClearSky( void )
{
	int i;

	for( i = 0; i < 6; i++ )
	{
		ri.skyMins[0][i] = ri.skyMins[1][i] = 9999999;
		ri.skyMaxs[0][i] = ri.skyMaxs[1][i] = -9999999;
	}
}

void MakeSkyVec( float x, float y, float z, int axis, vec3_t v )
{
	int j, k;
	vec3_t b;

	b[0] = x;
	b[1] = y;
	b[2] = z;

	for( j = 0; j < 3; j++ )
	{
		k = st_to_vec[axis][j];
		if( k < 0 )
			v[j] = -b[-k - 1];
		else
			v[j] = b[k - 1];
	}
}
