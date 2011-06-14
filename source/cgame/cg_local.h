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

// cg_local.h -- local definitions for client game module

#include "../gameshared/q_arch.h"
#include "../gameshared/q_math.h"
#include "../gameshared/q_shared.h"
#include "../gameshared/q_cvar.h"
#include "../gameshared/q_dynvar.h"
#include "../gameshared/q_comref.h"
#include "../gameshared/q_collision.h"

#include "../gameshared/gs_public.h"
#include "ref.h"

#include "cg_public.h"
#include "cg_syscalls.h"

#define CG_OBITUARY_HUD	    1
#define CG_OBITUARY_CENTER  2
#define CG_OBITUARY_CONSOLE 4

#define ITEM_RESPAWN_TIME   1000

#define FLAG_TRAIL_DROP_DELAY 300
#define HEADICON_TIMEOUT 8000

#define GAMECHAT_STRING_SIZE	150
#define GAMECHAT_STACK_SIZE		20

enum
{
	LOCALEFFECT_EV_PLAYER_TELEPORT_IN
	, LOCALEFFECT_EV_PLAYER_TELEPORT_OUT
	, LOCALEFFECT_VSAY_HEADICON
	, LOCALEFFECT_VSAY_HEADICON_TIMEOUT
	, LOCALEFFECT_ROCKETTRAIL_LAST_DROP
	, LOCALEFFECT_ROCKETFIRE_LAST_DROP
	, LOCALEFFECT_GRENADETRAIL_LAST_DROP
	, LOCALEFFECT_BLOODTRAIL_LAST_DROP
	, LOCALEFFECT_FLAGTRAIL_LAST_DROP
	, LOCALEFFECT_LASERBEAM
	, LOCALEFFECT_LASERBEAM_SMOKE_TRAIL
	, LOCALEFFECT_EV_WEAPONBEAM
	, MAX_LOCALEFFECTS = 64
};

typedef struct
{
	entity_state_t current;
	entity_state_t prev;        // will always be valid, but might just be a copy of current

	int serverFrame;            // if not current, this ent isn't in the frame
	unsigned int fly_stoptime;

	int respawnTime;

	entity_t ent;                   // interpolated, to be added to render list
	unsigned int type;
	unsigned int renderfx;
	unsigned int effects;
	struct cgs_skeleton_s *skel;

	vec3_t velocity;

	qboolean canExtrapolate;
	qboolean canExtrapolatePrev;
	vec3_t prevVelocity;
	int microSmooth;
	vec3_t microSmoothOrigin;
	vec3_t microSmoothOrigin2;
	//vec3_t prevExtrapolatedOrigin;
	//vec3_t extrapolatedOrigin;

	gsitem_t	*item;

	//effects
	vec3_t trailOrigin;         // for particle trails

	// local effects from events timers
	unsigned int localEffects[MAX_LOCALEFFECTS];

	// attached laser beam
	vec3_t laserOrigin;
	vec3_t laserPoint;
	vec3_t laserOriginOld;
	vec3_t laserPointOld;
	qboolean laserCurved;

	qboolean linearProjectileCanDraw;
	vec3_t linearProjectileViewerSource;
	vec3_t linearProjectileViewerVelocity;

	vec3_t teleportedFrom;
	byte_vec4_t outlineColor;

	// used for client side animation of player models
	qboolean pendingAnimationsUpdate;
	int lastAnims;
	int lastVelocitiesFrames[4];
	float lastVelocities[4][4];
	qboolean jumpedLeft;
	vec3_t animVelocity;
	float yawVelocity;
} centity_t;

#include "cg_pmodels.h"

typedef struct cgs_media_handle_s
{
	char *name;
	void *data;
	struct cgs_media_handle_s *next;
} cgs_media_handle_t;

#define MAX_SMALL_TECHY_GIBS	4
#define MAX_BIG_TECHY_GIBS		2
#define MAX_TECHY_GIBS			( MAX_SMALL_TECHY_GIBS + MAX_BIG_TECHY_GIBS )
#define MAX_MEATY_GIBS			4

#define STAT_MINUS				10  // num frame for '-' stats digit

