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

#include "ui_local.h"

//=============================================================================
//
// LIST OF PLAYER MODELS
//
//=============================================================================

#define MAX_DISPLAYNAME 16
#define MAX_PLAYERMODELS 1024

cvar_t *ui_playermodel_firstframe;
cvar_t *ui_playermodel_lastframe;
cvar_t *ui_playermodel_fps;

m_itemslisthead_t playermodelsItemsList;
byte_vec4_t playerColor;
void UI_ColorRedCallback( menucommon_t *menuitem )
{
	playerColor[0] = menuitem->curvalue;
}
void UI_ColorGreenCallback( menucommon_t *menuitem )
{
	playerColor[1] = menuitem->curvalue;
}
void UI_ColorBlueCallback( menucommon_t *menuitem )
{
	playerColor[2] = menuitem->curvalue;
}

//==============================
// UI_FindIndexForModelAndSkin
// if it fails returns 0 which will be the first indexed one
//==============================
void UI_FindIndexForModelAndSkin( const char *model, const char *skin, int *modelindex, int *skinindex )
{
	m_listitem_t *item;
	playermodelinfo_s *playermodel;
	int i, j;
	int currentdirectoryindex = 0;
	int currentskinindex = 0;

	if( model && model[0] )
	{
		// find the skin and model matching our user settings (if any)
		for( i = 0; i < playermodelsItemsList.numItems; i++ )
		{
			item = UI_FindItemInScrollListWithId( &playermodelsItemsList, i );
			if( item )
			{
				playermodel = (playermodelinfo_s *)item->data;
				if( !Q_stricmp( playermodel->directory, model ) )
				{
					currentdirectoryindex = i;
					if( skin && skin[0] )
					{
						for( j = 0; j < playermodel->nskins; j++ )
						{
							if( !Q_stricmp( playermodel->skinnames[j], skin ) )
							{
								currentskinindex = j;
								break;
							}
						}
					}
					break;
				}
			}
		}
	}

	*modelindex = currentdirectoryindex;
	*skinindex = currentskinindex;
}


//==============================
//	ui_PModel_ValidModel - skelmod
//==============================
static qboolean ui_PModel_ValidModel( char *model_name )
{
	qboolean found = qfalse;
	char scratch[MAX_QPATH];

	Q_snprintfz( scratch, sizeof( scratch ), "models/players/%s/tris.skm", model_name );
	found = (qboolean)( trap_FS_FOpenFile( scratch, NULL, FS_READ ) != -1 );
	if( !found )
		return qfalse;

	Q_snprintfz( scratch, sizeof( scratch ), "models/players/%s/animation.cfg", model_name );
	found = (qboolean)( trap_FS_FOpenFile( scratch, NULL, FS_READ ) != -1 );
	if( !found )
		return qfalse;

	// verify the existence of the "default" skin file
	Q_snprintfz( scratch, sizeof( scratch ), "models/players/%s/default.skin", model_name );
	found = (qboolean)( trap_FS_FOpenFile( scratch, NULL, FS_READ ) != -1 );
	if( !found )
		return qfalse;

	return qtrue;
}

//=================
//UI_Playermodels_ScanDirectory
//=================
static char **UI_Playermodels_ScanDirectory( char *model, int *numSkins )
{
	int i, j, k;
	char listbuf[1024], scratch[MAX_CONFIGSTRING_CHARS];
	char **skinnames, *skinptr;
	int skincount, nskins;

	*numSkins = 0;
	if( !ui_PModel_ValidModel( model ) )
		return NULL;

	// get the list of skins
	skincount = trap_FS_GetFileList( va( "models/players/%s", model ), ".skin", NULL, 0, 0, 0 );
	if( !skincount )  // we already checked that the default one was there, so this can never be false, but anyway.
		return NULL;

	// there are valid skins. So:
	skinnames = (char **)UI_Malloc( sizeof( char * ) * ( skincount + 1 ) );
	nskins = 0;

	i = 0;
	do
	{
		if( ( k = trap_FS_GetFileList( va( "models/players/%s", model ), ".skin", listbuf, sizeof( listbuf ), i, skincount ) ) == 0 )
		{
			i++;
			continue;
		}
		i += k;

		// copy the valid skins
		for( skinptr = listbuf; k > 0; k--, skinptr += strlen( skinptr )+1 )
		{
			Q_strncpyz( scratch, skinptr, sizeof( scratch ) );
			COM_StripExtension( scratch );

			// see if this skin is already in the list
			for( j = 0; j < nskins && Q_stricmp( scratch, skinnames[j] ); j++ ) ;
			if( j == nskins )
				skinnames[nskins++] = UI_CopyString( scratch );
		}
	}
	while( i < skincount );

	*numSkins = nskins;
	skinnames[nskins] = NULL;
	return skinnames;
}

