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

// r_model.c -- model loading and caching

#include "r_local.h"

#ifdef QUAKE2_JUNK
void Mod_LoadAliasMD2Model( model_t *mod, model_t *parent, void *buffer, bspFormatDesc_t *unused );
#endif
void Mod_LoadAliasMD3Model( model_t *mod, model_t *parent, void *buffer, bspFormatDesc_t *unused );
#ifdef QUAKE2_JUNK
void Mod_LoadSpriteModel( model_t *mod, model_t *parent, void *buffer, bspFormatDesc_t *unused );
#endif
void Mod_LoadSkeletalModel( model_t *mod, model_t *parent, void *buffer, bspFormatDesc_t *unused );
void Mod_LoadQ1BrushModel( model_t *mod, model_t *parent, void *buffer, bspFormatDesc_t *format );
void Mod_LoadQ2BrushModel( model_t *mod, model_t *parent, void *buffer, bspFormatDesc_t *format );
void Mod_LoadQ3BrushModel( model_t *mod, model_t *parent, void *buffer, bspFormatDesc_t *format );

model_t *Mod_LoadModel( model_t *mod, qboolean crash );

model_t *mod_inline;
static qbyte mod_novis[MAX_MAP_LEAFS/8];

#define	MAX_MOD_KNOWN	512*4
static model_t mod_known[MAX_MOD_KNOWN];
static int mod_numknown;
static int modfilelen;

static mempool_t *mod_mempool;

static const modelFormatDescr_t mod_supportedformats[] =
{
#ifdef QUAKE2_JUNK
	// Quake2 .md2 models
	{ IDMD2HEADER, 4, NULL, MD3_ALIAS_MAX_LODS, ( const modelLoader_t )Mod_LoadAliasMD2Model },
#endif
	// Quake III Arena .md3 models
	{ IDMD3HEADER, 4, NULL, MD3_ALIAS_MAX_LODS, ( const modelLoader_t )Mod_LoadAliasMD3Model },
#ifdef QUAKE2_JUNK
	// Quake2 .sp2 sprites
	{ IDSP2HEADER, 4, NULL, 0, ( const modelLoader_t )Mod_LoadSpriteModel },
#endif
	// Skeletal models
	{ SKMHEADER, 4, NULL, SKM_MAX_LODS, ( const modelLoader_t )Mod_LoadSkeletalModel },

	// Q3-alike .bsp models
	{ "*", 4, q3BSPFormats, 0, ( const modelLoader_t )Mod_LoadQ3BrushModel },

	// Q2 .bsp models
	{ "*", 4, q2BSPFormats, 0, ( const modelLoader_t )Mod_LoadQ2BrushModel },

	// Q1 .bsp models
	{ "*", 0, q1BSPFormats, 0, ( const modelLoader_t )Mod_LoadQ1BrushModel },

	// trailing NULL
	{ NULL,	0, NULL, 0, NULL }
};

//===============================================================================

/*
===============
Mod_PointInLeaf
===============
*/
mleaf_t *Mod_PointInLeaf( vec3_t p, model_t *model )
{
	mnode_t	*node;
	cplane_t *plane;
	mbrushmodel_t *bmodel;

	if( !model || !( bmodel = ( mbrushmodel_t * )model->extradata ) || !bmodel->nodes )
	{
		Com_Error( ERR_DROP, "Mod_PointInLeaf: bad model" );
		return NULL;
	}

	node = bmodel->nodes;
	do
	{
		plane = node->plane;
		node = node->children[PlaneDiff( p, plane ) < 0];
	}
	while( node->plane != NULL );

	return ( mleaf_t * )node;
}

/*
==============
Mod_ClusterVS
==============
*/
static inline qbyte *Mod_ClusterVS( int cluster, dvis_t *vis )
{
	if( cluster < 0 || !vis )
		return mod_novis;
	return ( (qbyte *)vis->data + cluster*vis->rowsize );
}

/*
==============
Mod_ClusterPVS
==============
*/
qbyte *Mod_ClusterPVS( int cluster, model_t *model )
{
	return Mod_ClusterVS( cluster, (( mbrushmodel_t * )model->extradata)->pvs );
}

/*
==============
Mod_ClusterPHS
==============
*/
qbyte *Mod_ClusterPHS( int cluster, model_t *model )
{
	return Mod_ClusterVS( cluster, (( mbrushmodel_t * )model->extradata)->phs );
}

//===============================================================================

/*
=================
Mod_SetParent
=================
*/
static void Mod_SetParent( mnode_t *node, mnode_t *parent )
{
	node->parent = parent;
	if( !node->plane )
		return;
	Mod_SetParent( node->children[0], node );
	Mod_SetParent( node->children[1], node );
}

