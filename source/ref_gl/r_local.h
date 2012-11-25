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
#ifndef __R_LOCAL_H__
#define __R_LOCAL_H__

#include "../qcommon/qcommon.h"

#include "r_glimp.h"
#include "r_public.h"

typedef unsigned int elem_t;

/*

skins will be outline flood filled and mip mapped
pics and sprites with alpha will be outline flood filled
pic won't be mip mapped

model skin
sprite frame
wall texture
pic

*/

enum
{
	IT_NONE
	,IT_CLAMP			= 1<<0
	,IT_NOMIPMAP		= 1<<1
	,IT_NOPICMIP		= 1<<2
	,IT_SKY				= 1<<3
	,IT_CUBEMAP			= 1<<4
	,IT_HEIGHTMAP		= 1<<5
	,IT_FLIPX			= 1<<6
	,IT_FLIPY			= 1<<7
	,IT_FLIPDIAGONAL	= 1<<8
	,IT_NOCOMPRESS		= 1<<9
	,IT_DEPTH			= 1<<10
	,IT_NORMALMAP		= 1<<11
	,IT_FRAMEBUFFER		= 1<<12
	,IT_NOFILTERING		= 1<<13
	,IT_WAL				= 1<<14
	,IT_MIPTEX			= 1<<15
	,IT_MIPTEX_FULLBRIGHT = 1<<16
	,IT_LEFTHALF		= 1<<17
	,IT_RIGHTHALF		= 1<<18
	,IT_LUMINANCE		= 1<<19
	,IT_BGRA			= 1<<20
};

#define IT_CINEMATIC		( IT_NOPICMIP|IT_NOMIPMAP|IT_CLAMP|IT_NOCOMPRESS )
#define IT_PORTALMAP		( IT_NOMIPMAP|IT_NOCOMPRESS|IT_NOPICMIP|IT_CLAMP )
#define IT_SHADOWMAP		( IT_NOMIPMAP|IT_NOCOMPRESS|IT_NOPICMIP|IT_CLAMP|IT_NOCOMPRESS|IT_DEPTH )

typedef struct image_s
{
	char			*name;						// game path, not including extension
	int				registration_sequence;

	char			extension[8];				// file extension
	int				flags;
	GLuint			texnum;						// gl texture binding
	int				width, height, depth;		// source image
	int				upload_width,
					upload_height,
					upload_depth;				// after power of two and picmip
	int				samples;
	int				fbo;						// frame buffer object texture is attached to
	unsigned int	framenum;					// r_framecount texture was updated (rendered to)
	float			bumpScale;
	struct image_s	*next, *prev;
} image_t;

enum
{
	TEXTURE_UNIT0,
	TEXTURE_UNIT1,
	TEXTURE_UNIT2,
	TEXTURE_UNIT3,
	TEXTURE_UNIT4,
	TEXTURE_UNIT5,
	TEXTURE_UNIT6,
	TEXTURE_UNIT7,
	MAX_TEXTURE_UNITS
};

#define DRAWFLAT_NORMAL_STEP	0.5		// floor or ceiling if < abs(normal.z) 
#define DRAWFLAT_TEXTURE_DEPTH	128

#define FOG_TEXTURE_WIDTH		256
#define FOG_TEXTURE_HEIGHT		32

#ifdef CGAMEGETLIGHTORIGIN
#define SHADOW_PLANAR			2
#else
#define SHADOW_PLANAR			1
#endif

#define SHADOW_MAPPING			( SHADOW_PLANAR+1 )

#define SUBDIVISIONS_MIN		3
#define SUBDIVISIONS_MAX		16
#define SUBDIVISIONS_DEFAULT	5

//===================================================================

#include "r_math.h"
#include "r_mesh.h"
#include "r_shader.h"
#include "r_backend.h"
#include "r_shadow.h"
#include "r_model.h"

#define BACKFACE_EPSILON		0.01

#define	ON_EPSILON				0.1         // point on plane side epsilon

#define Z_NEAR					4

#define	SIDE_FRONT				0
#define	SIDE_BACK				1
#define	SIDE_ON					2

#define RP_NONE					0x0
#define RP_MIRRORVIEW			0x1     // lock pvs at vieworg
#define RP_PORTALVIEW			0x2
#define RP_ENVVIEW				0x4
#define RP_NOSKY				0x8
#define RP_SKYPORTALVIEW		0x10
#define RP_OLDVIEWCLUSTER		0x20
#define RP_SHADOWMAPVIEW		0x40
#define RP_FLIPFRONTFACE		0x80
#define RP_WORLDSURFVISIBLE		0x100
#define RP_CLIPPLANE			0x200
#define RP_TRISOUTLINES			0x400
#define RP_SHOWNORMALS			0x800
#define RP_PVSCULL				0x1000
#define RP_NOVIS				0x2000
#define RP_BACKFACECULL			0x4000
#define RP_NOENTS				0x8000
#define RP_LIGHTMAP				0x10000
#define RP_DRAWFLAT				0x20000

#define RP_NONVIEWERREF			( RP_PORTALVIEW|RP_MIRRORVIEW|RP_ENVVIEW|RP_SKYPORTALVIEW|RP_SHADOWMAPVIEW )

//====================================================

typedef struct
{
	vec3_t			origin;
	vec3_t			color;
	vec3_t			mins, maxs;
	float			intensity;
	const shader_t	*shader;
} dlight_t;

typedef struct superLightStyle_s
{
	int				features;
	int				lightmapNum[MAX_LIGHTMAPS];
	int				lightmapStyles[MAX_LIGHTMAPS];
	int				vertexStyles[MAX_LIGHTMAPS];
	float			stOffset[MAX_LIGHTMAPS][2];
} superLightStyle_t;

