 /*
Copyright (C) 2002-2011 Victor Luchits

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
#include "iqm.h"

// typedefs
typedef struct iqmheader iqmheader_t;
typedef struct iqmvertexarray iqmvertexarray_t;
typedef struct iqmjoint iqmjoint_t;
typedef struct iqmpose iqmpose_t;
typedef struct iqmmesh iqmmesh_t;
typedef struct iqmbounds iqmbounds_t;


static mesh_t skm_mesh;

static vec3_t skm_mins;
static vec3_t skm_maxs;
static float skm_radius;

/*
==============================================================================

IQM MODELS

==============================================================================
*/

/*
* Mod_SkeletalBuildStaticVBOForMesh
* 
* Builds a static vertex buffer object for given skeletal model mesh
*/
static void Mod_SkeletalBuildStaticVBOForMesh( mskmesh_t *mesh )
{
	mesh_t skmmesh;

	mesh->vbo = R_CreateStaticMeshVBO( ( void * )mesh, 
		mesh->numverts, mesh->numtris * 3, MF_STCOORDS | (mesh->sVectorsArray ? MF_SVECTORS : 0) | MF_BONES, VBO_TAG_MODEL );

	if( !mesh->vbo )
		return;

	skmmesh.elems = mesh->elems;
	skmmesh.numElems = mesh->numtris * 3;
	skmmesh.numVerts = mesh->numverts;

	skmmesh.xyzArray = mesh->xyzArray;
	skmmesh.stArray = mesh->stArray;
	skmmesh.normalsArray = mesh->normalsArray;
	skmmesh.sVectorsArray = mesh->sVectorsArray;

	R_UploadVBOVertexData( mesh->vbo, 0, &skmmesh ); 
	R_UploadVBOElemData( mesh->vbo, 0, 0, &skmmesh );
	R_UploadVBOBonesData( mesh->vbo, 0, mesh->numverts, mesh->blendIndices, mesh->blendWeights );
}

/*
* Mod_TouchSkeletalModel
*/
static void Mod_TouchSkeletalModel( model_t *mod )
{
	unsigned int i;
	mskmesh_t *mesh;
	mskskin_t *skin;
	mskmodel_t *skmodel = ( mskmodel_t * )mod->extradata;

	mod->registration_sequence = r_front.registration_sequence;

	for( i = 0, mesh = skmodel->meshes; i < skmodel->nummeshes; i++, mesh++ ) {
		// register needed skins and images
		skin = &mesh->skin;
		if( skin->shader ) {
			R_TouchShader( skin->shader );
		}
		if( mesh->vbo ) {
			R_TouchMeshVBO( mesh->vbo );
		}
	}
}

/*
* Mod_SkeletalModel_AddBlend
* 
* If there's only one influencing bone, return its index early.
* Otherwise lookup identical blending combination.
*/
static int Mod_SkeletalModel_AddBlend( mskmodel_t *model, const mskblend_t *newblend )
{
	unsigned int i;
	mskblend_t *blends;

	if( !newblend->weights[1] ) {
		return newblend->indices[0];
	}

	for( i = 0, blends = model->blends; i < model->numblends; i++, blends++ ) {
		if( !memcmp( blends, newblend, sizeof( mskblend_t ) ) ) {
			return model->numbones + i;
		}
	}

	model->numblends++;
	memcpy( blends, newblend, sizeof( mskblend_t ) );

	return model->numbones + i;
}