typedef struct
{
	// sounds
	cgs_media_handle_t *sfxChat;

	// timers
	cgs_media_handle_t *sfxTimerBipBip;
	cgs_media_handle_t *sfxTimerPloink;

	cgs_media_handle_t *sfxRic[3];

	cgs_media_handle_t *sfxWeaponUp;
	cgs_media_handle_t *sfxWeaponUpNoAmmo;

	cgs_media_handle_t *sfxWalljumpFailed;

	//--------------------------------------

	cgs_media_handle_t *sfxWeaponHit[4];
	cgs_media_handle_t *sfxWeaponKill;
	cgs_media_handle_t *sfxWeaponHitTeam;

	cgs_media_handle_t *sfxItemRespawn;
	cgs_media_handle_t *sfxTeleportIn;
	cgs_media_handle_t *sfxTeleportOut;
	cgs_media_handle_t *sfxShellHit;

	// Gunblade sounds :
	cgs_media_handle_t *sfxGunbladeWeakShot[3];
	cgs_media_handle_t *sfxGunbladeStrongShot;
	cgs_media_handle_t *sfxBladeFleshHit[3];
	cgs_media_handle_t *sfxBladeWallHit[2];
	cgs_media_handle_t *sfxGunbladeStrongHit[3];

	// Riotgun sounds :
	cgs_media_handle_t *sfxRiotgunWeakHit;
	cgs_media_handle_t *sfxRiotgunStrongHit;

	// Grenade launcher sounds :
	cgs_media_handle_t *sfxGrenadeWeakBounce[2];
	cgs_media_handle_t *sfxGrenadeStrongBounce[2];
	cgs_media_handle_t *sfxGrenadeWeakExplosion;
	cgs_media_handle_t *sfxGrenadeStrongExplosion;

	// Rocket launcher sounds :
	cgs_media_handle_t *sfxRocketLauncherWeakHit;
	cgs_media_handle_t *sfxRocketLauncherStrongHit;

	// Plasmagun sounds
	cgs_media_handle_t *sfxPlasmaWeakHit;
	cgs_media_handle_t *sfxPlasmaStrongHit;

	cgs_media_handle_t *sfxQuadFireSound;

	// VSAY sounds
	cgs_media_handle_t *sfxVSaySounds[VSAY_TOTAL];

	//no wsw

	// models
	//	cgs_media_handle_t		*modTeleportEffect;
	cgs_media_handle_t *modDash;
	cgs_media_handle_t *modHeadStun;

	cgs_media_handle_t *modTechyGibs[MAX_TECHY_GIBS];
	cgs_media_handle_t *modMeatyGibs[MAX_MEATY_GIBS];

	//wsw weapon sfx
	cgs_media_handle_t *modRocketExplosion;
	cgs_media_handle_t *modPlasmaExplosion;

	cgs_media_handle_t *modBulletExplode;
	cgs_media_handle_t *modBladeWallHit;
	cgs_media_handle_t *modBladeWallExplo;

	cgs_media_handle_t *modElectroBoltWallHit;
	cgs_media_handle_t *modInstagunWallHit;

	//no wsw

	cgs_media_handle_t *shaderParticle;
	cgs_media_handle_t *shaderGrenadeExplosion;
	cgs_media_handle_t *shaderRocketExplosion;
	cgs_media_handle_t *shaderRocketExplosionRing;
	cgs_media_handle_t *shaderBulletExplosion;
	cgs_media_handle_t *shaderRaceGhostEffect;
	cgs_media_handle_t *shaderWaterBubble;
	//	cgs_media_handle_t		*shaderTeleportEffect;
	cgs_media_handle_t *shaderSmokePuff;

	cgs_media_handle_t *shaderSmokePuff1;
	cgs_media_handle_t *shaderSmokePuff2;
	cgs_media_handle_t *shaderSmokePuff3;

	cgs_media_handle_t *shaderStrongRocketFireTrailPuff;
	cgs_media_handle_t *shaderWeakRocketFireTrailPuff;
	cgs_media_handle_t *shaderGrenadeTrailSmokePuff;
	cgs_media_handle_t *shaderRocketTrailSmokePuff;
	cgs_media_handle_t *shaderBloodTrailPuff;
	cgs_media_handle_t *shaderBloodTrailLiquidPuff;
	cgs_media_handle_t *shaderBloodImpactPuff;
	cgs_media_handle_t *shaderCartoonHit;
	cgs_media_handle_t *shaderCartoonHit2;
	cgs_media_handle_t *shaderCartoonHit3;
	cgs_media_handle_t *shaderTeleporterSmokePuff;
	cgs_media_handle_t *shaderBulletMark;
	cgs_media_handle_t *shaderExplosionMark;
	cgs_media_handle_t *shaderEnergyMark;
	cgs_media_handle_t *shaderLaser;
	cgs_media_handle_t *shaderNet;
	cgs_media_handle_t *shaderBackTile;
	cgs_media_handle_t *shaderSelect;
	cgs_media_handle_t *shaderChatBalloon;
	cgs_media_handle_t *shaderDownArrow;

	//wsw
	//----------------------------------------------

	cgs_media_handle_t *shaderAdditiveParticleShine;

	//wsw weapon sfx
	cgs_media_handle_t *shaderPlasmaMark;
	cgs_media_handle_t *shaderElectroBeamOld;
	cgs_media_handle_t *shaderElectroBeamOldAlpha;
	cgs_media_handle_t *shaderElectroBeamOldBeta;
	cgs_media_handle_t *shaderElectroBeamA;
	cgs_media_handle_t *shaderElectroBeamAAlpha;
	cgs_media_handle_t *shaderElectroBeamABeta;
	cgs_media_handle_t *shaderElectroBeamB;
	cgs_media_handle_t *shaderElectroBeamBAlpha;
	cgs_media_handle_t *shaderElectroBeamBBeta;
	cgs_media_handle_t *shaderInstaBeam;
	cgs_media_handle_t *shaderLaserGunBeam;
	cgs_media_handle_t *shaderElectroboltMark;
	cgs_media_handle_t *shaderInstagunMark;

	//wsw
	cgs_media_handle_t *shaderPlayerShadow;
	cgs_media_handle_t *shaderFlagFlare;

	// hud icons
	cgs_media_handle_t *shaderWeaponIcon[WEAP_TOTAL];
	cgs_media_handle_t *shaderNoGunWeaponIcon[WEAP_TOTAL];

	cgs_media_handle_t *shaderKeyIconOn[KEYICON_TOTAL];
	cgs_media_handle_t *shaderKeyIconOff[KEYICON_TOTAL];

	//no wsw

	cgs_media_handle_t *sbNums[11];
	cgs_media_handle_t *shaderCrosshair[NUM_CROSSHAIRS];

	// VSAY icons
	cgs_media_handle_t *shaderVSayIcon[VSAY_TOTAL];
} cgs_media_t;

