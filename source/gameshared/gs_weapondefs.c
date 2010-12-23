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

// gs_weapondefs.c	-	hard coded weapon definitions

#include "q_arch.h"
#include "q_math.h"
#include "q_shared.h"
#include "q_comref.h"
#include "q_collision.h"
#include "gs_public.h"

#define INSTANT 0

#define WEAPONDOWN_FRAMETIME 50
#define WEAPONUP_FRAMETIME 50
#define DEFAULT_BULLET_SPREAD 350

gs_weapon_definition_t gs_weaponDefs[] =
{
	{
		"no weapon",
		WEAP_NONE,
		{
			FIRE_MODE_STRONG,               // fire mode
			AMMO_NONE,                      // ammo tag
			0,                              // ammo usage per shot
			0,                              // projectiles fired each shot

			//timings (in msecs)
			WEAPONUP_FRAMETIME,             // weapon up frametime
			WEAPONDOWN_FRAMETIME,                            // weapon down frametime
			0,                              // reload frametime
			0,                              // cooldown frametime
			0,                              // projectile timeout
			0,								// smooth refire

			//damages
			0,                              // damage
			0,								// selfdamage ratio
			0,                              // knockback
			0,								// stun
			0,                              // splash radius
			0,                              // splash minimum damage
			0,                              // splash minimum knockback

			//projectile def
			0,                              // speed
			0,                              // spread

			//ammo
			0,                              // pickup amount
			0                               // max amount
		},

		{
			FIRE_MODE_WEAK,                 // fire mode
			AMMO_NONE,                      // ammo tag
			0,                              // ammo usage per shot
			0,                              // projectiles fired each shot

			//timings (in msecs)
			WEAPONUP_FRAMETIME,             // weapon up frametime
			WEAPONDOWN_FRAMETIME,                            // weapon down frametime
			0,                              // reload frametime
			0,                              // cooldown frametime
			0,                              // projectile timeout
			0,								// smooth refire

			//damages
			0,                              // damage
			0,								// selfdamage ratio
			0,                              // knockback
			0,								// stun
			0,                              // splash radius
			0,                              // splash minimum damage
			0,                              // splash minimum knockback

			//projectile def
			0,                              // speed
			0,                              // spread

			//ammo
			0,                              // pickup amount
			0                               // max amount
		},
	},

	{
		"gunblade",
		WEAP_GUNBLADE,
		{
			FIRE_MODE_STRONG,
			AMMO_GUNBLADE,
			1,								// ammo usage per shot
			1,								// projectiles fired each shot

			//timings (in msecs)
			WEAPONUP_FRAMETIME,				// weapon up frametime
			WEAPONDOWN_FRAMETIME,           // weapon down frametime
			400,							// reload frametime
			0,								// cooldown frametime
			5000,							// projectile timeout
			0,								// smooth refire

			//damages
			45,								// damage
			0.25,							// selfdamage ratio
			78,								// knockback
			0,								// stun
			80,								// splash radius
			8,								// splash minimum damage
			10,                             // splash minimum knockback

			//projectile def
			3000,							// speed
			0,								// spread

			//ammo
			2,								// pickup amount
			10								// max amount
		},

		{
			FIRE_MODE_WEAK,
			AMMO_WEAK_GUNBLADE,
			0,								// ammo usage per shot
			0,								// projectiles fired each shot

			//timings (in msecs)
			WEAPONUP_FRAMETIME,				// weapon up frametime
			WEAPONDOWN_FRAMETIME,			// weapon down frametime
			800,							// reload frametime
			0,								// cooldown frametime
			64,								// projectile timeout  / projectile range for instant weapons
			0,								// smooth refire

			//damages
			50,								// damage
			0,								// selfdamage ratio
			50,								// knockback
			0,								// stun
			0,								// splash radius
			0,								// splash minimum damage
			0,                              // splash minimum knockback

			//projectile def
			0,								// speed
			0,								// spread

			//ammo
			0,								// pickup amount
			0								// max amount
		},
	},

	{
		"Machinegun",
		WEAP_MACHINEGUN,
		{
			FIRE_MODE_STRONG,
			AMMO_STRONG_BULLETS,
			1,								// ammo usage per shot
			1,								// projectiles fired each shot

			//timings (in msecs)
			WEAPONUP_FRAMETIME,				// weapon up frametime
			WEAPONDOWN_FRAMETIME,			// weapon down frametime
			75,								// reload frametime
			0,								// cooldown frametime
			6000,							// projectile timeout
			0,								// smooth refire

			//damages
			7.65,							// damage
			0,								// selfdamage ratio
			5,								// knockback
			0,								// stun
			0,								// splash radius
			0,								// splash minimum damage
			0,                              // splash minimum knockback

			//projectile def
			INSTANT,						// speed
			175,							// spread

			//ammo
			0,								// pickup amount
			0								// max amount
		},

		{
			FIRE_MODE_WEAK,
			AMMO_BULLETS,
			1,								// ammo usage per shot
			1,								// projectiles fired each shot

			//timings (in msecs)
			WEAPONUP_FRAMETIME,				// weapon up frametime
			WEAPONDOWN_FRAMETIME,			// weapon down frametime
			75,								// reload frametime
			0,								// cooldown frametime
			6000,							// projectile timeout
			0,								// smooth refire

			//damages
			7.65,							// damage
			0,								// selfdamage ratio
			5,								// knockback
			0,								// stun
			0,								// splash radius
			0,								// splash minimum damage
			0,                              // splash minimum knockback

			//projectile def
			INSTANT,						// speed
			175,							// spread

			//ammo
			100,							// pickup amount
			100								// max amount
		},
	},

	{
		"Riotgun",
		WEAP_RIOTGUN,
		{
			FIRE_MODE_STRONG,
			AMMO_SHELLS,
			1,								// ammo usage per shot
			26,								// projectiles fired each shot

			//timings (in msecs)
			WEAPONUP_FRAMETIME,				// weapon up frametime
			WEAPONDOWN_FRAMETIME,			// weapon down frametime
			900,							// reload frametime
			0,								// cooldown frametime
			8192,							// projectile timeout / projectile range for instant weapons
			0,								// smooth refire

			//damages
			4,								// damage
			0,								// selfdamage ratio (rg cant selfdamage)
			4,								// knockback
			85,								// stun
			0,								// splash radius
			0,								// splash minimum damage
			0,                              // splash minimum knockback

			//projectile def
			INSTANT,						// speed
			60,								// spread

			//ammo
			5,								// pickup amount
			10								// max amount
		},

		{
			FIRE_MODE_WEAK,
			AMMO_WEAK_SHELLS,
			1,								// ammo usage per shot
			23,								// projectiles fired each shot

			//timings (in msecs)
			WEAPONUP_FRAMETIME,				// weapon up frametime
			WEAPONDOWN_FRAMETIME,			// weapon down frametime
			900,							// reload frametime
			0,								// cooldown frametime
			8192,							// projectile timeout / projectile range for instant weapons
			0,								// smooth refire

			//damages
			4,								// damage
			0,								// selfdamage ratio (rg cant selfdamage)
			4,								// knockback
			85,								// stun
			0,								// splash radius
			0,								// splash minimum damage
			0,                              // splash minimum knockback

			//projectile def
			INSTANT,						// speed
			70,								// spread

			//ammo
			10,								// pickup amount
			15								// max amount
		},
	},

	{
		"Grenade Launcher",
		WEAP_GRENADELAUNCHER,
		{
			FIRE_MODE_STRONG,
			AMMO_GRENADES,
			1,								// ammo usage per shot
			1,								// projectiles fired each shot

			//timings (in msecs)
			WEAPONUP_FRAMETIME,				// weapon up frametime
			WEAPONDOWN_FRAMETIME,			// weapon down frametime
			800,							// reload frametime
			0,								// cooldown frametime
			1250,							// projectile timeout
			0,								// smooth refire

			//damages
			65,								// damage
			0.65,							// selfdamage ratio
			100,							// knockback
			1500,							// stun
			170,							// splash radius
			15,								// splash minimum damage
			10,                             // splash minimum knockback

			//projectile def
			900,							// speed
			0,								// spread

			//ammo
			5,								// pickup amount
			10								// max amount
		},

		{
			FIRE_MODE_WEAK,
			AMMO_WEAK_GRENADES,
			1,								// ammo usage per shot
			1,								// projectiles fired each shot

			//timings (in msecs)
			WEAPONUP_FRAMETIME,				// weapon up frametime
			WEAPONDOWN_FRAMETIME,			// weapon down frametime
			800,							// reload frametime
			0,								// cooldown frametime
			1250,							// projectile timeout
			0,								// smooth refire

			//damages
			60,								// damage
			0.65,							// selfdamage ratio
			90,								// knockback
			1500,							// stun
			160,							// splash radius
			15,								// splash minimum damage
			5,                              // splash minimum knockback

			//projectile def
			900,							// speed
			0,								// spread

			//ammo
			10,								// pickup amount
			15								// max amount
		},
	},

	{
		"Rocket Launcher",
		WEAP_ROCKETLAUNCHER,
		{
			FIRE_MODE_STRONG,
			AMMO_ROCKETS,
			1,								// ammo usage per shot
			1,								// projectiles fired each shot

			//timings (in msecs)
			WEAPONUP_FRAMETIME,             // weapon up frametime
			WEAPONDOWN_FRAMETIME,           // weapon down frametime
            /* racesow (950 in warsow.5, 850 in racesow.42) */
            850,                            // reload frametime
            /* !racesow */
			0,								// cooldown frametime
            100000,                         // projectile timeout; racesow
			0,								// smooth refire

			//damages
			75,								// damage
			0.65,							// selfdamage ratio
			100,							// knockback
			1500,							// stun
			140,							// splash radius
			8,								// splash minimum damage
			8,                             // splash minimum knockback

			//projectile def
            /* racesow (1150 in warsow.5, 950 in racesow.42) */
            950,                            // speed
            /* !racesow */
			0,								// spread

			//ammo
			5,								// pickup amount
			10								// max amount
		},

		{
			FIRE_MODE_WEAK,
			AMMO_WEAK_ROCKETS,
			1,								// ammo usage per shot
			1,								// projectiles fired each shot

			//timings (in msecs)
			WEAPONUP_FRAMETIME,             // weapon up frametime
			WEAPONDOWN_FRAMETIME,           // weapon down frametime
            /* racesow (950 in warsow.5, 850 in racesow.42) */
            850,                            // reload frametime
            /* !racesow */
			0,								// cooldown frametime
			100000,                          // projectile timeout; racesow
			0,								// smooth refire

			//damages
			70,								// damage
			0.65,							// selfdamage ratio
			95,								// knockback
			1500,							// stun
			140,							// splash radius
			6,								// splash minimum damage
			5,                              // splash minimum knockback

			//projectile def
            /* racesow (1150 in warsow.5, 950 in racesow.42) */
            950,                            // speed
            /* !racesow */
			0,								// spread

			//ammo
			10,								// pickup amount
			10								// max amount
		},
	},
			
	{
		"Plasmagun",
		WEAP_PLASMAGUN,
		{
			FIRE_MODE_STRONG,
			AMMO_PLASMA,
			1,								// ammo usage per shot
			1,								// projectiles fired each shot

			//timings (in msecs)
			WEAPONUP_FRAMETIME,				// weapon up frametime
			WEAPONDOWN_FRAMETIME,			// weapon down frametime
			100,							// reload frametime
			0,								// cooldown frametime
			5000,							// projectile timeout
			0,								// smooth refire

			//damages
			15,								// damage
			0.5,							// selfdamage ratio
			18,								// knockback
			200,							// stun
			45,								// splash radius
			5,								// splash minimum damage
			1,                              // splash minimum knockback

			//projectile def
			2400,							// speed
			0,								// spread

			//ammo
			40,								// pickup amount
			70								// max amount
		},

		{
			FIRE_MODE_WEAK,
			AMMO_WEAK_PLASMA,
			1,								// ammo usage per shot
			1,								// projectiles fired each shot

			//timings (in msecs)
			WEAPONUP_FRAMETIME,				// weapon up frametime
			WEAPONDOWN_FRAMETIME,			// weapon down frametime
			100,							// reload frametime
			0,								// cooldown frametime
			5000,							// projectile timeout
			0,								// smooth refire

			//damages
			12,								// damage
			0.5,							// selfdamage ratio
			14,								// knockback
			200,							// stun
			45,								// splash radius
			0,								// splash minimum damage
			1,                              // splash minimum knockback

			//projectile def
			2400,							// speed
            /* racesow (90 in warsow.5, 0 in racesow.42) */
            0,                              // spread
            /* !racesow */

			//ammo
			70,								// pickup amount
			125								// max amount
		},
	},

	{
		"Lasergun",
		WEAP_LASERGUN,
		{
			FIRE_MODE_STRONG,
			AMMO_LASERS,
			1,								// ammo usage per shot
			1,								// projectiles fired each shot

			//timings (in msecs)
			WEAPONUP_FRAMETIME,             // weapon up frametime
			WEAPONDOWN_FRAMETIME,           // weapon down frametime
			50,								// reload frametime
			0,								// cooldown frametime
			800,							// projectile timeout / projectile range for instant weapons
			1,								// smooth refire

			//damages
			6.15,							// damage
			0,								// selfdamage ratio (lg cant damage)
			8,								// knockback
			200,							// stun
			0,								// splash radius
			0,								// splash minimum damage
			0,                              // splash minimum knockback

			//projectile def
			INSTANT,						// speed
			0,								// spread

			//ammo
			50,								// pickup amount
			100								// max amount
		},

		{
			FIRE_MODE_WEAK,
			AMMO_WEAK_LASERS,
			1,								// ammo usage per shot
			1,								// projectiles fired each shot

			//timings (in msecs)
			WEAPONUP_FRAMETIME,				// weapon up frametime
			WEAPONDOWN_FRAMETIME,			// weapon down frametime
			50,								// reload frametime
			0,								// cooldown frametime
			800,							// projectile timeout / projectile range for instant weapons
			1,								// smooth refire

			//damages
			6.15,							// damage
			0,								// selfdamage ratio (lg cant damage)
			8,								// knockback
			200,							// stun
			0,								// splash radius
			0,								// splash minimum damage
			0,                              // splash minimum knockback

			//projectile def
			INSTANT,						// speed
			0,								// spread

			//ammo
			100,							// pickup amount
			175								// max amount
		},
	},

	{
		"Electrobolt",
		WEAP_ELECTROBOLT,
		{
			FIRE_MODE_STRONG,
			AMMO_BOLTS,
			1,								// ammo usage per shot
			1,								// projectiles fired each shot

			//timings (in msecs)
			WEAPONUP_FRAMETIME,				// weapon up frametime
			WEAPONDOWN_FRAMETIME,			// weapon down frametime
			1250,							// reload frametime
			0,								// cooldown frametime
			900,							// min damage range
			0,								// smooth refire

			//damages
			70,								// damage
			0,  							// selfdamage ratio
			40,								// knockback
			1000,							// stun
			0,								// splash radius
			70,								// minimum damage
			35,								// minimum knockback

			//projectile def
			INSTANT,						// speed
			0,								// spread

			//ammo
			5,								// pickup amount
			5								// max amount
		},

		{
			FIRE_MODE_WEAK,
			AMMO_WEAK_BOLTS,
			1,								// ammo usage per shot
			1,								// projectiles fired each shot

			//timings (in msecs)
			WEAPONUP_FRAMETIME,				// weapon up frametime
			WEAPONDOWN_FRAMETIME,			// weapon down frametime
			1250,							// reload frametime
			0,								// cooldown frametime
			1024,							// min damage range
			0,								// smooth refire

			//damages
			70,								// damage
			0,  							// selfdamage ratio (eb cant selfdamage)
			40,								// knockback
			1000,							// stun
			0,								// splash radius
			55,								// minimum damage
			35,								// minimum knockback

			//projectile def
			6000,							// speed
			0,								// spread

			//ammo
			5,								// pickup amount
			10								// max amount
		},
	},

	{
		"Instagun",
		WEAP_INSTAGUN,
		{
			FIRE_MODE_STRONG,
			AMMO_INSTAS,
			1,								// ammo usage per shot
			1,								// projectiles fired each shot

			//timings (in msecs)
			WEAPONUP_FRAMETIME,				// weapon up frametime
			WEAPONDOWN_FRAMETIME,			// weapon down frametime
			1300,							// reload frametime
			0,								// cooldown frametime
			8024,							// range
			0,								// smooth refire

			//damages
			200,							// damage
			0.1,							// selfdamage ratio (ig cant damage)
			95,								// knockback
			1000,							// stun
			80,								// splash radius
			0,								// splash minimum damage
			0,                              // splash minimum knockback

			//projectile def
			INSTANT,						// speed
			0,								// spread

			//ammo
			5,								// pickup amount
			5								// max amount
		},

		{
			FIRE_MODE_WEAK,
			AMMO_WEAK_INSTAS,
			1,								// ammo usage per shot
			1,								// projectiles fired each shot

			//timings (in msecs)
			WEAPONUP_FRAMETIME,				// weapon up frametime
			WEAPONDOWN_FRAMETIME,			// weapon down frametime
			1300,							// reload frametime
			0,								// cooldown frametime
			8024,							// range
			0,								// smooth refire

			//damages
			125,							// damage
			0.1,							// selfdamage ratio (ig cant damage)
			95,								// knockback
			1000,							// stun
			80,								// splash radius
			0,								// splash minimum damage
			0,                              // splash minimum knockback

			//projectile def
			INSTANT,						// speed
			0,								// spread

			//ammo
			5,								// pickup amount
			15								// max amount
		},
	},
};

