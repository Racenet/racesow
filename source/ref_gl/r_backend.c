/*
Copyright (C) 2002-2011 Victor Luchits

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

#include "r_local.h"
#include "r_backend_local.h"

#define FTABLE_SIZE_POW	12
#define FTABLE_SIZE	( 1<<FTABLE_SIZE_POW )
#define FTABLE_CLAMP( x ) ( ( (int)( ( x )*FTABLE_SIZE ) & ( FTABLE_SIZE-1 ) ) )
#define FTABLE_EVALUATE( table, x ) ( ( table )[FTABLE_CLAMP( x )] )

static float r_sintable[FTABLE_SIZE];
static float r_sintableByte[256];
static float r_triangletable[FTABLE_SIZE];
static float r_squaretable[FTABLE_SIZE];
static float r_sawtoothtable[FTABLE_SIZE];
static float r_inversesawtoothtable[FTABLE_SIZE];

#define NOISE_SIZE	256
#define NOISE_VAL( a )	  r_noiseperm[( a ) & ( NOISE_SIZE - 1 )]
#define NOISE_INDEX( x, y, z, t ) NOISE_VAL( x + NOISE_VAL( y + NOISE_VAL( z + NOISE_VAL( t ) ) ) )
#define NOISE_LERP( a, b, w ) ( a * ( 1.0f - w ) + b * w )

static float r_noisetable[NOISE_SIZE];
static int r_noiseperm[NOISE_SIZE];

ALIGN( 16 ) vec4_t inVertsArray[MAX_ARRAY_VERTS];
ALIGN( 16 ) vec4_t inNormalsArray[MAX_ARRAY_VERTS];
ALIGN( 16 ) vec4_t inSVectorsArray[MAX_ARRAY_VERTS];
elem_t inElemsArray[MAX_ARRAY_ELEMENTS];
vec2_t inCoordsArray[MAX_ARRAY_VERTS];
vec2_t inLightmapCoordsArray[MAX_LIGHTMAPS][MAX_ARRAY_VERTS];
byte_vec4_t inColorsArray[MAX_LIGHTMAPS][MAX_ARRAY_VERTS];

vec2_t tUnitCoordsArray[MAX_TEXTURE_UNITS][MAX_ARRAY_VERTS];

elem_t *elemsArray;
vec4_t *vertsArray;
vec4_t *normalsArray;
vec4_t *sVectorsArray;
vec2_t *coordsArray;
vec2_t *lightmapCoordsArray[MAX_LIGHTMAPS];
byte_vec4_t *colorArray;
byte_vec4_t colorArrayCopy[MAX_ARRAY_VERTS];

rbackend_t r_back;
rbackacc_t r_backacc;

static qboolean r_triangleOutlines;

static qboolean	r_arraysLocked;
static qboolean	r_normalsEnabled;
static qboolean r_bonesEnabled;
static qboolean r_bufferBound;

static shaderpass_t r_dlightsPass, r_fogPass, r_drawFlatPass;
static float r_lightmapPassesArgs[MAX_TEXTURE_UNITS+1][3];
static shaderpass_t r_lightmapPasses[MAX_TEXTURE_UNITS+1];

static shaderpass_t r_triOutlinesPass;
static vec4_t r_triOutlinesColor;

enum
{
	BUILTIN_GLSLPASS_DLIGHT,
	BUILTIN_GLSLPASS_DIFFUSE,
	BUILTIN_GLSLPASS_DECAL,
	BUILTIN_GLSLPASS_SHADOWMAP,
#ifdef HARDWARE_OUTLINES
	BUILTIN_GLSLPASS_OUTLINE,
#endif
	MAX_BUILTIN_GLSLPASSES
};

static shaderpass_t r_GLSLpasses[MAX_BUILTIN_GLSLPASSES];

int r_features;

static void R_LockArrays( int numverts );
static void R_UnlockArrays( void );
static void R_DrawTriangles( void );
static void R_DrawNormals( void );
static void R_CleanUpTextureUnits( int last );
static void R_AccumulatePass( shaderpass_t *pass, int lightmapStyle );
static void R_AccumulateDynamicLightPasses( void );

static void R_BackendInitPasses( void );

/*
* R_BackendInit
*/
void R_BackendInit( void )
{
	int i;
	float t;

	memset( &r_back, 0, sizeof( r_back ) );
	memset( &r_backacc, 0, sizeof( r_backacc ) );

	r_arraysLocked = qfalse;
	r_triangleOutlines = qfalse;
	r_bufferBound = qfalse;

	R_ClearArrays();

	R_BackendResetPassMask();

	qglEnableClientState( GL_VERTEX_ARRAY );

	if( r_ignorehwgamma->integer )
		r_back.overBrightBits = 0;
	else
		r_back.overBrightBits = ( r_overbrightbits->integer > 0 ) ? r_overbrightbits->integer : 0;
	r_back.identityLighting = (int)( 255.0f / (float)(1 << r_back.overBrightBits) );

	// build lookup tables
	for( i = 0; i < FTABLE_SIZE; i++ )
	{
		t = (float)i / (float)FTABLE_SIZE;

		r_sintable[i] = sin( t * M_TWOPI );

		if( t < 0.25 )
			r_triangletable[i] = t * 4.0;
		else if( t < 0.75 )
			r_triangletable[i] = 2 - 4.0 * t;
		else
			r_triangletable[i] = ( t - 0.75 ) * 4.0 - 1.0;

		if( t < 0.5 )
			r_squaretable[i] = 1.0f;
		else
			r_squaretable[i] = -1.0f;

		r_sawtoothtable[i] = t;
		r_inversesawtoothtable[i] = 1.0 - t;
	}

	for( i = 0; i < 256; i++ )
		r_sintableByte[i] = sin( (float)i / 255.0 * M_TWOPI );

	// init the noise table
	srand( 1001 );

	for( i = 0; i < NOISE_SIZE; i++ )
	{
		r_noisetable[i] = (float)( ( ( rand() / (float)RAND_MAX ) * 2.0 - 1.0 ) );
		r_noiseperm[i] = (unsigned char)( rand() / (float)RAND_MAX * 255 );
	}

	R_BackendInitPasses();

}

/*
* R_BackendInitPasses
*/
static void R_BackendInitPasses( void )
{
	int i;
	shaderpass_t *pass;

	// init dynamic lights pass
	pass = &r_dlightsPass;
	memset( pass, 0, sizeof( shaderpass_t ) );
	pass->flags = SHADERPASS_DLIGHT|GLSTATE_DEPTHFUNC_EQ|GLSTATE_SRCBLEND_DST_COLOR|GLSTATE_DSTBLEND_ONE;


	// init fog pass
	pass = &r_fogPass;
	memset( pass, 0, sizeof( shaderpass_t ) );
	if( glConfig.ext.GLSL )
	{
		pass->program = DEFAULT_GLSL_Q3A_SHADER_PROGRAM;
		pass->program_type = GLSL_PROGRAM_TYPE_Q3A_SHADER;
	}
	pass->tcgen = TC_GEN_FOG;
	pass->rgbgen.type = RGB_GEN_FOG;
	pass->alphagen.type = ALPHA_GEN_IDENTITY;
	pass->flags = SHADERPASS_NOCOLORARRAY|SHADERPASS_BLEND_DECAL|GLSTATE_SRCBLEND_SRC_ALPHA|GLSTATE_DSTBLEND_ONE_MINUS_SRC_ALPHA;


	// the very first lightmap pass is reserved for GL_REPLACE or GL_MODULATE
	memset( r_lightmapPasses, 0, sizeof( r_lightmapPasses ) );

	pass = &r_lightmapPasses[0];
	pass->rgbgen.args = r_lightmapPassesArgs[0];

	// the rest are GL_ADD
	for( i = 1; i < MAX_TEXTURE_UNITS+1; i++ )
	{
		pass = &r_lightmapPasses[i];
		pass->flags = SHADERPASS_LIGHTMAP|SHADERPASS_NOCOLORARRAY|GLSTATE_DEPTHFUNC_EQ
			|SHADERPASS_BLEND_ADD|GLSTATE_SRCBLEND_ONE|GLSTATE_DSTBLEND_ONE;
		pass->tcgen = TC_GEN_LIGHTMAP;
		pass->alphagen.type = ALPHA_GEN_IDENTITY;
		pass->rgbgen.args = r_lightmapPassesArgs[i];
	}


	// flat color pass
	pass = &r_drawFlatPass;
	memset( pass, 0, sizeof( shaderpass_t ) );
	pass->tcgen = TC_GEN_DRAWFLAT;
	pass->rgbgen.type = RGB_GEN_IDENTITY;
	pass->alphagen.type = ALPHA_GEN_IDENTITY;
	pass->flags = SHADERPASS_NOCOLORARRAY|SHADERPASS_BLEND_MODULATE|GLSTATE_SRCBLEND_ZERO|GLSTATE_DSTBLEND_SRC_COLOR;


	// init optional GLSL program passes
	memset( r_GLSLpasses, 0, sizeof( r_GLSLpasses ) );

	pass = &r_GLSLpasses[BUILTIN_GLSLPASS_DLIGHT];
	pass->flags = SHADERPASS_NOCOLORARRAY|GLSTATE_DEPTHFUNC_EQ|GLSTATE_SRCBLEND_DST_COLOR|GLSTATE_DSTBLEND_ONE;
	pass->program_type = GLSL_PROGRAM_TYPE_DYNAMIC_LIGHTS;
	pass->program = DEFAULT_GLSL_DYNAMIC_LIGHTS_PROGRAM;


	pass = &r_GLSLpasses[BUILTIN_GLSLPASS_DIFFUSE];
	pass->flags = SHADERPASS_NOCOLORARRAY|SHADERPASS_BLEND_MODULATE|GLSTATE_SRCBLEND_ZERO|GLSTATE_DSTBLEND_SRC_COLOR;
	pass->tcgen = TC_GEN_BASE;
	pass->rgbgen.type = RGB_GEN_IDENTITY;
	pass->alphagen.type = ALPHA_GEN_IDENTITY;


	memcpy( &r_GLSLpasses[BUILTIN_GLSLPASS_DECAL], &r_GLSLpasses[BUILTIN_GLSLPASS_DIFFUSE], sizeof( shaderpass_t ) );


	pass = &r_GLSLpasses[BUILTIN_GLSLPASS_SHADOWMAP];
	pass->flags = SHADERPASS_NOCOLORARRAY|SHADERPASS_BLEND_MODULATE|GLSTATE_DEPTHFUNC_EQ /*|GLSTATE_OFFSET_FILL*/|GLSTATE_SRCBLEND_ZERO|GLSTATE_DSTBLEND_SRC_COLOR;
	pass->tcgen = TC_GEN_PROJECTION_SHADOW;
	pass->rgbgen.type = RGB_GEN_IDENTITY;
	pass->alphagen.type = ALPHA_GEN_IDENTITY;
	pass->program = DEFAULT_GLSL_SHADOWMAP_PROGRAM;
	pass->program_type = GLSL_PROGRAM_TYPE_SHADOWMAP;


#ifdef HARDWARE_OUTLINES
	pass = &r_GLSLpasses[BUILTIN_GLSLPASS_OUTLINE];
	memset( pass, 0, sizeof( shaderpass_t ) );
	pass->flags = SHADERPASS_NOCOLORARRAY|GLSTATE_SRCBLEND_ONE|GLSTATE_DSTBLEND_ZERO|SHADERPASS_BLEND_MODULATE|GLSTATE_DEPTHWRITE;
	pass->rgbgen.type = RGB_GEN_OUTLINE;
	pass->alphagen.type = ALPHA_GEN_OUTLINE;
	pass->tcgen = TC_GEN_NONE;
	pass->program = DEFAULT_GLSL_OUTLINE_PROGRAM;
	pass->program_type = GLSL_PROGRAM_TYPE_OUTLINE;
#endif
}

/*
* R_BackendShutdown
*/
void R_BackendShutdown( void )
{
}

/*
* R_FastSin
*/
float R_FastSin( float t )
{
	return FTABLE_EVALUATE( r_sintable, t );
}

/*
* R_BackendMemcpy
*/
static inline void R_BackendMemcpy( void *to, const void *from, size_t size )
{
	memcpy( to, from, size );
	r_backacc.c_totalMemCpy += size;
}

/*
* R_LatLongToNorm
*/
void R_LatLongToNorm( const qbyte latlong[2], vec3_t out )
{
	float sin_a, sin_b, cos_a, cos_b;

	cos_a = r_sintableByte[( latlong[0] + 64 ) & 255];
	sin_a = r_sintableByte[latlong[0]];
	cos_b = r_sintableByte[( latlong[1] + 64 ) & 255];
	sin_b = r_sintableByte[latlong[1]];

	VectorSet( out, cos_b * sin_a, sin_b * sin_a, cos_a );
}

/*
* R_TableForFunc
*/
static float *R_TableForFunc( unsigned int func )
{
	switch( func )
	{
	case SHADER_FUNC_SIN:
		return r_sintable;
	case SHADER_FUNC_TRIANGLE:
		return r_triangletable;
	case SHADER_FUNC_SQUARE:
		return r_squaretable;
	case SHADER_FUNC_SAWTOOTH:
		return r_sawtoothtable;
	case SHADER_FUNC_INVERSESAWTOOTH:
		return r_inversesawtoothtable;
	default:
		break;
	}

	return r_sintable;  // default to sintable
}

/*
* R_DistanceRamp
*/
static float R_DistanceRamp( float x1, float x2, float y1, float y2 )
{
	float dist;

	if( !ri.currententity )
		return y2;

	if( (ri.currentmodel && ri.currentmodel->type != mod_brush) || ri.currententity->rtype == RT_SPRITE )
	{
		// for models and sprites, ignore vertex data, use origin
		dist = DistanceFast( ri.viewOrigin, ri.currententity->origin );
	}
	else
	{
		vec_t *v0, *v1, *v2;
		vec3_t e0, e1, e2;

		// calculate plane's normal and distance to eye
		v0 = vertsArray[elemsArray[0]];
		v1 = vertsArray[elemsArray[1]];
		v2 = vertsArray[elemsArray[2]];

		VectorSubtract( v1, v0, e0 );
		VectorSubtract( v2, v0, e1 );
		CrossProduct( e1, e0, e2 );
		VectorNormalize( e2 );

		VectorSubtract( ri.viewOrigin, v0, e0 );
		dist = DotProduct( e0, e2 );

		// autosprites always face the viewer so distance can't be negative
		if ( r_back.currentShader->flags & SHADER_AUTOSPRITE )
			dist = fabs( dist );
	}

	if( dist <= x1 )
		return y1;
	if( dist >= x2 )
		return y2;
	return ( (x1 == x2 ? 0 : ((y2 - y1) / (x2 - x1)) * (dist - x1)) + y1 );
}

