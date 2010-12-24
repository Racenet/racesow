/*
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

// r_mesh.c: transformation and sorting

#include "r_local.h"

static mempool_t *r_meshlistmempool;

meshlist_t r_worldlist, r_shadowlist;
static meshlist_t r_portallist, r_skyportallist;

static qboolean R_AddPortalSurface( const meshbuffer_t *mb );
static qboolean R_DrawPortalSurface( image_t **renderedtextures );

/*
=================
R_ReAllocMeshList
=================
*/
static int R_ReAllocMeshList( meshbuffer_t **mb, int minMeshes, int maxMeshes )
{
	int oldSize, newSize;
	meshbuffer_t *newMB;

	oldSize = maxMeshes;
	newSize = max( minMeshes, oldSize * 2 );

	newMB = Mem_AllocExt( r_meshlistmempool, newSize * sizeof( meshbuffer_t ), 0 );
	if( *mb )
	{
		memcpy( newMB, *mb, oldSize * sizeof( meshbuffer_t ) );
		Mem_Free( *mb );
	}
	*mb = newMB;

	return newSize;
}

/*
=================
R_AddMeshToList

Calculate sortkey and store info used for batching and sorting.
All 3D-geometry passes this function.
=================
*/
meshbuffer_t *R_AddMeshToList( int type, const mfog_t *fog, const shader_t *shader, int infokey, const mesh_t *mesh, unsigned short numVerts, unsigned short numElems )
{
	meshlist_t *list;
	meshbuffer_t *meshbuf;
	qboolean realloc = qfalse;

	if( !shader )
		return NULL;
	if( shader->flags & SHADER_PORTAL )
	{
		if( ri.params & ( RP_MIRRORVIEW|RP_PORTALVIEW|RP_SKYPORTALVIEW ) )
			return NULL;
	}

	list = ri.meshlist;
	if( shader->sort > SHADER_SORT_OPAQUE )
	{
		// reallocate if needed
		if( list->num_translucent_meshes >= list->max_translucent_meshes )
		{
			realloc = qtrue;
			list->max_translucent_meshes = R_ReAllocMeshList( &list->meshbuffer_translucent, MIN_RENDER_MESHES/2, list->max_translucent_meshes );
		}

		if( shader->flags & SHADER_PORTAL )
			ri.meshlist->num_portal_translucent_meshes++;
		meshbuf = &list->meshbuffer_translucent[list->num_translucent_meshes++];
	}
	else
	{
		// reallocate if needed
		if( list->num_opaque_meshes >= list->max_opaque_meshes )
		{
			realloc = qtrue;
			list->max_opaque_meshes = R_ReAllocMeshList( &list->meshbuffer_opaque, MIN_RENDER_MESHES, list->max_opaque_meshes );
		}

		if( shader->flags & SHADER_PORTAL )
			ri.meshlist->num_portal_opaque_meshes++;
		meshbuf = &list->meshbuffer_opaque[list->num_opaque_meshes++];
	}

	// NULL all pointers to old membuffers so we don't crash
	if( realloc && r_worldmodel && !( ri.refdef.rdflags & RDF_NOWORLDMODEL ) && ri.meshlist->surfmbuffers )
		memset( ri.meshlist->surfmbuffers, 0, r_worldbrushmodel->numsurfaces * sizeof( meshbuffer_t * ) );

	if( shader->flags & SHADER_VIDEOMAP )
		R_UploadCinematicShader( shader );

	meshbuf->sortkey = MB_ENTITY2NUM( ri.currententity ) | MB_FOG2NUM( fog ) | type;
	meshbuf->shaderkey = shader->sortkey;
	meshbuf->infokey = infokey;
	meshbuf->dlightbits = 0;
	meshbuf->shadowbits = r_entShadowBits[ri.currententity - r_entities];
	meshbuf->vbo = NULL;

	if( mesh )
	{
		meshbuf->mesh = mesh;
		meshbuf->numVerts = numVerts;
		meshbuf->numElems = numElems;
	}
	else
	{
		meshbuf->mesh = NULL;
		meshbuf->numVerts = meshbuf->numElems = 0;
	}

	return meshbuf;
}

/*
=================
R_AddMeshToList
=================
*/
void R_AddModelMeshToList( unsigned int modhandle, const mfog_t *fog, const shader_t *shader, int meshnum )
{
	meshbuffer_t *mb;

	mb = R_AddMeshToList( MB_MODEL, fog, shader, -( meshnum+1 ), NULL, 0, 0 );
	if( mb )
		mb->LODModelHandle = modhandle;

#ifdef HARDWARE_OUTLINES
	if( !glConfig.ext.GLSL && ri.currententity->outlineHeight/* && !(ri.params & RP_SHADOWMAPVIEW)*/ )
	{
		if( ( shader->sort == SHADER_SORT_OPAQUE ) && ( shader->flags & SHADER_CULL_FRONT ) )
			R_AddModelMeshOutline( modhandle, fog, meshnum );
	}
#endif
}

