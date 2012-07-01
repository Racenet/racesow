/*
Copyright (C) 1997-2001 Id Software, Inc.
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

// r_surf.c: surface-related refresh code

#include "r_local.h"

static vec3_t modelorg;       // relative to viewpoint
static vec3_t modelmins;
static vec3_t modelmaxs;

/*
=============================================================

BRUSH MODELS

=============================================================
*/

/*
* R_SurfPotentiallyVisible
*/
qboolean R_SurfPotentiallyVisible( msurface_t *surf )
{
	if( surf->flags & SURF_NODRAW )
		return qfalse;
	if( surf->facetype == FACETYPE_FLARE )
		return qtrue;
	if( !surf->mesh || R_InvalidMesh( surf->mesh ) )
		return qfalse;
	return qtrue;
}

/*
* R_CullSurface
*/
qboolean R_CullSurface( msurface_t *surf, unsigned int clipflags )
{
	shader_t *shader = surf->shader;

	if( ( shader->flags & SHADER_SKY ) && R_FASTSKY() )
		return qtrue;
	if( r_nocull->integer )
		return qfalse;
	if( ( shader->flags & SHADER_ALLDETAIL ) && !r_detailtextures->integer )
		return qtrue;

	// flare
	if( surf->facetype == FACETYPE_FLARE )
	{
		if( r_flares->integer && r_flarefade->value )
		{
			vec3_t origin;

			if( ri.currentmodel != r_worldmodel )
			{
				Matrix_TransformVector( ri.currententity->axis, surf->origin, origin );
				VectorAdd( origin, ri.currententity->origin, origin );
			}
			else
			{
				VectorCopy( surf->origin, origin );
			}

			// cull it because we don't want to sort unneeded things
			if( ( origin[0] - ri.viewOrigin[0] ) * ri.viewAxis[0][0] +
				( origin[1] - ri.viewOrigin[1] ) * ri.viewAxis[0][1] +
				( origin[2] - ri.viewOrigin[2] ) * ri.viewAxis[0][2] < 0 )
				return qtrue;

			return ( clipflags && R_CullSphere( origin, 1, clipflags ) );
		}
		return qtrue;
	}

	if( surf->facetype == FACETYPE_PLANAR && (ri.params & RP_BACKFACECULL)
#ifdef HARDWARE_OUTLINES
		&& (!ri.currententity->outlineHeight || (ri.params & RP_CLIPPLANE))
#endif
		&& ( shader->flags & ( SHADER_CULL_FRONT|SHADER_CULL_BACK ) )
		)
	{
		// Vic: I hate q3map2. I really do.
		if( !VectorCompare( surf->plane->normal, vec3_origin ) )
		{
			float dist;

			dist = PlaneDiff( modelorg, surf->plane );
			if( ( shader->flags & SHADER_CULL_FRONT ) || ( ri.params & RP_MIRRORVIEW ) )
			{
				if( dist <= BACKFACE_EPSILON )
					return qtrue;
			}
			else
			{
				if( dist >= -BACKFACE_EPSILON )
					return qtrue;
			}
		}
	}

	return ( clipflags && R_CullBox( surf->mins, surf->maxs, clipflags ) );
}

/*
* R_BrushModelBBox
*/
float R_BrushModelBBox( entity_t *e, vec3_t mins, vec3_t maxs, qboolean *rotated )
{
	int i;
	model_t	*model = e->model;

	if( !Matrix_Compare( e->axis, axis_identity ) )
	{
		if( rotated )
			*rotated = qtrue;
		for( i = 0; i < 3; i++ )
		{
			mins[i] = e->origin[i] - model->radius * e->scale;
			maxs[i] = e->origin[i] + model->radius * e->scale;
		}
		return model->radius * e->scale;
	}
	else
	{
		if( rotated )
			*rotated = qfalse;
		VectorMA( e->origin, e->scale, model->mins, mins );
		VectorMA( e->origin, e->scale, model->maxs, maxs );
		return RadiusFromBounds( mins, maxs );
	}
}

