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

#include "cg_local.h"

cgs_media_handle_t *sfx_headnode;

/*
   =================
   CG_RegisterMediaSfx
   =================
 */
static cgs_media_handle_t *CG_RegisterMediaSfx( char *name, qboolean precache )
{
	cgs_media_handle_t *mediasfx;

	for( mediasfx = sfx_headnode; mediasfx; mediasfx = mediasfx->next )
	{
		if( !Q_stricmp( mediasfx->name, name ) )
			return mediasfx;
	}

	mediasfx = CG_Malloc( sizeof( cgs_media_handle_t ) );
	mediasfx->name = CG_CopyString( name );
	mediasfx->next = sfx_headnode;
	sfx_headnode = mediasfx;

	if( precache )
		mediasfx->data = ( void * )trap_S_RegisterSound( mediasfx->name );

	return mediasfx;
}

/*
   =================
   CG_MediaSfx
   =================
 */
struct sfx_s *CG_MediaSfx( cgs_media_handle_t *mediasfx )
{
	if( !mediasfx->data )
		mediasfx->data = ( void * )trap_S_RegisterSound( mediasfx->name );
	return ( struct sfx_s * )mediasfx->data;
}

/*
   =================
   CG_RegisterMediaSounds
   =================
 */
void CG_RegisterMediaSounds( void )
{
	int i;

	sfx_headnode = NULL;

	cgs.media.sfxChat = CG_RegisterMediaSfx( S_CHAT, qtrue );
	//timer sounds
	cgs.media.sfxTimerBipBip = CG_RegisterMediaSfx( S_TIMER_BIP_BIP, qtrue );
	cgs.media.sfxTimerPloink = CG_RegisterMediaSfx( S_TIMER_PLOINK, qtrue );

	for( i = 0; i < 3; i++ )
		cgs.media.sfxRic[i] = CG_RegisterMediaSfx( va( "sounds/weapons/ric%i", i+1 ), qfalse );

	// weapon
	for( i = 0; i < 4; i++ )
		cgs.media.sfxWeaponHit[i] = CG_RegisterMediaSfx( va( S_WEAPON_HITS, i ), qtrue );
	cgs.media.sfxWeaponKill = CG_RegisterMediaSfx( S_WEAPON_KILL, qtrue );
	cgs.media.sfxWeaponHitTeam = CG_RegisterMediaSfx( S_WEAPON_HIT_TEAM, qtrue );
	cgs.media.sfxWeaponUp = CG_RegisterMediaSfx( S_WEAPON_SWITCH, qtrue );
	cgs.media.sfxWeaponUpNoAmmo = CG_RegisterMediaSfx( S_WEAPON_NOAMMO, qtrue );

	cgs.media.sfxWalljumpFailed = CG_RegisterMediaSfx( "sounds/world/ft_walljump_failed", qtrue );

	cgs.media.sfxItemRespawn = CG_RegisterMediaSfx( S_ITEM_RESPAWN, qtrue );

	cgs.media.sfxTeleportIn = CG_RegisterMediaSfx( S_TELEPORT, qtrue );
	cgs.media.sfxTeleportOut = CG_RegisterMediaSfx( S_TELEPORT, qtrue );
	//	cgs.media.sfxJumpPad = CG_RegisterMediaSfx ( S_JUMPPAD, qtrue );
	cgs.media.sfxShellHit = CG_RegisterMediaSfx( S_SHELL_HIT, qtrue );

	// Gunblade sounds (weak is blade):
	for( i = 0; i < 3; i++ ) cgs.media.sfxGunbladeWeakShot[i] = CG_RegisterMediaSfx( va( S_WEAPON_GUNBLADE_W_SHOT_1_to_3, i + 1 ), qtrue );
	for( i = 0; i < 3; i++ ) cgs.media.sfxBladeFleshHit[i] = CG_RegisterMediaSfx( va( S_WEAPON_GUNBLADE_W_HIT_FLESH_1_to_3, i+1 ), qtrue );
	for( i = 0; i < 2; i++ ) cgs.media.sfxBladeWallHit[i] = CG_RegisterMediaSfx( va( S_WEAPON_GUNBLADE_W_HIT_WALL_1_to_2, i+1 ), qfalse );
	cgs.media.sfxGunbladeStrongShot = CG_RegisterMediaSfx( S_WEAPON_GUNBLADE_S_SHOT, qtrue );
	for( i = 0; i < 3; i++ ) cgs.media.sfxGunbladeStrongHit[i] = CG_RegisterMediaSfx( va( S_WEAPON_GUNBLADE_S_HIT_1_to_2, i + 1 ), qtrue );

	// Riotgun sounds :
	cgs.media.sfxRiotgunWeakHit = CG_RegisterMediaSfx( S_WEAPON_RIOTGUN_W_HIT, qtrue );
	cgs.media.sfxRiotgunStrongHit = CG_RegisterMediaSfx( S_WEAPON_RIOTGUN_S_HIT, qtrue );

	// Grenade launcher sounds :
	for( i = 0; i < 2; i++ ) cgs.media.sfxGrenadeWeakBounce[i] = CG_RegisterMediaSfx( va( S_WEAPON_GRENADE_W_BOUNCE_1_to_2, i+1 ), qtrue );
	for( i = 0; i < 2; i++ ) cgs.media.sfxGrenadeStrongBounce[i] = CG_RegisterMediaSfx( va( S_WEAPON_GRENADE_S_BOUNCE_1_to_2, i+1 ), qtrue );
	cgs.media.sfxGrenadeWeakExplosion = CG_RegisterMediaSfx( S_WEAPON_GRENADE_W_HIT, qtrue );
	cgs.media.sfxGrenadeStrongExplosion = CG_RegisterMediaSfx( S_WEAPON_GRENADE_S_HIT, qtrue );

	// Rocket launcher sounds :
	cgs.media.sfxRocketLauncherWeakHit = CG_RegisterMediaSfx( S_WEAPON_ROCKET_W_HIT, qtrue );
	cgs.media.sfxRocketLauncherStrongHit = CG_RegisterMediaSfx( S_WEAPON_ROCKET_S_HIT, qtrue );

	// Plasmagun sounds :
	cgs.media.sfxPlasmaWeakHit = CG_RegisterMediaSfx( S_WEAPON_PLASMAGUN_W_HIT, qtrue );
	cgs.media.sfxPlasmaStrongHit = CG_RegisterMediaSfx( S_WEAPON_PLASMAGUN_S_HIT, qtrue );

	cgs.media.sfxQuadFireSound = CG_RegisterMediaSfx( S_QUAD_FIRE, qtrue );

	//VSAY sounds
	cgs.media.sfxVSaySounds[VSAY_GENERIC] = CG_RegisterMediaSfx( S_CHAT, qtrue );
	cgs.media.sfxVSaySounds[VSAY_NEEDHEALTH] = CG_RegisterMediaSfx( S_VSAY_NEEDHEALTH, qtrue );
	cgs.media.sfxVSaySounds[VSAY_NEEDWEAPON] = CG_RegisterMediaSfx( S_VSAY_NEEDWEAPON, qtrue );
	cgs.media.sfxVSaySounds[VSAY_NEEDARMOR] = CG_RegisterMediaSfx( S_VSAY_NEEDARMOR, qtrue );
	cgs.media.sfxVSaySounds[VSAY_AFFIRMATIVE] = CG_RegisterMediaSfx( S_VSAY_AFFIRMATIVE, qtrue );
	cgs.media.sfxVSaySounds[VSAY_NEGATIVE] = CG_RegisterMediaSfx( S_VSAY_NEGATIVE, qtrue );
	cgs.media.sfxVSaySounds[VSAY_YES] = CG_RegisterMediaSfx( S_VSAY_YES, qtrue );
	cgs.media.sfxVSaySounds[VSAY_NO] = CG_RegisterMediaSfx( S_VSAY_NO, qtrue );
	cgs.media.sfxVSaySounds[VSAY_ONDEFENSE] = CG_RegisterMediaSfx( S_VSAY_ONDEFENSE, qtrue );
	cgs.media.sfxVSaySounds[VSAY_ONOFFENSE] = CG_RegisterMediaSfx( S_VSAY_ONOFFENSE, qtrue );
	cgs.media.sfxVSaySounds[VSAY_OOPS] = CG_RegisterMediaSfx( S_VSAY_OOPS, qtrue );
	cgs.media.sfxVSaySounds[VSAY_SORRY] = CG_RegisterMediaSfx( S_VSAY_SORRY, qtrue );
	cgs.media.sfxVSaySounds[VSAY_THANKS] = CG_RegisterMediaSfx( S_VSAY_THANKS, qtrue );
	cgs.media.sfxVSaySounds[VSAY_NOPROBLEM] = CG_RegisterMediaSfx( S_VSAY_NOPROBLEM, qtrue );
	cgs.media.sfxVSaySounds[VSAY_YEEHAA] = CG_RegisterMediaSfx( S_VSAY_YEEHAA, qtrue );
	cgs.media.sfxVSaySounds[VSAY_GOODGAME] = CG_RegisterMediaSfx( S_VSAY_GOODGAME, qtrue );
	cgs.media.sfxVSaySounds[VSAY_DEFEND] = CG_RegisterMediaSfx( S_VSAY_DEFEND, qtrue );
	cgs.media.sfxVSaySounds[VSAY_ATTACK] = CG_RegisterMediaSfx( S_VSAY_ATTACK, qtrue );
	cgs.media.sfxVSaySounds[VSAY_NEEDBACKUP] = CG_RegisterMediaSfx( S_VSAY_NEEDBACKUP, qtrue );
	cgs.media.sfxVSaySounds[VSAY_BOOO] = CG_RegisterMediaSfx( S_VSAY_BOOO, qtrue );
	cgs.media.sfxVSaySounds[VSAY_NEEDDEFENSE] = CG_RegisterMediaSfx( S_VSAY_NEEDDEFENSE, qtrue );
	cgs.media.sfxVSaySounds[VSAY_NEEDOFFENSE] = CG_RegisterMediaSfx( S_VSAY_NEEDOFFENSE, qtrue );
	cgs.media.sfxVSaySounds[VSAY_NEEDHELP] = CG_RegisterMediaSfx( S_VSAY_NEEDHELP, qtrue );
	cgs.media.sfxVSaySounds[VSAY_ROGER] = CG_RegisterMediaSfx( S_VSAY_ROGER, qtrue );
	cgs.media.sfxVSaySounds[VSAY_ARMORFREE] = CG_RegisterMediaSfx( S_VSAY_ARMORFREE, qtrue );
	cgs.media.sfxVSaySounds[VSAY_AREASECURED] = CG_RegisterMediaSfx( S_VSAY_AREASECURED, qtrue );
	cgs.media.sfxVSaySounds[VSAY_SHUTUP] = CG_RegisterMediaSfx( S_VSAY_SHUTUP, qtrue );
	cgs.media.sfxVSaySounds[VSAY_BOOMSTICK] = CG_RegisterMediaSfx( S_VSAY_BOOMSTICK, qtrue );
	cgs.media.sfxVSaySounds[VSAY_GOTOPOWERUP] = CG_RegisterMediaSfx( S_VSAY_GOTOPOWERUP, qtrue );
	cgs.media.sfxVSaySounds[VSAY_GOTOQUAD] = CG_RegisterMediaSfx( S_VSAY_GOTOQUAD, qtrue );
	cgs.media.sfxVSaySounds[VSAY_OK] = CG_RegisterMediaSfx( S_VSAY_OK, qtrue );
}

