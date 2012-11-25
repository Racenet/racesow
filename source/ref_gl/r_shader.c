/*
Copyright (C) 1999 Stephen C. Taylor
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

// r_shader.c

#include "r_local.h"

#define _RGB_GEN_IDENTITY_LIGHTING (r_overbrightbits->integer > 0 ? RGB_GEN_IDENTITY_LIGHTING : RGB_GEN_IDENTITY)

#define SHADERS_HASH_SIZE	128
#define SHADERCACHE_HASH_SIZE	128

typedef struct
{
	const char *keyword;
	void ( *func )( shader_t *shader, shaderpass_t *pass, const char **ptr );
} shaderkey_t;

typedef struct shadercache_s
{
	char *name;
	char *buffer;
	const char *filename;
	size_t offset;
	struct shadercache_s *hash_next;
} shadercache_t;

shader_t r_shaders[MAX_SHADERS];

static char *shaderPaths;
static shader_t r_shaders_hash_headnode[SHADERS_HASH_SIZE], *r_free_shaders;
static shadercache_t *shadercache_hash[SHADERCACHE_HASH_SIZE];

static deformv_t r_currentDeforms[MAX_SHADER_DEFORMVS];
static shaderpass_t r_currentPasses[MAX_SHADER_PASSES];
static float r_currentRGBgenArgs[MAX_SHADER_PASSES][3], r_currentAlphagenArgs[MAX_SHADER_PASSES][2];
static shaderfunc_t r_currentRGBgenFuncs[MAX_SHADER_PASSES], r_currentAlphagenFuncs[MAX_SHADER_PASSES];
static tcmod_t r_currentTcmods[MAX_SHADER_PASSES][MAX_SHADER_TCMODS];
static vec4_t r_currentTcGen[MAX_SHADER_PASSES][2];

static qboolean	r_shaderNoMipMaps;
static qboolean	r_shaderNoPicMip;
static qboolean r_shaderNoFiltering;
static qboolean	r_shaderNoCompress;
static qboolean	r_shaderHasDlightPass;
static qboolean	r_shaderHasDistanceRamp;
static qboolean r_shaderHasLightmapPass;
static qboolean r_shaderPolygonOffset;
static qboolean r_shaderDisableVBO;
static int		r_shaderAllDetail;

static image_t	*r_defaultImage;

mempool_t *r_shadersmempool;
static char *r_shaderTemplateBuf;

static qboolean Shader_Parsetok( shader_t *shader, shaderpass_t *pass, const shaderkey_t *keys, const char *token, const char **ptr );
static void Shader_MakeCache( const char *filename );
static unsigned int Shader_GetCache( const char *name, shadercache_t **cache );
#define Shader_FreePassCinematics(pass) if( (pass)->cin ) { R_FreeCinematic( (pass)->cin ); (pass)->cin = 0; }

//===========================================================================

static char *Shader_ParseString( const char **ptr )
{
	char *token;

	if( !ptr || !( *ptr ) )
		return "";
	if( !**ptr || **ptr == '}' )
		return "";

	token = COM_ParseExt( ptr, qfalse );
	return Q_strlwr( token );
}

static float Shader_ParseFloat( const char **ptr )
{
	if( !ptr || !( *ptr ) )
		return 0;
	if( !**ptr || **ptr == '}' )
		return 0;

	return atof( COM_ParseExt( ptr, qfalse ) );
}

static void Shader_ParseVector( const char **ptr, float *v, unsigned int size )
{
	unsigned int i;
	char *token;
	qboolean bracket;

	if( !size )
		return;
	if( size == 1 )
	{
		Shader_ParseFloat( ptr );
		return;
	}

	token = Shader_ParseString( ptr );
	if( !strcmp( token, "(" ) )
	{
		bracket = qtrue;
		token = Shader_ParseString( ptr );
	}
	else if( token[0] == '(' )
	{
		bracket = qtrue;
		token = &token[1];
	}
	else
	{
		bracket = qfalse;
	}

	v[0] = atof( token );
	for( i = 1; i < size-1; i++ )
		v[i] = Shader_ParseFloat( ptr );

	token = Shader_ParseString( ptr );
	if( !token[0] )
	{
		v[i] = 0;
	}
	else if( token[strlen( token )-1] == ')' )
	{
		token[strlen( token )-1] = 0;
		v[i] = atof( token );
	}
	else
	{
		v[i] = atof( token );
		if( bracket )
			Shader_ParseString( ptr );
	}
}

static void Shader_SkipLine( const char **ptr )
{
	while( ptr )
	{
		const char *token = COM_ParseExt( ptr, qfalse );
		if( !token[0] )
			return;
	}
}

static void Shader_SkipBlock( const char **ptr )
{
	const char *tok;
	int brace_count;

	// Opening brace
	tok = COM_ParseExt( ptr, qtrue );
	if( tok[0] != '{' )
		return;

	for( brace_count = 1; brace_count > 0; )
	{
		tok = COM_ParseExt( ptr, qtrue );
		if( !tok[0] )
			return;
		if( tok[0] == '{' )
			brace_count++;
		else if( tok[0] == '}' )
			brace_count--;
	}
}

#define MAX_CONDITIONS		8
typedef enum { COP_LS, COP_LE, COP_EQ, COP_GR, COP_GE, COP_NE } conOp_t;
typedef enum { COP2_AND, COP2_OR } conOp2_t;
typedef struct { int operand; conOp_t op; qboolean negative; int val; conOp2_t logic; } shaderCon_t;

char *conOpStrings[] = { "<", "<=", "==", ">", ">=", "!=", NULL };
char *conOpStrings2[] = { "&&", "||", NULL };

static qboolean Shader_ParseConditions( const char **ptr, shader_t *shader )
{
	int i;
	char *tok;
	int numConditions;
	qboolean isVolatile = qfalse;
	unsigned int volatileFlags;
	shaderCon_t conditions[MAX_CONDITIONS];
	qboolean result = qfalse, val = qfalse, skip, expectingOperator;
	static const int falseCondition = 0;

	numConditions = 0;
	memset( conditions, 0, sizeof( conditions ) );

	isVolatile = qfalse;
	volatileFlags = 0;

	skip = qfalse;
	expectingOperator = qfalse;
	while( 1 )
	{
		tok = Shader_ParseString( ptr );
		if( !tok[0] )
		{
			if( expectingOperator )
				numConditions++;
			break;
		}
		if( skip )
			continue;

		for( i = 0; conOpStrings[i]; i++ )
		{
			if( !strcmp( tok, conOpStrings[i] ) )
				break;
		}

		if( conOpStrings[i] )
		{
			if( !expectingOperator )
			{
				Com_Printf( S_COLOR_YELLOW "WARNING: Bad syntax in condition (shader %s)\n", shader->name );
				skip = qtrue;
			}
			else
			{
				conditions[numConditions].op = i;
				expectingOperator = qfalse;
			}
			continue;
		}

		for( i = 0; conOpStrings2[i]; i++ )
		{
			if( !strcmp( tok, conOpStrings2[i] ) )
				break;
		}

		if( conOpStrings2[i] )
		{
			if( !expectingOperator )
			{
				Com_Printf( S_COLOR_YELLOW "WARNING: Bad syntax in condition (shader %s)\n", shader->name );
				skip = qtrue;
			}
			else
			{
				conditions[numConditions++].logic = i;
				if( numConditions == MAX_CONDITIONS )
					skip = qtrue;
				else
					expectingOperator = qfalse;
			}
			continue;
		}

		if( expectingOperator )
		{
			Com_Printf( S_COLOR_YELLOW "WARNING: Bad syntax in condition (shader %s)\n", shader->name );
			skip = qtrue;
			continue;
		}

		if( !strcmp( tok, "!" ) )
		{
			conditions[numConditions].negative = !conditions[numConditions].negative;
			continue;
		}

		if( !conditions[numConditions].operand )
		{
			if( !Q_stricmp( tok, "maxTextureSize" ) )
				conditions[numConditions].operand = ( int  )glConfig.maxTextureSize;
			else if( !Q_stricmp( tok, "maxTextureCubemapSize" ) )
				conditions[numConditions].operand = ( int )glConfig.maxTextureCubemapSize;
			else if( !Q_stricmp( tok, "maxTextureUnits" ) )
				conditions[numConditions].operand = ( int )glConfig.maxTextureUnits;
			else if( !Q_stricmp( tok, "textureCubeMap" ) )
				conditions[numConditions].operand = ( int )glConfig.ext.texture_cube_map;
			else if( !Q_stricmp( tok, "textureEnvCombine" ) )
				conditions[numConditions].operand = ( int )glConfig.ext.texture_env_combine;
			else if( !Q_stricmp( tok, "textureEnvDot3" ) )
				conditions[numConditions].operand = ( int )glConfig.ext.GLSL;
			else if( !Q_stricmp( tok, "GLSL" ) )
				conditions[numConditions].operand = ( int )glConfig.ext.GLSL;
			else if( !Q_stricmp( tok, "deluxeMaps" ) || !Q_stricmp( tok, "deluxe" ) ) {
				conditions[numConditions].operand = ( int )mapConfig.deluxeMappingEnabled;

				isVolatile = qtrue;
				if( mapConfig.deluxeMappingEnabled ) {
					volatileFlags |= SHADER_VOLATILE_MAPCONFIG_DELUXEMAPPING;
				}
			}
			else if( !Q_stricmp( tok, "portalMaps" ) )
				conditions[numConditions].operand = ( int )r_portalmaps->integer;
			else
			{
				Com_Printf( S_COLOR_YELLOW "WARNING: Unknown expression '%s' in shader %s\n", tok, shader->name );
//				skip = qtrue;
				conditions[numConditions].operand = ( int )falseCondition;
			}

			conditions[numConditions].operand++;
			if( conditions[numConditions].operand < 0 )
				conditions[numConditions].operand = 0;

			if( !skip )
			{
				conditions[numConditions].op = COP_NE;
				expectingOperator = qtrue;
			}
			continue;
		}

		if( !strcmp( tok, "false" ) )
			conditions[numConditions].val = 0;
		else if( !strcmp( tok, "true" ) )
			conditions[numConditions].val = 1;
		else
			conditions[numConditions].val = atoi( tok );
		expectingOperator = qtrue;
	}

	if( skip )
		return qfalse;

	if( !conditions[0].operand )
	{
		Com_Printf( S_COLOR_YELLOW "WARNING: Empty 'if' statement in shader %s\n", shader->name );
		return qfalse;
	}


	for( i = 0; i < numConditions; i++ )
	{
		conditions[i].operand--;

		switch( conditions[i].op )
		{
		case COP_LS:
			val = ( conditions[i].operand < conditions[i].val );
			break;
		case COP_LE:
			val = ( conditions[i].operand <= conditions[i].val );
			break;
		case COP_EQ:
			val = ( conditions[i].operand == conditions[i].val );
			break;
		case COP_GR:
			val = ( conditions[i].operand > conditions[i].val );
			break;
		case COP_GE:
			val = ( conditions[i].operand >= conditions[i].val );
			break;
		case COP_NE:
			val = ( conditions[i].operand != conditions[i].val );
			break;
		default:
			break;
		}

		if( conditions[i].negative )
			val = !val;
		if( i )
		{
			switch( conditions[i-1].logic )
			{
			case COP2_AND:
				result = result && val;
				break;
			case COP2_OR:
				result = result || val;
				break;
			}
		}
		else
		{
			result = val;
		}
	}

	if( isVolatile ) {
		// store volatile flags based on parsed conditions
		shader->flags |= SHADER_VOLATILE;
		shader->volatileFlags |= volatileFlags;
	}

	return result;
}

static qboolean Shader_SkipConditionBlock( const char **ptr )
{
	const char *tok;
	int condition_count;

	for( condition_count = 1; condition_count > 0; )
	{
		tok = COM_ParseExt( ptr, qtrue );
		if( !tok[0] )
			return qfalse;
		if( !Q_stricmp( tok, "if" ) )
			condition_count++;
		else if( !Q_stricmp( tok, "endif" ) )
			condition_count--;
// Vic: commented out for now
//		else if( !Q_stricmp( tok, "else" ) && (condition_count == 1) )
//			return qtrue;
	}

	return qtrue;
}

//===========================================================================

static void Shader_ParseSkySides( const char **ptr, shader_t **shaders, qboolean farbox, qboolean underscore )
{
	int i, j;
	char *token;
	image_t *image;
	qboolean noskybox = qfalse;

	token = Shader_ParseString( ptr );
	if( token[0] == '-' )
	{
		noskybox = qtrue;
	}
	else
	{
		struct cubemapSufAndFlip
		{
			char *suf; int flags;
		} cubemapSides[2][6] = {
			{
				{ "px", IT_FLIPDIAGONAL },
				{ "py", IT_FLIPY },
				{ "nx", IT_FLIPX|IT_FLIPY|IT_FLIPDIAGONAL },
				{ "ny", IT_FLIPX },
				{ "pz", IT_FLIPDIAGONAL },
				{ "nz", IT_FLIPDIAGONAL }
			},
			{
				{ "rt", 0 },
				{ "bk", 0 },
				{ "lf", 0 },
				{ "ft", 0 },
				{ "up", 0 },
				{ "dn", 0 }
			}
		};

		for( i = 0; i < 2; i++ )
		{
			char suffix[6];

			memset( shaders, 0, sizeof( shader_t * ) * 6 );

			for( j = 0; j < 6; j++ )
			{
				if( underscore )
					Q_strncpyz( suffix, "_", sizeof( suffix ) );
				else
					suffix[0] = '\0';
				Q_strncatz( suffix, cubemapSides[i][j].suf, sizeof( suffix ) );

				image = R_FindImage( token, suffix, IT_SKY|IT_NOMIPMAP|IT_CLAMP|cubemapSides[i][j].flags, 0 );
				if( !image )
					break;

				shaders[j] = R_LoadShader( image->name, ( farbox ? SHADER_FARBOX : SHADER_NEARBOX ), qtrue, cubemapSides[i][j].flags, SHADER_INVALID, NULL );
			}

			if( j == 6 )
				break;
		}

		if( i == 2 )
			noskybox = qtrue;
	}

	if( noskybox )
		memset( shaders, 0, sizeof( shader_t * ) * 6 );
}

static void Shader_ParseFunc( const char **ptr, shaderfunc_t *func )
{
	char *token;

	token = Shader_ParseString( ptr );
	if( !strcmp( token, "sin" ) )
		func->type = SHADER_FUNC_SIN;
	else if( !strcmp( token, "triangle" ) )
		func->type = SHADER_FUNC_TRIANGLE;
	else if( !strcmp( token, "square" ) )
		func->type = SHADER_FUNC_SQUARE;
	else if( !strcmp( token, "sawtooth" ) )
		func->type = SHADER_FUNC_SAWTOOTH;
	else if( !strcmp( token, "inversesawtooth" ) )
		func->type = SHADER_FUNC_INVERSESAWTOOTH;
	else if( !strcmp( token, "noise" ) )
		func->type = SHADER_FUNC_NOISE;
	else if( !strcmp( token, "distanceramp" ) )
	{
		func->type = SHADER_FUNC_RAMP;
		r_shaderHasDistanceRamp = qtrue;
	}

	func->args[0] = Shader_ParseFloat( ptr );
	func->args[1] = Shader_ParseFloat( ptr );
	func->args[2] = Shader_ParseFloat( ptr );
	func->args[3] = Shader_ParseFloat( ptr );
}

//===========================================================================

static int Shader_SetImageFlags( shader_t *shader )
{
	int flags = 0;

	if( shader->flags & SHADER_SKY )
		flags |= IT_SKY;
	if( r_shaderNoMipMaps )
		flags |= IT_NOMIPMAP;
	if( r_shaderNoPicMip )
		flags |= IT_NOPICMIP;
	if( r_shaderNoCompress )
		flags |= IT_NOCOMPRESS;
	if( r_shaderNoFiltering )
		flags |= IT_NOFILTERING;

	return flags;
}

static image_t *Shader_FindImage( shader_t *shader, char *name, int flags, float bumpScale )
{
	image_t *image;

	if( !Q_stricmp( name, "$whiteimage" ) || !Q_stricmp( name, "*white" ) )
		return r_whitetexture;
	if( !Q_stricmp( name, "$blackimage" ) || !Q_stricmp( name, "*black" ) )
		return r_blacktexture;
	if( !Q_stricmp( name, "$greyimage" ) || !Q_stricmp( name, "*grey" ) )
		return r_greytexture;
	if( !Q_stricmp( name, "$blankbumpimage" ) || !Q_stricmp( name, "*blankbump" ) )
		return r_blankbumptexture;
	if( !Q_stricmp( name, "$particleimage" ) || !Q_stricmp( name, "*particle" ) )
		return r_particletexture;
	if( !Q_strnicmp( name, "*lm", 3 ) )
	{
		Com_DPrintf( S_COLOR_YELLOW "WARNING: shader %s has a stage with explicit lightmap image\n", shader->name );
		return r_whitetexture;
	}

	image = R_FindImage( name, NULL, flags, bumpScale );
	if( !image )
	{
		Com_DPrintf( S_COLOR_YELLOW "WARNING: shader %s has a stage with no image: %s\n", shader->name, name );
		return r_defaultImage;
	}

	return image;
}

/****************** shader keyword functions ************************/

