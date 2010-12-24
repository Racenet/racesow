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

// r_program.c - OpenGL Shading Language support

#include "r_local.h"

#define MAX_GLSL_PROGRAMS	    1024
#define GLSL_PROGRAMS_HASH_SIZE 32

typedef struct
{
	int				bit;
	const char		*define;
	const char		*suffix;
} glsl_feature_t;

typedef struct glsl_program_s
{
	char			*name;
	int				type;
	int				features;
	const char		*string;
	struct glsl_program_s *hash_next;

	int				object;
	int				vertexShader;
	int				fragmentShader;

	int				locEyeOrigin,
					locLightDir,
					locLightOrigin,
					locLightAmbient,
					locLightDiffuse,

					locGlossIntensity,
					locGlossExponent,

					locOffsetMappingScale,
#ifdef HARDWARE_OUTLINES
					locOutlineHeight,
					locOutlineCutOff,
#endif
					locFrontPlane,
					locInvTextureWidth,
					locInvTextureHeight,
					locProjDistance,

					locTurbAmplitude,
					locTurbPhase,

					locEntDist,
					locConstColor,
					locOverbrightScale,

					locFogPlane,
					locEyePlane,
					locEyeFogDist,

					locDeluxemapOffset[MAX_LIGHTMAPS],
					loclsColor[MAX_LIGHTMAPS]
	;
} glsl_program_t;

static int r_numglslprograms;
static glsl_program_t r_glslprograms[MAX_GLSL_PROGRAMS];
static glsl_program_t *r_glslprograms_hash[GLSL_PROGRAMS_HASH_SIZE];
static mempool_t *r_glslProgramsPool;

static void R_GetProgramUniformLocations( glsl_program_t *program );

static const char *r_defaultGLSLProgram;
static const char *r_defaultDistortionGLSLProgram;
static const char *r_defaultShadowmapGLSLProgram;
#ifdef HARDWARE_OUTLINES
static const char *r_defaultOutlineGLSLProgram;
#endif
static const char *r_defaultTurbulenceProgram;
static const char *r_defaultDynamicLightsProgram;
static const char *r_defaultQ3AShaderProgram;

/*
================
R_InitGLSLPrograms
================
*/
void R_InitGLSLPrograms( void )
{
	int i, bit;
	int features, common;

	memset( r_glslprograms, 0, sizeof( r_glslprograms ) );
	memset( r_glslprograms_hash, 0, sizeof( r_glslprograms_hash ) );

	if( !glConfig.ext.GLSL )
		return;

	r_glslProgramsPool = Mem_AllocPool( NULL, "GLSL Programs" );

	// common features, determined by enabled extensions
	common = R_CommonProgramFeatures ();

	// register programs that are most likely to be used

	features = common;
	R_RegisterGLSLProgram( PROGRAM_TYPE_MATERIAL, DEFAULT_GLSL_PROGRAM, NULL, 0|features );
	R_RegisterGLSLProgram( PROGRAM_TYPE_MATERIAL, DEFAULT_GLSL_PROGRAM, NULL, PROGRAM_APPLY_FB_LIGHTMAP|PROGRAM_APPLY_LIGHTSTYLE0|features );
	R_RegisterGLSLProgram( PROGRAM_TYPE_MATERIAL, DEFAULT_GLSL_PROGRAM, NULL, PROGRAM_APPLY_FB_LIGHTMAP|PROGRAM_APPLY_LIGHTSTYLE0|PROGRAM_APPLY_SPECULAR|features );
	R_RegisterGLSLProgram( PROGRAM_TYPE_MATERIAL, DEFAULT_GLSL_PROGRAM, NULL, PROGRAM_APPLY_FB_LIGHTMAP|PROGRAM_APPLY_LIGHTSTYLE0
		|PROGRAM_APPLY_SPECULAR|PROGRAM_APPLY_AMBIENT_COMPENSATION|features );

	features = common;
#ifdef CELLSHADEDMATERIAL
	features |= PROGRAM_APPLY_CELLSHADING;
#endif

#ifdef HALFLAMBERTLIGHTING
	features |= PROGRAM_APPLY_HALFLAMBERT;
#endif

	R_RegisterGLSLProgram( PROGRAM_TYPE_MATERIAL, DEFAULT_GLSL_PROGRAM, NULL, PROGRAM_APPLY_DIRECTIONAL_LIGHT|features );
//	R_RegisterGLSLProgram( PROGRAM_TYPE_MATERIAL, DEFAULT_GLSL_PROGRAM, NULL, PROGRAM_APPLY_DIRECTIONAL_LIGHT|PROGRAM_APPLY_SPECULAR|features );
//	R_RegisterGLSLProgram( PROGRAM_TYPE_MATERIAL, DEFAULT_GLSL_PROGRAM, NULL, PROGRAM_APPLY_DIRECTIONAL_LIGHT|PROGRAM_APPLY_OFFSETMAPPING|features );

	features = common;

	R_RegisterGLSLProgram( PROGRAM_TYPE_DISTORTION, DEFAULT_GLSL_DISTORTION_PROGRAM, r_defaultDistortionGLSLProgram, 0|features );

	R_RegisterGLSLProgram( PROGRAM_TYPE_SHADOWMAP, DEFAULT_GLSL_SHADOWMAP_PROGRAM, r_defaultShadowmapGLSLProgram, 0|features );

#ifdef HARDWARE_OUTLINES
	R_RegisterGLSLProgram( PROGRAM_TYPE_OUTLINE, DEFAULT_GLSL_OUTLINE_PROGRAM, r_defaultOutlineGLSLProgram, 0|features );
#endif

	R_RegisterGLSLProgram( PROGRAM_TYPE_TURBULENCE, DEFAULT_GLSL_TURBULENCE_PROGRAM, r_defaultTurbulenceProgram, 0|features );

	R_RegisterGLSLProgram( PROGRAM_TYPE_Q3A_SHADER, DEFAULT_GLSL_Q3A_SHADER_PROGRAM, r_defaultQ3AShaderProgram, 0|features );

	for( i = bit = PROGRAM_APPLY_DLIGHT0; i <= PROGRAM_APPLY_DLIGHT_MAX; i <<= 1 )
	{
		bit |= i;
		R_RegisterGLSLProgram( PROGRAM_TYPE_DYNAMIC_LIGHTS, DEFAULT_GLSL_DYNAMIC_LIGHTS_PROGRAM, r_defaultDynamicLightsProgram, 0|features|bit );
	}
}

/*
================
R_GLSLProgramCopyString
================
*/
static char *R_GLSLProgramCopyString( const char *in )
{
	char *out;

	out = Mem_Alloc( r_glslProgramsPool, ( strlen( in ) + 1 ) );
	strcpy( out, in );

	return out;
}

/*
================
R_DeleteGLSLProgram
================
*/
static void R_DeleteGLSLProgram( glsl_program_t *program )
{
	glsl_program_t *hash_next;

	if( program->vertexShader )
	{
		qglDetachObjectARB( program->object, program->vertexShader );
		qglDeleteObjectARB( program->vertexShader );
		program->vertexShader = 0;
	}

	if( program->fragmentShader )
	{
		qglDetachObjectARB( program->object, program->fragmentShader );
		qglDeleteObjectARB( program->fragmentShader );
		program->fragmentShader = 0;
	}

	if( program->object )
		qglDeleteObjectARB( program->object );

	if( program->name )
		Mem_Free( program->name );

	hash_next = program->hash_next;
	memset( program, 0, sizeof( glsl_program_t ) );
	program->hash_next = hash_next;
}

/*
================
R_CompileGLSLShader
================
*/
static int R_CompileGLSLShader( int program, const char *programName, const char *shaderName, int shaderType, const char **strings, int numStrings )
{
	int shader, compiled;

	shader = qglCreateShaderObjectARB( (GLenum)shaderType );
	if( !shader )
		return 0;

	// if lengths is NULL, then each string is assumed to be null-terminated
	qglShaderSourceARB( shader, numStrings, strings, NULL );
	qglCompileShaderARB( shader );
	qglGetObjectParameterivARB( shader, GL_OBJECT_COMPILE_STATUS_ARB, &compiled );

	if( !compiled )
	{
		char log[4096];

		qglGetInfoLogARB( shader, sizeof( log ) - 1, NULL, log );
		log[sizeof( log ) - 1] = 0;

		if( log[0] )
			Com_Printf( S_COLOR_YELLOW "Failed to compile %s shader for program %s:\n%s\n", shaderName, programName, log );

		qglDeleteObjectARB( shader );
		return 0;
	}

	qglAttachObjectARB( program, shader );

	return shader;
}

/*
================
R_FindGLSLProgram
================
*/
int R_FindGLSLProgram( const char *name )
{
	int i;
	glsl_program_t *program;

	for( i = 0, program = r_glslprograms; i < MAX_GLSL_PROGRAMS; i++, program++ )
	{
		if( !program->name )
			break;
		if( !Q_stricmp( program->name, name ) )
			return ( i+1 );
	}

	return 0;
}

// ======================================================================================

#define MAX_DEFINES_FEATURES	255

static const glsl_feature_t glsl_features_standard[] =
{
	{ PROGRAM_APPLY_CLIPPING, "#define APPLY_CLIPPING\n", "_clip" },
	{ PROGRAM_APPLY_GRAYSCALE, "#define APPLY_GRAYSCALE\n", "_grey" },

	{ 0, NULL, NULL }
};