/*
=================
Mod_CreateVisLeafs
=================
*/
static void Mod_CreateVisLeafs( model_t *mod )
{
	int i;
	int count, numVisLeafs;
	int numVisSurfaces, numFragmentSurfaces;
	mleaf_t *leaf;
	msurface_t *surf, **mark;
	mbrushmodel_t *loadbmodel = (( mbrushmodel_t * )mod->extradata);

	count = loadbmodel->numleafs;
	loadbmodel->visleafs = Mod_Malloc( mod, ( count+1 )*sizeof( *loadbmodel->visleafs ) );
	memset( loadbmodel->visleafs, 0, ( count+1 )*sizeof( *loadbmodel->visleafs ) );

	numVisLeafs = 0;
	for( i = 0; i < count; i++ )
	{
		numVisSurfaces = numFragmentSurfaces = 0;

		leaf = loadbmodel->leafs + i;
		if( leaf->cluster < 0 || !leaf->firstVisSurface )
		{
			leaf->firstVisSurface = NULL;
			leaf->firstFragmentSurface = NULL;
			continue;
		}

		mark = leaf->firstVisSurface;
		do
		{
			surf = *mark++;

			if( R_SurfPotentiallyVisible( surf ) )
			{
				leaf->firstVisSurface[numVisSurfaces++] = surf;
				if( R_SurfPotentiallyFragmented( surf ) )
					leaf->firstFragmentSurface[numFragmentSurfaces++] = surf;
			}
		} while( *mark );

		if( numVisSurfaces )
			leaf->firstVisSurface[numVisSurfaces] = NULL;
		else
			leaf->firstVisSurface = NULL;

		if( numFragmentSurfaces )
			leaf->firstFragmentSurface[numFragmentSurfaces] = NULL;
		else
			leaf->firstFragmentSurface = NULL;

		if( !numVisSurfaces )
		{
			//out->cluster = -1;
			continue;
		}

		loadbmodel->visleafs[numVisLeafs++] = leaf;
	}

	loadbmodel->visleafs[numVisLeafs] = NULL;
}

/*
=================
Mod_FinishFaces
=================
*/
static void Mod_FinishFaces( model_t *mod )
{
	int i, j;
	mbrushmodel_t *loadbmodel = (( mbrushmodel_t * )mod->extradata);

	for( i = 0; i < loadbmodel->numsurfaces; i++ )
	{
		vec_t *vert;
		msurface_t *surf = loadbmodel->surfaces + i;
		mesh_t *mesh = surf->mesh;
		vec3_t ebbox = { 0, 0, 0 };

		if( !mesh || R_InvalidMesh( mesh ) )
			continue;

		ClearBounds( surf->mins, surf->maxs );
		for( j = 0, vert = mesh->xyzArray[0]; j < mesh->numVerts; j++, vert += 4 )
			AddPointToBounds( vert, surf->mins, surf->maxs );
		VectorSubtract( surf->mins, ebbox, surf->mins );
		VectorAdd( surf->maxs, ebbox, surf->maxs );

		// store mesh information in surface struct for faster access
		surf->numVertexes = mesh->numVerts;
		surf->numElems = mesh->numElems;
	}
}

/*
=================
Mod_SetupSubmodels
=================
*/
static void Mod_SetupSubmodels( model_t *mod )
{
	int i;
	mmodel_t *bm;
	model_t	*starmod;
	mbrushmodel_t *bmodel;
	mbrushmodel_t *loadbmodel = (( mbrushmodel_t * )mod->extradata);

	// set up the submodels
	for( i = 0; i < loadbmodel->numsubmodels; i++ )
	{
		bm = &loadbmodel->submodels[i];
		starmod = &mod_inline[i];
		bmodel = ( mbrushmodel_t * )starmod->extradata;

		memcpy( starmod, mod, sizeof( model_t ) );
		memcpy( bmodel, mod->extradata, sizeof( mbrushmodel_t ) );

		bmodel->firstmodelsurface = bmodel->surfaces + bm->firstface;
		bmodel->nummodelsurfaces = bm->numfaces;
		starmod->extradata = bmodel;

		VectorCopy( bm->maxs, starmod->maxs );
		VectorCopy( bm->mins, starmod->mins );
		starmod->radius = bm->radius;

		if( i == 0 )
			*mod = *starmod;
		else
			bmodel->numsubmodels = 0;
	}
}

#ifdef PUBLIC_BUILD
#define VBO_Printf Com_DPrintf
#else
#define VBO_Printf Com_Printf
#endif

