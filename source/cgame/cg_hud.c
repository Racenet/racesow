/*
Copyright (C) 2006 Pekka Lampila ("Medar"), Damien Deville ("Pb")
and German Garcia Fernandez ("Jal") for Chasseur de bots association.


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

/*
* Warsow hud scripts
*/

#include "cg_local.h"

#define TEAM_OWN    ( GS_MAX_TEAMS + 1 )
#define TEAM_ENEMY  ( GS_MAX_TEAMS + 2 )

extern cvar_t *cg_weaponlist;
extern cvar_t *cg_debug_HUD;
extern cvar_t *cg_clientHUD;

cvar_t *cg_showminimap;
cvar_t *cg_showitemtimers;
cvar_t *cg_placebo;
//cvar_t *cg_strafeHUD; //racesow - moved to cg_screen.c

//=============================================================================

enum { DEFAULTSCALE=0, NOSCALE, SCALEBYWIDTH, SCALEBYHEIGHT };

typedef struct
{
	char *name;
	int value;
} constant_numeric_t;

static const constant_numeric_t cg_numeric_constants[] = {
	{ "NOTSET", STAT_NOTSET },

	// teams
	{ "TEAM_SPECTATOR", TEAM_SPECTATOR },
	{ "TEAM_PLAYERS", TEAM_PLAYERS },
	{ "TEAM_ALPHA", TEAM_ALPHA },
	{ "TEAM_BETA", TEAM_BETA },

	// align
	{ "LEFT", 1 },
	{ "CENTER", 2 },
	{ "RIGHT", 3 },
	{ "TOP", 1 },
	{ "MIDDLE", 2 },
	{ "BOTTOM", 3 },

	{ "WIDTH", 800 },
	{ "HEIGHT", 600 },

	// scale
	{ "DEFAULTSCALE", DEFAULTSCALE },
	{ "NOSCALE", NOSCALE },
	{ "SCALEBYWIDTH", SCALEBYWIDTH },
	{ "SCALEBYHEIGHT", SCALEBYHEIGHT },

	// match states
	{ "MATCH_STATE_NONE", MATCH_STATE_NONE },
	{ "MATCH_STATE_WARMUP", MATCH_STATE_WARMUP },
	{ "MATCH_STATE_COUNTDOWN", MATCH_STATE_COUNTDOWN },
	{ "MATCH_STATE_PLAYTIME", MATCH_STATE_PLAYTIME },
	{ "MATCH_STATE_POSTMATCH", MATCH_STATE_POSTMATCH },
	{ "MATCH_STATE_WAITEXIT", MATCH_STATE_WAITEXIT },

	// weapon
	{ "WEAP_GUNBLADE", WEAP_GUNBLADE },
	{ "WEAP_MACHINEGUN", WEAP_MACHINEGUN },
	{ "WEAP_RIOTGUN", WEAP_RIOTGUN },
	{ "WEAP_GRENADELAUNCHER", WEAP_GRENADELAUNCHER },
	{ "WEAP_ROCKETLAUNCHER", WEAP_ROCKETLAUNCHER },
	{ "WEAP_PLASMAGUN", WEAP_PLASMAGUN },
	{ "WEAP_LASERGUN", WEAP_LASERGUN },
	{ "WEAP_ELECTROBOLT", WEAP_ELECTROBOLT },

	{ "NOGUN", 0 },
	{ "GUN", 1 },

	{ NULL, 0 }
};

// picnames for custom icons (weaponlist)
static char *customWeaponPics[WEAP_TOTAL-1];
static char *customNoGunWeaponPics[WEAP_TOTAL-1];
static char *customWeaponSelectPic;

//=============================================================================

static int CG_GetStatEnemyTeam( void *parameter )
{
	return ( ( cg.predictedPlayerState.stats[STAT_TEAM] == TEAM_ALPHA ) ? TEAM_BETA :
		( ( cg.predictedPlayerState.stats[STAT_TEAM] == TEAM_BETA ) ? TEAM_ALPHA : TEAM_SPECTATOR ) );
}

static int CG_GetStatValue( void *parameter )
{
	assert( (qintptr)parameter >= 0 && (qintptr)parameter < MAX_STATS );

	return cg.predictedPlayerState.stats[(qintptr)parameter];
}

static int CG_GetRaceStatValue( void *parameter )
{
	return CG_GetStatValue( parameter );
}

static int CG_GetPOVnum( void *parameter )
{
	return ( cg.predictedPlayerState.POVnum != cgs.playerNum + 1 ) ? cg.predictedPlayerState.POVnum : STAT_NOTSET;
}

static int CG_GetArmorItem( void *parameter )
{
	return GS_Armor_TagForCount( cg.predictedPlayerState.stats[STAT_ARMOR] );
}

static float _getspeed( void )
{
	vec3_t hvel;

	VectorSet( hvel, cg.predictedPlayerState.pmove.velocity[0], cg.predictedPlayerState.pmove.velocity[1], 0 );

	return VectorLength( hvel );
}

static int CG_GetSpeed( void *parameter )
{
	return (int)_getspeed();
}

static int CG_GetSpeedVertical( void *parameter )
{
	return cg.predictedPlayerState.pmove.velocity[2];
}

static int CG_GetFPS( void *parameter )
{
#define FPSSAMPLESCOUNT 32
#define FPSSAMPLESMASK ( FPSSAMPLESCOUNT-1 )
	static int fps;
	static double oldtime;
	static int oldframecount;
	static float frameTimes[FPSSAMPLESCOUNT];
	static float avFrameTime;
	int i;
	double t;

	frameTimes[cg.frameCount & FPSSAMPLESMASK] = cg.realFrameTime;

	if( cg_showFPS->integer != 2 )
	{
		for( avFrameTime = 0.0f, i = 0; i < FPSSAMPLESCOUNT; i++ )
		{
			avFrameTime += frameTimes[( cg.frameCount-i ) & FPSSAMPLESMASK];
		}
		avFrameTime /= FPSSAMPLESCOUNT;
		fps = (int)( 1.0f/avFrameTime );
	}
	else
	{
		t = cg.realTime * 0.001f;
		if( ( t - oldtime ) >= 0.25 )
		{                       // updates 4 times a second
			fps = ( cg.frameCount - oldframecount ) / ( t - oldtime ) + 0.5;
			oldframecount = cg.frameCount;
			oldtime = t;
		}
	}

	return fps;
}

static int CG_GetPowerupTime( void *parameter )
{
	int powerup = (qintptr)parameter;
	return cg.predictedPlayerState.inventory[powerup];
}

static int CG_GetMatchState( void *parameter )
{
	if( cgs.demoTutorial )
		return MATCH_STATE_NONE;
	return GS_MatchState();
}

static int CG_GetMatchTime( void *parameter )
{
	return GS_MatchStartTime() + GS_MatchDuration() - ( GS_MatchPaused() ? cg.frame.serverTime : cg.time );
}

static int CG_GetMatchDuration( void *parameter )
{
	return GS_MatchDuration();
}

static int CG_GetOvertime( void *parameter )
{
	return GS_MatchExtended();
}

static int CG_GetInstagib( void *parameter )
{
	return GS_Instagib();
}

static int CG_GetTeamBased( void *parameter )
{
	return GS_TeamBasedGametype();
}

static int CG_InvidualGameType( void *parameter )
{
	return GS_InvidualGameType();
}

static int CG_RaceGameType( void *parameter )
{
	return GS_RaceGametype();
}

static int CG_Paused( void *parameter )
{
	return GS_MatchPaused();
}

static int CG_GetZoom( void *parameter )
{
	return ( !cg.view.thirdperson && ( cg.predictedPlayerState.pmove.stats[PM_STAT_ZOOMTIME] != 0 ) );
}

static int CG_GetVidWidth( void *parameter )
{
	return cgs.vidWidth;
}

static int CG_GetVidHeight( void *parameter )
{
	return cgs.vidHeight;
}

static int CG_GetStunned( void *parameter )
{
	return cg.predictedPlayerState.pmove.stats[PM_STAT_STUN];
}

static int CG_GetCvar( void *parameter )
{
	return trap_Cvar_Value( (char *)parameter );
}

static int CG_GetDamageIndicatorDirValue( void *parameter )
{
	float frac = 0;
	int index = (qintptr)parameter;

	if( cg.damageBlends[index] > cg.time && !cg.view.thirdperson )
	{
		frac = ( cg.damageBlends[index] - cg.time ) / 300.0f;
		clamp( frac, 0.0f, 1.0f );
	}

	return frac * 1000;
}

static int CG_GetCurrentWeaponInventoryData( void *parameter )
{
	gs_weapon_definition_t *weapondef = GS_GetWeaponDef( cg.predictedPlayerState.stats[STAT_WEAPON] );
	firedef_t *firedef;
	int result;

	switch( (qintptr)parameter )
	{
	case 0: // AMMO_ITEM
	default:
		firedef = GS_FiredefForPlayerState( &cg.predictedPlayerState, cg.predictedPlayerState.stats[STAT_WEAPON] );
		result = firedef->ammo_id;
		break;
	case 1: // STRONG AMMO COUNT
		result = cg.predictedPlayerState.inventory[weapondef->firedef.ammo_id];
		break;
	case 2: // WEAK AMMO COUNT
		result = cg.predictedPlayerState.inventory[weapondef->firedef_weak.ammo_id];
		break;
	}

	return result;
}

static int CG_DownloadInProgress( void *parameter )
{
	const char *str;

	str = trap_Cvar_String( "cl_download_name" );
	if( str[0] )
		return 1;
	return 0;
}

static int CG_GetShowItemTimers( void *parameter )
{
	if( cgs.tv )
		return (int)trap_Cvar_Value( (char *)parameter ) & 2;
	return (int)trap_Cvar_Value( (char *)parameter ) & 1;
}

static int CG_GetItemTimer( void *parameter )
{
	int num = (qintptr)parameter;
	centity_t *cent;

	cent = CG_GetItemTimerEnt( num );
	if( !cent )
		return 0;
	return cent->item->tag;
}

static int CG_GetItemTimerCount( void *parameter )
{
	int num = (qintptr)parameter;
	centity_t *cent;

	cent = CG_GetItemTimerEnt( num );
	if( !cent )
		return 0;
	return cent->ent.frame;
}

static int CG_GetItemTimerLocation( void *parameter )
{
	int num = (qintptr)parameter;
	centity_t *cent;

	cent = CG_GetItemTimerEnt( num );
	if( !cent )
		return 0;
	return cent->current.modelindex2;
}

static int CG_GetItemTimerTeam( void *parameter )
{
	int num = (qintptr)parameter;
	centity_t *cent;

	cent = CG_GetItemTimerEnt( num );
	if( !cent )
		return 0;
	return max( cent->current.modelindex-1, 0 );
}

// ch : backport some of racesow hud elements
/*********************************************************************************
lm: edit for race mod,
	adds bunch of vars to the hud.

*********************************************************************************/

//lm: for readability
enum race_index {
	//racesow
	cp1,
	cp2,
	cp3,
	cp4,
	cp5,
	cp6,
	cp7,
	cp8,
	cp9,
	cp10,
	cp11,
	cp12,
	cp13,
	cp14,
	cp15,
	//!racesow
	mouse_x,
	mouse_y,
	jumpspeed,
	move_an,
	diff_an,
	strafe_an,
	max_index
};

//racesow
/*
%mouse_x - display currently mouse x (-18000,18000)
%mouse_y - display currently mouse y (-9000,9000)
%acceleration - speed increase over last 1/10th of second (0, MAX_SPEED
%jumpspeed - speed at the last jump (either wall or dash)
%moveangle - angle of the direction currently moving towards (-18000,18000)
%cp1 ... %cp15 - index to use with drawCheckPoint (only use if %cpXX != #NOTSET)
%diff_angle - difference of %mouse_x and %moveangle (%mouse_x - %moveangle)
	moving backwards gives the same angles like moving forwards (-9000,9000)
%strafe_angle - the optimal strafe angle based on current speed and FrameTime
%max_accel - the maximum possible acceleration based on current speed, same unit as %acceleration
%rocketaccel - returns big speed differences between two frames
*/

int race_jump = 0;
//!racesow

//racesow - the following functions have differences in calculations, because warsow backported from an old version. warsow huds SHOULD still work with racesow
static int CG_GetRaceVars( void* parameter )
{
	int index = (qintptr)parameter;
	float fNum;
	vec3_t hor_vel, an;

	if( GS_MatchState() != MATCH_STATE_WARMUP && !GS_RaceGametype() )
		return 0;

	switch( index ) {
		case cp1: case cp2: case cp3: case cp4: case cp5: case cp6: case cp7: case cp8:
		case cp9: case cp10: case cp11: case cp12: case cp13: case cp14: case cp15:
			return cg.checkpoints[index];

		case diff_an:
			// difference of look and move angles
			VectorSet( hor_vel, cg.predictedPlayerState.pmove.velocity[0], cg.predictedPlayerState.pmove.velocity[1], 0 );
			VecToAngles( hor_vel, an );
			fNum = cg.predictedPlayerState.viewangles[YAW] - an[YAW];

			// forward and backward movement get the same angles
			while ( fNum < -90 )
				fNum += 180;
			while ( fNum > 90 )
				fNum -= 180;
			return Q_rint( 100*fNum );

		case strafe_an:
			// optimal strafing angle
			fNum = RAD2DEG( acos( cg.predictedPlayerState.pmove.stats[PM_STAT_MAXSPEED]*( 1 - cg.frameTime )/_getspeed() ) ) - 45;
			if ( fNum > 0 )
				return Q_rint( 100*fNum );
			else
				return 0;
		case move_an:
			// angle of current moving direction
			VectorSet( hor_vel, cg.predictedPlayerState.pmove.velocity[0], cg.predictedPlayerState.pmove.velocity[1], 0 );
			VecToAngles( hor_vel, an );
			return Q_rint( 100*an[YAW] );
		case jumpspeed:
			return race_jump;
		case mouse_x:
			return Q_rint( 100*cg.predictedPlayerState.viewangles[YAW] );
		case mouse_y:
			return Q_rint( 100*cg.predictedPlayerState.viewangles[PITCH] );
		default:
			return STAT_NOTSET;
	}
}

static int CG_GetMaxAccel( void* parameter )
{
	int base_speed = cg.predictedPlayerState.pmove.stats[PM_STAT_MAXSPEED];
	float base_accel = base_speed * cg.frameTime;

	float speed = _getspeed();
	return (int)( 1000 *
			(
				sqrt( speed*speed + base_accel*( 2*base_speed - base_accel ) )
				- speed
			)
			/ cg.frameTime);
}

static int CG_GetRocketAccel( void* parameter )
{
#define MINSPEEDDIFF 100.0f
#define RASAMPLESCOUNT 2
#define RASAMPLESMASK ( RASAMPLESCOUNT-1 )
	static float speeds[RASAMPLESCOUNT];
	static int speedDiff = 0;
	float newSpeed;

	newSpeed = _getspeed();
	speeds[cg.frameCount & RASAMPLESMASK] = newSpeed;
	if ( fabs( newSpeed - speeds[(cg.frameCount-1) & RASAMPLESMASK] ) > MINSPEEDDIFF )
	{
		speedDiff = (int)( newSpeed - speeds[(cg.frameCount-1) & RASAMPLESMASK] );
	}
	return speedDiff;
}