static const glsl_feature_t glsl_features_materal[] =
{
	{ PROGRAM_APPLY_CLIPPING, "#define APPLY_CLIPPING\n", "_clip" },
	{ PROGRAM_APPLY_GRAYSCALE, "#define APPLY_GRAYSCALE\n", "_grey" },

	{ PROGRAM_APPLY_TC_GEN_ENV, "#define APPLY_TC_GEN_ENV\n", "_tc_env" },
	{ PROGRAM_APPLY_TC_GEN_VECTOR, "#define APPLY_TC_GEN_VECTOR\n", "_tc_vec" },
	{ PROGRAM_APPLY_TC_GEN_REFLECTION, "#define APPLY_TC_GEN_REFLECTION\n", "_tc_refl" },

	{ PROGRAM_APPLY_RGB_GEN_CONST, "#define APPLY_RGB_CONST\n", "_cc" },
	{ PROGRAM_APPLY_RGB_GEN_VERTEX, "#define APPLY_RGB_VERTEX\n", "_cv" },
	{ PROGRAM_APPLY_RGB_GEN_ONE_MINUS_VERTEX, "#define APPLY_RGB_ONE_MINUS_VERTEX\n", "_c1-v" },

	{ PROGRAM_APPLY_ALPHA_GEN_CONST, "#define APPLY_ALPHA_CONST\n", "_ac" },
	{ PROGRAM_APPLY_ALPHA_GEN_VERTEX, "#define APPLY_ALPHA_VERTEX\n", "_av" },
	{ PROGRAM_APPLY_ALPHA_GEN_ONE_MINUS_VERTEX, "#define APPLY_ALPHA_ONE_MINUS_VERTEX\n", "_a1-v" },

	{ PROGRAM_APPLY_LIGHTSTYLE0, "#define APPLY_LIGHTSTYLE0\n", "_ls0" },
	{ PROGRAM_APPLY_FB_LIGHTMAP, "#define APPLY_FBLIGHTMAP\n", "_fb" },
	{ PROGRAM_APPLY_LIGHTSTYLE1, "#define APPLY_LIGHTSTYLE1\n", "_ls1" },
	{ PROGRAM_APPLY_LIGHTSTYLE2, "#define APPLY_LIGHTSTYLE2\n", "_ls2" },
	{ PROGRAM_APPLY_LIGHTSTYLE3, "#define APPLY_LIGHTSTYLE3\n", "_ls3" },
	{ PROGRAM_APPLY_DIRECTIONAL_LIGHT, "#define APPLY_DIRECTIONAL_LIGHT\n", "_dirlight" },
	{ PROGRAM_APPLY_SPECULAR, "#define APPLY_SPECULAR\n", "_gloss" },
	{ PROGRAM_APPLY_OFFSETMAPPING, "#define APPLY_OFFSETMAPPING\n", "_offmap" },
	{ PROGRAM_APPLY_RELIEFMAPPING, "#define APPLY_RELIEFMAPPING\n", "_relmap" },
	{ PROGRAM_APPLY_AMBIENT_COMPENSATION, "#define APPLY_AMBIENT_COMPENSATION\n", "_amb" },
	{ PROGRAM_APPLY_DECAL, "#define APPLY_DECAL\n", "_decal" },
	{ PROGRAM_APPLY_BASETEX_ALPHA_ONLY, "#define APPLY_BASETEX_ALPHA_ONLY\n", "_alpha" },
	{ PROGRAM_APPLY_DECAL_ADD, "#define APPLY_DECAL_ADD\n", "_decal_add" },
	{ PROGRAM_APPLY_CLAMPING, "#define APPLY_COLOR_CLAMPING\n", "_clamp" },
	{ PROGRAM_APPLY_CELLSHADING, "#define APPLY_CELLSHADING\n", "_cell" },

	// doesn't make sense without APPLY_DIRECTIONAL_LIGHT
	{ PROGRAM_APPLY_DIRECTIONAL_LIGHT_MIX, "#define APPLY_DIRECTIONAL_LIGHT_MIX\n", "_mix" },

	{ PROGRAM_APPLY_FOG, "#define APPLY_FOG\n", "_fog" },
	{ PROGRAM_APPLY_FOG2, "#define APPLY_FOG2\n", "_2" },

	{ PROGRAM_APPLY_HALFLAMBERT, "#define APPLY_HALFLAMBERT\n", "_2" },

	{ PROGRAM_APPLY_OVERBRIGHT_SCALING, "#define APPLY_OVERBRIGHT_SCALING\n", "_os" },

	{ 0, NULL, NULL }
};

static const glsl_feature_t glsl_features_distortion[] =
{
	{ PROGRAM_APPLY_CLIPPING, "#define APPLY_CLIPPING\n", "_clip" },
	{ PROGRAM_APPLY_GRAYSCALE, "#define APPLY_GRAYSCALE\n", "_grey" },

	{ PROGRAM_APPLY_DUDV, "#define APPLY_DUDV\n", "_dudv" },
	{ PROGRAM_APPLY_EYEDOT, "#define APPLY_EYEDOT\n", "_eyedot" },
	{ PROGRAM_APPLY_DISTORTION_ALPHA, "#define APPLY_DISTORTION_ALPHA\n", "_alpha" },
	{ PROGRAM_APPLY_REFLECTION, "#define APPLY_REFLECTION\n", "_refl" },
	{ PROGRAM_APPLY_REFRACTION, "#define APPLY_REFRACTION\n", "_refr" },

	{ 0, NULL, NULL }
};

static const glsl_feature_t glsl_features_shadowmap[] =
{
	{ PROGRAM_APPLY_CLIPPING, "#define APPLY_CLIPPING\n", "_clip" },

	{ PROGRAM_APPLY_PCF2x2, "#define APPLY_PCF2x2\n", "_pcf2x2" },
	{ PROGRAM_APPLY_PCF3x3, "#define APPLY_PCF3x3\n", "_pcf3x3" },
	{ PROGRAM_APPLY_PCF4x4, "#define APPLY_PCF4x4\n", "_pcf4x4" },

	{ 0, NULL, NULL }
};

static const glsl_feature_t glsl_features_outline[] =
{
	{ PROGRAM_APPLY_CLIPPING, "#define APPLY_CLIPPING\n", "_clip" },

	{ PROGRAM_APPLY_OUTLINES_CUTOFF, "#define APPLY_OUTLINES_CUTOFF\n", "_outcut" },

	{ 0, NULL, NULL }
};

static const glsl_feature_t glsl_features_dynamiclights[] =
{
	{ PROGRAM_APPLY_CLIPPING, "#define APPLY_CLIPPING\n", "_clip" },

	{ PROGRAM_APPLY_DLIGHT0, "#undef MAX_DLIGHTS\n#define MAX_DLIGHTS 1\n", "_0" },
	{ PROGRAM_APPLY_DLIGHT1, "#undef MAX_DLIGHTS\n#define MAX_DLIGHTS 2\n", "1" },
	{ PROGRAM_APPLY_DLIGHT2, "#undef MAX_DLIGHTS\n#define MAX_DLIGHTS 3\n", "2" },
	{ PROGRAM_APPLY_DLIGHT3, "#undef MAX_DLIGHTS\n#define MAX_DLIGHTS 4\n", "3" },
	{ PROGRAM_APPLY_DLIGHT4, "#undef MAX_DLIGHTS\n#define MAX_DLIGHTS 5\n", "4" },
	{ PROGRAM_APPLY_DLIGHT5, "#undef MAX_DLIGHTS\n#define MAX_DLIGHTS 6\n", "5" },
	{ PROGRAM_APPLY_DLIGHT6, "#undef MAX_DLIGHTS\n#define MAX_DLIGHTS 7\n", "6" },
	{ PROGRAM_APPLY_DLIGHT7, "#undef MAX_DLIGHTS\n#define MAX_DLIGHTS 8\n", "7" },

	{ 0, NULL, NULL }
};

static const glsl_feature_t glsl_features_q3a[] =
{
	{ PROGRAM_APPLY_CLIPPING, "#define APPLY_CLIPPING\n", "_clip" },

	{ PROGRAM_APPLY_TC_GEN_ENV, "#define APPLY_TC_GEN_ENV\n", "_tc_env" },
	{ PROGRAM_APPLY_TC_GEN_VECTOR, "#define APPLY_TC_GEN_VECTOR\n", "_tc_vec" },
	{ PROGRAM_APPLY_TC_GEN_REFLECTION, "#define APPLY_TC_GEN_REFLECTION\n", "_tc_refl" },
	{ PROGRAM_APPLY_TC_GEN_FOG, "#define APPLY_TC_GEN_FOG\n", "_tc_fog" },

	{ PROGRAM_APPLY_RGB_GEN_CONST, "#define APPLY_RGB_CONST\n", "_cc" },
	{ PROGRAM_APPLY_RGB_GEN_VERTEX, "#define APPLY_RGB_VERTEX\n", "_cv" },
	{ PROGRAM_APPLY_RGB_GEN_ONE_MINUS_VERTEX, "#define APPLY_RGB_ONE_MINUS_VERTEX\n", "_c1-v" },

	{ PROGRAM_APPLY_ALPHA_GEN_CONST, "#define APPLY_ALPHA_CONST\n", "_ac" },
	{ PROGRAM_APPLY_ALPHA_GEN_VERTEX, "#define APPLY_ALPHA_VERTEX\n", "_av" },
	{ PROGRAM_APPLY_ALPHA_GEN_ONE_MINUS_VERTEX, "#define APPLY_ALPHA_ONE_MINUS_VERTEX\n", "_a1-v" },

	{ PROGRAM_APPLY_OVERBRIGHT_SCALING, "#define APPLY_OVERBRIGHT_SCALING\n", "_os" },

	{ PROGRAM_APPLY_FOG, "#define APPLY_FOG\n", "_fog" },
	{ PROGRAM_APPLY_FOG2, "#define APPLY_FOG2\n", "_2" },
	{ PROGRAM_APPLY_COLOR_FOG, "#define APPLY_COLOR_FOG\n", "_cfog" },
	{ PROGRAM_APPLY_COLOR_FOG_ALPHA, "#define APPLY_COLOR_FOG_ALPHA\n", "_afog" },

	{ 0, NULL, NULL }
};

static const glsl_feature_t * const glsl_programtypes_features[] =
{
	// PROGRAM_TYPE_NONE
	NULL,
	// PROGRAM_TYPE_MATERIAL
	glsl_features_materal,
	// PROGRAM_TYPE_DISTORTION
	glsl_features_distortion,
	// PROGRAM_TYPE_SHADOWMAP
	glsl_features_shadowmap,
	// PROGRAM_TYPE_OUTLINE
	glsl_features_outline,
	// PROGRAM_TYPE_TURBULENCE
	glsl_features_standard,
	// PROGRAM_TYPE_DYNAMIC_LIGHTS
	glsl_features_dynamiclights,
	// PROGRAM_TYPE_Q3A_SHADER
	glsl_features_q3a
};

// ======================================================================================

#ifndef STR_HELPER
#define STR_HELPER( s )					# s
#define STR_TOSTR( x )					STR_HELPER( x )
#endif

#define MYHALFTYPES "" \
"#if !defined(__GLSL_CG_DATA_TYPES)\n" \
"#define myhalf float\n" \
"#define myhalf2 vec2\n" \
"#define myhalf3 vec3\n" \
"#define myhalf4 vec4\n" \
"#else\n" \
"#define myhalf half\n" \
"#define myhalf2 half2\n" \
"#define myhalf3 half3\n" \
"#define myhalf4 half4\n" \
"#endif\n"

