/*
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
#ifndef __R_SHADOW_H__
#define __R_SHADOW_H__

#define MAX_SHADOWGROUPS    32

typedef struct shadowGroup_s
{
	unsigned int		bit;
	image_t				*depthTexture;

	vec3_t				origin;
//	int					cluster;
	qbyte				*vis;

	float				projDist;
	vec3_t				mins, maxs;

	vec4_t				lightAmbient;

	mat4x4_t			worldviewProjectionMatrix;
	struct shadowGroup_s *hashNext;
} shadowGroup_t;

extern int r_numShadowGroups;
extern shadowGroup_t r_shadowGroups[MAX_SHADOWGROUPS];
extern int r_entShadowBits[MAX_ENTITIES];

void		R_InitShadows( void );
void		R_ShutdownShadows( void );

qboolean	R_CullPlanarShadow( entity_t *e, vec3_t mins, vec3_t maxs, qboolean occclusionQuery );
void		R_DeformVPlanarShadow( int numV, float *v );
void		R_PlanarShadowPass( int state );
shader_t	*R_PlanarShadowShader( void );

void		R_ClearShadowmaps( void );
void		R_GroupShadowCasters( void );
qboolean	R_AddShadowCaster( entity_t *ent );
void		R_CullShadowmapGroups( void );
void		R_DrawShadowmaps( void );

#endif /*__R_SHADOW_H__*/