static int CG_GetAccel( void* parameter )
{
#define ACCEL_SAMPLE_COUNT 16
#define ACCEL_SAMPLE_MASK (ACCEL_SAMPLE_COUNT-1)
	int i;
	float t, dt;
	float accel;
	float newSpeed;
	static float oldSpeed = 0.0;
	static float oldTime = 0.0;
	static float accelHistory[ACCEL_SAMPLE_COUNT] = {0.0};
	static int sampleCount = 0;

	t = cg.time * 0.001f;
	dt = t - oldTime;
	if( dt < 0.0 )
	{
		oldTime = t;
	}
	else if( dt > 0.0 )
	{
		// raw acceleration
		newSpeed = _getspeed();
		accel = ( newSpeed - oldSpeed ) / dt;
		accelHistory[sampleCount&ACCEL_SAMPLE_MASK] = accel;
		sampleCount++;
		oldSpeed = newSpeed;
		oldTime = t;
	}

	// average accel for n frames (TODO: emphasis on later frames)
	accel = 0.0f;
	for( i = 0; i < ACCEL_SAMPLE_COUNT; i++ )
		accel += accelHistory[i];
	accel /= (float)(ACCEL_SAMPLE_COUNT);

	if( GS_MatchState() != MATCH_STATE_WARMUP && !GS_RaceGametype() )
		return 0;

	return (int)(1000*accel);
}

void CG_AddJumpspeed( void )
{
	race_jump = CG_GetSpeed(0);
}
//!racesow

typedef struct
{
	char *name;
	int ( *func )( void *parameter );
	void *parameter;
} reference_numeric_t;

static const reference_numeric_t cg_numeric_references[] =
{
	// stats
	{ "HEALTH", CG_GetStatValue, (void *)STAT_HEALTH },
	{ "ARMOR", CG_GetStatValue, (void *)STAT_ARMOR },
	{ "WEAPON_ITEM", CG_GetStatValue, (void *)STAT_WEAPON },
	{ "PENDING_WEAPON", CG_GetStatValue, (void *)STAT_PENDING_WEAPON },

	{ "PICKUP_ITEM", CG_GetStatValue, (void *)STAT_PICKUP_ITEM },

	{ "SCORE", CG_GetStatValue, (void *)STAT_SCORE },
	{ "TEAM", CG_GetStatValue, (void *)STAT_TEAM },
	{ "REALTEAM", CG_GetStatValue, (void *)STAT_REALTEAM },
	{ "TEAM_ENEMY", CG_GetStatEnemyTeam, NULL },
	{ "RESPAWN_TIME", CG_GetStatValue, (void *)STAT_NEXT_RESPAWN },

	{ "POINTED_PLAYER", CG_GetStatValue, (void *)STAT_POINTED_PLAYER },
	{ "POINTED_TEAMPLAYER", CG_GetStatValue, (void *)STAT_POINTED_TEAMPLAYER },

	{ "TEAM_ALPHA_SCORE", CG_GetStatValue, (void *)STAT_TEAM_ALPHA_SCORE },
	{ "TEAM_BETA_SCORE", CG_GetStatValue, (void *)STAT_TEAM_BETA_SCORE },

	{ "PROGRESS_SELF", CG_GetStatValue, (void *)STAT_PROGRESS_SELF },
	{ "PROGRESS_OTHER", CG_GetStatValue, (void *)STAT_PROGRESS_OTHER },
	{ "PROGRESS_ALPHA", CG_GetStatValue, (void *)STAT_PROGRESS_ALPHA },
	{ "PROGRESS_BETA", CG_GetStatValue, (void *)STAT_PROGRESS_BETA },

	{ "IMAGE_SELF", CG_GetStatValue, (void *)STAT_IMAGE_SELF },
	{ "IMAGE_OTHER", CG_GetStatValue, (void *)STAT_IMAGE_OTHER },
	{ "IMAGE_ALPHA", CG_GetStatValue, (void *)STAT_IMAGE_ALPHA },
	{ "IMAGE_BETA", CG_GetStatValue, (void *)STAT_IMAGE_BETA },

	{ "TIME_SELF", CG_GetRaceStatValue, (void *)STAT_TIME_SELF },
	{ "TIME_BEST", CG_GetRaceStatValue, (void *)STAT_TIME_BEST },
	{ "TIME_RECORD", CG_GetRaceStatValue, (void *)STAT_TIME_RECORD },
	{ "TIME_ALPHA", CG_GetRaceStatValue, (void *)STAT_TIME_ALPHA },
	{ "TIME_BETA", CG_GetRaceStatValue, (void *)STAT_TIME_BETA },

	{ "MESSAGE_SELF", CG_GetStatValue, (void *)STAT_MESSAGE_SELF },
	{ "MESSAGE_OTHER", CG_GetStatValue, (void *)STAT_MESSAGE_OTHER },
	{ "MESSAGE_ALPHA", CG_GetStatValue, (void *)STAT_MESSAGE_ALPHA },
	{ "MESSAGE_BETA", CG_GetStatValue, (void *)STAT_MESSAGE_BETA },

	// inventory grabs
	{ "AMMO_ITEM", CG_GetCurrentWeaponInventoryData, (void *)0 },
	{ "AMMO", CG_GetCurrentWeaponInventoryData, (void *)1 },
	{ "WEAK_AMMO", CG_GetCurrentWeaponInventoryData, (void *)2 },

	// other
	{ "CHASING", CG_GetPOVnum, NULL },
	{ "ARMOR_ITEM", CG_GetArmorItem, NULL },
	{ "SPEED", CG_GetSpeed, NULL },
	{ "SPEED_VERTICAL", CG_GetSpeedVertical, NULL },
	{ "FPS", CG_GetFPS, NULL },
	{ "MATCH_STATE", CG_GetMatchState, NULL },
	{ "MATCH_TIME", CG_GetMatchTime, NULL },
	{ "MATCH_DURATION", CG_GetMatchDuration, NULL },
	{ "OVERTIME", CG_GetOvertime, NULL },
	{ "INSTAGIB", CG_GetInstagib, NULL },
	{ "TEAMBASED", CG_GetTeamBased, NULL },
	{ "INDIVIDUAL", CG_InvidualGameType, NULL },
	{ "RACE", CG_RaceGameType, NULL },
	{ "PAUSED", CG_Paused, NULL },
	{ "ZOOM", CG_GetZoom, NULL },
	{ "VIDWIDTH", CG_GetVidWidth, NULL },
	{ "VIDHEIGHT", CG_GetVidHeight, NULL },
	{ "STUNNED", CG_GetStunned, NULL },

	{ "POWERUP_QUAD_TIME", CG_GetPowerupTime, (void *)POWERUP_QUAD },
	{ "POWERUP_WARSHELL_TIME", CG_GetPowerupTime, (void *)POWERUP_SHELL },
	{ "POWERUP_REGEN_TIME", CG_GetPowerupTime, (void *)POWERUP_REGEN },

	{ "DAMAGE_INDICATOR_TOP", CG_GetDamageIndicatorDirValue, (void *)0 },
	{ "DAMAGE_INDICATOR_RIGHT", CG_GetDamageIndicatorDirValue, (void *)1 },
	{ "DAMAGE_INDICATOR_BOTTOM", CG_GetDamageIndicatorDirValue, (void *)2 },
	{ "DAMAGE_INDICATOR_LEFT", CG_GetDamageIndicatorDirValue, (void *)3 },

// ch : backport racesow hud elements
//lm: race stuff
	{ "MOUSE_X", CG_GetRaceVars, (void *)mouse_x },
	{ "MOUSE_Y", CG_GetRaceVars, (void *)mouse_y },
	{ "ACCELERATION", CG_GetAccel, NULL },
	{ "ROCKETACCEL", CG_GetRocketAccel, NULL}, // racesow
	{ "JUMPSPEED", CG_GetRaceVars, (void *)jumpspeed }, // racesow
	{ "MOVEANGLE", CG_GetRaceVars, (void *)move_an	},
	{ "STRAFEANGLE", CG_GetRaceVars, (void *)strafe_an },
	{ "DIFF_ANGLE", CG_GetRaceVars, (void *)diff_an	},
    //racesow
	{ "MAX_ACCEL", CG_GetMaxAccel, NULL },
	{ "SHOW_ACCEL", CG_GetCvar, "cg_showAcceleration" },

	{ "CP1", CG_GetRaceVars, (void *)cp1 },
	{ "CP2", CG_GetRaceVars, (void *)cp2 },
	{ "CP3", CG_GetRaceVars, (void *)cp3 },
	{ "CP4", CG_GetRaceVars, (void *)cp4 },
	{ "CP5", CG_GetRaceVars, (void *)cp5 },
	{ "CP6", CG_GetRaceVars, (void *)cp6 },
	{ "CP7", CG_GetRaceVars, (void *)cp7 },
	{ "CP8", CG_GetRaceVars, (void *)cp8 },
	{ "CP9", CG_GetRaceVars, (void *)cp9 },
	{ "CP10", CG_GetRaceVars, (void *)cp10 },
	{ "CP11", CG_GetRaceVars, (void *)cp11 },
	{ "CP12", CG_GetRaceVars, (void *)cp12 },
	{ "CP13", CG_GetRaceVars, (void *)cp13 },
	{ "CP14", CG_GetRaceVars, (void *)cp14 },
	{ "CP15", CG_GetRaceVars, (void *)cp15 },
	//!racesow

	// cvars
	{ "SHOW_FPS", CG_GetCvar, "cg_showFPS" },
	{ "SHOW_OBITUARIES", CG_GetCvar, "cg_showObituaries" },
	{ "SHOW_PICKUP", CG_GetCvar, "cg_showPickup" },
	{ "SHOW_POINTED_PLAYER", CG_GetCvar, "cg_showPointedPlayer" },
	{ "SHOW_PRESSED_KEYS", CG_GetCvar, "cg_showPressedKeys" },
	{ "SHOW_SPEED", CG_GetCvar, "cg_showSpeed" },
	{ "SHOW_TEAM_LOCATIONS", CG_GetCvar, "cg_showTeamLocations" },
	{ "SHOW_TIMER", CG_GetCvar, "cg_showTimer" },
	{ "SHOW_AWARDS", CG_GetCvar, "cg_showAwards" },
	{ "SHOW_ZOOM_EFFECT", CG_GetCvar, "cg_showZoomEffect" },
	{ "SHOW_R_SPEEDS", CG_GetCvar, "r_speeds" },
	{ "SHOW_ITEM_TIMERS", CG_GetShowItemTimers, "cg_showItemTimers" },
	{ "SHOW_STRAFE", CG_GetCvar, "cg_strafeHUD" },

	{ "DOWNLOAD_IN_PROGRESS", CG_DownloadInProgress, NULL },
	{ "DOWNLOAD_PERCENT", CG_GetCvar, "cl_download_percent" },

	{ "CHAT_MODE", CG_GetCvar, "con_messageMode" },

	{ "ITEM_TIMER0", CG_GetItemTimer, (void *)0 },
	{ "ITEM_TIMER0_COUNT", CG_GetItemTimerCount, (void *)0 },
	{ "ITEM_TIMER0_LOCATION", CG_GetItemTimerLocation, (void *)0 },
	{ "ITEM_TIMER0_TEAM", CG_GetItemTimerTeam, (void *)0 },
	{ "ITEM_TIMER1", CG_GetItemTimer, (void *)1 },
	{ "ITEM_TIMER1_COUNT", CG_GetItemTimerCount, (void *)1 },
	{ "ITEM_TIMER1_LOCATION", CG_GetItemTimerLocation, (void *)1 },
	{ "ITEM_TIMER1_TEAM", CG_GetItemTimerTeam, (void *)1 },
	{ "ITEM_TIMER2", CG_GetItemTimer, (void *)2 },
	{ "ITEM_TIMER2_COUNT", CG_GetItemTimerCount, (void *)2 },
	{ "ITEM_TIMER2_LOCATION", CG_GetItemTimerLocation, (void *)2 },
	{ "ITEM_TIMER2_TEAM", CG_GetItemTimerTeam, (void *)2 },
	{ "ITEM_TIMER3", CG_GetItemTimer, (void *)3 },
	{ "ITEM_TIMER3_COUNT", CG_GetItemTimerCount, (void *)3 },
	{ "ITEM_TIMER3_LOCATION", CG_GetItemTimerLocation, (void *)3 },
	{ "ITEM_TIMER3_TEAM", CG_GetItemTimerTeam, (void *)3 },
	{ "ITEM_TIMER4", CG_GetItemTimer, (void *)4 },
	{ "ITEM_TIMER4_COUNT", CG_GetItemTimerCount, (void *)4 },
	{ "ITEM_TIMER4_LOCATION", CG_GetItemTimerLocation, (void *)4 },
	{ "ITEM_TIMER4_TEAM", CG_GetItemTimerTeam, (void *)4 },
	{ "ITEM_TIMER5", CG_GetItemTimer, (void *)5 },
	{ "ITEM_TIMER5_COUNT", CG_GetItemTimerCount, (void *)5 },
	{ "ITEM_TIMER5_LOCATION", CG_GetItemTimerLocation, (void *)5 },
	{ "ITEM_TIMER5_TEAM", CG_GetItemTimerTeam, (void *)5 },
	{ "ITEM_TIMER6", CG_GetItemTimer, (void *)6 },
	{ "ITEM_TIMER6_COUNT", CG_GetItemTimerCount, (void *)6 },
	{ "ITEM_TIMER6_LOCATION", CG_GetItemTimerLocation, (void *)6 },
	{ "ITEM_TIMER6_TEAM", CG_GetItemTimerTeam, (void *)6 },
	{ "ITEM_TIMER7", CG_GetItemTimer, (void *)7 },
	{ "ITEM_TIMER7_COUNT", CG_GetItemTimerCount, (void *)7 },
	{ "ITEM_TIMER7_LOCATION", CG_GetItemTimerLocation, (void *)7 },
	{ "ITEM_TIMER7_TEAM", CG_GetItemTimerTeam, (void *)7 },

	{ NULL, NULL, NULL }
};

//=============================================================================

#define MAX_OBITUARIES 32

typedef enum { OBITUARY_NONE, OBITUARY_NORMAL, OBITUARY_TEAM, OBITUARY_SUICIDE, OBITUARY_ACCIDENT } obituary_type_t;

typedef struct obituary_s
{
	obituary_type_t	type;
	unsigned int time;
	char victim[MAX_INFO_VALUE];
	int victim_team;
	char attacker[MAX_INFO_VALUE];
	int attacker_team;
	int mod;
} obituary_t;

static obituary_t cg_obituaries[MAX_OBITUARIES];
static int cg_obituaries_current = -1;

