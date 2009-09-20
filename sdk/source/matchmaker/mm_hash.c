/*
   Copyright (C) 2007 Will Franklin

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

#include "mm_common.h"
#include "mm_hash.h"

void MM_Hash( char *out, const char *in )
{
#ifdef _USE_SHA1
	unsigned char output[20];
	int i;

	*out = 0;

	sha1( ( unsigned char * )in, strlen( in ), output );

	for ( i = 0; i < 20; i++ )
		Q_strncatz( out, va( "%02x", output[i] ), HASH_SIZE );

	out[HASH_SIZE] = 0;

#elif defined ( _USE_MD5 )
	md5_byte_t digest[16];
	md5_state_t state;
	int i;

	md5_init( &state );
	md5_append( &state, ( md5_byte_t * )in, strlen( in ) );
	md5_finish( &state, digest );

	*out = 0;
	for( i = 0; i < 16; i++ )
		Q_strncatz( out, va( "%02x", digest[i] ), HASH_SIZE );

	out[HASH_SIZE] = 0;
#endif
}
