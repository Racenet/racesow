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

#include "../gameshared/q_shared.h"
#include "../gameshared/q_math.h"
#include "../gameshared/q_cvar.h"
#include "../gameshared/q_dynvar.h"
#include "../gameshared/gs_ref.h"
#include "../cgame/ref.h"

namespace UIWsw
{
	typedef struct playermodelinfo_s
	{
		int		nskins;
		char	**skinnames;
		char	directory[MAX_QPATH];
		struct playermodelinfo_s *next;
	} playermodelinfo_t;

	class PlayerModels
	{
	private:
		static playermodelinfo_t *pmodels;

		static bool isValidModel( char *model_name );
		static void CreatePlayerModelList( void );
		static char **FindModelSkins( char *model, int *numSkins );
	public:
		static void Init( void );
		static const playermodelinfo_t* getModelList( void );
		static const playermodelinfo_t* getModel( const char *modelname );
		static void DrawPlayerModel( const char *model, const char *skin, byte_vec4_t color, int xpos, int ypos, int width, int height, int frame, int oldframe );
	};
}