//=================
//UI_Playermodels_ScanDirectories
//=================
static void UI_Playermodels_CreateItemList( m_itemslisthead_t *itemlist )
{
	int i, j, k;
	int ndirs, nskins;
	char dirnames[1024], *dirptr, **skinnames;
	size_t dirlen;

	UI_FreeScrollItemList( itemlist );

	// get a list of directories (model names)
	ndirs = trap_FS_GetFileList( "models/players", "/", dirnames, sizeof( dirnames ), 0, 0 );
	if( !ndirs )
		return;

	// go through the subdirectories
	//------------------------------
	if( ndirs > MAX_PLAYERMODELS )
		ndirs = MAX_PLAYERMODELS;

	i = 0;
	do
	{
		if( ( k = trap_FS_GetFileList( "models/players", "/", dirnames, sizeof( dirnames ), 0, 0 ) ) == 0 )
		{
			k++;
			continue;
		}
		i += k;

		for( dirptr = dirnames; k > 0; k--, dirptr += dirlen+1 )
		{
			dirlen = strlen( dirptr );
			if( dirlen && dirptr[dirlen-1] == '/' )
				dirptr[dirlen-1] = '\0';

			if( !strcmp( dirptr, "." ) || !strcmp( dirptr, ".." ) )  // invalid indirections
				continue;

			// check for being already in the list
			for( j = 0; j < itemlist->numItems; j++ )
			{
				m_listitem_t *item = UI_FindItemInScrollListWithId( itemlist, j );
				if( item && !Q_stricmp( item->name, dirptr ) )
					continue;
			}

			if( ( skinnames = UI_Playermodels_ScanDirectory( dirptr, &nskins ) ) != NULL )
			{                                                                   // put it in the list
				playermodelinfo_s *playermodel = (playermodelinfo_s *)UI_Malloc( sizeof( playermodelinfo_s ) );
				Q_strncpyz( playermodel->directory, dirptr, sizeof( playermodel->directory ) );
				playermodel->skinnames = skinnames;
				playermodel->nskins = nskins;
				UI_AddItemToScrollList( itemlist, playermodel->directory, (void *)playermodel );
			}
		}
	}
	while( i < ndirs );
}

//===============
// UI_Playermodel_Init
//===============
void UI_Playermodel_Init( void )
{
	UI_Playermodels_CreateItemList( &playermodelsItemsList );

	ui_playermodel_firstframe = trap_Cvar_Get( "ui_playermodel_firstframe", "1", CVAR_DEVELOPER );
	ui_playermodel_lastframe = trap_Cvar_Get( "ui_playermodel_lastframe", "39", CVAR_DEVELOPER );
	ui_playermodel_fps = trap_Cvar_Get( "ui_playermodel_fps", "30", CVAR_DEVELOPER );
}

//===============
// UI_PlayerModelNextFrameTime
//===============
qboolean UI_PlayerModelNextFrameTime( void )
{
	static unsigned int lastModelFrameTime = 0;

	if( lastModelFrameTime > uis.time )
		lastModelFrameTime = uis.time;

	if( lastModelFrameTime + ( 1000/ui_playermodel_fps->value ) > uis.time )
	{
		return qfalse;
	}

	lastModelFrameTime = uis.time;
	return qtrue;
}

