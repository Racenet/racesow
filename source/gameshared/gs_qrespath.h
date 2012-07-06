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

//
//
// q_paths.h
//
//



//
//
// SHADERS
//
//

//outlines
#define DEFAULT_OUTLINE_HEIGHT	    0.3f

// icons

// armor
#define PATH_GA_ICON		    "gfx/hud/icons/armor/ga"
#define PATH_YA_ICON		    "gfx/hud/icons/armor/ya"
#define PATH_RA_ICON		    "gfx/hud/icons/armor/ra"
#define PATH_SHARD_ICON		    "gfx/hud/icons/armor/shard"

// weapon
#define PATH_GUNBLADE_ICON	    "gfx/hud/icons/weapon/gunblade"
#define PATH_SHOCKWAVE_ICON	    "gfx/hud/icons/weapon/shockwave"
#define PATH_MACHINEGUN_ICON	"gfx/hud/icons/weapon/machinegun"
#define PATH_RIOTGUN_ICON	    "gfx/hud/icons/weapon/riot"
#define PATH_GRENADELAUNCHER_ICON   "gfx/hud/icons/weapon/grenade"
#define PATH_ROCKETLAUNCHER_ICON    "gfx/hud/icons/weapon/rocket"
#define PATH_PLASMAGUN_ICON	    "gfx/hud/icons/weapon/plasma"
#define PATH_ELECTROBOLT_ICON	    "gfx/hud/icons/weapon/electro"
#define PATH_LASERGUN_ICON	    "gfx/hud/icons/weapon/laser"
#define PATH_INSTAGUN_ICON	"gfx/hud/icons/weapon/instagun"

// ammo
#define PATH_GUNBLADE_AMMO_ICON		"gfx/hud/icons/ammo/gunbladeammo"
#define PATH_SHOCKWAVE_AMMO_ICON	"gfx/hud/icons/ammo/shockwaveammo"
#define PATH_MACHINEGUN_AMMO_ICON	"gfx/hud/icons/ammo/bulletsammo"
#define PATH_RIOTGUN_AMMO_ICON		"gfx/hud/icons/ammo/riotammo"
#define PATH_GRENADELAUNCHER_AMMO_ICON	"gfx/hud/icons/ammo/grenadeammo"
#define PATH_ROCKETLAUNCHER_AMMO_ICON	"gfx/hud/icons/ammo/rocketammo"
#define PATH_PLASMAGUN_AMMO_ICON	"gfx/hud/icons/ammo/plasmaammo"
#define PATH_ELECTROBOLT_AMMO_ICON	"gfx/hud/icons/ammo/electroammo"
#define PATH_LASERGUN_AMMO_ICON		"gfx/hud/icons/ammo/laserammo"
#define PATH_INSTAGUN_AMMO_ICON	    "gfx/hud/icons/ammo/instaammo"

#define PATH_AMMOPACK_ICON		"gfx/hud/icons/ammo/pack"

// nogun weap
#define PATH_NG_GUNBLADE_ICON		"gfx/hud/icons/weapon/nogun_gunblade"
#define PATH_NG_SHOCKWAVE_ICON		"gfx/hud/icons/weapon/nogun_shockwave"
#define PATH_NG_MACHINEGUN_ICON		"gfx/hud/icons/weapon/nogun_machinegun"
#define PATH_NG_RIOTGUN_ICON		"gfx/hud/icons/weapon/nogun_riot"
#define PATH_NG_GRENADELAUNCHER_ICON	"gfx/hud/icons/weapon/nogun_grenade"
#define PATH_NG_ROCKETLAUNCHER_ICON	"gfx/hud/icons/weapon/nogun_rocket"
#define PATH_NG_PLASMAGUN_ICON		"gfx/hud/icons/weapon/nogun_plasma"
#define PATH_NG_ELECTROBOLT_ICON	"gfx/hud/icons/weapon/nogun_electro"
#define PATH_NG_LASERGUN_ICON		"gfx/hud/icons/weapon/nogun_laser"
#define PATH_NG_INSTAGUN_ICON	    "gfx/hud/icons/weapon/nogun_instagun"

// misc
#define PATH_BALLONCHAT_ICON	    "gfx/2d/bubblechat"

#define PATH_HEALTH_5_ICON	    "gfx/hud/icons/health/5"
#define PATH_HEALTH_25_ICON	    "gfx/hud/icons/health/25"
#define PATH_HEALTH_50_ICON	    "gfx/hud/icons/health/50"
#define PATH_HEALTH_100_ICON	    "gfx/hud/icons/health/100"
#define PATH_HEALTH_ULTRA_ICON	    "gfx/hud/icons/health/100ultra"

