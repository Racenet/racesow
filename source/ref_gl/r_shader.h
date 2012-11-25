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
#ifndef __R_SHADER_H__
#define __R_SHADER_H__

#define MAX_SHADERS					4096
#define MAX_SHADER_PASSES			8
#define MAX_SHADER_DEFORMVS			8
#define MAX_SHADER_ANIM_FRAMES		16
#define MAX_SHADER_TCMODS			8

// shader types
enum
{
	SHADER_INVALID = -1,
	SHADER_UNKNOWN,
	SHADER_BSP,
	SHADER_BSP_VERTEX,
	SHADER_BSP_FLARE,
	SHADER_MD3,
	SHADER_2D,
	SHADER_2D_RAW,
	SHADER_FARBOX,
	SHADER_NEARBOX,
	SHADER_PLANAR_SHADOW,
	SHADER_OPAQUE_OCCLUDER,
#ifdef HARDWARE_OUTLINES
	SHADER_OUTLINE,
#endif
	SHADER_VIDEO
};

// shader flags
enum
{
	SHADER_DEPTHWRITE				= 1 << 0,
	SHADER_SKY						= 1 << 1,
	SHADER_CULL_FRONT				= 1 << 2,
	SHADER_CULL_BACK				= 1 << 3,
	SHADER_VIDEOMAP					= 1 << 4,
	SHADER_MATERIAL					= 1 << 5,
	SHADER_DEFORMV_NORMAL			= 1 << 6,
	SHADER_ENTITY_MERGABLE			= 1 << 7,
	SHADER_NO_DEPTH_TEST			= 1 << 8,
	SHADER_AUTOSPRITE				= 1 << 9,
	SHADER_NO_MODULATIVE_DLIGHTS	= 1 << 10,
	SHADER_LIGHTMAP					= 1 << 11,
	SHADER_PORTAL					= 1 << 12,
	SHADER_PORTAL_CAPTURE			= 1 << 13,
	SHADER_PORTAL_CAPTURE2			= 1 << 14,
	SHADER_NO_TEX_FILTERING			= 1 << 15,
	SHADER_ALLDETAIL				= 1 << 16,
	SHADER_NODRAWFLAT				= 1 << 17,
	SHADER_VOLATILE					= 1 << 18
};

// sorting
enum
{
	SHADER_SORT_NONE				= 0,
	SHADER_SORT_PORTAL				= 1,
	SHADER_SORT_SKY					= 2,
	SHADER_SORT_OPAQUE				= 3,
	SHADER_SORT_DECAL				= 4,
	SHADER_SORT_ALPHATEST			= 5,
	SHADER_SORT_BANNER				= 6,
	SHADER_SORT_UNDERWATER			= 8,
	SHADER_SORT_ADDITIVE			= 9,
	SHADER_SORT_NEAREST				= 16
};

// shaderpass flags
#define SHADERPASS_MARK_BEGIN		0x20000 // same as GLSTATE_MARK_END
enum
{
	SHADERPASS_LIGHTMAP				= SHADERPASS_MARK_BEGIN,
	SHADERPASS_DETAIL				= SHADERPASS_MARK_BEGIN << 1,
	SHADERPASS_NOCOLORARRAY			= SHADERPASS_MARK_BEGIN << 2,
	SHADERPASS_DLIGHT				= SHADERPASS_MARK_BEGIN << 3,
	SHADERPASS_DELUXEMAP			= SHADERPASS_MARK_BEGIN << 4,
	SHADERPASS_PORTALMAP			= SHADERPASS_MARK_BEGIN << 5,
	SHADERPASS_STENCILSHADOW		= SHADERPASS_MARK_BEGIN << 6,
	SHADERPASS_GRAYSCALE			= SHADERPASS_MARK_BEGIN << 7,

