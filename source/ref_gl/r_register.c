/*
Copyright (C) 2007 Victor Luchits

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

// r_register.c
#include "r_local.h"

glconfig_t	glConfig;
glstate_t	glState;

cvar_t *r_norefresh;
cvar_t *r_drawentities;
cvar_t *r_drawworld;
cvar_t *r_speeds;
cvar_t *r_drawelements;
cvar_t *r_fullbright;
cvar_t *r_lightmap;
cvar_t *r_novis;
cvar_t *r_nocull;
cvar_t *r_lerpmodels;
cvar_t *r_ignorehwgamma;
cvar_t *r_overbrightbits;
cvar_t *r_mapoverbrightbits;
cvar_t *r_flares;
cvar_t *r_flaresize;
cvar_t *r_flarefade;
cvar_t *r_dynamiclight;
cvar_t *r_coronascale;
cvar_t *r_detailtextures;
cvar_t *r_subdivisions;
cvar_t *r_faceplanecull;
cvar_t *r_showtris;
cvar_t *r_shownormals;
cvar_t *r_draworder;

cvar_t *r_fastsky;
cvar_t *r_portalonly;
cvar_t *r_portalmaps;
cvar_t *r_portalmaps_maxtexsize;

cvar_t *r_lighting_bumpscale;
cvar_t *r_lighting_deluxemapping;
cvar_t *r_lighting_useblankbumpmap;
cvar_t *r_lighting_diffuse2heightmap;
cvar_t *r_lighting_specular;
cvar_t *r_lighting_glossintensity;
cvar_t *r_lighting_glossexponent;
cvar_t *r_lighting_models_followdeluxe;
cvar_t *r_lighting_ambientscale;
cvar_t *r_lighting_directedscale;
cvar_t *r_lighting_packlightmaps;
cvar_t *r_lighting_maxlmblocksize;
cvar_t *r_lighting_vertexlight;
cvar_t *r_lighting_maxglsldlights;
cvar_t *r_lighting_grayscale;

cvar_t *r_offsetmapping;
cvar_t *r_offsetmapping_scale;
cvar_t *r_offsetmapping_reliefmapping;

cvar_t *r_occlusion_queries;
cvar_t *r_occlusion_queries_finish;

cvar_t *r_shadows;
cvar_t *r_shadows_alpha;
cvar_t *r_shadows_nudge;
cvar_t *r_shadows_projection_distance;
cvar_t *r_shadows_maxtexsize;
cvar_t *r_shadows_pcf;
cvar_t *r_shadows_self_shadow;
cvar_t *r_shadows_dither;

cvar_t *r_bloom;
cvar_t *r_bloom_alpha;
cvar_t *r_bloom_diamond_size;
cvar_t *r_bloom_intensity;
cvar_t *r_bloom_darken;
cvar_t *r_bloom_sample_size;
cvar_t *r_bloom_fast_sample;

cvar_t *r_outlines_world;
cvar_t *r_outlines_scale;
cvar_t *r_outlines_cutoff;

cvar_t *r_allow_software;

cvar_t *r_lodbias;
cvar_t *r_lodscale;

cvar_t *r_stencilbits;
cvar_t *r_gamma;
cvar_t *r_texturebits;
cvar_t *r_texturemode;
cvar_t *r_texturefilter;
cvar_t *r_picmip;
cvar_t *r_skymip;
cvar_t *r_nobind;
cvar_t *r_clear;
cvar_t *r_polyblend;
cvar_t *r_lockpvs;
cvar_t *r_screenshot_fmtstr;
cvar_t *r_screenshot_jpeg;
cvar_t *r_screenshot_jpeg_quality;
cvar_t *r_swapinterval;

cvar_t *r_temp1;

cvar_t *r_drawflat;
cvar_t *r_wallcolor;
cvar_t *r_floorcolor;

cvar_t *r_maxglslbones;

cvar_t *gl_extensions;
cvar_t *gl_drawbuffer;
cvar_t *gl_driver;
cvar_t *gl_finish;
cvar_t *gl_cull;

static qboolean	r_verbose;

static void R_FinalizeGLExtensions( void );
static void R_GfxInfo_f( void );

static void R_InitVolatileAssets( void );
static void R_DestroyVolatileAssets( void );

//=======================================================================

#define	GLINF_FOFS(x) (size_t)&(((glextinfo_t *)0)->x)
#define	GLINF_EXMRK() GLINF_FOFS(_extMarker)
#define	GLINF_FROM(from,ofs) (*((char *)from + ofs))

typedef struct
{
	const char * const name;				// constant pointer to constant string
	void ** const pointer;					// constant pointer to function's pointer (function itself)
} gl_extension_func_t;

typedef struct
{
	const char * const prefix;				// constant pointer to constant string
	const char * const name;
	const char * const cvar_default;
	const qboolean cvar_readonly;
	gl_extension_func_t * const funcs;		// constant pointer to array of functions
	const size_t offset;					// offset to respective variable
	const size_t depOffset;					// offset to required pre-initialized variable
} gl_extension_t;

#define GL_EXTENSION_FUNC_EXT(name,func) { name, (void ** const)func }
#define GL_EXTENSION_FUNC(name) GL_EXTENSION_FUNC_EXT("gl"#name,&(qgl##name))

/* GL_ARB_multitexture */
static const gl_extension_func_t gl_ext_multitexture_ARB_funcs[] =
{
	 GL_EXTENSION_FUNC(ActiveTextureARB)
	,GL_EXTENSION_FUNC(ClientActiveTextureARB)

	,GL_EXTENSION_FUNC_EXT(NULL,NULL)
};

/* GL_EXT_compiled_vertex_array */
static const gl_extension_func_t gl_ext_compiled_vertex_array_EXT_funcs[] =
{
	 GL_EXTENSION_FUNC(LockArraysEXT)
	,GL_EXTENSION_FUNC(UnlockArraysEXT)

	,GL_EXTENSION_FUNC_EXT(NULL,NULL)
};

/* GL_ARB_vertex_buffer_object */
static const gl_extension_func_t gl_ext_vertex_buffer_object_ARB_funcs[] =
{
	 GL_EXTENSION_FUNC(BindBufferARB)
	,GL_EXTENSION_FUNC(DeleteBuffersARB)
	,GL_EXTENSION_FUNC(GenBuffersARB)
	,GL_EXTENSION_FUNC(BufferDataARB)
	,GL_EXTENSION_FUNC(BufferSubDataARB)

	,GL_EXTENSION_FUNC_EXT(NULL,NULL)
};

/* GL_EXT_texture3D */
static const gl_extension_func_t gl_ext_texture3D_EXT_funcs[] =
{
	 GL_EXTENSION_FUNC(TexImage3D)
	,GL_EXTENSION_FUNC(TexSubImage3D)

	,GL_EXTENSION_FUNC_EXT(NULL,NULL)
};