static void Shader_Cull( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	char *token;

	shader->flags &= ~( SHADER_CULL_FRONT|SHADER_CULL_BACK );

	token = Shader_ParseString( ptr );
	if( !strcmp( token, "disable" ) || !strcmp( token, "none" ) || !strcmp( token, "twosided" ) )
		;
	else if( !strcmp( token, "back" ) || !strcmp( token, "backside" ) || !strcmp( token, "backsided" ) )
		shader->flags |= SHADER_CULL_BACK;
	else
		shader->flags |= SHADER_CULL_FRONT;
}

static void Shader_NoMipMaps( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	r_shaderNoMipMaps = r_shaderNoPicMip = qtrue;
}

static void Shader_NoPicMip( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	r_shaderNoPicMip = qtrue;
}

static void Shader_NoCompress( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	r_shaderNoCompress = qtrue;
}

static void Shader_NoFiltering( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	r_shaderNoFiltering = qtrue;
	shader->flags |= SHADER_NO_TEX_FILTERING;
}

static void Shader_DeformVertexes( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	char *token;
	deformv_t *deformv;

	if( shader->numdeforms == MAX_SHADER_DEFORMVS )
	{
		Com_Printf( S_COLOR_YELLOW "WARNING: shader %s has too many deforms\n", shader->name );
		Shader_SkipLine( ptr );
		return;
	}

	deformv = &r_currentDeforms[shader->numdeforms];

	token = Shader_ParseString( ptr );
	if( !strcmp( token, "wave" ) )
	{
		deformv->type = DEFORMV_WAVE;
		deformv->args[0] = Shader_ParseFloat( ptr );
		if( deformv->args[0] )
			deformv->args[0] = 1.0f / deformv->args[0];
		else
			deformv->args[0] = 100.0f;
		Shader_ParseFunc( ptr, &deformv->func );
	}
	else if( !strcmp( token, "normal" ) )
	{
		shader->flags |= SHADER_DEFORMV_NORMAL;
		deformv->type = DEFORMV_NORMAL;
		deformv->args[0] = Shader_ParseFloat( ptr );
		deformv->args[1] = Shader_ParseFloat( ptr );
	}
	else if( !strcmp( token, "bulge" ) )
	{
		deformv->type = DEFORMV_BULGE;
		Shader_ParseVector( ptr, deformv->args, 4 );
	}
	else if( !strcmp( token, "move" ) )
	{
		deformv->type = DEFORMV_MOVE;
		Shader_ParseVector( ptr, deformv->args, 3 );
		Shader_ParseFunc( ptr, &deformv->func );
	}
	else if( !strcmp( token, "autosprite" ) )
	{
		deformv->type = DEFORMV_AUTOSPRITE;
		shader->flags |= SHADER_AUTOSPRITE;
	}
	else if( !strcmp( token, "autosprite2" ) )
	{
		deformv->type = DEFORMV_AUTOSPRITE2;
		shader->flags |= SHADER_AUTOSPRITE;
	}
	else if( !strcmp( token, "projectionShadow" ) )
		deformv->type = DEFORMV_PROJECTION_SHADOW;
	else if( !strcmp( token, "autoparticle" ) )
		deformv->type = DEFORMV_AUTOPARTICLE;
#ifdef HARDWARE_OUTLINES
	else if( !strcmp( token, "outline" ) )
		deformv->type = DEFORMV_OUTLINE;
#endif
	else
	{
		Shader_SkipLine( ptr );
		return;
	}

	shader->numdeforms++;
}

static void Shader_SkyParmsExt( shader_t *shader, shaderpass_t *pass, const char **ptr, qboolean underscore )
{
	float skyheight;
	shader_t *farboxShaders[6];
	shader_t *nearboxShaders[6];

	if( shader->skydome )
		R_FreeSkydome( shader->skydome );

	Shader_ParseSkySides( ptr, farboxShaders, qtrue, underscore );

	skyheight = Shader_ParseFloat( ptr );
	if( !skyheight )
		skyheight = 512.0f;

	Shader_ParseSkySides( ptr, nearboxShaders, qfalse, underscore );

	shader->skydome = R_CreateSkydome( skyheight, farboxShaders, nearboxShaders );
	shader->flags |= SHADER_SKY;
}

static void Shader_SkyParms( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	Shader_SkyParmsExt( shader, pass, ptr, qtrue );
}

static void Shader_SkyParms2( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	Shader_SkyParmsExt( shader, pass, ptr, qfalse );
}

static void Shader_FogParms( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	float div;
	vec3_t color, fcolor;

	if( !r_ignorehwgamma->integer )
		div = 1.0f / pow( 2, max( 0, floor( r_overbrightbits->value ) ) );
	else
		div = 1.0f;

	Shader_ParseVector( ptr, color, 3 );
	ColorNormalize( color, fcolor );
	VectorScale( fcolor, div, fcolor );

	shader->fog_color[0] = R_FloatToByte( fcolor[0] );
	shader->fog_color[1] = R_FloatToByte( fcolor[1] );
	shader->fog_color[2] = R_FloatToByte( fcolor[2] );
	shader->fog_color[3] = 255;
	shader->fog_dist = Shader_ParseFloat( ptr );
	if( shader->fog_dist <= 0.1 )
		shader->fog_dist = 128.0;

	shader->fog_clearDist = Shader_ParseFloat( ptr );
	if( shader->fog_clearDist > shader->fog_dist - 128 )
		shader->fog_clearDist = shader->fog_dist - 128;
	if( shader->fog_clearDist <= 0.0 )
		shader->fog_clearDist = 0;
}

static void Shader_Sort( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	char *token;

	token = Shader_ParseString( ptr );
	if( !strcmp( token, "portal" ) )
		shader->sort = SHADER_SORT_PORTAL;
	else if( !strcmp( token, "sky" ) )
		shader->sort = SHADER_SORT_SKY;
	else if( !strcmp( token, "opaque" ) )
		shader->sort = SHADER_SORT_OPAQUE;
	else if( !strcmp( token, "banner" ) )
		shader->sort = SHADER_SORT_BANNER;
	else if( !strcmp( token, "underwater" ) )
		shader->sort = SHADER_SORT_UNDERWATER;
	else if( !strcmp( token, "additive" ) )
		shader->sort = SHADER_SORT_ADDITIVE;
	else if( !strcmp( token, "nearest" ) )
		shader->sort = SHADER_SORT_NEAREST;
	else
	{
		shader->sort = atoi( token );
		if( shader->sort > SHADER_SORT_NEAREST )
			shader->sort = SHADER_SORT_NEAREST;
	}
}

static void Shader_Portal( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	shader->flags |= SHADER_PORTAL;
	shader->sort = SHADER_SORT_PORTAL;
}

static void Shader_PolygonOffset( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	r_shaderPolygonOffset = qtrue;
}

static void Shader_EntityMergable( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	shader->flags |= SHADER_ENTITY_MERGABLE;
}

static void Shader_If( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	if( !Shader_ParseConditions( ptr, shader ) )
	{
		if( !Shader_SkipConditionBlock( ptr ) )
			Com_Printf( S_COLOR_YELLOW "WARNING: Mismatched if/endif pair in shader %s\n", shader->name );
	}
}

static void Shader_Endif( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
}

static void Shader_NoModulativeDlights( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	shader->flags |= SHADER_NO_MODULATIVE_DLIGHTS;
}

static void Shader_OffsetMappingScale( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	shader->offsetmapping_scale = Shader_ParseFloat( ptr );
	if( shader->offsetmapping_scale <= 0 )
		shader->offsetmapping_scale = 0;
}

static void Shader_GlossExponent( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	shader->gloss_exponent = Shader_ParseFloat( ptr );
	if( shader->gloss_exponent <= 0 )
		shader->gloss_exponent = 0;
}

static void Shader_NoDepthTest( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	shader->flags |= SHADER_NO_DEPTH_TEST;
}

#define MAX_SHADER_TEMPLATE_ARGS	12
static void Shader_Template( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	int i;
	char *tmpl, *buf, *out, *token;
	char *pos, *before, *ptr2;
	const char *ptr_backup;
	char backup;
	char args[MAX_SHADER_TEMPLATE_ARGS][MAX_QPATH];
	shadercache_t *cache;
	int num_args;
	size_t length;

	token = Shader_ParseString( ptr );
	if( !*token )
	{
		Com_Printf( S_COLOR_YELLOW "WARNING: missing template arguments in shader %s\n", shader->name );
		Shader_SkipLine( ptr );
		return;
	}

	// search for template in cache
	tmpl = token;
	Shader_GetCache( tmpl, &cache );
	if( !cache )
	{
		Com_Printf( S_COLOR_YELLOW "WARNING: shader template %s not found in cache\n", tmpl );
		Shader_SkipLine( ptr );
		return;
	}

	// aha, found it

	// find total length
	buf = cache->buffer + cache->offset;
	ptr2 = buf;
	Shader_SkipBlock( (const char **)&ptr2 );
	length = ptr2 - buf;

	// replace the following char with a EOF
	backup = cache->buffer[ptr2 - cache->buffer];
	cache->buffer[ptr2 - cache->buffer] = '\0';

	// now count occurences of each argument in a template
	ptr_backup = *ptr;
	for( i = 1, num_args = 0; ; i++ )
	{
		char arg[8];
		size_t arg_count;

		token = Shader_ParseString( ptr );
		if( !*token ) {
			break;
		}

		if( num_args == MAX_SHADER_TEMPLATE_ARGS ) {
			Com_Printf( S_COLOR_YELLOW "WARNING: shader template %s has too many arguments\n", tmpl );
			break;
		}

		Q_snprintfz( arg, sizeof( arg ), "$%i", i );
		arg_count = Q_strcount( buf, arg );
		length += arg_count * strlen( token );

		Q_strncpyz( args[num_args], token, sizeof( args[0] ) );
		num_args++;
	}

	// (re)allocate string buffer, if needed
	if( !r_shaderTemplateBuf )
		r_shaderTemplateBuf = Shader_Malloc( length + 1 );
	else
		r_shaderTemplateBuf = Shader_Realloc( r_shaderTemplateBuf, length + 1 );

	// start with an empty string
	out = r_shaderTemplateBuf;
	memset( out, 0, length + 1 );

	// now replace all occurences of placeholders
	pos = before = buf;
	*ptr = ptr_backup;
	while( (pos = strstr( pos, "$" )) != NULL )
	{
		int i, arg;

		// calculate the placeholder index
		for( i = 1, arg = 0; ; i++ )
		{
			if( *(pos+i) >= '1' && *(pos+i) <= '9' )
			{
				arg = arg * 10 + (int)(*(pos+i) - '0');
				continue;
			}
			break;
		}

		if( arg && arg <= num_args )
		{
			token = args[arg-1];

			// hack in EOF, then concat
			*pos = '\0';
			strcat( out, before );
			strcat( out, token );
			*pos = '$';

			pos += i;
			before = pos;
		}
		else
		{
			// treat a single '$' as a regular character
			pos += i;
		}
	}

	strcat( out, before );

	// skip the initial { and change the original pointer
	*ptr = r_shaderTemplateBuf;
	COM_ParseExt( ptr, qtrue );

	// restore backup char
	cache->buffer[ptr2 - cache->buffer] = backup;
}

static void Shader_Skip( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	Shader_SkipLine( ptr );
}

static const shaderkey_t shaderkeys[] =
{
	{ "cull", Shader_Cull },
	{ "skyparms", Shader_SkyParms },
	{ "skyparms2", Shader_SkyParms2 },
	{ "fogparms", Shader_FogParms },
	{ "nomipmaps", Shader_NoMipMaps },
	{ "nopicmip", Shader_NoPicMip },
	{ "nocompress",	Shader_NoCompress },
	{ "nofiltering", Shader_NoFiltering },
	{ "polygonoffset", Shader_PolygonOffset },
	{ "sort", Shader_Sort },
	{ "deformvertexes", Shader_DeformVertexes },
	{ "portal", Shader_Portal },
	{ "entitymergable", Shader_EntityMergable },
	{ "if",	Shader_If },
	{ "endif", Shader_Endif },
	{ "nomodulativedlights", Shader_NoModulativeDlights },
	{ "offsetmappingscale", Shader_OffsetMappingScale },
	{ "glossexponent", Shader_GlossExponent },
	{ "nodepthtest", Shader_NoDepthTest },
	{ "template", Shader_Template },
	{ "skip", Shader_Skip },

	{ NULL,	NULL }
};

// ===============================================================

static qboolean Shaderpass_LoadMaterial( image_t **normalmap, image_t **glossmap, image_t **decalmap, const char *name, int addFlags, float bumpScale )
{
	image_t *images[3];

	// set defaults
	images[0] = images[1] = images[2] = NULL;

	// load normalmap image
	images[0] = R_FindImage( name, "_bump", addFlags|IT_HEIGHTMAP, bumpScale );
	if( !images[0] )
	{
		images[0] = R_FindImage( name, "_norm", (addFlags|IT_NORMALMAP) & ~IT_HEIGHTMAP , 0 );

		if( !images[0] )
		{
			if( r_lighting_diffuse2heightmap->integer ) {
				// convert diffuse to heightmap
				images[0] = R_FindImage( name, NULL, addFlags|IT_HEIGHTMAP, 2 );
				if( !images[0] )
					return qfalse;
			}
			else if( r_lighting_useblankbumpmap->integer ) {
				// use blank normalmap texture
				*normalmap = r_blankbumptexture;
				*glossmap = *decalmap = NULL;
				return qtrue;
			}
		}
	}

	// load glossmap image
	if( r_lighting_specular->integer )
		images[1] = R_FindImage( name, "_gloss", addFlags & ~IT_HEIGHTMAP, 0 );

	images[2] = R_FindImage( name, "_decal", addFlags & ~IT_HEIGHTMAP, 0 );
	if( !images[2] )
		images[2] = R_FindImage( name, "_add", addFlags & ~IT_HEIGHTMAP, 0 );

	*normalmap = images[0];
	*glossmap = images[1];
	*decalmap = images[2];

	return qtrue;
}