// powerups
#define PATH_QUAD_ICON		    "gfx/hud/icons/powerup/quad"
#define PATH_SHELL_ICON		    "gfx/hud/icons/powerup/warshell"
#define PATH_REGEN_ICON		    "gfx/hud/icons/powerup/regen"

// flags
#define PATH_ALPHAFLAG_ICON	    "gfx/hud/icons/flags/iconflag_alpha"
#define PATH_BETAFLAG_ICON	    "gfx/hud/icons/flags/iconflag_beta"

#define PATH_FLAG_FLARE_SHADER	    "gfx/misc/ctf_flare"

// decals
#define PATH_BULLET_MARK	    "gfx/decals/d_bullet_hit"
#define PATH_EXPLOSION_MARK	    "gfx/decals/d_explode_hit"

// explosions
#define PATH_ROCKET_EXPLOSION_SPRITE	"gfx/rocket_explosion"
#define PATH_ROCKET_EXPLOSION_RING_SPRITE   "gfx/misc/rlexplo_ring"

// simpleitems

#define PATH_SHOCKWAVE_SIMPLEITEM		"gfx/simpleitems/weapon/shockwave"
#define PATH_MACHINEGUN_SIMPLEITEM		"gfx/simpleitems/weapon/machinegun"
#define PATH_RIOTGUN_SIMPLEITEM			"gfx/simpleitems/weapon/riot"
#define PATH_GRENADELAUNCHER_SIMPLEITEM		"gfx/simpleitems/weapon/grenade"
#define PATH_ROCKETLAUNCHER_SIMPLEITEM		"gfx/simpleitems/weapon/rocket"
#define PATH_PLASMAGUN_SIMPLEITEM		"gfx/simpleitems/weapon/plasma"
#define PATH_LASERGUN_SIMPLEITEM		"gfx/simpleitems/weapon/laser"
#define PATH_ELECTROBOLT_SIMPLEITEM		"gfx/simpleitems/weapon/electro"
#define PATH_INSTAGUN_SIMPLEITEM		"gfx/simpleitems/weapon/instagun"
#define PATH_AMMOPACK_SIMPLEITEM		"gfx/simpleitems/ammo/pack"
#define PATH_GA_SIMPLEITEM			"gfx/simpleitems/armor/ga"
#define PATH_YA_SIMPLEITEM			"gfx/simpleitems/armor/ya"
#define PATH_RA_SIMPLEITEM			"gfx/simpleitems/armor/ra"
#define PATH_SHARD_SIMPLEITEM			"gfx/simpleitems/armor/shard"
#define PATH_HEALTH_5_SIMPLEITEM		"gfx/simpleitems/health/5"
#define PATH_HEALTH_25_SIMPLEITEM		"gfx/simpleitems/health/25"
#define PATH_HEALTH_50_SIMPLEITEM		"gfx/simpleitems/health/50"
#define PATH_HEALTH_100_SIMPLEITEM		"gfx/simpleitems/health/100"
#define PATH_HEALTH_ULTRA_SIMPLEITEM		"gfx/simpleitems/health/100ultra"
#define PATH_QUAD_SIMPLEITEM			"gfx/simpleitems/powerup/quad"
#define PATH_SHELL_SIMPLEITEM			"gfx/simpleitems/powerup/warshell"
#define PATH_REGEN_SIMPLEITEM			"gfx/simpleitems/powerup/regen"

#define PATH_KEYICON_FORWARD_ON			"gfx/hud/keys/key_forward_on"
#define PATH_KEYICON_BACKWARD_ON		"gfx/hud/keys/key_back_on"
#define PATH_KEYICON_LEFT_ON			"gfx/hud/keys/key_left_on"
#define PATH_KEYICON_RIGHT_ON			"gfx/hud/keys/key_right_on"
#define PATH_KEYICON_FIRE_ON			"gfx/hud/keys/act_fire_on"
#define PATH_KEYICON_JUMP_ON			"gfx/hud/keys/act_jump_on"
#define PATH_KEYICON_CROUCH_ON			"gfx/hud/keys/act_crouch_on"
#define PATH_KEYICON_SPECIAL_ON			"gfx/hud/keys/act_special_on"
#define PATH_KEYICON_FORWARD_OFF		"gfx/hud/keys/key_forward_off"
#define PATH_KEYICON_BACKWARD_OFF		"gfx/hud/keys/key_back_off"
#define PATH_KEYICON_LEFT_OFF			"gfx/hud/keys/key_left_off"
#define PATH_KEYICON_RIGHT_OFF			"gfx/hud/keys/key_right_off"
#define PATH_KEYICON_FIRE_OFF			"gfx/hud/keys/act_fire_off"
#define PATH_KEYICON_JUMP_OFF			"gfx/hud/keys/act_jump_off"
#define PATH_KEYICON_CROUCH_OFF			"gfx/hud/keys/act_crouch_off"
#define PATH_KEYICON_SPECIAL_OFF		"gfx/hud/keys/act_special_off"