/*
* Mod_LoadSkeletalModel
*/
void Mod_LoadSkeletalModel( model_t *mod, const model_t *parent, void *buffer, bspFormatDesc_t *unused )
{
	unsigned int i, j, k;
	size_t filesize;
	qbyte *pbase;
	size_t memsize;
	qbyte *pmem;
	iqmheader_t *header;
	char *texts;
	iqmvertexarray_t *va;
	iqmjoint_t *joints;
	bonepose_t *baseposes;
	iqmpose_t *poses;
	unsigned short *framedata;
	const int *inelems;
	elem_t *outelems;
	iqmmesh_t *inmesh;
	iqmbounds_t *inbounds;
	float *vposition, *vtexcoord, *vnormal, *vtangent;
	qbyte *vblendindexes, *vblendweights;
	mskmodel_t *poutmodel;

	header = ( iqmheader_t * )buffer;

	// check IQM magic
	if( memcmp( header->magic, "INTERQUAKEMODEL", 16 ) ) {
		Com_Error( ERR_DROP, "%s is not an Inter-Quake Model", mod->name );
	}

	// check header version
	header->version = LittleLong( header->version );
	if( header->version != IQM_VERSION ) {
		Com_Error( ERR_DROP, "%s has wrong type number (%i should be %i)", mod->name, header->version, IQM_VERSION );
	}

	// byteswap header
#define H_SWAP(s) (header->s = LittleLong( header->s ))
	H_SWAP( filesize );
	H_SWAP( flags );
	H_SWAP( num_text );
	H_SWAP( ofs_text );
	H_SWAP( num_meshes );
	H_SWAP( ofs_meshes );
	H_SWAP( num_vertexarrays );
	H_SWAP( num_vertexes );
	H_SWAP( ofs_vertexarrays );
	H_SWAP( num_triangles );
	H_SWAP( ofs_triangles );
	H_SWAP( ofs_adjacency );
	H_SWAP( num_joints );
	H_SWAP( ofs_joints );
	H_SWAP( num_poses );
	H_SWAP( ofs_poses );
	H_SWAP( num_anims );
	H_SWAP( ofs_anims );
	H_SWAP( num_frames );
	H_SWAP( num_framechannels );
	H_SWAP( ofs_frames );
	H_SWAP( ofs_bounds );
	H_SWAP( num_comment );
	H_SWAP( ofs_comment );
	H_SWAP( num_extensions );
	H_SWAP( ofs_extensions );
#undef H_SWAP

	if( header->num_triangles < 1 || header->num_vertexes < 3 || header->num_vertexarrays < 1 || header->num_meshes < 1 ) {
		Com_Error( ERR_DROP, "%s has no geometry", mod->name );
	}
	if( header->num_frames < 1 || header->num_anims < 1 ) {
		Com_Error( ERR_DROP, "%s has no animations", mod->name );
	}
	if( header->num_joints != header->num_poses ) {
		Com_Error( ERR_DROP, "%s has an invalid number of poses: %i vs %i", mod->name, header->num_joints, header->num_poses );
	}
	if( !header->ofs_bounds ) {
		Com_Error( ERR_DROP, "%s has no frame bounds", mod->name );
	}

	pbase = ( qbyte * )buffer;
	filesize = header->filesize;

	// check data offsets against the filesize
	if( header->ofs_text + header->num_text > filesize
		|| header->ofs_vertexarrays + header->num_vertexarrays * sizeof( iqmvertexarray_t ) > filesize
		|| header->ofs_joints + header->num_joints * sizeof( iqmjoint_t ) > filesize
		|| header->ofs_frames + header->num_frames * header->num_framechannels * sizeof( unsigned short ) > filesize
		|| header->ofs_triangles + header->num_triangles * sizeof( int[3] ) > filesize
		|| header->ofs_meshes + header->num_meshes * sizeof( iqmmesh_t ) > filesize
		|| header->ofs_bounds + header->num_frames * sizeof( iqmbounds_t ) > filesize
		) {
		Com_Error( ERR_DROP, "%s has invalid size or offset information", mod->name );
	}

	poutmodel = mod->extradata = Mod_Malloc( mod, sizeof( *poutmodel ) );


	// load text
	texts = Mod_Malloc( mod, header->num_text + 1 );
	if( header->ofs_text ) {
		memcpy( texts, (const char *)(pbase + header->ofs_text), header->num_text );
	}
	texts[header->ofs_text] = '\0';


	// load vertex arrays
	vposition = NULL;
	vtexcoord = NULL;
	vnormal = NULL;
	vtangent = NULL;
	vblendindexes = NULL;
	vblendweights = NULL;

	va = ( iqmvertexarray_t * )( pbase + header->ofs_vertexarrays );
	for( i = 0; i < header->num_vertexarrays; i++ ) {
		size_t vsize;

		va[i].type = LittleLong( va[i].type );
		va[i].flags = LittleLong( va[i].flags );
		va[i].format = LittleLong( va[i].format );
		va[i].size = LittleLong( va[i].size );
		va[i].offset = LittleLong( va[i].offset );

		vsize = header->num_vertexes*va[i].size;
		switch( va[i].format ) { 
			case IQM_FLOAT:
				vsize *= sizeof( float );
				break;
			case IQM_UBYTE:
				vsize *= sizeof( unsigned char );
				break;
			default:
				continue;
		}

		if( va[i].offset + vsize > filesize ) {
			continue;
		}

		switch( va[i].type ) {
			case IQM_POSITION:
				if( va[i].format == IQM_FLOAT && va[i].size == 3 ) {
					vposition = ( float * )( pbase + va[i].offset );
				}
				break;
			case IQM_TEXCOORD:
				if( va[i].format == IQM_FLOAT && va[i].size == 2 ) {
					vtexcoord = ( float * )( pbase + va[i].offset );
				}
				break;
			case IQM_NORMAL:
				if( va[i].format == IQM_FLOAT && va[i].size == 3 ) {
					vnormal = ( float * )( pbase + va[i].offset );
				}
				break;
			case IQM_TANGENT:
				if( va[i].format == IQM_FLOAT && va[i].size == 4 ) {
					vtangent = ( float * )( pbase + va[i].offset );
				}
				break;
			case IQM_BLENDINDEXES:
				if( va[i].format == IQM_UBYTE && va[i].size == SKM_MAX_WEIGHTS ) {
					vblendindexes = ( qbyte * )( pbase + va[i].offset );
				}
				break;
			case IQM_BLENDWEIGHTS:
				if( va[i].format == IQM_UBYTE && va[i].size == SKM_MAX_WEIGHTS ) {
					vblendweights = ( qbyte * )( pbase + va[i].offset );
				}
				break;
		}
	}

	if( !vposition || !vtexcoord || !vblendindexes || !vblendweights ) {
		Com_Error( ERR_DROP, "%s is missing vertex array data", mod->name );
	}

	// load joints
	memsize = 0;
	memsize += sizeof( bonepose_t ) * header->num_joints;
	pmem = Mod_Malloc( mod, memsize );

	baseposes = ( void * )pmem; pmem += sizeof( *baseposes );

	memsize = 0;
	memsize += sizeof( mskbone_t ) * header->num_joints;
	memsize += sizeof( bonepose_t ) * header->num_joints;
	pmem = Mod_Malloc( mod, memsize );

	poutmodel->numbones = header->num_joints;
	poutmodel->bones = ( void * )pmem; pmem += sizeof( *poutmodel->bones ) * poutmodel->numbones;
	poutmodel->invbaseposes = ( void * )pmem; pmem += sizeof( *poutmodel->invbaseposes ) * poutmodel->numbones;

	joints = ( iqmjoint_t * )( pbase + header->ofs_joints );
	for( i = 0; i < poutmodel->numbones; i++ ) {
		joints[i].name = LittleLong( joints[i].name );
		joints[i].parent = LittleLong( joints[i].parent );

		for( j = 0; j < 3; j++ ) {
			joints[i].translate[j] = LittleFloat( joints[i].translate[j] );
			joints[i].rotate[j] = LittleFloat( joints[i].rotate[j] );
			joints[i].scale[j] = LittleFloat( joints[i].scale[j] );
		}

		if( joints[i].parent >= (int)i ) {
			Com_Error( ERR_DROP, "%s bone[%i].parent(%i) >= %i", mod->name, i, joints[i].parent, i );
		}

		poutmodel->bones[i].name = texts + joints[i].name;
		poutmodel->bones[i].parent = joints[i].parent;

		DualQuat_FromQuat3AndVector( joints[i].rotate, joints[i].translate, baseposes[i].dualquat );

		// scale is unused

		// reconstruct invserse bone pose

		if( joints[i].parent >= 0 )
		{
			bonepose_t bp, *pbp;
			bp = baseposes[i];
			pbp = &baseposes[joints[i].parent];

			DualQuat_Multiply( pbp->dualquat, bp.dualquat, baseposes[i].dualquat );
		}

		DualQuat_Copy( baseposes[i].dualquat, poutmodel->invbaseposes[i].dualquat );
		DualQuat_Invert( poutmodel->invbaseposes[i].dualquat );
	}


	// load frames
	poses = ( iqmpose_t * )( pbase + header->ofs_poses );
	for( i = 0; i < header->num_poses; i++ ) {
		poses[i].parent = LittleLong( poses[i].parent );
		poses[i].mask = LittleLong( poses[i].mask );

		for( j = 0; j < 10; j++ ) {
			poses[i].channeloffset[j] = LittleFloat( poses[i].channeloffset[j] );
			poses[i].channelscale[j] = LittleFloat( poses[i].channelscale[j] );
		}
	}

	memsize = 0;
	memsize += sizeof( mskframe_t ) * header->num_frames;
	memsize += sizeof( bonepose_t ) * header->num_joints * header->num_frames;
	pmem = Mod_Malloc( mod, memsize );

	poutmodel->numframes = header->num_frames;
	poutmodel->frames = ( mskframe_t * )pmem; pmem += sizeof( mskframe_t ) * poutmodel->numframes;

	framedata = ( unsigned short * )( pbase + header->ofs_frames );
	for( i = 0; i < header->num_frames; i++ ) {
		bonepose_t *pbp;
		vec3_t translate;
		quat_t rotate;

		poutmodel->frames[i].boneposes = ( bonepose_t * )pmem; pmem += sizeof( bonepose_t ) * poutmodel->numbones;

		for( j = 0, pbp = poutmodel->frames[i].boneposes; j < header->num_poses; j++, pbp++ ) {
			translate[0] = poses[j].channeloffset[0]; if( poses[j].mask & 0x01 ) translate[0] += *framedata++ * poses[j].channelscale[0];
			translate[1] = poses[j].channeloffset[1]; if( poses[j].mask & 0x02 ) translate[1] += *framedata++ * poses[j].channelscale[1];
			translate[2] = poses[j].channeloffset[2]; if( poses[j].mask & 0x04 ) translate[2] += *framedata++ * poses[j].channelscale[2];

			rotate[0] = poses[j].channeloffset[3]; if( poses[j].mask & 0x08 ) rotate[0] += *framedata++ * poses[j].channelscale[3];
			rotate[1] = poses[j].channeloffset[4]; if( poses[j].mask & 0x10 ) rotate[1] += *framedata++ * poses[j].channelscale[4];
			rotate[2] = poses[j].channeloffset[5]; if( poses[j].mask & 0x20 ) rotate[2] += *framedata++ * poses[j].channelscale[5];
			rotate[3] = poses[j].channeloffset[6]; if( poses[j].mask & 0x40 ) rotate[3] += *framedata++ * poses[j].channelscale[6];
			if( rotate[3] > 0 ) {
				Vector4Inverse( rotate );
			}
			Vector4Normalize( rotate );

			// scale is unused
			if( poses[j].mask & 0x80  ) framedata++;
			if( poses[j].mask & 0x100 ) framedata++;
			if( poses[j].mask & 0x200 ) framedata++;

			DualQuat_FromQuatAndVector( rotate, translate, pbp->dualquat );
		}
	}


	// load triangles
	memsize = 0;
	memsize += sizeof( *outelems ) * header->num_triangles * 3;
	pmem = Mod_Malloc( mod, memsize );

	poutmodel->numtris = header->num_triangles;
	poutmodel->elems = ( elem_t * )pmem; pmem += sizeof( *outelems ) * header->num_triangles * 3;

	inelems = ( const int * )(pbase + header->ofs_triangles);
	outelems = poutmodel->elems;

	for( i = 0; i < header->num_triangles; i++ ) {
		for( j = 0; j < 3; j++ ) {
			outelems[j] = LittleLong( inelems[j] );
		}
		inelems += 3;
		outelems += 3;
	}


	// load vertices
	memsize = 0;
	memsize += sizeof( *poutmodel->xyzArray ) * header->num_vertexes;		// 16-bytes aligned
	memsize += sizeof( *poutmodel->normalsArray ) * header->num_vertexes;	// 16-bytes aligned
	if( glConfig.ext.GLSL ) {
		memsize += sizeof( *poutmodel->sVectorsArray ) * header->num_vertexes;	// 16-bytes aligned
	}
	memsize += sizeof( *poutmodel->stArray ) * header->num_vertexes;
	memsize += sizeof( *poutmodel->blendWeights ) * header->num_vertexes * SKM_MAX_WEIGHTS;
	memsize += sizeof( *poutmodel->blendIndices ) * header->num_vertexes * SKM_MAX_WEIGHTS;
	pmem = Mod_Malloc( mod, memsize );

	poutmodel->numverts = header->num_vertexes;

	// XYZ positions
	poutmodel->xyzArray = ( vec4_t * )pmem; pmem += sizeof( *poutmodel->xyzArray ) * header->num_vertexes;
	for( i = 0; i < header->num_vertexes; i++ ) {
		for( j = 0; j < 3; j++ ) {
			poutmodel->xyzArray[i][j] = LittleFloat( vposition[j] );
		}
		poutmodel->xyzArray[i][3] = 1;
		vposition += 3;
	}

	// normals
	poutmodel->normalsArray = ( vec4_t * )pmem; pmem += sizeof( *poutmodel->normalsArray ) * header->num_vertexes;
	for( i = 0; i < header->num_vertexes; i++ ) {
		for( j = 0; j < 3; j++ ) {
			poutmodel->normalsArray[i][j] = LittleFloat( vnormal[j] );
		}
		poutmodel->normalsArray[i][3] = 0;
		vnormal += 3;
	}

	// S-vectors
	if( glConfig.ext.GLSL ) {
		poutmodel->sVectorsArray = ( vec4_t * )pmem; pmem += sizeof( *poutmodel->sVectorsArray ) * header->num_vertexes;

		if( vtangent ) {
			for( i = 0; i < header->num_vertexes; i++ ) {
				for( j = 0; j < 4; j++ ) {
					poutmodel->sVectorsArray[i][j] = LittleFloat( vtangent[j] );
				}
				vtangent += 4;
			}
		}
	}

	// texture coordinates
	poutmodel->stArray = ( vec2_t * )pmem; pmem += sizeof( *poutmodel->stArray ) * header->num_vertexes;
	for( i = 0; i < header->num_vertexes; i++ ) {
		for( j = 0; j < 2; j++ ) {
			poutmodel->stArray[i][j] = LittleFloat( vtexcoord[j] );
		}
		vtexcoord += 2;
	}

	if( glConfig.ext.GLSL && !vtangent ) {
		// if the loaded file is missing precomputed S-vectors, compute them now
		R_BuildTangentVectors( poutmodel->numverts, poutmodel->xyzArray, poutmodel->normalsArray, poutmodel->stArray, 
			poutmodel->numtris, poutmodel->elems, poutmodel->sVectorsArray );
	}

	// blend indices
	poutmodel->blendIndices = ( qbyte * )pmem; pmem += sizeof( *poutmodel->blendIndices ) * header->num_vertexes * SKM_MAX_WEIGHTS;
	memcpy( poutmodel->blendIndices, vblendindexes, sizeof( qbyte ) * header->num_vertexes * SKM_MAX_WEIGHTS );

	// blend weights
	poutmodel->blendWeights = ( qbyte * )pmem; pmem += sizeof( *poutmodel->blendWeights ) * header->num_vertexes * SKM_MAX_WEIGHTS;
	memcpy( poutmodel->blendWeights, vblendweights, sizeof( qbyte ) * header->num_vertexes * SKM_MAX_WEIGHTS );


	// blends
	memsize = 0;
	memsize += poutmodel->numverts * ( sizeof( mskblend_t ) + sizeof( unsigned int ) );
	pmem = Mod_Malloc( mod, memsize );

	poutmodel->numblends = 0;
	poutmodel->blends = ( mskblend_t * )pmem; pmem += sizeof( *poutmodel->blends ) * poutmodel->numverts;
	poutmodel->vertexBlends = ( unsigned int * )pmem;

	for( i = 0; i < poutmodel->numverts; i++ ) {
		mskblend_t blend;

		for( j = 0; j < SKM_MAX_WEIGHTS; j++ ) {
			blend.indices[j] = vblendindexes[j];
			blend.weights[j] = vblendweights[j];
		}
		poutmodel->vertexBlends[i] = Mod_SkeletalModel_AddBlend( poutmodel, &blend );

		vblendindexes += SKM_MAX_WEIGHTS;
		vblendweights += SKM_MAX_WEIGHTS;
	}

	// meshes
	memsize = 0;
	memsize += sizeof( mskmesh_t ) * header->num_meshes;
	pmem = Mod_Malloc( mod, memsize );

	poutmodel->nummeshes = header->num_meshes;
	poutmodel->meshes = ( mskmesh_t * )pmem; pmem += sizeof( *poutmodel->meshes ) * header->num_meshes;

	inmesh = ( iqmmesh_t * )(pbase + header->ofs_meshes);
	for( i = 0; i < header->num_meshes; i++ ) {
		inmesh[i].name = LittleLong( inmesh[i].name );
		inmesh[i].material = LittleLong( inmesh[i].material );
		inmesh[i].first_vertex = LittleLong( inmesh[i].first_vertex );
		inmesh[i].num_vertexes = LittleLong( inmesh[i].num_vertexes );
		inmesh[i].first_triangle = LittleLong( inmesh[i].first_triangle );
		inmesh[i].num_triangles = LittleLong( inmesh[i].num_triangles );

		poutmodel->meshes[i].name = texts + inmesh[i].name;
		Mod_StripLODSuffix( poutmodel->meshes[i].name );

		poutmodel->meshes[i].skin.name = texts + inmesh[i].material;
		poutmodel->meshes[i].skin.shader = R_RegisterSkin( poutmodel->meshes[i].skin.name );

		poutmodel->meshes[i].elems = poutmodel->elems + inmesh[i].first_triangle * 3;
		poutmodel->meshes[i].numtris = inmesh[i].num_triangles;

		poutmodel->meshes[i].numverts = inmesh[i].num_vertexes;
		poutmodel->meshes[i].xyzArray = poutmodel->xyzArray + inmesh[i].first_vertex;
		poutmodel->meshes[i].normalsArray = poutmodel->normalsArray + inmesh[i].first_vertex;
		poutmodel->meshes[i].stArray = poutmodel->stArray + inmesh[i].first_vertex;
		if( glConfig.ext.GLSL ) {
			poutmodel->meshes[i].sVectorsArray = poutmodel->sVectorsArray + inmesh[i].first_vertex;
		}

		poutmodel->meshes[i].blendIndices = poutmodel->blendIndices + inmesh[i].first_vertex * SKM_MAX_WEIGHTS;
		poutmodel->meshes[i].blendWeights = poutmodel->blendWeights + inmesh[i].first_vertex * SKM_MAX_WEIGHTS;

		poutmodel->meshes[i].vertexBlends = poutmodel->vertexBlends + inmesh[i].first_vertex;

		// elements are always offset to start vertex 0 for each mesh
		outelems = poutmodel->meshes[i].elems;
		for( j = 0; j < poutmodel->meshes[i].numtris; j++ ) {
			outelems[0] -= inmesh[i].first_vertex;
			outelems[1] -= inmesh[i].first_vertex;
			outelems[2] -= inmesh[i].first_vertex;
			outelems += 3;
		}

		poutmodel->meshes[i].maxWeights = 1;
		for( j = 0, vblendweights = poutmodel->meshes[i].blendWeights; j < poutmodel->meshes[i].numverts; j++, vblendweights += SKM_MAX_WEIGHTS ) {
			for( k = 1; k < SKM_MAX_WEIGHTS && vblendweights[k]; k++ );

			if( k > poutmodel->meshes[i].maxWeights ) {
				poutmodel->meshes[i].maxWeights = k;
				if( k == SKM_MAX_WEIGHTS ) {
					break;
				}
			}
		}

		// creating a VBO only makes sense if GLSL is present and the number of bones 
		// we can handle on the GPU is sufficient
		if( glConfig.ext.vertex_buffer_object && poutmodel->numbones <= glConfig.maxGLSLBones ) {
			// build a static vertex buffer object for this mesh
			Mod_SkeletalBuildStaticVBOForMesh( &poutmodel->meshes[i] );
		}
	}

	// bounds
	ClearBounds( mod->mins, mod->maxs );

	inbounds = ( iqmbounds_t * )(pbase + header->ofs_bounds);
	for( i = 0; i < header->num_frames; i++ ) {
		for( j = 0; j < 3; j++ ) {
			inbounds[i].bbmin[j] = LittleFloat( inbounds[i].bbmin[j] );
			inbounds[i].bbmax[j] = LittleFloat( inbounds[i].bbmax[j] );
		}
		inbounds[i].radius = LittleFloat( inbounds[i].radius );
		inbounds[i].xyradius = LittleFloat( inbounds[i].xyradius );

		VectorCopy( inbounds[i].bbmin, poutmodel->frames[i].mins );
		VectorCopy( inbounds[i].bbmax, poutmodel->frames[i].maxs );
		poutmodel->frames[i].radius = inbounds[i].radius;

		AddPointToBounds( poutmodel->frames[i].mins, mod->mins, mod->maxs );
		AddPointToBounds( poutmodel->frames[i].maxs, mod->mins, mod->maxs );
	}

	mod->radius = RadiusFromBounds( mod->mins, mod->maxs );
	mod->type = mod_skeletal;
	mod->registration_sequence = r_front.registration_sequence;
	mod->touch = &Mod_TouchSkeletalModel;

	Mem_Free( baseposes );
}

