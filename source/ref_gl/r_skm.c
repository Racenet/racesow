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

// r_skm.c: skeletal animation model format

#include "r_local.h"

static mesh_t skm_mesh;

static vec3_t skm_mins;
static vec3_t skm_maxs;
static float skm_radius;

/*
==============================================================================

SKM MODELS

==============================================================================
*/

/*
=================
Mod_LoadSkeletalPose
=================
*/
void Mod_LoadSkeletalPose( char *name, model_t *mod, void *buffer )
{
	unsigned int i, j, k;
	mskmodel_t *poutmodel;
	dskpheader_t *pinmodel;
	dskpbone_t *pinbone;
	mskbone_t *poutbone;
	dskpframe_t *pinframe;
	mskframe_t *poutframe;
	dskpbonepose_t *pinbonepose;
	bonepose_t *poutbonepose;
	qbyte *membuffer;
	size_t memBufferSize;

	if( strncmp( (const char *)buffer, SKMHEADER, 4 ) )
		Com_Error( ERR_DROP, "uknown fileid for %s", name );

	pinmodel = ( dskpheader_t * )buffer;
	poutmodel = ( mskmodel_t * )mod->extradata;

	if( LittleLong( pinmodel->type ) != SKM_MODELTYPE )
		Com_Error( ERR_DROP, "%s has wrong type number (%i should be %i)",
		name, LittleLong( pinmodel->type ), SKM_MODELTYPE );
	if( LittleLong( pinmodel->filesize ) > SKM_MAX_FILESIZE )
		Com_Error( ERR_DROP, "%s has has wrong filesize (%i should be less than %i)",
		name, LittleLong( pinmodel->filesize ), SKM_MAX_FILESIZE );
	if( LittleLong( pinmodel->num_bones ) != poutmodel->numbones )
		Com_Error( ERR_DROP, "%s has has wrong number of bones (%i should be less than %i)",
		name, LittleLong( pinmodel->num_bones ), poutmodel->numbones );

	poutmodel->numframes = LittleLong( pinmodel->num_frames );
	if( poutmodel->numframes <= 0 )
		Com_Error( ERR_DROP, "%s has no frames", name );
	else if( poutmodel->numframes > SKM_MAX_FRAMES )
		Com_Error( ERR_DROP, "%s has too many frames", name );

	memBufferSize = 0;
	memBufferSize += sizeof( mskbone_t ) * poutmodel->numbones;
	memBufferSize += sizeof( mskframe_t ) * poutmodel->numframes;
	memBufferSize += sizeof( bonepose_t ) * poutmodel->numbones * poutmodel->numframes;
	membuffer = Mod_Malloc( mod, memBufferSize );

	pinbone = ( dskpbone_t * )( ( qbyte * )pinmodel + LittleLong( pinmodel->ofs_bones ) );
	poutbone = poutmodel->bones = ( mskbone_t * )membuffer; membuffer += sizeof( mskbone_t ) * poutmodel->numbones;

	for( i = 0; i < poutmodel->numbones; i++, pinbone++, poutbone++ )
	{
		Q_strncpyz( poutbone->name, pinbone->name, SKM_MAX_NAME );
		poutbone->flags = LittleLong( pinbone->flags );
		poutbone->parent = LittleLong( pinbone->parent );
	}

	pinframe = ( dskpframe_t * )( ( qbyte * )pinmodel + LittleLong( pinmodel->ofs_frames ) );
	poutframe = poutmodel->frames = ( mskframe_t * )membuffer; membuffer += sizeof( mskframe_t ) * poutmodel->numframes;

	for( i = 0; i < poutmodel->numframes; i++, pinframe++, poutframe++ )
	{
		pinbonepose = ( dskpbonepose_t * )( ( qbyte * )pinmodel + LittleLong( pinframe->ofs_bonepositions ) );
		poutbonepose = poutframe->boneposes = ( bonepose_t * )membuffer; membuffer += sizeof( bonepose_t ) * poutmodel->numbones;

		ClearBounds( poutframe->mins, poutframe->maxs );

		for( j = 0; j < poutmodel->numbones; j++, pinbonepose++, poutbonepose++ )
		{
			for( k = 0; k < 4; k++ )
				poutbonepose->quat[k] = LittleFloat( pinbonepose->quat[k] );
			for( k = 0; k < 3; k++ )
				poutbonepose->origin[k] = LittleFloat( pinbonepose->origin[k] );
		}
	}
}

