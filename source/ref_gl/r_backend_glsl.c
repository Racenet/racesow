/*
Copyright (C) 2011 Victor Luchits

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

/*
* R_RGBAlphaGenToProgramFeatures
*/
static int R_RGBAlphaGenToProgramFeatures( int rgbgen, int alphagen )
{
	r_glslfeat_t programFeatures;

	programFeatures = 0;

	switch( rgbgen )
	{
		case RGB_GEN_VERTEX:
			if( r_back.overBrightBits )
				programFeatures |= GLSL_COMMON_APPLY_OVERBRIGHT_SCALING;
		case RGB_GEN_EXACT_VERTEX:
			programFeatures |= GLSL_COMMON_APPLY_RGB_GEN_VERTEX;
			break;
		case RGB_GEN_ONE_MINUS_VERTEX:
			if( r_back.overBrightBits )
				programFeatures |= GLSL_COMMON_APPLY_OVERBRIGHT_SCALING;
			programFeatures |= GLSL_COMMON_APPLY_RGB_GEN_ONE_MINUS_VERTEX;
			break;
		default:
			programFeatures |= GLSL_COMMON_APPLY_RGB_GEN_CONST;
			break;
	}

	switch( alphagen )
	{
		case ALPHA_GEN_VERTEX:
			programFeatures |= GLSL_COMMON_APPLY_ALPHA_GEN_VERTEX;
			break;
		case ALPHA_GEN_ONE_MINUS_VERTEX:
			programFeatures |= GLSL_COMMON_APPLY_ALPHA_GEN_ONE_MINUS_VERTEX;
			break;
		default:
			programFeatures |= GLSL_COMMON_APPLY_ALPHA_GEN_CONST;
			break;
	}

	return programFeatures;
}

/*
* R_BonesTransformsToProgramFeatures
*/
static r_glslfeat_t R_BonesTransformsToProgramFeatures( void )
{
	// check whether the current model is actually sketetal
	if( !ri.currentmodel || ri.currentmodel->type != mod_skeletal ) {
		return 0;
	}
	// we can only do hardware skinning in VBO mode
	if( !r_back.currentMeshVBO || !r_back.currentMeshVBO->bonesIndicesOffset || !r_back.currentMeshVBO->bonesWeightsOffset ) {
		return 0;
	}
	// base pose sketetal models aren't animated and rendered as-is
	if( !r_back.currentAnimData ) {
		return 0;
	}
	return r_back.currentAnimData->maxWeights * GLSL_COMMON_APPLY_BONETRANSFORMS1;
}

/*
* R_DlightbitsToProgramFeatures
*/
static r_glslfeat_t R_DlightbitsToProgramFeatures( void )
{
	int numDlights;

	if( !r_back.currentDlightBits ) {
		return 0;
	}
	
	numDlights = Q_bitcount( r_back.currentDlightBits );
	if( r_lighting_maxglsldlights->integer && numDlights > r_lighting_maxglsldlights->integer ) {
		numDlights = r_lighting_maxglsldlights->integer;
	}

	if( numDlights <= 4 ) {
		return GLSL_COMMON_APPLY_DLIGHTS_4;
	}
	if( numDlights <= 8 ) {
		return GLSL_COMMON_APPLY_DLIGHTS_8;
	}
	if( numDlights <= 16 ) {
		return GLSL_COMMON_APPLY_DLIGHTS_16;
	}
	return GLSL_COMMON_APPLY_DLIGHTS_32;
}