/* GL_EXT_draw_range_elements */
static const gl_extension_func_t gl_ext_draw_range_elements_EXT_funcs[] =
{
	 GL_EXTENSION_FUNC(DrawRangeElementsEXT)
	,GL_EXTENSION_FUNC_EXT("glDrawRangeElements",&qglDrawRangeElementsEXT)

	,GL_EXTENSION_FUNC_EXT(NULL,NULL)
};

/* GL_ARB_GLSL (meta extension) */
static const gl_extension_func_t gl_ext_GLSL_ARB_funcs[] =
{
	 GL_EXTENSION_FUNC(DeleteObjectARB)
	,GL_EXTENSION_FUNC(GetHandleARB)
	,GL_EXTENSION_FUNC(DetachObjectARB)
	,GL_EXTENSION_FUNC(CreateShaderObjectARB)
	,GL_EXTENSION_FUNC(ShaderSourceARB)
	,GL_EXTENSION_FUNC(CompileShaderARB)
	,GL_EXTENSION_FUNC(CreateProgramObjectARB)
	,GL_EXTENSION_FUNC(AttachObjectARB)
	,GL_EXTENSION_FUNC(LinkProgramARB)
	,GL_EXTENSION_FUNC(UseProgramObjectARB)
	,GL_EXTENSION_FUNC(ValidateProgramARB)
	,GL_EXTENSION_FUNC(Uniform1fARB)
	,GL_EXTENSION_FUNC(Uniform2fARB)
	,GL_EXTENSION_FUNC(Uniform3fARB)
	,GL_EXTENSION_FUNC(Uniform4fARB)
	,GL_EXTENSION_FUNC(Uniform1iARB)
	,GL_EXTENSION_FUNC(Uniform2iARB)
	,GL_EXTENSION_FUNC(Uniform3iARB)
	,GL_EXTENSION_FUNC(Uniform4iARB)
	,GL_EXTENSION_FUNC(Uniform1fvARB)
	,GL_EXTENSION_FUNC(Uniform2fvARB)
	,GL_EXTENSION_FUNC(Uniform3fvARB)
	,GL_EXTENSION_FUNC(Uniform4fvARB)
	,GL_EXTENSION_FUNC(Uniform1ivARB)
	,GL_EXTENSION_FUNC(Uniform2ivARB)
	,GL_EXTENSION_FUNC(Uniform3ivARB)
	,GL_EXTENSION_FUNC(Uniform4ivARB)
	,GL_EXTENSION_FUNC(UniformMatrix2fvARB)
	,GL_EXTENSION_FUNC(UniformMatrix3fvARB)
	,GL_EXTENSION_FUNC(UniformMatrix4fvARB)
	,GL_EXTENSION_FUNC(GetObjectParameterfvARB)
	,GL_EXTENSION_FUNC(GetObjectParameterivARB)
	,GL_EXTENSION_FUNC(GetInfoLogARB)
	,GL_EXTENSION_FUNC(GetAttachedObjectsARB)
	,GL_EXTENSION_FUNC(GetUniformLocationARB)
	,GL_EXTENSION_FUNC(GetActiveUniformARB)
	,GL_EXTENSION_FUNC(GetUniformfvARB)
	,GL_EXTENSION_FUNC(GetUniformivARB)
	,GL_EXTENSION_FUNC(GetShaderSourceARB)

	,GL_EXTENSION_FUNC(VertexAttribPointerARB)
	,GL_EXTENSION_FUNC(EnableVertexAttribArrayARB)
	,GL_EXTENSION_FUNC(DisableVertexAttribArrayARB)
	,GL_EXTENSION_FUNC(BindAttribLocationARB)
	,GL_EXTENSION_FUNC(GetActiveAttribARB)
	,GL_EXTENSION_FUNC(GetAttribLocationARB)

	,GL_EXTENSION_FUNC_EXT(NULL,NULL)
};

/* GL_ARB_occlusion_query */
static const gl_extension_func_t gl_ext_occlusion_query_ARB_funcs[] =
{
	 GL_EXTENSION_FUNC(GenQueriesARB)
	,GL_EXTENSION_FUNC(DeleteQueriesARB)
	,GL_EXTENSION_FUNC(IsQueryARB)
	,GL_EXTENSION_FUNC(BeginQueryARB)
	,GL_EXTENSION_FUNC(EndQueryARB)
	,GL_EXTENSION_FUNC(GetQueryivARB)
	,GL_EXTENSION_FUNC(GetQueryObjectivARB)
	,GL_EXTENSION_FUNC(GetQueryObjectuivARB)

	,GL_EXTENSION_FUNC_EXT(NULL,NULL)
};

/* GL_ARB_draw_instanced */
static const gl_extension_func_t gl_ext_draw_instanced_ARB_funcs[] =
{
	 GL_EXTENSION_FUNC(DrawArraysInstancedARB)
	,GL_EXTENSION_FUNC(DrawElementsInstancedARB)

	,GL_EXTENSION_FUNC_EXT(NULL,NULL)
};

/* GL_EXT_framebuffer_object */
static const gl_extension_func_t gl_ext_framebuffer_object_EXT_funcs[] =
{
	 GL_EXTENSION_FUNC(IsRenderbufferEXT)
 	,GL_EXTENSION_FUNC(BindRenderbufferEXT)
 	,GL_EXTENSION_FUNC(DeleteRenderbuffersEXT)
 	,GL_EXTENSION_FUNC(GenRenderbuffersEXT)
 	,GL_EXTENSION_FUNC(RenderbufferStorageEXT)
 	,GL_EXTENSION_FUNC(GetRenderbufferParameterivEXT)
 	,GL_EXTENSION_FUNC(IsFramebufferEXT)
 	,GL_EXTENSION_FUNC(BindFramebufferEXT)
 	,GL_EXTENSION_FUNC(DeleteFramebuffersEXT)
 	,GL_EXTENSION_FUNC(GenFramebuffersEXT)
	,GL_EXTENSION_FUNC(CheckFramebufferStatusEXT)
	,GL_EXTENSION_FUNC(FramebufferTexture1DEXT)
	,GL_EXTENSION_FUNC(FramebufferTexture2DEXT)
	,GL_EXTENSION_FUNC(FramebufferTexture3DEXT)
	,GL_EXTENSION_FUNC(FramebufferRenderbufferEXT)
	,GL_EXTENSION_FUNC(GetFramebufferAttachmentParameterivEXT)
	,GL_EXTENSION_FUNC(GenerateMipmapEXT)

	,GL_EXTENSION_FUNC_EXT(NULL,NULL)
};

#ifdef _WIN32

/* WGL_EXT_swap_interval */
static const gl_extension_func_t wgl_ext_swap_interval_EXT_funcs[] =
{
	 GL_EXTENSION_FUNC_EXT("wglSwapIntervalEXT",&qglSwapInterval)

	,GL_EXTENSION_FUNC_EXT(NULL,NULL)
};

#endif

#ifdef GLX_VERSION

