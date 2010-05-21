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
Mod_CalcBBoxesForFaces
=================
*/
static void Mod_CalcBBoxesForFaces( model_t *mod )
{
	int i, j;
	mbrushmodel_t *loadbmodel = (( mbrushmodel_t * )mod->extradata);

	for( i = 0; i < loadbmodel->numsurfaces; i++ )
	{
		vec_t *vert;
		msurface_t *surf = loadbmodel->surfaces + i;
		mesh_t *mesh = surf->mesh;
		vec3_t ebbox = { 0, 0, 0 };

		if( !mesh )
			continue;

		ClearBounds( surf->mins, surf->maxs );
		for( j = 0, vert = mesh->xyzArray[0]; j < mesh->numVertexes; j++, vert += 4 )
			AddPointToBounds( vert, surf->mins, surf->maxs );
		VectorSubtract( surf->mins, ebbox, surf->mins );
		VectorAdd( surf->maxs, ebbox, surf->maxs );
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

/*
=================
Mod_FinalizeBrushModel
=================
*/
static void Mod_FinalizeBrushModel( model_t *model )
{
	Mod_CreateVisLeafs( model );

	Mod_CalcBBoxesForFaces( model );

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
		for( i = 0; i < 3; i++ )
			mapConfig.ambient[i] = bound( 0, mapConfig.ambient[i]*( (float)( 1 << mapConfig.pow2MapOvrbr )/255.0f ), 1 );
	}

	r_framecount = 1;

	r_oldviewcluster = r_viewcluster = -1;  // force markleafs

	R_AllocWorldMeshLists ();
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