/*
* Mod_CreateSubmodelBufferObjects
*/
static int Mod_CreateSubmodelBufferObjects( model_t *mod, int modnum, size_t *vbo_total_size )
{
	int i, j, k;
	qbyte *visdata = NULL;
	qbyte *areadata = NULL;
	int rowbytes, rowlongs;
	int areabytes;
	qbyte *arearow;
	int *longrow, *longrow2;
	mmodel_t *bm;
	mbrushmodel_t *loadbmodel;
	msurface_t *surf, *surf2, **mark;
	msurface_t **surfmap;
	int num_vbos;

	assert( mod );

	loadbmodel = (( mbrushmodel_t * )mod->extradata);
	assert( loadbmodel );

	assert( modnum >= 0 && modnum < loadbmodel->numsubmodels );
	bm = loadbmodel->submodels + modnum;

	// ignore empty models
	if( !bm->numfaces )
		return 0;

	// PVS only exists for world submodel
    if( !modnum && loadbmodel->pvs )
    {
	    mleaf_t *leaf, **pleaf;

		rowbytes = loadbmodel->pvs->rowsize;
		rowlongs = (rowbytes + 3) / 4;
		areabytes = (loadbmodel->numareas + 7) / 8;

		if( !rowbytes )
			return 0;

		// build visibility data for each face, based on what leafs
		// this face belongs to (visible from)
		visdata = Mod_Malloc( mod, rowbytes * loadbmodel->numsurfaces );
		areadata = Mod_Malloc( mod, areabytes * loadbmodel->numsurfaces );
		for( pleaf = loadbmodel->visleafs, leaf = *pleaf; leaf; leaf = *pleaf++ )
		{
			mark = leaf->firstVisSurface;
			do
			{
				int surfnum;

				surf = *mark;

				surfnum = surf - loadbmodel->surfaces;
				longrow  = ( int * )( visdata + surfnum * rowbytes );
				longrow2 = ( int * )( Mod_ClusterPVS( leaf->cluster, mod ) );

				// merge parent leaf cluster visibility into face visibility set
				// we could probably check for duplicates here because face can be
				// shared among multiple leafs
				for( j = 0; j < rowlongs; j++ )
					longrow[j] |= longrow2[j];

				if( leaf->area >= 0 ) {
					arearow = areadata + surfnum * areabytes;
					arearow[leaf->area>>3] |= (1<<(leaf->area&7));
				}
			} while( *++mark );
		}
    }
    else
    {
    	// either a submodel or an unvised map
		rowbytes = 0;
		rowlongs = 0;
		visdata = NULL;
		areabytes = 0;
		areadata = NULL;
    }

	// now linearly scan all faces for this submodel, merging them into
	// vertex buffer objects if they share shader, lightmap texture and we can render
	// them in hardware (some Q3A shaders require GLSL for that)

	surfmap = Mod_Malloc( mod, bm->numfaces * sizeof( *surfmap ) );

	num_vbos = 0;
	*vbo_total_size = 0;
	for( i = 0, surf = loadbmodel->surfaces + bm->firstface; i < bm->numfaces; i++, surf++ )
	{
		mesh_vbo_t *vbo;
		mesh_t *mesh, *mesh2;
		shader_t *shader;
		int fcount;
		int vcount, ecount;

		// ignore faces already merged
		if( surfmap[i] )
			continue;

		// ignore invisible faces and flares
		if( !R_SurfPotentiallyVisible( surf ) || surf->facetype == FACETYPE_FLARE )
			continue;

		shader = surf->shader;

		// skies and portals can not be merged either
		if( shader->flags & (SHADER_SKY|SHADER_PORTAL|SHADER_PORTAL_CAPTURE|SHADER_PORTAL_CAPTURE2) )
			continue;

		// we use hardware transforms to render this shader for whatever reasons
		if( !(shader->features & MF_HARDWARE) )
			continue;

		// lightstyled faces with vertex lighting enabled can not be done in hardware
		// despite shader saying otherwise
		if( r_lighting_vertexlight->integer && ( r_superLightStyles[surf->superLightStyle].features & MF_COLORS1 ) )
			continue;

		// hardware fog requires GLSL support
		if( surf->fog && !glConfig.ext.GLSL )
			continue;

		longrow  = ( int * )( visdata + i * rowbytes );
		arearow = areadata + i * areabytes;

		fcount = 1;
		mesh = surf->mesh;
		vcount = mesh->numVerts;
		ecount = mesh->numElems;

		// if the shader explicitly says we can not using batch, so be it
		if( !(surf->shader->features & MF_NONBATCHED) )
		{
			// scan remaining face checking whether we merge them with the current one
			for( j = 0, surf2 = loadbmodel->surfaces + bm->firstface + j; j < bm->numfaces; j++, surf2++ )
			{
				if( i == j )
					continue;

				// already merged
				if( surf2->vbo )
					continue;

				// the following checks ensure the two faces are compatible can can be merged
				// into a single vertex buffer object
				if( !R_SurfPotentiallyVisible( surf2 ) || surf2->facetype == FACETYPE_FLARE )
					continue;
				if( surf2->shader != surf->shader || surf2->superLightStyle != surf->superLightStyle )
					continue;
				if( surf2->fog != surf->fog )
					continue;

				// unvised maps and submodels submodel can simply skip PVS checks
				if( !visdata )
					goto merge;

				// only merge faces that reside in same map areas
				if( areabytes > 0 ) {
					// if areabits aren't equal, faces have different area visibility
					if( memcmp( arearow, areadata + j * areabytes, areabytes ) ) {
						continue;
					}
				}

				// if two faces potentially see same things, we can merge them
				longrow2 = ( int * )( visdata + j * rowbytes );
				for( k = 0; k < rowlongs && !(longrow[k] & longrow2[k]); k++ );

				if( k != rowlongs )
				{
					// merge visibility sets
					for( k = 0; k < rowlongs; k++ )
						longrow[k] |= longrow2[k];
merge:
					fcount++;
					mesh2 = surf2->mesh;
					vcount += mesh2->numVerts;
					ecount += mesh2->numElems;
					surfmap[j] = surf;
				}
			}
		}

		// create vertex buffer object for this face then upload data
		vbo = R_CreateStaticMeshVBO( ( void * )surf, vcount, ecount, shader->features | r_superLightStyles[surf->superLightStyle].features );
		if( vbo )
		{
			int errArr;

			// upload vertex and elements data for face itself
			surf->vbo = vbo;
			errArr = R_UploadVBOVertexData( vbo, 0, surf->mesh );
			R_UploadVBOElemData( vbo, 0, 0, surf->mesh );

			vcount = mesh->numVerts;
			ecount = mesh->numElems;

			// now if there are any merged faces upload them to the same VBO
			if( fcount > 1 )
			{
				for( j = 0, surf2 = loadbmodel->surfaces + bm->firstface + j; j < bm->numfaces; j++, surf2++ )
				{
					if( surfmap[j] != surf )
						continue;

					mesh2 = surf2->mesh;

					surf2->vbo = vbo;
					errArr |= R_UploadVBOVertexData( vbo, vcount, mesh2 );
					R_UploadVBOElemData( vbo, vcount, ecount, mesh2 );

					vcount += mesh2->numVerts;
					ecount += mesh2->numElems;
				}
			}

			// now if we have detected any errors, let the developer know.. usually they indicate
			// either an unintentionally missing lightmap
			if( errArr )
			{
				VBO_Printf( S_COLOR_YELLOW "WARNING: Missing arrays for surface %s:", shader->name );
				if( errArr & MF_NORMALS )
					VBO_Printf( " norms" );
				if( errArr & MF_SVECTORS )
					VBO_Printf( " svecs" );
				if( errArr & MF_STCOORDS )
					VBO_Printf( " st" );
				if( errArr & MF_LMCOORDS )
					VBO_Printf( " lmst" );
				if( errArr & MF_COLORS )
					VBO_Printf( " colors" );
				VBO_Printf( "\n" );
			}

			num_vbos++;
			*vbo_total_size += vbo->size;
		}
	}

	Mem_Free( surfmap );

	if( visdata )
		Mem_Free( visdata );
	if( areadata )
		Mem_Free( areadata );

	return num_vbos;
}