static void Shaderpass_MapExt( shader_t *shader, shaderpass_t *pass, int addFlags, const char **ptr )
{
	int flags;
	char *token;

	Shader_FreePassCinematics( pass );

	token = Shader_ParseString( ptr );
	if( token[0] == '$' )
	{
		token++;
		if( !strcmp( token, "lightmap" ) )
		{
			r_shaderHasLightmapPass = qtrue;

			pass->tcgen = TC_GEN_LIGHTMAP;
			pass->flags = ( pass->flags & ~( SHADERPASS_PORTALMAP|SHADERPASS_DLIGHT ) ) | SHADERPASS_LIGHTMAP;
			pass->anim_fps = 0;
			pass->anim_frames[0] = NULL;
			return;
		}
		else if( !strcmp( token, "dlight" ) )
		{
			r_shaderHasDlightPass = qtrue;

			pass->tcgen = TC_GEN_BASE;
			pass->flags = ( pass->flags & ~( SHADERPASS_LIGHTMAP|SHADERPASS_PORTALMAP ) ) | SHADERPASS_DLIGHT;
			pass->anim_fps = 0;
			pass->anim_frames[0] = NULL;
			return;
		}
		else if( !strcmp( token, "portalmap" ) || !strcmp( token, "mirrormap" ) )
		{
			pass->tcgen = TC_GEN_PROJECTION;
			pass->flags = ( pass->flags & ~( SHADERPASS_LIGHTMAP|SHADERPASS_DLIGHT ) ) | SHADERPASS_PORTALMAP;
			pass->anim_fps = 0;
			pass->anim_frames[0] = NULL;
			if( ( shader->flags & SHADER_PORTAL ) && ( shader->sort == SHADER_SORT_PORTAL ) )
				shader->sort = 0; // reset sorting so we can figure it out later. FIXME?
			shader->flags |= SHADER_PORTAL|( r_portalmaps->integer ? SHADER_PORTAL_CAPTURE : 0 );
			return;
		}
		else
		{
			token--;
		}
	}

	flags = Shader_SetImageFlags( shader ) | addFlags;
	pass->tcgen = TC_GEN_BASE;
	pass->flags &= ~( SHADERPASS_LIGHTMAP|SHADERPASS_DLIGHT|SHADERPASS_PORTALMAP );
	pass->anim_fps = 0;
	pass->anim_frames[0] = Shader_FindImage( shader, token, flags, 0 );
	if( !pass->anim_frames[0] )
		Com_DPrintf( S_COLOR_YELLOW "Shader %s has a stage with no image: %s\n", shader->name, token );
}

static void Shaderpass_AnimMapExt( shader_t *shader, shaderpass_t *pass, int addFlags, const char **ptr )
{
	int flags;
	char *token;

	Shader_FreePassCinematics( pass );

	flags = Shader_SetImageFlags( shader ) | addFlags;

	pass->tcgen = TC_GEN_BASE;
	pass->flags &= ~( SHADERPASS_LIGHTMAP|SHADERPASS_DLIGHT|SHADERPASS_PORTALMAP );
	pass->anim_fps = Shader_ParseFloat( ptr );
	pass->anim_numframes = 0;

	for(;; )
	{
		token = Shader_ParseString( ptr );
		if( !token[0] )
			break;
		if( pass->anim_numframes < MAX_SHADER_ANIM_FRAMES )
			pass->anim_frames[pass->anim_numframes++] = Shader_FindImage( shader, token, flags, 0 );
	}

	if( pass->anim_numframes == 0 )
		pass->anim_fps = 0;
}

static void Shaderpass_CubeMapExt( shader_t *shader, shaderpass_t *pass, int addFlags, int tcgen, const char **ptr )
{
	int flags;
	char *token;

	Shader_FreePassCinematics( pass );

	token = Shader_ParseString( ptr );
	flags = Shader_SetImageFlags( shader ) | addFlags;
	pass->anim_fps = 0;
	pass->flags &= ~( SHADERPASS_LIGHTMAP|SHADERPASS_DLIGHT|SHADERPASS_PORTALMAP );

	if( !glConfig.ext.texture_cube_map )
	{
		Com_DPrintf( S_COLOR_YELLOW "Shader %s has an unsupported cubemap stage: %s.\n", shader->name );
		pass->anim_frames[0] = r_notexture;
		pass->tcgen = TC_GEN_BASE;
		return;
	}

	pass->anim_frames[0] = R_FindImage( token, NULL, flags|IT_CUBEMAP, 0 );
	if( pass->anim_frames[0] )
	{
		pass->tcgen = tcgen;
	}
	else
	{
		Com_DPrintf( S_COLOR_YELLOW "Shader %s has a stage with no image: %s\n", shader->name, token );
		pass->anim_frames[0] = r_notexture;
		pass->tcgen = TC_GEN_BASE;
	}
}

static void Shaderpass_Map( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	Shaderpass_MapExt( shader, pass, 0, ptr );
}

static void Shaderpass_ClampMap( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	Shaderpass_MapExt( shader, pass, IT_CLAMP, ptr );
}

static void Shaderpass_AnimMap( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	Shaderpass_AnimMapExt( shader, pass, 0, ptr );
}

static void Shaderpass_AnimClampMap( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	Shaderpass_AnimMapExt( shader, pass, IT_CLAMP, ptr );
}

static void Shaderpass_CubeMap( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	Shaderpass_CubeMapExt( shader, pass, IT_CLAMP, TC_GEN_REFLECTION, ptr );
}

static void Shaderpass_ShadeCubeMap( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	Shaderpass_CubeMapExt( shader, pass, IT_CLAMP, TC_GEN_REFLECTION_CELLSHADE, ptr );
}

static void Shaderpass_VideoMap( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	char *token;

	Shader_FreePassCinematics( pass );

	token = Shader_ParseString( ptr );

	pass->cin = R_StartCinematic( token );
	pass->tcgen = TC_GEN_BASE;
	pass->anim_fps = 0;
	pass->flags &= ~(SHADERPASS_LIGHTMAP|SHADERPASS_DLIGHT|SHADERPASS_PORTALMAP);
}

static void Shaderpass_NormalMap( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	int flags;
	char *token;
	float bumpScale = 0;

	if( !glConfig.ext.GLSL )
	{
		Com_DPrintf( S_COLOR_YELLOW "WARNING: shader %s has a normalmap stage, while GLSL is not supported\n", shader->name );
		Shader_SkipLine( ptr );
		return;
	}

	Shader_FreePassCinematics( pass );

	flags = Shader_SetImageFlags( shader );
	token = Shader_ParseString( ptr );

	if( !strcmp( token, "$heightmap" ) )
	{
		flags |= IT_HEIGHTMAP;
		bumpScale = Shader_ParseFloat( ptr );
		token = Shader_ParseString( ptr );
	}

	pass->tcgen = TC_GEN_BASE;
	pass->flags &= ~( SHADERPASS_LIGHTMAP|SHADERPASS_DLIGHT|SHADERPASS_PORTALMAP );
	pass->anim_frames[1] = Shader_FindImage( shader, token, flags, bumpScale );
	if( pass->anim_frames[1] )
	{
		pass->program = DEFAULT_GLSL_PROGRAM;
		pass->program_type = GLSL_PROGRAM_TYPE_MATERIAL;
	}

	token = Shader_ParseString( ptr );
	if( !strcmp( token, "$noimage" ) )
		pass->anim_frames[0] = r_whitetexture;
	else
		pass->anim_frames[0] = Shader_FindImage( shader, token, Shader_SetImageFlags( shader ), 0 );
}

static void Shaderpass_Material( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	int i, flags;
	char *token;
	float bumpScale = 0;

	if( !glConfig.ext.GLSL )
	{
		Com_DPrintf( S_COLOR_YELLOW "WARNING: shader %s has a normalmap stage, while GLSL is not supported\n", shader->name );
		Shader_SkipLine( ptr );
		return;
	}

	Shader_FreePassCinematics( pass );

	flags = Shader_SetImageFlags( shader );
	token = Shader_ParseString( ptr );

	if( token[0] == '$' )
	{
		token++;
		if( !strcmp( token, "left" ) )
		{
			flags |= IT_LEFTHALF;
			token = Shader_ParseString( ptr );
		}
		else if( !strcmp( token, "right" ) )
		{
			flags |= IT_RIGHTHALF;
			token = Shader_ParseString( ptr );
		}
		else
		{
			token--;
		}
	}

	pass->anim_frames[0] = Shader_FindImage( shader, token, flags, 0 );
	if( !pass->anim_frames[0] )
	{
		Com_DPrintf( S_COLOR_YELLOW "WARNING: failed to load base/diffuse image for material %s in shader %s.\n", token, shader->name );
		return;
	}

	pass->anim_frames[1] = pass->anim_frames[2] = pass->anim_frames[3] = NULL;

	pass->tcgen = TC_GEN_BASE;
	pass->flags &= ~( SHADERPASS_LIGHTMAP|SHADERPASS_DLIGHT|SHADERPASS_PORTALMAP );
	if( pass->rgbgen.type == RGB_GEN_UNKNOWN )
		pass->rgbgen.type = RGB_GEN_IDENTITY;

	while( 1 )
	{
		token = Shader_ParseString( ptr );
		if( !*token )
			break;

		if( Q_isdigit( token ) )
		{
			flags |= IT_HEIGHTMAP;
			bumpScale = atoi( token );
		}
		else if( !pass->anim_frames[1] )
		{
			pass->anim_frames[1] = Shader_FindImage( shader, token, flags, bumpScale );
			if( !pass->anim_frames[1] )
			{
				Com_DPrintf( S_COLOR_YELLOW "WARNING: missing normalmap image %s in shader %s.\n", token, shader->name );
				pass->anim_frames[1] = r_blankbumptexture;
			}
			else
			{
				pass->program = DEFAULT_GLSL_PROGRAM;
				pass->program_type = GLSL_PROGRAM_TYPE_MATERIAL;
			}
			flags &= ~IT_HEIGHTMAP;
		}
		else if( !pass->anim_frames[2] )
		{
			if( strcmp( token, "-" ) && r_lighting_specular->integer )
			{
				pass->anim_frames[2] = Shader_FindImage( shader, token, flags, 0 );
				if( !pass->anim_frames[2] )
					Com_DPrintf( S_COLOR_YELLOW "WARNING: missing glossmap image %s in shader %s.\n", token, shader->name );
			}

			// set gloss to r_blacktexture so we know we have already parsed the gloss image
			if( pass->anim_frames[2] == NULL )
				pass->anim_frames[2] = r_blacktexture;
		}
		else
		{
			// parse decal images
			for( i = 3; i < 5; i++ ) {
				image_t *decal;

				if( pass->anim_frames[i] ) {
					continue;
				}

				decal = r_whitetexture;
				if( strcmp( token, "-" ) ) {
					decal = Shader_FindImage( shader, token, flags, 0 );
					if( !decal ) {
						decal = r_whitetexture;
						Com_DPrintf( S_COLOR_YELLOW "WARNING: missing decal image %s in shader %s.\n", token, shader->name );
					}
				}

				pass->anim_frames[i] = decal;
				break;
			}
		}
	}

	// black texture => no gloss, so don't waste time in the GLSL program
	if( pass->anim_frames[2] == r_blacktexture )
		pass->anim_frames[2] = NULL;

	for( i = 3; i < 5; i++ ) {
		if( pass->anim_frames[i] == r_whitetexture )
			pass->anim_frames[i] = NULL;
	}

	if( pass->anim_frames[1] )
		return;

	// try loading default images
	if( Shaderpass_LoadMaterial( &pass->anim_frames[1], &pass->anim_frames[2], &pass->anim_frames[3], pass->anim_frames[0]->name, flags, bumpScale ) )
	{
		pass->program = DEFAULT_GLSL_PROGRAM;
		pass->program_type = GLSL_PROGRAM_TYPE_MATERIAL;
	}
	else
	{
		Com_DPrintf( S_COLOR_YELLOW "WARNING: failed to load default images for material %s in shader %s.\n", pass->anim_frames[0]->name, shader->name );
	}
}

static void Shaderpass_Distortion( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	int flags;
	char *token;
	float bumpScale = 0;

	if( !glConfig.ext.GLSL || !r_portalmaps->integer )
	{
		Com_DPrintf( S_COLOR_YELLOW "WARNING: shader %s has a distortion stage, while GLSL is not supported\n", shader->name );
		Shader_SkipLine( ptr );
		return;
	}

	Shader_FreePassCinematics( pass );

	flags = Shader_SetImageFlags( shader );
	pass->flags &= ~( SHADERPASS_LIGHTMAP|SHADERPASS_DLIGHT|SHADERPASS_PORTALMAP );
	pass->anim_frames[0] = pass->anim_frames[1] = NULL;

	while( 1 )
	{
		token = Shader_ParseString( ptr );
		if( !*token )
			break;

		if( Q_isdigit( token ) )
		{
			flags |= IT_HEIGHTMAP;
			bumpScale = atoi( token );
		}
		else if( !pass->anim_frames[0] )
		{
			pass->anim_frames[0] = Shader_FindImage( shader, token, flags, 0 );
			if( !pass->anim_frames[0] )
			{
				Com_DPrintf( S_COLOR_YELLOW "WARNING: missing dudvmap image %s in shader %s.\n", token, shader->name );
				pass->anim_frames[0] = r_blacktexture;
			}

			pass->program = DEFAULT_GLSL_DISTORTION_PROGRAM;
			pass->program_type = GLSL_PROGRAM_TYPE_DISTORTION;
		}
		else
		{
			pass->anim_frames[1] = Shader_FindImage( shader, token, flags, bumpScale );
			if( !pass->anim_frames[1] )
				Com_DPrintf( S_COLOR_YELLOW "WARNING: missing normalmap image %s in shader.\n", token, shader->name );
			flags &= ~IT_HEIGHTMAP;
		}
	}

	if( pass->rgbgen.type == RGB_GEN_UNKNOWN )
	{
		pass->rgbgen.type = RGB_GEN_CONST;
		VectorClear( pass->rgbgen.args );
	}

	shader->flags |= SHADER_PORTAL|SHADER_PORTAL_CAPTURE|SHADER_PORTAL_CAPTURE2;
}