/*
* R_BackendGetNoiseValue
*/
float R_BackendGetNoiseValue( float x, float y, float z, float t )
{
	int i;
	int ix, iy, iz, it;
	float fx, fy, fz, ft;
	float front[4], back[4];
	float fvalue, bvalue, value[2], finalvalue;

	ix = ( int )floor( x );
	fx = x - ix;
	iy = ( int )floor( y );
	fy = y - iy;
	iz = ( int )floor( z );
	fz = z - iz;
	it = ( int )floor( t );
	ft = t - it;

	for( i = 0; i < 2; i++ )
	{
		front[0] = r_noisetable[NOISE_INDEX( ix, iy, iz, it + i )];
		front[1] = r_noisetable[NOISE_INDEX( ix+1, iy, iz, it + i )];
		front[2] = r_noisetable[NOISE_INDEX( ix, iy+1, iz, it + i )];
		front[3] = r_noisetable[NOISE_INDEX( ix+1, iy+1, iz, it + i )];

		back[0] = r_noisetable[NOISE_INDEX( ix, iy, iz + 1, it + i )];
		back[1] = r_noisetable[NOISE_INDEX( ix+1, iy, iz + 1, it + i )];
		back[2] = r_noisetable[NOISE_INDEX( ix, iy+1, iz + 1, it + i )];
		back[3] = r_noisetable[NOISE_INDEX( ix+1, iy+1, iz + 1, it + i )];

		fvalue = NOISE_LERP( NOISE_LERP( front[0], front[1], fx ), NOISE_LERP( front[2], front[3], fx ), fy );
		bvalue = NOISE_LERP( NOISE_LERP( back[0], back[1], fx ), NOISE_LERP( back[2], back[3], fx ), fy );
		value[i] = NOISE_LERP( fvalue, bvalue, fz );
	}

	finalvalue = NOISE_LERP( value[0], value[1], ft );

	return finalvalue;
}

/*
* R_BackendResetCounters
*/
void R_BackendResetCounters( void )
{
	memset( &r_backacc, 0, sizeof( r_backacc ) );
}

/*
* R_BackendStartFrame
*/
void R_BackendStartFrame( void )
{
	r_speeds_msg[0] = '\0';
	R_BackendResetCounters();
}

/*
* R_BackendEndFrame
*/
void R_BackendEndFrame( void )
{
	// unlock arrays if any
	R_UnlockArrays();

	// clean up texture units
	R_CleanUpTextureUnits( 1 );

	if( r_speeds->integer && !( ri.refdef.rdflags & RDF_NOWORLDMODEL ) )
	{
		switch( r_speeds->integer )
		{
		case 1:
		default:
			Q_snprintfz( r_speeds_msg, sizeof( r_speeds_msg ),
				"%4i wpoly %4i leafs %5i wverts %5i wtris %4i vbos\n"
				"%4i verts %4i tris %4i vboverts(%3i%%) %4i vbotris(%3i%%)\n"
				"%6ik memcpy %4i draws",
				c_brush_polys, c_world_leafs, c_world_verts, c_world_tris, c_world_vbos,
				r_backacc.c_totalVerts, r_backacc.c_totalTris,
				r_backacc.c_totalVboVerts, r_backacc.c_totalVerts ? 100 * r_backacc.c_totalVboVerts/r_backacc.c_totalVerts : 0,
				r_backacc.c_totalVboTris, r_backacc.c_totalTris ? 100 * r_backacc.c_totalVboTris/r_backacc.c_totalTris : 0,
				(r_backacc.c_totalMemCpy + 1023) / 1024, r_backacc.c_totalDraws
			);
			break;
		case 2:
			Q_snprintfz( r_speeds_msg, sizeof( r_speeds_msg ),
				"lvs: %5i  node: %5i  farclip: %6.f",
				r_mark_leaves,
				r_world_node,
				ri.farClip
			);
			break;
		case 3:
			Q_snprintfz( r_speeds_msg, sizeof( r_speeds_msg ),
				"polys\\ents: %5i\\%5i  sort\\draw: %5i\\%i",
				r_add_polys, r_add_entities,
				r_sort_meshes, r_draw_meshes
			);
			break;
		case 4:
		case 5:
			if( r_debug_surface )
			{
				Q_snprintfz( r_speeds_msg, sizeof( r_speeds_msg ),
					"%s", r_debug_surface->shader->name );

				Q_strncatz( r_speeds_msg, "\n", sizeof( r_speeds_msg ) );

				if( r_debug_surface->mesh )
					Q_snprintfz( r_speeds_msg + strlen( r_speeds_msg ), sizeof( r_speeds_msg ) - strlen( r_speeds_msg ),
						"verts: %5i tris: %5i", r_debug_surface->mesh->numVerts, r_debug_surface->mesh->numElems / 3 );

				if( r_debug_surface->vbo )
					Q_strncatz( r_speeds_msg,
						va( "\nvboverts: %5i vbotris: %5i", r_debug_surface->vbo->numVerts, r_debug_surface->vbo->numElems / 3 ),
						sizeof( r_speeds_msg ) );

				Q_strncatz( r_speeds_msg, "\n", sizeof( r_speeds_msg ) );

				if( r_debug_surface->fog && r_debug_surface->fog->shader
					&& r_debug_surface->fog->shader != r_debug_surface->shader )
					Q_strncatz( r_speeds_msg, r_debug_surface->fog->shader->name, sizeof( r_speeds_msg ) );
			}
			break;
		case 6:
			Q_snprintfz( r_speeds_msg, sizeof( r_speeds_msg ),
				"%.1f %.1f %.1f",
				ri.refdef.vieworg[0], ri.refdef.vieworg[1], ri.refdef.vieworg[2]
				);
			break;
		}
	}
}

/*
* R_LockArrays
*/
static void R_LockArrays( int numverts )
{
	if( r_bufferBound || r_arraysLocked )
		return;

	if( r_back.currentMeshVBO )
	{
		GL_BindBuffer( GL_ARRAY_BUFFER_ARB, r_back.currentMeshVBO->vertexId );

		GL_BindBuffer( GL_ELEMENT_ARRAY_BUFFER_ARB, r_back.currentMeshVBO->elemId );

		if( r_features & MF_BONES ) {
			r_bonesEnabled = qtrue;

			// submit indices
			GL_EnableVertexAttrib( GLSL_ATTRIB_BONESINDICES, qtrue );
			qglVertexAttribPointerARB( GLSL_ATTRIB_BONESINDICES, 4, GL_UNSIGNED_BYTE, GL_FALSE, SKM_MAX_WEIGHTS, ( const GLvoid * )r_back.currentMeshVBO->bonesIndicesOffset );

			// submit weights
			GL_EnableVertexAttrib( GLSL_ATTRIB_BONESWEIGHTS, qtrue );
			qglVertexAttribPointerARB( GLSL_ATTRIB_BONESWEIGHTS, 4, GL_UNSIGNED_BYTE, GL_TRUE, SKM_MAX_WEIGHTS, ( const GLvoid * )r_back.currentMeshVBO->bonesWeightsOffset );
		}

		if( r_features & MF_ENABLENORMALS ) {
			r_normalsEnabled = qtrue;

			qglEnableClientState( GL_NORMAL_ARRAY );
			qglNormalPointer( GL_FLOAT, 16, ( const GLvoid * ) r_back.currentMeshVBO->normalsOffset );
		}

		qglVertexPointer( 3, GL_FLOAT, 16, ( const GLvoid * )0 );

		r_bufferBound = qtrue;
	}
	else
	{
		qglVertexPointer( 3, GL_FLOAT, 16, vertsArray );

		if( r_features & MF_ENABLENORMALS )
		{
			r_normalsEnabled = qtrue;
			qglEnableClientState( GL_NORMAL_ARRAY );
			qglNormalPointer( GL_FLOAT, 16, normalsArray );
		}

		if( glConfig.ext.compiled_vertex_array )
			qglLockArraysEXT( 0, numverts );

		r_arraysLocked = qtrue;
	}
}

/*
* R_UnlockArrays
*/
static void R_UnlockArrays( void )
{
	if( r_bufferBound )
	{
		if( r_bonesEnabled ) {
			r_bonesEnabled = qfalse;
			GL_EnableVertexAttrib( GLSL_ATTRIB_BONESINDICES, qfalse );
			GL_EnableVertexAttrib( GLSL_ATTRIB_BONESWEIGHTS, qfalse );
		}

		if( r_normalsEnabled ) {
			r_normalsEnabled = qfalse;
			qglDisableClientState( GL_NORMAL_ARRAY );
		}

		GL_BindBuffer( GL_ARRAY_BUFFER_ARB, 0 );
		GL_BindBuffer( GL_ELEMENT_ARRAY_BUFFER_ARB, 0 );

		r_bufferBound = qfalse;
	}
	else if ( r_arraysLocked )
	{
		if( glConfig.ext.compiled_vertex_array )
			qglUnlockArraysEXT();

		if( r_normalsEnabled )
		{
			r_normalsEnabled = qfalse;
			qglDisableClientState( GL_NORMAL_ARRAY );
		}

		r_arraysLocked = qfalse;
	}
}

/*
* R_ClearArrays
*/
void R_ClearArrays( void )
{
	int i;

	r_backacc.numVerts = 0;
	r_backacc.numElems = 0;
	r_backacc.numColors = 0;

	vertsArray = inVertsArray;
	elemsArray = inElemsArray;
	normalsArray = inNormalsArray;
	sVectorsArray = inSVectorsArray;
	coordsArray = inCoordsArray;
	for( i = 0; i < MAX_LIGHTMAPS; i++ )
		lightmapCoordsArray[i] = inLightmapCoordsArray[i];
	colorArray = colorArrayCopy;
}

/*
* R_DrawElements
*/
void R_DrawElements( void )
{
	const GLvoid *elems;
	int numVerts, numElems;
	int firstVert;

	if( r_bufferBound )
	{
		firstVert = r_back.currentMeshBuffer->firstVBOVert;
		numVerts = r_back.currentMeshBuffer->numVerts;
		numElems = r_back.currentMeshBuffer->numElems;
		elems = (GLvoid *)(r_back.currentMeshBuffer->firstVBOElem * sizeof( int ));

		r_backacc.c_totalVboVerts += numVerts;
		r_backacc.c_totalVboTris += numElems / 3;
	}
	else
	{
		firstVert = 0;
		numVerts = r_backacc.numVerts;
		numElems = r_backacc.numElems;
		elems = elemsArray;
	}

	if( glConfig.ext.draw_range_elements )
		qglDrawRangeElementsEXT( GL_TRIANGLES, firstVert, firstVert + numVerts - 1, numElems, GL_UNSIGNED_INT, elems );
	else
		qglDrawElements( GL_TRIANGLES, numElems, GL_UNSIGNED_INT, elems );

	r_backacc.c_totalVerts += numVerts;
	r_backacc.c_totalTris += numElems / 3;
	r_backacc.c_totalDraws++;
}

/*
* R_FlushArrays
*/
void R_FlushArrays( void )
{
	if( !r_backacc.numVerts || !r_backacc.numElems )
		return;

	if( r_backacc.numColors == 1 )
	{
		qglColor4ubv( colorArray[0] );
	}
	else if( r_backacc.numColors > 1 )
	{
		qglEnableClientState( GL_COLOR_ARRAY );
		qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, r_bufferBound ? ( const GLvoid * )r_back.currentMeshVBO->colorsOffset[0] : colorArray );
	}

	if( r_drawelements->integer || glState.in2DMode || ri.refdef.rdflags & RDF_NOWORLDMODEL )
		R_DrawElements();

	if( r_backacc.numColors > 1 )
		qglDisableClientState( GL_COLOR_ARRAY );
}

/*
* GL_DisableAllTexGens
*/
static inline void GL_DisableAllTexGens( void )
{
	GL_EnableTexGen( GL_S, 0 );
	GL_EnableTexGen( GL_T, 0 );
	GL_EnableTexGen( GL_R, 0 );
	GL_EnableTexGen( GL_Q, 0 );
}

/*
* R_CleanUpTextureUnits
*/
static void R_CleanUpTextureUnits( int last )
{
	int i;

	for( i = glState.currentTMU; i > last - 1; i-- )
	{
		GL_DisableAllTexGens();
		GL_SetTexCoordArrayMode( 0 );

		qglDisable( GL_TEXTURE_2D );
		qglDisable( GL_TEXTURE_3D );
		GL_SelectTexture( i - 1 );
	}
}

/*
* R_TransformFogPlanes
*/
void R_TransformFogPlanes( const mfog_t *fog, vec3_t fogNormal, vec_t *fogDist, vec3_t vpnNormal, vec_t *vpnDist )
{
	cplane_t *fogPlane;
	shader_t *fogShader;
	vec3_t viewtofog;
	float dist;
	entity_t *e = ri.currententity;

	assert( fog );
	assert( fogNormal && fogDist );
	assert( vpnNormal && vpnDist );

	fogPlane = fog->visibleplane;
	fogShader = fog->shader;

	// distance to fog
	dist = ri.fog_dist_to_eye[fog-r_worldbrushmodel->fogs];

	if( r_back.currentShader->flags & SHADER_SKY )
	{
		if( dist > 0 )
			VectorMA( ri.viewOrigin, -dist, fogPlane->normal, viewtofog );
		else
			VectorCopy( ri.viewOrigin, viewtofog );
	}
	else
	{
		VectorCopy( e->origin, viewtofog );
	}

	// some math tricks to take entity's rotation matrix into account
	// for fog texture coordinates calculations:
	// M is rotation matrix, v is vertex, t is transform vector
	// n is plane's normal, d is plane's dist, r is view origin
	// (M*v + t)*n - d = (M*n)*v - ((d - t*n))
	// (M*v + t - r)*n = (M*n)*v - ((r - t)*n)
	fogNormal[0] = DotProduct( e->axis[0], fogPlane->normal ) * e->scale;
	fogNormal[1] = DotProduct( e->axis[1], fogPlane->normal ) * e->scale;
	fogNormal[2] = DotProduct( e->axis[2], fogPlane->normal ) * e->scale;
	*fogDist = ( fogPlane->dist - DotProduct( viewtofog, fogPlane->normal ) );

	vpnNormal[0] = DotProduct( e->axis[0], ri.viewAxis[0] ) * e->scale;
	vpnNormal[1] = DotProduct( e->axis[1], ri.viewAxis[0] ) * e->scale;
	vpnNormal[2] = DotProduct( e->axis[2], ri.viewAxis[0] ) * e->scale;
	*vpnDist = ( ( ri.viewOrigin[0] - viewtofog[0] ) * ri.viewAxis[0][0] + ( ri.viewOrigin[1] - viewtofog[1] ) * ri.viewAxis[0][1] + ( ri.viewOrigin[2] - viewtofog[2] ) * ri.viewAxis[0][2] ) + fogShader->fog_clearDist;
}

