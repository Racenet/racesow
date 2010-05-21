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

// r_cin.c
#include "r_local.h"
#include "../client/cin.h"

#define MAX_CINEMATICS	256

typedef struct r_cinhandle_s
{
	unsigned int	id;
	char			*name;
	cinematics_t	*cin;
	image_t			*image;
	struct r_cinhandle_s *prev, *next;
} r_cinhandle_t;

static mempool_t *r_cinMemPool;

static r_cinhandle_t *r_cinematics;
static r_cinhandle_t r_cinematics_headnode, *r_free_cinematics;

#define Cin_Malloc(size) _Mem_Alloc(r_cinMemPool,size,0,0,__FILE__,__LINE__)
#define Cin_Free(data) Mem_Free(data)

/*
==================
R_ReadNextRoQFrame
==================
*/
static qbyte *R_ReadNextRoQFrame( cinematics_t *cin )
{
	roq_chunk_t *chunk = &cin->chunk;

	while( !FS_Eof( cin->file ) )
	{
		RoQ_ReadChunk( cin );

		if( FS_Eof( cin->file ) )
			return NULL;
		if( chunk->size <= 0 )
			continue;

		if( chunk->id == RoQ_INFO )
			RoQ_ReadInfo( cin );
		else if( chunk->id == RoQ_QUAD_VQ )
			return RoQ_ReadVideo( cin );
		else if( chunk->id == RoQ_QUAD_CODEBOOK )
			RoQ_ReadCodebook( cin );
		else
			RoQ_SkipChunk( cin );
	}

	return NULL;
}

/*
==================
R_RunRoQ
==================
*/
static void R_RunRoQ( cinematics_t *cin )
{
	unsigned int frame;

	frame = (Sys_Milliseconds () - cin->time) * (float)(RoQ_FRAMERATE) / 1000;
	if( frame <= cin->frame )
		return;
	if( frame > cin->frame + 1 )
		cin->time = Sys_Milliseconds () - cin->frame * 1000 / RoQ_FRAMERATE;

	cin->pic = cin->pic_pending;
	cin->pic_pending = R_ReadNextRoQFrame( cin );

	if( !cin->pic_pending )
	{
		FS_Seek( cin->file, cin->headerlen, FS_SEEK_SET );
		cin->frame = 0;
		cin->pic_pending = R_ReadNextRoQFrame( cin );
		cin->time = Sys_Milliseconds ();
	}

	cin->new_frame = qtrue;
}

/*
==================
R_StopRoQ
==================
*/
static void R_StopRoQ( cinematics_t *cin )
{
	cin->frame = 0;
	cin->time = 0;	// done
	cin->pic = NULL;
	cin->pic_pending = NULL;

	if( cin->file )
	{
		FS_FCloseFile( cin->file );
		cin->file = 0;
	}
	if( cin->name )
	{
		Mem_Free( cin->name );
		cin->name = NULL;
	}
	if( cin->vid_buffer )
	{
		Mem_Free( cin->vid_buffer );
		cin->vid_buffer = NULL;
	}
}

/*
==================
R_OpenCinematics
==================
*/
static cinematics_t *R_OpenCinematics( char *filename )
{
	int file = 0;
	cinematics_t *cin = NULL;
	roq_chunk_t *chunk = &cin->chunk;

	if( FS_FOpenFile( filename, &file, FS_READ ) == -1 )
		return NULL;

	cin = Cin_Malloc( sizeof( cinematics_t ) );
	memset( cin, 0, sizeof( cinematics_t ) );
	cin->name = filename;
	cin->file = file;
	cin->mempool = r_cinMemPool;

	// read header
	RoQ_ReadChunk( cin );

	chunk = &cin->chunk;
	if( chunk->id != RoQ_HEADER1 || chunk->size != RoQ_HEADER2 || chunk->argument != RoQ_HEADER3 )
	{
		R_StopRoQ( cin );
		Cin_Free( cin );
		return NULL;
	}

	cin->headerlen = FS_Tell( cin->file );
	cin->frame = 0;
	cin->pic = cin->pic_pending = R_ReadNextRoQFrame( cin );
	cin->time = Sys_Milliseconds ();
	cin->new_frame = qtrue;

	return cin;
}

/*
==================
R_ResampleCinematicFrame
==================
*/
static image_t *R_ResampleCinematicFrame( r_cinhandle_t *handle )
{
	image_t			*image;
	cinematics_t	*cin = handle->cin;

	if( !cin->pic )
		return NULL;

	if( !handle->image )
	{
		handle->image = R_LoadPic( handle->name, &cin->pic, cin->width, cin->height, IT_CINEMATIC, 3 );
		cin->new_frame = qfalse;
	}

	if( !cin->new_frame )
		return handle->image;

	cin->new_frame = qfalse;

	image = handle->image;
	GL_Bind( 0, image );
	if( image->width != cin->width || image->height != cin->height )
		R_Upload32( &cin->pic, image->width, image->height, IT_CINEMATIC, 
		&(image->upload_width), &(image->upload_height), &(image->samples), qfalse );
	else
		R_Upload32( &cin->pic, image->width, image->height, IT_CINEMATIC, 
		&(image->upload_width), &(image->upload_height), &(image->samples), qtrue );
	image->width = cin->width;
	image->height = cin->height;

	return image;
}

//==================================================================================

