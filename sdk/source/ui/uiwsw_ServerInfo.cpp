/*
Copyright (C) 2002-2008 The Warsow devteam

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

#include "uiwsw_Utils.h"
#include "uiwsw_ServerInfo.h"

using namespace UIWsw;
using namespace UICore;

ServerInfo::ServerInfo( char *adr, int pingbatch )
{
	// fill in fields with empty msgs, in case any of them isn't received
	hostname = "Unnamed Server";
	map = "Unknown";
	gametype = "Unknown";
	instagib = false;
	curuser = -1;
	maxuser = -1;
	skilllevel = 1;
	password = false;
	bots = 0;
	ping = 9999;
	ping_retries = 0;
	has_changed = true;
	ping_updated = false;
	refresh_batch = pingbatch;
	Q_strncpyz( address, adr, sizeof(address) );
}

ServerInfo::~ServerInfo()
{
}

ALLOCATOR_DEFINITION(ServerInfo)
DELETER_DEFINITION(ServerInfo)

bool ServerInfo::hasChanged( void ) const
{
	return has_changed;
}

bool ServerInfo::pingUpdated( void ) const
{
	return ping_updated;
}

void ServerInfo::changeUpdated( void )
{
	has_changed = false;
	ping_updated = false;
}

bool ServerInfo::isPingBatch(int querybatch)
{
	return (refresh_batch == querybatch);
}
/*
void ServerInfo::getMoreInfo(void)
{
	//
}
*/

