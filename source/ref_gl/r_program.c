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

#define MAX_GLSL_PROGRAMS			1024
#define GLSL_PROGRAMS_HASH_SIZE		64

#define GLSL_CACHE_FILE_NAME		"glsl.cache"

typedef struct
{
	r_glslfeat_t	bit;
	const char		*define;
	const char		*suffix;
} glsl_feature_t;

typedef struct glsl_program_s
{
	char			*name;
	int				type;
	r_glslfeat_t	features;
	const char		*string;
	char 			*deformsKey;
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
					locTextureParams,
					locProjDistance,

					locTurbAmplitude,
					locTurbPhase,

					locEntDist,
					locEntColor,
					locConstColor,
					locOverbrightScale,

					locFogPlane,
					locEyePlane,
					locEyeFogDist,

					locShaderTime,

					locReflectionMatrix,

					locPlaneNormal,
					locPlaneDist,

					locDeluxemapOffset[MAX_LIGHTMAPS],
					loclsColor[MAX_LIGHTMAPS],

					locDynamicLightsRadius[MAX_DLIGHTS],
					locDynamicLightsPosition[MAX_DLIGHTS],
					locDynamicLightsDiffuse[MAX_DLIGHTS],

					locAttrBonesIndices,
					locAttrBonesWeights,
					*locDualQuats,

					locWallColor,
					locFloorColor
	;
} glsl_program_t;

static int r_numglslprograms;
static glsl_program_t r_glslprograms[MAX_GLSL_PROGRAMS];
static glsl_program_t *r_glslprograms_hash[GLSL_PROGRAM_TYPE_MAXTYPE][GLSL_PROGRAMS_HASH_SIZE];
static mempool_t *r_glslProgramsPool;

#define R_ProgramAlloc(size) Mem_Alloc( r_glslProgramsPool, ( size ) )

static void R_GetProgramUniformLocations( glsl_program_t *program );
static void R_BindProgramAttrbibutesLocations( glsl_program_t *program );

static void R_PrecacheGLSLPrograms( void );
static void R_StoreGLSLPrecacheList( void );

static const char *r_defaultMaterialProgram;
static const char *r_defaultDistortionGLSLProgram;
static const char *r_defaultShadowmapGLSLProgram;
#ifdef HARDWARE_OUTLINES
static const char *r_defaultOutlineGLSLProgram;
#endif
static const char *r_defaultTurbulenceProgram;
static const char *r_defaultDynamicLightsProgram;
static const char *r_defaultQ3AShaderProgram;
static const char *r_defaultPlanarShadowProgram;
static const char *r_defaultCellshadeProgram;

/*
* R_InitGLSLPrograms
*/
void R_InitGLSLPrograms( void )
{
	int program;

	memset( r_glslprograms, 0, sizeof( r_glslprograms ) );
	memset( r_glslprograms_hash, 0, sizeof( r_glslprograms_hash ) );

	if( !glConfig.ext.GLSL )
		return;

	r_glslProgramsPool = Mem_AllocPool( NULL, "GLSL Programs" );

	// register base programs
	R_RegisterGLSLProgram( GLSL_PROGRAM_TYPE_MATERIAL, DEFAULT_GLSL_PROGRAM, r_defaultMaterialProgram, NULL, NULL, 0, 0 );
	R_RegisterGLSLProgram( GLSL_PROGRAM_TYPE_DISTORTION, DEFAULT_GLSL_DISTORTION_PROGRAM, r_defaultDistortionGLSLProgram, NULL, NULL, 0, 0 );
	R_RegisterGLSLProgram( GLSL_PROGRAM_TYPE_SHADOWMAP, DEFAULT_GLSL_SHADOWMAP_PROGRAM, r_defaultShadowmapGLSLProgram, NULL, NULL, 0, 0 );
#ifdef HARDWARE_OUTLINES
	R_RegisterGLSLProgram( GLSL_PROGRAM_TYPE_OUTLINE, DEFAULT_GLSL_OUTLINE_PROGRAM, r_defaultOutlineGLSLProgram, NULL, NULL, 0, 0 );
#endif
	R_RegisterGLSLProgram( GLSL_PROGRAM_TYPE_TURBULENCE, DEFAULT_GLSL_TURBULENCE_PROGRAM, r_defaultTurbulenceProgram, NULL, NULL, 0, 0 );
	R_RegisterGLSLProgram( GLSL_PROGRAM_TYPE_Q3A_SHADER, DEFAULT_GLSL_Q3A_SHADER_PROGRAM, r_defaultQ3AShaderProgram, NULL, NULL, 0, 0 );
	R_RegisterGLSLProgram( GLSL_PROGRAM_TYPE_PLANAR_SHADOW, DEFAULT_GLSL_PLANAR_SHADOW_PROGRAM, r_defaultPlanarShadowProgram, NULL, NULL, 0, 0 );
	R_RegisterGLSLProgram( GLSL_PROGRAM_TYPE_CELLSHADE, DEFAULT_GLSL_CELLSHADE_PROGRAM, r_defaultCellshadeProgram, NULL, NULL, 0, 0 );

	R_RegisterGLSLProgram( GLSL_PROGRAM_TYPE_DYNAMIC_LIGHTS, DEFAULT_GLSL_DYNAMIC_LIGHTS_PROGRAM, r_defaultDynamicLightsProgram, NULL, NULL, 0, GLSL_COMMON_APPLY_DLIGHTS_4 );
	R_RegisterGLSLProgram( GLSL_PROGRAM_TYPE_DYNAMIC_LIGHTS, DEFAULT_GLSL_DYNAMIC_LIGHTS_PROGRAM, r_defaultDynamicLightsProgram, NULL, NULL, 0, GLSL_COMMON_APPLY_DLIGHTS_8 );
	R_RegisterGLSLProgram( GLSL_PROGRAM_TYPE_DYNAMIC_LIGHTS, DEFAULT_GLSL_DYNAMIC_LIGHTS_PROGRAM, r_defaultDynamicLightsProgram, NULL, NULL, 0, GLSL_COMMON_APPLY_DLIGHTS_16 );
	R_RegisterGLSLProgram( GLSL_PROGRAM_TYPE_DYNAMIC_LIGHTS, DEFAULT_GLSL_DYNAMIC_LIGHTS_PROGRAM, r_defaultDynamicLightsProgram, NULL, NULL, 0, GLSL_COMMON_APPLY_DLIGHTS_32 );

	// check whether compilation of the shader with GPU skinning succeeds, if not, disable GPU bone transforms
	if( glConfig.maxGLSLBones ) {
		program = R_RegisterGLSLProgram( GLSL_PROGRAM_TYPE_MATERIAL, DEFAULT_GLSL_PROGRAM, NULL, NULL, NULL, 0, GLSL_COMMON_APPLY_BONETRANSFORMS1 );
		if( !program ) {
			glConfig.maxGLSLBones = 0;
		}
	}

	R_PrecacheGLSLPrograms();
}

/*
* R_PrecacheGLSLPrograms
*
* Loads the list of known program permutations from disk file.
*
* Expected file format:
* application_name\n
* version_number\n*
* program_type1 features_lower_bits1 features_higher_bits1 program_name1
* ..
* program_typeN features_lower_bitsN features_higher_bitsN program_nameN
*/
static void R_PrecacheGLSLPrograms( void )
{
#ifdef NDEBUG
	int length;
	int version;
	char *buffer = NULL, *data, **ptr;
	const char *token;
	const char *fileName;

	fileName = GLSL_CACHE_FILE_NAME;

	length = FS_LoadFile( fileName, ( void ** )&buffer, NULL, 0 );
	if( !buffer ) {
		return;
	}

	data = buffer;
	ptr = &data;

	token = COM_Parse( ptr );
	if( strcmp( token, APPLICATION ) ) {
		Com_DPrintf( "Ignoring %s: unknown application name \"%s\", expected \"%s\"\n", token, APPLICATION );
		return;
	}

	token = COM_Parse( ptr );
	version = atoi( token );
	if( version != GLSL_BITS_VERSION ) {
		// ignore cache files with mismatching version number
		Com_DPrintf( "Ignoring %s: found version %i, expcted %i\n", version, GLSL_BITS_VERSION );
	}
	else {
		while( 1 ) {
			int type;
			r_glslfeat_t lb, hb;
			r_glslfeat_t features;
			const char *name;

			// read program type
			token = COM_Parse( ptr );
			if( !token[0] ) {
				break;
			}
			type = atoi( token );

			// read lower bits
			token = COM_ParseExt( ptr, qfalse );
			if( !token[0] ) {
				break;
			}
			lb = atoi( token );

			// read higher bits
			token = COM_ParseExt( ptr, qfalse );
			if( !token[0] ) {
				break;
			}
			hb = atoi( token );

			// read program full name
			token = COM_ParseExt( ptr, qfalse );
			if( !token[0] ) {
				break;
			}

			name = token;
			features = (hb << 32) | lb; 

			Com_DPrintf( "Loading program %s...\n", name );

			R_RegisterGLSLProgram( type, name, NULL, NULL, NULL, 0, features );
		}
	}

	FS_FreeFile( buffer );
#endif
}


/*
* R_StoreGLSLPrecacheList
*
* Stores the list of known GLSL program permutations to file on the disk.
* File format matches that expected by R_PrecacheGLSLPrograms.
*/
static void R_StoreGLSLPrecacheList( void )
{
#ifdef NDEBUG
	int i;
	int handle;
	const char *fileName;
	glsl_program_t *program;

	fileName = GLSL_CACHE_FILE_NAME;
	if( FS_FOpenFile( fileName, &handle, FS_WRITE ) == -1 ) {
		Com_Printf( S_COLOR_YELLOW "Could not open %s for writing.\n", fileName );
		return;
	}

	FS_Printf( handle, "%s\n", APPLICATION );
	FS_Printf( handle, "%i\n", GLSL_BITS_VERSION );

	for( i = 0, program = r_glslprograms; i < r_numglslprograms; i++, program++ ) {
		if( !program->features ) {
			continue;
		}
		if( *program->deformsKey ) {
			continue;
		}
		FS_Printf( handle, "%i %i %i %s\n", 
			program->type, (int)(program->features & ULONG_MAX), (int)((program->features>>32) & ULONG_MAX), program->name );
	}

	FS_FCloseFile( handle );
#endif
}

/*
* R_GLSLProgramCopyString
*/
static char *R_GLSLProgramCopyString( const char *in )
{
	char *out;

	out = R_ProgramAlloc( strlen( in ) + 1 );
	strcpy( out, in );

	return out;
}