/*
* Mod_CreateVertexBufferObjects
*/
void Mod_CreateVertexBufferObjects( model_t *mod )
{
	int i;
	int vbos = 0, total = 0;
	size_t size = 0, total_size = 0;
	mbrushmodel_t *loadbmodel = (( mbrushmodel_t * )mod->extradata);

	if( !glConfig.ext.vertex_buffer_object )
		return;

	for( i = 0; i < loadbmodel->numsubmodels; i++ )
	{
		vbos = Mod_CreateSubmodelBufferObjects( mod, i, &size );
		total += vbos;
		total_size += size;
	}

	if( total )
		VBO_Printf( "Created %i VBOs, totalling %.1f MiB of memory\n", total, (total_size + 1048574) / 1048576.0f );
}

/*
=================
Mod_FinalizeBrushModel
=================
*/
static void Mod_FinalizeBrushModel( model_t *model )
{
	Mod_FinishFaces( model );

	Mod_CreateVisLeafs( model );

	Mod_SetupSubmodels( model );

	Mod_SetParent( (( mbrushmodel_t * )model->extradata)->nodes, NULL );
}

//===============================================================================

/*
================
Mod_Modellist_f
================
*/
void Mod_Modellist_f( void )
{
	int i;
	model_t	*mod;
	int total;

	total = 0;
	Com_Printf( "Loaded models:\n" );
	for( i = 0, mod = mod_known; i < mod_numknown; i++, mod++ )
	{
		if( !mod->name )
			break;
		Com_Printf( "%8i : %s\n", mod->mempool->totalsize, mod->name );
		total += mod->mempool->totalsize;
	}
	Com_Printf( "Total: %i\n", mod_numknown );
	Com_Printf( "Total resident: %i\n", total );
}