typedef struct bonenode_s
{
	int bonenum;
	int numbonechildren;
	struct bonenode_s **bonechildren;
} bonenode_t;

typedef struct cg_tagmask_s
{
	char tagname[64];
	char bonename[64];
	int bonenum;
	struct cg_tagmask_s *next;
	vec3_t offset;
	vec3_t rotate;
} cg_tagmask_t;

typedef struct
{
	char name[MAX_QPATH];
	int flags;
	int parent;
	struct bonenode_s *node;
} cgs_bone_t;

typedef struct cgs_skeleton_s
{
	struct model_s *model;

	int numBones;
	cgs_bone_t *bones;

	int numFrames;
	bonepose_t **bonePoses;

	struct cgs_skeleton_s *next;

	// store the tagmasks as part of the skeleton (they are only used by player models, tho)
	struct cg_tagmask_s *tagmasks;

	struct bonenode_s *bonetree;
} cgs_skeleton_t;

#include "cg_boneposes.h"

typedef struct cg_sexedSfx_s
{
	char *name;
	struct sfx_s *sfx;
	struct cg_sexedSfx_s *next;
} cg_sexedSfx_t;

typedef struct
{
	char name[MAX_QPATH];
	int hand;
	int fov, zoomfov;
	byte_vec4_t color;
	struct shader_s *icon;
} cg_clientInfo_t;

#define MAX_ANGLES_KICKS 3

typedef struct
{
	unsigned int timestamp;
	unsigned int kicktime;
	float v_roll, v_pitch;
} cg_kickangles_t;

#define MAX_COLORBLENDS 3

typedef struct
{
	unsigned int timestamp;
	unsigned int blendtime;
	float blend[4];
} cg_viewblend_t;

#define PREDICTED_STEP_TIME 150 // stairs smoothing time
#define MAX_AWARD_LINES 3
#define MAX_AWARD_DISPLAYTIME 5000
#define MAX_CHECKPOINTS 50 //racesow

// view types
enum
{
	VIEWDEF_CAMERA,
	VIEWDEF_PLAYERVIEW,

	VIEWDEF_MAXTYPES
};

typedef struct
{
	int type;
	int POVent;
	qboolean thirdperson;
	qboolean playerPrediction;
	qboolean drawWeapon;
	qboolean draw2D;
	refdef_t refdef;
	float fracDistFOV;
	vec3_t origin;
	vec3_t angles;
	vec3_t axis[3];
	vec3_t velocity;
} cg_viewdef_t;

#include "cg_democams.h"

// this is not exactly "static" but still...
typedef struct
{
	unsigned int playerNum;

	// shaders
	struct shader_s	*shaderWhite;
	struct shader_s	*shaderMiniMap;

	// fonts
	struct mufont_s	*fontSystemSmall;
	struct mufont_s	*fontSystemMedium;
	struct mufont_s	*fontSystemBig;

	char fontNameSystemSmall[MAX_QPATH];
	char fontNameSystemMedium[MAX_QPATH];
	char fontNameSystemBig[MAX_QPATH];

	int initialSharedSeed;

	cgs_media_t media;

	qboolean precacheDone;

	int vidWidth, vidHeight;

	qboolean demoPlaying;
	qboolean demoTutorial;
	qboolean pure;
	qboolean tv, tvRequested;
	int gameProtocol;
	unsigned int snapFrameTime;
	unsigned int extrapolationTime;

	char *demoAudioStream;

	//
	// locally derived information from server state
	//
	char configStrings[MAX_CONFIGSTRINGS][MAX_CONFIGSTRING_CHARS];

	char weaponModels[WEAP_TOTAL][MAX_QPATH];
	int numWeaponModels;
	weaponinfo_t *weaponInfos[WEAP_TOTAL];    // indexed list of weapon model infos
	orientation_t weaponItemTag;

	cg_clientInfo_t	clientInfo[MAX_CLIENTS];

	struct model_s *modelDraw[MAX_MODELS];

	struct pmodelinfo_s *pModelsIndex[MAX_MODELS];
	struct pmodelinfo_s *basePModelInfo; //fall back replacements
	struct skinfile_s *baseSkin;

	// force models
	struct pmodelinfo_s *teamModelInfo[GS_MAX_TEAMS];
	struct skinfile_s *teamCustomSkin[GS_MAX_TEAMS]; // user defined
	int teamColor[GS_MAX_TEAMS];

	struct sfx_s *soundPrecache[MAX_SOUNDS];
	struct shader_s	*imagePrecache[MAX_IMAGES];
	struct skinfile_s *skinPrecache[MAX_SKINFILES];
} cg_static_t;