/*
================
R_BatchMeshBuffer

Draw the mesh or batch it.
================
*/
static int R_BatchMeshBuffer( const meshbuffer_t *meshbuffers, int cur, int end )
{
	int next;
	int type, features;
	qboolean nonMergable;
	const meshbuffer_t *mb;
	const meshbuffer_t *nextmb;
	entity_t *ent;
	shader_t *shader;
	qboolean stopBatching;

	next = cur + 1;
	mb = meshbuffers + cur;
	MB_NUM2ENTITY( mb->sortkey, ent );

	if( ri.currententity != ent )
	{
		ri.previousentity = ri.currententity;
		ri.currententity = ent;
		ri.currentmodel = ent->model;
	}

	type = mb->sortkey & 3;

	switch( type )
	{
	case MB_MODEL:
		switch( ent->model->type )
		{
		case mod_brush:
			MB_NUM2SHADER( mb->shaderkey, shader );

			features = shader->features;

			if( shader->flags & SHADER_SKY )
			{
				// render skybox and skydome, unless already rendered
				if( !( ri.params & RP_NOSKY ) )
				{
					if( R_DrawSky( shader ) )
						ri.params |= RP_NOSKY;
				}

				// to make sure level geometry doesn't bleed through the sky
				// render sky surfaces with zero color mask and depth write enabled
				// (primitive occluding surfaces) - only used on q1 maps
				if( mapConfig.depthWritingSky )
					features |= MF_NOCOLORWRITE;
				else
					break;
			}

			if( r_shownormals->integer )
				features |= MF_NORMALS;
#ifdef HARDWARE_OUTLINES
			if( ENTITY_OUTLINE(ent) )
				features |= (MF_NORMALS|MF_ENABLENORMALS);
#endif
			if( mb->shadowbits )
				features |= (MF_NORMALS|MF_ENABLENORMALS);
			if( mapConfig.polygonOffsetSubmodels && ent != r_worldent )
				features |= MF_POLYGONOFFSET2;

			features |= r_superLightStyles[((mb->sortkey >> 10) - 1) & (MAX_SUPER_STYLES-1)].features;

			stopBatching = qfalse;
			for( nextmb = meshbuffers + next; next <= end; next++, mb = nextmb++ )
			{
				// check if we need to render batched geometry this frame
				if( next < end
					&& ( nextmb->shaderkey == mb->shaderkey )
					&& ( nextmb->sortkey == mb->sortkey )
					&& ( nextmb->dlightbits == mb->dlightbits )
					&& ( nextmb->shadowbits == mb->shadowbits ) )
				{

					nonMergable = mb->vbo || R_MeshOverflow2( mb, nextmb );
					if( nonMergable && !r_backacc.numVerts )
						features |= MF_NONBATCHED;
				}
				else
				{
					nonMergable = qtrue;
					stopBatching = qtrue;
				}

				if( features & MF_NONBATCHED )
					nonMergable = qtrue;

				R_PushMesh( mb->vbo, mb->mesh, features );

				if( nonMergable )
				{
					if( ri.previousentity != ri.currententity )
						R_RotateForEntity( ri.currententity );
					R_RenderMeshBuffer( mb );
				}

				if( stopBatching )
					break;
			}
			break;
		case mod_alias:
			R_DrawAliasModel( mb );
			break;
#ifdef QUAKE2_JUNK
		case mod_sprite:
			R_PushSpriteModel( mb );

			// no rotation for sprites
			R_TranslateForEntity( ri.currententity );
			R_RenderMeshBuffer( mb );
			break;
#endif
		case mod_skeletal:
			R_DrawSkeletalModel( mb );
			break;
		default:
			assert( 0 );    // shut up compiler
			break;
		}
		break;
	case MB_SPRITE:
	case MB_CORONA:
		for( nextmb = meshbuffers + next; next <= end; next++, mb = nextmb++ )
		{
			nonMergable = R_PushSpritePoly( mb );
			stopBatching = ( next == end
				|| ( nextmb->shaderkey & 0xFC000FFF ) != ( mb->shaderkey & 0xFC000FFF )
				|| ( nextmb->sortkey & 0xFFFFF ) != ( mb->sortkey & 0xFFFFF )
			);

			if( !nonMergable )
			{
				ri.previousentity = ri.currententity;
				ri.currententity = r_worldent;
			}

			if( nonMergable || stopBatching	|| R_SpriteOverflow() )
			{
                R_TranslateForEntity( ri.currententity );
				R_RenderMeshBuffer( mb );
			}

			if( stopBatching )
				break;

			ri.previousentity = ri.currententity;
			MB_NUM2ENTITY( nextmb->sortkey, ri.currententity );
		}
		break;
	case MB_POLY:
		// polys are already batched at this point
		R_PushPoly( mb );

		if( ri.previousentity != ri.currententity )
			R_LoadIdentity();
		R_RenderMeshBuffer( mb );
		break;
	}

	return next;
}

/*
================
R_SortMeshes
================
*/
static int R_MBCmp( const meshbuffer_t *mb1, const meshbuffer_t *mb2 )
{
	if( mb1->shaderkey > mb2->shaderkey )
		return 1;
	if( mb2->shaderkey > mb1->shaderkey )
		return -1;

	if( mb1->sortkey > mb2->sortkey )
		return 1;
	if( mb2->sortkey > mb1->sortkey )
		return -1;

	if( mb1->dlightbits > mb2->dlightbits )
		return 1;
	if( mb2->dlightbits > mb1->dlightbits )
		return -1;

	return 0;
}

void R_SortMeshes( void )
{
	if( r_draworder->integer )
		return;

	qsort( ri.meshlist->meshbuffer_opaque, ri.meshlist->num_opaque_meshes, sizeof( meshbuffer_t ), (int (*)(const void *, const void *))R_MBCmp );
	qsort( ri.meshlist->meshbuffer_translucent, ri.meshlist->num_translucent_meshes, sizeof( meshbuffer_t ), (int (*)(const void *, const void *))R_MBCmp );
}

/*
================
R_DrawPortals

Render portal views. For regular portals we stop after rendering the
first valid portal view.
Skyportal views are rendered afterwards.
================
*/
#define MAX_ONSCREEN_PORTALS	255
void R_DrawPortals( void )
{
	int i;
	int trynum, num_meshes, total_meshes;
	meshbuffer_t *mb;
	shader_t *shader;
	image_t *portaltextures[MAX_ONSCREEN_PORTALS+1];

	if( r_viewcluster == -1 )
		return;

	if( !( ri.params & ( RP_MIRRORVIEW|RP_PORTALVIEW|RP_SHADOWMAPVIEW ) ) )
	{
		if( ri.meshlist->num_portal_opaque_meshes || ri.meshlist->num_portal_translucent_meshes )
		{
			memset( portaltextures, 0, sizeof( portaltextures ) );

			trynum = 0;

			R_AddPortalSurface( NULL );

			do
			{
				switch( trynum )
				{
				case 0:
					mb = ri.meshlist->meshbuffer_opaque;
					total_meshes = ri.meshlist->num_opaque_meshes;
					num_meshes = ri.meshlist->num_portal_opaque_meshes;
					break;
				case 1:
					mb = ri.meshlist->meshbuffer_translucent;
					total_meshes = ri.meshlist->num_translucent_meshes;
					num_meshes = ri.meshlist->num_portal_translucent_meshes;
					break;
				default:
					mb = NULL;
					total_meshes = num_meshes = 0;
					assert( 0 );
					break;
				}

				for( i = 0; i < total_meshes && num_meshes; i++, mb++ )
				{
					MB_NUM2SHADER( mb->shaderkey, shader );

					if( shader->flags & SHADER_PORTAL )
					{
						num_meshes--;

						if( R_FASTSKY() && !( shader->flags & (SHADER_PORTAL_CAPTURE|SHADER_PORTAL_CAPTURE2) ) )
							continue;

						if( !R_AddPortalSurface( mb ) )
						{
							if( R_DrawPortalSurface( portaltextures ) )
							{
								trynum = 2;
								break;
							}
						}
					}
				}
			} while( ++trynum < 2 );

			R_DrawPortalSurface( portaltextures );
		}
	}

	if( ( ri.refdef.rdflags & RDF_SKYPORTALINVIEW ) && !( ri.params & RP_NOSKY ) && !R_FASTSKY() )
	{
		for( i = 0, mb = ri.meshlist->meshbuffer_opaque; i < ri.meshlist->num_opaque_meshes; i++, mb++ )
		{
			MB_NUM2SHADER( mb->shaderkey, shader );

			if( shader->flags & SHADER_SKY )
			{
				if( R_DrawSky( shader ) )
				{
					ri.params |= RP_NOSKY;
					return;
				}
			}
			else
			{
				if( shader->sort > SHADER_SORT_SKY )
					return;
			}
		}
	}
}