// weapon firing
#define PATH_SMOKE_PUFF		    "smokePuff"

#define PATH_UKNOWN_MAP_PIC				"gfx/ui/unknownmap"

//
//
// MODELS
//
//

// armors
#define PATH_GA_MODEL		    "models/items/armor/ga/ga.md3"
#define PATH_YA_MODEL		    "models/items/armor/ya/ya.md3"
#define PATH_RA_MODEL		    "models/items/armor/ra/ra.md3"
#define PATH_SHARD_MODEL	    "models/items/armor/shard/shard.md3"

// weapons
#define PATH_GUNBLADE_MODEL		"models/weapons/gunblade/gunblade.md3"
#define PATH_SHOCKWAVE_MODEL		"models/weapons/shockwave/shockwave.md3"
#define PATH_MACHINEGUN_MODEL		"models/weapons/machinegun/machinegun.md3"
#define PATH_RIOTGUN_MODEL		"models/weapons/riotgun/riotgun.md3"
#define PATH_GRENADELAUNCHER_MODEL	"models/weapons/glauncher/glauncher.md3"
#define PATH_ROCKETLAUNCHER_MODEL	"models/weapons/rlauncher/rlauncher.md3"
#define PATH_PLASMAGUN_MODEL		"models/weapons/plasmagun/plasmagun.md3"
#define PATH_ELECTROBOLT_MODEL		"models/weapons/electrobolt/electrobolt.md3"
#define PATH_LASERGUN_MODEL		"models/weapons/lasergun/lasergun.md3"
#define PATH_INSTAGUN_MODEL		"models/weapons/instagun/instagun.md3"

// ammoboxes
#define PATH_AMMO_BOX_MODEL		"models/items/ammo/ammobox/ammobox.md3"
#define PATH_AMMO_BOX_MODEL2		"models/items/ammo/ammobox/ammobox_icon.md3"
#define PATH_AMMO_PACK_MODEL		"models/items/ammo/pack/pack.md3"

// health
#define PATH_SMALL_HEALTH_MODEL	    "models/items/health/small/small_health.md3"
#define PATH_MEDIUM_HEALTH_MODEL    "models/items/health/medium/medium_health.md3"
#define PATH_LARGE_HEALTH_MODEL	    "models/items/health/large/large_health.md3"
#define PATH_MEGA_HEALTH_MODEL	    "models/items/health/mega/mega_health.md3"
#define PATH_ULTRA_HEALTH_MODEL	    "models/items/health/ultra/ultra_health.md3"

// flags
#define	PATH_FLAG_BASE_MODEL	    "models/objects/flag/flag_base.md3"
#define	PATH_FLAG_MODEL		    "models/objects/flag/flag.md3"
/*
// captureAreaIndicators
#define PATH_CAPTUREAREA_A_MODEL	    "models/objects/capture_a/capture_a.md3"
#define PATH_CAPTUREAREA_B_MODEL	    "models/objects/capture_b/capture_b.md3"
#define PATH_CAPTUREAREA_C_MODEL	    "models/objects/capture_c/capture_c.md3"
#define PATH_CAPTUREAREA_D_MODEL	    "models/objects/capture_d/capture_d.md3"
*/
// powerups
#define	PATH_QUAD_MODEL		    "models/powerups/instant/quad.md3"
#define	PATH_QUAD_LIGHT_MODEL	    "models/powerups/instant/quad_light.md3"
#define	PATH_WARSHELL_BELT_MODEL    "models/powerups/instant/warshell_belt.md3"
#define	PATH_WARSHELL_SPHERE_MODEL  "models/powerups/instant/warshell_sphere.md3"
#define	PATH_REGEN_MODEL		    "models/powerups/instant/regen.md3"

// misc

// weapon projectiles
#define PATH_GUNBLADEBLAST_STRONG_MODEL	    "models/objects/projectile/gunblade/proj_gunbladeblast.md3"
#define PATH_PLASMA_WEAK_MODEL		    "models/objects/projectile/plasmagun/proj_plasmagun.md3"
#define PATH_PLASMA_STRONG_MODEL	    "models/objects/projectile/plasmagun/proj_plasmagun.md3"
#define PATH_GRENADE_WEAK_MODEL		    "models/objects/projectile/glauncher/grenadeweak.md3"
#define PATH_GRENADE_STRONG_MODEL	    "models/objects/projectile/glauncher/grenadestrong.md3"
#define PATH_ROCKET_WEAK_MODEL		    "models/objects/projectile/rlauncher/rocket_weak.md3"
#define PATH_ROCKET_STRONG_MODEL	    "models/objects/projectile/rlauncher/rocket_strong.md3"
#define PATH_ELECTROBOLT_WEAK_MODEL	    "models/objects/projectile/electrobolt/proj_electrobolt.md3"

