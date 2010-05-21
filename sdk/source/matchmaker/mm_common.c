/*
   Copyright (C) 2007 Will Franklin.

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

static char loadkeyerr[1024];
#define seterror( err ) strncpy( loadkeyerr, err, sizeof( loadkeyerr ) )

const char *MM_LoadKeyError( void )
{
	return loadkeyerr;
}

qboolean MM_LoadKey( rsa_context *ctx, int mode, size_t keypart_offsets[], const char *filename )
{
	size_t *offset;
	char *original, *buffer, *c;
	int ret;

	if( !ctx || !keypart_offsets || !filename )
	{
		seterror( "Invalid parameters supplied" );
		return qfalse;
	}

	if( FS_LoadFile( filename, ( void ** )&buffer, NULL, 0 ) == -1 )
	{
		seterror( "Failed to load key file" );
		return qfalse;
	}

	original = buffer;

	memset( ctx, 0, sizeof( rsa_context ) );
	ctx->len = SIGNATURE_LEN;

	for( offset = keypart_offsets ; *offset ; offset++ )
	{
		c = COM_Parse( &buffer );
		if( !c )
		{
			FS_FreeFile( original );
			seterror( "Not enough data in key file" );
			return qfalse;
		}

		ret = mpi_read_string( ( mpi * )( ( qbyte * )ctx + *offset ), 16, c );
		if( ret )
		{
			FS_FreeFile( original );
			seterror( va( "Invalid data (%s). Error code %x", c, ret ) );
			return qfalse;
		}
	}

	FS_FreeFile( original );

	ret = ( mode == RSA_PRIVATE ? rsa_check_privkey( ctx ) : rsa_check_pubkey( ctx ) );
	if( ret )
	{
		seterror( va( "Invalid %s key file. Error code %x", mode == RSA_PRIVATE ? "private" : "public", ret ) );
		return qfalse;
	}

	return qtrue;
}