/*
* CG_SC_PrintObituary
*/
void CG_SC_PrintObituary( const char *format, ... )
{
	va_list	argptr;
	char msg[1024];

	va_start( argptr, format );
	Q_vsnprintfz( msg, sizeof( msg ), format, argptr );
	va_end( argptr );

	trap_Print( msg );
	
	CG_StackChatString( &cg.chat, msg );
}

/*
* CG_SC_Obituary
*/
void CG_SC_Obituary( void )
{
	char message[128];
	char message2[128];
	cg_clientInfo_t *victim, *attacker;
	int victimNum = atoi( trap_Cmd_Argv( 1 ) );
	int attackerNum = atoi( trap_Cmd_Argv( 2 ) );
	int mod = atoi( trap_Cmd_Argv( 3 ) );
	int victim_gender = GENDER_MALE;
	obituary_t *current;

	// wsw : jal : extract gender from their player model info, if any
	if( victimNum >= 0 && victimNum < MAX_EDICTS && cg_entPModels[victimNum].pmodelinfo )
		victim_gender = cg_entPModels[victimNum].pmodelinfo->sex;

	victim = &cgs.clientInfo[victimNum - 1];

	if( attackerNum )
	{
		attacker = &cgs.clientInfo[attackerNum - 1];
	}
	else
	{
		attacker = NULL;
	}

	cg_obituaries_current++;
	if( cg_obituaries_current >= MAX_OBITUARIES )
		cg_obituaries_current = 0;
	current = &cg_obituaries[cg_obituaries_current];

	current->time = cg.time;
	if( victim )
	{
		Q_strncpyz( current->victim, victim->name, sizeof( current->victim ) );
		current->victim_team = cg_entities[victimNum].current.team;
	}
	if( attacker )
	{
		Q_strncpyz( current->attacker, attacker->name, sizeof( current->attacker ) );
		current->attacker_team = cg_entities[attackerNum].current.team;
	}
	current->mod = mod;

	GS_Obituary( victim, victim_gender, attacker, mod, message, message2 );

	if( attackerNum )
	{
		if( victimNum != attackerNum )
		{
			// teamkill
			if( cg_entities[attackerNum].current.team == cg_entities[victimNum].current.team &&
			   GS_TeamBasedGametype() )
			{
				current->type = OBITUARY_TEAM;
				if( cg_showObituaries->integer & CG_OBITUARY_CONSOLE )
				{
					CG_LocalPrint( qfalse, "%s%s%s %s %s%s %s%s%s\n", S_COLOR_RED, "TEAMKILL:", S_COLOR_WHITE, victim->name,
					           S_COLOR_WHITE, message, attacker->name, S_COLOR_WHITE, message2 );
				}

				if( ISVIEWERENTITY( attackerNum ) && ( cg_showObituaries->integer & CG_OBITUARY_CENTER ) )
				{
					CG_CenterPrintToUpper( va( "%s%s%s %s\n", S_COLOR_RED, "YOU TEAMFRAGGED", S_COLOR_WHITE, victim->name ) );
				}
			}
			else // good kill
			{
				current->type = OBITUARY_NORMAL;
				if( cg_showObituaries->integer & CG_OBITUARY_CONSOLE )
					CG_LocalPrint( qfalse, "%s %s%s %s%s%s\n", victim->name, S_COLOR_WHITE, message, attacker->name, S_COLOR_WHITE,
					           message2 );

				if( ISVIEWERENTITY( attackerNum ) && ( cg_showObituaries->integer & CG_OBITUARY_CENTER ) )
				{
					CG_CenterPrintToUpper( va( "%s%s %s\n", S_COLOR_WHITE, "YOU FRAGGED", victim->name ) );
				}
			}
		}
		else // suicide
		{
			current->type = OBITUARY_SUICIDE;
			if( cg_showObituaries->integer & CG_OBITUARY_CONSOLE )
				CG_LocalPrint( qfalse, "%s %s%s\n", victim->name, S_COLOR_WHITE, message );
		}
	}
	else // world accidents
	{
		current->type = OBITUARY_ACCIDENT;
		if( cg_showObituaries->integer & CG_OBITUARY_CONSOLE )
			CG_LocalPrint( qfalse, "%s %s%s\n", victim->name, S_COLOR_WHITE, message );
	}
}

static void CG_DrawObituaries( int x, int y, int align, struct mufont_s *font, vec4_t color, int width, int height,
                               int internal_align, unsigned int icon_size )
{
	int i, num, skip, next, w, num_max;
	size_t line_height;
	int xoffset, yoffset;
	obituary_t *obr;
	struct shader_s *pic;
	vec4_t teamcolor;

	if( !( cg_showObituaries->integer & CG_OBITUARY_HUD ) )
		return;

	line_height = max( trap_SCR_strHeight( font ), icon_size );
	num_max = height / line_height;

	if( width < (int)icon_size || !num_max )
		return;

	next = cg_obituaries_current + 1;
	if( next >= MAX_OBITUARIES )
		next = 0;

	num = 0;
	i = next;
	do
	{
		if( cg_obituaries[i].type != OBITUARY_NONE && cg.time - cg_obituaries[i].time <= 5000 )
			num++;
		if( ++i >= MAX_OBITUARIES )
			i = 0;
	}
	while( i != next );

	if( num > num_max )
	{
		skip = num - num_max;
		num = num_max;
	}
	else
	{
		skip = 0;
	}

	y = CG_VerticalAlignForHeight( y, align, height );
	x = CG_HorizontalAlignForWidth( x, align, width );

	xoffset = 0;
	yoffset = 0;

	i = next;
	do
	{
		obr = &cg_obituaries[i];
		if( ++i >= MAX_OBITUARIES )
			i = 0;

		if( obr->type == OBITUARY_NONE || cg.time - obr->time > 5000 )
			continue;

		if( skip > 0 )
		{
			skip--;
			continue;
		}

		switch( obr->mod )
		{
		case MOD_GUNBLADE_W:
		case MOD_GUNBLADE_S:
			pic = CG_MediaShader( cgs.media.shaderWeaponIcon[WEAP_GUNBLADE-1] );
			break;
		case MOD_MACHINEGUN_W:
		case MOD_MACHINEGUN_S:
			pic = CG_MediaShader( cgs.media.shaderWeaponIcon[WEAP_MACHINEGUN-1] );
			break;
		case MOD_RIOTGUN_W:
		case MOD_RIOTGUN_S:
			pic = CG_MediaShader( cgs.media.shaderWeaponIcon[WEAP_RIOTGUN-1] );
			break;
		case MOD_GRENADE_W:
		case MOD_GRENADE_S:
		case MOD_GRENADE_SPLASH_W:
		case MOD_GRENADE_SPLASH_S:
			pic = CG_MediaShader( cgs.media.shaderWeaponIcon[WEAP_GRENADELAUNCHER-1] );
			break;
		case MOD_ROCKET_W:
		case MOD_ROCKET_S:
		case MOD_ROCKET_SPLASH_W:
		case MOD_ROCKET_SPLASH_S:
			pic = CG_MediaShader( cgs.media.shaderWeaponIcon[WEAP_ROCKETLAUNCHER-1] );
			break;
		case MOD_PLASMA_W:
		case MOD_PLASMA_S:
		case MOD_PLASMA_SPLASH_W:
		case MOD_PLASMA_SPLASH_S:
			pic = CG_MediaShader( cgs.media.shaderWeaponIcon[WEAP_PLASMAGUN-1] );
			break;
		case MOD_ELECTROBOLT_W:
		case MOD_ELECTROBOLT_S:
			pic = CG_MediaShader( cgs.media.shaderWeaponIcon[WEAP_ELECTROBOLT-1] );
			break;
		case MOD_INSTAGUN_W:
		case MOD_INSTAGUN_S:
			pic = CG_MediaShader( cgs.media.shaderWeaponIcon[WEAP_INSTAGUN-1] );
			break;
		case MOD_LASERGUN_W:
		case MOD_LASERGUN_S:
			pic = CG_MediaShader( cgs.media.shaderWeaponIcon[WEAP_LASERGUN-1] );
			break;
		default:
			pic = CG_MediaShader( cgs.media.shaderWeaponIcon[WEAP_GUNBLADE-1] ); // FIXME
			break;
		}

		w = 0;
		if( obr->type != OBITUARY_ACCIDENT )
			w += min( trap_SCR_strWidth( obr->attacker, font, 0 ), ( width - icon_size * cgs.vidWidth/800 ) / 2 );
		w += icon_size * cgs.vidWidth/800;
		w += min( trap_SCR_strWidth( obr->victim, font, 0 ), ( width - icon_size * cgs.vidWidth/800 ) / 2 );

		if( internal_align == 1 )
		{
			// left
			xoffset = 0;
		}
		else if( internal_align == 2 )
		{
			// center
			xoffset = ( width - w ) / 2;
		}
		else
		{
			// right
			xoffset = width - w;
		}

		if( obr->type != OBITUARY_ACCIDENT )
		{
			if( GS_TeamBasedGametype() )
			{
				CG_TeamColor( obr->attacker_team, teamcolor );
			}
			else
			{
				Vector4Set( teamcolor, 255, 255, 255, 255 );
			}
			trap_SCR_DrawStringWidth( x + xoffset, y + yoffset + ( line_height - trap_SCR_strHeight( font ) ) / 2,
			                          ALIGN_LEFT_TOP, COM_RemoveColorTokensExt( obr->attacker, qtrue ), ( width - icon_size * cgs.vidWidth/800 ) / 2,
			                          font, teamcolor );
			xoffset += min( trap_SCR_strWidth( obr->attacker, font, 0 ), ( width - icon_size * cgs.vidWidth/800 ) / 2 );
		}
		trap_R_DrawStretchPic( x + xoffset, y + yoffset + ( line_height - icon_size ) / 2, icon_size * cgs.vidWidth/800,
		                       icon_size * cgs.vidHeight/600, 0, 0, 1, 1, colorWhite, pic );
		xoffset += icon_size * cgs.vidWidth/800;
		if( GS_TeamBasedGametype() )
		{
			CG_TeamColor( obr->victim_team, teamcolor );
		}
		else
		{
			Vector4Set( teamcolor, 255, 255, 255, 255 );
		}
		trap_SCR_DrawStringWidth( x + xoffset, y + yoffset + ( (int) line_height ) / 2, ALIGN_LEFT_MIDDLE,
		                          COM_RemoveColorTokensExt( obr->victim, qtrue ), ( width - icon_size * cgs.vidWidth/800 ) / 2, font, teamcolor );
		xoffset += min( trap_SCR_strWidth( obr->victim, font, 0 ), ( width - icon_size * cgs.vidWidth/800 ) / 2 );

		yoffset += line_height;
	}
	while( i != next );
}

//=============================================================================

static void CG_DrawAwards( int x, int y, int align, struct mufont_s *font, vec4_t basecolor )
{
	int i, count, current;
	int yoffset;
	vec4_t color;
	float frac;

	if( !cg_showAwards->integer )
		return;

	if( !cg.award_head )
		return;

	for( count = 0; count < MAX_AWARD_LINES; count++ )
	{
		current = ( (cg.award_head - 1) - count );
		if( current < 0 )
			break;

		if( cg.award_times[current % MAX_AWARD_LINES] + MAX_AWARD_DISPLAYTIME < cg.time )
			break;

		if( !cg.award_lines[current % MAX_AWARD_LINES][0] )
			break;
	}

	if( !count )
		return;

	y = CG_VerticalAlignForHeight( y, align, trap_SCR_strHeight( font ) * MAX_AWARD_LINES );

	for( i = count; i > 0; i-- )
	{
		unsigned int fadeTime;

		current = ( cg.award_head - i ) % MAX_AWARD_LINES;

		yoffset = trap_SCR_strHeight( font ) * ( MAX_AWARD_LINES - i );

		Vector4Copy( basecolor, color );

		fadeTime = MAX_AWARD_DISPLAYTIME * 0.66;
		if( cg.time - cg.award_times[ current ] < fadeTime )
		{
			frac = 1.0f;
		}
		else
		{
			frac = 1.0f - ( (float)( ( cg.time - cg.award_times[ current ] ) - fadeTime ) / (float)( MAX_AWARD_DISPLAYTIME - fadeTime ) );
			clamp( frac, 0.0f, 1.0f );
		}

		color[3] *= frac;

		trap_SCR_DrawStringWidth( x, y + yoffset, align % 3, cg.award_lines[ current ], 0, font, color );
	}
}

//=============================================================================

static qboolean CG_IsWeaponSelected( int weapon )
{
	if( cg.view.playerPrediction && cg.predictedWeaponSwitch && cg.predictedWeaponSwitch != cg.predictedPlayerState.stats[STAT_PENDING_WEAPON] )
		return ( weapon == cg.predictedWeaponSwitch );

	return ( weapon == cg.predictedPlayerState.stats[STAT_PENDING_WEAPON] );
}

/*
* CG_DrawWeaponIcons
*/
static void CG_DrawWeaponIcons( int x, int y, int offx, int offy, int iw, int ih, int align )
{
	int curx, cury, curw, curh;
	int i, j, n;
	float fj, fn;
	vec4_t color;
	qboolean selected_weapon;

	if( !cg_weaponlist || !cg_weaponlist->integer )
		return;

	if( iw > 0 )
		curw = iw;
	else
		curw = 32 * cgs.vidWidth/800; // 32 = default size for icons
	if( ih > 0 )
		curh = ih;
	else
		curh = 32 * cgs.vidHeight/600; // 32 = default size for icons

	n = 0;

	for( i = 0; i < WEAP_TOTAL-1; i++ )
	{
		if( ( cg.predictedPlayerState.inventory[WEAP_GUNBLADE+i] || cg.predictedPlayerState.inventory[AMMO_GUNBLADE+i] || cg.predictedPlayerState.inventory[AMMO_WEAK_GUNBLADE+i] ) )
			n++;
	}

	VectorCopy( colorWhite, color );

	for( i = j = 0; i < WEAP_TOTAL-1; i++ )
	{
		// if player doesnt have this weapon, skip it
		if( !( cg.predictedPlayerState.inventory[WEAP_GUNBLADE+i] || cg.predictedPlayerState.inventory[AMMO_GUNBLADE+i] || cg.predictedPlayerState.inventory[AMMO_WEAK_GUNBLADE+i] ) )
			continue;

		selected_weapon = CG_IsWeaponSelected( WEAP_GUNBLADE+i );
		if( !selected_weapon )
			color[3] = 1.0;
		else
			color[3] = 0.5;

		fj = (float)j;
		fn = (float)n;
		curx = CG_HorizontalAlignForWidth( x + (int)( offx * ( fj - fn / 2.0f ) ), align, curw );
		cury = CG_VerticalAlignForHeight( y + (int)( offy * ( fj - fn / 2.0f ) ), align, curh );

		if( cg.predictedPlayerState.inventory[WEAP_GUNBLADE+i] )
		{
			// wsw : pb : display a little box around selected weapon in weaponlist
			if( selected_weapon )
			{
				if( customWeaponSelectPic )
					trap_R_DrawStretchPic( curx, cury, curw, curh, 0, 0, 1, 1, color, trap_R_RegisterPic( customWeaponSelectPic ) );
				else
					trap_R_DrawStretchPic( curx, cury, curw, curh, 0, 0, 1, 1, color, CG_MediaShader( cgs.media.shaderSelect ) );
			}
			if( customWeaponPics[i] )
				trap_R_DrawStretchPic( curx, cury, curw, curh, 0, 0, 1, 1, color, trap_R_RegisterPic( customWeaponPics[i] ) );
			else
				trap_R_DrawStretchPic( curx, cury, curw, curh, 0, 0, 1, 1, color, CG_MediaShader( cgs.media.shaderWeaponIcon[i] ) );
		}
		else
			if( customNoGunWeaponPics[i] )
				trap_R_DrawStretchPic( curx, cury, curw, curh, 0, 0, 1, 1, color, trap_R_RegisterPic( customNoGunWeaponPics[i] ) );
			else
				trap_R_DrawStretchPic( curx, cury, curw, curh, 0, 0, 1, 1, color, CG_MediaShader( cgs.media.shaderNoGunWeaponIcon[i] ) );
		j++;
	}
}