static const char *r_defaultGLSLProgram =
"// " APPLICATION " GLSL shader\n"
"\n"
MYHALFTYPES
"\n"
"varying vec2 TexCoord;\n"
"#ifdef APPLY_LIGHTSTYLE0\n"
"varying vec4 LightmapTexCoord01;\n"
"#ifdef APPLY_LIGHTSTYLE2\n"
"varying vec4 LightmapTexCoord23;\n"
"#endif\n"
"#endif\n"
"\n"
"#if defined(APPLY_SPECULAR) || defined(APPLY_OFFSETMAPPING) || defined(APPLY_RELIEFMAPPING)\n"
"varying vec3 EyeVector;\n"
"#endif\n"
"\n"
"#ifdef APPLY_DIRECTIONAL_LIGHT\n"
"varying vec3 LightVector;\n"
"#endif\n"
"\n"
"varying mat3 strMatrix; // directions of S/T/R texcoords (tangent, binormal, normal)\n"
"\n"
"#ifdef VERTEX_SHADER\n"
"// Vertex shader\n"
"\n"
"uniform vec3 EyeOrigin;\n"
"\n"
"#ifdef APPLY_DIRECTIONAL_LIGHT\n"
"uniform vec3 LightDir;\n"
"#endif\n"
"\n"
"#ifdef APPLY_FOG\n"
"uniform vec4 FogPlane;\n"
"#endif\n"
"\n"
"#ifdef APPLY_OVERBRIGHT_SCALING\n"
"uniform myhalf OverbrightScale;\n"
"#endif\n"
"\n"
"uniform myhalf4 ConstColor;\n"
"\n"
"void main()\n"
"{\n"
"\n"
"#if defined(APPLY_RGB_CONST) && defined(APPLY_ALPHA_CONST)\n"
"myhalf4 VertexColor = ConstColor;\n"
"#else\n"
"myhalf4 VertexColor = myhalf4(gl_Color);\n"
"\n"
"#if defined(APPLY_RGB_CONST)\n"
"VertexColor.rgb = myhalf3(ConstColor.r, ConstColor.g, ConstColor.b);\n"
"#elif defined(APPLY_RGB_VERTEX)\n"
"#if defined(APPLY_OVERBRIGHT_SCALING)\n"
"VertexColor.rgb = myhalf3(gl_Color.r * OverbrightScale, gl_Color.g * OverbrightScale, gl_Color.b * OverbrightScale);\n"
"#else\n"
"//VertexColor.rgb = myhalf3(gl_Color.r, gl_Color.g, gl_Color.b);\n"
"#endif\n"
"#elif defined(APPLY_RGB_ONE_MINUS_VERTEX)\n"
"#if defined(APPLY_OVERBRIGHT_SCALING)\n"
"VertexColor.rgb = myhalf3(1.0 - gl_Color.r * OverbrightScale, 1.0 - gl_Color.g * OverbrightScale, 1.0 - gl_Color.b * OverbrightScale);\n"
"#else\n"
"VertexColor.rgb = myhalf3(1.0 - gl_Color.r, 1.0 - gl_Color.g, 1.0 - gl_Color.b);\n"
"#endif\n"
"#endif\n"
"\n"
"#if defined(APPLY_ALPHA_CONST)\n"
"VertexColor.a = ConstColor.a;\n"
"#elif defined(APPLY_ALPHA_VERTEX)\n"
"//VertexColor.a = myhalf(gl_Color.a);\n"
"#elif defined(APPLY_ALPHA_ONE_MINUS_VERTEX)\n"
"VertexColor.a = myhalf(1.0 - gl_Color.a);\n"
"#endif\n"
"\n"
"#endif\n"
"\n"
"gl_FrontColor = vec4(VertexColor);\n"
"\n"
"TexCoord = vec2(gl_TextureMatrix[0] * gl_MultiTexCoord0);\n"
"\n"
"#ifdef APPLY_LIGHTSTYLE0\n"
"LightmapTexCoord01.st = gl_MultiTexCoord4.st;\n"
"#ifdef APPLY_LIGHTSTYLE1\n"
"LightmapTexCoord01.pq = gl_MultiTexCoord5.st;\n"
"#ifdef APPLY_LIGHTSTYLE2\n"
"LightmapTexCoord23.st = gl_MultiTexCoord6.st;\n"
"#ifdef APPLY_LIGHTSTYLE3\n"
"LightmapTexCoord23.pq = gl_MultiTexCoord7.st;\n"
"#endif\n"
"#endif\n"
"#endif\n"
"#endif\n"
"\n"
"strMatrix[0] = gl_MultiTexCoord1.xyz;\n"
"strMatrix[2] = gl_Normal.xyz;\n"
"strMatrix[1] = gl_MultiTexCoord1.w * cross (strMatrix[2], strMatrix[0]);\n"
"\n"
"#if defined(APPLY_SPECULAR) || defined(APPLY_OFFSETMAPPING) || defined(APPLY_RELIEFMAPPING)\n"
"vec3 EyeVectorWorld = EyeOrigin - gl_Vertex.xyz;\n"
"EyeVector = EyeVectorWorld * strMatrix;\n"
"#endif\n"
"\n"
"#ifdef APPLY_DIRECTIONAL_LIGHT\n"
"LightVector = LightDir * strMatrix;\n"
"#endif\n"
"\n"
"gl_Position = ftransform ();\n"
"#ifdef APPLY_CLIPPING\n"
"#ifdef __GLSL_CG_DATA_TYPES\n"
"gl_ClipVertex = gl_ModelViewMatrix * gl_Vertex;\n"
"#endif\n"
"#endif\n"
"\n"
"#ifdef APPLY_FOG\n"
"gl_FogFragCoord = dot(gl_Vertex.xyz, FogPlane.xyz) - FogPlane.w;\n"
"#endif\n"
"\n"
"}\n"
"\n"
"#endif // VERTEX_SHADER\n"
"\n"
"\n"
"#ifdef FRAGMENT_SHADER\n"
"// Fragment shader\n"
"\n"
"#ifdef APPLY_LIGHTSTYLE0\n"
"uniform sampler2D LightmapTexture0;\n"
"uniform float DeluxemapOffset0; // s-offset for LightmapTexCoord\n"
"uniform myhalf3 lsColor0; // lightstyle color\n"
"\n"
"#ifdef APPLY_LIGHTSTYLE1\n"
"uniform sampler2D LightmapTexture1;\n"
"uniform float DeluxemapOffset1;\n"
"uniform myhalf3 lsColor1;\n"
"\n"
"#ifdef APPLY_LIGHTSTYLE2\n"
"uniform sampler2D LightmapTexture2;\n"
"uniform float DeluxemapOffset2;\n"
"uniform myhalf3 lsColor2;\n"
"\n"
"#ifdef APPLY_LIGHTSTYLE3\n"
"uniform sampler2D LightmapTexture3;\n"
"uniform float DeluxemapOffset3;\n"
"uniform myhalf3 lsColor3;\n"
"\n"
"#endif\n"
"#endif\n"
"#endif\n"
"#endif\n"
"\n"
"uniform sampler2D BaseTexture;\n"
"uniform sampler2D NormalmapTexture;\n"
"uniform sampler2D GlossTexture;\n"
"#ifdef APPLY_DECAL\n"
"uniform sampler2D DecalTexture;\n"
"#endif\n"
"\n"
"#if defined(APPLY_OFFSETMAPPING) || defined(APPLY_RELIEFMAPPING)\n"
"uniform float OffsetMappingScale;\n"
"#endif\n"
"\n"
"uniform myhalf3 LightAmbient;\n"
"#ifdef APPLY_DIRECTIONAL_LIGHT\n"
"uniform myhalf3 LightDiffuse;\n"
"#endif\n"
"\n"
"uniform myhalf GlossIntensity; // gloss scaling factor\n"
"uniform myhalf GlossExponent; // gloss exponent factor\n"
"\n"
"#if defined(APPLY_OFFSETMAPPING) || defined(APPLY_RELIEFMAPPING)\n"
"// The following reliefmapping and offsetmapping routine was taken from DarkPlaces\n"
"// The credit goes to LordHavoc (as always)\n"
"vec2 OffsetMapping(vec2 TexCoord)\n"
"{\n"
"#ifdef APPLY_RELIEFMAPPING\n"
"// 14 sample relief mapping: linear search and then binary search\n"
"// this basically steps forward a small amount repeatedly until it finds\n"
"// itself inside solid, then jitters forward and back using decreasing\n"
"// amounts to find the impact\n"
"//vec3 OffsetVector = vec3(EyeVector.xy * ((1.0 / EyeVector.z) * OffsetMappingScale) * vec2(-1, 1), -1);\n"
"//vec3 OffsetVector = vec3(normalize(EyeVector.xy) * OffsetMappingScale * vec2(-1, 1), -1);\n"
"vec3 OffsetVector = vec3(normalize(EyeVector).xy * OffsetMappingScale * vec2(-1, 1), -1);\n"
"vec3 RT = vec3(TexCoord, 1);\n"
"OffsetVector *= 0.1;\n"
"RT += OffsetVector *  step(texture2D(NormalmapTexture, RT.xy).a, RT.z);\n"
"RT += OffsetVector *  step(texture2D(NormalmapTexture, RT.xy).a, RT.z);\n"
"RT += OffsetVector *  step(texture2D(NormalmapTexture, RT.xy).a, RT.z);\n"
"RT += OffsetVector *  step(texture2D(NormalmapTexture, RT.xy).a, RT.z);\n"
"RT += OffsetVector *  step(texture2D(NormalmapTexture, RT.xy).a, RT.z);\n"
"RT += OffsetVector *  step(texture2D(NormalmapTexture, RT.xy).a, RT.z);\n"
"RT += OffsetVector *  step(texture2D(NormalmapTexture, RT.xy).a, RT.z);\n"
"RT += OffsetVector *  step(texture2D(NormalmapTexture, RT.xy).a, RT.z);\n"
"RT += OffsetVector *  step(texture2D(NormalmapTexture, RT.xy).a, RT.z);\n"
"RT += OffsetVector * (step(texture2D(NormalmapTexture, RT.xy).a, RT.z)          - 0.5);\n"
"RT += OffsetVector * (step(texture2D(NormalmapTexture, RT.xy).a, RT.z) * 0.5    - 0.25);\n"
"RT += OffsetVector * (step(texture2D(NormalmapTexture, RT.xy).a, RT.z) * 0.25   - 0.125);\n"
"RT += OffsetVector * (step(texture2D(NormalmapTexture, RT.xy).a, RT.z) * 0.125  - 0.0625);\n"
"RT += OffsetVector * (step(texture2D(NormalmapTexture, RT.xy).a, RT.z) * 0.0625 - 0.03125);\n"
"return RT.xy;\n"
"#else\n"
"// 2 sample offset mapping (only 2 samples because of ATI Radeon 9500-9800/X300 limits)\n"
"// this basically moves forward the full distance, and then backs up based\n"
"// on height of samples\n"
"//vec2 OffsetVector = vec2(EyeVector.xy * ((1.0 / EyeVector.z) * OffsetMappingScale) * vec2(-1, 1));\n"
"//vec2 OffsetVector = vec2(normalize(EyeVector.xy) * OffsetMappingScale * vec2(-1, 1));\n"
"vec2 OffsetVector = vec2(normalize(EyeVector).xy * OffsetMappingScale * vec2(-1, 1));\n"
"TexCoord += OffsetVector;\n"
"OffsetVector *= 0.5;\n"
"TexCoord -= OffsetVector * texture2D(NormalmapTexture, TexCoord).a;\n"
"TexCoord -= OffsetVector * texture2D(NormalmapTexture, TexCoord).a;\n"
"return TexCoord;\n"
"#endif\n"
"}\n"
"#endif\n"
"\n"
"#if defined(APPLY_FOG) && defined(APPLY_FOG2)\n"
"uniform float EyeFogDist;\n"
"#endif\n"
"\n"
"void main()\n"
"{\n"
"#if defined(APPLY_OFFSETMAPPING) || defined(APPLY_RELIEFMAPPING)\n"
"// apply offsetmapping\n"
"vec2 TexCoordOffset = OffsetMapping(TexCoord);\n"
"#define TexCoord TexCoordOffset\n"
"#endif\n"
"myhalf3 surfaceNormal;\n"
"myhalf3 diffuseNormalModelspace;\n"
"myhalf3 diffuseNormal = myhalf3 (0.0, 0.0, -1.0);\n"
"float diffuseProduct;\n"
"#ifdef APPLY_CELLSHADING\n"
"int lightcell;\n"
"float diffuseProductPositive;\n"
"float diffuseProductNegative;\n"
"float hardShadow;\n"
"#endif\n"
"\n"
"myhalf3 weightedDiffuseNormal;\n"
"myhalf3 specularNormal;\n"
"float specularProduct;\n"
"\n"
"#if !defined(APPLY_DIRECTIONAL_LIGHT) && !defined(APPLY_LIGHTSTYLE0)\n"
"myhalf4 color = myhalf4 (1.0, 1.0, 1.0, 1.0);\n"
"#else\n"
"myhalf4 color = myhalf4 (0.0, 0.0, 0.0, 1.0);\n"
"#endif\n"
"\n"
"// get the surface normal\n"
"surfaceNormal = normalize (myhalf3 (texture2D (NormalmapTexture, TexCoord)) - myhalf3 (0.5));\n"
"\n"
"#ifdef APPLY_DIRECTIONAL_LIGHT\n"
"diffuseNormal = myhalf3 (LightVector);\n"
"weightedDiffuseNormal = diffuseNormal;\n"
"\n"
"#ifdef APPLY_CELLSHADING\n"
"hardShadow = 0.0;\n"
"#ifdef APPLY_HALFLAMBERT\n"
"diffuseProduct = float (dot (surfaceNormal, diffuseNormal));\n"
"diffuseProductPositive = float ( clamp(diffuseProduct, 0.0, 1.0) * 0.5 + 0.5 );\n"
"diffuseProductPositive *= diffuseProductPositive;\n"
"diffuseProductNegative = float ( clamp(diffuseProduct, -1.0, 0.0) * 0.5 - 0.5 );\n"
"diffuseProductNegative *= diffuseProductNegative;\n"
"diffuseProductNegative -= 0.25;\n"
"diffuseProduct = diffuseProductPositive;\n"
"#else\n"
"diffuseProduct = float (dot (surfaceNormal, diffuseNormal));\n"
"diffuseProductPositive = max (diffuseProduct, 0.0);\n"
"diffuseProductNegative = (-min (diffuseProduct, 0.0) - 0.3);\n"
"#endif\n"
"\n"
"\n"
"// smooth the hard shadow edge\n"
"lightcell = int(max(diffuseProduct + 0.1, 0.0) * 2.0);\n"
"hardShadow += float(lightcell);\n"
"\n"
"lightcell = int(max(diffuseProduct + 0.055, 0.0) * 2.0);\n"
"hardShadow += float(lightcell);\n"
"\n"
"lightcell = int(diffuseProductPositive * 2.0);\n"
"hardShadow += float(lightcell);\n"
"\n"
"color.rgb += myhalf(0.6 + hardShadow * 0.3333333333 * 0.27 + diffuseProductPositive * 0.14);\n"
"\n"
"// backlight\n"
"lightcell = int (diffuseProductNegative * 2.0);\n"
"color.rgb += myhalf (float(lightcell) * 0.085 + diffuseProductNegative * 0.085);\n"
"#else\n"
"#ifdef APPLY_HALFLAMBERT\n"
"diffuseProduct = float ( clamp(dot (surfaceNormal, diffuseNormal), 0.0, 1.0) * 0.5 + 0.5 );\n"
"diffuseProduct *= diffuseProduct;\n"
"#else\n"
"diffuseProduct = float (dot (surfaceNormal, diffuseNormal));\n"
"#endif\n"
"\n"
"#ifdef APPLY_DIRECTIONAL_LIGHT_MIX\n"
"color.rgb += (LightDiffuse.rgb * 0.6 + gl_Color.rgb * 0.4) * myhalf(max (diffuseProduct, 0.0)) + LightAmbient;\n"
"#else\n"
"color.rgb += LightDiffuse.rgb * myhalf(max (diffuseProduct, 0.0)) + LightAmbient;\n"
"#endif\n"
"#endif\n"
"\n"
"#endif\n"
"\n"
"// deluxemapping using light vectors in modelspace\n"
"\n"
"#ifdef APPLY_LIGHTSTYLE0\n"
"\n"
"// get light normal\n"
"diffuseNormalModelspace = myhalf3 (texture2D(LightmapTexture0, vec2(LightmapTexCoord01.s+DeluxemapOffset0,LightmapTexCoord01.t))) - myhalf3 (0.5);\n"
"diffuseNormal = normalize (myhalf3(dot(diffuseNormalModelspace,myhalf3(strMatrix[0])),dot(diffuseNormalModelspace,myhalf3(strMatrix[1])),dot(diffuseNormalModelspace,myhalf3(strMatrix[2]))));\n"
"// calculate directional shading\n"
"diffuseProduct = float (dot (surfaceNormal, diffuseNormal));\n"
"\n"
"#ifdef APPLY_FBLIGHTMAP\n"
"weightedDiffuseNormal = diffuseNormal;\n"
"// apply lightmap color\n"
"color.rgb += myhalf3 (max (diffuseProduct, 0.0) * myhalf3 (texture2D (LightmapTexture0, LightmapTexCoord01.st)));\n"
"#else\n"
"\n"
"#define NORMALIZE_DIFFUSE_NORMAL\n"
"\n"
"weightedDiffuseNormal = lsColor0 * diffuseNormal;\n"
"// apply lightmap color\n"
"color.rgb += lsColor0 * myhalf(max (diffuseProduct, 0.0)) * myhalf3 (texture2D (LightmapTexture0, LightmapTexCoord01.st));\n"
"#endif\n"
"\n"
"#ifdef APPLY_AMBIENT_COMPENSATION\n"
"// compensate for ambient lighting\n"
"color.rgb += myhalf((1.0 - max (diffuseProduct, 0.0))) * LightAmbient;\n"
"#endif\n"
"\n"
"#ifdef APPLY_LIGHTSTYLE1\n"
"diffuseNormalModelspace = myhalf3 (texture2D (LightmapTexture1, vec2(LightmapTexCoord01.p+DeluxemapOffset1,LightmapTexCoord01.q))) - myhalf3 (0.5);\n"
"diffuseNormal = normalize (myhalf3(dot(diffuseNormalModelspace,myhalf3(strMatrix[0])),dot(diffuseNormalModelspace,myhalf3(strMatrix[1])),dot(diffuseNormalModelspace,myhalf3(strMatrix[2]))));\n"
"diffuseProduct = float (dot (surfaceNormal, diffuseNormal));\n"
"weightedDiffuseNormal += lsColor1 * diffuseNormal;\n"
"color.rgb += lsColor1 * myhalf(max (diffuseProduct, 0.0)) * myhalf3 (texture2D (LightmapTexture1, LightmapTexCoord01.pq));\n"
"\n"
"#ifdef APPLY_LIGHTSTYLE2\n"
"diffuseNormalModelspace = myhalf3 (texture2D (LightmapTexture2, vec2(LightmapTexCoord23.s+DeluxemapOffset2,LightmapTexCoord23.t))) - myhalf3 (0.5);\n"
"diffuseNormal = normalize (myhalf3(dot(diffuseNormalModelspace,myhalf3(strMatrix[0])),dot(diffuseNormalModelspace,myhalf3(strMatrix[1])),dot(diffuseNormalModelspace,myhalf3(strMatrix[2]))));\n"
"diffuseProduct = float (dot (surfaceNormal, diffuseNormal));\n"
"weightedDiffuseNormal += lsColor2 * diffuseNormal;\n"
"color.rgb += lsColor2 * myhalf(max (diffuseProduct, 0.0)) * myhalf3 (texture2D (LightmapTexture2, LightmapTexCoord23.st));\n"
"\n"
"#ifdef APPLY_LIGHTSTYLE3\n"
"diffuseNormalModelspace = myhalf3 (texture2D (LightmapTexture3, vec2(LightmapTexCoord23.p+DeluxemapOffset3,LightmapTexCoord23.q))) - myhalf3 (0.5);;\n"
"diffuseNormal = normalize (myhalf3(dot(diffuseNormalModelspace,myhalf3(strMatrix[0])),dot(diffuseNormalModelspace,myhalf3(strMatrix[1])),dot(diffuseNormalModelspace,myhalf3(strMatrix[2]))));\n"
"diffuseProduct = float (dot (surfaceNormal, diffuseNormal));\n"
"weightedDiffuseNormal += lsColor3 * diffuseNormal;\n"
"color.rgb += lsColor3 * myhalf(max (diffuseProduct, 0.0)) * myhalf3 (texture2D (LightmapTexture3, LightmapTexCoord23.pq));\n"
"\n"
"#endif\n"
"#endif\n"
"#endif\n"
"#endif\n"
"\n"
"#ifdef APPLY_SPECULAR\n"
"\n"
"#ifdef NORMALIZE_DIFFUSE_NORMAL\n"
"specularNormal = normalize (myhalf3 (normalize (weightedDiffuseNormal)) + myhalf3 (normalize (EyeVector)));\n"
"#else\n"
"specularNormal = normalize (weightedDiffuseNormal + myhalf3 (normalize (EyeVector)));\n"
"#endif\n"
"\n"
"specularProduct = float (dot (surfaceNormal, specularNormal));\n"
"color.rgb += (myhalf3(texture2D(GlossTexture, TexCoord)) * GlossIntensity) * pow(myhalf(max(specularProduct, 0.0)), GlossExponent);\n"
"#endif\n"
"\n"
"#ifdef APPLY_BASETEX_ALPHA_ONLY\n"
"color = min(color, myhalf4(texture2D(BaseTexture, TexCoord).a));\n"
"#else\n"
"#ifdef APPLY_COLOR_CLAMPING\n"
"color = min(color, myhalf4(1.0));\n"
"#endif\n"
"color = color * myhalf4(texture2D(BaseTexture, TexCoord));\n"
"#endif\n"
"\n"
"#ifdef APPLY_DECAL\n"
"#ifdef APPLY_DECAL_ADD\n"
"myhalf3 decal = myhalf3(gl_Color.rgb) * myhalf3(texture2D(DecalTexture, TexCoord));\n"
"color.rgb = decal.rgb + color.rgb;\n"
"color.a = color.a * myhalf(gl_Color.a);\n"
"#else\n"
"myhalf4 decal = myhalf4(gl_Color.rgba);\n"
"if (decal.a > 0.0)\n"
"{\n"
"decal = decal * myhalf4(texture2D(DecalTexture, TexCoord));\n"
"color.rgb = mix(color.rgb, decal.rgb, decal.a);\n"
"}\n"
"#endif\n"
"#else\n"
"#if defined (APPLY_DIRECTIONAL_LIGHT) && defined(APPLY_DIRECTIONAL_LIGHT_MIX)\n"
"color = color;\n"
"#else\n"
"color = color * myhalf4(gl_Color.rgba);\n"
"#endif\n"
"#endif\n"
"\n"
"#ifdef APPLY_GRAYSCALE\n"
"myhalf grey = dot(color, myhalf3(0.299, 0.587, 0.114));\n"
"color.rgb = myhalf3(grey);\n"
"#endif\n"
"\n"
"#ifdef APPLY_FOG\n"
"if (gl_FogFragCoord < 0.1)\n"
"{\n"
"float frac = (gl_FragCoord.z / gl_FragCoord.w - gl_Fog.start) * gl_Fog.scale;\n"
"#ifdef APPLY_FOG2\n"
"frac *= gl_FogFragCoord / (gl_FogFragCoord - EyeFogDist);\n"
"#endif\n"
"frac = sqrt(clamp(frac, 0.0, 1.0));\n"
"\n"
"color.rgb = mix(color.rgb, myhalf3(gl_Fog.color.rgb), myhalf(frac));\n"
"}\n"
"#endif\n"
"\n"
"gl_FragColor = vec4(color);\n"
"}\n"
"\n"
"#endif // FRAGMENT_SHADER\n"
"\n";