typedef struct
{
	unsigned int	params;					// rendering parameters

	refdef_t		refdef;
	int				scissor[4];
	int				viewport[4];
	meshlist_t		*meshlist;				// meshes to be rendered

	unsigned int	shadowBits;
	shadowGroup_t	*shadowGroup;

	entity_t		*currententity;
	model_t			*currentmodel;
	entity_t		*previousentity;

	//
	// view origin
	//
	vec3_t			viewOrigin;
	vec3_t			viewAxis[3];
	cplane_t		frustum[6];
	float			farClip;
	unsigned int	clipFlags;
	vec3_t			visMins, visMaxs;

	mat4x4_t		objectMatrix;
	mat4x4_t		worldviewMatrix;
	mat4x4_t		modelviewMatrix;			// worldviewMatrix * objectMatrix

	mat4x4_t		projectionMatrix;
	mat4x4_t		worldviewProjectionMatrix;	// worldviewMatrix * projectionMatrix

	float			skyMins[2][6];
	float			skyMaxs[2][6];

	float			lod_dist_scale_for_fov;

	mfog_t			*fog_eye;
	float			fog_dist_to_eye[MAX_MAP_FOGS];

	vec3_t			lodOrigin;
	vec3_t			pvsOrigin;
	cplane_t		clipPlane;
	cplane_t		portalPlane;
} refinst_t;

//====================================================

typedef struct r_frontend_s
{
	// any asset (model, shader, texture, etc) with has not been registered
	// or "touched" during the last registration sequence will be freed
	int				registration_sequence;

	// FIXME: move most of the global variables below here
	vec3_t			wallColor, floorColor;
} r_frontend_t;

extern r_frontend_t r_front;

#define MAX_PORTAL_TEXTURES	512

extern image_t *r_cintexture;
extern image_t *r_portaltextures[];
extern image_t *r_notexture;
extern image_t *r_whitetexture;
extern image_t *r_blacktexture;
extern image_t *r_greytexture;
extern image_t *r_blankbumptexture;
extern image_t *r_particletexture;
extern image_t *r_dlighttexture;
extern image_t *r_fogtexture;
extern image_t *r_coronatexture;
extern image_t *r_drawflattexture;
extern image_t *r_shadowmapTextures[];

extern unsigned int r_pvsframecount;
extern unsigned int r_framecount;
extern unsigned int c_brush_polys, c_world_leafs, c_world_verts, c_world_tris, c_world_vbos;

extern unsigned int r_mark_leaves, r_world_node;
extern unsigned int r_add_polys, r_add_entities;
extern unsigned int r_sort_meshes, r_draw_meshes;

extern msurface_t *r_debug_surface;

extern int gl_filter_min, gl_filter_max;

#define MAX_RSPEEDSMSGSIZE	1024
extern char r_speeds_msg[MAX_RSPEEDSMSGSIZE];

extern float gldepthmin, gldepthmax;

#define R_ENT2NUM(ent) ((ent)-r_entities)
#define R_NUM2ENT(num) (r_entities+(num))

extern unsigned int r_numEntities;
extern entity_t	r_entities[MAX_ENTITIES];

extern unsigned int r_numDlights;
extern dlight_t	r_dlights[MAX_DLIGHTS];

extern unsigned int r_numPolys;
extern poly_t r_polys[MAX_POLYS];

extern lightstyle_t r_lightStyles[MAX_LIGHTSTYLES];

extern refdef_t	r_lastRefdef;

extern int r_viewcluster, r_oldviewcluster, r_viewarea;

extern float r_farclip_min, r_farclip_bias;

extern entity_t	*r_worldent;
extern model_t *r_worldmodel;
extern mbrushmodel_t *r_worldbrushmodel;

extern cvar_t *r_norefresh;
extern cvar_t *r_drawentities;
extern cvar_t *r_drawworld;
extern cvar_t *r_speeds;
extern cvar_t *r_drawelements;
extern cvar_t *r_fullbright;
extern cvar_t *r_lightmap;
extern cvar_t *r_novis;
extern cvar_t *r_nocull;
extern cvar_t *r_lerpmodels;
extern cvar_t *r_ignorehwgamma;
extern cvar_t *r_overbrightbits;
extern cvar_t *r_mapoverbrightbits;

extern cvar_t *r_flares;
extern cvar_t *r_flaresize;
extern cvar_t *r_flarefade;

extern cvar_t *r_dynamiclight;
extern cvar_t *r_coronascale;
extern cvar_t *r_detailtextures;
extern cvar_t *r_subdivisions;
extern cvar_t *r_faceplanecull;
extern cvar_t *r_showtris;
extern cvar_t *r_shownormals;
extern cvar_t *r_draworder;

extern cvar_t *r_fastsky;
extern cvar_t *r_portalonly;
extern cvar_t *r_portalmaps;
extern cvar_t *r_portalmaps_maxtexsize;

extern cvar_t *r_lighting_bumpscale;
extern cvar_t *r_lighting_deluxemapping;
extern cvar_t *r_lighting_useblankbumpmap;
extern cvar_t *r_lighting_diffuse2heightmap;
extern cvar_t *r_lighting_specular;
extern cvar_t *r_lighting_glossintensity;
extern cvar_t *r_lighting_glossexponent;
extern cvar_t *r_lighting_ambientscale;
extern cvar_t *r_lighting_directedscale;
extern cvar_t *r_lighting_packlightmaps;
extern cvar_t *r_lighting_maxlmblocksize;
extern cvar_t *r_lighting_vertexlight;
extern cvar_t *r_lighting_maxglsldlights;
extern cvar_t *r_lighting_grayscale;

extern cvar_t *r_offsetmapping;
extern cvar_t *r_offsetmapping_scale;
extern cvar_t *r_offsetmapping_reliefmapping;

extern cvar_t *r_occlusion_queries;
extern cvar_t *r_occlusion_queries_finish;

