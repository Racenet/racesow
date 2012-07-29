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

#ifndef __G_GAMETYPE_H__
#define __G_GAMETYPE_H__

//g_gametypes.c
extern cvar_t *g_warmup_timelimit;
extern cvar_t *g_postmatch_timelimit;
extern cvar_t *g_countdown_time;
extern cvar_t *g_match_extendedtime;
extern cvar_t *g_votable_gametypes;
extern cvar_t *g_gametype; // only for use in function that deal with changing gametype, use GS_Gametype()
extern cvar_t *g_gametypes_list;

#define G_CHALLENGERS_MIN_JOINTEAM_MAPTIME  9000 // must wait 10 seconds before joining
#define GAMETYPE_PROJECT_EXTENSION ".gt"
#define GAMETYPE_SCRIPT_EXTENSION ".as"
#define CHAR_GAMETYPE_SEPARATOR ';'

#define MAX_RACE_CHECKPOINTS	32

typedef struct gameaward_s
{
	// ch : size of this?
	char *name;
	int count;
	// struct gameaward_s *next;
} gameaward_t;

typedef struct
{
	int mm_attacker;	// session-id
	int mm_victim;		// session-id
	int weapon;			// weapon used
	unsigned int time;	// server timestamp
} loggedFrag_t;

typedef struct
{
	int owner;		// session-id
	unsigned int timestamp;	// milliseconds
	int numSectors;
	unsigned int *times;	// unsigned int * numSectors+1, where last is final time
} raceRun_t;

typedef struct
{
	int score;
	int deaths;
	int frags;
	int suicides;
	int teamfrags;
	int numrounds;
	int awards;

	int accuracy_shots[AMMO_TOTAL-AMMO_GUNBLADE];
	int accuracy_hits[AMMO_TOTAL-AMMO_GUNBLADE];
	int accuracy_hits_direct[AMMO_TOTAL-AMMO_GUNBLADE];
	int accuracy_hits_air[AMMO_TOTAL-AMMO_GUNBLADE];
	int accuracy_damage[AMMO_TOTAL-AMMO_GUNBLADE];
	int accuracy_frags[AMMO_TOTAL-AMMO_GUNBLADE];
	int total_damage_given;
	int total_damage_received;
	int total_teamdamage_given;
	int total_teamdamage_received;
	int health_taken;
	int armor_taken;
	// item counts for mm
	int ga_taken;
	int ya_taken;
	int ra_taken;
	int mh_taken;
	int uh_taken;
	int quads_taken;
	int shells_taken;
	int regens_taken;
	int bombs_planted;
	int bombs_defused;
	int flags_capped;

	// loggedFrag_t
	linear_allocator_t *fragAllocator;

	// gameaward_t
	linear_allocator_t *awardAllocator;
	// gameaward_t *gameawards;

	raceRun_t currentRun;
	raceRun_t raceRecords;

	int asFactored;
	int asRefCount;
} score_stats_t;

// this is only really used to create the script objects
typedef struct
{
	qboolean dummy;
}match_t;

typedef struct
{
	match_t match;

	int asEngineHandle;
	qboolean asEngineIsGeneric;

	void *initFunc;
	void *spawnFunc;
	void *matchStateStartedFunc;
	void *matchStateFinishedFunc;
	void *thinkRulesFunc;
	void *playerRespawnFunc;
	void *scoreEventFunc;
	void *scoreboardMessageFunc;
	void *selectSpawnPointFunc;
	void *clientCommandFunc;
	void *botStatusFunc;
	void *shutdownFunc;
	int RS_MysqlAuthenticate_Callback; //racesow

	int spawnableItemsMask;
	int respawnableItemsMask;
	int dropableItemsMask;
	int pickableItemsMask;

	qboolean isTeamBased;
	qboolean isRace;
	qboolean hasChallengersQueue;
	int maxPlayersPerTeam;

	// default item respawn time
	int ammo_respawn;
	int armor_respawn;
	int weapon_respawn;
	int health_respawn;
	int powerup_respawn;
	int megahealth_respawn;
	int ultrahealth_respawn;

	// few default settings
	qboolean readyAnnouncementEnabled;
	qboolean scoreAnnouncementEnabled;
	qboolean countdownEnabled;
	qboolean mathAbortDisabled;
	qboolean shootingDisabled;
	qboolean infiniteAmmo;
	qboolean canForceModels;
	qboolean canShowMinimap;
	qboolean teamOnlyMinimap;
	qboolean customDeadBodyCam;

	int spawnpoint_radius;

	qboolean mmCompatible;

	//racesow
	qboolean autoInactivityRemove;
	qboolean playerInteraction;
	qboolean freestyleMapFix;
	qboolean enableDrowning;
	//!racesow

} gametype_descriptor_t;

typedef struct
{
	int playerIndices[MAX_CLIENTS];
	int numplayers;
	score_stats_t stats;
	int ping;
	qboolean locked;
	int invited[MAX_CLIENTS];
	qboolean has_coach;

	int asRefCount;
	int asFactored;
} g_teamlist_t;

g_teamlist_t teamlist[GS_MAX_TEAMS];

//clock
char clockstring[16];

//
//	matches management
//
qboolean G_Match_Tied( void );
qboolean G_Match_CheckExtendPlayTime( void );
void G_Match_RemoveAllProjectiles( void );
void G_Match_CleanUpPlayerStats( edict_t *ent );
void G_Match_FreeBodyQueue( void );
void G_Match_LaunchState( int matchState );

//
//	teams
//
void G_Teams_Init( void );
void G_Teams_UpdateTeamInfoMessages( void );

void G_Teams_ExecuteChallengersQueue( void );
void G_Teams_AdvanceChallengersQueue( void );

void G_Match_Autorecord_Start( void );
void G_Match_Autorecord_AltStart( void );
void G_Match_Autorecord_Stop( void );
void G_Match_Autorecord_Cancel( void );
qboolean G_Match_ScorelimitHit( void );
qboolean G_Match_SuddenDeathFinished( void );
qboolean G_Match_TimelimitHit( void );

//coach
void G_Teams_Coach( edict_t *ent );
void G_Teams_CoachLockTeam( edict_t *ent );
void G_Teams_CoachUnLockTeam( edict_t *ent );
void G_Teams_CoachRemovePlayer( edict_t *ent );

qboolean G_Gametype_Exists( const char *name );
char *G_Gametype_GENERIC_ScoreboardMessage( void );
void G_Gametype_GENERIC_ClientRespawn( edict_t *self, int old_team, int new_team );

#endif //  __G_GAMETYPE_H__