#define PATH_BULLET_EXPLOSION_MODEL	    "models/weapon_hits/bullet/hit_bullet.md3"
#define PATH_GRENADE_EXPLOSION_MODEL	    "models/weapon_hits/glauncher/hit_glauncher.md3"
#define PATH_PLASMA_EXPLOSION_MODEL	    "models/weapon_hits/plasmagun/hit_plasmagun.md3"
#define PATH_ROCKET_EXPLOSION_MODEL	    "models/weapon_hits/rlauncher/hit_rlauncher.md3"
#define PATH_GUNBLADEBLAST_IMPACT_MODEL	    "models/weapon_hits/gunblade/hit_blast.md3"
#define PATH_GUNBLADEBLAST_EXPLOSION_MODEL  "models/weapon_hits/gunblade/hit_blastexp.md3"
#define PATH_ELECTROBLAST_IMPACT_MODEL	    "models/weapon_hits/electrobolt/hit_electrobolt.md3"
#define PATH_INSTABLAST_IMPACT_MODEL	    "models/weapon_hits/instagun/hit_instagun.md3"

#define GRENADE_EXPLOSION_MODEL_RADIUS		30.0
#define PLASMA_EXPLOSION_MODEL_RADIUS		10.0
#define ROCKET_EXPLOSION_MODEL_RADIUS		10.0
#define GUNBLADEBLAST_EXPLOSION_MODEL_RADIUS	10.0

//
//
// SOUNDS
//
//

// pickup
#define S_PICKUP_WEAPON		"sounds/items/weapon_pickup"
#define S_PICKUP_AMMO		"sounds/items/ammo_pickup"


#define S_PICKUP_HEALTH_SMALL	"sounds/items/health_5"          // pickup health +5
#define S_PICKUP_HEALTH_MEDIUM	"sounds/items/health_25"         // pickup health +25
#define S_PICKUP_HEALTH_LARGE	"sounds/items/health_50"         // pickup health +50
#define S_PICKUP_HEALTH_MEGA	"sounds/items/megahealth"        // pickup megahealth

#define S_PICKUP_ARMOR_GA	"sounds/items/armor_green"       // pickup green armor
#define S_PICKUP_ARMOR_YA	"sounds/items/armor_yellow"      // pickup yellow armor
#define S_PICKUP_ARMOR_RA	"sounds/items/armor_red"     // pickup red armor
#define S_PICKUP_ARMOR_SHARD	"sounds/items/armor_shard"       // pickup shard

#define S_PICKUP_QUAD		"sounds/items/quad_pickup"       // pickup Quad damage
#define S_PICKUP_SHELL		"sounds/items/shell_pickup"      // pickup WarShell
#define S_PICKUP_REGEN		"sounds/items/regen_pickup"      // pickup Regeneration

#define S_ITEM_RESPAWN		"sounds/items/item_spawn"        // item respawn
#define S_ITEM_QUAD_RESPAWN	"sounds/items/quad_spawn"        // Quad respawn
#define S_ITEM_WARSHELL_RESPAWN	"sounds/items/shell_spawn"       // WarShell respawn
#define S_ITEM_REGEN_RESPAWN	"sounds/items/regen_spawn"        // Regen respawn

// misc sounds
#define S_CHAT			"sounds/misc/chat"
#define S_TIMER_BIP_BIP		"sounds/misc/timer_bip_bip"
#define S_TIMER_PLOINK		"sounds/misc/timer_ploink"

//wsw: pb disable unreferenced sounds
//#define S_LAND					"sounds/misc/land"
#define S_HIT_WATER		"sounds/misc/hit_water"

#define S_TELEPORT		"sounds/world/tele_in"
#define S_JUMPPAD		"sounds/world/jumppad"
#define S_LAUNCHPAD		"sounds/world/launchpad"

//#define S_PLAT_START		"sounds/movers/elevator_start"
#define S_PLAT_START		NULL
#define S_PLAT_MOVE		"sounds/movers/elevator_move"
//#define S_PLAT_STOP		"sounds/movers/elevator_stop"
#define S_PLAT_STOP		NULL