typedef struct
{
	unsigned int time;
	char text[GAMECHAT_STRING_SIZE];
} cg_gamemessage_t;

typedef struct
{
	unsigned int nextMsg;
	unsigned int lastMsgTime;
	qboolean lastActive;
	unsigned int lastActiveChangeTime;
	float activeFrac;
	cg_gamemessage_t messages[GAMECHAT_STACK_SIZE];
} cg_gamechat_t;

typedef struct
{
	unsigned int time;
	float delay;

	unsigned int realTime;
	float frameTime;
	float realFrameTime;
	int frameCount;

	unsigned int firstViewRealTime;
	int viewFrameCount;
	qboolean startedMusic;

	int sharedSeed;

	snapshot_t frame, oldFrame;
	qboolean frameSequenceRunning;
	qboolean oldAreabits;
	qboolean portalInView;
	qboolean fireEvents;

	float predictedOrigins[CMD_BACKUP][3];              // for debug comparing against server

	float predictedStep;                // for stair up smoothing
	unsigned int predictedStepTime;

	unsigned int predictingTimeStamp;
	unsigned int predictedEventTimes[PREDICTABLE_EVENTS_MAX];
	vec3_t predictionError;
	player_state_t predictedPlayerState;     // current in use, predicted or interpolated
	int predictedWeaponSwitch;				// inhibit shooting prediction while a weapon change is expected
	gs_laserbeamtrail_t weaklaserTrail;

	// prediction optimization (don't run all ucmds in not needed)
	int predictFrom;
	entity_state_t predictFromEntityState;
	player_state_t predictFromPlayerState;

	int lastWeapon;

	vec3_t autorotateAxis[3];

	float lerpfrac;                     // between oldframe and frame
	float xerpTime;
	float oldXerpTime;
	float xerpSmoothFrac;

	int effects;

	vec3_t lightingOrigin;

	qboolean showScoreboard;            // demos and multipov

	unsigned int multiviewPlayerNum;       // for multipov chasing, takes effect on next snap

	int pointedNum;

	//
	// all cyclic walking effects
	//
	float xyspeed;

	float oldBobTime;
	int bobCycle;                   // odd cycles are right foot going forward
	float bobFracSin;               // sin(bobfrac*M_PI)

	//
	// kick angles and color blend effects
	//

	cg_kickangles_t	kickangles[MAX_ANGLES_KICKS];
	cg_viewblend_t colorblends[MAX_COLORBLENDS];
	unsigned int damageBlends[4];

	//
	// transient data from server
	//
	char checkname[MAX_QPATH];
	char loadingstring[MAX_QPATH];
	int precacheCount, precacheTotal;
	const char *matchmessage;
	char *teaminfo;
	size_t teaminfo_size;
	char *motd;
	unsigned int motd_time;

	// awards
	char award_lines[MAX_AWARD_LINES][MAX_CONFIGSTRING_CHARS];
	unsigned int award_times[MAX_AWARD_LINES];
	int award_head;

	//racesow
	// checkpoints
	int checkpoints[MAX_CHECKPOINTS];
	//!racesow

	// statusbar program
	struct cg_layoutnode_s *statusBar;

	cg_viewweapon_t weapon;
	cg_viewdef_t view;

	cg_gamechat_t chat;
} cg_state_t;

extern cg_static_t cgs;
extern cg_state_t cg;

#define ISVIEWERENTITY( entNum )  ( ( cg.predictedPlayerState.POVnum > 0 ) && ( (int)cg.predictedPlayerState.POVnum == entNum ) && ( cg.view.type == VIEWDEF_PLAYERVIEW ) )
#define ISBRUSHMODEL( x ) ( ( ( x > 0 ) && ( (int)x < trap_CM_NumInlineModels() ) ) ? qtrue : qfalse )

extern centity_t cg_entities[MAX_EDICTS];

//
// cg_ents.c
//
extern cvar_t *cg_gun;
extern cvar_t *cg_damage_kick;

void CG_NewFrameSnap( snapshot_t *frame, snapshot_t *lerpframe );
struct cmodel_s *CG_CModelForEntity( int entNum );
void CG_SoundEntityNewState( centity_t *cent );
void CG_AddEntities( void );
void CG_GetEntitySpatilization( int entNum, vec3_t origin, vec3_t velocity );
void CG_LerpEntities( void );
void CG_LerpGenericEnt( centity_t *cent );