static const char *r_defaultDistortionGLSLProgram =
"// " APPLICATION " GLSL shader\n"
"\n"
MYHALFTYPES
"\n"
"varying vec4 TexCoord;\n"
"varying vec4 ProjVector;\n"
"#ifdef APPLY_EYEDOT\n"
"varying vec3 EyeVector;\n"
"#endif\n"
"\n"
"#ifdef VERTEX_SHADER\n"
"// Vertex shader\n"
"\n"
"#ifdef APPLY_EYEDOT\n"
"uniform vec3 EyeOrigin;\n"
"uniform float FrontPlane;\n"
"#endif\n"
"\n"
"void main(void)\n"
"{\n"
"gl_FrontColor = gl_Color;\n"
"\n"
"mat4 textureMatrix;\n"
"\n"
"textureMatrix = gl_TextureMatrix[0];\n"
"TexCoord.st = vec2 (textureMatrix * gl_MultiTexCoord0);\n"
"\n"
"textureMatrix = gl_TextureMatrix[0];\n"
"textureMatrix[0] = -textureMatrix[0];\n"
"textureMatrix[1] = -textureMatrix[1];\n"
"TexCoord.pq = vec2 (textureMatrix * gl_MultiTexCoord0);\n"
"\n"
"#ifdef APPLY_EYEDOT\n"
"mat3 strMatrix;\n"
"strMatrix[0] = gl_MultiTexCoord1.xyz;\n"
"strMatrix[2] = gl_Normal.xyz;\n"
"strMatrix[1] = gl_MultiTexCoord1.w * cross (strMatrix[2], strMatrix[0]);\n"
"\n"
"vec3 EyeVectorWorld = (EyeOrigin - gl_Vertex.xyz) * FrontPlane;\n"
"EyeVector = EyeVectorWorld * strMatrix;\n"
"#endif\n"
"\n"
"gl_Position = ftransform();\n"
"ProjVector = gl_Position;\n"
"#ifdef APPLY_CLIPPING\n"
"#ifdef __GLSL_CG_DATA_TYPES\n"
"gl_ClipVertex = gl_ModelViewMatrix * gl_Vertex;\n"
"#endif\n"
"#endif\n"
"}\n"
"\n"
"#endif // VERTEX_SHADER\n"
"\n"
"\n"
"#ifdef FRAGMENT_SHADER\n"
"// Fragment shader\n"
"\n"
"#ifdef APPLY_DUDV\n"
"uniform sampler2D DuDvMapTexture;\n"
"#endif\n"
"\n"
"#ifdef APPLY_EYEDOT\n"
"uniform sampler2D NormalmapTexture;\n"
"#endif\n"
"uniform sampler2D ReflectionTexture;\n"
"uniform sampler2D RefractionTexture;\n"
"uniform float InvTextureWidth, InvTextureHeight;\n"
"\n"
"void main(void)\n"
"{\n"
"myhalf3 color;\n"
"\n"
"#ifdef APPLY_DUDV\n"
"vec3 displacement = vec3(texture2D(DuDvMapTexture, vec2(TexCoord.pq) * vec2(0.25)));\n"
"vec2 coord = vec2(TexCoord.st) + vec2(displacement) * vec2 (0.2);\n"
"\n"
"vec3 fdist = vec3 (normalize(vec3(texture2D(DuDvMapTexture, coord)) - vec3 (0.5))) * vec3(0.005);\n"
"#else\n"
"vec3 fdist = vec3(0.0);\n"
"#endif\n"
"\n"
"// get projective texcoords\n"
"float scale = float(1.0 / float(ProjVector.w));\n"
"float inv2NW = InvTextureWidth * 0.5;\n"
"float inv2NH = InvTextureHeight * 0.5;\n"
"vec2 projCoord = (vec2(ProjVector.xy) * scale + vec2 (1.0)) * vec2 (0.5) + vec2(fdist.xy);\n"
"projCoord.s = float (clamp (float(projCoord.s), inv2NW, 1.0 - inv2NW));\n"
"projCoord.t = float (clamp (float(projCoord.t), inv2NH, 1.0 - inv2NH));\n"
"\n"
"\n"
"myhalf3 refr = myhalf3(0.0);\n"
"myhalf3 refl = myhalf3(0.0);\n"
"\n"
"#ifdef APPLY_EYEDOT\n"
"// calculate dot product between the surface normal and eye vector\n"
"// great for simulating varying water translucency based on the view angle\n"
"myhalf3 surfaceNormal = normalize(myhalf3(texture2D(NormalmapTexture, coord)) - myhalf3 (0.5));\n"
"vec3 eyeNormal = normalize(myhalf3(EyeVector));\n"
"\n"
"float refrdot = float(dot(surfaceNormal, eyeNormal));\n"
"//refrdot = float (clamp (refrdot, 0.0, 1.0));\n"
"float refldot = 1.0 - refrdot;\n"
"// get refraction and reflection\n"
"\n"
"#ifdef APPLY_REFRACTION\n"
"refr = (myhalf3(texture2D(RefractionTexture, projCoord))) * refrdot;\n"
"#endif\n"
"#ifdef APPLY_REFLECTION\n"
"refl = (myhalf3(texture2D(ReflectionTexture, projCoord))) * refldot;\n"
"#endif\n"
"\n"
"#else\n"
"\n"
"#ifdef APPLY_REFRACTION\n"
"refr = (myhalf3(texture2D(RefractionTexture, projCoord)));\n"
"#endif\n"
"#ifdef APPLY_REFLECTION\n"
"refl = (myhalf3(texture2D(ReflectionTexture, projCoord)));\n"
"#endif\n"
"\n"
"#endif\n"
"\n"
"// add reflection and refraction\n"
"#ifdef APPLY_DISTORTION_ALPHA\n"
"color = myhalf3(gl_Color.rgb) + myhalf3(mix (refr, refl, float(gl_Color.a)));\n"
"#else\n"
"color = myhalf3(gl_Color.rgb) + refr + refl;\n"
"#endif\n"
"\n"
"#ifdef APPLY_GRAYSCALE\n"
"float grey = dot(color, myhalf3(0.299, 0.587, 0.114));\n"
"gl_FragColor = vec4(vec3(grey),1.0);\n"
"#else\n"
"gl_FragColor = vec4(vec3(color),1.0);\n"
"#endif\n"
"}\n"
"\n"
"#endif // FRAGMENT_SHADER\n"
"\n";