/*
================
R_DrawMeshes
================
*/
void R_DrawMeshes( void )
{
	int i;

	ri.previousentity = NULL;
	if( ri.meshlist->num_opaque_meshes )
	{
		for( i = 0; i < ri.meshlist->num_opaque_meshes;  )
			i = R_BatchMeshBuffer( ri.meshlist->meshbuffer_opaque, i, ri.meshlist->num_opaque_meshes );
	}

	if( ri.meshlist->num_translucent_meshes )
	{
		for( i = 0; i < ri.meshlist->num_translucent_meshes;  )
			i = R_BatchMeshBuffer( ri.meshlist->meshbuffer_translucent, i, ri.meshlist->num_translucent_meshes );
	}

	R_LoadIdentity();
}

/*
===============
R_InitMeshLists
===============
*/
void R_InitMeshLists( void )
{
	if( !r_meshlistmempool )
		r_meshlistmempool = Mem_AllocPool( NULL, "MeshList" );
}

/*
===============
R_AllocWorldMeshLists
===============
*/
void R_AllocWorldMeshLists( void )
{
	int i;
	int min_meshes;
	meshlist_t *list, *lists[] = { &r_worldlist, &r_portallist, &r_skyportallist };

	min_meshes = max( r_worldbrushmodel->numsurfaces, MIN_RENDER_MESHES );
	for( i = 0; i < sizeof( lists ) / sizeof( lists[0] ); i++ )
	{
		list = lists[i];

		list->surfmbuffers = Mem_Alloc( r_meshlistmempool, r_worldbrushmodel->numsurfaces * sizeof( meshbuffer_t * ) );
		list->max_opaque_meshes = R_ReAllocMeshList( &list->meshbuffer_opaque, min_meshes, list->max_opaque_meshes );
		list->max_translucent_meshes = R_ReAllocMeshList( &list->meshbuffer_translucent, min_meshes / 2, list->max_translucent_meshes );
	}
}

/*
===============
R_FreeMeshLists
===============
*/
void R_FreeMeshLists( void )
{
	if( !r_meshlistmempool )
		return;

	Mem_FreePool( &r_meshlistmempool );

	memset( &r_worldlist, 0, sizeof( meshlist_t ) );
	memset( &r_shadowlist, 0, sizeof( meshlist_t ) );
	memset( &r_portallist, 0, sizeof( meshlist_t ) );
	memset( &r_skyportallist, 0, sizeof( r_skyportallist ) );
}

/*
===============
R_ClearMeshList
===============
*/
void R_ClearMeshList( meshlist_t *meshlist )
{
	// clear counters
	meshlist->num_opaque_meshes = 0;
	meshlist->num_translucent_meshes = 0;

	meshlist->num_portal_opaque_meshes = 0;
	meshlist->num_portal_translucent_meshes = 0;
}

/*
===============
R_DrawTriangleOutlines
===============
*/
void R_DrawTriangleOutlines( qboolean showTris, qboolean showNormals )
{
	if( !showTris && !showNormals )
		return;

	ri.params |= (showTris ? RP_TRISOUTLINES : 0) | (showNormals ? RP_SHOWNORMALS : 0);
	R_BackendBeginTriangleOutlines();
	R_DrawMeshes();
	R_BackendEndTriangleOutlines();
	ri.params &= ~(RP_TRISOUTLINES|RP_SHOWNORMALS);
}

/*
===============
R_AddPortalSurface
===============
*/
static const meshbuffer_t *r_portal_mb;
static entity_t *r_portal_ent;
static cplane_t r_portal_plane, r_original_portal_plane;
static shader_t *r_portal_shader;
static vec3_t r_portal_mins, r_portal_maxs, r_portal_centre;