void CG_SetOutlineColor( byte_vec4_t outlineColor, byte_vec4_t color );
void CG_AddColoredOutLineEffect( entity_t *ent, int effects, qbyte r, qbyte g, qbyte b, qbyte a );
void CG_AddCentityOutLineEffect( centity_t *cent );

void CG_AddFlagModelOnTag( centity_t *cent, byte_vec4_t teamcolor, char *tagname );

void CG_ResetItemTimers( void );
centity_t *CG_GetItemTimerEnt( int num );

//
// cg_draw.c
//
int CG_HorizontalAlignForWidth( const int x, int align, int width );
int CG_VerticalAlignForHeight( const int y, int align, int height );

void CG_DrawHUDField( int x, int y, int align, float *color, int size, int width, int value );
void CG_DrawHUDModel( int x, int y, int align, int w, int h, struct model_s *model, struct shader_s *shader, float yawspeed );
void CG_DrawMiniMap( int x, int y, int iw, int ih, qboolean draw_playernames, qboolean draw_itemnames, int align, vec4_t color );
void CG_DrawHUDRect( int x, int y, int align, int w, int h, int val, int maxval, vec4_t color, struct shader_s *shader );
void CG_DrawPicBar( int x, int y, int width, int height, int align, float percent, struct shader_s *shader, vec4_t backColor, vec4_t color );

//
// cg_media.c
//
void CG_RegisterMediaSounds( void );
void CG_RegisterMediaModels( void );
void CG_RegisterMediaShaders( void );
void CG_RegisterLevelMinimap( void );
void CG_RegisterFonts( void );

struct model_s *CG_RegisterModel( const char *name );

struct sfx_s *CG_MediaSfx( cgs_media_handle_t *mediasfx );
struct model_s *CG_MediaModel( cgs_media_handle_t *mediamodel );
struct shader_s *CG_MediaShader( cgs_media_handle_t *mediashader );

//
// cg_players.c
//
extern cvar_t *model;
extern cvar_t *skin;
extern cvar_t *hand;

void CG_LoadClientInfo( cg_clientInfo_t *ci, char *s, int client );
void CG_UpdateSexedSoundsRegistration( pmodelinfo_t *pmodelinfo );
void CG_SexedSound( int entnum, int entchannel, char *name, float fvol );
struct sfx_s *CG_RegisterSexedSound( int entnum, char *name );

//
// cg_predict.c
//
extern cvar_t *cg_predict;
extern cvar_t *cg_predict_optimize;
extern cvar_t *cg_showMiss;

void CG_PredictedEvent( int entNum, int ev, int parm );
void CG_Predict_ChangeWeapon( int new_weapon );
void CG_PredictMovement( void );
void CG_CheckPredictionError( void );
void CG_BuildSolidList( void );
void CG_Trace( trace_t *t, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int ignore, int contentmask );
int CG_PointContents( vec3_t point );
void CG_Predict_TouchTriggers( pmove_t *pm );

//
// cg_screen.c
//
typedef struct
{
	int x, y, width, height;
} vrect_t;

extern vrect_t scr_vrect;

extern cvar_t *cg_scoreboardFont;
extern cvar_t *cg_showFPS;
extern cvar_t *cg_showAwards;
extern cvar_t *cg_showZoomEffect;
extern cvar_t *cg_showCaptureAreas;

void CG_ScreenInit( void );
void CG_ScreenShutdown( void );
void CG_Draw2D( void );
void CG_CalcVrect( void );
void CG_TileClear( void );
void CG_DrawLoading( void );
void CG_CenterPrint( char *str );
void CG_CenterPrintToUpper( char *str );

void CG_EscapeKey( void );
void CG_LoadStatusBar( void );

void CG_LoadingString( char *str );
void CG_LoadingItemName( char *str );

void CG_DrawCrosshair( int x, int y, int align );
void CG_DrawKeyState( int x, int y, int w, int h, int align, char *key );

void CG_ScreenCrosshairDamageUpdate( void );

int CG_ParseValue( const char **s );

void CG_DrawClock( int x, int y, int align, struct mufont_s *font, vec4_t color );
void CG_DrawPlayerNames( struct mufont_s *font, vec4_t color );
void CG_DrawHUDNumeric( int x, int y, int align, float *color, int charwidth, int charheight, int value );
void CG_DrawTeamInfo( int x, int y, int align, struct mufont_s *font, vec4_t color );
void CG_DrawNet( int x, int y, int w, int h, int align, vec4_t color );

//
// cg_hud.c
//
extern cvar_t *cg_showminimap;
extern cvar_t *cg_showitemtimers;
extern cvar_t *cg_placebo;

void CG_SC_Obituary( void );
void Cmd_CG_PrintHudHelp_f( void );
void CG_ExecuteLayoutProgram( struct cg_layoutnode_s *rootnode );