// <base> <cellshade> [diffuse] [decal] [entitydecal] [stripes] [celllight]
static void Shaderpass_Cellshade( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	int i;
	int flags;
	char *token;

	if( !glConfig.ext.GLSL )
	{
		Com_DPrintf( S_COLOR_YELLOW "WARNING: shader %s has a distortion stage, while GLSL is not supported\n", shader->name );
		Shader_SkipLine( ptr );
		return;
	}

	Shader_FreePassCinematics( pass );

	flags = Shader_SetImageFlags( shader );
	pass->tcgen = TC_GEN_BASE;
	pass->flags &= ~( SHADERPASS_LIGHTMAP|SHADERPASS_DLIGHT|SHADERPASS_PORTALMAP );
	if( pass->rgbgen.type == RGB_GEN_UNKNOWN )
		pass->rgbgen.type = RGB_GEN_IDENTITY;

	pass->anim_fps = 0;
	memset( pass->anim_frames, 0, sizeof( pass->anim_frames ) );

	// at least two valid images are required: 'base' and 'cellshade'
	for( i = 0; i < 2; i++ ) {
		token = Shader_ParseString( ptr );
		if( *token && strcmp( token, "-" ) )
			pass->anim_frames[i] = Shader_FindImage( shader, token, flags | (i ? IT_CLAMP|IT_CUBEMAP : 0), 0 );

		if( !pass->anim_frames[i] ) {
			Com_DPrintf( S_COLOR_YELLOW "Shader %s has a stage with no image: %s\n", shader->name, token );
			pass->anim_frames[0] = r_notexture;
			return;
		}
	}

	pass->program_type = GLSL_PROGRAM_TYPE_CELLSHADE;
	pass->program = DEFAULT_GLSL_CELLSHADE_PROGRAM;

	// parse optional images: [diffuse] [decal] [entitydecal] [stripes] [celllight]
	for( i = 0; i < 5; i++ ) {
		token = Shader_ParseString( ptr );
		if( !*token ) {
			break;
		}
		if( strcmp( token, "-" ) ) {
			pass->anim_frames[i+2] = Shader_FindImage( shader, token, flags | (i == 4 ? IT_CLAMP|IT_CUBEMAP : 0), 0 );
		}
	}
}

static void Shaderpass_RGBGen( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	char *token;
	qboolean wave = qfalse;

	token = Shader_ParseString( ptr );
	if( !strcmp( token, "identitylighting" ) )
		pass->rgbgen.type = _RGB_GEN_IDENTITY_LIGHTING;
	else if( !strcmp( token, "identity" ) )
		pass->rgbgen.type = RGB_GEN_IDENTITY;
	else if( !strcmp( token, "wave" ) )
	{
		pass->rgbgen.type = RGB_GEN_WAVE;
		pass->rgbgen.args[0] = 1.0f;
		pass->rgbgen.args[1] = 1.0f;
		pass->rgbgen.args[2] = 1.0f;
		Shader_ParseFunc( ptr, pass->rgbgen.func );
	}
	else if( ( wave = !strcmp( token, "colorwave" ) ? qtrue : qfalse ) )
	{
		pass->rgbgen.type = RGB_GEN_WAVE;
		Shader_ParseVector( ptr, pass->rgbgen.args, 3 );
		Shader_ParseFunc( ptr, pass->rgbgen.func );
	}
	else if( !strcmp( token, "custom" ) || !strcmp( token, "teamcolor" )
		|| ( wave = !strcmp( token, "teamcolorwave" ) || !strcmp( token, "customcolorwave" ) ? qtrue : qfalse ) )
	{
		pass->rgbgen.type = RGB_GEN_CUSTOMWAVE;
		pass->rgbgen.args[0] = (int)Shader_ParseFloat( ptr );
		if( pass->rgbgen.args[0] < 0 || pass->rgbgen.args[0] >= NUM_CUSTOMCOLORS )
			pass->rgbgen.args[0] = 0;
		pass->rgbgen.func->type = SHADER_FUNC_NONE;
		if( wave )
			Shader_ParseFunc( ptr, pass->rgbgen.func );
	}
	else if( !strcmp( token, "entity" ) || ( wave = !strcmp( token, "entitycolorwave" ) ? qtrue : qfalse ) )
	{
		pass->rgbgen.type = RGB_GEN_ENTITYWAVE;
		pass->rgbgen.func->type = SHADER_FUNC_NONE;
		if( wave )
		{
			Shader_ParseVector( ptr, pass->rgbgen.args, 3 );
			Shader_ParseFunc( ptr, pass->rgbgen.func );
		}
	}
	else if( !strcmp( token, "oneminusentity" ) )
		pass->rgbgen.type = RGB_GEN_ONE_MINUS_ENTITY;
	else if( !strcmp( token, "vertex" ) )
		pass->rgbgen.type = RGB_GEN_VERTEX;
	else if( !strcmp( token, "oneminusvertex" ) )
		pass->rgbgen.type = RGB_GEN_ONE_MINUS_VERTEX;
	else if( !strcmp( token, "lightingdiffuse" ) )
		pass->rgbgen.type = RGB_GEN_LIGHTING_DIFFUSE;
	else if( !strcmp( token, "lightingdiffuseonly" ) )
		pass->rgbgen.type = RGB_GEN_LIGHTING_DIFFUSE_ONLY;
	else if( !strcmp( token, "lightingambientonly" ) )
		pass->rgbgen.type = RGB_GEN_LIGHTING_AMBIENT_ONLY;
	else if( !strcmp( token, "exactvertex" ) )
		pass->rgbgen.type = RGB_GEN_EXACT_VERTEX;
	else if( !strcmp( token, "const" ) || !strcmp( token, "constant" ) )
	{
		float div;
		vec3_t color;

		if( !r_ignorehwgamma->integer )
			div = 1.0f / pow( 2, max( 0, floor( r_overbrightbits->value ) ) );
		else
			div = 1.0f;

		pass->rgbgen.type = RGB_GEN_CONST;
		Shader_ParseVector( ptr, color, 3 );
		ColorNormalize( color, pass->rgbgen.args );
		VectorScale( pass->rgbgen.args, div, pass->rgbgen.args );
	}
}

static void Shaderpass_AlphaGen( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	char *token;

	token = Shader_ParseString( ptr );
	if( !strcmp( token, "portal" ) )
	{
		pass->alphagen.type = ALPHA_GEN_PORTAL;
		pass->alphagen.args[0] = fabs( Shader_ParseFloat( ptr ) );
		if( !pass->alphagen.args[0] )
			pass->alphagen.args[0] = 256;
		pass->alphagen.args[0] = pass->alphagen.args[0];
	}
	else if( !strcmp( token, "vertex" ) )
		pass->alphagen.type = ALPHA_GEN_VERTEX;
	else if( !strcmp( token, "oneminusvertex" ) )
		pass->alphagen.type = ALPHA_GEN_ONE_MINUS_VERTEX;
	else if( !strcmp( token, "entity" ) )
		pass->alphagen.type = ALPHA_GEN_ENTITY;
	else if( !strcmp( token, "wave" ) )
	{
		pass->alphagen.type = ALPHA_GEN_WAVE;
		Shader_ParseFunc( ptr, pass->alphagen.func );
	}
	else if( !strcmp( token, "lightingspecular" ) )
	{
		pass->alphagen.type = ALPHA_GEN_SPECULAR;
		pass->alphagen.args[0] = fabs( Shader_ParseFloat( ptr ) );
		if( !pass->alphagen.args[0] )
			pass->alphagen.args[0] = 5.0f;
	}
	else if( !strcmp( token, "const" ) || !strcmp( token, "constant" ) )
	{
		pass->alphagen.type = ALPHA_GEN_CONST;
		pass->alphagen.args[0] = fabs( Shader_ParseFloat( ptr ) );
	}
	else if( !strcmp( token, "dot" ) )
	{
		pass->alphagen.type = ALPHA_GEN_DOT;
		pass->alphagen.args[0] = fabs( Shader_ParseFloat( ptr ) );
		pass->alphagen.args[1] = fabs( Shader_ParseFloat( ptr ) );
		if( !pass->alphagen.args[1] )
			pass->alphagen.args[1] = 1.0f;
	}
	else if( !strcmp( token, "oneminusdot" ) )
	{
		pass->alphagen.type = ALPHA_GEN_ONE_MINUS_DOT;
		pass->alphagen.args[0] = fabs( Shader_ParseFloat( ptr ) );
		pass->alphagen.args[1] = fabs( Shader_ParseFloat( ptr ) );
		if( !pass->alphagen.args[1] )
			pass->alphagen.args[1] = 1.0f;
	}
}

static inline int Shaderpass_SrcBlendBits( char *token )
{
	if( !strcmp( token, "gl_zero" ) )
		return GLSTATE_SRCBLEND_ZERO;
	if( !strcmp( token, "gl_one" ) )
		return GLSTATE_SRCBLEND_ONE;
	if( !strcmp( token, "gl_dst_color" ) )
		return GLSTATE_SRCBLEND_DST_COLOR;
	if( !strcmp( token, "gl_one_minus_dst_color" ) )
		return GLSTATE_SRCBLEND_ONE_MINUS_DST_COLOR;
	if( !strcmp( token, "gl_src_alpha" ) )
		return GLSTATE_SRCBLEND_SRC_ALPHA;
	if( !strcmp( token, "gl_one_minus_src_alpha" ) )
		return GLSTATE_SRCBLEND_ONE_MINUS_SRC_ALPHA;
	if( !strcmp( token, "gl_dst_alpha" ) )
		return GLSTATE_SRCBLEND_DST_ALPHA;
	if( !strcmp( token, "gl_one_minus_dst_alpha" ) )
		return GLSTATE_SRCBLEND_ONE_MINUS_DST_ALPHA;
	return GLSTATE_SRCBLEND_ONE;
}

static inline int Shaderpass_DstBlendBits( char *token )
{
	if( !strcmp( token, "gl_zero" ) )
		return GLSTATE_DSTBLEND_ZERO;
	if( !strcmp( token, "gl_one" ) )
		return GLSTATE_DSTBLEND_ONE;
	if( !strcmp( token, "gl_src_color" ) )
		return GLSTATE_DSTBLEND_SRC_COLOR;
	if( !strcmp( token, "gl_one_minus_src_color" ) )
		return GLSTATE_DSTBLEND_ONE_MINUS_SRC_COLOR;
	if( !strcmp( token, "gl_src_alpha" ) )
		return GLSTATE_DSTBLEND_SRC_ALPHA;
	if( !strcmp( token, "gl_one_minus_src_alpha" ) )
		return GLSTATE_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	if( !strcmp( token, "gl_dst_alpha" ) )
		return GLSTATE_DSTBLEND_DST_ALPHA;
	if( !strcmp( token, "gl_one_minus_dst_alpha" ) )
		return GLSTATE_DSTBLEND_ONE_MINUS_DST_ALPHA;
	return GLSTATE_DSTBLEND_ONE;
}

static void Shaderpass_BlendFunc( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	char *token;

	token = Shader_ParseString( ptr );

	pass->flags &= ~(GLSTATE_SRCBLEND_MASK|GLSTATE_DSTBLEND_MASK);
	if( !strcmp( token, "blend" ) )
		pass->flags |= GLSTATE_SRCBLEND_SRC_ALPHA|GLSTATE_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	else if( !strcmp( token, "filter" ) )
		pass->flags |= GLSTATE_SRCBLEND_DST_COLOR|GLSTATE_DSTBLEND_ZERO;
	else if( !strcmp( token, "add" ) )
		pass->flags |= GLSTATE_SRCBLEND_ONE|GLSTATE_DSTBLEND_ONE;
	else
	{
		pass->flags |= Shaderpass_SrcBlendBits( token );
		pass->flags |= Shaderpass_DstBlendBits( Shader_ParseString( ptr ) );
	}
}

static void Shaderpass_AlphaFunc( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	char *token;

	token = Shader_ParseString( ptr );

	pass->flags &= ~(GLSTATE_ALPHAFUNC);
	if( !strcmp( token, "gt0" ) )
		pass->flags |= GLSTATE_AFUNC_GT0;
	else if( !strcmp( token, "lt128" ) )
		pass->flags |= GLSTATE_AFUNC_LT128;
	else if( !strcmp( token, "ge128" ) )
		pass->flags |= GLSTATE_AFUNC_GE128;
}

static void Shaderpass_DepthFunc( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	char *token;

	token = Shader_ParseString( ptr );

	pass->flags &= ~GLSTATE_DEPTHFUNC_EQ;
	if( !strcmp( token, "equal" ) )
		pass->flags |= GLSTATE_DEPTHFUNC_EQ;
}

static void Shaderpass_DepthWrite( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	pass->flags |= GLSTATE_DEPTHWRITE;
}

static void Shaderpass_TcMod( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	int i;
	tcmod_t *tcmod;
	char *token;

	if( pass->numtcmods == MAX_SHADER_TCMODS )
	{
		Com_Printf( S_COLOR_YELLOW "WARNING: shader %s has too many tcmods\n", shader->name );
		Shader_SkipLine( ptr );
		return;
	}

	tcmod = &pass->tcmods[pass->numtcmods];

	token = Shader_ParseString( ptr );
	if( !strcmp( token, "rotate" ) )
	{
		tcmod->args[0] = -Shader_ParseFloat( ptr ) / 360.0f;
		if( !tcmod->args[0] )
			return;
		tcmod->type = TC_MOD_ROTATE;
	}
	else if( !strcmp( token, "scale" ) )
	{
		Shader_ParseVector( ptr, tcmod->args, 2 );
		tcmod->type = TC_MOD_SCALE;
	}
	else if( !strcmp( token, "scroll" ) )
	{
		Shader_ParseVector( ptr, tcmod->args, 2 );
		tcmod->type = TC_MOD_SCROLL;
	}
	else if( !strcmp( token, "stretch" ) )
	{
		shaderfunc_t func;

		Shader_ParseFunc( ptr, &func );

		tcmod->args[0] = func.type;
		for( i = 1; i < 5; i++ )
			tcmod->args[i] = func.args[i-1];
		tcmod->type = TC_MOD_STRETCH;
	}
	else if( !strcmp( token, "transform" ) )
	{
		Shader_ParseVector( ptr, tcmod->args, 6 );
		tcmod->args[4] = tcmod->args[4] - floor( tcmod->args[4] );
		tcmod->args[5] = tcmod->args[5] - floor( tcmod->args[5] );
		tcmod->type = TC_MOD_TRANSFORM;
	}
	else if( !strcmp( token, "turb" ) )
	{
		Shader_ParseVector( ptr, tcmod->args, 4 );
		tcmod->type = TC_MOD_TURB;

		// if GLSL is enabled and we don't have a program for this pass and it's not a cubemap
		// use turbulence GLSL program
		if( glConfig.ext.GLSL && !pass->program
			&& !(pass->anim_frames[0] && pass->anim_frames[0]->flags & IT_CUBEMAP) )
		{
			pass->program = DEFAULT_GLSL_TURBULENCE_PROGRAM;
			pass->program_type = GLSL_PROGRAM_TYPE_TURBULENCE;
		}
	}
	else
	{
		Shader_SkipLine( ptr );
		return;
	}

	r_currentPasses[shader->numpasses].numtcmods++;
}

static void Shaderpass_TcGen( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	char *token;

	token = Shader_ParseString( ptr );
	if( !strcmp( token, "base" ) )
		pass->tcgen = TC_GEN_BASE;
	else if( !strcmp( token, "lightmap" ) )
		pass->tcgen = TC_GEN_LIGHTMAP;
	else if( !strcmp( token, "environment" ) )
		pass->tcgen = TC_GEN_ENVIRONMENT;
	else if( !strcmp( token, "vector" ) )
	{
		pass->tcgen = TC_GEN_VECTOR;
		Shader_ParseVector( ptr, &pass->tcgenVec[0], 4 );
		Shader_ParseVector( ptr, &pass->tcgenVec[4], 4 );
	}
	else if( !strcmp( token, "reflection" ) )
		pass->tcgen = TC_GEN_REFLECTION;
	else if( !strcmp( token, "cellshade" ) )
		pass->tcgen = TC_GEN_REFLECTION_CELLSHADE;
}

