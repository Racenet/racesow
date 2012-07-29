/*
Copyright (C) 2011 Victor Luchits

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
=========================================================

STATIC VERTEX BUFFER OBJECTS

=========================================================
*/

typedef struct vbohandle_s
{
	unsigned int index;
	mesh_vbo_t *vbo;
	struct vbohandle_s *prev, *next;
} vbohandle_t;

#define MAX_MESH_VERTREX_BUFFER_OBJECTS 	8192

static mesh_vbo_t r_mesh_vbo[MAX_MESH_VERTREX_BUFFER_OBJECTS];

static vbohandle_t r_vbohandles[MAX_MESH_VERTREX_BUFFER_OBJECTS];
static vbohandle_t r_vbohandles_headnode, *r_free_vbohandles;

static int r_num_active_vbos;

/*
* R_InitVBO
*/
void R_InitVBO( void )
{
	int i;

	r_num_active_vbos = 0;

	memset( r_mesh_vbo, 0,  sizeof( r_mesh_vbo ) );
	memset( r_vbohandles, 0,  sizeof( r_vbohandles ) );

	// link vbo handles
	r_free_vbohandles = r_vbohandles;
	r_vbohandles_headnode.prev = &r_vbohandles_headnode;
	r_vbohandles_headnode.next = &r_vbohandles_headnode;
	for( i = 0; i < MAX_MESH_VERTREX_BUFFER_OBJECTS; i++ ) {
		r_vbohandles[i].index = i;
		r_vbohandles[i].vbo = &r_mesh_vbo[i];
	}
	for( i = 0; i < MAX_MESH_VERTREX_BUFFER_OBJECTS - 1; i++ ) {
		r_vbohandles[i].next = &r_vbohandles[i+1];
	}
}

/*
* R_CreateStaticMeshVBO
*
* Create two static buffer objects: vertex buffer and elements buffer, the real
* data is uploaded by calling R_UploadVBOVertexData and R_UploadVBOElemData.
*
* Tag allows vertex buffer objects to be grouped and released simultaneously.
*/
mesh_vbo_t *R_CreateStaticMeshVBO( void *owner, int numVerts, int numElems, int features, vbo_tag_t tag )
{
	int i;
	size_t size;
	GLuint vbo_id;
	vbohandle_t *vboh = NULL;
	mesh_vbo_t *vbo = NULL;

	if( !glConfig.ext.vertex_buffer_object )
		return NULL;

	if( !r_free_vbohandles )
		return NULL;

	vboh = r_free_vbohandles;
	vbo = &r_mesh_vbo[vboh->index];
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

	// bones data for skeletal animation
	if( features & MF_BONES ) {
		vbo->bonesIndicesOffset = size;
		size += numVerts * sizeof( qbyte ) * SKM_MAX_WEIGHTS;

		vbo->bonesWeightsOffset = size;
		size += numVerts * sizeof( qbyte ) * SKM_MAX_WEIGHTS;		
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

	r_free_vbohandles = vboh->next;

	// link to the list of active vbo handles
	vboh->prev = &r_vbohandles_headnode;
	vboh->next = r_vbohandles_headnode.next;
	vboh->next->prev = vboh;
	vboh->prev->next = vboh;

	r_num_active_vbos++;

	vbo->registration_sequence = r_front.registration_sequence;
	vbo->numVerts = numVerts;
	vbo->numElems = numElems;
	vbo->owner = owner;
	vbo->index = vboh->index + 1;
	vbo->tag = tag;

	return vbo;

error:
	if( vbo )
		R_ReleaseMeshVBO( vbo );

	qglBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
	qglBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, 0 );

	return NULL;
}

/*
* R_TouchMeshVBO
*/
void R_TouchMeshVBO( mesh_vbo_t *vbo )
{
	vbo->registration_sequence = r_front.registration_sequence;
}

/*
* R_VBOByIndex
*/
mesh_vbo_t *R_VBOByIndex( int index )
{
	if( index >= 1 && index <= MAX_MESH_VERTREX_BUFFER_OBJECTS ) {
		return r_mesh_vbo + index - 1;
	}
	return NULL;
}