//======================================================================

cgs_media_handle_t *model_headnode;

/*
* CG_RegisterModel
*/
struct model_s *CG_RegisterModel( const char *name )
{
	struct model_s *model;

	model = trap_R_RegisterModel( name );

	// precache bones
	if( trap_R_SkeletalGetNumBones( model, NULL ) )
		CG_SkeletonForModel( model );

	return model;
}

/*
   =================
   CG_RegisterMediaModel
   =================
 */
static cgs_media_handle_t *CG_RegisterMediaModel( char *name, qboolean precache )
{
	cgs_media_handle_t *mediamodel;

	for( mediamodel = model_headnode; mediamodel; mediamodel = mediamodel->next )
	{
		if( !Q_stricmp( mediamodel->name, name ) )
			return mediamodel;
	}

	mediamodel = CG_Malloc( sizeof( cgs_media_handle_t ) );
	mediamodel->name = CG_CopyString( name );
	mediamodel->next = model_headnode;
	model_headnode = mediamodel;

	if( precache )
		mediamodel->data = ( void * )CG_RegisterModel( mediamodel->name );

	return mediamodel;
}

/*
   =================
   CG_MediaModel
   =================
 */
struct model_s *CG_MediaModel( cgs_media_handle_t *mediamodel )
{
	if( !mediamodel )
		return NULL;