extern cvar_t *r_shadows;
extern cvar_t *r_shadows_alpha;
extern cvar_t *r_shadows_nudge;
extern cvar_t *r_shadows_projection_distance;
extern cvar_t *r_shadows_maxtexsize;
extern cvar_t *r_shadows_pcf;
extern cvar_t *r_shadows_self_shadow;
extern cvar_t *r_shadows_dither;

extern cvar_t *r_bloom;
extern cvar_t *r_bloom_alpha;
extern cvar_t *r_bloom_diamond_size;
extern cvar_t *r_bloom_intensity;
extern cvar_t *r_bloom_darken;
extern cvar_t *r_bloom_sample_size;
extern cvar_t *r_bloom_fast_sample;

#ifdef HARDWARE_OUTLINES
extern cvar_t *r_outlines_world;
extern cvar_t *r_outlines_scale;
extern cvar_t *r_outlines_cutoff;
#endif

extern cvar_t *r_lodbias;
extern cvar_t *r_lodscale;

extern cvar_t *r_gamma;
extern cvar_t *r_texturebits;
extern cvar_t *r_texturemode;
extern cvar_t *r_texturefilter;
extern cvar_t *r_mode;
extern cvar_t *r_nobind;
extern cvar_t *r_picmip;
extern cvar_t *r_skymip;
extern cvar_t *r_clear;
extern cvar_t *r_polyblend;
extern cvar_t *r_lockpvs;
extern cvar_t *r_screenshot_fmtstr;
extern cvar_t *r_screenshot_jpeg;
extern cvar_t *r_screenshot_jpeg_quality;
extern cvar_t *r_swapinterval;

extern cvar_t *r_temp1;

extern cvar_t *r_drawflat;
extern cvar_t *r_wallcolor;
extern cvar_t *r_floorcolor;

extern cvar_t *r_maxglslbones;

extern cvar_t *gl_finish;
extern cvar_t *gl_cull;
extern cvar_t *gl_extensions;

extern cvar_t *vid_displayfrequency;
extern cvar_t *vid_multiscreen_head;

//====================================================================

static inline qbyte R_FloatToByte( float x )
{
	union {
		float f;
		unsigned int i;
	} f2i;

	// shift float to have 8bit fraction at base of number
	f2i.f = x + 32768.0f;
	f2i.i &= 0x7FFFFF;

	// then read as integer and kill float bits...
	return ( qbyte )min( f2i.i, 255 );
}

float		R_FastSin( float t );
void		R_LatLongToNorm( const qbyte latlong[2], vec3_t out );

//====================================================================

//
// r_alias.c
//
qboolean	R_CullAliasModel( entity_t *e );
void		R_AddAliasModelToList( entity_t *e );
void		R_DrawAliasModel( const meshbuffer_t *mb );
qboolean	R_AliasModelLerpTag( orientation_t *orient, maliasmodel_t *aliasmodel, int framenum, int oldframenum,
								float lerpfrac, const char *name );
float		R_AliasModelBBox( entity_t *e, vec3_t mins, vec3_t maxs );
void		R_AliasModelFrameBounds( const model_t *mod, int frame, vec3_t mins, vec3_t maxs );

//
// r_bloom.c
//
void		R_InitBloomTextures( void );
void		R_TouchBloomTextures( void );
void		R_BloomBlend( const refdef_t *fd );

//
// r_cin.c
//
void		R_InitCinematics( void );
void		R_ShutdownCinematics( void );
unsigned int R_StartCinematic( const char *arg );
void		R_FreeCinematic( unsigned int id );
void		R_RunAllCinematics( void );
void		R_TouchCinematic( unsigned int id );
void		R_FreeUnusedCinematics( void );
image_t		*R_UploadCinematic( unsigned int id );

//
// r_cull.c
//
enum
{
	OQ_NONE = -1,
	OQ_ENTITY,
	OQ_PLANARSHADOW,
	OQ_SHADOWGROUP,
	OQ_CUSTOM
};

#define OCCLUSION_QUERIES_CVAR_HACK( ri ) ( !(r_occlusion_queries->integer == 2 && r_shadows->integer != SHADOW_MAPPING)/* \
											|| ((ri).refdef.rdflags & RDF_PORTALINVIEW)*/ )
#define OCCLUSION_QUERIES_ENABLED( ri )	( glConfig.ext.occlusion_query && r_occlusion_queries->integer && r_drawentities->integer \
											&& !((ri).params & RP_NONVIEWERREF) && !((ri).refdef.rdflags & RDF_NOWORLDMODEL) \
											&& OCCLUSION_QUERIES_CVAR_HACK( ri ) )
#define OCCLUSION_OPAQUE_SHADER( s )	( ((s)->sort == SHADER_SORT_OPAQUE ) && ((s)->flags & SHADER_DEPTHWRITE ) && !(s)->numdeforms )
#define OCCLUSION_TEST_ENTITY( e )		( ((e)->flags & (RF_OCCLUSIONTEST|RF_WEAPONMODEL)) == RF_OCCLUSIONTEST )

qboolean	R_CullBox( const vec3_t mins, const vec3_t maxs, const unsigned int clipflags );
qboolean	R_CullSphere( const vec3_t centre, const float radius, const unsigned int clipflags );
qboolean	R_VisCullBox( const vec3_t mins, const vec3_t maxs );
qboolean	R_VisCullSphere( const vec3_t origin, float radius );
int			R_CullModel( entity_t *e, vec3_t mins, vec3_t maxs, float radius );
qboolean	R_CullBrushModel( entity_t *e );
qboolean	R_CullSprite( entity_t *e );

void		R_InitOcclusionQueries( void );
void		R_BeginOcclusionPass( void );
shader_t	*R_OcclusionShader( void );
void		R_AddOccludingSurface( msurface_t *surf, shader_t *shader );
int			R_GetOcclusionQueryNum( int type, int key );
int			R_IssueOcclusionQuery( int query, entity_t *e, vec3_t mins, vec3_t maxs );
qboolean	R_OcclusionQueryIssued( int query );
unsigned int R_GetOcclusionQueryResult( int query, qboolean wait );
qboolean	R_GetOcclusionQueryResultBool( int type, int key, qboolean wait );
void		R_EndOcclusionPass( void );
void		R_ShutdownOcclusionQueries( void );