	SHADERPASS_BLEND_REPLACE		= SHADERPASS_MARK_BEGIN << 8,
	SHADERPASS_BLEND_MODULATE		= SHADERPASS_MARK_BEGIN << 9,
	SHADERPASS_BLEND_ADD			= SHADERPASS_MARK_BEGIN << 10,
	SHADERPASS_BLEND_DECAL			= SHADERPASS_MARK_BEGIN << 11
};

#define SHADERPASS_BLENDMODE ( SHADERPASS_BLEND_REPLACE|SHADERPASS_BLEND_MODULATE|SHADERPASS_BLEND_ADD|SHADERPASS_BLEND_DECAL )

// transform functions
enum
{
	SHADER_FUNC_NONE,
	SHADER_FUNC_SIN,
	SHADER_FUNC_TRIANGLE,
	SHADER_FUNC_SQUARE,
	SHADER_FUNC_SAWTOOTH,
	SHADER_FUNC_INVERSESAWTOOTH,
	SHADER_FUNC_NOISE,
	SHADER_FUNC_CONSTANT,
	SHADER_FUNC_RAMP,

	MAX_SHADER_FUNCS
};

// RGB colors generation
enum
{
	RGB_GEN_UNKNOWN,
	RGB_GEN_IDENTITY,
	RGB_GEN_IDENTITY_LIGHTING,
	RGB_GEN_CONST,
	RGB_GEN_WAVE,
	RGB_GEN_ENTITYWAVE,
	RGB_GEN_ONE_MINUS_ENTITY,
	RGB_GEN_VERTEX,
	RGB_GEN_ONE_MINUS_VERTEX,
	RGB_GEN_LIGHTING_DIFFUSE,
	RGB_GEN_EXACT_VERTEX,
	RGB_GEN_LIGHTING_DIFFUSE_ONLY,
	RGB_GEN_LIGHTING_AMBIENT_ONLY,
	RGB_GEN_FOG,
	RGB_GEN_CUSTOMWAVE,
#ifdef HARDWARE_OUTLINES
	RGB_GEN_OUTLINE,
#endif
	RGB_GEN_ENVIRONMENT
};

// alpha channel generation
enum
{
	ALPHA_GEN_UNKNOWN,
	ALPHA_GEN_IDENTITY,
	ALPHA_GEN_CONST,
	ALPHA_GEN_PORTAL,
	ALPHA_GEN_VERTEX,
	ALPHA_GEN_ONE_MINUS_VERTEX,
	ALPHA_GEN_ENTITY,
	ALPHA_GEN_SPECULAR,
	ALPHA_GEN_WAVE,
	ALPHA_GEN_DOT,
	ALPHA_GEN_ONE_MINUS_DOT,
#ifdef HARDWARE_OUTLINES
	ALPHA_GEN_OUTLINE,
#endif
	ALPHA_GEN_PLANAR_SHADOW
};

// texture coordinates generation
enum
{
	TC_GEN_NONE,
	TC_GEN_BASE,
	TC_GEN_LIGHTMAP,
	TC_GEN_ENVIRONMENT,
	TC_GEN_VECTOR,
	TC_GEN_REFLECTION,
	TC_GEN_FOG,
	TC_GEN_REFLECTION_CELLSHADE,
	TC_GEN_SVECTORS,
	TC_GEN_PROJECTION,
	TC_GEN_PROJECTION_SHADOW,
	TC_GEN_DRAWFLAT
};

// tcmod functions
enum
{
	TC_MOD_NONE,
	TC_MOD_SCALE,
	TC_MOD_SCROLL,
	TC_MOD_ROTATE,
	TC_MOD_TRANSFORM,
	TC_MOD_TURB,
	TC_MOD_STRETCH
};

// vertices deformation
enum
{
	DEFORMV_NONE,
	DEFORMV_WAVE,
	DEFORMV_NORMAL,
	DEFORMV_BULGE,
	DEFORMV_MOVE,
	DEFORMV_AUTOSPRITE,
	DEFORMV_AUTOSPRITE2,
	DEFORMV_PROJECTION_SHADOW,
	DEFORMV_AUTOPARTICLE
#ifdef HARDWARE_OUTLINES
	,DEFORMV_OUTLINE
#endif
};