/*
* R_DeformVertices
*/
static void R_DeformVertices( void )
{
	unsigned int i, j, k;
	double args[4], temp;
	float deflect, *quad[4];
	const float *table;
	const deformv_t *deformv;
	vec3_t tv, rot_centre;

	if( r_features & MF_HARDWARE )
		return;

	deformv = &r_back.currentShader->deforms[0];
	for( i = 0; i < r_back.currentShader->numdeforms; i++, deformv++ )
	{
		switch( deformv->type )
		{
		case DEFORMV_NONE:
			break;

		case DEFORMV_WAVE:
			table = R_TableForFunc( deformv->func.type );

			// Deflect vertex along its normal by wave amount
			if( deformv->func.args[3] == 0 )
			{
				temp = deformv->func.args[2];
				deflect = FTABLE_EVALUATE( table, temp ) * deformv->func.args[1] + deformv->func.args[0];

				for( j = 0; j < r_backacc.numVerts; j++ )
					VectorMA( inVertsArray[j], deflect, inNormalsArray[j], inVertsArray[j] );
			}
			else
			{
				args[0] = deformv->func.args[0];
				args[1] = deformv->func.args[1];
				args[2] = deformv->func.args[2] + deformv->func.args[3] * r_back.currentShaderTime;
				args[3] = deformv->args[0];

				for( j = 0; j < r_backacc.numVerts; j++ )
				{
					temp = args[2] + args[3] * ( inVertsArray[j][0] + inVertsArray[j][1] + inVertsArray[j][2] );
					deflect = FTABLE_EVALUATE( table, temp ) * args[1] + args[0];
					VectorMA( inVertsArray[j], deflect, inNormalsArray[j], inVertsArray[j] );
				}
			}
			break;

		case DEFORMV_NORMAL:
			// without this * 0.1f deformation looks wrong, although q3a doesn't have it
			args[0] = deformv->func.args[3] * r_back.currentShaderTime * 0.1f;
			args[1] = deformv->func.args[1];

			for( j = 0; j < r_backacc.numVerts; j++ )
			{
				VectorScale( inVertsArray[j], 0.98f, tv );
				inNormalsArray[j][0] += args[1] * R_BackendGetNoiseValue( tv[0], tv[1], tv[2], args[0] );
				inNormalsArray[j][1] += args[1] * R_BackendGetNoiseValue( tv[0] + 100, tv[1], tv[2], args[0] );
				inNormalsArray[j][2] += args[1] * R_BackendGetNoiseValue( tv[0] + 200, tv[1], tv[2], args[0] );
				VectorNormalizeFast( inNormalsArray[j] );
			}
			break;

		case DEFORMV_MOVE:
			table = R_TableForFunc( deformv->func.type );
			temp = deformv->func.args[2] + r_back.currentShaderTime * deformv->func.args[3];
			deflect = FTABLE_EVALUATE( table, temp ) * deformv->func.args[1] + deformv->func.args[0];

			for( j = 0; j < r_backacc.numVerts; j++ )
				VectorMA( inVertsArray[j], deflect, deformv->args, inVertsArray[j] );
			break;

		case DEFORMV_BULGE:
			args[0] = deformv->args[0];
			args[1] = deformv->args[1];
			args[2] = r_back.currentShaderTime * deformv->args[2];

			for( j = 0; j < r_backacc.numVerts; j++ )
			{
				temp = ( coordsArray[j][0] * args[0] + args[2] ) / M_TWOPI;
				deflect = R_FastSin( temp );
				deflect = max( -1 + deformv->args[3], deflect ) * args[1];
				VectorMA( inVertsArray[j], deflect, inNormalsArray[j], inVertsArray[j] );
			}
			break;

		case DEFORMV_AUTOSPRITE:
			{
				vec4_t *v;
				vec2_t *st;
				elem_t *elem;
				float radius;
				vec3_t point, v_centre, v_left, v_up;

				if( r_backacc.numVerts % 4 || r_backacc.numElems % 6 )
					break;
				if( !ri.currententity )
					break;

				if( ri.currentmodel != r_worldmodel )
				{
					Matrix_TransformVector( ri.currententity->axis, ri.viewAxis[1], v_left );
					Matrix_TransformVector( ri.currententity->axis, ri.viewAxis[2], v_up );
				}
				else
				{
					VectorCopy( ri.viewAxis[1], v_left );
					VectorCopy( ri.viewAxis[2], v_up );
				}

				if( ri.params & RP_MIRRORVIEW )
					VectorInverse( v_left );

				radius = ri.currententity->scale;
				if( radius && radius != 1.0f )
				{
					radius = 1.0f / radius;
					VectorScale( v_left, radius, v_left );
					VectorScale( v_up, radius, v_up );
				}

				for( k = 0, v = inVertsArray, st = coordsArray, elem = elemsArray; k < r_backacc.numVerts; k += 4, v += 4, st += 4, elem += 6 )
				{
					for( j = 0; j < 3; j++ )
						v_centre[j] = (v[0][j] + v[1][j] + v[2][j] + v[3][j]) * 0.25;

					VectorSubtract( v[0], v_centre, point );
					radius = VectorLength( point ) * 0.707106f;		// 1.0f / sqrt(2)

					// very similar to R_PushSprite
					VectorMA( v_centre, -radius, v_up, point );
					VectorMA( point,  radius, v_left, v[0] );
					VectorMA( point, -radius, v_left, v[3] );

					VectorMA( v_centre, radius, v_up, point );
					VectorMA( point,  radius, v_left, v[1] );
					VectorMA( point, -radius, v_left, v[2] );

					// reset texcoords
					Vector2Set( st[0], 0, 1 );
					Vector2Set( st[1], 0, 0 );
					Vector2Set( st[2], 1, 0 );
					Vector2Set( st[3], 1, 1 );

					// trifan elems
					elem[0] = k;
					elem[1] = k + 2 - 1;
					elem[2] = k + 2;

					elem[3] = k;
					elem[4] = k + 3 - 1;
					elem[5] = k + 3;
				}
			}
			break;

		case DEFORMV_AUTOSPRITE2:
			if( r_backacc.numElems % 6 )
				break;
			if( !ri.currententity )
				break;

			for( k = 0; k < r_backacc.numElems; k += 6 )
			{
				int long_axis = 0, short_axis = 0;
				vec3_t axis, tmp;
				float len[3];
				vec3_t m0[3], m1[3], m2[3], result[3];

				quad[0] = ( float * )( inVertsArray + elemsArray[k+0] );
				quad[1] = ( float * )( inVertsArray + elemsArray[k+1] );
				quad[2] = ( float * )( inVertsArray + elemsArray[k+2] );

				for( j = 2; j >= 0; j-- )
				{
					quad[3] = ( float * )( inVertsArray + elemsArray[k+3+j] );

					if( !VectorCompare( quad[3], quad[0] ) &&
						!VectorCompare( quad[3], quad[1] ) &&
						!VectorCompare( quad[3], quad[2] ) )
					{
						break;
					}
				}

				// build a matrix were the longest axis of the billboard is the Y-Axis
				VectorSubtract( quad[1], quad[0], m0[0] );
				VectorSubtract( quad[2], quad[0], m0[1] );
				VectorSubtract( quad[2], quad[1], m0[2] );
				len[0] = DotProduct( m0[0], m0[0] );
				len[1] = DotProduct( m0[1], m0[1] );
				len[2] = DotProduct( m0[2], m0[2] );

				for( j = 0; j < 3; j++ )
				{
					int a1 = (j+1)%3, a2 = (j+2)%3;

					if( len[j] > len[a1] && len[j] > len[a2] )
					{
						if( len[a1] > len[a2] )
						{
							long_axis = a1;
							short_axis = a2;
						}
						else
						{
							long_axis = a2;
							short_axis = a1;
						}
						break;
					}
				}

				if( !len[long_axis] )
					break;
				len[long_axis] = Q_RSqrt( len[long_axis] );
				VectorScale( m0[long_axis], len[long_axis], axis );

				if( DotProduct( m0[long_axis], m0[short_axis] ) )
				{
					VectorCopy( axis, m0[1] );
					if( axis[0] || axis[1] )
						MakeNormalVectors( m0[1], m0[0], m0[2] );
					else
						MakeNormalVectors( m0[1], m0[2], m0[0] );
				}
				else
				{
					if( !len[short_axis] )
						break;
					len[short_axis] = Q_RSqrt( len[short_axis] );
					VectorScale( m0[short_axis], len[short_axis], m0[0] );
					VectorCopy( axis, m0[1] );
					CrossProduct( m0[0], m0[1], m0[2] );
				}

				for( j = 0; j < 3; j++ )
					rot_centre[j] = ( quad[0][j] + quad[1][j] + quad[2][j] + quad[3][j] ) * 0.25;

				if( ri.currententity && ( ri.currentmodel != r_worldmodel ) )
				{
					VectorAdd( ri.currententity->origin, rot_centre, tv );
					VectorSubtract( ri.viewOrigin, tv, tmp );
					Matrix_TransformVector( ri.currententity->axis, tmp, tv );
				}
				else
				{
					VectorCopy( rot_centre, tv );
					VectorSubtract( ri.viewOrigin, tv, tv );
				}

				// filter any longest-axis-parts off the camera-direction
				deflect = -DotProduct( tv, axis );

				VectorMA( tv, deflect, axis, m1[2] );
				VectorNormalizeFast( m1[2] );
				VectorCopy( axis, m1[1] );
				CrossProduct( m1[1], m1[2], m1[0] );

				Matrix_Transpose( m1, m2 );
				Matrix_Multiply( m2, m0, result );

				for( j = 0; j < 4; j++ )
				{
					VectorSubtract( quad[j], rot_centre, tv );
					Matrix_TransformVector( result, tv, quad[j] );
					VectorAdd( rot_centre, quad[j], quad[j] );
				}
			}
			break;

		case DEFORMV_PROJECTION_SHADOW:
			{
				float dist, planedist;
				vec3_t planenormal, lightdir;

				R_DeformVPlanarShadowParams( planenormal, &planedist, lightdir );

				for( j = 0; j < r_backacc.numVerts; j++ )
				{
					dist = DotProduct( inVertsArray[j], planenormal ) - planedist;
					if( dist > 0 )
						VectorMA( inVertsArray[j], dist, lightdir, inVertsArray[j] );
				}
			}
			break;

		case DEFORMV_AUTOPARTICLE:
			{
				float scale;
				vec3_t m0[3], m1[3], m2[3], result[3];

				if( r_backacc.numElems % 6 )
					break;

				if( ri.currententity && ( ri.currentmodel != r_worldmodel ) )
					Matrix4_Matrix( ri.modelviewMatrix, m1 );
				else
					Matrix4_Matrix( ri.worldviewMatrix, m1 );

				Matrix_Transpose( m1, m2 );

				for( k = 0; k < r_backacc.numElems; k += 6 )
				{
					quad[0] = ( float * )( inVertsArray + elemsArray[k+0] );
					quad[1] = ( float * )( inVertsArray + elemsArray[k+1] );
					quad[2] = ( float * )( inVertsArray + elemsArray[k+2] );

					for( j = 2; j >= 0; j-- )
					{
						quad[3] = ( float * )( inVertsArray + elemsArray[k+3+j] );

						if( !VectorCompare( quad[3], quad[0] ) &&
							!VectorCompare( quad[3], quad[1] ) &&
							!VectorCompare( quad[3], quad[2] ) )
						{
							break;
						}
					}

					Matrix_FromPoints( quad[0], quad[1], quad[2], m0 );
					Matrix_Multiply( m2, m0, result );

					// hack a scale up to keep particles from disappearing
					scale = ( quad[0][0] - ri.viewOrigin[0] ) * ri.viewAxis[0][0] + ( quad[0][1] - ri.viewOrigin[1] ) * ri.viewAxis[0][1] + ( quad[0][2] - ri.viewOrigin[2] ) * ri.viewAxis[0][2];
					if( scale < 20 )
						scale = 1.5;
					else
						scale = 1.5 + scale * 0.006f;

					for( j = 0; j < 3; j++ )
						rot_centre[j] = ( quad[0][j] + quad[1][j] + quad[2][j] + quad[3][j] ) * 0.25;

					for( j = 0; j < 4; j++ )
					{
						VectorSubtract( quad[j], rot_centre, tv );
						Matrix_TransformVector( result, tv, quad[j] );
						VectorMA( rot_centre, scale, quad[j], quad[j] );
					}
				}
			}
			break;

#ifdef HARDWARE_OUTLINES
		case DEFORMV_OUTLINE:
			if( !ri.currententity )
				break;

			// Deflect vertex along its normal by outline amount
			deflect = ri.currententity->outlineHeight * r_outlines_scale->value;
			for( j = 0; j < r_backacc.numVerts; j++ )
				VectorMA( inVertsArray[j], deflect, inNormalsArray[j], inVertsArray[j] );
			break;
#endif

		default:
			break;
		}
	}
}