//------------------------------------------------------------------------

//===============
// UI_SkelposeBounds
//===============
static void UI_SkelposeBounds( cgs_skeleton_t *skel, bonepose_t *boneposes, vec3_t mins, vec3_t maxs )
{
	int j, i;

	VectorClear( mins );
	VectorClear( maxs );

	for( j = 0; j < skel->numBones; j++ )
	{
		for( i = 0; i < 3; i++ )
		{
			if( boneposes[j].origin[i] > maxs[i] )
				maxs[i] = boneposes[j].origin[i];
			if( boneposes[j].origin[i] < mins[i] )
				mins[i] = boneposes[j].origin[i];
		}
	}

	// HACK: since the model is always some bigger than the skeleton, expand it a 40%
	for( i = 0; i < 3; i++ )
	{
		maxs[i] *= 1.45f;
		mins[i] *= 1.45f;
	}
}

//===============
// UI_DrawPlayerModel
//===============
void UI_DrawPlayerModel( char *model, char *skin, byte_vec4_t color, int xpos, int ypos, int width, int height, int frame, int oldframe )
{
	refdef_t refdef;
	static vec3_t angles;
	entity_t entity;
	vec3_t mins, maxs;
	cgs_skeleton_t *skel = NULL;
	char scratch[MAX_QPATH];

	if( !skin || !model )
		return;

	// refdef
	memset( &refdef, 0, sizeof( refdef ) );

	refdef.x = xpos;
	refdef.y = ypos;
	refdef.width = width;
	refdef.height = height;

	refdef.fov_x = 30;
	refdef.fov_y = CalcFov( refdef.fov_x, width, height );
	refdef.areabits = 0;
	refdef.time = uis.time;
	refdef.rdflags = RDF_NOWORLDMODEL;
	Matrix_Copy( axis_identity, refdef.viewaxis );

	// draw player model

	memset( &entity, 0, sizeof( entity ) );

	Q_snprintfz( scratch, sizeof( scratch ), "models/players/%s/%s.skin", model, skin );
	entity.customShader = NULL;
	entity.customSkin = trap_R_RegisterSkinFile( scratch );
	if( !entity.customSkin )
		return;

	Q_snprintfz( scratch, sizeof( scratch ), "models/players/%s/tris.skm", model );
	entity.model = trap_R_RegisterModel( scratch );
	if( trap_R_SkeletalGetNumBones( entity.model, NULL ) )
	{
		skel = UI_SkeletonForModel( entity.model );
		if( !skel )
			return;
	}

	entity.frame = frame;
	entity.oldframe = oldframe;
	UI_SetBoneposesForTemporaryEntity( &entity );
	UI_SkelposeBounds( skel, entity.boneposes, mins, maxs );
	//trap_R_ModelBounds( entity.model, mins, maxs );
	entity.origin[0] = 0.5 * ( maxs[2] - mins[2] ) * ( 1.0 / 0.268 );
	entity.origin[1] = 0.5 * ( mins[1] + maxs[1] );
	entity.origin[2] = -0.5 * ( mins[2] + maxs[2] );
	entity.renderfx = RF_NOSHADOW | RF_FORCENOLOD;
	entity.scale = 0.9f;
	Vector4Set( entity.shaderRGBA, color[0], color[1], color[2], 255 );
	VectorCopy( entity.origin, entity.origin2 );
	VectorCopy( entity.origin, entity.lightingOrigin );
	angles[1] += 250 * uis.frameTime;
	if( angles[1] > 360 )
		angles[1] -= 360;
	AnglesToAxis( angles, entity.axis );
	entity.outlineHeight = DEFAULT_OUTLINE_HEIGHT;
	Vector4Set( entity.outlineRGBA, color[0] * 0.25f, color[1] * 0.25f, color[2] * 0.25f, 255 );

	trap_R_ClearScene();
	trap_R_AddEntityToScene( &entity );

	trap_R_RenderScene( &refdef );

	UI_ResetTemporaryBoneposesCache();
}
