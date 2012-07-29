/*
Copyright (C) 1997-2001 Id Software, Inc.

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

#include "cg_local.h"

#define MAX_CGPOLYS 					800
#define MAX_CGPOLY_VERTS				16

typedef struct cpoly_s
{
	struct cpoly_s *prev, *next;

	struct shader_s	*shader;

	unsigned int die;                   // remove after this time
	unsigned int fadetime;
	float fadefreq;
	float color[4];

	int tag;
	poly_t *poly;

	vec3_t verts[MAX_CGPOLY_VERTS];
	vec3_t origin;
	vec3_t angles;
} cpoly_t;

static cpoly_t cg_polys[MAX_CGPOLYS];
static cpoly_t cg_polys_headnode, *cg_free_polys;

static poly_t cg_poly_polys[MAX_CGPOLYS];
static vec3_t cg_poly_verts[MAX_CGPOLYS][MAX_CGPOLY_VERTS];
static vec2_t cg_poly_stcoords[MAX_CGPOLYS][MAX_CGPOLY_VERTS];
static byte_vec4_t cg_poly_colors[MAX_CGPOLYS][MAX_CGPOLY_VERTS];

/*
* CG_Clearpolys
*/
void CG_ClearPolys( void )
{
	int i;

	memset( cg_polys, 0, sizeof( cg_polys ) );

	// link polys
	cg_free_polys = cg_polys;
	cg_polys_headnode.prev = &cg_polys_headnode;
	cg_polys_headnode.next = &cg_polys_headnode;

	for( i = 0; i < MAX_CGPOLYS; i++ )
	{
		if( i < MAX_CGPOLYS - 1 )
			cg_polys[i].next = &cg_polys[i+1];
		cg_polys[i].poly = &cg_poly_polys[i];
		cg_polys[i].poly->verts = cg_poly_verts[i];
		cg_polys[i].poly->stcoords = cg_poly_stcoords[i];
		cg_polys[i].poly->colors = cg_poly_colors[i];
	}
}

/*
* CG_Allocpoly
*
* Returns either a free poly or the oldest one
*/
static cpoly_t *CG_AllocPoly( void )
{
	cpoly_t *pl;

	// take a free poly if possible
	if( cg_free_polys )
	{
		pl = cg_free_polys;
		cg_free_polys = pl->next;
	}
	else
	{
		// grab the oldest one otherwise
		pl = cg_polys_headnode.prev;
		pl->prev->next = pl->next;
		pl->next->prev = pl->prev;
	}

	// put the poly at the start of the list
	pl->prev = &cg_polys_headnode;
	pl->next = cg_polys_headnode.next;
	pl->next->prev = pl;
	pl->prev->next = pl;

	return pl;
}

/*
* CG_FreePoly
*/
static void CG_FreePoly( cpoly_t *dl )
{
	// remove from linked active list
	dl->prev->next = dl->next;
	dl->next->prev = dl->prev;

	// insert into linked free list
	dl->next = cg_free_polys;
	cg_free_polys = dl;
}

/*
* CG_SpawnPolygon
*/
static cpoly_t *CG_SpawnPolygon( float r, float g, float b, float a,
								unsigned int die, unsigned int fadetime,
struct shader_s *shader, int tag )
{
	cpoly_t *pl;

	fadetime = min( fadetime, die );

	// allocate poly
	pl = CG_AllocPoly();
	pl->die = cg.time + die;
	pl->fadetime = cg.time + ( die - fadetime );
	pl->fadefreq = (fadetime ? (1000.0f/fadetime) * 0.001f : 0);
	pl->shader = shader;
	pl->tag = tag;
	pl->color[0] = r;
	pl->color[1] = g;
	pl->color[2] = b;
	pl->color[3] = a;
	clamp( pl->color[0], 0.0f, 1.0f );
	clamp( pl->color[1], 0.0f, 1.0f );
	clamp( pl->color[2], 0.0f, 1.0f );
	clamp( pl->color[3], 0.0f, 1.0f );

	return pl;
}

/*
* CG_OrientPolygon
*/
static void CG_OrientPolygon( vec3_t origin, vec3_t angles, poly_t *poly )
{
	int i;
	vec3_t perp;
	vec3_t ax[3], localAxis[3];

	AnglesToAxis( angles, ax );
	Matrix_Transpose( ax, localAxis );

	for( i = 0; i < poly->numverts; i++ )
	{
		Matrix_TransformVector( localAxis, poly->verts[i], perp );
		VectorAdd( perp, origin, poly->verts[i] );
	}
}

