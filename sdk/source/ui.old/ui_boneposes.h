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
//

//=============================================================================
//
//							BONEPOSES
//
//=============================================================================
typedef struct
{
	char name[MAX_QPATH];
	int flags;
	int parent;
} cgs_bone_t;

typedef struct cgs_skeleton_s
{
	struct model_s *model;

	int numBones;
	cgs_bone_t *bones;

	int numFrames;
	bonepose_t **bonePoses;

	struct cgs_skeleton_s *next;
} cgs_skeleton_t;



cgs_skeleton_t *UI_SkeletonForModel( struct model_s *model );
void UI_InitTemporaryBoneposesCache( void );
void UI_ResetTemporaryBoneposesCache( void );
void UI_TransformBoneposes( cgs_skeleton_t *skel, bonepose_t *boneposes, bonepose_t *sourceboneposes );
void UI_RotateBonePose( vec3_t angles, bonepose_t *bonepose );
qboolean UI_SkeletalPoseLerpAttachment( orientation_t *orient, cgs_skeleton_t *skel, bonepose_t *boneposes, bonepose_t *oldboneposes, float backlerp, char *bonename );
qboolean UI_SkeletalUntransformedPoseLerpAttachment( orientation_t *orient, cgs_skeleton_t *skel, bonepose_t *boneposes, bonepose_t *oldboneposes, float backlerp, char *bonename );
cgs_skeleton_t *UI_SetBoneposesForTemporaryEntity( entity_t *ent );