static void Shaderpass_Detail( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	pass->flags |= SHADERPASS_DETAIL;
}

static void Shaderpass_Grayscale( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	pass->flags |= SHADERPASS_GRAYSCALE;
}

static void Shaderpass_Skip( shader_t *shader, shaderpass_t *pass, const char **ptr )
{
	Shader_SkipLine( ptr );
}

static const shaderkey_t shaderpasskeys[] =
{
	{ "rgbgen", Shaderpass_RGBGen },
	{ "blendfunc", Shaderpass_BlendFunc },
	{ "depthfunc", Shaderpass_DepthFunc },
	{ "depthwrite",	Shaderpass_DepthWrite },
	{ "alphafunc", Shaderpass_AlphaFunc },
	{ "tcmod", Shaderpass_TcMod },
	{ "map", Shaderpass_Map },
	{ "animmap", Shaderpass_AnimMap },
	{ "cubemap", Shaderpass_CubeMap },
	{ "shadecubemap", Shaderpass_ShadeCubeMap },
	{ "videomap", Shaderpass_VideoMap },
	{ "clampmap", Shaderpass_ClampMap },
	{ "animclampmap", Shaderpass_AnimClampMap },
	{ "normalmap", Shaderpass_NormalMap },
	{ "material", Shaderpass_Material },
	{ "distortion",	Shaderpass_Distortion },
	{ "cellshade", Shaderpass_Cellshade },
	{ "tcgen", Shaderpass_TcGen },
	{ "alphagen", Shaderpass_AlphaGen },
	{ "detail", Shaderpass_Detail },
	{ "grayscale", Shaderpass_Grayscale },
	{ "skip", Shaderpass_Skip },

	{ NULL,	NULL }
};

// ===============================================================

/*
* R_ShaderList_f
*/
void R_ShaderList_f( void )
{
	int i;
	int numShaders;
	shader_t *shader;

	numShaders = 0;

	Com_Printf( "------------------\n" );
	for( i = 0, shader = r_shaders; i < MAX_SHADERS; i++, shader++ ) {
		if( !shader->name ) {
			continue;
		}

		Com_Printf( " %2i %2i: %s\n", shader->numpasses, shader->sort, shader->name );
		numShaders++;
	}
	Com_Printf( "%i shaders total\n", numShaders );
}

/*
* R_ShaderDump_f
*/
void R_ShaderDump_f( void )
{
	char backup, *start;
	const char *name, *ptr;
	shadercache_t *cache;

	if( (Cmd_Argc() < 2) && !r_debug_surface )
	{
		Com_Printf( "Usage: %s [name]\n", Cmd_Argv(0) );
		return;
	}

	if( Cmd_Argc() < 2 )
		name = r_debug_surface->shader->name;
	else
		name = Cmd_Argv( 1 );

	Shader_GetCache( name, &cache );
	if( !cache )
	{
		Com_Printf( "Could not find shader %s in cache.\n", name );
		return;
	}

	start = cache->buffer + cache->offset;

	// temporarily hack in the zero-char
	ptr = start;
	Shader_SkipBlock( &ptr );
	backup = cache->buffer[ptr - cache->buffer];
	cache->buffer[ptr - cache->buffer] = '\0';

	Com_Printf( "Found in %s:\n\n", cache->filename );
	Com_Printf( S_COLOR_YELLOW "%s%s\n", name, start );

	cache->buffer[ptr - cache->buffer] = backup;
}

static void Shader_MakeCache( const char *filename )
{
	int size;
	unsigned int key;
	char *pathName = NULL;
	size_t pathNameSize;
	char *buf, *temp = NULL;
	const char *token, *ptr;
	shadercache_t *cache;
	qbyte *cacheMemBuf;
	size_t cacheMemSize;

	pathNameSize = strlen( "scripts/" ) + strlen( filename ) + 1;
	pathName = Shader_Malloc( pathNameSize );
	assert( pathName );
	Q_snprintfz( pathName, pathNameSize, "scripts/%s", filename );

	Com_Printf( "...loading '%s'\n", pathName );

	size = FS_LoadFile( pathName, ( void ** )&temp, NULL, 0 );
	if( !temp || size <= 0 )
		goto done;

	size = COM_Compress( temp );
	if( !size )
		goto done;

	buf = Shader_Malloc( size+1 );
	strcpy( buf, temp );
	FS_FreeFile( temp );
	temp = NULL;

	// calculate buffer size to allocate our cache objects all at once (we may leak
	// insignificantly here because of duplicate entries)
	for( ptr = buf, cacheMemSize = 0; ptr; )
	{
		token = COM_ParseExt( &ptr, qtrue );
		if( !token[0] )
			break;

		cacheMemSize += sizeof( shadercache_t ) + strlen( token ) + 1;
		Shader_SkipBlock( &ptr );
	}

	if( !cacheMemSize )
	{
		Shader_Free( buf );
		goto done;
	}

	cacheMemBuf = Shader_Malloc( cacheMemSize );
	memset( cacheMemBuf, 0, cacheMemSize );
	for( ptr = buf; ptr; )
	{
		token = COM_ParseExt( &ptr, qtrue );
		if( !token[0] )
			break;

		key = Shader_GetCache( token, &cache );
		if( cache )
			goto set_path_and_offset;

		cache = ( shadercache_t * )cacheMemBuf; cacheMemBuf += sizeof( shadercache_t ) + strlen( token ) + 1;
		cache->hash_next = shadercache_hash[key];
		cache->name = ( char * )( (qbyte *)cache + sizeof( shadercache_t ) );
		strcpy( cache->name, token );
		shadercache_hash[key] = cache;

set_path_and_offset:
		cache->filename = filename;
		cache->buffer = buf;
		cache->offset = ptr - buf;

		Shader_SkipBlock( &ptr );
	}

done:
	if( temp )
		FS_FreeFile( temp );
	if( pathName )
		Shader_Free( pathName );
}

/*
* Shader_GetCache
*/
static unsigned int Shader_GetCache( const char *name, shadercache_t **cache )
{
	unsigned int key;
	shadercache_t *c;

	*cache = NULL;

	key = Com_HashKey( name, SHADERCACHE_HASH_SIZE );
	for( c = shadercache_hash[key]; c; c = c->hash_next )
	{
		if( !Q_stricmp( c->name, name ) )
		{
			*cache = c;
			return key;
		}
	}

	return key;
}

/*
* R_InitShadersCache
*/
static qboolean R_InitShadersCache( void )
{
	int i, numfiles;
	const char *fileptr;
	size_t filelen, shaderbuflen;

	numfiles = FS_GetFileListExt( "scripts", ".shader", NULL, &shaderbuflen, 0, 0 );
	if( !numfiles ) {
		return qfalse;
	}

	shaderPaths = Shader_Malloc( shaderbuflen );
	FS_GetFileList( "scripts", ".shader", shaderPaths, shaderbuflen, 0, 0 );

	// now load all the scripts
	fileptr = shaderPaths;
	memset( shadercache_hash, 0, sizeof( shadercache_t * )*SHADERCACHE_HASH_SIZE );

	for( i = 0; i < numfiles; i++, fileptr += filelen + 1 ) {
		filelen = strlen( fileptr );
		Shader_MakeCache( fileptr );
	}

	return qtrue;
}

/*
* R_InitShaders
*/
void R_InitShaders( void )
{
	int i;

	Com_Printf( "Initializing Shaders:\n" );

	r_shadersmempool = Mem_AllocPool( NULL, "Shaders" );
	r_shaderTemplateBuf = NULL;

	if( !R_InitShadersCache() ) {
		Mem_FreePool( &r_shadersmempool );
		Com_Error( ERR_DROP, "Could not find any shaders!" );
	}

	memset( r_shaders, 0, sizeof( r_shaders ) );

	// link shaders
	r_free_shaders = r_shaders;
	for( i = 0; i < SHADERS_HASH_SIZE; i++ ) {
		r_shaders_hash_headnode[i].prev = &r_shaders_hash_headnode[i];
		r_shaders_hash_headnode[i].next = &r_shaders_hash_headnode[i];
	}
	for( i = 0; i < MAX_SHADERS - 1; i++ ) {
		r_shaders[i].next = &r_shaders[i+1];
	}

	Com_Printf( "--------------------------------------\n\n" );
}

/*
* R_FreeShader
*/
static void R_FreeShader( shader_t *shader )
{
	int i;
	int shaderNum;
	shaderpass_t *pass;

	shaderNum = shader - r_shaders;
	if( ( shader->flags & SHADER_SKY ) && shader->skydome ) {
		R_FreeSkydome( shader->skydome );
		shader->skydome = NULL;
	}

	if( shader->flags & SHADER_VIDEOMAP ) {
		for( i = 0, pass = shader->passes; i < shader->numpasses; i++, pass++ )
			Shader_FreePassCinematics( pass );
	}

	Shader_Free( shader->name );
	shader->name = NULL;
	shader->flags = 0;
	shader->numpasses = 0;
	shader->registration_sequence = 0;
}

/*
* R_UnlinkShader
*/
static void R_UnlinkShader( shader_t *shader )
{
	// remove from linked active list
	shader->prev->next = shader->next;
	shader->next->prev = shader->prev;

	// insert into linked free list
	shader->next = r_free_shaders;
	r_free_shaders = shader;
}

/*
* R_TouchShader
*/
void R_TouchShader( shader_t *s )
{
	int i, j;

	if( s->registration_sequence == r_front.registration_sequence ) {
		return;
	}

	s->registration_sequence = r_front.registration_sequence;

	// touch all images this shader references
	for( i = 0; i < s->numpasses; i++ ) {
		shaderpass_t *pass = s->passes + i;

		for( j = 0; j < MAX_SHADER_ANIM_FRAMES; j++ ) {
			image_t *image = pass->anim_frames[j];
			if( image ) {
				R_TouchImage( image );
			} else if( !pass->program_type ) {
				// only programs can have gaps in anim_frames
				break;
			}
		}

		if( pass->cin ) {
			R_TouchCinematic( pass->cin );
		}
	}

	if( s->skydome ) {
		// touch sky images for this shader
		R_TouchSkydome( s->skydome );
	}
}

/*
* R_FreeUnusedShaders
*/
void R_FreeUnusedShaders( void )
{
	int i;
	shader_t *s;

	for( i = 0, s = r_shaders; i < MAX_SHADERS; i++, s++ ) {
		if( !s->name ) {
			// free shader
			continue;
		}
		if( s->registration_sequence == r_front.registration_sequence ) {
			// we need this shader
			continue;
		}

		R_FreeShader( s );

		R_UnlinkShader( s );
	}
}

/*
* R_ShutdownShaders
*/
void R_ShutdownShaders( void )
{
	int i;
	shader_t *s;

	if( !r_shadersmempool ) {
		return;
	}

	for( i = 0, s = r_shaders; i < MAX_SHADERS; i++, s++ ) {
		if( !s->name ) {
			// free shader
			continue;
		}
		R_FreeShader( s );
	}

	Mem_FreePool( &r_shadersmempool );

	r_shaderTemplateBuf = NULL;
	shaderPaths = NULL;

	memset( shadercache_hash, 0, sizeof( shadercache_hash ) );
}

static void Shader_SetBlendmode( shaderpass_t *pass )
{
	int blendsrc, blenddst;

	if( pass->flags & SHADERPASS_BLENDMODE )
		return;
	if( !pass->anim_frames[0] && !( pass->flags & ( SHADERPASS_LIGHTMAP|SHADERPASS_DLIGHT ) ) )
		return;

	if( !( pass->flags & ( GLSTATE_SRCBLEND_MASK|GLSTATE_DSTBLEND_MASK ) ) )
	{
		if( ( pass->rgbgen.type == RGB_GEN_IDENTITY ) && ( pass->alphagen.type == ALPHA_GEN_IDENTITY ) )
			pass->flags |= SHADERPASS_BLEND_REPLACE;
		else
			pass->flags |= SHADERPASS_BLEND_MODULATE;
		return;
	}

	blendsrc = pass->flags & GLSTATE_SRCBLEND_MASK;
	blenddst = pass->flags & GLSTATE_DSTBLEND_MASK;

	if( blendsrc == GLSTATE_SRCBLEND_ONE && blenddst == GLSTATE_DSTBLEND_ZERO )
		pass->flags |= SHADERPASS_BLEND_MODULATE;
	else if( ( blendsrc == GLSTATE_SRCBLEND_ZERO && blenddst == GLSTATE_DSTBLEND_SRC_COLOR ) || ( blendsrc == GLSTATE_SRCBLEND_DST_COLOR && blenddst == GLSTATE_DSTBLEND_ZERO ) )
		pass->flags |= SHADERPASS_BLEND_MODULATE;
	else if( blendsrc == GLSTATE_SRCBLEND_ONE && blenddst == GLSTATE_DSTBLEND_ONE )
		pass->flags |= SHADERPASS_BLEND_ADD;
	else if( blendsrc == GLSTATE_SRCBLEND_SRC_ALPHA && blenddst == GLSTATE_DSTBLEND_ONE_MINUS_SRC_ALPHA )
		pass->flags |= SHADERPASS_BLEND_DECAL;
}

