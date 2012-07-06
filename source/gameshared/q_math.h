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

#ifndef GAME_QMATH_H
#define GAME_QMATH_H

#include "q_arch.h"

#ifdef __cplusplus
extern "C" {
#endif

//==============================================================
//
//MATHLIB
//
//==============================================================

#define	PITCH						0			// up / down
#define	YAW							1			// left / right
#define	ROLL						2			// fall over

#define	FORWARD						0
#define	RIGHT						1
#define	UP							2

typedef float vec_t;
typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];
typedef vec_t vec5_t[5];

typedef vec_t quat_t[4];

typedef vec_t dualquat_t[8];

typedef qbyte byte_vec4_t[4];

// 0-2 are axial planes
#define	PLANE_X		0
#define	PLANE_Y		1
#define	PLANE_Z		2
#define	PLANE_NONAXIAL	3

// cplane_t structure
typedef struct cplane_s
{
	vec3_t normal;
	float dist;
	short type;					// for fast side tests
	short signbits;				// signx + (signy<<1) + (signz<<1)
} cplane_t;

extern vec3_t vec3_origin;
extern vec3_t axis_identity[3];
extern quat_t quat_identity;

extern vec4_t colorBlack;
extern vec4_t colorRed;
extern vec4_t colorGreen;
extern vec4_t colorBlue;
extern vec4_t colorYellow;
extern vec4_t colorMagenta;
extern vec4_t colorCyan;
extern vec4_t colorWhite;
extern vec4_t colorLtGrey;
extern vec4_t colorMdGrey;
extern vec4_t colorDkGrey;
extern vec4_t colorOrange;

#define MAX_S_COLORS 10

extern vec4_t color_table[MAX_S_COLORS];

#define	nanmask ( 255<<23 )

#define	IS_NAN( x ) ( ( ( *(int *)&x )&nanmask ) == nanmask )

#ifndef M_PI
#define M_PI	   3.14159265358979323846   // matches value in gcc v2 math.h
#endif

#ifndef M_TWOPI
#define M_TWOPI	   6.28318530717958647692
#endif

#define DEG2RAD( a ) ( a * M_PI ) / 180.0F
#define RAD2DEG( a ) ( a * 180.0F ) / M_PI


// returns b clamped to [a..c] range
//#define bound(a,b,c) (max((a), min((b), (c))))

#ifndef max
#define max( a, b ) ( ( a ) > ( b ) ? ( a ) : ( b ) )
#endif

#ifndef min
#define min( a, b ) ( ( a ) < ( b ) ? ( a ) : ( b ) )
#endif

#define bound( a, b, c ) ( ( a ) >= ( c ) ? ( a ) : ( b ) < ( a ) ? ( a ) : ( b ) > ( c ) ? ( c ) : ( b ) )

// clamps a (must be lvalue) to [b..c] range
#define clamp( a, b, c ) ( ( b ) >= ( c ) ? ( a ) = ( b ) : ( a ) < ( b ) ? ( a ) = ( b ) : ( a ) > ( c ) ? ( a ) = ( c ) : ( a ) )

#define clamp_low( a, low ) ( ( a ) = ( a ) < ( low ) ? ( low ) : ( a ) )
#define clamp_high( a, high ) ( ( a ) = ( a ) > ( high ) ? ( high ) : ( a ) )

#define random()	( ( rand() & 0x7fff ) / ( (float)0x7fff ) )  // 0..1
#define brandom( a, b )	   ( ( a )+random()*( ( b )-( a ) ) )                // a..b
#define crandom()	brandom( -1, 1 )                           // -1..1

int	Q_rand( int *seed );
#define Q_random( seed )      ( ( Q_rand( seed ) & 0x7fff ) / ( (float)0x7fff ) )    // 0..1
#define Q_brandom( seed, a, b )	( ( a )+Q_random( seed )*( ( b )-( a ) ) )                      // a..b
#define Q_crandom( seed )     Q_brandom( seed, -1, 1 )

float	Q_RSqrt( float number );
int	Q_log2( int val );

int Q_bitcount( int v );

