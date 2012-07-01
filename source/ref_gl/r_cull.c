/*
Copyright (C) 2007 Victor Luchits

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

#include "r_local.h"


/*
=============================================================

FRUSTUM AND PVS CULLING

=============================================================
*/

/*
* R_CullBox
* 
* Returns true if the box is completely outside the frustum
*/
qboolean R_CullBox( const vec3_t mins, const vec3_t maxs, const unsigned int clipflags )
{
	unsigned int i, bit;
	const cplane_t *p;

	if( r_nocull->integer )
		return qfalse;

	for( i = sizeof( ri.frustum )/sizeof( ri.frustum[0] ), bit = 1, p = ri.frustum; i > 0; i--, bit<<=1, p++ )
	{
		if( !( clipflags & bit ) )
			continue;

		switch( p->signbits )
		{
		case 0:
			if( p->normal[0]*maxs[0] + p->normal[1]*maxs[1] + p->normal[2]*maxs[2] < p->dist )
				return qtrue;
			break;
		case 1:
			if( p->normal[0]*mins[0] + p->normal[1]*maxs[1] + p->normal[2]*maxs[2] < p->dist )
				return qtrue;
			break;
		case 2:
			if( p->normal[0]*maxs[0] + p->normal[1]*mins[1] + p->normal[2]*maxs[2] < p->dist )
				return qtrue;
			break;
		case 3:
			if( p->normal[0]*mins[0] + p->normal[1]*mins[1] + p->normal[2]*maxs[2] < p->dist )
				return qtrue;
			break;
		case 4:
			if( p->normal[0]*maxs[0] + p->normal[1]*maxs[1] + p->normal[2]*mins[2] < p->dist )
				return qtrue;
			break;
		case 5:
			if( p->normal[0]*mins[0] + p->normal[1]*maxs[1] + p->normal[2]*mins[2] < p->dist )
				return qtrue;
			break;
		case 6:
			if( p->normal[0]*maxs[0] + p->normal[1]*mins[1] + p->normal[2]*mins[2] < p->dist )
				return qtrue;
			break;
		case 7:
			if( p->normal[0]*mins[0] + p->normal[1]*mins[1] + p->normal[2]*mins[2] < p->dist )
				return qtrue;
			break;
		default:
			assert( 0 );
			return qfalse;
		}
	}

	return qfalse;
}

/*
* R_CullSphere
* 
* Returns true if the sphere is completely outside the frustum
*/
qboolean R_CullSphere( const vec3_t centre, const float radius, const unsigned int clipflags )
{
	unsigned int i;
	unsigned int bit;
	const cplane_t *p;

	if( r_nocull->integer )
		return qfalse;

	for( i = sizeof( ri.frustum )/sizeof( ri.frustum[0] ), bit = 1, p = ri.frustum; i > 0; i--, bit<<=1, p++ )
	{
		if( !( clipflags & bit ) )
			continue;
		if( DotProduct( centre, p->normal ) - p->dist <= -radius )
			return qtrue;
	}

	return qfalse;
}

/*
* R_VisCullBox
*/
qboolean R_VisCullBox( const vec3_t mins, const vec3_t maxs )
{
	int s, stackdepth = 0;
	vec3_t extmins, extmaxs;
	mnode_t *node, *localstack[2048];

	if( !r_worldmodel || ( ri.refdef.rdflags & RDF_NOWORLDMODEL ) )
		return qfalse;
	if( ri.params & RP_NOVIS )
		return qfalse;

	for( s = 0; s < 3; s++ )
	{
		extmins[s] = mins[s] - 4;
		extmaxs[s] = maxs[s] + 4;
	}

	for( node = r_worldbrushmodel->nodes;; )
	{
		if( node->pvsframe != r_pvsframecount )
		{
			if( !stackdepth )
				return qtrue;
			node = localstack[--stackdepth];
			continue;
		}

		if( !node->plane )
			return qfalse;

		s = BOX_ON_PLANE_SIDE( extmins, extmaxs, node->plane ) - 1;
		if( s < 2 )
		{
			node = node->children[s];
			continue;
		}

		// go down both sides
		if( stackdepth < sizeof( localstack )/sizeof( mnode_t * ) )
			localstack[stackdepth++] = node->children[0];
		node = node->children[1];
	}

	return qtrue;
}