//
// r_framebuffer.c
//
void		R_InitFBObjects( void );
int			R_RegisterFBObject( void );
void		R_TouchFBObject( int object );
qboolean	R_UseFBObject( int object );
int			R_ActiveFBObject( void );
qboolean	R_AttachTextureToFBOject( int object, image_t *texture, qboolean depthOnly );
qboolean	R_CheckFBObjectStatus( void );
void		R_FreeUnusedFBObjects( void );
void		R_ShutdownFBObjects( void );

//
// r_image.c
//
void		GL_SelectTexture( int tmu );
void		GL_Bind( int tmu, image_t *tex );
void		GL_TexEnv( GLenum mode );
void		GL_LoadTexMatrix( const mat4x4_t m );
void		GL_LoadIdentityTexMatrix( void );
void		GL_EnableTexGen( int coord, int mode );
void		GL_SetTexCoordArrayMode( int mode );
qboolean 	GL_IsAlphaBlending( int blendsrc, int blenddst );
void		GL_EnableVertexAttrib( int index, qboolean enable );

void		R_InitImages( void );
void		R_TouchImage( image_t *image );
void		R_FreeImage( image_t *image );
void		R_FreeUnusedImages( void );
void		R_ShutdownImages( void );
int			R_FindPortalTextureSlot( const char *key, int id );
void		R_InitPortalTexture( image_t **texture, const char *key, int id, int screenWidth, int screenHeight, int flags );
void		R_InitShadowmapTexture( image_t **texture, int id, int screenWidth, int screenHeight, int flags );
void		R_InitDrawFlatTexture( void );
void		R_FreeImageBuffers( void );

void		R_ScreenShot_f( void );
void		R_EnvShot_f( void );
void		R_ImageList_f( void );
void		R_TextureMode( char *string );
void		R_AnisotropicFilter( int value );

image_t		*R_LoadPic( const char *name, qbyte **pic, int width, int height, int flags, int samples );
image_t		*R_FindImage( const char *name, const char *suffix, int flags, float bumpScale );

void		R_Upload32( qbyte **data, int width, int height, int flags, int *upload_width, int *upload_height,
					   int *samples, int inSamples, qboolean subImage );

qboolean	R_MiptexHasFullbrights( qbyte *pixels, int width, int height );

//
// r_light.c
//
#define DLIGHT_SCALE	    0.5f
#define MAX_SUPER_STYLES    128

void		R_LightBounds( const vec3_t origin, float intensity, vec3_t mins, vec3_t maxs );
qboolean	R_SurfPotentiallyLit( msurface_t *surf );
unsigned int R_AddSurfDlighbits( msurface_t *surf, unsigned int dlightbits );
void		R_AddDynamicLights( unsigned int dlightbits, int state );
void		R_LightForEntity( entity_t *e, qbyte *bArray );
void		R_LightForOrigin( const vec3_t origin, vec3_t dir, vec4_t ambient, vec4_t diffuse, float radius );
void		R_BuildLightmaps( model_t *mod, int numLightmaps, int w, int h, const qbyte *data, mlightmapRect_t *rects );
void		R_InitLightStyles( model_t *mod );
int			R_AddSuperLightStyle( model_t *mod, const int *lightmaps, const qbyte *lightmapStyles, const qbyte *vertexStyles,
								 mlightmapRect_t **lmRects );
void		R_SortSuperLightStyles( model_t *mod );
void		R_TouchLightmapImages( model_t *mod );

void		R_InitCoronas( void );
void		R_DrawCoronas( void );
void		R_ShutdownCoronas( void );

//
// r_main.c
//
#define R_FASTSKY() (r_fastsky->integer || r_viewcluster == -1)

void		GL_DepthRange( float depthmin, float depthmax );
void		GL_Cull( int cull );
void		GL_SetState( int state );
void		GL_FrontFace( int front );
void		GL_PolygonOffset( float factor, float offset );
void 		GL_BindBuffer( int target, int buffer );
void		GL_Scissor( int x, int y, int w, int h );
void		GL_Viewport( int x, int y, int w, int h );

void		R_BeginFrame( float cameraSeparation, qboolean forceClear );
void		R_EndFrame( void );
void		R_RenderView( const refdef_t *fd );
const char *R_SpeedsMessage( char *out, size_t size );

mfog_t		*R_FogForSphere( const vec3_t centre, const float radius );
qboolean	R_CompletelyFogged( mfog_t *fog, vec3_t origin, float radius );

void		R_LoadIdentity( void );
void		R_RotateForEntity( entity_t *e );
void		R_TranslateForEntity( entity_t *e );
void		R_TransformToScreen_Vec3( const vec3_t in, vec3_t out );
void		R_TransformVectorToScreen( const refdef_t *rd, const vec3_t in, vec2_t out );
void		R_TransformEntityBBox( entity_t *e, vec3_t mins, vec3_t maxs, vec3_t bbox[8], qboolean local );
qboolean	R_ScissorForBounds( vec3_t bbox[8], int *x, int *y, int *w, int *h );
qboolean	R_ScissorForEntity( entity_t *ent, vec3_t mins, vec3_t maxs, int *x, int *y, int *w, int *h );

void		R_AddDebugBounds( const vec3_t mins, const vec3_t maxs );

qboolean	R_SpriteOverflow( void );
qboolean	R_PushSpriteModel( const meshbuffer_t *mb );
qboolean	R_PushSpritePoly( const meshbuffer_t *mb );