/*
===============
R_InitModels
===============
*/
void R_InitModels( void )
{
	mod_mempool = Mem_AllocPool( NULL, "Models" );
	memset( mod_novis, 0xff, sizeof( mod_novis ) );
}

/*
================
R_ShutdownModels
================
*/
void R_ShutdownModels( void )
{
	int i;

	if( !mod_mempool )
		return;

	if( mod_inline )
	{
		Mem_Free( mod_inline );
		mod_inline = NULL;
	}

	for( i = 0; i < mod_numknown; i++ )
	{
		if( mod_known[i].mempool )
			Mem_FreePool( &mod_known[i].mempool );
	}

	r_worldmodel = NULL;
	r_worldbrushmodel = NULL;

	mod_numknown = 0;
	memset( mod_known, 0, sizeof( mod_known ) );

	Mem_FreePool( &mod_mempool );
}

/*
=================
Mod_StripLODSuffix
=================
*/
void Mod_StripLODSuffix( char *name )
{
	int len, lodnum;

	len = strlen( name );
	if( len <= 2 )
		return;

	lodnum = atoi( &name[len - 1] );
	if( lodnum < MD3_ALIAS_MAX_LODS )
	{
		if( name[len-2] == '_' )
			name[len-2] = 0;
	}
}

/*
==================
Mod_FindSlot
==================
*/
static model_t *Mod_FindSlot( const char *name, const char *shortname )
{
	int i;
	model_t	*mod, *best;
	size_t shortlen = shortname ? strlen( shortname ) : 0;

	//
	// search the currently loaded models
	//
	for( i = 0, mod = mod_known, best = NULL; i < mod_numknown; i++, mod++ )
	{
		if( !Q_stricmp( mod->name, name ) )
			return mod;

		if( ( mod->type == mod_bad ) && shortlen )
		{
			if( !Q_strnicmp( mod->name, shortname, shortlen ) )
			{                                               // same basename, different extension
				best = mod;
				shortlen = 0;
			}
		}
	}

	//
	// return best candidate
	//
	if( best )
		return best;

	//
	// find a free model slot spot
	//
	if( mod_numknown == MAX_MOD_KNOWN )
		Com_Error( ERR_DROP, "mod_numknown == MAX_MOD_KNOWN" );
	return &mod_known[mod_numknown];
}

/*
==================
Mod_Handle
==================
*/
unsigned int Mod_Handle( model_t *mod )
{
	return mod - mod_known;
}

/*
==================
Mod_ForHandle
==================
*/
model_t *Mod_ForHandle( unsigned int elem )
{
	return mod_known + elem;
}

/*
==================
Mod_ForName

Loads in a model for the given name
==================
*/
model_t *Mod_ForName( const char *name, qboolean crash )
{
	int i;
	model_t	*mod, *lod;
	unsigned *buf;
	char shortname[MAX_QPATH], lodname[MAX_QPATH];
	const char *extension;
	const modelFormatDescr_t *descr;
	bspFormatDesc_t *bspFormat = NULL;
	qbyte stack[0x4000];

	if( !name[0] )
		Com_Error( ERR_DROP, "Mod_ForName: NULL name" );

	//
	// inline models are grabbed only from worldmodel
	//
	if( name[0] == '*' )
	{
		i = atoi( name+1 );
		if( i < 1 || !r_worldmodel || i >= r_worldbrushmodel->numsubmodels )
			Com_Error( ERR_DROP, "bad inline model number" );
		return &mod_inline[i];
	}

	Q_strncpyz( shortname, name, sizeof( shortname ) );
	COM_StripExtension( shortname );
	extension = &name[strlen( shortname )+1];

	mod = Mod_FindSlot( name, shortname );
	if( mod->name && !strcmp( mod->name, name ) )
		return mod->type != mod_bad ? mod : NULL;

	//
	// load the file
	//
	modfilelen = FS_LoadFile( name, (void **)&buf, stack, sizeof( stack ) );
	if( !buf && crash )
		Com_Error( ERR_DROP, "Mod_NumForName: %s not found", name );

	if( mod->mempool )  // overwrite
		Mem_FreePool( &mod->mempool );
	else
		mod_numknown++;

	mod->type = mod_bad;
	mod->mempool = Mem_AllocPool( mod_mempool, name );
	mod->name = Mod_Malloc( mod, strlen( name ) + 1 );
	strcpy( mod->name, name );

	// return the NULL model
	if( !buf )
		return NULL;

	// call the apropriate loader
	descr = Com_FindFormatDescriptor( mod_supportedformats, ( const qbyte * )buf, (const bspFormatDesc_t **)&bspFormat );
	if( !descr )
	{
		Com_DPrintf( S_COLOR_YELLOW "Mod_NumForName: unknown fileid for %s", mod->name );
		return NULL;
	}

	descr->loader( mod, NULL, buf, bspFormat );
	if( ( qbyte *)buf != stack )
		FS_FreeFile( buf );

	// do some common things
	if( mod->type == mod_brush )
		Mod_FinalizeBrushModel( mod );

	if( !descr->maxLods )
		return mod;

	//
	// load level-of-detail models
	//
	mod->numlods = 0;
	for( i = 0; i < descr->maxLods; i++ )
	{
		Q_snprintfz( lodname, sizeof( lodname ), "%s_%i.%s", shortname, i+1, extension );
		FS_LoadFile( lodname, (void **)&buf, stack, sizeof( stack ) );
		if( !buf || strncmp( (const char *)buf, descr->header, descr->headerLen ) )
			break;

		lod = mod->lods[i] = Mod_FindSlot( lodname, NULL );
		if( lod->name && !strcmp( lod->name, lodname ) )
			continue;

		lod->type = mod_bad;
		lod->mempool = Mem_AllocPool( mod_mempool, lodname );
		lod->name = Mod_Malloc( lod, strlen( lodname ) + 1 );
		strcpy( lod->name, lodname );

		mod_numknown++;

		descr->loader( lod, mod, buf, bspFormat );
		if( (qbyte *)buf != stack )
			FS_FreeFile( buf );

		mod->numlods++;
	}

	return mod;
}

