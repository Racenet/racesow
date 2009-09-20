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

//================
// AuthC_Auth
// Attemps to authenticate user, and replies with the outcome
//================
void AuthC_Auth( const socket_t *socket, const netadr_t *address )
{
	char email[MAX_EMAIL_LENGTH], pass[32 + 1]; // md5ed pass
	//int uid;

	Q_strncpyz( email, Cmd_Argv( 1 ), sizeof( email ) );
	//email = Cmd_Argv( 1 );
	Q_strncpyz( pass, Cmd_Argv( 2 ), sizeof( pass ) );
	//pass = Cmd_Argv( 2 );
	if( !*email || !*pass )
		return;

	// md5 hashes are 32 characters long right?
	if( strlen( pass ) != 32 )
	{
		Com_Printf( "Authentication request with possible plain password by %s\n", NET_AddressToString( address ) );
		return;
	}

	//uid = Auth_AuthenticateUser( email, pass );

	// user authentication has failed
/*	if( !uid )
		Netchan_OutOfBandPrint( socket, address, "auth invalid" );
	// success, send them their userid!
	else
		Netchan_OutOfBandPrint( socket, address, "auth valid %d", uid );*/
}