/*
* R_AddSurfaceToList
*/
static meshbuffer_t *R_AddSurfaceToList( msurface_t *surf, unsigned int clipflags )
{
	shader_t *shader;
	meshbuffer_t *mb;
	mesh_vbo_t *vbo;
	msurface_t *vbo_surf;

	if( R_CullSurface( surf, clipflags ) )
		return NULL;

	shader = ((r_drawworld->integer == 2) ? R_OcclusionShader() : surf->shader);
	if( shader->flags & SHADER_SKY )
	{
		qboolean vis = R_AddSkySurface( surf );
		if( vis )
		{
			R_AddMeshToList( MB_MODEL, surf->fog, shader, surf - r_worldbrushmodel->surfaces + 1,
				surf->mesh, surf->numVerts, surf->numElems );
			ri.params &= ~RP_NOSKY;
		}
		return NULL;
	}

	if( OCCLUSION_QUERIES_ENABLED( ri ) )
	{
		if( shader->flags & SHADER_PORTAL )
			R_SurfOcclusionQueryKey( ri.currententity, surf );
		if( OCCLUSION_OPAQUE_SHADER( shader ) )
			R_AddOccludingSurface( surf, shader );
	}

	c_brush_polys++;

	vbo = surf->vbo;
	if( vbo )
	{
		vbo_surf = ( msurface_t * )vbo->owner;

		mb = ri.meshlist->surfmbuffers[vbo_surf - r_worldbrushmodel->surfaces];
		if( mb )
		{
			// keep track of the actual vbo chunk we need to render
			if( surf->firstVBOVert < mb->firstVBOVert )
			{
				// prepend
				mb->numVerts = mb->numVerts + mb->firstVBOVert - surf->firstVBOVert;
				mb->numElems = mb->numElems + mb->firstVBOElem - surf->firstVBOElem;

				mb->firstVBOVert = surf->firstVBOVert;
				mb->firstVBOElem = surf->firstVBOElem;
			}
			else
			{
				// append
				mb->numVerts = max( mb->numVerts, surf->numVerts + surf->firstVBOVert - mb->firstVBOVert );
				mb->numElems = max( mb->numElems, surf->numElems + surf->firstVBOElem - mb->firstVBOElem );
			}

			ri.meshlist->surfmbuffers[surf - r_worldbrushmodel->surfaces] = mb;
			return mb;
		}
	}
	else
	{
		vbo_surf = NULL;
	}

	mb = R_AddMeshToList( surf->facetype == FACETYPE_FLARE ? MB_SPRITE : MB_MODEL,
		surf->fog, shader, surf - r_worldbrushmodel->surfaces + 1,
		surf->mesh, surf->numVerts, surf->numElems );

	if( mb )
	{
		mb->sortkey |= ( ( surf->superLightStyle+1 ) << 10 );

		if( vbo )
		{
			mb->vboIndex = vbo->index;
			mb->firstVBOVert = surf->firstVBOVert;
			mb->firstVBOElem = surf->firstVBOElem;
			mb->numVerts = surf->numVerts;
			mb->numElems = surf->numElems;

			ri.meshlist->surfmbuffers[vbo_surf - r_worldbrushmodel->surfaces] = mb;
			c_world_vbos++;
		}

		c_world_verts += surf->numVerts;
		c_world_tris += surf->numElems / 3;
	}

	ri.meshlist->surfmbuffers[surf - r_worldbrushmodel->surfaces] = mb;
	return mb;
}

