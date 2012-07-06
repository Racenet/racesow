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
	int				registration_sequence;
	char			*name;
	char			*uploadName;
	struct cinematics_s	*cin;
	image_t			*image;
	int				width, height;
	qbyte			*pic;
	qboolean		new_frame;
	struct r_cinhandle_s *prev, *next;
} r_cinhandle_t;

static mempool_t *r_cinMemPool;

static r_cinhandle_t *r_cinematics;
static r_cinhandle_t r_cinematics_headnode, *r_free_cinematics;

#define Cin_Malloc(size) _Mem_Alloc(r_cinMemPool,size,0,0,__FILE__,__LINE__)
#define Cin_Free(data) Mem_Free(data)

/*
* R_RunCin
*/
static qboolean R_RunCin( r_cinhandle_t *h )
{
	qboolean redraw;

	if( !CIN_NeedNextFrame( h->cin, Sys_Milliseconds() ) )
		return qfalse;

	h->pic = CIN_ReadNextFrame( h->cin, &h->width, &h->height, NULL, NULL, &redraw );
	return redraw;
}

/*
* R_ResampleCinematicFrame
*/
static image_t *R_ResampleCinematicFrame( r_cinhandle_t *handle )
{
	image_t			*image;

	if( !handle->pic )
		return NULL;

	if( !handle->image )
	{
		handle->image = R_LoadPic( handle->name, &handle->pic, handle->width, handle->height, IT_CINEMATIC, 4 );
		handle->new_frame = qfalse;
	}

	if( !handle->new_frame )
		return handle->image;

	handle->new_frame = qfalse;

	image = handle->image;
	GL_Bind( 0, image );
	if( image->width != handle->width || image->height != handle->height )
		R_Upload32( &handle->pic, image->width, image->height, IT_CINEMATIC, 
		&(image->upload_width), &(image->upload_height), &(image->samples), image->samples, qfalse );
	else
		R_Upload32( &handle->pic, image->width, image->height, IT_CINEMATIC, 
		&(image->upload_width), &(image->upload_height), &(image->samples), image->samples, qtrue );
	image->width = handle->width;
	image->height = handle->height;

	return image;
}

//==================================================================================

/*
* R_CinList_f
*/
void R_CinList_f( void )
{
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
		assert( handle->cin );

		image = handle->image;
		if( image && (handle->width != image->upload_width || handle->height != image->upload_height) )
			Com_Printf( "%s %i(%i)x%i(%i)\n", handle->name, handle->width, image->upload_width, handle->height, image->upload_height );
		else
			Com_Printf( "%s %ix%i\n", handle->name, handle->width, handle->height );

		handle = handle->next;
	} while( handle != hnode );
}

/*
* R_InitCinematics
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
* R_RunAllCinematic
*/
void R_RunAllCinematics( void )
{
	r_cinhandle_t *handle, *hnode, *next;

	hnode = &r_cinematics_headnode;
	for( handle = hnode->prev; handle != hnode; handle = next )
	{
		next = handle->prev;
		handle->new_frame = R_RunCin( handle );
	}
}

/*
* R_UploadCinematic
*/
image_t *R_UploadCinematic( unsigned int id )
{
	assert( id > 0 && id <= MAX_CINEMATICS );
	if( id == 0 || id > MAX_CINEMATICS ) {
		return NULL;
	}
	return R_ResampleCinematicFrame( r_cinematics + id - 1 );
}

/*
* R_StartCinematic
*/
unsigned int R_StartCinematic( const char *arg )
{
	char uploadName[128];
	size_t name_size;
	r_cinhandle_t *handle, *hnode, *next;
	struct cinematics_s *cin;

	// find cinematics with the same name
	hnode = &r_cinematics_headnode;
	for( handle = hnode->prev; handle != hnode; handle = next )
	{
		next = handle->prev;
		assert( handle->cin );

		// reuse
		if( !Q_stricmp( handle->name, arg ) )
			return handle->id;
	}

	// open the file, read header, etc
	cin = CIN_Open( arg, Sys_Milliseconds(), CIN_LOOP|CIN_NOAUDIO );

	// take a free cinematic handle if possible
	if( !r_free_cinematics || !cin )
		return 0;

	handle = r_free_cinematics;
	r_free_cinematics = handle->next;

	// copy name
	name_size = strlen( arg ) + 1;
	handle->name = Cin_Malloc( name_size );
	memcpy( handle->name, arg, name_size );

	// copy upload name
	Q_snprintfz( uploadName, sizeof( uploadName ), "***r_cinematic%i***", handle->id-1 );
	name_size = strlen( uploadName ) + 1;
	handle->uploadName = Cin_Malloc( name_size );
	memcpy( handle->uploadName, uploadName, name_size );

	handle->cin = cin;
	handle->new_frame = qtrue;
	handle->registration_sequence = r_front.registration_sequence;

	// put handle at the start of the list
	handle->prev = &r_cinematics_headnode;
	handle->next = r_cinematics_headnode.next;
	handle->next->prev = handle;
	handle->prev->next = handle;

	return handle->id;
}

/*
* R_TouchCinematic
*/
void R_TouchCinematic( unsigned int id )
{
	assert( id > 0 && id <= MAX_CINEMATICS );
	if( id == 0 || id > MAX_CINEMATICS ) {
		return;
	}
	r_cinematics[id - 1].registration_sequence = r_front.registration_sequence;
}

/*
* R_FreeUnusedCinematics
*/
void R_FreeUnusedCinematics( void )
{
	r_cinhandle_t *handle, *hnode, *next;

	hnode = &r_cinematics_headnode;
	for( handle = hnode->prev; handle != hnode; handle = next ) {
		next = handle->prev;
		if( handle->registration_sequence != r_front.registration_sequence ) {
			R_FreeCinematic( handle->id );
		}
	}
}

/*
* R_FreeCinematic
*/
void R_FreeCinematic( unsigned int id )
{
	r_cinhandle_t *handle;

	handle = r_cinematics + id - 1;
	if( !handle->cin )
		return;

	CIN_Close( handle->cin );
	handle->cin = NULL;

	assert( handle->name );
	Cin_Free( handle->name );
	handle->name = NULL;

	assert( handle->uploadName );
	Cin_Free( handle->uploadName );
	handle->uploadName = NULL;

	// remove from linked active list
	handle->prev->next = handle->next;
	handle->next->prev = handle->prev;

	// insert into linked free list
	handle->next = r_free_cinematics;
	r_free_cinematics = handle;
}

/*
* R_ShutdownCinematics
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
		R_FreeCinematic( handle->id );
	}

	Cin_Free( r_cinematics );
	Mem_FreePool( &r_cinMemPool );

	Cmd_RemoveCommand( "cinlist" );
}