//racesow
void CG_RaceAddCheckpoint( char* msg ); //lm: race hud checkpoints helper
void CG_AddJumpspeed( void ); //lm: don't depend on the hudscripts for obtaining jump values..
//!racesow

//
// cg_damage_indicator.c
//
void CG_ResetDamageIndicator( void );
void CG_DamageIndicatorAdd( int damage, const vec3_t dir );

//
// cg_scoreboard.c
//
void CG_DrawScoreboard( void );
void CG_ToggleScores_f( void );
void CG_ScoresOn_f( void );
void CG_ScoresOff_f( void );
qboolean CG_ExecuteScoreboardTemplateLayout( char *s );
void SCR_UpdateScoreboardMessage( const char *string );
void SCR_UpdatePlayerStatsMessage( const char *string );

//
// cg_main.c
//
extern cvar_t *developer;
extern cvar_t *cg_showClamp;

// wsw
extern cvar_t *cg_showObituaries;
extern cvar_t *cg_volume_hitsound;    // hit sound volume
extern cvar_t *cg_autoaction_demo;
extern cvar_t *cg_autoaction_screenshot;
extern cvar_t *cg_autoaction_stats;
extern cvar_t *cg_autoaction_spectator;
extern cvar_t *cg_simpleItems; // simple items
extern cvar_t *cg_simpleItemsSize; // simple items
extern cvar_t *cg_volume_players; // players sound volume
extern cvar_t *cg_volume_effects; // world sound volume
extern cvar_t *cg_volume_announcer; // announcer sounds volume
extern cvar_t *cg_volume_voicechats; //vsays volume
extern cvar_t *cg_rocketTrail;
extern cvar_t *cg_rocketFireTrail;
extern cvar_t *cg_grenadeTrail;
extern cvar_t *cg_bloodTrail;
extern cvar_t *cg_showBloodTrail;
extern cvar_t *cg_rocketTrailAlpha;
extern cvar_t *cg_rocketFireTrailAlpha;
extern cvar_t *cg_grenadeTrailAlpha;
extern cvar_t *cg_bloodTrailAlpha;

extern cvar_t *cg_cartoonRockets;
extern cvar_t *cg_cartoonEffects;
extern cvar_t *cg_cartoonHitEffect;

extern cvar_t *cg_explosionsRing;
extern cvar_t *cg_explosionsDust;
extern cvar_t *cg_gibs;
extern cvar_t *cg_outlineModels;
extern cvar_t *cg_outlineWorld;
extern cvar_t *cg_outlinePlayers;

extern cvar_t *cg_drawEntityBoxes;
extern cvar_t *cg_fov;
extern cvar_t *cg_oldMovement;
extern cvar_t *cg_noAutohop;
extern cvar_t *cg_zoomSens;
extern cvar_t *cg_particles;
extern cvar_t *cg_showhelp;
extern cvar_t *cg_predictLaserBeam;
extern cvar_t *cg_voiceChats;
#ifdef CGAMEGETLIGHTORIGIN
extern cvar_t *cg_shadows;
#endif
extern cvar_t *cg_showSelfShadow;
extern cvar_t *cg_laserBeamSubdivisions;
extern cvar_t *cg_projectileAntilagOffset;
extern cvar_t *cg_raceGhosts;
extern cvar_t *cg_raceGhostsAlpha;
extern cvar_t *cg_chatBeep;
extern cvar_t *cg_chatFilter;
extern cvar_t *cg_chatFilterTV;

//force models
extern cvar_t *cg_teamPLAYERSmodel;
extern cvar_t *cg_teamALPHAmodel;
extern cvar_t *cg_teamBETAmodel;

extern cvar_t *cg_teamPLAYERSskin;
extern cvar_t *cg_teamALPHAskin;
extern cvar_t *cg_teamBETAskin;

extern cvar_t *cg_teamPLAYERScolor;
extern cvar_t *cg_teamALPHAcolor;
extern cvar_t *cg_teamBETAcolor;

extern cvar_t *cg_forceMyTeamAlpha;
extern cvar_t *cg_forceTeamPlayersTeamBeta;

extern cvar_t *cg_teamColoredBeams;

extern cvar_t *cg_demoname;

extern cvar_t *cg_playList;
extern cvar_t *cg_playListShuffle;

#define CG_Malloc( size ) trap_MemAlloc( size, __FILE__, __LINE__ )
#define CG_Free( data ) trap_MemFree( data, __FILE__, __LINE__ )

int CG_API( void );
void CG_Init( unsigned int playerNum, int vidWidth, int vidHeight, qboolean demoplaying, qboolean pure, unsigned int snapFrameTime, int protocol, int sharedSeed );
void CG_Shutdown( void );
void CG_ValidateItemDef( int tag, char *name );
void CG_Printf( const char *format, ... );
void CG_Error( const char *format, ... );
void CG_Reset( void );
char *_CG_CopyString( const char *in, const char *filename, int fileline );
#define CG_CopyString( in ) _CG_CopyString( in, __FILE__, __LINE__ )