/*
* R_AddBrushModelToList
*/
void R_AddBrushModelToList( entity_t *e )
{
	unsigned int i;
	qboolean rotated;
	model_t	*model = e->model;
	mbrushmodel_t *bmodel = ( mbrushmodel_t * )model->extradata;
	msurface_t *psurf;
	unsigned int dlightbits;
	meshbuffer_t *mb;

#ifdef HARDWARE_OUTLINES
	e->outlineHeight = r_worldent->outlineHeight;
	Vector4Copy( r_worldent->outlineRGBA, e->outlineColor );
#endif

	R_BrushModelBBox( e, modelmins, modelmaxs, &rotated );
	VectorSubtract( ri.refdef.vieworg, e->origin, modelorg );
	if( rotated )
	{
		vec3_t temp;

		VectorCopy( modelorg, temp );
		Matrix_TransformVector( e->axis, temp, modelorg );
	}

	dlightbits = 0;
	if( ( r_dynamiclight->integer == 1 ) && !r_fullbright->integer && !( ri.params & RP_SHADOWMAPVIEW ) )
	{
		for( i = 0; i < r_numDlights; i++ )
		{
			if( BoundsIntersect( modelmins, modelmaxs, r_dlights[i].mins, r_dlights[i].maxs ) )
				dlightbits |= ( 1<<i );
		}
	}

	for( i = 0, psurf = bmodel->firstmodelsurface; i < (unsigned)bmodel->nummodelsurfaces; i++, psurf++ )
	{
		if( !R_SurfPotentiallyVisible( psurf ) )
			continue;

		if( ri.params & RP_SHADOWMAPVIEW )
		{
			if( psurf->visframe != r_framecount )
				continue;
			if( ( psurf->shader->sort >= SHADER_SORT_OPAQUE ) && ( psurf->shader->sort <= SHADER_SORT_BANNER ) )
			{
				if( prevRI.meshlist->surfmbuffers[psurf - r_worldbrushmodel->surfaces] )
				{
					if( !R_CullSurface( psurf, 0 ) )
					{
						ri.params |= RP_WORLDSURFVISIBLE;
						prevRI.meshlist->surfmbuffers[psurf - r_worldbrushmodel->surfaces]->shadowbits |= ri.shadowGroup->bit;
					}
				}
			}
			continue;
		}

		psurf->visframe = r_framecount;
		mb = R_AddSurfaceToList( psurf, 0 );
		if( mb )
		{
			if( R_SurfPotentiallyLit( psurf ) )
				mb->dlightbits = dlightbits;
		}
	}
}

/*
=============================================================

WORLD MODEL

=============================================================
*/

/*
* R_MarkLeafSurfaces
*/
static void R_MarkLeafSurfaces( msurface_t **mark, unsigned int clipflags, unsigned int dlightbits )
{
	unsigned int newDlightbits;
	msurface_t *surf;
	meshbuffer_t *mb;

	do
	{
		surf = *mark++;

		// note that R_AddSurfaceToList may set meshBuffer to NULL
		// for world ALL surfaces to prevent referencing to freed memory region
		if( surf->visframe != r_framecount )
		{
			surf->visframe = r_framecount;
			mb = R_AddSurfaceToList( surf, clipflags );
		}
		else if( dlightbits )
		{
			mb = ri.meshlist->surfmbuffers[surf - r_worldbrushmodel->surfaces];
		}
		else
		{
			continue;
		}

		newDlightbits = mb ? dlightbits & ~mb->dlightbits : 0;
		if( newDlightbits && R_SurfPotentiallyLit( surf ) )
			mb->dlightbits |= R_AddSurfDlighbits( surf, newDlightbits );
	} while( *mark );
}