/*
* R_RenderMeshGLSL_Material
*/
static void R_RenderMeshGLSL_Material( r_glslfeat_t programFeatures )
{
	int i;
	int tcgen, rgbgen;
	int state;
	int program, object;
	image_t *base, *normalmap, *glossmap, *decalmap, *entdecalmap;
	mat4x4_t unused;
	vec3_t lightDir = { 0.0f, 0.0f, 0.0f };
	vec4_t ambient = { 0.0f, 0.0f, 0.0f, 0.0f }, diffuse = { 0.0f, 0.0f, 0.0f, 0.0f };
	float offsetmappingScale, glossExponent;
	const superLightStyle_t *lightStyle = NULL;
	const mfog_t *fog = r_back.colorFog;
	shaderpass_t *pass = r_back.accumPasses[0];
	qboolean applyDecal;

	// handy pointers
	base = pass->anim_frames[0];
	normalmap = pass->anim_frames[1];
	glossmap = pass->anim_frames[2];
	decalmap = pass->anim_frames[3];
	entdecalmap = pass->anim_frames[4];

	tcgen = pass->tcgen;                // store the original tcgen
	rgbgen = pass->rgbgen.type;			// store the original rgbgen

	assert( normalmap );

	if( normalmap->samples == 4 )
		offsetmappingScale = r_offsetmapping_scale->value * r_back.currentShader->offsetmapping_scale;
	else	// no alpha in normalmap, don't bother with offset mapping
		offsetmappingScale = 0;

	if( r_back.currentShader->gloss_exponent )
		glossExponent = r_back.currentShader->gloss_exponent;
	else
		glossExponent = r_lighting_glossexponent->value;

	applyDecal = decalmap != NULL;

	if( ri.params & RP_CLIPPLANE )
		programFeatures |= GLSL_COMMON_APPLY_CLIPPING;
	if( pass->flags & SHADERPASS_GRAYSCALE )
		programFeatures |= GLSL_COMMON_APPLY_GRAYSCALE;

	if( fog )
	{
		programFeatures |= GLSL_COMMON_APPLY_FOG;
		if( fog != ri.fog_eye )
			programFeatures |= GLSL_COMMON_APPLY_FOG2;
		if( GL_IsAlphaBlending( pass->flags & GLSTATE_SRCBLEND_MASK, pass->flags & GLSTATE_DSTBLEND_MASK ) )
			programFeatures |= GLSL_COMMON_APPLY_COLOR_FOG_ALPHA;
	}

	if( r_back.currentMeshBuffer->infokey > 0 && ( rgbgen != RGB_GEN_LIGHTING_DIFFUSE ) )
	{
		if( !( r_offsetmapping->integer & 1 ) ) {
			offsetmappingScale = 0;
		}
		if( ri.params & RP_LIGHTMAP ) {
			programFeatures |= GLSL_MATERIAL_APPLY_BASETEX_ALPHA_ONLY;
		}
		if( ( ri.params & RP_DRAWFLAT ) && !( r_back.currentShader->flags & SHADER_NODRAWFLAT ) ) {
			programFeatures |= GLSL_COMMON_APPLY_DRAWFLAT|GLSL_MATERIAL_APPLY_BASETEX_ALPHA_ONLY;
		}
	}
	else if( ( r_back.currentMeshBuffer->sortkey & 3 ) == MB_POLY )
	{
		// polys
		if( !( r_offsetmapping->integer & 2 ) )
			offsetmappingScale = 0;

		R_BuildTangentVectors( r_backacc.numVerts, vertsArray, normalsArray, coordsArray, r_backacc.numElems/3, elemsArray, inSVectorsArray );
	}
	else
	{
		// models and world lightingDiffuse materials
		if( r_back.currentMeshBuffer->infokey > 0 )
		{
			if( !( r_offsetmapping->integer & 1 ) )
				offsetmappingScale = 0;
			pass->rgbgen.type = RGB_GEN_VERTEX;
		}
		else
		{
			// models
			if( !( r_offsetmapping->integer & 4 ) )
				offsetmappingScale = 0;
#ifdef CELLSHADEDMATERIAL
			programFeatures |= GLSL_MATERIAL_APPLY_CELLSHADING;
#endif
#ifdef HALFLAMBERTLIGHTING
			programFeatures |= GLSL_MATERIAL_APPLY_HALFLAMBERT;
#endif
		}
	}

	// add dynamic lights
	if( r_back.currentDlightBits ) {
		programFeatures |= R_DlightbitsToProgramFeatures();
	}

	pass->tcgen = TC_GEN_BASE;
	R_BindShaderpass( pass, base, 0, &programFeatures );

	// calculate vertex color
	R_ModifyColor( pass, applyDecal, r_back.currentMeshVBO != NULL );

	// since R_ModifyColor has forcefully generated RGBA for the first vertex,
	// set the proper number of color elements here, so GL_COLOR_ARRAY will be enabled
	// before the DrawElements call
	if( !(pass->flags & SHADERPASS_NOCOLORARRAY) )
		r_backacc.numColors = r_backacc.numVerts;

	// convert rgbgen and alphagen to GLSL feature defines
	programFeatures |= R_RGBAlphaGenToProgramFeatures( pass->rgbgen.type, pass->alphagen.type );

	GL_TexEnv( GL_MODULATE );

	// set shaderpass state (blending, depthwrite, etc)
	state = r_back.currentShaderState | ( pass->flags & r_back.currentShaderPassMask ) | GLSTATE_BLEND_MTEX;
	GL_SetState( state );

#if 1
	// don't waste time on processing GLSL programs with zero colormask
	if( ( ri.params & RP_SHADOWMAPVIEW ) & !(programFeatures & GLSL_COMMON_APPLY_BONETRANSFORMS) )
	{
		pass->tcgen = tcgen; // restore original tcgen
		R_FlushArrays();
		return;
	}
#endif

	// we only send S-vectors to GPU and recalc T-vectors as cross product
	// in vertex shader
	pass->tcgen = TC_GEN_SVECTORS;
	GL_Bind( 1, normalmap );         // normalmap
	GL_SetTexCoordArrayMode( GL_TEXTURE_COORD_ARRAY );
	R_VertexTCBase( pass, 1, unused, NULL );

	if( glossmap && r_lighting_glossintensity->value )
	{
		programFeatures |= GLSL_MATERIAL_APPLY_SPECULAR;
		GL_Bind( 2, glossmap ); // gloss
		GL_SetTexCoordArrayMode( 0 );
	}

	if( applyDecal )
	{
		programFeatures |= GLSL_MATERIAL_APPLY_DECAL;

		if( ri.params & RP_LIGHTMAP ) {
			decalmap = r_blacktexture;
			programFeatures |= GLSL_MATERIAL_APPLY_DECAL_ADD;
		}
		else {
			// if no alpha, use additive blending
			if( decalmap->samples == 3 )
				programFeatures |= GLSL_MATERIAL_APPLY_DECAL_ADD;
		}

		GL_Bind( 3, decalmap ); // decal
		GL_SetTexCoordArrayMode( 0 );
	}

	if( entdecalmap )
	{
		programFeatures |= GLSL_MATERIAL_APPLY_ENTITY_DECAL;

		// if no alpha, use additive blending
		if( entdecalmap->samples == 3 )
			programFeatures |= GLSL_MATERIAL_APPLY_ENTITY_DECAL_ADD;

		GL_Bind( 4, entdecalmap ); // decal
		GL_SetTexCoordArrayMode( 0 );
	}

	if( offsetmappingScale > 0 )
		programFeatures |= r_offsetmapping_reliefmapping->integer ? GLSL_MATERIAL_APPLY_RELIEFMAPPING : GLSL_MATERIAL_APPLY_OFFSETMAPPING;

	if( r_back.currentMeshBuffer->infokey > 0 && ( rgbgen != RGB_GEN_LIGHTING_DIFFUSE ) )
	{
		// world surface
		if( r_back.superLightStyle && r_back.superLightStyle->lightmapNum[0] >= 0 )
		{
			lightStyle = r_back.superLightStyle;

			// bind lightmap textures and set program's features for lightstyles
			pass->tcgen = TC_GEN_LIGHTMAP;

			for( i = 0; i < MAX_LIGHTMAPS && lightStyle->lightmapStyles[i] != 255; i++ )
			{
				r_back.lightmapStyleNum[i+4] = i;
				GL_Bind( i+4, r_worldbrushmodel->lightmapImages[lightStyle->lightmapNum[i]] ); // lightmap
				GL_SetTexCoordArrayMode( GL_TEXTURE_COORD_ARRAY );
				R_VertexTCBase( pass, i+4, unused, NULL );
			}

			programFeatures |= ( i * GLSL_MATERIAL_APPLY_LIGHTSTYLE0 );

			if( i == 1 && !mapConfig.lightingIntensity )
			{
				vec_t *rgb = r_lightStyles[lightStyle->lightmapStyles[0]].rgb;

				// GLSL_MATERIAL_APPLY_FB_LIGHTMAP indicates that there's no need to renormalize
				// the lighting vector for specular (saves 3 adds, 3 muls and 1 normalize per pixel)
				if( rgb[0] == 1 && rgb[1] == 1 && rgb[2] == 1 )
					programFeatures |= GLSL_MATERIAL_APPLY_FB_LIGHTMAP;
			}

			if( !VectorCompare( mapConfig.ambient, vec3_origin ) )
			{
				VectorCopy( mapConfig.ambient, ambient );
				programFeatures |= GLSL_MATERIAL_APPLY_AMBIENT_COMPENSATION;
			}
		}
	}
	else
	{
		vec3_t temp;

		programFeatures |= GLSL_MATERIAL_APPLY_DIRECTIONAL_LIGHT;

		if( ( r_back.currentMeshBuffer->sortkey & 3 ) == MB_POLY )
		{
			VectorCopy( r_polys[-r_back.currentMeshBuffer->infokey-1].normal, lightDir );
			Vector4Set( ambient, 0, 0, 0, 0 );
			Vector4Set( diffuse, 1, 1, 1, 1 );
		}
		else if( ri.currententity )
		{
			if( ri.currententity->flags & RF_FULLBRIGHT )
			{
				Vector4Set( ambient, 1, 1, 1, 1 );
				Vector4Set( diffuse, 1, 1, 1, 1 );
			}
			else
			{
				if( r_back.currentMeshBuffer->infokey > 0 )
				{
					programFeatures |= GLSL_MATERIAL_APPLY_DIRECTIONAL_LIGHT_MIX;
					if( r_back.overBrightBits )
						programFeatures |= GLSL_COMMON_APPLY_OVERBRIGHT_SCALING;
				}

				if( ri.currententity->model && ri.currententity != r_worldent )
				{
					// get weighted incoming direction of world and dynamic lights
					R_LightForOrigin( ri.currententity->lightingOrigin, temp, ambient, diffuse, 
						ri.currententity->model->radius * ri.currententity->scale);
				}
				else
				{
					VectorSet( temp, 0.1f, 0.2f, 0.7f );
				}

				if( ri.currententity->flags & RF_MINLIGHT )
				{
					if( ambient[0] <= 0.1f || ambient[1] <= 0.1f || ambient[2] <= 0.1f )
						VectorSet( ambient, 0.1f, 0.1f, 0.1f );
				}

				// rotate direction
				Matrix_TransformVector( ri.currententity->axis, temp, lightDir );
			}
		}
	}

	pass->tcgen = tcgen;    // restore original tcgen
	pass->rgbgen.type = rgbgen;		// restore original rgbgen

	program = R_RegisterGLSLProgram( pass->program_type, pass->program, NULL, 
		r_back.currentShader->name, r_back.currentShader->deforms, r_back.currentShader->numdeforms, programFeatures );
	object = R_GetProgramObject( program );
	if( object )
	{
		qglUseProgramObjectARB( object );

		// update uniforms
		R_UpdateProgramUniforms( program, ri.viewOrigin, vec3_origin, lightDir, ambient, diffuse, lightStyle,
			qtrue, 0, 0, 0, offsetmappingScale, glossExponent, 
			colorArrayCopy[0], r_back.overBrightBits, r_back.currentShaderTime, r_back.entityColor );

		if( programFeatures & GLSL_COMMON_APPLY_FOG )
		{
			cplane_t fogPlane, vpnPlane;

			R_TransformFogPlanes( fog, fogPlane.normal, &fogPlane.dist, vpnPlane.normal, &vpnPlane.dist );

			R_UpdateProgramFogParams( program, fog->shader->fog_color, fog->shader->fog_clearDist,
				fog->shader->fog_dist, &fogPlane, &vpnPlane, ri.fog_dist_to_eye[fog-r_worldbrushmodel->fogs] );
		}

		// submit animation data
		if( programFeatures & GLSL_COMMON_APPLY_BONETRANSFORMS ) {
			R_UpdateProgramBonesParams( program, r_back.currentAnimData->numBones, r_back.currentAnimData->dualQuats );
		}

		// dynamic lights
		if( r_back.currentDlightBits ) {
			R_UpdateProgramLightsParams( program, ri.currententity->origin, ri.currententity->axis, r_back.currentDlightBits );
			r_back.doDynamicLightsPass = qfalse;
		}

		// r_drawflat
		if( programFeatures & GLSL_COMMON_APPLY_DRAWFLAT ) {
			R_UpdateDrawFlatParams( program, r_front.wallColor, r_front.floorColor );
		}

		R_FlushArrays();

		qglUseProgramObjectARB( 0 );
	}
}

