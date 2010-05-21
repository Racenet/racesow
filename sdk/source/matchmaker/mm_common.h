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

#ifndef _MM_COMMON
#define _MM_COMMON

#include "../qcommon/qcommon.h"
#include "../qcommon/rsa.h"

#define SALT_LEN 15
#define SIGNATURE_LEN 128

#define MAX_MAXCLIENTS 24
#define MAX_TIMELIMIT 20.0f
#define MAX_SCORELIMIT 50

#define MM_HEARTBEAT_SECONDS 300

//===============
// mmserver cvar
//===============
#define MM_SERVER_IP "warsow.net:46002"

//===============
// mm_packet_t
//===============
typedef struct mm_packet_s
{
	msg_t msg;
	struct mm_packet_s *next;
} mm_packet_t;

//===============
// mm_type_t
//===============
typedef enum
{
	TYPE_ANY,
	TYPE_DEPENDENT,

	TYPE_TOTAL
} mm_type_t;

//===============
// mm_common.c
//===============
#define CTXOFS( x ) (size_t)&( ( ( rsa_context * )0 )->x )

const char *MM_LoadKeyError( void );
qboolean MM_LoadKey( rsa_context *ctx, int mode, size_t keypart_offsets[], const char *filename );

#endif
