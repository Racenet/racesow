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

#include "../game/q_shared.h"
#include "uicore_Global.h"

namespace UIWsw
{
	class ServerInfo
	{
	private:
		bool has_changed;
		bool ping_updated;
		static char *GetResponseToken( char **data_p );

	public:
		ServerInfo( char *adr, int batch );
		~ServerInfo();

		MEMORY_OPERATORS

		void setServerInfo( char *info );
		bool hasChanged( void ) const;
		bool pingUpdated( void ) const;
		void changeUpdated( void );
		bool isPingBatch(int querybatch);

		// Attributes
		std::string	hostname;
		std::string	map;
		int			curuser;
		int			maxuser;
		int			bots;
		std::string	gametype;
		bool		instagib;
		int			skilllevel;
		bool		password;
		bool		mm;
		unsigned int ping;
		unsigned int ping_retries;
		char		address[80];
		// Keep which refresh it's part of
		int refresh_batch;
	};
}