/*
=================
Mod_LoadSkeletalModel
=================
*/
void Mod_LoadSkeletalModel( model_t *mod, const model_t *parent, void *buffer, bspFormatDesc_t *unused )
{
	unsigned int i, j, k, l, m;
	vec_t *v, *n;
	dskmheader_t *pinmodel;
	mskmodel_t *poutmodel;
	dskmmesh_t *pinmesh;
	mskmesh_t *poutmesh;
	dskmvertex_t *pinskmvert;
	dskmbonevert_t *pinbonevert;
	dskmcoord_t *pinstcoord;
	vec2_t *poutstcoord;
	elem_t	*pintris, *pouttris;
	unsigned int *pinreferences, *poutreferences;
	bonepose_t *bp, *basepose, *poutbonepose;
	mskframe_t *pframe;
	vec3_t ebbox = { 0, 0, 0 };
	size_t alignment = 16;
	qbyte *buf;

	pinmodel = ( dskmheader_t * )buffer;

	if( LittleLong( pinmodel->type ) != SKM_MODELTYPE )
		Com_Error( ERR_DROP, "%s has wrong type number (%i should be %i)",
		mod->name, LittleLong( pinmodel->type ), SKM_MODELTYPE );
	if( LittleLong( pinmodel->filesize ) > SKM_MAX_FILESIZE )
		Com_Error( ERR_DROP, "%s has has wrong filesize (%i should be less than %i)",
		mod->name, LittleLong( pinmodel->filesize ), SKM_MAX_FILESIZE );

	poutmodel = mod->extradata = Mod_Malloc( mod, sizeof( mskmodel_t ) );
	poutmodel->nummeshes = LittleLong( pinmodel->num_meshes );
	if( poutmodel->nummeshes <= 0 )
		Com_Error( ERR_DROP, "%s has no meshes", mod->name );
	else if( poutmodel->nummeshes > SKM_MAX_MESHES )
		Com_Error( ERR_DROP, "%s has too many meshes", mod->name );

	poutmodel->numbones = LittleLong( pinmodel->num_bones );
	if( poutmodel->numbones <= 0 )
		Com_Error( ERR_DROP, "%s has no bones", mod->name );
	else if( poutmodel->numbones > SKM_MAX_BONES )
		Com_Error( ERR_DROP, "%s has too many bones", mod->name );

	// if we have a parent model then we are a LOD file and should use parent model's pose data
	if( parent )
	{
		mskmodel_t *parentskm = ( mskmodel_t * )parent->extradata;

		if( !parentskm )
			Com_Error( ERR_DROP, "%s is not a LOD model for %s",
			mod->name, parent->name );

		if( poutmodel->numbones != parentskm->numbones )
			Com_Error( ERR_DROP, "%s has has wrong number of bones (%i should be less than %i)",
			mod->name, poutmodel->numbones, parentskm->numbones );

		poutmodel->bones = parentskm->bones;
		poutmodel->frames = parentskm->frames;
		poutmodel->numframes = parentskm->numframes;
	}
	else
	{	// load a config file
		char temp[MAX_QPATH];
		char poseName[MAX_QPATH], configName[MAX_QPATH];

		Q_strncpyz( temp, mod->name, sizeof( temp ) );
		COM_StripExtension( temp );
		Q_snprintfz( configName, sizeof( configName ), "%s.cfg", temp );

		memset( poseName, 0, sizeof( poseName ) );

		FS_LoadFile( configName, (void **)&buf, NULL, 0 );
		if( !buf )
		{
			Q_snprintfz( poseName, sizeof( poseName ), "%s.skp", temp );
		}
		else
		{
			char *ptr, *token;

			ptr = ( char * )buf;
			while( ptr )
			{
				token = COM_ParseExt( &ptr, qtrue );
				if( !token[0] )
					break;

				if( !Q_stricmp( token, "import" ) )
				{
					token = COM_ParseExt( &ptr, qfalse );

					Q_strncpyz( temp, token, sizeof( temp ) );
					COM_StripExtension( temp );
					Q_snprintfz( poseName, sizeof( poseName ), "%s.skp", temp );
					break;
				}
			}

			FS_FreeFile( buf );
		}

		FS_LoadFile( poseName, (void **)&buf, NULL, 0 );
		if( !buf )
			Com_Error( ERR_DROP, "Could not find pose file for %s", mod->name );

		Mod_LoadSkeletalPose( poseName, mod, buf );

		FS_FreeFile( buf );
	}

	// clear model's bounding box
	mod->radius = 0;
	ClearBounds( mod->mins, mod->maxs );

	// reconstruct frame 0 bone poses and inverse bone poses
	basepose = Mod_Malloc( mod, sizeof( *basepose ) * poutmodel->numbones );
	poutmodel->invbaseposes = Mod_Malloc( mod, sizeof( *poutmodel->invbaseposes ) * poutmodel->numbones );

	for( i = 0, bp = basepose; i < poutmodel->numbones; i++, bp++ )
	{
		int parent = poutmodel->bones[i].parent;
		bonepose_t *obp = &poutmodel->frames[0].boneposes[i], *ibp = &poutmodel->invbaseposes[i];

		if( parent >= 0 )
			Quat_ConcatTransforms( basepose[parent].quat, basepose[parent].origin,
				obp->quat, obp->origin, bp->quat, bp->origin );
		else
			*bp = *obp;

		// reconstruct invserse bone pose
		Quat_Conjugate( bp->quat, ibp->quat );
		Quat_TransformVector( ibp->quat, bp->origin, ibp->origin );
		VectorInverse( ibp->origin );
	}

	pinmesh = ( dskmmesh_t * )( ( qbyte * )pinmodel + LittleLong( pinmodel->ofs_meshes ) );
	poutmesh = poutmodel->meshes = Mod_Malloc( mod, sizeof( mskmesh_t ) * poutmodel->nummeshes );

	for( i = 0; i < poutmodel->nummeshes; i++, pinmesh++, poutmesh++ )
	{
		float *influences;
		unsigned int size, *bones;

		poutmesh->numverts = LittleLong( pinmesh->num_verts );
		if( poutmesh->numverts <= 0 )
			Com_Error( ERR_DROP, "mesh %i in model %s has no vertexes", i, mod->name );
		else if( poutmesh->numverts > SKM_MAX_VERTS )
			Com_Error( ERR_DROP, "mesh %i in model %s has too many vertexes", i, mod->name );

		poutmesh->numtris = LittleLong( pinmesh->num_tris );
		if( poutmesh->numtris <= 0 )
			Com_Error( ERR_DROP, "mesh %i in model %s has no indices", i, mod->name );
		else if( poutmesh->numtris > SKM_MAX_TRIS )
			Com_Error( ERR_DROP, "mesh %i in model %s has too many indices", i, mod->name );

		poutmesh->numreferences = LittleLong( pinmesh->num_references );
		if( poutmesh->numreferences <= 0 )
			Com_Error( ERR_DROP, "mesh %i in model %s has no bone references", i, mod->name );
		else if( poutmesh->numreferences > SKM_MAX_BONES )
			Com_Error( ERR_DROP, "mesh %i in model %s has too many bone references", i, mod->name );

		Q_strncpyz( poutmesh->name, pinmesh->meshname, sizeof( poutmesh->name ) );
		Mod_StripLODSuffix( poutmesh->name );

		poutmesh->skin.shader = R_RegisterSkin( pinmesh->shadername );
		R_DeformvBBoxForShader( poutmesh->skin.shader, ebbox );

		pinreferences = ( elem_t *)( ( qbyte * )pinmodel + LittleLong( pinmesh->ofs_references ) );
		poutreferences = poutmesh->references = Mod_Malloc( mod, sizeof( unsigned int ) * poutmesh->numreferences );
		for( j = 0; j < poutmesh->numreferences; j++, pinreferences++, poutreferences++ )
			*poutreferences = LittleLong( *pinreferences );

		pinskmvert = ( dskmvertex_t * )( ( qbyte * )pinmodel + LittleLong( pinmesh->ofs_verts ) );

		poutmesh->influences = ( float * )Mod_Malloc( mod, ( sizeof( *poutmesh->influences ) + sizeof( *poutmesh->bones ) ) * SKM_MAX_WEIGHTS * poutmesh->numverts );
		poutmesh->bones = ( unsigned int * )( ( qbyte * )poutmesh->influences + sizeof( *poutmesh->influences ) * SKM_MAX_WEIGHTS * poutmesh->numverts );

		size = sizeof( vec4_t ) + sizeof( vec4_t ); // xyz and normals
		if( glConfig.ext.GLSL )
			size += sizeof( vec4_t );       // s-vectors

		size *= ( poutmesh->numverts+1 );       // pad by one additional vertex for prefetching

		// align poutmesh->xyzArray on a 16-byte boundary (we leak 16 bytes at most though)
		buf = ( qbyte * )( Mod_Malloc( mod, size+alignment ) );
		poutmesh->xyzArray = ( vec4_t * )( buf+alignment-( (size_t)buf % alignment ) );
		poutmesh->normalsArray = ( vec4_t * )( ( qbyte * )poutmesh->xyzArray + sizeof( vec4_t ) * ( poutmesh->numverts+1 ) );

		v = ( vec_t * )poutmesh->xyzArray[0];
		n = ( vec_t * )poutmesh->normalsArray[0];
		influences = poutmesh->influences;
		bones = poutmesh->bones;
		pframe = &poutmodel->frames[0];
		for( j = 0; j < poutmesh->numverts; j++, v += 4, n += 4, influences += SKM_MAX_WEIGHTS, bones += SKM_MAX_WEIGHTS )
		{
			float sum, influence;
			unsigned int bonenum, numweights;
			vec3_t origin, normal, t, matrix[3];

			pinbonevert = ( dskmbonevert_t * )( ( qbyte * )pinskmvert + sizeof( pinskmvert->numweights ) );
			numweights = LittleLong( pinskmvert->numweights );

			for( l = 0; l < numweights; l++, pinbonevert++ )
			{
				bonenum = LittleLong( pinbonevert->bonenum );
				influence = LittleFloat( pinbonevert->influence );
				poutbonepose = basepose + bonenum;
				for( k = 0; k < 3; k++ )
				{
					origin[k] = LittleFloat( pinbonevert->origin[k] );
					normal[k] = LittleFloat( pinbonevert->normal[k] );
				}

				// rebuild the base pose
				Quat_Matrix( poutbonepose->quat, matrix );

				Matrix_TransformVector( matrix, origin, t );
				VectorAdd( v, t, v );
				VectorMA( v, influence, poutbonepose->origin, v );
				v[3] = 1;

				Matrix_TransformVector( matrix, normal, t );
				VectorAdd( n, t, n );
				n[3] = 0;

				if( !l )
				{ // store the first influence
					bones[0] = bonenum;
					influences[0] = influence;
				}
				else
				{ // store the new influence if needed
					for( k = 0; k < SKM_MAX_WEIGHTS; k++ )
					{
						if( influence > influences[k] )
						{
							// pop the most weak influences out of the array,
							// shifting the rest of them to the beginning
							for( m = SKM_MAX_WEIGHTS-1; m > k; m-- )
							{
								bones[m] = bones[m-1];
								influences[m] = influences[m-1];
							}

							// store the new influence
							bones[k] = bonenum;
							influences[k] = influence;
							break;
						}
					}
				}
			}

			// normalize influences if needed
			for( l = 0, sum = 0; l < SKM_MAX_WEIGHTS && influences[l]; l++ )
				sum += influences[l];
			if( sum > 1.0f - 1.0/256.0f )
			{
				for( l = 0, sum = 1.0f / sum; l < SKM_MAX_WEIGHTS && influences[l]; l++ )
					influences[l] *= sum;
			}

			for( l = 0; l < SKM_MAX_WEIGHTS; l++ )
			{
				if( influences[l] == 1.0f )
				{
					assert( l < SKM_MAX_WEIGHTS-1 );
					influences[l] = 1;
					influences[l+1] = 0;
					break;
				}
			}

			// test vertex against the bounding box
			AddPointToBounds( v, pframe->mins, pframe->maxs );

			pinskmvert = ( dskmvertex_t * )( ( qbyte * )pinbonevert );
		}

		pinstcoord = ( dskmcoord_t * )( ( qbyte * )pinmodel + LittleLong( pinmesh->ofs_texcoords ) );
		poutstcoord = poutmesh->stArray = Mod_Malloc( mod, poutmesh->numverts * sizeof( vec2_t ) );
		for( j = 0; j < poutmesh->numverts; j++, pinstcoord++ )
		{
			poutstcoord[j][0] = LittleFloat( pinstcoord->st[0] );
			poutstcoord[j][1] = LittleFloat( pinstcoord->st[1] );
		}

		pintris = ( elem_t * )( ( qbyte * )pinmodel + LittleLong( pinmesh->ofs_indices ) );
		pouttris = poutmesh->elems = Mod_Malloc( mod, sizeof( elem_t ) * poutmesh->numtris * 3 );
		for( j = 0; j < poutmesh->numtris; j++, pintris += 3, pouttris += 3 )
		{
			pouttris[0] = (elem_t)LittleLong( pintris[0] );
			pouttris[1] = (elem_t)LittleLong( pintris[1] );
			pouttris[2] = (elem_t)LittleLong( pintris[2] );
		}

		//
		// build S and T vectors
		//
		if( glConfig.ext.GLSL )
		{
			poutmesh->sVectorsArray = ( vec4_t * )( ( qbyte * )poutmesh->normalsArray + sizeof( vec4_t ) * ( poutmesh->numverts+1 ) );
			R_BuildTangentVectors( poutmesh->numverts, poutmesh->xyzArray, poutmesh->normalsArray, poutmesh->stArray,
				poutmesh->numtris, poutmesh->elems, poutmesh->sVectorsArray );
		}
	}

#ifndef PUBLIC_BUILD
	// enable this to speed up loading
	for( j = 1; j < poutmodel->numframes; j++ )
	{
		VectorMA( poutmodel->frames[0].mins, -1, tv(48, 48, 48), poutmodel->frames[j].mins );
		VectorMA( poutmodel->frames[0].maxs,  1, tv(48, 48, 48), poutmodel->frames[j].maxs );
	}
#else
	// so much work just to calc the model's bounds, doh
	for( j = 1; j < poutmodel->numframes; j++ )
	{
		pframe = &poutmodel->frames[j];

		for( i = 0, bp = basepose; i < poutmodel->numbones; i++, bp++ )
		{
			int parent = poutmodel->bones[i].parent;
			bonepose_t *obp = &pframe->boneposes[i];

			if( parent >= 0 )
				Quat_ConcatTransforms( basepose[parent].quat, basepose[parent].origin,
					obp->quat, obp->origin, bp->quat, bp->origin );
			else
				*bp = *obp;
		}

		pinmesh = ( dskmmesh_t * )( ( qbyte * )pinmodel + LittleLong( pinmodel->ofs_meshes ) );
		for( i = 0, poutmesh = poutmodel->meshes; i < poutmodel->nummeshes; i++, pinmesh++, poutmesh++ )
		{
			pinskmvert = ( dskmvertex_t * )( ( qbyte * )pinmodel + LittleLong( pinmesh->ofs_verts ) );

			for( k = 0; k < poutmesh->numverts; k++ )
			{
				float influence;
				unsigned int numweights, bonenum;
				vec3_t v;

				pinbonevert = ( dskmbonevert_t * )( ( qbyte * )pinskmvert + sizeof( pinskmvert->numweights ) );
				numweights = LittleLong( pinskmvert->numweights );

				VectorClear( v );
				for( l = 0; l < numweights; l++, pinbonevert++ )
				{
					vec3_t origin, t;

					bonenum = LittleLong( pinbonevert->bonenum );
					influence = LittleFloat( pinbonevert->influence );
					poutbonepose = basepose + bonenum;
					for( m = 0; m < 3; m++ )
						origin[m] = LittleFloat( pinbonevert->origin[m] );

					// transform vertex
					Quat_TransformVector( poutbonepose->quat, origin, t );
					VectorAdd( v, t, v );
					VectorMA( v, influence, poutbonepose->origin, v );
				}

				// test vertex against the bounding box
				AddPointToBounds( v, pframe->mins, pframe->maxs );

				pinskmvert = ( dskmvertex_t * )( ( qbyte * )pinbonevert );
			}
		}

	}
#endif

	ClearBounds( mod->mins, mod->maxs );
	for( j = 0, pframe = poutmodel->frames; j < poutmodel->numframes; j++, pframe++ )
	{
		VectorSubtract( pframe->mins, ebbox, pframe->mins );
		VectorAdd( pframe->maxs, ebbox, pframe->maxs );
		pframe->radius = RadiusFromBounds( pframe->mins, pframe->maxs );
		AddPointToBounds( pframe->mins, mod->mins, mod->maxs );
		AddPointToBounds( pframe->maxs, mod->mins, mod->maxs );
	}
	mod->radius = RadiusFromBounds( mod->mins, mod->maxs );

	Mem_Free( basepose );
	mod->type = mod_skeletal;
}