/*
* R_SkeletalGetNumBones
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
* R_SkeletalGetBoneInfo
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
* R_SkeletalGetBonePose
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
* R_SkeletalModelLOD
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
* R_SkeletalModelLerpBBox
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

typedef struct skmcacheentry_s
{
	size_t size;
	qbyte *data;
	struct skmcacheentry_s *next;
} skmcacheentry_t;

mempool_t *r_skmcachepool;

static skmcacheentry_t *r_skmcache_head;	// actual entries are linked to this
static skmcacheentry_t *r_skmcache_free;	// actual entries are linked to this
static skmcacheentry_t *r_skmcachekeys[MAX_ENTITIES*(MOD_MAX_LODS+1)];		// entities linked to cache entries

#define R_SKMCacheAlloc(size) Mem_Alloc(r_skmcachepool, (size))

/*
* R_InitSkeletalCache
*/
void R_InitSkeletalCache( void )
{
	r_skmcachepool = Mem_AllocPool( NULL, "SKM Cache" );

	r_skmcache_head = NULL;
	r_skmcache_free = NULL;
}

/*
* R_SkeletalModelLerpBBox
*/
static qbyte *R_GetSketalCache( int entNum, int lodNum )
{
	skmcacheentry_t *cache;
	
	cache = r_skmcachekeys[entNum*(MOD_MAX_LODS+1) + lodNum];
	if( !cache ) {
		return NULL;
	}
	return cache->data;
}

