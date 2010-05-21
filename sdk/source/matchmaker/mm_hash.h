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

#ifndef _MM_HASH
#define _MM_HASH

#define _USE_SHA1
//#define _USE_MD5

#ifdef _USE_SHA1
#	include "../qcommon/sha1.h"
#	define HASH_SIZE 40
#elif defined ( _USE_MD5 )
#	include "../qcommon/md5.h"
#	define HASH_SIZE 32
#else
#	error No hash type defined
#endif

void MM_Hash( char *out, const char *in );

#endif
