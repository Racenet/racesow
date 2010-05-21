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

#include "uiwsw_Utils.h"
#include "uiwsw_BonePoses.h"

using namespace UIWsw;

cgs_skeleton_t *BonePoses::skel_headnode = NULL;

cgs_skeleton_t *BonePoses::SkeletonForModel( struct model_s *model )
{
	int i, j;
	cgs_skeleton_t *skel;
	qbyte *buffer;
	cgs_bone_t *bone;
	bonepose_t *bonePose;
	int numBones, numFrames;

	if( !model )
		return NULL;

	numBones = Trap::R_SkeletalGetNumBones( model, &numFrames );
	if( !numBones || !numFrames )
		return NULL;		// no bones or frames

	for( skel = skel_headnode; skel; skel = skel->next ) {
		if( skel->model == model )
			return skel;
	}

	// allocate one huge array to hold our data
	buffer = (qbyte*)UIMem::Malloc( sizeof( cgs_skeleton_t ) + numBones * sizeof( cgs_bone_t ) +
		numFrames * (sizeof( bonepose_t * ) + numBones * sizeof( bonepose_t )) );

	skel = ( cgs_skeleton_t * )buffer; buffer += sizeof( cgs_skeleton_t );
	skel->bones = ( cgs_bone_t * )buffer; buffer += numBones * sizeof( cgs_bone_t );
	skel->numBones = numBones;
	skel->bonePoses = ( bonepose_t ** )buffer; buffer += numFrames * sizeof( bonepose_t * );
	skel->numFrames = numFrames;
	// register bones
	for( i = 0, bone = skel->bones; i < numBones; i++, bone++ )
		bone->parent = Trap::R_SkeletalGetBoneInfo( model, i, bone->name, sizeof( bone->name ), &bone->flags );

	// register poses for all frames for all bones
	for( i = 0; i < numFrames; i++ ) {
		skel->bonePoses[i] = ( bonepose_t * )buffer; buffer += numBones * sizeof( bonepose_t );
		for( j = 0, bonePose = skel->bonePoses[i]; j < numBones; j++, bonePose++ )
			Trap::R_SkeletalGetBonePose( model, j, i, bonePose );
	}

	skel->next = skel_headnode;
	skel_headnode = skel;

	skel->model = model;

	return skel;
}

void BonePoses::TransformBoneposes( cgs_skeleton_t *skel, bonepose_t *boneposes, bonepose_t *sourceboneposes )
{
	int				j;
	bonepose_t	temppose;

	for( j = 0; j < (int)skel->numBones; j++ ) 
	{
		if( skel->bones[j].parent >= 0 )
		{
			memcpy( &temppose, &sourceboneposes[j], sizeof(bonepose_t));
			Quat_ConcatTransforms ( boneposes[skel->bones[j].parent].quat, boneposes[skel->bones[j].parent].origin, temppose.quat, temppose.origin, boneposes[j].quat, boneposes[j].origin );
			
		} else
			memcpy( &boneposes[j], &sourceboneposes[j], sizeof(bonepose_t));	
	}
}

void BonePoses::RotateBonePose( vec3_t angles, bonepose_t *bonepose )
{
	vec3_t		axis_rotator[3];
	quat_t		quat_rotator;
	bonepose_t	temppose;
	vec3_t		tempangles;

	tempangles[0] = -angles[YAW];
	tempangles[1] = -angles[PITCH];
	tempangles[2] = -angles[ROLL];
	AnglesToAxis ( tempangles, axis_rotator );
	Matrix_Quat( axis_rotator, quat_rotator );

	memcpy( &temppose, bonepose, sizeof(bonepose_t));

	Quat_ConcatTransforms ( quat_rotator, vec3_origin, temppose.quat, temppose.origin, bonepose->quat, bonepose->origin );
}

bool BonePoses::SkeletalPoseLerpAttachment( orientation_t *orient, cgs_skeleton_t *skel, bonepose_t *boneposes, bonepose_t *oldboneposes, float backlerp, char *bonename )
{
	int			i;
	quat_t		quat;
	cgs_bone_t	*bone;
	bonepose_t	*bonepose, *oldbonepose;
	float frontlerp = 1.0 - backlerp;

	if( !boneposes || !oldboneposes || !skel){
		UI_Printf( "UI_SkeletalPoseLerpAttachment: Wrong model or boneposes %s\n", bonename );
		return qfalse;
	}

	// find the appropriate attachment bone
	bone = skel->bones;
	for( i = 0; i < skel->numBones; i++, bone++ ) {
		if( !Q_stricmp( bone->name, bonename ) )
			break;
	}

	if( i == skel->numBones ) {
		UI_Printf( "UI_SkeletalPoseLerpAttachment: no such bone %s\n", bonename );
		return false;
	}

	//get the desired bone
	bonepose = boneposes + i;
	oldbonepose = oldboneposes + i;

	// lerp
	Quat_Lerp( oldbonepose->quat, bonepose->quat, frontlerp, quat );
	Quat_Conjugate( quat, quat );	//inverse the tag direction
	Quat_Matrix( quat, orient->axis );
	orient->origin[0] = oldbonepose->origin[0] + (bonepose->origin[0] - oldbonepose->origin[0]) * frontlerp;
	orient->origin[1] = oldbonepose->origin[1] + (bonepose->origin[1] - oldbonepose->origin[1]) * frontlerp;
	orient->origin[2] = oldbonepose->origin[2] + (bonepose->origin[2] - oldbonepose->origin[2]) * frontlerp;

	return true;
}