/*
* R_AllocSkeletalDataCache
* 
* Allocates or reuses a memory chunk and links it to entity+LOD num pair. The chunk
* is then linked to other chunks allocated in the same frame. At the end of the frame
* all of the entries in the "allocation" list are moved to the "free" list, to be reused in the 
* later function calls.
*/
static qbyte *R_AllocSkeletalDataCache( int entNum, int lodNum, size_t size )
{
	size_t best_size;
	skmcacheentry_t *cache, *prev;
	skmcacheentry_t *best_prev, *best;

	best = NULL;
	best_prev = NULL;
	best_size = 0;

	assert( !r_skmcachekeys[entNum * (MOD_MAX_LODS+1) + lodNum] );

	// scan the list of free cache entries to see if there's a suitable candidate
	prev = NULL;
	cache = r_skmcache_free;
	while( cache ) {
		if( cache->size >= size ) {
			// keep track of the cache entry with the minimal overhead
			if( !best || cache->size < best_size ) {
				best_size = cache->size;
				best = cache;
				best_prev = prev;
			}
		}

		// return early if we find a perfect fit
		if( cache->size == size ) {
			break;
		}

		prev = cache;
		cache = cache->next;
	}

	// no suitable entries found, allocate
	if( !best ) {
		best = R_SKMCacheAlloc( sizeof( *best ) );
		best->data = R_SKMCacheAlloc( size );
		best->size = size;
		best_prev = NULL;
	}

	assert( best->size >= size );

	// unlink this cache entry from the current list
	if( best_prev ) {
		best_prev->next = best->next;
	}
	if( best == r_skmcache_free ) {
		r_skmcache_free = best->next;
	}

	// and link it to the allocation list
	best->next = r_skmcache_head;
	r_skmcache_head = best;
	r_skmcachekeys[entNum * (MOD_MAX_LODS+1) + lodNum] = best;

	return best->data;
}