/*
* R_DeleteGLSLProgram
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
* R_CompileGLSLShader
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
* R_FindGLSLProgram
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
	{ GLSL_COMMON_APPLY_CLIPPING, "#define APPLY_CLIPPING\n", "_clip" },
	{ GLSL_COMMON_APPLY_GRAYSCALE, "#define APPLY_GRAYSCALE\n", "_grey" },

	{ 0, NULL, NULL }
};

static const glsl_feature_t glsl_features_materal[] =
{
	{ GLSL_COMMON_APPLY_CLIPPING, "#define APPLY_CLIPPING\n", "_clip" },
	{ GLSL_COMMON_APPLY_GRAYSCALE, "#define APPLY_GRAYSCALE\n", "_grey" },

	{ GLSL_COMMON_APPLY_OVERBRIGHT_SCALING, "#define APPLY_OVERBRIGHT_SCALING\n", "_os" },

	{ GLSL_COMMON_APPLY_BONETRANSFORMS4, "#define APPLY_BONETRANSFORMS 4\n", "_bones4" },
	{ GLSL_COMMON_APPLY_BONETRANSFORMS3, "#define APPLY_BONETRANSFORMS 3\n", "_bones3" },
	{ GLSL_COMMON_APPLY_BONETRANSFORMS2, "#define APPLY_BONETRANSFORMS 2\n", "_bones2" },
	{ GLSL_COMMON_APPLY_BONETRANSFORMS1, "#define APPLY_BONETRANSFORMS 1\n", "_bones1" },

	{ GLSL_COMMON_APPLY_TC_GEN_REFLECTION, "#define APPLY_TC_GEN_REFLECTION\n", "_tc_refl" },
	{ GLSL_COMMON_APPLY_TC_GEN_ENV, "#define APPLY_TC_GEN_ENV\n", "_tc_env" },
	{ GLSL_COMMON_APPLY_TC_GEN_VECTOR, "#define APPLY_TC_GEN_VECTOR\n", "_tc_vec" },

	{ GLSL_COMMON_APPLY_RGB_GEN_ONE_MINUS_VERTEX, "#define APPLY_RGB_ONE_MINUS_VERTEX\n", "_c1-v" },
	{ GLSL_COMMON_APPLY_RGB_GEN_CONST, "#define APPLY_RGB_CONST\n", "_cc" },
	{ GLSL_COMMON_APPLY_RGB_GEN_VERTEX, "#define APPLY_RGB_VERTEX\n", "_cv" },

	{ GLSL_COMMON_APPLY_ALPHA_GEN_ONE_MINUS_VERTEX, "#define APPLY_ALPHA_ONE_MINUS_VERTEX\n", "_a1-v" },
	{ GLSL_COMMON_APPLY_ALPHA_GEN_VERTEX, "#define APPLY_ALPHA_VERTEX\n", "_av" },
	{ GLSL_COMMON_APPLY_ALPHA_GEN_CONST, "#define APPLY_ALPHA_CONST\n", "_ac" },

	{ GLSL_COMMON_APPLY_FOG, "#define APPLY_FOG\n", "_fog" },
	{ GLSL_COMMON_APPLY_FOG2, "#define APPLY_FOG\n#define APPLY_FOG2\n", "2" },
	{ GLSL_COMMON_APPLY_COLOR_FOG_ALPHA, "#define APPLY_COLOR_FOG_ALPHA\n", "_afog" },

	{ GLSL_COMMON_APPLY_DLIGHTS_32, "#undef MAX_DLIGHTS\n#define MAX_DLIGHTS 32\n", "_dl32" },
	{ GLSL_COMMON_APPLY_DLIGHTS_16, "#undef MAX_DLIGHTS\n#define MAX_DLIGHTS 16\n", "_dl16" },
	{ GLSL_COMMON_APPLY_DLIGHTS_8, "#undef MAX_DLIGHTS\n#define MAX_DLIGHTS 8\n", "_dl8" },
	{ GLSL_COMMON_APPLY_DLIGHTS_4, "#undef MAX_DLIGHTS\n#define MAX_DLIGHTS 4\n", "_dl4" },

	{ GLSL_COMMON_APPLY_DRAWFLAT, "#define APPLY_DRAWFLAT\n", "_flat" },

	{ GLSL_MATERIAL_APPLY_LIGHTSTYLE3, "#define APPLY_LIGHTSTYLE0\n#define APPLY_LIGHTSTYLE1\n#define APPLY_LIGHTSTYLE2\nAPPLY_LIGHTSTYLE3\n", "_ls3" },
	{ GLSL_MATERIAL_APPLY_LIGHTSTYLE2, "#define APPLY_LIGHTSTYLE0\n#define APPLY_LIGHTSTYLE1\n#define APPLY_LIGHTSTYLE2\n", "_ls2" },
	{ GLSL_MATERIAL_APPLY_LIGHTSTYLE1, "#define APPLY_LIGHTSTYLE0\n#define APPLY_LIGHTSTYLE1\n", "_ls1" },
	{ GLSL_MATERIAL_APPLY_LIGHTSTYLE0, "#define APPLY_LIGHTSTYLE0\n", "_ls0" },
	{ GLSL_MATERIAL_APPLY_FB_LIGHTMAP, "#define APPLY_FBLIGHTMAP\n", "_fb" },
	{ GLSL_MATERIAL_APPLY_DIRECTIONAL_LIGHT, "#define APPLY_DIRECTIONAL_LIGHT\n", "_dirlight" },
	{ GLSL_MATERIAL_APPLY_SPECULAR, "#define APPLY_SPECULAR\n", "_gloss" },
	{ GLSL_MATERIAL_APPLY_OFFSETMAPPING, "#define APPLY_OFFSETMAPPING\n", "_offmap" },
	{ GLSL_MATERIAL_APPLY_RELIEFMAPPING, "#define APPLY_RELIEFMAPPING\n", "_relmap" },
	{ GLSL_MATERIAL_APPLY_AMBIENT_COMPENSATION, "#define APPLY_AMBIENT_COMPENSATION\n", "_amb" },
	{ GLSL_MATERIAL_APPLY_DECAL, "#define APPLY_DECAL\n", "_decal" },
	{ GLSL_MATERIAL_APPLY_DECAL_ADD, "#define APPLY_DECAL_ADD\n", "_add" },
	{ GLSL_MATERIAL_APPLY_BASETEX_ALPHA_ONLY, "#define APPLY_BASETEX_ALPHA_ONLY\n", "_alpha" },
	{ GLSL_MATERIAL_APPLY_CELLSHADING, "#define APPLY_CELLSHADING\n", "_cell" },
	{ GLSL_MATERIAL_APPLY_HALFLAMBERT, "#define APPLY_HALFLAMBERT\n", "_lambert" },

	{ GLSL_MATERIAL_APPLY_ENTITY_DECAL, "#define APPLY_ENTITY_DECAL\n", "_decal2" },
	{ GLSL_MATERIAL_APPLY_ENTITY_DECAL_ADD, "#define APPLY_ENTITY_DECAL_ADD\n", "_decal2_add" },

	// doesn't make sense without APPLY_DIRECTIONAL_LIGHT
	{ GLSL_MATERIAL_APPLY_DIRECTIONAL_LIGHT_MIX, "#define APPLY_DIRECTIONAL_LIGHT_MIX\n", "_mix" },

	{ 0, NULL, NULL }
};

static const glsl_feature_t glsl_features_distortion[] =
{
	{ GLSL_COMMON_APPLY_CLIPPING, "#define APPLY_CLIPPING\n", "_clip" },
	{ GLSL_COMMON_APPLY_GRAYSCALE, "#define APPLY_GRAYSCALE\n", "_grey" },

	{ GLSL_DISTORTION_APPLY_DUDV, "#define APPLY_DUDV\n", "_dudv" },
	{ GLSL_DISTORTION_APPLY_EYEDOT, "#define APPLY_EYEDOT\n", "_eyedot" },
	{ GLSL_DISTORTION_APPLY_DISTORTION_ALPHA, "#define APPLY_DISTORTION_ALPHA\n", "_alpha" },
	{ GLSL_DISTORTION_APPLY_REFLECTION, "#define APPLY_REFLECTION\n", "_refl" },
	{ GLSL_DISTORTION_APPLY_REFRACTION, "#define APPLY_REFRACTION\n", "_refr" },

	{ 0, NULL, NULL }
};

static const glsl_feature_t glsl_features_shadowmap[] =
{
	{ GLSL_COMMON_APPLY_CLIPPING, "#define APPLY_CLIPPING\n", "_clip" },

	{ GLSL_SHADOWMAP_APPLY_DITHER, "#define APPLY_DITHER\n", "_dither" },
	{ GLSL_SHADOWMAP_APPLY_PCF, "#define APPLY_PCF\n", "_pcf" },

	{ 0, NULL, NULL }
};

static const glsl_feature_t glsl_features_outline[] =
{
	{ GLSL_COMMON_APPLY_CLIPPING, "#define APPLY_CLIPPING\n", "_clip" },

	{ GLSL_COMMON_APPLY_BONETRANSFORMS4, "#define APPLY_BONETRANSFORMS 4\n", "_bones4" },
	{ GLSL_COMMON_APPLY_BONETRANSFORMS3, "#define APPLY_BONETRANSFORMS 3\n", "_bones3" },
	{ GLSL_COMMON_APPLY_BONETRANSFORMS2, "#define APPLY_BONETRANSFORMS 2\n", "_bones2" },
	{ GLSL_COMMON_APPLY_BONETRANSFORMS1, "#define APPLY_BONETRANSFORMS 1\n", "_bones1" },

	{ GLSL_OUTLINE_APPLY_OUTLINES_CUTOFF, "#define APPLY_OUTLINES_CUTOFF\n", "_outcut" },

	{ 0, NULL, NULL }
};

static const glsl_feature_t glsl_features_dynamiclights[] =
{
	{ GLSL_COMMON_APPLY_CLIPPING, "#define APPLY_CLIPPING\n", "_clip" },

	{ GLSL_COMMON_APPLY_DLIGHTS_32, "#undef NUM_DLIGHTS\n#define NUM_DLIGHTS 32\n", "_dl32" },
	{ GLSL_COMMON_APPLY_DLIGHTS_16, "#undef NUM_DLIGHTS\n#define NUM_DLIGHTS 16\n", "_dl16" },
	{ GLSL_COMMON_APPLY_DLIGHTS_8, "#undef NUM_DLIGHTS\n#define NUM_DLIGHTS 8\n", "_dl8" },
	{ GLSL_COMMON_APPLY_DLIGHTS_4, "#undef NUM_DLIGHTS\n#define NUM_DLIGHTS 4\n", "_dl4" },

	{ 0, NULL, NULL }
};

static const glsl_feature_t glsl_features_q3a[] =
{
	{ GLSL_COMMON_APPLY_CLIPPING, "#define APPLY_CLIPPING\n", "_clip" },

	{ GLSL_COMMON_APPLY_BONETRANSFORMS4, "#define APPLY_BONETRANSFORMS 4\n", "_bones4" },
	{ GLSL_COMMON_APPLY_BONETRANSFORMS3, "#define APPLY_BONETRANSFORMS 3\n", "_bones3" },
	{ GLSL_COMMON_APPLY_BONETRANSFORMS2, "#define APPLY_BONETRANSFORMS 2\n", "_bones2" },
	{ GLSL_COMMON_APPLY_BONETRANSFORMS1, "#define APPLY_BONETRANSFORMS 1\n", "_bones1" },

	{ GLSL_COMMON_APPLY_TC_GEN_REFLECTION, "#define APPLY_TC_GEN_REFLECTION\n", "_tc_refl" },
	{ GLSL_COMMON_APPLY_TC_GEN_ENV, "#define APPLY_TC_GEN_ENV\n", "_tc_env" },
	{ GLSL_COMMON_APPLY_TC_GEN_VECTOR, "#define APPLY_TC_GEN_VECTOR\n", "_tc_vec" },

	{ GLSL_COMMON_APPLY_RGB_GEN_ONE_MINUS_VERTEX, "#define APPLY_RGB_ONE_MINUS_VERTEX\n", "_c1-v" },
	{ GLSL_COMMON_APPLY_RGB_GEN_CONST, "#define APPLY_RGB_CONST\n", "_cc" },
	{ GLSL_COMMON_APPLY_RGB_GEN_VERTEX, "#define APPLY_RGB_VERTEX\n", "_cv" },

	{ GLSL_COMMON_APPLY_ALPHA_GEN_ONE_MINUS_VERTEX, "#define APPLY_ALPHA_ONE_MINUS_VERTEX\n", "_a1-v" },
	{ GLSL_COMMON_APPLY_ALPHA_GEN_CONST, "#define APPLY_ALPHA_CONST\n", "_ac" },
	{ GLSL_COMMON_APPLY_ALPHA_GEN_VERTEX, "#define APPLY_ALPHA_VERTEX\n", "_av" },

	{ GLSL_COMMON_APPLY_OVERBRIGHT_SCALING, "#define APPLY_OVERBRIGHT_SCALING\n", "_os" },

	{ GLSL_COMMON_APPLY_FOG, "#define APPLY_FOG\n", "_fog" },
	{ GLSL_COMMON_APPLY_FOG2, "#define APPLY_FOG\n#define APPLY_FOG2\n", "2" },
	{ GLSL_COMMON_APPLY_COLOR_FOG_ALPHA, "#define APPLY_COLOR_FOG_ALPHA\n", "_afog" },

	{ GLSL_COMMON_APPLY_DLIGHTS_32, "#undef MAX_DLIGHTS\n#define MAX_DLIGHTS 32\n", "_dl32" },
	{ GLSL_COMMON_APPLY_DLIGHTS_16, "#undef MAX_DLIGHTS\n#define MAX_DLIGHTS 16\n", "_dl16" },
	{ GLSL_COMMON_APPLY_DLIGHTS_8, "#undef MAX_DLIGHTS\n#define MAX_DLIGHTS 8\n", "_dl8" },
	{ GLSL_COMMON_APPLY_DLIGHTS_4, "#undef MAX_DLIGHTS\n#define MAX_DLIGHTS 4\n", "_dl4" },

	{ GLSL_COMMON_APPLY_DRAWFLAT, "#define APPLY_DRAWFLAT\n", "_flat" },

	{ GLSL_Q3A_APPLY_TC_GEN_FOG, "#define APPLY_TC_GEN_FOG\n", "_tc_fog" },
	{ GLSL_Q3A_APPLY_COLOR_FOG, "#define APPLY_COLOR_FOG\n", "_cfog" },

	{ 0, NULL, NULL }
};

static const glsl_feature_t glsl_features_planar_shadow[] =
{
	{ GLSL_COMMON_APPLY_CLIPPING, "#define APPLY_CLIPPING\n", "_clip" },

	{ GLSL_COMMON_APPLY_BONETRANSFORMS4, "#define APPLY_BONETRANSFORMS 4\n", "_bones4" },
	{ GLSL_COMMON_APPLY_BONETRANSFORMS3, "#define APPLY_BONETRANSFORMS 3\n", "_bones3" },
	{ GLSL_COMMON_APPLY_BONETRANSFORMS2, "#define APPLY_BONETRANSFORMS 2\n", "_bones2" },
	{ GLSL_COMMON_APPLY_BONETRANSFORMS1, "#define APPLY_BONETRANSFORMS  1\n", "_bones1" },

	{ 0, NULL, NULL }
};

static const glsl_feature_t glsl_features_cellshade[] =
{
	{ GLSL_COMMON_APPLY_CLIPPING, "#define APPLY_CLIPPING\n", "_clip" },
	{ GLSL_COMMON_APPLY_GRAYSCALE, "#define APPLY_GRAYSCALE\n", "_grey" },

	{ GLSL_COMMON_APPLY_BONETRANSFORMS4, "#define APPLY_BONETRANSFORMS 4\n", "_bones4" },
	{ GLSL_COMMON_APPLY_BONETRANSFORMS3, "#define APPLY_BONETRANSFORMS 3\n", "_bones3" },
	{ GLSL_COMMON_APPLY_BONETRANSFORMS2, "#define APPLY_BONETRANSFORMS 2\n", "_bones2" },
	{ GLSL_COMMON_APPLY_BONETRANSFORMS1, "#define APPLY_BONETRANSFORMS 1\n", "_bones1" },

	{ GLSL_COMMON_APPLY_RGB_GEN_ONE_MINUS_VERTEX, "#define APPLY_RGB_ONE_MINUS_VERTEX\n", "_c1-v" },
	{ GLSL_COMMON_APPLY_RGB_GEN_CONST, "#define APPLY_RGB_CONST\n", "_cc" },
	{ GLSL_COMMON_APPLY_RGB_GEN_VERTEX, "#define APPLY_RGB_VERTEX\n", "_cv" },

	{ GLSL_COMMON_APPLY_ALPHA_GEN_ONE_MINUS_VERTEX, "#define APPLY_ALPHA_ONE_MINUS_VERTEX\n", "_a1-v" },
	{ GLSL_COMMON_APPLY_ALPHA_GEN_VERTEX, "#define APPLY_ALPHA_VERTEX\n", "_av" },
	{ GLSL_COMMON_APPLY_ALPHA_GEN_CONST, "#define APPLY_ALPHA_CONST\n", "_ac" },

	{ GLSL_COMMON_APPLY_FOG, "#define APPLY_FOG\n", "_fog" },
	{ GLSL_COMMON_APPLY_FOG2, "#define APPLY_FOG\n#define APPLY_FOG2\n", "2" },
	{ GLSL_COMMON_APPLY_COLOR_FOG_ALPHA, "#define APPLY_COLOR_FOG_ALPHA\n", "_afog" },

	{ GLSL_CELLSHADE_APPLY_DIFFUSE, "#define APPLY_DIFFUSE\n", "_diff" },
	{ GLSL_CELLSHADE_APPLY_DECAL, "#define APPLY_DECAL\n", "_decal" },
	{ GLSL_CELLSHADE_APPLY_DECAL_ADD, "#define APPLY_DECAL_ADD\n", "_decal" },
	{ GLSL_CELLSHADE_APPLY_ENTITY_DECAL, "#define APPLY_ENTITY_DECAL\n", "_edecal" },
	{ GLSL_CELLSHADE_APPLY_ENTITY_DECAL_ADD, "#define APPLY_ENTITY_DECAL\n#define APPLY_ENTITY_DECAL_ADD\n", "_add" },
	{ GLSL_CELLSHADE_APPLY_STRIPES, "#define APPLY_STRIPES\n", "_stripes" },
	{ GLSL_CELLSHADE_APPLY_STRIPES_ADD, "#define APPLY_STRIPES_ADD\n", "_stripes_add" },
	{ GLSL_CELLSHADE_APPLY_CELL_LIGHT, "#define APPLY_CELL_LIGHT\n", "_light" },
	{ GLSL_CELLSHADE_APPLY_CELL_LIGHT_ADD, "#define APPLY_CELL_LIGHT\n#define APPLY_CELL_LIGHT_ADD\n", "_add" },

	{ 0, NULL, NULL }
};

static const glsl_feature_t * const glsl_programtypes_features[] =
{
	// GLSL_PROGRAM_TYPE_NONE
	NULL,
	// GLSL_PROGRAM_TYPE_MATERIAL
	glsl_features_materal,
	// GLSL_PROGRAM_TYPE_DISTORTION
	glsl_features_distortion,
	// GLSL_PROGRAM_TYPE_SHADOWMAP
	glsl_features_shadowmap,
	// GLSL_PROGRAM_TYPE_OUTLINE
	glsl_features_outline,
	// GLSL_PROGRAM_TYPE_TURBULENCE
	glsl_features_standard,
	// GLSL_PROGRAM_TYPE_DYNAMIC_LIGHTS
	glsl_features_dynamiclights,
	// GLSL_PROGRAM_TYPE_Q3A_SHADER
	glsl_features_q3a,
	// GLSL_PROGRAM_TYPE_PLANAR_SHADOW
	glsl_features_planar_shadow,
	// GLSL_PROGRAM_TYPE_CELLSHADE
	glsl_features_cellshade
};

// ======================================================================================

#ifndef STR_HELPER
#define STR_HELPER( s )					# s
#define STR_TOSTR( x )					STR_HELPER( x )
#endif

#define MYHALFTYPES "" \
"#if !defined(myhalf)\n" \
"//#if !defined(__GLSL_CG_DATA_TYPES)\n" \
"#define myhalf float\n" \
"#define myhalf2 vec2\n" \
"#define myhalf3 vec3\n" \
"#define myhalf4 vec4\n" \
"//#else\n" \
"//#define myhalf half\n" \
"//#define myhalf2 half2\n" \
"//#define myhalf3 half3\n" \
"//#define myhalf4 half4\n" \
"//#endif\n" \
"#endif\n"

#define MYTWOPI "" \
"#ifndef M_TWOPI\n" \
"#define M_TWOPI 6.28318530717958647692\n" \
"#endif\n"

static const char r_GLSLWaveFuncs[] =
"\n"
MYTWOPI
"\n"
"float WaveFunc_Sin(float x)\n"
"{\n"
"x -= floor(x);\n"
"return sin(x * M_TWOPI);\n"
"}\n"
"float WaveFunc_Triangle(float x)\n"
"{\n"
"x -= floor(x);\n"
"return step(x, 0.25) * x * 4.0 + (2.0 - 4.0 * step(0.25, x) * step(x, 0.75) * x) + ((step(0.75, x) * x - 0.75) * 4.0 - 1.0);\n"
"}\n"
"float WaveFunc_Square(float x)\n"
"{\n"
"x -= floor(x);\n"
"return step(x, 0.5) * 2.0 - 1.0;\n"
"}\n"
"float WaveFunc_Sawtooth(float x)\n"
"{\n"
"x -= floor(x);\n"
"return x;\n"
"}\n"
"float WaveFunc_InverseSawtooth(float x)\n"
"{\n"
"x -= floor(x);\n"
"return 1.0 - x;\n"
"}\n"
"\n"
"#define WAVE_SIN(time,base,amplitude,phase,freq) (((base)+(amplitude)*WaveFunc_Sin((phase)+(time)*(freq))))\n"
"#define WAVE_TRIANGLE(time,base,amplitude,phase,freq) (((base)+(amplitude)*WaveFunc_Triangle((phase)+(time)*(freq))))\n"
"#define WAVE_SQUARE(time,base,amplitude,phase,freq) (((base)+(amplitude)*WaveFunc_Square((phase)+(time)*(freq))))\n"
"#define WAVE_SAWTOOTH(time,base,amplitude,phase,freq) (((base)+(amplitude)*WaveFunc_Sawtooth((phase)+(time)*(freq))))\n"
"#define WAVE_INVERSESAWTOOTH(time,base,amplitude,phase,freq) (((base)+(amplitude)*WaveFunc_InverseSawtooth((phase)+(time)*(freq))))\n"
"\n"
;

#define GLSL_DUAL_QUAT_TRANSFORM_OVERLOAD "" \
"#if defined(DUAL_QUAT_TRANSFORM_NORMALS)\n" \
"#if defined(DUAL_QUAT_TRANSFORM_TANGENT)\n" \
"void VertexDualQuatsTransform(const int numWeights, in vec4 Indices, in vec4 Weights, inout vec4 Position, inout vec3 Normal, inout vec3 Tangent)\n" \
"#else\n" \
"void VertexDualQuatsTransform(const int numWeights, vec4 Indices, vec4 Weights, inout vec4 Position, inout vec3 Normal)\n" \
"#endif\n" \
"#else\n" \
"void VertexDualQuatsTransform(const int numWeights, vec4 Indices, vec4 Weights, inout vec4 Position)\n" \
"#endif\n" \
"{\n" \
"int index;\n" \
"vec4 Indices_2 = Indices * 2.0;\n" \
"vec4 DQReal, DQDual;\n" \
"\n" \
"index = int(Indices_2.x);\n" \
"DQReal = DualQuats[index+0];\n" \
"DQDual = DualQuats[index+1];\n" \
"\n" \
"if (numWeights > 1)\n" \
"{\n" \
"DQReal *= Weights.x;\n" \
"DQDual *= Weights.x;\n" \
"\n" \
"vec4 DQReal1, DQDual1;\n" \
"float scale;\n" \
"\n" \
"index = int(Indices_2.y);\n" \
"DQReal1 = DualQuats[index+0];\n" \
"DQDual1 = DualQuats[index+1];\n" \
"// antipodality handling\n" \
"scale = (dot(DQReal1, DQReal) < 0.0 ? -1.0 : 1.0) * Weights.y;\n" \
"DQReal += DQReal1 * scale;\n" \
"DQDual += DQDual1 * scale;\n" \
"\n" \
"if (numWeights > 2)\n" \
"{\n" \
"index = int(Indices_2.z);\n" \
"DQReal1 = DualQuats[index+0];\n" \
"DQDual1 = DualQuats[index+1];\n" \
"// antipodality handling\n" \
"scale = (dot(DQReal1, DQReal) < 0.0 ? -1.0 : 1.0) * Weights.z;\n" \
"DQReal += DQReal1 * scale;\n" \
"DQDual += DQDual1 * scale;\n" \
"\n" \
"if (numWeights > 3)\n" \
"{\n" \
"index = int(Indices_2.w);\n" \
"DQReal1 = DualQuats[index+0];\n" \
"DQDual1 = DualQuats[index+1];\n" \
"// antipodality handling\n" \
"scale = (dot(DQReal1, DQReal) < 0.0 ? -1.0 : 1.0) * Weights.w;\n" \
"DQReal += DQReal1 * scale;\n" \
"DQDual += DQDual1 * scale;\n" \
"}\n" \
"}\n" \
"}\n" \
"\n" \
"float len = length(DQReal);\n" \
"DQReal /= len;\n" \
"DQDual /= len;\n" \
"\n" \
"Position.xyz = (cross(DQReal.xyz, cross(DQReal.xyz, Position.xyz) + Position.xyz*DQReal.w + DQDual.xyz) + DQDual.xyz*DQReal.w - DQReal.xyz*DQDual.w)*2.0 + Position.xyz;\n" \
"\n" \
"#ifdef DUAL_QUAT_TRANSFORM_NORMALS\n" \
"Normal = cross(DQReal.xyz, cross(DQReal.xyz, Normal) + Normal*DQReal.w)*2.0 + Normal;\n" \
"#endif\n" \
"\n" \
"#ifdef DUAL_QUAT_TRANSFORM_TANGENT\n" \
"Tangent = cross(DQReal.xyz, cross(DQReal.xyz, Tangent) + Tangent*DQReal.w)*2.0 + Tangent;\n" \
"#endif\n" \
"}\n"

// FIXME: version 140 and up?
#define GLSL_DUAL_QUAT_TRANSFORMS \
"\n" \
"#ifdef VERTEX_SHADER\n" \
"attribute vec4 BonesIndices;\n" \
"attribute vec4 BonesWeights;\n" \
"\n" \
"uniform vec4 DualQuats[MAX_GLSL_BONES*2];\n" \
"\n" \
GLSL_DUAL_QUAT_TRANSFORM_OVERLOAD \
"\n" \
"// use defines to overload the transform function\n" \
"\n" \
"#define DUAL_QUAT_TRANSFORM_NORMALS\n" \
GLSL_DUAL_QUAT_TRANSFORM_OVERLOAD \
"\n" \
"#define DUAL_QUAT_TRANSFORM_TANGENT\n" \
GLSL_DUAL_QUAT_TRANSFORM_OVERLOAD \
"\n" \
"#endif\n" \

#define GLSL_RGBGEN \
"myhalf4 QF_VertexRGBGen(in myhalf4 ConstColor, in myhalf4 VertexColor)\n" \
"{\n" \
"#if defined(APPLY_RGB_CONST) && defined(APPLY_ALPHA_CONST)\n" \
"myhalf4 Color = ConstColor;\n" \
"#else\n" \
"myhalf4 Color = VertexColor;\n" \
"\n" \
"#if defined(APPLY_RGB_CONST)\n" \
"Color.rgb = myhalf3(ConstColor.r, ConstColor.g, ConstColor.b);\n" \
"#elif defined(APPLY_RGB_VERTEX)\n" \
"#if defined(APPLY_OVERBRIGHT_SCALING)\n" \
"Color.rgb = myhalf3(VertexColor.r * OverbrightScale, VertexColor.g * OverbrightScale, VertexColor.b * OverbrightScale);\n" \
"#else\n" \
"//Color.rgb = Color.rgb;\n" \
"#endif\n" \
"#elif defined(APPLY_RGB_ONE_MINUS_VERTEX)\n" \
"#if defined(APPLY_OVERBRIGHT_SCALING)\n" \
"Color.rgb = myhalf3(1.0 - VertexColor.r * OverbrightScale, 1.0 - VertexColor.g * OverbrightScale, 1.0 - VertexColor.b * OverbrightScale);\n" \
"#else\n" \
"Color.rgb = myhalf3(1.0 - VertexColor.r, 1.0 - VertexColor.g, 1.0 - VertexColor.b);\n" \
"#endif\n" \
"#endif\n" \
"\n" \
"#if defined(APPLY_ALPHA_CONST)\n" \
"Color.a = ConstColor.a;\n" \
"#elif defined(APPLY_ALPHA_VERTEX)\n" \
"//Color.a = VertexColor.a;\n" \
"#elif defined(APPLY_ALPHA_ONE_MINUS_VERTEX)\n" \
"Color.a = 1.0 - VertexColor.a;\n" \
"#endif\n" \
"\n" \
"#endif\n" \
"\n" \
"return Color;\n" \
"}\n" \
"\n"

#define GLSL_FOGGEN_OVERLOAD \
"#if defined(FOG_GEN_OUTPUT_COLOR) && defined(FOG_GEN_OUTPUT_TEXCOORDS)\n" \
"void QF_FogGen(in vec4 Position, inout myhalf4 outColor, inout vec2 outTexCoord)\n" \
"#elif defined(FOG_GEN_OUTPUT_COLOR)\n" \
"void QF_FogGen(in vec4 Position, inout myhalf4 outColor)\n" \
"#elif defined(FOG_GEN_OUTPUT_TEXCOORDS)\n" \
"void QF_FogGen(in vec4 Position, inout vec2 outTexCoord)\n" \
"#endif\n" \
"{\n" \
"float FDist = dot(Position.xyz, EyePlane.xyz) - EyePlane.w;\n" \
"float FVdist = dot(Position.xyz, FogPlane.xyz) - FogPlane.w;\n" \
"\n" \
"#if defined(APPLY_FOG2)\n" \
"// camera is outside the fog brush\n" \
"float FogDistScale = FVdist / (FVdist - EyeFogDist);\n" \
"#endif\n" \
"\n" \
"#if defined(FOG_GEN_OUTPUT_COLOR)\n" \
"#if defined(APPLY_FOG2)\n" \
"// camera is outside the fog brush\n" \
"float FogDist = FogDistScale;\n" \
"#else\n" \
"float FogDist = FDist;\n" \
"#endif\n" \
"\n" \
"#ifdef APPLY_COLOR_FOG_ALPHA\n" \
"outColor.a *= myhalf(clamp(1.0 - FogDist * gl_Fog.scale, 0.0, 1.0));\n" \
"#else\n" \
"outColor.rgb *= myhalf(clamp(1.0 - FogDist * gl_Fog.scale, 0.0, 1.0));\n" \
"#endif\n" \
"#endif\n" \
"\n" \
"#if defined(FOG_GEN_OUTPUT_TEXCOORDS)\n" \
"\n" \
"float FogS = FDist;\n" \
"float FogT = -FVdist;\n" \
"\n" \
"#if defined(APPLY_FOG2)\n" \
"// camera is outside the fog brush\n" \
"FogS *= (1.0 - step(0.0, FVdist)) * FogDistScale;\n" \
"#endif\n" \
"\n" \
"outTexCoord = vec2(FogS * gl_Fog.scale, FogT * gl_Fog.scale + 1.5/" STR_TOSTR( FOG_TEXTURE_HEIGHT ) ".0);\n" \
"#endif\n" \
"}\n" \
"\n"

#define GLSL_FOGGEN \
"#if defined(VERTEX_SHADER) && defined(APPLY_FOG)\n" \
"uniform float EyeFogDist;\n" \
"uniform vec4 EyePlane, FogPlane;\n" \
"\n" \
"#define FOG_GEN_OUTPUT_COLOR\n" \
GLSL_FOGGEN_OVERLOAD \
"#define FOG_GEN_OUTPUT_TEXCOORDS\n" \
GLSL_FOGGEN_OVERLOAD \
"#undef FOG_GEN_OUTPUT_COLOR\n" \
GLSL_FOGGEN_OVERLOAD \
"\n" \
"#endif\n"

#define GLSL_DLIGHTS_SUMCOLOR_OVERLOAD \
"#ifdef DLIGHTS_SURFACE_NORMAL_IN\n" \
"myhalf3 DynamicLightsSummaryColor(in myhalf3 surfaceNormalModelspace)\n" \
"#else\n" \
"myhalf3 DynamicLightsSummaryColor()\n" \
"#endif\n" \
"{\n" \
"myhalf3 Color = myhalf3(0.0);\n" \
"\n" \
"for (int i = 0; i < MAX_DLIGHTS; i++)\n" \
"{\n" \
"myhalf3 STR = myhalf3(DynamicLights[i].Position - v_VertexPosition);\n" \
"myhalf length = myhalf(length(STR));\n" \
"myhalf falloff = clamp(1.0 - length / DynamicLights[i].Radius, 0.0, 1.0);\n" \
"\n" \
"falloff *= falloff;\n" \
"\n" \
"#ifdef DLIGHTS_SURFACE_NORMAL_IN\n" \
"falloff *= myhalf(max(dot(normalize(STR), surfaceNormalModelspace), 0.0));\n" \
"#endif\n" \
"\n" \
"Color += falloff * DynamicLights[i].Diffuse;\n" \
"}\n" \
"\n" \
"return Color;\n" \
"}\n" \

#define GLSL_DLIGHTS \
"#if defined(MAX_DLIGHTS)\n" \
"# if defined(FRAGMENT_SHADER)\n" \
"struct DynamicLight\n" \
"{\n" \
"float Radius;\n" \
"vec3 Position;\n" \
"myhalf3 Diffuse;\n" \
"};\n" \
"\n" \
"uniform DynamicLight DynamicLights[MAX_DLIGHTS];\n" \
"\n" \
GLSL_DLIGHTS_SUMCOLOR_OVERLOAD \
"\n" \
"#define DLIGHTS_SURFACE_NORMAL_IN\n" \
GLSL_DLIGHTS_SUMCOLOR_OVERLOAD \
"\n" \
"# endif\n" \
"#endif\n"

static const char *r_defaultMaterialProgram =
"// " APPLICATION " default GLSL shader\n"
"\n"
MYHALFTYPES
"\n"
"uniform float ShaderTime;\n"
"\n"
"uniform vec3 EyeOrigin;\n"
"\n"
"#ifdef APPLY_DIRECTIONAL_LIGHT\n"
"uniform vec3 LightDir;\n"
"#endif\n"
"\n"
"varying vec2 TexCoord;\n"
"#ifdef APPLY_LIGHTSTYLE0\n"
"varying vec4 LightmapTexCoord01;\n"
"#ifdef APPLY_LIGHTSTYLE2\n"
"varying vec4 LightmapTexCoord23;\n"
"#endif\n"
"#endif\n"
"\n"
"varying vec3 v_VertexPosition;\n"
"\n"
"#if defined(APPLY_SPECULAR) || defined(APPLY_OFFSETMAPPING) || defined(APPLY_RELIEFMAPPING)\n"
"varying vec3 EyeVector;\n"
"#endif\n"
"\n"
"varying mat3 strMatrix; // directions of S/T/R texcoords (tangent, binormal, normal)\n"
"\n"
GLSL_DLIGHTS
"\n"
"#ifdef VERTEX_SHADER\n"
"// Vertex shader\n"
"\n"
"#ifdef APPLY_OVERBRIGHT_SCALING\n"
"uniform myhalf OverbrightScale;\n"
"#endif\n"
"\n"
"uniform myhalf4 ConstColor;\n"
"\n"
"#ifdef APPLY_BONETRANSFORMS\n"
GLSL_DUAL_QUAT_TRANSFORMS
"#endif\n"
"\n"
GLSL_RGBGEN
"\n"
GLSL_FOGGEN
"\n"
"void main()\n"
"{\n"
"\n"
"vec4 Position = gl_Vertex;\n"
"vec3 Normal = gl_Normal;\n"
"myhalf4 inColor = myhalf4(gl_Color);\n"
"vec4 TexCoord0 = gl_MultiTexCoord0;\n"
"vec3 Tangent = gl_MultiTexCoord1.xyz;\n"
"float TangentDir = gl_MultiTexCoord1.w;\n"
"\n"
"#ifdef APPLY_BONETRANSFORMS\n"
"VertexDualQuatsTransform(APPLY_BONETRANSFORMS, BonesIndices, BonesWeights, Position, Normal, Tangent);\n"
"#endif\n"
"\n"
"#ifdef APPLY_DEFORMVERTS\n"
"Position = APPLY_DEFORMVERTS(Position, Normal, TexCoord0.st, ShaderTime);\n"
"#endif\n"
"\n"
"myhalf4 outColor = QF_VertexRGBGen(ConstColor, inColor);\n"
"\n"
"#ifdef APPLY_FOG\n"
"QF_FogGen(Position, outColor);\n"
"#endif\n"
"\n"
"gl_FrontColor = vec4(outColor);\n"
"\n"
"TexCoord = vec2(gl_TextureMatrix[0] * TexCoord0);\n"
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
"strMatrix[0] = Tangent;\n"
"strMatrix[2] = Normal;\n"
"strMatrix[1] = TangentDir * cross (Normal, Tangent);\n"
"\n"
"#if defined(APPLY_SPECULAR) || defined(APPLY_OFFSETMAPPING) || defined(APPLY_RELIEFMAPPING)\n"
"vec3 EyeVectorWorld = EyeOrigin - Position.xyz;\n"
"EyeVector = EyeVectorWorld * strMatrix;\n"
"#endif\n"
"\n"
"v_VertexPosition = Position.xyz;\n"
"\n"
"gl_Position = gl_ModelViewProjectionMatrix * Position;\n"
"\n"
"#ifdef APPLY_CLIPPING\n"
"#ifdef __GLSL_CG_DATA_TYPES\n"
"gl_ClipVertex = gl_ModelViewMatrix * Position;\n"
"#endif\n"
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
"#ifdef APPLY_ENTITY_DECAL\n"
"uniform myhalf4 EntityColor;\n"
"uniform sampler2D EntityDecalTexture;\n"
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
"#ifdef APPLY_DRAWFLAT\n"
"uniform myhalf3 WallColor;\n"
"uniform myhalf3 FloorColor;\n"
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
"void main()\n"
"{\n"
"#if defined(APPLY_OFFSETMAPPING) || defined(APPLY_RELIEFMAPPING)\n"
"// apply offsetmapping\n"
"vec2 TexCoordOffset = OffsetMapping(TexCoord);\n"
"#define TexCoord TexCoordOffset\n"
"#endif\n"
"myhalf3 surfaceNormal;\n"
"myhalf3 surfaceNormalModelspace;\n"
"myhalf3 diffuseNormalModelspace;\n"
"float diffuseProduct;\n"
"\n"
"#ifdef APPLY_CELLSHADING\n"
"int lightcell;\n"
"float diffuseProductPositive;\n"
"float diffuseProductNegative;\n"
"float hardShadow;\n"
"#endif\n"
"\n"
"myhalf3 weightedDiffuseNormalModelspace;\n"
"\n"
"#if !defined(APPLY_DIRECTIONAL_LIGHT) && !defined(APPLY_LIGHTSTYLE0)\n"
"myhalf4 color = myhalf4 (1.0, 1.0, 1.0, 1.0);\n"
"#else\n"
"myhalf4 color = myhalf4 (0.0, 0.0, 0.0, 1.0);\n"
"#endif\n"
"\n"
"myhalf4 decal = myhalf4 (0.0, 0.0, 0.0, 1.0);\n"
"\n"
"// get the surface normal\n"
"surfaceNormal = normalize(myhalf3(texture2D (NormalmapTexture, TexCoord)) - myhalf3 (0.5));\n"
"surfaceNormalModelspace = normalize(strMatrix * surfaceNormal);\n"
"\n"
"#ifdef APPLY_DIRECTIONAL_LIGHT\n"
"diffuseNormalModelspace = LightDir;\n"
"weightedDiffuseNormalModelspace = diffuseNormalModelspace;\n"
"\n"
"#ifdef APPLY_CELLSHADING\n"
"hardShadow = 0.0;\n"
"#ifdef APPLY_HALFLAMBERT\n"
"diffuseProduct = float (dot (surfaceNormalModelspace, diffuseNormalModelspace));\n"
"diffuseProductPositive = float ( clamp(diffuseProduct, 0.0, 1.0) * 0.5 + 0.5 );\n"
"diffuseProductPositive *= diffuseProductPositive;\n"
"diffuseProductNegative = float ( clamp(diffuseProduct, -1.0, 0.0) * 0.5 - 0.5 );\n"
"diffuseProductNegative *= diffuseProductNegative;\n"
"diffuseProductNegative -= 0.25;\n"
"diffuseProduct = diffuseProductPositive;\n"
"#else\n"
"diffuseProduct = float (dot (surfaceNormalModelspace, diffuseNormalModelspace));\n"
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
"diffuseProduct = float ( clamp(dot (surfaceNormalModelspace, diffuseNormalModelspace), 0.0, 1.0) * 0.5 + 0.5 );\n"
"diffuseProduct *= diffuseProduct;\n"
"#else\n"
"diffuseProduct = float (dot (surfaceNormalModelspace, diffuseNormalModelspace));\n"
"#endif\n"
"\n"
"#ifdef APPLY_DIRECTIONAL_LIGHT_MIX\n"
"color.rgb += gl_Color.rgb;\n"
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
"diffuseNormalModelspace = normalize(myhalf3 (texture2D(LightmapTexture0, vec2(LightmapTexCoord01.s+DeluxemapOffset0,LightmapTexCoord01.t))) - myhalf3 (0.5));\n"
"// calculate directional shading\n"
"diffuseProduct = float (dot (surfaceNormalModelspace, diffuseNormalModelspace));\n"
"\n"
"#ifdef APPLY_FBLIGHTMAP\n"
"weightedDiffuseNormalModelspace = diffuseNormalModelspace;\n"
"// apply lightmap color\n"
"color.rgb += myhalf3 (max (diffuseProduct, 0.0) * myhalf3 (texture2D (LightmapTexture0, LightmapTexCoord01.st)));\n"
"#else\n"
"\n"
"#define NORMALIZE_DIFFUSE_NORMAL\n"
"\n"
"weightedDiffuseNormalModelspace = lsColor0 * diffuseNormalModelspace;\n"
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
"diffuseNormalModelspace = normalize(myhalf3 (texture2D (LightmapTexture1, vec2(LightmapTexCoord01.p+DeluxemapOffset1,LightmapTexCoord01.q))) - myhalf3 (0.5));\n"
"diffuseProduct = float (dot (surfaceNormalModelspace, diffuseNormalModelspace));\n"
"weightedDiffuseNormalModelspace += lsColor1 * diffuseNormalModelspace;\n"
"color.rgb += lsColor1 * myhalf(max (diffuseProduct, 0.0)) * myhalf3 (texture2D (LightmapTexture1, LightmapTexCoord01.pq));\n"
"\n"
"#ifdef APPLY_LIGHTSTYLE2\n"
"diffuseNormalModelspace = normalize(myhalf3 (texture2D (LightmapTexture2, vec2(LightmapTexCoord23.s+DeluxemapOffset2,LightmapTexCoord23.t))) - myhalf3 (0.5));\n"
"diffuseProduct = float (dot (surfaceNormalModelspace, diffuseNormalModelspace));\n"
"weightedDiffuseNormalModelspace += lsColor2 * diffuseNormalModelspace;\n"
"color.rgb += lsColor2 * myhalf(max (diffuseProduct, 0.0)) * myhalf3 (texture2D (LightmapTexture2, LightmapTexCoord23.st));\n"
"\n"
"#ifdef APPLY_LIGHTSTYLE3\n"
"diffuseNormalModelspace = normalize(myhalf3 (texture2D (LightmapTexture3, vec2(LightmapTexCoord23.p+DeluxemapOffset3,LightmapTexCoord23.q))) - myhalf3 (0.5));\n"
"diffuseProduct = float (dot (surfaceNormalModelspace, diffuseNormalModelspace));\n"
"weightedDiffuseNormalModelspace += lsColor3 * diffuseNormalModelspace;\n"
"color.rgb += lsColor3 * myhalf(max (diffuseProduct, 0.0)) * myhalf3 (texture2D (LightmapTexture3, LightmapTexCoord23.pq));\n"
"\n"
"#endif\n"
"#endif\n"
"#endif\n"
"#endif\n"
"\n"
"#if defined(MAX_DLIGHTS)\n"
"color.rgb += DynamicLightsSummaryColor(surfaceNormalModelspace);\n"
"#endif\n"
"\n"
"#ifdef APPLY_SPECULAR\n"
"\n"
"#ifdef NORMALIZE_DIFFUSE_NORMAL\n"
"myhalf3 specularNormal = normalize (myhalf3 (normalize (weightedDiffuseNormalModelspace)) + myhalf3 (normalize (EyeOrigin - v_VertexPosition)));\n"
"#else\n"
"myhalf3 specularNormal = normalize (weightedDiffuseNormalModelspace + myhalf3 (normalize (EyeOrigin - v_VertexPosition)));\n"
"#endif\n"
"\n"
"myhalf specularProduct = myhalf(dot (surfaceNormalModelspace, specularNormal));\n"
"color.rgb += (myhalf3(texture2D(GlossTexture, TexCoord)) * GlossIntensity) * pow(myhalf(max(specularProduct, 0.0)), GlossExponent);\n"
"#endif\n"
"\n"
"#if defined(APPLY_BASETEX_ALPHA_ONLY) && !defined(APPLY_DRAWFLAT)\n"
"color = min(color, myhalf4(texture2D(BaseTexture, TexCoord).a));\n"
"#else\n"
"myhalf4 diffuse;\n"
"#ifdef APPLY_DRAWFLAT\n"
"myhalf n = myhalf(step(" STR_TOSTR( DRAWFLAT_NORMAL_STEP ) ", abs(strMatrix[2].z)));\n"
"diffuse = myhalf4((n * FloorColor + (1.0 - n) * WallColor), myhalf(texture2D(BaseTexture, TexCoord).a));\n"
"#else\n"
"diffuse = myhalf4(texture2D(BaseTexture, TexCoord));\n"
"#endif\n"
"\n"
"#ifdef APPLY_ENTITY_DECAL\n"
"#ifdef APPLY_ENTITY_DECAL_ADD\n"
"decal.rgb = myhalf3(texture2D(EntityDecalTexture, TexCoord));\n"
"diffuse.rgb += EntityColor.rgb * decal.rgb;\n"
"#else\n"
"decal = myhalf4(EntityColor.rgb, 1.0) * myhalf4(texture2D(EntityDecalTexture, TexCoord));\n"
"diffuse.rgb = mix(diffuse.rgb, decal.rgb, decal.a);\n"
"#endif\n"
"#endif\n"
"\n"
"color = color * diffuse;\n"
"#endif\n"
"\n"
"#ifdef APPLY_DECAL\n"
"#ifdef APPLY_DECAL_ADD\n"
"decal.rgb = myhalf3(gl_Color.rgb) * myhalf3(texture2D(DecalTexture, TexCoord));\n"
"color.rgb = decal.rgb + color.rgb;\n"
"color.a = color.a * myhalf(gl_Color.a);\n"
"#else\n"
"decal = gl_Color;\n"
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
"color = color * myhalf4(gl_Color);\n"
"#endif\n"
"#endif\n"
"\n"
"#ifdef APPLY_GRAYSCALE\n"
"myhalf grey = dot(color, myhalf3(0.299, 0.587, 0.114));\n"
"color.rgb = myhalf3(grey);\n"
"#endif\n"
"\n"
"gl_FragColor = vec4(color);\n"
"}\n"
"\n"
"#endif // FRAGMENT_SHADER\n"
"\n";

static const char *r_defaultDistortionGLSLProgram =
"// " APPLICATION " distortion GLSL shader\n"
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
"ProjVector = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
"gl_Position = ProjVector;\n"
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
"uniform vec4 TextureParams;\n"
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
"float inv2NW = TextureParams.z * 0.5; // .z - inverse width\n"
"float inv2NH = TextureParams.w * 0.5; // .w - inverse height\n"
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
"// " APPLICATION " shadowmap GLSL shader\n"
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
"\n"
"vec4 Position = gl_Vertex;\n"
"\n"
"mat4 textureMatrix;\n"
"textureMatrix = gl_TextureMatrix[0];\n"
"\n"
"gl_Position = gl_ModelViewProjectionMatrix * Position;\n"
"ProjVector = textureMatrix * Position;\n"
"// a trick whish allows us not to perform the\n"
"// 'shadowmaptc = (shadowmaptc + vec3 (1.0)) * vec3 (0.5)'\n"
"// computation in the fragment shader\n"
"ProjVector.xyz = (ProjVector.xyz + vec3(ProjVector.w)) * 0.5;\n"
"\n"
"NormalDot = dot(gl_Normal, LightDir);\n"
"\n"
"gl_FrontColor = gl_Color;\n"
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
"uniform vec4 TextureParams;\n"
"uniform float ProjDistance;\n"
"\n"
"uniform sampler2DShadow ShadowmapTexture;\n"
"\n"
"void main(void)\n"
"{\n"
"myhalf color = myhalf(1.0);\n"
"\n"
"myhalf d = step(ProjVector.w, 32.0) + step(ProjDistance, ProjVector.w);\n"
"\n"
"\n"
"// most of what follows was written by eihrul\n"
"\n"
"float dtW = TextureParams.z; // .x - inverse texture width\n"
"float dtH = TextureParams.w; // .w - inverse texture height\n"
"\n"
"vec3 shadowmaptc = vec3 (ProjVector.xyz / ProjVector.w);\n"
"//shadowmaptc = (shadowmaptc + vec3 (1.0)) * vec3 (0.5);\n"
"shadowmaptc.x = shadowmaptc.s * TextureParams.x; // .x - texture width\n"
"shadowmaptc.y = shadowmaptc.t * TextureParams.y; // .y - texture height\n"
"shadowmaptc.z = clamp(shadowmaptc.r, 0.0, 1.0);\n"
"shadowmaptc.xy = vec2(clamp(shadowmaptc.x, dtW, TextureParams.x - dtW), clamp(shadowmaptc.y, dtH, TextureParams.y - dtH));\n"
"\n"
"vec2 ShadowMap_TextureScale = TextureParams.zw;\n"
"\n"
"myhalf f;\n"
"\n"
"#ifdef APPLY_DITHER\n"
"# ifdef APPLY_PCF\n"
"#  define texval(x, y) myhalf(shadow2D(ShadowmapTexture, vec3(center + vec2(x, y)*ShadowMap_TextureScale, shadowmaptc.z)).r)\n"
"\n"
"// this method can be described as a 'dithered pinwheel' (4 texture lookups)\n"
"// which is a combination of the 'pinwheel' filter suggested by eihrul and dithered 4x4 PCF,\n"
"// described here: http://http.developer.nvidia.com/GPUGems/gpugems_ch11.html \n"
"\n"
"vec2 offset_dither = mod(floor(gl_FragCoord.xy), 2.0);\n"
"offset_dither.y += offset_dither.x;  // y ^= x in floating point\n"
"offset_dither.y *= step(offset_dither.y, 1.1);\n"
"\n"
"vec2 center = (shadowmaptc.xy + offset_dither.xy) * ShadowMap_TextureScale;\n"
"myhalf group1 = texval(-0.4,  1.0);\n"
"myhalf group2 = texval(-1.0, -0.4);\n"
"myhalf group3 = texval( 0.4, -1.0);\n"
"myhalf group4 = texval( 1.0,  0.4);\n"
"\n"
"f = dot(myhalf4(0.25), myhalf4(group1, group2, group3, group4));\n"
"# else\n"
"f = myhalf(shadow2D(ShadowmapTexture, vec3(shadowmaptc.xy*ShadowMap_TextureScale, shadowmaptc.z)).r);\n"
"# endif\n"
"#else\n"
"// an essay by eihrul:\n"
"// now think of bilinear filtering as a 1x1 weighted box filter\n"
"// that is, it's sampling over a 2x2 area, but only collecting the portion of each pixel it actually steps on\n"
"// with a linear shadowmap filter, you are getting that, like normal bilinear sampling\n"
"// only its doing the shadowmap test on each pixel first, to generate a new little 2x2 area, then its doing\n"
"// the bilinear filtering on that\n"
"// so now if you consider your 2x2 filter you have\n"
"// each of those taps is actually using linear filtering as you've configured it\n"
"// so you are literally sampling almost 16 pixels as is and all you are getting for it is 2x2\n"
"// the trick is to realize that in essence you could instead be sampling a 4x4 area of pixels\n"
"// and running a 3x3 weighted box filter on it\n"
"// but you would need some way to get the shadowmap to simply return the 4 pixels covered by each\n"
"// tap, rather than the filtered result\n"
"// which is what the ARB_texture_gather extension is for\n"
"// NOTE: we're using emulation of texture_gather now\n"
"\n"
"# ifdef APPLY_PCF\n"
"# define texval(off) shadow2D(ShadowmapTexture, vec3(off,shadowmaptc.z)).r\n"
"vec2 offset = fract(shadowmaptc.xy - 0.5);\n\n"
"vec4 size = vec4(offset + 1.0, 2.0 - offset), weight = (vec4(2.0 - 1.0 / size.xy, 1.0 / size.zw - 1.0) + (shadowmaptc.xy - offset).xyxy)*ShadowMap_TextureScale.xyxy;\n"
"f = (1.0/9.0)*dot(size.zxzx*size.wwyy, vec4(texval(weight.zw), texval(weight.xw), texval(weight.zy), texval(weight.xy)));\n"
"# else\n"
"f = shadow2D(ShadowmapTexture, vec3(shadowmaptc.xy*ShadowMap_TextureScale, shadowmaptc.z)).r;\n"
"# endif\n"
"#endif\n"
"\n"
"myhalf attenuation = myhalf(float (ProjVector.w) / ProjDistance);\n"
"myhalf compensation = myhalf(0.25) - max(LightAmbient.x, max(LightAmbient.y, LightAmbient.z))\n;"
"compensation = max (compensation, 0.0);\n"
"f = f + attenuation + compensation;\n"
"\n"
"gl_FragColor = vec4(vec3(max(f,clamp(d, 0.0, 1.0))),1.0);\n"
"}\n"
"\n"
"#endif // FRAGMENT_SHADER\n"
"\n";

#ifdef HARDWARE_OUTLINES
static const char *r_defaultOutlineGLSLProgram =
"// " APPLICATION " outline GLSL shader\n"
"\n"
"\n"
"#ifdef VERTEX_SHADER\n"
"// Vertex shader\n"
"\n"
"uniform float OutlineHeight;\n"
"\n"
"#ifdef APPLY_BONETRANSFORMS\n"
GLSL_DUAL_QUAT_TRANSFORMS
"#endif\n"
"\n"
"void main(void)\n"
"{\n"
"vec4 Position = gl_Vertex;\n"
"vec3 Normal = gl_Normal;\n"
"\n"
"#ifdef APPLY_BONETRANSFORMS\n"
"VertexDualQuatsTransform(APPLY_BONETRANSFORMS, BonesIndices, BonesWeights, Position, Normal);\n"
"#endif\n"
"\n"
"Position += vec4(Normal * OutlineHeight, 0.0);\n"
"gl_Position = gl_ModelViewProjectionMatrix * Position;\n"
"#ifdef APPLY_CLIPPING\n"
"#ifdef __GLSL_CG_DATA_TYPES\n"
"gl_ClipVertex = gl_ModelViewMatrix * Position;\n"
"#endif\n"
"#endif\n"
"\n"
"gl_FrontColor = gl_Color;\n"
"\n"
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
"// " APPLICATION " turbulence GLSL shader\n"
"\n"
MYHALFTYPES
"\n"
MYTWOPI
"\n"
"uniform float ShaderTime;\n"
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
"\n"
"vec4 Position = gl_Vertex;\n"
"vec3 Normal = gl_Normal;\n"
"vec4 TexCoord0 = gl_MultiTexCoord0;\n"
"\n"
"#ifdef APPLY_DEFORMVERTS\n"
"Position = APPLY_DEFORMVERTS(Position, Normal, TexCoord0.st, ShaderTime);\n"
"#endif\n"
"\n"
"gl_FrontColor = gl_Color;\n"
"\n"
"vec4 turb;\n"
"turb = TexCoord0;\n"
"turb.s += TurbAmplitude * sin( ((TexCoord0.t / 4.0 + TurbPhase)) * M_TWOPI );\n"
"turb.t += TurbAmplitude * sin( ((TexCoord0.s / 4.0 + TurbPhase)) * M_TWOPI );\n"
"TexCoord = vec2(gl_TextureMatrix[0] * turb);\n"
"\n"
"gl_Position = gl_ModelViewProjectionMatrix * Position;\n"
"#ifdef APPLY_CLIPPING\n"
"#ifdef __GLSL_CG_DATA_TYPES\n"
"gl_ClipVertex = gl_ModelViewMatrix * Position;\n"
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
"// " APPLICATION " dynamic lighting GLSL shader\n"
"\n"
MYHALFTYPES
"\n"
"varying vec3 v_VertexPosition;\n"
"\n"
GLSL_DLIGHTS
"\n"
"#ifdef VERTEX_SHADER\n"
"// Vertex shader\n"
"\n"
"void main(void)\n"
"{\n"
"gl_FrontColor = gl_Color;\n"
"\n"
"v_VertexPosition = gl_Vertex.xyz;\n"
"\n"
"gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
"#ifdef APPLY_CLIPPING\n"
"#ifdef __GLSL_CG_DATA_TYPES\n"
"gl_ClipVertex = gl_ModelViewMatrix * gl_Vertex;\n"
"#endif\n"
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
"void main(void)\n"
"{\n"
"\n"
"myhalf3 color = myhalf3(0.0);\n"
"\n"
"#if defined(MAX_DLIGHTS)\n"
"color.rgb += DynamicLightsSummaryColor();\n"
"#endif\n"
"\n"
"gl_FragColor = vec4(color.rgb, 1.0);\n"
"}\n"
"\n"
"#endif // FRAGMENT_SHADER\n"
"\n";

static const char *r_defaultQ3AShaderProgram =
"// " APPLICATION " Q3A GLSL shader\n"
"\n"
MYHALFTYPES
"\n"
"uniform float ShaderTime;\n"
"\n"
"varying vec3 v_VertexPosition;\n"
"\n"
"#ifdef APPLY_DRAWFLAT\n"
"varying myhalf v_flatColor;\n"
"#endif\n"
"\n"
"#ifdef APPLY_TC_GEN_REFLECTION\n"
"#define APPLY_CUBEMAP\n"
"\n"
"uniform vec3 EyeOrigin;\n"
"#endif\n"
"\n"
"#if defined(APPLY_TC_GEN_ENV) || defined(APPLY_TC_GEN_REFLECTION)\n"
"uniform vec3 EntDist;\n"
"#endif\n"
"\n"
"#ifdef APPLY_CUBEMAP\n"
"varying vec3 TexCoord;\n"
"#else\n"
"varying vec2 TexCoord;\n"
"#endif\n"
"\n"
GLSL_DLIGHTS
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
"#ifdef APPLY_BONETRANSFORMS\n"
GLSL_DUAL_QUAT_TRANSFORMS
"#endif\n"
"\n"
GLSL_RGBGEN
"\n"
GLSL_FOGGEN
"\n"
"void main(void)\n"
"{\n"
"\n"
"vec4 Position = gl_Vertex;\n"
"vec3 Normal = gl_Normal;\n"
"vec4 TexCoord0 = gl_MultiTexCoord0;\n"
"myhalf4 inColor = myhalf4(gl_Color);\n"
"\n"
"#ifdef APPLY_BONETRANSFORMS\n"
"VertexDualQuatsTransform(APPLY_BONETRANSFORMS, BonesIndices, BonesWeights, Position, Normal);\n"
"#endif\n"
"\n"
"#ifdef APPLY_DEFORMVERTS\n"
"Position = APPLY_DEFORMVERTS(Position, Normal, TexCoord0.st, ShaderTime);\n"
"#endif\n"
"\n"
"#if defined(APPLY_TC_GEN_FOG)\n"
"myhalf4 outColor = myhalf4(gl_Fog.color.rgb, 1.0);\n"
"#else\n"
"myhalf4 outColor = QF_VertexRGBGen(ConstColor, inColor);\n"
"#endif\n"
"\n"
"#ifdef APPLY_FOG\n"
"#if defined(APPLY_TC_GEN_FOG) && defined(APPLY_COLOR_FOG)\n"
"QF_FogGen(Position, outColor, TexCoord);\n"
"#elif defined(APPLY_COLOR_FOG)\n"
"QF_FogGen(Position, outColor);\n"
"#elif defined(APPLY_TC_GEN_FOG)\n"
"QF_FogGen(Position, TexCoord);\n"
"#endif\n"
"#endif\n"
"\n"
"gl_FrontColor = vec4(outColor);\n"
"\n"
"#if defined(APPLY_TC_GEN_ENV)\n"
"vec3 Projection;\n"
"\n"
"Projection = EntDist - Position.xyz;\n"
"Projection = normalize(Projection);\n"
"\n"
"float Depth = dot(Normal.xyz, Projection) * 2.0;\n"
"TexCoord = vec2(0.5 + (Normal.y * Depth - Projection.y) * 0.5, 0.5 - (Normal.z * Depth - Projection.z) * 0.5);\n"
"#elif defined(APPLY_TC_GEN_VECTOR)\n"
"TexCoord = vec2(gl_TextureMatrix[0] * vec4(dot(Position,gl_ObjectPlaneS[0]), dot(Position,gl_ObjectPlaneT[0]), dot(Position,gl_ObjectPlaneR[0]), dot(Position,gl_ObjectPlaneQ[0])));\n"
"#elif defined(APPLY_TC_GEN_REFLECTION)\n"
"TexCoord = vec3(gl_TextureMatrix[0] * vec4(reflect(normalize(Position.xyz - EntDist), Normal.xyz), 0));\n"
"#elif defined(APPLY_TC_GEN_FOG)\n"
"#else\n"
"TexCoord = vec2(gl_TextureMatrix[0] * TexCoord0);\n"
"#endif\n"
"\n"
"v_VertexPosition = Position.xyz;\n"
"\n"
"#ifdef APPLY_DRAWFLAT\n"
"v_flatColor = Normal.z;\n"
"#endif\n"
"\n"
"gl_Position = gl_ModelViewProjectionMatrix * Position;\n"
"#ifdef APPLY_CLIPPING\n"
"#ifdef __GLSL_CG_DATA_TYPES\n"
"gl_ClipVertex = gl_ModelViewMatrix * Position;\n"
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
"#ifdef APPLY_DRAWFLAT\n"
"uniform myhalf3 WallColor;\n"
"uniform myhalf3 FloorColor;\n"
"#endif\n"
"\n"
"void main(void)\n"
"{\n"
"\n"
"myhalf4 color = myhalf4(gl_Color);\n"
"\n"
"#if defined(MAX_DLIGHTS)\n"
"color.rgb += DynamicLightsSummaryColor();\n"
"#endif\n"
"\n"
"myhalf4 diffuse;\n"
"\n"
"#ifdef APPLY_CUBEMAP\n"
"diffuse = myhalf4(textureCube(BaseTexture, TexCoord));\n"
"#else\n"
"diffuse = myhalf4(texture2D(BaseTexture, TexCoord));\n"
"#endif\n"
"\n"
"#ifdef APPLY_DRAWFLAT\n"
"myhalf n = myhalf(step(" STR_TOSTR( DRAWFLAT_NORMAL_STEP ) ", abs(v_flatColor)));\n"
"diffuse.rgb *= (n * FloorColor + (1.0 - n) * WallColor);\n"
"#endif\n"
"\n"
"gl_FragColor = color * diffuse;\n"
"}\n"
"\n"
"#endif // FRAGMENT_SHADER\n"
"\n";

static const char *r_defaultPlanarShadowProgram =
"// " APPLICATION " planar shadow GLSL shader\n"
"\n"
MYHALFTYPES
"\n"
"uniform float ShaderTime;\n"
"uniform vec3 PlaneNormal;\n"
"uniform float PlaneDist;\n"
"uniform vec3 LightDir;\n"
"\n"
"\n"
"#ifdef VERTEX_SHADER\n"
"// Vertex shader\n"
"\n"
"#ifdef APPLY_BONETRANSFORMS\n"
GLSL_DUAL_QUAT_TRANSFORMS
"#endif\n"
"\n"
"void main(void)\n"
"{\n"
"\n"
"vec4 Position = gl_Vertex;\n"
"vec3 Normal = gl_Normal;\n"
"vec4 TexCoord0 = gl_MultiTexCoord0;\n"
"vec4 Color = gl_Color;\n"
"\n"
"#ifdef APPLY_BONETRANSFORMS\n"
"VertexDualQuatsTransform(APPLY_BONETRANSFORMS, BonesIndices, BonesWeights, Position, Normal);\n"
"#endif\n"
"\n"
"#ifdef APPLY_DEFORMVERTS\n"
"Position = APPLY_DEFORMVERTS(Position, Normal, TexCoord0.st, ShaderTime);\n"
"#endif\n"
"\n"
"float Dist = dot(Position.xyz, PlaneNormal) - PlaneDist;\n"
"Position.xyz += max(0.0, Dist) * LightDir;\n"
"\n"
"gl_FrontColor = vec4(Color);\n"
"\n"
"gl_Position = gl_ModelViewProjectionMatrix * Position;\n"
"#ifdef APPLY_CLIPPING\n"
"#ifdef __GLSL_CG_DATA_TYPES\n"
"gl_ClipVertex = gl_ModelViewMatrix * Position;\n"
"#endif\n"
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
"void main(void)\n"
"{\n"
"\n"
"gl_FragColor = gl_Color;\n"
"}\n"
"\n"
"#endif // FRAGMENT_SHADER\n"
"\n";

static const char *r_defaultCellshadeProgram =
"// " APPLICATION "cell shading shader\n"
"\n"
MYHALFTYPES
"\n"
"uniform float ShaderTime;\n"
"\n"
"uniform vec3 EyeOrigin;\n"
"uniform vec3 EntDist;\n"
"\n"
"varying vec2 TexCoord;\n"
"varying vec3 TexCoordCube;\n"
"\n"
"#ifdef VERTEX_SHADER\n"
"// Vertex shader\n"
"\n"
"uniform myhalf4 ConstColor;\n"
"\n"
"uniform mat4 ReflectionMatrix;\n"
"\n"
"#ifdef APPLY_OVERBRIGHT_SCALING\n"
"uniform myhalf OverbrightScale;\n"
"#endif\n"
"\n"
"\n"
"#ifdef APPLY_BONETRANSFORMS\n"
GLSL_DUAL_QUAT_TRANSFORMS
"#endif\n"
"\n"
GLSL_RGBGEN
"\n"
GLSL_FOGGEN
"\n"
"void main(void)\n"
"{\n"
"\n"
"vec4 Position = gl_Vertex;\n"
"vec3 Normal = gl_Normal;\n"
"vec4 TexCoord0 = gl_MultiTexCoord0;\n"
"myhalf4 inColor = myhalf4(gl_Color);\n"
"\n"
"#ifdef APPLY_BONETRANSFORMS\n"
"VertexDualQuatsTransform(APPLY_BONETRANSFORMS, BonesIndices, BonesWeights, Position, Normal);\n"
"#endif\n"
"\n"
"#ifdef APPLY_DEFORMVERTS\n"
"Position = APPLY_DEFORMVERTS(Position, Normal, TexCoord0.st, ShaderTime);\n"
"#endif\n"
"\n"
"myhalf4 outColor = QF_VertexRGBGen(ConstColor, inColor);\n"
"\n"
"#ifdef APPLY_FOG\n"
"QF_FogGen(Position, outColor);\n"
"#endif\n"
"\n"
"gl_FrontColor = vec4(outColor);\n"
"\n"
"TexCoord = vec2(gl_TextureMatrix[0] * TexCoord0);\n"
"TexCoordCube = vec3(ReflectionMatrix * vec4(reflect(normalize(Position.xyz - EntDist), Normal.xyz), 0));\n"
"\n"
"gl_Position = gl_ModelViewProjectionMatrix * Position;\n"
"#ifdef APPLY_CLIPPING\n"
"#ifdef __GLSL_CG_DATA_TYPES\n"
"gl_ClipVertex = gl_ModelViewMatrix * Position;\n"
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
"uniform samplerCube CellShadeTexture;\n"
"\n"
"#ifdef APPLY_DIFFUSE\n"
"uniform sampler2D DiffuseTexture;\n"
"#endif\n"
"#ifdef APPLY_DECAL\n"
"uniform sampler2D DecalTexture;\n"
"#endif\n"
"#ifdef APPLY_ENTITY_DECAL\n"
"uniform sampler2D EntityDecalTexture;\n"
"#endif\n"
"#ifdef APPLY_STRIPES\n"
"uniform sampler2D StripesTexture;\n"
"#endif\n"
"#ifdef APPLY_CELL_LIGHT\n"
"uniform samplerCube CellLightTexture;\n"
"#endif\n"
"\n"
"#if defined(APPLY_ENTITY_DECAL) || defined(APPLY_STRIPES)\n"
"uniform myhalf4 EntityColor;\n"
"#endif\n"
"\n"
"void main(void)\n"
"{\n"
"\n"
"myhalf4 inColor = myhalf4(gl_Color);\n"
"\n"
"myhalf4 tempColor;\n"
"\n"
"myhalf4 outColor;\n"
"outColor = myhalf4(texture2D(BaseTexture, TexCoord));\n"
"\n"
"#ifdef APPLY_ENTITY_DECAL\n"
"#ifdef APPLY_ENTITY_DECAL_ADD\n"
"outColor.rgb += myhalf3(EntityColor.rgb) * myhalf3(texture2D(EntityDecalTexture, TexCoord));\n"
"#else\n"
"tempColor = myhalf4(EntityColor.rgb, 1.0) * myhalf4(texture2D(EntityDecalTexture, TexCoord));\n"
"outColor.rgb = mix(outColor.rgb, tempColor.rgb, tempColor.a);\n"
"#endif\n"
"#endif\n"
"\n"
"#ifdef APPLY_DIFFUSE\n"
"outColor.rgb *= myhalf3(texture2D(DiffuseTexture, TexCoord));\n"
"#endif\n"
"\n"
"outColor.rgb *= myhalf3(textureCube(CellShadeTexture, TexCoordCube));\n"
"\n"
"#ifdef APPLY_STRIPES\n"
"#ifdef APPLY_STRIPES_ADD\n"
"outColor.rgb += myhalf3(EntityColor.rgb) * myhalf3(texture2D(StripesTexture, TexCoord));\n"
"#else\n"
"tempColor = myhalf4(EntityColor.rgb, 1.0) * myhalf4(texture2D(StripesTexture, TexCoord));\n"
"outColor.rgb = mix(outColor.rgb, tempColor.rgb, tempColor.a);\n"
"#endif\n"
"#endif\n"
"\n"
"#ifdef APPLY_CELL_LIGHT\n"
"#ifdef APPLY_CELL_LIGHT_ADD\n"
"outColor.rgb += myhalf3(textureCube(CellLightTexture, TexCoordCube));\n"
"#else\n"
"tempColor = myhalf4(textureCube(EntityDecalTexture, TexCoordCube));\n"
"outColor.rgb = mix(outColor.rgb, tempColor.rgb, tempColor.a);\n"
"#endif\n"
"#endif\n"
"\n"
"#ifdef APPLY_DECAL\n"
"#ifdef APPLY_DECAL_ADD\n"
"outColor.rgb += myhalf3(texture2D(DecalTexture, TexCoord));\n"
"#else\n"
"tempColor = myhalf4(texture2D(DecalTexture, TexCoord));\n"
"outColor.rgb = mix(outColor.rgb, tempColor.rgb, tempColor.a);\n"
"#endif\n"
"#endif\n"
"\n"
"outColor = myhalf4(inColor * outColor);\n"
"\n"
"#ifdef APPLY_GRAYSCALE\n"
"myhalf grey = dot(color, myhalf3(0.299, 0.587, 0.114));\n"
"outColor.rgb = myhalf3(grey);\n"
"#endif\n"
"\n"
"gl_FragColor = vec4(outColor);\n"
"}\n"
"\n"
"#endif // FRAGMENT_SHADER\n"
"\n";

/*
* R_GLSLBuildDeformv
* 
* Converts some of the Q3A vertex deforms to a GLSL vertex shader. 
* Supported deforms are: wave, move, bulge.
* NOTE: Autosprite deforms can only be performed in a geometry shader.
* NULL is returned in case an unsupported deform is passed.
*/
static const char *R_GLSLBuildDeformv( const deformv_t *deformv, int numDeforms )
{
	int i;
	int funcType;
	static char program[sizeof(r_GLSLWaveFuncs) + 10*1024];
	static const char * const funcs[] = {
		NULL, "WAVE_SIN", "WAVE_TRIANGLE", "WAVE_SQUARE", "WAVE_SAWTOOTH", "WAVE_INVERSESAWTOOTH", NULL
	};
	static const int numSupportedFuncs = sizeof( funcs ) / sizeof( funcs[0] ) - 1;

	if( !numDeforms ) {
		return NULL;
	}

	program[0] = '\0';
	Q_strncpyz( program, r_GLSLWaveFuncs, sizeof( program ) );

	Q_strncatz( program, 
		"#if defined(VERTEX_SHADER) && defined(WAVE_SIN) && !defined(APPLY_DEFORMVERTS)\n"
		"\n"
		"vec4 DeformVerts(vec4 inVert, vec3 inNorm, vec2 inST, float time)\n"
		"{\n"
		"vec4 outVert = inVert;\n"
		"float t = 0.0;\n"
		"\n"
		, sizeof( program ) );

	for( i = 0; i < numDeforms; i++, deformv++ ) {
		switch( deformv->type ) {
			case DEFORMV_WAVE:
				funcType = deformv->func.type;
				if( funcType <= SHADER_FUNC_NONE || funcType > numSupportedFuncs || !funcs[funcType] ) {
					return NULL;
				}

				Q_strncatz( program, va( "outVert.xyz += %s(time,%f,%f,%f+%f*(outVert.x+outVert.y+outVert.z),%f) * inNorm.xyz;\n", 
					funcs[funcType], deformv->func.args[0], deformv->func.args[1], deformv->func.args[2], deformv->func.args[3] ? deformv->args[0] : 0.0, deformv->func.args[3] ), 
					sizeof( program ) );
				break;
			case DEFORMV_MOVE:
				funcType = deformv->func.type;
				if( funcType <= SHADER_FUNC_NONE || funcType > numSupportedFuncs || !funcs[funcType] ) {
					return NULL;
				}

				Q_strncatz( program, va( "outVert.xyz += %s(time,%f,%f,%f,%f) * vec3(%f, %f, %f);\n", 
					funcs[funcType], deformv->func.args[0], deformv->func.args[1], deformv->func.args[2], deformv->func.args[3],
						deformv->args[0], deformv->args[1], deformv->args[2] ), 
					sizeof( program ) );
				break;
			case DEFORMV_BULGE:
				Q_strncatz( program, va( 
						"t = sin(inST.s * %f + time * %f);\n"
						"outVert.xyz += max (-1.0 + %f, t) * %f * inNorm.xyz;\n", 
						deformv->args[0], deformv->args[2], deformv->args[3], deformv->args[1] ), 
					sizeof( program ) );
				break;
			default:
				return NULL;
		}
	}

	Q_strncatz( program, 
		"\n"
		"return outVert;\n"
		"}\n"
		"\n"
		"#define APPLY_DEFORMVERTS DeformVerts\n"
		"#endif\n"
		"\n"
		, sizeof( program ) );

	return program;
}