static void Shader_Readpass( shader_t *shader, const char **ptr )
{
	int n = shader->numpasses;
	int blendmask;
	const char *token;
	shaderpass_t *pass;
	qboolean hasDlights = r_shaderHasDlightPass;
	qboolean singleColorRGB, singleColorAlpha;

	if( n == MAX_SHADER_PASSES )
	{
		Com_Printf( S_COLOR_YELLOW "WARNING: shader %s has too many passes\n", shader->name );

		while( ptr )
		{	// skip
			token = COM_ParseExt( ptr, qtrue );
			if( !token[0] || token[0] == '}' )
				break;
		}
		return;
	}

	// Set defaults
	pass = &r_currentPasses[n];
	memset( pass, 0, sizeof( shaderpass_t ) );
	pass->rgbgen.type = RGB_GEN_UNKNOWN;
	pass->rgbgen.args = r_currentRGBgenArgs[n];
	pass->rgbgen.func = &r_currentRGBgenFuncs[n];
	pass->alphagen.type = ALPHA_GEN_UNKNOWN;
	pass->alphagen.args = r_currentAlphagenArgs[n];
	pass->alphagen.func = &r_currentAlphagenFuncs[n];
	pass->tcgenVec = r_currentTcGen[n][0];
	pass->tcgen = TC_GEN_BASE;
	pass->tcmods = r_currentTcmods[n];

	while( ptr )
	{
		token = COM_ParseExt( ptr, qtrue );

		if( !token[0] )
			break;
		else if( token[0] == '}' )
			break;
		else if( Shader_Parsetok( shader, pass, shaderpasskeys, token, ptr ) )
			break;
	}

	blendmask = (pass->flags & (GLSTATE_SRCBLEND_MASK|GLSTATE_DSTBLEND_MASK));

	if( (pass->flags & (SHADERPASS_LIGHTMAP)) && r_lighting_vertexlight->integer )
		return;

	// keep track of detail passes. if all passes are detail, the whole shader is also detail
	r_shaderAllDetail &= (pass->flags & SHADERPASS_DETAIL);

	// ignore additive dlights if they are disabled in the renderer
	if( pass->flags & SHADERPASS_DLIGHT )
	{
		if( blendmask == (GLSTATE_SRCBLEND_ONE|GLSTATE_DSTBLEND_ONE) )
		{
			shader->flags &= ~SHADER_NO_MODULATIVE_DLIGHTS;
			r_shaderHasDlightPass = hasDlights;
			return;
		}
	}

	if( pass->rgbgen.type == RGB_GEN_UNKNOWN )
	{
		if( !(pass->flags & SHADERPASS_LIGHTMAP)
			&& !(pass->program_type == GLSL_PROGRAM_TYPE_MATERIAL || pass->program_type == GLSL_PROGRAM_TYPE_DISTORTION)
			&& ( !blendmask || ( blendmask & GLSTATE_SRCBLEND_MASK ) == GLSTATE_SRCBLEND_ONE
				|| ( blendmask & GLSTATE_SRCBLEND_MASK ) == GLSTATE_SRCBLEND_SRC_ALPHA )  )
			pass->rgbgen.type = _RGB_GEN_IDENTITY_LIGHTING;
		else
			pass->rgbgen.type = RGB_GEN_IDENTITY;
	}

	if( pass->alphagen.type == ALPHA_GEN_UNKNOWN )
	{
		if( pass->rgbgen.type == RGB_GEN_VERTEX || pass->rgbgen.type == RGB_GEN_EXACT_VERTEX )
			pass->alphagen.type = ALPHA_GEN_VERTEX;
		else
			pass->alphagen.type = ALPHA_GEN_IDENTITY;
	}

	if( blendmask == (GLSTATE_SRCBLEND_ONE|GLSTATE_DSTBLEND_ZERO) )
	{
		pass->flags &= ~blendmask;
		pass->flags |= SHADERPASS_BLEND_MODULATE;
	}

	switch( pass->rgbgen.type )
	{
	case RGB_GEN_IDENTITY_LIGHTING:
	case RGB_GEN_IDENTITY:
	case RGB_GEN_CONST:
	case RGB_GEN_WAVE:
	case RGB_GEN_ENTITYWAVE:
	case RGB_GEN_ONE_MINUS_ENTITY:
	case RGB_GEN_LIGHTING_DIFFUSE_ONLY:
	case RGB_GEN_LIGHTING_AMBIENT_ONLY:
	case RGB_GEN_CUSTOMWAVE:
#ifdef HARDWARE_OUTLINES
	case RGB_GEN_OUTLINE:
#endif
		singleColorRGB = qtrue;
		break;
	default:
		singleColorRGB = qfalse;
		break;
	}

	switch( pass->alphagen.type )
	{
	case ALPHA_GEN_IDENTITY:
	case ALPHA_GEN_CONST:
	case ALPHA_GEN_WAVE:
	case ALPHA_GEN_ENTITY:
#ifdef HARDWARE_OUTLINES
	case ALPHA_GEN_OUTLINE:
#endif
		singleColorAlpha = qtrue;
		break;
	default:
		singleColorAlpha = qfalse;
		break;
	}

	if( singleColorRGB && singleColorAlpha )
		pass->flags |= SHADERPASS_NOCOLORARRAY;
	else if( !glConfig.ext.GLSL || ! ( ( (
				(singleColorRGB || pass->rgbgen.type == RGB_GEN_VERTEX || pass->rgbgen.type == RGB_GEN_EXACT_VERTEX || pass->rgbgen.type == RGB_GEN_ONE_MINUS_VERTEX)
					&& (singleColorAlpha || pass->alphagen.type == ALPHA_GEN_VERTEX || pass->alphagen.type == ALPHA_GEN_ONE_MINUS_VERTEX) ) )
				|| (pass->program_type == GLSL_PROGRAM_TYPE_MATERIAL && pass->rgbgen.type == RGB_GEN_LIGHTING_DIFFUSE) ) )
		r_shaderDisableVBO = qtrue;

	shader->numpasses++;
}

static qboolean Shader_Parsetok( shader_t *shader, shaderpass_t *pass, const shaderkey_t *keys, const char *token, const char **ptr )
{
	const shaderkey_t *key;

	for( key = keys; key->keyword != NULL; key++ )
	{
		if( !Q_stricmp( token, key->keyword ) )
		{
			if( key->func )
				key->func( shader, pass, ptr );
			if( *ptr && **ptr == '}' )
			{
				*ptr = *ptr + 1;
				return qtrue;
			}
			return qfalse;
		}
	}

	Shader_SkipLine( ptr );

	return qfalse;
}

void Shader_SetFeatures( shader_t *s )
{
	int i;
	shaderpass_t *pass;

	if( s->numdeforms )
	{
		if( !glConfig.ext.GLSL )
			r_shaderDisableVBO = qtrue;
		s->features |= MF_DEFORMVS;
	}
	if( s->flags & SHADER_AUTOSPRITE )
		s->features |= MF_NOCULL;
	if( s->flags & (SHADER_PORTAL_CAPTURE|SHADER_PORTAL_CAPTURE2) )
		s->features |= MF_NONBATCHED;
	if( r_shaderHasDistanceRamp )
		s->features |= MF_NONBATCHED;
	if( r_shaderPolygonOffset )
		s->features |= MF_POLYGONOFFSET;

	for( i = 0; i < s->numdeforms; i++ )
	{
		switch( s->deforms[i].type )
		{
		case DEFORMV_BULGE:
			s->features |= MF_STCOORDS;
		case DEFORMV_WAVE:
			s->features |= MF_NORMALS;
			break;
		case DEFORMV_MOVE:
			break;
		case DEFORMV_NORMAL:
			s->features |= MF_NORMALS;
		default:
			r_shaderDisableVBO = qtrue;
			break;
		}
	}

	if( !r_shaderDisableVBO )
		s->features |= MF_HARDWARE;

	for( i = 0, pass = s->passes; i < s->numpasses; i++, pass++ )
	{
		if( pass->program ) {
			if ( pass->program_type == GLSL_PROGRAM_TYPE_MATERIAL )
				s->features |= MF_NORMALS|MF_SVECTORS|MF_LMCOORDS|MF_ENABLENORMALS;
			else if( pass->program_type == GLSL_PROGRAM_TYPE_DISTORTION )
				s->features |= MF_NORMALS|MF_SVECTORS|MF_ENABLENORMALS;
			else if( pass->program_type == GLSL_PROGRAM_TYPE_CELLSHADE )
				s->features |= MF_NORMALS|MF_ENABLENORMALS;

			if( s->numdeforms && ( s->features & MF_HARDWARE ) )
				s->features |= MF_NORMALS|MF_ENABLENORMALS;
		}

		switch( pass->rgbgen.type )
		{
		case RGB_GEN_LIGHTING_DIFFUSE:
			if( pass->program && ( pass->program_type == GLSL_PROGRAM_TYPE_MATERIAL ) )
				s->features = (s->features | MF_COLORS) & ~MF_LMCOORDS;
			s->features |= MF_NORMALS;
			break;
		case RGB_GEN_VERTEX:
		case RGB_GEN_ONE_MINUS_VERTEX:
		case RGB_GEN_EXACT_VERTEX:
			s->features |= MF_COLORS;
			break;
		}

		switch( pass->alphagen.type )
		{
		case ALPHA_GEN_SPECULAR:
		case ALPHA_GEN_DOT:
		case ALPHA_GEN_ONE_MINUS_DOT:
			s->features |= MF_NORMALS;
			break;
		case ALPHA_GEN_VERTEX:
		case ALPHA_GEN_ONE_MINUS_VERTEX:
			s->features |= MF_COLORS;
			break;
		}

		switch( pass->tcgen )
		{
		case TC_GEN_LIGHTMAP:
			s->features |= MF_LMCOORDS;
			break;
		case TC_GEN_ENVIRONMENT:
			if( s->features & MF_HARDWARE )
				s->features |= MF_ENABLENORMALS;
			s->features |= MF_NORMALS;
			break;
		case TC_GEN_REFLECTION:
		case TC_GEN_REFLECTION_CELLSHADE:
			s->features |= MF_NORMALS|MF_ENABLENORMALS;
			break;
		default:
			s->features |= MF_STCOORDS;
			break;
		}
	}
}

static void Shader_Finish( int defaultType, shader_t *s )
{
	int i;
	int opaque = -1;
	int blendmask;
	const char *oldname = s->name;
	size_t size = strlen( oldname ) + 1;
	shaderpass_t *pass;
	qbyte *buffer;

	if( defaultType >= SHADER_BSP && defaultType <= SHADER_BSP_FLARE ) {
		if( s->sort > SHADER_SORT_ADDITIVE )
			s->sort = SHADER_SORT_ADDITIVE;
	}

	if( !s->numpasses && !s->sort )
	{
		if( s->flags & SHADER_PORTAL )
			s->sort = SHADER_SORT_PORTAL;
		else
			s->sort = SHADER_SORT_ADDITIVE;
	}

	if( s->flags & SHADER_SKY )
		s->sort = SHADER_SORT_SKY;
	if( r_shaderPolygonOffset && !s->sort )
		s->sort = SHADER_SORT_DECAL;

	// fix up rgbgen's and blendmodes for lightmapped shaders and vertex lighting
	if( r_shaderHasLightmapPass )
	{
		for( i = 0, pass = r_currentPasses; i < s->numpasses; i++, pass++ )
		{
			blendmask = pass->flags & ( GLSTATE_SRCBLEND_MASK|GLSTATE_DSTBLEND_MASK );

			if( !blendmask || blendmask == (GLSTATE_SRCBLEND_DST_COLOR|GLSTATE_DSTBLEND_ZERO) || s->numpasses == 1 )
			{
				if( r_lighting_vertexlight->integer )
				{
					if( pass->rgbgen.type == RGB_GEN_IDENTITY || pass->rgbgen.type == RGB_GEN_IDENTITY_LIGHTING ) {
						pass->rgbgen.type = RGB_GEN_VERTEX;
						pass->flags &= ~SHADERPASS_NOCOLORARRAY;
					}
					//if( pass->alphagen.type == ALPHA_GEN_IDENTITY )
					//	pass->alphagen.type = ALPHA_GEN_VERTEX;

					if( !(pass->flags & GLSTATE_ALPHAFUNC) )
						pass->flags &= ~blendmask;
				}
				else
				{
					if( pass->rgbgen.type == RGB_GEN_IDENTITY_LIGHTING )
						pass->rgbgen.type = RGB_GEN_IDENTITY;
				}
				break;
			}
		}
	}

	size += s->numdeforms * sizeof( deformv_t ) + s->numpasses * sizeof( shaderpass_t );
	for( i = 0, pass = r_currentPasses; i < s->numpasses; i++, pass++ )
	{
		// rgbgen args
		if( pass->rgbgen.type == RGB_GEN_WAVE ||
			pass->rgbgen.type == RGB_GEN_CONST )
			size += sizeof( float ) * 3;
		else if( pass->rgbgen.type == RGB_GEN_CUSTOMWAVE )
			size += sizeof( float ) * 1;

		// alphagen args
		if( pass->alphagen.type == ALPHA_GEN_PORTAL ||
			pass->alphagen.type == ALPHA_GEN_SPECULAR ||
			pass->alphagen.type == ALPHA_GEN_CONST ||
			pass->alphagen.type == ALPHA_GEN_DOT || pass->alphagen.type == ALPHA_GEN_ONE_MINUS_DOT )
			size += sizeof( float ) * 2;

		if( pass->rgbgen.type == RGB_GEN_WAVE ||
			( (pass->rgbgen.type == RGB_GEN_ENTITYWAVE || pass->rgbgen.type == RGB_GEN_CUSTOMWAVE) && r_currentPasses[i].rgbgen.func->type != SHADER_FUNC_NONE) )
			size += sizeof( shaderfunc_t );
		if( pass->alphagen.type == ALPHA_GEN_WAVE )
			size += sizeof( shaderfunc_t );
		size += pass->numtcmods * sizeof( tcmod_t );
		if( pass->tcgen == TC_GEN_VECTOR )
			size += sizeof( vec4_t ) * 2;
	}

	buffer = Shader_Malloc( size );

	s->name = ( char * )buffer; buffer += strlen( oldname ) + 1;
	s->passes = ( shaderpass_t * )buffer; buffer += s->numpasses * sizeof( shaderpass_t );

	strcpy( s->name, oldname );
	memcpy( s->passes, r_currentPasses, s->numpasses * sizeof( shaderpass_t ) );

	for( i = 0, pass = s->passes; i < s->numpasses; i++, pass++ )
	{
		if( pass->rgbgen.type == RGB_GEN_WAVE ||
			pass->rgbgen.type == RGB_GEN_CONST )
		{
			pass->rgbgen.args = ( float * )buffer; buffer += sizeof( float ) * 3;
			memcpy( pass->rgbgen.args, r_currentPasses[i].rgbgen.args, sizeof( float ) * 3 );
		}
		else if( pass->rgbgen.type == RGB_GEN_CUSTOMWAVE )
		{
			pass->rgbgen.args = ( float * )buffer; buffer += sizeof( float ) * 1;
			memcpy( pass->rgbgen.args, r_currentPasses[i].rgbgen.args, sizeof( float ) * 1 );
		}

		if( pass->alphagen.type == ALPHA_GEN_PORTAL ||
			pass->alphagen.type == ALPHA_GEN_SPECULAR ||
			pass->alphagen.type == ALPHA_GEN_CONST ||
			pass->alphagen.type == ALPHA_GEN_DOT || pass->alphagen.type == ALPHA_GEN_ONE_MINUS_DOT )
		{
			pass->alphagen.args = ( float * )buffer; buffer += sizeof( float ) * 2;
			memcpy( pass->alphagen.args, r_currentPasses[i].alphagen.args, sizeof( float ) * 2 );
		}

		if( pass->rgbgen.type == RGB_GEN_WAVE ||
			( (pass->rgbgen.type == RGB_GEN_ENTITYWAVE || pass->rgbgen.type == RGB_GEN_CUSTOMWAVE) && r_currentPasses[i].rgbgen.func->type != SHADER_FUNC_NONE) )
		{
			pass->rgbgen.func = ( shaderfunc_t * )buffer; buffer += sizeof( shaderfunc_t );
			memcpy( pass->rgbgen.func, r_currentPasses[i].rgbgen.func, sizeof( shaderfunc_t ) );
		}
		else
		{
			pass->rgbgen.func = NULL;
		}

		if( pass->alphagen.type == ALPHA_GEN_WAVE )
		{
			pass->alphagen.func = ( shaderfunc_t * )buffer; buffer += sizeof( shaderfunc_t );
			memcpy( pass->alphagen.func, r_currentPasses[i].alphagen.func, sizeof( shaderfunc_t ) );
		}
		else
		{
			pass->alphagen.func = NULL;
		}

		if( pass->numtcmods )
		{
			pass->tcmods = ( tcmod_t * )buffer; buffer += r_currentPasses[i].numtcmods * sizeof( tcmod_t );
			pass->numtcmods = r_currentPasses[i].numtcmods;
			memcpy( pass->tcmods, r_currentPasses[i].tcmods, r_currentPasses[i].numtcmods * sizeof( tcmod_t ) );
		}

		if( pass->tcgen == TC_GEN_VECTOR )
		{
			pass->tcgenVec = ( vec_t * )buffer; buffer += sizeof( vec4_t ) * 2;
			Vector4Copy( &r_currentPasses[i].tcgenVec[0], &pass->tcgenVec[0] );
			Vector4Copy( &r_currentPasses[i].tcgenVec[4], &pass->tcgenVec[4] );
		}
	}

	if( s->numdeforms )
	{
		s->deforms = ( deformv_t * )buffer;
		memcpy( s->deforms, r_currentDeforms, s->numdeforms * sizeof( deformv_t ) );
	}

	if( s->flags & SHADER_AUTOSPRITE )
		s->flags &= ~( SHADER_CULL_FRONT|SHADER_CULL_BACK );
	if( r_shaderHasDlightPass )
		s->flags |= SHADER_NO_MODULATIVE_DLIGHTS;
	if( s->numpasses && r_shaderAllDetail )
		s->flags |= SHADER_ALLDETAIL;

	for( i = 0, pass = s->passes; i < s->numpasses; i++, pass++ )
	{
		blendmask = pass->flags & ( GLSTATE_SRCBLEND_MASK|GLSTATE_DSTBLEND_MASK );

		if( opaque == -1 && !blendmask )
			opaque = i;

		if( !blendmask )
			pass->flags |= GLSTATE_DEPTHWRITE;
		if( pass->cin )
			s->flags |= SHADER_VIDEOMAP;
		if( pass->flags & SHADERPASS_LIGHTMAP )
			s->flags |= SHADER_LIGHTMAP;
		if( pass->flags & GLSTATE_DEPTHWRITE )
		{
			if( s->flags & SHADER_SKY )
				pass->flags &= ~GLSTATE_DEPTHWRITE;
			else
				s->flags |= SHADER_DEPTHWRITE;
		}

		if( pass->program )
		{
			if( pass->program_type == GLSL_PROGRAM_TYPE_MATERIAL )
				s->flags |= SHADER_MATERIAL;
		}

		// disable r_drawflat for shaders with customizable color passes
		if( pass->rgbgen.type == RGB_GEN_CONST || pass->rgbgen.type == RGB_GEN_CUSTOMWAVE ||
			pass->rgbgen.type == RGB_GEN_ENTITYWAVE || pass->rgbgen.type == RGB_GEN_ONE_MINUS_ENTITY ) {
			s->flags |= SHADER_NODRAWFLAT;
		}

		Shader_SetBlendmode( pass );
	}

	// all passes have blendfuncs
	if( opaque == -1 )
	{
		if( !s->sort )
		{
			if( s->flags & SHADER_DEPTHWRITE )
				s->sort = SHADER_SORT_ALPHATEST;
			else
				s->sort = SHADER_SORT_ADDITIVE;
		}
	}
	else
	{
		pass = s->passes + opaque;

		// fix up rgbgen to prevent double scaling of color info
		if( s->flags & SHADER_LIGHTMAP )
		{
			if( pass->rgbgen.type == RGB_GEN_IDENTITY_LIGHTING )
				pass->rgbgen.type = RGB_GEN_IDENTITY;
		}

		if( !s->sort )
		{
			if( pass->flags & GLSTATE_ALPHAFUNC )
				s->sort = SHADER_SORT_ALPHATEST;
		}
	}

	if( !s->sort )
		s->sort = SHADER_SORT_OPAQUE;

	// disable r_drawflat for transparent shaders
	if( s->sort != SHADER_SORT_OPAQUE ) {
		s->flags |= SHADER_NODRAWFLAT;
	}

	Shader_SetFeatures( s );
}