/*
* R_RenderMeshGLSL_Distortion
*/
static void R_RenderMeshGLSL_Distortion( r_glslfeat_t programFeatures )
{
	int i, last_slot;
	unsigned last_framenum;
	int state, tcgen;
	int width = 1, height = 1;
	int program, object;
	mat4x4_t unused;
	cplane_t plane;
	const char *key;
	shaderpass_t *pass = r_back.accumPasses[0];
	image_t *portaltexture[2];
	qboolean frontPlane;

	PlaneFromPoints( r_back.r_triangle0Copy, &plane );
	plane.dist += DotProduct( ri.currententity->origin, plane.normal );
	key = R_PortalKeyForPlane( &plane );
	last_framenum = last_slot = 0;

	for( i = 0; i < 2; i++ )
	{
		int slot;

		portaltexture[i] = NULL;

		slot = R_FindPortalTextureSlot( key, i+1 );
		if( slot )
			portaltexture[i] = r_portaltextures[slot-1];

		if( portaltexture[i] == NULL )
		{
			portaltexture[i] = r_blacktexture;
		}
		else
		{
			width = portaltexture[i]->upload_width;
			height = portaltexture[i]->upload_height;
		}

		// find the most recently updated texture
		if( portaltexture[i]->framenum > last_framenum )
		{
			last_slot = i;
			last_framenum = i;
		}
	}

	// if textures were not updated sequentially, use the most recent one
	// and reset the remaining to black
	if( portaltexture[0]->framenum+1 != portaltexture[1]->framenum )
		portaltexture[(last_slot+1)&1] = r_blacktexture;

	if( pass->anim_frames[0] != r_blankbumptexture )
		programFeatures |= GLSL_DISTORTION_APPLY_DUDV;
	if( ri.params & RP_CLIPPLANE )
		programFeatures |= GLSL_COMMON_APPLY_CLIPPING;
	if( pass->flags & SHADERPASS_GRAYSCALE )
		programFeatures |= GLSL_COMMON_APPLY_GRAYSCALE;
	if( portaltexture[0] != r_blacktexture )
		programFeatures |= GLSL_DISTORTION_APPLY_REFLECTION;
	if( portaltexture[1] != r_blacktexture )
		programFeatures |= GLSL_DISTORTION_APPLY_REFRACTION;

	frontPlane = (PlaneDiff( ri.viewOrigin, &ri.portalPlane ) > 0 ? qtrue : qfalse);

	if( frontPlane )
	{
		if( pass->alphagen.type != ALPHA_GEN_IDENTITY )
			programFeatures |= GLSL_DISTORTION_APPLY_DISTORTION_ALPHA;
	}

	tcgen = pass->tcgen;                // store the original tcgen

	R_BindShaderpass( pass, pass->anim_frames[0], 0, NULL );  // dudvmap

	// calculate the fragment color
	R_ModifyColor( pass, programFeatures & GLSL_DISTORTION_APPLY_DISTORTION_ALPHA ? qtrue : qfalse, qfalse );

	GL_TexEnv( GL_MODULATE );

	// set shaderpass state (blending, depthwrite, etc)
	state = r_back.currentShaderState | ( pass->flags & r_back.currentShaderPassMask ) | GLSTATE_BLEND_MTEX;
	GL_SetState( state );

	if( pass->anim_frames[1] )
	{	// eyeDot
		programFeatures |= GLSL_DISTORTION_APPLY_EYEDOT;

		pass->tcgen = TC_GEN_SVECTORS;
		GL_Bind( 1, pass->anim_frames[1] ); // normalmap
		GL_SetTexCoordArrayMode( GL_TEXTURE_COORD_ARRAY );
		R_VertexTCBase( pass, 1, unused, NULL );
	}

	GL_Bind( 2, portaltexture[0] );            // reflection
	GL_Bind( 3, portaltexture[1] );           // refraction

	pass->tcgen = tcgen;    // restore original tcgen

	// update uniforms
	program = R_RegisterGLSLProgram( pass->program_type, pass->program, NULL, NULL, NULL, 0, programFeatures );
	object = R_GetProgramObject( program );
	if( object )
	{
		qglUseProgramObjectARB( object );

		R_UpdateProgramUniforms( program, ri.viewOrigin, vec3_origin, vec3_origin, NULL, NULL, NULL,
			frontPlane, width, height, 0, 0, 0, 
			colorArrayCopy[0], r_back.overBrightBits, r_back.currentShaderTime, r_back.entityColor );

		R_FlushArrays();

		qglUseProgramObjectARB( 0 );
	}
}