/*
================
R_SkeletalGetNumBones
================
*/
int R_SkeletalGetNumBones( const model_t *mod, int *numFrames )
{
	mskmodel_t *skmodel;

	if( !mod || mod->type != mod_skeletal )
		return 0;

	skmodel = ( mskmodel_t * )mod->extradata;
	if( numFrames )
		*numFrames = skmodel->numframes;
	return skmodel->numbones;
}

/*
================
R_SkeletalGetBoneInfo
================
*/
int R_SkeletalGetBoneInfo( const model_t *mod, int bonenum, char *name, size_t name_size, int *flags )
{
	const mskbone_t *bone;
	const mskmodel_t *skmodel;

	if( !mod || mod->type != mod_skeletal )
		return 0;

	skmodel = ( mskmodel_t * )mod->extradata;
	if( (unsigned int)bonenum >= (int)skmodel->numbones )
		Com_Error( ERR_DROP, "R_SkeletalGetBone: bad bone number" );

	bone = &skmodel->bones[bonenum];
	if( name && name_size )
		Q_strncpyz( name, bone->name, name_size );
	if( flags )
		*flags = bone->flags;
	return bone->parent;
}

/*
================
R_SkeletalGetBonePose
================
*/
void R_SkeletalGetBonePose( const model_t *mod, int bonenum, int frame, bonepose_t *bonepose )
{
	const mskmodel_t *skmodel;

	if( !mod || mod->type != mod_skeletal )
		return;

	skmodel = ( mskmodel_t * )mod->extradata;
	if( bonenum < 0 || bonenum >= (int)skmodel->numbones )
		Com_Error( ERR_DROP, "R_SkeletalGetBonePose: bad bone number" );
	if( frame < 0 || frame >= (int)skmodel->numframes )
		Com_Error( ERR_DROP, "R_SkeletalGetBonePose: bad frame number" );

	if( bonepose )
		*bonepose = skmodel->frames[frame].boneposes[bonenum];
}

/*
================
R_SkeletalModelLOD
================
*/
static model_t *R_SkeletalModelLOD( entity_t *e )
{
	int lod;
	float dist;

	if( !e->model->numlods || ( e->flags & RF_FORCENOLOD ) )
		return e->model;

	dist = DistanceFast( e->origin, ri.viewOrigin );
	dist *= ri.lod_dist_scale_for_fov;

	lod = (int)( dist / e->model->radius );
	if( r_lodscale->integer )
		lod /= r_lodscale->integer;
	lod += r_lodbias->integer;

	if( lod < 1 )
		return e->model;
	return e->model->lods[min( lod, e->model->numlods )-1];
}