/*
* R_UploadCinematicShader
*/
void R_UploadCinematicShader( const shader_t *shader )
{
	int j;
	shaderpass_t *pass;

	// upload cinematics
	for( j = 0, pass = shader->passes; j < shader->numpasses; j++, pass++ )
	{
		if( pass->cin )
			pass->anim_frames[0] = R_UploadCinematic( pass->cin );
	}
}

/*
* R_ShaderVolatileFlagsForGlobalState
*/
unsigned int R_ShaderVolatileFlagsForGlobalState( void )
{
	unsigned int volatileFlags = 0;

	if( mapConfig.deluxeMappingEnabled ) {
		volatileFlags |= SHADER_VOLATILE_MAPCONFIG_DELUXEMAPPING;
	}

	return volatileFlags;
}

/*
* R_ShaderShortName
*/
static size_t R_ShaderShortName( const char *name, char *shortname, size_t shortname_size )
{
	int i;
	size_t length = 0;
	size_t lastDot = 0;

	for( i = ( name[0] == '/' || name[0] == '\\' ), length = 0; name[i] && ( length < shortname_size-1 ); i++ )
	{
		if( name[i] == '.' )
			lastDot = length;
		if( name[i] == '\\' )
			shortname[length++] = '/';
		else
			shortname[length++] = tolower( name[i] );
	}

	if( !length )
		return 0;
	if( lastDot )
		length = lastDot;
	shortname[length] = 0;

	return length;
}

/*
* R_LoadShaderReal
*/
static void R_LoadShaderReal( shader_t *s, char *shortname, size_t shortname_length, int type, 
							   qboolean forceDefault, int addFlags, const char *text )
{
	shadercache_t *cache;
	shaderpass_t *pass;
	image_t *materialImages[MAX_SHADER_ANIM_FRAMES];

	s->name = shortname;
	s->offsetmapping_scale = 1;

	r_shaderNoMipMaps =	qfalse;
	r_shaderNoPicMip = qfalse;
	r_shaderNoCompress = qfalse;
	r_shaderNoFiltering = qfalse;
	r_shaderHasDlightPass = qfalse;
	r_shaderHasDistanceRamp = qfalse;
	r_shaderPolygonOffset = qfalse;
	r_shaderHasLightmapPass = qfalse;
	r_shaderDisableVBO = qfalse;
	r_shaderAllDetail = SHADERPASS_DETAIL;
	if( !r_defaultImage )
		r_defaultImage = r_notexture;

	cache = NULL;
	if( !forceDefault )
		Shader_GetCache( shortname, &cache );

	if( cache ) {
		// shader is in the shader scripts
		text = cache->buffer + cache->offset;
		Com_DPrintf( "Loading shader %s from cache...\n", shortname );
	}

	if( text ) {
		const char *ptr, *token;

		// set defaults
		s->type = SHADER_UNKNOWN;
		s->flags = SHADER_CULL_FRONT;
		s->features = MF_NONE;

		ptr = text;
		token = COM_ParseExt( &ptr, qtrue );

		if( !ptr || token[0] != '{' ) {
			goto create_default;
		}

		while( ptr ) {
			token = COM_ParseExt( &ptr, qtrue );

			if( !token[0] )
				break;
			else if( token[0] == '}' )
				break;
			else if( token[0] == '{' )
				Shader_Readpass( s, &ptr );
			else if( Shader_Parsetok( s, NULL, shaderkeys, token, &ptr ) )
				break;
		}

		Shader_Finish( type, s );
	}
	else
	{
		// make default shader
		// FIXME: replace this mess with templates?
		switch( type )
		{
		case SHADER_BSP_VERTEX:
			if( 0 && mapConfig.deluxeMappingEnabled
				&& Shaderpass_LoadMaterial( &materialImages[0], &materialImages[1], &materialImages[2], shortname, addFlags, 1 ) )
			{
				s->type = SHADER_BSP_VERTEX;
				s->flags = SHADER_DEPTHWRITE|SHADER_CULL_FRONT|SHADER_MATERIAL|SHADER_VOLATILE;
				s->volatileFlags = SHADER_VOLATILE_MAPCONFIG_DELUXEMAPPING;
				s->features = MF_STCOORDS|MF_NORMALS|MF_SVECTORS|MF_ENABLENORMALS|MF_COLORS|MF_HARDWARE;
				s->sort = SHADER_SORT_OPAQUE;
				s->numpasses = 1;
				s->name = Shader_Malloc( shortname_length + 1 + sizeof( shaderpass_t ) * s->numpasses );
				strcpy( s->name, shortname );
				s->passes = ( shaderpass_t * )( ( qbyte * )s->name + shortname_length + 1 );

				pass = &s->passes[0];
				pass->flags = SHADERPASS_DELUXEMAP|GLSTATE_DEPTHWRITE|SHADERPASS_NOCOLORARRAY|SHADERPASS_BLEND_REPLACE;
				pass->tcgen = TC_GEN_BASE;
				pass->rgbgen.type = RGB_GEN_LIGHTING_DIFFUSE;
				pass->alphagen.type = ALPHA_GEN_IDENTITY;
				pass->program = DEFAULT_GLSL_PROGRAM;
				pass->program_type = GLSL_PROGRAM_TYPE_MATERIAL;
				pass->anim_frames[0] = Shader_FindImage( s, shortname, addFlags, 0 );
				pass->anim_frames[1] = materialImages[0]; // normalmap
				pass->anim_frames[2] = materialImages[1]; // glossmap
				pass->anim_frames[3] = materialImages[2]; // decalmap
			}
			else
			{
				s->type = SHADER_BSP_VERTEX;
				s->flags = SHADER_DEPTHWRITE|SHADER_CULL_FRONT;
				s->features = MF_STCOORDS|MF_COLORS|MF_HARDWARE;
				s->sort = SHADER_SORT_OPAQUE;
				s->numpasses = 1;
				s->name = Shader_Malloc( shortname_length + 1 + sizeof( shaderpass_t ) * s->numpasses );
				strcpy( s->name, shortname );
				s->passes = ( shaderpass_t * )( ( qbyte * )s->name + shortname_length + 1 );

				s->numpasses = 0;
				pass = &s->passes[s->numpasses++];
				pass->flags = GLSTATE_DEPTHWRITE|SHADERPASS_BLEND_MODULATE /*|GLSTATE_SRCBLEND_ONE|GLSTATE_DSTBLEND_ZERO*/;
				pass->tcgen = TC_GEN_BASE;
				pass->rgbgen.type = RGB_GEN_VERTEX;
				pass->alphagen.type = ALPHA_GEN_IDENTITY;
				pass->anim_frames[0] = Shader_FindImage( s, shortname, addFlags, 0 );
			}
			break;
		case SHADER_BSP_FLARE:
			s->type = SHADER_BSP_FLARE;
			s->features = MF_STCOORDS|MF_COLORS;
			s->sort = SHADER_SORT_ADDITIVE;
			s->numpasses = 1;
			s->name = Shader_Malloc( shortname_length + 1 + sizeof( shaderpass_t ) * s->numpasses );
			strcpy( s->name, shortname );
			s->passes = ( shaderpass_t * )( ( qbyte * )s->name + shortname_length + 1 );

			pass = &s->passes[0];
			pass->flags = SHADERPASS_BLEND_ADD|GLSTATE_SRCBLEND_ONE|GLSTATE_DSTBLEND_ONE;
			pass->rgbgen.type = RGB_GEN_VERTEX;
			pass->alphagen.type = ALPHA_GEN_IDENTITY;
			pass->tcgen = TC_GEN_BASE;
			pass->anim_frames[0] = Shader_FindImage( s, shortname, addFlags, 0 );
			break;
		case SHADER_MD3:
			s->type = SHADER_MD3;
			s->flags = SHADER_DEPTHWRITE|SHADER_CULL_FRONT;
			s->features = MF_STCOORDS|MF_NORMALS;
			s->sort = SHADER_SORT_OPAQUE;
			s->numpasses = 1;
			s->name = Shader_Malloc( shortname_length + 1 + sizeof( shaderpass_t ) * s->numpasses );
			strcpy( s->name, shortname );
			s->passes = ( shaderpass_t * )( ( qbyte * )s->name + shortname_length + 1 );

			pass = &s->passes[0];
			pass->flags = GLSTATE_DEPTHWRITE|SHADERPASS_BLEND_MODULATE;
			pass->rgbgen.type = RGB_GEN_LIGHTING_DIFFUSE;
			pass->alphagen.type = ALPHA_GEN_IDENTITY;
			pass->tcgen = TC_GEN_BASE;
			pass->anim_frames[0] = Shader_FindImage( s, shortname, addFlags, 0 );

			// load default GLSL program if bumpmap was found
			if( mapConfig.deluxeMappingEnabled
				&& Shaderpass_LoadMaterial( &materialImages[0], &materialImages[1], &materialImages[2], shortname, addFlags, 1 ) )
			{
				pass->rgbgen.type = RGB_GEN_IDENTITY;
				pass->program = DEFAULT_GLSL_PROGRAM;
				pass->program_type = GLSL_PROGRAM_TYPE_MATERIAL;
				pass->anim_frames[1] = materialImages[0]; // normalmap
				pass->anim_frames[2] = materialImages[1]; // glossmap
				pass->anim_frames[3] = materialImages[2]; // decalmap
				s->features |= MF_SVECTORS|MF_ENABLENORMALS;
				s->flags |= SHADER_MATERIAL|SHADER_VOLATILE;
				s->volatileFlags |= SHADER_VOLATILE_MAPCONFIG_DELUXEMAPPING;
			}
			break;
		case SHADER_2D:
		case SHADER_2D_RAW:
		case SHADER_VIDEO:
			s->type = SHADER_2D;
			s->features = MF_STCOORDS|MF_COLORS;
			s->sort = SHADER_SORT_ADDITIVE;
			s->numpasses = 1;
			s->name = Shader_Malloc( shortname_length + 1 + sizeof( shaderpass_t ) * s->numpasses );
			strcpy( s->name, shortname );
			s->passes = ( shaderpass_t * )( ( qbyte * )s->name + shortname_length + 1 );

			pass = &s->passes[0];
			pass->flags = SHADERPASS_BLEND_MODULATE|GLSTATE_SRCBLEND_SRC_ALPHA|GLSTATE_DSTBLEND_ONE_MINUS_SRC_ALPHA /* | SHADERPASS_NOCOLORARRAY*/;
			pass->rgbgen.type = RGB_GEN_VERTEX;
			pass->alphagen.type = ALPHA_GEN_VERTEX;
			pass->tcgen = TC_GEN_BASE;
			if( type == SHADER_VIDEO )
			{
				s->flags = SHADER_VIDEOMAP;
				// we don't want "video/" to be there since it's hardcoded into cinematics
				if( !Q_strnicmp(shortname, "video/", 6 ) )
					pass->cin = R_StartCinematic( shortname+6 );
				else
					pass->cin = R_StartCinematic( shortname );
			}
			else if( type != SHADER_2D_RAW ) {
				pass->anim_frames[0] = Shader_FindImage( s, shortname, IT_CLAMP|IT_NOPICMIP|IT_NOMIPMAP|addFlags, 0 );
			}
			break;
		case SHADER_FARBOX:
			s->type = SHADER_FARBOX;
			s->features = MF_STCOORDS;
			s->sort = SHADER_SORT_SKY;
			s->flags = SHADER_SKY;
			s->numpasses = 1;
			s->name = Shader_Malloc( shortname_length + 1 + sizeof( shaderpass_t ) * s->numpasses );
			strcpy( s->name, shortname );
			s->passes = ( shaderpass_t * )( ( qbyte * )s->name + shortname_length + 1 );

			pass = &s->passes[0];
			pass->flags = SHADERPASS_NOCOLORARRAY|SHADERPASS_BLEND_MODULATE /*|GLSTATE_SRCBLEND_ONE|GLSTATE_DSTBLEND_ZERO*/;
			pass->rgbgen.type = _RGB_GEN_IDENTITY_LIGHTING;
			pass->alphagen.type = ALPHA_GEN_IDENTITY;
			pass->tcgen = TC_GEN_BASE;
			pass->anim_frames[0] = R_FindImage( shortname, NULL, IT_NOMIPMAP|IT_CLAMP|addFlags, 0 );
			break;
		case SHADER_NEARBOX:
			s->type = SHADER_NEARBOX;
			s->features = MF_STCOORDS;
			s->sort = SHADER_SORT_SKY;
			s->numpasses = 1;
			s->flags = SHADER_SKY;
			s->name = Shader_Malloc( shortname_length + 1 + sizeof( shaderpass_t ) * s->numpasses );
			strcpy( s->name, shortname );
			s->passes = ( shaderpass_t * )( ( qbyte * )s->name + shortname_length + 1 );

			pass = &s->passes[0];
			pass->flags = GLSTATE_ALPHAFUNC|SHADERPASS_NOCOLORARRAY|SHADERPASS_BLEND_DECAL|GLSTATE_SRCBLEND_SRC_ALPHA|GLSTATE_DSTBLEND_ONE_MINUS_SRC_ALPHA;
			pass->rgbgen.type = _RGB_GEN_IDENTITY_LIGHTING;
			pass->alphagen.type = ALPHA_GEN_IDENTITY;
			pass->tcgen = TC_GEN_BASE;
			pass->anim_frames[0] = R_FindImage( shortname, NULL, IT_NOMIPMAP|IT_CLAMP|addFlags, 0 );
			break;
		case SHADER_PLANAR_SHADOW:
			s->type = SHADER_PLANAR_SHADOW;
			s->features = MF_DEFORMVS | (glConfig.ext.GLSL ? MF_HARDWARE : 0);
			s->sort = SHADER_SORT_DECAL;
			s->flags = 0;
			s->numdeforms = 1;
			s->numpasses = 1;
			s->name = Shader_Malloc( shortname_length + 1 + s->numdeforms * sizeof( deformv_t ) + sizeof( shaderpass_t ) * s->numpasses + 3 * sizeof( float ) );
			strcpy( s->name, shortname );
			s->deforms = ( deformv_t * )( ( qbyte * )s->name + shortname_length + 1 );
			s->deforms[0].type = DEFORMV_PROJECTION_SHADOW;
			s->passes = ( shaderpass_t * )( ( qbyte * )s->deforms + s->numdeforms * sizeof( deformv_t ) );

			pass = &s->passes[0];
			pass->flags = SHADERPASS_NOCOLORARRAY|SHADERPASS_STENCILSHADOW|SHADERPASS_BLEND_DECAL|GLSTATE_SRCBLEND_SRC_ALPHA|GLSTATE_DSTBLEND_ONE_MINUS_SRC_ALPHA;
			pass->rgbgen.type = RGB_GEN_CONST;
			pass->rgbgen.args = ( float * )( ( qbyte * )s->passes + sizeof( shaderpass_t ) * s->numpasses );
			VectorClear( pass->rgbgen.args );
			pass->alphagen.type = ALPHA_GEN_PLANAR_SHADOW;
			pass->tcgen = TC_GEN_NONE;
			if( glConfig.ext.GLSL ) {
				pass->program = DEFAULT_GLSL_PLANAR_SHADOW_PROGRAM;
				pass->program_type = GLSL_PROGRAM_TYPE_PLANAR_SHADOW;
			}
			break;
		case SHADER_OPAQUE_OCCLUDER:
			s->type = SHADER_OPAQUE_OCCLUDER;
			s->sort = SHADER_SORT_NONE;
			s->flags = SHADER_CULL_FRONT|SHADER_DEPTHWRITE;
			s->numpasses = 1;
			s->name = Shader_Malloc( shortname_length + 1 + sizeof( shaderpass_t ) * s->numpasses + 3 * sizeof( float ) );
			strcpy( s->name, shortname );
			s->passes = ( shaderpass_t * )( ( qbyte * )s->name + shortname_length + 1 );

			pass = &s->passes[0];
			pass->flags = SHADERPASS_NOCOLORARRAY|GLSTATE_DEPTHWRITE;
			pass->rgbgen.type = RGB_GEN_ENVIRONMENT;
			pass->rgbgen.args = ( float * )( ( qbyte * )s->passes + sizeof( shaderpass_t ) * s->numpasses );
			VectorClear( pass->rgbgen.args );
			pass->alphagen.type = ALPHA_GEN_IDENTITY;
			pass->tcgen = TC_GEN_NONE;
			pass->anim_frames[0] = r_whitetexture;
			break;
#ifdef HARDWARE_OUTLINES
		case SHADER_OUTLINE:
			s->type = SHADER_OUTLINE|SHADER_DEPTHWRITE;
			s->features = MF_NORMALS|MF_DEFORMVS;
			s->sort = SHADER_SORT_OPAQUE;
			s->flags = SHADER_CULL_BACK|SHADER_DEPTHWRITE;
			s->numdeforms = 1;
			s->numpasses = 1;
			s->name = Shader_Malloc( shortname_length + 1 + s->numdeforms * sizeof( deformv_t ) + sizeof( shaderpass_t ) * s->numpasses );
			strcpy( s->name, shortname );
			s->deforms = ( deformv_t * )( ( qbyte * )s->name + shortname_length + 1 );
			s->deforms[0].type = DEFORMV_OUTLINE;
			s->passes = ( shaderpass_t * )( ( qbyte * )s->deforms + s->numdeforms * sizeof( deformv_t ) );

			pass = &s->passes[0];
			pass->flags = SHADERPASS_NOCOLORARRAY|GLSTATE_SRCBLEND_ONE|GLSTATE_DSTBLEND_ZERO|SHADERPASS_BLEND_MODULATE|GLSTATE_DEPTHWRITE;
			pass->rgbgen.type = RGB_GEN_OUTLINE;
			pass->alphagen.type = ALPHA_GEN_OUTLINE;
			pass->tcgen = TC_GEN_NONE;
			pass->anim_frames[0] = r_whitetexture;
			break;
#endif
		case SHADER_BSP:
		default:
create_default:
			if( mapConfig.deluxeMappingEnabled
				&& Shaderpass_LoadMaterial( &materialImages[0], &materialImages[1], &materialImages[2], shortname, addFlags, 1 ) )
			{
				s->type = SHADER_BSP;
				s->flags = SHADER_DEPTHWRITE|SHADER_CULL_FRONT|SHADER_LIGHTMAP|SHADER_MATERIAL|SHADER_VOLATILE;
				s->volatileFlags |= SHADER_VOLATILE_MAPCONFIG_DELUXEMAPPING;
				s->features = MF_STCOORDS|MF_LMCOORDS|MF_NORMALS|MF_SVECTORS|MF_ENABLENORMALS|MF_HARDWARE;
				s->sort = SHADER_SORT_OPAQUE;
				s->numpasses = 1;
				s->name = Shader_Malloc( shortname_length + 1 + sizeof( shaderpass_t ) * s->numpasses );
				strcpy( s->name, shortname );
				s->passes = ( shaderpass_t * )( ( qbyte * )s->name + shortname_length + 1 );

				pass = &s->passes[0];
				pass->flags = SHADERPASS_LIGHTMAP|SHADERPASS_DELUXEMAP|GLSTATE_DEPTHWRITE|SHADERPASS_NOCOLORARRAY|SHADERPASS_BLEND_REPLACE;
				pass->tcgen = TC_GEN_BASE;
				pass->rgbgen.type = RGB_GEN_IDENTITY;
				pass->alphagen.type = ALPHA_GEN_IDENTITY;
				pass->program = DEFAULT_GLSL_PROGRAM;
				pass->program_type = GLSL_PROGRAM_TYPE_MATERIAL;
				pass->anim_frames[0] = Shader_FindImage( s, shortname, addFlags, 0 );
				pass->anim_frames[1] = materialImages[0]; // normalmap
				pass->anim_frames[2] = materialImages[1]; // glossmap
				pass->anim_frames[3] = materialImages[2]; // decalmap
			}
			else
			{
				s->type = SHADER_BSP;
				s->flags = SHADER_DEPTHWRITE|SHADER_CULL_FRONT|SHADER_LIGHTMAP;
				s->features = MF_STCOORDS|MF_LMCOORDS|MF_HARDWARE;
				s->sort = SHADER_SORT_OPAQUE;
				s->numpasses = 2;
				s->name = Shader_Malloc( shortname_length + 1 + sizeof( shaderpass_t ) * s->numpasses );
				strcpy( s->name, shortname );
				s->passes = ( shaderpass_t * )( ( qbyte * )s->name + shortname_length + 1 );
				s->numpasses = 0;

				pass = &s->passes[s->numpasses++];
				pass->flags = SHADERPASS_LIGHTMAP|GLSTATE_DEPTHWRITE|SHADERPASS_NOCOLORARRAY|SHADERPASS_BLEND_REPLACE;
				pass->tcgen = TC_GEN_LIGHTMAP;
				pass->rgbgen.type = RGB_GEN_IDENTITY;
				pass->alphagen.type = ALPHA_GEN_IDENTITY;

				pass = &s->passes[s->numpasses++];
				pass->flags = SHADERPASS_NOCOLORARRAY|SHADERPASS_BLEND_MODULATE|GLSTATE_SRCBLEND_ZERO|GLSTATE_DSTBLEND_SRC_COLOR;
				pass->tcgen = TC_GEN_BASE;
				pass->rgbgen.type = RGB_GEN_IDENTITY;
				pass->alphagen.type = ALPHA_GEN_IDENTITY;
				pass->anim_frames[0] = Shader_FindImage( s, shortname, addFlags, 0 );
			}
			break;
		}
	}

	// calculate sortkey
	s->sortkey = Shader_Sortkey( s, s->sort );
	s->registration_sequence = r_front.registration_sequence;
}