#define S_DOOR_START		"sounds/movers/door_start"
//#define S_DOOR_MOVE		"sounds/movers/door_move"
#define S_DOOR_MOVE		NULL
#define S_DOOR_STOP		"sounds/movers/door_stop"

//#define S_DOOR_ROTATING_START	"sounds/movers/door_rotating_start"
#define S_DOOR_ROTATING_START	"sounds/movers/door_start"
//#define S_DOOR_ROTATING_MOVE	"sounds/movers/door_rotating_move"
#define S_DOOR_ROTATING_MOVE	NULL
//#define S_DOOR_ROTATING_STOP	"sounds/movers/door_rotating_stop"
#define S_DOOR_ROTATING_STOP	"sounds/movers/door_stop"

//#define S_FUNC_ROTATING_START	"sounds/movers/rotating_start"
//#define S_FUNC_ROTATING_MOVE	"sounds/movers/rotating_move"
//#define S_FUNC_ROTATING_STOP	"sounds/movers/rotating_stop"
#define S_FUNC_ROTATING_START	NULL
#define S_FUNC_ROTATING_MOVE	NULL
#define S_FUNC_ROTATING_STOP	NULL

#define S_BUTTON_START		"sounds/movers/button"

//#define S_QUAD_USE				"sounds/items/quad_use"
#define S_QUAD_FIRE		"sounds/items/quad_fire"

//#define S_SHELL_USE				"sounds/items/shell_use"
#define S_SHELL_HIT		"sounds/items/shell_hit"

// world sounds
#define S_WORLD_WATER_IN		    "sounds/world/water_in"
#define S_WORLD_UNDERWATER		    "sounds/world/underwater"
#define S_WORLD_WATER_OUT		    "sounds/world/water_out"

#define S_WORLD_SLIME_IN		    "sounds/world/water_in" // using water sounds for now
#define S_WORLD_UNDERSLIME		    "sounds/world/underwater"
#define S_WORLD_SLIME_OUT		    "sounds/world/water_out"

#define S_WORLD_LAVA_IN			    "sounds/world/lava_in"
#define S_WORLD_UNDERLAVA		    "sounds/world/underwater"
#define S_WORLD_LAVA_OUT		    "sounds/world/lava_out"

#define S_WORLD_SECRET			    "sounds/misc/secret"
#define S_WORLD_MESSAGE			    "sounds/misc/talk"

// player sounds
#define S_PLAYER_JUMP_1_to_2	    "*jump_%i"                   // player jump
#define S_PLAYER_WALLJUMP_1_to_2    "*wj_%i"                 // player walljump
#define S_PLAYER_DASH_1_to_2	    "*dash_%i"                   // player dash


//#define S_PLAYER_FALL_0_to_3		"*fall_%i"					// player fall (height)
#define S_PLAYER_FALLDEATH	    "*falldeath"             // player falling to his death
#define S_PLAYER_PAINS		    "*pain%i"                    // player pain (percent)
#define S_PLAYER_DEATH		    "*death"

#define S_PLAYER_DROWN		    "*drown"                 // player drown
#define S_PLAYER_GASP		    "*gasp"                      // player gasp
#define S_PLAYER_BURN_1_to_2		"*burn%i"                    // player dash

// PLAYERS FALL MISSING

// combat and weapons
#define S_WEAPON_HITS				"sounds/misc/hit_%i"
#define S_WEAPON_KILL				"sounds/misc/kill"
#define S_WEAPON_HIT_TEAM			"sounds/misc/hit_team"
#define S_WEAPON_SWITCH				"sounds/weapons/weapon_switch"
#define S_WEAPON_NOAMMO				"sounds/weapons/weapon_noammo"

// weapon sounds
//#define S_WEAPON_BULLET_HIT_1_to_2				"sounds/weapons/gunblade_weak_hit%i"
#define S_WEAPON_GUNBLADE_W_SHOT_1_to_3		"sounds/weapons/blade_strike%i"
#define S_WEAPON_GUNBLADE_W_HIT_FLESH_1_to_3	"sounds/weapons/blade_hitflsh%i"
#define S_WEAPON_GUNBLADE_W_HIT_WALL_1_to_2	"sounds/weapons/blade_hitwall%i"
#define S_WEAPON_GUNBLADE_S_SHOT		"sounds/weapons/bladegun_strong_fire"
#define S_WEAPON_GUNBLADE_S_HIT_1_to_2		"sounds/weapons/bladegun_strong_hit_%i"


#define S_WEAPON_RIOTGUN_W_HIT			"sounds/weapons/riotgun_weak_hit"
#define S_WEAPON_RIOTGUN_S_HIT			"sounds/weapons/riotgun_strong_hit"