/*
* R_RenderMeshGLSL_Shadowmap
*/
static void R_RenderMeshGLSL_Shadowmap( r_glslfeat_t programFeatures )
{
	int i;
	int state;
	int scissor[4], old_scissor[4];
	int program, object;
	vec3_t tdir, lightDir;
	shaderpass_t *pass = r_back.accumPasses[0];

	if( r_shadows_pcf->integer )
		programFeatures |= GLSL_SHADOWMAP_APPLY_PCF;
	if( r_shadows_dither->integer )
		programFeatures |= GLSL_SHADOWMAP_APPLY_DITHER;

	// update uniforms
	program = R_RegisterGLSLProgram( pass->program_type, pass->program, NULL, NULL, NULL, 0, programFeatures );
	object = R_GetProgramObject( program );
	if( !object )
		return;

	Vector4Copy( ri.scissor, old_scissor );

	for( i = 0, r_back.currentCastGroup = r_shadowGroups; i < r_numShadowGroups; i++, r_back.currentCastGroup++ )
	{
		if( !( r_back.currentShadowBits & r_back.currentCastGroup->bit ) )
			continue;

		// project the bounding box on to screen then use scissor test
		// so that fragment shader isn't run for unshadowed regions
		if( !R_ScissorForBounds( r_back.currentCastGroup->visCorners, 
			&scissor[0], &scissor[1], &scissor[2], &scissor[3] ) )
			continue;

		GL_Scissor( ri.refdef.x + scissor[0], ri.refdef.y + scissor[1], scissor[2], scissor[3] );

		R_BindShaderpass( pass, r_back.currentCastGroup->depthTexture, 0, NULL );

		GL_TexEnv( GL_MODULATE );

		qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE_ARB );
		qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL );

		// calculate the fragment color
		R_ModifyColor( pass, qfalse, qfalse );

		// set shaderpass state (blending, depthwrite, etc)
		state = r_back.currentShaderState | ( pass->flags & r_back.currentShaderPassMask ) | GLSTATE_BLEND_MTEX;
		GL_SetState( state );

		qglUseProgramObjectARB( object );

		VectorCopy( r_back.currentCastGroup->direction, tdir );
		Matrix_TransformVector( ri.currententity->axis, tdir, lightDir );

		R_UpdateProgramUniforms( program, ri.viewOrigin, vec3_origin, lightDir,
			r_back.currentCastGroup->lightAmbient, NULL, NULL, qtrue,
			r_back.currentCastGroup->depthTexture->upload_width, r_back.currentCastGroup->depthTexture->upload_height,
			r_back.currentCastGroup->projDist,
			0, 0, 
			colorArrayCopy[0], r_back.overBrightBits, r_back.currentShaderTime, r_back.entityColor );

		R_FlushArrays();

		qglUseProgramObjectARB( 0 );

		qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE );
	}

	GL_Scissor( old_scissor[0], old_scissor[1], old_scissor[2], old_scissor[3] );
}

