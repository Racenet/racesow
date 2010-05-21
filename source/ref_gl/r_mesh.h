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
#ifndef __R_MESH_H__
#define __R_MESH_H__

enum
{
	MF_NONE				= 0,
	MF_NORMALS			= 1 << 0,
	MF_STCOORDS			= 1 << 1,
	MF_LMCOORDS			= 1 << 2,
	MF_LMCOORDS1		= 1 << 3,
	MF_LMCOORDS2		= 1 << 4,
	MF_LMCOORDS3		= 1 << 5,
	MF_COLORS			= 1 << 6,
	MF_COLORS1			= 1 << 7,
	MF_COLORS2			= 1 << 8,
	MF_COLORS3			= 1 << 9,
	MF_ENABLENORMALS    = 1 << 10,
	MF_DEFORMVS			= 1 << 11,
	MF_SVECTORS			= 1 << 12,
	MF_NONBATCHED		= 1 << 13,
	MF_POLYGONOFFSET	= 1 << 14,

	// global features
	MF_NOCULL			= 1 << 16,
	MF_TRIFAN			= 1 << 17,
	MF_KEEPLOCK			= 1 << 18,
	MF_NOCOLORWRITE		= 1 << 19,
	MF_POLYGONOFFSET2	= 1 << 20
};

enum
{
	MB_MODEL,
	MB_SPRITE,
	MB_POLY,
	MB_CORONA,

	MB_MAXTYPES = 4
};

typedef struct mesh_s
{
	int					numVertexes;
	vec4_t				*xyzArray;
	vec4_t				*normalsArray;
	vec4_t				*sVectorsArray;
	vec2_t				*stArray;
	vec2_t				*lmstArray[MAX_LIGHTMAPS];
	byte_vec4_t			*colorsArray[MAX_LIGHTMAPS];

	int					numElems;
	elem_t				*elems;
} mesh_t;

#define MB_FOG2NUM( fog )			( (fog) ? ((((int)((fog) - r_worldbrushmodel->fogs))+1) << 2) : 0 )
#define MB_NUM2FOG( num, fog )		( (fog) = r_worldbrushmodel ? r_worldbrushmodel->fogs+(((num)>>2) & 0xFF) : 0, (fog) = r_worldbrushmodel ? ( (fog) == r_worldbrushmodel->fogs ? NULL : (fog)-1 ) : NULL )

#define MB_ENTITY2NUM( ent )		( (int)((ent)-r_entities)<<20 )
#define MB_NUM2ENTITY( num, ent )	( ent = r_entities+(((num)>>20)&1023) )

#define MB_SHADER2NUM( s )			( (s)->sort << 26 ) | ((s) - r_shaders )
#define MB_NUM2SHADER( num, s )		( (s) = r_shaders + ((num) & 0xFFF) )
#define MB_NUM2SHADERSORT( num )	( ((num) >> 26) & 0x1F )

#define MIN_RENDER_MESHES			2048

typedef struct
{
	int					shaderkey;
	unsigned int		sortkey;
	int					infokey;			// surface number or mesh number
	union
	{
		int				lastPoly;
		unsigned int	dlightbits;
		unsigned int	LODModelHandle;
	};
	unsigned int		shadowbits;
} meshbuffer_t;

typedef struct
{
	int					num_opaque_meshes, max_opaque_meshes;
	meshbuffer_t		*meshbuffer_opaque;

	int					num_translucent_meshes, max_translucent_meshes;
	meshbuffer_t		*meshbuffer_translucent;

	int					num_portal_opaque_meshes;
	int					num_portal_translucent_meshes;

	meshbuffer_t		**surfmbuffers;				// pointers to meshbuffers of world surfaces
} meshlist_t;

enum
{
	SKYBOX_RIGHT,
	SKYBOX_LEFT,
	SKYBOX_FRONT,
	SKYBOX_BACK,
	SKYBOX_TOP,
	SKYBOX_BOTTOM		// not used for skydome, but is used for skybox
};

typedef struct
{
	mesh_t				*meshes;
	vec2_t				*sphereStCoords[5];
	vec2_t				*linearStCoords[6];

	struct shader_s		*farboxShaders[6];
	struct shader_s		*nearboxShaders[6];
} skydome_t;

#endif /*__R_MESH_H__*/