#define S_WEAPON_GRENADE_W_BOUNCE_1_to_2	"sounds/weapons/gren_weak_bounce%i"
#define S_WEAPON_GRENADE_S_BOUNCE_1_to_2	"sounds/weapons/gren_strong_bounce%i"
#define S_WEAPON_GRENADE_W_HIT			"sounds/weapons/gren_weak_explosion"
#define S_WEAPON_GRENADE_S_HIT			"sounds/weapons/gren_strong_explosion1"

#define S_WEAPON_ROCKET_W_FLY			"sounds/weapons/rocket_fly_weak"
#define S_WEAPON_ROCKET_S_FLY			"sounds/weapons/rocket_fly_strong"
#define S_WEAPON_ROCKET_W_HIT			"sounds/weapons/rocket_weak_explosion"
#define S_WEAPON_ROCKET_S_HIT			"sounds/weapons/rocket_strong_explosion"

#define S_WEAPON_PLASMAGUN_W_FLY		"sounds/weapons/plasmagun_strong_fly"
#define S_WEAPON_PLASMAGUN_S_FLY		"sounds/weapons/plasmagun_weak_fly"
#define S_WEAPON_PLASMAGUN_W_HIT		"sounds/weapons/plasmagun_weak_explosion"
#define S_WEAPON_PLASMAGUN_S_HIT		"sounds/weapons/plasmagun_strong_explosion"

#define S_WEAPON_LASERGUN_S_HUM			"sounds/weapons/laser_strong_hum"
#define S_WEAPON_LASERGUN_W_HUM			"sounds/weapons/laser_weak_hum"
#define S_WEAPON_LASERGUN_S_QUAD_HUM		"sounds/weapons/laser_strong_quad_hum"
#define S_WEAPON_LASERGUN_W_QUAD_HUM		"sounds/weapons/laser_weak_quad_hum"
#define S_WEAPON_LASERGUN_S_STOP		"sounds/weapons/laser_strong_stop"
#define S_WEAPON_LASERGUN_W_STOP		"sounds/weapons/laser_weak_stop"

// announcer sounds
// readyup
#define S_ANNOUNCER_READY_UP_POLITE		"sounds/announcer/pleasereadyup"
#define S_ANNOUNCER_READY_UP_PISSEDOFF		"sounds/announcer/readyupalready"

// countdown
#define S_ANNOUNCER_COUNTDOWN_READY_1_to_2		"sounds/announcer/countdown/ready%02i"
#define S_ANNOUNCER_COUNTDOWN_GET_READY_TO_FIGHT_1_to_2	"sounds/announcer/countdown/get_ready_to_fight%02i"
#define S_ANNOUNCER_COUNTDOWN_FIGHT_1_to_2		"sounds/announcer/countdown/fight%02i"
//#define S_ANNOUNCER_COUNTDOWN_GO_1_to_2				"sounds/announcer/countdown/go%02i"
#define S_ANNOUNCER_COUNTDOWN_COUNT_1_to_3_SET_1_to_2	"sounds/announcer/countdown/%i_%02i"

// post match
#define S_ANNOUNCER_POSTMATCH_GAMEOVER_1_to_2		"sounds/announcer/postmatch/game_over%02i"

// timeout
//#define S_ANNOUNCER_TIMEOUT_MATCH_PAUSED_1_to_2		"sounds/announcer/timeout/matchpaused%02i"
#define S_ANNOUNCER_TIMEOUT_MATCH_RESUMED_1_to_2    "sounds/announcer/timeout/matchresumed%02i"
#define S_ANNOUNCER_TIMEOUT_TIMEOUT_1_to_2	    "sounds/announcer/timeout/timeout%02i"
#define S_ANNOUNCER_TIMEOUT_TIMEIN_1_to_2	    "sounds/announcer/timeout/timein%02i"

// callvote
#define S_ANNOUNCER_CALLVOTE_CALLED_1_to_2	    "sounds/announcer/callvote/vote_called%02i"
#define S_ANNOUNCER_CALLVOTE_FAILED_1_to_2	    "sounds/announcer/callvote/vote_failed%02i"
#define S_ANNOUNCER_CALLVOTE_PASSED_1_to_2	    "sounds/announcer/callvote/vote_passed%02i"
#define S_ANNOUNCER_CALLVOTE_VOTE_NOW		    "sounds/announcer/callvote/vote_now"

// overtime
#define S_ANNOUNCER_OVERTIME_GOING_TO_OVERTIME	    "sounds/announcer/overtime/going_to_overtime"
#define S_ANNOUNCER_OVERTIME_OVERTIME		    "sounds/announcer/overtime/overtime"
#define S_ANNOUNCER_OVERTIME_SUDDENDEATH_1_to_2	    "sounds/announcer/overtime/suddendeath%02i"