#ifdef HARDWARE_OUTLINES
/*
* R_RenderMeshGLSL_Outline
*/
static void R_RenderMeshGLSL_Outline( r_glslfeat_t programFeatures )
{
	int faceCull;
	int state;
	int program, object;
	shaderpass_t *pass = r_back.accumPasses[0];

	if( ri.params & RP_CLIPPLANE )
		programFeatures |= GLSL_COMMON_APPLY_CLIPPING;
	if( ri.currentmodel && ri.currentmodel->type == mod_brush )
		programFeatures |= GLSL_OUTLINE_APPLY_OUTLINES_CUTOFF;

	// update uniforms
	program = R_RegisterGLSLProgram( pass->program_type, pass->program, NULL, NULL, NULL, 0, programFeatures );
	object = R_GetProgramObject( program );
	if( !object )
		return;

	faceCull = glState.faceCull;
	GL_Cull( GL_BACK );

	GL_SelectTexture( 0 );
	GL_SetTexCoordArrayMode( 0 );

	// calculate the fragment color
	R_ModifyColor( pass, qfalse, qfalse );

	// set shaderpass state (blending, depthwrite, etc)
	state = r_back.currentShaderState | ( pass->flags & r_back.currentShaderPassMask ) | GLSTATE_BLEND_MTEX;
	GL_SetState( state );

	qglUseProgramObjectARB( object );

	R_UpdateProgramUniforms( program, ri.viewOrigin, vec3_origin, vec3_origin, NULL, NULL, NULL, qtrue,
		0, 0, ri.currententity->outlineHeight * r_outlines_scale->value, 0, 0, 
		colorArrayCopy[0], r_back.overBrightBits, r_back.currentShaderTime, r_back.entityColor );

	// submit animation data
	if( programFeatures & GLSL_COMMON_APPLY_BONETRANSFORMS ) {
		R_UpdateProgramBonesParams( program, r_back.currentAnimData->numBones, r_back.currentAnimData->dualQuats );
	}

	R_FlushArrays();

	qglUseProgramObjectARB( 0 );

	GL_Cull( faceCull );
}
#endif

/*
* R_RenderMeshGLSL_Turbulence
*/
static void R_RenderMeshGLSL_Turbulence( r_glslfeat_t programFeatures )
{
	int i;
	int state;
	int program, object;
	float scale = 0, phase = 0;
	shaderpass_t *pass = r_back.accumPasses[0];

	if( ri.params & RP_CLIPPLANE )
		programFeatures |= GLSL_COMMON_APPLY_CLIPPING;
	if( pass->flags & SHADERPASS_GRAYSCALE )
		programFeatures |= GLSL_COMMON_APPLY_GRAYSCALE;

	for( i = 0; i < pass->numtcmods; i++ )
	{
		if( pass->tcmods[i].type != TC_MOD_TURB )
			continue;

		scale += pass->tcmods[i].args[1];
		phase += pass->tcmods[i].args[2] + r_back.currentShaderTime * pass->tcmods[i].args[3];
	}

	R_BindShaderpass( pass, NULL, 0, &programFeatures );

	GL_TexEnv( GL_MODULATE );

	// calculate the fragment color
	R_ModifyColor( pass, qfalse, qfalse );

	// set shaderpass state (blending, depthwrite, etc)
	state = r_back.currentShaderState | ( pass->flags & r_back.currentShaderPassMask ) | GLSTATE_BLEND_MTEX;
	GL_SetState( state );

	// update uniforms
	program = R_RegisterGLSLProgram( pass->program_type, pass->program, NULL, 
		r_back.currentShader->name, r_back.currentShader->deforms, r_back.currentShader->numdeforms, programFeatures );
	object = R_GetProgramObject( program );
	if( object )
	{
		qglUseProgramObjectARB( object );

		R_UpdateProgramUniforms( program, ri.viewOrigin, vec3_origin, vec3_origin, NULL, NULL, NULL, qtrue, 0, 0, scale, phase, 0, 
			colorArrayCopy[0], r_back.overBrightBits, r_back.currentShaderTime, r_back.entityColor );

		R_FlushArrays();

		qglUseProgramObjectARB( 0 );
	}
}