	if( !mediamodel->data )
		mediamodel->data = ( void * )CG_RegisterModel( mediamodel->name );
	return ( struct model_s * )mediamodel->data;
}

/*
   =================
   CG_RegisterMediaModels
   =================
 */
void CG_RegisterMediaModels( void )
{
	int i;
	model_headnode = NULL;

	//	cgs.media.modGrenadeExplosion = CG_RegisterMediaModel( PATH_GRENADE_EXPLOSION_MODEL, qtrue );
	cgs.media.modRocketExplosion = CG_RegisterMediaModel( PATH_ROCKET_EXPLOSION_MODEL, qtrue );
	cgs.media.modPlasmaExplosion = CG_RegisterMediaModel( PATH_PLASMA_EXPLOSION_MODEL, qtrue );
	//	cgs.media.modBoltExplosion = CG_RegisterMediaModel( "models/weapon_hits/electrobolt/hit_electrobolt.md3", qtrue );
	//	cgs.media.modInstaExplosion = CG_RegisterMediaModel( "models/weapon_hits/instagun/hit_instagun.md3", qtrue );
	//	cgs.media.modTeleportEffect = CG_RegisterMediaModel( "models/misc/telep.md3", qfalse );

	cgs.media.modDash = CG_RegisterMediaModel( "models/effects/dash_burst.md3", qtrue );
	cgs.media.modHeadStun = CG_RegisterMediaModel( "models/effects/head_stun.md3", qtrue );

	cgs.media.modBulletExplode = CG_RegisterMediaModel( PATH_BULLET_EXPLOSION_MODEL, qtrue );
	cgs.media.modBladeWallHit = CG_RegisterMediaModel( PATH_GUNBLADEBLAST_IMPACT_MODEL, qtrue );
	cgs.media.modBladeWallExplo = CG_RegisterMediaModel( PATH_GUNBLADEBLAST_EXPLOSION_MODEL, qtrue );
	cgs.media.modElectroBoltWallHit = CG_RegisterMediaModel( PATH_ELECTROBLAST_IMPACT_MODEL, qtrue );
	cgs.media.modInstagunWallHit = CG_RegisterMediaModel( PATH_INSTABLAST_IMPACT_MODEL, qtrue );


	// gibs models
	for( i = 0; i < MAX_TECHY_GIBS; i++ )
		cgs.media.modTechyGibs[i] = CG_RegisterMediaModel( va( "models/objects/gibs/gib%i/gib%i.md3", i+1, i+1 ), qtrue );

	for( i = 0; i < MAX_MEATY_GIBS; i++ )
		cgs.media.modMeatyGibs[i] = CG_RegisterMediaModel( va( "models/objects/oldgibs/gib%i/gib%i.md3", i+1, i+1 ), qtrue );
}

