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
--------------------------------------------------------------
The ACE Bot is a product of Steve Yeager, and is available from
the ACE Bot homepage, at http://www.axionfx.com/ace.

This program is a modification of the ACE Bot, and is therefore
in NO WAY supported by Steve Yeager.
*/

//==========================================================

#define AI_VERSION_STRING "A0059"

//bot debug_chase options
extern cvar_t *bot_showpath;
extern cvar_t *bot_showcombat;
extern cvar_t *bot_showsrgoal;
extern cvar_t *bot_showlrgoal;
extern cvar_t *bot_dummy;
extern cvar_t *sv_botpersonality;

//----------------------------------------------------------

#define AI_STATUS_TIMEOUT	150
#define AI_LONG_RANGE_GOAL_DELAY 1000
#define AI_SHORT_RANGE_GOAL_DELAY 75

#define AI_DEFAULT_YAW_SPEED	( self->ai.pers.cha.default_yaw_speed )
#define AI_REACTION_TIME	( self->ai.pers.cha.reaction_time )
#define AI_COMBATMOVE_TIMEOUT	( self->ai.pers.cha.combatmove_timeout )
#define AI_YAW_ACCEL		( self->ai.pers.cha.yaw_accel * FRAMETIME )
#define AI_CHAR_OFFENSIVNESS ( self->ai.pers.cha.offensiveness )
#define AI_CHAR_CAMPINESS ( self->ai.pers.cha.campiness )

// Platform states:
#define	STATE_TOP	    0
#define	STATE_BOTTOM	    1
#define STATE_UP	    2
#define STATE_DOWN	    3

#define BOT_MOVE_LEFT		0
#define BOT_MOVE_RIGHT		1
#define BOT_MOVE_FORWARD	2
#define BOT_MOVE_BACK		3

//=============================================================
//	NAVIGATION DATA
//=============================================================

#define MAX_NODES 2048        //jalToDo: needs dynamic alloc (big terrain maps)
#define NODE_INVALID  -1
#define NODE_DENSITY 128         // Density setting for nodes
#define NODE_TIMEOUT 1500 // (milli)seconds to reach the next node
#define NODE_REACH_RADIUS 36
#define NODE_WIDE_REACH_RADIUS 92
#define	NODES_MAX_PLINKS 16
#define	NAV_FILE_VERSION 10
#define NAV_FILE_EXTENSION "nav"
#define NAV_FILE_FOLDER "navigation"

#define	AI_STEPSIZE	STEPSIZE    // 18
#define AI_JUMPABLE_HEIGHT 50
#define AI_JUMPABLE_DISTANCE 360
#define AI_WATERJUMP_HEIGHT 24
#define AI_MIN_RJ_HEIGHT 128
#define AI_MAX_RJ_HEIGHT 512
#define AI_GOAL_SR_RADIUS	200

#define MASK_NODESOLID      ( CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP )
#define MASK_AISOLID        ( CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_BODY|CONTENTS_MONSTERCLIP )

// node flags
#define	NODEFLAGS_WATER 0x00000001
#define	NODEFLAGS_LADDER 0x00000002
#define NODEFLAGS_SERVERLINK 0x00000004  // plats, doors, teles. Only server can link 2 nodes with this flag
#define	NODEFLAGS_FLOAT 0x00000008  // don't drop node to floor ( air & water )
#define	NODEFLAGS_DONOTENTER 0x00000010
#define	NODEFLAGS_BOTROAM 0x00000020
#define NODEFLAGS_JUMPPAD 0x00000040
#define NODEFLAGS_JUMPPAD_LAND 0x00000080
#define	NODEFLAGS_PLATFORM 0x00000100
#define	NODEFLAGS_TELEPORTER_IN 0x00000200
#define NODEFLAGS_TELEPORTER_OUT 0x00000400
#define NODEFLAGS_REACHATTOUCH 0x00000800
#define NODEFLAGS_ENTITYREACH 0x00001000 // never reachs on it's own, the entity has to declare itself reached

#define NODE_ALL 0xFFFFFFFF