/*
================
R_SkeletalModelLerpBBox
================
*/
static void R_SkeletalModelLerpBBox( entity_t *e, model_t *mod )
{
	int i;
	mskframe_t *pframe, *poldframe;
	float *thismins, *oldmins, *thismaxs, *oldmaxs;
	mskmodel_t *skmodel = ( mskmodel_t * )mod->extradata;

	if( !skmodel->nummeshes )
	{
		skm_radius = 0;
		ClearBounds( skm_mins, skm_maxs );
		return;
	}

	if( ( e->frame >= (int)skmodel->numframes ) || ( e->frame < 0 ) )
	{
#ifndef PUBLIC_BUILD
		Com_DPrintf( "R_SkeletalModelBBox %s: no such frame %d\n", mod->name, e->frame );
#endif
		e->frame = 0;
	}
	if( ( e->oldframe >= (int)skmodel->numframes ) || ( e->oldframe < 0 ) )
	{
#ifndef PUBLIC_BUILD
		Com_DPrintf( "R_SkeletalModelBBox %s: no such oldframe %d\n", mod->name, e->oldframe );
#endif
		e->oldframe = 0;
	}

	pframe = skmodel->frames + e->frame;
	poldframe = skmodel->frames + e->oldframe;

	// compute axially aligned mins and maxs
	if( pframe == poldframe )
	{
		VectorCopy( pframe->mins, skm_mins );
		VectorCopy( pframe->maxs, skm_maxs );
		skm_radius = pframe->radius;
	}
	else
	{
		thismins = pframe->mins;
		thismaxs = pframe->maxs;

		oldmins = poldframe->mins;
		oldmaxs = poldframe->maxs;

		for( i = 0; i < 3; i++ )
		{
			skm_mins[i] = min( thismins[i], oldmins[i] );
			skm_maxs[i] = max( thismaxs[i], oldmaxs[i] );
		}
		skm_radius = RadiusFromBounds( thismins, thismaxs );
	}

	if( e->scale != 1.0f )
	{
		VectorScale( skm_mins, e->scale, skm_mins );
		VectorScale( skm_maxs, e->scale, skm_maxs );
		skm_radius *= e->scale;
	}
}

//=======================================================================

// set the FP precision to fast
#if defined ( _WIN32 ) && ( _MSC_VER >= 1400 ) && defined( NDEBUG )
# pragma float_control(except, off, push)
# pragma float_control(precise, off, push)
# pragma fp_contract(on)		// this line is needed on Itanium processors
#endif

/*
================
R_SkeletalTransformVerts
================
*/
static void R_SkeletalTransformVerts( int numverts, const unsigned int *bones, const float *influences, mat4x4_t *relbonepose, const vec_t *v, vec_t *ov )
{
	int j;
	float i;
	const float *pose;

	for( ; numverts; numverts--, v += 4, ov += 4, bones += SKM_MAX_WEIGHTS, influences += SKM_MAX_WEIGHTS )
	{
		i = *influences;
		pose = relbonepose[*bones];

		if( i == 1 )
		{
			// most common case
			ov[0] = v[0] * pose[0] + v[1] * pose[4] + v[2] * pose[8] + pose[12];
			ov[1] = v[0] * pose[1] + v[1] * pose[5] + v[2] * pose[9] + pose[13];
			ov[2] = v[0] * pose[2] + v[1] * pose[6] + v[2] * pose[10] + pose[14];
		}
		else
		{
			ov[0] = i * ( v[0] * pose[0] + v[1] * pose[4] + v[2] * pose[8] + pose[12] );
			ov[1] = i * ( v[0] * pose[1] + v[1] * pose[5] + v[2] * pose[9] + pose[13] );
			ov[2] = i * ( v[0] * pose[2] + v[1] * pose[6] + v[2] * pose[10] + pose[14] );

			for( j = 1; j < SKM_MAX_WEIGHTS && influences[j]; j++ )
			{
				i = influences[j];
				pose = relbonepose[bones[j]];

				ov[0] += i * ( v[0] * pose[0] + v[1] * pose[4] + v[2] * pose[8] + pose[12] );
				ov[1] += i * ( v[0] * pose[1] + v[1] * pose[5] + v[2] * pose[9] + pose[13] );
				ov[2] += i * ( v[0] * pose[2] + v[1] * pose[6] + v[2] * pose[10] + pose[14] );
			}
		}
	}
}

/*
================
R_SkeletalTransformNormals
================
*/
static void R_SkeletalTransformNormals( int numverts, const unsigned int *bones, const float *influences, mat4x4_t *relbonepose, const vec_t *v, vec_t *ov )
{
	int j;
	float i;
	const float *pose;

	for( ; numverts; numverts--, v += 4, ov += 4, bones += SKM_MAX_WEIGHTS, influences += SKM_MAX_WEIGHTS )
	{
		i = *influences;
		pose = relbonepose[*bones];

		if( i == 1 )
		{       // most common case
			ov[0] = v[0] * pose[0] + v[1] * pose[4] + v[2] * pose[8];
			ov[1] = v[0] * pose[1] + v[1] * pose[5] + v[2] * pose[9];
			ov[2] = v[0] * pose[2] + v[1] * pose[6] + v[2] * pose[10];
			ov[3] = v[3];
		}
		else
		{
			ov[0] = i * ( v[0] * pose[0] + v[1] * pose[4] + v[2] * pose[8] );
			ov[1] = i * ( v[0] * pose[1] + v[1] * pose[5] + v[2] * pose[9] );
			ov[2] = i * ( v[0] * pose[2] + v[1] * pose[6] + v[2] * pose[10] );
			ov[3] = v[3];

			for( j = 1; j < SKM_MAX_WEIGHTS && influences[j]; j++ )
			{
				i = influences[j];
				pose = relbonepose[bones[j]];

				ov[0] += i * ( v[0] * pose[0] + v[1] * pose[4] + v[2] * pose[8] );
				ov[1] += i * ( v[0] * pose[1] + v[1] * pose[5] + v[2] * pose[9] );
				ov[2] += i * ( v[0] * pose[2] + v[1] * pose[6] + v[2] * pose[10] );
			}
		}
	}
}

// set the FP precision back to whatever value it was
#if defined ( _WIN32 ) && ( _MSC_VER >= 1400 ) && defined( NDEBUG )
# pragma float_control(pop)
# pragma float_control(pop)
# pragma fp_contract(off)	// this line is needed on Itanium processors
#endif

#if defined ( id386 ) && !defined ( __MACOSX__ )

#if defined ( __GNUC__ ) && defined ( __SSE__ )

#include <xmmintrin.h>

