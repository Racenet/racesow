/*
   Copyright (C) 2002-2003 Victor Luchits

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

cgs_skeleton_t *skel_headnode;

/*
   =================
   UI_SkeletonForModel
   =================
 */
cgs_skeleton_t *UI_SkeletonForModel( struct model_s *model )
{
	int i, j;
	cgs_skeleton_t *skel;
	qbyte *buffer;
	cgs_bone_t *bone;
	bonepose_t *bonePose;
	int numBones, numFrames;

	if( !model )
		return NULL;

	numBones = trap_R_SkeletalGetNumBones( model, &numFrames );
	if( !numBones || !numFrames )
		return NULL; // no bones or frames

	for( skel = skel_headnode; skel; skel = skel->next )
	{
		if( skel->model == model )
			return skel;
	}

	// allocate one huge array to hold our data
	buffer = (qbyte *)UI_Malloc( sizeof( cgs_skeleton_t ) + numBones * sizeof( cgs_bone_t ) +
	                            numFrames * ( sizeof( bonepose_t * ) + numBones * sizeof( bonepose_t ) ) );

	skel = ( cgs_skeleton_t * )buffer; buffer += sizeof( cgs_skeleton_t );
	skel->bones = ( cgs_bone_t * )buffer; buffer += numBones * sizeof( cgs_bone_t );
	skel->numBones = numBones;
	skel->bonePoses = ( bonepose_t ** )buffer; buffer += numFrames * sizeof( bonepose_t * );
	skel->numFrames = numFrames;
	// register bones
	for( i = 0, bone = skel->bones; i < numBones; i++, bone++ )
		bone->parent = trap_R_SkeletalGetBoneInfo( model, i, bone->name, sizeof( bone->name ), &bone->flags );

	// register poses for all frames for all bones
	for( i = 0; i < numFrames; i++ )
	{
		skel->bonePoses[i] = ( bonepose_t * )buffer; buffer += numBones * sizeof( bonepose_t );
		for( j = 0, bonePose = skel->bonePoses[i]; j < numBones; j++, bonePose++ )
			trap_R_SkeletalGetBonePose( model, j, i, bonePose );
	}

	skel->next = skel_headnode;
	skel_headnode = skel;

	skel->model = model;

	return skel;
}

//========================================================================
//
//				BONEPOSES
//
//========================================================================


//===============
// UI_TransformBoneposes
// place bones in it's final position in the skeleton
//===============
void UI_TransformBoneposes( cgs_skeleton_t *skel, bonepose_t *boneposes, bonepose_t *sourceboneposes )
{
	int j;
	bonepose_t temppose;

	for( j = 0; j < (int)skel->numBones; j++ )
	{
		if( skel->bones[j].parent >= 0 )
		{
			memcpy( &temppose, &sourceboneposes[j], sizeof( bonepose_t ) );
			Quat_ConcatTransforms( boneposes[skel->bones[j].parent].quat, boneposes[skel->bones[j].parent].origin, temppose.quat, temppose.origin, boneposes[j].quat, boneposes[j].origin );

		}
		else
			memcpy( &boneposes[j], &sourceboneposes[j], sizeof( bonepose_t ) );
	}
}

//==============================
// UI_RotateBonePose
//==============================
void UI_RotateBonePose( vec3_t angles, bonepose_t *bonepose )
{
	vec3_t axis_rotator[3];
	quat_t quat_rotator;
	bonepose_t temppose;
	vec3_t tempangles;

	tempangles[0] = -angles[YAW];
	tempangles[1] = -angles[PITCH];
	tempangles[2] = -angles[ROLL];
	AnglesToAxis( tempangles, axis_rotator );
	Matrix_Quat( axis_rotator, quat_rotator );

	memcpy( &temppose, bonepose, sizeof( bonepose_t ) );

	Quat_ConcatTransforms( quat_rotator, vec3_origin, temppose.quat, temppose.origin, bonepose->quat, bonepose->origin );
}