/*
* R_RecursiveWorldNode
*/
static void R_RecursiveWorldNode( mnode_t *node, unsigned int clipflags, unsigned int dlightbits )
{
	unsigned int i, newDlightbits;
	unsigned int bit;
	const cplane_t *clipplane;
	mleaf_t	*pleaf;

	while( 1 )
	{
		if( node->pvsframe != r_pvsframecount )
			return;

		if( clipflags )
		{
			for( i = sizeof( ri.frustum )/sizeof( ri.frustum[0] ), bit = 1, clipplane = ri.frustum; i > 0; i--, bit<<=1, clipplane++ )
			{
				if( clipflags & bit )
				{
					int clipped = BoxOnPlaneSide( node->mins, node->maxs, clipplane );
					if( clipped == 2 )
						return;
					if( clipped == 1 )
						clipflags &= ~bit; // node is entirely on screen
				}
			}
		}

		if( !node->plane )
			break;

		newDlightbits = 0;
		if( dlightbits )
		{
			float dist;

			for( i = 0, bit = 1; i < r_numDlights; i++, bit <<= 1 )
			{
				if( !( dlightbits & bit ) )
					continue;

				dist = PlaneDiff( r_dlights[i].origin, node->plane );
				if( dist < -r_dlights[i].intensity )
					dlightbits &= ~bit;
				if( dist < r_dlights[i].intensity )
					newDlightbits |= bit;
			}
		}

		R_RecursiveWorldNode( node->children[0], clipflags, dlightbits );

		node = node->children[1];
		dlightbits = newDlightbits;
	}

	// if a leaf node, draw stuff
	pleaf = ( mleaf_t * )node;
	pleaf->visframe = r_framecount;

	// add leaf bounds to view bounds
	for( i = 0; i < 3; i++ )
	{
		ri.visMins[i] = min( ri.visMins[i], pleaf->mins[i] );
		ri.visMaxs[i] = max( ri.visMaxs[i], pleaf->maxs[i] );
	}

	R_MarkLeafSurfaces( pleaf->firstVisSurface, clipflags, dlightbits );
	c_world_leafs++;
}

/*
* R_MarkShadowLeafSurfaces
*/
static void R_MarkShadowLeafSurfaces( msurface_t **mark, unsigned int clipflags )
{
	msurface_t *surf;
	meshbuffer_t *mb;
	const unsigned int bit = ri.shadowGroup->bit;

	do
	{
		surf = *mark++;
		if( surf->flags & ( SURF_NOIMPACT|SURF_NODRAW|SURF_SKY ) )
			continue;

		// this surface is visible in previous RI, not marked as shadowed...
		mb = prevRI.meshlist->surfmbuffers[surf - r_worldbrushmodel->surfaces];
		if( !mb || (mb->shadowbits & bit) )
			continue;

		// is opaque...
		if( ( surf->shader->sort >= SHADER_SORT_OPAQUE ) && ( surf->shader->sort <= SHADER_SORT_ALPHATEST ) )
		{
			if( !R_CullSurface( surf, clipflags ) )
			{
				int j;

				// add surface bounds to view bounds
				for( j = 0; j < 3; j++ )
				{
					ri.visMins[j] = min( ri.visMins[j], surf->mins[j] );
					ri.visMaxs[j] = max( ri.visMaxs[j], surf->maxs[j] );
				}

				// and is visible to the light source too
				ri.params |= RP_WORLDSURFVISIBLE;
				mb->shadowbits |= bit;
			}
		}
	} while( *mark );
}

/*
* R_LinearShadowLeafs
*/
static void R_LinearShadowLeafs( void )
{
	unsigned int i;
	unsigned int cpf, bit;
	const cplane_t *clipplane;
	mleaf_t	*leaf, **pleaf;

	for( pleaf = r_worldbrushmodel->visleafs, leaf = *pleaf; leaf; leaf = *pleaf++ )
	{
		if( leaf->visframe != r_framecount )
			continue;
		if( !( ri.shadowGroup->vis[leaf->cluster>>3] & ( 1<<( leaf->cluster&7 ) ) ) )
			continue;

		cpf = ri.clipFlags;
		for( i = sizeof( ri.frustum )/sizeof( ri.frustum[0] ), bit = 1, clipplane = ri.frustum; i > 0; i--, bit<<=1, clipplane++ )
		{
			int clipped = BoxOnPlaneSide( leaf->mins, leaf->maxs, clipplane );
			if( clipped == 2 )
				break;
			if( clipped == 1 )
				cpf &= ~bit;	// leaf is entirely on screen
		}

		if( !i )
		{
			R_MarkShadowLeafSurfaces( leaf->firstVisSurface, cpf );
			c_world_leafs++;
		}
	}
}

//==================================================================================

int r_surfQueryKeys[MAX_SURF_QUERIES];

/*
* R_ClearSurfOcclusionQueryKeys
*/
void R_ClearSurfOcclusionQueryKeys( void )
{
	memset( r_surfQueryKeys, -1, sizeof( r_surfQueryKeys ) );
}