/*
* R_VertexTCCellshadeMatrix
*/
void R_VertexTCCellshadeMatrix( mat4x4_t matrix )
{
	vec3_t dir;
	mat4x4_t m;

	if( ri.currententity && !( ri.params & RP_SHADOWMAPVIEW ) )
	{
		R_LightForOrigin( ri.currententity->lightingOrigin, dir, NULL, NULL, ri.currentmodel->radius * ri.currententity->scale );

		Matrix4_Identity( m );

		// rotate direction
		Matrix_TransformVector( ri.currententity->axis, dir, &m[0] );
		VectorNormalize( &m[0] );

		MakeNormalVectors( &m[0], &m[4], &m[8] );
		Matrix4_Transpose( m, matrix );
	}
	else
	{
		Matrix4_Identity( matrix );
	}
}

/*
* R_VertexTCBase
*/
qboolean R_VertexTCBase( const shaderpass_t *pass, int unit, mat4x4_t matrix, r_glslfeat_t *programFeatures )
{
	unsigned int i;
	float *outCoords;
	qboolean identityMatrix = qfalse;

	Matrix4_Identity( matrix );

	switch( pass->tcgen )
	{
	case TC_GEN_BASE:
		GL_DisableAllTexGens();
		qglTexCoordPointer( 2, GL_FLOAT, 0, r_bufferBound ? ( const GLvoid * )r_back.currentMeshVBO->stOffset : coordsArray );
		return qtrue;

	case TC_GEN_LIGHTMAP:
		GL_DisableAllTexGens();
		qglTexCoordPointer( 2, GL_FLOAT, 0, r_bufferBound ? ( const GLvoid * )r_back.currentMeshVBO->lmstOffset[r_back.lightmapStyleNum[unit]] : lightmapCoordsArray[r_back.lightmapStyleNum[unit]] );
		return qtrue;

	case TC_GEN_ENVIRONMENT:
		{
			float depth, *n;
			vec3_t projection, transform;

			if( glState.in2DMode )
				return qtrue;

			if( programFeatures )
			{
				GL_SetTexCoordArrayMode( 0 );
				*programFeatures |= GLSL_COMMON_APPLY_TC_GEN_ENV;
			}
			else
			{
				if( ri.currententity && !( ri.params & RP_SHADOWMAPVIEW ) )
				{
					VectorSubtract( ri.viewOrigin, ri.currententity->origin, projection );
					Matrix_TransformVector( ri.currententity->axis, projection, transform );

					outCoords = tUnitCoordsArray[unit][0];
					for( i = 0, n = normalsArray[0]; i < r_backacc.numVerts; i++, outCoords += 2, n += 4 )
					{
						VectorSubtract( transform, vertsArray[i], projection );
						VectorNormalizeFast( projection );

						depth = DotProduct( n, projection ); depth += depth;
						outCoords[0] = 0.5 + ( n[1] * depth - projection[1] ) * 0.5;
						outCoords[1] = 0.5 - ( n[2] * depth - projection[2] ) * 0.5;
					}
				}

				GL_DisableAllTexGens();
				qglTexCoordPointer( 2, GL_FLOAT, 0, tUnitCoordsArray[unit] );
			}
			return qtrue;
		}

	case TC_GEN_VECTOR:
		{
			GLfloat genVector[2][4];

			for( i = 0; i < 3; i++ )
			{
				genVector[0][i] = pass->tcgenVec[i];
				genVector[1][i] = pass->tcgenVec[i+4];
			}
			genVector[0][3] = genVector[1][3] = 0;

			matrix[12] = pass->tcgenVec[3];
			matrix[13] = pass->tcgenVec[7];

			if( programFeatures )
				*programFeatures |= GLSL_COMMON_APPLY_TC_GEN_VECTOR;

			GL_SetTexCoordArrayMode( 0 );

			GL_EnableTexGen( GL_S, GL_OBJECT_LINEAR );
			GL_EnableTexGen( GL_T, GL_OBJECT_LINEAR );
			GL_EnableTexGen( GL_R, 0 );
			GL_EnableTexGen( GL_Q, 0 );

			qglTexGenfv( GL_S, GL_OBJECT_PLANE, genVector[0] );
			qglTexGenfv( GL_T, GL_OBJECT_PLANE, genVector[1] );
			return qfalse;
		}
	case TC_GEN_PROJECTION:
		{
			mat4x4_t m1, m2;
			GLfloat genVector[4][4];

			Matrix4_Copy( ri.worldviewProjectionMatrix, matrix );

			Matrix4_Identity( m1 );
			Matrix4_Scale( m1, 0.5, 0.5, 0.5 );
			Matrix4_Multiply( m1, matrix, m2 );

			Matrix4_Identity( m1 );
			Matrix4_Translate( m1, 0.5, 0.5, 0.5 );
			Matrix4_Multiply( m1, m2, matrix );

			for( i = 0; i < 4; i++ )
			{
				genVector[0][i] = i == 0 ? 1 : 0;
				genVector[1][i] = i == 1 ? 1 : 0;
				genVector[2][i] = i == 2 ? 1 : 0;
				genVector[3][i] = i == 3 ? 1 : 0;
			}

			if( programFeatures )
				*programFeatures |= GLSL_COMMON_APPLY_TC_GEN_VECTOR;

			GL_SetTexCoordArrayMode( 0 );

			GL_EnableTexGen( GL_S, GL_OBJECT_LINEAR );
			GL_EnableTexGen( GL_T, GL_OBJECT_LINEAR );
			GL_EnableTexGen( GL_R, GL_OBJECT_LINEAR );
			GL_EnableTexGen( GL_Q, GL_OBJECT_LINEAR );

			qglTexGenfv( GL_S, GL_OBJECT_PLANE, genVector[0] );
			qglTexGenfv( GL_T, GL_OBJECT_PLANE, genVector[1] );
			qglTexGenfv( GL_R, GL_OBJECT_PLANE, genVector[2] );
			qglTexGenfv( GL_Q, GL_OBJECT_PLANE, genVector[3] );
			return qfalse;
		}

	case TC_GEN_REFLECTION_CELLSHADE:
		R_VertexTCCellshadeMatrix( matrix );

	case TC_GEN_REFLECTION:
		if( programFeatures )
			*programFeatures |= GLSL_COMMON_APPLY_TC_GEN_REFLECTION;

		GL_EnableTexGen( GL_S, GL_REFLECTION_MAP_ARB );
		GL_EnableTexGen( GL_T, GL_REFLECTION_MAP_ARB );
		GL_EnableTexGen( GL_R, GL_REFLECTION_MAP_ARB );
		GL_EnableTexGen( GL_Q, 0 );
		return qtrue;

	case TC_GEN_FOG:
		{
			int fogPtype;
			vec3_t fogNormal, vpnNormal;
			vec_t dist, vdist, fogDist, vpnDist;

			GL_DisableAllTexGens();

			if( programFeatures )
			{
				GL_SetTexCoordArrayMode( 0 );
				*programFeatures |= GLSL_Q3A_APPLY_TC_GEN_FOG;
				return qtrue;
			}

			matrix[0] = matrix[5] = 1.0/(r_back.texFog->shader->fog_dist - r_back.texFog->shader->fog_clearDist);
			matrix[13] = 1.5f/(float)FOG_TEXTURE_HEIGHT;

			// distance to fog
			dist = ri.fog_dist_to_eye[r_back.texFog-r_worldbrushmodel->fogs];

			R_TransformFogPlanes( r_back.texFog, fogNormal, &fogDist, vpnNormal, &vpnDist );

			fogPtype = ( fogNormal[0] == 1.0 ? PLANE_X : ( fogNormal[1] == 1.0 ? PLANE_Y : ( fogNormal[2] == 1.0 ? PLANE_Z : PLANE_NONAXIAL ) ) );

			outCoords = tUnitCoordsArray[unit][0];
			if( r_back.texFog == ri.fog_eye )
			{
				// camera is inside the fog brush
				for( i = 0; i < r_backacc.numVerts; i++, outCoords += 2 )
				{
					outCoords[0] = DotProduct( vertsArray[i], vpnNormal ) - vpnDist;
					if( fogPtype < 3 )
						outCoords[1] = -( vertsArray[i][fogPtype] - fogDist );
					else
						outCoords[1] = -( DotProduct( vertsArray[i], fogNormal ) - fogDist );
				}
			}
			else
			{
				for( i = 0; i < r_backacc.numVerts; i++, outCoords += 2 )
				{
					if( fogPtype < 3 )
						vdist = vertsArray[i][fogPtype] - fogDist;
					else
						vdist = DotProduct( vertsArray[i], fogNormal ) - fogDist;
					outCoords[0] = ( ( vdist < 0 ) ? ( DotProduct( vertsArray[i], vpnNormal ) - vpnDist ) * vdist / ( vdist - dist ) : 0.0f );
					outCoords[1] = -vdist;
				}
			}

			qglTexCoordPointer( 2, GL_FLOAT, 0, tUnitCoordsArray[unit] );
			return qfalse;
		}

	case TC_GEN_SVECTORS:
		GL_DisableAllTexGens();
		qglTexCoordPointer( 4, GL_FLOAT, 0, r_bufferBound ? ( const GLvoid * )r_back.currentMeshVBO->sVectorsOffset : sVectorsArray );
		return qtrue;

	case TC_GEN_PROJECTION_SHADOW:
		GL_SetTexCoordArrayMode( 0 );
		GL_DisableAllTexGens();
		Matrix4_Multiply( r_back.currentCastGroup->worldviewProjectionMatrix, ri.objectMatrix, matrix );
		break;

	case TC_GEN_NONE:
		GL_SetTexCoordArrayMode( 0 );
		GL_DisableAllTexGens();
		return qtrue;

	case TC_GEN_DRAWFLAT:
#if 0
		// although this does work, I'm still unsure I really want this enabled
		// since I'd also have to cover non-identity matrix in GLSL too
		if( ri.currententity ) {
			entity_t *e = ri.currententity;
			matrix[0] = e->axis[0][0];
			matrix[1] = e->axis[0][1];
			matrix[2] = e->axis[0][2];
			matrix[4] = e->axis[1][0];
			matrix[5] = e->axis[1][1];
			matrix[6] = e->axis[1][2];
			matrix[8] = e->axis[2][0];
			matrix[9] = e->axis[2][1];
			matrix[10] = e->axis[2][2];
		}
#endif
		matrix[0] *= 0.5;
		matrix[1] *= 0.5;
		matrix[2] *= 0.5;
		matrix[4] *= 0.5;
		matrix[5] *= 0.5;
		matrix[6] *= 0.5;
		matrix[8] *= 0.5;
		matrix[9] *= 0.5;
		matrix[10] *= 0.5;
		matrix[12] += 0.5;
		matrix[13] += 0.5;
		matrix[14] += 0.5;

		GL_DisableAllTexGens();
		qglTexCoordPointer( 3, GL_FLOAT, 16, r_bufferBound ? ( const GLvoid * )r_back.currentMeshVBO->normalsOffset : normalsArray );
		return qfalse;

	default:
		break;
	}

	return identityMatrix;
}

/*
* R_ApplyTCMods
*/
static void R_ApplyTCMods( const shaderpass_t *pass, mat4x4_t result )
{
	int i;
	const float *table;
	double t1, t2, sint, cost;
	mat4x4_t m1, m2;
	const tcmod_t *tcmod;

	for( i = 0, tcmod = pass->tcmods; i < pass->numtcmods; i++, tcmod++ )
	{
		switch( tcmod->type )
		{
		case TC_MOD_ROTATE:
			cost = tcmod->args[0] * r_back.currentShaderTime;
			sint = R_FastSin( cost );
			cost = R_FastSin( cost + 0.25 );
			m2[0] =  cost, m2[1] = sint, m2[12] =  0.5f * ( sint - cost + 1 );
			m2[4] = -sint, m2[5] = cost, m2[13] = -0.5f * ( sint + cost - 1 );
			Matrix4_Copy2D( result, m1 );
			Matrix4_Multiply2D( m2, m1, result );
			break;
		case TC_MOD_SCALE:
			Matrix4_Scale2D( result, tcmod->args[0], tcmod->args[1] );
			break;
		case TC_MOD_TURB:
			if( pass->program_type != GLSL_PROGRAM_TYPE_TURBULENCE )
			{
				t1 = ( 1.0 / 4.0 );
				t2 = tcmod->args[2] + r_back.currentShaderTime * tcmod->args[3];
				Matrix4_Scale2D( result, 1 + ( tcmod->args[1] * R_FastSin( t2 ) + tcmod->args[0] ) * t1, 1 + ( tcmod->args[1] * R_FastSin( t2 + 0.25 ) + tcmod->args[0] ) * t1 );
			}
			break;
		case TC_MOD_STRETCH:
			table = R_TableForFunc( tcmod->args[0] );
			t2 = tcmod->args[3] + r_back.currentShaderTime * tcmod->args[4];
			t1 = FTABLE_EVALUATE( table, t2 ) * tcmod->args[2] + tcmod->args[1];
			t1 = t1 ? 1.0f / t1 : 1.0f;
			t2 = 0.5f - 0.5f * t1;
			Matrix4_Stretch2D( result, t1, t2 );
			break;
		case TC_MOD_SCROLL:
			t1 = tcmod->args[0] * r_back.currentShaderTime;
			t2 = tcmod->args[1] * r_back.currentShaderTime;
			if( pass->program_type != GLSL_PROGRAM_TYPE_DISTORTION )
			{	// HACK HACK HACK
				t1 = t1 - floor( t1 );
				t2 = t2 - floor( t2 );
			}
			Matrix4_Translate2D( result, t1, t2 );
			break;
		case TC_MOD_TRANSFORM:
			m2[0] = tcmod->args[0], m2[1] = tcmod->args[2], m2[12] = tcmod->args[4],
				m2[5] = tcmod->args[1], m2[4] = tcmod->args[3], m2[13] = tcmod->args[5];
			Matrix4_Copy2D( result, m1 );
			Matrix4_Multiply2D( m2, m1, result );
			break;
		default:
			break;
		}
	}
}