//======================================================================

cgs_media_handle_t *shader_headnode;

/*
   =================
   CG_RegisterMediaShader
   =================
 */
static cgs_media_handle_t *CG_RegisterMediaShader( char *name, qboolean precache )
{
	cgs_media_handle_t *mediashader;

	for( mediashader = shader_headnode; mediashader; mediashader = mediashader->next )
	{
		if( !Q_stricmp( mediashader->name, name ) )
			return mediashader;
	}

	mediashader = CG_Malloc( sizeof( cgs_media_handle_t ) );
	mediashader->name = CG_CopyString( name );
	mediashader->next = shader_headnode;
	shader_headnode = mediashader;

	if( precache )
		mediashader->data = ( void * )trap_R_RegisterPic( mediashader->name );

	return mediashader;
}

/*
   =================
   CG_MediaShader
   =================
 */
struct shader_s *CG_MediaShader( cgs_media_handle_t *mediashader )
{
	if( !mediashader->data )
		mediashader->data = ( void * )trap_R_RegisterPic( mediashader->name );
	return ( struct shader_s * )mediashader->data;
}

char *sb_nums[11] =
{
	"gfx/hud/0", "gfx/hud/1",
	"gfx/hud/2", "gfx/hud/3",
	"gfx/hud/4", "gfx/hud/5",
	"gfx/hud/6", "gfx/hud/7",
	"gfx/hud/8", "gfx/hud/9",
	"gfx/hud/minus"
};


/*
   =================
   CG_RegisterMediaShaders
   =================
 */
