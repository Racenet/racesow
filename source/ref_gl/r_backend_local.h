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
#ifndef __R_BACKEND_LOCAL_H__
#define __R_BACKEND_LOCAL_H__

typedef struct r_backend_s
{
	unsigned int identityLighting;
	unsigned int overBrightBits;

	const meshbuffer_t *currentMeshBuffer;
	const mesh_vbo_t *currentMeshVBO;
	const rbackAnimData_t *currentAnimData;
	shadowGroup_t *currentCastGroup;

	unsigned int currentDlightBits;
	unsigned int currentShadowBits;

	const shader_t *currentShader;
	float currentShaderTime;
	int currentShaderState;
	int currentShaderPassMask;
	qboolean currentShaderHasGLSLPasses;
	qboolean doDynamicLightsPass;
	qboolean lightingPassDone;

	int lightmapStyleNum[MAX_TEXTURE_UNITS];
	const superLightStyle_t *superLightStyle;

	vec3_t r_triangle0Copy[3];	// 3 verts before deformv's - used to calc the portal plane

	int numAccumPasses;
	shaderpass_t *accumPasses[MAX_TEXTURE_UNITS];

	qbyte entityColor[4];

	const mfog_t *texFog, *colorFog;
} rbackend_t;

extern rbackend_t r_back;

// r_backend.c
void R_BindShaderpass( const shaderpass_t *pass, image_t *tex, int unit, r_glslfeat_t *programFeatures );
void R_ModifyColor( const shaderpass_t *pass, qboolean forceAlpha, qboolean firstVertColor );
qboolean R_VertexTCBase( const shaderpass_t *pass, int unit, mat4x4_t matrix, r_glslfeat_t *programFeatures );
void R_TransformFogPlanes( const mfog_t *fog, vec3_t fogNormal, vec_t *fogDist, vec3_t vpnNormal, vec_t *vpnDist );
void R_VertexTCCellshadeMatrix( mat4x4_t matrix );

// r_backend_glsl.c
void R_RenderMeshGLSLProgrammed( int programType );

#endif /*__R_BACKEND_LOCAL_H__*/