/*
* CG_SpawnPolyBeam
* Spawns a polygon from start to end points length and given width.
* shaderlenght makes reference to size of the texture it will draw, so it can be tiled.
*/
static cpoly_t *CG_SpawnPolyBeam( vec3_t start, vec3_t end, vec4_t color, int width, unsigned int dietime, unsigned int fadetime, struct shader_s *shader, int shaderlength, int tag )
{
	cpoly_t *cgpoly;
	poly_t *poly;
	vec3_t angles, dir;
	int i;
	float xmin, ymin, xmax, ymax;
	float stx = 1.0f, sty = 1.0f;

	// find out beam polygon sizes
	VectorSubtract( end, start, dir );
	VecToAngles( dir, angles );

	xmin = 0;
	xmax = VectorNormalize( dir );
	ymin = -( width*0.5 );
	ymax = width*0.5;
	if( shaderlength && xmax > shaderlength )
		stx = xmax / (float)shaderlength;

	cgpoly = CG_SpawnPolygon( 1.0, 1.0, 1.0, 1.0, dietime ? dietime : cgs.snapFrameTime, fadetime, shader, tag );

	VectorCopy( angles, cgpoly->angles );
	VectorCopy( start, cgpoly->origin );
	if( color )
		Vector4Copy( color, cgpoly->color );

	// create the polygon inside the cgpolygon
	poly = cgpoly->poly;
	poly->shader = cgpoly->shader;
	poly->numverts = 0;

	// Vic: I think it's safe to assume there should be no fog applied to the beams...
	//poly->fognum = 0;
	poly->fognum = -1;

	// A
	VectorSet( poly->verts[poly->numverts], xmin, 0, ymin );
	poly->stcoords[poly->numverts][0] = 0;
	poly->stcoords[poly->numverts][1] = 0;
	poly->colors[poly->numverts][0] = ( qbyte )( cgpoly->color[0] * 255 );
	poly->colors[poly->numverts][1] = ( qbyte )( cgpoly->color[1] * 255 );
	poly->colors[poly->numverts][2] = ( qbyte )( cgpoly->color[2] * 255 );
	poly->colors[poly->numverts][3] = ( qbyte )( cgpoly->color[3] * 255 );
	poly->numverts++;

	// B
	VectorSet( poly->verts[poly->numverts], xmin, 0, ymax );
	poly->stcoords[poly->numverts][0] = 0;
	poly->stcoords[poly->numverts][1] = sty;
	poly->colors[poly->numverts][0] = ( qbyte )( cgpoly->color[0] * 255 );
	poly->colors[poly->numverts][1] = ( qbyte )( cgpoly->color[1] * 255 );
	poly->colors[poly->numverts][2] = ( qbyte )( cgpoly->color[2] * 255 );
	poly->colors[poly->numverts][3] = ( qbyte )( cgpoly->color[3] * 255 );
	poly->numverts++;

	// C
	VectorSet( poly->verts[poly->numverts], xmax, 0, ymax );
	poly->stcoords[poly->numverts][0] = stx;
	poly->stcoords[poly->numverts][1] = sty;
	poly->colors[poly->numverts][0] = ( qbyte )( cgpoly->color[0] * 255 );
	poly->colors[poly->numverts][1] = ( qbyte )( cgpoly->color[1] * 255 );
	poly->colors[poly->numverts][2] = ( qbyte )( cgpoly->color[2] * 255 );
	poly->colors[poly->numverts][3] = ( qbyte )( cgpoly->color[3] * 255 );
	poly->numverts++;

	// D
	VectorSet( poly->verts[poly->numverts], xmax, 0, ymin );
	poly->stcoords[poly->numverts][0] = stx;
	poly->stcoords[poly->numverts][1] = 0;
	poly->colors[poly->numverts][0] = ( qbyte )( cgpoly->color[0] * 255 );
	poly->colors[poly->numverts][1] = ( qbyte )( cgpoly->color[1] * 255 );
	poly->colors[poly->numverts][2] = ( qbyte )( cgpoly->color[2] * 255 );
	poly->colors[poly->numverts][3] = ( qbyte )( cgpoly->color[3] * 255 );
	poly->numverts++;

	// the verts data is stored inside cgpoly, cause it can be moved later
	for( i = 0; i < poly->numverts; i++ )
		VectorCopy( poly->verts[i], cgpoly->verts[i] );

	return cgpoly;
}

/*
* CG_KillPolyBeamsByTag
*/
void CG_KillPolyBeamsByTag( int tag )
{
	cpoly_t	*cgpoly, *next, *hnode;

	// kill polys that have this tag
	hnode = &cg_polys_headnode;
	for( cgpoly = hnode->prev; cgpoly != hnode; cgpoly = next )
	{
		next = cgpoly->prev;
		if( cgpoly->tag == tag )
			CG_FreePoly( cgpoly );
	}
}