/*
* R_ClearSkeletalCache
* 
* Remove entries from the "allocation" list to the "free" list.
* FIXME: this can probably be optimized a bit better.
*/
void R_ClearSkeletalCache( void )
{
	skmcacheentry_t *next, *cache;

	cache = r_skmcache_head;
	while( cache ) {
		next = cache->next;

		cache->next = r_skmcache_free;
		r_skmcache_free = cache;

		cache = next;
	}
	r_skmcache_head = NULL;

	memset( r_skmcachekeys, 0, sizeof( r_skmcachekeys ) );
}

/*
* R_ShutdownSkeletalCache
*/
void R_ShutdownSkeletalCache( void )
{
	if( !r_skmcachepool )
		return;

	Mem_FreePool( &r_skmcachepool );

	r_skmcache_head = NULL;
	r_skmcache_free = NULL;
}

//=======================================================================

// set the FP precision to fast
#if defined ( _WIN32 ) && ( _MSC_VER >= 1400 ) && defined( NDEBUG )
# pragma float_control(except, off, push)
# pragma float_control(precise, off, push)
# pragma fp_contract(on)		// this line is needed on Itanium processors
#endif

/*
* R_SkeletalBlendPoses
*/
static void R_SkeletalBlendPoses( unsigned int numblends, mskblend_t *blends, unsigned int numbones, mat4x4_t *relbonepose )
{
	unsigned int i, j, k;
	float *pose;
	mskblend_t *blend;

	for( i = 0, j = numbones, blend = blends; i < numblends; i++, j++, blend++ ) {
		float *b, f;

		pose = relbonepose[j];

		b = relbonepose[blend->indices[0]];
		f = blend->weights[0] * (1.0 / 255.0);

		pose[ 0] = f * b[ 0]; pose[ 1] = f * b[ 1]; pose[ 2] = f * b[ 2];
		pose[ 4] = f * b[ 4]; pose[ 5] = f * b[ 5]; pose[ 6] = f * b[ 6];
		pose[ 8] = f * b[ 8]; pose[ 9] = f * b[ 9]; pose[10] = f * b[10];
		pose[12] = f * b[12]; pose[13] = f * b[13]; pose[14] = f * b[14];

		for( k = 1; k < SKM_MAX_WEIGHTS && blend->weights[k]; k++ ) {
			b = relbonepose[blend->indices[k]];
			f = blend->weights[k] * (1.0 / 255.0);

			pose[ 0] += f * b[ 0]; pose[ 1] += f * b[ 1]; pose[ 2] += f * b[ 2];
			pose[ 4] += f * b[ 4]; pose[ 5] += f * b[ 5]; pose[ 6] += f * b[ 6];
			pose[ 8] += f * b[ 8]; pose[ 9] += f * b[ 9]; pose[10] += f * b[10];
			pose[12] += f * b[12]; pose[13] += f * b[13]; pose[14] += f * b[14];
		}
	}
}

/*
* R_SkeletalTransformVerts
*/
static void R_SkeletalTransformVerts( int numverts, const unsigned int *blends, mat4x4_t *relbonepose, const vec_t *v, vec_t *ov )
{
	const float *pose;

	for( ; numverts; numverts--, v += 4, ov += 4, blends++ ) {
		pose = relbonepose[*blends];

		ov[0] = v[0] * pose[0] + v[1] * pose[4] + v[2] * pose[ 8] + pose[12];
		ov[1] = v[0] * pose[1] + v[1] * pose[5] + v[2] * pose[ 9] + pose[13];
		ov[2] = v[0] * pose[2] + v[1] * pose[6] + v[2] * pose[10] + pose[14];
	}
}

/*
* R_SkeletalTransformNormals
*/
static void R_SkeletalTransformNormals( int numverts, const unsigned int *blends, mat4x4_t *relbonepose, const vec_t *v, vec_t *ov )
{
	const float *pose;

	for( ; numverts; numverts--, v += 4, ov += 4, blends++ ) {
		pose = relbonepose[*blends];

		ov[0] = v[0] * pose[0] + v[1] * pose[4] + v[2] * pose[ 8];
		ov[1] = v[0] * pose[1] + v[1] * pose[5] + v[2] * pose[ 9];
		ov[2] = v[0] * pose[2] + v[1] * pose[6] + v[2] * pose[10];
		ov[3] = v[3];
	}
}