static const char *r_defaultShadowmapGLSLProgram =
"// " APPLICATION " GLSL shader\n"
"\n"
MYHALFTYPES
"\n"
"varying vec4 ProjVector;\n"
"varying float NormalDot;\n"
"\n"
"#ifdef VERTEX_SHADER\n"
"// Vertex shader\n"
"\n"
"uniform vec3 LightDir;\n"
"\n"
"void main(void)\n"
"{\n"
"gl_FrontColor = gl_Color;\n"
"\n"
"\n"
"mat4 textureMatrix;\n"
"\n"
"textureMatrix = gl_TextureMatrix[0];\n"
"\n"
"gl_Position = ftransform();\n"
"ProjVector = textureMatrix * gl_Vertex;\n"
"\n"
"NormalDot = dot(gl_Normal, LightDir);\n"
"}\n"
"\n"
"#endif // VERTEX_SHADER\n"
"\n"
"\n"
"#ifdef FRAGMENT_SHADER\n"
"// Fragment shader\n"
"\n"
"uniform myhalf3 LightAmbient;\n"
"\n"
"uniform float InvTextureWidth, InvTextureHeight;\n"
"uniform float ProjDistance;\n"
"uniform sampler2DShadow ShadowmapTexture;\n"
"\n"
"myhalf ShadowLookup(vec3 ShadowCoord, vec2 offset)\n"
"{\n"
"return myhalf(shadow2D(ShadowmapTexture, ShadowCoord + vec3(offset.s * InvTextureWidth, offset.t * InvTextureHeight, 0.0) ).r);\n"
"}\n"
"\n"
"void main(void)\n"
"{\n"
"myhalf color = myhalf(1.0);\n"
"\n"
"if (ProjVector.w <= 0.0 || ProjVector.w >= ProjDistance)\n"
"discard;\n"
"\n"
"float dtW = InvTextureWidth;\n"
"float dtH = InvTextureHeight;\n"
"\n"
"vec3 coord = vec3 (ProjVector.xyz / ProjVector.w);\n"
"coord = (coord + vec3 (1.0)) * vec3 (0.5);\n"
"coord.s = float (clamp (float(coord.s), dtW, 1.0 - dtW));\n"
"coord.t = float (clamp (float(coord.t), dtH, 1.0 - dtH));\n"
"coord.r = float (clamp (float(coord.r), 0.0, 1.0));\n"
"\n"
"myhalf shadow = 0.0;\n"
"\n"
"#if defined(APPLY_PCF2x2)\n"
"shadow = (ShadowLookup(coord, vec2(0.0, 0.0)) +\n"
"ShadowLookup(coord, vec2(0.0, 1.0)) +\n"
"ShadowLookup(coord, vec2(1.0, 1.0)) +\n"
"ShadowLookup(coord, vec2(1.0, 0.0))) * 0.25;\n"
"#elif defined(APPLY_PCF3x3)\n"
"shadow = (ShadowLookup(coord, vec2(0.0, 0.0)) +\n"
"ShadowLookup(coord, vec2(0.0, 1.0)) +\n"
"ShadowLookup(coord, vec2(1.0, 1.0)) +\n"
"ShadowLookup(coord, vec2(0.0, -1.0)) +\n"
"ShadowLookup(coord, vec2(-1.0, -1.0)) +\n"
"ShadowLookup(coord, vec2(-1.0, 0.0)) +\n"
"ShadowLookup(coord, vec2(1.0, -1.0)) +\n"
"ShadowLookup(coord, vec2(-1.0, 1.0))) * 0.11;\n"
"#elif defined(APPLY_PCF4x4)\n"
"// taken from GPU Gems: http://http.developer.nvidia.com/GPUGems/gpugems_ch11.html\n"
"vec2 offset = mod(floor(gl_FragCoord.xy), 2.0);\n"
"offset.y += offset.x;  // y ^= x in floating point\n"
"offset.y *= step(offset.y, 1.1);\n"
"\n"
"shadow = (ShadowLookup(coord, offset + vec2(-1.5, 0.5)) +\n"
"ShadowLookup(coord, offset + vec2(0.5, 0.5)) +\n"
"ShadowLookup(coord, offset + vec2(-1.5, -1.5)) +\n"
"ShadowLookup(coord, offset + vec2(0.5, -1.5))) * 0.25;\n"
"#else\n"
"shadow = ShadowLookup(coord, vec2(0.0));\n"
"#endif\n"
"\n"
"float attenuation = float (ProjVector.w) / ProjDistance;\n"
"myhalf compensation = myhalf(0.25) - max(LightAmbient.x, max(LightAmbient.y, LightAmbient.z))\n;"
"compensation = max (compensation, 0.0);\n"
"color = shadow + attenuation + compensation;\n"
"\n"
"gl_FragColor = vec4(vec3(color),1.0);\n"
"}\n"
"\n"
"#endif // FRAGMENT_SHADER\n"
"\n";

#ifdef HARDWARE_OUTLINES
static const char *r_defaultOutlineGLSLProgram =
"// " APPLICATION " GLSL shader\n"
"\n"
"\n"
"#ifdef VERTEX_SHADER\n"
"// Vertex shader\n"
"\n"
"uniform float OutlineHeight;\n"
"\n"
"void main(void)\n"
"{\n"
"gl_FrontColor = gl_Color;\n"
"\n"
"vec4 n = vec4(gl_Normal.xyz, 0.0);\n"
"vec4 v = vec4(gl_Vertex) + n * OutlineHeight;\n"
"\n"
"gl_Position = gl_ModelViewProjectionMatrix * v;\n"
"#ifdef APPLY_CLIPPING\n"
"#ifdef __GLSL_CG_DATA_TYPES\n"
"gl_ClipVertex = gl_ModelViewMatrix * v;\n"
"#endif\n"
"#endif\n"
"}\n"
"\n"
"#endif // VERTEX_SHADER\n"
"\n"
"\n"
"#ifdef FRAGMENT_SHADER\n"
"// Fragment shader\n"
"\n"
"uniform float OutlineCutOff;\n"
"\n"
"void main(void)\n"
"{\n"
"\n"
"#ifdef APPLY_OUTLINES_CUTOFF\n"
"if (OutlineCutOff > 0.0 && (gl_FragCoord.z / gl_FragCoord.w > OutlineCutOff))\n"
"discard;\n"
"#endif\n"
"\n"
"gl_FragColor = vec4 (gl_Color);\n"
"}\n"
"\n"
"#endif // FRAGMENT_SHADER\n"
"\n";
#endif

