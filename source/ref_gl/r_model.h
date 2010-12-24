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
#ifndef __R_MODEL_H__
#define __R_MODEL_H__

/*

d*_t structures are on-disk representations
m*_t structures are in-memory

*/

/*
==============================================================================

BRUSH MODELS

==============================================================================
*/


//
// in memory representation
//
typedef struct
{
	vec3_t			mins, maxs;
	float			radius;
	int				firstface, numfaces;
} mmodel_t;

typedef struct
{
	shader_t		*shader;
	cplane_t		*visibleplane;

	int				numplanes;
	cplane_t		*planes;
} mfog_t;

typedef struct msurface_s
{
	unsigned int	visframe;			// should be drawn when node is crossed
	unsigned int	facetype, flags;
	unsigned short	numVertexes, numElems;

	shader_t		*shader;
	mesh_t			*mesh;
	mfog_t			*fog;
	cplane_t		*plane;
	mesh_vbo_t 		*vbo;

	union
	{
		float		origin[3];
		float		mins[3];
	};
	union
	{
		float		maxs[3];
		float		color[3];
	};

	int				superLightStyle;
	int				fragmentframe;		// for multi-check avoidance
} msurface_t;

typedef struct mnode_s
{
	// common with leaf
	cplane_t		*plane;

	unsigned int	pvsframe;

	float			mins[3];
	float			maxs[3];			// for bounding box culling

	struct mnode_s	*parent;

	// node specific
	struct mnode_s	*children[2];
} mnode_t;

typedef struct mleaf_s
{
	// common with node
	cplane_t		*plane;

	unsigned int	pvsframe;

	float			mins[3];
	float			maxs[3];			// for bounding box culling

	struct			mnode_s *parent;

	// leaf specific
	unsigned int	visframe;
	int				cluster, area;

	msurface_t		**firstVisSurface;
	msurface_t		**firstFragmentSurface;
} mleaf_t;

typedef struct
{
	qbyte			ambient[MAX_LIGHTMAPS][3];
	qbyte			diffuse[MAX_LIGHTMAPS][3];
	qbyte			styles[MAX_LIGHTMAPS];
	qbyte			direction[2];
} mgridlight_t;

typedef struct
{
	int				texNum;
	float			texMatrix[2][2];
} mlightmapRect_t;

typedef struct
{
	const bspFormatDesc_t *format;

	dvis_t			*pvs, *phs;

	int				numsubmodels;
	mmodel_t		*submodels;

	int				nummodelsurfaces;
	msurface_t		*firstmodelsurface;

	int				numplanes;
	cplane_t		*planes;

	int				numleafs;			// number of visible leafs, not counting 0
	mleaf_t			*leafs;
	mleaf_t			**visleafs;

	int				numnodes;
	mnode_t			*nodes;

	int				numsurfaces;
	msurface_t		*surfaces;

	int				numlightgridelems;
	mgridlight_t	*lightgrid;

	int				numlightarrayelems;
	mgridlight_t	**lightarray;

	int				numfogs;
	mfog_t			*fogs;
	mfog_t			*globalfog;

	int				numareas;

	vec3_t			gridSize;
	vec3_t			gridMins;
	int				gridBounds[4];
} mbrushmodel_t;

/*
==============================================================================

ALIAS MODELS

==============================================================================
*/

//
// in memory representation
//
typedef struct
{
	short			point[3];
	qbyte			latlong[2];				// use bytes to keep 8-byte alignment
} maliasvertex_t;

typedef struct
{
	vec3_t			mins, maxs;
	vec3_t			scale;
	vec3_t			translate;
	float			radius;
} maliasframe_t;

typedef struct
{
	char			name[MD3_MAX_PATH];
	quat_t			quat;
	vec3_t			origin;
} maliastag_t;

typedef struct
{
	shader_t		*shader;
} maliasskin_t;

typedef struct
{
	char			name[MD3_MAX_PATH];

	int				numverts;
	maliasvertex_t *vertexes;
	vec2_t			*stArray;

	vec4_t			*xyzArray;
	vec4_t			*normalsArray;
	vec4_t			*sVectorsArray;

	int				numtris;
	elem_t			*elems;

	int				numskins;
	maliasskin_t	*skins;
} maliasmesh_t;

typedef struct
{
	int				numframes;
	maliasframe_t	*frames;

	int				numtags;
	maliastag_t		*tags;

	int				nummeshes;
	maliasmesh_t	*meshes;

	int numskins;
	maliasskin_t	*skins;
} maliasmodel_t;

/*
==============================================================================

SKELETAL MODELS

==============================================================================
*/

//
// in memory representation
//
#define SKM_MAX_WEIGHTS		4

//
// in memory representation
//
typedef struct
{
	shader_t		*shader;
} mskskin_t;

typedef struct
{
	char			name[SKM_MAX_NAME];

	float			*influences;
	unsigned int	*bones;

	unsigned int	numverts;
	vec4_t			*xyzArray;
	vec4_t			*normalsArray;
	vec2_t			*stArray;
	vec4_t			*sVectorsArray;

	unsigned int	numtris;
	elem_t			*elems;

	unsigned int	numreferences;
	unsigned int	*references;

	mskskin_t		skin;
} mskmesh_t;

typedef struct
{
	char			name[SKM_MAX_NAME];
	signed int		parent;
	unsigned int	flags;
} mskbone_t;

typedef struct
{
	vec3_t			mins, maxs;
	float			radius;
	bonepose_t		*boneposes;
} mskframe_t;

typedef struct
{
	unsigned int	numbones;
	mskbone_t		*bones;

	unsigned int	nummeshes;
	mskmesh_t		*meshes;

	unsigned int	numframes;
	mskframe_t		*frames;
	bonepose_t		*invbaseposes;
} mskmodel_t;

/*
==============================================================================

SPRITE MODELS

==============================================================================
*/

#ifdef QUAKE2_JUNK

//
// in memory representation
//
typedef struct
{
	int				width, height;
	int				origin_x, origin_y;             // raster coordinates inside pic

	char			name[SPRITE_MAX_NAME];
	shader_t		*shader;

	float			mins[3], maxs[3];
	float			radius;
} sframe_t;

typedef struct
{
	int				numframes;
	sframe_t		*frames;
} smodel_t;

#endif

//===================================================================

//
// Whole model
//

typedef enum { mod_bad, mod_brush, mod_sprite, mod_alias, mod_skeletal } modtype_t;

#define MOD_MAX_LODS	4

typedef struct model_s
{
	char			*name;

	modtype_t		type;

	//
	// volume occupied by the model graphics
	//
	vec3_t			mins, maxs;
	float			radius;

	//
	// memory representation pointer
	//
	void			*extradata;

	int				numlods;
	struct model_s	*lods[MOD_MAX_LODS];

	mempool_t		*mempool;
} model_t;

//============================================================================

void		R_InitModels( void );
void		R_ShutdownModels( void );

void		Mod_ClearAll( void );
model_t		*Mod_ForName( const char *name, qboolean crash );
mleaf_t		*Mod_PointInLeaf( float *p, model_t *model );
qbyte		*Mod_ClusterPVS( int cluster, model_t *model );
qbyte		*Mod_ClusterPHS( int cluster, model_t *model );

unsigned int Mod_Handle( model_t *mod );
model_t		*Mod_ForHandle( unsigned int elem );

#define		Mod_Malloc( mod, size ) Mem_Alloc( ( mod )->mempool, size )
#define		Mod_Free( data ) Mem_Free( data )

void		Mod_StripLODSuffix( char *name );

void		Mod_Modellist_f( void );

#endif /*__R_MODEL_H__*/
