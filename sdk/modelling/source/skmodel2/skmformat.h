/*
type 2 model (hierarchical skeletal pose)

within this specification, int is assumed to be 32bit, float is assumed to be 32bit, char is assumed to be 8bit, text is assumed to be an array of chars with NULL termination

all values are x86 little endian

general notes:
a pose is a 4 floats quaternion (standard quaternion representation -> 3 complex components and a scalar)
parent bones must always be lower in number than their children, models will be rejected if this is not obeyed (can be fixed by modelling utilities)

utility notes:
if a hard edge is desired (faceted lighting, or a jump to another set of skin coordinates), vertices must be duplicated
ability to visually edit groupids of triangles is highly recommended
frame 0 is always the base pose (the one the skeleton was built for)

game notes:
the loader should be very thorough about error checking, all vertex and bone indices should be validated, etc
the gamecode can look up bone numbers by name using a builtin function, for use in attachment situations (the client should have the same model as the host of the gamecode in question - that is to say if the server gamecode is setting the bone number, the client and server must have vaguely compatible models so the client understands, and if the client gamecode is setting the bone number, the server could have a completely different model with no harm done)
frame 0 should be usable, not skipped

speed optimizations for the saver to do:
remove all unused data (unused bones, vertices, etc, be sure to check if bones are used for attachments however)
sort triangles into strips
sort vertices according to first use in a triangle (caching benefits) after sorting triangles

speed optimizations for the loader to do:
if the model only has one frame, process it at load time to create a simple static vertex mesh to render (this is a hassle, but it is rewarding to optimize all such models)

configuration file:
during the load engine searches for a .cfg file stored in the same directory as the loaded model.
possible commands for a config file:
'import path_to_skeleton_file' (without the quotes) -> imports skeletal animation from an .skp file
if no .cfg file is found, the default skeleton (the one in the same directory with the name matching the loaded model's name) is loaded
note that LOD's are forced to use the same skeleton as the main model

rendering process:
1*. one or two poses are looked up by number
2*. boneposes (matrices) are interpolated, building bone matrix array
3. bones are parsed sequentially, each bone's matrix is transformed by it's parent bone (which can be -1; the model to world matrix)
4. meshes are parsed sequentially, as follows:
  1. vertices are parsed sequentially and may be influenced by more than one bone (the results of the 3x4 matrix transform will be added together - weighting is already built into these)
  2. shader is looked up and called, passing vertex buffer (temporary) and triangle indices (which are stored in the mesh)
  5. rendering is complete

* - these stages can be replaced with completely dynamic animation instead of pose animations.
*/

// header for the entire file
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

// model format related flags
#define SKM_BONEFLAG_ATTACH		1
#define SKM_MODELTYPE			2	// (hierarchical skeletal pose)

typedef struct
{
	char			id[4];				// SKMHEADER
	unsigned int	type;
	unsigned int	filesize;	// size of entire model file

	unsigned int	num_bones;
	unsigned int	num_meshes;

	// this offset is relative to the file
	unsigned int	ofs_meshes;
} dskmheader_t;

// there may be more than one of these
typedef struct
{
	// these offsets are relative to the file
	char			shadername[SKM_MAX_NAME];		// name of the shader to use
	char			meshname[SKM_MAX_NAME];

	unsigned int	num_verts;
	unsigned int	num_tris;
	unsigned int	num_references;
	unsigned int	ofs_verts;	
	unsigned int	ofs_texcoords;
	unsigned int	ofs_indices;
	unsigned int	ofs_references;	// references (bones' numbers referenced to in this mesh)
} dskmmesh_t;

// one or more of these per vertex
typedef struct
{
	float			origin[3];		// vertex location (these blend)
	float			influence;		// influence fraction (these must add up to 1)
	float			normal[3];		// surface normal (these blend)
	unsigned int	bonenum;		// number of the bone
} dskmbonevert_t;

// variable size, parsed sequentially
typedef struct
{
	unsigned int	numbones;
	// immediately followed by 1 or more ddpmbonevert_t structures
	dskmbonevert_t	verts[1];
} dskmvertex_t;

typedef struct
{
	float	st[2];
} dskmcoord_t;

typedef struct
{
	char			id[4];			// SKMHEADER
	unsigned int	type;
	unsigned int	filesize;	// size of entire model file

	unsigned int	num_bones;
	unsigned int	num_frames;

	// these offsets are relative to the file
	unsigned int	ofs_bones;
	unsigned int	ofs_frames;
} dskpheader_t;

// one per bone
typedef struct
{
	// name examples: upperleftarm leftfinger1 leftfinger2 hand, etc
	char			name[SKM_MAX_NAME];
	signed int		parent;		// parent bone number (-1 indicates this is a parent bone)
	unsigned int	flags;		// flags for the bone
} dskpbone_t;

typedef struct
{
	float			quat[4];
	float			origin[3];
} dskpbonepose_t;

// immediately followed by bone positions for the frame
typedef struct
{
	// name examples: idle_1 idle_2 idle_3 shoot_1 shoot_2 shoot_3, etc
	char			name[SKM_MAX_NAME];
	unsigned int	ofs_bonepositions;
} dskpframe_t;