/*
================
R_SkeletalTransformVerts_GCC
================
*/
void R_SkeletalTransformVerts_GCC( int numverts, const unsigned int *bones, const float *influences, mat4x4_t *relbonepose, const vec_t *v, vec_t *ov )
{
	int j;
	//float i;
	//const float *pose;
	__m128 pose[4];
	__m128 i_sse;

	for( ; numverts; numverts--, v += 4, ov += 4, bones += SKM_MAX_WEIGHTS, influences += SKM_MAX_WEIGHTS )
	{
		__m128 accumulator;
		__m128 v0_sse, v1_sse, v2_sse;

		_mm_prefetch( relbonepose[*(bones+SKM_MAX_WEIGHTS)], _MM_HINT_NTA );
		_mm_prefetch( v+32, _MM_HINT_NTA );
		    v0_sse = v1_sse = v2_sse = _mm_load_ps( v );
		    v0_sse = _mm_shuffle_ps( v0_sse,v0_sse, 0x00 );
		    v1_sse = _mm_shuffle_ps( v1_sse,v1_sse, 0x55 );
		    v2_sse = _mm_shuffle_ps( v2_sse,v2_sse, 0xaa );

		i_sse = _mm_load_ss( influences );
		//pose = relbonepose[*bones];
		pose[0] = _mm_load_ps( relbonepose[*bones] );
		pose[1] = _mm_load_ps( relbonepose[*bones] + 4 );
		pose[2] = _mm_load_ps( relbonepose[*bones] + 8 );
		pose[3] = _mm_load_ps( relbonepose[*bones] + 12 );

		if( _mm_comieq_ss( i_sse, _mm_set_ss( 1.0f) ) )
		{       // most common case
			//ov[0] = v[0] * pose[0] + v[1] * pose[4] + v[2] * pose[8] + pose[12];
			//ov[1] = v[0] * pose[1] + v[1] * pose[5] + v[2] * pose[9] + pose[13];
			//ov[2] = v[0] * pose[2] + v[1] * pose[6] + v[2] * pose[10] + pose[14];

			accumulator = _mm_mul_ps( v0_sse, pose[0] );
			accumulator = _mm_add_ps( accumulator, _mm_mul_ps( v1_sse, pose[1] ) );
			accumulator = _mm_add_ps( accumulator, _mm_mul_ps( v2_sse, pose[2] ) );
			accumulator = _mm_add_ps( accumulator, pose[3] );
			_mm_stream_ps( ov, accumulator );
		}
		else
		{
			i_sse = _mm_shuffle_ps( i_sse, i_sse, 0x00 );

			accumulator = _mm_mul_ps( v0_sse, pose[0] );
			accumulator = _mm_add_ps( accumulator, _mm_mul_ps( v1_sse, pose[1] ) );
			accumulator = _mm_add_ps( accumulator, _mm_mul_ps( v2_sse, pose[2] ) );
			accumulator = _mm_add_ps( accumulator, pose[3] );

			accumulator = _mm_mul_ps( accumulator, i_sse );
			//ov[0] = i * ( v[0] * pose[0] + v[1] * pose[4] + v[2] * pose[8] + pose[12] );
			//ov[1] = i * ( v[0] * pose[1] + v[1] * pose[5] + v[2] * pose[9] + pose[13] );
			//ov[2] = i * ( v[0] * pose[2] + v[1] * pose[6] + v[2] * pose[10] + pose[14] );

			for( j = 1; j < SKM_MAX_WEIGHTS && influences[j]; j++ )
			{
				__m128 acc2;
				i_sse = _mm_load1_ps( &(influences[j]) );
				//__builtin_ia32_shufps( i_sse, i_sse, 0x00 );
				//pose = relbonepose[bones[j]];
				pose[0] = _mm_load_ps( relbonepose[bones[j]] );
				pose[1] = _mm_load_ps( relbonepose[bones[j]] + 4 );
				pose[2] = _mm_load_ps( relbonepose[bones[j]] + 8 );
				pose[3] = _mm_load_ps( relbonepose[bones[j]] + 12 );

				acc2 = _mm_mul_ps( v0_sse, pose[0] );
				acc2 = _mm_add_ps( acc2, _mm_mul_ps( v1_sse, pose[1] ) );
				acc2 = _mm_add_ps( acc2, _mm_mul_ps( v2_sse, pose[2] ) );
				acc2 = _mm_add_ps( acc2, pose[3] );

				acc2 = _mm_mul_ps( acc2, i_sse );

				accumulator = _mm_add_ps( accumulator, acc2 );
				//ov[0] += i * ( v[0] * pose[0] + v[1] * pose[4] + v[2] * pose[8] + pose[12] );
				//ov[1] += i * ( v[0] * pose[1] + v[1] * pose[5] + v[2] * pose[9] + pose[13] );
				//ov[2] += i * ( v[0] * pose[2] + v[1] * pose[6] + v[2] * pose[10] + pose[14] );
			}
			_mm_stream_ps( ov, accumulator );
		}
	}
}

/*
================
R_SkeletalTransformNormals_GCC
================
*/
static void R_SkeletalTransformNormals_GCC( int numverts, const unsigned int *bones, const float *influences, mat4x4_t *relbonepose, const vec_t *v, vec_t *ov )
{
	int j;
	float i;
	//const float *pose;
	__m128 pose[4];

	for( ; numverts; numverts--, v += 4, ov += 4, bones += SKM_MAX_WEIGHTS, influences += SKM_MAX_WEIGHTS )
	{
		__m128 accumulator;

		__m128 v0_sse, v1_sse, v2_sse, v3_sse;
		    v0_sse = v1_sse = v2_sse = v3_sse = _mm_load_ps( v );
		    v0_sse = _mm_shuffle_ps( v0_sse,v0_sse, 0x00 );
		    v1_sse = _mm_shuffle_ps( v1_sse,v1_sse, 0x55 );
		    v2_sse = _mm_shuffle_ps( v2_sse,v2_sse, 0xaa );
		    v3_sse = _mm_shuffle_ps( v3_sse,v3_sse, 0xff );

		i = *influences;
		//pose = relbonepose[*bones];
		pose[0] = _mm_load_ps( relbonepose[*bones] );
		pose[1] = _mm_load_ps( relbonepose[*bones] + 4 );
		pose[2] = _mm_load_ps( relbonepose[*bones] + 8 );
		pose[3] = _mm_load_ps( relbonepose[*bones] + 12 );

		if( i == 1.0f )
		{       // most common case
			//ov[0] = v[0] * pose[0] + v[1] * pose[4] + v[2] * pose[8] + pose[12];
			//ov[1] = v[0] * pose[1] + v[1] * pose[5] + v[2] * pose[9] + pose[13];
			//ov[2] = v[0] * pose[2] + v[1] * pose[6] + v[2] * pose[10] + pose[14];

			accumulator = _mm_mul_ps( v0_sse, pose[0] );
			accumulator = _mm_add_ps( accumulator, _mm_mul_ps( v1_sse, pose[1] ) );
			accumulator = _mm_add_ps( accumulator, _mm_mul_ps( v2_sse, pose[2] ) );
			accumulator = _mm_add_ps( accumulator, _mm_mul_ps( v3_sse, pose[3] ) );

			_mm_store_ps( ov, accumulator );
		}
		else
		{
		    __m128 i_sse = _mm_load1_ps( influences );

			accumulator = _mm_mul_ps( v0_sse, pose[0] );
			accumulator = _mm_add_ps( accumulator, _mm_mul_ps( v1_sse, pose[1] ) );
			accumulator = _mm_add_ps( accumulator, _mm_mul_ps( v2_sse, pose[2] ) );
			accumulator = _mm_add_ps( accumulator, _mm_mul_ps( v3_sse, pose[3] ) );

			accumulator = _mm_mul_ps( accumulator, i_sse );

			//ov[0] = i * ( v[0] * pose[0] + v[1] * pose[4] + v[2] * pose[8] + pose[12] );
			//ov[1] = i * ( v[0] * pose[1] + v[1] * pose[5] + v[2] * pose[9] + pose[13] );
			//ov[2] = i * ( v[0] * pose[2] + v[1] * pose[6] + v[2] * pose[10] + pose[14] );

			for( j = 1; j < SKM_MAX_WEIGHTS && influences[j]; j++ )
			{
				__m128 acc2;
				i_sse = _mm_load1_ps( &(influences[j]) );
				//__builtin_ia32_shufps( i_sse, i_sse, 0x00 );
				//pose = relbonepose[bones[j]];
				pose[0] = _mm_load_ps( relbonepose[bones[j]] );
				pose[1] = _mm_load_ps( relbonepose[bones[j]] + 4 );
				pose[2] = _mm_load_ps( relbonepose[bones[j]] + 8 );
				pose[3] = _mm_load_ps( relbonepose[bones[j]] + 12 );

				acc2 = _mm_mul_ps( v0_sse, pose[0] );
				acc2 = _mm_add_ps( acc2, _mm_mul_ps( v1_sse, pose[1] ) );
				acc2 = _mm_add_ps( acc2, _mm_mul_ps( v2_sse, pose[2] ) );
				acc2 = _mm_add_ps( acc2, _mm_mul_ps( v3_sse, pose[3] ) );

				acc2 = _mm_mul_ps( acc2, i_sse );

				accumulator = _mm_add_ps( accumulator, acc2 );
				//ov[0] += i * ( v[0] * pose[0] + v[1] * pose[4] + v[2] * pose[8] + pose[12] );
				//ov[1] += i * ( v[0] * pose[1] + v[1] * pose[5] + v[2] * pose[9] + pose[13] );
				//ov[2] += i * ( v[0] * pose[2] + v[1] * pose[6] + v[2] * pose[10] + pose[14] );
			}
			_mm_store_ps( ov, accumulator );
		}
	}
}