/*
* R_RenderMeshGLSL_DynamicLights
*/
static void R_RenderMeshGLSL_DynamicLights( r_glslfeat_t programFeatures )
{
	int state;
	int program, object;
	shaderpass_t *pass = r_back.accumPasses[0];

	if( ri.params & RP_CLIPPLANE )
		programFeatures |= GLSL_COMMON_APPLY_CLIPPING;
	programFeatures |= R_DlightbitsToProgramFeatures();

	GL_SelectTexture( 0 );
	GL_SetTexCoordArrayMode( 0 );

	GL_TexEnv( GL_MODULATE );

	// set shaderpass state (blending, depthwrite, etc)
	state = r_back.currentShaderState | ( pass->flags & r_back.currentShaderPassMask ) | GLSTATE_BLEND_MTEX;
	GL_SetState( state );

	// update uniforms
	program = R_RegisterGLSLProgram( pass->program_type, pass->program, NULL, NULL, NULL, 0, programFeatures );
	object = R_GetProgramObject( program );
	if( object )
	{
		qglUseProgramObjectARB( object );

		R_UpdateProgramLightsParams( program, ri.currententity->origin, ri.currententity->axis, r_back.currentDlightBits );
		r_back.doDynamicLightsPass = qfalse;

		R_FlushArrays();

		qglUseProgramObjectARB( 0 );
	}
}

/*
* R_RenderMeshGLSL_Q3AShader
*/
static void R_RenderMeshGLSL_Q3AShader( r_glslfeat_t programFeatures )
{
	int state;
	int program, object;
	vec3_t entDist;
	const mfog_t *fog = r_back.texFog ? r_back.texFog : r_back.colorFog;
	shaderpass_t *pass = r_back.accumPasses[0];
	qboolean isLightmap, isWorldVertexLight;

	if( ri.params & RP_CLIPPLANE )
		programFeatures |= GLSL_COMMON_APPLY_CLIPPING;
	if( pass->flags & SHADERPASS_GRAYSCALE )
		programFeatures |= GLSL_COMMON_APPLY_GRAYSCALE;
	if( fog )
	{
		programFeatures |= GLSL_COMMON_APPLY_FOG;
		if( fog != ri.fog_eye )
			programFeatures |= GLSL_COMMON_APPLY_FOG2;

		if( fog == r_back.colorFog )
		{
			programFeatures |= GLSL_Q3A_APPLY_COLOR_FOG;

			if( GL_IsAlphaBlending( pass->flags & GLSTATE_SRCBLEND_MASK, pass->flags & GLSTATE_DSTBLEND_MASK ) )
				programFeatures |= GLSL_COMMON_APPLY_COLOR_FOG_ALPHA;
		}
	}

	// lightmapping pass
	if( pass->flags & SHADERPASS_LIGHTMAP ) {
		isLightmap = qtrue;
	}
	else {
		isLightmap = qfalse;
	}

	// vertex-lit world surface
	if( ( r_back.currentMeshBuffer->infokey > 0 ) 
		&& ( pass->rgbgen.type == RGB_GEN_VERTEX || pass->rgbgen.type == RGB_GEN_EXACT_VERTEX ) ) {
		isWorldVertexLight = qtrue;
	}
	else {
		isWorldVertexLight = qfalse;
	}

	if( isLightmap ) {
		// add dynamic lights
		if( r_back.currentDlightBits ) {
			programFeatures |= R_DlightbitsToProgramFeatures();
		}
		if( ( ri.params & RP_DRAWFLAT ) && !( r_back.currentShader->flags & SHADER_NODRAWFLAT ) ) {
			programFeatures |= GLSL_COMMON_APPLY_DRAWFLAT;
		}
	}

	R_BindShaderpass( pass, NULL, 0, &programFeatures );

	GL_TexEnv( GL_MODULATE );

	// calculate the fragment color
	R_ModifyColor( pass, qfalse, qtrue );

	// since R_ModifyColor has forcefully generated RGBA for the first vertex,
	// set the proper number of color elements here, so GL_COLOR_ARRAY will be enabled
	// before the DrawElements call
	if( !(pass->flags & SHADERPASS_NOCOLORARRAY) )
		r_backacc.numColors = r_backacc.numVerts;

	// convert rgbgen and alphagen to GLSL feature defines
	programFeatures |= R_RGBAlphaGenToProgramFeatures( pass->rgbgen.type, pass->alphagen.type );

	// set shaderpass state (blending, depthwrite, etc)
	state = r_back.currentShaderState | ( pass->flags & r_back.currentShaderPassMask ) | GLSTATE_BLEND_MTEX;
	GL_SetState( state );

	// update uniforms
	program = R_RegisterGLSLProgram( GLSL_PROGRAM_TYPE_Q3A_SHADER, DEFAULT_GLSL_Q3A_SHADER_PROGRAM, NULL, 
		r_back.currentShader->name, r_back.currentShader->deforms, r_back.currentShader->numdeforms, programFeatures );
	object = R_GetProgramObject( program );
	if( object )
	{
		qglUseProgramObjectARB( object );

		VectorCopy( ri.viewOrigin, entDist );
		if( ri.currententity )
		{
			vec3_t tmp;
			VectorSubtract( entDist, ri.currententity->origin, tmp );
			Matrix_TransformVector( ri.currententity->axis, tmp, entDist );
		}

		R_UpdateProgramQ3AParams( program, r_back.currentShaderTime, ri.viewOrigin, entDist, colorArrayCopy[0], r_back.overBrightBits );

		if( fog )
		{
			cplane_t fogPlane, vpnPlane;

			R_TransformFogPlanes( fog, fogPlane.normal, &fogPlane.dist, vpnPlane.normal, &vpnPlane.dist );

			R_UpdateProgramFogParams( program, fog->shader->fog_color, fog->shader->fog_clearDist,
				fog->shader->fog_dist, &fogPlane, &vpnPlane, ri.fog_dist_to_eye[fog-r_worldbrushmodel->fogs] );
		}

		// submit animation data
		if( programFeatures & GLSL_COMMON_APPLY_BONETRANSFORMS ) {
			R_UpdateProgramBonesParams( program, r_back.currentAnimData->numBones, r_back.currentAnimData->dualQuats );
		}

		// dynamic lights
		if( r_back.currentDlightBits && ( isLightmap || isWorldVertexLight ) ) {
			R_UpdateProgramLightsParams( program, ri.currententity->origin, ri.currententity->axis, r_back.currentDlightBits );
			r_back.doDynamicLightsPass = qfalse;
		}

		// r_drawflat
		if( programFeatures & GLSL_COMMON_APPLY_DRAWFLAT ) {
			R_UpdateDrawFlatParams( program, r_front.wallColor, r_front.floorColor );
		}

		R_FlushArrays();

		qglUseProgramObjectARB( 0 );
	}
}