/*
* CG_DrawWeaponAmmos
*/
static void CG_DrawWeaponAmmos( int x, int y, int offx, int offy, int fontsize, int ammotype, int align )
{
	int curx, cury, curwh;
	int i, j, n, fs;
	float fj, fn;
	vec4_t color;
	int startammo;

	if( !cg_weaponlist || !cg_weaponlist->integer )
		return;

	if( ammotype == 1 )
		startammo = AMMO_GUNBLADE;
	else
		startammo = AMMO_WEAK_GUNBLADE;

	if( fontsize > 0 )
		fs = fontsize;
	else
		fs = 12; // 12 = default size for font
	curwh = (int)( fs * cgs.vidHeight/600 );

	n = 0;

	for( i = 0; i < WEAP_TOTAL-1; i++ )
	{
		if( ( cg.predictedPlayerState.inventory[WEAP_GUNBLADE+i] || cg.predictedPlayerState.inventory[AMMO_GUNBLADE+i] || cg.predictedPlayerState.inventory[AMMO_WEAK_GUNBLADE+i] ) )
			n++;
	}

	VectorCopy( colorWhite, color );

	for( i = j = 0; i < WEAP_TOTAL-1; i++ )
	{
		// if player doesn't have this weapon, skip it
		if( !( cg.predictedPlayerState.inventory[WEAP_GUNBLADE+i] || cg.predictedPlayerState.inventory[AMMO_GUNBLADE+i] || cg.predictedPlayerState.inventory[AMMO_WEAK_GUNBLADE+i] ) )
			continue;

		if( CG_IsWeaponSelected( WEAP_GUNBLADE+i ) )
			color[3] = 1.0;
		else
			color[3] = 0.5;

		fj = (float)j;
		fn = (float)n;
		curx = x + (int)( offx * ( fj - fn / 2.0f ) );
		cury = y + (int)( offy * ( fj - fn / 2.0f ) );

		if( cg.predictedPlayerState.inventory[i+startammo] )
			CG_DrawHUDNumeric( curx, cury, align, color, curwh, curwh, cg.predictedPlayerState.inventory[i+startammo] );
		j++;
	}
}

//=============================================================================
//	STATUS BAR PROGRAMS
//=============================================================================


// we will always operate with floats so we don't have to code 2 different numeric paths
// it's not like using float or ints would make a difference in this simple-scripting case.

static float CG_OpFuncAdd( const float a, const float b )
{
	return a + b;
}

static float CG_OpFuncSubtract( const float a, const float b )
{
	return a - b;
}

static float CG_OpFuncMultiply( const float a, const float b )
{
	return a * b;
}

static float CG_OpFuncDivide( const float a, const float b )
{
	return a / b;
}

static float CG_OpFuncAND( const float a, const float b )
{
	return (int)a & (int)b;
}

static float CG_OpFuncOR( const float a, const float b )
{
	return (int)a | (int)b;
}

static float CG_OpFuncXOR( const float a, const float b )
{
	return (int)a ^ (int)b;
}

static float CG_OpFuncCompareEqual( const float a, const float b )
{
	return ( a == b );
}

static float CG_OpFuncCompareNotEqual( const float a, const float b )
{
	return ( a != b );
}

static float CG_OpFuncCompareGreater( const float a, const float b )
{
	return ( a > b );
}

static float CG_OpFuncCompareGreaterOrEqual( const float a, const float b )
{
	return ( a >= b );
}

static float CG_OpFuncCompareSmaller( const float a, const float b )
{
	return ( a < b );
}

static float CG_OpFuncCompareSmallerOrEqual( const float a, const float b )
{
	return ( a <= b );
}

static float CG_OpFuncCompareAnd( const float a, const float b )
{
	return ( a && b );
}

static float CG_OpFuncCompareOr( const float a, const float b )
{
	return ( a || b );
}

typedef struct cg_layoutoperators_s
{
	char *name;
	float ( *opFunc )( const float a, const float b );
} cg_layoutoperators_t;

static cg_layoutoperators_t cg_LayoutOperators[] =
{
	{
		"+",
		CG_OpFuncAdd
	},

	{
		"-",
		CG_OpFuncSubtract
	},

	{
		"*",
		CG_OpFuncMultiply
	},

	{
		"/",
		CG_OpFuncDivide
	},

	{
		"&",
		CG_OpFuncAND
	},

	{
		"|",
		CG_OpFuncOR
	},

	{
		"^",
		CG_OpFuncXOR
	},

	{
		"==",
		CG_OpFuncCompareEqual
	},

	{
		"!=",
		CG_OpFuncCompareNotEqual
	},

	{
		">",
		CG_OpFuncCompareGreater
	},

	{
		">=",
		CG_OpFuncCompareGreaterOrEqual
	},

	{
		"<",
		CG_OpFuncCompareSmaller
	},

	{
		"<=",
		CG_OpFuncCompareSmallerOrEqual
	},

	{
		"&&",
		CG_OpFuncCompareAnd
	},

	{
		"||",
		CG_OpFuncCompareOr
	},

	{
		NULL,
		NULL
	},
};

/*
* CG_OperatorFuncForArgument
*/
static void *CG_OperatorFuncForArgument( const char *token )
{
	cg_layoutoperators_t *op;

	while( *token == ' ' )
		token++;

	for( op = cg_LayoutOperators; op->name; op++ )
	{
		if( !Q_stricmp( token, op->name ) )
			return op->opFunc;
	}

	return NULL;
}

//=============================================================================

static char *CG_GetStringArg( struct cg_layoutnode_s **argumentsnode );
static float CG_GetNumericArg( struct cg_layoutnode_s **argumentsnode );

//=============================================================================

static int layout_cursor_scale = DEFAULTSCALE;
static int layout_cursor_x = 400;
static int layout_cursor_y = 300;
static int layout_cursor_width = 100;
static int layout_cursor_height = 100;
static int layout_cursor_align = ALIGN_LEFT_TOP;
static vec4_t layout_cursor_color = { 1, 1, 1, 1 };
static vec3_t layout_cursor_rotation = { 0, 0, 0 };
static struct mufont_s *layout_cursor_font;
static char layout_cursor_font_name[MAX_QPATH];

enum
{
	LNODE_NUMERIC,
	LNODE_STRING,
	LNODE_REFERENCE_NUMERIC,
	LNODE_COMMAND
};

//=============================================================================
// Commands' Functions
//=============================================================================
static int CG_LFuncDrawTimer( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	char time[64];
	int min, sec, milli;

	milli = (int)CG_GetNumericArg( &argumentnode );
	if( milli < 0 )
		return qtrue;

	// stat is in milliseconds/100.0f
	min = milli/600;
	milli -= min*600;
	sec = milli/10;
	milli -= sec*10;
	// we want MM:SS:m
	Q_snprintfz( time, sizeof( time ), "%02d:%02d.%1d", min, sec, milli );
	trap_SCR_DrawString( layout_cursor_x, layout_cursor_y, layout_cursor_align, time, layout_cursor_font, layout_cursor_color );
	return qtrue;
}

static int CG_LFuncDrawPicVar( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	int min, max, val, firstimg, lastimg, imgcount;
	static char filefmt[MAX_QPATH], filenm[MAX_QPATH], *ptr;
	int x, y, filenr;
	int cnt = 1;

	// get tje arguments
	val      = (int)( CG_GetNumericArg( &argumentnode ) );
	min      = (int)( CG_GetNumericArg( &argumentnode ) );
	max      = (int)( CG_GetNumericArg( &argumentnode ) );
	firstimg = (int)( CG_GetNumericArg( &argumentnode ) );
	lastimg  = (int)( CG_GetNumericArg( &argumentnode ) );

	if( min > max )
	{            // swap min and max and count downwards
		max ^= min;
		min ^= max;
		max ^= min;
		cnt = -cnt;
	}
	if( firstimg > lastimg )
	{                     // swap firstimg and lastimg and count the other way around
		firstimg ^= lastimg;
		lastimg  ^= firstimg;
		firstimg ^= lastimg;
		cnt = -cnt;
	}

	if( val < min )
		val = min;
	if( val > max )
		val = max;
	val -= min;
	max -= min;
	min = 0;

	imgcount = lastimg - firstimg + 1;

	if( ( max != 0 ) && ( imgcount != 0 ) )
	{                                // Check for division by 0
		filenr =  (int)( ( (double)val / ( (double)max / imgcount ) ) );
	}
	else
	{
		filenr = 0;
	}
	if( filenr >= imgcount )
		filenr = ( imgcount - 1 );
	if( filenr < 0 )
		filenr = 0;

	if( cnt < 0 )
	{
		filenr = ( imgcount - filenr ) - 1;
	}
	filenr += firstimg;

	filefmt[0] = '\0';
	Q_strncpyz( filefmt, CG_GetStringArg( &argumentnode ), sizeof( filenm ) );
	ptr = filefmt;
	while( ( ptr[0] ) && ( ptr[1] ) )
	{
		if( ( ptr[0] == '#' ) && ( ptr[1] == '#' ) )
		{
			ptr[0] = '%';
			ptr[1] = 'd';
			break; // Only replace first occurance?
		}
		ptr++;
	}
	if( ( ptr[0] != '%' ) && ( ptr[1] != 'd' ) )
	{
		CG_Printf( "WARNING 'CG_LFuncDrawPicVar' Invalid file string parameter, no '##' present!" );
		return qfalse;
	}
	Q_snprintfz( filenm, sizeof( filenm ), filefmt, filenr );
	x = CG_HorizontalAlignForWidth( layout_cursor_x, layout_cursor_align, layout_cursor_width );
	y = CG_VerticalAlignForHeight( layout_cursor_y, layout_cursor_align, layout_cursor_height );
	trap_R_DrawStretchPic( x, y, layout_cursor_width, layout_cursor_height, 0, 0, 1, 1, layout_cursor_color, trap_R_RegisterPic( filenm ) );
	return qtrue;
}

static int CG_LFuncDrawPicByIndex( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	int value = (int)CG_GetNumericArg( &argumentnode );
	int x, y;

	if( value >= 0 && value < MAX_IMAGES )
	{
		if( cgs.configStrings[CS_IMAGES + value][0] )
		{
			x = CG_HorizontalAlignForWidth( layout_cursor_x, layout_cursor_align, layout_cursor_width );
			y = CG_VerticalAlignForHeight( layout_cursor_y, layout_cursor_align, layout_cursor_height );
			trap_R_DrawStretchPic( x, y, layout_cursor_width, layout_cursor_height, 0, 0, 1, 1, layout_cursor_color, trap_R_RegisterPic( cgs.configStrings[CS_IMAGES+value] ) );
			return qtrue;
		}
	}

	return qfalse;
}

static int CG_LFuncDrawPicByItemIndex( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	int itemindex = (int)CG_GetNumericArg( &argumentnode );
	int x, y;
	gsitem_t	*item;

	item = GS_FindItemByTag( itemindex );
	if( !item )
		return qfalse;
	x = CG_HorizontalAlignForWidth( layout_cursor_x, layout_cursor_align, layout_cursor_width );
	y = CG_VerticalAlignForHeight( layout_cursor_y, layout_cursor_align, layout_cursor_height );
	trap_R_DrawStretchPic( x, y, layout_cursor_width, layout_cursor_height, 0, 0, 1, 1, layout_cursor_color, trap_R_RegisterPic( item->icon ) );
	return qtrue;
}

static int CG_LFuncDrawPicByName( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	int x, y;

	x = CG_HorizontalAlignForWidth( layout_cursor_x, layout_cursor_align, layout_cursor_width );
	y = CG_VerticalAlignForHeight( layout_cursor_y, layout_cursor_align, layout_cursor_height );
	trap_R_DrawStretchPic( x, y, layout_cursor_width, layout_cursor_height, 0, 0, 1, 1, layout_cursor_color, trap_R_RegisterPic( CG_GetStringArg( &argumentnode ) ) );
	return qtrue;
}

static int CG_LFuncDrawModelByIndex( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	struct model_s *model;
	int value = (int)CG_GetNumericArg( &argumentnode );

	if( value >= 0 && value < MAX_MODELS )
	{
		model = value > 1 ? CG_RegisterModel( cgs.configStrings[CS_MODELS+value] ) : NULL;
		CG_DrawHUDModel( layout_cursor_x, layout_cursor_y, layout_cursor_align, layout_cursor_width, layout_cursor_height, model, NULL, layout_cursor_rotation[YAW] );
		return qtrue;
	}

	return qfalse;
}

static int CG_LFuncDrawModelByName( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	struct model_s *model;
	struct shader_s *shader;
	char *shadername;

	model = CG_RegisterModel( CG_GetStringArg( &argumentnode ) );
	shadername = CG_GetStringArg( &argumentnode );
	shader = Q_stricmp( shadername, "NULL" ) ? trap_R_RegisterPic( shadername ) : NULL;
	CG_DrawHUDModel( layout_cursor_x, layout_cursor_y, layout_cursor_align, layout_cursor_width, layout_cursor_height, model, shader, layout_cursor_rotation[YAW] );
	return qtrue;
}

static int CG_LFuncDrawModelByItemIndex( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	int i;
	gsitem_t	*item;
	struct model_s *model;
	int itemindex = (int)CG_GetNumericArg( &argumentnode );

	item = GS_FindItemByTag( itemindex );
	if( !item )
		return qfalse;
	for( i = 0; i < MAX_ITEM_MODELS; i++ )
	{
		if( item->world_model[i] != NULL )
		{
			model = itemindex >= 1 ? CG_RegisterModel( item->world_model[i] ) : NULL;
			CG_DrawHUDModel( layout_cursor_x, layout_cursor_y, layout_cursor_align, layout_cursor_width, layout_cursor_height, model, NULL, layout_cursor_rotation[YAW] );
		}
	}
	return qtrue;
}

static int CG_LFuncScale( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	layout_cursor_scale = (int)CG_GetNumericArg( &argumentnode );
	return qtrue;
}