void		R_DrawStretchBegin( int numVerts, int numElems, const shader_t *shader, float x_offset, float y_offset );
void		R_DrawStretchEnd( const mesh_t *mesh, mesh_vbo_t *vbo, int features );

#define NUM_CUSTOMCOLORS	16
void		R_InitCustomColors( void );
void		R_SetCustomColor( int num, int r, int g, int b );
int			R_GetCustomColor( int num );
void		R_ShutdownCustomColors( void );

#ifdef HARDWARE_OUTLINES
void		R_InitOutlines( void );
void		R_AddModelMeshOutline( unsigned int modhandle, const mfog_t *fog, int meshnum );
void		R_ShutdownOutlines( void );
#endif

#define ENTITY_OUTLINE(ent) (( !(ri.params & RP_MIRRORVIEW) && ((ent)->renderfx & RF_VIEWERMODEL) ) ? 0 : (ent)->outlineHeight)

msurface_t *R_TraceLine( trace_t *tr, const vec3_t start, const vec3_t end, int surfumask );

//
// r_mesh.c
//

extern meshlist_t r_worldlist, r_shadowlist;

void		R_InitMeshLists( void );
void		R_FreeMeshLists( void );
void		R_ClearMeshList( meshlist_t *meshlist );
void		R_AllocWorldMeshLists( void );
meshbuffer_t *R_AddMeshToList( int type, const mfog_t *fog, const shader_t *shader, int infokey, const mesh_t *mesh, unsigned short numVerts, unsigned short numElems );
void		R_AddModelMeshToList( unsigned int modhandle, const mesh_vbo_t *vbo, const mfog_t *fog, const shader_t *shader, int meshnum, float distance, int numVerts, int numElems );

void		R_SortMeshes( void );
void		R_DrawMeshes( void );
void		R_DrawTriangleOutlines( qboolean showTris, qboolean showNormals );
void		R_DrawPortals( void );

void		R_DrawCubemapView( vec3_t origin, vec3_t angles, int size );
qboolean	R_DrawSkyPortal( skyportal_t *skyportal, vec3_t mins, vec3_t maxs );

void		R_BuildTangentVectors( int numVertexes, vec4_t *xyzArray, vec4_t *normalsArray, vec2_t *stArray,
								  int numTris, elem_t *elems, vec4_t *sVectorsArray );

const char	*R_PortalKeyForPlane( cplane_t *plane );

//
// r_program.c
//
typedef quint64 r_glslfeat_t;

#define GLSL_BIT(x)					(1ULL << (x))
#define GLSL_BITS_VERSION			1

#define MAX_GLSL_BONES				100

#define DEFAULT_GLSL_PROGRAM		"*r_defaultMaterialProgram"
#define DEFAULT_GLSL_DISTORTION_PROGRAM	"*r_defaultDistortionProgram"
#define DEFAULT_GLSL_SHADOWMAP_PROGRAM	"*r_defaultShadowmapProgram"
#define DEFAULT_GLSL_OUTLINE_PROGRAM "*r_defaultOutlineProgram"
#define DEFAULT_GLSL_TURBULENCE_PROGRAM "*r_defaultTurbulenceProgram"
#define DEFAULT_GLSL_DYNAMIC_LIGHTS_PROGRAM "*r_defaultDynamicLightsProgram"
#define DEFAULT_GLSL_Q3A_SHADER_PROGRAM "*r_defaultQ3AShaderProgram"
#define DEFAULT_GLSL_PLANAR_SHADOW_PROGRAM "*r_defaultPlanarShadowProgram"
#define DEFAULT_GLSL_CELLSHADE_PROGRAM "*r_defaultCellshadeProgram"

// program types
enum
{
	GLSL_PROGRAM_TYPE_NONE,
	GLSL_PROGRAM_TYPE_MATERIAL,
	GLSL_PROGRAM_TYPE_DISTORTION,
	GLSL_PROGRAM_TYPE_SHADOWMAP,
	GLSL_PROGRAM_TYPE_OUTLINE,
	GLSL_PROGRAM_TYPE_TURBULENCE,
	GLSL_PROGRAM_TYPE_DYNAMIC_LIGHTS,
	GLSL_PROGRAM_TYPE_Q3A_SHADER,
	GLSL_PROGRAM_TYPE_PLANAR_SHADOW,
	GLSL_PROGRAM_TYPE_CELLSHADE,

	GLSL_PROGRAM_TYPE_MAXTYPE
};

// features common for all program types
#define GLSL_COMMON_APPLY_CLIPPING				GLSL_BIT(0)
#define GLSL_COMMON_APPLY_GRAYSCALE				GLSL_BIT(1)
#define GLSL_COMMON_APPLY_FOG					GLSL_BIT(2)
#define GLSL_COMMON_APPLY_FOG2					GLSL_BIT(3)
#define GLSL_COMMON_APPLY_COLOR_FOG_ALPHA		GLSL_BIT(4)
#define GLSL_COMMON_APPLY_OVERBRIGHT_SCALING 	GLSL_BIT(5)

#define GLSL_COMMON_APPLY_TC_GEN_ENV			GLSL_BIT(6)
#define GLSL_COMMON_APPLY_TC_GEN_VECTOR			GLSL_BIT(7)
#define GLSL_COMMON_APPLY_TC_GEN_REFLECTION		(GLSL_COMMON_APPLY_TC_GEN_ENV | GLSL_COMMON_APPLY_TC_GEN_VECTOR)

#define GLSL_COMMON_APPLY_RGB_GEN_CONST 		GLSL_BIT(8)
#define GLSL_COMMON_APPLY_RGB_GEN_VERTEX 		GLSL_BIT(9)
#define GLSL_COMMON_APPLY_RGB_GEN_ONE_MINUS_VERTEX (GLSL_COMMON_APPLY_RGB_GEN_CONST | GLSL_COMMON_APPLY_RGB_GEN_VERTEX)