//==============================
// UI_SkeletalPoseLerpAttachment
// Get the interpolated bone from TRANSFORMED bonepose and oldbonepose
//==============================
qboolean UI_SkeletalPoseLerpAttachment( orientation_t *orient, cgs_skeleton_t *skel, bonepose_t *boneposes, bonepose_t *oldboneposes, float backlerp, char *bonename )
{
	int i;
	quat_t quat;
	cgs_bone_t *bone;
	bonepose_t *bonepose, *oldbonepose;
	float frontlerp = 1.0 - backlerp;

	if( !boneposes || !oldboneposes || !skel )
	{
		UI_Printf( "UI_SkeletalPoseLerpAttachment: Wrong model or boneposes %s\n", bonename );
		return qfalse;
	}

	// find the appropriate attachment bone
	bone = skel->bones;
	for( i = 0; i < skel->numBones; i++, bone++ )
	{
		if( !Q_stricmp( bone->name, bonename ) )
			break;
	}

	if( i == skel->numBones )
	{
		UI_Printf( "UI_SkeletalPoseLerpAttachment: no such bone %s\n", bonename );
		return qfalse;
	}

	//get the desired bone
	bonepose = boneposes + i;
	oldbonepose = oldboneposes + i;

	// lerp
	Quat_Lerp( oldbonepose->quat, bonepose->quat, frontlerp, quat );
	Quat_Conjugate( quat, quat ); //inverse the tag direction
	Quat_Matrix( quat, orient->axis );
	orient->origin[0] = oldbonepose->origin[0] + ( bonepose->origin[0] - oldbonepose->origin[0] ) * frontlerp;
	orient->origin[1] = oldbonepose->origin[1] + ( bonepose->origin[1] - oldbonepose->origin[1] ) * frontlerp;
	orient->origin[2] = oldbonepose->origin[2] + ( bonepose->origin[2] - oldbonepose->origin[2] ) * frontlerp;

	return qtrue;
}


//==============================
// UI_SkeletalUntransformedPoseLerpAttachment
// Build both old and new frame poses to get the tag bone
// slow: use UI_SkeletalPoseLerpAttachment if possible
//==============================
qboolean UI_SkeletalUntransformedPoseLerpAttachment( orientation_t *orient, cgs_skeleton_t *skel, bonepose_t *boneposes, bonepose_t *oldboneposes, float backlerp, char *bonename )
{
	int i;
	quat_t quat;
	cgs_bone_t *bone;
	bonepose_t *bonepose, *oldbonepose;
	bonepose_t *tr_boneposes, *tr_oldboneposes;
	quat_t oldbonequat, bonequat;
	float frontlerp = 1.0 - backlerp;

	if( !boneposes || !oldboneposes || !skel )
	{
		UI_Printf( "UI_SkeletalPoseLerpAttachment: Wrong model or boneposes %s\n", bonename );
		return qfalse;
	}

	// find the appropriate attachment bone
	bone = skel->bones;
	for( i = 0; i < skel->numBones; i++, bone++ )
	{
		if( !Q_stricmp( bone->name, bonename ) )
		{
			break;
		}
	}

	if( i == skel->numBones )
	{
		UI_Printf( "UI_SkeletalPoseLerpAttachment: no such bone %s\n", bonename );
		return qfalse;
	}

	// transform frameposes

	//alloc new space for them: JALFIXME: Make a cache for this operation
	tr_boneposes = (bonepose_t *)UI_Malloc( sizeof( bonepose_t ) * skel->numBones );
	UI_TransformBoneposes( skel, tr_boneposes, boneposes );
	tr_oldboneposes = (bonepose_t *)UI_Malloc( sizeof( bonepose_t ) * skel->numBones );
	UI_TransformBoneposes( skel, tr_oldboneposes, oldboneposes );

	//get the desired bone
	bonepose = tr_boneposes + i;
	oldbonepose = tr_oldboneposes + i;

	//inverse the tag, cause bones point to it's parent, and tags are understood to point to the end of the bones chain
	Quat_Conjugate( oldbonepose->quat, oldbonequat );
	Quat_Conjugate( bonepose->quat, bonequat );

	// interpolate quaternions and origin
	Quat_Lerp( oldbonequat, bonequat, frontlerp, quat );
	Quat_Matrix( quat, orient->axis );
	orient->origin[0] = oldbonepose->origin[0] + ( bonepose->origin[0] - oldbonepose->origin[0] ) * frontlerp;
	orient->origin[1] = oldbonepose->origin[1] + ( bonepose->origin[1] - oldbonepose->origin[1] ) * frontlerp;
	orient->origin[2] = oldbonepose->origin[2] + ( bonepose->origin[2] - oldbonepose->origin[2] ) * frontlerp;

	//free
	UI_Free( tr_boneposes );
	UI_Free( tr_oldboneposes );

	return qtrue;
}



