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

#include "uiwsw_MapList.h"
#include "uicore_Global.h"
#include "uiwsw_SysCalls.h"
#include "uiwsw_Utils.h"

using namespace UIWsw;

struct shader_s *MapList::unknownMapPic = NULL;

void MapList::Precache( void )
{
	int i;
	char mapinfo[MAX_CONFIGSTRING_CHARS];
	const char *mapname;
	char screenshotname[MAX_CONFIGSTRING_CHARS];
	struct shader_s *unknownMapPic;

	unknownMapPic = MapList::GetUnknownMapPic();
	for( i = 0; ; i++ )
	{
	    if( !Trap::ML_GetMapByNum( i, mapinfo, sizeof( mapinfo ) ) )
			break;

		mapname = mapinfo;

		Q_snprintfz( screenshotname, sizeof( screenshotname ), "levelshots/%s.jpg", mapname );
		Trap::R_RegisterLevelshot( screenshotname, unknownMapPic, NULL ); 
	}
}

struct shader_s *MapList::GetUnknownMapPic( void )
{
	if( !MapList::unknownMapPic )
		MapList::unknownMapPic = Trap::R_RegisterPic( PATH_UKNOWN_MAP_PIC );
	return MapList::unknownMapPic;
}