/*
* CG_QuickPolyBeam
*/
void CG_QuickPolyBeam( vec3_t start, vec3_t end, int width, struct shader_s *shader )
{
	cpoly_t *cgpoly, *cgpoly2;

	if( !shader )
		shader = CG_MediaShader( cgs.media.shaderLaser );

	cgpoly = CG_SpawnPolyBeam( start, end, NULL, width, 1, 0, shader, 64, 0 );

	// since autosprite doesn't work, spawn a second and rotate it 90 degrees
	cgpoly2 = CG_SpawnPolyBeam( start, end, NULL, width, 1, 0, shader, 64, 0 );
	cgpoly2->angles[ROLL] += 90;
}

/*
* CG_LaserGunPolyBeam
*/
void CG_LaserGunPolyBeam( vec3_t start, vec3_t end, vec4_t color, int tag )
{
	cpoly_t *cgpoly, *cgpoly2;
	vec4_t tcolor = { 0, 0, 0, 0.35f };
	vec_t total;
	vec_t min;
	vec4_t min_team_color;

	// learn0more: this kinda looks best
	if( color )
	{
		// dmh: if teamcolor is too dark set color to default brighter
		VectorCopy( color, tcolor );
		min = 90 * ( 1.0f/255.0f );
		min_team_color[0] = min_team_color[1] = min_team_color[2] = min;
		total = tcolor[0] + tcolor[1] + tcolor[2];
		if( total < min )
			VectorCopy( min_team_color, tcolor );
	}

	if( cg_lgbeam_old->integer ) {
		cgpoly = CG_SpawnPolyBeam( start, end, color ? tcolor : NULL, 12, 1, 0, CG_MediaShader( cgs.media.shaderLaserGunBeamOld ), 64, tag );

		// since autosprite doesn't work, spawn a second and rotate it 90 degrees
		cgpoly2 = CG_SpawnPolyBeam( start, end, color ? tcolor : NULL, 12, 1, 0, CG_MediaShader( cgs.media.shaderLaserGunBeamOld ), 64, tag );
		cgpoly2->angles[ROLL] += 90;
	} else {
		cgpoly = CG_SpawnPolyBeam( start, end, color ? tcolor : NULL, 12, 1, 0, CG_MediaShader( cgs.media.shaderLaserGunBeam ), 64, tag );

		// since autosprite doesn't work, spawn a second and rotate it 90 degrees
		cgpoly2 = CG_SpawnPolyBeam( start, end, color ? tcolor : NULL, 12, 1, 0, CG_MediaShader( cgs.media.shaderLaserGunBeam ), 64, tag );
		cgpoly2->angles[ROLL] += 90;
	}
}

/*
* CG_ElectroPolyBeam
*/
void CG_ElectroPolyBeam( vec3_t start, vec3_t end, int team )
{
	cpoly_t *cgpoly, *cgpoly2;
	struct shader_s *shader;

	if( cg_ebbeam_time->value <= 0.0f || cg_ebbeam_width->integer <= 0 )
		return;

	if( cg_ebbeam_old->integer )
	{
		if( cg_teamColoredBeams->integer && ( team == TEAM_ALPHA || team == TEAM_BETA ) )
		{
			if( team == TEAM_ALPHA )
				shader = CG_MediaShader( cgs.media.shaderElectroBeamOldAlpha );
			else
				shader = CG_MediaShader( cgs.media.shaderElectroBeamOldBeta );
		}
		else
		{
			shader = CG_MediaShader( cgs.media.shaderElectroBeamOld );
		}

		cgpoly = CG_SpawnPolyBeam( start, end, NULL, cg_ebbeam_width->integer, cg_ebbeam_time->value * 1000, cg_ebbeam_time->value * 1000 * 0.4f, shader, 128, 0 );
		cgpoly->angles[ROLL] += 45;

		cgpoly2 = CG_SpawnPolyBeam( start, end, NULL, cg_ebbeam_width->integer, cg_ebbeam_time->value * 1000, cg_ebbeam_time->value * 1000 * 0.4f, shader, 128, 0 );
		cgpoly2->angles[ROLL] += 135;
	}
	else
	{
		if( cg_teamColoredBeams->integer && ( team == TEAM_ALPHA || team == TEAM_BETA ) )
		{
			if( team == TEAM_ALPHA )
				shader = CG_MediaShader( cgs.media.shaderElectroBeamAAlpha );
			else
				shader = CG_MediaShader( cgs.media.shaderElectroBeamABeta );
		}
		else
		{
			shader = CG_MediaShader( cgs.media.shaderElectroBeamA );
		}

		cgpoly = CG_SpawnPolyBeam( start, end, NULL, cg_ebbeam_width->integer, cg_ebbeam_time->value * 1000, cg_ebbeam_time->value * 1000 * 0.4f, shader, 128, 0 );
		cgpoly->angles[ROLL] += 45;

		if( cg_teamColoredBeams->integer && ( team == TEAM_ALPHA || team == TEAM_BETA ) )
		{
			if( team == TEAM_ALPHA )
				shader = CG_MediaShader( cgs.media.shaderElectroBeamBAlpha );
			else
				shader = CG_MediaShader( cgs.media.shaderElectroBeamBBeta );
		}
		else
		{
			shader = CG_MediaShader( cgs.media.shaderElectroBeamB );
		}

		cgpoly2 = CG_SpawnPolyBeam( start, end, NULL, cg_ebbeam_width->integer, cg_ebbeam_time->value * 1000, cg_ebbeam_time->value * 1000 * 0.4f, shader, 128, 0 );
		cgpoly2->angles[ROLL] += 135;
	}
}