/*
==================
R_CinList_f
==================
*/
void R_CinList_f( void )
{
	cinematics_t *cin;
	image_t *image;
	r_cinhandle_t *handle, *hnode;

	Com_Printf( "Active cintematics:" );
	hnode = &r_cinematics_headnode;
	handle = hnode->prev;
	if( handle == hnode )
	{
		Com_Printf( " none\n" );
		return;
	}

	Com_Printf( "\n" );
	do {
		cin = handle->cin;
		image = handle->image;
		assert( cin );

		if( image && (cin->width != image->upload_width || cin->height != image->upload_height) )
			Com_Printf( "%s %i(%i)x%i(%i) f:%i\n", cin->name, cin->width, image->upload_width, cin->height, image->upload_height, cin->frame );
		else
			Com_Printf( "%s %ix%i f:%i\n", cin->name, cin->width, cin->height, cin->frame );

		handle = handle->next;
	} while( handle != hnode );
}

/*
==================
R_InitCinematics
==================
*/
void R_InitCinematics( void )
{
	int i;

	r_cinMemPool = Mem_AllocPool( NULL, "Cinematics" );

	r_cinematics = Cin_Malloc( sizeof( r_cinhandle_t ) * MAX_CINEMATICS );
	memset( r_cinematics, 0, sizeof( r_cinhandle_t ) * MAX_CINEMATICS );

	// link cinemtics
	r_free_cinematics = r_cinematics;
	r_cinematics_headnode.id = 0;
	r_cinematics_headnode.prev = &r_cinematics_headnode;
	r_cinematics_headnode.next = &r_cinematics_headnode;
	for( i = 0; i < MAX_CINEMATICS - 1; i++ )
	{
		if( i < MAX_CINEMATICS - 1 )
			r_cinematics[i].next = &r_cinematics[i+1];
		r_cinematics[i].id = i + 1;
	}

	Cmd_AddCommand( "cinlist", R_CinList_f );
}

/*
==================
R_RunAllCinematics
==================
*/
void R_RunAllCinematics( void )
{
	r_cinhandle_t *handle, *hnode, *next;

	hnode = &r_cinematics_headnode;
	for( handle = hnode->prev; handle != hnode; handle = next )
	{
		next = handle->prev;
		R_RunRoQ( handle->cin );
	}
}

/*
==================
R_UploadCinematics
==================
*/
image_t *R_UploadCinematics( unsigned int id )
{
	assert( id > 0 && id <= MAX_CINEMATICS );
	return R_ResampleCinematicFrame( r_cinematics + id - 1 );
}

/*
==================
R_StartCinematic
==================
*/
unsigned int R_StartCinematics( const char *arg )
{
	char *name = NULL, uploadName[128];
	size_t name_size;
	cinematics_t *cin = NULL;
	r_cinhandle_t *handle, *hnode, *next;

	name_size = strlen( "video/" ) + strlen( arg ) + strlen( ".roq" ) + 1;
	name = Cin_Malloc( name_size );
	Q_snprintfz( name, name_size, "video/%s", arg );
	COM_DefaultExtension( name, ".roq", name_size );

	// find cinematics with the same name
	hnode = &r_cinematics_headnode;
	for( handle = hnode->prev; handle != hnode; handle = next )
	{
		next = handle->prev;
		assert( handle->cin );

		// reuse
		if( !Q_stricmp( handle->cin->name, name ) )
		{
			Cin_Free( name );
			return handle->id;
		}
	}

	// open the file, read header, etc
	cin = R_OpenCinematics( name );

	// take a free cinematic handle if possible
	if( !r_free_cinematics || !cin )
	{
		Cin_Free( name );
		return 0;
	}

	handle = r_free_cinematics;
	r_free_cinematics = handle->next;

	Q_snprintfz( uploadName, sizeof( uploadName ), "***r_cinematic%i***", handle->id-1 );
	name_size = strlen( uploadName ) + 1;
	handle->name = Cin_Malloc( name_size );
	memcpy( handle->name, uploadName, name_size );
	handle->cin = cin;

	// put handle at the start of the list
	handle->prev = &r_cinematics_headnode;
	handle->next = r_cinematics_headnode.next;
	handle->next->prev = handle;
	handle->prev->next = handle;

	return handle->id;
}

/*
=================
R_FreeCinematics
=================
*/
void R_FreeCinematics( unsigned int id )
{
	r_cinhandle_t *handle;

	handle = r_cinematics + id - 1;
	if( !handle->cin )
		return;

	R_StopRoQ( handle->cin );
	Cin_Free( handle->cin );
	handle->cin = NULL;

	assert( handle->name );
	Cin_Free( handle->name );
	handle->name = NULL;

	// remove from linked active list
	handle->prev->next = handle->next;
	handle->next->prev = handle->prev;

	// insert into linked free list
	handle->next = r_free_cinematics;
	r_free_cinematics = handle;
}

/*
==================
R_ShutdownCinematics
==================
*/
void R_ShutdownCinematics( void )
{
	r_cinhandle_t *handle, *hnode, *next;

	if( !r_cinMemPool )
		return;

	hnode = &r_cinematics_headnode;
	for( handle = hnode->prev; handle != hnode; handle = next )
	{
		next = handle->prev;
		R_FreeCinematics( handle->id );
	}

	Cin_Free( r_cinematics );
	Mem_FreePool( &r_cinMemPool );

	Cmd_RemoveCommand( "cinlist" );
}
