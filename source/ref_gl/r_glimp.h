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
#ifndef __R_GLIMP_H__
#define __R_GLIMP_H__

#ifdef __cplusplus
#define QGL_EXTERN extern "C"
#else
#define QGL_EXTERN extern
#endif

#ifdef _WIN32
#include <windows.h>

#define QGL_WGL( type, name, params ) QGL_EXTERN type( APIENTRY * q ## name ) params;
#define QGL_WGL_EXT( type, name, params ) QGL_EXTERN type( APIENTRY * q ## name ) params;
#define QGL_GLX( type, name, params )
#define QGL_GLX_EXT( type, name, params )
#endif

#if defined ( __linux__ ) || defined ( __FreeBSD__ )
#define QGL_WGL( type, name, params )
#define QGL_WGL_EXT( type, name, params )
#define QGL_GLX( type, name, params ) QGL_EXTERN type( APIENTRY * q ## name ) params;
#define QGL_GLX_EXT( type, name, params ) QGL_EXTERN type( APIENTRY * q ## name ) params;
#endif

#if defined ( __MACOSX__ )
#define QGL_WGL( type, name, params )
#define QGL_WGL_EXT( type, name, params )
#define QGL_GLX( type, name, params )
#define QGL_GLX_EXT( type, name, params )
#endif

#define QGL_FUNC( type, name, params ) QGL_EXTERN type( APIENTRY * q ## name ) params;
#define QGL_EXT( type, name, params ) QGL_EXTERN type( APIENTRY * q ## name ) params;

#include "qgl.h"

#undef QGL_GLX_EXT
#undef QGL_GLX
#undef QGL_WGL_EXT
#undef QGL_WGL
#undef QGL_EXT
#undef QGL_FUNC

//====================================================================

extern cvar_t *r_colorbits;
extern cvar_t *r_stencilbits;
extern cvar_t *gl_drawbuffer;
extern cvar_t *gl_driver;

//====================================================================

typedef enum glsl_attribute_e
{
	GLSL_ATTRIB_POSITION	= 0,
	GLSL_ATTRIB_BONESINDICES= 1,
	GLSL_ATTRIB_NORMAL		= 2,
	GLSL_ATTRIB_COLOR		= 3,
//	GLSL_ATTRIB_UNUSED0		= 4,
//	GLSL_ATTRIB_UNUSED1		= 5,
//	GLSL_ATTRIB_UNUSED2		= 6,
	GLSL_ATTRIB_BONESWEIGHTS= 7,
	GLSL_ATTRIB_TEXCOORD0	= 8,
	GLSL_ATTRIB_TEXCOORD1	= 9,
	GLSL_ATTRIB_TEXCOORD2	= 10,
	GLSL_ATTRIB_TEXCOORD3	= 11,
	GLSL_ATTRIB_TEXCOORD4	= 12,
	GLSL_ATTRIB_TEXCOORD5	= 13,
	GLSL_ATTRIB_TEXCOORD6	= 14,
	GLSL_ATTRIB_TEXCOORD7	= 15
} glsl_attribute_t;

typedef struct
{
	int			_extMarker;

	//
	// only qbytes must follow the extensionsBoolMarker
	//

	char		compiled_vertex_array
				,draw_range_elements
				,multitexture
				,texture_cube_map
				,texture_env_add
				,texture_env_combine
				,texture_edge_clamp
				,texture_filter_anisotropic
				,texture3D
				,texture_non_power_of_two
				,texture_compression
				,vertex_buffer_object
				,GLSL
				,depth_texture
				,shadow
				,occlusion_query
				,framebuffer_object
				,vertex_shader
				,fragment_shader
				,shader_objects
				,shading_language_100
				,bgra
				,gamma_control
				,swap_control
				,draw_instanced
				,gpu_memory_info
				,meminfo
				;
} glextinfo_t;

typedef struct
{
	const char		*rendererString;
	const char		*vendorString;
	const char		*versionString;
	const char		*extensionsString;
	const char		*glwExtensionsString;
	const char		*shadingLanguageVersionString;

	int				shadingLanguageVersion100;

	int				maxTextureSize
					,maxTextureUnits
					,maxTextureCubemapSize
					,maxTextureSize3D
					,maxTextureFilterAnisotropic
					,maxVaryingFloats
					,maxVertexUniformComponents
					,maxFragmentUniformComponents;
	unsigned int	maxGLSLBones;	// the maximum amount of bones we can handle in a vertex shader
	glextinfo_t ext;
} glconfig_t;

typedef struct
{
	int				flags;

	int				width, height;
	qboolean		fullScreen;
	qboolean		wideScreen;

	qboolean		warmupRenderer;
	qboolean		initializedMedia;

	int				currentTMU;
	GLuint			*currentTextures;
	int				*currentEnvModes;
	qboolean		*texIdentityMatrix;
	int				*genSTEnabled;			// 0 - disabled, OR 1 - S, OR 2 - T, OR 4 - R
	int				*texCoordArrayMode;		// 0 - disabled, 1 - enabled, 2 - cubemap
	int 			currentArrayVBO;
	int 			currentElemArrayVBO;

	qbyte			texture2DEnabled;
	qbyte			texture3DEnabled;

	int				faceCull;
	int				frontFace;

	int				scissorX, scissorY;
	int				scissorW, scissorH;

	float			cameraSeparation;
	qboolean		stereoEnabled;
	qboolean		stencilEnabled;
	qboolean		in2DMode;

	float			polygonOffset[2];

	qboolean		hwGamma;
	unsigned short	orignalGammaRamp[3*256];

	unsigned int	vertexAttribEnabled;
} glstate_t;

extern glconfig_t	glConfig;
extern glstate_t	glState;

/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

void		GLimp_BeginFrame( void );
void		GLimp_EndFrame( void );
int			GLimp_Init( void *hinstance, void *wndproc, void *parenthWnd );
void	    GLimp_Shutdown( void );
rserr_t		GLimp_SetMode( int x, int y, int width, int height, qboolean fullscreen, qboolean wideScreen );
void	    GLimp_AppActivate( qboolean active, qboolean destroy );
qboolean	GLimp_GetGammaRamp( size_t stride, unsigned short *ramp );
void		GLimp_SetGammaRamp( size_t stride, unsigned short *ramp );

#endif /*__R_GLIMP_H__*/