/*
* R_ShaderpassTex
*/
static inline image_t *R_ShaderpassTex( const shaderpass_t *pass, int unit )
{
	if( pass->anim_fps )
		return pass->anim_frames[(int)( pass->anim_fps * r_back.currentShaderTime ) % pass->anim_numframes];
	if( pass->flags & SHADERPASS_LIGHTMAP )
		return r_worldbrushmodel->lightmapImages[r_back.superLightStyle->lightmapNum[r_back.lightmapStyleNum[unit]]];
	if( pass->flags & SHADERPASS_PORTALMAP )
		return r_portaltextures[0] ? r_portaltextures[0] : r_blacktexture;
	return ( pass->anim_frames[0] ? pass->anim_frames[0] : r_notexture );
}

/*
* R_BindShaderpass
*/
void R_BindShaderpass( const shaderpass_t *pass, image_t *tex, int unit, r_glslfeat_t *programFeatures )
{
	mat4x4_t m1, m2, result;
	qboolean identityMatrix;

	if( !tex )
		tex = R_ShaderpassTex( pass, unit );

	GL_Bind( unit, tex );
	if( !pass->program_type ) {
		if( tex->depth > 1 ) {
			qglDisable( GL_TEXTURE_2D );
			qglEnable( GL_TEXTURE_3D );
		}
		else {
			qglEnable( GL_TEXTURE_2D );
			qglDisable( GL_TEXTURE_3D );
		}
	}

	GL_SetTexCoordArrayMode( ( tex->flags & IT_CUBEMAP ? GL_TEXTURE_CUBE_MAP_ARB : GL_TEXTURE_COORD_ARRAY ) );

	identityMatrix = R_VertexTCBase( pass, unit, result, programFeatures );

	if( pass->numtcmods )
	{
		identityMatrix = qfalse;
		R_ApplyTCMods( pass, result );
	}

	if( pass->tcgen == TC_GEN_REFLECTION || pass->tcgen == TC_GEN_REFLECTION_CELLSHADE )
	{
		// in GLSL these calculations are done in world-space so we don't
		// need to apply inverse modelview transform here.. alternatively, we
		// could multiply this matrix by gl_ModelViewMatrixInverseTranspose in the vertex
		// shader
		if( !programFeatures )
		{
			Matrix4_Transpose( ri.modelviewMatrix, m1 );
			Matrix4_Copy( result, m2 );
			Matrix4_Multiply( m2, m1, result );
		}
		GL_LoadTexMatrix( result );
		return;
	}

	if( identityMatrix )
		GL_LoadIdentityTexMatrix();
	else
		GL_LoadTexMatrix( result );
}

/*
* R_RGBGenVertexLightstyled
*/
static void R_RGBGenVertexLightstyled( qbyte *out )
{
	unsigned int i, j;
	float *tc;
	qbyte *inArray;
	vec3_t style, temp[MAX_ARRAY_VERTS];

	memset( temp, 0, sizeof( vec3_t ) * r_backacc.numColors );

	for( j = 0; j < MAX_LIGHTMAPS && r_back.superLightStyle->vertexStyles[j] != 255; j++ )
	{
		VectorCopy( r_lightStyles[r_back.superLightStyle->vertexStyles[j]].rgb, style );
		if( VectorCompare( style, vec3_origin ) )
			continue;

		inArray = inColorsArray[j][0];
		for( i = 0, tc = temp[0]; i < r_backacc.numColors; i++, tc += 3, inArray += 4 )
		{
			tc[0] += ( inArray[0] >> r_back.overBrightBits ) * style[0];
			tc[1] += ( inArray[1] >> r_back.overBrightBits ) * style[1];
			tc[2] += ( inArray[2] >> r_back.overBrightBits ) * style[2];
		}
	}

	for( i = 0, tc = temp[0]; i < r_backacc.numColors; i++, tc += 3, out += 4 )
	{
		out[0] = bound( 0, tc[0], 255 );
		out[1] = bound( 0, tc[1], 255 );
		out[2] = bound( 0, tc[2], 255 );
		out[3] = 255;
	}
}

/*
* R_ModifyColor
*/
void R_ModifyColor( const shaderpass_t *pass, qboolean forceAlpha, qboolean firstVertColor )
{
	unsigned int i;
	int c;
	double temp;
	float *table, a;
	vec3_t t, v;
	qbyte *bArray, *inArray, rgba[4] = { 255, 255, 255, 255 };
	qboolean noArray, identityAlpha, entityAlpha, noAlpha;
	const shaderfunc_t *rgbgenfunc, *alphagenfunc;

	if( forceAlpha )
		noAlpha = qfalse;
	else
		noAlpha = !GL_IsAlphaBlending( pass->flags & GLSTATE_SRCBLEND_MASK, pass->flags & GLSTATE_DSTBLEND_MASK );

	noArray = firstVertColor || (( pass->flags & SHADERPASS_NOCOLORARRAY ) && !r_back.colorFog);
	r_backacc.numColors = noArray ? 1 : r_backacc.numVerts;

	bArray = colorArrayCopy[0];
	inArray = inColorsArray[0][0];
	colorArray = colorArrayCopy;

	if( pass->rgbgen.type == RGB_GEN_IDENTITY_LIGHTING )
	{
		entityAlpha = identityAlpha = qfalse;
		memset( bArray, r_back.identityLighting, sizeof( byte_vec4_t ) * r_backacc.numColors );
	}
	else if( pass->rgbgen.type == RGB_GEN_EXACT_VERTEX )
	{
		// if we can switch the array, do that, avoiding loops and memcpy calls
		if( noAlpha && !r_back.colorFog )
		{
			colorArray = inColorsArray[0];
			return;
		}

		entityAlpha = identityAlpha = qfalse;
		memcpy( bArray, inArray, sizeof( byte_vec4_t ) * r_backacc.numColors );
	}
	else if( pass->rgbgen.type == RGB_GEN_VERTEX )
	{
		if( !r_back.superLightStyle
			|| (r_back.superLightStyle->vertexStyles[1] == 255 && VectorCompare( r_lightStyles[r_back.superLightStyle->vertexStyles[0]].rgb, colorWhite ) ) )
		{
			// if we can switch the array, do that, avoiding loops and memcpy calls
			if( noAlpha && !r_back.overBrightBits && !r_back.colorFog )
			{
				colorArray = inColorsArray[0];
				return;
			}

			entityAlpha = qfalse;
			identityAlpha = qtrue;
			for( i = 0; i < r_backacc.numColors; i++, bArray += 4, inArray += 4 )
			{
				bArray[0] = inArray[0] >> r_back.overBrightBits;
				bArray[1] = inArray[1] >> r_back.overBrightBits;
				bArray[2] = inArray[2] >> r_back.overBrightBits;
				bArray[3] = 255;
			}
		}
		else
		{
			entityAlpha = qfalse;
			identityAlpha = qtrue;
			R_RGBGenVertexLightstyled( bArray );
		}
	}
	else
	{
		entityAlpha = qfalse;
		identityAlpha = qtrue;
		memset( bArray, 255, sizeof( byte_vec4_t ) * r_backacc.numColors );

		switch( pass->rgbgen.type )
		{
		case RGB_GEN_IDENTITY:
			break;
		case RGB_GEN_CONST:
			rgba[0] = R_FloatToByte( pass->rgbgen.args[0] );
			rgba[1] = R_FloatToByte( pass->rgbgen.args[1] );
			rgba[2] = R_FloatToByte( pass->rgbgen.args[2] );

			for( i = 0, c = *(int *)rgba; i < r_backacc.numColors; i++, bArray += 4 )
				*(int *)bArray = c;
			break;
		case RGB_GEN_ENTITYWAVE:
			entityAlpha = qtrue;
			identityAlpha = ( r_back.entityColor[3] == 255 );
		case RGB_GEN_WAVE:
		case RGB_GEN_CUSTOMWAVE:
			rgbgenfunc = pass->rgbgen.func;
			if( !rgbgenfunc || rgbgenfunc->type == SHADER_FUNC_NONE )
			{
				temp = 1;
			}
			else if( rgbgenfunc->type == SHADER_FUNC_RAMP )
			{
				temp = R_DistanceRamp( rgbgenfunc->args[2], rgbgenfunc->args[3], rgbgenfunc->args[0], rgbgenfunc->args[1] );
			}
			else if( rgbgenfunc->args[1] == 0 )
			{
				temp = rgbgenfunc->args[0];
			}
			else
			{
				if( rgbgenfunc->type == SHADER_FUNC_NOISE )
				{
					temp = R_BackendGetNoiseValue( 0, 0, 0, ( r_back.currentShaderTime + rgbgenfunc->args[2] ) * rgbgenfunc->args[3] );
				}
				else
				{
					table = R_TableForFunc( rgbgenfunc->type );
					temp = r_back.currentShaderTime * rgbgenfunc->args[3] + rgbgenfunc->args[2];
					temp = FTABLE_EVALUATE( table, temp ) * rgbgenfunc->args[1] + rgbgenfunc->args[0];
				}
				temp = temp * rgbgenfunc->args[1] + rgbgenfunc->args[0];
			}

			if( pass->rgbgen.type == RGB_GEN_ENTITYWAVE )
			{
				VectorSet( v,
					r_back.entityColor[0] * (1.0/255.0),
					r_back.entityColor[1] * (1.0/255.0),
					r_back.entityColor[2] * (1.0/255.0) );
				rgba[3] = r_back.entityColor[3];
			}
			else if( pass->rgbgen.type == RGB_GEN_CUSTOMWAVE )
			{
				c = R_GetCustomColor( (int)pass->rgbgen.args[0] );
				VectorSet( v,
					COLOR_R( c ) * (1.0 / 255.0),
					COLOR_G( c ) * (1.0 / 255.0),
					COLOR_B( c ) * (1.0 / 255.0) );
				VectorScale( v, (1.0 / (1<<r_back.overBrightBits)), v );
			}
			else
			{
				VectorCopy( pass->rgbgen.args, v );
				VectorScale( v, (1.0 / (1<<r_back.overBrightBits)), v );
			}

			a = v[0] * temp; rgba[0] = a <= 0 ? 0 : R_FloatToByte( a );
			a = v[1] * temp; rgba[1] = a <= 0 ? 0 : R_FloatToByte( a );
			a = v[2] * temp; rgba[2] = a <= 0 ? 0 : R_FloatToByte( a );

			for( i = 0, c = *(int *)rgba; i < r_backacc.numColors; i++, bArray += 4 )
				*(int *)bArray = c;
			break;
#ifdef HARDWARE_OUTLINES
		case RGB_GEN_OUTLINE:
			identityAlpha = ( ri.currententity->outlineColor[3] == 255 );
			c = COLOR_RGBA(
				ri.currententity->outlineColor[0] >> r_back.overBrightBits,
				ri.currententity->outlineColor[1] >> r_back.overBrightBits,
				ri.currententity->outlineColor[2] >> r_back.overBrightBits,
				ri.currententity->outlineColor[3] );

			for( i = 0; i < r_backacc.numColors; i++, bArray += 4 )
				*(int *)bArray = c;
			break;
#endif
		case RGB_GEN_ONE_MINUS_ENTITY:
			rgba[0] = 255 - ri.currententity->color[0];
			rgba[1] = 255 - ri.currententity->color[1];
			rgba[2] = 255 - ri.currententity->color[2];

			for( i = 0, c = *(int *)rgba; i < r_backacc.numColors; i++, bArray += 4 )
				*(int *)bArray = c;
			break;
		case RGB_GEN_ONE_MINUS_VERTEX:
			for( i = 0; i < r_backacc.numColors; i++, bArray += 4, inArray += 4 )
			{
				bArray[0] = ( 255 - inArray[0] ) >> r_back.overBrightBits;
				bArray[1] = ( 255 - inArray[1] ) >> r_back.overBrightBits;
				bArray[2] = ( 255 - inArray[2] ) >> r_back.overBrightBits;
			}
			break;
		case RGB_GEN_LIGHTING_DIFFUSE:
			if( ri.currententity )
				R_LightForEntity( ri.currententity, bArray );
			break;
		case RGB_GEN_LIGHTING_DIFFUSE_ONLY:
			if( ri.currententity && !( ri.params & RP_SHADOWMAPVIEW ) )
			{
				vec4_t diffuse;

				if( ri.currententity->flags & RF_FULLBRIGHT )
					VectorSet( diffuse, 1, 1, 1 );
				else
					R_LightForOrigin( ri.currententity->lightingOrigin, t, NULL, diffuse, ri.currentmodel->radius * ri.currententity->scale );

				rgba[0] = R_FloatToByte( diffuse[0] );
				rgba[1] = R_FloatToByte( diffuse[1] );
				rgba[2] = R_FloatToByte( diffuse[2] );

				for( i = 0, c = *(int *)rgba; i < r_backacc.numColors; i++, bArray += 4 )
					*(int *)bArray = c;
			}
			break;
		case RGB_GEN_LIGHTING_AMBIENT_ONLY:
			if( ri.currententity && !( ri.params & RP_SHADOWMAPVIEW ) )
			{
				vec4_t ambient;

				if( ri.currententity->flags & RF_FULLBRIGHT )
					VectorSet( ambient, 1, 1, 1 );
				else
					R_LightForOrigin( ri.currententity->lightingOrigin, t, ambient, NULL, ri.currentmodel->radius * ri.currententity->scale );

				rgba[0] = R_FloatToByte( ambient[0] );
				rgba[1] = R_FloatToByte( ambient[1] );
				rgba[2] = R_FloatToByte( ambient[2] );

				for( i = 0, c = *(int *)rgba; i < r_backacc.numColors; i++, bArray += 4 )
					*(int *)bArray = c;
			}
			break;
		case RGB_GEN_FOG:
			for( i = 0, c = *(int *)r_back.texFog->shader->fog_color; i < r_backacc.numColors; i++, bArray += 4 )
				*(int *)bArray = c;
			break;
		case RGB_GEN_ENVIRONMENT:
			for( i = 0, c = *(int *)mapConfig.environmentColor; i < r_backacc.numColors; i++, bArray += 4 )
				*(int *)bArray = c;
			break;
		default:
			break;
		}
	}

	bArray = colorArrayCopy[0];
	inArray = inColorsArray[0][0];

	if( noAlpha )
		goto done_alpha;

	switch( pass->alphagen.type )
	{
	case ALPHA_GEN_IDENTITY:
		if( identityAlpha )
			break;
		for( i = 0; i < r_backacc.numColors; i++, bArray += 4 )
			bArray[3] = 255;
		break;
	case ALPHA_GEN_CONST:
		c = R_FloatToByte( pass->alphagen.args[0] );
		for( i = 0; i < r_backacc.numColors; i++, bArray += 4 )
			bArray[3] = c;
		break;
	case ALPHA_GEN_WAVE:
		alphagenfunc = pass->alphagen.func;
		if( !alphagenfunc || alphagenfunc->type == SHADER_FUNC_NONE )
		{
			a = 1;
		}
		else if( alphagenfunc->type == SHADER_FUNC_RAMP )
		{
			a = R_DistanceRamp( alphagenfunc->args[2], alphagenfunc->args[3], alphagenfunc->args[0], alphagenfunc->args[1] );
		}
		else
		{
			if( alphagenfunc->type == SHADER_FUNC_NOISE )
			{
				a = R_BackendGetNoiseValue( 0, 0, 0, ( r_back.currentShaderTime + alphagenfunc->args[2] ) * alphagenfunc->args[3] );
			}
			else
			{
				table = R_TableForFunc( alphagenfunc->type );
				a = alphagenfunc->args[2] + r_back.currentShaderTime * alphagenfunc->args[3];
				a = FTABLE_EVALUATE( table, a );
			}

			a = a * alphagenfunc->args[1] + alphagenfunc->args[0];
		}

		c = a <= 0 ? 0 : R_FloatToByte( a );
		for( i = 0; i < r_backacc.numColors; i++, bArray += 4 )
			bArray[3] = c;
		break;
	case ALPHA_GEN_PORTAL:
		a = R_DistanceRamp( 0, pass->alphagen.args[0], 0, 1 );
		c = a <= 0 ? 0 : R_FloatToByte( a );

		for( i = 0; i < r_backacc.numColors; i++, bArray += 4 )
			bArray[3] = c;
		break;
	case ALPHA_GEN_VERTEX:
		for( i = 0; i < r_backacc.numColors; i++, bArray += 4, inArray += 4 )
			bArray[3] = inArray[3];
		break;
	case ALPHA_GEN_ONE_MINUS_VERTEX:
		for( i = 0; i < r_backacc.numColors; i++, bArray += 4, inArray += 4 )
			bArray[3] = 255 - inArray[3];
		break;
	case ALPHA_GEN_ENTITY:
		if( entityAlpha )
			break;
		for( i = 0; i < r_backacc.numColors; i++, bArray += 4 )
			bArray[3] = ri.currententity->color[3];
		break;
#ifdef HARDWARE_OUTLINES
	case ALPHA_GEN_OUTLINE:
		for( i = 0; i < r_backacc.numColors; i++, bArray += 4 )
			bArray[3] = ri.currententity->outlineColor[3];
		break;
#endif
	case ALPHA_GEN_SPECULAR:
		VectorSubtract( ri.viewOrigin, ri.currententity->origin, t );
		if( !Matrix_Compare( ri.currententity->axis, axis_identity ) )
			Matrix_TransformVector( ri.currententity->axis, t, v );
		else
			VectorCopy( t, v );

		for( i = 0; i < r_backacc.numColors; i++, bArray += 4 )
		{
			VectorSubtract( v, vertsArray[i], t );
			c = VectorLength( t );
			a = DotProduct( t, normalsArray[i] ) / max( 0.1, c );
			a = pow( a, pass->alphagen.args[0] );
			bArray[3] = a <= 0 ? 0 : R_FloatToByte( a );
		}
		break;
	case ALPHA_GEN_DOT:
		if( !Matrix_Compare( ri.currententity->axis, axis_identity ) )
			Matrix_TransformVector( ri.currententity->axis, ri.viewAxis[0], v );
		else
			VectorCopy( ri.viewAxis[0], v );

		for( i = 0; i < r_backacc.numColors; i++, bArray += 4 )
		{
			a = DotProduct( v, inNormalsArray[i] ); if( a < 0 ) a = -a;
			bArray[3] = R_FloatToByte( bound( pass->alphagen.args[0], a, pass->alphagen.args[1] ) );
		}
		break;
	case ALPHA_GEN_ONE_MINUS_DOT:
		if( !Matrix_Compare( ri.currententity->axis, axis_identity ) )
			Matrix_TransformVector( ri.currententity->axis, ri.viewAxis[0], v );
		else
			VectorCopy( ri.viewAxis[0], v );

		for( i = 0; i < r_backacc.numColors; i++, bArray += 4 )
		{
			a = DotProduct( v, inNormalsArray[i] ); if( a < 0 ) a = -a; a = 1.0f - a;
			bArray[3] = R_FloatToByte( bound( pass->alphagen.args[0], a, pass->alphagen.args[1] ) );
		}
		break;
	case ALPHA_GEN_PLANAR_SHADOW:
		c = R_FloatToByte( bound( 0.0f, r_shadows_alpha->value, 1.0f ) );
		for( i = 0; i < r_backacc.numColors; i++, bArray += 4 )
			bArray[3] = c;
		break;
	default:
		break;
	}

done_alpha:
	;

	if( r_back.colorFog && !firstVertColor )
	{
		float dist, vdist;
		vec3_t fogNormal, vpnNormal;
		vec_t fogDist, vpnDist;
		float fogShaderDistScale;
		int fogPtype;
		qboolean alphaFog = !noAlpha;

		// distance to fog
		dist = ri.fog_dist_to_eye[r_back.colorFog-r_worldbrushmodel->fogs];

		R_TransformFogPlanes( r_back.colorFog, fogNormal, &fogDist, vpnNormal, &vpnDist );

		fogPtype = ( fogNormal[0] == 1.0 ? PLANE_X : ( fogNormal[1] == 1.0 ? PLANE_Y : ( fogNormal[2] == 1.0 ? PLANE_Z : PLANE_NONAXIAL ) ) );

		fogShaderDistScale = 1.0 / (r_back.colorFog->shader->fog_dist - r_back.colorFog->shader->fog_clearDist);

		vpnNormal[0] *= fogShaderDistScale;
		vpnNormal[1] *= fogShaderDistScale;
		vpnNormal[2] *= fogShaderDistScale;
		vpnDist *= fogShaderDistScale;

		bArray = colorArray[0];
		if( r_back.colorFog == ri.fog_eye )
		{
			// camera is inside the fog
			for( i = 0; i < r_backacc.numColors; i++, bArray += 4 )
			{
				temp = DotProduct( vertsArray[i], vpnNormal ) - vpnDist;
				c = ( 1.0f - bound( 0, temp, 1.0f ) ) * 0xFFFF;

				if( alphaFog )
				{
					bArray[3] = ( bArray[3] * c ) >> 16;
				}
				else
				{
					bArray[0] = ( bArray[0] * c ) >> 16;
					bArray[1] = ( bArray[1] * c ) >> 16;
					bArray[2] = ( bArray[2] * c ) >> 16;
				}
			}
		}
		else
		{
			for( i = 0; i < r_backacc.numColors; i++, bArray += 4 )
			{
				if( fogPtype < 3 )
					vdist = vertsArray[i][fogPtype] - fogDist;
				else
					vdist = DotProduct( vertsArray[i], fogNormal ) - fogDist;

				if( vdist < 0 )
				{
					temp = ( DotProduct( vertsArray[i], vpnNormal ) - vpnDist ) * vdist / ( vdist - dist );
					c = ( 1.0f - bound( 0, temp, 1.0f ) ) * 0xFFFF;

					if( alphaFog )
					{
						bArray[3] = ( bArray[3] * c ) >> 16;
					}
					else
					{
						bArray[0] = ( bArray[0] * c ) >> 16;
						bArray[1] = ( bArray[1] * c ) >> 16;
						bArray[2] = ( bArray[2] * c ) >> 16;
					}
				}
			}
		}
	}
}

