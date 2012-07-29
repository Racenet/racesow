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

#include "cg_local.h"

static char *cg_defaultSexedSounds[] =
{
	"*death", //"*death2", "*death3", "*death4",
	"*fall_0", "*fall_1", "*fall_2",
	"*falldeath",
	"*gasp", "*drown",
	"*jump_1", "*jump_2",
	"*pain25", "*pain50", "*pain75", "*pain100",
	"*wj_1", "*wj_2",
	"*dash_1", "*dash_2",
	"*taunt",
	NULL
};


/*
* CG_RegisterPmodelSexedSound
*/
static struct sfx_s *CG_RegisterPmodelSexedSound( pmodelinfo_t *pmodelinfo, char *name )
{
	char *p, *s, model[MAX_QPATH];
	cg_sexedSfx_t *sexedSfx;
	char oname[MAX_QPATH];
	char sexedFilename[MAX_QPATH];

	if( !pmodelinfo )
		return NULL;

	Q_strncpyz( oname, name, sizeof( oname ) );
	COM_StripExtension( oname );
	for( sexedSfx = pmodelinfo->sexedSfx; sexedSfx; sexedSfx = sexedSfx->next )
	{
		if( !Q_stricmp( sexedSfx->name, oname ) )
			return sexedSfx->sfx;
	}

	// find out what's the model name
	s = pmodelinfo->name;
	if( s[0] )
	{
		p = strchr( s, '/' );
		if( p )
		{
			s = p + 1;
			p = strchr( s, '/' );
			if( p )
			{
				Q_strncpyz( model, p + 1, sizeof( model ) );
				p = strchr( model, '/' );
				if( p )
					*p = 0;
			}
		}
	}

	// if we can't figure it out, they're DEFAULT_PLAYERMODEL
	if( !model[0] )
		Q_strncpyz( model, DEFAULT_PLAYERMODEL, sizeof( model ) );

	sexedSfx = CG_Malloc( sizeof( cg_sexedSfx_t ) );
	sexedSfx->name = CG_CopyString( oname );
	sexedSfx->next = pmodelinfo->sexedSfx;
	pmodelinfo->sexedSfx = sexedSfx;

	// see if we already know of the model specific sound
	Q_snprintfz( sexedFilename, sizeof( sexedFilename ), "sounds/players/%s/%s", model, oname+1 );

	if( ( !COM_FileExtension( sexedFilename ) &&
		trap_FS_FirstExtension( sexedFilename, SOUND_EXTENSIONS, NUM_SOUND_EXTENSIONS ) ) ||
		trap_FS_FOpenFile( sexedFilename, NULL, FS_READ ) != -1 )
	{
		sexedSfx->sfx = trap_S_RegisterSound( sexedFilename );
	}
	else
	{       // no, revert to default player sounds folders
		if( pmodelinfo->sex == GENDER_FEMALE )
		{
			Q_snprintfz( sexedFilename, sizeof( sexedFilename ), "sounds/players/%s/%s", "female", oname+1 );
			sexedSfx->sfx = trap_S_RegisterSound( sexedFilename );
		}
		else
		{
			Q_snprintfz( sexedFilename, sizeof( sexedFilename ), "sounds/players/%s/%s", "male", oname+1 );
			sexedSfx->sfx = trap_S_RegisterSound( sexedFilename );
		}
	}

	return sexedSfx->sfx;
}

/*
* CG_UpdateSexedSoundsRegistration
*/
void CG_UpdateSexedSoundsRegistration( pmodelinfo_t *pmodelinfo )
{
	cg_sexedSfx_t *sexedSfx, *next;
	char *name;
	int i;

	if( !pmodelinfo )
		return;

	// free loaded sounds
	for( sexedSfx = pmodelinfo->sexedSfx; sexedSfx; sexedSfx = next )
	{
		next = sexedSfx->next;
		CG_Free( sexedSfx );
	}
	pmodelinfo->sexedSfx = NULL;

	// load default sounds
	for( i = 0;; i++ )
	{
		name = cg_defaultSexedSounds[i];
		if( !name )
			break;
		CG_RegisterPmodelSexedSound( pmodelinfo, name );
	}

	// load sounds server told us
	for( i = 1; i < MAX_SOUNDS; i++ )
	{
		name = cgs.configStrings[CS_SOUNDS+i];
		if( !name[0] )
			break;
		if( name[0] == '*' )
			CG_RegisterPmodelSexedSound( pmodelinfo, name );
	}
}

/*
* CG_RegisterSexedSound
*/
struct sfx_s *CG_RegisterSexedSound( int entnum, char *name )
{
	if( entnum < 0 || entnum >= MAX_EDICTS )
		return NULL;
	return CG_RegisterPmodelSexedSound( cg_entPModels[entnum].pmodelinfo, name );
}

/*
* CG_SexedSound
*/
void CG_SexedSound( int entnum, int entchannel, char *name, float fvol )
{
	qboolean fixed;

	fixed = entchannel & CHAN_FIXED ? qtrue : qfalse;
	entchannel &= ~CHAN_FIXED;

	if( fixed )
		trap_S_StartFixedSound( CG_RegisterSexedSound( entnum, name ), cg_entities[entnum].current.origin, entchannel, fvol, ATTN_NORM );
	else if( ISVIEWERENTITY( entnum ) )
		trap_S_StartGlobalSound( CG_RegisterSexedSound( entnum, name ), entchannel, fvol );
	else
		trap_S_StartRelativeSound( CG_RegisterSexedSound( entnum, name ), entnum, entchannel, fvol, ATTN_NORM );
}


/*
* CG_LoadClientInfo
*/
void CG_LoadClientInfo( cg_clientInfo_t *ci, char *info, int client )
{
	char *s;
	int rgbcolor;

	assert( ci );
	assert( info );
	assert( client >= 0 && client < gs.maxclients );

	if( !Info_Validate( info ) )
		CG_Error( "Invalid client info" );

	s = Info_ValueForKey( info, "name" );
	Q_strncpyz( ci->name, s && s[0] ? s : "badname", sizeof( ci->name ) );

	s = Info_ValueForKey( info, "hand" );
	ci->hand = s && s[0] ? atoi( s ) : 2;

	s = Info_ValueForKey( info, "fov" );
	if( !(s && s[0]) || sscanf( s, "%3i %3i", &ci->fov, &ci->zoomfov ) != 2 )
	{
		ci->fov = DEFAULT_FOV;
		ci->zoomfov = DEFAULT_ZOOMFOV;
	}

	// color
	s = Info_ValueForKey( info, "color" );
	rgbcolor = s && s[0] ? COM_ReadColorRGBString( s ) : -1;
	if( rgbcolor != -1 )
		Vector4Set( ci->color, COLOR_R( rgbcolor ), COLOR_G( rgbcolor ), COLOR_B( rgbcolor ), 255 );
	else
		Vector4Set( ci->color, 255, 255, 255, 255 );

	// wsw : jal : dunno why are we still keeping the icon
	ci->icon = cgs.basePModelInfo[0].icon;
}