#define NEARESTEXPOF2(x)  ((int)floor( ( log( max( (x), 1 ) ) - log( 1.5 ) ) / log( 2 ) + 1 ))

#define SQRTFAST( x ) ( ( x ) * Q_RSqrt( x ) ) // jal : //The expression a * rsqrt(b) is intended as a higher performance alternative to a / sqrt(b). The two expressions are comparably accurate, but do not compute exactly the same value in every case. For example, a * rsqrt(a*a + b*b) can be just slightly greater than 1, in rare cases.

#define DotProduct( x, y )	   ( ( x )[0]*( y )[0]+( x )[1]*( y )[1]+( x )[2]*( y )[2] )
#define CrossProduct( v1, v2, cross ) ( ( cross )[0] = ( v1 )[1]*( v2 )[2]-( v1 )[2]*( v2 )[1], ( cross )[1] = ( v1 )[2]*( v2 )[0]-( v1 )[0]*( v2 )[2], ( cross )[2] = ( v1 )[0]*( v2 )[1]-( v1 )[1]*( v2 )[0] )

#define PlaneDiff( point, plane ) ( ( ( plane )->type < 3 ? ( point )[( plane )->type] : DotProduct( ( point ), ( plane )->normal ) ) - ( plane )->dist )

#define VectorSubtract( a, b, c )   ( ( c )[0] = ( a )[0]-( b )[0], ( c )[1] = ( a )[1]-( b )[1], ( c )[2] = ( a )[2]-( b )[2] )
#define VectorAdd( a, b, c )	    ( ( c )[0] = ( a )[0]+( b )[0], ( c )[1] = ( a )[1]+( b )[1], ( c )[2] = ( a )[2]+( b )[2] )
#define VectorCopy( a, b )	   ( ( b )[0] = ( a )[0], ( b )[1] = ( a )[1], ( b )[2] = ( a )[2] )
#define VectorClear( a )	  ( ( a )[0] = ( a )[1] = ( a )[2] = 0 )
#define VectorNegate( a, b )	   ( ( b )[0] = -( a )[0], ( b )[1] = -( a )[1], ( b )[2] = -( a )[2] )
#define VectorSet( v, x, y, z )	  ( ( v )[0] = ( x ), ( v )[1] = ( y ), ( v )[2] = ( z ) )
#define VectorAvg( a, b, c )	    ( ( c )[0] = ( ( a )[0]+( b )[0] )*0.5f, ( c )[1] = ( ( a )[1]+( b )[1] )*0.5f, ( c )[2] = ( ( a )[2]+( b )[2] )*0.5f )
#define VectorMA( a, b, c, d )	     ( ( d )[0] = ( a )[0]+( b )*( c )[0], ( d )[1] = ( a )[1]+( b )*( c )[1], ( d )[2] = ( a )[2]+( b )*( c )[2] )
#define VectorCompare( v1, v2 )	   ( ( v1 )[0] == ( v2 )[0] && ( v1 )[1] == ( v2 )[1] && ( v1 )[2] == ( v2 )[2] )
#define VectorLengthSquared( v )	( DotProduct( ( v ), ( v ) ) )
#define VectorLength( v )	  ( sqrt( VectorLengthSquared( v ) ) )
#define VectorInverse( v )	  ( ( v )[0] = -( v )[0], ( v )[1] = -( v )[1], ( v )[2] = -( v )[2] )
#define VectorLerp( a, c, b, v )     ( ( v )[0] = ( a )[0]+( c )*( ( b )[0]-( a )[0] ), ( v )[1] = ( a )[1]+( c )*( ( b )[1]-( a )[1] ), ( v )[2] = ( a )[2]+( c )*( ( b )[2]-( a )[2] ) )
#define VectorScale( in, scale, out ) ( ( out )[0] = ( in )[0]*( scale ), ( out )[1] = ( in )[1]*( scale ), ( out )[2] = ( in )[2]*( scale ) )