/*
* R_ShaderpassBlendmode
*/
static int R_ShaderpassBlendmode( int passFlags )
{
	switch( passFlags & (SHADERPASS_BLEND_REPLACE|SHADERPASS_BLEND_MODULATE|SHADERPASS_BLEND_ADD|SHADERPASS_BLEND_DECAL) )
	{
		case SHADERPASS_BLEND_REPLACE:
			return GL_REPLACE;
		case SHADERPASS_BLEND_MODULATE:
			return GL_MODULATE;
		case SHADERPASS_BLEND_ADD:
			return GL_ADD;
		case SHADERPASS_BLEND_DECAL:
			return GL_DECAL;
	}
	return 0;
}

/*
* R_SetShaderState
*/
static void R_SetShaderState( void )
{
	int state;

	// Face culling
	if( !gl_cull->integer || ( r_features & MF_NOCULL ) )
		GL_Cull( 0 );
	else if( r_back.currentShader->flags & SHADER_CULL_FRONT )
		GL_Cull( GL_FRONT );
	else if( r_back.currentShader->flags & SHADER_CULL_BACK )
		GL_Cull( GL_BACK );
	else
		GL_Cull( 0 );

	state = 0;

	if( r_features & MF_POLYGONOFFSET )
	{
		state |= GLSTATE_OFFSET_FILL;
		GL_PolygonOffset( -1, -2 );
	}
	else if( r_features & MF_POLYGONOFFSET2 )
	{
		state |= GLSTATE_OFFSET_FILL;
		GL_PolygonOffset( 0, 25 );
	}
	else if( ri.params & RP_SHADOWMAPVIEW )
	{
		state |= GLSTATE_OFFSET_FILL;
		GL_PolygonOffset( 1, 4 );
	}

	if( r_back.currentShader->flags & SHADER_NO_DEPTH_TEST )
		state |= GLSTATE_NO_DEPTH_TEST;

	if( r_features & MF_NOCOLORWRITE )
		state |= GLSTATE_NOCOLORWRITE;

	r_back.currentShaderState = state;
}

/*
* R_RenderMeshGeneric
*/
static void R_RenderMeshGeneric( void )
{
	const shaderpass_t *pass = r_back.accumPasses[0];

	R_BindShaderpass( pass, NULL, 0, NULL );
	R_ModifyColor( pass, qfalse, qfalse );

	if( pass->flags & SHADERPASS_BLEND_REPLACE )
		GL_TexEnv( GL_REPLACE );
	else
		GL_TexEnv( GL_MODULATE );
	GL_SetState( r_back.currentShaderState | ( pass->flags & r_back.currentShaderPassMask ) );

	R_FlushArrays();
}

/*
* R_RenderMeshMultitextured
*/
static void R_RenderMeshMultitextured( void )
{
	int i;
	const shaderpass_t *pass = r_back.accumPasses[0];

	R_BindShaderpass( pass, NULL, 0, NULL );
	R_ModifyColor( pass, qfalse, qfalse );

	GL_TexEnv( GL_MODULATE );
	GL_SetState( r_back.currentShaderState | ( pass->flags & r_back.currentShaderPassMask ) | GLSTATE_BLEND_MTEX );

	for( i = 1; i < r_back.numAccumPasses; i++ )
	{
		pass = r_back.accumPasses[i];
		R_BindShaderpass( pass, NULL, i, NULL );
		GL_TexEnv( R_ShaderpassBlendmode( pass->flags ) );
	}

	R_FlushArrays();
}

/*
* R_RenderMeshCombined
*/
static void R_RenderMeshCombined( void )
{
	int i;
	const shaderpass_t *pass = r_back.accumPasses[0];

	R_BindShaderpass( pass, NULL, 0, NULL );
	R_ModifyColor( pass, qfalse, qfalse );

	GL_TexEnv( GL_MODULATE );
	GL_SetState( r_back.currentShaderState | ( pass->flags & r_back.currentShaderPassMask ) | GLSTATE_BLEND_MTEX );

	for( i = 1; i < r_back.numAccumPasses; i++ )
	{
		pass = r_back.accumPasses[i];
		R_BindShaderpass( pass, NULL, i, NULL );

		if( pass->flags & ( SHADERPASS_BLEND_REPLACE|SHADERPASS_BLEND_MODULATE ) )
		{
			GL_TexEnv( GL_MODULATE );
		}
		else if( pass->flags & SHADERPASS_BLEND_ADD )
		{
			// these modes are best set with TexEnv, Combine4 would need much more setup
			GL_TexEnv( GL_ADD );
		}
		else if( pass->flags & SHADERPASS_BLEND_DECAL )
		{
			// mimics Alpha-Blending in upper texture stage, but instead of multiplying the alpha-channel, they're added
			// this way it can be possible to use GL_DECAL in both texture-units, while still looking good
			// normal mutlitexturing would multiply the alpha-channel which looks ugly
			GL_TexEnv( GL_COMBINE_ARB );
			qglTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_INTERPOLATE_ARB );
			qglTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_ADD );

			qglTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE );
			qglTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR );
			qglTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE );
			qglTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA );

			qglTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB );
			qglTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR );
			qglTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_PREVIOUS_ARB );
			qglTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB, GL_SRC_ALPHA );

			qglTexEnvi( GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB, GL_TEXTURE );
			qglTexEnvi( GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB, GL_SRC_ALPHA );
		}
		else
		{
			assert( 0 );
		}
	}

	R_FlushArrays();
}

/*
* R_AccumulateDynamicLightPasses
*/
static void R_AccumulateDynamicLightPasses( void )
{
	shaderpass_t *pass;

	if( !r_back.currentDlightBits )
		return;

	// prefer GLSL dlights, if available and there is more than one dlight touching
	if( r_back.currentShaderHasGLSLPasses )
	{
		if( !r_lighting_maxglsldlights->integer ) {
			return;
		}
		pass = &r_GLSLpasses[BUILTIN_GLSLPASS_DLIGHT];
	}
	else
	{
		// dynamic lights using falloff texture
		pass = &r_dlightsPass;
	}

	R_AccumulatePass( pass, 0 );
	r_back.currentDlightBits = 0;
}