#define R_SkeletalTransformVerts_SSE R_SkeletalTransformVerts_GCC
#define R_SkeletalTransformNormals_SSE R_SkeletalTransformNormals_GCC

#elif defined ( _WIN32 ) && ( _MSC_VER >= 1400 )
#pragma optimize( "", off )

/*
================
R_SkeletalTransformVerts_MSVC
================
*/
static void R_SkeletalTransformVerts_MSVC( int numverts, const unsigned int *bones, const float *influences, mat4x4_t *relbonepose, const vec_t *v, vec_t *ov )
{
	__asm {
		mov eax, numverts;
		test eax, eax;
		jz done;

		imul eax, 0x10;
		mov numverts, eax;
		xor eax, eax;

		mov ebx, v;
		mov edi, ov;
		mov ecx, relbonepose;

sl1:
		movaps xmm0, ds : dword ptr[ebx+eax];
		movaps xmm1, ds : dword ptr[ebx+eax];
		movaps xmm2, ds : dword ptr[ebx+eax];
		//prefetchnta [ebx+eax+0x10];

		// loop init
		xor edx, edx;

		//i = influence[0];
		mov esi, influences;
		cmp ds : dword ptr[esi], 0x3F800000; // IEEE-format representation of 1.0f

		movss xmm7, ds : dword ptr[esi]; // xmm7 contains influence
		shufps xmm7, xmm7, 0x00;

		shufps xmm0, xmm0, 0x00;
		shufps xmm1, xmm1, 0x55;
		shufps xmm2, xmm2, 0xAA;

		mov esi, bones;
		mov esi, ds : dword ptr[esi];
		lea esi,[esi *8];
		lea esi,[esi *8+ecx];

		jne slowCase;

		//	fastCase:
		// copying pose[0], pose[1], pose[2], pose[3] into xmm3
		movaps xmm3, ds : dword ptr[esi+0x00];
		// copying pose[4], pose[5], pose[6], pose[7] into xmm4
		movaps xmm4, ds : dword ptr[esi+0x10];
		// copying pose[8], pose[9], pose[10], pose[11] into xmm5
		movaps xmm5, ds : dword ptr[esi+0x20];
		// copying pose[12], pose[13], pose[14], pose[15] into xmm5
		movaps xmm6, ds : dword ptr[esi+0x30];

		mulps xmm3, xmm0; // xmm3 * v[0]
		mulps xmm4, xmm1; // xmm4 * v[1]
		mulps xmm5, xmm2; // xmm5 * v[2]

		// adding each column vector into xmm6
		addps xmm6, xmm3;
		addps xmm6, xmm4;
		addps xmm6, xmm5;
		jmp endOfLoop;

		// loop start
slowCase:
		xorps xmm6, xmm6;

continueSlowLoop:
		// copying pose[0], pose[1], pose[2], pose[3] into xmm4
		movaps xmm3, ds : dword ptr[esi+0x00];
		// copying pose[4], pose[5], pose[6], pose[7] into xmm5
		movaps xmm4, ds : dword ptr[esi+0x10];
		// copying pose[8], pose[9], pose[10], pose[11] into xmm4
		movaps xmm5, ds : dword ptr[esi+0x20];

		mulps xmm3, xmm0; // xmm3 * v[0]
		mulps xmm4, xmm1; // xmm4 * v[1]
		mulps xmm5, xmm2; // xmm5 * v[2]

		// adding each column vector into xmm5
		addps xmm5, ds : dword ptr[esi+0x30]; // adding pose[12], pose[13], pose[14], pose[15] into xmm5

		mulps xmm3, xmm7; // i * v[0] * pos
		mulps xmm4, xmm7; // i * v[1] * pos
		mulps xmm5, xmm7; // i * v[2] * pos

		addps xmm6, xmm3;
		addps xmm6, xmm4;
		addps xmm6, xmm5; // adding the result to ov vector

		inc edx;
		cmp edx, SKM_MAX_WEIGHTS;

		// loop condition
		je endOfLoop;

		mov esi, influences;
		cmp ds : dword ptr[esi+edx*0x04], 0;
		je endOfLoop;

		//i = influence[j];
		movss xmm7, ds : dword ptr[esi+edx*0x04]; // xmm6 contains i
		shufps xmm7, xmm7, 0x00;

		mov esi, bones;
		mov esi, ds : dword ptr[esi+edx*0x04];
		lea esi,[esi *8];
		lea esi,[esi *8+ecx];

		jmp continueSlowLoop;

endOfLoop:
		movaps ds : dword ptr[edi+eax], xmm6;

		add bones, SKM_MAX_WEIGHTS *0x04;
		add influences, SKM_MAX_WEIGHTS *0x04;

		add eax, 0x10;
		cmp eax, numverts;

		jl sl1;
done:
		;
	}
}

/*
================
R_SkeletalTransformNormals_MSVC
================
*/
static void R_SkeletalTransformNormals_MSVC( int numverts, const unsigned int *bones, const float *influences, mat4x4_t *relbonepose, const vec_t *v, vec_t *ov )
{
	__asm {
		mov eax, numverts;
		test eax, eax;
		jz done;

		imul eax, 0x10;
		mov numverts, eax;
		xor eax, eax;

		mov ebx, v;
		mov edi, ov;
		mov ecx, relbonepose;

sl1:
		movaps xmm0, ds : dword ptr[ebx+eax];
		movaps xmm1, ds : dword ptr[ebx+eax];
		movaps xmm2, ds : dword ptr[ebx+eax];
		//prefetchnta [ebx+eax+0x10];

		// loop init
		xor edx, edx;

		//i = influence[0];
		mov esi, influences;
		cmp ds : dword ptr[esi], 0x3F800000; // IEEE-format representation of 1.0f

		movss xmm7, ds : dword ptr[esi]; // xmm7 contains influence
		shufps xmm7, xmm7, 0x00;

		shufps xmm0, xmm0, 0x00;
		shufps xmm1, xmm1, 0x55;
		shufps xmm2, xmm2, 0xAA;

		mov esi, bones;
		mov esi, ds : dword ptr[esi];
		lea esi,[esi *8];
		lea esi,[esi *8+ecx];

		jne slowCase;

		//	fastCase:
		xorps xmm6, xmm6;

		// copying pose[0], pose[1], pose[2], pose[3] into xmm3
		movaps xmm3, ds : dword ptr[esi+0x00];
		// copying pose[4], pose[5], pose[6], pose[7] into xmm4
		movaps xmm4, ds : dword ptr[esi+0x10];
		// copying pose[8], pose[9], pose[10], pose[11] into xmm5
		movaps xmm5, ds : dword ptr[esi+0x20];

		mulps xmm3, xmm0; // xmm3 * v[0]
		mulps xmm4, xmm1; // xmm4 * v[1]
		mulps xmm5, xmm2; // xmm5 * v[2]

		// adding each column vector into xmm6
		addps xmm6, xmm3;
		addps xmm6, xmm4;
		addps xmm6, xmm5;
		jmp endOfLoop;

		// loop start
slowCase:
		xorps xmm6, xmm6;

continueSlowLoop:
		// copying pose[0], pose[1], pose[2], pose[3] into xmm4
		movaps xmm3, ds : dword ptr[esi+0x00];
		// copying pose[4], pose[5], pose[6], pose[7] into xmm5
		movaps xmm4, ds : dword ptr[esi+0x10];
		// copying pose[8], pose[9], pose[10], pose[11] into xmm4
		movaps xmm5, ds : dword ptr[esi+0x20];

		mulps xmm3, xmm0; // xmm3 * v[0]
		mulps xmm4, xmm1; // xmm4 * v[1]
		mulps xmm5, xmm2; // xmm5 * v[2]

		mulps xmm3, xmm7; // i * v[0] * pos
		mulps xmm4, xmm7; // i * v[1] * pos
		mulps xmm5, xmm7; // i * v[2] * pos

		addps xmm6, xmm3;
		addps xmm6, xmm4;
		addps xmm6, xmm5; // adding the result to ov vector

		inc edx;
		cmp edx, SKM_MAX_WEIGHTS;

		// loop condition
		je endOfLoop;

		mov esi, influences;
		cmp ds : dword ptr[esi+edx*0x04], 0;
		je endOfLoop;

		//i = influence[j];
		movss xmm7, ds : dword ptr[esi+edx*0x04]; // xmm6 contains i
		shufps xmm7, xmm7, 0x00;

		mov esi, bones;
		mov esi, ds : dword ptr[esi+edx*0x04];
		lea esi,[esi *8];
		lea esi,[esi *8+ecx];

		jmp continueSlowLoop;

endOfLoop:
		movaps ds : dword ptr[edi+eax], xmm6;

		add bones, SKM_MAX_WEIGHTS *0x04;
		add influences, SKM_MAX_WEIGHTS *0x04;

		add eax, 0x10;
		cmp eax, numverts;

		jl sl1;
done:
		;
	}
}