#define GS_NUMWEAPONDEFS ( sizeof( gs_weaponDefs )/sizeof( gs_weapon_definition_t ) )

/*
* GS_GetWeaponDef
*/
gs_weapon_definition_t *GS_GetWeaponDef( int weapon )
{
	assert( GS_NUMWEAPONDEFS == WEAP_TOTAL );
	assert( weapon >= 0 && weapon < WEAP_TOTAL );
	return &gs_weaponDefs[weapon];
}

/*
* GS_InitWeapons
*/
void GS_InitWeapons( void )
{
	int i;
	gsitem_t *item;
	gs_weapon_definition_t *weapondef;

	for( i = WEAP_GUNBLADE; i < WEAP_TOTAL; i++ )
	{
		item = GS_FindItemByTag( i );
		weapondef = GS_GetWeaponDef( i );

		assert( item && weapondef );

		// hack : use the firedef pickup counts on items
		if( item->weakammo_tag && GS_FindItemByTag( item->weakammo_tag ) )
		{
			GS_FindItemByTag( item->weakammo_tag )->quantity = weapondef->firedef_weak.ammo_pickup;
			GS_FindItemByTag( item->weakammo_tag )->inventory_max = weapondef->firedef_weak.ammo_max;
		}

		if( item->ammo_tag && GS_FindItemByTag( item->ammo_tag ) )
		{
			GS_FindItemByTag( item->ammo_tag )->quantity = weapondef->firedef.ammo_pickup;
			GS_FindItemByTag( item->ammo_tag )->inventory_max = weapondef->firedef.ammo_max;
		}
	}
}