#define NODE_MASK_SERVERFLAGS ( NODEFLAGS_SERVERLINK|NODEFLAGS_BOTROAM|NODEFLAGS_JUMPPAD|NODEFLAGS_JUMPPAD_LAND|NODEFLAGS_PLATFORM|NODEFLAGS_TELEPORTER_IN|NODEFLAGS_TELEPORTER_OUT|NODEFLAGS_REACHATTOUCH|NODEFLAGS_ENTITYREACH )
#define NODE_MASK_NOREUSE ( NODEFLAGS_LADDER|NODEFLAGS_JUMPPAD|NODEFLAGS_JUMPPAD_LAND|NODEFLAGS_PLATFORM|NODEFLAGS_TELEPORTER_IN|NODEFLAGS_TELEPORTER_OUT|NODEFLAGS_ENTITYREACH )

// links types (movetypes required to run node links)
#define	LINK_MOVE 0x00000001
#define	LINK_STAIRS 0x00000002
#define LINK_FALL 0x00000004
#define	LINK_CLIMB 0x00000008
#define	LINK_TELEPORT 0x00000010
#define	LINK_PLATFORM 0x00000020
#define LINK_JUMPPAD 0x00000040
#define LINK_WATER 0x00000080
#define	LINK_WATERJUMP 0x00000100
#define	LINK_LADDER	0x00000200
#define LINK_JUMP 0x00000400
#define LINK_CROUCH 0x00000800
#define LINK_ROCKETJUMP 0x00002000
#define LINK_DOOR 0x00004000

#define LINK_INVALID 0x00001000

typedef struct nav_plink_s
{
	int numLinks;
	int nodes[NODES_MAX_PLINKS];
	int dist[NODES_MAX_PLINKS];
	int moveType[NODES_MAX_PLINKS];

} nav_plink_t;

typedef struct nav_node_s
{
	vec3_t origin;
	int flags;
	int area;

} nav_node_t;

typedef struct nav_ents_s
{
	edict_t	*ent;
	int node;

} nav_ents_t;

typedef struct nav_path_s
{
	int next;   // next node
	int cost;
	int moveTypes; // type of movements required to run along this path (flags)

} nav_path_t;

nav_plink_t pLinks[MAX_NODES];      // pLinks array
nav_node_t nodes[MAX_NODES];        // nodes array

typedef struct
{
	qboolean loaded;
	qboolean editmode;
	qboolean debugMode;

	int num_nodes;          // total number of nodes
	int serverNodesStart;

	int num_goalEnts;
	nav_ents_t goalEnts[MAX_EDICTS]; // entities which are potential goals

	int num_navigableEnts;
	nav_ents_t navigableEnts[MAX_EDICTS]; // plats, etc

} ai_navigation_t;

ai_navigation_t	nav;

//=============================================================
//	WEAPON DECISSIONS DATA
//=============================================================

enum
{
	AI_AIMSTYLE_INSTANTHIT,
	AI_AIMSTYLE_PREDICTION,
	AI_AIMSTYLE_PREDICTION_EXPLOSIVE,
	AI_AIMSTYLE_DROP,

	AIWEAP_AIM_TYPES
};

enum
{
	AIWEAP_MELEE_RANGE,
	AIWEAP_SHORT_RANGE,
	AIWEAP_MEDIUM_RANGE,
	AIWEAP_LONG_RANGE,

	AIWEAP_RANGES
};

typedef struct
{
	int aimType;
	float RangeWeight[AIWEAP_RANGES];
} ai_weapon_t;

extern ai_weapon_t AIWeapons[WEAP_TOTAL];

//----------------------------------------------------------

//game
//----------------------------------------------------------
void	    Use_Plat( edict_t *ent, edict_t *other, edict_t *activator );


// ai_main.c
//----------------------------------------------------------
nav_ents_t *AI_GetGoalentForEnt( edict_t *target );
void	    AI_PickLongRangeGoal( edict_t *ent );
void	    AI_Frame( edict_t *self, usercmd_t *ucmd );
void AI_ClearGoal( edict_t *self );
void	    AI_ResetNavigation( edict_t *ent );
void	    AI_CategorizePosition( edict_t *ent );
void AI_UpdateStatus( edict_t *self );