void CG_RegisterMediaShaders( void )
{
	int i;

	shader_headnode = NULL;

	cgs.media.shaderParticle = CG_RegisterMediaShader( "particle", qtrue );

	cgs.media.shaderNet = CG_RegisterMediaShader( "gfx/hud/net", qtrue );
	cgs.media.shaderBackTile = CG_RegisterMediaShader( "gfx/ui/backtile", qtrue );
	cgs.media.shaderSelect = CG_RegisterMediaShader( "gfx/hud/select", qtrue );
	cgs.media.shaderChatBalloon = CG_RegisterMediaShader( PATH_BALLONCHAT_ICON, qtrue );
	cgs.media.shaderDownArrow = CG_RegisterMediaShader( "gfx/ui/arrow_down", qtrue );

	cgs.media.shaderPlayerShadow = CG_RegisterMediaShader( "gfx/decals/shadow", qtrue );

	cgs.media.shaderWaterBubble = CG_RegisterMediaShader( "gfx/misc/waterBubble", qtrue );
	cgs.media.shaderSmokePuff = CG_RegisterMediaShader( "gfx/misc/smokepuff", qtrue );

	cgs.media.shaderSmokePuff1 = CG_RegisterMediaShader( "gfx/misc/smokepuff1", qtrue );
	cgs.media.shaderSmokePuff2 = CG_RegisterMediaShader( "gfx/misc/smokepuff2", qtrue );
	cgs.media.shaderSmokePuff3 = CG_RegisterMediaShader( "gfx/misc/smokepuff3", qtrue );

	cgs.media.shaderSmokePuff1dark = CG_RegisterMediaShader( "gfx/misc/smokepuff1_dark", qtrue );
	cgs.media.shaderSmokePuff2dark = CG_RegisterMediaShader( "gfx/misc/smokepuff2_dark", qtrue );
	cgs.media.shaderSmokePuff3dark = CG_RegisterMediaShader( "gfx/misc/smokepuff3_dark", qtrue );

	cgs.media.shaderStrongRocketFireTrailPuff = CG_RegisterMediaShader( "gfx/misc/strong_rocket_fire", qtrue );
	cgs.media.shaderWeakRocketFireTrailPuff = CG_RegisterMediaShader( "gfx/misc/weak_rocket_fire", qtrue );
	cgs.media.shaderTeleporterSmokePuff = CG_RegisterMediaShader( "TeleporterSmokePuff", qtrue );
	cgs.media.shaderGrenadeTrailSmokePuff = CG_RegisterMediaShader( "gfx/grenadetrail_smoke_puf", qtrue );
	cgs.media.shaderBloodTrailPuff = CG_RegisterMediaShader( "gfx/misc/bloodtrail_puff", qtrue );
	cgs.media.shaderBloodTrailLiquidPuff = CG_RegisterMediaShader( "gfx/misc/bloodtrailliquid_puff", qtrue );
	cgs.media.shaderBloodImpactPuff = CG_RegisterMediaShader( "gfx/misc/bloodimpact_puff", qtrue );

	cgs.media.shaderAdditiveParticleShine = CG_RegisterMediaShader( "additiveParticleShine", qtrue );

	//	cgs.media.shaderBladeMark = CG_RegisterMediaShader( "gfx/decals/d_blade_hit" );
	cgs.media.shaderBulletMark = CG_RegisterMediaShader( "gfx/decals/d_bullet_hit", qtrue );
	cgs.media.shaderExplosionMark = CG_RegisterMediaShader( "gfx/decals/d_explode_hit", qtrue );
	cgs.media.shaderPlasmaMark = CG_RegisterMediaShader( "gfx/decals/d_plasma_hit", qtrue );
	cgs.media.shaderElectroboltMark = CG_RegisterMediaShader( "gfx/decals/d_electrobolt_hit", qtrue );
	cgs.media.shaderInstagunMark = CG_RegisterMediaShader( "gfx/decals/d_instagun_hit", qtrue );

	cgs.media.shaderElectroBeamOld = CG_RegisterMediaShader( "gfx/misc/electro", qtrue );
	cgs.media.shaderElectroBeamOldAlpha = CG_RegisterMediaShader( "gfx/misc/electro_alpha", qtrue );
	cgs.media.shaderElectroBeamOldBeta = CG_RegisterMediaShader( "gfx/misc/electro_beta", qtrue );
	cgs.media.shaderElectroBeamA = CG_RegisterMediaShader( "gfx/misc/electro2a", qtrue );
	cgs.media.shaderElectroBeamAAlpha = CG_RegisterMediaShader( "gfx/misc/electro2a_alpha", qtrue );
	cgs.media.shaderElectroBeamABeta = CG_RegisterMediaShader( "gfx/misc/electro2a_beta", qtrue );
	cgs.media.shaderElectroBeamB = CG_RegisterMediaShader( "gfx/misc/electro2b", qtrue );
	cgs.media.shaderElectroBeamBAlpha = CG_RegisterMediaShader( "gfx/misc/electro2b_alpha", qtrue );
	cgs.media.shaderElectroBeamBBeta = CG_RegisterMediaShader( "gfx/misc/electro2b_beta", qtrue );
	cgs.media.shaderInstaBeam = CG_RegisterMediaShader( "gfx/misc/instagun", qtrue );
	cgs.media.shaderLaserGunBeam = CG_RegisterMediaShader( "gfx/misc/laserbeam", qtrue );
	cgs.media.shaderRocketExplosion = CG_RegisterMediaShader( PATH_ROCKET_EXPLOSION_SPRITE, qtrue );
	cgs.media.shaderRocketExplosionRing = CG_RegisterMediaShader( PATH_ROCKET_EXPLOSION_RING_SPRITE, qtrue );

	cgs.media.shaderLaser = CG_RegisterMediaShader( "gfx/misc/laser", qfalse );

	cgs.media.shaderFlagFlare = CG_RegisterMediaShader( PATH_FLAG_FLARE_SHADER, qfalse ); // load only in ctf

	cgs.media.shaderRaceGhostEffect = CG_RegisterMediaShader( "gfx/raceghost", qfalse );

	// Kurim : weapon icons
	cgs.media.shaderWeaponIcon[WEAP_GUNBLADE-1] = CG_RegisterMediaShader( PATH_GUNBLADE_ICON, qtrue );
	cgs.media.shaderWeaponIcon[WEAP_MACHINEGUN-1] = CG_RegisterMediaShader( PATH_MACHINEGUN_ICON, qtrue );
	cgs.media.shaderWeaponIcon[WEAP_RIOTGUN-1] = CG_RegisterMediaShader( PATH_RIOTGUN_ICON, qtrue );
	cgs.media.shaderWeaponIcon[WEAP_GRENADELAUNCHER-1] = CG_RegisterMediaShader( PATH_GRENADELAUNCHER_ICON, qtrue );
	cgs.media.shaderWeaponIcon[WEAP_ROCKETLAUNCHER-1] = CG_RegisterMediaShader( PATH_ROCKETLAUNCHER_ICON, qtrue );
	cgs.media.shaderWeaponIcon[WEAP_PLASMAGUN-1] = CG_RegisterMediaShader( PATH_PLASMAGUN_ICON, qtrue );
	cgs.media.shaderWeaponIcon[WEAP_LASERGUN-1] = CG_RegisterMediaShader( PATH_LASERGUN_ICON, qtrue );
	cgs.media.shaderWeaponIcon[WEAP_ELECTROBOLT-1] = CG_RegisterMediaShader( PATH_ELECTROBOLT_ICON, qtrue );
	cgs.media.shaderWeaponIcon[WEAP_INSTAGUN-1] = CG_RegisterMediaShader( PATH_INSTAGUN_ICON, qtrue );

	cgs.media.shaderNoGunWeaponIcon[WEAP_GUNBLADE-1] = CG_RegisterMediaShader( PATH_NG_GUNBLADE_ICON, qtrue );
	cgs.media.shaderNoGunWeaponIcon[WEAP_MACHINEGUN-1] = CG_RegisterMediaShader( PATH_NG_MACHINEGUN_ICON, qtrue );
	cgs.media.shaderNoGunWeaponIcon[WEAP_RIOTGUN-1] = CG_RegisterMediaShader( PATH_NG_RIOTGUN_ICON, qtrue );
	cgs.media.shaderNoGunWeaponIcon[WEAP_GRENADELAUNCHER-1] = CG_RegisterMediaShader( PATH_NG_GRENADELAUNCHER_ICON, qtrue );
	cgs.media.shaderNoGunWeaponIcon[WEAP_ROCKETLAUNCHER-1] = CG_RegisterMediaShader( PATH_NG_ROCKETLAUNCHER_ICON, qtrue );
	cgs.media.shaderNoGunWeaponIcon[WEAP_PLASMAGUN-1] = CG_RegisterMediaShader( PATH_NG_PLASMAGUN_ICON, qtrue );
	cgs.media.shaderNoGunWeaponIcon[WEAP_LASERGUN-1] = CG_RegisterMediaShader( PATH_NG_LASERGUN_ICON, qtrue );
	cgs.media.shaderNoGunWeaponIcon[WEAP_ELECTROBOLT-1] = CG_RegisterMediaShader( PATH_NG_ELECTROBOLT_ICON, qtrue );
	cgs.media.shaderNoGunWeaponIcon[WEAP_INSTAGUN-1] = CG_RegisterMediaShader( PATH_NG_INSTAGUN_ICON, qtrue );

	// Kurim : keyicons
	cgs.media.shaderKeyIconOn[KEYICON_FORWARD] = CG_RegisterMediaShader( PATH_KEYICON_FORWARD_ON, qtrue );
	cgs.media.shaderKeyIconOn[KEYICON_BACKWARD] = CG_RegisterMediaShader( PATH_KEYICON_BACKWARD_ON, qtrue );
	cgs.media.shaderKeyIconOn[KEYICON_LEFT] = CG_RegisterMediaShader( PATH_KEYICON_LEFT_ON, qtrue );
	cgs.media.shaderKeyIconOn[KEYICON_RIGHT] = CG_RegisterMediaShader( PATH_KEYICON_RIGHT_ON, qtrue );
	cgs.media.shaderKeyIconOn[KEYICON_FIRE] = CG_RegisterMediaShader( PATH_KEYICON_FIRE_ON, qtrue );
	cgs.media.shaderKeyIconOn[KEYICON_JUMP] = CG_RegisterMediaShader( PATH_KEYICON_JUMP_ON, qtrue );
	cgs.media.shaderKeyIconOn[KEYICON_CROUCH] = CG_RegisterMediaShader( PATH_KEYICON_CROUCH_ON, qtrue );
	cgs.media.shaderKeyIconOn[KEYICON_SPECIAL] = CG_RegisterMediaShader( PATH_KEYICON_SPECIAL_ON, qtrue );
	cgs.media.shaderKeyIconOff[KEYICON_FORWARD] = CG_RegisterMediaShader( PATH_KEYICON_FORWARD_OFF, qtrue );
	cgs.media.shaderKeyIconOff[KEYICON_BACKWARD] = CG_RegisterMediaShader( PATH_KEYICON_BACKWARD_OFF, qtrue );
	cgs.media.shaderKeyIconOff[KEYICON_LEFT] = CG_RegisterMediaShader( PATH_KEYICON_LEFT_OFF, qtrue );
	cgs.media.shaderKeyIconOff[KEYICON_RIGHT] = CG_RegisterMediaShader( PATH_KEYICON_RIGHT_OFF, qtrue );
	cgs.media.shaderKeyIconOff[KEYICON_FIRE] = CG_RegisterMediaShader( PATH_KEYICON_FIRE_OFF, qtrue );
	cgs.media.shaderKeyIconOff[KEYICON_JUMP] = CG_RegisterMediaShader( PATH_KEYICON_JUMP_OFF, qtrue );
	cgs.media.shaderKeyIconOff[KEYICON_CROUCH] = CG_RegisterMediaShader( PATH_KEYICON_CROUCH_OFF, qtrue );
	cgs.media.shaderKeyIconOff[KEYICON_SPECIAL] = CG_RegisterMediaShader( PATH_KEYICON_SPECIAL_OFF, qtrue );

	for( i = 0; i < 11; i++ )
	{
		cgs.media.sbNums[i] = CG_RegisterMediaShader( sb_nums[i], qtrue );
	}

	for( i = 0; i < NUM_CROSSHAIRS; i++ )
		cgs.media.shaderCrosshair[i] = CG_RegisterMediaShader( va( "gfx/hud/crosshair%i", i ), qtrue );

	// VSAY icons
	cgs.media.shaderVSayIcon[VSAY_GENERIC] = CG_RegisterMediaShader( PATH_VSAY_GENERIC_ICON, qtrue );
	cgs.media.shaderVSayIcon[VSAY_NEEDHEALTH] = CG_RegisterMediaShader( PATH_VSAY_NEEDHEALTH_ICON, qtrue );
	cgs.media.shaderVSayIcon[VSAY_NEEDWEAPON] = CG_RegisterMediaShader( PATH_VSAY_NEEDWEAPON_ICON, qtrue );
	cgs.media.shaderVSayIcon[VSAY_NEEDARMOR] = CG_RegisterMediaShader( PATH_VSAY_NEEDARMOR_ICON, qtrue );
	cgs.media.shaderVSayIcon[VSAY_AFFIRMATIVE] = CG_RegisterMediaShader( PATH_VSAY_AFFIRMATIVE_ICON, qtrue );
	cgs.media.shaderVSayIcon[VSAY_NEGATIVE] = CG_RegisterMediaShader( PATH_VSAY_NEGATIVE_ICON, qtrue );
	cgs.media.shaderVSayIcon[VSAY_YES] = CG_RegisterMediaShader( PATH_VSAY_YES_ICON, qtrue );
	cgs.media.shaderVSayIcon[VSAY_NO] = CG_RegisterMediaShader( PATH_VSAY_NO_ICON, qtrue );
	cgs.media.shaderVSayIcon[VSAY_ONDEFENSE] = CG_RegisterMediaShader( PATH_VSAY_ONDEFENSE_ICON, qtrue );
	cgs.media.shaderVSayIcon[VSAY_ONOFFENSE] = CG_RegisterMediaShader( PATH_VSAY_ONOFFENSE_ICON, qtrue );
	cgs.media.shaderVSayIcon[VSAY_OOPS] = CG_RegisterMediaShader( PATH_VSAY_OOPS_ICON, qtrue );
	cgs.media.shaderVSayIcon[VSAY_SORRY] = CG_RegisterMediaShader( PATH_VSAY_SORRY_ICON, qtrue );
	cgs.media.shaderVSayIcon[VSAY_THANKS] = CG_RegisterMediaShader( PATH_VSAY_THANKS_ICON, qtrue );
	cgs.media.shaderVSayIcon[VSAY_NOPROBLEM] = CG_RegisterMediaShader( PATH_VSAY_NOPROBLEM_ICON, qtrue );
	cgs.media.shaderVSayIcon[VSAY_YEEHAA] = CG_RegisterMediaShader( PATH_VSAY_YEEHAA_ICON, qtrue );
	cgs.media.shaderVSayIcon[VSAY_GOODGAME] = CG_RegisterMediaShader( PATH_VSAY_GOODGAME_ICON, qtrue );
	cgs.media.shaderVSayIcon[VSAY_DEFEND] = CG_RegisterMediaShader( PATH_VSAY_DEFEND_ICON, qtrue );
	cgs.media.shaderVSayIcon[VSAY_ATTACK] = CG_RegisterMediaShader( PATH_VSAY_ATTACK_ICON, qtrue );
	cgs.media.shaderVSayIcon[VSAY_NEEDBACKUP] = CG_RegisterMediaShader( PATH_VSAY_NEEDBACKUP_ICON, qtrue );
	cgs.media.shaderVSayIcon[VSAY_BOOO] = CG_RegisterMediaShader( PATH_VSAY_BOOO_ICON, qtrue );
	cgs.media.shaderVSayIcon[VSAY_NEEDDEFENSE] = CG_RegisterMediaShader( PATH_VSAY_NEEDDEFENSE_ICON, qtrue );
	cgs.media.shaderVSayIcon[VSAY_NEEDOFFENSE] = CG_RegisterMediaShader( PATH_VSAY_NEEDOFFENSE_ICON, qtrue );
	cgs.media.shaderVSayIcon[VSAY_NEEDHELP] = CG_RegisterMediaShader( PATH_VSAY_NEEDHELP_ICON, qtrue );
	cgs.media.shaderVSayIcon[VSAY_ROGER] = CG_RegisterMediaShader( PATH_VSAY_ROGER_ICON, qtrue );
	cgs.media.shaderVSayIcon[VSAY_ARMORFREE] = CG_RegisterMediaShader( PATH_VSAY_ARMORFREE_ICON, qtrue );
	cgs.media.shaderVSayIcon[VSAY_AREASECURED] = CG_RegisterMediaShader( PATH_VSAY_AREASECURED_ICON, qtrue );
	cgs.media.shaderVSayIcon[VSAY_SHUTUP] = CG_RegisterMediaShader( PATH_VSAY_SHUTUP_ICON, qtrue );
	cgs.media.shaderVSayIcon[VSAY_BOOMSTICK] = CG_RegisterMediaShader( PATH_VSAY_BOOMSTICK_ICON, qtrue );
	cgs.media.shaderVSayIcon[VSAY_GOTOPOWERUP] = CG_RegisterMediaShader( PATH_VSAY_GOTOPOWERUP_ICON, qtrue );
	cgs.media.shaderVSayIcon[VSAY_GOTOQUAD] = CG_RegisterMediaShader( PATH_VSAY_GOTOQUAD_ICON, qtrue );
	cgs.media.shaderVSayIcon[VSAY_OK] = CG_RegisterMediaShader( PATH_VSAY_OK_ICON, qtrue );
}