/*
* R_ReleaseMeshVBO
*/
void R_ReleaseMeshVBO( mesh_vbo_t *vbo )
{
	GLuint vbo_id;

	assert( vbo );

	if( vbo->vertexId ) {
		vbo_id = vbo->vertexId;
		qglDeleteBuffersARB( 1, &vbo_id );
	}

	if( vbo->elemId ) {
		vbo_id = vbo->elemId;
		qglDeleteBuffersARB( 1, &vbo_id );
	}

	if( vbo->index >= 1 && vbo->index <= MAX_MESH_VERTREX_BUFFER_OBJECTS ) {
		vbohandle_t *vboh = &r_vbohandles[vbo->index - 1];

		// remove from linked active list
		vboh->prev->next = vboh->next;
		vboh->next->prev = vboh->prev;

		// insert into linked free list
		vboh->next = r_free_vbohandles;
		r_free_vbohandles = vboh;

		r_num_active_vbos--;
	}

	memset( vbo, 0, sizeof( *vbo ) );
	vbo->tag = VBO_TAG_NONE;
}

/*
* R_GetNumberOfActiveVBOs
*/
int R_GetNumberOfActiveVBOs( void )
{
	return r_num_active_vbos;
}

/*
* R_UploadVBOVertexData
*
* Uploads required vertex data to the buffer.
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
	if( !vbo->vertexId ) {
		return 0;
	}

	qglBindBufferARB( GL_ARRAY_BUFFER_ARB, vbo->vertexId );

	// upload vertex xyz data
	if( mesh->xyzArray )
		qglBufferSubDataARB( GL_ARRAY_BUFFER_ARB, 0 + vertsOffset * sizeof( vec4_t ), numVerts * sizeof( vec4_t ), mesh->xyzArray );

	// upload normals data
	if( vbo->normalsOffset ) {
		if( !mesh->normalsArray )
			errArrays |= MF_NORMALS;
		else
			qglBufferSubDataARB( GL_ARRAY_BUFFER_ARB, vbo->normalsOffset + vertsOffset * sizeof( vec4_t ), numVerts * sizeof( vec4_t ), mesh->normalsArray );
	}

	// upload tangent vectors
	if( vbo->sVectorsOffset ) {
		if( !mesh->sVectorsArray )
			errArrays |= MF_SVECTORS;
		else
			qglBufferSubDataARB( GL_ARRAY_BUFFER_ARB, vbo->sVectorsOffset + vertsOffset * sizeof( vec4_t ), numVerts * sizeof( vec4_t ), mesh->sVectorsArray );
	}

	// upload texture coordinates
	if( vbo->stOffset ) {
		if( !mesh->stArray )
			errArrays |= MF_STCOORDS;
		else
			qglBufferSubDataARB( GL_ARRAY_BUFFER_ARB, vbo->stOffset + vertsOffset * sizeof( vec2_t ), numVerts * sizeof( vec2_t ), mesh->stArray );
	}

	// upload lightmap texture coordinates
	for( i = 0; i < MAX_LIGHTMAPS; i++ ) {
		if( !vbo->lmstOffset[i] )
			break;
		if( !mesh->lmstArray[i] ) {
			errArrays |= MF_LMCOORDS<<i;
			break;
		}
		qglBufferSubDataARB( GL_ARRAY_BUFFER_ARB, vbo->lmstOffset[i] + vertsOffset * sizeof( vec2_t ), numVerts * sizeof( vec2_t ), mesh->lmstArray[i] );
	}

	// upload vertex colors (although indices > 0 are never used)
	for( i = 0; i < MAX_LIGHTMAPS; i++ ) {
		if( !vbo->colorsOffset[i] )
			break;
		if( !mesh->colorsArray[i] ) {
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
	for( i = 0; i < mesh->numElems; i++ ) {
		ielems[i] = vertsOffset + mesh->elems[i];
	}

	qglBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, vbo->elemId );
	qglBufferSubDataARB( GL_ELEMENT_ARRAY_BUFFER_ARB, elemsOffset * sizeof( unsigned int ), mesh->numElems * sizeof( unsigned int ), ielems );
	qglBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, 0 );

	Mem_TempFree( ielems );
}

/*
* R_UploadVBOElemTrifanData
*
* Builds and uploads indexes in trifan order, properly offsetting them for batching
*/
void R_UploadVBOElemTrifanData( mesh_vbo_t *vbo, int vertsOffset, int elemsOffset, int numVerts )
{
	int i;
	int numElems;
	unsigned int *ielems, *elem;

	assert( vbo );

	if( !vbo->elemId )
		return;

	numElems = numVerts + numVerts + numVerts - 6;
	ielems = Mem_TempMalloc( sizeof( *ielems ) * numElems );

	elem = ielems;
	for( i = 2; i < numVerts; i++, elem += 3 ) {
		elem[0] = vertsOffset;
		elem[1] = vertsOffset + i - 1;
		elem[2] = vertsOffset + i;
	}

	qglBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, vbo->elemId );
	qglBufferSubDataARB( GL_ELEMENT_ARRAY_BUFFER_ARB, elemsOffset * sizeof( unsigned int ), numElems * sizeof( unsigned int ), ielems );
	qglBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, 0 );

	Mem_TempFree( ielems );
}