/*
* R_RenderAccumulatedPasses
*/
static void R_RenderAccumulatedPasses( void )
{
	const shaderpass_t *pass = r_back.accumPasses[0];

	if( !r_back.numAccumPasses ) {
		return;
	}

	R_CleanUpTextureUnits( r_back.numAccumPasses );

	if( pass->program_type )
	{
		r_back.numAccumPasses = 0;
		R_RenderMeshGLSLProgrammed( pass->program_type );
		return;
	}
	if( pass->flags & SHADERPASS_DLIGHT )
	{
		r_back.numAccumPasses = 0;
		if( r_back.currentShaderHasGLSLPasses )
		{
			R_AccumulateDynamicLightPasses();
		}
		else
		{
			R_AddDynamicLights( r_back.currentDlightBits, r_back.currentShaderState | ( pass->flags & r_back.currentShaderPassMask ) );
		}
		return;
	}
	if( r_features & MF_HARDWARE )
	{
		if( glConfig.ext.GLSL )
		{
			r_back.numAccumPasses = 0;
			R_RenderMeshGLSLProgrammed( GLSL_PROGRAM_TYPE_Q3A_SHADER );
			return;
		}
	}
	if( pass->flags & SHADERPASS_STENCILSHADOW )
	{
		r_back.numAccumPasses = 0;
		R_PlanarShadowPass( r_back.currentShaderState | ( pass->flags & r_back.currentShaderPassMask ) );
		return;
	}

	if( r_back.numAccumPasses == 1 )
		R_RenderMeshGeneric();
	else if( glConfig.ext.texture_env_combine )
		R_RenderMeshCombined();
	else
		R_RenderMeshMultitextured();

	r_back.numAccumPasses = 0;
}

/*
* R_AccumulatePass
*/
static void R_AccumulatePass( shaderpass_t *pass, int lightmapStyle )
{
	qboolean accumulate, renderNow;
	const shaderpass_t *prevPass;

	// for depth texture we render light's view to, ignore passes that do not write into depth buffer
	if( ( ri.params & RP_SHADOWMAPVIEW ) && !( pass->flags & GLSTATE_DEPTHWRITE ) )
		return;

	// see if there are any free texture units
	renderNow = ( pass->flags & ( SHADERPASS_DLIGHT|SHADERPASS_STENCILSHADOW ) ) || pass->program_type || ((r_features & MF_HARDWARE) && glConfig.ext.GLSL);
	accumulate = ( r_back.numAccumPasses < glConfig.maxTextureUnits ) && !renderNow;

	if( accumulate )
	{
		if( !r_back.numAccumPasses )
		{
			r_back.lightmapStyleNum[r_back.numAccumPasses] = lightmapStyle;
			r_back.accumPasses[r_back.numAccumPasses] = pass;
			r_back.numAccumPasses++;
			return;
		}

		// ok, we've got several passes, diff against the previous
		prevPass = r_back.accumPasses[r_back.numAccumPasses-1];

		// see if depthfuncs and colors are good
		if(
			( pass->rgbgen.type != RGB_GEN_IDENTITY ) ||
			( pass->alphagen.type != ALPHA_GEN_IDENTITY ) ||
			( pass->flags & GLSTATE_ALPHAFUNC ) ||
			( ( prevPass->flags ^ pass->flags ) & GLSTATE_DEPTHFUNC_EQ ) ||
			( ( prevPass->flags & GLSTATE_ALPHAFUNC ) && !( pass->flags & GLSTATE_DEPTHFUNC_EQ ) )
			)
			accumulate = qfalse;

		// see if blendmodes are good
		if( accumulate )
		{
			int mode, prevMode;

			mode = R_ShaderpassBlendmode( pass->flags );
			if( mode )
			{
				prevMode = R_ShaderpassBlendmode( prevPass->flags );

				if( glConfig.ext.texture_env_combine )
				{
					if( prevMode == GL_REPLACE )
						accumulate = ( mode == GL_ADD ) ? glConfig.ext.texture_env_add : qtrue;
					else if( prevMode == GL_ADD )
						accumulate = ( mode == GL_ADD ) && glConfig.ext.texture_env_add;
					else if( prevMode == GL_MODULATE )
						accumulate = ( mode == GL_MODULATE || mode == GL_REPLACE );
					else
						accumulate = qfalse;
				}
				else /* if( glConfig.ext.multitexture )*/
				{
					if( prevMode == GL_REPLACE )
						accumulate = ( mode == GL_ADD ) ? glConfig.ext.texture_env_add : ( mode != GL_DECAL );
					else if( prevMode == GL_ADD )
						accumulate = ( mode == GL_ADD ) && glConfig.ext.texture_env_add;
					else if( prevMode == GL_MODULATE )
						accumulate = ( mode == GL_MODULATE || mode == GL_REPLACE );
					else
						accumulate = qfalse;
				}
			}
			else
			{
				accumulate = qfalse;
			}
		}
	}

	// no, failed to accumulate
	if( !accumulate ) {
		R_RenderAccumulatedPasses();
	}

	r_back.lightmapStyleNum[r_back.numAccumPasses] = lightmapStyle;
	r_back.accumPasses[r_back.numAccumPasses] = pass;
	r_back.numAccumPasses++;

	if( renderNow )
		R_RenderAccumulatedPasses();
}

/*
* R_SetupLightmapMode
*/
static void R_SetupLightmapMode( void )
{
	r_lightmapPasses[0].tcgen = TC_GEN_LIGHTMAP;
	r_lightmapPasses[0].rgbgen.type = RGB_GEN_CONST;
	r_lightmapPasses[0].alphagen.type = ALPHA_GEN_IDENTITY;
	r_lightmapPasses[0].flags &= ~( SHADERPASS_BLENDMODE|SHADERPASS_DELUXEMAP|GLSTATE_ALPHAFUNC|GLSTATE_SRCBLEND_MASK|GLSTATE_DSTBLEND_MASK|GLSTATE_DEPTHFUNC_EQ );
	r_lightmapPasses[0].flags |= SHADERPASS_LIGHTMAP|SHADERPASS_NOCOLORARRAY|SHADERPASS_BLEND_MODULATE /*|GLSTATE_SRCBLEND_ONE|GLSTATE_DSTBLEND_ZERO*/;
	r_lightmapPasses[0].flags |= GLSTATE_DEPTHWRITE;
	VectorSet( r_lightmapPasses[0].rgbgen.args, 1, 1, 1 );
}

/*
* R_PushElems
*/
static void R_PushElems( elem_t *elems, int count, int features )
{
	// this is a fast path for non-batched geometry, use carefully
	// used on pics, sprites, .dpm, .md3 and .md2 models
	if( features & MF_NONBATCHED )
	{
		// simply change elemsArray to point at elems
		r_backacc.numElems = count;
		elemsArray = elems;
	}
	else
	{
		elem_t *currentElem = elemsArray + r_backacc.numElems;
		r_backacc.numElems += count;

		// the following code assumes that R_PushElems is fed with triangles...
		for(; count > 0; count -= 3, elems += 3, currentElem += 3 )
		{
			currentElem[0] = r_backacc.numVerts + elems[0];
			currentElem[1] = r_backacc.numVerts + elems[1];
			currentElem[2] = r_backacc.numVerts + elems[2];
		}
	}
}

/*
* R_PushTrifanElems
*/
static void R_PushTrifanElems( int numverts )
{
	int count;
	elem_t *currentElem;

	currentElem = elemsArray + r_backacc.numElems;
	r_backacc.numElems += numverts + numverts + numverts - 6;

	for( count = 2; count < numverts; count++, currentElem += 3 )
	{
		currentElem[0] = r_backacc.numVerts;
		currentElem[1] = r_backacc.numVerts + count - 1;
		currentElem[2] = r_backacc.numVerts + count;
	}
}

/*
* R_PushQuadElems
*/
static void R_PushQuadElems( int numverts )
{
	int i, k;
	elem_t *currentElem;

	currentElem = elemsArray + r_backacc.numElems;
	r_backacc.numElems += numverts * 3 / 2;

	for( i = 0, k = r_backacc.numVerts; i < numverts; i += 4, k += 4, currentElem += 6 )
	{
		currentElem[0] = k;
		currentElem[1] = k + 2 - 1;
		currentElem[2] = k + 2;

		currentElem[3] = k;
		currentElem[4] = k + 3 - 1;
		currentElem[5] = k + 3;
	}
}

/*
* R_PushMesh
*/
void R_PushMesh( const mesh_t *mesh, qboolean vbo, int features )
{
	int numverts;

	if( !( mesh->elems || ( features & (MF_TRIFAN|MF_QUAD) ) ) || !mesh->xyzArray )
		return;

	if( vbo )
		features |= MF_HARDWARE|MF_NONBATCHED;
	else
		features &= ~MF_HARDWARE;
	r_features = features;

	if( features & MF_TRIFAN )
		R_PushTrifanElems( mesh->numVerts );
	else if( features & MF_QUAD )
		R_PushQuadElems( mesh->numVerts );
	else
		R_PushElems( mesh->elems, mesh->numElems, features );

	numverts = mesh->numVerts;

	if( features & MF_NONBATCHED )
	{
		if( (features & (MF_DEFORMVS|MF_HARDWARE)) == MF_DEFORMVS )
		{
			if( mesh->xyzArray != inVertsArray )
				R_BackendMemcpy( inVertsArray, mesh->xyzArray, numverts * sizeof( vec4_t ) );

			if( ( features & MF_NORMALS ) && mesh->normalsArray && ( mesh->normalsArray != inNormalsArray ) )
				R_BackendMemcpy( inNormalsArray, mesh->normalsArray, numverts * sizeof( vec4_t ) );
		}
		else
		{
			vertsArray = mesh->xyzArray;

			if( ( features & MF_NORMALS ) && mesh->normalsArray )
				normalsArray = mesh->normalsArray;
		}

		if( ( features & MF_STCOORDS ) && mesh->stArray )
			coordsArray = mesh->stArray;

		if( ( features & MF_LMCOORDS ) && mesh->lmstArray[0] )
		{
			lightmapCoordsArray[0] = mesh->lmstArray[0];
			if( features & MF_LMCOORDS1 )
			{
				lightmapCoordsArray[1] = mesh->lmstArray[1];
				if( features & MF_LMCOORDS2 )
				{
					lightmapCoordsArray[2] = mesh->lmstArray[2];
					if( features & MF_LMCOORDS3 )
						lightmapCoordsArray[3] = mesh->lmstArray[3];
				}
			}
		}

		if( ( features & MF_SVECTORS ) && mesh->sVectorsArray )
			sVectorsArray = mesh->sVectorsArray;
	}
	else
	{
		if( mesh->xyzArray != inVertsArray )
			R_BackendMemcpy( inVertsArray[r_backacc.numVerts], mesh->xyzArray, numverts * sizeof( vec4_t ) );

		if( ( features & MF_NORMALS ) && mesh->normalsArray && (mesh->normalsArray != inNormalsArray ) )
			R_BackendMemcpy( inNormalsArray[r_backacc.numVerts], mesh->normalsArray, numverts * sizeof( vec4_t ) );

		if( ( features & MF_STCOORDS ) && mesh->stArray && (mesh->stArray != inCoordsArray ) )
			R_BackendMemcpy( inCoordsArray[r_backacc.numVerts], mesh->stArray, numverts * sizeof( vec2_t ) );

		if( ( features & MF_LMCOORDS ) && mesh->lmstArray[0] )
		{
			R_BackendMemcpy( inLightmapCoordsArray[0][r_backacc.numVerts], mesh->lmstArray[0], numverts * sizeof( vec2_t ) );
			if( features & MF_LMCOORDS1 )
			{
				R_BackendMemcpy( inLightmapCoordsArray[1][r_backacc.numVerts], mesh->lmstArray[1], numverts * sizeof( vec2_t ) );
				if( features & MF_LMCOORDS2 )
				{
					R_BackendMemcpy( inLightmapCoordsArray[2][r_backacc.numVerts], mesh->lmstArray[2], numverts * sizeof( vec2_t ) );
					if( features & MF_LMCOORDS3 )
						R_BackendMemcpy( inLightmapCoordsArray[3][r_backacc.numVerts], mesh->lmstArray[3], numverts * sizeof( vec2_t ) );
				}
			}
		}

		if( ( features & MF_SVECTORS ) && mesh->sVectorsArray && (mesh->sVectorsArray != inSVectorsArray ) )
			R_BackendMemcpy( inSVectorsArray[r_backacc.numVerts], mesh->sVectorsArray, numverts * sizeof( vec4_t ) );
	}

	if( ( features & MF_COLORS ) && mesh->colorsArray[0] )
	{
		if( features & MF_HARDWARE )
		{
			colorArray = mesh->colorsArray[0];
		}
		else
		{
			R_BackendMemcpy( inColorsArray[0][r_backacc.numVerts], mesh->colorsArray[0], numverts * sizeof( byte_vec4_t ) );
			if( features & MF_COLORS1 )
			{
				R_BackendMemcpy( inColorsArray[1][r_backacc.numVerts], mesh->colorsArray[1], numverts * sizeof( byte_vec4_t ) );
				if( features & MF_COLORS2 )
				{
					R_BackendMemcpy( inColorsArray[2][r_backacc.numVerts], mesh->colorsArray[2], numverts * sizeof( byte_vec4_t ) );
					if( features & MF_COLORS3 )
						R_BackendMemcpy( inColorsArray[3][r_backacc.numVerts], mesh->colorsArray[3], numverts * sizeof( byte_vec4_t ) );
				}
			}
		}
	}

	r_backacc.numVerts += numverts;
}