// score
#define S_ANNOUNCER_SCORE_TAKEN_LEAD_1_to_2		"sounds/announcer/score/taken_lead%02i"
#define S_ANNOUNCER_SCORE_TEAM_TAKEN_LEAD_1_to_2	"sounds/announcer/score/taken_lead%02i"
#define S_ANNOUNCER_SCORE_LOST_LEAD_1_to_2		"sounds/announcer/score/lost_lead%02i"
#define S_ANNOUNCER_SCORE_TEAM_LOST_LEAD_1_to_2		"sounds/announcer/score/team_lost_lead%02i"
#define S_ANNOUNCER_SCORE_TIED_LEAD_1_to_2		"sounds/announcer/score/tied_lead%02i"
#define S_ANNOUNCER_SCORE_TEAM_TIED_LEAD_1_to_2		"sounds/announcer/score/team_tied_lead%02i"
#define S_ANNOUNCER_SCORE_TEAM_1_to_4_TAKEN_LEAD_1_to_2	"sounds/announcer/score/team%i_leads%02i"

// ctf
#define S_ANNOUNCER_CTF_RECOVERY_1_to_2		    "sounds/announcer/ctf/recovery%02i"
#define S_ANNOUNCER_CTF_RECOVERY_TEAM		    "sounds/announcer/ctf/recovery_team"
#define S_ANNOUNCER_CTF_RECOVERY_ENEMY		    "sounds/announcer/ctf/recovery_enemy"
#define S_ANNOUNCER_CTF_FLAG_TAKEN		    	"sounds/announcer/ctf/flag_taken"
#define S_ANNOUNCER_CTF_FLAG_TAKEN_TEAM_1_to_2 	"sounds/announcer/ctf/flag_taken_team%02i"
#define S_ANNOUNCER_CTF_FLAG_TAKEN_ENEMY_1_to_2 "sounds/announcer/ctf/flag_taken_enemy_%02i"
#define S_ANNOUNCER_CTF_SCORE_1_to_2		    "sounds/announcer/ctf/score%02i"
#define S_ANNOUNCER_CTF_SCORE_TEAM_1_to_2	    "sounds/announcer/ctf/score_team%02i"
#define S_ANNOUNCER_CTF_SCORE_ENEMY_1_to_2	    "sounds/announcer/ctf/score_enemy%02i"

//music
#define S_PLAYLIST_MENU							"sounds/music/menu.m3u"
#define S_PLAYLIST_MATCH						"sounds/music/match.m3u"
#define S_PLAYLIST_POSTMATCH					"sounds/music/postmatch.m3u"

//===============================
// UI
//===============================
#define S_UI_MENU_IN_SOUND			"sounds/menu/ok"
#define S_UI_MENU_MOVE_SOUND		"sounds/menu/mouseover"
#define S_UI_MENU_OUT_SOUND			"sounds/menu/back"

#define UI_SHADER_VIDEOBACK			"gfx/ui/background"
#define UI_SHADER_FXBACK			"gfx/ui/menubackfx"
#define UI_SHADER_BIGLOGO			"gfx/ui/logo512"
#define UI_SHADER_CURSOR			"gfx/ui/cursor"

#define UI_SHADER_PROGRESSBAR		"gfx/ui/progressbar"
#define UI_SHADER_BACKGROUND		"gfx/ui/loadingscreen"
#define UI_SHADER_BACKGROUND_LOADING "gfx/ui/loadingscreen%02i"