/*
* R_SurfOcclusionQueryKey
*/
int R_SurfOcclusionQueryKey( entity_t *e, msurface_t *surf )
{
	int i;
	int *keys = r_surfQueryKeys;
	int key = surf - r_worldbrushmodel->surfaces;

	if( e != r_worldent )
		return -1;

	for( i = 0; i < MAX_SURF_QUERIES; i++ )
	{
		if( keys[i] >= 0 )
		{
			if( keys[i] == key )
				return i;
		}
		else
		{
			keys[i] = key;
			return i;
		}
	}

	return -1;
}

/*
* R_SurfIssueOcclusionQueries
*/
void R_SurfIssueOcclusionQueries( void )
{
	int i, *keys = r_surfQueryKeys;
	msurface_t *surf;

	for( i = 0; keys[i] >= 0; i++ )
	{
		surf = &r_worldbrushmodel->surfaces[keys[i]];
		R_IssueOcclusionQuery( R_GetOcclusionQueryNum( OQ_CUSTOM, i ), r_worldent, surf->mins, surf->maxs );
	}
}

//==================================================================================

/*
* R_CalcDistancesToFogVolumes
*/
static void R_CalcDistancesToFogVolumes( void )
{
	int i, j;
	float dist;
	const vec_t *v;
	mfog_t *fog;

	v = ri.viewOrigin;
	ri.fog_eye = NULL;

	for( i = 0, fog = r_worldbrushmodel->fogs; i < r_worldbrushmodel->numfogs; i++, fog++ ) {
		dist = PlaneDiff( v, fog->visibleplane );

		// determine the fog volume the viewer is inside
		if( dist < 0 ) {	
			for( j = 0; j < 3; j++ ) {
				if( v[j] >= fog->maxs[j] ) {
					break;
				}
				if( v[j] <= fog->mins[j] ) {
					break;
				}
			}
			if( j == 3 ) {
				ri.fog_eye = fog;
			}
		}

		ri.fog_dist_to_eye[i] = dist;
	}
}

/*
* R_DrawWorld
*/
void R_DrawWorld( void )
{
	int clipflags, msec = 0;
	unsigned int dlightbits;

	if( !r_drawworld->integer )
		return;
	if( !r_worldmodel )
		return;
	if( ri.refdef.rdflags & RDF_NOWORLDMODEL )
		return;

	VectorCopy( ri.refdef.vieworg, modelorg );

	ri.previousentity = NULL;
	ri.currententity = r_worldent;
	ri.currentmodel = ri.currententity->model;
#ifdef HARDWARE_OUTLINES
	if( (ri.refdef.rdflags & RDF_WORLDOUTLINES) && (r_viewcluster != -1) && r_outlines_scale->value > 0 )
		ri.currententity->outlineHeight = max( 0.0f, r_outlines_world->value );
	else
		ri.currententity->outlineHeight = 0;
	Vector4Copy( mapConfig.outlineColor, ri.currententity->outlineColor );
#endif

	if( !( ri.params & RP_SHADOWMAPVIEW ) )
	{
		memset( ri.meshlist->surfmbuffers, 0, r_worldbrushmodel->numsurfaces * sizeof( meshbuffer_t * ) );

		R_CalcDistancesToFogVolumes();
	}

	ClearBounds( ri.visMins, ri.visMaxs );

	R_ClearSky();

	if( r_nocull->integer )
		clipflags = 0;
	else
		clipflags = ri.clipFlags;

	if( r_dynamiclight->integer != 1 || r_fullbright->integer )
		dlightbits = 0;
	else
		dlightbits = r_numDlights < 32 ? ( 1 << r_numDlights ) - 1 : -1;

	if( r_speeds->integer )
		msec = Sys_Milliseconds();
	if( ri.params & RP_SHADOWMAPVIEW )
		R_LinearShadowLeafs ();
	else
		R_RecursiveWorldNode( r_worldbrushmodel->nodes, clipflags, dlightbits );
	if( r_speeds->integer )
		r_world_node += Sys_Milliseconds() - msec;
}