/*
* R_RenderMeshGLSL_PlanarShadow
*/
static void R_RenderMeshGLSL_PlanarShadow( r_glslfeat_t programFeatures )
{
	int state;
	int program, object;
	shaderpass_t *pass = r_back.accumPasses[0];
	float planeDist;
	vec3_t planeNormal, lightDir;

	if( !R_DeformVPlanarShadowParams( planeNormal, &planeDist, lightDir ) )
		return;

	if( ri.params & RP_CLIPPLANE )
		programFeatures |= GLSL_COMMON_APPLY_CLIPPING;

	GL_SelectTexture( 0 );
	GL_SetTexCoordArrayMode( 0 );

	qglDisable( GL_TEXTURE_2D );

	// set shaderpass state (blending, depthwrite, etc)
	state = r_back.currentShaderState | ( pass->flags & r_back.currentShaderPassMask ) | GLSTATE_STENCIL_TEST;
	GL_SetState( state );

	// calculate the fragment color
	R_ModifyColor( pass, qtrue, qtrue );

	// update uniforms
	program = R_RegisterGLSLProgram( GLSL_PROGRAM_TYPE_PLANAR_SHADOW, DEFAULT_GLSL_PLANAR_SHADOW_PROGRAM, NULL, 
		NULL, NULL, 0, programFeatures );
	object = R_GetProgramObject( program );
	if( object )
	{
		qglUseProgramObjectARB( object );

		R_UpdateProgramPlanarShadowParams( program, r_back.currentShaderTime, planeNormal, planeDist, lightDir );

		// submit animation data
		if( programFeatures & GLSL_COMMON_APPLY_BONETRANSFORMS ) {
			R_UpdateProgramBonesParams( program, r_back.currentAnimData->numBones, r_back.currentAnimData->dualQuats );
		}

		R_FlushArrays();

		qglUseProgramObjectARB( 0 );
	}
	
	qglEnable( GL_TEXTURE_2D );
}