#define GLSL_COMMON_APPLY_ALPHA_GEN_CONST 		GLSL_BIT(10)
#define GLSL_COMMON_APPLY_ALPHA_GEN_VERTEX 		GLSL_BIT(11)
#define GLSL_COMMON_APPLY_ALPHA_GEN_ONE_MINUS_VERTEX (GLSL_COMMON_APPLY_ALPHA_GEN_CONST | GLSL_COMMON_APPLY_ALPHA_GEN_VERTEX)

#define GLSL_COMMON_APPLY_BONETRANSFORMS1		GLSL_BIT(12)
#define GLSL_COMMON_APPLY_BONETRANSFORMS2		GLSL_BIT(13)
#define GLSL_COMMON_APPLY_BONETRANSFORMS3		(GLSL_COMMON_APPLY_BONETRANSFORMS1 | GLSL_COMMON_APPLY_BONETRANSFORMS2)
#define GLSL_COMMON_APPLY_BONETRANSFORMS4		GLSL_BIT(14)
#define GLSL_COMMON_APPLY_BONETRANSFORMS		(GLSL_COMMON_APPLY_BONETRANSFORMS1 | GLSL_COMMON_APPLY_BONETRANSFORMS2 | GLSL_COMMON_APPLY_BONETRANSFORMS3 | GLSL_COMMON_APPLY_BONETRANSFORMS4)

#define GLSL_COMMON_APPLY_DLIGHTS_4				GLSL_BIT(15) // 4
#define GLSL_COMMON_APPLY_DLIGHTS_8				GLSL_BIT(16) // 8
#define GLSL_COMMON_APPLY_DLIGHTS_16			(GLSL_COMMON_APPLY_DLIGHTS_4 | GLSL_COMMON_APPLY_DLIGHTS_8) // 16
#define GLSL_COMMON_APPLY_DLIGHTS_32			GLSL_BIT(17) // 32

#define GLSL_COMMON_APPLY_DRAWFLAT				GLSL_BIT(18) // 32

// material prgoram type features
#define GLSL_MATERIAL_APPLY_LIGHTSTYLE0			GLSL_BIT(32)
#define GLSL_MATERIAL_APPLY_LIGHTSTYLE1			GLSL_BIT(33)
#define GLSL_MATERIAL_APPLY_LIGHTSTYLE2			(GLSL_MATERIAL_APPLY_LIGHTSTYLE0 | GLSL_MATERIAL_APPLY_LIGHTSTYLE1)
#define GLSL_MATERIAL_APPLY_LIGHTSTYLE3			GLSL_BIT(34)
#define GLSL_MATERIAL_APPLY_LIGHTSTYLE			((GLSL_MATERIAL_APPLY_LIGHTSTYLE0 | GLSL_MATERIAL_APPLY_LIGHTSTYLE1 | GLSL_MATERIAL_APPLY_LIGHTSTYLE2 | GLSL_MATERIAL_APPLY_LIGHTSTYLE3))
#define GLSL_MATERIAL_APPLY_SPECULAR			GLSL_BIT(35)
#define GLSL_MATERIAL_APPLY_DIRECTIONAL_LIGHT	GLSL_BIT(36)
#define GLSL_MATERIAL_APPLY_FB_LIGHTMAP			GLSL_BIT(37)
#define GLSL_MATERIAL_APPLY_OFFSETMAPPING		GLSL_BIT(38)
#define GLSL_MATERIAL_APPLY_RELIEFMAPPING		GLSL_BIT(39)
#define GLSL_MATERIAL_APPLY_AMBIENT_COMPENSATION GLSL_BIT(40)
#define GLSL_MATERIAL_APPLY_DECAL				GLSL_BIT(41)
#define GLSL_MATERIAL_APPLY_DECAL_ADD			GLSL_BIT(42)
#define GLSL_MATERIAL_APPLY_BASETEX_ALPHA_ONLY	GLSL_BIT(43)
#define GLSL_MATERIAL_APPLY_CELLSHADING			GLSL_BIT(44)
#define GLSL_MATERIAL_APPLY_HALFLAMBERT			GLSL_BIT(45)
#define GLSL_MATERIAL_APPLY_DIRECTIONAL_LIGHT_MIX GLSL_BIT(46)
#define GLSL_MATERIAL_APPLY_ENTITY_DECAL		GLSL_BIT(47)
#define GLSL_MATERIAL_APPLY_ENTITY_DECAL_ADD	GLSL_BIT(48)

// q3a shader features
#define GLSL_Q3A_APPLY_TC_GEN_FOG				GLSL_BIT(32)
#define GLSL_Q3A_APPLY_COLOR_FOG				GLSL_BIT(33)

// distortions
#define GLSL_DISTORTION_APPLY_DUDV				GLSL_BIT(32)
#define GLSL_DISTORTION_APPLY_EYEDOT			GLSL_BIT(33)
#define GLSL_DISTORTION_APPLY_DISTORTION_ALPHA	GLSL_BIT(34)
#define GLSL_DISTORTION_APPLY_REFLECTION		GLSL_BIT(35)
#define GLSL_DISTORTION_APPLY_REFRACTION		GLSL_BIT(36)

// shadowmaps
#define GLSL_SHADOWMAP_LIMIT					4 // shadowmaps per program limit
#define GLSL_SHADOWMAP_APPLY_DITHER				GLSL_BIT(32)
#define GLSL_SHADOWMAP_APPLY_PCF				GLSL_BIT(33)
#define GLSL_SHADOWMAP_APPLY_SHADOW2			GLSL_BIT(34)
#define GLSL_SHADOWMAP_APPLY_SHADOW3			GLSL_BIT(35)
#define GLSL_SHADOWMAP_APPLY_SHADOW4			GLSL_BIT(36)

// outlines
#define GLSL_OUTLINE_APPLY_OUTLINES_CUTOFF		GLSL_BIT(32)