/*
* R_SkeletalTransformNormalsAndSVecs
*/
static void R_SkeletalTransformNormalsAndSVecs( int numverts, const unsigned int *blends, mat4x4_t *relbonepose, const vec_t *v, vec_t *ov, const vec_t *sv, vec_t *osv )
{
	const float *pose;

	for( ; numverts; numverts--, v += 4, ov += 4, sv += 4, osv += 4, blends++ ) {
		pose = relbonepose[*blends];

		ov[0] = v[0] * pose[0] + v[1] * pose[4] + v[2] * pose[ 8];
		ov[1] = v[0] * pose[1] + v[1] * pose[5] + v[2] * pose[ 9];
		ov[2] = v[0] * pose[2] + v[1] * pose[6] + v[2] * pose[10];
		ov[3] = v[3];

		osv[0] = sv[0] * pose[0] + sv[1] * pose[4] + sv[2] * pose[ 8];
		osv[1] = sv[0] * pose[1] + sv[1] * pose[5] + sv[2] * pose[ 9];
		osv[2] = sv[0] * pose[2] + sv[1] * pose[6] + sv[2] * pose[10];
		osv[3] = sv[3];
	}
}

// set the FP precision back to whatever value it was
#if defined ( _WIN32 ) && ( _MSC_VER >= 1400 ) && defined( NDEBUG )
# pragma float_control(pop)
# pragma float_control(pop)
# pragma fp_contract(off)	// this line is needed on Itanium processors
#endif

// hand-written SSE versions seem to perform a bit slower..
#define SKM_USE_SSE

#if defined ( id386 ) && !defined ( __MACOSX__ ) && defined( SKM_USE_SSE )

#if ( defined ( __GNUC__ ) && defined ( __SSE__ ) ) ||  ( defined ( _WIN32 ) && ( _MSC_VER >= 1400 ) )

#include <xmmintrin.h>

/*
* R_SkeletalBlendPoses_INTRIN
*/
static void R_SkeletalBlendPoses_INTRIN( unsigned int numblends, mskblend_t *blends, unsigned int numbones, mat4x4_t *relbonepose )
{
	unsigned int i, j, k;
	float *pose;
	__m128 mm_pose0, mm_pose1, mm_pose2, mm_pose3;
	mskblend_t *blend;

	for( i = 0, j = numbones, blend = blends; i < numblends; i++, j++, blend++ ) {
		float f;
		float *b;
		__m128 mm_f;

		pose = relbonepose[j];

		b = relbonepose[blend->indices[0]];
		f = blend->weights[0] * (1.0 / 255.0);

		mm_f = _mm_set1_ps( f );
		mm_pose0 = _mm_mul_ps( _mm_load_ps( b      ), mm_f );
		mm_pose1 = _mm_mul_ps( _mm_load_ps( b +  4 ), mm_f );
		mm_pose2 = _mm_mul_ps( _mm_load_ps( b +  8 ), mm_f );
		mm_pose3 = _mm_mul_ps( _mm_load_ps( b + 12 ), mm_f );

		for( k = 1; k < SKM_MAX_WEIGHTS && blend->weights[k]; k++ ) {
			b = relbonepose[blend->indices[k]];
			f = blend->weights[k] * (1.0 / 255.0);

			mm_f = _mm_set1_ps( f );
			mm_pose0 = _mm_add_ps( mm_pose0, _mm_mul_ps( _mm_load_ps( b      ), mm_f ) );
			mm_pose1 = _mm_add_ps( mm_pose1, _mm_mul_ps( _mm_load_ps( b + 4  ), mm_f ) );
			mm_pose2 = _mm_add_ps( mm_pose2, _mm_mul_ps( _mm_load_ps( b + 8  ), mm_f ) );
			mm_pose3 = _mm_add_ps( mm_pose3, _mm_mul_ps( _mm_load_ps( b + 12 ), mm_f ) );
		}

		_mm_store_ps( pose     , mm_pose0 );
		_mm_store_ps( pose +  4, mm_pose1 );
		_mm_store_ps( pose +  8, mm_pose2 );
		_mm_store_ps( pose + 12, mm_pose3 );
	}
}

/*
* R_SkeletalTransformVerts_INTRIN
*/
void R_SkeletalTransformVerts_INTRIN( int numverts, const unsigned int *blends, mat4x4_t *relbonepose, const vec_t *v, vec_t *ov )
{
	const float *pose;

	for( ; numverts; numverts--, v += 4, ov += 4, blends++ ) {
		__m128 accumulator;
		__m128 v_sse;

		pose = relbonepose[*blends];

		v_sse = _mm_load_ps( v );
		accumulator = _mm_mul_ps( _mm_shuffle_ps( v_sse, v_sse, 0x00 ), _mm_load_ps( pose ) );
		accumulator = _mm_add_ps( accumulator, _mm_mul_ps( _mm_shuffle_ps( v_sse, v_sse, 0x55 ), _mm_load_ps( pose + 4 ) ) );
		accumulator = _mm_add_ps( accumulator, _mm_mul_ps( _mm_shuffle_ps( v_sse, v_sse, 0xaa ), _mm_load_ps( pose + 8 ) ) );
		accumulator = _mm_add_ps( accumulator, _mm_load_ps( pose + 12 ) );
		_mm_stream_ps( ov, accumulator );
	}
}

/*
* R_SkeletalTransformNormals_INTRIN
*/
static void R_SkeletalTransformNormals_INTRIN( int numverts, const unsigned int *blends, mat4x4_t *relbonepose, const vec_t *v, vec_t *ov )
{
	const float *pose;

	for( ; numverts; numverts--, v += 4, ov += 4, blends++ ) {
		__m128 accumulator;
		__m128 v_sse;

		pose = relbonepose[*blends];

		v_sse = _mm_load_ps( v );
		accumulator = _mm_mul_ps( _mm_shuffle_ps( v_sse, v_sse, 0x00 ), _mm_load_ps( pose ) );
		accumulator = _mm_add_ps( accumulator, _mm_mul_ps( _mm_shuffle_ps( v_sse, v_sse, 0x55 ), _mm_load_ps( pose + 4 ) ) );
		accumulator = _mm_add_ps( accumulator, _mm_mul_ps( _mm_shuffle_ps( v_sse, v_sse, 0xaa ), _mm_load_ps( pose + 8 ) ) );
		_mm_store_ps( ov, accumulator );
		ov[3] = v[3];
	}
}