static const char *r_defaultTurbulenceProgram =
"// " APPLICATION " GLSL shader\n"
"\n"
MYHALFTYPES
"\n"
"#define M_TWOPI 6.28318530717958647692\n"
"\n"
"varying vec2 TexCoord;\n"
"\n"
"#ifdef VERTEX_SHADER\n"
"// Vertex shader\n"
"\n"
"uniform float TurbAmplitude, TurbPhase;\n"
"\n"
"void main(void)\n"
"{\n"
"gl_FrontColor = gl_Color;\n"
"\n"
"vec4 turb;\n"
"turb = vec4(gl_MultiTexCoord0);\n"
"turb.s += TurbAmplitude * sin( ((gl_MultiTexCoord0.t / 4.0 + TurbPhase)) * M_TWOPI );\n"
"turb.t += TurbAmplitude * sin( ((gl_MultiTexCoord0.s / 4.0 + TurbPhase)) * M_TWOPI );\n"
"TexCoord = vec2(gl_TextureMatrix[0] * turb);\n"
"\n"
"gl_Position = ftransform();\n"
"#ifdef APPLY_CLIPPING\n"
"#ifdef __GLSL_CG_DATA_TYPES\n"
"gl_ClipVertex = gl_ModelViewMatrix * gl_Vertex;\n"
"#endif\n"
"#endif\n"
"}\n"
"\n"
"#endif // VERTEX_SHADER\n"
"\n"
"\n"
"#ifdef FRAGMENT_SHADER\n"
"// Fragment shader\n"
"\n"
"uniform sampler2D BaseTexture;\n"
"\n"
"void main(void)\n"
"{\n"
"\n"
"myhalf4 color;\n"
"\n"
"color = myhalf4(gl_Color) * myhalf4(texture2D(BaseTexture, TexCoord));\n"
"\n"
"gl_FragColor = vec4(color);\n"
"}\n"
"\n"
"#endif // FRAGMENT_SHADER\n"
"\n";

static const char *r_defaultDynamicLightsProgram =
"// " APPLICATION " GLSL shader\n"
"\n"
MYHALFTYPES
"\n"
"\n"
"#ifdef MAX_DLIGHTS\n"
"varying vec3 DynamicLightSTR[MAX_DLIGHTS];\n"
"#endif\n"
"\n"
"#ifdef VERTEX_SHADER\n"
"// Vertex shader\n"
"\n"
"void main(void)\n"
"{\n"
"gl_FrontColor = gl_Color;\n"
"\n"
"gl_Position = ftransform ();\n"
"#ifdef APPLY_CLIPPING\n"
"#ifdef __GLSL_CG_DATA_TYPES\n"
"gl_ClipVertex = gl_ModelViewMatrix * gl_Vertex;\n"
"#endif\n"
"#endif\n"
"\n"
"#ifdef MAX_DLIGHTS\n"
"for (int i = 0; i < MAX_DLIGHTS; i++)\n"
"{\n"
"DynamicLightSTR[i] = vec3(gl_LightSource[i].spotCutoff*gl_Vertex.x-gl_LightSource[i].specular.x,\n"
"gl_LightSource[i].spotCutoff*gl_Vertex.y-gl_LightSource[i].specular.y,\n"
"gl_LightSource[i].spotCutoff*gl_Vertex.z-gl_LightSource[i].specular.z);\n"
"}\n"
"#endif\n"
"}\n"
"\n"
"#endif // VERTEX_SHADER\n"
"\n"
"\n"
"#ifdef FRAGMENT_SHADER\n"
"// Fragment shader\n"
"\n"
"void main(void)\n"
"{\n"
"\n"
"myhalf3 color = myhalf3(0.0);\n"
"\n"
"#ifdef MAX_DLIGHTS\n"
"vec3 DLightSTR;\n"
"myhalf IntensitySqr;\n"
"\n"
"for (int i = 0; i < MAX_DLIGHTS; i++)\n"
"{\n"
"DLightSTR = clamp(DynamicLightSTR[i], -1.0, 1.0);\n"
"IntensitySqr = clamp(1.0 - myhalf(length(DLightSTR)), 0.0, 1.0);\n"
"IntensitySqr = IntensitySqr * IntensitySqr * 2.0 * 0.85;\n"
"color.rgb += IntensitySqr * myhalf3(gl_LightSource[i].diffuse);\n"
"}\n"
"\n"
"#endif\n"
"\n"
"gl_FragColor = vec4(color.rgb, 1.0);\n"
"}\n"
"\n"
"#endif // FRAGMENT_SHADER\n"
"\n";

static const char *r_defaultQ3AShaderProgram =
"// " APPLICATION " GLSL shader\n"
"\n"
MYHALFTYPES
"\n"
"#ifdef APPLY_TC_GEN_REFLECTION\n"
"#define APPLY_CUBEMAP\n"
"\n"
"uniform vec3 EyeOrigin;\n"
"#endif\n"
"\n"
"#ifdef APPLY_TC_GEN_ENV\n"
"uniform vec3 EntDist;\n"
"#endif\n"
"\n"
"#ifdef APPLY_CUBEMAP\n"
"varying vec3 TexCoord;\n"
"#else\n"
"varying vec2 TexCoord;\n"
"#endif\n"
"\n"
"#ifdef VERTEX_SHADER\n"
"// Vertex shader\n"
"\n"
"uniform myhalf4 ConstColor;\n"
"\n"
"#ifdef APPLY_OVERBRIGHT_SCALING\n"
"uniform myhalf OverbrightScale;\n"
"#endif\n"
"\n"
"#if defined(APPLY_FOG)\n"
"uniform float EyeFogDist;\n"
"uniform vec4 EyePlane, FogPlane;\n"
"#endif\n"
"\n"
"void main(void)\n"
"{\n"
"#if defined(APPLY_TC_GEN_FOG)\n"
"myhalf4 VertexColor = myhalf4(gl_Fog.color.rgb, 1.0);\n"
"#elif defined(APPLY_RGB_CONST) && defined(APPLY_ALPHA_CONST)\n"
"myhalf4 VertexColor = ConstColor;\n"
"#else\n"
"myhalf4 VertexColor = myhalf4(gl_Color);\n"
"\n"
"#if defined(APPLY_RGB_CONST)\n"
"VertexColor.rgb = myhalf3(ConstColor.r, ConstColor.g, ConstColor.b);\n"
"#elif defined(APPLY_RGB_VERTEX)\n"
"#if defined(APPLY_OVERBRIGHT_SCALING)\n"
"VertexColor.rgb = myhalf3(gl_Color.r * OverbrightScale, gl_Color.g * OverbrightScale, gl_Color.b * OverbrightScale);\n"
"#else\n"
"//VertexColor.rgb = myhalf3(gl_Color.r, gl_Color.g, gl_Color.b);\n"
"#endif\n"
"#elif defined(APPLY_RGB_ONE_MINUS_VERTEX)\n"
"#if defined(APPLY_OVERBRIGHT_SCALING)\n"
"VertexColor.rgb = myhalf3(1.0 - gl_Color.r * OverbrightScale, 1.0 - gl_Color.g * OverbrightScale, 1.0 - gl_Color.b * OverbrightScale);\n"
"#else\n"
"VertexColor.rgb = myhalf3(1.0 - gl_Color.r, 1.0 - gl_Color.g, 1.0 - gl_Color.b);\n"
"#endif\n"
"#endif\n"
"\n"
"#if defined(APPLY_ALPHA_CONST)\n"
"VertexColor.a = ConstColor.a;\n"
"#elif defined(APPLY_ALPHA_VERTEX)\n"
"//VertexColor.a = myhalf(gl_Color.a);\n"
"#elif defined(APPLY_ALPHA_ONE_MINUS_VERTEX)\n"
"VertexColor.a = myhalf(1.0 - gl_Color.a);\n"
"#endif\n"
"\n"
"#endif\n"
"\n"
"#ifdef APPLY_FOG\n"
"float FDist = dot(gl_Vertex.xyz, EyePlane.xyz) - EyePlane.w;\n"
"float FVdist = dot(gl_Vertex.xyz, FogPlane.xyz) - FogPlane.w;\n"
"\n"
"#ifdef APPLY_FOG2\n"
"// camera is outside the fog brush\n"
"float FogDistScale = FVdist / (FVdist - EyeFogDist);\n"
"#endif\n"
"\n"
"#ifdef APPLY_COLOR_FOG\n"
"#ifdef APPLY_FOG2\n"
"// camera is outside the fog brush\n"
"float FogDist = FogDistScale;\n"
"#else\n"
"float FogDist = FDist;\n"
"#endif\n"
"\n"
"#ifdef APPLY_COLOR_FOG_ALPHA\n"
"VertexColor.a *= myhalf(clamp(1.0 - FogDist * gl_Fog.scale, 0.0, 1.0));\n"
"#else\n"
"VertexColor.rgb *= myhalf(clamp(1.0 - FogDist * gl_Fog.scale, 0.0, 1.0));\n"
"#endif\n"
"\n"
"#endif\n"
"#endif\n"
"gl_FrontColor = vec4(VertexColor);\n"
"\n"
"#if defined(APPLY_TC_GEN_ENV)\n"
"vec3 Projection;\n"
"\n"
"Projection = EntDist - gl_Vertex.xyz;\n"
"Projection = normalize(Projection);\n"
"\n"
"float Depth = dot(gl_Normal.xyz, Projection) * 2.0;\n"
"TexCoord = vec2(0.5 + (gl_Normal.y * Depth - Projection.y) * 0.5, 0.5 - (gl_Normal.z * Depth - Projection.z) * 0.5);\n"
"#elif defined(APPLY_TC_GEN_VECTOR)\n"
"TexCoord = vec2(gl_TextureMatrix[0] * vec4(dot(gl_Vertex,gl_ObjectPlaneS[0]), dot(gl_Vertex,gl_ObjectPlaneT[0]), dot(gl_Vertex,gl_ObjectPlaneR[0]), dot(gl_Vertex,gl_ObjectPlaneQ[0])));\n"
"#elif defined(APPLY_TC_GEN_REFLECTION)\n"
"TexCoord = vec3(gl_TextureMatrix[0] * vec4(reflect(normalize(gl_Vertex.xyz - EyeOrigin), gl_Normal.xyz), 0));\n"
"#elif defined(APPLY_TC_GEN_FOG)\n"
"\n"
"float FogS = FDist;\n"
"float FogT = -FVdist;\n"
"\n"
"#ifdef APPLY_FOG2\n"
"// camera is outside the fog brush\n"
"FogS *= (1.0 - step(0.0, FVdist)) * FogDistScale;\n"
"#endif\n"
"\n"
"TexCoord = vec2(FogS * gl_Fog.scale, FogT * gl_Fog.scale + 1.5/" STR_TOSTR( FOG_TEXTURE_HEIGHT ) ".0);\n"
"#else\n"
"TexCoord = vec2(gl_TextureMatrix[0] * gl_MultiTexCoord0);\n"
"#endif\n"
"\n"
"gl_Position = ftransform();\n"
"#ifdef APPLY_CLIPPING\n"
"#ifdef __GLSL_CG_DATA_TYPES\n"
"gl_ClipVertex = gl_ModelViewMatrix * gl_Vertex;\n"
"#endif\n"
"#endif\n"
"}\n"
"\n"
"#endif // VERTEX_SHADER\n"
"\n"
"\n"
"#ifdef FRAGMENT_SHADER\n"
"// Fragment shader\n"
"\n"
"#ifdef APPLY_CUBEMAP\n"
"uniform samplerCube BaseTexture;\n"
"#else\n"
"uniform sampler2D BaseTexture;\n"
"#endif\n"
"\n"
"void main(void)\n"
"{\n"
"\n"
"#ifdef APPLY_CUBEMAP\n"
"gl_FragColor = gl_Color * textureCube(BaseTexture, TexCoord);\n"
"#else\n"
"gl_FragColor = gl_Color * texture2D(BaseTexture, TexCoord);\n"
"#endif\n"
"}\n"
"\n"
"#endif // FRAGMENT_SHADER\n"
"\n";