// cellshading program features
#define GLSL_CELLSHADE_APPLY_DIFFUSE			GLSL_BIT(32)
#define GLSL_CELLSHADE_APPLY_DECAL				GLSL_BIT(33)
#define GLSL_CELLSHADE_APPLY_DECAL_ADD			GLSL_BIT(34)
#define GLSL_CELLSHADE_APPLY_ENTITY_DECAL		GLSL_BIT(35)
#define GLSL_CELLSHADE_APPLY_ENTITY_DECAL_ADD	GLSL_BIT(36)
#define GLSL_CELLSHADE_APPLY_STRIPES			GLSL_BIT(37)
#define GLSL_CELLSHADE_APPLY_STRIPES_ADD		GLSL_BIT(38)
#define GLSL_CELLSHADE_APPLY_CELL_LIGHT			GLSL_BIT(39)
#define GLSL_CELLSHADE_APPLY_CELL_LIGHT_ADD		GLSL_BIT(40)

void		R_InitGLSLPrograms( void );
int			R_FindGLSLProgram( const char *name );
int			R_RegisterGLSLProgram( int type, const char *name, const char *string, const char *deformsKey, const deformv_t *deforms, int numDeforms, r_glslfeat_t features );
int			R_GetProgramObject( int elem );
void		R_UpdateProgramUniforms( int elem, const vec3_t eyeOrigin, const vec3_t lightOrigin, const vec3_t lightDir,
									const vec4_t ambient, const vec4_t diffuse, const superLightStyle_t *superLightStyle,
									qboolean frontPlane, int TexWidth, int TexHeight,
									float projDistance, float offsetmappingScale, float glossExponent,
									const qbyte *constColor, int overbrightBits, float shaderTime, const qbyte *entityColor );
void		R_UpdateProgramFogParams( int elem, byte_vec4_t color, float clearDist, float opaqueDist, cplane_t *fogPlane, cplane_t *eyePlane, float eyeFogDist );
unsigned int R_UpdateProgramLightsParams( int elem, const vec3_t entOrigin, vec3_t entAxis[3], unsigned int dlightbits );
void 		R_UpdateProgramQ3AParams( int elem, float shaderTime, const vec3_t eyeOrigin, const vec3_t entDist, const qbyte *constColor, int overbrightBits );
void		R_UpdateProgramPlanarShadowParams( int elem, float shaderTime, const vec3_t planeNormal, float planeDist, const vec3_t lightDir );
void		R_UpdateProgramCellshadeParams( int elem, float shaderTime, const vec3_t eyeOrigin, const vec3_t entDist, const qbyte *constColor, int overbrightBits, const qbyte *entityColor, mat4x4_t reflectionMatrix );
void		R_UpdateProgramBonesParams( int elem, unsigned int numBones, dualquat_t *animDualQuat );
void		R_UpdateDrawFlatParams( int elem, const vec3_t wallColor, const vec3_t floorColor );
void		R_UpdateProgramShadowmapUniforms( int elem, vec3_t entityAxis[3], int numShadows, const shadowGroup_t **groups );
void		R_ShutdownGLSLPrograms( void );
void		R_ProgramList_f( void );
void		R_ProgramDump_f( void );

//
// r_poly.c
//
void		R_PushPoly( const meshbuffer_t *mb );
void		R_AddPolysToList( void );
void		R_DrawStretchPoly( const poly_t *poly, float x_offset, float y_offset );
qboolean	R_SurfPotentiallyFragmented( msurface_t *surf );
int			R_GetClippedFragments( const vec3_t origin, float radius, vec3_t axis[3], int maxfverts,
								  vec3_t *fverts, int maxfragments, fragment_t *fragments );
msurface_t *R_TransformedTraceLine( trace_t *tr, const vec3_t start, const vec3_t end, entity_t *test, int surfumask );

//
// r_register.c
//
rserr_t		R_Init( void *hinstance, void *wndproc, void *parenthWnd, int x, int y, int width, int height, qboolean fullscreen, qboolean wideScreen, qboolean verbose );
void		R_BeginRegistration( void );
void		R_EndRegistration( void );
void		R_Shutdown( qboolean verbose );


//
// r_surf.c
//
#define MAX_SURF_QUERIES		0x1E0

void		R_MarkLeaves( void );
void		R_DrawWorld( void );
qboolean	R_SurfPotentiallyVisible( msurface_t *surf );
qboolean	R_CullBrushModel( entity_t *e );
void		R_AddBrushModelToList( entity_t *e );
float		R_BrushModelBBox( entity_t *e, vec3_t mins, vec3_t maxs, qboolean *rotated );

void		R_ClearSurfOcclusionQueryKeys( void );
int			R_SurfOcclusionQueryKey( entity_t *e, msurface_t *surf );
void		R_SurfIssueOcclusionQueries( void );

//
// r_skin.c
//
void		R_InitSkinFiles( void );
void		R_ShutdownSkinFiles( void );
void		R_FreeUnusedSkinFiles( void );
struct skinfile_s *R_SkinFile_Load( const char *name );
struct skinfile_s *R_RegisterSkinFile( const char *name );
shader_t	*R_FindShaderForSkinFile( const struct skinfile_s *skinfile, const char *meshname );

//
// r_skm.c
//
qboolean	R_CullSkeletalModel( entity_t *e );
void		R_AddSkeletalModelToList( entity_t *e );
void		R_DrawSkeletalModel( const meshbuffer_t *mb );
float		R_SkeletalModelBBox( entity_t *e, vec3_t mins, vec3_t maxs );
void		R_SkeletalModelFrameBounds( const model_t *mod, int frame, vec3_t mins, vec3_t maxs );
int			R_SkeletalGetBoneInfo( const model_t *mod, int bonenum, char *name, size_t name_size, int *flags );
void		R_SkeletalGetBonePose( const model_t *mod, int bonenum, int frame, bonepose_t *bonepose );

