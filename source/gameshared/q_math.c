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

#include "q_math.h"
#include "q_shared.h"
#include "q_collision.h"

vec3_t vec3_origin = { 0, 0, 0 };
vec3_t axis_identity[3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };
quat_t quat_identity = { 0, 0, 0, 1 };

//============================================================================

vec3_t bytedirs[NUMVERTEXNORMALS] =
{
#include "anorms.h"
};

int DirToByte( vec3_t dir )
{
	int i, best;
	float d, bestd;
	qboolean normalized;

	if( !dir || VectorCompare( dir, vec3_origin ) )
		return NUMVERTEXNORMALS;

	if( DotProduct( dir, dir ) == 1 )
		normalized = qtrue;
	else
		normalized = qfalse;

	bestd = 0;
	best = 0;
	for( i = 0; i < NUMVERTEXNORMALS; i++ )
	{
		d = DotProduct( dir, bytedirs[i] );
		if( ( d == 1 ) && normalized )
			return i;
		if( d > bestd )
		{
			bestd = d;
			best = i;
		}
	}

	return best;
}

void ByteToDir( int b, vec3_t dir )
{
	if( b < 0 || b >= NUMVERTEXNORMALS )
		VectorSet( dir, 0, 0, 0 );
	else
		VectorCopy( bytedirs[b], dir );
}

//============================================================================

vec4_t colorBlack  = { 0, 0, 0, 1 };
vec4_t colorRed	   = { 1, 0, 0, 1 };
vec4_t colorGreen  = { 0, 1, 0, 1 };
vec4_t colorBlue   = { 0, 0, 1, 1 };
vec4_t colorYellow = { 1, 1, 0, 1 };
vec4_t colorOrange = { 1, 0.5, 0, 1 };
vec4_t colorMagenta = { 1, 0, 1, 1 };
vec4_t colorCyan   = { 0, 1, 1, 1 };
vec4_t colorWhite  = { 1, 1, 1, 1 };
vec4_t colorLtGrey = { 0.75, 0.75, 0.75, 1 };
vec4_t colorMdGrey = { 0.5, 0.5, 0.5, 1 };
vec4_t colorDkGrey = { 0.25, 0.25, 0.25, 1 };

vec4_t color_table[MAX_S_COLORS] =
{
	{ 0.0, 0.0, 0.0, 1.0 },
	{ 1.0, 0.0, 0.0, 1.0 },
	{ 0.0, 1.0, 0.0, 1.0 },
	{ 1.0, 1.0, 0.0, 1.0 },
	{ 0.0, 0.0, 1.0, 1.0 },
	{ 0.0, 1.0, 1.0, 1.0 },
	{ 1.0, 0.0, 1.0, 1.0 }, // magenta
	{ 1.0, 1.0, 1.0, 1.0 },
	{ 1.0, 0.5, 0.0, 1.0 }, // orange
	{ 0.5, 0.5, 0.5, 1.0 }, // grey
};

/*
* ColorNormalize
*/
vec_t ColorNormalize( const vec_t *in, vec_t *out )
{
	vec_t f = max( max( in[0], in[1] ), in[2] );

	if( f > 1.0f )
	{
		f = 1.0f / f;
		out[0] = in[0] * f;
		out[1] = in[1] * f;
		out[2] = in[2] * f;
	}
	else
	{
		out[0] = in[0];
		out[1] = in[1];
		out[2] = in[2];
	}

	return f;
}

//============================================================================

void NormToLatLong( const vec3_t normal, qbyte latlong[2] )
{
	// can't do atan2 (normal[1], normal[0])
	if( normal[0] == 0 && normal[1] == 0 )
	{
		if( normal[2] > 0 )
		{
			latlong[0] = 0; // acos ( 1 )
			latlong[1] = 0;
		}
		else
		{
			latlong[0] = 128; // acos ( -1 )
			latlong[1] = 0;
		}
	}
	else
	{
		int angle;

		angle = (int)( acos( normal[2] ) * 255.0 / M_TWOPI ) & 255;
		latlong[0] = angle;
		angle = (int)( atan2( normal[1], normal[0] ) * 255.0 / M_TWOPI ) & 255;
		latlong[1] = angle;
	}
}

void MakeNormalVectors( const vec3_t forward, vec3_t right, vec3_t up )
{
	float d;

	// this rotate and negate guarantees a vector not colinear with the original
	VectorSet( right, forward[2], -forward[0], forward[1] );
	d = DotProduct( right, forward );
	VectorMA( right, -d, forward, right );
	VectorNormalize( right );
	CrossProduct( right, forward, up );
}

void RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point, float degrees )
{
	float t0, t1;
	float c, s;
	vec3_t vr, vu, vf;

	s = DEG2RAD( degrees );
	c = cos( s );
	s = sin( s );

	VectorCopy( dir, vf );
	MakeNormalVectors( vf, vr, vu );

	t0 = vr[0] * c + vu[0] * -s;
	t1 = vr[0] * s + vu[0] *  c;
	dst[0] = ( t0 * vr[0] + t1 * vu[0] + vf[0] * vf[0] ) * point[0]
	         + ( t0 * vr[1] + t1 * vu[1] + vf[0] * vf[1] ) * point[1]
	         + ( t0 * vr[2] + t1 * vu[2] + vf[0] * vf[2] ) * point[2];

	t0 = vr[1] * c + vu[1] * -s;
	t1 = vr[1] * s + vu[1] *  c;
	dst[1] = ( t0 * vr[0] + t1 * vu[0] + vf[1] * vf[0] ) * point[0]
	         + ( t0 * vr[1] + t1 * vu[1] + vf[1] * vf[1] ) * point[1]
	         + ( t0 * vr[2] + t1 * vu[2] + vf[1] * vf[2] ) * point[2];

	t0 = vr[2] * c + vu[2] * -s;
	t1 = vr[2] * s + vu[2] *  c;
	dst[2] = ( t0 * vr[0] + t1 * vu[0] + vf[2] * vf[0] ) * point[0]
	         + ( t0 * vr[1] + t1 * vu[1] + vf[2] * vf[1] ) * point[1]
	         + ( t0 * vr[2] + t1 * vu[2] + vf[2] * vf[2] ) * point[2];
}

void AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up )
{
	float angle;
	static float sr, sp, sy, cr, cp, cy, t;
	// static to help MS compiler fp bugs

	angle = DEG2RAD( angles[YAW] );
	sy = sin( angle );
	cy = cos( angle );
	angle = DEG2RAD( angles[PITCH] );
	sp = sin( angle );
	cp = cos( angle );
	angle = DEG2RAD( angles[ROLL] );
	sr = sin( angle );
	cr = cos( angle );

	if( forward )
	{
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;
	}
	if( right )
	{
		t = sr*sp;
		right[0] = ( -1*t*cy+ -1*cr* -sy );
		right[1] = ( -1*t*sy+ -1*cr*cy );
		right[2] = -1*sr*cp;
	}
	if( up )
	{
		t = cr*sp;
		up[0] = ( t*cy+ -sr* -sy );
		up[1] = ( t*sy+ -sr*cy );
		up[2] = cr*cp;
	}
}

void VecToAngles( const vec3_t vec, vec3_t angles )
{
	float forward;
	float yaw, pitch;

	if( vec[1] == 0 && vec[0] == 0 )
	{
		yaw = 0;
		if( vec[2] > 0 )
			pitch = 90;
		else
			pitch = 270;
	}
	else
	{
		if( vec[0] )
			yaw = RAD2DEG( atan2( vec[1], vec[0] ) );
		else if( vec[1] > 0 )
			yaw = 90;
		else
			yaw = -90;
		if( yaw < 0 )
			yaw += 360;

		forward = sqrt( vec[0]*vec[0] + vec[1]*vec[1] );
		pitch = RAD2DEG( atan2( vec[2], forward ) );
		if( pitch < 0 )
			pitch += 360;
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
}

void AnglesToAxis( const vec3_t angles, vec3_t axis[3] )
{
	AngleVectors( angles, axis[0], axis[1], axis[2] );
	VectorInverse( axis[1] );
}

// similar to MakeNormalVectors but for rotational matrices
// (FIXME: weird, what's the diff between this and MakeNormalVectors?)
void NormalVectorToAxis( const vec3_t forward, vec3_t axis[3] )
{
	VectorCopy( forward, axis[0] );
	if( forward[0] || forward[1] )
	{
		VectorSet( axis[1], forward[1], -forward[0], 0 );
		VectorNormalize( axis[1] );
		CrossProduct( axis[0], axis[1], axis[2] );
	}
	else
	{
		VectorSet( axis[1], 1, 0, 0 );
		VectorSet( axis[2], 0, 1, 0 );
	}
}

void BuildBoxPoints( vec3_t p[8], const vec3_t org, const vec3_t mins, const vec3_t maxs )
{
	VectorAdd( org, mins, p[0] );
	VectorAdd( org, maxs, p[1] );
	VectorSet( p[2], p[0][0], p[0][1], p[1][2] );
	VectorSet( p[3], p[0][0], p[1][1], p[0][2] );
	VectorSet( p[4], p[0][0], p[1][1], p[1][2] );
	VectorSet( p[5], p[1][0], p[1][1], p[0][2] );
	VectorSet( p[6], p[1][0], p[0][1], p[1][2] );
	VectorSet( p[7], p[1][0], p[0][1], p[0][2] );
}

void ProjectPointOntoPlane( vec3_t dst, const vec3_t p, const vec3_t normal )
{
	float d;
	vec3_t n;
	float inv_denom;

	inv_denom = 1.0F / DotProduct( normal, normal );

	d = DotProduct( normal, p ) * inv_denom;

	n[0] = normal[0] * inv_denom;
	n[1] = normal[1] * inv_denom;
	n[2] = normal[2] * inv_denom;

	dst[0] = p[0] - d * n[0];
	dst[1] = p[1] - d * n[1];
	dst[2] = p[2] - d * n[2];
}

//
// assumes "src" is normalized
//
void PerpendicularVector( vec3_t dst, const vec3_t src )
{
	int pos;
	int i;
	float minelem = 1.0F;
	vec3_t tempvec;

	//
	// find the smallest magnitude axially aligned vector
	//
	for( pos = 0, i = 0; i < 3; i++ )
	{
		if( fabs( src[i] ) < minelem )
		{
			pos = i;
			minelem = fabs( src[i] );
		}
	}
	tempvec[0] = tempvec[1] = tempvec[2] = 0.0F;
	tempvec[pos] = 1.0F;

	//
	// project the point onto the plane defined by src
	//
	ProjectPointOntoPlane( dst, tempvec, src );

	//
	// normalize the result
	//
	VectorNormalize( dst );
}

/*
* ProjectPointOntoVector
*/
void ProjectPointOntoVector( const vec3_t point, const vec3_t vStart, const vec3_t vDir, vec3_t vProj )
{
	vec3_t pVec;

	VectorSubtract( point, vStart, pVec );
	// project onto the directional vector for this segment
	VectorMA( vStart, DotProduct( pVec, vDir ), vDir, vProj );
}

/*
* DistanceFromLineSquared
*/
float DistanceFromLineSquared(const vec3_t p, const vec3_t lp1, const vec3_t lp2, const vec3_t dir)
{
	vec3_t proj, t;
	int j;

	ProjectPointOntoVector(p, lp1, dir, proj);

	for (j = 0; j < 3; j++) {
		if ((proj[j] > lp1[j] && proj[j] > lp2[j]) ||
			(proj[j] < lp1[j] && proj[j] < lp2[j]))
			break;
	}

	if (j < 3) {
		if (fabs(proj[j] - lp1[j]) < fabs(proj[j] - lp2[j]))
			VectorSubtract(p, lp1, t);
		else
			VectorSubtract(p, lp2, t);
		return VectorLengthSquared(t);
	}

	VectorSubtract(p, proj, t);
	return VectorLengthSquared(t);
}

//============================================================================

float Q_RSqrt( float number )
{
	int i;
	float x2, y;

	if( number == 0.0 )
		return 0.0;

	x2 = number * 0.5f;
	y = number;
	i = *(int *) &y;    // evil floating point bit level hacking
	i = 0x5f3759df - ( i >> 1 ); // what the fuck?
	y = *(float *) &i;
	y = y * ( 1.5f - ( x2 * y * y ) ); // this can be done a second time

	return y;
}

int Q_rand( int *seed )
{
	*seed = *seed * 1103515245 + 12345;
	return ( (unsigned int)( *seed / 65536 ) % 32768 );
}

// found here: http://graphics.stanford.edu/~seander/bithacks.html
int Q_bitcount( int v )
{
	int c;

	v = v - ((v >> 1) & 0x55555555);                    // reuse input as temporary
	v = (v & 0x33333333) + ((v >> 2) & 0x33333333);     // temp
	c = (((v + (v >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24; // count

	return c;
}

/*
* LerpAngle
* 
*/
float LerpAngle( float a2, float a1, const float frac )
{
	if( a1 - a2 > 180 )
		a1 -= 360;
	if( a1 - a2 < -180 )
		a1 += 360;
	return a2 + frac * ( a1 - a2 );
}

/*
* AngleSubtract
* 
* Always returns a value from -180 to 180
*/
float AngleSubtract( float a1, float a2 )
{
	float a;

	a = a1 - a2;
	while( a > 180 )
	{
		a -= 360;
	}
	while( a < -180 )
	{
		a += 360;
	}
	return a;
}

/*
* AnglesSubtract
* 
* Always returns a value from -180 to 180
*/
void AnglesSubtract( vec3_t v1, vec3_t v2, vec3_t v3 )
{
	v3[0] = AngleSubtract( v1[0], v2[0] );
	v3[1] = AngleSubtract( v1[1], v2[1] );
	v3[2] = AngleSubtract( v1[2], v2[2] );
}

/*
* AngleNormalize360
* 
* returns angle normalized to the range [0 <= angle < 360]
*/
float AngleNormalize360( float angle )
{
	return ( 360.0 / 65536 ) * ( (int)( angle * ( 65536 / 360.0 ) ) & 65535 );
}

/*
* AngleNormalize180
* 
* returns angle normalized to the range [-180 < angle <= 180]
*/
float AngleNormalize180( float angle )
{
	angle = AngleNormalize360( angle );
	if( angle > 180.0 )
	{
		angle -= 360.0;
	}
	return angle;
}

/*
* AngleDelta
* 
* returns the normalized delta from angle1 to angle2
*/
float AngleDelta( float angle1, float angle2 )
{
	return AngleNormalize180( angle1 - angle2 );
}

/*
* anglemod
*/
float anglemod( float a )
{
	a = ( 360.0/65536 ) * ( (int)( a*( 65536/360.0 ) ) & 65535 );
	return a;
}

/*
* CalcFov
*/
float CalcFov( float fov_x, float width, float height )
{
	float x;

	if( fov_x < 1 || fov_x > 179 )
		Sys_Error( "Bad fov: %f", fov_x );

	x = width/tan( fov_x/360*M_PI );

	return atan( height/x )*360/M_PI;
}

/*
* AdjustFov
*/
void AdjustFov( float *fov_x, float *fov_y, float width, float height, qboolean lock_x )
{
	float x, y;

	if( width*3 == 4*height || width*4 == height*5 )
	{                                                   // 4:3 or 5:4 ratio
		return;
	}

	if( lock_x )
	{
		*fov_y = 2 *atan( ( width *3 ) / ( height *4 ) *tan ( *fov_y *M_PI / 360.0 *0.5 ) )*360/M_PI;
		return;
	}

	y = CalcFov( *fov_x, 640, 480 );
	x = *fov_x;

	*fov_x = CalcFov( y, height, width );
	if( *fov_x < x )
		*fov_x = x;
	else
		*fov_y = y;
}

/*
* BoxOnPlaneSide
* 
* Returns 1, 2, or 1 + 2
*/
int BoxOnPlaneSide( const vec3_t emins, const vec3_t emaxs, const struct cplane_s *p )
{
	float dist1, dist2;
	int sides;

	// general case
	switch( p->signbits )
	{
	case 0:
		dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		dist2 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		break;
	case 1:
		dist1 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		break;
	case 2:
		dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		dist2 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		break;
	case 3:
		dist1 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		break;
	case 4:
		dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		dist2 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		break;
	case 5:
		dist1 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		break;
	case 6:
		dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		dist2 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		break;
	case 7:
		dist1 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		break;
	default:
		dist1 = dist2 = 0; // shut up compiler
		assert( 0 );
		break;
	}

	sides = 0;
	if( dist1 >= p->dist )
		sides = 1;
	if( dist2 < p->dist )
		sides |= 2;

#if 0
	assert( sides != 0 );
#endif

	return sides;
}

/*
* SignbitsForPlane
*/
int SignbitsForPlane( const cplane_t *out )
{
	int bits, j;

	// for fast box on planeside test

	bits = 0;
	for( j = 0; j < 3; j++ )
	{
		if( out->normal[j] < 0 )
			bits |= 1<<j;
	}
	return bits;
}

/*
* PlaneTypeForNormal
*/
int PlaneTypeForNormal( const vec3_t normal )
{
	// NOTE: should these have an epsilon around 1.0?
	if( normal[0] >= 1.0 )
		return PLANE_X;
	if( normal[1] >= 1.0 )
		return PLANE_Y;
	if( normal[2] >= 1.0 )
		return PLANE_Z;

	return PLANE_NONAXIAL;
}

/*
* CategorizePlane
* 
* A slightly more complex version of SignbitsForPlane and PlaneTypeForNormal,
* which also tries to fix possible floating point glitches (like -0.00000 cases)
*/
void CategorizePlane( cplane_t *plane )
{
	int i;

	plane->signbits = 0;
	plane->type = PLANE_NONAXIAL;
	for( i = 0; i < 3; i++ )
	{
		if( plane->normal[i] < 0 )
		{
			plane->signbits |= 1<<i;
			if( plane->normal[i] == -1.0f )
			{
				plane->signbits = ( 1<<i );
				VectorClear( plane->normal );
				plane->normal[i] = -1.0f;
				break;
			}
		}
		else if( plane->normal[i] == 1.0f )
		{
			plane->type = i;
			plane->signbits = 0;
			VectorClear( plane->normal );
			plane->normal[i] = 1.0f;
			break;
		}
	}
}

/*
* PlaneFromPoints
*/
void PlaneFromPoints( vec3_t verts[3], cplane_t *plane )
{
	vec3_t v1, v2;

	VectorSubtract( verts[1], verts[0], v1 );
	VectorSubtract( verts[2], verts[0], v2 );
	CrossProduct( v2, v1, plane->normal );
	VectorNormalize( plane->normal );
	plane->dist = DotProduct( verts[0], plane->normal );
}

#define	PLANE_NORMAL_EPSILON	0.00001
#define	PLANE_DIST_EPSILON	0.01

/*
* ComparePlanes
*/
qboolean ComparePlanes( const vec3_t p1normal, vec_t p1dist, const vec3_t p2normal, vec_t p2dist )
{
	if( fabs( p1normal[0] - p2normal[0] ) < PLANE_NORMAL_EPSILON
	    && fabs( p1normal[1] - p2normal[1] ) < PLANE_NORMAL_EPSILON
	    && fabs( p1normal[2] - p2normal[2] ) < PLANE_NORMAL_EPSILON
	    && fabs( p1dist - p2dist ) < PLANE_DIST_EPSILON )
		return qtrue;

	return qfalse;
}

/*
* SnapVector
*/
void SnapVector( vec3_t normal )
{
	int i;

	for( i = 0; i < 3; i++ )
	{
		if( fabs( normal[i] - 1 ) < PLANE_NORMAL_EPSILON )
		{
			VectorClear( normal );
			normal[i] = 1;
			break;
		}
		if( fabs( normal[i] - -1 ) < PLANE_NORMAL_EPSILON )
		{
			VectorClear( normal );
			normal[i] = -1;
			break;
		}
	}
}

/*
* SnapPlane
*/
void SnapPlane( vec3_t normal, vec_t *dist )
{
	SnapVector( normal );

	if( fabs( *dist - Q_rint( *dist ) ) < PLANE_DIST_EPSILON )
	{
		*dist = Q_rint( *dist );
	}
}

void ClearBounds( vec3_t mins, vec3_t maxs )
{
	mins[0] = mins[1] = mins[2] = 99999;
	maxs[0] = maxs[1] = maxs[2] = -99999;
}

qboolean BoundsIntersect( const vec3_t mins1, const vec3_t maxs1, const vec3_t mins2, const vec3_t maxs2 )
{
	return (qboolean)( mins1[0] <= maxs2[0] && mins1[1] <= maxs2[1] && mins1[2] <= maxs2[2] &&
	                   maxs1[0] >= mins2[0] && maxs1[1] >= mins2[1] && maxs1[2] >= mins2[2] );
}

qboolean BoundsAndSphereIntersect( const vec3_t mins, const vec3_t maxs, const vec3_t centre, float radius )
{
	int i;
	float dmin = 0;
	float radius2 = radius * radius;

	for( i = 0; i < 3; i++ )
	{
		if( centre[i] < mins[i] )
			dmin += ( centre[i] - mins[i] ) * ( centre[i] - mins[i] );
		else if( centre[i] > maxs[i] )
			dmin += ( centre[i] - maxs[i] ) * ( centre[i] - maxs[i] );
	}

	if( dmin <= radius2 )
		return qtrue;
	return qfalse;
}

void AddPointToBounds( const vec3_t v, vec3_t mins, vec3_t maxs )
{
	int i;
	vec_t val;

	for( i = 0; i < 3; i++ )
	{
		val = v[i];
		if( val < mins[i] )
			mins[i] = val;
		if( val > maxs[i] )
			maxs[i] = val;
	}
}

/*
* RadiusFromBounds
*/
float RadiusFromBounds( const vec3_t mins, const vec3_t maxs )
{
	int i;
	vec3_t corner;

	for( i = 0; i < 3; i++ )
	{
		corner[i] = fabs( mins[i] ) > fabs( maxs[i] ) ? fabs( mins[i] ) : fabs( maxs[i] );
	}

	return VectorLength( corner );
}

vec_t VectorNormalize( vec3_t v )
{
	float length, ilength;

	length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];

	if( length )
	{
		length = sqrt( length ); // FIXME
		ilength = 1.0/length;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}

	return length;
}

vec_t VectorNormalize2( const vec3_t v, vec3_t out )
{
	float length, ilength;

	length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];

	if( length )
	{
		length = sqrt( length ); // FIXME
		ilength = 1.0/length;
		out[0] = v[0]*ilength;
		out[1] = v[1]*ilength;
		out[2] = v[2]*ilength;
	}
	else
	{
		VectorClear( out );
	}

	return length;
}

vec_t Vector4Normalize( vec4_t v )
{
	float length, ilength;

	length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2] + v[3]*v[3];

	if( length )
	{
		length = sqrt( length ); // FIXME
		ilength = 1.0/length;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
		v[3] *= ilength;
	}

	return length;
}

// fast vector normalize routine that does not check to make sure
// that length != 0, nor does it return length, uses rsqrt approximation
void VectorNormalizeFast( vec3_t v )
{
	float ilength = Q_RSqrt( DotProduct( v, v ) );

	v[0] *= ilength;
	v[1] *= ilength;
	v[2] *= ilength;
}

void _VectorMA( const vec3_t veca, float scale, const vec3_t vecb, vec3_t vecc )
{
	vecc[0] = veca[0] + scale*vecb[0];
	vecc[1] = veca[1] + scale*vecb[1];
	vecc[2] = veca[2] + scale*vecb[2];
}


vec_t _DotProduct( const vec3_t v1, const vec3_t v2 )
{
	return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

void _VectorSubtract( const vec3_t veca, const vec3_t vecb, vec3_t out )
{
	out[0] = veca[0]-vecb[0];
	out[1] = veca[1]-vecb[1];
	out[2] = veca[2]-vecb[2];
}

void _VectorAdd( const vec3_t veca, const vec3_t vecb, vec3_t out )
{
	out[0] = veca[0]+vecb[0];
	out[1] = veca[1]+vecb[1];
	out[2] = veca[2]+vecb[2];
}

void _VectorCopy( const vec3_t in, vec3_t out )
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

int Q_log2( int val )
{
	int answer = 0;
	while( val >>= 1 )
		answer++;
	return answer;
}

//============================================================================

void Matrix_Identity( vec3_t m[3] )
{
	int i, j;

	for( i = 0; i < 3; i++ )
		for( j = 0; j < 3; j++ )
			if( i == j )
				m[i][j] = 1.0;
			else
				m[i][j] = 0.0;
}

void Matrix_Copy( vec3_t m1[3], vec3_t m2[3] )
{
	int i, j;

	for( i = 0; i < 3; i++ )
		for( j = 0; j < 3; j++ )
			m2[i][j] = m1[i][j];
}

qboolean Matrix_Compare( vec3_t m1[3], vec3_t m2[3] )
{
	int i, j;

	for( i = 0; i < 3; i++ )
		for( j = 0; j < 3; j++ )
			if( m1[i][j] != m2[i][j] )
				return qfalse;
	return qtrue;
}

void Matrix_Multiply( vec3_t m1[3], vec3_t m2[3], vec3_t out[3] )
{
	out[0][0] = m1[0][0]*m2[0][0] + m1[0][1]*m2[1][0] + m1[0][2]*m2[2][0];
	out[0][1] = m1[0][0]*m2[0][1] + m1[0][1]*m2[1][1] + m1[0][2]*m2[2][1];
	out[0][2] = m1[0][0]*m2[0][2] + m1[0][1]*m2[1][2] + m1[0][2]*m2[2][2];
	out[1][0] = m1[1][0]*m2[0][0] + m1[1][1]*m2[1][0] + m1[1][2]*m2[2][0];
	out[1][1] = m1[1][0]*m2[0][1] + m1[1][1]*m2[1][1] + m1[1][2]*m2[2][1];
	out[1][2] = m1[1][0]*m2[0][2] + m1[1][1]*m2[1][2] + m1[1][2]*m2[2][2];
	out[2][0] = m1[2][0]*m2[0][0] + m1[2][1]*m2[1][0] + m1[2][2]*m2[2][0];
	out[2][1] = m1[2][0]*m2[0][1] + m1[2][1]*m2[1][1] + m1[2][2]*m2[2][1];
	out[2][2] = m1[2][0]*m2[0][2] + m1[2][1]*m2[1][2] + m1[2][2]*m2[2][2];
}

void Matrix_TransformVector( vec3_t m[3], vec3_t v, vec3_t out )
{
	out[0] = m[0][0]*v[0] + m[0][1]*v[1] + m[0][2]*v[2];
	out[1] = m[1][0]*v[0] + m[1][1]*v[1] + m[1][2]*v[2];
	out[2] = m[2][0]*v[0] + m[2][1]*v[1] + m[2][2]*v[2];
}

void Matrix_Transpose( vec3_t in[3], vec3_t out[3] )
{
	out[0][0] = in[0][0];
	out[1][1] = in[1][1];
	out[2][2] = in[2][2];

	out[0][1] = in[1][0];
	out[0][2] = in[2][0];
	out[1][0] = in[0][1];
	out[1][2] = in[2][1];
	out[2][0] = in[0][2];
	out[2][1] = in[1][2];
}

void Matrix_EulerAngles( vec3_t m[3], vec3_t angles )
{
	vec_t c;
	vec_t pitch, yaw, roll;

	pitch = -asin( m[0][2] );
	c = cos( pitch );
	if( fabs( c ) > 5*10e-6 )       // Gimball lock?
	{ // no
		c = 1.0f / c;
		pitch = RAD2DEG( pitch );
		yaw = RAD2DEG( atan2( m[0][1] * c, m[0][0] * c ) );
		roll = RAD2DEG( atan2( -m[1][2] * c, m[2][2] * c ) );
	}
	else
	{ // yes
		pitch = m[0][2] > 0 ? -90 : 90;
		yaw = RAD2DEG( atan2( m[1][0], -m[1][1] ) );
		roll = 180;
	}

	angles[PITCH] = pitch;
	angles[YAW] = yaw;
	angles[ROLL] = roll;
}

void Matrix_Rotate( vec3_t m[3], vec_t angle, vec_t x, vec_t y, vec_t z )
{
	vec3_t t[3], b[3];
	vec_t c = cos( DEG2RAD( angle ) );
	vec_t s = sin( DEG2RAD( angle ) );
	vec_t mc = 1 - c, t1, t2;

	t[0][0] = ( x * x * mc ) + c;
	t[1][1] = ( y * y * mc ) + c;
	t[2][2] = ( z * z * mc ) + c;

	t1 = y * x * mc;
	t2 = z * s;
	t[0][1] = t1 + t2;
	t[1][0] = t1 - t2;

	t1 = x * z * mc;
	t2 = y * s;
	t[0][2] = t1 - t2;
	t[2][0] = t1 + t2;

	t1 = y * z * mc;
	t2 = x * s;
	t[1][2] = t1 + t2;
	t[2][1] = t1 - t2;

	Matrix_Copy( m, b );
	Matrix_Multiply( b, t, m );
}

void Matrix_FromPoints( vec3_t v1, vec3_t v2, vec3_t v3, vec3_t m[3] )
{
	float d;

	m[2][0] = ( v1[1] - v2[1] ) * ( v3[2] - v2[2] ) - ( v1[2] - v2[2] ) * ( v3[1] - v2[1] );
	m[2][1] = ( v1[2] - v2[2] ) * ( v3[0] - v2[0] ) - ( v1[0] - v2[0] ) * ( v3[2] - v2[2] );
	m[2][2] = ( v1[0] - v2[0] ) * ( v3[1] - v2[1] ) - ( v1[1] - v2[1] ) * ( v3[0] - v2[0] );
	VectorNormalizeFast( m[2] );

	// this rotate and negate guarantees a vector not colinear with the original
	VectorSet( m[1], m[2][2], -m[2][0], m[2][1] );
	d = -DotProduct( m[1], m[2] );
	VectorMA( m[1], d, m[2], m[1] );
	VectorNormalizeFast( m[1] );
	CrossProduct( m[1], m[2], m[0] );
}

//============================================================================

void Quat_Identity( quat_t q )
{
	q[0] = 0;
	q[1] = 0;
	q[2] = 0;
	q[3] = 1;
}

void Quat_Copy( const quat_t q1, quat_t q2 )
{
	q2[0] = q1[0];
	q2[1] = q1[1];
	q2[2] = q1[2];
	q2[3] = q1[3];
}

void Quat_Quat3( const vec3_t in, quat_t out )
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
	out[3] = -sqrt(max(1 - in[0]*in[0] - in[1]*in[1] - in[2]*in[2], 0.0f));
}

qboolean Quat_Compare( const quat_t q1, const quat_t q2 )
{
	if( q1[0] != q2[0] || q1[1] != q2[1] || q1[2] != q2[2] || q1[3] != q2[3] )
		return qfalse;
	return qtrue;
}

void Quat_Conjugate( const quat_t q1, quat_t q2 )
{
	q2[0] = -q1[0];
	q2[1] = -q1[1];
	q2[2] = -q1[2];
	q2[3] = q1[3];
}

vec_t Quat_DotProduct( const quat_t q1, const quat_t q2 )
{
	return (q1[0] * q2[0] + q1[1] * q2[1] + q1[2] * q2[2] + q1[3] * q2[3]);
}

vec_t Quat_Normalize( quat_t q )
{
	vec_t length;

	length = q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];
	if( length != 0 )
	{
		vec_t ilength = 1.0 / sqrt( length );
		q[0] *= ilength;
		q[1] *= ilength;
		q[2] *= ilength;
		q[3] *= ilength;
	}

	return length;
}

vec_t Quat_Inverse( const quat_t q1, quat_t q2 )
{
	Quat_Conjugate( q1, q2 );

	return Quat_Normalize( q2 );
}

void Matrix_Quat( vec3_t m[3], quat_t q )
{
	vec_t tr, s;

	tr = m[0][0] + m[1][1] + m[2][2];
	if( tr > 0.00001 )
	{
		s = sqrt( tr + 1.0 );
		q[3] = s * 0.5; s = 0.5 / s;
		q[0] = ( m[2][1] - m[1][2] ) * s;
		q[1] = ( m[0][2] - m[2][0] ) * s;
		q[2] = ( m[1][0] - m[0][1] ) * s;
	}
	else
	{
		int i, j, k;

		i = 0;
		if( m[1][1] > m[0][0] ) i = 1;
		if( m[2][2] > m[i][i] ) i = 2;
		j = ( i + 1 ) % 3;
		k = ( i + 2 ) % 3;

		s = sqrt( m[i][i] - ( m[j][j] + m[k][k] ) + 1.0 );

		q[i] = s * 0.5; if( s != 0.0 ) s = 0.5 / s;
		q[j] = ( m[j][i] + m[i][j] ) * s;
		q[k] = ( m[k][i] + m[i][k] ) * s;
		q[3] = ( m[k][j] - m[j][k] ) * s;
	}

	Quat_Normalize( q );
}

void Quat_Multiply( const quat_t q1, const quat_t q2, quat_t out )
{
	out[0] = q1[3] * q2[0] + q1[0] * q2[3] + q1[1] * q2[2] - q1[2] * q2[1];
	out[1] = q1[3] * q2[1] + q1[1] * q2[3] + q1[2] * q2[0] - q1[0] * q2[2];
	out[2] = q1[3] * q2[2] + q1[2] * q2[3] + q1[0] * q2[1] - q1[1] * q2[0];
	out[3] = q1[3] * q2[3] - q1[0] * q2[0] - q1[1] * q2[1] - q1[2] * q2[2];
}

void Quat_Lerp( const quat_t q1, const quat_t q2, vec_t t, quat_t out )
{
	quat_t p1;
	vec_t omega, cosom, sinom, scale0, scale1, sinsqr;

	if( Quat_Compare( q1, q2 ) )
	{
		Quat_Copy( q1, out );
		return;
	}

	cosom = q1[0] * q2[0] + q1[1] * q2[1] + q1[2] * q2[2] + q1[3] * q2[3];
	if( cosom < 0.0 )
	{
		cosom = -cosom;
		p1[0] = -q1[0]; p1[1] = -q1[1];
		p1[2] = -q1[2]; p1[3] = -q1[3];
	}
	else
	{
		p1[0] = q1[0]; p1[1] = q1[1];
		p1[2] = q1[2]; p1[3] = q1[3];
	}

	if( cosom < 1.0 - 0.0001 )
	{
		sinsqr = 1.0 - cosom * cosom;
		sinom = Q_RSqrt( sinsqr );
		omega = atan2( sinsqr * sinom, cosom );
		scale0 = sin( ( 1.0 - t ) * omega ) * sinom;
		scale1 = sin( t * omega ) * sinom;
	}
	else
	{
		scale0 = 1.0 - t;
		scale1 = t;
	}

	out[0] = scale0 * p1[0] + scale1 * q2[0];
	out[1] = scale0 * p1[1] + scale1 * q2[1];
	out[2] = scale0 * p1[2] + scale1 * q2[2];
	out[3] = scale0 * p1[3] + scale1 * q2[3];
}

void Quat_Vectors( const quat_t q, vec3_t f, vec3_t r, vec3_t u )
{
	vec_t wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;

	x2 = q[0] + q[0]; y2 = q[1] + q[1]; z2 = q[2] + q[2];

	xx = q[0] * x2; yy = q[1] * y2; zz = q[2] * z2;
	f[0] = 1.0f - yy - zz; r[1] = 1.0f - xx - zz; u[2] = 1.0f - xx - yy;

	yz = q[1] * z2; wx = q[3] * x2;
	r[2] = yz - wx; u[1] = yz + wx;

	xy = q[0] * y2; wz = q[3] * z2;
	f[1] = xy - wz; r[0] = xy + wz;

	xz = q[0] * z2; wy = q[3] * y2;
	f[2] = xz + wy; u[0] = xz - wy;
}

void Quat_Matrix( const quat_t q, vec3_t m[3] )
{
	Quat_Vectors( q, m[0], m[1], m[2] );
}

void Quat_TransformVector( const quat_t q, const vec3_t v, vec3_t out )
{
	vec_t wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;

	x2 = q[0] + q[0]; y2 = q[1] + q[1]; z2 = q[2] + q[2];
	xx = q[0] * x2; xy = q[0] * y2; xz = q[0] * z2;
	yy = q[1] * y2; yz = q[1] * z2; zz = q[2] * z2;
	wx = q[3] * x2; wy = q[3] * y2; wz = q[3] * z2;

	out[0] = ( 1.0f - yy - zz ) * v[0] + ( xy - wz ) * v[1] + ( xz + wy ) * v[2];
	out[1] = ( xy + wz ) * v[0] + ( 1.0f - xx - zz ) * v[1] + ( yz - wx ) * v[2];
	out[2] = ( xz - wy ) * v[0] + ( yz + wx ) * v[1] + ( 1.0f - xx - yy ) * v[2];
}

void Quat_ConcatTransforms( const quat_t q1, const vec3_t v1, const quat_t q2, const vec3_t v2, quat_t q, vec3_t v )
{
	Quat_Multiply( q1, q2, q );
	Quat_TransformVector( q1, v2, v );
	v[0] += v1[0]; v[1] += v1[1]; v[2] += v1[2];
}

//============================================================================

void DualQuat_Identity( dualquat_t dq )
{
	Vector4Set( &dq[0], 0, 0, 0, 1 );
	Vector4Set( &dq[4], 0, 0, 0, 0 );
}

void DualQuat_Copy( const dualquat_t in, dualquat_t out )
{
	Quat_Copy( &in[0], &out[0] );
	Quat_Copy( &in[4], &out[4] );
}

static inline void DualQuat_SetVector( dualquat_t dq, const vec3_t v )
{
	// convert translation vector to dual part
	Vector4Set( &dq[4], 0.5f * (v[0]*dq[3] + v[1]*dq[2] - v[2]*dq[1]),
               0.5f*(-v[0]*dq[2] + v[1]*dq[3] + v[2]*dq[0]),
               0.5f*( v[0]*dq[1] - v[1]*dq[0] + v[2]*dq[3]),
              -0.5f*( v[0]*dq[0] + v[1]*dq[1] + v[2]*dq[2]) );
}

void DualQuat_FromAnglesAndVector( const vec3_t angles, const vec3_t v, dualquat_t out )
{
	vec3_t axis[3];

	AnglesToAxis( angles, axis );
	DualQuat_FromMatrixAndVector( axis, v, out );
}

void DualQuat_FromMatrixAndVector( vec3_t m[3], const vec3_t v, dualquat_t out )
{
	// regular matrix to a quaternion
	Matrix_Quat( m, &out[0] );

	// convert translation vector to dual part
	DualQuat_SetVector( out, v );
}

void DualQuat_FromQuatAndVector( const quat_t q, const vec3_t v, dualquat_t out )
{
	// regular quaternion, copy
	Quat_Copy( q, &out[0] );

	// convert translation vector to dual part
	DualQuat_SetVector( out, v );
}

void DualQuat_FromQuat3AndVector( const vec3_t q, const vec3_t v, dualquat_t out )
{
	// regular quaternion, copy
	Quat_Quat3( q, &out[0] );

	// convert translation vector to dual part
	DualQuat_SetVector( out, v );
}

void DualQuat_GetVector( const dualquat_t dq, vec3_t v )
{
	const vec_t *const real = &dq[0], *const dual = &dq[4];

	// translation vector
	CrossProduct( real, dual, v );
	VectorMA( v,  real[3], dual, v );
	VectorMA( v, -dual[3], real, v );
	VectorScale( v, 2, v );
}

void DualQuat_ToQuatAndVector( const dualquat_t dq, quat_t q, vec3_t v )
{
	// regular quaternion, copy
	Quat_Copy( &dq[0], q );

	// translation vector
	DualQuat_GetVector( dq, v );
}

void DualQuat_ToMatrixAndVector( const dualquat_t dq, vec3_t m[3], vec3_t v )
{
	// convert quaternion to matrix
	Quat_Matrix( &dq[0], m );

	// translation vector
	DualQuat_GetVector( dq, v );
}

void DualQuat_Invert( dualquat_t dq )
{
	vec_t s;
	vec_t *const real = &dq[0], *const dual = &dq[4];

	Quat_Conjugate( real, real );
	Quat_Conjugate( dual, dual );

	s = 2 * Quat_DotProduct( real, dual );
	dual[0] -= real[0] * s;
	dual[1] -= real[1] * s;
	dual[2] -= real[2] * s;
	dual[3] -= real[3] * s;
}

vec_t DualQuat_Normalize( dualquat_t dq )
{
	vec_t length;
	vec_t *const real = &dq[0], *const dual = &dq[4];

	length = real[0] * real[0] + real[1] * real[1] + real[2] * real[2] + real[3] * real[3];
	if( length != 0 )
	{
		vec_t ilength = 1.0 / sqrt( length );
		Vector4Scale( real, ilength, real );
		Vector4Scale( dual, ilength, dual );
	}

	return length;
}

void DualQuat_Multiply( const dualquat_t dq1, const dualquat_t dq2, dualquat_t out )
{
	quat_t tq1, tq2;

	Quat_Multiply( &dq1[0], &dq2[4], tq1 );
	Quat_Multiply( &dq1[4], &dq2[0], tq2 );

	Quat_Multiply( &dq1[0], &dq2[0], &out[0] );
	Vector4Set( &out[4], tq1[0] + tq2[0], tq1[1] + tq2[1], tq1[2] + tq2[2], tq1[3] + tq2[3] );
}

void DualQuat_Lerp( const dualquat_t dq1, const dualquat_t dq2, vec_t t, dualquat_t out )
{
	Quat_Lerp( &dq1[0], &dq2[0], t, &out[0] );
	Quat_Lerp( &dq1[4], &dq2[4], t, &out[4] );
}

/*
 * Distribution functions
 * Standard distribution is expected with mean=0, deviation=1
 */

vec_t LogisticCDF( vec_t x )
{
	return 1.0 / ( 1.0 + exp( -x ) );
}

vec_t LogisticPDF( vec_t x )
{
	float e;
	e = exp( -x );
	return e / ( (1.0+e) * (1.0+e) );
}

// closer approximation from
// http://www.wilmott.com/pdfs/090721_west.pdf
vec_t NormalCDF( vec_t x )
{
	float cumnorm = 0.0;
	float sign = 1.0;
	float build = 0;
	float e = 0.0;

	if ( x < 0.0 )
		sign = -1.0;
	x = abs(x);
	if( x > 37.0 )
		cumnorm = 0.0;
	else
	{
		e = expf( -(x*x) * 0.5 );
		if( x < 7.07106781186547 )
		{
			build = 3.52624965998911e-02 * x + 0.700383064443688;
			build = build * x + 6.37396220353165;
			build = build * x + 33.912866078383;
			build = build * x + 112.079291497871;
			build = build * x + 221.213596169931;
			build = build * x + 220.206867912376;
			cumnorm = e * build;
			build = 8.8388347683184e-02;
			build = build * x + 16.064177579207;
			build = build * x + 86.7807322029461;
			build = build * x + 296.564248779674;
			build = build * x + 637.333633378831;
			build = build * x + 793.826512519948;
			build = build * x + 440.413735824752;
			cumnorm /= build;
		}
		else
		{
			build = x + 0.65;
			build = x + 4 / build;
			build = x + 3 / build;
			build = x + 2 / build;
			build = x + 1 / build;
			cumnorm = e / build / 2.506628274631;
		}
	}

	if( sign > 0 )
		cumnorm = 1 - cumnorm;

	return cumnorm;
}

vec_t NormalPDF( vec_t x )
{
	return exp( (-x*x) / 2 ) / sqrt( 2.0 * M_PI );
}