static qboolean R_AddPortalSurface( const meshbuffer_t *mb )
{
	int i;
	float dist;
	entity_t *ent;
	shader_t *shader;
	msurface_t *surf;
	cplane_t plane, oplane;
	mesh_t *mesh;
	vec3_t mins, maxs, centre;
	vec3_t v[3], entity_rotation[3];
	qboolean continueBatching = qtrue;

	if( !mb )
	{
		r_portal_mb = NULL;
		r_portal_ent = r_worldent;
		r_portal_shader = NULL;
		VectorClear( r_portal_plane.normal );
		ClearBounds( r_portal_mins, r_portal_maxs );
		return qfalse;
	}

	MB_NUM2ENTITY( mb->sortkey, ent );
	if( !ent->model )
		return qfalse;

	surf = mb->infokey > 0 ? &r_worldbrushmodel->surfaces[mb->infokey-1] : NULL;
	if( !surf || !( mesh = surf->mesh ) || !mesh->xyzArray )
		return qfalse;

	MB_NUM2SHADER( mb->shaderkey, shader );

	for( i = 0; i < 3; i++ )
		VectorCopy( mesh->xyzArray[mesh->elems[i]], v[i] );

	PlaneFromPoints( v, &oplane );
	oplane.dist += DotProduct( ent->origin, oplane.normal );
	CategorizePlane( &oplane );

	if( shader->flags & SHADER_AUTOSPRITE )
	{
		vec3_t centre;

		VectorCopy( mesh->xyzArray[mesh->elems[3]], centre );
		for( i = 0; i < 3; i++ )
			VectorAdd( centre, v[i], centre );
		VectorMA( ent->origin, 0.25, centre, centre );

		VectorNegate( ri.viewAxis[0], plane.normal );
		plane.dist = DotProduct( plane.normal, centre );
		CategorizePlane( &plane );
	}
	else
	{
		if( !Matrix_Compare( ent->axis, axis_identity ) )
		{
			Matrix_Transpose( ent->axis, entity_rotation );
			Matrix_TransformVector( entity_rotation, mesh->xyzArray[mesh->elems[0]], v[0] ); VectorMA( ent->origin, ent->scale, v[0], v[0] );
			Matrix_TransformVector( entity_rotation, mesh->xyzArray[mesh->elems[1]], v[1] ); VectorMA( ent->origin, ent->scale, v[1], v[1] );
			Matrix_TransformVector( entity_rotation, mesh->xyzArray[mesh->elems[2]], v[2] ); VectorMA( ent->origin, ent->scale, v[2], v[2] );
			PlaneFromPoints( v, &plane );
			CategorizePlane( &plane );
		}
		else
		{
			plane = oplane;
		}
	}

	if( ( dist = PlaneDiff( ri.viewOrigin, &plane ) ) <= BACKFACE_EPSILON )
	{
		if( !( shader->flags & SHADER_PORTAL_CAPTURE2 ) )
			return qtrue;
	}

	// check if we are too far away and the portal view is obscured
	// by an alphagen portal stage
	for( i = 0; i < shader->numpasses; i++ )
	{
		if( shader->passes[i].alphagen.type == ALPHA_GEN_PORTAL )
		{
			if( dist >= shader->passes[i].alphagen.args[0] )
				return qtrue; // completely alpha'ed out
		}
	}

	if( OCCLUSION_QUERIES_ENABLED( ri ) )
	{
		if( !R_GetOcclusionQueryResultBool( OQ_CUSTOM, R_SurfOcclusionQueryKey( ent, surf ), qtrue ) )
			return qtrue;
	}

	VectorAdd( ent->origin, surf->mins, mins );
	VectorAdd( ent->origin, surf->maxs, maxs );
	VectorAdd( mins, maxs, centre );
	VectorScale( centre, 0.5, centre );

	// if rendering to a texture, ignore previously collected portal information
	// as it's main purpose is to support multiple onscreen portals that
	// render to main framebuffer
	if( shader->flags & SHADER_PORTAL_CAPTURE )
	{
		continueBatching = qfalse;

		r_portal_ent = ent;
		r_portal_shader = NULL;
		VectorClear( r_portal_plane.normal );
		ClearBounds( r_portal_mins, r_portal_maxs );
	}

	if( r_portal_shader && ( shader != r_portal_shader ) )
	{
		if( DistanceSquared( ri.viewOrigin, centre ) > DistanceSquared( ri.viewOrigin, r_portal_centre ) )
			return qtrue;
		VectorClear( r_portal_plane.normal );
		ClearBounds( r_portal_mins, r_portal_maxs );
	}

	r_portal_mb = mb;
	r_portal_shader = shader;

	if( !Matrix_Compare( ent->axis, axis_identity ) )
	{
		continueBatching = qfalse;

		r_portal_ent = ent;
		r_portal_plane = plane;
		r_original_portal_plane = oplane;
		VectorCopy( surf->mins, r_portal_mins );
		VectorCopy( surf->maxs, r_portal_maxs );
		return qfalse;
	}

	if( !VectorCompare( r_portal_plane.normal, vec3_origin ) && !( VectorCompare( plane.normal, r_portal_plane.normal ) && plane.dist == r_portal_plane.dist ) )
	{
		if( DistanceSquared( ri.viewOrigin, centre ) > DistanceSquared( ri.viewOrigin, r_portal_centre ) )
			return qtrue;
		VectorClear( r_portal_plane.normal );
		ClearBounds( r_portal_mins, r_portal_maxs );
	}

	if( VectorCompare( r_portal_plane.normal, vec3_origin ) )
	{
		r_portal_plane = plane;
		r_original_portal_plane = oplane;
	}

	AddPointToBounds( mins, r_portal_mins, r_portal_maxs );
	AddPointToBounds( maxs, r_portal_mins, r_portal_maxs );
	VectorAdd( r_portal_mins, r_portal_maxs, r_portal_centre );
	VectorScale( r_portal_centre, 0.5, r_portal_centre );

	return continueBatching;
}