#define SCALE_X( n ) ( (layout_cursor_scale == NOSCALE) ? (n) : ((layout_cursor_scale == SCALEBYHEIGHT) ? (n)*cgs.vidHeight/600.0f : (n)*cgs.vidWidth/800.0f) )
#define SCALE_Y( n ) ( (layout_cursor_scale == NOSCALE) ? (n) : ((layout_cursor_scale == SCALEBYWIDTH) ? (n)*cgs.vidWidth/800.0f : (n)*cgs.vidHeight/600.0f) )

static int CG_LFuncCursor( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	float x, y;

	x = CG_GetNumericArg( &argumentnode );
	x = SCALE_X( x );
	y = CG_GetNumericArg( &argumentnode );
	y = SCALE_Y( y );

	layout_cursor_x = Q_rint( x );
	layout_cursor_y = Q_rint( y );
	return qtrue;
}

static int CG_LFuncMoveCursor( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	float x, y;

	x = CG_GetNumericArg( &argumentnode );
	x = SCALE_X( x );
	y = CG_GetNumericArg( &argumentnode );
	y = SCALE_Y( y );

	layout_cursor_x += Q_rint( x );
	layout_cursor_y += Q_rint( y );
	return qtrue;
}

static int CG_LFuncSize( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	float x, y;

	x = CG_GetNumericArg( &argumentnode );
	x = SCALE_X( x );
	y = CG_GetNumericArg( &argumentnode );
	y = SCALE_Y( y );

	layout_cursor_width = Q_rint( x );
	layout_cursor_height = Q_rint( y );
	return qtrue;
}

static int CG_LFuncColor( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	int i;
	for( i = 0; i < 4; i++ )
	{
		layout_cursor_color[i] = CG_GetNumericArg( &argumentnode );
		clamp( layout_cursor_color[i], 0, 1 );
	}
	return qtrue;
}

static int CG_LFuncColorToTeamColor( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	CG_TeamColor( CG_GetNumericArg( &argumentnode ), layout_cursor_color );
	return qtrue;
}

static int CG_LFuncColorAlpha( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	layout_cursor_color[3] = CG_GetNumericArg( &argumentnode );
	return qtrue;
}

static int CG_LFuncRotationSpeed( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	int i;
	for( i = 0; i < 3; i++ )
	{
		layout_cursor_rotation[i] = CG_GetNumericArg( &argumentnode );
		clamp( layout_cursor_rotation[i], 0, 999 );
	}
	return qtrue;
}

static int CG_LFuncAlign( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	int v, h;

	h = (int)CG_GetNumericArg( &argumentnode );
	v = (int)CG_GetNumericArg( &argumentnode );
	if( h < 1 ) h = 1;
	if( v < 1 ) v = 1;
	layout_cursor_align = ( h-1 )+( 3*( v-1 ) );
	return qtrue;
}

static int CG_LFuncFont( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	struct mufont_s *font = NULL;
	char *fontname = CG_GetStringArg( &argumentnode );

	if( !Q_stricmp( fontname, "con_fontsystemsmall" ) )
	{
		font = cgs.fontSystemSmall;
		Q_strncpyz( layout_cursor_font_name, cgs.fontNameSystemSmall, sizeof( layout_cursor_font_name ) );
	}
	else if( !Q_stricmp( fontname, "con_fontsystemmedium" ) )
	{
		font = cgs.fontSystemMedium;
		Q_strncpyz( layout_cursor_font_name, cgs.fontNameSystemMedium, sizeof( layout_cursor_font_name ) );
	}
	else if( !Q_stricmp( fontname, "con_fontsystembig" ) )
	{
		font = cgs.fontSystemBig;
		Q_strncpyz( layout_cursor_font_name, cgs.fontNameSystemBig, sizeof( layout_cursor_font_name ) );
	}
	else
	{
		font = trap_SCR_RegisterFont( fontname );
		Q_strncpyz( layout_cursor_font_name, fontname, sizeof( layout_cursor_font_name ) );
	}

	if( font )
	{
		layout_cursor_font = font;
		return qtrue;
	}

	return qfalse;
}

static int CG_LFuncDrawObituaries( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	int internal_align = (int)CG_GetNumericArg( &argumentnode );
	int icon_size = (int)CG_GetNumericArg( &argumentnode );

	CG_DrawObituaries( layout_cursor_x, layout_cursor_y, layout_cursor_align, layout_cursor_font, layout_cursor_color,
	                   layout_cursor_width, layout_cursor_height, internal_align, icon_size );
	return qtrue;
}

static int CG_LFuncDrawAwards( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	CG_DrawAwards( layout_cursor_x, layout_cursor_y, layout_cursor_align, layout_cursor_font, layout_cursor_color );
	return qtrue;
}

static int CG_LFuncDrawClock( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	CG_DrawClock( layout_cursor_x, layout_cursor_y, layout_cursor_align, layout_cursor_font, layout_cursor_color );
	return qtrue;
}

static int CG_LFuncDrawHelpMessage( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	// hide this one when scoreboard is up
	if( !( cg.predictedPlayerState.stats[STAT_LAYOUTS] & STAT_LAYOUT_SCOREBOARD ) )
	{
		if( !cgs.demoPlaying )
		{
			int i;
			int y = layout_cursor_y;
			const char *helpmessage = "";

			for( i = 0; i < 3; i++ )
			{
				size_t len;

				switch( i )
				{
				case 0:
					helpmessage = ( cg_showhelp->integer && cg.matchmessage ? cg.matchmessage : "" );
					break;
				case 1:
					if( !cg.motd )
						return qtrue;
					helpmessage = "\nMessage of the Day:";
					break;
				case 2:
					helpmessage = cg.motd;
					break;
				default:
					return qtrue;
				}

				if( helpmessage[0] )
				{
					do
					{
						len = trap_SCR_DrawStringWidth( layout_cursor_x, y, layout_cursor_align, helpmessage, layout_cursor_width, layout_cursor_font, layout_cursor_color );
						if( !len )
						{
							if( *helpmessage == '\r' || *helpmessage == '\n' )
								len = 1;
							else
								break;
						}
						if( helpmessage[len-1] == '\n' )
						{
							y += trap_SCR_strHeight( layout_cursor_font );
						}
						helpmessage += len;
					}
					while( helpmessage[0] );

					y += trap_SCR_strHeight( layout_cursor_font );
				}
			}
		}
	}
	return qtrue;
}

static int CG_LFuncDrawTeamMates( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	CG_DrawTeamMates();
	return qtrue;
}

static int CG_LFuncDrawPointed( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	CG_DrawPlayerNames( layout_cursor_font, layout_cursor_color );
	return qtrue;
}

static int CG_LFuncDrawString( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	char *string = CG_GetStringArg( &argumentnode );

	if( !string || !string[0] )
		return qfalse;
	trap_SCR_DrawString( layout_cursor_x, layout_cursor_y, layout_cursor_align, string, layout_cursor_font, layout_cursor_color );
	return qtrue;
}

static int CG_LFuncDrawItemNameFromIndex( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	gsitem_t	*item;
	int itemindex = CG_GetNumericArg( &argumentnode );

	item = GS_FindItemByTag( itemindex );
	if( !item || !item->name )
		return qfalse;
	trap_SCR_DrawString( layout_cursor_x, layout_cursor_y, layout_cursor_align, item->name, layout_cursor_font, layout_cursor_color );
	return qtrue;
}

static int CG_LFuncDrawConfigstring( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	int index = (int)CG_GetNumericArg( &argumentnode );

	if( index < 0 || index >= MAX_CONFIGSTRINGS )
	{
		CG_Printf( "WARNING 'CG_LFuncDrawConfigstring' Bad stat_string index" );
		return qfalse;
	}
	trap_SCR_DrawString( layout_cursor_x, layout_cursor_y, layout_cursor_align, cgs.configStrings[index], layout_cursor_font, layout_cursor_color );
	return qtrue;
}

static int CG_LFuncDrawPlayername( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	int index = (int)CG_GetNumericArg( &argumentnode ) - 1;

	if( cgs.demoTutorial )
		return qtrue;

	if( index >= 0 && ( index < gs.maxclients + 1 ) && cgs.clientInfo[index].name )
	{
		trap_SCR_DrawString( layout_cursor_x, layout_cursor_y, layout_cursor_align, cgs.clientInfo[index].name, layout_cursor_font, layout_cursor_color );
		return qtrue;
	}
	return qfalse;
}

static int CG_LFuncDrawNumeric( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	int value = (int)CG_GetNumericArg( &argumentnode );
	CG_DrawHUDNumeric( layout_cursor_x, layout_cursor_y, layout_cursor_align, layout_cursor_color, layout_cursor_width, layout_cursor_height, value );
	return qtrue;
}

static int CG_LFuncDrawStretchNum( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	static char num[16];
	int len;
	int value = (int)CG_GetNumericArg( &argumentnode );

	Q_snprintfz( num, sizeof( num ), "%i", value );
	len = strlen( num );
	if( len * layout_cursor_height <= layout_cursor_width )
	{
		CG_DrawHUDNumeric( layout_cursor_x, layout_cursor_y, layout_cursor_align, layout_cursor_color, layout_cursor_height, layout_cursor_height, value );
	}
	else
	{    //stretch numbers
		CG_DrawHUDNumeric( layout_cursor_x, layout_cursor_y, layout_cursor_align, layout_cursor_color, layout_cursor_width / len, layout_cursor_height, value );
	}
	return qtrue;
}

static int CG_LFuncDrawNumeric2( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	int value = (int)CG_GetNumericArg( &argumentnode );

	trap_SCR_DrawString( layout_cursor_x, layout_cursor_y, layout_cursor_align, va( "%i", value ), layout_cursor_font, layout_cursor_color );
	return qtrue;
}

static int CG_LFuncDrawBar( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	int value = (int)CG_GetNumericArg( &argumentnode );
	int maxvalue = (int)CG_GetNumericArg( &argumentnode );
	CG_DrawHUDRect( layout_cursor_x, layout_cursor_y, layout_cursor_align,
	                layout_cursor_width, layout_cursor_height, value, maxvalue,
	                layout_cursor_color, NULL );
	return qtrue;
}

static int CG_LFuncDrawPicBar( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	int value = (int)CG_GetNumericArg( &argumentnode );
	int maxvalue = (int)CG_GetNumericArg( &argumentnode );

	CG_DrawHUDRect( layout_cursor_x, layout_cursor_y, layout_cursor_align,
	               layout_cursor_width, layout_cursor_height, value, maxvalue,
	               layout_cursor_color, trap_R_RegisterPic( CG_GetStringArg( &argumentnode ) ) );
	return qtrue;
}

static int CG_LFuncCustomWeaponIcons( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	int weapon = (int)CG_GetNumericArg( &argumentnode );
	int hasgun = (int)CG_GetNumericArg( &argumentnode );

	if( weapon <= WEAP_NONE || weapon >= WEAP_TOTAL )
		return qfalse;

	if( hasgun )
		customWeaponPics[weapon-1] = CG_GetStringArg( &argumentnode );
	else
		customNoGunWeaponPics[weapon-1] = CG_GetStringArg( &argumentnode );

	return qtrue;
}

static int CG_LFuncCustomWeaponSelect( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	customWeaponSelectPic = CG_GetStringArg( &argumentnode );
	return qtrue;
}

static int CG_LFuncDrawWeaponIcons( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	int offx, offy, w, h;

	offx = (int)( CG_GetNumericArg( &argumentnode ) * cgs.vidWidth/800 );
	offy = (int)( CG_GetNumericArg( &argumentnode ) * cgs.vidHeight/600 );
	w = (int)( CG_GetNumericArg( &argumentnode ) * cgs.vidWidth/800 );
	h = (int)( CG_GetNumericArg( &argumentnode ) * cgs.vidHeight/600 );

	CG_DrawWeaponIcons( layout_cursor_x, layout_cursor_y, offx, offy, w, h, layout_cursor_align );

	return qtrue;
}

static int CG_LFuncDrawCaptureAreas( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	// FIXME: DELETE ME
	return qtrue;
}

static int CG_LFuncDrawMiniMap( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	int draw_playernames, draw_itemnames;

	draw_playernames = (int)( CG_GetNumericArg( &argumentnode ) );
	draw_itemnames = (int)( CG_GetNumericArg( &argumentnode ) );

	CG_DrawMiniMap( layout_cursor_x, layout_cursor_y, layout_cursor_width, layout_cursor_height, draw_playernames, draw_itemnames, layout_cursor_align, layout_cursor_color );

	return qtrue;
}

static int CG_LFuncDrawLocationName( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	int loc_tag = CG_GetNumericArg( &argumentnode );
	char string[MAX_CONFIGSTRING_CHARS];

	if( loc_tag < 0 || loc_tag >= MAX_LOCATIONS )
		return qfalse;

	trap_GetConfigString( CS_LOCATIONS + loc_tag, string, sizeof( string ) );

	trap_SCR_DrawString( layout_cursor_x, layout_cursor_y, layout_cursor_align, string, layout_cursor_font, layout_cursor_color );
	return qtrue;
}

static int CG_LFuncDrawWeaponWeakAmmo( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	int offx, offy, fontsize;

	offx = (int)( CG_GetNumericArg( &argumentnode ) * cgs.vidWidth/800 );
	offy = (int)( CG_GetNumericArg( &argumentnode ) * cgs.vidHeight/600 );
	fontsize = (int)CG_GetNumericArg( &argumentnode );

	CG_DrawWeaponAmmos( layout_cursor_x, layout_cursor_y, offx, offy, fontsize, 2, layout_cursor_align );

	return qtrue;
}

static int CG_LFuncDrawWeaponStrongAmmo( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	int offx, offy, fontsize;

	offx = (int)( CG_GetNumericArg( &argumentnode ) * cgs.vidWidth/800 );
	offy = (int)( CG_GetNumericArg( &argumentnode ) * cgs.vidHeight/600 );
	fontsize = (int)CG_GetNumericArg( &argumentnode );

	CG_DrawWeaponAmmos( layout_cursor_x, layout_cursor_y, offx, offy, fontsize, 1, layout_cursor_align );

	return qtrue;
}

static int CG_LFuncDrawTeamInfo( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	CG_DrawTeamInfo( layout_cursor_x, layout_cursor_y, layout_cursor_align, layout_cursor_font, layout_cursor_color );
	return qtrue;
}

static int CG_LFuncDrawCrossHair( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	CG_DrawCrosshair( layout_cursor_x, layout_cursor_y, layout_cursor_align );
	return qtrue;
}

static int CG_LFuncDrawKeyState( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	char *key = CG_GetStringArg( &argumentnode );

	CG_DrawKeyState( layout_cursor_x, layout_cursor_y, layout_cursor_width, layout_cursor_height, layout_cursor_align, key );
	return qtrue;
}

static int CG_LFuncDrawNet( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	CG_DrawNet( layout_cursor_x, layout_cursor_y, layout_cursor_width, layout_cursor_height, layout_cursor_align, layout_cursor_color );
	return qtrue;
}