#ifdef QUAKE2_JUNK

/*
==============================================================================

SPRITE MODELS

==============================================================================
*/

/*
=================
Mod_LoadSpriteModel
=================
*/
void Mod_LoadSpriteModel( model_t *mod, model_t *parent, void *buffer, bspFormatDesc_t *unused )
{
	int i;
	dsprite_t *sprin;
	smodel_t *sprout;
	dsprframe_t *sprinframe;
	sframe_t *sproutframe;

	sprin = (dsprite_t *)buffer;

	if( LittleLong( sprin->version ) != SPRITE_VERSION )
		Com_Error( ERR_DROP, "%s has wrong version number (%i should be %i)",
		mod->name, LittleLong( sprin->version ), SPRITE_VERSION );

	mod->extradata = sprout = Mod_Malloc( mod, sizeof( smodel_t ) );
	sprout->numframes = LittleLong( sprin->numframes );

	sprinframe = sprin->frames;
	sprout->frames = sproutframe = Mod_Malloc( mod, sizeof( sframe_t ) * sprout->numframes );

	mod->radius = 0;
	ClearBounds( mod->mins, mod->maxs );

	// byte swap everything
	for( i = 0; i < sprout->numframes; i++, sprinframe++, sproutframe++ )
	{
		sproutframe->width = LittleLong( sprinframe->width );
		sproutframe->height = LittleLong( sprinframe->height );
		sproutframe->origin_x = LittleLong( sprinframe->origin_x );
		sproutframe->origin_y = LittleLong( sprinframe->origin_y );
		sproutframe->shader = R_RegisterPic( sprinframe->name );
		sproutframe->radius = sqrt( sproutframe->width * sproutframe->width + sproutframe->height * sproutframe->height );
		mod->radius = max( mod->radius, sproutframe->radius );
	}

	mod->type = mod_sprite;
}

#endif

//=============================================================================