/*
* R_VisCullSphere
*/
qboolean R_VisCullSphere( const vec3_t origin, float radius )
{
	float dist;
	int stackdepth = 0;
	mnode_t *node, *localstack[2048];

	if( !r_worldmodel || ( ri.refdef.rdflags & RDF_NOWORLDMODEL ) )
		return qfalse;
	if( ri.params & RP_NOVIS )
		return qfalse;

	radius += 4;
	for( node = r_worldbrushmodel->nodes;; )
	{
		if( node->pvsframe != r_pvsframecount )
		{
			if( !stackdepth )
				return qtrue;
			node = localstack[--stackdepth];
			continue;
		}

		if( !node->plane )
			return qfalse;

		dist = PlaneDiff( origin, node->plane );
		if( dist > radius )
		{
			node = node->children[0];
			continue;
		}
		else if( dist < -radius )
		{
			node = node->children[1];
			continue;
		}

		// go down both sides
		if( stackdepth < sizeof( localstack )/sizeof( mnode_t * ) )
			localstack[stackdepth++] = node->children[0];
		node = node->children[1];
	}

	return qtrue;
}

/*
* R_CullModel
*/
int R_CullModel( entity_t *e, vec3_t mins, vec3_t maxs, float radius )
{
	if( e->flags & RF_WEAPONMODEL )
	{
		if( ri.params & RP_NONVIEWERREF )
			return 1;
		return 0;
	}

	if( e->flags & RF_VIEWERMODEL )
	{
		//if( !(ri.params & RP_NONVIEWERREF) )
		if( !( ri.params & ( RP_MIRRORVIEW|RP_SHADOWMAPVIEW ) ) )
			return 1;
	}

	// account for possible outlines
#ifdef HARDWARE_OUTLINES
	if( e->outlineHeight )
		radius += e->outlineHeight * r_outlines_scale->value * 1.73/*sqrt(3)*/;
#endif

	if( R_CullSphere( e->origin, radius, ri.clipFlags ) )
		return 1;

	if( ri.params & RP_PVSCULL )
	{
		if( R_VisCullSphere( e->origin, radius ) )
			return 2;
	}

	return 0;
}

/*
* R_CullBrushModel
*/
qboolean R_CullBrushModel( entity_t *e )
{
	qboolean rotated;
	vec3_t mins, maxs;
	float radius;
	model_t	*model = e->model;
	mbrushmodel_t *bmodel = ( mbrushmodel_t * )model->extradata;

	if( bmodel->nummodelsurfaces == 0 )
		return qtrue;

	radius = R_BrushModelBBox( e, mins, maxs, &rotated );
	if( rotated )
	{
		if( R_CullSphere( e->origin, radius, ri.clipFlags ) )
			return qtrue;
	}
	else
	{
		if( R_CullBox( mins, maxs, ri.clipFlags ) )
			return qtrue;
	}

	if( ri.params & RP_PVSCULL )
	{
		if( rotated )
		{
			if( R_VisCullSphere( e->origin, radius ) )
				return qtrue;
		}
		else
		{
			if( R_VisCullBox( mins, maxs ) )
				return qtrue;
		}
	}

	return qfalse;
}

/*
* R_CullSprite
*/
qboolean R_CullSprite( entity_t *e )
{
	float dist;

	if( e->radius <= 0 || e->customShader == NULL || e->scale <= 0 )
		return qtrue;

	dist =
		( e->origin[0] - ri.refdef.vieworg[0] ) * ri.viewAxis[0][0] +
		( e->origin[1] - ri.refdef.vieworg[1] ) * ri.viewAxis[0][1] +
		( e->origin[2] - ri.refdef.vieworg[2] ) * ri.viewAxis[0][2];

	if( dist < 0 )
		return qtrue;
	return qfalse;
}

/*
=============================================================

OCCLUSION QUERIES

=============================================================
*/

// occlusion queries for entities
#define BEGIN_OQ_ENTITIES		0
#define MAX_OQ_ENTITIES			MAX_ENTITIES
#define END_OQ_ENTITIES			( BEGIN_OQ_ENTITIES+MAX_OQ_ENTITIES )

// occlusion queries for planar shadows
#define BEGIN_OQ_PLANARSHADOWS	END_OQ_ENTITIES
#define MAX_OQ_PLANARSHADOWS	MAX_ENTITIES
#define END_OQ_PLANARSHADOWS	( BEGIN_OQ_PLANARSHADOWS+MAX_OQ_PLANARSHADOWS )

