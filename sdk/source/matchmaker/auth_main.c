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

#include "matchmaker.h"
#include "auth.h"

static mempool_t *auth_mempool;
static qboolean auth_initialized = qfalse;

//=================
// Auth_Init
// Initialize auth system
//=================
void Auth_Init( void )
{
	if( auth_initialized )
		return;

	auth_mempool = Mem_AllocPool( NULL, "Authentication" );

	auth_initialized = qtrue;
}

//=================
// Auth_Shutdown
// Cleanup the auth system
//=================
void Auth_Shutdown( void )
{
	if( !auth_initialized )
		return;

	Mem_FreePool( &auth_mempool );

	auth_initialized = qfalse;
}

//=================
// Auth_GetUserInfo
//=================
qboolean Auth_GetUserInfo( int *id, char *user, int ulen, char *pass, int plen )
{
	/*char query[MAX_QUERY_SIZE];
	MYSQL_RES *res;
	MYSQL_ROW row;

	if( !id && !user && !*id && !*user )
		return qfalse;

	if( *id )
		Q_snprintfz( query, sizeof( query ), "SELECT `id`, `user`, `pass` FROM `%s` WHERE `id`='%d'", DBTABLE_USERS, *id );
	else
		Q_snprintfz( query, sizeof( query ), "SELECT `id`, `user`, `pass` FROM `%s` WHERE `user`='%s'", DBTABLE_USERS, user );

	if( DB_Query( db_handle, query ) != DB_SUCCESS )
		return qfalse;

	if( DB_FetchResult( db_handle, &res ) != DB_SUCCESS )
		return qfalse;

	if( DB_FetchRow( res, &row, 0 ) != DB_SUCCESS ) 
		return qfalse;

	if( id ) *id = atoi( row[0] );
	if( user && ulen ) Q_strncpyz( user, row[1], ulen );
	if( pass && plen ) Q_strncpyz( pass, row[2], plen );*/
	return qtrue;
}