void CG_UseItem( const char *name );
void CG_RegisterCGameCommands( void );
void CG_UnregisterCGameCommands( void );
void CG_RegisterDemoCommands( void );
void CG_UnregisterDemoCommands( void );
void CG_UpdateTVServerString( void );
void CG_AddAward( const char *str );
void CG_OverrideWeapondef( int index, char *cstring );

void CG_StartBackgroundTrack( void );
void CG_LocalPrint( qboolean team, const char *format, ... );

//
// cg_svcmds.c
//
void CG_Cmd_DemoGet_f( void );
void CG_ConfigString( int i, char *s );
void CG_GameCommand( const char *command );

//
// cg_teams.c
//
void CG_RegisterTeamColor( int team );
void CG_RegisterForceModels( void );
void CG_SetSceneTeamColors( void );
pmodelinfo_t *CG_PModelForCentity( centity_t *cent );
struct skinfile_s *CG_SkinForCentity( centity_t *cent );
vec_t *CG_TeamColor( int team, vec4_t color );
qbyte *CG_TeamColorForEntity( int entNum, byte_vec4_t color );
qbyte *CG_PlayerColorForEntity( int entNum, byte_vec4_t color );

//
// cg_view.c
//
extern cvar_t *cg_thirdPerson;
extern cvar_t *cg_thirdPersonAngle;
extern cvar_t *cg_thirdPersonRange;

void CG_ResetKickAngles( void );
void CG_ResetColorBlend( void );

void CG_StartKickAnglesEffect( vec3_t source, float knockback, float radius, int time );
void CG_StartColorBlendEffect( float r, float g, float b, float a, int time );
float CG_SetSensitivityScale( const float sens );
void CG_ViewSmoothPredictedSteps( vec3_t vieworg );
void CG_RenderView( float frameTime, float realFrameTime, int realTime, unsigned int serverTime, float stereo_separation, unsigned int extrapolationTime );
void CG_AddKickAngles( vec3_t viewangles );
void CG_ChaseStep( int step );

//
// cg_lents.c
//
void CG_ClearLocalEntities( void );
void CG_AddLocalEntities( void );

void CG_AddLaser( vec3_t start, vec3_t end, float radius, int colors, struct shader_s *shader );
void CG_BulletExplosion( vec3_t origin, vec_t *dir, trace_t *trace );
void CG_BubbleTrail( vec3_t start, vec3_t end, int dist );
void CG_Explosion1( vec3_t pos );
void CG_Explosion2( vec3_t pos );
void CG_NewRocketTrail( centity_t *cent );
void CG_NewGrenadeTrail( centity_t *cent );
void CG_NewBloodTrail( centity_t *cent );
void CG_BloodDamageEffect( vec3_t origin, vec3_t dir, int damage );
void CG_CartoonHitEffect( vec3_t origin, vec3_t dir, int damage );
void CG_NewElectroBeamPuff( centity_t *cent, vec3_t origin, vec3_t dir );
void CG_FlagTrail( vec3_t origin, vec3_t start, vec3_t end, float r, float g, float b );
void CG_GreenLaser( vec3_t start, vec3_t end );
void CG_SmallPileOfGibs( vec3_t origin, int damage, const vec3_t initialVelocity );
void CG_PlasmaExplosion( vec3_t pos, vec3_t dir, int fire_mode, float radius );
void CG_GrenadeExplosionMode( vec3_t pos, vec3_t dir, int fire_mode, float radius );
void CG_GenericExplosion( vec3_t pos, vec3_t dir, int fire_mode, float radius );
void CG_RocketExplosionMode( vec3_t pos, vec3_t dir, int fire_mode, float radius );
void CG_ElectroTrail2( vec3_t start, vec3_t end, int team );
void CG_ImpactSmokePuff( vec3_t origin, vec3_t dir, float radius, float alpha, int time, int speed );
void CG_BoltExplosionMode( vec3_t pos, vec3_t dir, int fire_mode );
void CG_InstaExplosionMode( vec3_t pos, vec3_t dir, int fire_mode );
void CG_BladeImpact( vec3_t pos, vec3_t dir );
void CG_GunBladeBlastImpact( vec3_t pos, vec3_t dir, float radius );
void CG_PModel_SpawnTeleportEffect( centity_t *cent );
void CG_SpawnSprite( vec3_t origin, vec3_t velocity, vec3_t accel,
					float radius, int time, int bounce, qboolean expandEffect, qboolean shrinkEffect,
					float r, float g, float b, float a,
					float light, float lr, float lg, float lb, struct shader_s *shader );

void CG_Dash( entity_state_t *state );
void CG_SpawnTracer( vec3_t origin, vec3_t dir, vec3_t dir_per1, vec3_t dir_per2 );
void CG_Explosion_Puff_2( vec3_t pos, vec3_t vel, int radius );
void CG_DustCircle( vec3_t pos, vec3_t dir, float radius, int count );
void CG_ExplosionsDust( vec3_t pos, vec3_t dir, float radius);