/*
=================
R_RegisterWorldModel

Specifies the model that will be used as the world
=================
*/
void R_RegisterWorldModel( const char *model, const dvis_t *pvsData, const dvis_t *phsData )
{
	memset( &mapConfig, 0, sizeof( mapConfig ) );

	mapConfig.pow2MapOvrbr = 0;
	mapConfig.lightmapsPacking = qfalse;
	mapConfig.deluxeMaps = qfalse;
	mapConfig.deluxeMappingEnabled = qfalse;
	mapConfig.overbrightBits = max( 0, r_mapoverbrightbits->integer );
	mapConfig.checkWaterCrossing = qfalse;
	mapConfig.depthWritingSky = qfalse;
	mapConfig.polygonOffsetSubmodels = qfalse;
	mapConfig.forceClear = qfalse;
	mapConfig.lightingIntensity = 0;

	VectorClear( mapConfig.ambient );
#ifdef HARDWARE_OUTLINES
	VectorClear( mapConfig.outlineColor );
#endif

	if( r_lighting_packlightmaps->integer )
	{
		char lightmapsPath[MAX_QPATH], *p;

		mapConfig.lightmapsPacking = qtrue;

		Q_strncpyz( lightmapsPath, model, sizeof( lightmapsPath ) );
		p = strrchr( lightmapsPath, '.' );
		if( p )
		{
			*p = 0;
			Q_strncatz( lightmapsPath, "/lm_0000.tga", sizeof( lightmapsPath ) );
			if( FS_FOpenFile( lightmapsPath, NULL, FS_READ ) != -1 )
			{
				Com_DPrintf( S_COLOR_YELLOW "External lightmap stage: lightmaps packing is disabled\n" );
				mapConfig.lightmapsPacking = qfalse;
			}
		}
	}

	r_farclip_min = Z_NEAR; // sky shaders will most likely modify this value
	r_environment_color->modified = qtrue;

	r_worldmodel = Mod_ForName( model, qtrue );
	r_worldbrushmodel = ( mbrushmodel_t * )r_worldmodel->extradata;
	r_worldbrushmodel->pvs = ( dvis_t * )pvsData;
	r_worldbrushmodel->phs = ( dvis_t * )phsData;

	r_worldent->scale = 1.0f;
	r_worldent->model = r_worldmodel;
	r_worldent->rtype = RT_MODEL;
	Matrix_Identity( r_worldent->axis );

	// ambient lighting
	if( r_fullbright->integer )
	{
		VectorSet( mapConfig.ambient, 1, 1, 1 );
	}
	else
	{
		int i;
		float scale;

		scale = (float)( 1 << mapConfig.pow2MapOvrbr ) * (mapConfig.lightingIntensity ? mapConfig.lightingIntensity : 1.0) / 255.0f;
		if( mapConfig.lightingIntensity )
		{
			for( i = 0; i < 3; i++ )
				mapConfig.ambient[i] = mapConfig.ambient[i]* scale;
		}
		else
		{
			for( i = 0; i < 3; i++ )
				mapConfig.ambient[i] = bound( 0, mapConfig.ambient[i] * scale, 1 );
		}
	}

	r_framecount = 1;

	r_oldviewcluster = r_viewcluster = -1;  // force markleafs

	R_AllocWorldMeshLists ();

	Mod_CreateVertexBufferObjects( r_worldmodel );
}

/*
=================
R_RegisterModel
=================
*/
struct model_s *R_RegisterModel( const char *name )
{
	return Mod_ForName( name, qfalse );
}

/*
=================
R_ModelBounds
=================
*/
void R_ModelBounds( const model_t *model, vec3_t mins, vec3_t maxs )
{
	if( model )
	{
		VectorCopy( model->mins, mins );
		VectorCopy( model->maxs, maxs );
	}
	else if( r_worldmodel )
	{
		VectorCopy( r_worldmodel->mins, mins );
		VectorCopy( r_worldmodel->maxs, maxs );
	}
}

//==================================================================================