#define R_SkeletalTransformVerts_SSE R_SkeletalTransformVerts_MSVC
#define R_SkeletalTransformNormals_SSE R_SkeletalTransformNormals_MSVC

#pragma optimize( "", on )
#endif

#endif

#ifndef R_SkeletalTransformVerts_SSE
#define R_SkeletalTransformVerts_SSE R_SkeletalTransformVerts
#define R_SkeletalTransformNormals_SSE R_SkeletalTransformNormals
#endif

//=======================================================================

static ALIGN( 16 ) mat4x4_t relbonepose[SKM_MAX_BONES];

/*
================
R_DrawBonesFrameLerp
================
*/
static void R_DrawBonesFrameLerp( const meshbuffer_t *mb, float backlerp )
{
	unsigned int i, j, meshnum;
	int features;
	float frontlerp = 1.0 - backlerp, *pose;
	mskmesh_t *mesh;
	bonepose_t *bonepose, *oldbonepose, tempbonepose[SKM_MAX_BONES], *lerpedbonepose;
	bonepose_t *bp, *oldbp, *out, tp;
	entity_t *e = ri.currententity;
	model_t	*mod = Mod_ForHandle( mb->LODModelHandle );
	mskmodel_t *skmodel = ( mskmodel_t * )mod->extradata;
	shader_t *shader;
	mskbone_t *bone;
	vec4_t *xyzArray, *normalsArray, *sVectorsArray;

	meshnum = -( mb->infokey + 1 );
	if( meshnum >= skmodel->nummeshes )
		return;
	mesh = skmodel->meshes + meshnum;

	xyzArray = inVertsArray;
	normalsArray = inNormalsArray;
	sVectorsArray = inSVectorsArray;

	MB_NUM2SHADER( mb->shaderkey, shader );

	features = MF_NONBATCHED | shader->features;
	if( ri.params & RP_SHADOWMAPVIEW )
	{
		features &= ~( MF_COLORS|MF_SVECTORS|MF_ENABLENORMALS );
		if( !( shader->features & MF_DEFORMVS ) )
			features &= ~MF_NORMALS;
	}
	else
	{
		if( ( features & MF_SVECTORS ) || r_shownormals->integer )
			features |= MF_NORMALS;
#ifdef HARDWARE_OUTLINES
		if( e->outlineHeight )
			features |= MF_NORMALS|(glConfig.ext.GLSL ? MF_ENABLENORMALS : 0);
#endif
	}

	// not sure if it's really needed
	if( e->boneposes == skmodel->frames[0].boneposes )
	{
		e->boneposes = NULL;
		e->frame = e->oldframe = 0;
	}

	// choose boneposes for lerping
	if( e->boneposes )
	{
		bp = e->boneposes;
		if( e->oldboneposes )
			oldbp = e->oldboneposes;
		else
			oldbp = bp;
	}
	else
	{
		if( ( e->frame >= (int)skmodel->numframes ) || ( e->frame < 0 ) )
		{
#ifndef PUBLIC_BUILD
			Com_DPrintf( "R_DrawBonesFrameLerp %s: no such frame %d\n", mod->name, e->frame );
#endif
			e->frame = 0;
		}
		if( ( e->oldframe >= (int)skmodel->numframes ) || ( e->oldframe < 0 ) )
		{
#ifndef PUBLIC_BUILD
			Com_DPrintf( "R_DrawBonesFrameLerp %s: no such oldframe %d\n", mod->name, e->oldframe );
#endif
			e->oldframe = 0;
		}

		bp = skmodel->frames[e->frame].boneposes;
		oldbp = skmodel->frames[e->oldframe].boneposes;
	}

	lerpedbonepose = tempbonepose;
	if( bp == oldbp || frontlerp == 1 )
	{
		if( e->boneposes )
		{	// assume that parent transforms have already been applied
			lerpedbonepose = bp;
		}
		else
		{	// transform
			if( !e->frame )
			{	// fastpath: render frame 0 as is
				xyzArray = mesh->xyzArray;
				normalsArray = mesh->normalsArray;
				sVectorsArray = mesh->sVectorsArray;

				goto pushmesh;
			}

			for( i = 0; i < mesh->numreferences; i++ )
			{
				j = mesh->references[i];
				out = lerpedbonepose + j;
				bonepose = bp + j;
				bone = skmodel->bones + j;

				if( bone->parent >= 0 )
				{
					Quat_ConcatTransforms( lerpedbonepose[bone->parent].quat, lerpedbonepose[bone->parent].origin,
						bonepose->quat, bonepose->origin, out->quat, out->origin );
				}
				else
				{
					Quat_Copy( bonepose->quat, out->quat );
					VectorCopy( bonepose->origin, out->origin );
				}
			}
		}
	}
	else
	{
		if( e->boneposes )
		{	// lerp, assume that parent transforms have already been applied
			for( i = 0, out = lerpedbonepose, bonepose = bp, oldbonepose = oldbp, bone = skmodel->bones; i < skmodel->numbones; i++, out++, bonepose++, oldbonepose++, bone++ )
			{
				Quat_Lerp( oldbonepose->quat, bonepose->quat, frontlerp, out->quat );
				out->origin[0] = oldbonepose->origin[0] + ( bonepose->origin[0] - oldbonepose->origin[0] ) * frontlerp;
				out->origin[1] = oldbonepose->origin[1] + ( bonepose->origin[1] - oldbonepose->origin[1] ) * frontlerp;
				out->origin[2] = oldbonepose->origin[2] + ( bonepose->origin[2] - oldbonepose->origin[2] ) * frontlerp;
			}
		}
		else
		{	// lerp and transform
			for( i = 0; i < mesh->numreferences; i++ )
			{
				j = mesh->references[i];
				out = lerpedbonepose + j;
				bonepose = bp + j;
				oldbonepose = oldbp + j;
				bone = skmodel->bones + j;

				Quat_Lerp( oldbonepose->quat, bonepose->quat, frontlerp, tp.quat );
				tp.origin[0] = oldbonepose->origin[0] + ( bonepose->origin[0] - oldbonepose->origin[0] ) * frontlerp;
				tp.origin[1] = oldbonepose->origin[1] + ( bonepose->origin[1] - oldbonepose->origin[1] ) * frontlerp;
				tp.origin[2] = oldbonepose->origin[2] + ( bonepose->origin[2] - oldbonepose->origin[2] ) * frontlerp;

				if( bone->parent >= 0 )
				{
					Quat_ConcatTransforms( tempbonepose[bone->parent].quat, tempbonepose[bone->parent].origin,
						tp.quat, tp.origin, out->quat, out->origin );
				}
				else
				{
					Quat_Copy( tp.quat, out->quat );
					VectorCopy( tp.origin, out->origin );
				}
			}
		}
	}

	for( i = 0; i < mesh->numreferences; i++ )
	{
		j = mesh->references[i];
		pose = relbonepose[j];

		Quat_ConcatTransforms( lerpedbonepose[j].quat, lerpedbonepose[j].origin,
			skmodel->invbaseposes[j].quat, skmodel->invbaseposes[j].origin, tp.quat, &pose[12] );
		pose[15] = 1.0f;

		// make origin the forth column instead of row so that
		// things can be optimized more easily
		Matrix_FromQuaternion( tp.quat, pose );
	}

	if( COM_CPUFeatures() & QCPU_HAS_SSE )
	{
		R_SkeletalTransformVerts_SSE( mesh->numverts, mesh->bones, mesh->influences, relbonepose,
			( vec_t * )mesh->xyzArray[0], ( vec_t * )inVertsArray );

		if( features & MF_NORMALS )
			R_SkeletalTransformNormals_SSE( mesh->numverts, mesh->bones, mesh->influences, relbonepose,
			( vec_t * )mesh->normalsArray[0], ( vec_t * )inNormalsArray );

		if( features & MF_SVECTORS )
			R_SkeletalTransformNormals_SSE( mesh->numverts, mesh->bones, mesh->influences, relbonepose,
			( vec_t * )mesh->sVectorsArray[0], ( vec_t * )inSVectorsArray );
	}
	else
	{
		R_SkeletalTransformVerts( mesh->numverts, mesh->bones, mesh->influences, relbonepose,
			( vec_t * )mesh->xyzArray[0], ( vec_t * )inVertsArray );

		if( features & MF_NORMALS )
			R_SkeletalTransformNormals( mesh->numverts, mesh->bones, mesh->influences, relbonepose,
			( vec_t * )mesh->normalsArray[0], ( vec_t * )inNormalsArray );

		if( features & MF_SVECTORS )
			R_SkeletalTransformNormals( mesh->numverts, mesh->bones, mesh->influences, relbonepose,
			( vec_t * )mesh->sVectorsArray[0], ( vec_t * )inSVectorsArray );
	}


pushmesh:
	skm_mesh.elems = mesh->elems;
	skm_mesh.numElems = mesh->numtris * 3;
	skm_mesh.numVerts = mesh->numverts;
	skm_mesh.xyzArray = xyzArray;
	skm_mesh.stArray = mesh->stArray;
	skm_mesh.normalsArray = normalsArray;
	skm_mesh.sVectorsArray = sVectorsArray;

	R_RotateForEntity( e );

	R_PushMesh( NULL, &skm_mesh, features );
	R_RenderMeshBuffer( mb );
}