//
// cg_decals.c
//
extern cvar_t *cg_addDecals;

void CG_ClearDecals( void );
void CG_SpawnDecal( vec3_t origin, vec3_t dir, float orient, float radius,
                    float r, float g, float b, float a, float die, float fadetime, qboolean fadealpha, struct shader_s *shader );
void CG_AddDecals( void );

//
// cg_polys.c	-	wsw	: jal
//
extern cvar_t *cg_ebbeam_old;
extern cvar_t *cg_ebbeam_width;
extern cvar_t *cg_ebbeam_alpha;
extern cvar_t *cg_ebbeam_time;
extern cvar_t *cg_instabeam_width;
extern cvar_t *cg_instabeam_alpha;
extern cvar_t *cg_instabeam_time;

void CG_ClearPolys( void );
void CG_AddPolys( void );
void CG_KillPolyBeamsByTag( int key );
void CG_QuickPolyBeam( vec3_t start, vec3_t end, int width, struct shader_s *shader );
void CG_LaserGunPolyBeam( vec3_t start, vec3_t end, vec4_t color, int key );
void CG_ElectroPolyBeam( vec3_t start, vec3_t end, int team );
void CG_InstaPolyBeam( vec3_t start, vec3_t end, int team );
void CG_PLink( vec3_t start, vec3_t end, vec4_t color, int flags );

//
// cg_effects.c
//

//racesow - client console variables
cvar_t *rc_showPlayerTrails;
cvar_t *rc_playerTrailsColor;
cvar_t *rc_playerTrailsAlpha;
cvar_t *rc_playerTrailsSize;
void RC_AddLinearTrail( centity_t *cent, float lifetime );
//!racesow

void CG_ClearEffects( void );

void CG_AddLightToScene( vec3_t org, float radius, float r, float g, float b, struct shader_s *shader );
void CG_AddDlights( void );
#ifdef CGAMEGETLIGHTORIGIN
void CG_AllocShadeBox( int entNum, const vec3_t origin, const vec3_t mins, const vec3_t maxs, struct shader_s *shader );
void CG_AddShadeBoxes( void );
#endif
void CG_RunLightStyles( void );
void CG_SetLightStyle( int i );
void CG_AddLightStyles( void );

void CG_ClearFragmentedDecals( void );
void CG_AddFragmentedDecal( vec3_t origin, vec3_t dir, float orient, float radius,
							 float r, float g, float b, float a, struct shader_s *shader );

void CG_AddParticles( void );
void CG_ParticleEffect( vec3_t org, vec3_t dir, float r, float g, float b, int count );
void CG_ParticleEffect2( vec3_t org, vec3_t dir, float r, float g, float b, int count );
void CG_BlasterTrail( vec3_t start, vec3_t end );
void CG_FlyEffect( centity_t *ent, vec3_t origin );
void CG_ElectroIonsTrail( vec3_t start, vec3_t end );
void CG_ElectroWeakTrail( vec3_t start, vec3_t end, vec4_t color );
void CG_ImpactPuffParticles( vec3_t org, vec3_t dir, int count, float scale, float r, float g, float b, float a, struct shader_s *shader );

//
// cg_test.c - debug only
//
#ifndef PUBLIC_BUILD
void CG_DrawTestLine( vec3_t start, vec3_t end );
void CG_DrawTestBox( vec3_t origin, vec3_t mins, vec3_t maxs, vec3_t angles );
void CG_AddTest( void );
#endif

//
//	cg_vweap.c - client weapon
//
void CG_AddViewWeapon( cg_viewweapon_t *viewweapon );
void CG_CalcViewWeapon( cg_viewweapon_t *viewweapon );
void CG_ViewWeapon_StartAnimationEvent( int newAnim );
void CG_ViewWeapon_RefreshAnimation( cg_viewweapon_t *viewweapon );

//
// cg_events.c
//
//extern cvar_t *cg_footSteps;
extern cvar_t *cg_damage_indicator;
extern cvar_t *cg_damage_indicator_time;
extern cvar_t *cg_pickup_flash;
extern cvar_t *cg_weaponAutoSwitch;

void CG_FireEvents( qboolean early );
void CG_EntityEvent( entity_state_t *ent, int ev, int parm, qboolean predicted );
void CG_AddAnnouncerEvent( struct sfx_s *sound, qboolean queued );
void CG_ReleaseAnnouncerEvents( void );
void CG_ClearAnnouncerEvents( void );

// I don't know where to put these ones
void CG_WeaponBeamEffect( centity_t *cent );
void CG_LaserBeamEffect( centity_t *cent );


//
// cg_chat.c
//
extern cvar_t *con_chatCGame;

void CG_InitChat( cg_gamechat_t *chat );
void CG_StackChatString( cg_gamechat_t *chat, const char *str );
void CG_DrawChat( cg_gamechat_t *chat, int x, int y, char *fontName, struct mufont_s *font, 
				 int width, int height, int padding_x, int padding_y, vec4_t backColor, struct shader_s *backShader );

//=================================================