/*
* R_LoadShader
*/
shader_t *R_LoadShader( const char *name, int type, qboolean forceDefault, int addFlags, int ignoreType, const char *text )
{
	unsigned int key, nameLength;
	char shortname[MAX_QPATH];
	shader_t *s, *best;
	shader_t *hnode, *prev, *next;
	unsigned int globalVolatileFlags = R_ShaderVolatileFlagsForGlobalState();

	if( !name || !name[0] )
		return NULL;

	nameLength = R_ShaderShortName( name, shortname, sizeof( shortname ) );
	if( !nameLength )
		return NULL;

	// redefine shader type for vertex lighting
	if( type == SHADER_BSP && r_lighting_vertexlight->integer ) {
		type = SHADER_BSP_VERTEX;
		if( ignoreType == SHADER_BSP_VERTEX ) {
			ignoreType = SHADER_INVALID;
		}
	}

	if( ignoreType == SHADER_UNKNOWN )
		forceDefault = qtrue;

	// test if already loaded
	key = Com_HashKey( shortname, SHADERS_HASH_SIZE );
	hnode = &r_shaders_hash_headnode[key];
	best = NULL;

	// scan all instances of the same shader for exact match of the type
	for( s = hnode->next; s != hnode; s = s->next ) {
		if( !strcmp( s->name, shortname ) && ( s->type != ignoreType ) ) {
			best = s;
			if( s->type == type ) {
				break;
			}
		}
	}

	if( best ) {
		// shader must also be non-volatile or its volatile flags matching the current state
		if ( ( best->flags & SHADER_VOLATILE ) && best->volatileFlags != globalVolatileFlags ) {
			R_FreeShader( best );

			// reload the shader with different set of volatile flags
			R_LoadShaderReal( best, shortname, nameLength, type, forceDefault, addFlags, text );
			return best;
		}
	}

	if( best ) {
		// match found
		R_TouchShader( best );
		return best;
	}

	if( !r_free_shaders ) {
		Com_Error( ERR_FATAL, "R_LoadShader: Shader limit exceeded" );
	}

	s = r_free_shaders;
	r_free_shaders = s->next;

	prev = s->prev;
	next = s->next;
	memset( s, 0, sizeof( shader_t ) );
	s->next = next;
	s->prev = prev;

	R_LoadShaderReal( s, shortname, nameLength, type, forceDefault, addFlags, text );

	// add to linked lists
	s->prev = hnode;
	s->next = hnode->next;
	s->next->prev = s;
	s->prev->next = s;

	return s;
}

/*
* R_RegisterPic
*/
shader_t *R_RegisterPic( const char *name )
{
	return R_LoadShader( name, SHADER_2D, qfalse, 0, SHADER_INVALID, NULL );
}

/*
* R_RegisterRawPic
*
* Registers default 2D shader with base image provided as RGBA data.
*/
shader_t *R_RegisterRawPic( const char *name, int width, int height, qbyte *data )
{
	shader_t *s;

	s = R_LoadShader( name, SHADER_2D_RAW, qtrue, 0, SHADER_INVALID, NULL );
	if( s ) {
		image_t *image;

		// unlink and delete the old image from memory, unless it's the default one
		image = s->passes[0].anim_frames[0];
		if( image && image != r_notexture ) {
			R_FreeImage( image );
		}

		image = R_LoadPic( name, &data, width, height, IT_CLAMP|IT_NOPICMIP|IT_NOMIPMAP, 4 );
		if( !image ) {
			// the passed data can fail to load properly, so default to chessboard image
			image = r_notexture;
		}
		s->passes[0].anim_frames[0] = image;
	}
	return s;
}

/*
* R_RegisterLevelshot
*/
shader_t *R_RegisterLevelshot( const char *name, shader_t *defaultShader, qboolean *matchesDefault )
{
	shader_t *shader;

	r_defaultImage = defaultShader ? defaultShader->passes[0].anim_frames[0] : NULL;
	shader = R_LoadShader( name, SHADER_2D, qtrue, 0, SHADER_INVALID, NULL );

	if( matchesDefault )
		*matchesDefault = ((shader->passes[0].anim_frames[0] == r_defaultImage) ? qtrue : qfalse);

	r_defaultImage = NULL;

	return shader;
}

/*
* R_RegisterShader
*/
shader_t *R_RegisterShader( const char *name )
{
	return R_LoadShader( name, SHADER_BSP, qfalse, 0, SHADER_INVALID, NULL );
}

/*
* R_RegisterSkin
*/
shader_t *R_RegisterSkin( const char *name )
{
	return R_LoadShader( name, SHADER_MD3, qfalse, 0, SHADER_INVALID, NULL );
}

/*
* R_RegisterVideo
*/
shader_t *R_RegisterVideo( const char *name )
{
	return R_LoadShader( name, SHADER_VIDEO, qfalse, 0, SHADER_INVALID, NULL );
}

/*
* R_RemapShader
*/
void R_RemapShader( const char *from, const char *to, int timeOffset )
{
}

/*
* R_GetShaderDimensions
*
* Returns dimensions for shader's base (taken from the first pass) image
*/
void R_GetShaderDimensions( const shader_t *shader, int *width, int *height, int *depth )
{
	image_t *baseImage;

	assert( shader );
	if( !shader || !shader->numpasses ) {
		return;
	}

	baseImage = shader->passes[0].anim_frames[0];
	if( !baseImage ) {
		Com_DPrintf( S_COLOR_YELLOW "R_GetShaderDimensions: shader %s is missing base image\n", shader->name );
		return;
	}

	if( width ) {
		*width = baseImage->width;
	}
	if( height ) {
		*height = baseImage->height;
	}
	if( depth ) {
		*depth = baseImage->depth;
	}
}