//========================================================================
//
//		TMP BONEPOSES
//
//========================================================================


#define TBC_Block_Size	    1024
static int TBC_Size;

bonepose_t *TBC;        //Temporary Boneposes Cache
static int TBC_Count;


//===============
// UI_InitTemporaryBoneposesCache
// allocate space for temporary boneposes
//===============
void UI_InitTemporaryBoneposesCache( void )
{
	TBC_Size = TBC_Block_Size;
	TBC = (bonepose_t *)UI_Malloc( sizeof( bonepose_t ) * TBC_Size );
	TBC_Count = 0;
}

//===============
// UI_ExpandTemporaryBoneposesCache
// allocate more space for temporary boneposes
//===============
static void UI_ExpandTemporaryBoneposesCache( void )
{
	bonepose_t *temp;

	temp = TBC;

	TBC = (bonepose_t *)UI_Malloc( sizeof( bonepose_t ) * ( TBC_Size + TBC_Block_Size ) );
	memcpy( TBC, temp, sizeof( bonepose_t ) * TBC_Size );
	TBC_Size += TBC_Block_Size;

	UI_Free( temp );
}

//===============
// UI_ResetTemporaryBoneposesCache
// These boneposes are REMOVED EACH FRAME after drawing.
//===============
void UI_ResetTemporaryBoneposesCache( void )
{
	TBC_Count = 0;
}

//===============
//	UI_RegisterTemporaryExternalBoneposes
// These boneposes are REMOVED EACH FRAME after drawing. Register
// here only in the case you create an entity which is not UI_entity.
//===============
static bonepose_t *UI_RegisterTemporaryExternalBoneposes( cgs_skeleton_t *skel, bonepose_t *poses )
{
	bonepose_t *boneposes;
	if( ( TBC_Count + skel->numBones ) > TBC_Size )
		UI_ExpandTemporaryBoneposesCache();

	boneposes = &TBC[TBC_Count];
	TBC_Count += skel->numBones;

	return boneposes;
}

//===============
// UI_SetBoneposesForTemporaryEntity
//	Sets up skeleton with inline boneposes based on frame/oldframe values
//	These boneposes will be REMOVED EACH FRAME. Use only for temporary entities,
//	UI_entities have a persistant registration method available.
//===============
cgs_skeleton_t *UI_SetBoneposesForTemporaryEntity( entity_t *ent )
{
	cgs_skeleton_t *skel;

	skel = UI_SkeletonForModel( ent->model );
	if( skel )
	{
		if( ent->frame >= skel->numFrames )
			ent->frame = 0;
		if( ent->oldframe >= skel->numFrames )
			ent->oldframe = 0;
		ent->boneposes = UI_RegisterTemporaryExternalBoneposes( skel, ent->boneposes );
		UI_TransformBoneposes( skel, ent->boneposes, skel->bonePoses[ent->frame] );
		ent->oldboneposes = UI_RegisterTemporaryExternalBoneposes( skel, ent->oldboneposes );
		UI_TransformBoneposes( skel, ent->oldboneposes, skel->bonePoses[ent->oldframe] );
	}

	return skel;
}