#define DistanceSquared( v1, v2 ) ( ( ( v1 )[0]-( v2 )[0] )*( ( v1 )[0]-( v2 )[0] )+( ( v1 )[1]-( v2 )[1] )*( ( v1 )[1]-( v2 )[1] )+( ( v1 )[2]-( v2 )[2] )*( ( v1 )[2]-( v2 )[2] ) )
#define Distance( v1, v2 ) ( sqrt( DistanceSquared( v1, v2 ) ) )

#define VectorLengthFast( v )	  ( SQRTFAST( DotProduct( ( v ), ( v ) ) ) )  // jal :  //The expression a * rsqrt(b) is intended as a higher performance alternative to a / sqrt(b). The two expressions are comparably accurate, but do not compute exactly the same value in every case. For example, a * rsqrt(a*a + b*b) can be just slightly greater than 1, in rare cases.
#define DistanceFast( v1, v2 )	   ( SQRTFAST( DistanceSquared( v1, v2 ) ) )  // jal :  //The expression a * rsqrt(b) is intended as a higher performance alternative to a / sqrt(b). The two expressions are comparably accurate, but do not compute exactly the same value in every case. For example, a * rsqrt(a*a + b*b) can be just slightly greater than 1, in rare cases.

#define Vector2Set( v, x, y )	  ( ( v )[0] = ( x ), ( v )[1] = ( y ) )
#define Vector2Copy( a, b )	   ( ( b )[0] = ( a )[0], ( b )[1] = ( a )[1] )
#define Vector2Avg( a, b, c )	    ( ( c )[0] = ( ( ( a[0] )+( b[0] ) )*0.5f ), ( c )[1] = ( ( ( a[1] )+( b[1] ) )*0.5f ) )

#define Vector4Set( v, a, b, c, d )   ( ( v )[0] = ( a ), ( v )[1] = ( b ), ( v )[2] = ( c ), ( v )[3] = ( d ) )
#define Vector4Clear( a )	  ( ( a )[0] = ( a )[1] = ( a )[2] = ( a )[3] = 0 )
#define Vector4Copy( a, b )	   ( ( b )[0] = ( a )[0], ( b )[1] = ( a )[1], ( b )[2] = ( a )[2], ( b )[3] = ( a )[3] )
#define Vector4Scale( in, scale, out )	    ( ( out )[0] = ( in )[0]*scale, ( out )[1] = ( in )[1]*scale, ( out )[2] = ( in )[2]*scale, ( out )[3] = ( in )[3]*scale )
#define Vector4Add( a, b, c )	    ( ( c )[0] = ( ( ( (a)[0] )+( (b)[0] ) ) ), ( c )[1] = ( ( ( (a)[1] )+( (b)[1] ) ) ), ( c )[2] = ( ( ( (a)[2] )+( (b)[2] ) ) ), ( c )[3] = ( ( ( (a)[3] )+( (b)[3] ) ) ) )
#define Vector4Avg( a, b, c )	    ( ( c )[0] = ( ( ( (a)[0] )+( (b)[0] ) )*0.5f ), ( c )[1] = ( ( ( (a)[1] )+( (b)[1] ) )*0.5f ), ( c )[2] = ( ( ( (a)[2] )+ (b)[2] ) )*0.5f ), ( c )[3] = ( ( ( (a)[3] )+( (b)[3] ) )*0.5f ) )
#define Vector4Negate( a, b )	   ( ( b )[0] = -( a )[0], ( b )[1] = -( a )[1], ( b )[2] = -( a )[2], ( b )[3] = -( a )[3] )
#define Vector4Inverse( v )			( ( v )[0] = -( v )[0], ( v )[1] = -( v )[1], ( v )[2] = -( v )[2], ( v )[3] = -( v )[3] )

vec_t VectorNormalize( vec3_t v );       // returns vector length
vec_t VectorNormalize2( const vec3_t v, vec3_t out );
void  VectorNormalizeFast( vec3_t v );

vec_t Vector4Normalize( vec4_t v );      // returns vector length