/*
===============
R_DrawPortalSurface

Renders the portal view and captures the results from framebuffer if
we need to do a $portalmap stage. Note that for $portalmaps we must
use a different viewport.
Return qtrue upon success so that we can stop rendering portals.
===============
*/
static qboolean R_DrawPortalSurface( image_t **renderedtextures )
{
	unsigned int i;
	int x, y, w, h;
	int oldcluster, oldarea;
	float dist, d, best_d;
	refinst_t oldRI;
	vec3_t origin, axis[3];
	entity_t *ent, *best;
	cplane_t *portal_plane = &r_portal_plane, *original_plane = &r_original_portal_plane;
	shader_t *shader = r_portal_shader;
	qboolean mirror, refraction = qfalse;
	image_t *captureTexture;
	int captureTextureID;
	qboolean doReflection, doRefraction;
	qboolean useFBO = qfalse;

	if( !r_portal_shader )
		return qfalse;

	doReflection = doRefraction = qtrue;
	if( shader->flags & SHADER_PORTAL_CAPTURE )
	{
		shaderpass_t *pass;

		captureTexture = NULL;
		captureTextureID = 1;

		for( i = 0, pass = shader->passes; i < shader->numpasses; i++, pass++ )
		{
			if( pass->program && pass->program_type == PROGRAM_TYPE_DISTORTION )
			{
				if( ( pass->alphagen.type == ALPHA_GEN_CONST && pass->alphagen.args[0] == 1 ) )
					doRefraction = qfalse;
				else if( ( pass->alphagen.type == ALPHA_GEN_CONST && pass->alphagen.args[0] == 0 ) )
					doReflection = qfalse;
				break;
			}
		}
	}
	else
	{
		captureTexture = NULL;
		captureTextureID = 0;
	}

	x = y = 0;
	w = ri.refdef.width;
	h = ri.refdef.height;

	// copy portal plane here because we may be flipped later for refractions
	ri.portalPlane = r_original_portal_plane;

	dist = PlaneDiff( ri.viewOrigin, portal_plane );
	if( ( (ri.params & RP_BACKFACECULL) && (dist <= BACKFACE_EPSILON) ) || !doReflection )
	{
		if( !( shader->flags & SHADER_PORTAL_CAPTURE2 ) || !doRefraction )
			return qfalse;

		// even if we're behind the portal, we still need to capture
		// the second portal image for refraction
		refraction = qtrue;
		captureTexture = NULL;
		captureTextureID = 2;
		if( dist < 0 )
		{
			VectorInverse( portal_plane->normal );
			portal_plane->dist = -portal_plane->dist;
		}
	}

	if( !(ri.params & RP_NOVIS) && !R_ScissorForEntityBBox( r_portal_ent, r_portal_mins, r_portal_maxs, &x, &y, &w, &h ) )
		return qfalse;

	mirror = qtrue; // default to mirror view
	// it is stupid IMO that mirrors require a RT_PORTALSURFACE entity

	best = NULL;
	best_d = 100000000;
	for( i = 1; i < r_numEntities; i++ )
	{
		ent = &r_entities[i];
		if( ent->rtype != RT_PORTALSURFACE )
			continue;

		d = PlaneDiff( ent->origin, original_plane );
		if( ( d >= -64 ) && ( d <= 64 ) )
		{
			d = Distance( ent->origin, r_portal_centre );
			if( d < best_d )
			{
				best = ent;
				best_d = d;
			}
		}
	}

	if( best == NULL )
	{
		if( !captureTextureID )
			return qfalse;
	}
	else
	{
		if( !VectorCompare( best->origin, best->origin2 ) )	// portal
			mirror = qfalse;
		best->rtype = -1;
	}

	oldcluster = r_viewcluster;
	oldarea = r_viewarea;
setup_and_render:
	ri.previousentity = NULL;
	memcpy( &oldRI, &prevRI, sizeof( refinst_t ) );
	memcpy( &prevRI, &ri, sizeof( refinst_t ) );

	if( refraction )
	{
		VectorInverse( portal_plane->normal );
		portal_plane->dist = -portal_plane->dist - 1;
		CategorizePlane( portal_plane );
		VectorCopy( ri.viewOrigin, origin );
		Matrix_Copy( ri.refdef.viewaxis, axis );

		ri.params = RP_PORTALVIEW;
		if( !mirror )
			ri.params |= RP_PVSCULL;
		if( r_viewcluster != -1 )
			ri.params |= RP_OLDVIEWCLUSTER;
	}
	else if( mirror )
	{
		d = -2 * ( DotProduct( ri.viewOrigin, portal_plane->normal ) - portal_plane->dist );
		VectorMA( ri.viewOrigin, d, portal_plane->normal, origin );

		for( i = 0; i < 3; i++ )
		{
			d = -2 * DotProduct( ri.viewAxis[i], portal_plane->normal );
			VectorMA( ri.viewAxis[i], d, portal_plane->normal, axis[i] );
			VectorNormalize( axis[i] );
		}

		VectorInverse( axis[1] );

		ri.params = RP_MIRRORVIEW|RP_FLIPFRONTFACE;
		if( r_viewcluster != -1 )
			ri.params |= RP_OLDVIEWCLUSTER;
	}
	else
	{
		vec3_t tvec;
		vec3_t A[3], B[3], C[3], rot[3];

		// build world-to-portal rotation matrix
		VectorNegate( portal_plane->normal, A[0] );
		NormalVectorToAxis( A[0], A );

		// build portal_dest-to-world rotation matrix
		ByteToDir( best->frame, tvec );
		NormalVectorToAxis( tvec, B );
		Matrix_Transpose( B, C );

		// multiply to get world-to-world rotation matrix
		Matrix_Multiply( C, A, rot );

		// translate view origin
		VectorSubtract( ri.viewOrigin, best->origin, tvec );
		Matrix_TransformVector( rot, tvec, origin );
		VectorAdd( origin, best->origin2, origin );

		for( i = 0; i < 3; i++ )
			Matrix_TransformVector( A, ri.viewAxis[i], rot[i] );
		Matrix_Multiply( best->axis, rot, B );
		for( i = 0; i < 3; i++ )
			Matrix_TransformVector( C, B[i], axis[i] );

		// set up portal_plane
		VectorCopy( axis[0], portal_plane->normal );
		portal_plane->dist = DotProduct( best->origin2, portal_plane->normal );
		CategorizePlane( portal_plane );

		// for portals, vis data is taken from portal origin, not
		// view origin, because the view point moves around and
		// might fly into (or behind) a wall
		ri.params = RP_PORTALVIEW|RP_PVSCULL;
		VectorCopy( best->origin2, ri.pvsOrigin );
		VectorCopy( best->origin2, ri.lodOrigin );

		// ignore entities, if asked politely
		if( best->renderfx & RF_NOPORTALENTS )
			ri.params |= RP_NOENTS;
	}

	ri.refdef.rdflags &= ~( RDF_UNDERWATER|RDF_CROSSINGWATER );

	ri.shadowGroup = NULL;
	ri.meshlist = &r_portallist;

	ri.params |= RP_CLIPPLANE;
	ri.clipPlane = *portal_plane;

	ri.clipFlags |= ( 1<<5 );
	ri.frustum[5] = *portal_plane;
	CategorizePlane( &ri.frustum[5] );

	// if we want to render to a texture, initialize texture
	// but do not try to render to it more than once
	if( captureTextureID )
	{
		int slot;
		const char *key = R_PortalKeyForPlane( &r_original_portal_plane );

		slot = R_FindPortalTextureSlot( key, captureTextureID );
		if( !slot )
			return qtrue;		// failed to find a slot for this texture

		R_InitPortalTexture( &r_portaltextures[slot-1], key, captureTextureID, r_lastRefdef.width, r_lastRefdef.height,
			shader->flags & SHADER_NO_TEX_FILTERING ? IT_NOFILTERING : 0 );

		// make sure we don't try to render the same portal plane for this frame again
		captureTexture = r_portaltextures[slot-1];
		for( i = 0; renderedtextures[i]; i++ )
		{
			if( renderedtextures[i] == captureTexture )
			{
				captureTexture = NULL;
				goto done;	// continue rendering portals
			}
		}
		if( i == MAX_ONSCREEN_PORTALS )
			goto done;
		renderedtextures[i] = captureTexture;

		x = y = 0;
		w = captureTexture->upload_width;
		h = captureTexture->upload_height;
		ri.refdef.width = w;
		ri.refdef.height = h;
		if( captureTexture->fbo )
		{
			ri.refdef.x = 0;
			ri.refdef.y = 0;
		}
		Vector4Set( ri.viewport, ri.refdef.x, ri.refdef.y, w, h );
	}

	Vector4Set( ri.scissor, ri.refdef.x + x, ri.refdef.y + y, w, h );
	VectorCopy( origin, ri.refdef.vieworg );
	Matrix_Copy( axis, ri.refdef.viewaxis );

	// check if we can use framebuffer object for RTT (render-to-texture)
	useFBO = qfalse;
	if( captureTexture && captureTexture->fbo )
		useFBO = R_UseFBObject( captureTexture->fbo );

	R_RenderView( &ri.refdef );

done:
	if( !( ri.params & RP_OLDVIEWCLUSTER ) )
		r_oldviewcluster = -1;		// force markleafs
	r_viewcluster = oldcluster;		// restore viewcluster for current frame
	r_viewarea = oldarea;

	memcpy( &ri, &prevRI, sizeof( refinst_t ) );
	memcpy( &prevRI, &oldRI, sizeof( refinst_t ) );

	if( captureTexture )
	{
		GL_Cull( 0 );
		GL_SetState( GLSTATE_NO_DEPTH_TEST );
		qglColor4f( 1, 1, 1, 1 );

		// grab the results from framebuffer
		if( useFBO )
		{
			R_UseFBObject( 0 );
		}
		else
		{
			GL_Bind( 0, captureTexture );
			qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, ri.refdef.x, ri.refdef.y, captureTexture->upload_width, captureTexture->upload_height );
		}
		captureTexture->framenum = r_framecount;
	}

	if( doRefraction && !refraction && ( shader->flags & SHADER_PORTAL_CAPTURE2 ) )
	{
		refraction = qtrue;
		captureTexture = NULL;
		captureTextureID = 2;
		goto setup_and_render;
	}

	R_AddPortalSurface( NULL );

	if( shader->flags & (SHADER_PORTAL_CAPTURE|SHADER_PORTAL_CAPTURE2) )
		return qfalse;
	return qtrue;
}

