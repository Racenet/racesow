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


#ifndef _UIWSW_BONEPOSES_H_
#define _UIWSW_BONEPOSES_H_
#include "../game/q_shared.h"
#include "../cgame/ref.h"

namespace UIWsw
{
	typedef struct
	{
		char					name[MAX_QPATH];
		int						flags;
		int						parent;
	} cgs_bone_t;

	typedef struct cgs_skeleton_s
	{
		struct model_s			*model;

		int						numBones;
		cgs_bone_t				*bones;

		int						numFrames;
		bonepose_t				**bonePoses;

		struct cgs_skeleton_s	*next;
	} cgs_skeleton_t;

	class BonePoses
	{
	private:
		static cgs_skeleton_t *skel_headnode;
		/** allocate more space for temporary boneposes */
		static void ExpandTemporaryBoneposesCache( void );
		/** These boneposes are REMOVED EACH FRAME after drawing. Register
			here only in the case you create an entity which is not UI_entity. */
		static bonepose_t *RegisterTemporaryExternalBoneposes( cgs_skeleton_t *skel, bonepose_t *poses );

		/** Temporary Boneposes Cache */
		static bonepose_t *TBC;
		static int TBC_Size;
		static int	TBC_Count;

	public:
		static cgs_skeleton_t *SkeletonForModel( struct model_s *model );
		/** Allocate space for temporary boneposes */
		static void InitTemporaryBoneposesCache( void );
		/** These boneposes are REMOVED EACH FRAME after drawing. */
		static void ResetTemporaryBoneposesCache( void );
		/** Place bones in it's final position in the skeleton */
		static void TransformBoneposes( cgs_skeleton_t *skel, bonepose_t *boneposes, bonepose_t *sourceboneposes );
		/** Rotate bones */
		static void RotateBonePose( vec3_t angles, bonepose_t *bonepose );
		/** Get the interpolated bone from TRANSFORMED bonepose and oldbonepose */
		static bool SkeletalPoseLerpAttachment( orientation_t *orient, cgs_skeleton_t *skel, bonepose_t *boneposes, bonepose_t *oldboneposes, float backlerp, char *bonename );
		/** Build both old and new frame poses to get the tag bone 
			@remark slow: use UI_SkeletalPoseLerpAttachment if possible */
		static bool SkeletalUntransformedPoseLerpAttachment( orientation_t *orient, cgs_skeleton_t *skel, bonepose_t *boneposes, bonepose_t *oldboneposes, float backlerp, char *bonename );
		/**	Sets up skeleton with inline boneposes based on frame/oldframe values
			These boneposes will be REMOVED EACH FRAME. Use only for temporary entities,
			UI_entities have a persistant registration method available. */
		static cgs_skeleton_t *UI_SetBoneposesForTemporaryEntity( entity_t *ent );
		/** Sets up skeleton with inline boneposes based on frame/oldframe values
			These boneposes will be REMOVED EACH FRAME. Use only for temporary entities,
			UI_entities have a persistant registration method available. */
		static cgs_skeleton_t *SetBoneposesForTemporaryEntity( entity_t *ent );
	};
}

#endif