/* GLX_SGI_occlusion_query */
static const gl_extension_func_t glx_ext_swap_control_SGI_funcs[] =
{
	 GL_EXTENSION_FUNC_EXT("glXSwapIntervalSGI",&qglSwapInterval)

	,GL_EXTENSION_FUNC_EXT(NULL,NULL)
};

#endif

//
// legacy crap
//

/* GL_SGIS_multitexture */
static const gl_extension_func_t gl_ext_multitexture_SGIS_funcs[] =
{
	 GL_EXTENSION_FUNC(SelectTextureSGIS)

	,GL_EXTENSION_FUNC_EXT(NULL,NULL)
};

#ifdef _WIN32

/* WGL_3DFX_gamma_control */
static const gl_extension_func_t wgl_ext_gamma_control_3DFX_funcs[] =
{
	 GL_EXTENSION_FUNC_EXT("wglGetDeviceGammaRamp3DFX",&qwglGetDeviceGammaRamp3DFX)
	,GL_EXTENSION_FUNC_EXT("wglSetDeviceGammaRamp3DFX",&qwglSetDeviceGammaRamp3DFX)

	,GL_EXTENSION_FUNC_EXT(NULL,NULL)
};

#endif

#undef GL_EXTENSION_FUNC
#undef GL_EXTENSION_FUNC_EXT

//=======================================================================

#define GL_EXTENSION_EXT(pre,name,val,ro,funcs,dep) { #pre, #name, #val, q##ro, (gl_extension_func_t * const)funcs, GLINF_FOFS(name), GLINF_FOFS(dep) }
#define GL_EXTENSION(pre,name,ro,funcs) GL_EXTENSION_EXT(pre,name,1,ro,funcs,_extMarker)

//
// OpenGL extensions list
//
// short notation: vendor, name, default value, list of functions
// extended notation: vendor, name, default value, list of functions, required extension
static const gl_extension_t gl_extensions_decl[] =
{
	 GL_EXTENSION( ARB, multitexture, true, &gl_ext_multitexture_ARB_funcs )
	,GL_EXTENSION( SGIS, multitexture, true, &gl_ext_multitexture_SGIS_funcs )
	,GL_EXTENSION( EXT, compiled_vertex_array, true, &gl_ext_compiled_vertex_array_EXT_funcs )
	,GL_EXTENSION( SGI, compiled_vertex_array, true, &gl_ext_compiled_vertex_array_EXT_funcs )
	,GL_EXTENSION( ARB, vertex_buffer_object, true, &gl_ext_vertex_buffer_object_ARB_funcs )
	,GL_EXTENSION( EXT, texture3D, false, &gl_ext_texture3D_EXT_funcs )
	,GL_EXTENSION( EXT, draw_range_elements, true, &gl_ext_draw_range_elements_EXT_funcs )
	,GL_EXTENSION( ARB, occlusion_query, false, &gl_ext_occlusion_query_ARB_funcs )
	,GL_EXTENSION( EXT, framebuffer_object, true, &gl_ext_framebuffer_object_EXT_funcs )
	,GL_EXTENSION( ARB, texture_env_add, true, NULL )
	,GL_EXTENSION( ARB, texture_env_combine, true, NULL )
	,GL_EXTENSION( EXT, texture_env_combine, true, NULL )
	,GL_EXTENSION_EXT( ARB, texture_compression, 0, false, NULL, _extMarker )
	,GL_EXTENSION( EXT, texture_edge_clamp, true, NULL )
	,GL_EXTENSION( SGIS, texture_edge_clamp, true, NULL )
	,GL_EXTENSION( EXT, texture_filter_anisotropic, true, NULL )
	,GL_EXTENSION( ARB, texture_cube_map, false, NULL )
	,GL_EXTENSION( EXT, bgra, true, NULL )
	,GL_EXTENSION( ARB, depth_texture, false, NULL )
	,GL_EXTENSION( ARB, shadow, false, NULL )
	,GL_EXTENSION( ARB, texture_non_power_of_two, false, NULL )
	//,GL_EXTENSION( ARB, draw_instanced, true, &gl_ext_draw_instanced_ARB_funcs )

	// extensions required by meta-extension gl_ext_GLSL
	,GL_EXTENSION_EXT( ARB, vertex_shader, 1, true, NULL, multitexture )
	,GL_EXTENSION_EXT( ARB, fragment_shader, 1, true, NULL, vertex_shader )
	,GL_EXTENSION_EXT( ARB, shader_objects, 1, true, NULL, fragment_shader )
	,GL_EXTENSION_EXT( ARB, shading_language_100, 1, true, NULL, shader_objects )

	// meta GLSL extensions
	,GL_EXTENSION_EXT( \0, GLSL, 1, false, &gl_ext_GLSL_ARB_funcs, shading_language_100 )

	// memory info
	,GL_EXTENSION( NVX, gpu_memory_info, true, NULL )
	,GL_EXTENSION( ATI, meminfo, true, NULL )

#ifdef GLX_VERSION
	,GL_EXTENSION( GLX_SGI, swap_control, true, &glx_ext_swap_control_SGI_funcs )
#endif

#ifdef _WIN32
	,GL_EXTENSION( WGL_3DFX, gamma_control, true, &wgl_ext_gamma_control_3DFX_funcs )
	,GL_EXTENSION( WGL_EXT, swap_control, true, &wgl_ext_swap_interval_EXT_funcs )
#endif
};

static const int num_gl_extensions = sizeof( gl_extensions_decl ) / sizeof( gl_extensions_decl[0] );

#undef GL_EXTENSION
#undef GL_EXTENSION_EXT

/*
* R_RegisterGLExtensions
*/
void R_RegisterGLExtensions( void )
{
	int i;
	char *var, name[128];
	cvar_t *cvar;
	cvar_flag_t cvar_flags;
	gl_extension_func_t *func;
	const gl_extension_t *extension;

	memset( &glConfig.ext, 0, sizeof( glextinfo_t ) );

	// only initialize some common variables
	if( !gl_extensions->integer )
	{
		R_FinalizeGLExtensions ();
		return;
	}

	for( i = 0, extension = gl_extensions_decl; i < num_gl_extensions; i++, extension++ )
	{
		Q_snprintfz( name, sizeof( name ), "gl_ext_%s", extension->name );

		// register a cvar and check if this extension is explicitly disabled
		cvar_flags = CVAR_ARCHIVE|CVAR_LATCH_VIDEO;
#ifdef PUBLIC_BUILD
		if( extension->cvar_readonly ) {
			cvar_flags |= CVAR_READONLY;
		}
#endif

		cvar = Cvar_Get( name, extension->cvar_default ? extension->cvar_default : "0", cvar_flags );
		if( !cvar->integer )
			continue;

		// an alternative extension of higher priority is available so ignore this one
		var = &(GLINF_FROM( &glConfig.ext, extension->offset ));
		if( *var )
			continue;

		// required extension is not available, ignore
		if( extension->depOffset != GLINF_EXMRK() && !GLINF_FROM( &glConfig.ext, extension->depOffset ) )
			continue;

		// let's see what the driver's got to say about this...
		if( *extension->prefix )
		{
			const char *extstring = ( !strncmp( extension->prefix, "WGL", 3 ) || !strncmp( extension->prefix, "GLX", 3 ) )
				? glConfig.glwExtensionsString : glConfig.extensionsString;

			Q_snprintfz( name, sizeof( name ), "%s_%s", extension->prefix, extension->name );
			if( !strstr( extstring, name ) )
				continue;
		}

		// initialize function pointers
		func = extension->funcs;
		if( func )
		{
			do {
				*(func->pointer) = ( void * )qglGetProcAddress( (const GLubyte *)func->name );
				if( !*(func->pointer) )
					break;
			} while( (++func)->name );

			// some function is missing
			if( func->name )
			{
				gl_extension_func_t *func2 = extension->funcs;

				// whine about buggy driver
				Com_Printf( "R_RegisterGLExtensions: broken %s support, contact your video card vendor\n", cvar->name );

				// reset previously initialized functions back to NULL
				do {
					*(func2->pointer) = NULL;
				} while( ++func2 != func );

				continue;
			}
		}

		// mark extension as available
		*var = qtrue;
	}

	R_FinalizeGLExtensions ();
}