bool BonePoses::SkeletalUntransformedPoseLerpAttachment( orientation_t *orient, cgs_skeleton_t *skel, bonepose_t *boneposes, bonepose_t *oldboneposes, float backlerp, char *bonename )
{
	int			i;
	quat_t		quat;
	cgs_bone_t	*bone;
	bonepose_t	*bonepose, *oldbonepose;
	bonepose_t *tr_boneposes, *tr_oldboneposes;
	quat_t		oldbonequat, bonequat;
	float frontlerp = 1.0 - backlerp;

	if( !boneposes || !oldboneposes || !skel){
		UI_Printf( "UI_SkeletalPoseLerpAttachment: Wrong model or boneposes %s\n", bonename );
		return qfalse;
	}

	// find the appropriate attachment bone
	bone = skel->bones;
	for( i = 0; i < skel->numBones; i++, bone++ ) {
		if( !Q_stricmp( bone->name, bonename ) ){
			break;
		}
	}

	if( i == skel->numBones ) {
		UI_Printf( "UI_SkeletalPoseLerpAttachment: no such bone %s\n", bonename );
		return false;
	}

	// transform frameposes

	//alloc new space for them: JALFIXME: Make a cache for this operation
	tr_boneposes = (bonepose_t*)UIMem::Malloc ( sizeof(bonepose_t) * skel->numBones );
	TransformBoneposes( skel, tr_boneposes, boneposes );
	tr_oldboneposes = (bonepose_t*)UIMem::Malloc ( sizeof(bonepose_t) * skel->numBones );
	TransformBoneposes( skel, tr_oldboneposes, oldboneposes );
	
	//get the desired bone
	bonepose = tr_boneposes + i;
	oldbonepose = tr_oldboneposes + i;

	//inverse the tag, cause bones point to it's parent, and tags are understood to point to the end of the bones chain
	Quat_Conjugate( oldbonepose->quat, oldbonequat );
	Quat_Conjugate( bonepose->quat, bonequat );

	// interpolate quaternions and origin
	Quat_Lerp( oldbonequat, bonequat, frontlerp, quat );
	Quat_Matrix( quat, orient->axis );
	orient->origin[0] = oldbonepose->origin[0] + (bonepose->origin[0] - oldbonepose->origin[0]) * frontlerp;
	orient->origin[1] = oldbonepose->origin[1] + (bonepose->origin[1] - oldbonepose->origin[1]) * frontlerp;
	orient->origin[2] = oldbonepose->origin[2] + (bonepose->origin[2] - oldbonepose->origin[2]) * frontlerp;

	//free
	UIMem::Free(tr_boneposes);
	UIMem::Free(tr_oldboneposes);

	return true;
}



//========================================================================
//
//		TMP BONEPOSES
//
//========================================================================


#define TBC_Block_Size		1024

int BonePoses::TBC_Size = 0;
bonepose_t *BonePoses::TBC = NULL;
int	BonePoses::TBC_Count = 0;

void BonePoses::InitTemporaryBoneposesCache( void )
{
	TBC_Size = TBC_Block_Size;
	TBC = (bonepose_t*)UIMem::Malloc ( sizeof(bonepose_t) * TBC_Size );
	TBC_Count = 0;
}

void BonePoses::ExpandTemporaryBoneposesCache( void )
{
	bonepose_t *temp;

	temp = TBC;

	TBC = (bonepose_t*)UIMem::Malloc ( sizeof(bonepose_t) * (TBC_Size + TBC_Block_Size) );
	memcpy( TBC, temp, sizeof(bonepose_t) * TBC_Size );
	TBC_Size += TBC_Block_Size;

	UIMem::Free( temp );
}

void BonePoses::ResetTemporaryBoneposesCache( void )
{
	TBC_Count = 0;
}

bonepose_t *BonePoses::RegisterTemporaryExternalBoneposes( cgs_skeleton_t *skel, bonepose_t *poses )
{
	bonepose_t	*boneposes;
	if( (TBC_Count + skel->numBones) > TBC_Size )
		ExpandTemporaryBoneposesCache();

	boneposes = &TBC[TBC_Count];
	TBC_Count += skel->numBones;

	return boneposes;
}

cgs_skeleton_t *BonePoses::SetBoneposesForTemporaryEntity( entity_t *ent )
{
	cgs_skeleton_t	*skel;

	skel = SkeletonForModel( ent->model );
	if( skel ) {
		ent->boneposes = RegisterTemporaryExternalBoneposes( skel, ent->boneposes );
		TransformBoneposes( skel, ent->boneposes, skel->bonePoses[ent->frame] );
		ent->oldboneposes = RegisterTemporaryExternalBoneposes( skel, ent->oldboneposes );
		TransformBoneposes( skel, ent->oldboneposes, skel->bonePoses[ent->oldframe] );
	}

	return skel;
}