void ServerInfo::setServerInfo( char *info )
{
	int		l;
	char	*pntr;
	char	*tok;
	char	buffer[256];
	// parse the info string
	pntr = info;
	if( strchr(pntr, '\\') == NULL ) // temp: check for old formatting
	{
		pntr = info;
		l = strlen(pntr) - 15;
		Q_snprintfz( buffer, l, "%s", pntr );
		buffer[l] = '\0';
		hostname = buffer;
		pntr += l;
		l = strlen(pntr) - 6;
		Q_snprintfz( buffer, l, "%s", pntr );
		buffer[l] = '\0';
		map = buffer;
		pntr += l;
		sscanf( pntr, "%d/%d", &curuser, &maxuser );
	}
	else
	{ // new formatting
		pntr = info;

		while( &pntr )
		{
			tok = GetResponseToken( &pntr );
			if( !tok || !strlen(tok) || !Q_stricmp(tok, "EOT") )
				break;

			// got cmd n : server hostname
			if( !Q_stricmp(tok, "n") )
			{
				tok = GetResponseToken( &pntr );
				if( !tok || !strlen(tok) || !Q_stricmp(tok, "EOT") )
					break;

				if( Q_stricmp( hostname.c_str(), tok ) ) {
					has_changed = true;
					hostname = tok;
				}
				continue;
			}

			// got cmd m : map name
			if( !Q_stricmp(tok, "m") )
			{
				tok = GetResponseToken( &pntr );
				if( !tok || !strlen(tok) || !Q_stricmp(tok, "EOT") )
					break;

				while( *tok == ' ' ) tok++; // remove spaces in front of the gametype name

				if( Q_stricmp( map.c_str(), tok ) ) {
					has_changed = true;
					map = tok;
				}
				continue;
			}

			// got cmd u : curuser/maxuser
			if( !Q_stricmp(tok, "u") )
			{
				int tmpcur, tmpmax;
				tok = GetResponseToken( &pntr );
				if( !tok || !strlen(tok) || !Q_stricmp(tok, "EOT") )
					break;

				sscanf( tok, "%d/%d", &tmpcur, &tmpmax );
				if( tmpcur != curuser || tmpmax != maxuser ) {
					has_changed = true;
					curuser = tmpcur;
					maxuser = tmpmax;
				}
				continue;
			}

			// got cmd b : bot count
			if( !Q_stricmp(tok, "b") ) {
				int tmpbots;
				tok = GetResponseToken( &pntr );
				if( !tok || !strlen(tok) || !Q_stricmp(tok, "EOT") )
					break;
				tmpbots = atoi(tok);
				if( tmpbots != bots ) {
					has_changed = true;
					bots = tmpbots;
				}
				continue;
			}

			// got cmd g : gametype (as string, starting by i for instagib)
			if( !Q_stricmp(tok, "g") ) {
				tok = GetResponseToken( &pntr );
				if( !tok || !strlen(tok) || !Q_stricmp(tok, "EOT") )
					break;

				while( *tok == ' ' ) tok++; // remove spaces in front of the gametype name

				if( Q_stricmp( gametype.c_str(), tok ) ) {
					has_changed = true;
					gametype = tok;
				}
				continue;
			}

			// got cmd ig : instagib
			if( !Q_stricmp(tok, "ig") ) {
				bool tmpinsta;

				tok = GetResponseToken( &pntr );
				if( !tok || !strlen(tok) || !Q_stricmp(tok, "EOT") )
					break;

				tmpinsta = (qboolean)(atoi(tok) != 0);

				if( instagib != tmpinsta ) {
					has_changed = true;
					instagib = tmpinsta;
				}
				continue;
			}

			// got cmd s : skill level
			if( !Q_stricmp(tok, "s") ) {
				int tmpskill;
				tok = GetResponseToken( &pntr );
				if( !tok || !strlen(tok) || !Q_stricmp(tok, "EOT") )
					break;
				tmpskill = atoi(tok);
				if( tmpskill != skilllevel ) {
					has_changed = true;
					skilllevel = tmpskill;
				}
				continue;
			}

			// got cmd p : password
			if( !Q_stricmp(tok, "p") ) {
				bool tmppwd;
				tok = GetResponseToken( &pntr );
				if( !tok || !strlen(tok) || !Q_stricmp(tok, "EOT") )
					break;
				tmppwd = atoi(tok) != 0;
				if( tmppwd != password ) {
					has_changed = true;
					password = tmppwd;
				}
				continue;
			}

			// got cmd ping : ping value
			if( !Q_stricmp(tok, "ping") ) {
				int tmpping;
				tok = GetResponseToken( &pntr );
				if( !tok || !strlen(tok) || !Q_stricmp(tok, "EOT") )
					break;
				tmpping = (atoi(tok));
				if( tmpping != (int) ping || ping_retries == 0u ) {
					has_changed = true;
					ping_updated = true;
					ping = tmpping;
					if( ping > 999 )
						ping = 999;
				}
				continue;
			}

			// Server available for matchmaking?
			if( !Q_stricmp(tok, "mm") ) {
				bool tmpmm;
				tok = GetResponseToken( &pntr );
				if( !tok || !strlen(tok) || !Q_stricmp(tok, "EOT") )
					break;
				tmpmm = (atoi(tok) != 0);
				if ( tmpmm != mm) {
					has_changed = true;
					mm = tmpmm;
				}
				continue;
			}
			UI_Printf( "ServerInfo::setServerInfo(%s): Unknown token:\"%s\"\n", address, tok );
		}
	}
}

char *ServerInfo::GetResponseToken( char **data_p )
{
	static char ui_responseToken[MAX_TOKEN_CHARS];
	int		c;
	int		len;
	unsigned backlen;
	char	*data;

	data = *data_p;
	len = 0;
	ui_responseToken[0] = 0;

	if( !data )
	{
		*data_p = NULL;
		return "";
	}

	backlen = strlen( data );
	if( backlen < strlen("\\EOT") )
	{
		*data_p = NULL;
		return "";
	}

skipbackslash:
	c = *data;
	if( c == '\\' ) {
		if( data[1] == '\\' ) {
			data += 2;
			goto skipbackslash;
		}
	}

	if( !c ) {
		*data_p = NULL;
		return "";
	}

	do
	{
		if( len < MAX_TOKEN_CHARS ) {
			ui_responseToken[len] = c;
			len++;
		}
		data++;
		c = *data;
	}while( c && c != '\\' );

	if( len == MAX_TOKEN_CHARS ) {
		len = 0;
	}
	ui_responseToken[len] = 0;

	*data_p = data;
	return ui_responseToken;
}