// ai_items.c
//----------------------------------------------------------
//float	    AI_ItemWeight( edict_t *ent, edict_t *item );
qboolean    AI_ShortRangeReachable( edict_t *self, vec3_t goal );


// ai_movement.c
//----------------------------------------------------------
int	AI_ChangeAngle( edict_t *ent );
qboolean    AI_MoveToShortRangeGoalEntity( edict_t *self, usercmd_t *ucmd );
qboolean    AI_SpecialMove( edict_t *self, usercmd_t *ucmd );
qboolean    AI_CanMove( edict_t *self, int direction );
qboolean    AI_IsLadder( vec3_t origin, vec3_t v_angle, vec3_t mins, vec3_t maxs, edict_t *passent );
qboolean    AI_IsStep( edict_t *ent );

// ai_navigation.c
//----------------------------------------------------------
int	    AI_FindCost( int from, int to, int movetypes );
int	    AI_FindClosestReachableNode( vec3_t origin, edict_t *passent, int range, int flagsmask );
int	    AI_FindClosestNode( vec3_t origin, float mindist, int range, int flagsmask );
void	    AI_SetGoal( edict_t *self, int goal_node );
void AI_NodeReached( edict_t *self );
int AI_GetNodeFlags( int node );
void AI_GetNodeOrigin( int node, vec3_t origin );
qboolean AI_NodeHasTimedOut( edict_t *self );


// ai_nodes.c
//----------------------------------------------------------

void AI_InitNavigationData( qboolean silent );
void AI_SaveNavigation( void );
int	    AI_FlagsForNode( vec3_t origin, edict_t *passent );
qboolean    AI_LoadPLKFile( char *mapname );
void AI_DeleteNode( int node );


// ai_tools.c
//----------------------------------------------------------
void	    AITools_DrawPath( edict_t *self, int node_to );
void	    AITools_DrawLine( vec3_t origin, vec3_t dest );
void	    AITools_DrawColorLine( vec3_t origin, vec3_t dest, int color, int parm );
void	    AITools_InitEditnodes( void );
void	    AITools_InitMakenodes( void );

// ai_links.c
//----------------------------------------------------------
qboolean    AI_VisibleOrigins( vec3_t spot1, vec3_t spot2 );
int	    AI_LinkCloseNodes( void );
int	    AI_FindLinkType( int n1, int n2 );
qboolean    AI_AddLink( int n1, int n2, int linkType );
qboolean    AI_PlinkExists( int n1, int n2 );
int	    AI_PlinkMoveType( int n1, int n2 );
int	    AI_findNodeInRadius( int from, vec3_t org, float rad, qboolean ignoreHeight );
char *AI_LinkString( int linktype );
int	    AI_GravityBoxToLink( int n1, int n2 );
int	    AI_LinkCloseNodes_JumpPass( int start );
int		AI_LinkCloseNodes_RocketJumpPass( int start );
void AI_LinkNavigationFile( qboolean silent );


//bot_classes
//----------------------------------------------------------
void	    BOT_DMclass_InitPersistant( edict_t *self );

//ai_common.c
//----------------------------------------------------------
qboolean    AI_DropNodeOriginToFloor( vec3_t origin, edict_t *passent );
qboolean    AI_visible( edict_t *self, edict_t *other );
qboolean    AI_infront( edict_t *self, edict_t *other );
qboolean	AI_infront2D( vec3_t lookDir, vec3_t origin, vec3_t point, float accuracy );
void	    AI_NewEnemyInView( edict_t *self, edict_t *enemy );
unsigned int AI_CurrentLinkType( edict_t *self );

//ai_class_dmbot.c
//----------------------------------------------------------
void BOT_DMclass_FindEnemy( edict_t *self );
void BOT_DMclass_CombatMovement( edict_t *self, usercmd_t *ucmd );
void BOT_DMclass_MoveWander( edict_t *self, usercmd_t *ucmd );
void BOT_DMclass_Move( edict_t *self, usercmd_t *ucmd );