void CG_RegisterLevelMinimap( void )
{
	int file;
	char *name, minimap[MAX_QPATH];

	cgs.shaderMiniMap = NULL;

	name = cgs.configStrings[CS_MAPNAME];

	Q_snprintfz( minimap, sizeof( minimap ), "minimaps/%s.tga", name );
	file = trap_FS_FOpenFile( minimap, NULL, FS_READ );

	if( file == -1 )
	{
		Q_snprintfz( minimap, sizeof( minimap ), "minimaps/%s.jpg", name );
		file = trap_FS_FOpenFile( minimap, NULL, FS_READ );
	}

	if( file != -1 )
		cgs.shaderMiniMap = trap_R_RegisterPic( minimap );
}

/*
* CG_RegisterFonts
*/
void CG_RegisterFonts( void )
{
	cvar_t *con_fontSystemSmall = trap_Cvar_Get( "con_fontSystemSmall", DEFAULT_FONT_SMALL, CVAR_ARCHIVE );
	cvar_t *con_fontSystemMedium = trap_Cvar_Get( "con_fontSystemMedium", DEFAULT_FONT_MEDIUM, CVAR_ARCHIVE );
	cvar_t *con_fontSystemBig = trap_Cvar_Get( "con_fontSystemBig", DEFAULT_FONT_BIG, CVAR_ARCHIVE );

	cgs.fontSystemSmall = trap_SCR_RegisterFont( con_fontSystemSmall->string );
	if( !cgs.fontSystemSmall )
	{
		cgs.fontSystemSmall = trap_SCR_RegisterFont( DEFAULT_FONT_SMALL );
		if( !cgs.fontSystemSmall )
			CG_Error( "Couldn't load default font \"%s\"", DEFAULT_FONT_SMALL );
	}
	cgs.fontSystemMedium = trap_SCR_RegisterFont( con_fontSystemMedium->string );
	if( !cgs.fontSystemMedium )
		cgs.fontSystemMedium = trap_SCR_RegisterFont( DEFAULT_FONT_MEDIUM );

	cgs.fontSystemBig = trap_SCR_RegisterFont( con_fontSystemBig->string );
	if( !cgs.fontSystemBig )
		cgs.fontSystemBig = trap_SCR_RegisterFont( DEFAULT_FONT_BIG );
}