/*
=================
R_ExportSkm2Iqe

IQE format export according to http://lee.fov120.com/iqm/iqe.txt
=================
*/
static void R_ExportSkmToIqe( model_t *mod )
{
	unsigned int i;
	unsigned int j, k;
	int filenum;
	char buf[4096];
	char *checkname;
	size_t checkname_size;
	mskmodel_t *skmodel;
	mskbone_t *bone;
	mskmesh_t *mesh;
	bonepose_t *bp;

	assert( mod );
	assert( mod->type == mod_skeletal );

	if( !mod )
		return;

	if( mod->type != mod_skeletal )
	{
		Com_Printf( "R_ExportSkmToIqe: bad model type %i for '%s'\n", mod->type, mod->name );
		return;
	}

	checkname_size = strlen( "iqe/" ) + strlen( mod->name ) + 4;
	checkname = Mem_TempMalloc( checkname_size );

	Q_snprintfz( checkname, checkname_size, "iqe/%s", mod->name );
	COM_ReplaceExtension( checkname, ".iqe", checkname_size );

	FS_FOpenFile( checkname, &filenum, FS_WRITE );
	if( !filenum )
	{
		Com_Printf( "R_ExportSkmToIqe: failed opening '%s' for writing\n", checkname );
		Mem_Free( checkname );
		return;
	}

	// start
	Q_snprintfz( buf, sizeof( buf ), "# Inter-Quake Export\n" );
	FS_Write( buf, strlen( buf ), filenum );


	// joints
	Q_snprintfz( buf, sizeof( buf ), "\n" );
	FS_Write( buf, strlen( buf ), filenum );

	skmodel = ( mskmodel_t * )mod->extradata;
	for( i = 0, bone = skmodel->bones; i < skmodel->numbones; i++, bone++ )
	{
		bp = &skmodel->frames[0].boneposes[i];

		Q_snprintfz( buf, sizeof( buf ), 
			"joint \"%s\" %i\n"
			"\tpq %f %f %f %f %f %f %f\n", 
				bone->name, bone->parent,
				bp->origin[0], bp->origin[1], bp->origin[2], bp->quat[0], bp->quat[1], bp->quat[2], bp->quat[3] );
		FS_Write( buf, strlen( buf ), filenum );
	}


	// meshes
	for( i = 0, mesh = skmodel->meshes; i < skmodel->nummeshes; i++, mesh++ )
	{
		unsigned int *bones;
		float *influences;

		Q_snprintfz( buf, sizeof( buf ), "\n" );
		FS_Write( buf, strlen( buf ), filenum );

		Q_snprintfz( buf, sizeof( buf ), 
			"mesh \"%s\"\n" 
			"\tmaterial \"%s\"\n",
			mesh->name, mesh->skin.shader->name );
		FS_Write( buf, strlen( buf ), filenum );

		// vertices
		Q_snprintfz( buf, sizeof( buf ), "\n" );
		FS_Write( buf, strlen( buf ), filenum );

		bones = mesh->bones;
		influences = mesh->influences;
		for( j = 0; j < mesh->numverts; j++ )
		{
			// vertex position, texture coordinates and normal
			Q_snprintfz( buf, sizeof( buf ), 
				"vp %f %f %f\n"
				"\tvt %f %f\n"
				"\tvn %f %f %f\n",
				mesh->xyzArray[j][0], mesh->xyzArray[j][1], mesh->xyzArray[j][2],
				mesh->stArray[j][0], mesh->stArray[j][1],
				mesh->normalsArray[j][0], mesh->normalsArray[j][1], mesh->normalsArray[j][2]
			);
			FS_Write( buf, strlen( buf ), filenum );

			// influences
			Q_snprintfz( buf, sizeof( buf ), "\tvb" );

			for( k = 0; k < SKM_MAX_WEIGHTS; k++ )
			{
				if( !influences[k] )
					break;
				Q_strncatz( buf, va( " %i %f", bones[k], influences[k] ), sizeof( buf ) );
			}
			Q_strncatz( buf, "\n", sizeof( buf ) );
			FS_Write( buf, strlen( buf ), filenum );

			bones += SKM_MAX_WEIGHTS;
			influences += SKM_MAX_WEIGHTS;
		}

		// triangles
		Q_snprintfz( buf, sizeof( buf ), "\n" );
		FS_Write( buf, strlen( buf ), filenum );

		for( j = 0; j < mesh->numtris; j++ )
		{
			Q_snprintfz( buf, sizeof( buf ), 
				"fm %i %i %i\n",
				mesh->elems[j*3+0], mesh->elems[j*3+1], mesh->elems[j*3+2] );
			FS_Write( buf, strlen( buf ), filenum );
		}
	}


	// animation
	Q_snprintfz( buf, sizeof( buf ), "\n" );
	FS_Write( buf, strlen( buf ), filenum );

	Q_snprintfz( buf, sizeof( buf ), 
		"animation\n"
		"\tframerate 0\n" );
	FS_Write( buf, strlen( buf ), filenum );

	// frames
	for( i = 0; i < skmodel->numframes; i++ )
	{
		Q_snprintfz( buf, sizeof( buf ), "\n" );
		FS_Write( buf, strlen( buf ), filenum );

		Q_snprintfz( buf, sizeof( buf ), 
			"frame\n"
			"#%s\n",
			skmodel->frames[i].name );
		FS_Write( buf, strlen( buf ), filenum );

		for( j = 0, bp = skmodel->frames[i].boneposes; j < skmodel->numbones; j++, bp++ )
		{
			Q_snprintfz( buf, sizeof( buf ), "pq %f %f %f %f %f %f %f\n", 
				bp->origin[0], bp->origin[1], bp->origin[2], bp->quat[0], bp->quat[1], bp->quat[2], bp->quat[3] );
			FS_Write( buf, strlen( buf ), filenum );
		}
	}


	// end
	FS_FCloseFile( filenum );

	Com_Printf( "R_ExportSkmToIqe: wrote '%s'\n", checkname );

	Mem_Free( checkname );
}

/*
=================
R_Skm2Iqe_f
=================
*/
void R_Skm2Iqe_f( void )
{
	model_t *mod;
	const char *mod_name;

	mod_name = Cmd_Argv( 1 );
	if( !strcmp( mod_name, "*" ) )
	{
		int i;

		for( i = 0, mod = mod_known; i < mod_numknown; i++, mod++ )
		{
			if( !mod->name )
				break;
			if( mod->type != mod_skeletal )
				continue;

			R_ExportSkmToIqe( mod );
		}
	}
	else
	{
		mod = Mod_ForName( mod_name, qfalse );
		if( !mod )
		{
			Com_Printf( "R_Skm2Iqe_f: no such model '%s'\n", mod_name );
			return;
		}

		R_ExportSkmToIqe( mod );
	}
}