/*
* R_SkeletalTransformNormalsAndSVecs_INTRIN
*/
static void R_SkeletalTransformNormalsAndSVecs_INTRIN( int numverts, const unsigned int *blends, mat4x4_t *relbonepose, const vec_t *v, vec_t *ov, const vec_t *sv, vec_t *osv )
{
	const float *pose;

	for( ; numverts; numverts--, v += 4, ov += 4, sv += 4, osv += 4, blends++ ) {
		__m128 accumulator;
		__m128 v_sse;

		pose = relbonepose[*blends];

		// normals
		v_sse = _mm_load_ps( v );
		accumulator = _mm_mul_ps( _mm_shuffle_ps( v_sse, v_sse, 0x00 ), _mm_load_ps( pose ) );
		accumulator = _mm_add_ps( accumulator, _mm_mul_ps( _mm_shuffle_ps( v_sse, v_sse, 0x55 ), _mm_load_ps( pose + 4 ) ) );
		accumulator = _mm_add_ps( accumulator, _mm_mul_ps( _mm_shuffle_ps( v_sse, v_sse, 0xaa ), _mm_load_ps( pose + 8 ) ) );
		_mm_store_ps( ov, accumulator );
		ov[3] = v[3];

		// s(tangent) - vectors
		v_sse = _mm_load_ps( sv );
		accumulator = _mm_mul_ps( _mm_shuffle_ps( v_sse, v_sse, 0x00 ), _mm_load_ps( pose ) );
		accumulator = _mm_add_ps( accumulator, _mm_mul_ps( _mm_shuffle_ps( v_sse, v_sse, 0x55 ), _mm_load_ps( pose + 4 ) ) );
		accumulator = _mm_add_ps( accumulator, _mm_mul_ps( _mm_shuffle_ps( v_sse, v_sse, 0xaa ), _mm_load_ps( pose + 8 ) ) );
		_mm_store_ps( osv, accumulator );
		osv[3] = sv[3];
	}
}

#define R_SkeletalBlendPoses_SSE R_SkeletalBlendPoses_INTRIN
#define R_SkeletalTransformVerts_SSE R_SkeletalTransformVerts_INTRIN
#define R_SkeletalTransformNormals_SSE R_SkeletalTransformNormals_INTRIN
#define R_SkeletalTransformNormalsAndSVecs_SSE R_SkeletalTransformNormalsAndSVecs_INTRIN

#endif

#endif

#ifndef R_SkeletalTransformVerts_SSE
#define R_SkeletalBlendPoses_SSE R_SkeletalBlendPoses
#define R_SkeletalTransformVerts_SSE R_SkeletalTransformVerts
#define R_SkeletalTransformNormals_SSE R_SkeletalTransformNormals
#define R_SkeletalTransformNormalsAndSVecs_SSE R_SkeletalTransformNormalsAndSVecs
#endif

//=======================================================================

