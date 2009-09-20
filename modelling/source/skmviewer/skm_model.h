
//-------------------------------------------------------
//
//		
//
//-------------------------------------------------------



//SHUT UPs
typedef struct
{
	int		shutup;
} shader_t;

typedef unsigned int index_t;

/*
========================================================================

.SKM and .SKP models file formats

========================================================================
*/

#define SKMHEADER				"SKM1"

#define SKM_MAX_NAME			64
#define SKM_MAX_MESHES			32
#define SKM_MAX_FRAMES			65536
#define SKM_MAX_TRIS			65536
#define SKM_MAX_VERTS			(SKM_MAX_TRIS * 3)
#define SKM_MAX_BONES			256
#define SKM_MAX_SHADERS			256
#define SKM_MAX_FILESIZE		16777216
#define SKM_MAX_ATTACHMENTS		SKM_MAX_BONES
#define SKM_MAX_LODS			4


#define SKM_MODELTYPE			2	// (hierarchical skeletal pose)

// model format related flags
#define SKM_BONEFLAG_ATTACH			1



typedef struct
{
	char id[4];				// SKMHEADER
	unsigned int type;
	unsigned int filesize;	// size of entire model file

	unsigned int num_bones;
	unsigned int num_meshes;

	// this offset is relative to the file
	unsigned int ofs_meshes;
} dskmheader_t;

// there may be more than one of these
typedef struct
{
	// these offsets are relative to the file
	char shadername[SKM_MAX_NAME];		// name of the shader to use
	char meshname[SKM_MAX_NAME];

	unsigned int num_verts;
	unsigned int num_tris;
	unsigned int num_references;
	unsigned int ofs_verts;	
	unsigned int ofs_texcoords;
	unsigned int ofs_indices;
	unsigned int ofs_references;
} dskmmesh_t;

// one or more of these per vertex
typedef struct
{
	float origin[3];		// vertex location (these blend)
	float influence;		// influence fraction (these must add up to 1)
	float normal[3];		// surface normal (these blend)
	unsigned int bonenum;	// number of the bone
} dskmbonevert_t;

// variable size, parsed sequentially
typedef struct
{
	unsigned int numbones;
	// immediately followed by 1 or more ddpmbonevert_t structures
	dskmbonevert_t verts[1];
} dskmvertex_t;

typedef struct
{
	float			st[2];
} dskmcoord_t;

typedef struct
{
	char id[4];				// SKMHEADER
	unsigned int type;
	unsigned int filesize;	// size of entire model file

	unsigned int num_bones;
	unsigned int num_frames;

	// these offsets are relative to the file
	unsigned int ofs_bones;
	unsigned int ofs_frames;
} dskpheader_t;

// one per bone
typedef struct
{
	// name examples: upperleftarm leftfinger1 leftfinger2 hand, etc
	char name[SKM_MAX_NAME];
	signed int parent;		// parent bone number
	unsigned int flags;		// flags for the bone
} dskpbone_t;

typedef struct
{
	float quat[4];
	float origin[3];
} dskpbonepose_t;

// immediately followed by bone positions for the frame
typedef struct
{
	// name examples: idle_1 idle_2 idle_3 shoot_1 shoot_2 shoot_3, etc
	char name[SKM_MAX_NAME];
	unsigned int ofs_bonepositions;
} dskpframe_t;




/*
==============================================================================

SKELETAL MODELS (in memory)

==============================================================================
*/

//
// in memory representation
//

//jalfixme vec2 (I want to avoid vec structs)
typedef float vec;
typedef vec vec2[2];
typedef vec vec3[3];

typedef struct
{
	float			quat[4];
	float			origin[3];
} mskbonepose_t;

typedef struct
{
	float			origin[3];
	float			influence;
	float			normal[3];
	unsigned int	bonenum;
} mskbonevert_t;

typedef struct
{
	unsigned int	numbones;
	mskbonevert_t	*verts;
} mskvertex_t;

typedef struct
{
	shader_t		*shader;
} mskskin_t;

typedef struct
{
	//--------------------------------------
	char			shadername[SKM_MAX_NAME];	//it's not loaded up in QFusion
	//--------------------------------------
	char			name[SKM_MAX_NAME];

	unsigned int	numverts;
	mskvertex_t		*vertexes;
	vec2			*stcoords;

	unsigned int	numtris;
	index_t			*indexes;

	unsigned int	numreferences;
	unsigned int	*references;

#if SHADOW_VOLUMES
	int				*trneighbors;
#endif

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
	mskbonepose_t	*boneposes;

	float			mins[3], maxs[3];
	float			radius;					// for clipping uses

	//---------------------------------
	char			name[SKM_MAX_NAME];	//frame name is ignored at QFusion
	//---------------------------------
} mskframe_t;

typedef struct
{
	unsigned int	numbones;
	mskbone_t		*bones;

	unsigned int	nummeshes;
	mskmesh_t		*meshes;

	unsigned int	numframes;
	mskframe_t		*frames;
} mskmodel_t;