void		R_InitSkeletalCache( void );
void		R_ClearSkeletalCache( void );
void		R_ShutdownSkeletalCache( void );

//
// r_vbo.c
//
void 		R_InitVBO( void );
mesh_vbo_t *R_CreateStaticMeshVBO( void *owner, int numVerts, int numElems, int features, vbo_tag_t tag );
void		R_ReleaseMeshVBO( mesh_vbo_t *vbo );
void		R_TouchMeshVBO( mesh_vbo_t *vbo );
mesh_vbo_t *R_VBOByIndex( int index );
int			R_GetNumberOfActiveVBOs( void );
int 		R_UploadVBOVertexData( mesh_vbo_t *vbo, int vertsOffset, mesh_t *mesh );
void 		R_UploadVBOElemData( mesh_vbo_t *vbo, int vertsOffset, int elemsOffset, mesh_t *mesh );
void		R_UploadVBOElemTrifanData( mesh_vbo_t *vbo, int vertsOffset, int elemsOffset, int numVerts );
int			R_UploadVBOBonesData( mesh_vbo_t *vbo, int vertsOffset, int numVerts, qbyte *bonesIndices, qbyte *bonesWeights );
void		R_FreeVBOsByTag( vbo_tag_t tag );
void		R_FreeUnusedVBOs( void );
void 		R_ShutdownVBO( void );

//
// r_warp.c
//
skydome_t	*R_CreateSkydome( float skyheight, shader_t **farboxShaders, shader_t **nearboxShaders );
void		R_FreeSkydome( skydome_t *skydome );
void		R_TouchSkydome( skydome_t *skydome );
void		R_ClearSky( void );
qboolean	R_DrawSky( shader_t *shader );
qboolean	R_AddSkySurface( msurface_t *fa );

//====================================================================

enum
{
	GLSTATE_NONE = 0,

	//
	// glBlendFunc args
	//
	GLSTATE_SRCBLEND_ZERO					= 1,
	GLSTATE_SRCBLEND_ONE					= 2,
	GLSTATE_SRCBLEND_DST_COLOR				= 1|2,
	GLSTATE_SRCBLEND_ONE_MINUS_DST_COLOR	= 4,
	GLSTATE_SRCBLEND_SRC_ALPHA				= 1|4,
	GLSTATE_SRCBLEND_ONE_MINUS_SRC_ALPHA	= 2|4,
	GLSTATE_SRCBLEND_DST_ALPHA				= 1|2|4,
	GLSTATE_SRCBLEND_ONE_MINUS_DST_ALPHA	= 8,

	GLSTATE_DSTBLEND_ZERO					= 16,
	GLSTATE_DSTBLEND_ONE					= 32,
	GLSTATE_DSTBLEND_SRC_COLOR				= 16|32,
	GLSTATE_DSTBLEND_ONE_MINUS_SRC_COLOR	= 64,
	GLSTATE_DSTBLEND_SRC_ALPHA				= 16|64,
	GLSTATE_DSTBLEND_ONE_MINUS_SRC_ALPHA	= 32|64,
	GLSTATE_DSTBLEND_DST_ALPHA				= 16|32|64,
	GLSTATE_DSTBLEND_ONE_MINUS_DST_ALPHA	= 128,

	GLSTATE_BLEND_MTEX						= 0x100,

	GLSTATE_AFUNC_GT0						= 0x200,
	GLSTATE_AFUNC_LT128						= 0x400,
	GLSTATE_AFUNC_GE128						= 0x200|0x400,

	GLSTATE_NOCOLORWRITE					= 0x800,

	GLSTATE_DEPTHWRITE						= 0x1000,
	GLSTATE_DEPTHFUNC_EQ					= 0x2000,

	GLSTATE_OFFSET_FILL						= 0x4000,
	GLSTATE_NO_DEPTH_TEST					= 0x8000,

	GLSTATE_STENCIL_TEST					= 0x10000,

	GLSTATE_MARK_END						= 0x20000 // SHADERPASS_MARK_BEGIN
};

#define GLSTATE_MASK		( GLSTATE_MARK_END-1 )

// #define SHADERPASS_SRCBLEND_MASK (((GLSTATE_SRCBLEND_DST_ALPHA)<<1)-GLSTATE_SRCBLEND_ZERO)
#define GLSTATE_SRCBLEND_MASK	0xF

// #define SHADERPASS_DSTBLEND_MASK (((GLSTATE_DSTBLEND_DST_ALPHA)<<1)-GLSTATE_DSTBLEND_ZERO)
#define GLSTATE_DSTBLEND_MASK	0xF0

#define GLSTATE_ALPHAFUNC		( GLSTATE_AFUNC_GT0|GLSTATE_AFUNC_LT128|GLSTATE_AFUNC_GE128 )

typedef struct
{
	int				overbrightBits;			// map specific overbright bits
	int				pow2MapOvrbr;
	float			mapLightColorScale;		// 1<<overbrightbits * intensity

	float			ambient[3];
#ifdef HARDWARE_OUTLINES
	byte_vec4_t		outlineColor;
#endif
	byte_vec4_t		environmentColor;

	float			lightingIntensity;

	qboolean		lightmapsPacking;
	qboolean		deluxeMaps;				// true if there are valid deluxemaps in the .bsp
	qboolean		deluxeMappingEnabled;	// true if deluxeMaps is true and r_lighting_deluxemaps->integer != 0

	qboolean		depthWritingSky;		// draw invisible sky surfaces with writing to depth buffer enabled
	qboolean		checkWaterCrossing;		// check above and below so crossing solid water doesn't draw wrong
	qboolean		polygonOffsetSubmodels;	// submodels need to be pushed further into z-buffer to prevent z-fights

	qboolean		forceClear;
} mapconfig_t;

extern mapconfig_t	mapConfig;
extern refinst_t	ri, prevRI;

#endif /*__R_LOCAL_H__*/