/*
* R_MarkLeaves
* 
* Mark the leaves and nodes that are in the PVS for the current cluster
*/
void R_MarkLeaves( void )
{
	qbyte *pvs;
	int i;
	int rdflags;
	mleaf_t	*leaf, **pleaf;
	mnode_t *node;
	qbyte *areabits;
	int cluster;
	qbyte fatpvs[MAX_MAP_LEAFS/8];

	rdflags = ri.refdef.rdflags;
	if( rdflags & RDF_NOWORLDMODEL )
		return;
	if( r_oldviewcluster == r_viewcluster && ( rdflags & RDF_OLDAREABITS ) && !(ri.params & RP_NOVIS) && r_viewcluster != -1 && r_oldviewcluster != -1 )
		return;
	if( ri.params & RP_SHADOWMAPVIEW )
		return;
	if( !r_worldmodel )
		return;

	// development aid to let you run around and see exactly where
	// the pvs ends
	if( r_lockpvs->integer )
		return;

	r_pvsframecount++;
	r_oldviewcluster = r_viewcluster;

	if( ri.params & RP_NOVIS || r_viewcluster == -1 || !r_worldbrushmodel->pvs )
	{
		// mark everything
		for( pleaf = r_worldbrushmodel->visleafs, leaf = *pleaf; leaf; leaf = *pleaf++ )
			leaf->pvsframe = r_pvsframecount;
		for( i = 0, node = r_worldbrushmodel->nodes; i < r_worldbrushmodel->numnodes; i++, node++ )
			node->pvsframe = r_pvsframecount;
		return;
	}

	pvs = Mod_ClusterPVS( r_viewcluster, r_worldmodel );
	if( r_viewarea > -1 && ri.refdef.areabits )
#ifdef AREAPORTALS_MATRIX
		areabits = ri.refdef.areabits + r_viewarea * ((r_worldbrushmodel->numareas+7)/8);
#else
		areabits = ri.refdef.areabits;
#endif
	else
		areabits = NULL;

	// may have to combine two clusters because of solid water boundaries
	if( mapConfig.checkWaterCrossing && ( rdflags & RDF_CROSSINGWATER ) )
	{
		int i, c;
		vec3_t pvsOrigin2;
		int viewcluster2;

		VectorCopy( ri.pvsOrigin, pvsOrigin2 );
		if( rdflags & RDF_UNDERWATER )
		{
			// look up a bit
			pvsOrigin2[2] += 9;
		}
		else
		{
			// look down a bit
			pvsOrigin2[2] -= 9;
		}

		leaf = Mod_PointInLeaf( pvsOrigin2, r_worldmodel );
		viewcluster2 = leaf->cluster;
		if( viewcluster2 > -1 && viewcluster2 != r_viewcluster && !( pvs[viewcluster2>>3] & ( 1<<( viewcluster2&7 ) ) ) )
		{
			memcpy( fatpvs, pvs, ( r_worldbrushmodel->pvs->numclusters + 7 ) / 8 ); // same as pvs->rowsize
			pvs = Mod_ClusterPVS( viewcluster2, r_worldmodel );
			c = ( r_worldbrushmodel->pvs->numclusters + 31 ) / 32;
			for( i = 0; i < c; i++ )
				(( int * )fatpvs)[i] |= (( int * )pvs)[i];
			pvs = fatpvs;
		}
	}

	for( pleaf = r_worldbrushmodel->visleafs, leaf = *pleaf; leaf; leaf = *pleaf++ )
	{
		cluster = leaf->cluster;

		// check for door connected areas
		if( areabits )
		{
			if( leaf->area < 0 || !( areabits[leaf->area>>3] & ( 1<<( leaf->area&7 ) ) ) )
				continue; // not visible
		}

		if( pvs[cluster>>3] & ( 1<<( cluster&7 ) ) )
		{
			node = (mnode_t *)leaf;
			do
			{
				if( node->pvsframe == r_pvsframecount )
					break;
				node->pvsframe = r_pvsframecount;
				node = node->parent;
			}
			while( node );
		}
	}
}