/*
===============
R_PortalKeyForPlane
===============
*/
const char *R_PortalKeyForPlane( cplane_t *plane )
{
	static char key[80];

	Q_snprintfz( key, sizeof( key ), "%i_%i_%i_%i", (int)plane->normal[0], (int)plane->normal[1],
		(int)plane->normal[2], (int)plane->dist );

	return key;
}

/*
===============
R_DrawSkyPortal
===============
*/
qboolean R_DrawSkyPortal( skyportal_t *skyportal, vec3_t mins, vec3_t maxs )
{
	int x, y, w, h;
	int oldcluster, oldarea;
	refinst_t oldRI;

	if( !R_ScissorForEntityBBox( r_worldent, mins, maxs, &x, &y, &w, &h ) )
		return qfalse;

	oldcluster = r_viewcluster;
	oldarea = r_viewarea;
	ri.previousentity = NULL;
	memcpy( &oldRI, &prevRI, sizeof( refinst_t ) );
	memcpy( &prevRI, &ri, sizeof( refinst_t ) );

	ri.params = ( ri.params|RP_SKYPORTALVIEW ) & ~( RP_OLDVIEWCLUSTER );
	VectorCopy( skyportal->vieworg, ri.pvsOrigin );

	ri.clipFlags = 15;
	ri.shadowGroup = NULL;
	ri.meshlist = &r_skyportallist;
	Vector4Set( ri.scissor, ri.refdef.x + x, ri.refdef.y + y, w, h );
//	Vector4Set( ri.viewport, ri.refdef.x, glState.height - ri.refdef.height - ri.refdef.y, ri.refdef.width, ri.refdef.height );

	if( skyportal->scale )
	{
		vec3_t centre, diff;

		VectorAdd( r_worldmodel->mins, r_worldmodel->maxs, centre );
		VectorScale( centre, 0.5f, centre );
		VectorSubtract( centre, ri.viewOrigin, diff );
		VectorMA( skyportal->vieworg, -skyportal->scale, diff, ri.refdef.vieworg );
	}
	else
	{
		VectorCopy( skyportal->vieworg, ri.refdef.vieworg );
	}

	// FIXME
	if( !VectorCompare( skyportal->viewanglesOffset, vec3_origin ) )
	{
		vec3_t axis[3], angles;

		Matrix_Copy( ri.refdef.viewaxis, axis );
		VectorInverse( axis[1] );
		Matrix_EulerAngles( axis, angles );

		VectorAdd( angles, skyportal->viewanglesOffset, angles );
		AnglesToAxis( angles, axis );
		Matrix_Copy( axis, ri.refdef.viewaxis );
	}

	ri.refdef.rdflags &= ~( RDF_UNDERWATER|RDF_CROSSINGWATER|RDF_SKYPORTALINVIEW );
	if( skyportal->fov )
	{
		ri.refdef.fov_x = skyportal->fov;
		ri.refdef.fov_y = CalcFov( ri.refdef.fov_x, ri.refdef.width, ri.refdef.height );
		if( glState.wideScreen && !( ri.refdef.rdflags & RDF_NOFOVADJUSTMENT ) )
			AdjustFov( &ri.refdef.fov_x, &ri.refdef.fov_y, glState.width, glState.height, qfalse );
	}

	R_RenderView( &ri.refdef );

	r_oldviewcluster = -1;			// force markleafs
	r_viewcluster = oldcluster;		// restore viewcluster for current frame
	r_viewarea = oldarea;

	memcpy( &ri, &prevRI, sizeof( refinst_t ) );
	memcpy( &prevRI, &oldRI, sizeof( refinst_t ) );

	return qtrue;
}