/*
* R_DrawBonesFrameLerp
*/
static void R_DrawBonesFrameLerp( const meshbuffer_t *mb, float backlerp )
{
	unsigned int i, j, meshnum;
	int features;
	float frontlerp = 1.0 - backlerp, *pose;
	mskmesh_t *mesh;
	bonepose_t *bonepose, *oldbonepose, tempbonepose[256], *lerpedbonepose;
	bonepose_t *bp, *oldbp, *out, tp;
	entity_t *e;
	model_t	*mod;
	mskmodel_t *skmodel;
	shader_t *shader;
	mskbone_t *bone;
	vec4_t *xyzArray, *normalsArray, *sVectorsArray;
	mat4x4_t *bonePoseRelativeMat;
	dualquat_t *bonePoseRelativeDQ;
	size_t bonePoseRelativeMatSize, bonePoseRelativeDQSize;
	rbackAnimData_t animData;
	qboolean useSSE = COM_CPUFeatures() & QCPU_HAS_SSE ? qtrue : qfalse;

	MB_NUM2ENTITY( mb->sortkey, e );
	mod = Mod_ForHandle( mb->LODModelHandle );
	skmodel = ( mskmodel_t * )mod->extradata;

	meshnum = -( mb->infokey + 1 );
	if( meshnum >= skmodel->nummeshes )
		return;
	mesh = skmodel->meshes + meshnum;

	xyzArray = inVertsArray;
	normalsArray = inNormalsArray;
	sVectorsArray = inSVectorsArray;
	bonePoseRelativeMat = NULL;
	bonePoseRelativeDQ = NULL;

	MB_NUM2SHADER( mb->shaderkey, shader );

	features = MF_NONBATCHED | shader->features;
	if( !mb->vboIndex ) {
		features &= ~MF_HARDWARE;
	}

	if( ri.params & RP_SHADOWMAPVIEW )
	{
		features &= ~( MF_COLORS|MF_SVECTORS|MF_ENABLENORMALS );
		if( !( shader->features & MF_DEFORMVS ) )
			features &= ~MF_NORMALS;
	}
	else
	{
		if( features & MF_SVECTORS )
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

	if( bp == oldbp && !e->frame ) {
		// fastpath: render frame 0 as is
		xyzArray = mesh->xyzArray;
		normalsArray = mesh->normalsArray;
		sVectorsArray = mesh->sVectorsArray;
		goto pushmesh;
	}

	// if we've this far and we're going to go the GPU-path, we'll need to transfer
	// the bones data
	if( features & MF_HARDWARE ) {
		features |= MF_BONES;
	}

	// cache size
	bonePoseRelativeMatSize = sizeof( mat4x4_t ) * (skmodel->numbones + skmodel->numblends);
	bonePoseRelativeDQSize = sizeof( dualquat_t ) * skmodel->numbones;

	// fetch bones tranforms from cache (both matrices and dual quaternions)
	bonePoseRelativeMat = ( mat4x4_t * )R_GetSketalCache( R_ENT2NUM( e ), mod->lodnum );
	if( bonePoseRelativeMat ) {
		bonePoseRelativeDQ = ( dualquat_t * )(( qbyte * )bonePoseRelativeMat + bonePoseRelativeMatSize);

		// since we've succeeded in fetching the transforms from cache, 
		// we don't need to recalc them again, so push the mesh right now
		if( features & MF_HARDWARE ) {
			goto pushmesh;
		}
		goto transform;
	}

	lerpedbonepose = tempbonepose;
	if( bp == oldbp || frontlerp == 1 )
	{
		if( e->boneposes )
		{
			// assume that parent transforms have already been applied
			lerpedbonepose = bp;
		}
		else
		{
			for( i = 0; i < skmodel->numbones; i++ )
			{
				j = i;
				out = lerpedbonepose + j;
				bonepose = bp + j;
				bone = skmodel->bones + j;

				if( bone->parent >= 0 ) {
					DualQuat_Multiply( lerpedbonepose[bone->parent].dualquat, bonepose->dualquat, out->dualquat );
				}
				else {
					DualQuat_Copy( bonepose->dualquat, out->dualquat );
				}
			}
		}
	}
	else
	{
		if( e->boneposes )
		{
			// lerp, assume that parent transforms have already been applied
			for( i = 0, out = lerpedbonepose, bonepose = bp, oldbonepose = oldbp, bone = skmodel->bones; i < skmodel->numbones; i++, out++, bonepose++, oldbonepose++, bone++ )
			{
				DualQuat_Lerp( oldbonepose->dualquat, bonepose->dualquat, frontlerp, out->dualquat );
			}
		}
		else
		{
			// lerp and transform
			for( i = 0; i < skmodel->numbones; i++ )
			{
				j = i;
				out = lerpedbonepose + j;
				bonepose = bp + j;
				oldbonepose = oldbp + j;
				bone = skmodel->bones + j;

				DualQuat_Lerp( oldbonepose->dualquat, bonepose->dualquat, frontlerp, out->dualquat );

				if( bone->parent >= 0 ) {
					DualQuat_Copy( out->dualquat, tp.dualquat );
					DualQuat_Multiply( tempbonepose[bone->parent].dualquat, tp.dualquat, out->dualquat );
				}
			}
		}
	}

	bonePoseRelativeMat = ( mat4x4_t * )R_AllocSkeletalDataCache( R_ENT2NUM( e ), mod->lodnum, 
		bonePoseRelativeMatSize + bonePoseRelativeDQSize );
	bonePoseRelativeDQ = ( dualquat_t * )(( qbyte * )bonePoseRelativeMat + bonePoseRelativeMatSize);

	// generate matrices and dual quaternions for all bones
	for( i = 0; i < skmodel->numbones; i++ ) {
		pose = bonePoseRelativeMat[i];

		DualQuat_Multiply( lerpedbonepose[i].dualquat, skmodel->invbaseposes[i].dualquat, bonePoseRelativeDQ[i] );
		DualQuat_Normalize( bonePoseRelativeDQ[i] );

		Matrix_FromDualQuaternion( bonePoseRelativeDQ[i], pose );
	}

	// no CPU transforms
	if( features & MF_HARDWARE ) {
		goto pushmesh;
	}

	// generate matrices for all blend combinations
	if( useSSE ) {
		R_SkeletalBlendPoses_SSE( skmodel->numblends, skmodel->blends, skmodel->numbones, bonePoseRelativeMat );
	}
	else {
		R_SkeletalBlendPoses( skmodel->numblends, skmodel->blends, skmodel->numbones, bonePoseRelativeMat );
	}

transform:
	if( useSSE )
	{
		R_SkeletalTransformVerts_SSE( mesh->numverts, mesh->vertexBlends, bonePoseRelativeMat,
			( vec_t * )mesh->xyzArray[0], ( vec_t * )inVertsArray );

		if( features & MF_SVECTORS ) {
			R_SkeletalTransformNormals_SSE( mesh->numverts, mesh->vertexBlends, bonePoseRelativeMat,
			( vec_t * )mesh->sVectorsArray[0], ( vec_t * )inSVectorsArray );
		}
		if( features & MF_NORMALS ) {
			R_SkeletalTransformNormals_SSE( mesh->numverts, mesh->vertexBlends, bonePoseRelativeMat,
			( vec_t * )mesh->normalsArray[0], ( vec_t * )inNormalsArray );
		}
	}
	else
	{
		R_SkeletalTransformVerts( mesh->numverts, mesh->vertexBlends, bonePoseRelativeMat,
			( vec_t * )mesh->xyzArray[0], ( vec_t * )inVertsArray );

		if( features & MF_SVECTORS ) {
			R_SkeletalTransformNormalsAndSVecs( mesh->numverts, mesh->vertexBlends, bonePoseRelativeMat,
			( vec_t * )mesh->normalsArray[0], ( vec_t * )inNormalsArray,
			( vec_t * )mesh->sVectorsArray[0], ( vec_t * )inSVectorsArray );
		} else if( features & MF_NORMALS ) {
			R_SkeletalTransformNormals( mesh->numverts, mesh->vertexBlends, bonePoseRelativeMat,
			( vec_t * )mesh->normalsArray[0], ( vec_t * )inNormalsArray );
		}
	}

pushmesh:
	skm_mesh.elems = mesh->elems;
	skm_mesh.numElems = mesh->numtris * 3;
	skm_mesh.numVerts = mesh->numverts;
	skm_mesh.xyzArray = xyzArray;
	skm_mesh.stArray = mesh->stArray;
	skm_mesh.normalsArray = normalsArray;
	skm_mesh.sVectorsArray = sVectorsArray;

	animData.numBones = skmodel->numbones;
	animData.dualQuats = bonePoseRelativeDQ;
	animData.maxWeights = mesh->maxWeights;

	R_RotateForEntity( e );

	R_PushMesh( &skm_mesh, mb->vboIndex != 0, features );
	R_RenderMeshBuffer( mb, &animData );
}

/*
* R_DrawSkeletalModel
*/
void R_DrawSkeletalModel( const meshbuffer_t *mb )
{
	entity_t *e;
	float depthmin = gldepthmin, depthmax = gldepthmax;

	MB_NUM2ENTITY( mb->sortkey, e );

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
* R_SkeletalModelBBox
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
* R_SkeletalModelFrameBounds
*/
void R_SkeletalModelFrameBounds( const model_t *mod, int frame, vec3_t mins, vec3_t maxs )
{
	mskframe_t *pframe;
	mskmodel_t *skmodel = ( mskmodel_t * )mod->extradata;

	if( !skmodel->nummeshes )
	{
		ClearBounds( mins, maxs );
		return;
	}

	if( ( frame >= (int)skmodel->numframes ) || ( frame < 0 ) )
	{
#ifndef PUBLIC_BUILD
		Com_DPrintf( "R_SkeletalModelFrameBounds %s: no such frame %d\n", mod->name, frame );
#endif
		ClearBounds( mins, maxs );
		return;
	}

	pframe = skmodel->frames + frame;
	VectorCopy( pframe->mins, mins );
	VectorCopy( pframe->maxs, maxs );
}

/*
* R_CullSkeletalModel
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
			if( mb ) {
				mb->LODModelHandle = modhandle;
				if( shader->features & MF_HARDWARE && mesh->vbo != NULL ) {
					mb->vboIndex = mesh->vbo->index;
				}
			}
		}
	}

	return frustum;
}

/*
* R_AddSkeletalModelToList
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
	float distance;

	mod = R_SkeletalModelLOD( e );
	skmodel = ( mskmodel_t * )mod->extradata;
	modhandle = Mod_Handle( mod );

	// make sure weapon model is always closest to the viewer
	if( e->renderfx & RF_WEAPONMODEL ) {
		distance = 0;
	}
	else {
		distance = Distance( e->origin, ri.viewOrigin ) + 1;
	}

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
			R_AddModelMeshToList( modhandle, mesh->vbo, fog, shader, i, distance, mesh->numverts, mesh->numtris * 3 );
	}
}