// volatile flags for shaders
// a volatile shader can change its behavior based on cvars
// or global flags and thus may need to be reloaded between
// map loads (or maybe even on a cvar value change?)
#define SHADER_VOLATILE_MAPCONFIG_DELUXEMAPPING		1<<0


typedef struct
{
	unsigned short		type;			// SHADER_FUNC enum
	float				args[4];		// offset, amplitude, phase_offset, rate
} shaderfunc_t;

typedef struct
{
	unsigned short		type;
	float				args[6];
} tcmod_t;

typedef struct
{
	unsigned short		type;
	float				*args;
	shaderfunc_t		*func;
} colorgen_t;

typedef struct
{
	unsigned short		type;
	float				args[4];
	shaderfunc_t		func;
} deformv_t;

// Per-pass rendering state information
typedef struct
{
	unsigned int		flags;

	colorgen_t			rgbgen;
	colorgen_t			alphagen;

	unsigned short		tcgen;
	vec_t				*tcgenVec;

	unsigned short		numtcmods;
	tcmod_t				*tcmods;

	unsigned int		cin;

	const char			*program;
	unsigned short		program_type;

	float				anim_fps;						// animation frames per sec
	unsigned short		anim_numframes;
	image_t				*anim_frames[MAX_SHADER_ANIM_FRAMES];	// texture refs
} shaderpass_t;

// Shader information
typedef struct shader_s
{
	char				*name;
	int					registration_sequence;

	unsigned int		flags;
	unsigned int		features;
	unsigned int		sort;
	unsigned int		sortkey;
	unsigned int		volatileFlags;

	int					type;

	unsigned short		numpasses;
	shaderpass_t		*passes;

	unsigned short		numdeforms;
	deformv_t			*deforms;

	qbyte				fog_color[4];
	float				fog_dist, fog_clearDist;

	float				gloss_exponent;
	float				offsetmapping_scale;

	struct skydome_s	*skydome;

	struct shader_s		*prev, *next;
} shader_t;

// memory management
extern mempool_t *r_shadersmempool;

extern shader_t	r_shaders[MAX_SHADERS];
extern int r_numShaders;

#define		Shader_Malloc( size ) Mem_Alloc( r_shadersmempool, size )
#define		Shader_Realloc( data, size ) Mem_Realloc( data, size )
#define		Shader_Free( data ) Mem_Free( data )
#define		Shader_Sortkey( shader, sort ) ( ( ( sort )<<26 )|( shader-r_shaders ) )

#define 	Shader_UseTextureFog(s) ( ( (s)->sort <= SHADER_SORT_ALPHATEST && \
				( (s)->flags & ( SHADER_DEPTHWRITE|SHADER_SKY ) ) ) || (s)->fog_dist )

void		R_InitShaders( void );
void		R_ShutdownShaders( void );

void		R_UploadCinematicShader( const shader_t *shader );

shader_t	*R_LoadShader( const char *name, int type, qboolean forceDefault, int addFlags, int ignoreType, const char *text );

shader_t	*R_RegisterPic( const char *name );
shader_t	*R_RegisterRawPic( const char *name, int width, int height, qbyte *data );
shader_t	*R_RegisterLevelshot( const char *name, shader_t *defaultShader, qboolean *matchesDefault );
shader_t	*R_RegisterShader( const char *name );
shader_t	*R_RegisterSkin( const char *name );
shader_t	*R_RegisterVideo( const char *name );

void		R_TouchShader( shader_t *s );
void		R_FreeUnusedShaders( void );

void		R_RemapShader( const char *from, const char *to, int timeOffset );

void		R_ShaderList_f( void );
void		R_ShaderDump_f( void );

#endif /*__R_SHADER_H__*/
