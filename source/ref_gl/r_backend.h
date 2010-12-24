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
#ifndef __R_BACKEND_H__
#define __R_BACKEND_H__

#define MAX_ARRAY_VERTS			4096
#define MAX_ARRAY_ELEMENTS		MAX_ARRAY_VERTS*6
#define MAX_ARRAY_TRIANGLES		MAX_ARRAY_ELEMENTS/3
#define MAX_ARRAY_NEIGHBORS		MAX_ARRAY_TRIANGLES*3

enum
{
	VBO_VERTS,
	VBO_NORMALS,
	VBO_COLORS,
//	VBO_INDEXES,
	VBO_TC0,

	VBO_ENDMARKER
};

#define MAX_VERTEX_BUFFER_OBJECTS   VBO_ENDMARKER+MAX_TEXTURE_UNITS-1

extern ALIGN( 16 ) vec4_t inVertsArray[MAX_ARRAY_VERTS];
extern ALIGN( 16 ) vec4_t inNormalsArray[MAX_ARRAY_VERTS];
extern vec4_t inSVectorsArray[MAX_ARRAY_VERTS];
extern elem_t inElemsArray[MAX_ARRAY_ELEMENTS];
extern vec2_t inCoordsArray[MAX_ARRAY_VERTS];
extern vec2_t inLightmapCoordsArray[MAX_LIGHTMAPS][MAX_ARRAY_VERTS];
extern byte_vec4_t inColorsArray[MAX_LIGHTMAPS][MAX_ARRAY_VERTS];

extern elem_t *elemsArray;
extern vec4_t *vertsArray;
extern vec4_t *normalsArray;
extern vec4_t *sVectorsArray;
extern vec2_t *coordsArray;
extern vec2_t *lightmapCoordsArray[MAX_LIGHTMAPS];
extern byte_vec4_t *colorArray;
extern byte_vec4_t colorArrayCopy[MAX_ARRAY_VERTS];

extern int r_numVertexBufferObjects;
extern GLuint r_vertexBufferObjects[MAX_VERTEX_BUFFER_OBJECTS];

extern int r_features;

//===================================================================

typedef struct
{
	unsigned int numVerts, numElems, numColors;
	unsigned int c_totalVerts, c_totalTris, c_totalVboVerts, c_totalVboTris, c_totalDraws, c_totalKeptLocks;
	size_t c_totalMemCpy;
} rbackacc_t;

extern rbackacc_t r_backacc;

//===================================================================

void R_BackendInit( void );
void R_BackendShutdown( void );
void R_BackendStartFrame( void );
void R_BackendEndFrame( void );
void R_BackendResetCounters( void );

void R_BackendBeginTriangleOutlines( void );
void R_BackendEndTriangleOutlines( void );

void R_BackendCleanUpTextureUnits( void );

void R_BackendSetPassMask( int mask );
void R_BackendResetPassMask( void );

void R_LockArrays( int numverts );
void R_UnlockArrays( void );
void R_UnlockArrays( void );
void R_DrawElements( void );
void R_FlushArrays( void );
void R_FlushArraysMtex( void );
void R_ClearArrays( void );

static inline qboolean R_MeshOverflow( const mesh_t *mesh )
{
	return ( r_backacc.numVerts + mesh->numVerts > MAX_ARRAY_VERTS ||
		r_backacc.numElems + mesh->numElems > MAX_ARRAY_ELEMENTS );
}

static inline qboolean R_MeshOverflow2( const meshbuffer_t *mb, const meshbuffer_t *nextmb )
{
	return ( r_backacc.numVerts + mb->numVerts + nextmb->numVerts > MAX_ARRAY_VERTS ||
		r_backacc.numElems + mb->numElems + nextmb->numElems > MAX_ARRAY_ELEMENTS );
}

static inline qboolean R_InvalidMesh( const mesh_t *mesh )
{
	return ( !mesh->numVerts || !mesh->numElems ||
		mesh->numVerts > MAX_ARRAY_VERTS || mesh->numElems > MAX_ARRAY_ELEMENTS );
}

void R_PushMesh( const mesh_vbo_t *vbo, const mesh_t *mesh, int features );
void R_RenderMeshBuffer( const meshbuffer_t *mb );

#endif /*__R_BACKEND_H__*/
