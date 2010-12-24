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

/*
** GL config stuff
*/
#define GL_RENDERER_VOODOO			0x00000001
#define GL_RENDERER_VOODOO2			0x00000002
#define GL_RENDERER_VOODOO_RUSH		0x00000004
#define GL_RENDERER_BANSHEE			0x00000008
#define	GL_RENDERER_3DFX			0x0000000F

#define GL_RENDERER_PCX1			0x00000010
#define GL_RENDERER_PCX2			0x00000020
#define GL_RENDERER_PMX				0x00000040
#define	GL_RENDERER_POWERVR			0x00000070

#define GL_RENDERER_PERMEDIA2		0x00000100
#define GL_RENDERER_GLINT_MX		0x00000200
#define GL_RENDERER_GLINT_TX		0x00000400
#define GL_RENDERER_3DLABS_MISC		0x00000800
#define	GL_RENDERER_3DLABS			0x00000F00

#define GL_RENDERER_REALIZM			0x00001000
#define GL_RENDERER_REALIZM2		0x00002000
#define	GL_RENDERER_INTERGRAPH		0x00003000

#define GL_RENDERER_3DPRO			0x00004000
#define GL_RENDERER_REAL3D			0x00008000
#define GL_RENDERER_RIVA128			0x00010000
#define GL_RENDERER_DYPIC			0x00020000

#define GL_RENDERER_V1000			0x00040000
#define GL_RENDERER_V2100			0x00080000
#define GL_RENDERER_V2200			0x00100000
#define	GL_RENDERER_RENDITION		0x001C0000

#define GL_RENDERER_O2				0x00100000
#define GL_RENDERER_IMPACT			0x00200000
#define GL_RENDERER_RE				0x00400000
#define GL_RENDERER_IR				0x00800000
#define	GL_RENDERER_SGI				0x00F00000

#define GL_RENDERER_MCD				0x01000000
#define GL_RENDERER_OTHER			0x80000000

enum
{
	rserr_ok,

	rserr_invalid_fullscreen,
	rserr_invalid_mode,

	rserr_unknown
} rserr_t;

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
				,generate_mipmap
				,vertex_shader
				,fragment_shader
				,shader_objects
				,shading_language_100
				,bgra
				,gamma_control
				,swap_control;
} glextinfo_t;

typedef struct
{
	int				renderer;
	const char		*rendererString;
	const char		*vendorString;
	const char		*versionString;
	const char		*extensionsString;
	const char		*glwExtensionsString;

	qboolean		allowCDS;

	int				maxTextureSize
					,maxTextureUnits
					,maxTextureCubemapSize
					,maxTextureSize3D
					,maxTextureFilterAnisotropic
					,maxVaryingFloats;

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

	int				previousMode;

	int				currentTMU;
	GLuint			*currentTextures;
	int				*currentEnvModes;
	qboolean		*texIdentityMatrix;
	int				*genSTEnabled;			// 0 - disabled, OR 1 - S, OR 2 - T, OR 4 - R
	int				*texCoordArrayMode;		// 0 - disabled, 1 - enabled, 2 - cubemap
	int 			currentArrayVBO;
	int 			currentElemArrayVBO;

	int				faceCull;
	int				frontFace;

	float			cameraSeparation;
	qboolean		stereoEnabled;
	qboolean		stencilEnabled;
	qboolean		in2DMode;

	float			polygonOffset[2];

	qboolean		hwGamma;
	unsigned short	orignalGammaRamp[3*256];
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
int			GLimp_SetMode( int mode, qboolean fullscreen );
int			GLimp_GetCurrentMode( void );
void	    GLimp_AppActivate( qboolean active, qboolean destroy );
qboolean	GLimp_GetGammaRamp( size_t stride, unsigned short *ramp );
void		GLimp_SetGammaRamp( size_t stride, unsigned short *ramp );

void		VID_NewWindow( int width, int height );
qboolean	VID_GetModeInfo( int *width, int *height, qboolean *wideScreen, int mode );
int			VID_GetModeNum( int width, int height );

#endif /*__R_GLIMP_H__*/