/*
* R_RenderMeshGLSL_Cellshade
*/
static void R_RenderMeshGLSL_Cellshade( r_glslfeat_t programFeatures )
{
	int state;
	int program, object;
	vec3_t entDist;
	image_t *base, *shade, *diffuse, *decal, *entdecal, *stripes, *light;
	const mfog_t *fog = r_back.colorFog;
	shaderpass_t *pass = r_back.accumPasses[0];
	mat4x4_t reflectionMatrix;

	base = pass->anim_frames[0];
	shade = pass->anim_frames[1];
	diffuse = pass->anim_frames[2];
	decal = pass->anim_frames[3];
	entdecal = pass->anim_frames[4];
	stripes = pass->anim_frames[5];
	light = pass->anim_frames[6];

	if( ri.params & RP_CLIPPLANE )
		programFeatures |= GLSL_COMMON_APPLY_CLIPPING;
	if( pass->flags & SHADERPASS_GRAYSCALE )
		programFeatures |= GLSL_COMMON_APPLY_GRAYSCALE;
	if( fog )
	{
		programFeatures |= GLSL_COMMON_APPLY_FOG;
		if( fog != ri.fog_eye )
			programFeatures |= GLSL_COMMON_APPLY_FOG2;
	}

	R_BindShaderpass( pass, base, 0, &programFeatures );

	R_VertexTCCellshadeMatrix( reflectionMatrix );

	GL_TexEnv( GL_MODULATE );

	// calculate the fragment color
	R_ModifyColor( pass, qfalse, qtrue );

	// since R_ModifyColor has forcefully generated RGBA for the first vertex,
	// set the proper number of color elements here, so GL_COLOR_ARRAY will be enabled
	// before the DrawElements call
	if( !(pass->flags & SHADERPASS_NOCOLORARRAY) )
		r_backacc.numColors = r_backacc.numVerts;

	// convert rgbgen and alphagen to GLSL feature defines
	programFeatures |= R_RGBAlphaGenToProgramFeatures( pass->rgbgen.type, pass->alphagen.type );

	// set shaderpass state (blending, depthwrite, etc)
	state = r_back.currentShaderState | ( pass->flags & r_back.currentShaderPassMask ) | GLSTATE_BLEND_MTEX;
	GL_SetState( state );

	// bind white texture for shadow map view
#define CELLSHADE_BIND(tmu,tex,feature,canAdd) \
	if( tex ) { \
		if( ri.params & RP_SHADOWMAPVIEW ) { \
			tex = r_whitetexture; \
		} else {\
			programFeatures |= feature; \
			if( canAdd && tex->samples == 3 ) programFeatures |= ((feature) << 1); \
		} \
		GL_Bind(tmu, tex); \
		GL_SetTexCoordArrayMode( 0 ); \
	}

	CELLSHADE_BIND( 1, shade, 0, qfalse );
	CELLSHADE_BIND( 2, diffuse, GLSL_CELLSHADE_APPLY_DIFFUSE, qfalse );
	CELLSHADE_BIND( 3, decal, GLSL_CELLSHADE_APPLY_DECAL, qtrue );
	CELLSHADE_BIND( 4, entdecal, GLSL_CELLSHADE_APPLY_ENTITY_DECAL, qtrue );
	CELLSHADE_BIND( 5, stripes, GLSL_CELLSHADE_APPLY_STRIPES, qtrue );
	CELLSHADE_BIND( 6, light, GLSL_CELLSHADE_APPLY_CELL_LIGHT, qtrue );

#undef CELLSHADE_BIND

	// update uniforms
	program = R_RegisterGLSLProgram( GLSL_PROGRAM_TYPE_CELLSHADE, DEFAULT_GLSL_CELLSHADE_PROGRAM, NULL, 
		r_back.currentShader->name, r_back.currentShader->deforms, r_back.currentShader->numdeforms, programFeatures );
	object = R_GetProgramObject( program );
	if( object )
	{
		VectorCopy( ri.viewOrigin, entDist );
		if( ri.currententity )
		{
			vec3_t tmp;
			VectorSubtract( entDist, ri.currententity->origin, tmp );
			Matrix_TransformVector( ri.currententity->axis, tmp, entDist );
		}

		qglUseProgramObjectARB( object );

		R_UpdateProgramCellshadeParams( program, r_back.currentShaderTime, ri.viewOrigin, entDist, colorArrayCopy[0], r_back.overBrightBits
			, r_back.entityColor, reflectionMatrix );

		if( fog )
		{
			cplane_t fogPlane, vpnPlane;

			R_TransformFogPlanes( fog, fogPlane.normal, &fogPlane.dist, vpnPlane.normal, &vpnPlane.dist );

			R_UpdateProgramFogParams( program, fog->shader->fog_color, fog->shader->fog_clearDist,
				fog->shader->fog_dist, &fogPlane, &vpnPlane, ri.fog_dist_to_eye[fog-r_worldbrushmodel->fogs] );
		}

		// submit animation data
		if( programFeatures & GLSL_COMMON_APPLY_BONETRANSFORMS ) {
			R_UpdateProgramBonesParams( program, r_back.currentAnimData->numBones, r_back.currentAnimData->dualQuats );
		}

		R_FlushArrays();

		qglUseProgramObjectARB( 0 );
	}
}

/*
* R_RenderMeshGLSLProgrammed
*/
void R_RenderMeshGLSLProgrammed( int programType )
{
	r_glslfeat_t features = 0;

	features |= R_BonesTransformsToProgramFeatures();

	switch( programType )
	{
	case GLSL_PROGRAM_TYPE_MATERIAL:
		R_RenderMeshGLSL_Material( features );
		break;
	case GLSL_PROGRAM_TYPE_DISTORTION:
		R_RenderMeshGLSL_Distortion( features );
		break;
	case GLSL_PROGRAM_TYPE_SHADOWMAP:
		R_RenderMeshGLSL_Shadowmap( features );
		break;
	case GLSL_PROGRAM_TYPE_OUTLINE:
#ifdef HARDWARE_OUTLINES
		R_RenderMeshGLSL_Outline( features );
#endif
		break;
	case GLSL_PROGRAM_TYPE_TURBULENCE:
		R_RenderMeshGLSL_Turbulence( features );
		break;
	case GLSL_PROGRAM_TYPE_DYNAMIC_LIGHTS:
		R_RenderMeshGLSL_DynamicLights( features );
		break;
	case GLSL_PROGRAM_TYPE_Q3A_SHADER:
		R_RenderMeshGLSL_Q3AShader( features );
		break;
	case GLSL_PROGRAM_TYPE_PLANAR_SHADOW:
		R_RenderMeshGLSL_PlanarShadow( features );
		break;
	case GLSL_PROGRAM_TYPE_CELLSHADE:
		R_RenderMeshGLSL_Cellshade( features );
		break;
	default:
		Com_DPrintf( S_COLOR_YELLOW "WARNING: Unknown GLSL program type %i\n", programType );
		return;
	}

	r_back.currentShaderHasGLSLPasses = qtrue;
}