/*
* R_GLSLBonesString
*/
static const char *R_GLSLBonesString( void )
{
	static char tString[MAX_STRING_CHARS];

	Q_snprintfz( tString, sizeof( tString ), "#define MAX_GLSL_BONES %i\n", glConfig.maxGLSLBones );

	return tString;
}

/*
* R_ProgramFeatures2Defines
* 
* Return an array of strings for bitflags
*/
static const char **R_ProgramFeatures2Defines( const glsl_feature_t *type_features, r_glslfeat_t features, char *name, size_t size )
{
	int i, p;
	static const char *headers[MAX_DEFINES_FEATURES+1]; // +1 for NULL safe-guard

	for( i = 0, p = 0; features && type_features[i].bit; i++ )
	{
		if( (features & type_features[i].bit) == type_features[i].bit )
		{
			headers[p++] = type_features[i].define;
			if( name )
				Q_strncatz( name, type_features[i].suffix, size );

			features &= ~type_features[i].bit;

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
* R_RegisterGLSLProgram
*/
int R_RegisterGLSLProgram( int type, const char *name, const char *string, const char *deformsKey, const deformv_t *deforms, int numDeforms, r_glslfeat_t features )
{
	int i;
	int linked, error = 0;
	int body_start, num_vs_strings, num_fs_strings;
	unsigned int hash_key;
	glsl_program_t *program;
	char fullName[1024];
	const char **header;
	const char *vertexShaderStrings[MAX_DEFINES_FEATURES+4];
	const char *fragmentShaderStrings[MAX_DEFINES_FEATURES+4];

	if( !glConfig.ext.GLSL )
		return 0; // fail early

	if( type <= GLSL_PROGRAM_TYPE_NONE || type >= GLSL_PROGRAM_TYPE_MAXTYPE )
		return 0;

	assert( !deforms || deformsKey );

	// default deformsKey to empty string, easier on checking later
	if( !deforms )
		deformsKey = "";

	// compute hash key for the features+deformsKey pair
	hash_key = Com_SuperFastHash64BitInt( features );
	if( *deformsKey ) {
		hash_key = Com_SuperFastHash( ( const qbyte * )deformsKey, strlen( deformsKey ), hash_key );
	}
	hash_key &= ( GLSL_PROGRAMS_HASH_SIZE - 1 );

	for( program = r_glslprograms_hash[type][hash_key]; program; program = program->hash_next )
	{
		if( ( program->features == features ) && !strcmp( program->deformsKey, deformsKey ) ) {
			return ( (program - r_glslprograms) + 1 );
		}
	}

	if( r_numglslprograms == MAX_GLSL_PROGRAMS )
	{
		Com_Printf( S_COLOR_YELLOW "R_RegisterGLSLProgram: GLSL programs limit exceeded\n" );
		return 0;
	}

	// if no string was specified, search for an already registered program of the same type
	// with minimal set of features specified
	if( !string || !name )
	{
		glsl_program_t *parent;

		parent = NULL;
		for( i = 0; i < r_numglslprograms; i++ ) {
			program = r_glslprograms + i;

			if( (program->type == type) && !program->features ) {
				parent = program;
				break;
			}
		}

		if( parent ) {
			if( !name )
				name = parent->name;
			if( !string )
				string = parent->string;
		}
		else {
			name = DEFAULT_GLSL_PROGRAM;
			string = r_defaultMaterialProgram;
		}
	}

	program = r_glslprograms + r_numglslprograms++;
	program->object = qglCreateProgramObjectARB();
	if( !program->object )
	{
		error = 1;
		goto done;
	}

	body_start = 1;
	Q_strncpyz( fullName, name, sizeof( fullName ) );
	header = R_ProgramFeatures2Defines( glsl_programtypes_features[type], features, fullName, sizeof( fullName ) );

	Com_DPrintf( "Registering GLSL program %s\n", fullName );

	if( header )
		for( ; header[body_start-1] && *header[body_start-1]; body_start++ );

	vertexShaderStrings[0] = "#define VERTEX_SHADER\n";
	for( i = 1; i < body_start; i++ )
		vertexShaderStrings[i] = ( char * )header[i-1];
	if( numDeforms ) {
		vertexShaderStrings[i] = R_GLSLBuildDeformv( deforms, numDeforms );
		if( vertexShaderStrings[i] ) {
			i++;
		}
	}

	// bones
	vertexShaderStrings[i++] = R_GLSLBonesString();

	// find vertex shader header
	vertexShaderStrings[i] = string;
	num_vs_strings = i+1;

	fragmentShaderStrings[0] = "#define FRAGMENT_SHADER\n";
	for( i = 1; i < body_start; i++ )
		fragmentShaderStrings[i] = ( char * )header[i-1];
	// find fragment shader header
	fragmentShaderStrings[i] = string;
	num_fs_strings = i+1;

#if 0
	{
		int file;

		if( FS_FOpenFile( va("programs/%s.glsl", &fullName[1]), &file, FS_WRITE ) != -1 ) 
		{
			for( i = 0; i < num_vs_strings; i++ ) {
				FS_Print( file, vertexShaderStrings[i] ); 
			}
			for( i = 0; i < num_fs_strings; i++ ) {
				FS_Print( file, fragmentShaderStrings[i] ); 
			}
			FS_FCloseFile( file );
		}
	}
#endif

	R_BindProgramAttrbibutesLocations( program );

	// compile vertex shader
	program->vertexShader = R_CompileGLSLShader( program->object, fullName, "vertex", GL_VERTEX_SHADER_ARB, vertexShaderStrings, num_vs_strings );
	if( !program->vertexShader )
	{
		error = 1;
		goto done;
	}

	// compile fragment shader
	program->fragmentShader = R_CompileGLSLShader( program->object, fullName, "fragment", GL_FRAGMENT_SHADER_ARB, fragmentShaderStrings, num_fs_strings );
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
	program->deformsKey = *deformsKey ? R_GLSLProgramCopyString( deformsKey ) : "";

	if( !program->hash_next )
	{
		program->hash_next = r_glslprograms_hash[type][hash_key];
		r_glslprograms_hash[type][hash_key] = program;
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
* R_GetProgramObject
*/
int R_GetProgramObject( int elem )
{
	return r_glslprograms[elem - 1].object;
}

/*
* R_ProgramList_f
*/
void R_ProgramList_f( void )
{
	int i;
	glsl_program_t *program;
	char fullName[1024];

	Com_Printf( "------------------\n" );
	for( i = 0, program = r_glslprograms; i < MAX_GLSL_PROGRAMS; i++, program++ )
	{
		if( !program->name )
			break;

		Q_strncpyz( fullName, program->name, sizeof( fullName ) );
		R_ProgramFeatures2Defines( glsl_programtypes_features[program->type], program->features, fullName, sizeof( fullName ) );

		Com_Printf( " %3i %s", i+1, fullName );
		if( *program->deformsKey ) {
			Com_Printf( " dv:%s", program->deformsKey );
		}
		Com_Printf( "\n" );
	}
	Com_Printf( "%i programs total\n", i );
}

/*
* R_ProgramDump_f
*/
#define DUMP_PROGRAM(p) { int file; if( FS_FOpenFile( va("programs/%s.glsl", #p), &file, FS_WRITE ) != -1 ) { FS_Print( file, r_##p ); FS_FCloseFile( file ); } }
void R_ProgramDump_f( void )
{
	DUMP_PROGRAM( defaultMaterialProgram );
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
* R_UpdateProgramUniforms
*/
void R_UpdateProgramUniforms( int elem, const vec3_t eyeOrigin,
							 const vec3_t lightOrigin, const vec3_t lightDir, const vec4_t ambient, const vec4_t diffuse,
							 const superLightStyle_t *superLightStyle, qboolean frontPlane, int TexWidth, int TexHeight,
							 float projDistance, float offsetmappingScale, float glossExponent, 
							 const qbyte *constColor, int overbrightBits, float shaderTime, const qbyte *entityColor )
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

	if( program->locTextureParams >= 0 )
		qglUniform4fARB( program->locTextureParams, TexWidth, TexHeight, TexWidth ? 1.0 / TexWidth : 1.0, TexHeight ? 1.0 / TexHeight : 1.0 );

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

	if( program->locEntColor >= 0 && entityColor )
		qglUniform4fARB( program->locEntColor, entityColor[0] * 1.0/255.0, entityColor[1] * 1.0/255.0, entityColor[2] * 1.0/255.0, entityColor[3] * 1.0/255.0 );
	if( program->locConstColor >= 0 && constColor )
		qglUniform4fARB( program->locConstColor, constColor[0] * 1.0/255.0, constColor[1] * 1.0/255.0, constColor[2] * 1.0/255.0, constColor[3] * 1.0/255.0 );
	if( program->locOverbrightScale >= 0 )
		qglUniform1fARB( program->locOverbrightScale, 1.0 / overbrightBits );

	if( program->locShaderTime >= 0 )
		qglUniform1fARB( program->locShaderTime, shaderTime );
}

/*
* R_UpdateProgramFogParams
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
* R_UpdateProgramLightsParams
*/
unsigned int R_UpdateProgramLightsParams( int elem, const vec3_t entOrigin, vec3_t entAxis[3], unsigned int dlightbits )
{
	int i, n;
	dlight_t *dl;
	float colorScale = mapConfig.mapLightColorScale;
	vec3_t dlorigin, tvec;
	glsl_program_t *program = r_glslprograms + elem - 1;
	qboolean identityAxis = Matrix_Compare( entAxis, axis_identity );

	n = 0;
	for( i = 0, dl = r_dlights; i < MAX_DLIGHTS; i++, dl++ ) {
		if( !(dlightbits & (1<<i)) ) {
			continue;
		}
		if( !dl->intensity ) {
			continue;
		}
		if( program->locDynamicLightsRadius[n] < 0 ) {
			break;
		}

		VectorSubtract( dl->origin, entOrigin, dlorigin );
		if( !identityAxis ) {
			VectorCopy( dlorigin, tvec );
			Matrix_TransformVector( entAxis, tvec, dlorigin );
		}

		qglUniform1fARB( program->locDynamicLightsRadius[n], dl->intensity );
		qglUniform3fARB( program->locDynamicLightsPosition[n], dlorigin[0], dlorigin[1], dlorigin[2] );
		qglUniform3fARB( program->locDynamicLightsDiffuse[n], dl->color[0] * colorScale, dl->color[1] * colorScale, dl->color[2] * colorScale );

		n++;
	}

	for( ; n < MAX_DLIGHTS; n++ ) {
		if( program->locDynamicLightsRadius[n] < 0 ) {
			break;
		}
		qglUniform1fARB( program->locDynamicLightsRadius[n], 1 );
		qglUniform3fARB( program->locDynamicLightsDiffuse[n], 0, 0, 0 );
	}

	return 0;
}

/*
* R_UpdateProgramQ3AShaderParams
*/
void R_UpdateProgramQ3AParams( int elem, float shaderTime, const vec3_t eyeOrigin, const vec3_t entDist, const qbyte *constColor, int overbrightBits )
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
	if( program->locShaderTime >= 0 )
		qglUniform1fARB( program->locShaderTime, shaderTime );
}

/*
* R_UpdateProgramPlanarShadowParams
*/
void R_UpdateProgramPlanarShadowParams( int elem, float shaderTime, const vec3_t planeNormal, float planeDist, const vec3_t lightDir )
{
	glsl_program_t *program = r_glslprograms + elem - 1;

	if( program->locPlaneNormal >= 0 )
		qglUniform3fARB( program->locPlaneNormal, planeNormal[0], planeNormal[1], planeNormal[2] );
	if( program->locPlaneDist >= 0 )
		qglUniform1fARB( program->locPlaneDist, planeDist );

	if( program->locLightDir >= 0 )
		qglUniform3fARB( program->locLightDir, lightDir[0], lightDir[1], lightDir[2] );

	if( program->locShaderTime >= 0 )
		qglUniform1fARB( program->locShaderTime, shaderTime );
}

/*
* R_UpdateProgramCellshadeParams
*/
void R_UpdateProgramCellshadeParams( int elem, float shaderTime, const vec3_t eyeOrigin, const vec3_t entDist, const qbyte *constColor, int overbrightBits, const qbyte *entityColor, mat4x4_t reflectionMatrix )
{
	glsl_program_t *program = r_glslprograms + elem - 1;

	if( program->locEyeOrigin >= 0 && eyeOrigin )
		qglUniform3fARB( program->locEyeOrigin, eyeOrigin[0], eyeOrigin[1], eyeOrigin[2] );
	if( program->locEntDist >= 0 && entDist )
		qglUniform3fARB( program->locEntDist, entDist[0], entDist[1], entDist[2] );
	if( program->locEntColor >= 0 && entityColor )
		qglUniform4fARB( program->locEntColor, entityColor[0] * 1.0/255.0, entityColor[1] * 1.0/255.0, entityColor[2] * 1.0/255.0, entityColor[3] * 1.0/255.0 );
	if( program->locConstColor >= 0 && constColor )
		qglUniform4fARB( program->locConstColor, constColor[0] * 1.0/255.0, constColor[1] * 1.0/255.0, constColor[2] * 1.0/255.0, constColor[3] * 1.0/255.0 );
	if( program->locOverbrightScale >= 0 )
		qglUniform1fARB( program->locOverbrightScale, 1.0 / (1 << overbrightBits) );
	if( program->locShaderTime >= 0 )
		qglUniform1fARB( program->locShaderTime, shaderTime );
	if( program->locReflectionMatrix >= 0 )
		qglUniformMatrix4fvARB( program->locReflectionMatrix, 1, GL_FALSE, reflectionMatrix );
}

/*
* R_UpdateProgramBonesParams
* 
* Set uniform values for animation dual quaternions
*/
void R_UpdateProgramBonesParams( int elem, unsigned int numBones, dualquat_t *animDualQuat )
{
	unsigned int i, j;
	glsl_program_t *program = r_glslprograms + elem - 1;

	assert( numBones <= glConfig.maxGLSLBones );
	if( numBones > glConfig.maxGLSLBones ) {
		return;
	}

	if( !program->locDualQuats ) {
		return;
	}

	for( i = 0, j = 0; i < numBones; i++ ) {
		qglUniform4fvARB( program->locDualQuats[j++], 1, &animDualQuat[i][0] ); // real
		qglUniform4fvARB( program->locDualQuats[j++], 1, &animDualQuat[i][4] ); // dual
	}
}

/*
* R_UpdateDrawFlatParams
*/
void R_UpdateDrawFlatParams( int elem, const vec3_t wallColor, const vec3_t floorColor )
{
	glsl_program_t *program = r_glslprograms + elem - 1;

	if( program->locWallColor >= 0 )
		qglUniform3fARB( program->locWallColor, wallColor[0], wallColor[1], wallColor[2] );
	if( program->locFloorColor >= 0 )
		qglUniform3fARB( program->locFloorColor, floorColor[0], floorColor[1], floorColor[2] );
}

/*
* R_GetProgramUniformLocations
*/
static void R_GetProgramUniformLocations( glsl_program_t *program )
{
	unsigned int i;
	int		locBaseTexture,
			locNormalmapTexture,
			locGlossTexture,
			locDecalTexture,
			locEntityDecalTexture,
			locLightmapTexture[MAX_LIGHTMAPS],
			locDuDvMapTexture,
			locReflectionTexture,
			locRefractionTexture,
			locShadowmapTexture,
			locCellShadeTexture,
			locCellLightTexture,
			locDiffuseTexture,
			locStripesTexture,
			locDualQuats0;
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
	locEntityDecalTexture = qglGetUniformLocationARB( program->object, "EntityDecalTexture" );

	locDuDvMapTexture = qglGetUniformLocationARB( program->object, "DuDvMapTexture" );
	locReflectionTexture = qglGetUniformLocationARB( program->object, "ReflectionTexture" );
	locRefractionTexture = qglGetUniformLocationARB( program->object, "RefractionTexture" );

	locShadowmapTexture = qglGetUniformLocationARB( program->object, "ShadowmapTexture" );

	locCellShadeTexture = qglGetUniformLocationARB( program->object, "CellShadeTexture" );
	locCellLightTexture = qglGetUniformLocationARB( program->object, "CellLightTexture" );
	locDiffuseTexture = qglGetUniformLocationARB( program->object, "DiffuseTexture" );
	locStripesTexture = qglGetUniformLocationARB( program->object, "StripesTexture" );

	locDualQuats0 = qglGetUniformLocationARB( program->object, "DualQuats[0]" );

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

	program->locTextureParams = qglGetUniformLocationARB( program->object, "TextureParams" );

	program->locProjDistance = qglGetUniformLocationARB( program->object, "ProjDistance" );

	program->locTurbAmplitude = qglGetUniformLocationARB( program->object, "TurbAmplitude" );
	program->locTurbPhase = qglGetUniformLocationARB( program->object, "TurbPhase" );

	program->locEntDist = qglGetUniformLocationARB( program->object, "EntDist" );
	program->locEntColor = qglGetUniformLocationARB( program->object, "EntityColor" );
	program->locConstColor = qglGetUniformLocationARB( program->object, "ConstColor" );
	program->locOverbrightScale = qglGetUniformLocationARB( program->object, "OverbrightScale" );

	program->locFogPlane = qglGetUniformLocationARB( program->object, "FogPlane" );
	program->locEyePlane = qglGetUniformLocationARB( program->object, "EyePlane" );
	program->locEyeFogDist = qglGetUniformLocationARB( program->object, "EyeFogDist" );

	program->locShaderTime = qglGetUniformLocationARB( program->object, "ShaderTime" );

	program->locPlaneNormal = qglGetUniformLocationARB( program->object, "PlaneNormal" );
	program->locPlaneDist = qglGetUniformLocationARB( program->object, "PlaneDist" );

	program->locReflectionMatrix = qglGetUniformLocationARB( program->object, "ReflectionMatrix" );

	if( locDualQuats0 >= 0 ) {
		program->locDualQuats = R_ProgramAlloc( sizeof( *program->locDualQuats ) * glConfig.maxGLSLBones * 2 );
		for( i = 0; i < glConfig.maxGLSLBones * 2; i++ ) {
			program->locDualQuats[i] = qglGetUniformLocationARB( program->object, va( "DualQuats[%i]", i ) );
		}
	}

	// dynamic lights
	for( i = 0; i < MAX_DLIGHTS; i++ ) {
		int locR, locP, locD;

		locR = qglGetUniformLocationARB( program->object, va( "DynamicLights[%i].Radius", i ) );
		locP = qglGetUniformLocationARB( program->object, va( "DynamicLights[%i].Position", i ) );
		locD = qglGetUniformLocationARB( program->object, va( "DynamicLights[%i].Diffuse", i ) );

		if( locR < 0 || locP < 0 || locD < 0 ) {
			program->locDynamicLightsRadius[i] = program->locDynamicLightsPosition[i] = 
				program->locDynamicLightsDiffuse[i] = -1;
			break;
		}

		program->locDynamicLightsRadius[i] = locR;
		program->locDynamicLightsPosition[i] = locP;
		program->locDynamicLightsDiffuse[i] = locD;
	}

	program->locWallColor = qglGetUniformLocationARB( program->object, "WallColor" );
	program->locFloorColor = qglGetUniformLocationARB( program->object, "FloorColor" );

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
	if( locEntityDecalTexture >= 0 )
		qglUniform1iARB( locEntityDecalTexture, 4 );

	if( locReflectionTexture >= 0 )
		qglUniform1iARB( locReflectionTexture, 2 );
	if( locRefractionTexture >= 0 )
		qglUniform1iARB( locRefractionTexture, 3 );

	if( locShadowmapTexture >= 0 )
		qglUniform1iARB( locShadowmapTexture, 0 );

//	if( locBaseTexture >= 0 )
//		qglUniform1iARB( locBaseTexture, 0 );
	if( locCellShadeTexture >= 0 )
		qglUniform1iARB( locCellShadeTexture, 1 );
	if( locDiffuseTexture >= 0 )
		qglUniform1iARB( locDiffuseTexture, 2 );
//	if( locDecalTexture >= 0 )
//		qglUniform1iARB( locDecalTexture, 3 );
//	if( locEntityDecalTexture >= 0 )
//		qglUniform1iARB( locEntityDecalTexture, 4 );
	if( locStripesTexture >= 0 )
		qglUniform1iARB( locStripesTexture, 5 );
	if( locCellLightTexture >= 0 )
		qglUniform1iARB( locCellLightTexture, 6 );

	for( i = 0; i < MAX_LIGHTMAPS; i++ )
	{
		if( locLightmapTexture[i] >= 0 )
			qglUniform1iARB( locLightmapTexture[i], i+4 );
	}
}

/*
* R_BindProgramAttrbibutesLocations
*/
static void R_BindProgramAttrbibutesLocations( glsl_program_t *program )
{
	qglBindAttribLocationARB( program->object, GLSL_ATTRIB_BONESINDICES, "BonesIndices" );
	qglBindAttribLocationARB( program->object, GLSL_ATTRIB_BONESWEIGHTS, "BonesWeights" );
}

/*
* R_ShutdownGLSLPrograms
*/
void R_ShutdownGLSLPrograms( void )
{
	int i;
	glsl_program_t *program;

	if( !r_glslProgramsPool )
		return;
	if( !glConfig.ext.GLSL )
		return;

	R_StoreGLSLPrecacheList();

	for( i = 0, program = r_glslprograms; i < r_numglslprograms; i++, program++ )
		R_DeleteGLSLProgram( program );

	Mem_FreePool( &r_glslProgramsPool );

	r_numglslprograms = 0;
}
