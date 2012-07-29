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

#include "../qcommon/qcommon.h"

#include "../qcommon/sys_library.h"

#include <windows.h>

/*
* Sys_Library_Close
*/
qboolean Sys_Library_Close( void *lib )
{
	return FreeLibrary( (HINSTANCE)lib );
}

/*
* Sys_Library_Open
*/
void *Sys_Library_Open( const char *name )
{
	return (void *)LoadLibrary( name );
}

/*
* Sys_Library_ProcAddress
*/
void *Sys_Library_ProcAddress( void *lib, const char *apifuncname )
{
	return (void *)GetProcAddress( (HINSTANCE)lib, apifuncname );
}

/*
* Sys_Library_ErrorString
*/
char *Sys_Library_ErrorString( void )
{
	static char errbuf[80];

	int error = GetLastError();
	FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, LANG_SYSTEM_DEFAULT, errbuf, sizeof( errbuf ), NULL );

	return errbuf;
}