// just in case you do't want to use the macros
void _VectorMA( const vec3_t veca, float scale, const vec3_t vecb, vec3_t vecc );
vec_t _DotProduct( const vec3_t v1, const vec3_t v2 );
void _VectorSubtract( const vec3_t veca, const vec3_t vecb, vec3_t out );
void _VectorAdd( const vec3_t veca, const vec3_t vecb, vec3_t out );
void _VectorCopy( const vec3_t in, vec3_t out );

void ClearBounds( vec3_t mins, vec3_t maxs );
void AddPointToBounds( const vec3_t v, vec3_t mins, vec3_t maxs );
float RadiusFromBounds( const vec3_t mins, const vec3_t maxs );
qboolean BoundsIntersect( const vec3_t mins1, const vec3_t maxs1, const vec3_t mins2, const vec3_t maxs2 );
qboolean BoundsAndSphereIntersect( const vec3_t mins, const vec3_t maxs, const vec3_t centre, float radius );

#define NUMVERTEXNORMALS    162
int DirToByte( vec3_t dir );
void ByteToDir( int b, vec3_t dir );

void NormToLatLong( const vec3_t normal, qbyte latlong[2] );

void MakeNormalVectors( const vec3_t forward, vec3_t right, vec3_t up );
void AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up );
int BoxOnPlaneSide( const vec3_t emins, const vec3_t emaxs, const struct cplane_s *plane );
float anglemod( float a );
float LerpAngle( float a1, float a2, const float frac );
float AngleSubtract( float a1, float a2 );
void AnglesSubtract( vec3_t v1, vec3_t v2, vec3_t v3 );
float AngleNormalize360( float angle );
float AngleNormalize180( float angle );
float AngleDelta( float angle1, float angle2 );
void VecToAngles( const vec3_t vec, vec3_t angles );
void AnglesToAxis( const vec3_t angles, vec3_t axis[3] );
void NormalVectorToAxis( const vec3_t forward, vec3_t axis[3] );
void BuildBoxPoints( vec3_t p[8], const vec3_t org, const vec3_t mins, const vec3_t maxs );

vec_t ColorNormalize( const vec_t *in, vec_t *out );

#define ColorGrayscale(c) (0.299 * (c)[0] + 0.587 * (c)[1] + 0.114 * (c)[2])

float CalcFov( float fov_x, float width, float height );
void AdjustFov( float *fov_x, float *fov_y, float width, float height, qboolean lock_x );

#define Q_rint( x )   ( ( x ) < 0 ? ( (int)( ( x )-0.5f ) ) : ( (int)( ( x )+0.5f ) ) )

int SignbitsForPlane( const cplane_t *out );
int PlaneTypeForNormal( const vec3_t normal );
void CategorizePlane( cplane_t *plane );
void PlaneFromPoints( vec3_t verts[3], cplane_t *plane );

qboolean ComparePlanes( const vec3_t p1normal, vec_t p1dist, const vec3_t p2normal, vec_t p2dist );
void SnapVector( vec3_t normal );
void SnapPlane( vec3_t normal, vec_t *dist );

#define BOX_ON_PLANE_SIDE(emins, emaxs, p)	\
	(((p)->type < 3)?						\
	(										\
		((p)->dist <= (emins)[(p)->type])?	\
			1								\
		:									\
		(									\
			((p)->dist >= (emaxs)[(p)->type])?\
				2							\
			:								\
				3							\
		)									\
	)										\
	:										\
		BoxOnPlaneSide( (emins), (emaxs), (p)))

void ProjectPointOntoPlane( vec3_t dst, const vec3_t p, const vec3_t normal );
void PerpendicularVector( vec3_t dst, const vec3_t src );
void RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point, float degrees );
void ProjectPointOntoVector( const vec3_t point, const vec3_t vStart, const vec3_t vDir, vec3_t vProj );
float DistanceFromLineSquared(const vec3_t p, const vec3_t lp1, const vec3_t lp2, const vec3_t dir);
#define DistanceFromLine(p,lp1,lp2,dir) (sqrt(DistanceFromLineSquared(p,lp1,lp2,dir)))