/*
=================
R_DrawSkeletalModel
=================
*/
void R_DrawSkeletalModel( const meshbuffer_t *mb )
{
	entity_t *e = ri.currententity;
	float depthmin = gldepthmin, depthmax = gldepthmax;

	if( OCCLUSION_QUERIES_ENABLED( ri ) && OCCLUSION_TEST_ENTITY( e ) )
	{
		shader_t *shader;

		MB_NUM2SHADER( mb->shaderkey, shader );
		if( !R_GetOcclusionQueryResultBool( shader->type == SHADER_PLANAR_SHADOW ? OQ_PLANARSHADOW : OQ_ENTITY,
			e - r_entities, qtrue ) )
			return;
	}

	// hack the depth range to prevent view model from poking into walls
	if( e->flags & RF_WEAPONMODEL )
		GL_DepthRange( depthmin, depthmin + 0.3 * ( depthmax - depthmin ) );

	// backface culling for left-handed weapons
	if( e->flags & RF_CULLHACK )
		GL_FrontFace( !glState.frontFace );

	if( !r_lerpmodels->integer )
		e->backlerp = 0;

	R_DrawBonesFrameLerp( mb, e->backlerp );

	if( e->flags & RF_WEAPONMODEL )
		GL_DepthRange( depthmin, gldepthmax );

	if( e->flags & RF_CULLHACK )
		GL_FrontFace( !glState.frontFace );
}

/*
=================
R_SkeletalModelBBox
=================
*/
float R_SkeletalModelBBox( entity_t *e, vec3_t mins, vec3_t maxs )
{
	model_t	*mod;

	mod = R_SkeletalModelLOD( e );
	if( !mod )
		return 0;

	R_SkeletalModelLerpBBox( e, mod );

	VectorCopy( skm_mins, mins );
	VectorCopy( skm_maxs, maxs );
	return skm_radius;
}

/*
=================
R_CullSkeletalModel
=================
*/
qboolean R_CullSkeletalModel( entity_t *e )
{
	int i, clipped;
	qboolean frustum, query;
	unsigned int modhandle;
	model_t	*mod;
	shader_t *shader;
	mskmesh_t *mesh;
	mskmodel_t *skmodel;
	meshbuffer_t *mb;

	mod = R_SkeletalModelLOD( e );
	if( !( skmodel = ( ( mskmodel_t * )mod->extradata ) ) || !skmodel->nummeshes )
		return qtrue;

	R_SkeletalModelLerpBBox( e, mod );
	modhandle = Mod_Handle( mod );

	clipped = R_CullModel( e, skm_mins, skm_maxs, skm_radius );
	frustum = clipped & 1;
	if( clipped & 2 )
		return qtrue;

	query =  OCCLUSION_QUERIES_ENABLED( ri ) && OCCLUSION_TEST_ENTITY( e ) ? qtrue : qfalse;
	if( !frustum && query )
	{
		R_IssueOcclusionQuery( R_GetOcclusionQueryNum( OQ_ENTITY, e - r_entities ), e, skm_mins, skm_maxs );
	}

	if( ri.refdef.rdflags & RDF_NOWORLDMODEL
		|| ( r_shadows->integer != SHADOW_PLANAR && !( r_shadows->integer == SHADOW_MAPPING && ( e->flags & RF_PLANARSHADOW ) ) )
		|| R_CullPlanarShadow( e, skm_mins, skm_maxs, query ) )
		return frustum; // entity is not in PVS or shadow is culled away by frustum culling

	for( i = 0, mesh = skmodel->meshes; i < (int)skmodel->nummeshes; i++, mesh++ )
	{
		shader = NULL;
		if( e->customSkin )
			shader = R_FindShaderForSkinFile( e->customSkin, mesh->name );
		else if( e->customShader )
			shader = e->customShader;
		else if( mesh->skin.shader )
			shader = mesh->skin.shader;

		if( shader && ( shader->sort <= SHADER_SORT_ALPHATEST ) )
		{
			mb = R_AddMeshToList( MB_MODEL, NULL, R_PlanarShadowShader(), -( i+1 ), NULL, 0, 0 );
			if( mb )
				mb->LODModelHandle = modhandle;
		}
	}

	return frustum;
}

/*
=================
R_AddSkeletalModelToList
=================
*/
void R_AddSkeletalModelToList( entity_t *e )
{
	int i;
	unsigned int modhandle, entnum = e - r_entities;
	mfog_t *fog = NULL;
	model_t	*mod;
	shader_t *shader;
	mskmesh_t *mesh;
	mskmodel_t *skmodel;

	mod = R_SkeletalModelLOD( e );
	skmodel = ( mskmodel_t * )mod->extradata;
	modhandle = Mod_Handle( mod );

	if( ri.params & RP_SHADOWMAPVIEW )
	{
		if( r_entShadowBits[entnum] & ri.shadowGroup->bit )
		{
			if( !r_shadows_self_shadow->integer )
				r_entShadowBits[entnum] &= ~ri.shadowGroup->bit;
			if( e->flags & RF_WEAPONMODEL )
				return;
		}
		else
		{
			R_SkeletalModelLerpBBox( e, mod );
			if( !R_CullModel( e, skm_mins, skm_maxs, skm_radius ) )
				r_entShadowBits[entnum] |= ri.shadowGroup->bit;
			return; // mark as shadowed, proceed with caster otherwise
		}
	}
	else
	{
		fog = R_FogForSphere( e->origin, skm_radius );
#if 0
		if( !( e->flags & RF_WEAPONMODEL ) && fog )
		{
			R_SkeletalModelLerpBBox( e, mod );
			if( R_CompletelyFogged( fog, e->origin, skm_radius ) )
				return;
		}
#endif
	}

	for( i = 0, mesh = skmodel->meshes; i < (int)skmodel->nummeshes; i++, mesh++ )
	{
		shader = NULL;
		if( e->customSkin )
			shader = R_FindShaderForSkinFile( e->customSkin, mesh->name );
		else if( e->customShader )
			shader = e->customShader;
		else
			shader = mesh->skin.shader;

		if( shader )
			R_AddModelMeshToList( modhandle, fog, shader, i );
	}
}