/*
* CG_InstaPolyBeam
*/
void CG_InstaPolyBeam( vec3_t start, vec3_t end, int team )
{
	cpoly_t *cgpoly, *cgpoly2;
	vec4_t tcolor = { 1, 1, 1, 0.35f };
	vec_t total;
	vec_t min;
	vec4_t min_team_color;

	if( cg_instabeam_time->value <= 0.0f || cg_instabeam_width->integer <= 0 )
		return;

	if( cg_teamColoredBeams->integer && ( team == TEAM_ALPHA || team == TEAM_BETA ) )
	{
		CG_TeamColor( team, tcolor );
		min = 90 * ( 1.0f/255.0f );
		min_team_color[0] = min_team_color[1] = min_team_color[2] = min;
		total = tcolor[0] + tcolor[1] + tcolor[2];
		if( total < min )
			VectorCopy( min_team_color, tcolor );
	}
	else
	{
		tcolor[0] = 1.0f;
		tcolor[1] = 0.0f;
		tcolor[2] = 0.4f;
	}

	tcolor[3] = min( cg_instabeam_alpha->value, 1 );
	if( !tcolor[3] )
		return;

	cgpoly = CG_SpawnPolyBeam( start, end, tcolor, cg_instabeam_width->integer, cg_instabeam_time->value * 1000, cg_instabeam_time->value * 1000 * 0.4f, CG_MediaShader( cgs.media.shaderInstaBeam ), 128, 0 );

	// since autosprite doesn't work, spawn a second and rotate it 90 degrees
	cgpoly2 = CG_SpawnPolyBeam( start, end, tcolor, cg_instabeam_width->integer, cg_instabeam_time->value * 1000, cg_instabeam_time->value * 1000 * 0.4f, CG_MediaShader( cgs.media.shaderInstaBeam ), 128, 0 );
	cgpoly2->angles[ROLL] += 90;
}

/*
* CG_PLink
*/
void CG_PLink( vec3_t start, vec3_t end, vec4_t color, int flags )
{
	cpoly_t *cgpoly;

	cgpoly = CG_SpawnPolyBeam( start, end, color, 4, 2000.0f, 0.0f, CG_MediaShader( cgs.media.shaderLaser ), 64, 0 );
}

/*
* CG_Addpolys
*/
void CG_AddPolys( void )
{
	int i;
	float fade;
	cpoly_t	*cgpoly, *next, *hnode;
	poly_t *poly;
	static vec3_t angles;

	// add polys in first-spawned - first-drawn order
	hnode = &cg_polys_headnode;
	for( cgpoly = hnode->prev; cgpoly != hnode; cgpoly = next )
	{
		next = cgpoly->prev;

		// it's time to die
		if( cgpoly->die <= cg.time )
		{
			CG_FreePoly( cgpoly );
			continue;
		}

		poly = cgpoly->poly;

		for( i = 0; i < poly->numverts; i++ )
			VectorCopy( cgpoly->verts[i], poly->verts[i] );
		for( i = 0; i < 3; i++ )
			angles[i] = anglemod( cgpoly->angles[i] );

		CG_OrientPolygon( cgpoly->origin, angles, poly );

		// fade out
		if( cgpoly->fadetime < cg.time )
		{
			fade = ( cgpoly->die - cg.time ) * cgpoly->fadefreq;

			for( i = 0; i < poly->numverts; i++ )
			{
				poly->colors[i][0] = ( qbyte )( cgpoly->color[0] * fade * 255 );
				poly->colors[i][1] = ( qbyte )( cgpoly->color[1] * fade * 255 );
				poly->colors[i][2] = ( qbyte )( cgpoly->color[2] * fade * 255 );
				poly->colors[i][3] = ( qbyte )( cgpoly->color[3] * fade * 255 );
			}
		}

		trap_R_AddPolyToScene( poly );
	}
}