// occlusion queries for shadowgroups
#define BEGIN_OQ_SHADOWGROUPS	END_OQ_PLANARSHADOWS
#define MAX_OQ_SHADOWGROUPS		MAX_SHADOWGROUPS
#define END_OQ_SHADOWGROUPS		( BEGIN_OQ_SHADOWGROUPS+MAX_OQ_SHADOWGROUPS )

// custom occlusion queries (portals, mirrors, etc)
#define BEGIN_OQ_CUSTOM			END_OQ_SHADOWGROUPS
#define MAX_OQ_CUSTOM			MAX_SURF_QUERIES
#define END_OQ_CUSTOM			( BEGIN_OQ_CUSTOM+MAX_OQ_CUSTOM )

#define MAX_OQ_TOTAL			( MAX_OQ_ENTITIES+MAX_OQ_PLANARSHADOWS+MAX_OQ_SHADOWGROUPS+MAX_OQ_CUSTOM )

static qbyte r_queriesBits[MAX_OQ_TOTAL/8];
static GLuint r_occlusionQueries[MAX_OQ_TOTAL];

static meshbuffer_t r_occluderMB;
static shader_t *r_occlusionShader;
static qboolean r_occludersQueued;
static entity_t	*r_occlusionEntity;

static void R_RenderOccludingSurfaces( void );

/*
* R_InitOcclusionQueries
*/
void R_InitOcclusionQueries( void )
{
	meshbuffer_t *meshbuf = &r_occluderMB;

	if( !r_occlusionShader )
		r_occlusionShader = R_LoadShader( "***r_occlusion***", SHADER_OPAQUE_OCCLUDER, qfalse, 0, SHADER_INVALID, NULL );

	if( !glConfig.ext.occlusion_query )
		return;

	qglGenQueriesARB( MAX_OQ_TOTAL, r_occlusionQueries );

	meshbuf->sortkey = MB_ENTITY2NUM( r_worldent ) | MB_MODEL;
	meshbuf->shaderkey = r_occlusionShader->sortkey;
}

/*
* R_BeginOcclusionPass
*/
void R_BeginOcclusionPass( void )
{
	assert( OCCLUSION_QUERIES_ENABLED( ri ) );

	r_occludersQueued = qfalse;
	r_occlusionEntity = r_worldent;
	memset( r_queriesBits, 0, sizeof( r_queriesBits ) );

	R_LoadIdentity();

	R_BackendResetPassMask();

	R_ClearSurfOcclusionQueryKeys();

	qglDisable( GL_TEXTURE_2D );
}

/*
* R_EndOcclusionPass
*/
void R_EndOcclusionPass( void )
{
	assert( OCCLUSION_QUERIES_ENABLED( ri ) );

	R_RenderOccludingSurfaces();

	R_SurfIssueOcclusionQueries();

	R_BackendResetPassMask();

	R_BackendResetCounters();

	if( r_occlusion_queries_finish->integer )
		qglFinish();
	else
		qglFlush();

	qglEnable( GL_TEXTURE_2D );
}

/*
* R_RenderOccludingSurfaces
*/
static void R_RenderOccludingSurfaces( void )
{
	if( !r_occludersQueued )
		return;

	r_occludersQueued = qfalse;
	R_RotateForEntity( r_occlusionEntity );
	R_BackendResetPassMask();
	R_RenderMeshBuffer( &r_occluderMB, NULL );
}

/*
* R_OcclusionShader
*/
shader_t *R_OcclusionShader( void )
{
	return r_occlusionShader;
}

/*
* R_AddOccludingSurface
*/
void R_AddOccludingSurface( msurface_t *surf, shader_t *shader )
{
	int diff = shader->flags ^ r_occlusionShader->flags;

	if( R_MeshOverflow( surf->mesh ) || ( diff & ( SHADER_CULL_FRONT|SHADER_CULL_BACK ) ) || r_occlusionEntity != ri.currententity )
	{
		R_RenderOccludingSurfaces();

		r_occlusionShader->flags ^= diff;
		r_occlusionEntity = ri.currententity;
	}
	r_occludersQueued = qtrue;

	R_PushMesh( surf->mesh, surf->vbo != NULL, MF_NOCOLORWRITE );
}