/*
================
R_CommonProgramFeatures
================
*/
int R_CommonProgramFeatures( void )
{
	int features;

	features = 0;

	return features;
}

/*
================
R_ProgramFeatures2Defines

Return an array of strings for bitflags
================
*/
static const char **R_ProgramFeatures2Defines( const glsl_feature_t *type_features, int features, char *name, size_t size )
{
	int i, p;
	static const char *headers[MAX_DEFINES_FEATURES+1]; // +1 for NULL safe-guard

	for( i = 0, p = 0; type_features[i].bit; i++ )
	{
		if( features & type_features[i].bit )
		{
			headers[p++] = type_features[i].define;
			if( name )
				Q_strncatz( name, type_features[i].suffix, size );

			if( p == MAX_DEFINES_FEATURES )
				break;
		}
	}

	if( p )
	{
		headers[p] = NULL;
		return headers;
	}

	return NULL;
}

/*
================
R_GLSLProgramHashKey
================
*/
static unsigned int R_GLSLProgramHashKey( const char *name, int type )
{
	int i, c;
	unsigned hashval = 0;

	for( i = 0; name[i]; i++ )
	{
		c = tolower( name[i] );
		if( c == '\\' )
			c = '/';
		hashval += (tolower( c ) - ' ') * (i + 119);
	}

	hashval = (hashval ^ (hashval >> 10) ^ (hashval >> 20));
	hashval = (hashval ^ type) & ( GLSL_PROGRAMS_HASH_SIZE - 1 );

	return hashval;
}

/*
================
R_RegisterGLSLProgram
================
*/
int R_RegisterGLSLProgram( int type, const char *name, const char *string, int features )
{
	int i;
	int linked, body, error = 0;
	unsigned int hash_key;
	unsigned int minfeatures;
	glsl_program_t *program, *parent;
	char fullName[MAX_QPATH];
	const char **header;
	const char *vertexShaderStrings[MAX_DEFINES_FEATURES+2];
	const char *fragmentShaderStrings[MAX_DEFINES_FEATURES+2];

	if( !glConfig.ext.GLSL )
		return 0; // fail early

	if( type <= PROGRAM_TYPE_NONE || type >= PROGRAM_TYPE_MAXTYPE )
		return 0;

	parent = NULL;
	minfeatures = features;
	hash_key = R_GLSLProgramHashKey( name, type );
	for( program = r_glslprograms_hash[hash_key]; program; program = program->hash_next )
	{
		if( !strcmp( program->name, name ) )
		{
			if ( program->features == features )
				return ( (program - r_glslprograms) + 1 );

			if( !parent || ((unsigned)program->features < minfeatures) )
			{
				parent = program;
				minfeatures = program->features;
			}
		}
	}

	if( r_numglslprograms == MAX_GLSL_PROGRAMS )
	{
		Com_Printf( S_COLOR_YELLOW "R_RegisterGLSLProgram: GLSL programs limit exceeded\n" );
		return 0;
	}

	if( !string )
	{
		if( parent )
			string = parent->string;
		else
			string = r_defaultGLSLProgram;
	}

	program = r_glslprograms + r_numglslprograms++;
	program->object = qglCreateProgramObjectARB();
	if( !program->object )
	{
		error = 1;
		goto done;
	}

	body = 1;
	Q_strncpyz( fullName, name, sizeof( fullName ) );
	header = R_ProgramFeatures2Defines( glsl_programtypes_features[type], features, fullName, sizeof( fullName ) );

	Com_DPrintf( "Registering GLSL program %s\n", fullName );

	if( header )
		for( ; header[body-1] && *header[body-1]; body++ );

	vertexShaderStrings[0] = "#define VERTEX_SHADER\n";
	for( i = 1; i < body; i++ )
		vertexShaderStrings[i] = ( char * )header[i-1];
	// find vertex shader header
	vertexShaderStrings[body] = string;

	fragmentShaderStrings[0] = "#define FRAGMENT_SHADER\n";
	for( i = 1; i < body; i++ )
		fragmentShaderStrings[i] = ( char * )header[i-1];
	// find fragment shader header
	fragmentShaderStrings[body] = string;

	// compile vertex shader
	program->vertexShader = R_CompileGLSLShader( program->object, fullName, "vertex", GL_VERTEX_SHADER_ARB, vertexShaderStrings, body+1 );
	if( !program->vertexShader )
	{
		error = 1;
		goto done;
	}

	// compile fragment shader
	program->fragmentShader = R_CompileGLSLShader( program->object, fullName, "fragment", GL_FRAGMENT_SHADER_ARB, fragmentShaderStrings, body+1 );
	if( !program->fragmentShader )
	{
		error = 1;
		goto done;
	}

	// link
	qglLinkProgramARB( program->object );
	qglGetObjectParameterivARB( program->object, GL_OBJECT_LINK_STATUS_ARB, &linked );
	if( !linked )
	{
		char log[8192];

		qglGetInfoLogARB( program->object, sizeof( log ), NULL, log );
		log[sizeof( log ) - 1] = 0;

		if( log[0] )
			Com_Printf( S_COLOR_YELLOW "Failed to compile link object for program %s:\n%s\n", fullName, log );

		error = 1;
		goto done;
	}

done:
	if( error )
		R_DeleteGLSLProgram( program );

	program->type = type;
	program->features = features;
	program->name = R_GLSLProgramCopyString( name );
	program->string = string;

	if( !program->hash_next )
	{
		program->hash_next = r_glslprograms_hash[hash_key];
		r_glslprograms_hash[hash_key] = program;
	}

	if( program->object )
	{
		qglUseProgramObjectARB( program->object );
		R_GetProgramUniformLocations( program );
		qglUseProgramObjectARB( 0 );
	}

	return ( program - r_glslprograms ) + 1;
}

/*
================
R_GetProgramObject
================
*/
int R_GetProgramObject( int elem )
{
	return r_glslprograms[elem - 1].object;
}

/*
================
R_ProgramList_f
================
*/
void R_ProgramList_f( void )
{
	int i;
	glsl_program_t *program;
	char fullName[MAX_QPATH];
	const char **header;

	Com_Printf( "------------------\n" );
	for( i = 0, program = r_glslprograms; i < MAX_GLSL_PROGRAMS; i++, program++ )
	{
		if( !program->name )
			break;

		Q_strncpyz( fullName, program->name, sizeof( fullName ) );
		header = R_ProgramFeatures2Defines( glsl_programtypes_features[program->type], program->features, fullName, sizeof( fullName ) );

		Com_Printf( " %3i %s\n", i+1, fullName );
	}
	Com_Printf( "%i programs total\n", i );
}

/*
================
R_ProgramDump_f
================
*/
#define DUMP_PROGRAM(p) { int file; if( FS_FOpenFile( va("programs/%s.glsl", #p), &file, FS_WRITE ) != -1 ) { FS_Print( file, r_##p ); FS_FCloseFile( file ); } }
void R_ProgramDump_f( void )
{
	DUMP_PROGRAM( defaultGLSLProgram );
	DUMP_PROGRAM( defaultDistortionGLSLProgram );
	DUMP_PROGRAM( defaultShadowmapGLSLProgram );
#ifdef HARDWARE_OUTLINES
	DUMP_PROGRAM( defaultOutlineGLSLProgram );
#endif
	DUMP_PROGRAM( defaultTurbulenceProgram );
	DUMP_PROGRAM( defaultDynamicLightsProgram );
	DUMP_PROGRAM( defaultQ3AShaderProgram );
}
#undef DUMP_PROGRAM

/*
================
R_UpdateProgramUniforms
================
*/
void R_UpdateProgramUniforms( int elem, const vec3_t eyeOrigin,
							 const vec3_t lightOrigin, const vec3_t lightDir, const vec4_t ambient, const vec4_t diffuse,
							 const superLightStyle_t *superLightStyle, qboolean frontPlane, int TexWidth, int TexHeight,
							 float projDistance, float offsetmappingScale, float glossExponent, 
							 const qbyte *constColor, int overbrightBits )
{
	glsl_program_t *program = r_glslprograms + elem - 1;
	int overbrights = 1 << max(0, r_overbrightbits->integer);

	if( program->locEyeOrigin >= 0 && eyeOrigin )
		qglUniform3fARB( program->locEyeOrigin, eyeOrigin[0], eyeOrigin[1], eyeOrigin[2] );

	if( program->locLightOrigin >= 0 && lightOrigin )
		qglUniform3fARB( program->locLightOrigin, lightOrigin[0], lightOrigin[1], lightOrigin[2] );
	if( program->locLightDir >= 0 && lightDir )
		qglUniform3fARB( program->locLightDir, lightDir[0], lightDir[1], lightDir[2] );

	if( program->locLightAmbient >= 0 && ambient )
		qglUniform3fARB( program->locLightAmbient, ambient[0], ambient[1], ambient[2] );
	if( program->locLightDiffuse >= 0 && diffuse )
		qglUniform3fARB( program->locLightDiffuse, diffuse[0], diffuse[1], diffuse[2] );

	if( overbrights != 1 )
	{
		if( program->locGlossIntensity >= 0 )
			qglUniform1fARB( program->locGlossIntensity, r_lighting_glossintensity->value * (1.0 / overbrights) );
		if( program->locGlossExponent >= 0 )
			qglUniform1fARB( program->locGlossExponent, log( exp( glossExponent ) / overbrights ) );
	}
	else
	{
		if( program->locGlossIntensity >= 0 )
			qglUniform1fARB( program->locGlossIntensity, r_lighting_glossintensity->value  );
		if( program->locGlossExponent >= 0 )
			qglUniform1fARB( program->locGlossExponent, glossExponent );
	}

	if( program->locOffsetMappingScale >= 0 )
		qglUniform1fARB( program->locOffsetMappingScale, offsetmappingScale );

#ifdef HARDWARE_OUTLINES
	if( program->locOutlineHeight >= 0 )
		qglUniform1fARB( program->locOutlineHeight, projDistance );
	if( program->locOutlineCutOff >= 0 )
		qglUniform1fARB( program->locOutlineCutOff, max( 0, r_outlines_cutoff->value ) );
#endif

	if( program->locFrontPlane >= 0 )
		qglUniform1fARB( program->locFrontPlane, frontPlane ? 1 : -1 );

	if( program->locInvTextureWidth >= 0 )
		qglUniform1fARB( program->locInvTextureWidth, TexWidth ? 1.0 / TexWidth : 1.0 );
	if( program->locInvTextureHeight >= 0 )
		qglUniform1fARB( program->locInvTextureHeight, TexHeight ? 1.0 / TexHeight : 1.0 );

	if( program->locProjDistance >= 0 )
		qglUniform1fARB( program->locProjDistance, projDistance );

	if( program->locTurbAmplitude )
		qglUniform1fARB( program->locTurbAmplitude, projDistance );
	if( program->locTurbPhase )
		qglUniform1fARB( program->locTurbPhase, offsetmappingScale );

	if( superLightStyle )
	{
		int i;

		for( i = 0; i < MAX_LIGHTMAPS && superLightStyle->lightmapStyles[i] != 255; i++ )
		{
			vec3_t rgb;

			VectorCopy( r_lightStyles[superLightStyle->lightmapStyles[i]].rgb, rgb );
			if( mapConfig.lightingIntensity )
				VectorScale( rgb, mapConfig.lightingIntensity, rgb );

			if( program->locDeluxemapOffset[i] >= 0 )
				qglUniform1fARB( program->locDeluxemapOffset[i], superLightStyle->stOffset[i][0] );
			if( program->loclsColor[i] >= 0 )
				qglUniform3fARB( program->loclsColor[i], rgb[0], rgb[1], rgb[2] );
		}

		for( ; i < MAX_LIGHTMAPS; i++ )
		{
			if( program->loclsColor[i] >= 0 )
				qglUniform3fARB( program->loclsColor[i], 0, 0, 0 );
		}
	}

	if( program->locConstColor >= 0 && constColor )
		qglUniform4fARB( program->locConstColor, constColor[0] * 1.0/255.0, constColor[1] * 1.0/255.0, constColor[2] * 1.0/255.0, constColor[3] * 1.0/255.0 );
	if( program->locOverbrightScale >= 0 )
		qglUniform1fARB( program->locOverbrightScale, 1.0 / (1 << overbrightBits) );
}