/*
* R_RenderMeshBuffer
*/
void R_RenderMeshBuffer( const meshbuffer_t *mb, const rbackAnimData_t *animData )
{
	int i;
	shaderpass_t *pass;
	mfog_t *fog;
#ifdef HARDWARE_OUTLINES
	qboolean addGLSLOutline = qfalse;
#endif
	qboolean drawFlat = qfalse;
	qboolean rawLightmap = qfalse;

	if( !r_backacc.numVerts || !r_backacc.numElems )
	{
		R_ClearArrays();
		return;
	}

	if( r_features & MF_NOCOLORWRITE )
	{
		r_back.currentShader = R_OcclusionShader();

		fog = NULL;
		r_back.texFog = r_back.colorFog = NULL;
		r_back.currentDlightBits = r_back.currentShadowBits = 0;
		r_back.superLightStyle = NULL;
		r_back.doDynamicLightsPass = qfalse;
	}
	else if( glState.in2DMode )
	{
		MB_NUM2SHADER( mb->shaderkey, r_back.currentShader );

		fog = NULL;
		r_back.texFog = r_back.colorFog = NULL;
		r_back.currentDlightBits = r_back.currentShadowBits = 0;
		r_back.superLightStyle = NULL;
		r_back.doDynamicLightsPass = qfalse;
	}
	else
	{
		MB_NUM2SHADER( mb->shaderkey, r_back.currentShader );

		// extract the fog volume number from sortkey
		MB_NUM2FOG( mb->sortkey, fog );

		if( fog && fog->shader )
		{
			// should we fog the geometry with alpha texture or scale colors?
			if( Shader_UseTextureFog( r_back.currentShader ) )
			{
				r_back.texFog = fog;
				r_back.colorFog = NULL;
			}
			else
			{
				// use scaling of colors
				r_back.texFog = NULL;
				r_back.colorFog = fog;
			}
		}
		else
		{
			fog = NULL;
			r_back.texFog = r_back.colorFog = NULL;
		}

		r_back.currentDlightBits = mb->infokey > 0 ? mb->dlightbits : 0;
		r_back.currentShadowBits = mb->shadowbits & ri.shadowBits;
		if( r_worldbrushmodel ) {
			r_back.superLightStyle = &r_worldbrushmodel->superLightStyles[((mb->sortkey >> 10) - 1) & (MAX_SUPER_STYLES-1)];
		}
		else {
			r_worldbrushmodel = NULL;
		}

		// whether we should special dynamic lighting pass(es). some GLSL passes
		// can do dlights on their own and reset this
		r_back.doDynamicLightsPass = !( r_back.currentShader->flags & SHADER_NO_MODULATIVE_DLIGHTS );
	}

	assert( r_back.currentShader != NULL );

	r_back.currentShaderHasGLSLPasses = qfalse;
	r_back.lightingPassDone = qfalse;

	r_back.currentMeshBuffer = mb;
	r_back.currentMeshVBO = R_VBOByIndex( mb->vboIndex );

	if( r_features & MF_BONES ) {
		r_back.currentAnimData = animData && animData->numBones ? animData : NULL;
	}
	else {
		r_back.currentAnimData = NULL;
	}

	if( glState.in2DMode )
	{
		r_back.currentShaderTime = Sys_Milliseconds() * 0.001;
	}
	else if( ri.currententity && ri.currententity != r_worldent )
	{
		Vector4Copy( ri.currententity->shaderRGBA, r_back.entityColor );
		if( ri.currententity->shaderTime > ri.refdef.time )
			r_back.currentShaderTime = 0;
		else
			r_back.currentShaderTime = (ri.refdef.time - ri.currententity->shaderTime) * 0.001;
	}
	else
	{
		Vector4Set( r_back.entityColor, 1, 1, 1, 1 );
		r_back.currentShaderTime = ri.refdef.time * 0.001;
	}

	// check whether this shader/mesh requires an additional drawflat pass
	drawFlat = ( ri.params & RP_DRAWFLAT ) 
		&& ( r_back.currentMeshBuffer->infokey > 0 ) 
		&& !( r_back.currentShader->flags & SHADER_NODRAWFLAT )
		&& r_drawflattexture != NULL;
	rawLightmap = drawFlat || ( ri.params & RP_LIGHTMAP );

	if( !r_triangleOutlines )
		R_SetShaderState();

	// copy the first triangle verts before doing deformvs
	if( r_back.currentShader->flags & (SHADER_PORTAL_CAPTURE|SHADER_PORTAL_CAPTURE2) )
	{
		for( i = 0; i < 3; i++ )
			VectorCopy( vertsArray[elemsArray[i]], r_back.r_triangle0Copy[i] );
	}

	if( r_features & MF_DEFORMVS ) {
		R_DeformVertices();
	}

	R_UnlockArrays();

	if( r_triangleOutlines )
	{
		R_LockArrays( r_backacc.numVerts );

		if( ri.params & RP_TRISOUTLINES ) {
			R_DrawTriangles();
		}
		if( ri.params & RP_SHOWNORMALS ) {
			R_DrawNormals();
		}

		R_ClearArrays();

		qglMatrixMode( GL_MODELVIEW );

		return;
	}

#ifdef HARDWARE_OUTLINES
	if( glConfig.ext.GLSL && ri.currententity && ENTITY_OUTLINE( ri.currententity ) && !(ri.params & RP_CLIPPLANE)
		&& ( r_back.currentShader->sort == SHADER_SORT_OPAQUE ) && ( r_back.currentShader->flags & SHADER_CULL_FRONT )
		&& !( ri.params & RP_SHADOWMAPVIEW ) )
	{
		addGLSLOutline = qtrue;
		r_features |= MF_ENABLENORMALS;
	}
#endif

	R_LockArrays( r_backacc.numVerts );

	// accumulate passes for dynamic merging
	for( i = 0, pass = r_back.currentShader->passes; i < r_back.currentShader->numpasses; i++, pass++ )
	{
		if( !pass->program_type )
		{
			if( pass->flags & SHADERPASS_LIGHTMAP )
			{
				int j, k, l, u;

				// no valid lightmaps, goodbye
				if( !r_back.superLightStyle || r_back.superLightStyle->lightmapNum[0] < 0 || r_back.superLightStyle->lightmapStyles[0] == 255 )
					continue;

				// try to apply lightstyles
				if( ( !( pass->flags & ( GLSTATE_SRCBLEND_MASK|GLSTATE_DSTBLEND_MASK ) ) || ( pass->flags & SHADERPASS_BLEND_MODULATE ) ) && ( pass->rgbgen.type == RGB_GEN_IDENTITY ) && ( pass->alphagen.type == ALPHA_GEN_IDENTITY ) )
				{
					// the most common case
					if( r_back.superLightStyle->lightmapStyles[1] == 255
						&& VectorCompare( r_lightStyles[r_back.superLightStyle->lightmapStyles[0]].rgb, colorWhite )
						&& !mapConfig.lightingIntensity
						&& !rawLightmap )
					{
						R_AccumulatePass( pass, 0 );
					}
					else
					{
						vec3_t colorSum, color;

						// the first pass is always GL_MODULATE or GL_REPLACE
						// other passes are GL_ADD
						r_lightmapPasses[0] = *pass;
						r_lightmapPasses[0].rgbgen.args = r_lightmapPassesArgs[0];

						for( j = 0, l = 0, u = 0; j < MAX_LIGHTMAPS && r_back.superLightStyle->lightmapStyles[j] != 255; j++ )
						{
							VectorCopy( r_lightStyles[r_back.superLightStyle->lightmapStyles[j]].rgb, colorSum );
							if( mapConfig.lightingIntensity )
								VectorScale( colorSum, mapConfig.lightingIntensity, colorSum );
							VectorClear( color );

							for( ; ; l++ )
							{
								for( k = 0; k < 3; k++ )
								{
									colorSum[k] -= color[k];
									color[k] = bound( 0, colorSum[k], 1 );
								}

								if( l )
								{
									if( !color[0] && !color[1] && !color[2] )
										break;
									if( l == MAX_TEXTURE_UNITS+1 )
										r_lightmapPasses[0] = r_lightmapPasses[1];
									u = l % ( MAX_TEXTURE_UNITS+1 );
								}

								// this doesn't seem to work for some reason
								if( VectorCompare( color, colorWhite ) )
								{
									r_lightmapPasses[u].rgbgen.type = RGB_GEN_IDENTITY;
								}
								else
								{
									if( !l )
										r_lightmapPasses[0].flags = (r_lightmapPasses[0].flags & ~SHADERPASS_BLENDMODE) | SHADERPASS_BLEND_MODULATE;
									r_lightmapPasses[u].rgbgen.type = RGB_GEN_CONST;
									r_lightmapPasses[u].flags |= SHADERPASS_NOCOLORARRAY;
									VectorCopy( color, r_lightmapPasses[u].rgbgen.args );
								}
								r_lightmapPasses[u].flags |= SHADERPASS_NOCOLORARRAY;

								if( rawLightmap && !l )
									R_SetupLightmapMode();
								R_AccumulatePass( &r_lightmapPasses[u], j );
								r_back.lightingPassDone = qtrue;
							}
						}
					}
				}
				else
				{
					if( rawLightmap )
					{
						R_SetupLightmapMode();
						pass = r_lightmapPasses;
					}
					R_AccumulatePass( pass, 0 );
					r_back.lightingPassDone = qtrue;
				}
				continue;
			}
			else if( rawLightmap && ( r_back.currentShader->flags & SHADER_LIGHTMAP ) )
				continue;
			if( ( pass->flags & SHADERPASS_DETAIL ) && !r_detailtextures->integer )
				continue;
			if( ( pass->flags & SHADERPASS_DLIGHT ) && !r_back.currentDlightBits )
				continue;
		}

		R_AccumulatePass( pass, 0 );
	}

	// r_drawflat mode (different coloring for walls and ceiling/floors)
	if( drawFlat
		&& r_back.lightingPassDone
		&& !r_back.currentShaderHasGLSLPasses ) {
		r_drawFlatPass.anim_frames[0] = r_drawflattexture;
		R_AccumulatePass( &r_drawFlatPass, 0 );
	}

	// accumulate dynamic lights passes
	if( r_back.currentDlightBits && r_back.doDynamicLightsPass ) {
		if( !(ri.params & RP_LIGHTMAP) || !( r_back.currentShader->flags & SHADER_LIGHTMAP ) ) {
			R_AccumulateDynamicLightPasses();
		}
	}

	// shadow map
	if( r_back.currentShadowBits && ( r_back.currentShader->sort >= SHADER_SORT_OPAQUE )
		&& ( r_back.currentShader->sort <= SHADER_SORT_ALPHATEST ) )
		R_AccumulatePass( &r_GLSLpasses[BUILTIN_GLSLPASS_SHADOWMAP], 0 );

	// outlines
#ifdef HARDWARE_OUTLINES
	if( addGLSLOutline )
		R_AccumulatePass( &r_GLSLpasses[BUILTIN_GLSLPASS_OUTLINE], 0 );
#endif

	// fog
	if( r_back.texFog && r_back.texFog->shader )
	{
		r_fogPass.anim_frames[0] = r_fogtexture;
		if( !r_back.currentShader->numpasses || r_back.currentShader->fog_dist || ( r_back.currentShader->flags & SHADER_SKY ) )
			r_fogPass.flags &= ~GLSTATE_DEPTHFUNC_EQ;
		else
			r_fogPass.flags |= GLSTATE_DEPTHFUNC_EQ;
		R_AccumulatePass( &r_fogPass, 0 );
	}

	// render accumulated passes
	R_RenderAccumulatedPasses();

	R_ClearArrays();

	qglMatrixMode( GL_MODELVIEW );
}

/*
* R_BackendCleanUpTextureUnits
*/
void R_BackendCleanUpTextureUnits( void )
{
	R_CleanUpTextureUnits( 1 );

	GL_LoadIdentityTexMatrix();
	qglMatrixMode( GL_MODELVIEW );

	GL_DisableAllTexGens();
	GL_SetTexCoordArrayMode( 0 );

	qglEnable( GL_TEXTURE_2D );
	qglDisable( GL_TEXTURE_3D );
}

/*
* R_BackendSetPassMask
*/
void R_BackendSetPassMask( int mask )
{
	r_back.currentShaderPassMask = mask;
}

/*
* R_BackendResetPassMask
*/
void R_BackendResetPassMask( void )
{
	r_back.currentShaderPassMask = GLSTATE_MASK;
}

/*
* R_BackendBeginTriangleOutlines
*
* This is mostly pre-GLSL stuff
*/
void R_BackendBeginTriangleOutlines( void )
{
	r_triangleOutlines = qtrue;
	qglColor4fv( colorWhite );

	GL_Cull( 0 );
	GL_SetState( GLSTATE_NO_DEPTH_TEST );
	qglDisable( GL_TEXTURE_2D );
	qglDisable( GL_TEXTURE_3D );
	qglPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
}

/*
* R_BackendEndTriangleOutlines
*/
void R_BackendEndTriangleOutlines( void )
{
	r_triangleOutlines = qfalse;
	qglColor4fv( colorWhite );
	GL_SetState( 0 );
	qglEnable( GL_TEXTURE_2D );
	qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
}

/*
* R_SetColorForOutlines
*/
static inline void R_SetColorForOutlines( vec4_t outColor )
{
	int type = r_back.currentMeshBuffer->sortkey & 3;

	switch( type )
	{
	case MB_MODEL:
		if( r_back.currentMeshBuffer->infokey < 0 )
			Vector4Copy( colorRed, outColor );
		else
			Vector4Copy( colorWhite, outColor );
		break;
	case MB_SPRITE:
		Vector4Copy( colorBlue, outColor );
		break;
	case MB_POLY:
		Vector4Copy( colorGreen, outColor );
		break;
	}
}

/*
* R_DrawTriangles
*/
static void R_DrawTriangles( void )
{
	shaderpass_t *pass = &r_back.currentShader->passes[0];

	// set some flags
	r_back.currentDlightBits = 0;
	r_back.colorFog = r_back.texFog = NULL;
	r_back.currentShaderState = GLSTATE_NO_DEPTH_TEST;

	// copy and override
	r_triOutlinesPass = *pass;
	r_triOutlinesPass.rgbgen.type = RGB_GEN_CONST;
	r_triOutlinesPass.rgbgen.args = &r_triOutlinesColor[0];
	r_triOutlinesPass.alphagen.type = ALPHA_GEN_CONST;
	r_triOutlinesPass.alphagen.args = &r_triOutlinesColor[3];
	r_triOutlinesPass.flags = SHADERPASS_NOCOLORARRAY;
	r_triOutlinesPass.anim_frames[0] = r_whitetexture;

	// this one is probably the most generic program type we might need
	if( r_triOutlinesPass.program_type != GLSL_PROGRAM_TYPE_NONE ) {
		r_triOutlinesPass.program = DEFAULT_GLSL_Q3A_SHADER_PROGRAM;
		r_triOutlinesPass.program_type = GLSL_PROGRAM_TYPE_Q3A_SHADER;
	}

	// set the actual color value
	if( r_showtris->integer == 2 ) {
		R_SetColorForOutlines( r_triOutlinesColor );
	} else {
		Vector4Copy( colorWhite, r_triOutlinesColor );
	}

	R_AccumulatePass( &r_triOutlinesPass, 0 );
	R_RenderAccumulatedPasses();
}

/*
* R_DrawNormals
*/
static void R_DrawNormals( void )
{
}