/*
* R_UploadVBOBonesData
*
* Uploads vertex bones data to the buffer
*/
int R_UploadVBOBonesData( mesh_vbo_t *vbo, int vertsOffset, int numVerts, qbyte *bonesIndices, qbyte *bonesWeights )
{
	assert( vbo );

	if( !vbo->vertexId ) {
		return 0;
	}
	if(	!bonesIndices || !bonesWeights ) {
		return MF_BONES;
	}

	qglBindBufferARB( GL_ARRAY_BUFFER_ARB, vbo->vertexId );

	if( vbo->bonesIndicesOffset ) {
		qglBufferSubDataARB( GL_ARRAY_BUFFER_ARB, vbo->bonesIndicesOffset + vertsOffset * sizeof( qbyte ) * SKM_MAX_WEIGHTS, 
			numVerts * sizeof( qbyte ) * SKM_MAX_WEIGHTS, bonesIndices );
	}

	if( vbo->bonesWeightsOffset ) {
		qglBufferSubDataARB( GL_ARRAY_BUFFER_ARB, vbo->bonesWeightsOffset + vertsOffset * sizeof( qbyte ) * SKM_MAX_WEIGHTS, 
			numVerts * sizeof( qbyte ) * SKM_MAX_WEIGHTS, bonesWeights );
	}

 	qglBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );

	return 0;
}

/*
* R_FreeVBOsByTag
*
* Release all vertex buffer objects with specified tag.
*/
void R_FreeVBOsByTag( vbo_tag_t tag )
{
	mesh_vbo_t *vbo;
	vbohandle_t *vboh, *next, *hnode;

	if( !r_num_active_vbos ) {
		return;
	}

	hnode = &r_vbohandles_headnode;
	for( vboh = hnode->prev; vboh != hnode; vboh = next ) {
		next = vboh->prev;
		vbo = &r_mesh_vbo[vboh->index];

		if( vbo->tag == tag ) {
			R_ReleaseMeshVBO( vbo );
		}
	}
}

/*
* R_FreeUnusedVBOs
*/
void R_FreeUnusedVBOs( void )
{
	mesh_vbo_t *vbo;
	vbohandle_t *vboh, *next, *hnode;

	if( !r_num_active_vbos ) {
		return;
	}

	hnode = &r_vbohandles_headnode;
	for( vboh = hnode->prev; vboh != hnode; vboh = next ) {
		next = vboh->prev;
		vbo = &r_mesh_vbo[vboh->index];

		if( vbo->registration_sequence != r_front.registration_sequence ) {
			R_ReleaseMeshVBO( vbo );
		}
	}
}

/*
* R_ShutdownVBO
*/
void R_ShutdownVBO( void )
{
	mesh_vbo_t *vbo;
	vbohandle_t *vboh, *next, *hnode;

	if( !r_num_active_vbos ) {
		return;
	}

	hnode = &r_vbohandles_headnode;
	for( vboh = hnode->prev; vboh != hnode; vboh = next ) {
		next = vboh->prev;
		vbo = &r_mesh_vbo[vboh->index];

		R_ReleaseMeshVBO( vbo );
	}
}
