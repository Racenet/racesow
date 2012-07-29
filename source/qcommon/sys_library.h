/*
Copyright (C) 2007 Pekka Lampila

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

#ifndef __SYS_LIBRARY_H
#define __SYS_LIBRARY_H

qboolean    Sys_Library_Close( void *lib );
void *Sys_Library_Open( const char *name );
void *Sys_Library_ProcAddress( void *lib, const char *apifuncname );
char *Sys_Library_ErrorString( void );

#endif // __SYS_LIBRARY_H