static int CG_LFuncDrawChat( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	int padding_x, padding_y;
	struct shader_s *shader;

	padding_x = (int)( CG_GetNumericArg( &argumentnode ) )*cgs.vidWidth/800;
	padding_y = (int)( CG_GetNumericArg( &argumentnode ) )*cgs.vidHeight/600;
	shader = trap_R_RegisterPic( CG_GetStringArg( &argumentnode ) );

	CG_DrawChat( &cg.chat, layout_cursor_x, layout_cursor_y, layout_cursor_font_name, layout_cursor_font, 
		layout_cursor_width, layout_cursor_height, padding_x, padding_y, layout_cursor_color, shader );
	return qtrue;
}


static int CG_LFuncIf( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	return (int)CG_GetNumericArg( &argumentnode );
}

//racesow - based on drawTimer
static int CG_LFuncDrawCheckpoint( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments )
{
	char time[64];
	char *sign;
	int min, sec, milli;

	milli = (int)CG_GetNumericArg( &argumentnode );

	if ( milli < 0 )
	{
		milli *= -1;
		sign = "-";
	}
	else
		sign = "+";

	min = milli/60000;
	milli -= min*60000;
	sec = milli/1000;
	milli -= sec*1000;

	if ( min >= 100 )
		Q_snprintfz( time, sizeof( time ), "%s%003d:%02d.%003d", sign, min, sec, milli );
	else if ( min >= 10 )
		Q_snprintfz( time, sizeof( time ), "%s%02d:%02d.%003d", sign, min, sec, milli );
	else if ( min >= 1 )
		Q_snprintfz( time, sizeof( time ), "%s%1d:%02d.%003d", sign, min, sec, milli );
	else if ( sec >= 10 )
		Q_snprintfz( time, sizeof( time ), "%s%02d.%003d", sign, sec, milli );
	else
		Q_snprintfz( time, sizeof( time ), "%s%1d.%003d", sign, sec, milli );

	trap_SCR_DrawString( layout_cursor_x, layout_cursor_y, layout_cursor_align, time, layout_cursor_font, layout_cursor_color );
	return qtrue;
}
//!racesow

typedef struct cg_layoutcommand_s
{
	char *name;
	int ( *func )( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments );
	int numparms;
	char *help;
	qboolean precache;
} cg_layoutcommand_t;

static cg_layoutcommand_t cg_LayoutCommands[] =
{
	{
		"setScale",
		CG_LFuncScale,
		1,
		"Sets the cursor scaling method.",
		qfalse
	},

	{
		"setCursor",
		CG_LFuncCursor,
		2,
		"Sets the cursor position to x and y coordinates.",
		qfalse
	},

	{
		"MoveCursor",
		CG_LFuncMoveCursor,
		2,
		"Moves the cursor position by dx and dy.",
		qfalse
	},

	{
		"setAlign",
		CG_LFuncAlign,
		2,
		"Changes align setting. Parameters: horizontal alignment, vertical alignment",
		qfalse
	},

	{
		"setSize",
		CG_LFuncSize,
		2,
		"Sets width and height. Used for pictures and models.",
		qfalse
	},

	{
		"setFont",
		CG_LFuncFont,
		1,
		"Sets font by font name. Accepts 'con_fontsystemsmall', 'con_fontsystemmedium' and 'con_fontsystembig' as shortcut to default game fonts.",
		qfalse
	},

	{
		"setColor",
		CG_LFuncColor,
		4,
		"Sets color setting in RGBA mode. Used for text and pictures",
		qfalse
	},

	{
		"setColorToTeamColor",
		CG_LFuncColorToTeamColor,
		1,
		"Sets cursor color to the color of the team provided in the argument",
		qfalse
	},

	{
		"setColorAlpha",
		CG_LFuncColorAlpha,
		1,
		"Changes the alpha value of the current color",
		qfalse
	},

	{
		"setRotationSpeed",
		CG_LFuncRotationSpeed,
		3,
		"Sets rotation speeds as vector. Used for models",
		qfalse
	},

	{
		"setCustomWeaponIcons",
		CG_LFuncCustomWeaponIcons,
		3,
		"Sets a custom shader for weapon icons",
		qfalse
	},

	{
		"setCustomWeaponSelect",
		CG_LFuncCustomWeaponSelect,
		1,
		"Sets a custom shader for weapon icons",
		qfalse
	},

	{
		"drawObituaries",
		CG_LFuncDrawObituaries,
		2,
		"Draws graphical death messages",
		qfalse
	},

	{
		"drawAwards",
		CG_LFuncDrawAwards,
		0,
		"Draws award messages",
		qfalse
	},

	{
		"drawClock",
		CG_LFuncDrawClock,
		0,
		"Draws clock",
		qfalse
	},

	{
		"drawHelpString",
		CG_LFuncDrawHelpMessage,
		0,
		"Draws the help message",
		qfalse
	},

	{
		"drawPlayername",
		CG_LFuncDrawPlayername,
		1,
		"Draws the name of the player with id provided by the argument",
		qfalse
	},

	{
		"drawPointing",
		CG_LFuncDrawPointed,
		0,
		"Draws the name of the player in the crosshair",
		qfalse
	},

	{
		"drawTeamMates",
		CG_LFuncDrawTeamMates,
		0,
		"Draws indicators where team mates are",
		qfalse
	},
	
	{
		"drawStatString",
		CG_LFuncDrawConfigstring,
		1,
		"Draws configstring of argument id",
		qfalse
	},

	//racesow
	{
		"drawCheckPoint",
		CG_LFuncDrawCheckpoint,
		1,
		"Draws last checkpoint time of argument id",
		qfalse
	},
	//!racesow

	{
		"drawItemName",
		CG_LFuncDrawItemNameFromIndex,
		1,
		"Draws the name of the item with given item index",
		qfalse
	},

	{
		"drawString",
		CG_LFuncDrawString,
		1,
		"Draws the string in the argument",
		qfalse
	},

	{
		"drawStringNum",
		CG_LFuncDrawNumeric2,
		1,
		"Draws numbers as text",
		qfalse
	},

	{
		"drawNum",
		CG_LFuncDrawNumeric,
		1,
		"Draws numbers of given character size",
		qfalse
	},

	{
		"drawStretchNum",
		CG_LFuncDrawStretchNum,
		1,
		"Draws numbers stretch inside a given size",
		qfalse
	},

	{
		"drawBar",
		CG_LFuncDrawBar,
		2,
		"Draws a bar of size setting, the bar is filled in proportion to the arguments",
		qfalse
	},

	{
		"drawPicBar",
		CG_LFuncDrawPicBar,
		3,
		"Draws a picture of size setting, is filled in proportion to the 2 arguments (value, maxvalue). 3rd argument is the picture path",
		qfalse
	},

	{
		"drawCrosshair",
		CG_LFuncDrawCrossHair,
		0,
		"Draws the game crosshair",
		qfalse
	},

	{
		"drawKeyState",
		CG_LFuncDrawKeyState,
		1,
		"Draws icons showing if the argument key is pressed. Possible arg: forward, backward, left, right, fire, jump, crouch, special",
		qfalse
	},

	{
		"drawNetIcon",
		CG_LFuncDrawNet,
		0,
		"Draws the disconnection icon",
		qfalse
	},

	{
		"drawChat",
		CG_LFuncDrawChat,
		3,
		"Draws the game chat messages",
		qfalse
	},

	{
		"drawPicByIndex",
		CG_LFuncDrawPicByIndex,
		1,
		"Draws a pic with argument as imageIndex",
		qtrue
	},

	{
		"drawPicByItemIndex",
		CG_LFuncDrawPicByItemIndex,
		1,
		"Draws a item icon pic with argument as itemIndex",
		qfalse
	},

	{
		"drawPicByName",
		CG_LFuncDrawPicByName,
		1,
		"Draws a pic with argument being the file path",
		qtrue
	},

	{
		"drawModelByIndex",
		CG_LFuncDrawModelByIndex,
		1,
		"Draws a model with argument being the modelIndex",
		qtrue
	},

	{
		"drawModelByName",
		CG_LFuncDrawModelByName,
		2,
		"Draws a model with argument being the path to the model file",
		qtrue
	},

	{
		"drawModelByItemIndex",
		CG_LFuncDrawModelByItemIndex,
		1,
		"Draws a item model with argument being the item index",
		qfalse
	},

	{
		"drawWeaponIcons",
		CG_LFuncDrawWeaponIcons,
		4,
		"Draws the icons of weapon/ammo owned by the player, arguments are offset x, offset y, size x, size y",
		qfalse
	},

	{
		"drawCaptureAreas",
		CG_LFuncDrawCaptureAreas,
		3,
		"Draws the capture areas for iTDM",
		qfalse
	},

	{
		"drawMiniMap",
		CG_LFuncDrawMiniMap,
		2,
		"Draws a minimap (radar). Arguments are : draw_playernames, draw_itemnames",
		qfalse
	},

	{
		"drawWeaponWeakAmmo",
		CG_LFuncDrawWeaponWeakAmmo,
		3,
		"Draws the amount of weak ammo owned by the player, arguments are offset x, offset y, fontsize",
		qfalse
	},
	{
		"drawWeaponStrongAmmo",
		CG_LFuncDrawWeaponStrongAmmo,
		3,
		"Draws the amount of strong ammo owned by the player,  arguments are offset x, offset y, fontsize",
		qfalse
	},

	{
		"drawTeamInfo",
		CG_LFuncDrawTeamInfo,
		0,
		"Draws the Team Info (locations) box",
		qfalse
	},

	{
		"if",
		CG_LFuncIf,
		1,
		"Conditional expression. Argument accepts operations >, <, ==, >=, <=, etc",
		qfalse
	},

	{
		"endif",
		NULL,
		0,
		"End of conditional expression block",
		qfalse
	},

	{
		"drawTimer",
		CG_LFuncDrawTimer,
		1,
		"Draws a timer clock for the race gametype",
		qfalse
	},
	{
		"drawPicVar",
		CG_LFuncDrawPicVar,
		6,
		"Draws a picture from a sequence, depending on the value of a given parameter. Parameters: minval, maxval, value, firstimg, lastimg, imagename (replacing ## by the picture number, no leading zeros), starting at 0)",
		qfalse
	},

	{
		"drawLocationName",
		CG_LFuncDrawLocationName,
		1,
		"Draws the location name with argument being location tag/index",
		qfalse
	},

	{
		NULL,
		NULL,
		0,
		NULL,
		qfalse
	}
};

void Cmd_CG_PrintHudHelp_f( void )
{
	cg_layoutcommand_t *cmd;
	cg_layoutoperators_t *op;
	int i;
	gsitem_t	*item;
	char *name, *p;

	CG_Printf( "- %sHUD scripts commands\n-------------------------------------%s\n", S_COLOR_YELLOW, S_COLOR_WHITE );
	for( cmd = cg_LayoutCommands; cmd->name; cmd++ )
	{
		CG_Printf( "- cmd: %s%s%s expected arguments: %s%i%s\n- desc: %s%s%s\n",
		           S_COLOR_YELLOW, cmd->name, S_COLOR_WHITE,
		           S_COLOR_YELLOW, cmd->numparms, S_COLOR_WHITE,
		           S_COLOR_BLUE, cmd->help, S_COLOR_WHITE );
	}
	CG_Printf( "\n" );

	CG_Printf( "- %sHUD scripts operators\n------------------------------------%s\n", S_COLOR_YELLOW, S_COLOR_WHITE );
	CG_Printf( "- " );
	for( op = cg_LayoutOperators; op->name; op++ )
	{
		CG_Printf( "%s%s%s, ", S_COLOR_YELLOW, op->name, S_COLOR_WHITE );
	}
	CG_Printf( "\n\n" );

	CG_Printf( "- %sHUD scripts CONSTANT names\n-------------------------------%s\n", S_COLOR_YELLOW, S_COLOR_WHITE );
	for( item = &itemdefs[1]; item->classname; item++ )
	{
		name = Q_strupr( CG_CopyString( item->name ) );
		p = name;
		while( ( p = strchr( p, ' ' ) ) )
		{
			*p = '_';
		}

		CG_Printf( "%sITEM_%s%s, ", S_COLOR_YELLOW, name, S_COLOR_WHITE );
	}
	for( i = 0; cg_numeric_constants[i].name != NULL; i++ )
	{
		CG_Printf( "%s%s%s, ", S_COLOR_YELLOW, cg_numeric_constants[i].name, S_COLOR_WHITE );
	}
	CG_Printf( "\n\n" );

	CG_Printf( "- %sHUD scripts REFERENCE names\n------------------------------%s\n", S_COLOR_YELLOW, S_COLOR_WHITE );
	for( i = 0; cg_numeric_references[i].name != NULL; i++ )
	{
		CG_Printf( "%s%s%s, ", S_COLOR_YELLOW, cg_numeric_references[i].name, S_COLOR_WHITE );
	}
	CG_Printf( "\n" );
}


//=============================================================================

typedef struct cg_layoutnode_s
{
	int ( *func )( struct cg_layoutnode_s *commandnode, struct cg_layoutnode_s *argumentnode, int numArguments );
	int type;
	char *string;
	int integer;
	float value;
	float ( *opFunc )( const float a, float b );
	struct cg_layoutnode_s *parent;
	struct cg_layoutnode_s *next;
	struct cg_layoutnode_s *ifthread;
	qboolean precache;
} cg_layoutnode_t;

/*
* CG_GetStringArg
*/
static char *CG_GetStringArg( struct cg_layoutnode_s **argumentsnode )
{
	struct cg_layoutnode_s *anode = *argumentsnode;

	if( !anode || anode->type == LNODE_COMMAND )
		CG_Error( "'CG_LayoutGetIntegerArg': bad arg count" );

	// we can return anything as string
	*argumentsnode = anode->next;
	return anode->string;
}

/*
* CG_GetNumericArg
* can use recursion for mathematical operations
*/
static float CG_GetNumericArg( struct cg_layoutnode_s **argumentsnode )
{
	struct cg_layoutnode_s *anode = *argumentsnode;
	float value;

	if( !anode || anode->type == LNODE_COMMAND )
		CG_Error( "'CG_LayoutGetIntegerArg': bad arg count" );

	if( anode->type != LNODE_NUMERIC && anode->type != LNODE_REFERENCE_NUMERIC )
		CG_Printf( "WARNING: 'CG_LayoutGetIntegerArg': arg %s is not numeric", anode->string );

	*argumentsnode = anode->next;
	if( anode->type == LNODE_REFERENCE_NUMERIC )
	{
		value = cg_numeric_references[anode->integer].func( cg_numeric_references[anode->integer].parameter );
	}
	else
	{
		value = anode->value;
	}

	// recurse if there are operators
	if( anode->opFunc != NULL )
	{
		value = anode->opFunc( value, CG_GetNumericArg( argumentsnode ) );
	}

	return value;
}

/*
* CG_LayoutParseCommandNode
* alloc a new node for a command
*/
static cg_layoutnode_t *CG_LayoutParseCommandNode( const char *token )
{
	int i = 0;
	cg_layoutcommand_t *command = NULL;
	cg_layoutnode_t *node;

	for( i = 0; cg_LayoutCommands[i].name; i++ )
	{
		if( !Q_stricmp( token, cg_LayoutCommands[i].name ) )
		{
			command = &cg_LayoutCommands[i];
			break;
		}
	}

	if( command == NULL )
		return NULL;

	node = CG_Malloc( sizeof( cg_layoutnode_t ) );
	node->type = LNODE_COMMAND;
	node->integer = command->numparms;
	node->value = 0.0f;
	node->string = CG_CopyString( command->name );
	node->func = command->func;
	node->ifthread = NULL;
	node->precache = command->precache;

	return node;
}