void Matrix_Identity( vec3_t m[3] );
void Matrix_Copy( vec3_t m1[3], vec3_t m2[3] );
qboolean Matrix_Compare( vec3_t m1[3], vec3_t m2[3] );
void Matrix_Multiply( vec3_t m1[3], vec3_t m2[3], vec3_t out[3] );
void Matrix_TransformVector( vec3_t m[3], vec3_t v, vec3_t out );
void Matrix_Transpose( vec3_t in[3], vec3_t out[3] );
void Matrix_EulerAngles( vec3_t m[3], vec3_t angles );
void Matrix_Rotate( vec3_t m[3], vec_t angle, vec_t x, vec_t y, vec_t z );
void Matrix_FromPoints( vec3_t v1, vec3_t v2, vec3_t v3, vec3_t m[3] );

void Quat_Identity( quat_t q );
void Quat_Copy( const quat_t q1, quat_t q2 );
void Quat_Quat3( const vec3_t in, quat_t out );
qboolean Quat_Compare( const quat_t q1, const quat_t q2 );
void Quat_Conjugate( const quat_t q1, quat_t q2 );
vec_t Quat_DotProduct( const quat_t q1, const quat_t q2 );
vec_t Quat_Normalize( quat_t q );
vec_t Quat_Inverse( const quat_t q1, quat_t q2 );
void Quat_Multiply( const quat_t q1, const quat_t q2, quat_t out );
void Quat_Lerp( const quat_t q1, const quat_t q2, vec_t t, quat_t out );
void Quat_Vectors( const quat_t q, vec3_t f, vec3_t r, vec3_t u );
void Quat_Matrix( const quat_t q, vec3_t m[3] );
void Matrix_Quat( vec3_t m[3], quat_t q );
void Quat_TransformVector( const quat_t q, const vec3_t v, vec3_t out );
void Quat_ConcatTransforms( const quat_t q1, const vec3_t v1, const quat_t q2, const vec3_t v2, quat_t q, vec3_t v );

void DualQuat_Identity( dualquat_t dq );
void DualQuat_Copy( const dualquat_t in, dualquat_t out );
void DualQuat_FromAnglesAndVector( const vec3_t angles, const vec3_t v, dualquat_t out );
void DualQuat_FromMatrixAndVector( vec3_t m[3], const vec3_t v, dualquat_t out );
void DualQuat_FromQuatAndVector( const quat_t q, const vec3_t v, dualquat_t out );
void DualQuat_FromQuat3AndVector( const vec3_t q, const vec3_t v, dualquat_t out );
void DualQuat_GetVector( const dualquat_t dq, vec3_t v );
void DualQuat_ToQuatAndVector( const dualquat_t dq, quat_t q, vec3_t v );
void DualQuat_ToMatrixAndVector( const dualquat_t dq, vec3_t m[3], vec3_t v );
void DualQuat_Invert( dualquat_t dq );
vec_t DualQuat_Normalize( dualquat_t dq );
void DualQuat_Multiply( const dualquat_t dq1, const dualquat_t dq2, dualquat_t out );
void DualQuat_Lerp( const dualquat_t dq1, const dualquat_t dq2, vec_t t, dualquat_t out );

vec_t LogisticCDF( vec_t x );
vec_t LogisticPDF( vec_t x );
vec_t NormalCDF( vec_t x );
vec_t NormalPDF( vec_t x );

//========================================

#define _double2fixmagic (double)(68719476736.0*1.5);     //2^36 * 1.5,  (52-_shiftamt=36) uses limited precisicion to floor
#define _shiftamt        16;                    //16.16 fixed point representation,

static inline long _fast_ftol( double x )
{
#ifdef ENDIAN_LITTLE
	x = x + _double2fixmagic;
	return ((long*)&x)[0] >> _shiftamt;
#elif defined ( ENDIAN_BIG )
	x = x + _double2fixmagic;
	return ((long*)&x)[1] >> _shiftamt;
#else
	return (long)x;
#endif
}

static inline long fast_ftol( float x )
{
	return _fast_ftol( (double)x );
}

#ifdef __cplusplus
};
#endif

#endif // GAME_QMATH_H