/*
===============
R_DrawCubemapView
===============
*/
void R_DrawCubemapView( vec3_t origin, vec3_t angles, int size )
{
	refdef_t *fd;

	fd = &ri.refdef;
	*fd = r_lastRefdef;
	fd->time = 0;
	fd->x = fd->y = 0;
	fd->width = size;
	fd->height = size;
	fd->fov_x = 90;
	fd->fov_y = 90;
	VectorCopy( origin, fd->vieworg );
	AnglesToAxis( angles, fd->viewaxis );

	r_numPolys = 0;
	r_numDlights = 0;

	R_RenderScene( fd );

	r_oldviewcluster = r_viewcluster = -1;		// force markleafs next frame
}

/*
===============
R_BuildTangentVectors
===============
*/
void R_BuildTangentVectors( int numVertexes, vec4_t *xyzArray, vec4_t *normalsArray, vec2_t *stArray, int numTris, elem_t *elems, vec4_t *sVectorsArray )
{
	int i, j;
	float d, *v[3], *tc[3];
	vec_t *s, *t, *n;
	vec3_t stvec[3], cross;
	vec3_t stackTVectorsArray[128];
	vec3_t *tVectorsArray;

	if( numVertexes > sizeof( stackTVectorsArray )/sizeof( stackTVectorsArray[0] ) )
		tVectorsArray = Mem_TempMalloc( sizeof( vec3_t )*numVertexes );
	else
		tVectorsArray = stackTVectorsArray;

	// assuming arrays have already been allocated
	// this also does some nice precaching
	memset( sVectorsArray, 0, numVertexes * sizeof( *sVectorsArray ) );
	memset( tVectorsArray, 0, numVertexes * sizeof( *tVectorsArray ) );

	for( i = 0; i < numTris; i++, elems += 3 )
	{
		for( j = 0; j < 3; j++ )
		{
			v[j] = ( float * )( xyzArray + elems[j] );
			tc[j] = ( float * )( stArray + elems[j] );
		}

		// calculate two mostly perpendicular edge directions
		VectorSubtract( v[1], v[0], stvec[0] );
		VectorSubtract( v[2], v[0], stvec[1] );

		// we have two edge directions, we can calculate the normal then
		CrossProduct( stvec[1], stvec[0], cross );

		for( j = 0; j < 3; j++ )
		{
			stvec[0][j] = ( ( tc[1][1] - tc[0][1] ) * ( v[2][j] - v[0][j] ) - ( tc[2][1] - tc[0][1] ) * ( v[1][j] - v[0][j] ) );
			stvec[1][j] = ( ( tc[1][0] - tc[0][0] ) * ( v[2][j] - v[0][j] ) - ( tc[2][0] - tc[0][0] ) * ( v[1][j] - v[0][j] ) );
		}

		// inverse tangent vectors if their cross product goes in the opposite
		// direction to triangle normal
		CrossProduct( stvec[1], stvec[0], stvec[2] );
		if( DotProduct( stvec[2], cross ) < 0 )
		{
			VectorInverse( stvec[0] );
			VectorInverse( stvec[1] );
		}

		for( j = 0; j < 3; j++ )
		{
			VectorAdd( sVectorsArray[elems[j]], stvec[0], sVectorsArray[elems[j]] );
			VectorAdd( tVectorsArray[elems[j]], stvec[1], tVectorsArray[elems[j]] );
		}
	}

	// normalize
	for( i = 0, s = *sVectorsArray, t = *tVectorsArray, n = *normalsArray; i < numVertexes; i++, s += 4, t += 3, n += 4 )
	{
		// keep s\t vectors perpendicular
		d = -DotProduct( s, n );
		VectorMA( s, d, n, s );
		VectorNormalize( s );

		d = -DotProduct( t, n );
		VectorMA( t, d, n, t );

		// store polarity of t-vector in the 4-th coordinate of s-vector
		CrossProduct( n, s, cross );
		if( DotProduct( cross, t ) < 0 )
			s[3] = -1;
		else
			s[3] = 1;
	}

	if( tVectorsArray != stackTVectorsArray )
		Mem_TempFree( tVectorsArray );
}

/*
=========================================================

STATIC VERTEX BUFFER OBJECTS

=========================================================
*/

#define MAX_MESH_VERTREX_BUFFER_OBJECTS 	65536

static mesh_vbo_t mesh_vbo[MAX_MESH_VERTREX_BUFFER_OBJECTS];
static int num_mesh_vbos;

static void R_DestroyMeshVBO( mesh_vbo_t *vbo );

/*
* R_InitVBO
*/
void R_InitVBO( void )
{
	num_mesh_vbos = 0;
	memset( mesh_vbo, 0,  sizeof( mesh_vbo ) );
}

/*
* R_CreateStaticMeshVBO
*
* Create two static buffer objects: vertex buffer and elements buffer, the real
* data is uploaded by calling R_UploadVBOVertexData and R_UploadVBOElemData
*/
mesh_vbo_t *R_CreateStaticMeshVBO( void *owner, int numVerts, int numElems, int features )
{
	int i;
	size_t size;
	GLuint vbo_id;
	mesh_vbo_t *vbo = NULL;

	if( !glConfig.ext.vertex_buffer_object )
		return NULL;

	if( num_mesh_vbos == MAX_MESH_VERTREX_BUFFER_OBJECTS )
		return NULL;

	vbo = &mesh_vbo[num_mesh_vbos];
	memset( vbo, 0, sizeof( *vbo ) );

	// vertex data
	size = 0;
	size += numVerts * sizeof( vec4_t );

	// normals data
	vbo->normalsOffset = size;
	size += numVerts * sizeof( vec4_t );

	// s-vectors (tangent vectors)
	if( features & MF_SVECTORS )
	{
		vbo->sVectorsOffset = size;
		size += numVerts * sizeof( vec4_t );
	}

	// texture coordinates
	if( features & MF_STCOORDS )
	{
		vbo->stOffset = size;
		size += numVerts * sizeof( vec2_t );
	}

	// lightmap texture coordinates
	for( i = 0; i < MAX_LIGHTMAPS; i++ )
	{
		if( !(features & (MF_LMCOORDS<<i)) )
			break;
		vbo->lmstOffset[i] = size;
		size += numVerts * sizeof( vec2_t );
	}

	// vertex colors
	for( i = 0; i < MAX_LIGHTMAPS; i++ )
	{
		if( !(features & (MF_COLORS<<i)) )
			break;
		vbo->colorsOffset[i] = size;
		size += numVerts * sizeof( byte_vec4_t );
	}

	// pre-allocate vertex buffer
	vbo_id = 0;
	qglGenBuffersARB( 1, &vbo_id );
	if( !vbo_id )
		goto error;
	vbo->vertexId = vbo_id;

	qglBindBufferARB( GL_ARRAY_BUFFER_ARB, vbo->vertexId );
	qglBufferDataARB( GL_ARRAY_BUFFER_ARB, size, NULL, GL_STATIC_DRAW_ARB );
	if( qglGetError () == GL_OUT_OF_MEMORY )
		goto error;

	vbo->size += size;

	// pre-allocate elements buffer
	vbo_id = 0;
	qglGenBuffersARB( 1, &vbo_id );
	if( !vbo_id )
		goto error;
	vbo->elemId = vbo_id;

	size = numElems * sizeof( unsigned int );
	qglBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, vbo->elemId );
	qglBufferDataARB( GL_ELEMENT_ARRAY_BUFFER_ARB, size, NULL, GL_STATIC_DRAW_ARB );
	if( qglGetError () == GL_OUT_OF_MEMORY )
		goto error;

	vbo->size += size;

	qglBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
	qglBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, 0 );

	vbo->numVerts = numVerts;
	vbo->numElems = numElems;
	vbo->owner = owner;

	num_mesh_vbos++;
	return vbo;