// vsay icons
#define PATH_VSAY_GENERIC_ICON	    "gfx/hud/icons/vsay/generic"
#define PATH_VSAY_NEEDHEALTH_ICON   "gfx/hud/icons/vsay/needhealth"
#define PATH_VSAY_NEEDWEAPON_ICON   "gfx/hud/icons/vsay/needweapon"
#define PATH_VSAY_NEEDARMOR_ICON    "gfx/hud/icons/vsay/needarmor"
#define PATH_VSAY_AFFIRMATIVE_ICON  "gfx/hud/icons/vsay/affirmative"
#define PATH_VSAY_NEGATIVE_ICON	    "gfx/hud/icons/vsay/negative"
#define PATH_VSAY_YES_ICON	    "gfx/hud/icons/vsay/yes"
#define PATH_VSAY_NO_ICON	    "gfx/hud/icons/vsay/no"
#define PATH_VSAY_ONDEFENSE_ICON    "gfx/hud/icons/vsay/ondefense"
#define PATH_VSAY_ONOFFENSE_ICON    "gfx/hud/icons/vsay/onoffense"
#define PATH_VSAY_OOPS_ICON	    "gfx/hud/icons/vsay/oops"
#define PATH_VSAY_SORRY_ICON	    "gfx/hud/icons/vsay/sorry"
#define PATH_VSAY_THANKS_ICON	    "gfx/hud/icons/vsay/thanks"
#define PATH_VSAY_NOPROBLEM_ICON    "gfx/hud/icons/vsay/noproblem"
#define PATH_VSAY_YEEHAA_ICON	    "gfx/hud/icons/vsay/yeehaa"
#define PATH_VSAY_GOODGAME_ICON	    "gfx/hud/icons/vsay/goodgame"
#define PATH_VSAY_DEFEND_ICON	    "gfx/hud/icons/vsay/defend"
#define PATH_VSAY_ATTACK_ICON	    "gfx/hud/icons/vsay/attack"
#define PATH_VSAY_NEEDBACKUP_ICON   "gfx/hud/icons/vsay/needbackup"
#define PATH_VSAY_BOOO_ICON	    "gfx/hud/icons/vsay/booo"
#define PATH_VSAY_NEEDDEFENSE_ICON  "gfx/hud/icons/vsay/needdefense"
#define PATH_VSAY_NEEDOFFENSE_ICON  "gfx/hud/icons/vsay/needoffense"
#define PATH_VSAY_NEEDHELP_ICON	    "gfx/hud/icons/vsay/needhelp"
#define PATH_VSAY_ROGER_ICON	    "gfx/hud/icons/vsay/roger"
#define PATH_VSAY_ARMORFREE_ICON    "gfx/hud/icons/vsay/armorfree"
#define PATH_VSAY_AREASECURED_ICON  "gfx/hud/icons/vsay/areasecured"
#define PATH_VSAY_SHUTUP_ICON	    "gfx/hud/icons/vsay/shutup"
#define PATH_VSAY_BOOMSTICK_ICON    "gfx/hud/icons/vsay/boomstick"
#define PATH_VSAY_GOTOPOWERUP_ICON  "gfx/hud/icons/vsay/gotopowerup"
#define PATH_VSAY_GOTOQUAD_ICON	    "gfx/hud/icons/vsay/gotoquad"
#define PATH_VSAY_OK_ICON	    "gfx/hud/icons/vsay/ok"

// vsay sounds
#define S_VSAY_NEEDHEALTH			"sounds/vsay/needhealth"
#define S_VSAY_NEEDWEAPON			"sounds/vsay/needweapon"
#define S_VSAY_NEEDARMOR			"sounds/vsay/needarmor"
#define S_VSAY_AFFIRMATIVE			"sounds/vsay/affirmative"
#define S_VSAY_NEGATIVE				"sounds/vsay/negative"
#define S_VSAY_YES				"sounds/vsay/yes"
#define S_VSAY_NO				"sounds/vsay/no"
#define S_VSAY_ONDEFENSE			"sounds/vsay/ondefense"
#define S_VSAY_ONOFFENSE			"sounds/vsay/onoffense"
#define S_VSAY_OOPS				"sounds/vsay/oops"
#define S_VSAY_SORRY				"sounds/vsay/sorry"
#define S_VSAY_THANKS				"sounds/vsay/thanks"
#define S_VSAY_NOPROBLEM			"sounds/vsay/noproblem"
#define S_VSAY_YEEHAA				"sounds/vsay/yeehaa"
#define S_VSAY_GOODGAME				"sounds/vsay/goodgame"
#define S_VSAY_DEFEND				"sounds/vsay/defend"
#define S_VSAY_ATTACK				"sounds/vsay/attack"
#define S_VSAY_NEEDBACKUP			"sounds/vsay/needbackup"
#define S_VSAY_BOOO				"sounds/vsay/booo"
#define S_VSAY_NEEDDEFENSE			"sounds/vsay/needdefense"
#define S_VSAY_NEEDOFFENSE			"sounds/vsay/needoffense"
#define S_VSAY_NEEDHELP				"sounds/vsay/needhelp"
#define S_VSAY_ROGER				"sounds/vsay/roger"
#define S_VSAY_ARMORFREE			"sounds/vsay/armorfree"
#define S_VSAY_AREASECURED			"sounds/vsay/areasecured"
#define S_VSAY_SHUTUP				"sounds/vsay/shutup"
#define S_VSAY_BOOMSTICK			"sounds/vsay/boomstick"
#define S_VSAY_GOTOPOWERUP			"sounds/vsay/gotopowerup"
#define S_VSAY_GOTOQUAD				"sounds/vsay/gotoquad"
#define S_VSAY_OK				"sounds/vsay/ok"