/*
================
R_UpdateProgramFogParams
================
*/
void R_UpdateProgramFogParams( int elem, byte_vec4_t color, float clearDist, float opaqueDist, cplane_t *fogPlane, cplane_t *eyePlane, float eyeFogDist )
{
	GLfloat fog_start, fog_end;
	GLfloat fog_color[4] = { 0, 0, 0, 1 };
	glsl_program_t *program = r_glslprograms + elem - 1;

	VectorScale( color, (1.0/255.0), fog_color );
	fog_start = clearDist, fog_end = opaqueDist;

	qglFogfv( GL_FOG_COLOR, fog_color );
	qglFogfv( GL_FOG_START, &fog_start );
	qglFogfv( GL_FOG_END, &fog_end );
	qglFogi( GL_FOG_MODE, GL_LINEAR );

	if( program->locFogPlane >= 0 )
		qglUniform4fARB( program->locFogPlane, fogPlane->normal[0], fogPlane->normal[1], fogPlane->normal[2], fogPlane->dist );
	if( program->locEyePlane >= 0 )
		qglUniform4fARB( program->locEyePlane, eyePlane->normal[0], eyePlane->normal[1], eyePlane->normal[2], eyePlane->dist );
	if( program->locEyeFogDist >= 0 )
		qglUniform1fARB( program->locEyeFogDist, eyeFogDist );
}

/*
================
R_UpdateProgramLightsParams
================
*/
unsigned int R_UpdateProgramLightsParams( int elem, unsigned int dlightbits )
{
	int i, n;
	int bit;
	GLfloat v[4], d[4];
	GLfloat icutoff, cutoff;
	dlight_t *dl;
	vec3_t acolor = { 0, 0, 0 };
	//glsl_program_t *program = r_glslprograms + elem - 1;

	if( !dlightbits )
		return 0;

	for( i = 0, n = 0, dl = r_dlights; i < MAX_DLIGHTS; i++, dl++ )
	{
		if( !(dlightbits & (1<<i)) )
			continue;

		// in case less color clamping is desired, the following code can be enabled
		if( 0 )
		{
			if( acolor[0] + dl->color[0] > 1 || acolor[1] + dl->color[1] > 1 || acolor[2] + dl->color[2] )
				break;
		}

		cutoff = dl->intensity;
		icutoff = 1.0 / cutoff;

		VectorCopy( dl->origin, v );
		VectorCopy( dl->color, d );

		qglLightfv( GL_LIGHT0 + n, GL_POSITION, v );

		// hack to avoid doing division in in vertex shader, we're using specular here
		// because all both position and spotDirection are transformed by the modelview matrix
		// prior to entering the shader, which is undesirable
		VectorScale( v, icutoff, v );
		qglLightfv( GL_LIGHT0 + n, GL_SPECULAR, v );
		qglLightf( GL_LIGHT0 + n, GL_SPOT_CUTOFF, icutoff );
		qglLightfv( GL_LIGHT0 + n, GL_DIFFUSE, d );

		VectorAdd( acolor, dl->color, acolor );

		// unmark this dynamic light
		dlightbits &= ~(1<<i);

		// stop at maximum amount of dynamic lights per program reached
		bit = (PROGRAM_APPLY_DLIGHT0 << n);
		if( bit == PROGRAM_APPLY_DLIGHT_MAX )
			break;

		// can't handle more than GL_MAX_VARYING_FLOATS_ARB/4 lights due to the hardware limit on varying variables
		if( ++n >= glConfig.maxVaryingFloats / 4 )
			break;
		if( n >= r_lighting_maxglsldlights->integer )
			break;
	}

	return dlightbits;
}

/*
================
R_UpdateProgramQ3AShaderParams
================
*/
void R_UpdateProgramQ3AParams( int elem, const vec3_t eyeOrigin, const vec3_t entDist, const qbyte *constColor, int overbrightBits )
{
	glsl_program_t *program = r_glslprograms + elem - 1;

	if( program->locEyeOrigin >= 0 && eyeOrigin )
		qglUniform3fARB( program->locEyeOrigin, eyeOrigin[0], eyeOrigin[1], eyeOrigin[2] );
	if( program->locEntDist >= 0 && entDist )
		qglUniform3fARB( program->locEntDist, entDist[0], entDist[1], entDist[2] );
	if( program->locConstColor >= 0 && constColor )
		qglUniform4fARB( program->locConstColor, constColor[0] * 1.0/255.0, constColor[1] * 1.0/255.0, constColor[2] * 1.0/255.0, constColor[3] * 1.0/255.0 );
	if( program->locOverbrightScale >= 0 )
		qglUniform1fARB( program->locOverbrightScale, 1.0 / (1 << overbrightBits) );
}

/*
================
R_GetProgramUniformLocations
================
*/
static void R_GetProgramUniformLocations( glsl_program_t *program )
{
	int		i;
	int		locBaseTexture,
			locNormalmapTexture,
			locGlossTexture,
			locDecalTexture,
			locLightmapTexture[MAX_LIGHTMAPS],
			locDuDvMapTexture,
			locReflectionTexture,
			locRefractionTexture,
			locShadowmapTexture;
	char	uniformName[128];

	program->locEyeOrigin = qglGetUniformLocationARB( program->object, "EyeOrigin" );
	program->locLightDir = qglGetUniformLocationARB( program->object, "LightDir" );
	program->locLightOrigin = qglGetUniformLocationARB( program->object, "LightOrigin" );
	program->locLightAmbient = qglGetUniformLocationARB( program->object, "LightAmbient" );
	program->locLightDiffuse = qglGetUniformLocationARB( program->object, "LightDiffuse" );

	locBaseTexture = qglGetUniformLocationARB( program->object, "BaseTexture" );
	locNormalmapTexture = qglGetUniformLocationARB( program->object, "NormalmapTexture" );
	locGlossTexture = qglGetUniformLocationARB( program->object, "GlossTexture" );
	locDecalTexture = qglGetUniformLocationARB( program->object, "DecalTexture" );

	locDuDvMapTexture = qglGetUniformLocationARB( program->object, "DuDvMapTexture" );
	locReflectionTexture = qglGetUniformLocationARB( program->object, "ReflectionTexture" );
	locRefractionTexture = qglGetUniformLocationARB( program->object, "RefractionTexture" );

	locShadowmapTexture = qglGetUniformLocationARB( program->object, "ShadowmapTexture" );

	for( i = 0; i < MAX_LIGHTMAPS; i++ )
	{
		Q_snprintfz( uniformName, sizeof( uniformName ), "LightmapTexture%i", i );
		locLightmapTexture[i] = qglGetUniformLocationARB( program->object, uniformName );

		Q_snprintfz( uniformName, sizeof( uniformName ), "DeluxemapOffset%i", i );
		program->locDeluxemapOffset[i] = qglGetUniformLocationARB( program->object, uniformName );

		Q_snprintfz( uniformName, sizeof( uniformName ), "lsColor%i", i );
		program->loclsColor[i] = qglGetUniformLocationARB( program->object, uniformName );
	}

	program->locGlossIntensity = qglGetUniformLocationARB( program->object, "GlossIntensity" );
	program->locGlossExponent = qglGetUniformLocationARB( program->object, "GlossExponent" );

	program->locOffsetMappingScale = qglGetUniformLocationARB( program->object, "OffsetMappingScale" );

#ifdef HARDWARE_OUTLINES
	program->locOutlineHeight = qglGetUniformLocationARB( program->object, "OutlineHeight" );
	program->locOutlineCutOff = qglGetUniformLocationARB( program->object, "OutlineCutOff" );
#endif

	program->locFrontPlane = qglGetUniformLocationARB( program->object, "FrontPlane" );

	program->locInvTextureWidth = qglGetUniformLocationARB( program->object, "InvTextureWidth" );
	program->locInvTextureHeight = qglGetUniformLocationARB( program->object, "InvTextureHeight" );

	program->locProjDistance = qglGetUniformLocationARB( program->object, "ProjDistance" );

	program->locTurbAmplitude = qglGetUniformLocationARB( program->object, "TurbAmplitude" );
	program->locTurbPhase = qglGetUniformLocationARB( program->object, "TurbPhase" );

	program->locEntDist = qglGetUniformLocationARB( program->object, "EntDist" );
	program->locConstColor = qglGetUniformLocationARB( program->object, "ConstColor" );
	program->locOverbrightScale = qglGetUniformLocationARB( program->object, "OverbrightScale" );

	program->locFogPlane = qglGetUniformLocationARB( program->object, "FogPlane" );
	program->locEyePlane = qglGetUniformLocationARB( program->object, "EyePlane" );
	program->locEyeFogDist = qglGetUniformLocationARB( program->object, "EyeFogDist" );

	if( locBaseTexture >= 0 )
		qglUniform1iARB( locBaseTexture, 0 );
	if( locDuDvMapTexture >= 0 )
		qglUniform1iARB( locDuDvMapTexture, 0 );

	if( locNormalmapTexture >= 0 )
		qglUniform1iARB( locNormalmapTexture, 1 );
	if( locGlossTexture >= 0 )
		qglUniform1iARB( locGlossTexture, 2 );
	if( locDecalTexture >= 0 )
		qglUniform1iARB( locDecalTexture, 3 );

	if( locReflectionTexture >= 0 )
		qglUniform1iARB( locReflectionTexture, 2 );
	if( locRefractionTexture >= 0 )
		qglUniform1iARB( locRefractionTexture, 3 );

	if( locShadowmapTexture >= 0 )
		qglUniform1iARB( locShadowmapTexture, 0 );

	for( i = 0; i < MAX_LIGHTMAPS; i++ )
	{
		if( locLightmapTexture[i] >= 0 )
			qglUniform1iARB( locLightmapTexture[i], i+4 );
	}
}

/*
================
R_ShutdownGLSLPrograms
================
*/
void R_ShutdownGLSLPrograms( void )
{
	int i;
	glsl_program_t *program;

	if( !r_glslProgramsPool )
		return;
	if( !glConfig.ext.GLSL )
		return;

	for( i = 0, program = r_glslprograms; i < r_numglslprograms; i++, program++ )
		R_DeleteGLSLProgram( program );

	Mem_FreePool( &r_glslProgramsPool );

	r_numglslprograms = 0;
}