/*
* CG_LayoutParseArgumentNode
* alloc a new node for an argument
*/
static cg_layoutnode_t *CG_LayoutParseArgumentNode( char *token )
{
	cg_layoutnode_t *node;
	int type = LNODE_NUMERIC;
	char *valuetok, *p;
	static char tmpstring[8];
	gsitem_t	*item;

	// find what's it
	if( !token )
		return NULL;

	valuetok = token;

	if( token[0] == '%' )
	{                  // it's a stat parm
		int i;
		type = LNODE_REFERENCE_NUMERIC;
		valuetok++; // skip %

		// replace stat names by values
		for( i = 0; cg_numeric_references[i].name != NULL; i++ )
		{
			if( !Q_stricmp( valuetok, cg_numeric_references[i].name ) )
			{
				Q_snprintfz( tmpstring, sizeof( tmpstring ), "%i", i );
				valuetok = tmpstring;
				break;
			}
		}
		if( cg_numeric_references[i].name == NULL )
		{
			CG_Printf( "Warning: HUD: %s is not valid numeric reference\n", valuetok );
			valuetok--;
			valuetok = "0";
		}
	}
	else if( token[0] == '#' )
	{                         // it's a integer constant
		int i;
		type = LNODE_NUMERIC;
		valuetok++; // skip #

		// replace constants names by values
		if( !strncmp( valuetok, "ITEM_", strlen( "ITEM_" ) ) )
		{
			p = valuetok;
			while( ( p = strchr( p, '_' ) ) )
			{
				*p = ' ';
			}
			if( ( item = GS_FindItemByName( valuetok + strlen( "ITEM_" ) ) ) )
			{
				Q_snprintfz( tmpstring, sizeof( tmpstring ), "%i", item->tag );
				valuetok = tmpstring;
			}
			if( item == NULL )
			{
				CG_Printf( "Warning: HUD: %s is not valid numeric constant\n", valuetok );
				valuetok = "0";
			}
		}
		else
		{
			for( i = 0; cg_numeric_constants[i].name != NULL; i++ )
			{
				if( !Q_stricmp( valuetok, cg_numeric_constants[i].name ) )
				{
					Q_snprintfz( tmpstring, sizeof( tmpstring ), "%i", cg_numeric_constants[i].value );
					valuetok = tmpstring;
					break;
				}
			}
			if( cg_numeric_constants[i].name == NULL )
			{
				CG_Printf( "Warning: HUD: %s is not valid numeric constant\n", valuetok );
				valuetok = "0";
			}
		}

#if 0 // not used yet at least
	}
	else if( token[0] == '$' )
	{                         // it's a string constant
		int i;
		type = LNODE_STRING;
		valuetok++; // skip $

		// replace stat names by values
		for( i = 0; cg_string_constants[i].name != NULL; i++ )
		{
			if( !Q_stricmp( valuetok, cg_string_constants[i].name ) )
			{
				Q_snprintfz( tmpstring, sizeof( tmpstring ), "%s", cg_string_constants[i].value );
				valuetok = tmpstring;
				break;
			}
		}
#endif
	}
	else if( token[0] == '\\' )
	{
		valuetok = ++token;
		type = LNODE_STRING;
	}
	else if( token[0] < '0' && token[0] > '9' && token[0] != '.' )
	{
		type = LNODE_STRING;
	}

	// alloc
	node = CG_Malloc( sizeof( cg_layoutnode_t ) );
	node->type = type;
	node->integer = atoi( valuetok );
	node->value = atof( valuetok );
	node->string = CG_CopyString( token );
	node->func = NULL;
	node->ifthread = NULL;
	node->precache = qfalse;

	// return it
	return node;
}

/*
* CG_LayoutCathegorizeToken
*/
static int CG_LayoutCathegorizeToken( char *token )
{
	int i = 0;

	for( i = 0; cg_LayoutCommands[i].name; i++ )
	{
		if( !Q_stricmp( token, cg_LayoutCommands[i].name ) )
			return LNODE_COMMAND;
	}

	if( token[0] == '%' )
	{                  // it's a numerical reference
		return LNODE_REFERENCE_NUMERIC;
	}
	else if( token[0] == '#' )
	{                         // it's a numerical constant
		return LNODE_NUMERIC;
#if 0
	}
	else if( token[0] == '$' )
	{                         // it's a string constant
		return LNODE_STRING;
#endif
	}
	else if( token[0] < '0' && token[0] > '9' && token[0] != '.' )
	{
		return LNODE_STRING;
	}

	return LNODE_NUMERIC;
}

/*
* CG_RecurseFreeLayoutThread
* recursive for freeing "if" subtrees
*/
static void CG_RecurseFreeLayoutThread( cg_layoutnode_t *rootnode )
{
	cg_layoutnode_t *node;

	if( !rootnode )
		return;

	while( rootnode )
	{
		node = rootnode;
		rootnode = rootnode->parent;

		if( node->ifthread )
			CG_RecurseFreeLayoutThread( node->ifthread );

		if( node->string )
			CG_Free( node->string );

		CG_Free( node );
	}
}

/*
* CG_LayoutFixCommasInToken
* commas are accepted in the scripts. They actually do nothing, but are good for readability
*/
static qboolean CG_LayoutFixCommasInToken( char **ptr, char **backptr )
{
	char *token;
	char *back;
	int offset, count;
	qboolean stepback = qfalse;

	token = *ptr;
	back = *backptr;

	if( !token || !strlen( token ) ) return qfalse;

	// check that sizes match (quotes are removed from tokens)
	offset = count = strlen( token );
	back = *backptr;
	while( count-- )
	{
		if( *back == '"' )
		{
			count++;
			offset++;
		}
		back--;
	}

	back = *backptr - offset;
	while( offset )
	{
		if( *back == '"' )
		{
			offset--;
			back++;
			continue;
		}

		if( *token != *back )
			CG_Printf( "Token and Back mismatch %c - %c\n", *token, *back );

		if( *back == ',' )
		{
			*back = ' ';
			stepback = qtrue;
		}

		offset--;
		token++;
		back++;
	}

	return stepback;
}

/*
* CG_RecurseParseLayoutScript
* recursive for generating "if" subtrees
*/
static cg_layoutnode_t *CG_RecurseParseLayoutScript( char **ptr, int level )
{
	cg_layoutnode_t	*command = NULL;
	cg_layoutnode_t	*argumentnode = NULL;
	cg_layoutnode_t	*node = NULL;
	cg_layoutnode_t	*rootnode = NULL;
	int expecArgs = 0, numArgs = 0;
	int token_type;
	qboolean add;
	char *token, *s_tokenback;

	if( !ptr )
		return NULL;

	if( !*ptr || !*ptr[0] )
		return NULL;

	while( *ptr )
	{
		s_tokenback = *ptr;

		token = COM_Parse( ptr );
		while( *token == ' ' ) token++; // eat up whitespaces
		if( !Q_stricmp( ",", token ) ) continue; // was just a comma
		if( CG_LayoutFixCommasInToken( &token, ptr ) )
		{
			*ptr = s_tokenback; // step back
			continue;
		}

		if( !*token ) continue;
		if( !strlen( token ) ) continue;

		add = qfalse;
		token_type = CG_LayoutCathegorizeToken( token );

		// if it's an operator, we don't create a node, but add the operation to the last one
		if( CG_OperatorFuncForArgument( token ) != NULL )
		{
			if( !node )
			{
				CG_Printf( "WARNING 'CG_RecurseParseLayoutScript'(level %i): \"%s\" Operator hasn't any prior argument\n", level, token );
				continue;
			}
			if( node->type == LNODE_COMMAND || node->type == LNODE_STRING )
				CG_Printf( "WARNING 'CG_RecurseParseLayoutScript'(level %i): \"%s\" Operator was assigned to a command node\n", level, token );
			else
				expecArgs++; // we now expect one extra argument (not counting the operator one)

			node->opFunc = CG_OperatorFuncForArgument( token );
			continue; // skip and continue
		}

		if( expecArgs > numArgs )
		{
			// we are expecting an argument
			switch( token_type )
			{
			case LNODE_NUMERIC:
			case LNODE_STRING:
			case LNODE_REFERENCE_NUMERIC:
				break;
			case LNODE_COMMAND:
			{
				CG_Printf( "WARNING 'CG_RecurseParseLayoutScript'(level %i): \"%s\" is not a valid argument for \"%s\"\n", level, token, command ? command->string : "" );
				continue;
			}
				break;
			default:
			{
				CG_Printf( "WARNING 'CG_RecurseParseLayoutScript'(level %i) skip and continue: Unrecognized token \"%s\"\n", level, token );
				continue;
			}
				break;
			}
		}
		else
		{
			if( token_type != LNODE_COMMAND )
			{
				// we are expecting a command
				CG_Printf( "WARNING 'CG_RecurseParseLayoutScript'(level %i): unrecognized command \"%s\"\n", level, token );
				continue;
			}

			// special case: endif commands interrupt the thread and are not saved
			if( !Q_stricmp( token, "endif" ) )
			{
				//finish the last command properly
				if( command )
					command->integer = expecArgs;
				return rootnode;
			}

			// special case: last command was "if", we create a new sub-thread and ignore the new command
			if( command && !Q_stricmp( command->string, "if" ) )
			{
				*ptr = s_tokenback; // step back one token
				command->ifthread = CG_RecurseParseLayoutScript( ptr, level + 1 );
			}
		}

		// things look fine, proceed creating the node
		switch( token_type )
		{
		case LNODE_NUMERIC:
		case LNODE_STRING:
		case LNODE_REFERENCE_NUMERIC:
		{
			node = CG_LayoutParseArgumentNode( token );
			if( !node )
			{
				CG_Printf( "WARNING 'CG_RecurseParseLayoutScript'(level %i): \"%s\" is not a valid argument for \"%s\"\n", level, token, command ? command->string : "" );
				break;
			}
			numArgs++;
			add = qtrue;
		}
			break;
		case LNODE_COMMAND:
		{
			node = CG_LayoutParseCommandNode( token );
			if( !node )
			{
				CG_Printf( "WARNING 'CG_RecurseParseLayoutScript'(level %i): \"%s\" is not a valid command\n", level, token );
				break; // skip and continue
			}

			// expected arguments could have been extended by the operators
			if( command )
				command->integer = expecArgs;

			// move on into the new command
			command = node;
			argumentnode = NULL;
			numArgs = 0;
			expecArgs = command->integer;
			add = qtrue;
		}
			break;
		default:
			break;
		}

		if( add == qtrue )
		{
			if( command && command == rootnode ) {
				if( !argumentnode )
					argumentnode = node;
			}

			if( rootnode )
				rootnode->next = node;
			node->parent = rootnode;
			rootnode = node;

			// precache arguments by calling the function at load time
			if( command && expecArgs == numArgs && command->func && command->precache ) {
				Vector4Set( layout_cursor_color, 0, 0, 0, 0 );
				command->func( command, argumentnode, numArgs );
			}
		}
	}

	if( level > 0 )
		CG_Printf( "WARNING 'CG_RecurseParseLayoutScript'(level %i): If without endif\n", level );

	return rootnode;
}

#if 0
static void CG_RecursePrintLayoutThread( cg_layoutnode_t *rootnode, int level )
{
	int i;
	cg_layoutnode_t *node;

	node = rootnode;
	while( node->parent )
		node = node->parent;

	while( node )
	{
		for( i = 0; i < level; i++ )
			CG_Printf( "   " );
		CG_Printf( "%s\n", node->string );

		if( node->ifthread )
			CG_RecursePrintLayoutThread( node->ifthread, level+1 );

		node = node->next;
	}
}
#endif

/*
* CG_ParseLayoutScript
*/
static void CG_ParseLayoutScript( char *string, cg_layoutnode_t *rootnode )
{

	CG_RecurseFreeLayoutThread( cg.statusBar );
	cg.statusBar = CG_RecurseParseLayoutScript( &string, 0 );

#if 0
	CG_RecursePrintLayoutThread( cg_layoutRootNode, 0 );
#endif
}

//=============================================================================

//=============================================================================

/*
* CG_RecurseExecuteLayoutThread
* Execution works like this: First node (on backwards) is expected to be the command, followed by arguments nodes.
* we keep a pointer to the command and run the tree counting arguments until we reach the next command,
* then we call the command function sending the pointer to first argument and the pointer to the command.
* At return we advance one node (we stopped at last argument node) so it starts again from the next command (if any).
* 
* When finding an "if" command with a subtree, we execute the "if" command. In the case it
* returns any value, we recurse execute the subtree
*/
static void CG_RecurseExecuteLayoutThread( cg_layoutnode_t *rootnode )
{
	cg_layoutnode_t	*argumentnode = NULL;
	cg_layoutnode_t	*commandnode = NULL;
	int numArguments;

	if( !rootnode )
		return;

	// run until the real root
	commandnode = rootnode;
	while( commandnode->parent )
	{
		commandnode = commandnode->parent;
	}

	// now run backwards up to the next command node
	while( commandnode )
	{
		argumentnode = commandnode->next;

		// we could trust the parser, but I prefer counting the arguments here
		numArguments = 0;
		while( argumentnode )
		{
			if( argumentnode->type == LNODE_COMMAND )
				break;

			argumentnode = argumentnode->next;
			numArguments++;
		}

		// reset
		argumentnode = commandnode->next;

		// Execute the command node
		if( commandnode->integer != numArguments )
		{
			CG_Printf( "ERROR: Layout command %s: invalid argument count (expecting %i, found %i)\n", commandnode->string, commandnode->integer, numArguments );
			return;
		}
		if( commandnode->func )
		{
			//special case for if commands
			if( commandnode->func( commandnode, argumentnode, numArguments ) )
			{
				// execute the "if" thread when command returns a value
				if( commandnode->ifthread )
					CG_RecurseExecuteLayoutThread( commandnode->ifthread );
			}
		}

		//move up to next command node
		commandnode = argumentnode;
		if( commandnode == rootnode )
			return;

		while( commandnode && commandnode->type != LNODE_COMMAND )
		{
			commandnode = commandnode->next;
		}
	}
}

/*
* CG_ExecuteLayoutProgram
*/
void CG_ExecuteLayoutProgram( struct cg_layoutnode_s *rootnode )
{
	CG_RecurseExecuteLayoutThread( rootnode );
}

//=============================================================================

//=============================================================================