/*
* R_GetOcclusionQueryNum
*/
int R_GetOcclusionQueryNum( int type, int key )
{
	switch( type )
	{
	case OQ_ENTITY:
		return ( r_occlusion_queries->integer & 1 ) ? BEGIN_OQ_ENTITIES + ( key%MAX_OQ_ENTITIES ) : -1;
	case OQ_PLANARSHADOW:
		return ( r_occlusion_queries->integer & 1 ) ? BEGIN_OQ_PLANARSHADOWS + ( key%MAX_OQ_PLANARSHADOWS ) : -1;
	case OQ_SHADOWGROUP:
		return ( r_occlusion_queries->integer & 2 ) ? BEGIN_OQ_SHADOWGROUPS + ( key%MAX_OQ_SHADOWGROUPS ) : -1;
	case OQ_CUSTOM:
		return BEGIN_OQ_CUSTOM + ( key%MAX_OQ_CUSTOM );
	}

	return -1;
}

/*
* R_IssueOcclusionQuery
*/
int R_IssueOcclusionQuery( int query, entity_t *e, vec3_t mins, vec3_t maxs )
{
	static vec4_t verts[8];
	static mesh_t mesh;
	static elem_t indices[] =
	{
		0, 1, 2, 1, 2, 3, // top
		7, 5, 6, 5, 6, 4, // bottom
		4, 0, 6, 0, 6, 2, // front
		3, 7, 1, 7, 1, 5, // back
		0, 1, 4, 1, 4, 5, // left
		7, 6, 3, 6, 3, 2, // right
	};
	int i;
	vec3_t bbox[8];

	if( query < 0 )
		return -1;
	if( r_queriesBits[query>>3] & ( 1<<( query&7 ) ) )
		return -1;
	r_queriesBits[query>>3] |= ( 1<<( query&7 ) );

	R_RenderOccludingSurfaces();

	mesh.numVerts = 8;
	mesh.xyzArray = verts;
	mesh.numElems = 36;
	mesh.elems = indices;
	r_occlusionShader->flags &= ~( SHADER_CULL_BACK | SHADER_CULL_FRONT );

	qglBeginQueryARB( GL_SAMPLES_PASSED_ARB, r_occlusionQueries[query] );

	R_TransformEntityBBox( e, mins, maxs, bbox, qfalse );
	for( i = 0; i < 8; i++ )
		Vector4Set( verts[i], bbox[i][0], bbox[i][1], bbox[i][2], 1 );

	R_RotateForEntity( e );
	R_BackendSetPassMask( GLSTATE_MASK & ~GLSTATE_DEPTHWRITE );
	R_PushMesh( &mesh, qfalse, MF_NONBATCHED|MF_NOCULL );
	R_RenderMeshBuffer( &r_occluderMB, NULL );

	qglEndQueryARB( GL_SAMPLES_PASSED_ARB );

	return query;
}

/*
* R_OcclusionQueryIssued
*/
qboolean R_OcclusionQueryIssued( int query )
{
	assert( query >= 0 && query < MAX_OQ_TOTAL );
	return r_queriesBits[query>>3] & ( 1<<( query&7 ) );
}

/*
* R_GetOcclusionQueryResult
*/
unsigned int R_GetOcclusionQueryResult( int query, qboolean wait )
{
	GLint available;
	GLuint sampleCount;

	if( query < 0 )
		return 1;
	if( !R_OcclusionQueryIssued( query ) )
		return 1;

	query = r_occlusionQueries[query];

	qglGetQueryObjectivARB( query, GL_QUERY_RESULT_AVAILABLE_ARB, &available );
	if( !available && wait )
	{
		int n = 0;
		do
		{
			n++;
			qglGetQueryObjectivARB( query, GL_QUERY_RESULT_AVAILABLE_ARB, &available );
		} while( !available && ( n < 10000 ) );
	}

	if( !available )
		return 0;

	qglGetQueryObjectuivARB( query, GL_QUERY_RESULT_ARB, &sampleCount );

	return sampleCount;
}

/*
* R_GetOcclusionQueryResultBool
*/
qboolean R_GetOcclusionQueryResultBool( int type, int key, qboolean wait )
{
	int query = R_GetOcclusionQueryNum( type, key );

	if( query >= 0 && R_OcclusionQueryIssued( query ) )
	{
		if( R_GetOcclusionQueryResult( query, wait ) )
			return qtrue;
		return qfalse;
	}

	return qtrue;
}

/*
* R_ShutdownOcclusionQueries
*/
void R_ShutdownOcclusionQueries( void )
{
	r_occlusionShader = NULL;

	if( !glConfig.ext.occlusion_query )
		return;

	qglDeleteQueriesARB( MAX_OQ_TOTAL, r_occlusionQueries );
	r_occludersQueued = qfalse;
}