/*
* R_PrintGLExtensionsInfo
*/
static void R_PrintGLExtensionsInfo( void )
{
	int i;
	size_t lastOffset;
	const gl_extension_t *extension;

	for( i = 0, lastOffset = 0, extension = gl_extensions_decl; i < num_gl_extensions; i++, extension++ )
	{
		if( lastOffset != extension->offset )
		{
			lastOffset = extension->offset;
			Com_Printf( "%s: %s\n", extension->name, GLINF_FROM( &glConfig.ext, lastOffset ) ? "enabled" : "disabled" );
		}
	}
}

/*
* R_PrintMemoryInfo
*/
static void R_PrintMemoryInfo( void )
{
	int mem[12];

	Com_Printf( "\n" );
	Com_Printf( "Video memory information:\n" );

	if( glConfig.ext.gpu_memory_info ) {
		// NV
		qglGetIntegerv( GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, mem );
		Com_Printf( "total: %i MB\n", mem[0] >>10 );

		qglGetIntegerv( GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, mem );
		Com_Printf( "dedicated: %i MB\n", mem[0] >>10 );

		qglGetIntegerv( GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, mem );
		Com_Printf( "available: %i MB\n", mem[0] >>10 );

		qglGetIntegerv( GPU_MEMORY_INFO_EVICTION_COUNT_NVX, mem );
		Com_Printf( "eviction count: %i MB\n", mem[0] >> 10 );

		qglGetIntegerv( GPU_MEMORY_INFO_EVICTED_MEMORY_NVX, mem );
		Com_Printf( "totally evicted: %i MB\n", mem[0] >>10 );
	}
	else if( glConfig.ext.meminfo ) {
		// ATI
		qglGetIntegerv( VBO_FREE_MEMORY_ATI, mem );
		qglGetIntegerv( TEXTURE_FREE_MEMORY_ATI, mem+4 );
		qglGetIntegerv( RENDERBUFFER_FREE_MEMORY_ATI, mem+8 );

		Com_Printf( "total memory free in the pool: (VBO:%i, Tex:%i, RBuf:%i) MB\n", mem[0] >> 10, mem[4] >> 10, mem[8] >> 10 );
		Com_Printf( "largest available free block in the pool: (V:%i, Tex:%i, RBuf:%i) MB\n", mem[5] >> 10, mem[4] >> 10, mem[9] >> 10 );
		Com_Printf( "total auxiliary memory free: (VBO:%i, Tex:%i, RBuf:%i) MB\n", mem[2] >> 10, mem[6] >> 10, mem[10] >> 10 );
		Com_Printf( "largest auxiliary free block: (VBO:%i, Tex:%i, RBuf:%i) MB\n", mem[3] >> 10, mem[7] >> 10, mem[11] >> 10 );
	}
	else {
		Com_Printf( "not available\n" );
	}
}

/*
* R_FinalizeGLExtensions
* 
* Verify correctness of values provided by the driver, init some variables
*/
static void R_FinalizeGLExtensions( void )
{
	cvar_t *cvar;

	glConfig.maxTextureSize = 0;
	qglGetIntegerv( GL_MAX_TEXTURE_SIZE, &glConfig.maxTextureSize );
	if( glConfig.maxTextureSize <= 0 )
		glConfig.maxTextureSize = 256;

	Cvar_Get( "gl_max_texture_size", "0", CVAR_READONLY );
	Cvar_ForceSet( "gl_max_texture_size", va( "%i", glConfig.maxTextureSize ) );

	/* GL_ARB_texture_cube_map */
	glConfig.maxTextureCubemapSize = 0;
	if( glConfig.ext.texture_cube_map )
		qglGetIntegerv( GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB, &glConfig.maxTextureCubemapSize );
	if( glConfig.maxTextureCubemapSize <= 1 )
		glConfig.ext.texture_cube_map = qfalse;

	/* GL_EXT_texture3D */
	glConfig.maxTextureSize3D = 0;
	if( glConfig.ext.texture3D )
		qglGetIntegerv( GL_MAX_3D_TEXTURE_SIZE, &glConfig.maxTextureSize3D );
	if( glConfig.maxTextureSize3D <= 1 )
		glConfig.ext.texture3D = qfalse;

	/* GL_ARB_multitexture */
	glConfig.maxTextureUnits = 1;
	if( glConfig.ext.multitexture ) {
		qglGetIntegerv( GL_MAX_TEXTURE_UNITS, &glConfig.maxTextureUnits );
		clamp( glConfig.maxTextureUnits, 1, MAX_TEXTURE_UNITS );
	}
	if( glConfig.maxTextureUnits == 1 )
		glConfig.ext.multitexture = qfalse;

	/* GL_EXT_texture_filter_anisotropic */
	glConfig.maxTextureFilterAnisotropic = 0;
	if( strstr( glConfig.extensionsString, "GL_EXT_texture_filter_anisotropic" ) )
		qglGetIntegerv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glConfig.maxTextureFilterAnisotropic );

	// GLSL version * 100
	glConfig.shadingLanguageVersion100 = (int)(atof(glConfig.shadingLanguageVersionString) * 100);

	glConfig.maxVaryingFloats = 0;
	glConfig.maxVertexUniformComponents = glConfig.maxFragmentUniformComponents = 0;
	if( glConfig.ext.GLSL ) {
		qglGetIntegerv( GL_MAX_VARYING_FLOATS_ARB, &glConfig.maxVaryingFloats );
		qglGetIntegerv( GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB, &glConfig.maxVertexUniformComponents );
		qglGetIntegerv( GL_MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB, &glConfig.maxFragmentUniformComponents );
	}

	// keep the maximum number of bones we can do in GLSL sane
	if( r_maxglslbones->integer > MAX_GLSL_BONES ) {
		Cvar_ForceSet( r_maxglslbones->name, r_maxglslbones->dvalue );
	}

	// the maximum amount of bones we can handle in a vertex shader (2 vec4 uniforms per vertex)
	if( glConfig.shadingLanguageVersion100 >= 140 ) {
		// require GLSL version 1.40 and higher
		glConfig.maxGLSLBones = bound( 0, glConfig.maxVertexUniformComponents / 8 - 20, r_maxglslbones->integer );
	}
	else {
		glConfig.maxGLSLBones = 0;
	}

	if( glConfig.ext.texture_non_power_of_two )
	{
		// blacklist this extension on Radeon X1600-X1950 hardware (they support it only with certain filtering/repeat modes)
		int val = 0;

		// LordHavoc: this is blocked on Mac OS X because the drivers claim support but often can't accelerate it or crash when used.
#ifndef __APPLE__
		if( glConfig.ext.vertex_shader )
			qglGetIntegerv( GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB, &val );
#endif

		if( val <= 0 )
		{
			glConfig.ext.texture_non_power_of_two = qfalse;
			Cvar_ForceSet( "gl_ext_texture_non_power_of_two", "0" );
		}
	}

	cvar = Cvar_Get( "gl_ext_vertex_buffer_object_hack", "0", CVAR_ARCHIVE|CVAR_NOSET );
	if( cvar && !cvar->integer ) 
	{
		Cvar_ForceSet( cvar->name, "1" );
		Cvar_ForceSet( "gl_ext_vertex_buffer_object", "1" );
	}

	Cvar_Get( "gl_ext_texture_filter_anisotropic_max", "0", CVAR_READONLY );
	Cvar_ForceSet( "gl_ext_texture_filter_anisotropic_max", va( "%i", glConfig.maxTextureFilterAnisotropic ) );

	// don't allow too high values for lightmap block size as they negatively impact performance
	if( r_lighting_maxlmblocksize->integer > glConfig.maxTextureSize / 4 &&
		!(glConfig.maxTextureSize / 4 < min(QF_LIGHTMAP_WIDTH,QF_LIGHTMAP_HEIGHT)*2) )
		Cvar_ForceSet( "r_lighting_maxlmblocksize", va( "%i", glConfig.maxTextureSize / 4 ) );
}

