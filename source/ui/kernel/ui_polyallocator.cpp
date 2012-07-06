/*
 * PolyAllocator.cpp
 *
 *  Created on: 26.6.2011
 *      Author: hc
 */

#include "ui_precompiled.h"
#include "kernel/ui_common.h"
#include "kernel/ui_polyallocator.h"

PolyAllocator::PolyAllocator()
{
	memset( &poly_temp, 0, sizeof( poly_temp ) );
	poly_temp.normal[0] = poly_temp.normal[1] = 0.0;
	poly_temp.normal[2] = 1.0;
	base_temp = 0;
	size_temp = 0;
}

PolyAllocator::~PolyAllocator()
{
	// TODO Auto-generated destructor stub
	if( base_temp != 0 )
		__delete__( base_temp );
}

void PolyAllocator::assignPointers( poly_t *p, unsigned char *b )
{
	// rely that base is set
	p->verts = ( vec3_t* )b;
	p->stcoords = ( vec2_t* )( p->verts + p->numverts );
	p->colors = ( byte_vec4_t* )( p->stcoords + p->numverts );
}

size_t PolyAllocator::sizeForPolyData( int numverts, int numelems )
{
	return numverts * ( sizeof( vec3_t ) + sizeof( vec2_t ) + sizeof( byte_vec4_t ) );
}

poly_t *PolyAllocator::get_temp( int numverts, int numelems )
{
	size_t newsize;

	newsize = sizeForPolyData( numverts, numelems );
	if( size_temp < newsize || !base_temp )
	{
		if( base_temp != 0 ) {
			__delete__( base_temp );
		}

		base_temp = __newa__( unsigned char, newsize );
		size_temp = newsize;
	}

	poly_temp.numverts = numverts;
	assignPointers( &poly_temp, base_temp );
	return &poly_temp;
}

poly_t *PolyAllocator::alloc( int numverts, int numelems )
{
	size_t size;

	size = sizeForPolyData( numverts, numelems ) + sizeof( poly_t );
	unsigned char *base = __newa__( unsigned char, size );

	poly_t *poly = ( poly_t * )base;
	poly->numverts = numverts;
	assignPointers( poly, base + sizeof( poly_t ) );
	return poly;
}

void PolyAllocator::free( poly_t *poly )
{
	__delete__( poly );
}