error:
	if( vbo )
		R_DestroyMeshVBO( vbo );

	qglBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
	qglBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, 0 );

	return NULL;
}

/*
* R_DestroyMeshVBO
*/
static void R_DestroyMeshVBO( mesh_vbo_t *vbo )
{
	GLuint vbo_id;

	assert( vbo );

	if( vbo->vertexId )
	{
		vbo_id = vbo->vertexId;
		qglDeleteBuffersARB( 1, &vbo_id );
	}

	if( vbo->elemId )
	{
		vbo_id = vbo->elemId;
		qglDeleteBuffersARB( 1, &vbo_id );
	}

	memset( vbo, 0, sizeof( *vbo ) );
}

/*
* R_UploadVBOVertexData
*
* Uploads required vertex data to the buffer
*/
int R_UploadVBOVertexData( mesh_vbo_t *vbo, int vertsOffset, mesh_t *mesh )
{
	int i;
	int numVerts;
	int errArrays;

	assert( vbo );
	assert( mesh );

	errArrays = 0;
	numVerts = mesh->numVerts;
	if( !vbo->vertexId )
		return 0;

	qglBindBufferARB( GL_ARRAY_BUFFER_ARB, vbo->vertexId );

	// upload vertex xyz data
	if( mesh->xyzArray )
		qglBufferSubDataARB( GL_ARRAY_BUFFER_ARB, 0 + vertsOffset * sizeof( vec4_t ), numVerts * sizeof( vec4_t ), mesh->xyzArray );

	// upload normals data
	if( vbo->normalsOffset )
	{
		if( !mesh->normalsArray )
			errArrays |= MF_NORMALS;
		else
			qglBufferSubDataARB( GL_ARRAY_BUFFER_ARB, vbo->normalsOffset + vertsOffset * sizeof( vec4_t ), numVerts * sizeof( vec4_t ), mesh->normalsArray );
	}

	// upload tangent vectors
	if( vbo->sVectorsOffset )
	{
		if( !mesh->sVectorsArray )
			errArrays |= MF_SVECTORS;
		else
			qglBufferSubDataARB( GL_ARRAY_BUFFER_ARB, vbo->sVectorsOffset + vertsOffset * sizeof( vec4_t ), numVerts * sizeof( vec4_t ), mesh->sVectorsArray );
	}

	// upload texture coordinates
	if( vbo->stOffset )
	{
		if( !mesh->stArray )
			errArrays |= MF_STCOORDS;
		else
			qglBufferSubDataARB( GL_ARRAY_BUFFER_ARB, vbo->stOffset + vertsOffset * sizeof( vec2_t ), numVerts * sizeof( vec2_t ), mesh->stArray );
	}

	// upload lightmap texture coordinates
	for( i = 0; i < MAX_LIGHTMAPS; i++ )
	{
		if( !vbo->lmstOffset[i] )
			break;
		if( !mesh->lmstArray[i] )
		{
			errArrays |= MF_LMCOORDS<<i;
			break;
		}
		qglBufferSubDataARB( GL_ARRAY_BUFFER_ARB, vbo->lmstOffset[i] + vertsOffset * sizeof( vec2_t ), numVerts * sizeof( vec2_t ), mesh->lmstArray[i] );
	}

	// upload vertex colors (although indices > 0 are never used)
	for( i = 0; i < MAX_LIGHTMAPS; i++ )
	{
		if( !vbo->colorsOffset[i] )
			break;
		if( !mesh->colorsArray[i] )
		{
			errArrays |= MF_COLORS<<i;
			break;
		}
		qglBufferSubDataARB( GL_ARRAY_BUFFER_ARB, vbo->colorsOffset[i] + vertsOffset * sizeof( byte_vec4_t ), numVerts * sizeof( byte_vec4_t ), mesh->colorsArray[i] );
	}

	qglBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );

	return errArrays;
}

/*
* R_UploadVBOElemData
*
* Upload elements into the buffer, properly offsetting them (batching)
*/
void R_UploadVBOElemData( mesh_vbo_t *vbo, int vertsOffset, int elemsOffset, mesh_t *mesh )
{
	int i;
	unsigned int *ielems;

	assert( vbo );

	if( !vbo->elemId )
		return;

	ielems = Mem_TempMalloc( sizeof( *ielems ) * mesh->numElems );
	for( i = 0; i < mesh->numElems; i++ )
		ielems[i] = vertsOffset + mesh->elems[i];

	qglBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, vbo->elemId );
	qglBufferSubDataARB( GL_ELEMENT_ARRAY_BUFFER_ARB, elemsOffset * sizeof( unsigned int ), mesh->numElems * sizeof( unsigned int ), ielems );
	qglBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, 0 );

	Mem_TempFree( ielems );
}

/*
* R_ShutdownVBO
*/
void R_ShutdownVBO( void )
{
	int i;
	mesh_vbo_t *vbo;

	for( i = 0, vbo = mesh_vbo; i < num_mesh_vbos; i++, vbo++ )
		R_DestroyMeshVBO( vbo );

	num_mesh_vbos = 0;
}