/*
* CG_LoadHUDFile
*/
// Loads the HUD-file recursively. Recursive includes now supported
// Also processes "preload" statements for graphics pre-loading
#define HUD_MAX_LVL 16 // maximum levels of recursive file loading
static char *CG_LoadHUDFile( char *path )
{
	char *rec_fn[HUD_MAX_LVL]; // Recursive filenames...
	char *rec_buf[HUD_MAX_LVL]; // Recursive file contents buffers
	char *rec_ptr[HUD_MAX_LVL]; // Recursive file position buffers
	char *token = NULL, *tmpbuf = NULL, *retbuf = NULL;
	int rec_lvl = 0, rec_plvl = -1;
	int retuse = 0, retlen = 0;
	int f, i, len;

	// Check if path is correct
	if( path == NULL )
		return NULL;
	memset( rec_ptr, 0, sizeof( rec_ptr ) );
	memset( rec_buf, 0, sizeof( rec_buf ) );
	memset( rec_ptr, 0, sizeof( rec_ptr ) );

	// Copy the path of the file to the first recursive level filename :)
	rec_fn[rec_lvl] = CG_Malloc( strlen( path ) + 1 );
	Q_strncpyz( rec_fn[rec_lvl], path, strlen( path ) + 1 );
	while( 1 )
	{
		if( rec_lvl > rec_plvl )
		{
			// We went a recursive level higher, our filename should have been filled in :)
			if( !rec_fn[rec_lvl] )
			{
				rec_lvl--;
			}
			else if( rec_fn[rec_lvl][0] == '\0' )
			{
				CG_Free( rec_fn[rec_lvl] );
				rec_fn[rec_lvl] = NULL;
				rec_lvl--;
			}
			else
			{
				// First check if this file hadn't been included already by one of the previous files
				// in the current file-stack to prevent problems :)
				for( i = 0; i < rec_lvl; i++ )
				{
					if( !Q_stricmp( rec_fn[rec_lvl], rec_fn[i] ) )
					{
						// Recursive file loading detected!!
						CG_Printf( "HUD: WARNING: Detected recursive file inclusion: %s\n", rec_fn[rec_lvl] );
						CG_Free( rec_fn[rec_lvl] );
						rec_fn[rec_lvl] = NULL;
					}
				}
			}
			// File was OK :)
			if( rec_fn[rec_lvl] != NULL )
			{
				len = trap_FS_FOpenFile( rec_fn[rec_lvl], &f, FS_READ );
				if( len > 0 )
				{
					rec_plvl = rec_lvl;
					rec_buf[rec_lvl] = CG_Malloc( len + 1 );
					rec_buf[rec_lvl][len] = '\0';
					rec_ptr[rec_lvl] = rec_buf[rec_lvl];
					// Now read the file
					if( trap_FS_Read( rec_buf[rec_lvl], len, f ) <= 0 )
					{
						CG_Free( rec_fn[rec_lvl] );
						CG_Free( rec_buf[rec_lvl] );
						rec_fn[rec_lvl] = NULL;
						rec_buf[rec_lvl] = NULL;
						if( rec_lvl > 0 )
						{
							CG_Printf( "HUD: WARNING: Read error while loading file: %s\n", rec_fn[rec_lvl] );
						}
						rec_lvl--;
					}
					trap_FS_FCloseFile( f );
				}
				else
				{
					if( !len )
					{
						// File was empty - still have to close
						trap_FS_FCloseFile( f );
					}
					else if( rec_lvl > 0 )
					{
						CG_Printf( "HUD: WARNING: Could not include file: %s\n", rec_fn[rec_lvl] );
					}
					CG_Free( rec_fn[rec_lvl] );
					rec_fn[rec_lvl] = NULL;
					rec_lvl--;
				}
			}
			else
			{
				// Skip this file, go down one level
				rec_lvl--;
			}
			rec_plvl = rec_lvl;
		}
		else if( rec_lvl < rec_plvl )
		{
			// Free previous level buffer
			if( rec_fn[rec_plvl] ) CG_Free( rec_fn[rec_plvl] );
			if( rec_buf[rec_plvl] ) CG_Free( rec_buf[rec_plvl] );
			rec_buf[rec_plvl] = NULL;
			rec_ptr[rec_plvl] = NULL;
			rec_fn[rec_plvl] = NULL;
			rec_plvl = rec_lvl;
			if( rec_lvl < 0 )
			{
				// Break - end of recursive looping
				if( retbuf == NULL )
				{
					CG_Printf( "HUD: ERROR: Could not load empty HUD-script: %s\n", path );
				}
				break;
			}
		}
		if( rec_lvl < 0 )
		{
			break;
		}
		token = COM_ParseExt2( ( const char ** )&rec_ptr[rec_lvl], qtrue, qfalse );
		if( !Q_stricmp( "include", token ) )
		{
			// Handle include
			token = COM_ParseExt2( ( const char ** )&rec_ptr[rec_lvl], qfalse, qfalse );
			if( ( ( rec_lvl + 1 ) < HUD_MAX_LVL ) && ( rec_ptr[rec_lvl] ) && ( token ) && ( token[0] != '\0' ) )
			{
				// Go to next recursive level and prepare it's filename :)
				rec_lvl++;
				i = strlen( "huds/" ) + strlen( token ) + strlen( ".hud" ) + 1;
				rec_fn[rec_lvl] = CG_Malloc( i );
				Q_snprintfz( rec_fn[rec_lvl], i, "huds/%s", token );
				COM_DefaultExtension( rec_fn[rec_lvl], ".hud", i );
				if( trap_FS_FOpenFile( rec_fn[rec_lvl], NULL, FS_READ ) < 0 )
				{
					// File doesn't exist!
					CG_Free( rec_fn[rec_lvl] );
					i = strlen( "huds/inc/" ) + strlen( token ) + strlen( ".hud" ) + 1;
					rec_fn[rec_lvl] = CG_Malloc( i );
					Q_snprintfz( rec_fn[rec_lvl], i, "huds/inc/%s", token );
					COM_DefaultExtension( rec_fn[rec_lvl], ".hud", i );
					if( trap_FS_FOpenFile( rec_fn[rec_lvl], NULL, FS_READ ) < 0 )
					{
						CG_Free( rec_fn[rec_lvl] );
						rec_fn[rec_lvl] = NULL;
						rec_lvl--;
					}
				}
			}
		}
		else if( !Q_stricmp( "precache", token ) )
		{
			// Handle graphics precaching
			if( rec_ptr[rec_lvl] == NULL )
			{
				CG_Printf( "HUD: ERROR: EOF instead of file argument for preload\n" );
			}
			else
			{
				token = COM_ParseExt2( ( const char ** )&rec_ptr[rec_lvl], qfalse, qfalse );
				if( ( token ) && ( token[0] != '\0' ) )
				{
					if( developer->integer )
						CG_Printf( "HUD: INFO: Precaching image '%s'\n", token );
					trap_R_RegisterPic( token );
				}
				else
				{
					CG_Printf( "HUD: ERROR: Missing argument for preload\n" );
				}
			}
		}
		else if( ( len = strlen( token ) ) > 0 )
		{
			// Normal token, add to token-pool.
			if( ( retuse + len + 1 ) >= retlen )
			{
				// Enlarge token buffer by 1kb
				retlen += 1024;
				tmpbuf = CG_Malloc( retlen );
				if( retbuf )
				{
					memcpy( tmpbuf, retbuf, retuse );
					CG_Free( retbuf );
				}
				retbuf = tmpbuf;
				retbuf[retuse] = '\0';
			}
			strcat( &retbuf[retuse], token );
			retuse += len;
			strcat( &retbuf[retuse], " " );
			retuse++;
			retbuf[retuse] = '\0';
		}
		// Detect "end-of-file" of included files and go down 1 level.
		if( ( rec_lvl <= rec_plvl ) && ( !rec_ptr[rec_lvl] ) )
		{
			rec_plvl = rec_lvl;
			rec_lvl--;
		}
	}
	if( retbuf == NULL )
	{
		CG_Printf( "HUD: ERROR: Could not load file: %s\n", path );
	}
	return retbuf;
}

/*
   //================
   //CG_OptimizeStatusBarFile
   //================

   static char *CG_OptimizeStatusBarFile( char *path, qboolean skip_include )
   {
    int length, f;
    char *temp_buffer;
    char *opt_buffer;
    char *parse, *token, *toinclude;
    int optimized_length, included_length;
    int fi, fi_length;
    size_t fipath_size;
    char *fipath;

    // load the file
    length = trap_FS_FOpenFile( path, &f, FS_READ );
    if( length == -1 )
   	return NULL;
    if( !length ) {
   	trap_FS_FCloseFile( f );
   	return NULL;
    }

    // alloc a temp buffer according to size
    temp_buffer = CG_Malloc( length + 1 );

    // load layout file in memory
    trap_FS_Read( temp_buffer, length, f );
    trap_FS_FCloseFile( f );

    // first pass: scan buffer line by line and check for include lines
    // if found compute needed length for included files
    // else count token length as Com_Parse as skipped comments and stuff like that
    parse=temp_buffer;
    optimized_length=0;
    included_length=0;
    while ( parse )
    {
   	token=COM_ParseExt2( &parse, qtrue, qfalse );

   	if( (!Q_stricmp( token, "include" )) && skip_include == qfalse)
   	{
   	    toinclude=COM_ParseExt2( &parse, qtrue, qfalse );
   	    //if( cg_debug_HUD && cg_debug_HUD->integer )
   	    //CG_Printf( "included: %s \n", toinclude );

   	    fipath_size = strlen("huds/inc/") + strlen(toinclude) + strlen(".hud") + 1;
   	    fipath = CG_Malloc( fipath_size );
   	    Q_snprintfz( fipath, fipath_size, "huds/inc/%s", toinclude );
   	    COM_DefaultExtension( fipath, ".hud", fipath_size );
   	    fi_length = trap_FS_FOpenFile( fipath, &fi, FS_READ );

   	    if( fi_length == -1 )
   	    {
   		// failed to include file
   		CG_Printf( "HUD: Failed to include hud subfile: %s \n", fipath );
   	    }

   	    if( fi_length > 0)
   	    {
   		// not an empty file
   		// we have the size we can close it
   		included_length+=fi_length;
   	    }
   	    trap_FS_FCloseFile( fi );

   	    CG_Free( fipath );
   	    fipath = NULL;
   	    fipath_size = 0;
   	} else {
   	    // not an include line
   	    // simply count token size for optimized hud layout
   	    optimized_length+=strlen( token ) + 1; // for spaces
   	}
    }

    // second pass: we now have the needed size
    // alloc optimized buffer
    opt_buffer = CG_Malloc( optimized_length + included_length + 1 );

    // reparse all file and copy it
    parse=temp_buffer;
    while ( parse )
    {
   	token=COM_ParseExt2( &parse, qtrue, qfalse );

   	if( (!Q_stricmp( token, "include" )) && skip_include == qfalse)
   	{
   	    toinclude=COM_ParseExt2( &parse, qtrue, qfalse );

   	    fipath_size = strlen("huds/inc/") + strlen(toinclude) + strlen(".hud") + 1;
   	    fipath = CG_Malloc( fipath_size );
   	    Q_snprintfz( fipath, fipath_size, "huds/inc/%s", toinclude );
   	    COM_ReplaceExtension( fipath, ".hud", fipath_size );
   	    fi_length = trap_FS_FOpenFile( fipath, &fi, FS_READ );

   	    if( fi_length == -1 )
   	    {
   		// failed to include file
   		CG_Printf( "HUD: Failed to include hud subfile: %s \n", path );
   	    }

   	    if( fi_length > 0)
   	    {
   		char *fi_parse;
   		char *include_buffer;

   		// not an empty file
   		if( cg_debug_HUD && cg_debug_HUD->integer )
   		    CG_Printf( "HUD: Including sub hud file: %s \n", toinclude );

   		// reparse all lines from included file to skip include commands

   		// alloc a temp buffer according to size
   		include_buffer = CG_Malloc( fi_length + 1 );

   		// load included layout file in memory
   		trap_FS_Read( include_buffer, fi_length, fi );

   		fi_parse=include_buffer;
   		while ( fi_parse )
   		{
   		    token=COM_ParseExt2( &fi_parse, qtrue, qfalse );

   		    if( !Q_stricmp( token, "include" ) )
   		    {
   			// skip recursive include
   			toinclude=COM_ParseExt2( &fi_parse, qtrue, qfalse );
   			CG_Printf( "HUD: No recursive include allowed: huds/inc/%s \n", toinclude );
   		    }
   		    else
   		    {
   			// normal token
   			strcat( opt_buffer, token );
   			strcat( opt_buffer, " ");
   		    }
   		}

   		// release memory
   		CG_Free( include_buffer );
   	    }

   	    // close included file
   	    trap_FS_FCloseFile( fi );

   	    CG_Free( fipath );
   	    fipath = NULL;
   	    fipath_size = 0;
   	} else {
   	    // normal token
   	    strcat( opt_buffer, token );
   	    strcat( opt_buffer, " ");
   	}
    }

    // free temp buffer
    CG_Free( temp_buffer );

    return opt_buffer;
   }
 */

/*
* CG_LoadStatusBarFile
*/
static void CG_LoadStatusBarFile( char *path )
{
	char *opt;
	int i;

	assert( path && path[0] );

	//opt = CG_OptimizeStatusBarFile( path, qfalse );
	opt = CG_LoadHUDFile( path );

	if( opt == NULL )
	{
		CG_Printf( "HUD: failed to load %s file\n", path );
		return;
	}
	// load the new status bar program
	CG_ParseLayoutScript( opt, cg.statusBar );
	// Free the opt buffer!
	CG_Free( opt );

	// set up layout font as default system font
	layout_cursor_font = trap_SCR_RegisterFont( DEFAULT_FONT_SMALL );

	for( i = 0; i < WEAP_TOTAL-1; i++ )
	{
		customWeaponPics[i] = NULL;
		customNoGunWeaponPics[i] = NULL;
	}
	customWeaponSelectPic = NULL;
}

/*
* CG_LoadStatusBar
*/
void CG_LoadStatusBar( void )
{
	size_t filename_size;
	char *filename;

	assert( cg_clientHUD && cg_clientHUD->dvalue[0] );

	// buffer for filenames
	filename_size = strlen( "huds/" ) + max( strlen( cg_clientHUD->dvalue ), strlen( cg_clientHUD->string ) ) + 4 + 1;
	filename = CG_Malloc( filename_size );

	// always load default first. Custom second if needed
	if( cg_debug_HUD && cg_debug_HUD->integer )
		CG_Printf( "HUD: Loading default clientHUD huds/%s\n", cg_clientHUD->dvalue );
	Q_snprintfz( filename, filename_size, "huds/%s", cg_clientHUD->dvalue );
	COM_DefaultExtension( filename, ".hud", filename_size );
	CG_LoadStatusBarFile( filename );

	if( cg_clientHUD->string[0] && Q_stricmp( cg_clientHUD->string, cg_clientHUD->dvalue ) )
	{
		if( cg_debug_HUD && cg_debug_HUD->integer )
			CG_Printf( "HUD: Loading custom clientHUD huds/%s\n", cg_clientHUD->string );
		Q_snprintfz( filename, filename_size, "huds/%s", cg_clientHUD->string );
		COM_DefaultExtension( filename, ".hud", filename_size );
		CG_LoadStatusBarFile( filename );
	}

	CG_Free( filename );
}
