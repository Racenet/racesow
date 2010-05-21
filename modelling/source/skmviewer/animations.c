

//===============================================
//
//	Viewer stuff, animations and such
//
//===============================================
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "animations.h"

#define VIEWER_MAX_ANIMATIONS 2048
#define VIEWER_ANIMATION_MAX_NAME 64	//SKM_MAX_NAME is 64, DPM 32

typedef struct {
	int		firstframe;
	int		lastframe;
	char	name[VIEWER_ANIMATION_MAX_NAME];

}animation_t;

animation_t	animations[VIEWER_MAX_ANIMATIONS];//jalfixme dynamic alloc

int anim_num_scenes = 0;



//===============================================
//	Registering
//===============================================


void SMViewer_scenenamefromframename(const char *framename, char *scenename)
{
	int n;
	strcpy(scenename, framename);
	for (n = strlen(scenename) - 1;n >= 0 && scenename[n] >= '0' && scenename[n] <= '9';n--);
	scenename[n + 1] = 0;

	//cut '_' if it's the last character too
	if( n > 1  && scenename[n] == '_' )
		scenename[n] = 0;
}

int SMViewer_AssignFrameToAnim( int frame, const char *framename )
{
	int		i;
	char	scenename[VIEWER_ANIMATION_MAX_NAME];

	SMViewer_scenenamefromframename( framename, scenename );
	for ( i=0; i<anim_num_scenes; i++ ) {
		if( !strcmp (scenename, animations[i].name) ) //already registered
		{
			if( frame < animations[i].firstframe )
				animations[i].firstframe = frame;

			if( frame > animations[i].lastframe )
				animations[i].lastframe = frame;

			return anim_num_scenes;
		}
	}

	//wasn't assigned to any animation, so create a new one for it
	if( anim_num_scenes >= VIEWER_MAX_ANIMATIONS ) {
		printf( "WARNING: max animations reached\n" );
		return anim_num_scenes;
	}

	strcpy( animations[anim_num_scenes].name, scenename );
	animations[anim_num_scenes].firstframe = frame;
	animations[anim_num_scenes].lastframe = frame;
	anim_num_scenes++;

	return anim_num_scenes;
}

void SMViewer_PrintAnimations( void )
{
	int		i;

	if( !anim_num_scenes ) {
		printf( "WARNING: no loaded animations:\n" );
		return;
	}

	printf( "listing animations:\n" );
	for ( i=0; i<anim_num_scenes; i++ ) {
		printf( "%i:%s (%i-%i)\n", i, animations[i].name, animations[i].firstframe, animations[i].lastframe );
	}
	printf( "%i animations in list\n", i );
}


//===============================================
//	Playing
//===============================================

char *SMViewer_NextFrame( int *frame, int *oldframe )
{
	int i;
	int curframe = *frame;

	for ( i=0; i<anim_num_scenes; i++ ) {
		if( curframe >= animations[i].firstframe && 
			curframe <= animations[i].lastframe ) //found the animation
		{
			curframe++;
			if( curframe > animations[i].lastframe )
				curframe = animations[i].firstframe;

			*oldframe = *frame;
			*frame = curframe;
			return animations[i].name;
		}
	}

	//curframe wasn't a valid value. Reset
	*frame = animations[0].firstframe;
	*oldframe = *frame;
	return animations[0].name;
}

char *SMViewer_PrevFrame( int *frame, int *oldframe )
{
	int i;
	int curframe = *frame;

	for ( i=0; i<anim_num_scenes; i++ ) {
		if( curframe >= animations[i].firstframe && 
			curframe <= animations[i].lastframe ) //found the animation
		{
			curframe--;
			if( curframe < animations[i].firstframe )
				curframe = animations[i].lastframe;

			*oldframe = *frame;
			*frame = curframe;
			return animations[i].name;
		}
	}

	//curframe wasn't a valid value. Reset
	*frame = animations[0].firstframe;
	*oldframe = *frame;
	return animations[0].name;
}


char *SMViewer_NextAnimation( int *frame, int *oldframe, int side )
{
	int i;
	int curframe = *frame;

	if( anim_num_scenes == 1 ) //if only one
	{
		if( curframe < animations[0].firstframe || 
			curframe > animations[0].lastframe )
			*frame = *oldframe = animations[0].firstframe;

		return animations[0].name;
	}

	for ( i=0; i<anim_num_scenes; i++ ) {
		if( curframe >= animations[i].firstframe && 
			curframe <= animations[i].lastframe ) //found the animation
		{
			if( i+side >= anim_num_scenes ) //if last set first
			{
				curframe = animations[0].firstframe;
				*frame = curframe;
				return animations[0].name;

			}
			if( i+side < 0 ) { //if first set last
				curframe = animations[anim_num_scenes-1].firstframe;
				*frame = curframe;
				return animations[anim_num_scenes-1].name;
			}

			//simply next
			curframe = animations[i+side].firstframe;
			*oldframe = *frame;
			*frame = curframe;
			return animations[i+side].name;
		}
	}

	//curframe wasn't a valid value. Reset
	*frame = animations[0].firstframe;
	*oldframe = *frame;
	return animations[0].name;
}