//=======================================================================

void R_Register( void )
{
	r_norefresh = Cvar_Get( "r_norefresh", "0", 0 );
	r_fullbright = Cvar_Get( "r_fullbright", "0", CVAR_LATCH_VIDEO );
	r_lightmap = Cvar_Get( "r_lightmap", "0", 0 );
	r_drawentities = Cvar_Get( "r_drawentities", "1", CVAR_CHEAT );
	r_drawworld = Cvar_Get( "r_drawworld", "1", CVAR_CHEAT );
	r_novis = Cvar_Get( "r_novis", "0", 0 );
	r_nocull = Cvar_Get( "r_nocull", "0", 0 );
	r_lerpmodels = Cvar_Get( "r_lerpmodels", "1", 0 );
	r_speeds = Cvar_Get( "r_speeds", "0", 0 );
	r_drawelements = Cvar_Get( "r_drawelements", "1", 0 );
	r_showtris = Cvar_Get( "r_showtris", "0", CVAR_CHEAT );
	r_lockpvs = Cvar_Get( "r_lockpvs", "0", CVAR_CHEAT );
	r_clear = Cvar_Get( "r_clear", "0", CVAR_ARCHIVE );
	r_nobind = Cvar_Get( "r_nobind", "0", 0 );
	r_picmip = Cvar_Get( "r_picmip", "0", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	r_skymip = Cvar_Get( "r_skymip", "0", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	r_polyblend = Cvar_Get( "r_polyblend", "1", 0 );

	r_bloom = Cvar_Get( "r_bloom", "0", CVAR_ARCHIVE );
	r_bloom_alpha = Cvar_Get( "r_bloom_alpha", "0.2", CVAR_ARCHIVE );
	r_bloom_diamond_size = Cvar_Get( "r_bloom_diamond_size", "8", CVAR_ARCHIVE );
	r_bloom_intensity = Cvar_Get( "r_bloom_intensity", "1.3", CVAR_ARCHIVE );
	r_bloom_darken = Cvar_Get( "r_bloom_darken", "4", CVAR_ARCHIVE );
	r_bloom_sample_size = Cvar_Get( "r_bloom_sample_size", "320", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	r_bloom_fast_sample = Cvar_Get( "r_bloom_fast_sample", "0", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );

	r_ignorehwgamma = Cvar_Get( "r_ignorehwgamma", "0", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	r_overbrightbits = Cvar_Get( "r_overbrightbits", "0", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	r_mapoverbrightbits = Cvar_Get( "r_mapoverbrightbits", "2", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );

	r_detailtextures = Cvar_Get( "r_detailtextures", "1", CVAR_ARCHIVE );
	r_flares = Cvar_Get( "r_flares", "0", CVAR_ARCHIVE );
	r_flaresize = Cvar_Get( "r_flaresize", "40", CVAR_ARCHIVE );
	r_flarefade = Cvar_Get( "r_flarefade", "3", CVAR_ARCHIVE );

	r_dynamiclight = Cvar_Get( "r_dynamiclight", "1", CVAR_ARCHIVE );
	r_coronascale = Cvar_Get( "r_coronascale", "0.2", 0 );
	r_subdivisions = Cvar_Get( "r_subdivisions", STR_TOSTR( SUBDIVISIONS_DEFAULT ), CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	r_faceplanecull = Cvar_Get( "r_faceplanecull", "1", CVAR_ARCHIVE );
	r_shownormals = Cvar_Get( "r_shownormals", "0", CVAR_CHEAT );
	r_draworder = Cvar_Get( "r_draworder", "0", CVAR_CHEAT );

	r_fastsky = Cvar_Get( "r_fastsky", "0", CVAR_ARCHIVE );
	r_portalonly = Cvar_Get( "r_portalonly", "0", 0 );
	r_portalmaps = Cvar_Get( "r_portalmaps", "1", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	r_portalmaps_maxtexsize = Cvar_Get( "r_portalmaps_maxtexsize", "512", CVAR_ARCHIVE );

	r_allow_software = Cvar_Get( "r_allow_software", "0", 0 );

	r_lighting_bumpscale = Cvar_Get( "r_lighting_bumpscale", "8", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	r_lighting_deluxemapping = Cvar_Get( "r_lighting_deluxemapping", "1", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	r_lighting_useblankbumpmap = Cvar_Get( "r_lighting_useblankbumpmap", "1", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	r_lighting_diffuse2heightmap = Cvar_Get( "r_lighting_diffuse2heightmap", "0", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	r_lighting_specular = Cvar_Get( "r_lighting_specular", "1", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	r_lighting_glossintensity = Cvar_Get( "r_lighting_glossintensity", "1", CVAR_ARCHIVE );
	r_lighting_glossexponent = Cvar_Get( "r_lighting_glossexponent", "48", CVAR_ARCHIVE );
	r_lighting_ambientscale = Cvar_Get( "r_lighting_ambientscale", "1.0", 0 );
	r_lighting_directedscale = Cvar_Get( "r_lighting_directedscale", "1.0", 0 );

	r_lighting_packlightmaps = Cvar_Get( "r_lighting_packlightmaps", "1", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	r_lighting_maxlmblocksize = Cvar_Get( "r_lighting_maxlmblocksize", "2048", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	r_lighting_vertexlight = Cvar_Get( "r_lighting_vertexlight", "0", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	r_lighting_maxglsldlights = Cvar_Get( "r_lighting_maxglsldlights", "8", CVAR_ARCHIVE|CVAR_READONLY );
	r_lighting_grayscale = Cvar_Get( "r_lighting_grayscale", "0", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );

	r_offsetmapping = Cvar_Get( "r_offsetmapping", "2", CVAR_ARCHIVE );
	r_offsetmapping_scale = Cvar_Get( "r_offsetmapping_scale", "0.02", CVAR_ARCHIVE );
	r_offsetmapping_reliefmapping = Cvar_Get( "r_offsetmapping_reliefmapping", "0", CVAR_ARCHIVE );

	r_occlusion_queries = Cvar_Get( "r_occlusion_queries", "0", CVAR_ARCHIVE );
	r_occlusion_queries_finish = Cvar_Get( "r_occlusion_queries_finish", "1", CVAR_ARCHIVE );

#ifdef CGAMEGETLIGHTORIGIN
	r_shadows = Cvar_Get( "cg_shadows", "1", CVAR_ARCHIVE );
#else
	r_shadows = Cvar_Get( "r_shadows", "0", CVAR_ARCHIVE );
#endif
	r_shadows_alpha = Cvar_Get( "r_shadows_alpha", "0.4", CVAR_ARCHIVE );
	r_shadows_nudge = Cvar_Get( "r_shadows_nudge", "1", CVAR_ARCHIVE );
	r_shadows_projection_distance = Cvar_Get( "r_shadows_projection_distance", "128", CVAR_CHEAT );
	r_shadows_maxtexsize = Cvar_Get( "r_shadows_maxtexsize", "128", CVAR_ARCHIVE );
	r_shadows_pcf = Cvar_Get( "r_shadows_pcf", "4", CVAR_ARCHIVE );
	r_shadows_self_shadow = Cvar_Get( "r_shadows_self_shadow", "0", CVAR_ARCHIVE );
	r_shadows_dither = Cvar_Get( "r_shadows_dither", "1", CVAR_ARCHIVE );

#ifdef HARDWARE_OUTLINES
	r_outlines_world = Cvar_Get( "r_outlines_world", "1.8", CVAR_ARCHIVE );
	r_outlines_scale = Cvar_Get( "r_outlines_scale", "1", CVAR_ARCHIVE );
	r_outlines_cutoff = Cvar_Get( "r_outlines_cutoff", "712", CVAR_ARCHIVE );
#endif

	r_lodbias = Cvar_Get( "r_lodbias", "0", CVAR_ARCHIVE );
	r_lodscale = Cvar_Get( "r_lodscale", "5.0", CVAR_ARCHIVE );

	r_gamma = Cvar_Get( "r_gamma", "1.0", CVAR_ARCHIVE );
	r_texturebits = Cvar_Get( "r_texturebits", "0", CVAR_ARCHIVE | CVAR_LATCH_VIDEO );
	r_texturemode = Cvar_Get( "r_texturemode", "GL_LINEAR_MIPMAP_LINEAR", CVAR_ARCHIVE );
	r_texturefilter = Cvar_Get( "r_texturefilter", "1", CVAR_ARCHIVE );
	r_stencilbits = Cvar_Get( "r_stencilbits", "8", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );

	r_screenshot_jpeg = Cvar_Get( "r_screenshot_jpeg", "1", CVAR_ARCHIVE );
	r_screenshot_jpeg_quality = Cvar_Get( "r_screenshot_jpeg_quality", "90", CVAR_ARCHIVE );
	r_screenshot_fmtstr = Cvar_Get( "r_screenshot_fmtstr", APP_SCREENSHOTS_PREFIX "%y%m%d_%H%M%S", CVAR_ARCHIVE );

#ifdef GLX_VERSION
	r_swapinterval = Cvar_Get( "r_swapinterval", "0", CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
#else
	r_swapinterval = Cvar_Get( "r_swapinterval", "0", CVAR_ARCHIVE );
#endif
	// make sure r_swapinterval is checked after vid_restart
	r_swapinterval->modified = qtrue;

	r_temp1 = Cvar_Get( "r_temp1", "0", 0 );

	r_drawflat = Cvar_Get( "r_drawflat", "0", CVAR_ARCHIVE );
	r_wallcolor = Cvar_Get( "r_wallcolor", "255 255 255", CVAR_ARCHIVE );
	r_floorcolor = Cvar_Get( "r_floorcolor", "255 153 0", CVAR_ARCHIVE );

	// make sure we rebuild our 3D texture after vid_restart
	r_wallcolor->modified = r_floorcolor->modified = qtrue;

	r_maxglslbones = Cvar_Get( "r_maxglslbones", STR_TOSTR( MAX_GLSL_BONES ), CVAR_LATCH_VIDEO );

	gl_finish = Cvar_Get( "gl_finish", "0", CVAR_ARCHIVE );
	gl_cull = Cvar_Get( "gl_cull", "1", 0 );
	gl_driver = Cvar_Get( "gl_driver", GL_DRIVERNAME, CVAR_ARCHIVE|CVAR_LATCH_VIDEO );
	gl_drawbuffer = Cvar_Get( "gl_drawbuffer", "GL_BACK", 0 );
	gl_extensions = Cvar_Get( "gl_extensions", "1", CVAR_ARCHIVE|CVAR_LATCH_VIDEO
#ifdef PUBLIC_BUILD
		|CVAR_READONLY
#endif
	);

	Cmd_AddCommand( "imagelist", R_ImageList_f );
	Cmd_AddCommand( "shaderlist", R_ShaderList_f );
	Cmd_AddCommand( "shaderdump", R_ShaderDump_f );
	Cmd_AddCommand( "screenshot", R_ScreenShot_f );
	Cmd_AddCommand( "envshot", R_EnvShot_f );
	Cmd_AddCommand( "modellist", Mod_Modellist_f );
	Cmd_AddCommand( "gfxinfo", R_GfxInfo_f );
	Cmd_AddCommand( "glslprogramlist", R_ProgramList_f );
	Cmd_AddCommand( "glslprogramdump", R_ProgramDump_f );
}

/*
* R_SetDefaultTexState
*/
static void R_SetDefaultTexState( void )
{
	memset( glState.currentTextures, -1, MAX_TEXTURE_UNITS*sizeof(*glState.currentTextures) );
	memset( glState.currentEnvModes, -1, MAX_TEXTURE_UNITS*sizeof(*glState.currentEnvModes) );
	memset( glState.texIdentityMatrix, 0, MAX_TEXTURE_UNITS*sizeof(*glState.texIdentityMatrix) );
	memset( glState.genSTEnabled, 0, MAX_TEXTURE_UNITS*sizeof(*glState.genSTEnabled) );
	memset( glState.texCoordArrayMode, 0, MAX_TEXTURE_UNITS*sizeof(*glState.texCoordArrayMode) );
}

/*
* R_SetDefaultState
*/
static void R_SetDefaultState( void )
{
	// FIXME: dynamically allocate these?
	static GLuint r_currentTextures[MAX_TEXTURE_UNITS];
	static int r_currentEnvModes[MAX_TEXTURE_UNITS];
	static qboolean	r_texIdentityMatrix[MAX_TEXTURE_UNITS];
	static int r_genSTEnabled[MAX_TEXTURE_UNITS];
	static int r_texCoordArrayMode[MAX_TEXTURE_UNITS];

	memset( &glState, 0, sizeof(glState) );

	glState.currentTextures = r_currentTextures;
	glState.currentEnvModes = r_currentEnvModes;
	glState.texIdentityMatrix = r_texIdentityMatrix;
	glState.genSTEnabled = r_genSTEnabled;
	glState.texCoordArrayMode = r_texCoordArrayMode;

	R_SetDefaultTexState ();

	// set our "safe" modes
	glState.initializedMedia = qfalse;
	glState.warmupRenderer = qtrue;
}

/*
* R_SetGLDefaults
*/
static void R_SetGLDefaults( void )
{
	int i;

	qglFinish();

	qglClearColor( 1, 0, 0.5, 0.5 );

	qglEnable( GL_DEPTH_TEST );
	qglDisable( GL_CULL_FACE );
	qglEnable( GL_SCISSOR_TEST );
	qglDepthFunc( GL_LEQUAL );
	qglDepthMask( GL_FALSE );

	if( glState.stencilEnabled )
	{
		qglDisable( GL_STENCIL_TEST );
		qglStencilMask( ( GLuint ) ~0 );
		qglStencilFunc( GL_EQUAL, 128, 0xFF );
		qglStencilOp( GL_KEEP, GL_KEEP, GL_INCR );
	}

	// enable gouraud shading
	qglShadeModel( GL_SMOOTH );

	qglColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );

	qglColor4f( 1, 1, 1, 1 );

	qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	qglPolygonOffset( -1, -2 );

	// properly disable multitexturing at startup
	for( i = glConfig.maxTextureUnits-1; i > 0; i-- )
	{
		GL_SelectTexture( i );
		GL_TexEnv( GL_MODULATE );
		qglDisable( GL_BLEND );
		qglDisable( GL_TEXTURE_2D );
	}

	GL_SelectTexture( 0 );
	qglDisable( GL_BLEND );
	qglDisable( GL_ALPHA_TEST );
	qglDisable( GL_POLYGON_OFFSET_FILL );
	qglEnable( GL_TEXTURE_2D );
	qglDisable( GL_TEXTURE_3D );

	GL_Cull( 0 );
	GL_FrontFace( 0 );

	GL_SetState( GLSTATE_DEPTHWRITE );
	GL_TexEnv( GL_MODULATE );

	R_TextureMode( r_texturemode->string );

	R_AnisotropicFilter( r_texturefilter->integer );

	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
}

/*
* R_GfxInfo_f
*/
static void R_GfxInfo_f( void )
{
	size_t len, p;

	Com_Printf( "\n" );
	Com_Printf( "GL_VENDOR: %s\n", glConfig.vendorString );
	Com_Printf( "GL_RENDERER: %s\n", glConfig.rendererString );
	Com_Printf( "GL_VERSION: %s\n", glConfig.versionString );
	Com_Printf( "GL_SHADING_LANGUAGE_VERSION: %s\n", glConfig.shadingLanguageVersionString );

	Com_Printf( "GL_EXTENSIONS: " );
	if( glConfig.extensionsString )
	{
		for( len = strlen( glConfig.extensionsString ), p = 0; p < len;  )
		{
			char chunk[MAX_PRINTMSG];

			Q_snprintfz( chunk, sizeof( chunk ), "%s", glConfig.extensionsString + p );
			Com_Printf( "%s", chunk );

			p += strlen( chunk );
		}
	}
	else
	{
		Com_Printf( "none" );
	}
	Com_Printf( "\n" );

	if( *glConfig.glwExtensionsString )
		Com_Printf( "GLW_EXTENSIONS: %s\n", glConfig.glwExtensionsString );
	Com_Printf( "GL_MAX_TEXTURE_SIZE: %i\n", glConfig.maxTextureSize );
	Com_Printf( "GL_MAX_TEXTURE_UNITS: %i\n", glConfig.maxTextureUnits );
	if( glConfig.ext.texture_cube_map )
		Com_Printf( "GL_MAX_CUBE_MAP_TEXTURE_SIZE: %i\n", glConfig.maxTextureCubemapSize );
	if( glConfig.ext.texture3D )
		Com_Printf( "GL_MAX_3D_TEXTURE_SIZE: %i\n", glConfig.maxTextureSize3D );
	if( glConfig.ext.texture_filter_anisotropic )
		Com_Printf( "GL_MAX_TEXTURE_MAX_ANISOTROPY: %i\n", glConfig.maxTextureFilterAnisotropic );
	if( glConfig.ext.GLSL ) {
		Com_Printf( "GL_MAX_VARYING_FLOATS: %i\n", glConfig.maxVaryingFloats );
		Com_Printf( "GL_MAX_VERTEX_UNIFORM_COMPONENTS: %i\n", glConfig.maxVertexUniformComponents );
		Com_Printf( "GL_MAX_FRAGMENT_UNIFORM_COMPONENTS: %i\n", glConfig.maxFragmentUniformComponents );
	}
	Com_Printf( "\n" );

	Com_Printf( "mode: %ix%i%s%s\n", glState.width, glState.height,
		glState.fullScreen ? ", fullscreen" : ", windowed", glState.wideScreen ? ", widescreen" : "" );
	Com_Printf( "picmip: %i\n", r_picmip->integer );
	Com_Printf( "texturemode: %s\n", r_texturemode->string );
	Com_Printf( "anisotropic filtering: %i\n", r_texturefilter->integer );
	Com_Printf( "vertical sync: %s\n", r_swapinterval->integer ? "enabled" : "disabled" );

	R_PrintGLExtensionsInfo();

	R_PrintMemoryInfo();
}

/*
* R_Init
*/
rserr_t R_Init( void *hinstance, void *wndproc, void *parenthWnd, int x, int y, int width, int height, qboolean fullScreen, qboolean wideScreen, qboolean verbose )
{
	char renderer_buffer[1024];
	char vendor_buffer[1024];
	rserr_t err;

	r_verbose = verbose;

	Com_Printf( "\n----- R_Init -----\n" );

	R_Register();

	R_SetDefaultState();

	// initialize our QGL dynamic bindings
init_qgl:
	if( !QGL_Init( gl_driver->string ) )
	{
		QGL_Shutdown();
		Com_Printf( "ref_gl::R_Init() - could not load \"%s\"\n", gl_driver->string );

		if( strcmp( gl_driver->string, GL_DRIVERNAME ) )
		{
			Cvar_ForceSet( gl_driver->name, GL_DRIVERNAME );
			goto init_qgl;
		}

		return rserr_invalid_driver;
	}

	// initialize OS-specific parts of OpenGL
	if( !GLimp_Init( hinstance, wndproc, parenthWnd ) )
	{
		QGL_Shutdown();
		return rserr_unknown;
	}

	// create the window and set up the context
	err = GLimp_SetMode( x, y, width, height, fullScreen, wideScreen );
	if( err != rserr_ok )
	{
		QGL_Shutdown();
		Com_Printf( "ref_gl::R_Init() - could not GLimp_SetMode()\n" );
		return err;
	}

	if( r_ignorehwgamma->integer )
		glState.hwGamma = qfalse;
	else
		glState.hwGamma = GLimp_GetGammaRamp( 256, glState.orignalGammaRamp );
	if( glState.hwGamma )
		r_gamma->modified = qtrue;

	/*
	** get our various GL strings
	*/
	glConfig.vendorString = (const char *)qglGetString( GL_VENDOR );
	glConfig.rendererString = (const char *)qglGetString( GL_RENDERER );
	glConfig.versionString = (const char *)qglGetString( GL_VERSION );
	glConfig.extensionsString = (const char *)qglGetString( GL_EXTENSIONS );
	glConfig.glwExtensionsString = (const char *)qglGetGLWExtensionsString ();
	glConfig.shadingLanguageVersionString = (const char *)qglGetString( GL_SHADING_LANGUAGE_VERSION_ARB );

	if( !glConfig.vendorString ) glConfig.vendorString = "";
	if( !glConfig.rendererString ) glConfig.rendererString = "";
	if( !glConfig.versionString ) glConfig.versionString = "";
	if( !glConfig.extensionsString ) glConfig.extensionsString = "";
	if( !glConfig.glwExtensionsString ) glConfig.glwExtensionsString = "";
	if( !glConfig.shadingLanguageVersionString ) glConfig.shadingLanguageVersionString = "";

	Q_strncpyz( renderer_buffer, glConfig.rendererString, sizeof( renderer_buffer ) );
	Q_strlwr( renderer_buffer );

	Q_strncpyz( vendor_buffer, glConfig.vendorString, sizeof( vendor_buffer ) );
	Q_strlwr( vendor_buffer );

	memset( &r_front, 0, sizeof( r_front ) );
	r_front.registration_sequence = 1; 

	R_RegisterGLExtensions();

	R_SetGLDefaults();

	if( r_verbose )
		R_GfxInfo_f();

	// load and compile GLSL programs
	R_InitGLSLPrograms();

	R_BackendInit();

	R_InitMeshLists();

	R_InitVBO();

	R_InitFBObjects();

	R_InitImages();

	R_InitCinematics();

	R_InitShaders();

	R_InitSkinFiles();

	R_InitModels();

	R_ClearScene();

	R_InitVolatileAssets();

	err = qglGetError();
	if( err != GL_NO_ERROR )
		Com_Printf( "glGetError() = 0x%x\n", err );

	Com_Printf( "----- finished R_Init -----\n" );

	return rserr_ok;
}

/*
* R_SetMode
*/
rserr_t R_SetMode( int x, int y, int width, int height, qboolean fullScreen, qboolean wideScreen )
{
	return GLimp_SetMode( x, y, width, height, fullScreen, wideScreen );
}

/*
* R_InitVolatileAssets
*/
static void R_InitVolatileAssets( void )
{
	// init volatile data
	R_InitSkeletalCache();
	R_InitOcclusionQueries();
	R_InitCoronas();
#ifdef HARDWARE_OUTLINES
	R_InitOutlines();
#endif
	R_InitShadows();
	R_InitCustomColors();
}

/*
* R_DestroyVolatileAssets
*/
static void R_DestroyVolatileAssets( void )
{
	// kill volatile data
	R_ShutdownCustomColors();
	R_ShutdownShadows();
#ifdef HARDWARE_OUTLINES
	R_ShutdownOutlines();
#endif
	R_ShutdownCoronas();
	R_ShutdownOcclusionQueries();
	R_ShutdownSkeletalCache();
}

/*
* R_BeginRegistration
*/
void R_BeginRegistration( void )
{
	R_DestroyVolatileAssets();

	r_front.registration_sequence++;
	if( !r_front.registration_sequence ) {
		// make sure assumption that an asset is free it its registration_sequence is 0
		// since r_front.registration_sequence never equals 0
		r_front.registration_sequence = 1; 
	}

	R_BackendCleanUpTextureUnits();

	R_InitVolatileAssets();
}

/*
* R_EndRegistration
*/
void R_EndRegistration( void )
{
	R_FreeUnusedModels();
	R_FreeUnusedVBOs();
	R_FreeUnusedSkinFiles();
	R_FreeUnusedShaders();
	R_FreeUnusedCinematics();
	R_FreeUnusedImages();

	R_SetDefaultTexState ();

	memset( &ri, 0, sizeof( refinst_t ) );

	glState.warmupRenderer = qtrue;
	glState.initializedMedia = qtrue;

	R_BackendResetPassMask();
}

/*
* R_Shutdown
*/
void R_Shutdown( qboolean verbose )
{
	Cmd_RemoveCommand( "modellist" );
	Cmd_RemoveCommand( "screenshot" );
	Cmd_RemoveCommand( "envshot" );
	Cmd_RemoveCommand( "imagelist" );
	Cmd_RemoveCommand( "gfxinfo" );
	Cmd_RemoveCommand( "shaderdump" );
	Cmd_RemoveCommand( "shaderlist" );
	Cmd_RemoveCommand( "glslprogramlist" );
	Cmd_RemoveCommand( "glslprogramdump" );

	R_StopAviDemo();

	R_FreeMeshLists();

	// destroy compiled GLSL programs
	R_ShutdownGLSLPrograms();

	// shutdown rendering backend
	R_BackendShutdown();

	// free shaders, models, etc.

	R_DestroyVolatileAssets();

	R_ShutdownModels();

	R_ShutdownSkinFiles();

	R_ShutdownVBO();

	R_ShutdownShaders();

	R_ShutdownCinematics();

	R_ShutdownImages();

	R_ShutdownFBObjects();

	// restore original gamma
	if( glState.hwGamma )
		GLimp_SetGammaRamp( 256, glState.orignalGammaRamp );

	// shut down OS specific OpenGL stuff like contexts, etc.
	GLimp_Shutdown();

	// shutdown our QGL subsystem
	QGL_Shutdown();
}
