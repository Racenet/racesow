/*
Copyright (C) 2008 German Garcia

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

#include "g_local.h"
#include "../gameshared/angelref.h"

#define G_AsMalloc								G_LevelMalloc
#define G_AsFree								G_LevelFree

static angelwrap_api_t *angelExport = NULL;

#define SCRIPT_MODULE_NAME						"gametypes"

//=======================================================================

typedef struct
{
	const char * name;
	int value;
} asEnumVal_t;

typedef struct
{
	const char * name;
	const asEnumVal_t * values;
} asEnum_t;

typedef struct
{
	const char * declaration;
} asFuncdef_t;

typedef struct
{
	unsigned int behavior;
	const char * declaration;
	void *funcPointer;
	int callConv;
} asBehavior_t;

typedef struct
{
	const char * declaration;
	const void *funcPointer;
	int callConv;
} asMethod_t;

typedef struct
{
	const char * declaration;
	unsigned int offset;
} asProperty_t;

typedef struct
{
	const char * name;
	asEObjTypeFlags typeFlags; 
	size_t size;
	const asFuncdef_t * funcdefs;
	const asBehavior_t * objBehaviors;
	const asMethod_t * objMethods;
	const asProperty_t * objProperties;
	const void * stringFactory;
	const void * stringFactory_asGeneric;
} asClassDescriptor_t;

typedef struct
{
	char *declaration;
	void *pointer;
	void **asFuncPtr;
} asglobfuncs_t;

typedef struct
{
	char *declaration;
	void *pointer;
} asglobproperties_t;

//=======================================================================

#define ASLIB_LOCAL_CLASS_DESCR(x)

#define ASLIB_FOFFSET(s,m)						(size_t)&(((s *)0)->m)

#define ASLIB_ENUM_VAL(name)					{ #name,name }
#define ASLIB_ENUM_VAL_NULL						{ NULL, 0 }

#define ASLIB_ENUM_NULL							{ NULL, NULL }

#define ASLIB_FUNCTION_DECL(type,name,params)	(#type " " #name #params)

#define ASLIB_PROPERTY_DECL(type,name)			#type " " #name

#define ASLIB_FUNCTION_NULL						NULL
#define ASLIB_FUNCDEF_NULL						{ ASLIB_FUNCTION_NULL }
#define ASLIB_BEHAVIOR_NULL						{ 0, ASLIB_FUNCTION_NULL, NULL, 0 }
#define ASLIB_METHOD_NULL						{ ASLIB_FUNCTION_NULL, NULL, 0 }
#define ASLIB_PROPERTY_NULL						{ NULL, 0 }

#define ASLIB_Malloc(s)							(aslib_import.Mem_Alloc(s,__FILE__,__LINE__))
#define ASLIB_Free(x)							(aslib_import.Mem_Free(x,__FILE__,__LINE__))

static const asEnumVal_t asConfigstringEnumVals[] =
{
	ASLIB_ENUM_VAL( CS_MODMANIFEST ),
	ASLIB_ENUM_VAL( CS_MESSAGE ),
	ASLIB_ENUM_VAL( CS_MAPNAME ),
	ASLIB_ENUM_VAL( CS_AUDIOTRACK ),
	ASLIB_ENUM_VAL( CS_HOSTNAME ),
	ASLIB_ENUM_VAL( CS_TVSERVER ),
	ASLIB_ENUM_VAL( CS_SKYBOX ),
	ASLIB_ENUM_VAL( CS_STATNUMS ),
	ASLIB_ENUM_VAL( CS_POWERUPEFFECTS ),
	ASLIB_ENUM_VAL( CS_GAMETYPETITLE ),
	ASLIB_ENUM_VAL( CS_GAMETYPENAME ),
	ASLIB_ENUM_VAL( CS_GAMETYPEVERSION ),
	ASLIB_ENUM_VAL( CS_GAMETYPEAUTHOR ),
	ASLIB_ENUM_VAL( CS_AUTORECORDSTATE ),
	ASLIB_ENUM_VAL( CS_SCB_PLAYERTAB_LAYOUT ),
	ASLIB_ENUM_VAL( CS_SCB_PLAYERTAB_TITLES ),
	ASLIB_ENUM_VAL( CS_TEAM_ALPHA_NAME ),
	ASLIB_ENUM_VAL( CS_TEAM_BETA_NAME ),
	ASLIB_ENUM_VAL( CS_MAXCLIENTS ),
	ASLIB_ENUM_VAL( CS_MAPCHECKSUM ),
	ASLIB_ENUM_VAL( CS_MATCHNAME ),
	ASLIB_ENUM_VAL( CS_MATCHSCORE ),

	ASLIB_ENUM_VAL( CS_MODELS ),
	ASLIB_ENUM_VAL( CS_SOUNDS ),
	ASLIB_ENUM_VAL( CS_IMAGES ),
	ASLIB_ENUM_VAL( CS_SKINFILES ),
	ASLIB_ENUM_VAL( CS_LIGHTS ),
	ASLIB_ENUM_VAL( CS_ITEMS ),
	ASLIB_ENUM_VAL( CS_PLAYERINFOS ),
	ASLIB_ENUM_VAL( CS_GAMECOMMANDS ),
	ASLIB_ENUM_VAL( CS_LOCATIONS ),
	ASLIB_ENUM_VAL( CS_GENERAL ),

	ASLIB_ENUM_VAL_NULL
};

static const asEnumVal_t asEffectEnumVals[] =
{
	ASLIB_ENUM_VAL( EF_ROTATE_AND_BOB ),
	ASLIB_ENUM_VAL( EF_SHELL ),
	ASLIB_ENUM_VAL( EF_STRONG_WEAPON ),
	ASLIB_ENUM_VAL( EF_QUAD ),
	ASLIB_ENUM_VAL( EF_REGEN ),
	ASLIB_ENUM_VAL( EF_CARRIER ),
	ASLIB_ENUM_VAL( EF_BUSYICON ),
	ASLIB_ENUM_VAL( EF_FLAG_TRAIL ),
	ASLIB_ENUM_VAL( EF_TAKEDAMAGE ),
	ASLIB_ENUM_VAL( EF_TEAMCOLOR_TRANSITION ),
	ASLIB_ENUM_VAL( EF_EXPIRING_QUAD ),
	ASLIB_ENUM_VAL( EF_EXPIRING_SHELL ),
	ASLIB_ENUM_VAL( EF_EXPIRING_REGEN ),
	ASLIB_ENUM_VAL( EF_GODMODE ),

	ASLIB_ENUM_VAL( EF_PLAYER_STUNNED ),
	ASLIB_ENUM_VAL( EF_PLAYER_HIDENAME ),

	ASLIB_ENUM_VAL_NULL
};

static const asEnumVal_t asMatchStateEnumVals[] =
{
	//ASLIB_ENUM_VAL( MATCH_STATE_NONE ), // I see no point in adding it
	ASLIB_ENUM_VAL( MATCH_STATE_WARMUP ),
	ASLIB_ENUM_VAL( MATCH_STATE_COUNTDOWN ),
	ASLIB_ENUM_VAL( MATCH_STATE_PLAYTIME ),
	ASLIB_ENUM_VAL( MATCH_STATE_POSTMATCH ),
	ASLIB_ENUM_VAL( MATCH_STATE_WAITEXIT ),

	ASLIB_ENUM_VAL_NULL
};

static const asEnumVal_t asSpawnSystemEnumVals[] =
{
	ASLIB_ENUM_VAL( SPAWNSYSTEM_INSTANT ),
	ASLIB_ENUM_VAL( SPAWNSYSTEM_WAVES ),
	ASLIB_ENUM_VAL( SPAWNSYSTEM_HOLD ),

	ASLIB_ENUM_VAL_NULL
};

static const asEnumVal_t asHUDStatEnumVals[] =
{
	ASLIB_ENUM_VAL( STAT_PROGRESS_SELF ),
	ASLIB_ENUM_VAL( STAT_PROGRESS_OTHER ),
	ASLIB_ENUM_VAL( STAT_PROGRESS_ALPHA ),
	ASLIB_ENUM_VAL( STAT_PROGRESS_BETA ),
	ASLIB_ENUM_VAL( STAT_IMAGE_SELF ),
	ASLIB_ENUM_VAL( STAT_IMAGE_OTHER ),
	ASLIB_ENUM_VAL( STAT_IMAGE_ALPHA ),
	ASLIB_ENUM_VAL( STAT_IMAGE_BETA ),
	ASLIB_ENUM_VAL( STAT_TIME_SELF ),
	ASLIB_ENUM_VAL( STAT_TIME_BEST ),
	ASLIB_ENUM_VAL( STAT_TIME_RECORD ),
	ASLIB_ENUM_VAL( STAT_TIME_ALPHA ),
	ASLIB_ENUM_VAL( STAT_TIME_BETA ),
	ASLIB_ENUM_VAL( STAT_MESSAGE_SELF ),
	ASLIB_ENUM_VAL( STAT_MESSAGE_OTHER ),
	ASLIB_ENUM_VAL( STAT_MESSAGE_ALPHA ),
	ASLIB_ENUM_VAL( STAT_MESSAGE_BETA ),

	ASLIB_ENUM_VAL_NULL
};

static const asEnumVal_t asTeamEnumVals[] =
{
	ASLIB_ENUM_VAL( TEAM_SPECTATOR ),
	ASLIB_ENUM_VAL( TEAM_PLAYERS ),
	ASLIB_ENUM_VAL( TEAM_ALPHA ),
	ASLIB_ENUM_VAL( TEAM_BETA ),
	ASLIB_ENUM_VAL( GS_MAX_TEAMS ),

	ASLIB_ENUM_VAL_NULL
};

static const asEnumVal_t asEntityTypeEnumVals[] =
{
	ASLIB_ENUM_VAL( ET_GENERIC ),
	ASLIB_ENUM_VAL( ET_PLAYER ),
	ASLIB_ENUM_VAL( ET_CORPSE ),
	ASLIB_ENUM_VAL( ET_BEAM ),
	ASLIB_ENUM_VAL( ET_PORTALSURFACE ),
	ASLIB_ENUM_VAL( ET_PUSH_TRIGGER ),
	ASLIB_ENUM_VAL( ET_GIB ),
	ASLIB_ENUM_VAL( ET_BLASTER ),
	ASLIB_ENUM_VAL( ET_ELECTRO_WEAK ),
	ASLIB_ENUM_VAL( ET_ROCKET ),
	ASLIB_ENUM_VAL( ET_GRENADE ),
	ASLIB_ENUM_VAL( ET_PLASMA ),
	ASLIB_ENUM_VAL( ET_SPRITE ),
	ASLIB_ENUM_VAL( ET_ITEM ),
	ASLIB_ENUM_VAL( ET_LASERBEAM ),
	ASLIB_ENUM_VAL( ET_CURVELASERBEAM ),
	ASLIB_ENUM_VAL( ET_FLAG_BASE ),
	ASLIB_ENUM_VAL( ET_MINIMAP_ICON ),
	ASLIB_ENUM_VAL( ET_DECAL ),
	ASLIB_ENUM_VAL( ET_ITEM_TIMER ),
	ASLIB_ENUM_VAL( ET_PARTICLES ),
	ASLIB_ENUM_VAL( ET_SPAWN_INDICATOR ),

	ASLIB_ENUM_VAL( ET_EVENT ),
	ASLIB_ENUM_VAL( ET_SOUNDEVENT ),

	ASLIB_ENUM_VAL_NULL
};

static const asEnumVal_t asSolidEnumVals[] =
{
	ASLIB_ENUM_VAL( SOLID_NOT ),
	ASLIB_ENUM_VAL( SOLID_TRIGGER ),
	ASLIB_ENUM_VAL( SOLID_YES ),

	ASLIB_ENUM_VAL_NULL
};

static const asEnumVal_t asMovetypeEnumVals[] =
{
	ASLIB_ENUM_VAL( MOVETYPE_NONE ),
	ASLIB_ENUM_VAL( MOVETYPE_PLAYER ),
	ASLIB_ENUM_VAL( MOVETYPE_NOCLIP ),
	ASLIB_ENUM_VAL( MOVETYPE_PUSH ),
	ASLIB_ENUM_VAL( MOVETYPE_STOP ),
	ASLIB_ENUM_VAL( MOVETYPE_FLY ),
	ASLIB_ENUM_VAL( MOVETYPE_TOSS ),
	ASLIB_ENUM_VAL( MOVETYPE_LINEARPROJECTILE ),
	ASLIB_ENUM_VAL( MOVETYPE_BOUNCE ),
	ASLIB_ENUM_VAL( MOVETYPE_BOUNCEGRENADE ),
	ASLIB_ENUM_VAL( MOVETYPE_TOSSSLIDE ),

	ASLIB_ENUM_VAL_NULL
};

static const asEnumVal_t asPMoveFeaturesVals[] =
{
	ASLIB_ENUM_VAL( PMFEAT_CROUCH ),
	ASLIB_ENUM_VAL( PMFEAT_WALK ),
	ASLIB_ENUM_VAL( PMFEAT_JUMP ),
	ASLIB_ENUM_VAL( PMFEAT_DASH ),
	ASLIB_ENUM_VAL( PMFEAT_WALLJUMP ),
	ASLIB_ENUM_VAL( PMFEAT_FWDBUNNY ),
	ASLIB_ENUM_VAL( PMFEAT_AIRCONTROL ),
	ASLIB_ENUM_VAL( PMFEAT_ZOOM ),
	ASLIB_ENUM_VAL( PMFEAT_GHOSTMOVE ),
	ASLIB_ENUM_VAL( PMFEAT_CONTINOUSJUMP ),
	ASLIB_ENUM_VAL( PMFEAT_ITEMPICK ),
	ASLIB_ENUM_VAL( PMFEAT_GUNBLADEAUTOATTACK ),
	ASLIB_ENUM_VAL( PMFEAT_WEAPONSWITCH ),
	ASLIB_ENUM_VAL( PMFEAT_ALL ),
	ASLIB_ENUM_VAL( PMFEAT_DEFAULT ),

	ASLIB_ENUM_VAL_NULL
};

static const asEnumVal_t asItemTypeEnumVals[] =
{
	ASLIB_ENUM_VAL( IT_WEAPON ),
	ASLIB_ENUM_VAL( IT_AMMO ),
	ASLIB_ENUM_VAL( IT_ARMOR ),
	ASLIB_ENUM_VAL( IT_POWERUP ),
	ASLIB_ENUM_VAL( IT_HEALTH ),

	ASLIB_ENUM_VAL_NULL
};

static const asEnumVal_t asInstagibNegItemMaskEnumVals[] =
{
	ASLIB_ENUM_VAL( G_INSTAGIB_NEGATE_ITEMMASK ),

	ASLIB_ENUM_VAL_NULL
};

static const asEnumVal_t asWeaponTagEnumVals[] =
{
	ASLIB_ENUM_VAL( WEAP_NONE ),
	ASLIB_ENUM_VAL( WEAP_GUNBLADE ),
	ASLIB_ENUM_VAL( WEAP_MACHINEGUN ),
	ASLIB_ENUM_VAL( WEAP_RIOTGUN ),
	ASLIB_ENUM_VAL( WEAP_GRENADELAUNCHER ),
	ASLIB_ENUM_VAL( WEAP_ROCKETLAUNCHER ),
	ASLIB_ENUM_VAL( WEAP_PLASMAGUN ),
	ASLIB_ENUM_VAL( WEAP_LASERGUN ),
	ASLIB_ENUM_VAL( WEAP_ELECTROBOLT ),
	ASLIB_ENUM_VAL( WEAP_INSTAGUN ),
	ASLIB_ENUM_VAL( WEAP_TOTAL ),

	ASLIB_ENUM_VAL_NULL
};

static const asEnumVal_t asAmmoTagEnumVals[] =
{
	ASLIB_ENUM_VAL( AMMO_NONE ),
	ASLIB_ENUM_VAL( AMMO_GUNBLADE ),
	ASLIB_ENUM_VAL( AMMO_BULLETS ),
	ASLIB_ENUM_VAL( AMMO_SHELLS ),
	ASLIB_ENUM_VAL( AMMO_GRENADES ),
	ASLIB_ENUM_VAL( AMMO_ROCKETS ),
	ASLIB_ENUM_VAL( AMMO_PLASMA ),
	ASLIB_ENUM_VAL( AMMO_LASERS ),
	ASLIB_ENUM_VAL( AMMO_BOLTS ),
	ASLIB_ENUM_VAL( AMMO_INSTAS ),

	ASLIB_ENUM_VAL( AMMO_WEAK_GUNBLADE ),
	ASLIB_ENUM_VAL( AMMO_WEAK_BULLETS ),
	ASLIB_ENUM_VAL( AMMO_WEAK_SHELLS ),
	ASLIB_ENUM_VAL( AMMO_WEAK_GRENADES ),
	ASLIB_ENUM_VAL( AMMO_WEAK_ROCKETS ),
	ASLIB_ENUM_VAL( AMMO_WEAK_PLASMA ),
	ASLIB_ENUM_VAL( AMMO_WEAK_LASERS ),
	ASLIB_ENUM_VAL( AMMO_WEAK_BOLTS ),
	ASLIB_ENUM_VAL( AMMO_WEAK_INSTAS ),

	ASLIB_ENUM_VAL( AMMO_TOTAL ),

	ASLIB_ENUM_VAL_NULL
};

static const asEnumVal_t asArmorTagEnumVals[] =
{
	ASLIB_ENUM_VAL( ARMOR_NONE ),
	ASLIB_ENUM_VAL( ARMOR_GA ),
	ASLIB_ENUM_VAL( ARMOR_YA ),
	ASLIB_ENUM_VAL( ARMOR_RA ),
	ASLIB_ENUM_VAL( ARMOR_SHARD ),

	ASLIB_ENUM_VAL_NULL
};

static const asEnumVal_t asHealthTagEnumVals[] =
{
	ASLIB_ENUM_VAL( HEALTH_NONE ),
	ASLIB_ENUM_VAL( HEALTH_SMALL ),
	ASLIB_ENUM_VAL( HEALTH_MEDIUM ),
	ASLIB_ENUM_VAL( HEALTH_LARGE ),
	ASLIB_ENUM_VAL( HEALTH_MEGA ),
	ASLIB_ENUM_VAL( HEALTH_ULTRA ),

	ASLIB_ENUM_VAL_NULL
};

static const asEnumVal_t asPowerupTagEnumVals[] =
{
	ASLIB_ENUM_VAL( POWERUP_NONE ),
	ASLIB_ENUM_VAL( POWERUP_QUAD ),
	ASLIB_ENUM_VAL( POWERUP_SHELL ),
	ASLIB_ENUM_VAL( POWERUP_REGEN ),

	ASLIB_ENUM_VAL( POWERUP_TOTAL ),

	ASLIB_ENUM_VAL_NULL
};

static const asEnumVal_t asMiscItemTagEnumVals[] =
{
	ASLIB_ENUM_VAL( AMMO_PACK_WEAK ),
	ASLIB_ENUM_VAL( AMMO_PACK_STRONG ),
	ASLIB_ENUM_VAL( AMMO_PACK ),
	ASLIB_ENUM_VAL_NULL
};

static const asEnumVal_t asClientStateEnumVals[] =
{
	ASLIB_ENUM_VAL( CS_FREE ),
	ASLIB_ENUM_VAL( CS_ZOMBIE ),
	ASLIB_ENUM_VAL( CS_CONNECTING ),
	ASLIB_ENUM_VAL( CS_CONNECTED ),
	ASLIB_ENUM_VAL( CS_SPAWNED ),

	ASLIB_ENUM_VAL_NULL
};

static const asEnumVal_t asSoundChannelEnumVals[] =
{
	ASLIB_ENUM_VAL( CHAN_AUTO ),
	ASLIB_ENUM_VAL( CHAN_PAIN ),
	ASLIB_ENUM_VAL( CHAN_VOICE ),
	ASLIB_ENUM_VAL( CHAN_ITEM ),
	ASLIB_ENUM_VAL( CHAN_BODY ),
	ASLIB_ENUM_VAL( CHAN_MUZZLEFLASH ),
	ASLIB_ENUM_VAL( CHAN_FIXED ),

	ASLIB_ENUM_VAL_NULL
};

static const asEnumVal_t asContentsEnumVals[] =
{
	ASLIB_ENUM_VAL( CONTENTS_SOLID ),
	ASLIB_ENUM_VAL( CONTENTS_LAVA ),
	ASLIB_ENUM_VAL( CONTENTS_SLIME ),
	ASLIB_ENUM_VAL( CONTENTS_WATER ),
	ASLIB_ENUM_VAL( CONTENTS_FOG ),
	ASLIB_ENUM_VAL( CONTENTS_AREAPORTAL ),
	ASLIB_ENUM_VAL( CONTENTS_PLAYERCLIP ),
	ASLIB_ENUM_VAL( CONTENTS_MONSTERCLIP ),
	ASLIB_ENUM_VAL( CONTENTS_TELEPORTER ),
	ASLIB_ENUM_VAL( CONTENTS_JUMPPAD ),
	ASLIB_ENUM_VAL( CONTENTS_CLUSTERPORTAL ),
	ASLIB_ENUM_VAL( CONTENTS_DONOTENTER ),
	ASLIB_ENUM_VAL( CONTENTS_ORIGIN ),
	ASLIB_ENUM_VAL( CONTENTS_BODY ),
	ASLIB_ENUM_VAL( CONTENTS_CORPSE ),
	ASLIB_ENUM_VAL( CONTENTS_DETAIL ),
	ASLIB_ENUM_VAL( CONTENTS_STRUCTURAL ),
	ASLIB_ENUM_VAL( CONTENTS_TRANSLUCENT ),
	ASLIB_ENUM_VAL( CONTENTS_TRIGGER ),
	ASLIB_ENUM_VAL( CONTENTS_NODROP ),
	ASLIB_ENUM_VAL( MASK_ALL ),
	ASLIB_ENUM_VAL( MASK_SOLID ),
	ASLIB_ENUM_VAL( MASK_PLAYERSOLID ),
	ASLIB_ENUM_VAL( MASK_DEADSOLID ),
	ASLIB_ENUM_VAL( MASK_MONSTERSOLID ),
	ASLIB_ENUM_VAL( MASK_WATER ),
	ASLIB_ENUM_VAL( MASK_OPAQUE ),
	ASLIB_ENUM_VAL( MASK_SHOT ),

	ASLIB_ENUM_VAL_NULL
};

static const asEnumVal_t asSurfFlagEnumVals[] =
{
	ASLIB_ENUM_VAL( SURF_NODAMAGE ),
	ASLIB_ENUM_VAL( SURF_SLICK ),
	ASLIB_ENUM_VAL( SURF_SKY ),
	ASLIB_ENUM_VAL( SURF_LADDER ),
	ASLIB_ENUM_VAL( SURF_NOIMPACT ),
	ASLIB_ENUM_VAL( SURF_NOMARKS ),
	ASLIB_ENUM_VAL( SURF_FLESH ),
	ASLIB_ENUM_VAL( SURF_NODRAW ),
	ASLIB_ENUM_VAL( SURF_HINT ),
	ASLIB_ENUM_VAL( SURF_SKIP ),
	ASLIB_ENUM_VAL( SURF_NOLIGHTMAP ),
	ASLIB_ENUM_VAL( SURF_POINTLIGHT ),
	ASLIB_ENUM_VAL( SURF_METALSTEPS ),
	ASLIB_ENUM_VAL( SURF_NOSTEPS ),
	ASLIB_ENUM_VAL( SURF_NONSOLID ),
	ASLIB_ENUM_VAL( SURF_LIGHTFILTER ),
	ASLIB_ENUM_VAL( SURF_ALPHASHADOW ),
	ASLIB_ENUM_VAL( SURF_NODLIGHT ),
	ASLIB_ENUM_VAL( SURF_DUST ),

	ASLIB_ENUM_VAL_NULL
};

static const asEnumVal_t asSVFlagEnumVals[] =
{
	ASLIB_ENUM_VAL( SVF_NOCLIENT ),
	ASLIB_ENUM_VAL( SVF_PORTAL ),
	ASLIB_ENUM_VAL( SVF_TRANSMITORIGIN2 ),
	ASLIB_ENUM_VAL( SVF_SOUNDCULL ),
	ASLIB_ENUM_VAL( SVF_FAKECLIENT ),
	ASLIB_ENUM_VAL( SVF_BROADCAST ),
	ASLIB_ENUM_VAL( SVF_CORPSE ),
	ASLIB_ENUM_VAL( SVF_PROJECTILE ),
	ASLIB_ENUM_VAL( SVF_ONLYTEAM ),
	ASLIB_ENUM_VAL( SVF_FORCEOWNER ),

	ASLIB_ENUM_VAL_NULL
};

static const asEnumVal_t asMeaningsOfDeathEnumVals[] =
{
	ASLIB_ENUM_VAL( MOD_GUNBLADE_W ),
	ASLIB_ENUM_VAL( MOD_GUNBLADE_S ),
	ASLIB_ENUM_VAL( MOD_MACHINEGUN_W ),
	ASLIB_ENUM_VAL( MOD_MACHINEGUN_S ),
	ASLIB_ENUM_VAL( MOD_RIOTGUN_W ),
	ASLIB_ENUM_VAL( MOD_RIOTGUN_S ),
	ASLIB_ENUM_VAL( MOD_GRENADE_W ),
	ASLIB_ENUM_VAL( MOD_GRENADE_S ),
	ASLIB_ENUM_VAL( MOD_ROCKET_W ),
	ASLIB_ENUM_VAL( MOD_ROCKET_S ),
	ASLIB_ENUM_VAL( MOD_PLASMA_W ),
	ASLIB_ENUM_VAL( MOD_PLASMA_S ),
	ASLIB_ENUM_VAL( MOD_ELECTROBOLT_W ),
	ASLIB_ENUM_VAL( MOD_ELECTROBOLT_S ),
	ASLIB_ENUM_VAL( MOD_INSTAGUN_W ),
	ASLIB_ENUM_VAL( MOD_INSTAGUN_S ),
	ASLIB_ENUM_VAL( MOD_LASERGUN_W ),
	ASLIB_ENUM_VAL( MOD_LASERGUN_S ),
	ASLIB_ENUM_VAL( MOD_GRENADE_SPLASH_W ),
	ASLIB_ENUM_VAL( MOD_GRENADE_SPLASH_S ),
	ASLIB_ENUM_VAL( MOD_ROCKET_SPLASH_W ),
	ASLIB_ENUM_VAL( MOD_ROCKET_SPLASH_S ),
	ASLIB_ENUM_VAL( MOD_PLASMA_SPLASH_W ),
	ASLIB_ENUM_VAL( MOD_PLASMA_SPLASH_S ),

	// World damage
	ASLIB_ENUM_VAL( MOD_WATER ),
	ASLIB_ENUM_VAL( MOD_SLIME ),
	ASLIB_ENUM_VAL( MOD_LAVA ),
	ASLIB_ENUM_VAL( MOD_CRUSH ),
	ASLIB_ENUM_VAL( MOD_TELEFRAG ),
	ASLIB_ENUM_VAL( MOD_FALLING ),
	ASLIB_ENUM_VAL( MOD_SUICIDE ),
	ASLIB_ENUM_VAL( MOD_EXPLOSIVE ),

	// probably not used
	ASLIB_ENUM_VAL( MOD_BARREL ),
	ASLIB_ENUM_VAL( MOD_BOMB ),
	ASLIB_ENUM_VAL( MOD_EXIT ),
	ASLIB_ENUM_VAL( MOD_SPLASH ),
	ASLIB_ENUM_VAL( MOD_TARGET_LASER ),
	ASLIB_ENUM_VAL( MOD_TRIGGER_HURT ),
	ASLIB_ENUM_VAL( MOD_HIT ),

	ASLIB_ENUM_VAL_NULL
};

static const asEnumVal_t asDamageEnumVals[] =
{
	ASLIB_ENUM_VAL( DAMAGE_NO ),
	ASLIB_ENUM_VAL( DAMAGE_YES ),
	ASLIB_ENUM_VAL( DAMAGE_AIM ),

	ASLIB_ENUM_VAL_NULL
};

static const asEnumVal_t asMiscelaneaEnumVals[] =
{
	ASLIB_ENUM_VAL_NULL
};

//=======================================================================

static const asEnum_t asEnums[] =
{
	{ "configstrings_e", asConfigstringEnumVals },
	{ "state_effects_e", asEffectEnumVals },
	{ "matchstates_e", asMatchStateEnumVals },
	{ "spawnsystem_e", asSpawnSystemEnumVals },
	{ "hudstats_e", asHUDStatEnumVals },
	{ "teams_e", asTeamEnumVals },
	{ "entitytype_e", asEntityTypeEnumVals },
	{ "solid_e", asSolidEnumVals },
	{ "movetype_e", asMovetypeEnumVals },
	{ "pmovefeats_e", asPMoveFeaturesVals },
	{ "itemtype_e", asItemTypeEnumVals },

	// we can't register defines, so we create a enum for each one of them :/
	{ "G_INSTAGIB_NEGATE_ITEMMASK_e", asInstagibNegItemMaskEnumVals },
	{ "weapon_tag_e", asWeaponTagEnumVals },
	{ "ammo_tag_e", asAmmoTagEnumVals },
	{ "armor_tag_e", asArmorTagEnumVals },
	{ "health_tag_e", asHealthTagEnumVals },
	{ "powerup_tag_e", asPowerupTagEnumVals },
	{ "otheritems_tag_e", asMiscItemTagEnumVals },

	{ "client_statest_e", asClientStateEnumVals },
	{ "sound_channels_e", asSoundChannelEnumVals },
	{ "contents_e", asContentsEnumVals },
	{ "surfaceflags_e", asSurfFlagEnumVals },
	{ "serverflags_e", asSVFlagEnumVals },
	{ "meaningsofdeath_e", asMeaningsOfDeathEnumVals },
	{ "takedamage_e", asDamageEnumVals },
	{ "miscelanea_e", asMiscelaneaEnumVals },

	ASLIB_ENUM_VAL_NULL
};

/*
* G_asRegisterEnums
*/
static void G_asRegisterEnums( int asEngineHandle )
{
	int i, j;
	const asEnum_t *asEnum;
	const asEnumVal_t *asEnumVal;

	for( i = 0, asEnum = asEnums; asEnum->name != NULL; i++, asEnum++ )
	{
		angelExport->asRegisterEnum( asEngineHandle, asEnum->name );

		for( j = 0, asEnumVal = asEnum->values; asEnumVal->name != NULL; j++, asEnumVal++ )
			angelExport->asRegisterEnumValue( asEngineHandle, asEnum->name, asEnumVal->name, asEnumVal->value );
	}
}

//=======================================================================

// CLASS: cTrace
typedef struct  
{
	trace_t trace;
} astrace_t;

void objectTrace_DefaultConstructor( astrace_t *self )
{
	memset( &self->trace, 0, sizeof( trace_t ) );
}

void objectTrace_CopyConstructor( astrace_t *other, astrace_t *self )
{
	self->trace = other->trace;
}

static qboolean objectTrace_doTrace( asvec3_t *start, asvec3_t *mins, asvec3_t *maxs, asvec3_t *end, int ignore, int contentMask, astrace_t *self )
{
	edict_t *passEnt = NULL;

	if( ignore > 0 && ignore < game.maxentities )
		passEnt = &game.edicts[ ignore ];

	if( !start || !end ) // should never happen unless the coder explicitly feeds null
	{
		G_Printf( "* WARNING: gametype plug-in script attempted to call method 'trace.doTrace' with a null vector pointer\n* Tracing skept" );
		return qfalse;
	}

	G_Trace( &self->trace, start->v, mins ? mins->v : vec3_origin, maxs ? maxs->v : vec3_origin, end->v, passEnt, contentMask );

	if( self->trace.startsolid || self->trace.allsolid )
		return qtrue;

	return ( self->trace.ent != -1 );
}

static asvec3_t objectTrace_getEndPos( astrace_t *self )
{
	asvec3_t asvec;

	VectorCopy( self->trace.endpos, asvec.v );
	return asvec;
}

static asvec3_t objectTrace_getPlaneNormal( astrace_t *self )
{
	asvec3_t asvec;

	VectorCopy( self->trace.plane.normal, asvec.v );
	return asvec;
}

static const asFuncdef_t astrace_Funcdefs[] =
{
	ASLIB_FUNCDEF_NULL
};

static const asBehavior_t astrace_ObjectBehaviors[] =
{
	{ asBEHAVE_CONSTRUCT, ASLIB_FUNCTION_DECL(void, f, ()), objectTrace_DefaultConstructor, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_CONSTRUCT, ASLIB_FUNCTION_DECL(void, f, (const cTrace &in)), objectTrace_CopyConstructor, asCALL_CDECL_OBJLAST },

	ASLIB_BEHAVIOR_NULL
};

static const asMethod_t astrace_Methods[] =
{
	{ ASLIB_FUNCTION_DECL(bool, doTrace, ( const Vec3 &in, const Vec3 &in, const Vec3 &in, const Vec3 &in, int ignore, int contentMask ) const), objectTrace_doTrace, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(Vec3, get_endPos, () const), objectTrace_getEndPos,asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(Vec3, get_planeNormal, () const), objectTrace_getPlaneNormal, asCALL_CDECL_OBJLAST },

	ASLIB_METHOD_NULL
};

static const asProperty_t astrace_Properties[] =
{
	{ ASLIB_PROPERTY_DECL(const bool, allSolid), ASLIB_FOFFSET(astrace_t, trace.allsolid) },
	{ ASLIB_PROPERTY_DECL(const bool, startSolid), ASLIB_FOFFSET(astrace_t, trace.startsolid) },
	{ ASLIB_PROPERTY_DECL(const float, fraction), ASLIB_FOFFSET(astrace_t, trace.fraction) },
	{ ASLIB_PROPERTY_DECL(const int, surfFlags), ASLIB_FOFFSET(astrace_t, trace.surfFlags) },
	{ ASLIB_PROPERTY_DECL(const int, contents), ASLIB_FOFFSET(astrace_t, trace.contents) },
	{ ASLIB_PROPERTY_DECL(const int, entNum), ASLIB_FOFFSET(astrace_t, trace.ent) },
	{ ASLIB_PROPERTY_DECL(const float, planeDist), ASLIB_FOFFSET(astrace_t, trace.plane.dist) },
	{ ASLIB_PROPERTY_DECL(const int16, planeType), ASLIB_FOFFSET(astrace_t, trace.plane.type) },
	{ ASLIB_PROPERTY_DECL(const int16, planeSignBits), ASLIB_FOFFSET(astrace_t, trace.plane.signbits) },

	ASLIB_PROPERTY_NULL
};

static const asClassDescriptor_t asTraceClassDescriptor =
{
	"cTrace",					/* name */
	asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CK,	/* object type flags */
	sizeof( astrace_t ),		/* size */
	astrace_Funcdefs,		/* funcdefs */
	astrace_ObjectBehaviors,	/* object behaviors */
	astrace_Methods,			/* methods */
	astrace_Properties,			/* properties */

	NULL, NULL					/* string factory hack */
};

//=======================================================================

// CLASS: cItem 
static int gsitem_factored_count = 0;
static int gsitem_released_count = 0;

static gsitem_t *objectGItem_Factory()
{
	static gsitem_t *object;

	object = G_AsMalloc( sizeof( gsitem_t ) );
	object->asRefCount = 1;
	object->asFactored = 1;
	gsitem_factored_count++;
	return object;
}

static void objectGItem_Addref( gsitem_t *obj ) { obj->asRefCount++; }

static void objectGItem_Release( gsitem_t *obj ) 
{
	obj->asRefCount--; 
	clamp_low( obj->asRefCount, 0 );
	if( !obj->asRefCount && obj->asFactored )
	{
		G_AsFree( obj );
		gsitem_released_count++;
	}
}

static asstring_t *objectGItem_getClassName( gsitem_t *self )
{
	return angelExport->asStringFactoryBuffer( self->classname, self->classname ? strlen(self->classname) : 0 );
}

static asstring_t *objectGItem_getName( gsitem_t *self )
{
	return angelExport->asStringFactoryBuffer( self->name, self->name ? strlen(self->name) : 0 );
}

static asstring_t *objectGItem_getShortName( gsitem_t *self )
{
	return angelExport->asStringFactoryBuffer( self->shortname, self->shortname ? strlen(self->shortname) : 0 );
}

static asstring_t *objectGItem_getModelName( gsitem_t *self )
{
	return angelExport->asStringFactoryBuffer( self->world_model[0], self->world_model[0] ? strlen(self->world_model[0]) : 0 );
}

static asstring_t *objectGItem_getModel2Name( gsitem_t *self )
{
	return angelExport->asStringFactoryBuffer( self->world_model[1], self->world_model[1] ? strlen(self->world_model[1]) : 0 );
}

static asstring_t *objectGItem_getIconName( gsitem_t *self )
{
	return angelExport->asStringFactoryBuffer( self->icon, self->icon ? strlen(self->icon) : 0 );
}

static asstring_t *objectGItem_getSimpleItemName( gsitem_t *self )
{
	return angelExport->asStringFactoryBuffer( self->simpleitem, self->simpleitem ? strlen(self->simpleitem) : 0 );
}

static asstring_t *objectGItem_getPickupSoundName( gsitem_t *self )
{
	return angelExport->asStringFactoryBuffer( self->pickup_sound, self->pickup_sound ? strlen(self->pickup_sound) : 0 );
}

static asstring_t *objectGItem_getColorToken( gsitem_t *self )
{
	return angelExport->asStringFactoryBuffer( self->color, self->color ? strlen(self->color) : 0 );
}

static qboolean objectGItem_isPickable( gsitem_t *self )
{
	return ( self && ( self->flags & ITFLAG_PICKABLE ) ) ? qtrue : qfalse;
}

static qboolean objectGItem_isUsable( gsitem_t *self )
{
	return ( self && ( self->flags & ITFLAG_USABLE ) ) ? qtrue : qfalse;
}

static qboolean objectGItem_isDropable( gsitem_t *self )
{
	return ( self && ( self->flags & ITFLAG_DROPABLE ) ) ? qtrue : qfalse;
}

static const asFuncdef_t asitem_Funcdefs[] =
{
	ASLIB_FUNCDEF_NULL
};

static const asBehavior_t asitem_ObjectBehaviors[] =
{
	{ asBEHAVE_FACTORY, ASLIB_FUNCTION_DECL(cItem@, f, ()), objectGItem_Factory, asCALL_CDECL },
	{ asBEHAVE_ADDREF, ASLIB_FUNCTION_DECL(void, f, ()), objectGItem_Addref, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_RELEASE, ASLIB_FUNCTION_DECL(void, f, ()), objectGItem_Release, asCALL_CDECL_OBJLAST },

	ASLIB_BEHAVIOR_NULL
};

static const asMethod_t asitem_Methods[] =
{
	{ ASLIB_FUNCTION_DECL(String @, get_classname, () const), objectGItem_getClassName, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(String @, get_name, () const), objectGItem_getName, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(String @, get_shortName, () const), objectGItem_getShortName, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(String @, get_model, () const), objectGItem_getModelName, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(String @, get_model2, () const), objectGItem_getModel2Name, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(String @, get_icon, () const), objectGItem_getIconName, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(String @, get_simpleIcon, () const), objectGItem_getSimpleItemName, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(String @, get_pickupSound, () const), objectGItem_getPickupSoundName, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(String @, get_colorToken, () const), objectGItem_getColorToken, asCALL_CDECL_OBJLAST },

	{ ASLIB_FUNCTION_DECL(bool, isPickable, () const), objectGItem_isPickable, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(bool, isUsable, () const), objectGItem_isUsable, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(bool, isDropable, () const), objectGItem_isDropable, asCALL_CDECL_OBJLAST },

	ASLIB_METHOD_NULL
};

static const asProperty_t asitem_Properties[] =
{
	{ ASLIB_PROPERTY_DECL(const int, tag), ASLIB_FOFFSET(gsitem_t, tag) },
	{ ASLIB_PROPERTY_DECL(const uint, type), ASLIB_FOFFSET(gsitem_t, type) },
	{ ASLIB_PROPERTY_DECL(const int, flags), ASLIB_FOFFSET(gsitem_t, flags) },
	{ ASLIB_PROPERTY_DECL(const int, quantity), ASLIB_FOFFSET(gsitem_t, quantity) },
	{ ASLIB_PROPERTY_DECL(const int, inventoryMax), ASLIB_FOFFSET(gsitem_t, inventory_max) },
	{ ASLIB_PROPERTY_DECL(const int, ammoTag), ASLIB_FOFFSET(gsitem_t, ammo_tag) },
	{ ASLIB_PROPERTY_DECL(const int, weakAmmoTag), ASLIB_FOFFSET(gsitem_t, weakammo_tag) },

	ASLIB_PROPERTY_NULL
};

static const asClassDescriptor_t asItemClassDescriptor =
{
	"cItem",					/* name */
	asOBJ_REF,					/* object type flags */
	sizeof( gsitem_t ),			/* size */
	asitem_Funcdefs,		/* funcdefs */
	asitem_ObjectBehaviors,		/* object behaviors */
	asitem_Methods,				/* methods */
	asitem_Properties,			/* properties */

	NULL, NULL					/* string factory hack */
};

//=======================================================================

// CLASS: cMatch

static void objectMatch_launchState( int state, match_t *self )
{
	if( state >= MATCH_STATE_NONE && state < MATCH_STATE_TOTAL )
		G_Match_LaunchState( state );
}

static void objectMatch_startAutorecord( match_t *self )
{
	G_Match_Autorecord_Start();
}

static void objectMatch_stopAutorecord( match_t *self )
{
	G_Match_Autorecord_Stop();
}

static qboolean objectMatch_scoreLimitHit( match_t *self )
{
	return G_Match_ScorelimitHit();
}

static qboolean objectMatch_timeLimitHit( match_t *self )
{
	return G_Match_TimelimitHit();
}

static qboolean objectMatch_isTied( match_t *self )
{
	return G_Match_Tied();
}

static qboolean objectMatch_checkExtendPlayTime( match_t *self )
{
	return G_Match_CheckExtendPlayTime();
}

static qboolean objectMatch_suddenDeathFinished( match_t *self )
{
	return G_Match_SuddenDeathFinished();
}

static qboolean objectMatch_isPaused( match_t *self )
{
	return GS_MatchPaused();
}

static qboolean objectMatch_isWaiting( match_t *self )
{
	return GS_MatchWaiting();
}

static qboolean objectMatch_isExtended( match_t *self )
{
	return GS_MatchExtended();
}

static unsigned int objectMatch_duration( match_t *self )
{
	return GS_MatchDuration();
}

static unsigned int objectMatch_startTime( match_t *self )
{
	return GS_MatchStartTime();
}

static unsigned int objectMatch_endTime( match_t *self )
{
	return GS_MatchStartTime();
}

static int objectMatch_getState( match_t *self )
{
	return GS_MatchState();
}

static asstring_t *objectMatch_getName( match_t *self )
{
	const char *s = trap_GetConfigString( CS_MATCHNAME );

	return angelExport->asStringFactoryBuffer( s, strlen( s ) );
}

static asstring_t *objectMatch_getScore( match_t *self )
{
	const char *s = trap_GetConfigString( CS_MATCHSCORE );

	return angelExport->asStringFactoryBuffer( s, strlen( s ) );
}

static void objectMatch_setName( asstring_t *name, match_t *self )
{
	char buf[MAX_CONFIGSTRING_CHARS];

	COM_SanitizeColorString( name->buffer, buf, sizeof( buf ), -1, COLOR_WHITE );

	trap_ConfigString( CS_MATCHNAME, buf );
}

static void objectMatch_setScore( asstring_t *name, match_t *self )
{
	char buf[MAX_CONFIGSTRING_CHARS];

	COM_SanitizeColorString( name->buffer, buf, sizeof( buf ), -1, COLOR_WHITE );

	trap_ConfigString( CS_MATCHSCORE, buf );
}

static void objectMatch_setClockOverride( unsigned int time, match_t *self )
{
	gs.gameState.longstats[GAMELONG_CLOCKOVERRIDE] = time;
}

static const asFuncdef_t match_Funcdefs[] =
{
	ASLIB_FUNCDEF_NULL
};

static const asBehavior_t match_ObjectBehaviors[] =
{
	ASLIB_BEHAVIOR_NULL
};

static const asMethod_t match_Methods[] =
{
	{ ASLIB_FUNCTION_DECL(void, launchState, (int state) const), objectMatch_launchState, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, startAutorecord, () const), objectMatch_startAutorecord, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, stopAutorecord, () const), objectMatch_stopAutorecord, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(bool, scoreLimitHit, () const), objectMatch_scoreLimitHit, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(bool, timeLimitHit, () const), objectMatch_timeLimitHit, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(bool, isTied, () const), objectMatch_isTied, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(bool, checkExtendPlayTime, () const), objectMatch_checkExtendPlayTime, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(bool, suddenDeathFinished, () const), objectMatch_suddenDeathFinished, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(bool, isPaused, () const), objectMatch_isPaused, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(bool, isWaiting, () const), objectMatch_isWaiting, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(bool, isExtended, () const), objectMatch_isExtended, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(uint, duration, () const), objectMatch_duration, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(uint, startTime, () const), objectMatch_startTime, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(uint, endTime, () const), objectMatch_endTime, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(int, getState, () const), objectMatch_getState, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(String @, get_name, () const), objectMatch_getName, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(String @, getScore, () const), objectMatch_getScore,  asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, set_name, ( String &in )), objectMatch_setName, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, setScore, ( String &in )), objectMatch_setScore, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, setClockOverride, ( uint milliseconds )), objectMatch_setClockOverride, asCALL_CDECL_OBJLAST },

	ASLIB_METHOD_NULL
};

static const asProperty_t match_Properties[] =
{
	ASLIB_PROPERTY_NULL
};

static const asClassDescriptor_t asMatchClassDescriptor =
{
	"cMatch",					/* name */
	asOBJ_REF|asOBJ_NOHANDLE,	/* object type flags */
	sizeof( match_t ),			/* size */
	match_Funcdefs,		/* funcdefs */
	match_ObjectBehaviors,		/* object behaviors */
	match_Methods,				/* methods */
	match_Properties,			/* properties */

	NULL, NULL					/* string factory hack */
};

//=======================================================================

// CLASS: cGametypeDesc

static asstring_t *objectGametypeDescriptor_getTitle( gametype_descriptor_t *self )
{
	const char *s = trap_GetConfigString( CS_GAMETYPETITLE );

	return angelExport->asStringFactoryBuffer( s, strlen( s ) );
}

static void objectGametypeDescriptor_setTitle( asstring_t *other, gametype_descriptor_t *self )
{
	if( !other || !other->buffer )
		return;

	trap_ConfigString( CS_GAMETYPETITLE, other->buffer );
}

static asstring_t *objectGametypeDescriptor_getName( gametype_descriptor_t *self )
{
	return angelExport->asStringFactoryBuffer( gs.gametypeName, strlen(gs.gametypeName) );
}

static asstring_t *objectGametypeDescriptor_getVersion( gametype_descriptor_t *self )
{
	const char *s = trap_GetConfigString( CS_GAMETYPEVERSION );

	return angelExport->asStringFactoryBuffer( s, strlen( s ) );
}

static void objectGametypeDescriptor_setVersion( asstring_t *other, gametype_descriptor_t *self )
{
	if( !other || !other->buffer )
		return;

	trap_ConfigString( CS_GAMETYPEVERSION, other->buffer );
}

static asstring_t *objectGametypeDescriptor_getAuthor( gametype_descriptor_t *self )
{
	const char *s = trap_GetConfigString( CS_GAMETYPEAUTHOR );

	return angelExport->asStringFactoryBuffer( s, strlen( s ) );
}

static void objectGametypeDescriptor_setAuthor( asstring_t *other, gametype_descriptor_t *self )
{
	if( !other || !other->buffer )
		return;

	trap_ConfigString( CS_GAMETYPEAUTHOR, other->buffer );
}

static asstring_t *objectGametypeDescriptor_getManifest( gametype_descriptor_t *self )
{
	const char *s = trap_GetConfigString( CS_MODMANIFEST );

	return angelExport->asStringFactoryBuffer( s, strlen( s ) );
}

static void objectGametypeDescriptor_SetTeamSpawnsystem( int team, int spawnsystem, int wave_time, int wave_maxcount, qboolean spectate_team, gametype_descriptor_t *self )
{
	G_SpawnQueue_SetTeamSpawnsystem( team, spawnsystem, wave_time, wave_maxcount, spectate_team );
}

static qboolean objectGametypeDescriptor_isInstagib( gametype_descriptor_t *self )
{
	return GS_Instagib();
}

static qboolean objectGametypeDescriptor_hasFallDamage( gametype_descriptor_t *self )
{
	return GS_FallDamage();
}

static qboolean objectGametypeDescriptor_hasSelfDamage( gametype_descriptor_t *self )
{
	return GS_SelfDamage();
}

static qboolean objectGametypeDescriptor_isInvidualGameType( gametype_descriptor_t *self )
{
	return GS_InvidualGameType();
}

static const asFuncdef_t gametypedescr_Funcdefs[] =
{
	ASLIB_FUNCDEF_NULL
};

static const asBehavior_t gametypedescr_ObjectBehaviors[] =
{
	ASLIB_BEHAVIOR_NULL
};

static const asMethod_t gametypedescr_Methods[] =
{
	{ ASLIB_FUNCTION_DECL(String @, get_name, () const), objectGametypeDescriptor_getName, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(String @, get_title, () const), objectGametypeDescriptor_getTitle, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, set_title, ( String & )), objectGametypeDescriptor_setTitle, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(String @, get_version, () const), objectGametypeDescriptor_getVersion, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, set_version, ( String & )), objectGametypeDescriptor_setVersion, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(String @, get_author, () const), objectGametypeDescriptor_getAuthor, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, set_author, ( String & )), objectGametypeDescriptor_setAuthor, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(String @, get_manifest, () const), objectGametypeDescriptor_getManifest, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, setTeamSpawnsystem, ( int team, int spawnsystem, int wave_time, int wave_maxcount, bool deadcam )), objectGametypeDescriptor_SetTeamSpawnsystem, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(bool, get_isInstagib, () const), objectGametypeDescriptor_isInstagib, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(bool, get_hasFallDamage, () const), objectGametypeDescriptor_hasFallDamage, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(bool, get_hasSelfDamage, () const), objectGametypeDescriptor_hasSelfDamage, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(bool, get_isInvidualGameType, () const), objectGametypeDescriptor_isInvidualGameType, asCALL_CDECL_OBJLAST },

	ASLIB_METHOD_NULL
};

static const asProperty_t gametypedescr_Properties[] =
{
	{ ASLIB_PROPERTY_DECL(uint, spawnableItemsMask), ASLIB_FOFFSET(gametype_descriptor_t, spawnableItemsMask) },
	{ ASLIB_PROPERTY_DECL(uint, respawnableItemsMask), ASLIB_FOFFSET(gametype_descriptor_t, respawnableItemsMask) },
	{ ASLIB_PROPERTY_DECL(uint, dropableItemsMask), ASLIB_FOFFSET(gametype_descriptor_t, dropableItemsMask) },
	{ ASLIB_PROPERTY_DECL(uint, pickableItemsMask), ASLIB_FOFFSET(gametype_descriptor_t, pickableItemsMask) },
	{ ASLIB_PROPERTY_DECL(bool, isTeamBased), ASLIB_FOFFSET(gametype_descriptor_t, isTeamBased) },
	{ ASLIB_PROPERTY_DECL(bool, isRace), ASLIB_FOFFSET(gametype_descriptor_t, isRace) },
	{ ASLIB_PROPERTY_DECL(bool, hasChallengersQueue), ASLIB_FOFFSET(gametype_descriptor_t, hasChallengersQueue) },
	{ ASLIB_PROPERTY_DECL(int, maxPlayersPerTeam), ASLIB_FOFFSET(gametype_descriptor_t, maxPlayersPerTeam) },
	{ ASLIB_PROPERTY_DECL(int, ammoRespawn), ASLIB_FOFFSET(gametype_descriptor_t, ammo_respawn) },
	{ ASLIB_PROPERTY_DECL(int, armorRespawn), ASLIB_FOFFSET(gametype_descriptor_t, armor_respawn) },
	{ ASLIB_PROPERTY_DECL(int, weaponRespawn), ASLIB_FOFFSET(gametype_descriptor_t, weapon_respawn) },
	{ ASLIB_PROPERTY_DECL(int, healthRespawn), ASLIB_FOFFSET(gametype_descriptor_t, health_respawn) },
	{ ASLIB_PROPERTY_DECL(int, powerupRespawn), ASLIB_FOFFSET(gametype_descriptor_t, powerup_respawn) },
	{ ASLIB_PROPERTY_DECL(int, megahealthRespawn), ASLIB_FOFFSET(gametype_descriptor_t, megahealth_respawn) },
	{ ASLIB_PROPERTY_DECL(int, ultrahealthRespawn), ASLIB_FOFFSET(gametype_descriptor_t, ultrahealth_respawn) },
	{ ASLIB_PROPERTY_DECL(bool, readyAnnouncementEnabled), ASLIB_FOFFSET(gametype_descriptor_t, readyAnnouncementEnabled) },
	{ ASLIB_PROPERTY_DECL(bool, scoreAnnouncementEnabled), ASLIB_FOFFSET(gametype_descriptor_t, scoreAnnouncementEnabled) },
	{ ASLIB_PROPERTY_DECL(bool, countdownEnabled), ASLIB_FOFFSET(gametype_descriptor_t, countdownEnabled) },
	{ ASLIB_PROPERTY_DECL(bool, mathAbortDisabled), ASLIB_FOFFSET(gametype_descriptor_t, mathAbortDisabled) },
	{ ASLIB_PROPERTY_DECL(bool, shootingDisabled), ASLIB_FOFFSET(gametype_descriptor_t, shootingDisabled) },
	{ ASLIB_PROPERTY_DECL(bool, infiniteAmmo), ASLIB_FOFFSET(gametype_descriptor_t, infiniteAmmo) },
	{ ASLIB_PROPERTY_DECL(bool, canForceModels), ASLIB_FOFFSET(gametype_descriptor_t, canForceModels) },
	{ ASLIB_PROPERTY_DECL(bool, canShowMinimap), ASLIB_FOFFSET(gametype_descriptor_t, canShowMinimap) },
	{ ASLIB_PROPERTY_DECL(bool, teamOnlyMinimap), ASLIB_FOFFSET(gametype_descriptor_t, teamOnlyMinimap) },
	{ ASLIB_PROPERTY_DECL(int, spawnpointRadius), ASLIB_FOFFSET(gametype_descriptor_t, spawnpoint_radius) },
	{ ASLIB_PROPERTY_DECL(bool, customDeadBodyCam), ASLIB_FOFFSET(gametype_descriptor_t, customDeadBodyCam) },
	{ ASLIB_PROPERTY_DECL(bool, mmCompatible), ASLIB_FOFFSET(gametype_descriptor_t, mmCompatible ) },

	//racesow
	{ ASLIB_PROPERTY_DECL(bool, autoInactivityRemove), ASLIB_FOFFSET(gametype_descriptor_t, autoInactivityRemove) },
	{ ASLIB_PROPERTY_DECL(bool, playerInteraction), ASLIB_FOFFSET(gametype_descriptor_t, playerInteraction) },
	{ ASLIB_PROPERTY_DECL(bool, freestyleMapFix), ASLIB_FOFFSET(gametype_descriptor_t, freestyleMapFix) },
	{ ASLIB_PROPERTY_DECL(bool, enableDrowning), ASLIB_FOFFSET(gametype_descriptor_t, enableDrowning) },
	//!racesow

	ASLIB_PROPERTY_NULL
};

static const asClassDescriptor_t asGametypeClassDescriptor =
{
	"cGametypeDesc",				/* name */
	asOBJ_REF|asOBJ_NOHANDLE,		/* object type flags */
	sizeof( gametype_descriptor_t ), /* size */
	gametypedescr_Funcdefs,	/* funcdefs */
	gametypedescr_ObjectBehaviors,	/* object behaviors */
	gametypedescr_Methods,			/* methods */
	gametypedescr_Properties,		/* properties */

	NULL, NULL						/* string factory hack */
};

//=======================================================================


// CLASS: cTeam
static int g_teamlist_stats_factored_count = 0;
static int g_teamlist_stats_released_count = 0;

static g_teamlist_t *objectTeamlist_Factory()
{
	g_teamlist_t *object;

	object = G_AsMalloc( sizeof( g_teamlist_t ) );
	object->asRefCount = 1;
	object->asFactored = 1;
	g_teamlist_stats_factored_count++;
	return object;
}

static void objectTeamlist_Addref( g_teamlist_t *obj ) { obj->asRefCount++; }

static void objectTeamlist_Release( g_teamlist_t *obj ) 
{
	obj->asRefCount--;
	clamp_low( obj->asRefCount, 0 );
	if( !obj->asRefCount && obj->asFactored )
	{
		G_AsFree( obj );
		g_teamlist_stats_released_count++;
	}
}

static edict_t *objectTeamlist_GetPlayerEntity( int index, g_teamlist_t *obj ) 
{
	if( index < 0 || index >= obj->numplayers )
		return NULL;

	if( obj->playerIndices[index] < 1 || obj->playerIndices[index] > gs.maxclients )
		return NULL;

	return &game.edicts[ obj->playerIndices[index] ];
}

static asstring_t *objectTeamlist_getName( g_teamlist_t *obj ) 
{
	const char *name = GS_TeamName( obj - teamlist );

	return angelExport->asStringFactoryBuffer( name, name ? strlen( name ) : 0 );
}

static asstring_t *objectTeamlist_getDefaultName( g_teamlist_t *obj ) 
{
	const char *name = GS_DefaultTeamName( obj - teamlist );

	return angelExport->asStringFactoryBuffer( name, name ? strlen( name ) : 0 );
}

static void objectTeamlist_setName( asstring_t *str, g_teamlist_t *obj )
{
	int team;
	char buf[MAX_CONFIGSTRING_CHARS];

	team = obj - teamlist;
	if( team < TEAM_ALPHA || team > TEAM_BETA )
		return;

	COM_SanitizeColorString( str->buffer, buf, sizeof( buf ), -1, COLOR_WHITE );

	trap_ConfigString( CS_TEAM_ALPHA_NAME + team - TEAM_ALPHA, buf );
}

static qboolean objectTeamlist_IsLocked( g_teamlist_t *obj ) 
{
	return G_Teams_TeamIsLocked( obj - teamlist );
}

static qboolean objectTeamlist_Lock( g_teamlist_t *obj ) 
{
	return ( obj ? G_Teams_LockTeam( obj - teamlist ) : qfalse );
}

static qboolean objectTeamlist_Unlock( g_teamlist_t *obj ) 
{
	return ( obj ? G_Teams_UnLockTeam( obj - teamlist ) : qfalse );
}

static void objectTeamlist_ClearInvites( g_teamlist_t *obj ) 
{
	obj->invited[0] = 0;
}

static int objectTeamlist_getTeamIndex( g_teamlist_t *obj )
{
	int index = ( obj - teamlist );

	if( index < 0 || index >= GS_MAX_TEAMS )
		return -1;

	return index;
}

static const asFuncdef_t teamlist_Funcdefs[] =
{
	ASLIB_FUNCDEF_NULL
};

static const asBehavior_t teamlist_ObjectBehaviors[] =
{
	{ asBEHAVE_FACTORY, ASLIB_FUNCTION_DECL(cTeam @, f, ()), objectTeamlist_Factory, asCALL_CDECL },
	{ asBEHAVE_ADDREF, ASLIB_FUNCTION_DECL(void, f, ()), objectTeamlist_Addref, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_RELEASE, ASLIB_FUNCTION_DECL(void, f, ()), objectTeamlist_Release, asCALL_CDECL_OBJLAST },

	ASLIB_BEHAVIOR_NULL
};

static const asMethod_t teamlist_Methods[] =
{
	{ ASLIB_FUNCTION_DECL(cEntity @, ent, ( int index )), objectTeamlist_GetPlayerEntity, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(String @, get_name, () const), objectTeamlist_getName, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(String @, get_defaultName, () const), objectTeamlist_getDefaultName, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, set_name, ( String &in )), objectTeamlist_setName, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(bool, isLocked, () const), objectTeamlist_IsLocked, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(bool, lock, () const), objectTeamlist_Lock, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(bool, unlock, () const), objectTeamlist_Unlock, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, clearInvites, ()), objectTeamlist_ClearInvites, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(int, team, () const), objectTeamlist_getTeamIndex, asCALL_CDECL_OBJLAST },

	ASLIB_METHOD_NULL
};

static const asProperty_t teamlist_Properties[] =
{
	{ ASLIB_PROPERTY_DECL(cStats, stats), ASLIB_FOFFSET(g_teamlist_t, stats) },
	{ ASLIB_PROPERTY_DECL(const int, numPlayers), ASLIB_FOFFSET(g_teamlist_t, numplayers) },
	{ ASLIB_PROPERTY_DECL(const int, ping), ASLIB_FOFFSET(g_teamlist_t, ping) },
	{ ASLIB_PROPERTY_DECL(const bool, hasCoach), ASLIB_FOFFSET(g_teamlist_t, has_coach) },

	ASLIB_PROPERTY_NULL
};

static const asClassDescriptor_t asTeamListClassDescriptor =
{
	"cTeam",					/* name */
	asOBJ_REF,					/* object type flags */
	sizeof( g_teamlist_t ),		/* size */
	teamlist_Funcdefs,	/* funcdefs */
	teamlist_ObjectBehaviors,	/* object behaviors */
	teamlist_Methods,			/* methods */
	teamlist_Properties,		/* properties */

	NULL, NULL					/* string factory hack */
};

//=======================================================================

// CLASS: cStats
static int client_stats_factored_count = 0;
static int client_stats_released_count = 0;

static score_stats_t *objectScoreStats_Factory()
{
	static score_stats_t *object;

	object = G_AsMalloc( sizeof( score_stats_t ) );
	object->asRefCount = 1;
	object->asFactored = 1;
	client_stats_factored_count++;
	return object;
}

static void objectScoreStats_Addref( score_stats_t *obj ) { obj->asRefCount++; }

static void objectScoreStats_Release( score_stats_t *obj ) 
{
	obj->asRefCount--;
	clamp_low( obj->asRefCount, 0 );
	if( !obj->asRefCount && obj->asFactored )
	{
		G_AsFree( obj );
		client_stats_released_count++;
	}
}

static void objectScoreStats_Clear( score_stats_t *obj ) 
{
	memset( obj, 0, sizeof( *obj ) );
}

static int objectScoreStats_AccShots( int ammo, score_stats_t *obj ) 
{
	if( ammo < AMMO_GUNBLADE || ammo >= AMMO_TOTAL )
		return 0;

	return obj->accuracy_shots[ ammo - AMMO_GUNBLADE ];
}

static int objectScoreStats_AccHits( int ammo, score_stats_t *obj ) 
{
	if( ammo < AMMO_GUNBLADE || ammo >= AMMO_TOTAL )
		return 0;

	return obj->accuracy_hits[ ammo - AMMO_GUNBLADE ];
}

static int objectScoreStats_AccHitsDirect( int ammo, score_stats_t *obj ) 
{
	if( ammo < AMMO_GUNBLADE || ammo >= AMMO_TOTAL )
		return 0;

	return obj->accuracy_hits_direct[ ammo - AMMO_GUNBLADE ];
}

static int objectScoreStats_AccHitsAir( int ammo, score_stats_t *obj ) 
{
	if( ammo < AMMO_GUNBLADE || ammo >= AMMO_TOTAL )
		return 0;

	return obj->accuracy_hits_air[ ammo - AMMO_GUNBLADE ];
}

static int objectScoreStats_AccDamage( int ammo, score_stats_t *obj ) 
{
	if( ammo < AMMO_GUNBLADE || ammo >= AMMO_TOTAL )
		return 0;

	return obj->accuracy_damage[ ammo - AMMO_GUNBLADE ];
}

static void objectScoreStats_ScoreSet( int newscore, score_stats_t *obj ) 
{
	obj->score = newscore;
}

static void objectScoreStats_ScoreAdd( int score, score_stats_t *obj ) 
{
	obj->score += score;
}

static void objectScoreStats_RoundAdd( score_stats_t *obj )
{
	obj->numrounds++;
}

static const asFuncdef_t scorestats_Funcdefs[] =
{
	ASLIB_FUNCDEF_NULL
};

static const asBehavior_t scorestats_ObjectBehaviors[] =
{
	{ asBEHAVE_FACTORY, ASLIB_FUNCTION_DECL(cStats @, f, ()), objectScoreStats_Factory, asCALL_CDECL },
	{ asBEHAVE_ADDREF, ASLIB_FUNCTION_DECL(void, f, ()), objectScoreStats_Addref, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_RELEASE, ASLIB_FUNCTION_DECL(void, f, ()), objectScoreStats_Release, asCALL_CDECL_OBJLAST },

	ASLIB_BEHAVIOR_NULL
};

static const asMethod_t scorestats_Methods[] =
{
	{ ASLIB_FUNCTION_DECL(void, setScore, ( int i )), objectScoreStats_ScoreSet, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, addScore, ( int i )), objectScoreStats_ScoreAdd, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, addRound, ()), objectScoreStats_RoundAdd, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, clear, ()), objectScoreStats_Clear, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(int, accuracyShots, ( int ammo ) const), objectScoreStats_AccShots, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(int, accuracyHits, ( int ammo ) const), objectScoreStats_AccHits, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(int, accuracyHitsDirect, ( int ammo ) const), objectScoreStats_AccHitsDirect, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(int, accuracyHitsAir, ( int ammo ) const), objectScoreStats_AccHitsAir, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(int, accuracyDamage, ( int ammo ) const), objectScoreStats_AccDamage, asCALL_CDECL_OBJLAST },

	ASLIB_METHOD_NULL
};

static const asProperty_t scorestats_Properties[] =
{
	{ ASLIB_PROPERTY_DECL(const int, score), ASLIB_FOFFSET(score_stats_t, score) },
	{ ASLIB_PROPERTY_DECL(const int, deaths), ASLIB_FOFFSET(score_stats_t, deaths) },
	{ ASLIB_PROPERTY_DECL(const int, frags), ASLIB_FOFFSET(score_stats_t, frags) },
	{ ASLIB_PROPERTY_DECL(const int, suicides), ASLIB_FOFFSET(score_stats_t, suicides) },
	{ ASLIB_PROPERTY_DECL(const int, teamFrags), ASLIB_FOFFSET(score_stats_t, teamfrags) },
	{ ASLIB_PROPERTY_DECL(const int, awards), ASLIB_FOFFSET(score_stats_t, awards) },
	{ ASLIB_PROPERTY_DECL(const int, totalDamageGiven), ASLIB_FOFFSET(score_stats_t, total_damage_given) },
	{ ASLIB_PROPERTY_DECL(const int, totalDamageReceived), ASLIB_FOFFSET(score_stats_t, total_damage_received) },
	{ ASLIB_PROPERTY_DECL(const int, totalTeamDamageGiven), ASLIB_FOFFSET(score_stats_t, total_teamdamage_given) },
	{ ASLIB_PROPERTY_DECL(const int, totalTeamDamageReceived), ASLIB_FOFFSET(score_stats_t, total_teamdamage_received) },
	{ ASLIB_PROPERTY_DECL(const int, healthTaken), ASLIB_FOFFSET(score_stats_t, health_taken) },
	{ ASLIB_PROPERTY_DECL(const int, armorTaken), ASLIB_FOFFSET(score_stats_t, armor_taken) },

	ASLIB_PROPERTY_NULL
};

static const asClassDescriptor_t asScoreStatsClassDescriptor =
{
	"cStats",					/* name */
	asOBJ_REF,					/* object type flags */
	sizeof( score_stats_t ),	/* size */
	scorestats_Funcdefs,	/* funcdefs */
	scorestats_ObjectBehaviors, /* object behaviors */
	scorestats_Methods,			/* methods */
	scorestats_Properties,		/* properties */

	NULL, NULL					/* string factory hack */
};

//=======================================================================

// CLASS: cBot
static int asbot_factored_count = 0;
static int asbot_released_count = 0;


typedef ai_handle_t asbot_t;

static asbot_t *objectBot_Factory()
{
	static asbot_t *object;

	object = G_AsMalloc( sizeof( asbot_t ) );
	object->asRefCount = 1;
	object->asFactored = 1;
	asbot_factored_count++;
	return object;
}

static void objectBot_Addref( asbot_t *obj ) { obj->asRefCount++; }

static void objectBot_Release( asbot_t *obj ) 
{
	obj->asRefCount--;
	clamp_low( obj->asRefCount, 0 );
	if( !obj->asRefCount && obj->asFactored )
	{
		G_AsFree( obj );
		asbot_released_count++;
	}
}

static void objectBot_ClearWeights( asbot_t *obj ) 
{
	memset( obj->status.entityWeights, 0, sizeof( obj->status.entityWeights ) );
}

static void objectBot_ResetWeights( asbot_t *obj ) 
{
	AI_ResetWeights( obj );
}

static void objectBot_SetGoalWeight( int index, float weight, asbot_t *obj ) 
{
	if( index < 0 || index >= MAX_EDICTS )
		return;

	obj->status.entityWeights[index] = weight;
}

static edict_t *objectBot_GetGoalEnt( int index, asbot_t *obj ) 
{
	return AI_GetGoalEnt( index );
}

static float objectBot_GetItemWeight( gsitem_t *item, asbot_t *obj ) 
{
	if( !item )
		return 0.0f;
	return obj->pers.inventoryWeights[item->tag];
}

static const asFuncdef_t asbot_Funcdefs[] =
{
	ASLIB_FUNCDEF_NULL
};

static const asBehavior_t asbot_ObjectBehaviors[] =
{
	{ asBEHAVE_FACTORY, ASLIB_FUNCTION_DECL(cBot @, f, ()), objectBot_Factory, asCALL_CDECL },
	{ asBEHAVE_ADDREF, ASLIB_FUNCTION_DECL(void, f, ()), objectBot_Addref, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_RELEASE, ASLIB_FUNCTION_DECL(void, f, ()), objectBot_Release, asCALL_CDECL_OBJLAST },

	ASLIB_BEHAVIOR_NULL
};

static const asMethod_t asbot_Methods[] =
{
	{ ASLIB_FUNCTION_DECL(void, clearGoalWeights, ()), objectBot_ClearWeights, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, resetGoalWeights, ()), objectBot_ResetWeights, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, setGoalWeight, ( int i, float weight )), objectBot_SetGoalWeight, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(cEntity @, getGoalEnt, ( int i ) const), objectBot_GetGoalEnt, asCALL_CDECL_OBJLAST },

	{ ASLIB_FUNCTION_DECL(float, getItemWeight, ( cItem @item ) const), objectBot_GetItemWeight, asCALL_CDECL_OBJLAST },

	ASLIB_METHOD_NULL
};

static const asProperty_t asbot_Properties[] =
{
	{ ASLIB_PROPERTY_DECL(const float, skill), ASLIB_FOFFSET(asbot_t, pers.skillLevel) },
	{ ASLIB_PROPERTY_DECL(const int, currentNode), ASLIB_FOFFSET(asbot_t, current_node) },
	{ ASLIB_PROPERTY_DECL(const int, nextNode), ASLIB_FOFFSET(asbot_t, next_node) },
	{ ASLIB_PROPERTY_DECL(uint, moveTypesMask), ASLIB_FOFFSET(asbot_t, status.moveTypesMask) },

	// character
	{ ASLIB_PROPERTY_DECL(const float, reactionTime), ASLIB_FOFFSET(asbot_t, pers.cha.reaction_time) },
	{ ASLIB_PROPERTY_DECL(const float, offensiveness), ASLIB_FOFFSET(asbot_t, pers.cha.offensiveness) },
	{ ASLIB_PROPERTY_DECL(const float, campiness), ASLIB_FOFFSET(asbot_t, pers.cha.campiness) },
	{ ASLIB_PROPERTY_DECL(const float, firerate), ASLIB_FOFFSET(asbot_t, pers.cha.firerate) },

	ASLIB_PROPERTY_NULL
};

static const asClassDescriptor_t asBotClassDescriptor =
{
	"cBot",						/* name */
	asOBJ_REF,					/* object type flags */
	sizeof( asbot_t ),			/* size */
	asbot_Funcdefs,		/* funcdefs */
	asbot_ObjectBehaviors,		/* object behaviors */
	asbot_Methods,				/* methods */
	asbot_Properties,			/* properties */

	NULL, NULL					/* string factory hack */
};

//=======================================================================

// CLASS: cClient
static int gclient_factored_count = 0;
static int gclient_released_count = 0;

static gclient_t *objectGameClient_Factory()
{
	static gclient_t *object;

	object = G_AsMalloc( sizeof( gclient_t ) );
	object->asRefCount = 1;
	object->asFactored = 1;
	gclient_factored_count++;
	return object;
}

static void objectGameClient_Addref( gclient_t *obj ) { obj->asRefCount++; }

static void objectGameClient_Release( gclient_t *obj ) 
{
	obj->asRefCount--;
	clamp_low( obj->asRefCount, 0 );
	if( !obj->asRefCount && obj->asFactored )
	{
		G_AsFree( obj );
		gclient_released_count++;
	}
}

static int objectGameClient_PlayerNum( gclient_t *self )
{
	if( self->asFactored )
		return -1;

	return (int)( self - game.clients );
}

static qboolean objectGameClient_isReady( gclient_t *self )
{
	if( self->asFactored )
		return qfalse;

	return ( level.ready[self - game.clients] || GS_MatchState() == MATCH_STATE_PLAYTIME );
}

static qboolean objectGameClient_isBot( gclient_t *self )
{
	int playerNum;

	if( self->asFactored )
		return qfalse;

	playerNum = (int)( self - game.clients );
	if( playerNum < 0 && playerNum >= gs.maxclients )
		return qfalse;

	return ( ( game.edicts[ playerNum + 1 ].r.svflags & SVF_FAKECLIENT ) &&
		game.edicts[ playerNum + 1 ].ai.type == AI_ISBOT );
}

static asbot_t *objectGameClient_getBot( gclient_t *self )
{
	int playerNum;
	edict_t *ent;

	if( self->asFactored )
		return NULL;

	playerNum = (int)( self - game.clients );
	if( playerNum < 0 && playerNum >= gs.maxclients )
		return NULL;

	ent = &game.edicts[playerNum + 1];

	if( !( ent->r.svflags & SVF_FAKECLIENT ) || ent->ai.type != AI_ISBOT )
		return NULL;

	return &ent->ai;
}

static int objectGameClient_ClientState( gclient_t *self )
{
	if( self->asFactored )
		return CS_FREE;

	return trap_GetClientState( (int)( self - game.clients ) );
}

static void objectGameClient_ClearPlayerStateEvents( gclient_t *self )
{
	G_ClearPlayerStateEvents( self );
}

static asstring_t *objectGameClient_getName( gclient_t *self )
{
	char temp[MAX_NAME_BYTES*2];

	Q_strncpyz( temp, self->netname, sizeof( temp ) );
	Q_strncatz( temp, S_COLOR_WHITE, sizeof( temp ) );

	return angelExport->asStringFactoryBuffer( temp, strlen( temp ) );
}

static asstring_t *objectGameClient_getClanName( gclient_t *self )
{
	char temp[MAX_CLANNAME_CHARS*2];

	Q_strncpyz( temp, self->clanname, sizeof( temp ) );
	Q_strncatz( temp, S_COLOR_WHITE, sizeof( temp ) );

	return angelExport->asStringFactoryBuffer( temp, strlen( temp ) );
}

static void objectGameClient_Respawn( qboolean ghost, gclient_t *self )
{
	int playerNum;

	if( self->asFactored )
		return;

	playerNum = (int)( self - game.clients );
	assert( playerNum >= 0 && playerNum < gs.maxclients );
	if( playerNum >= 0 && playerNum < gs.maxclients )
		G_ClientRespawn( &game.edicts[playerNum + 1], ghost );
}

static edict_t *objectGameClient_GetEntity( gclient_t *self )
{
	int playerNum;

	if( self->asFactored )
		return NULL;

	playerNum = (int)( self - game.clients );
	assert( playerNum >= 0 && playerNum < gs.maxclients );

	if( playerNum < 0 || playerNum >= gs.maxclients )
		return NULL;

	return &game.edicts[playerNum + 1];
}

static int objectGameClient_InventoryCount( int index, gclient_t *self )
{
	if( index < 0 || index >= MAX_ITEMS )
		return 0;

	return self->ps.inventory[ index ];
}

static void objectGameClient_InventorySetCount( int index, int newcount, gclient_t *self )
{
	gsitem_t *it;

	if( index < 0 || index >= MAX_ITEMS )
		return;

	it = GS_FindItemByTag( index );
	if( !it )
		return;

	if( newcount == 0 && ( it->type & IT_WEAPON ) )
	{
		if( index == self->ps.stats[STAT_PENDING_WEAPON] )
			self->ps.stats[STAT_PENDING_WEAPON] = self->ps.stats[STAT_WEAPON];
		else if( index == self->ps.stats[STAT_WEAPON] )
		{
			self->ps.stats[STAT_WEAPON] = self->ps.stats[STAT_PENDING_WEAPON] = WEAP_NONE;
			self->ps.weaponState = WEAPON_STATE_READY;
			self->ps.stats[STAT_WEAPON_TIME] = 0;
		}
	}

	self->ps.inventory[ index ] = newcount;
}

static void objectGameClient_InventoryGiveItemExt( int index, int count, gclient_t *self )
{
	gsitem_t *it;
	edict_t *tmpEnt, *selfEnt;
	int playerNum;

	if( index < 0 || index >= MAX_ITEMS )
		return;

	it = GS_FindItemByTag( index );
	if( !it )
		return;

	if( !(it->flags & ITFLAG_PICKABLE) )
		return;

	playerNum = self - game.clients;
	if( playerNum < 0 || playerNum >= gs.maxclients )
		return;

	selfEnt = PLAYERENT( playerNum );

	tmpEnt = G_Spawn();
	tmpEnt->r.solid = SOLID_TRIGGER;
	tmpEnt->s.type = ET_ITEM;
	tmpEnt->count = count < 0 ? it->quantity : count;
	tmpEnt->item = it;

	G_PickupItem( tmpEnt, selfEnt );

	G_FreeEdict( tmpEnt );
}

static void objectGameClient_InventoryGiveItem( int index, gclient_t *self )
{
	objectGameClient_InventoryGiveItemExt( index, -1, self );
}

static void objectGameClient_InventoryClear( gclient_t *self )
{
	memset( self->ps.inventory, 0, sizeof( self->ps.inventory ) );

	self->ps.stats[STAT_WEAPON] = self->ps.stats[STAT_PENDING_WEAPON] = WEAP_NONE;
	self->ps.weaponState = WEAPON_STATE_READY;
	self->ps.stats[STAT_WEAPON_TIME] = 0;
}

static qboolean objectGameClient_CanSelectWeapon( int index, gclient_t *self )
{
	if( index < WEAP_NONE || index >= WEAP_TOTAL )
		return qfalse;

	return ( GS_CheckAmmoInWeapon( &self->ps, index ) );
}

static void objectGameClient_SelectWeapon( int index, gclient_t *self )
{
	if( index < WEAP_NONE || index >= WEAP_TOTAL )
	{
		self->ps.stats[STAT_PENDING_WEAPON] = GS_SelectBestWeapon( &self->ps );
		return;
	}

	if( GS_CheckAmmoInWeapon( &self->ps, index ) )
		self->ps.stats[STAT_PENDING_WEAPON] = index;
}

static void objectGameClient_addAward( asstring_t *msg, gclient_t *self )
{
	int playerNum;

	if( self->asFactored || !msg )
		return;

	playerNum = (int)( self - game.clients );
	assert( playerNum >= 0 && playerNum < gs.maxclients );

	if( playerNum < 0 || playerNum >= gs.maxclients )
		return;

	G_PlayerAward( &game.edicts[playerNum + 1], msg->buffer );
}

static void objectGameClient_addMetaAward( asstring_t *msg, gclient_t *self )
{
	int playerNum;

	if( self->asFactored || !msg )
		return;

	playerNum = (int)( self - game.clients );
	assert( playerNum >= 0 && playerNum < gs.maxclients );

	if( playerNum < 0 || playerNum >= gs.maxclients )
		return;

	G_PlayerMetaAward( &game.edicts[playerNum + 1], msg->buffer );
}

static void objectGameClient_execGameCommand( asstring_t *msg, gclient_t *self )
{
	int playerNum;

	if( self->asFactored || !msg )
		return;

	playerNum = (int)( self - game.clients );
	assert( playerNum >= 0 && playerNum < gs.maxclients );

	if( playerNum < 0 || playerNum >= gs.maxclients )
		return;

	trap_GameCmd( &game.edicts[playerNum + 1], msg->buffer );
}

static void objectGameClient_setHUDStat( int stat, int value, gclient_t *self )
{
	if( !ISGAMETYPESTAT( stat ) )
	{
		if( stat > 0 && stat < GS_GAMETYPE_STATS_START )
			G_Printf( "* WARNING: stat %i is write protected\n", stat );
		else
			G_Printf( "* WARNING: %i is not a valid stat\n", stat );
		return;
	}

	self->ps.stats[ stat ] = ( (short)value & 0xFFFF );
}

static int objectGameClient_getHUDStat( int stat, gclient_t *self )
{
	if( stat < 0 && stat >= MAX_STATS )
	{
		G_Printf( "* WARNING: stat %i is out of range\n", stat );
		return 0;
	}

	return self->ps.stats[ stat ];
}

static void objectGameClient_setPMoveFeatures( unsigned int bitmask, gclient_t *self )
{
	self->ps.pmove.stats[PM_STAT_FEATURES] = ( bitmask & PMFEAT_ALL );
}

static void objectGameClient_setPMoveMaxSpeed( float speed, gclient_t *self )
{
	if( speed < 0.0f )
		self->ps.pmove.stats[PM_STAT_MAXSPEED] = DEFAULT_PLAYERSPEED;
	else
		self->ps.pmove.stats[PM_STAT_MAXSPEED] = ( (int)speed & 0xFFFF );
}

static void objectGameClient_setPMoveJumpSpeed( float speed, gclient_t *self )
{
	if( speed < 0.0f )
		self->ps.pmove.stats[PM_STAT_JUMPSPEED] = DEFAULT_JUMPSPEED;
	else
		self->ps.pmove.stats[PM_STAT_JUMPSPEED] = ( (int)speed & 0xFFFF );
}

static void objectGameClient_setPMoveDashSpeed( float speed, gclient_t *self )
{
	if( speed < 0.0f )
		self->ps.pmove.stats[PM_STAT_DASHSPEED] = DEFAULT_DASHSPEED;
	else
		self->ps.pmove.stats[PM_STAT_DASHSPEED] = ( (int)speed & 0xFFFF );
}

static asstring_t *objectGameClient_getUserInfoKey( asstring_t *key, gclient_t *self )
{
	char *s;

	if( !key || !key->buffer || !key->buffer[0] )
		return angelExport->asStringFactoryBuffer( NULL, 0 );

	s = Info_ValueForKey( self->userinfo, key->buffer );
	if( !s || !*s )
		return angelExport->asStringFactoryBuffer( NULL, 0 );

	return angelExport->asStringFactoryBuffer( s, strlen( s ) );
}

static void objectGameClient_printMessage( asstring_t *str, gclient_t *self )
{
	int playerNum;

	if( !str || !str->buffer )
		return;

	playerNum = (int)( self - game.clients );
	assert( playerNum >= 0 && playerNum < gs.maxclients );

	if( playerNum < 0 || playerNum >= gs.maxclients )
		return;

	G_PrintMsg( &game.edicts[ playerNum + 1 ], "%s", str->buffer );
}

static void objectGameClient_ChaseCam( asstring_t *str, qboolean teamonly, gclient_t *self )
{
	int playerNum;

	playerNum = (int)( self - game.clients );
	if( playerNum < 0 || playerNum >= gs.maxclients )
		return;

	G_ChasePlayer( &game.edicts[ playerNum + 1 ], str ? str->buffer : NULL, teamonly, 0 );
}

static void objectGameClient_SetChaseActive( qboolean active, gclient_t *self )
{
	int playerNum;

	playerNum = (int)( self - game.clients );
	assert( playerNum >= 0 && playerNum < gs.maxclients );

	if( playerNum < 0 || playerNum >= gs.maxclients )
		return;

	self->resp.chase.active = active;
	G_UpdatePlayerMatchMsg( &game.edicts[ playerNum + 1 ] );
}

static void objectGameClient_NewRaceRun( int numSectors, gclient_t *self )
{
	int playerNum;

	playerNum = (int)( self - game.clients );
	assert( playerNum >= 0 && playerNum < gs.maxclients );

	if( playerNum < 0 || playerNum >= gs.maxclients )
		return;

	G_NewRaceRun( &game.edicts[ playerNum + 1 ], numSectors );
}

static void objectGameClient_SetRaceTime( int sector, unsigned int time, gclient_t *self )
{
	int playerNum;

	playerNum = (int)( self - game.clients );
	assert( playerNum >= 0 && playerNum < gs.maxclients );

	if( playerNum < 0 || playerNum >= gs.maxclients )
		return;

	G_SetRaceTime( &game.edicts[ playerNum + 1 ], sector, time );
}

static const asFuncdef_t gameclient_Funcdefs[] =
{
	ASLIB_FUNCDEF_NULL
};

static const asBehavior_t gameclient_ObjectBehaviors[] =
{
	{ asBEHAVE_FACTORY, ASLIB_FUNCTION_DECL(cClient @, f, ()), objectGameClient_Factory, asCALL_CDECL },
	{ asBEHAVE_ADDREF, ASLIB_FUNCTION_DECL(void, f, ()), objectGameClient_Addref, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_RELEASE, ASLIB_FUNCTION_DECL(void, f, ()), objectGameClient_Release, asCALL_CDECL_OBJLAST },

	ASLIB_BEHAVIOR_NULL
};

static const asMethod_t gameclient_Methods[] =
{
	{ ASLIB_FUNCTION_DECL(int, get_playerNum, () const), objectGameClient_PlayerNum, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(bool, isReady, () const), objectGameClient_isReady, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(bool, isBot, () const), objectGameClient_isBot, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(cBot @, getBot, () const), objectGameClient_getBot, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(int, state, () const), objectGameClient_ClientState, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, respawn, ( bool ghost )), objectGameClient_Respawn, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, clearPlayerStateEvents, ()), objectGameClient_ClearPlayerStateEvents, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(String @, get_name, () const), objectGameClient_getName, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(String @, get_clanName, () const), objectGameClient_getClanName, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(cEntity @, getEnt, () const), objectGameClient_GetEntity, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(int, inventoryCount, ( int tag ) const), objectGameClient_InventoryCount, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, inventorySetCount, ( int tag, int count )), objectGameClient_InventorySetCount, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, inventoryGiveItem, ( int tag, int count )), objectGameClient_InventoryGiveItemExt, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, inventoryGiveItem, ( int tag )), objectGameClient_InventoryGiveItem, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, inventoryClear, ()), objectGameClient_InventoryClear, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(bool, canSelectWeapon, ( int tag ) const), objectGameClient_CanSelectWeapon, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, selectWeapon, ( int tag )), objectGameClient_SelectWeapon, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, addAward, ( const String &in )), objectGameClient_addAward, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, addMetaAward, ( const String &in )), objectGameClient_addMetaAward, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, execGameCommand, ( const String &in )), objectGameClient_execGameCommand, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, setHUDStat, ( int stat, int value )), objectGameClient_setHUDStat, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(int, getHUDStat, ( int stat ) const), objectGameClient_getHUDStat, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, setPMoveFeatures, ( uint bitmask )), objectGameClient_setPMoveFeatures, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, setPMoveMaxSpeed, ( float speed )), objectGameClient_setPMoveMaxSpeed, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, setPMoveJumpSpeed, ( float speed )), objectGameClient_setPMoveJumpSpeed, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, setPMoveDashSpeed, ( float speed )), objectGameClient_setPMoveDashSpeed, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(String @, getUserInfoKey, ( const String &in ) const), objectGameClient_getUserInfoKey, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, printMessage, ( const String &in )), objectGameClient_printMessage, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, chaseCam, ( String @, bool teamOnly )), objectGameClient_ChaseCam, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, setChaseActive, ( bool active )), objectGameClient_ChaseCam, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, newRaceRun, ( int numSectors )), objectGameClient_NewRaceRun, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, setRaceTime, ( int sector, uint time )), objectGameClient_SetRaceTime, asCALL_CDECL_OBJLAST },

	ASLIB_METHOD_NULL
};

static const asProperty_t gameclient_Properties[] =
{
	{ ASLIB_PROPERTY_DECL(cStats, stats), ASLIB_FOFFSET(gclient_t, level.stats) },
	{ ASLIB_PROPERTY_DECL(const bool, connecting), ASLIB_FOFFSET(gclient_t, connecting) },
	{ ASLIB_PROPERTY_DECL(const bool, multiview), ASLIB_FOFFSET(gclient_t, multiview) },
	{ ASLIB_PROPERTY_DECL(const bool, tv), ASLIB_FOFFSET(gclient_t, tv) },
	{ ASLIB_PROPERTY_DECL(int, team), ASLIB_FOFFSET(gclient_t, team) },
	{ ASLIB_PROPERTY_DECL(const int, hand), ASLIB_FOFFSET(gclient_t, hand) },
	{ ASLIB_PROPERTY_DECL(const int, fov), ASLIB_FOFFSET(gclient_t, fov) },
	{ ASLIB_PROPERTY_DECL(const int, zoomFov), ASLIB_FOFFSET(gclient_t, zoomfov) },
	{ ASLIB_PROPERTY_DECL(const bool, isOperator), ASLIB_FOFFSET(gclient_t, isoperator) },
	{ ASLIB_PROPERTY_DECL(const uint, queueTimeStamp), ASLIB_FOFFSET(gclient_t, queueTimeStamp) },
	{ ASLIB_PROPERTY_DECL(int, muted), ASLIB_FOFFSET(gclient_t, muted) }, //racesow allow editing the int
	{ ASLIB_PROPERTY_DECL(float, armor), ASLIB_FOFFSET(gclient_t, resp.armor) },
	{ ASLIB_PROPERTY_DECL(uint, gunbladeChargeTimeStamp), ASLIB_FOFFSET(gclient_t, resp.gunbladeChargeTimeStamp) },
	{ ASLIB_PROPERTY_DECL(const bool, chaseActive), ASLIB_FOFFSET(gclient_t, resp.chase.active) },
	{ ASLIB_PROPERTY_DECL(int, chaseTarget), ASLIB_FOFFSET(gclient_t, resp.chase.target) },
	{ ASLIB_PROPERTY_DECL(bool, chaseTeamonly), ASLIB_FOFFSET(gclient_t, resp.chase.teamonly) },
	{ ASLIB_PROPERTY_DECL(int, chaseFollowMode), ASLIB_FOFFSET(gclient_t, resp.chase.followmode) },
	{ ASLIB_PROPERTY_DECL(const bool, coach), ASLIB_FOFFSET(gclient_t, teamstate.is_coach) },
	{ ASLIB_PROPERTY_DECL(const int, ping), ASLIB_FOFFSET(gclient_t, r.ping) },
	{ ASLIB_PROPERTY_DECL(const int16, weapon), ASLIB_FOFFSET(gclient_t, ps.stats[STAT_WEAPON]) },
	{ ASLIB_PROPERTY_DECL(const int16, pendingWeapon), ASLIB_FOFFSET(gclient_t, ps.stats[STAT_PENDING_WEAPON]) },
	{ ASLIB_PROPERTY_DECL(const int16, pmoveFeatures), ASLIB_FOFFSET(gclient_t, ps.pmove.stats[PM_STAT_FEATURES]) },
	{ ASLIB_PROPERTY_DECL(bool, takeStun), ASLIB_FOFFSET(gclient_t, resp.takeStun) },
	{ ASLIB_PROPERTY_DECL(uint, lastActivity), ASLIB_FOFFSET(gclient_t, level.last_activity) },

	ASLIB_PROPERTY_NULL
};

static const asClassDescriptor_t asGameClientDescriptor =
{
	"cClient",					/* name */
	asOBJ_REF,					/* object type flags */
	sizeof( gclient_t ),		/* size */
	gameclient_Funcdefs,	/* funcdefs */
	gameclient_ObjectBehaviors,	/* object behaviors */
	gameclient_Methods,			/* methods */
	gameclient_Properties,		/* properties */

	NULL, NULL					/* string factory hack */
};

//=======================================================================

// CLASS: cEntity
static int edict_factored_count = 0;
static int edict_released_count = 0;

static void *asEntityCallThinkFuncPtr = NULL;
static void *asEntityCallTouchFuncPtr = NULL;
static void *asEntityCallUseFuncPtr = NULL;
static void *asEntityCallStopFuncPtr = NULL;
static void *asEntityCallPainFuncPtr = NULL;
static void *asEntityCallDieFuncPtr = NULL;

static edict_t *objectGameEntity_Factory()
{
	static edict_t *object;

	object = G_AsMalloc( sizeof( edict_t ) );
	object->asRefCount = 1;
	object->asFactored = 1;
	object->s.number = -1;
	edict_factored_count++;
	return object;
}

static void objectGameEntity_Addref( edict_t *obj ) {
	obj->asRefCount++;
}

static void objectGameEntity_Release( edict_t *obj ) 
{
	obj->asRefCount--;
	clamp_low( obj->asRefCount, 0 );
	if( !obj->asRefCount && obj->asFactored )
	{
		G_AsFree( obj );
		edict_released_count++;
	}
}


static qboolean objectGameEntity_EqualBehaviour( edict_t *first, edict_t *second )
{
	return ( first == second );
}

static asvec3_t objectGameEntity_GetVelocity( edict_t *obj )
{
	asvec3_t velocity;

	VectorCopy( obj->velocity, velocity.v );

	return velocity;
}

static void objectGameEntity_SetVelocity( asvec3_t *vel, edict_t *self )
{
	GS_SnapVelocity( self->velocity );

	VectorCopy( vel->v, self->velocity );

	if( self->r.client && trap_GetClientState( PLAYERNUM(self) ) >= CS_SPAWNED )
	{
		VectorCopy( vel->v, self->r.client->ps.pmove.velocity );
	}
}

static asvec3_t objectGameEntity_GetAVelocity( edict_t *obj )
{
	asvec3_t avelocity;

	VectorCopy( obj->avelocity, avelocity.v );

	return avelocity;
}

static void objectGameEntity_SetAVelocity( asvec3_t *vel, edict_t *self )
{
	VectorCopy( vel->v, self->avelocity );
}

static asvec3_t objectGameEntity_GetOrigin( edict_t *obj )
{
	asvec3_t origin;

	VectorCopy( obj->s.origin, origin.v );
	return origin;
}

static void objectGameEntity_SetOrigin( asvec3_t *vec, edict_t *self )
{
	if( self->r.client && trap_GetClientState( PLAYERNUM(self) ) >= CS_SPAWNED )
	{
		GS_SnapPosition( vec->v, self->r.mins, self->r.maxs, ENTNUM( self ), self->s.solid ? G_SolidMaskForEnt( self ) : 0 );
		VectorCopy( vec->v, self->r.client->ps.pmove.origin );
	}

	VectorCopy( vec->v, self->s.origin );
}

static asvec3_t objectGameEntity_GetOrigin2( edict_t *obj )
{
	asvec3_t origin;

	VectorCopy( obj->s.origin2, origin.v );
	return origin;
}

static void objectGameEntity_SetOrigin2( asvec3_t *vec, edict_t *self )
{
	VectorCopy( vec->v, self->s.origin2 );
}

static asvec3_t objectGameEntity_GetAngles( edict_t *obj )
{
	asvec3_t angles;

	VectorCopy( obj->s.angles, angles.v );
	return angles;
}

static void objectGameEntity_SetAngles( asvec3_t *vec, edict_t *self )
{
	VectorCopy( vec->v, self->s.angles );

	if( self->r.client && trap_GetClientState( PLAYERNUM(self) ) >= CS_SPAWNED )
	{
		int i;

		VectorCopy( vec->v, self->r.client->ps.viewangles );

		// update the delta angle
		for( i = 0; i < 3; i++ )
			self->r.client->ps.pmove.delta_angles[i] = ANGLE2SHORT( self->r.client->ps.viewangles[i] ) - self->r.client->ucmd.angles[i];
	}
}

static void objectGameEntity_GetSize( asvec3_t *mins, asvec3_t *maxs, edict_t *self )
{
	VectorCopy( self->r.maxs, maxs->v );
	VectorCopy( self->r.mins, mins->v );
}

static void objectGameEntity_SetSize( asvec3_t *mins, asvec3_t *maxs, edict_t *self )
{
	VectorCopy( mins->v, self->r.mins );
	VectorCopy( maxs->v, self->r.maxs );
}

static asvec3_t objectGameEntity_GetMovedir( edict_t *self )
{
	asvec3_t movedir;

	VectorCopy( self->moveinfo.movedir, movedir.v );
	return movedir;
}

static void objectGameEntity_SetMovedir( edict_t *self )
{
	G_SetMovedir( self->s.angles, self->moveinfo.movedir );
}

static qboolean objectGameEntity_isBrushModel( edict_t *self )
{
	return ISBRUSHMODEL( self->s.modelindex );
}

static qboolean objectGameEntity_IsGhosting( edict_t *self )
{
	if( self->r.client && trap_GetClientState( PLAYERNUM( self ) ) < CS_SPAWNED )
		return qtrue;

	return G_ISGHOSTING( self );
}

static int objectGameEntity_EntNum( edict_t *self )
{
	if( self->asFactored )
		return -1;
	return ( ENTNUM( self ) );
}

static int objectGameEntity_PlayerNum( edict_t *self )
{
	if( self->asFactored )
		return -1;
	return ( PLAYERNUM( self ) );
}

static asstring_t *objectGameEntity_getModelName( edict_t *self )
{
	return angelExport->asStringFactoryBuffer( self->model, self->model ? strlen(self->model) : 0 );
}

static asstring_t *objectGameEntity_getModel2Name( edict_t *self )
{
	return angelExport->asStringFactoryBuffer( self->model2, self->model2 ? strlen(self->model2) : 0 );
}

static asstring_t *objectGameEntity_getClassname( edict_t *self )
{
	return angelExport->asStringFactoryBuffer( self->classname, self->classname ? strlen(self->classname) : 0 );
}

/*
static asstring_t *objectGameEntity_getSpawnKey( asstring_t *key, edict_t *self )
{
const char *val;

if( !key )
return angelExport->asStringFactoryBuffer( NULL, 0 );

val = G_GetEntitySpawnKey( key->buffer, self );

return angelExport->asStringFactoryBuffer( val, strlen( val ) );
}
*/

static asstring_t *objectGameEntity_getTargetname( edict_t *self )
{
	return angelExport->asStringFactoryBuffer( self->targetname, self->targetname ? strlen(self->targetname) : 0 );
}

static void objectGameEntity_setTargetname( asstring_t *targetname, edict_t *self )
{
	self->targetname = G_RegisterLevelString( targetname->buffer );
}

static asstring_t *objectGameEntity_getTarget( edict_t *self )
{
	return angelExport->asStringFactoryBuffer( self->target, self->target ? strlen(self->target) : 0 );
}

static void objectGameEntity_setTarget( asstring_t *target, edict_t *self )
{
	self->target = G_RegisterLevelString( target->buffer );
}

static asstring_t *objectGameEntity_getMap( edict_t *self )
{
	return angelExport->asStringFactoryBuffer( self->map, self->map ? strlen(self->map) : 0 );
}

static asstring_t *objectGameEntity_getSoundName( edict_t *self )
{
	return angelExport->asStringFactoryBuffer( self->sounds, self->sounds ? strlen(self->sounds) : 0 );
}

static void objectGameEntity_setClassname( asstring_t *classname, edict_t *self )
{
	self->classname = G_RegisterLevelString( classname->buffer );
}

static void objectGameEntity_setMap( asstring_t *map, edict_t *self )
{
	self->map = G_RegisterLevelString( map->buffer );
}

static void objectGameEntity_GhostClient( edict_t *self )
{
	if( self->r.client )
		G_GhostClient( self );
}

static void objectGameEntity_SetupModelExt( asstring_t *modelstr, asstring_t *skinstr, edict_t *self )
{
	char *path;
	const char *s;

	if( !modelstr )
	{
		self->s.modelindex = 0;
		return;
	}

	path = modelstr->buffer;
	while( path[0] == '$' )
		path++;
	s = strstr( path, "models/players/" );

	// if it's a player model
	if( s == path )
	{
		char skin[MAX_QPATH], model[MAX_QPATH];

		s += strlen( "models/players/" );

		Q_snprintfz( model, sizeof( model ), "$%s", path );
		Q_snprintfz( skin, sizeof( skin ), "models/players/%s/%s", s, skinstr && skinstr->buffer[0] ? skinstr->buffer : DEFAULT_PLAYERSKIN );

		self->s.modelindex = trap_ModelIndex( model );
		self->s.skinnum = trap_SkinIndex( skin );
		return;
	}

	GClip_SetBrushModel( self, path );
}

static void objectGameEntity_SetupModel( asstring_t *modelstr, edict_t *self )
{
	objectGameEntity_SetupModelExt( modelstr, NULL, self );
}

static void objectGameEntity_UseTargets( edict_t *activator, edict_t *self )
{
	G_UseTargets( self, activator );
}

static edict_t *objectGameEntity_DropItem( int tag, edict_t *self )
{
	gsitem_t *item = GS_FindItemByTag( tag );

	if( !item )
		return NULL;

	return Drop_Item( self, item );
}

static edict_t *objectGameEntity_findTarget( edict_t *from, edict_t *self )
{
	if( !self->target )
		return NULL;

	return G_Find( from, FOFS( targetname ), self->target );
}

static edict_t *objectGameEntity_findTargeting( edict_t *from, edict_t *self )
{
	if( !self->targetname )
		return NULL;

	return G_Find( from, FOFS( target ), self->targetname );
}

static void objectGameEntity_setAIgoal( qboolean customReach, edict_t *self )
{
	if( customReach )
		AI_AddGoalEntityCustom( self );
	else
		AI_AddGoalEntity( self );
}

static void objectGameEntity_setAIgoalSimple( edict_t *self )
{
	objectGameEntity_setAIgoal( qfalse, self );
}

static void objectGameEntity_removeAIgoal( edict_t *self )
{
	AI_RemoveGoalEntity( self );
}

static void objectGameEntity_reachedAIgoal( edict_t *self )
{
	AI_ReachedEntity( self );
}

static void objectGameEntity_TeleportEffect( qboolean in, edict_t *self )
{
	G_TeleportEffect( self, in );
}

static void objectGameEntity_sustainDamage( edict_t *inflictor, edict_t *attacker, asvec3_t *dir, float damage, float knockback, float stun, int mod, edict_t *self )
{
	G_Damage( self, inflictor, attacker,
		dir ? dir->v : NULL, dir ? dir->v : NULL,
		inflictor ? inflictor->s.origin : self->s.origin,
		damage, knockback, stun, 0, mod >= 0 ? mod : 0 );
}

static void objectGameEntity_splashDamage( edict_t *attacker, int radius, float damage, float knockback, float stun, int mod, edict_t *self )
{
	if( radius < 1 )
		return;

	self->projectileInfo.maxDamage = damage;
	self->projectileInfo.minDamage = 1;
	self->projectileInfo.maxKnockback = knockback;
	self->projectileInfo.minKnockback = 1;
	self->projectileInfo.stun = stun;
	self->projectileInfo.radius = radius;

	G_RadiusDamage( self, attacker, NULL, self, mod >= 0 ? mod : 0 );
}

static void objectGameEntity_explosionEffect( int radius, edict_t *self )
{
	edict_t *event;
	int i, eventType, eventRadius;
	vec3_t center;

	if( radius < 8 )
		return;

	// turn entity into event
	if( radius > 255 * 8 )
	{
		eventType = EV_EXPLOSION2;
		eventRadius = (int)( radius * 1/16 ) & 0xFF;
	}
	else
	{
		eventType = EV_EXPLOSION1;
		eventRadius = (int)( radius * 1/8 ) & 0xFF;
	}

	if( eventRadius < 1 )
		eventRadius = 1;

	for( i = 0; i < 3; i++ )
		center[i] = self->s.origin[i] + ( 0.5f * ( self->r.maxs[i] + self->r.mins[i] ) );

	event = G_SpawnEvent( eventType, eventRadius, center );
}

static const asFuncdef_t gedict_Funcdefs[] =
{
	{ ASLIB_FUNCTION_DECL(void, entThink, (cEntity @ent)) },
	{ ASLIB_FUNCTION_DECL(void, entTouch, (cEntity @ent, cEntity @other, const Vec3 planeNormal, int surfFlags)) },
	{ ASLIB_FUNCTION_DECL(void, entUse, (cEntity @ent, cEntity @other, cEntity @activator)) },
	{ ASLIB_FUNCTION_DECL(void, entPain, (cEntity @ent, cEntity @other, float kick, float damage)) },
	{ ASLIB_FUNCTION_DECL(void, entDie, (cEntity @ent, cEntity @inflicter, cEntity @attacker)) },
	{ ASLIB_FUNCTION_DECL(void, entStop, (cEntity @ent)) },

	ASLIB_FUNCDEF_NULL
};

static const asBehavior_t gedict_ObjectBehaviors[] =
{
	{ asBEHAVE_FACTORY, ASLIB_FUNCTION_DECL(cEntity @, f, ()), objectGameEntity_Factory, asCALL_CDECL },
	{ asBEHAVE_ADDREF, ASLIB_FUNCTION_DECL(void, f, ()), objectGameEntity_Addref, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_RELEASE, ASLIB_FUNCTION_DECL(void, f, ()), objectGameEntity_Release, asCALL_CDECL_OBJLAST },

	ASLIB_BEHAVIOR_NULL
};

static const asMethod_t gedict_Methods[] =
{
	{ ASLIB_FUNCTION_DECL(Vec3, get_velocity, () const), objectGameEntity_GetVelocity, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, set_velocity, (const Vec3 &in)), objectGameEntity_SetVelocity, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(Vec3, get_avelocity, () const), objectGameEntity_GetAVelocity, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, set_avelocity, (const Vec3 &in)), objectGameEntity_SetAVelocity, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(Vec3, get_origin, () const), objectGameEntity_GetOrigin, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, set_origin, (const Vec3 &in)), objectGameEntity_SetOrigin, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(Vec3, get_origin2, () const), objectGameEntity_GetOrigin2, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, set_origin2, (const Vec3 &in)), objectGameEntity_SetOrigin2, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(Vec3, get_angles, () const), objectGameEntity_GetAngles, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, set_angles, (const Vec3 &in)), objectGameEntity_SetAngles, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, getSize, (Vec3 &out, Vec3 &out)), objectGameEntity_GetSize, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, setSize, (const Vec3 &in, const Vec3 &in)), objectGameEntity_SetSize, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(Vec3, get_movedir, () const), objectGameEntity_GetMovedir, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, set_movedir, ()), objectGameEntity_SetMovedir, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(bool, isBrushModel, () const), objectGameEntity_isBrushModel, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, freeEntity, ()), G_FreeEdict, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, linkEntity, ()), GClip_LinkEntity, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, unlinkEntity, ()), GClip_UnlinkEntity, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(bool, isGhosting, () const), objectGameEntity_IsGhosting, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(int, get_entNum, () const), objectGameEntity_EntNum, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(int, get_playerNum, () const), objectGameEntity_PlayerNum, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(String @, get_model, () const), objectGameEntity_getModelName, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(String @, get_model2, () const), objectGameEntity_getModel2Name, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(String @, get_sounds, () const), objectGameEntity_getSoundName, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(String @, get_classname, () const), objectGameEntity_getClassname, asCALL_CDECL_OBJLAST },
	//{ ASLIB_FUNCTION_DECL(String @, getSpawnKey, ( String &in )), objectGameEntity_getSpawnKey, NULL, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(String @, get_targetname, () const), objectGameEntity_getTargetname, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(String @, get_target, () const), objectGameEntity_getTarget, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(String @, get_map, () const), objectGameEntity_getMap, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, set_target, ( const String &in )), objectGameEntity_setTarget, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, set_targetname, ( const String &in )), objectGameEntity_setTargetname, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, set_classname, ( const String &in )), objectGameEntity_setClassname, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, set_map, ( const String &in )), objectGameEntity_setMap, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, ghost, ()), objectGameEntity_GhostClient, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, spawnqueueAdd, ()), G_SpawnQueue_AddClient, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, teleportEffect, ( bool )), objectGameEntity_TeleportEffect, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, respawnEffect, ()), G_RespawnEffect, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, setupModel, ( const String &in )), objectGameEntity_SetupModel, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, setupModel, ( const String &in, const String &in )), objectGameEntity_SetupModelExt, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(cEntity @, findTargetEntity, ( const cEntity @from ) const), objectGameEntity_findTarget, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(cEntity @, findTargetingEntity, ( const cEntity @from ) const), objectGameEntity_findTargeting, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, useTargets, ( const cEntity @activator )), objectGameEntity_UseTargets, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(cEntity @, dropItem, ( int tag ) const), objectGameEntity_DropItem, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, addAIGoal, ( bool customReach )), objectGameEntity_setAIgoal, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, addAIGoal, ()), objectGameEntity_setAIgoalSimple, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, removeAIGoal, ()), objectGameEntity_removeAIgoal, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, reachedAIGoal, ()), objectGameEntity_reachedAIgoal, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, sustainDamage, ( cEntity @inflicter, cEntity @attacker, const Vec3 &in dir, float damage, float knockback, float stun, int mod )), objectGameEntity_sustainDamage, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, splashDamage, ( cEntity @attacker, int radius, float damage, float knockback, float stun, int mod )), objectGameEntity_splashDamage, asCALL_CDECL_OBJLAST },
	{ ASLIB_FUNCTION_DECL(void, explosionEffect, ( int radius )), objectGameEntity_explosionEffect, asCALL_CDECL_OBJLAST },

	ASLIB_METHOD_NULL
};

static const asProperty_t gedict_Properties[] =
{
	{ ASLIB_PROPERTY_DECL(cClient @, client), ASLIB_FOFFSET(edict_t, r.client) },
	{ ASLIB_PROPERTY_DECL(cItem @, item), ASLIB_FOFFSET(edict_t, item) },
	{ ASLIB_PROPERTY_DECL(cEntity @, groundEntity), ASLIB_FOFFSET(edict_t, groundentity) },
	{ ASLIB_PROPERTY_DECL(cEntity @, owner), ASLIB_FOFFSET(edict_t, r.owner) },
	{ ASLIB_PROPERTY_DECL(cEntity @, enemy), ASLIB_FOFFSET(edict_t, enemy) },
	{ ASLIB_PROPERTY_DECL(cEntity @, activator), ASLIB_FOFFSET(edict_t, activator) },
	{ ASLIB_PROPERTY_DECL(int, type), ASLIB_FOFFSET(edict_t, s.type) },
	{ ASLIB_PROPERTY_DECL(int, modelindex), ASLIB_FOFFSET(edict_t, s.modelindex) },
	{ ASLIB_PROPERTY_DECL(int, modelindex2), ASLIB_FOFFSET(edict_t, s.modelindex2) },
	{ ASLIB_PROPERTY_DECL(int, frame), ASLIB_FOFFSET(edict_t, s.frame) },
	{ ASLIB_PROPERTY_DECL(int, ownerNum), ASLIB_FOFFSET(edict_t, s.ownerNum) },
	{ ASLIB_PROPERTY_DECL(int, counterNum), ASLIB_FOFFSET(edict_t, s.counterNum) },
	{ ASLIB_PROPERTY_DECL(int, skinNum), ASLIB_FOFFSET(edict_t, s.skinnum) },
	{ ASLIB_PROPERTY_DECL(int, colorRGBA), ASLIB_FOFFSET(edict_t, s.colorRGBA) },
	{ ASLIB_PROPERTY_DECL(int, weapon), ASLIB_FOFFSET(edict_t, s.weapon) },
	{ ASLIB_PROPERTY_DECL(bool, teleported), ASLIB_FOFFSET(edict_t, s.teleported) },
	{ ASLIB_PROPERTY_DECL(uint, effects), ASLIB_FOFFSET(edict_t, s.effects) },
	{ ASLIB_PROPERTY_DECL(int, sound), ASLIB_FOFFSET(edict_t, s.sound) },
	{ ASLIB_PROPERTY_DECL(int, team), ASLIB_FOFFSET(edict_t, s.team) },
	{ ASLIB_PROPERTY_DECL(int, light), ASLIB_FOFFSET(edict_t, s.light) },
	{ ASLIB_PROPERTY_DECL(const bool, inuse), ASLIB_FOFFSET(edict_t, r.inuse) },
	{ ASLIB_PROPERTY_DECL(uint, svflags), ASLIB_FOFFSET(edict_t, r.svflags) },
	{ ASLIB_PROPERTY_DECL(int, solid), ASLIB_FOFFSET(edict_t, r.solid) },
	{ ASLIB_PROPERTY_DECL(int, clipMask), ASLIB_FOFFSET(edict_t, r.clipmask) },
	{ ASLIB_PROPERTY_DECL(int, spawnFlags), ASLIB_FOFFSET(edict_t, spawnflags) },
	{ ASLIB_PROPERTY_DECL(int, style), ASLIB_FOFFSET(edict_t, style) },
	{ ASLIB_PROPERTY_DECL(int, moveType), ASLIB_FOFFSET(edict_t, movetype) },
	{ ASLIB_PROPERTY_DECL(uint, nextThink), ASLIB_FOFFSET(edict_t, nextThink) },
	{ ASLIB_PROPERTY_DECL(float, health), ASLIB_FOFFSET(edict_t, health) },
	{ ASLIB_PROPERTY_DECL(int, maxHealth), ASLIB_FOFFSET(edict_t, max_health) },
	{ ASLIB_PROPERTY_DECL(int, viewHeight), ASLIB_FOFFSET(edict_t, viewheight) },
	{ ASLIB_PROPERTY_DECL(int, takeDamage), ASLIB_FOFFSET(edict_t, takedamage) },
	{ ASLIB_PROPERTY_DECL(int, damage), ASLIB_FOFFSET(edict_t, dmg) },
	{ ASLIB_PROPERTY_DECL(int, projectileMaxDamage), ASLIB_FOFFSET(edict_t, projectileInfo.maxDamage) },
	{ ASLIB_PROPERTY_DECL(int, projectileMinDamage), ASLIB_FOFFSET(edict_t, projectileInfo.minDamage) },
	{ ASLIB_PROPERTY_DECL(int, projectileMaxKnockback), ASLIB_FOFFSET(edict_t, projectileInfo.maxKnockback) },
	{ ASLIB_PROPERTY_DECL(int, projectileMinKnockback), ASLIB_FOFFSET(edict_t, projectileInfo.minKnockback) },
	{ ASLIB_PROPERTY_DECL(float, projectileSplashRadius), ASLIB_FOFFSET(edict_t, projectileInfo.radius) },
	{ ASLIB_PROPERTY_DECL(int, count), ASLIB_FOFFSET(edict_t, count) },
	{ ASLIB_PROPERTY_DECL(float, wait), ASLIB_FOFFSET(edict_t, wait) },
	{ ASLIB_PROPERTY_DECL(float, delay), ASLIB_FOFFSET(edict_t, delay) },
	{ ASLIB_PROPERTY_DECL(int, waterLevel), ASLIB_FOFFSET(edict_t, waterlevel) },
	{ ASLIB_PROPERTY_DECL(float, attenuation), ASLIB_FOFFSET(edict_t, attenuation) },
	{ ASLIB_PROPERTY_DECL(int, mass), ASLIB_FOFFSET(edict_t, mass) },
	{ ASLIB_PROPERTY_DECL(uint, timeStamp), ASLIB_FOFFSET(edict_t, timeStamp) },

	{ ASLIB_PROPERTY_DECL(entThink @, think), ASLIB_FOFFSET(edict_t, asThinkFunc ) },
	{ ASLIB_PROPERTY_DECL(entTouch @, touch), ASLIB_FOFFSET(edict_t, asTouchFunc ) },
	{ ASLIB_PROPERTY_DECL(entUse @, use), ASLIB_FOFFSET(edict_t, asUseFunc ) },
	{ ASLIB_PROPERTY_DECL(entPain @, pain), ASLIB_FOFFSET(edict_t, asPainFunc ) },
	{ ASLIB_PROPERTY_DECL(entDie @, die), ASLIB_FOFFSET(edict_t, asDieFunc ) },
	{ ASLIB_PROPERTY_DECL(entStop @, stop), ASLIB_FOFFSET(edict_t, asStopFunc ) },

	// specific for ET_PARTICLES
	{ ASLIB_PROPERTY_DECL(int, particlesSpeed), ASLIB_FOFFSET(edict_t, particlesInfo.speed) },
	{ ASLIB_PROPERTY_DECL(int, particlesShaderIndex), ASLIB_FOFFSET(edict_t, particlesInfo.shaderIndex) },
	{ ASLIB_PROPERTY_DECL(int, particlesSpread), ASLIB_FOFFSET(edict_t, particlesInfo.spread) },
	{ ASLIB_PROPERTY_DECL(int, particlesSize), ASLIB_FOFFSET(edict_t, particlesInfo.size) },
	{ ASLIB_PROPERTY_DECL(int, particlesTime), ASLIB_FOFFSET(edict_t, particlesInfo.time) },
	{ ASLIB_PROPERTY_DECL(int, particlesFrequency), ASLIB_FOFFSET(edict_t, particlesInfo.frequency) },
	{ ASLIB_PROPERTY_DECL(bool, particlesSpherical), ASLIB_FOFFSET(edict_t, particlesInfo.spherical) },
	{ ASLIB_PROPERTY_DECL(bool, particlesBounce), ASLIB_FOFFSET(edict_t, particlesInfo.bounce) },
	{ ASLIB_PROPERTY_DECL(bool, particlesGravity), ASLIB_FOFFSET(edict_t, particlesInfo.gravity) },
	{ ASLIB_PROPERTY_DECL(bool, particlesExpandEffect), ASLIB_FOFFSET(edict_t, particlesInfo.expandEffect) },
	{ ASLIB_PROPERTY_DECL(bool, particlesShrinkEffect), ASLIB_FOFFSET(edict_t, particlesInfo.shrinkEffect) },

	ASLIB_PROPERTY_NULL
};

static const asClassDescriptor_t asGameEntityClassDescriptor =
{
	"cEntity",					/* name */
	asOBJ_REF,					/* object type flags */
	sizeof( edict_t ),			/* size */
	gedict_Funcdefs,		/* funcdefs */
	gedict_ObjectBehaviors,		/* object behaviors */
	gedict_Methods,				/* methods */
	gedict_Properties,			/* properties */

	NULL, NULL					/* string factory hack */
};

//=======================================================================

static const asClassDescriptor_t * const asClassesDescriptors[] = 
{
	&asTraceClassDescriptor,
	&asItemClassDescriptor,
	&asMatchClassDescriptor,
	&asGametypeClassDescriptor,
	&asTeamListClassDescriptor,
	&asScoreStatsClassDescriptor,
	&asBotClassDescriptor,
	&asGameClientDescriptor,
	&asGameEntityClassDescriptor,

	NULL
};

/*
* G_RegisterObjectClasses
*/
static void G_RegisterObjectClasses( int asEngineHandle )
{
	int i, j;
	int error;
	const asClassDescriptor_t *cDescr;

	// first register all class names so methods using custom classes work
	for( i = 0; ; i++ )
	{
		if( !(cDescr = asClassesDescriptors[i]) )
			break;

		error = angelExport->asRegisterObjectType( asEngineHandle, cDescr->name, cDescr->size, cDescr->typeFlags );
	}

	// now register object and global behaviors, then methods and properties
	for( i = 0; ; i++ )
	{
		if( !(cDescr = asClassesDescriptors[i]) )
			break;

		// funcdefs
		if( cDescr->funcdefs )
		{
			for( j = 0; ; j++ )
			{
				const asFuncdef_t *funcdef = &cDescr->funcdefs[j];
				if( !funcdef->declaration )
					break;

				error = angelExport->asRegisterFuncdef( asEngineHandle, funcdef->declaration );
			}
		}

		// object behaviors
		if( cDescr->objBehaviors )
		{
			for( j = 0; ; j++ )
			{
				const asBehavior_t *objBehavior = &cDescr->objBehaviors[j];
				if( !objBehavior->declaration )
					break;

				if( level.gametype.asEngineIsGeneric ) {
					error = -1;
				}
				else {
					error = angelExport->asRegisterObjectBehaviour( asEngineHandle, cDescr->name, objBehavior->behavior, objBehavior->declaration, objBehavior->funcPointer, objBehavior->callConv );
				}
			}
		}

		// object methods
		if( cDescr->objMethods )
		{
			for( j = 0; ; j++ )
			{
				const asMethod_t *objMethod = &cDescr->objMethods[j];
				if( !objMethod->declaration )
					break;

				if( level.gametype.asEngineIsGeneric ) {
					error = -1;
				}
				else {
					error = angelExport->asRegisterObjectMethod( asEngineHandle, cDescr->name, objMethod->declaration, objMethod->funcPointer, objMethod->callConv );
				}
			}
		}

		// object properties
		if( cDescr->objProperties )
		{
			for( j = 0; ; j++ )
			{
				const asProperty_t *objProperty = &cDescr->objProperties[j];
				if( !objProperty->declaration )
					break;

				error = angelExport->asRegisterObjectProperty( asEngineHandle, cDescr->name, objProperty->declaration, objProperty->offset );
			}
		}
	}
}

//=======================================================================


static edict_t *asFunc_G_Spawn( asstring_t *classname )
{
	edict_t *ent;

	if( !level.canSpawnEntities )
	{
		G_Printf( "* WARNING: Spawning entities is disallowed during initialization. Returning null object\n" );
		return NULL;
	}

	ent = G_Spawn();

	if( classname && classname->len ) {
		ent->classname = G_RegisterLevelString( classname->buffer );
	}

	ent->scriptSpawned = qtrue;
	G_asClearEntityBehavoirs( ent );

	return ent;
}

static edict_t *asFunc_GetEntity( int entNum )
{
	if( entNum < 0 || entNum >= game.numentities )
		return NULL;

	return &game.edicts[ entNum ];
}

static gclient_t *asFunc_GetClient( int clientNum )
{
	if( clientNum < 0 || clientNum >= gs.maxclients )
		return NULL;

	return &game.clients[ clientNum ];
}

static g_teamlist_t *asFunc_GetTeamlist( int teamNum )
{
	if( teamNum < TEAM_SPECTATOR || teamNum >= GS_MAX_TEAMS )
		return NULL;

	return &teamlist[teamNum];
}

static gsitem_t *asFunc_GS_FindItemByTag( int tag )
{
	return GS_FindItemByTag( tag );
}

static gsitem_t *asFunc_GS_FindItemByName( asstring_t *name )
{
	return ( !name || !name->len ) ? NULL : GS_FindItemByName( name->buffer );
}

static gsitem_t *asFunc_GS_FindItemByClassname( asstring_t *name )
{
	return ( !name || !name->len ) ? NULL : GS_FindItemByName( name->buffer );
}

static void asFunc_G_Match_RemoveAllProjectiles( void )
{
	G_Match_RemoveAllProjectiles();
}

static void asFunc_G_Match_FreeBodyQueue( void )
{
	G_Match_FreeBodyQueue();
}

static void asFunc_G_Items_RespawnByType( unsigned int typeMask, int item_tag, float delay )
{
	G_Items_RespawnByType( typeMask, item_tag, delay );
}

static void asFunc_Print( const asstring_t *str )
{
	if( !str || !str->buffer )
		return;

	G_Printf( "%s", str->buffer );
}

static void asFunc_PrintMsg( edict_t *ent, asstring_t *str )
{
	if( !str || !str->buffer )
		return;

	G_PrintMsg( ent, "%s", str->buffer );
}

static void asFunc_CenterPrintMsg( edict_t *ent, asstring_t *str )
{
	if( !str || !str->buffer )
		return;

	G_CenterPrintMsg( ent, "%s", str->buffer );
}

static void asFunc_G_Sound( edict_t *owner, int channel, int soundindex, float attenuation )
{
	G_Sound( owner, channel, soundindex, attenuation );
}

static float asFunc_Random( void )
{
	return random();
}

static float asFunc_BRandom( float min, float max )
{
	return brandom( min, max );
}

static int asFunc_DirToByte( asvec3_t *vec )
{
	if( !vec )
		return 0;

	return DirToByte( vec->v );
}

static int asFunc_PointContents( asvec3_t *vec )
{
	if( !vec )
		return 0;

	return G_PointContents( vec->v );
}

static qboolean asFunc_InPVS( asvec3_t *origin1, asvec3_t *origin2 )
{
	return trap_inPVS( origin1->v, origin2->v );
}

static qboolean asFunc_WriteFile( asstring_t *path, asstring_t *data )
{
	int filehandle;

	if( !path || !path->len )
		return qfalse;
	if( !data || !data->buffer )
		return qfalse;

	if( trap_FS_FOpenFile( path->buffer, &filehandle, FS_WRITE ) == -1 )
		return qfalse;

	trap_FS_Write( data->buffer, data->len, filehandle );
	trap_FS_FCloseFile( filehandle );

	return qtrue;
}

static qboolean asFunc_AppendToFile( asstring_t *path, asstring_t *data )
{
	int filehandle;

	if( !path || !path->len )
		return qfalse;
	if( !data || !data->buffer )
		return qfalse;

	if( trap_FS_FOpenFile( path->buffer, &filehandle, FS_APPEND ) == -1 )
		return qfalse;

	trap_FS_Write( data->buffer, data->len, filehandle );
	trap_FS_FCloseFile( filehandle );

	return qtrue;
}

static asstring_t *asFunc_LoadFile( asstring_t *path )
{
	int filelen, filehandle;
	qbyte *buf = NULL;
	asstring_t *data;

	if( !path || !path->len )
		return angelExport->asStringFactoryBuffer( NULL, 0 );

	filelen = trap_FS_FOpenFile( path->buffer, &filehandle, FS_READ );
	if( filehandle && filelen > 0 )
	{
		buf = G_Malloc( filelen + 1 );
		filelen = trap_FS_Read( buf, filelen, filehandle );
	}

	trap_FS_FCloseFile( filehandle );

	if( !buf )
		return angelExport->asStringFactoryBuffer( NULL, 0 );

	data = angelExport->asStringFactoryBuffer( (char *)buf, filelen );
	G_Free( buf );

	return data;
}

// racesow

// G_Md5
static asstring_t *asFunc_G_Md5( asstring_t *in )
{
	md5_state_t state;
	md5_byte_t digest[16];
	char hex_output[16*2 + 1];
	int di;

		md5_init(&state);
		md5_append(&state, (const md5_byte_t *)in->buffer, in->len);
		md5_finish(&state, digest);
		for (di = 0; di < 16; ++di)
			sprintf(hex_output + di * 2, "%02x", digest[di]);
	return angelExport->asStringFactoryBuffer(hex_output, strlen(hex_output));
}

/*static void asFunc_asGeneric_G_Md5( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, asFunc_G_Md5( G_asGeneric_GetArgAddress( gen, 0 ) ) );
}*/

// RS_MysqlMapFilter
static qboolean asFunc_RS_MapFilter(int player_id,asstring_t *filter,unsigned int page )
{
	return RS_MapFilter( player_id, filter->buffer, page);
}
/*static void asFunc_asGeneric_RS_MapFilter( void *gen )
{
	G_asGeneric_SetReturnBool( gen, asFunc_RS_MapFilter(
			(int)G_asGeneric_GetArgInt(gen, 0),
			(asstring_t *)G_asGeneric_GetArgAddress(gen, 1),
			(unsigned int)G_asGeneric_GetArgInt(gen, 2)));
}*/

// RS_UpdateMapList
static qboolean asFunc_RS_UpdateMapList( int playerNum )
{
	 return RS_UpdateMapList( playerNum );
}

/*static void asFunc_asGeneric_RS_UpdateMapList( void *gen )
{
	G_asGeneric_SetReturnBool( gen, asFunc_RS_UpdateMapList(
			(int)G_asGeneric_GetArgInt(gen, 0)
	));
}*/

// RS_LoadStats
static qboolean asFunc_RS_LoadStats(int player_id, asstring_t *what, asstring_t *which)
{
	 return RS_LoadStats( player_id, what->buffer, which->buffer);
}

/*static void asFunc_asGeneric_RS_LoadStats( void *gen )
{
	G_asGeneric_SetReturnBool( gen, asFunc_RS_LoadStats(
			(int)G_asGeneric_GetArgInt(gen, 0),
			(asstring_t *)G_asGeneric_GetArgAddress(gen, 1),
			(asstring_t *)G_asGeneric_GetArgAddress(gen, 2)));
}*/

// RS_MysqlMapFilterCallback
static asstring_t *asFunc_RS_PrintQueryCallback(int player_id )
{
	char *result;
	asstring_t *result_as;
	result = RS_PrintQueryCallback(player_id);
	result_as=angelExport->asStringFactoryBuffer(result, strlen(result));
	free(result);
	return result_as;
}

/*static void asFunc_asGeneric_RS_PrintQueryCallback( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, asFunc_RS_PrintQueryCallback(
			(unsigned int)G_asGeneric_GetArgInt(gen, 0)));
}*/

// RS_Maplist
static qboolean asFunc_RS_Maplist(int player_id, unsigned int page )
{
	 return RS_Maplist( player_id, page);
}

/*static void asFunc_asGeneric_RS_Maplist( void *gen )
{
	G_asGeneric_SetReturnBool( gen, asFunc_RS_Maplist(
			(int)G_asGeneric_GetArgInt(gen, 0),
			(unsigned int)G_asGeneric_GetArgInt(gen, 1)));
}*/

// RS_NextMap
static asstring_t *asFunc_RS_NextMap( )
{
	char *result;
	result = RS_ChooseNextMap( );
	return angelExport->asStringFactoryBuffer(result, strlen(result));
}

/*static void asFunc_asGeneric_RS_NextMap( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, asFunc_RS_NextMap( ) );
}*/

// RS_LastMap
static asstring_t *asFunc_RS_LastMap( )
{
	char *result;
	result = previousMapName;
	return angelExport->asStringFactoryBuffer(result, strlen(result));
}

/*static void asFunc_asGeneric_RS_LastMap( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, asFunc_RS_LastMap( ) );
}*/

// RS_LoadMapList
static void asFunc_RS_LoadMapList( int is_freestyle )
{
	RS_LoadMaplist( is_freestyle );
}

/*static void asFunc_asGeneric_RS_LoadMapList( void *gen )
{
	asFunc_RS_LoadMapList( (int)G_asGeneric_GetArgInt( gen, 0 ) );
}*/

// RS_removeProjectiles
static void asFunc_RS_removeProjectiles( edict_t *owner )
{
	RS_removeProjectiles( owner );
}

/*static void asFunc_asGeneric_RS_removeProjectiles( void *gen )
{
	edict_t *owner = (edict_t *)G_asGeneric_GetArgAddress( gen, 0 );
	RS_removeProjectiles( owner );
}*/

// RS_QueryPjState
static qboolean asFunc_RS_QueryPjState(int playerNum)
{
	 return RS_QueryPjState(playerNum);
}

/*static void asFunc_asGeneric_RS_QueryPjState( void *gen )
{
	G_asGeneric_SetReturnBool( gen, asFunc_RS_QueryPjState(
			(int)G_asGeneric_GetArgInt(gen, 0)));
}*/

// RS_ResetPjState
static qboolean asFunc_RS_ResetPjState(int playerNum)
{
	RS_ResetPjState(playerNum);
	return qtrue;
}

/*static void asFunc_asGeneric_RS_ResetPjState( void *gen )
{
	G_asGeneric_SetReturnBool( gen, asFunc_RS_ResetPjState(
			(int)G_asGeneric_GetArgInt(gen, 0)));
}*/

// RS_MysqlLoadMap
static qboolean asFunc_RS_MysqlLoadMap()
{
	return RS_MysqlLoadMap();
}

/*static void asFunc_asGeneric_RS_MysqlLoadMap( void *gen )
{
	G_asGeneric_SetReturnBool(gen, asFunc_RS_MysqlLoadMap());
}*/

// RS_MysqlInsertRace
static qboolean asFunc_RS_MysqlInsertRace( int player_id, int nick_id, int map_id, int race_time, int playerNum, int tries, int duration, asstring_t *checkpoints, qboolean prejumped)
{
	return RS_MysqlInsertRace(player_id, nick_id, map_id, race_time, playerNum, tries, duration, checkpoints->buffer, prejumped );
}

/*static void asFunc_asGeneric_RS_MysqlInsertRace( void *gen )
{
	G_asGeneric_SetReturnBool(gen, asFunc_RS_MysqlInsertRace(
		(int)G_asGeneric_GetArgInt(gen, 0),
		(int)G_asGeneric_GetArgInt(gen, 1),
		(int)G_asGeneric_GetArgInt(gen, 2),
		(int)G_asGeneric_GetArgInt(gen, 3),
		(int)G_asGeneric_GetArgInt(gen, 4),
		(int)G_asGeneric_GetArgInt(gen, 5),
		(int)G_asGeneric_GetArgInt(gen, 6),
		(asstring_t *)G_asGeneric_GetArgAddress(gen, 7),
		(qboolean)G_asGeneric_GetArgBool(gen, 8)));
}*/

// RS_MysqlPlayerAppear
static qboolean asFunc_RS_MysqlPlayerAppear( asstring_t *playerName, int playerNum, int player_id, int map_id, int is_authed, asstring_t *authName, asstring_t *authPass, asstring_t *authToken)
{
	return RS_MysqlPlayerAppear(playerName->buffer, playerNum, player_id, map_id, is_authed, authName->buffer, authPass->buffer, authToken->buffer);
}

/*static void asFunc_asGeneric_RS_MysqlPlayerAppear( void *gen )
{
	G_asGeneric_SetReturnBool(gen, asFunc_RS_MysqlPlayerAppear(
		(asstring_t *)G_asGeneric_GetArgAddress(gen, 0),
		(int)G_asGeneric_GetArgInt(gen, 1),
		(int)G_asGeneric_GetArgInt(gen, 2),
		(int)G_asGeneric_GetArgInt(gen, 3),
		(int)G_asGeneric_GetArgInt(gen, 4),
		(asstring_t *)G_asGeneric_GetArgAddress(gen, 5),
		(asstring_t *)G_asGeneric_GetArgAddress(gen, 6),
		(asstring_t *)G_asGeneric_GetArgAddress(gen, 7)));
}*/

// RS_MysqlPlayerDisappear
static qboolean asFunc_RS_MysqlPlayerDisappear( asstring_t *playerName, int playtime, int overall_tries, int racing_time, int player_id, int nick_id, int map_id, qboolean is_authed, qboolean is_threaded)
{
	return RS_MysqlPlayerDisappear(playerName->buffer, playtime, overall_tries, racing_time, player_id, nick_id, map_id, (int)(is_authed==qtrue), (int)(is_threaded==qtrue) );
}

/*static void asFunc_asGeneric_RS_MysqlPlayerDisappear( void *gen )
{
	G_asGeneric_SetReturnBool(gen, asFunc_RS_MysqlPlayerDisappear(
		(asstring_t *)G_asGeneric_GetArgAddress(gen, 0),
		(int)G_asGeneric_GetArgInt(gen, 1),
		(int)G_asGeneric_GetArgInt(gen, 2),
		(int)G_asGeneric_GetArgInt(gen, 3),
		(int)G_asGeneric_GetArgInt(gen, 4),
		(int)G_asGeneric_GetArgInt(gen, 5),
		(int)G_asGeneric_GetArgInt(gen, 6),
		(qboolean)G_asGeneric_GetArgBool(gen, 7),
		(qboolean)G_asGeneric_GetArgBool(gen, 8)));
}*/

// RS_GetPlayerNick
static qboolean asFunc_RS_GetPlayerNick( int playerNum, int player_id )
{
	return RS_GetPlayerNick(playerNum, player_id);
}

/*static void asFunc_asGeneric_RS_GetPlayerNick( void *gen )
{
	G_asGeneric_SetReturnBool(gen, asFunc_RS_GetPlayerNick(
		(int)G_asGeneric_GetArgInt(gen, 0),
		(int)G_asGeneric_GetArgInt(gen, 1)));
}*/

// RS_UpdatePlayerNick
static qboolean asFunc_RS_UpdatePlayerNick( asstring_t * name, int playerNum, int player_id )
{
	return RS_UpdatePlayerNick(name->buffer, playerNum, player_id);
}

/*static void asFunc_asGeneric_RS_UpdatePlayerNick( void *gen )
{
	G_asGeneric_SetReturnBool(gen, asFunc_RS_UpdatePlayerNick(
		(asstring_t *)G_asGeneric_GetArgAddress(gen, 0),
		(int)G_asGeneric_GetArgInt(gen, 1),
		(int)G_asGeneric_GetArgInt(gen, 2)));
}*/

// RS_MysqlLoadHighScores
static qboolean asFunc_RS_MysqlLoadHighscores( int playerNum, int limit, int map_id, asstring_t *mapname, pjflag prejumped )
{
	return RS_MysqlLoadHighscores(playerNum, limit, map_id, mapname->buffer, prejumped);
}

/*static void asFunc_asGeneric_RS_MysqlLoadHighscores( void *gen )
{
	G_asGeneric_SetReturnBool(gen, asFunc_RS_MysqlLoadHighscores(
		(int)G_asGeneric_GetArgInt(gen, 0),
		(int)G_asGeneric_GetArgInt(gen, 1),
		(int)G_asGeneric_GetArgInt(gen, 2),
		(asstring_t *)G_asGeneric_GetArgAddress(gen, 3),
		(int)G_asGeneric_GetArgInt(gen, 4)));
}*/

// RS_MysqlLoadRanking
static qboolean asFunc_RS_MysqlLoadRanking( int playerNum, int page, asstring_t *order )
{
	return RS_MysqlLoadRanking(playerNum, page, order->buffer);
}

/*static void asFunc_asGeneric_RS_MysqlLoadRanking( void *gen )
{
	G_asGeneric_SetReturnBool(gen, asFunc_RS_MysqlLoadRanking(
		(int)G_asGeneric_GetArgInt(gen, 0),
		(int)G_asGeneric_GetArgInt(gen, 1),
		(asstring_t *)G_asGeneric_GetArgAddress(gen, 2)));
}*/

// RS_MysqlSetOneliner
static qboolean asFunc_RS_MysqlSetOneliner( int playerNum, int player_id, int map_id, asstring_t *oneliner)
{
	return RS_MysqlSetOneliner(playerNum, player_id, map_id, oneliner->buffer);
}

/*static void asFunc_asGeneric_RS_MysqlSetOneliner( void *gen )
{
	G_asGeneric_SetReturnBool(gen, asFunc_RS_MysqlSetOneliner(
		(int)G_asGeneric_GetArgInt(gen, 0),
		(int)G_asGeneric_GetArgInt(gen, 1),
		(int)G_asGeneric_GetArgInt(gen, 2),
		(asstring_t *)G_asGeneric_GetArgAddress(gen, 3)));
}*/


// RS_PopCallbackQueue
static qboolean asFunc_RS_PopCallbackQueue( int *command, int *arg1, int *arg2, int *arg3, int *arg4, int *arg5, int *arg6, int *arg7 )
{
	return RS_PopCallbackQueue(command, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
}

/*static void asFunc_asGeneric_RS_PopCallbackQueue( void *gen )
{
	G_asGeneric_SetReturnBool(gen, asFunc_RS_PopCallbackQueue(
		(int *)G_asGeneric_GetArgAddress(gen, 0),
		(int *)G_asGeneric_GetArgAddress(gen, 1),
		(int *)G_asGeneric_GetArgAddress(gen, 2),
		(int *)G_asGeneric_GetArgAddress(gen, 3),
		(int *)G_asGeneric_GetArgAddress(gen, 4),
		(int *)G_asGeneric_GetArgAddress(gen, 5),
		(int *)G_asGeneric_GetArgAddress(gen, 6),
		(int *)G_asGeneric_GetArgAddress(gen, 7)));
}*/
// !racesow

static int asFunc_FileLength( asstring_t *path )
{
	if( !path || !path->len )
		return qfalse;

	return ( trap_FS_FOpenFile( path->buffer, NULL, FS_READ ) );
}

static void asFunc_Cmd_ExecuteText( asstring_t *str )
{
	if( !str || !str->buffer || !str->buffer[0] )
		return;

	trap_Cmd_ExecuteText( EXEC_APPEND, str->buffer );
}

static qboolean asFunc_ML_FilenameExists( asstring_t *filename )
{
	return trap_ML_FilenameExists( filename->buffer );
}

static asstring_t *asFunc_ML_GetMapByNum( int num )
{
	char mapname[MAX_QPATH];
	asstring_t *data;

	if( !trap_ML_GetMapByNum( num, mapname, sizeof( mapname ) ) )
		return NULL;

	data = angelExport->asStringFactoryBuffer( (char *)mapname, strlen( mapname ) );
	return data;
}

static asstring_t *asFunc_LocationName( asvec3_t *origin )
{
	char buf[MAX_CONFIGSTRING_CHARS];
	asstring_t *data;

	G_MapLocationNameForTAG( G_MapLocationTAGForOrigin( origin->v ), buf, sizeof( buf ) );

	data = angelExport->asStringFactoryBuffer( (char *)buf, strlen( buf ) );
	return data;
}

static int asFunc_LocationTag( asstring_t *str )
{
	return G_MapLocationTAGForName( str->buffer );
}

static asstring_t *asFunc_LocationForTag( int tag )
{
	char buf[MAX_CONFIGSTRING_CHARS];
	asstring_t *data;

	G_MapLocationNameForTAG( tag, buf, sizeof( buf ) );

	data = angelExport->asStringFactoryBuffer( (char *)buf, strlen( buf ) );

	return data;
}

static int asFunc_ImageIndex( asstring_t *str )
{
	if( !str || !str->buffer )
		return 0;

	return trap_ImageIndex( str->buffer );
}

static int asFunc_SkinIndex( asstring_t *str )
{
	if( !str || !str->buffer )
		return 0;

	return trap_SkinIndex( str->buffer );
}

static int asFunc_ModelIndexExt( asstring_t *str, qboolean pure )
{
	int index;

	if( !str || !str->buffer )
		return 0;

	index = trap_ModelIndex( str->buffer );
	if( index && pure )
		G_PureModel( str->buffer );

	return index;
}

static int asFunc_ModelIndex( asstring_t *str )
{
	return asFunc_ModelIndexExt( str, qfalse );
}

static int asFunc_SoundIndexExt( asstring_t *str, qboolean pure )
{
	int index;

	if( !str || !str->buffer )
		return 0;

	index = trap_SoundIndex( str->buffer );
	if( index && pure )
		G_PureSound( str->buffer );

	return index;
}

static int asFunc_SoundIndex( asstring_t *str )
{
	return asFunc_SoundIndexExt( str, qfalse );
}

static void asFunc_RegisterCommand( asstring_t *str )
{
	if( !str || !str->buffer || !str->len )
		return;

	G_AddCommand( str->buffer, NULL );
}

static void asFunc_RegisterCallvote( asstring_t *asname, asstring_t *asusage, asstring_t *ashelp )
{
	if( !asname || !asname->buffer || !asname->buffer[0]  )
		return;

	G_RegisterGametypeScriptCallvote( asname->buffer, asusage ? asusage->buffer : NULL, ashelp ? ashelp->buffer : NULL );
}

static void asFunc_ConfigString( int index, asstring_t *str )
{
	if( !str || !str->buffer )
		return;

	// write protect some configstrings
	if( index <= CS_POWERUPEFFECTS
		|| index == CS_AUTORECORDSTATE
		|| index == CS_MAXCLIENTS
		|| index == CS_WORLDMODEL
		|| index == CS_MAPCHECKSUM )
	{
		G_Printf( "WARNING: ConfigString %i is write protected\n", index );
		return;
	}

	// prevent team name exploits
	if( index >= CS_TEAM_SPECTATOR_NAME && index < CS_TEAM_SPECTATOR_NAME + GS_MAX_TEAMS )
	{
		qboolean forbidden = qfalse;

		// never allow to change spectator and player teams names
		if( index < CS_TEAM_ALPHA_NAME )
		{
			G_Printf( "WARNING: %s team name is write protected\n", GS_DefaultTeamName( index - CS_TEAM_SPECTATOR_NAME ) );
			return;
		}

		// don't allow empty team names
		if( !strlen( str->buffer ) )
		{
			G_Printf( "WARNING: empty team names are not allowed\n" );
			return;
		}

		// never allow to change alpha and beta team names to a different team default name
		if( index == CS_TEAM_ALPHA_NAME )
		{
			if( !Q_stricmp( str->buffer, GS_DefaultTeamName( TEAM_SPECTATOR ) ) )
				forbidden = qtrue;

			if( !Q_stricmp( str->buffer, GS_DefaultTeamName( TEAM_PLAYERS ) ) )
				forbidden = qtrue;

			if( !Q_stricmp( str->buffer, GS_DefaultTeamName( TEAM_BETA ) ) )
				forbidden = qtrue;
		}

		if( index == CS_TEAM_BETA_NAME )
		{
			if( !Q_stricmp( str->buffer, GS_DefaultTeamName( TEAM_SPECTATOR ) ) )
				forbidden = qtrue;

			if( !Q_stricmp( str->buffer, GS_DefaultTeamName( TEAM_PLAYERS ) ) )
				forbidden = qtrue;

			if( !Q_stricmp( str->buffer, GS_DefaultTeamName( TEAM_ALPHA ) ) )
				forbidden = qtrue;
		}

		if( forbidden )
		{
			G_Printf( "WARNING: %s team name can not be changed to %s\n", GS_DefaultTeamName( index - CS_TEAM_SPECTATOR_NAME ), str->buffer );
			return;
		}
	}

	trap_ConfigString( index, str->buffer );
}

static edict_t *asFunc_FindEntityInRadiusExt( edict_t *from, edict_t *to, asvec3_t *org, float radius )
{
	if( !org )
		return NULL;
	return G_FindBoxInRadius( from, to, org->v, radius );
}

static edict_t *asFunc_FindEntityInRadius( edict_t *from, asvec3_t *org, float radius )
{
	return asFunc_FindEntityInRadiusExt( from, NULL, org, radius );
}

static edict_t *asFunc_FindEntityWithClassname( edict_t *from, asstring_t *str )
{
	if( !str || !str->buffer )
		return NULL;

	return G_Find( from, FOFS( classname ), str->buffer );
}

static void asFunc_PositionedSound( asvec3_t *origin, int channel, int soundindex, float attenuation )
{
	if( !origin )
		return;

	G_PositionedSound( origin->v, channel, soundindex, attenuation );
}

static void asFunc_G_GlobalSound( int channel, int soundindex )
{
	G_GlobalSound( channel, soundindex );
}

static void asFunc_G_AnnouncerSound( gclient_t *target, int soundindex, int team, qboolean queued, gclient_t *ignore )
{
	edict_t *ent = NULL, *passent = NULL;
	int playerNum;

	if( target && !target->asFactored )
	{
		playerNum = target - game.clients;

		if( playerNum < 0 || playerNum >= gs.maxclients )
			return;

		ent = game.edicts + playerNum + 1;
	}

	if( ignore && !ignore->asFactored )
	{
		playerNum = ignore - game.clients;

		if( playerNum >= 0 && playerNum < gs.maxclients )
			passent = game.edicts + playerNum + 1;
	}


	G_AnnouncerSound( ent, soundindex, team, queued, passent );
}

static asstring_t *asFunc_G_SpawnTempValue( asstring_t *key )
{
	const char *val;

	if( !key )
		return angelExport->asStringFactoryBuffer( NULL, 0 );

	if( level.spawning_entity == NULL )
		G_Printf( "WARNING: G_SpawnTempValue: Spawn temp values can only be grabbed during the entity spawning process\n" );

	val = G_GetEntitySpawnKey( key->buffer, level.spawning_entity );

	return angelExport->asStringFactoryBuffer( val, strlen( val ) );
}

static void asFunc_FireInstaShot( asvec3_t *origin, asvec3_t *angles, int range, int damage, int knockback, int stun, edict_t *owner )
{
	W_Fire_Instagun( owner, origin->v, angles->v, damage, knockback, stun, 0, range, MOD_INSTAGUN_S, 0 );
}

static edict_t *asFunc_FireWeakBolt( asvec3_t *origin, asvec3_t *angles, int speed, int damage, int knockback, int stun, edict_t *owner )
{
	return W_Fire_Electrobolt_Weak( owner, origin->v, angles->v, speed, damage, min( 1, knockback ), knockback, stun, 5000, MOD_ELECTROBOLT_W, 0 );
}

static void asFunc_FireStrongBolt( asvec3_t *origin, asvec3_t *angles, int range, int damage, int knockback, int stun, edict_t *owner )
{
	W_Fire_Electrobolt_FullInstant( owner, origin->v, angles->v, damage, damage, knockback, knockback, stun, range, range, MOD_ELECTROBOLT_S, 0 );
}

static edict_t *asFunc_FirePlasma( asvec3_t *origin, asvec3_t *angles, int speed, int radius, int damage, int knockback, int stun, edict_t *owner )
{
	return W_Fire_Plasma( owner, origin->v, angles->v, damage, min( 1, knockback ), knockback, stun, min( 1, damage ), radius, speed, 5000, MOD_PLASMA_S, 0 );
}

static edict_t *asFunc_FireRocket( asvec3_t *origin, asvec3_t *angles, int speed, int radius, int damage, int knockback, int stun, edict_t *owner )
{
	return W_Fire_Rocket( owner, origin->v, angles->v, speed, damage, min( 1, knockback ), knockback, stun, min( 1, damage ), radius, 5000, MOD_ROCKET_S, 0 );
}

static edict_t *asFunc_FireGrenade( asvec3_t *origin, asvec3_t *angles, int speed, int radius, int damage, int knockback, int stun, edict_t *owner )
{
	return W_Fire_Grenade( owner, origin->v, angles->v, speed, damage, min( 1, knockback ), knockback, stun, min( 1, damage ), radius, 5000, MOD_GRENADE_S, 0, qfalse );
}

static void asFunc_FireRiotgun( asvec3_t *origin, asvec3_t *angles, int range, int spread, int count, int damage, int knockback, int stun, edict_t *owner )
{
	W_Fire_Riotgun( owner, origin->v, angles->v, rand() & 255, range, spread, count, damage, knockback, stun, MOD_RIOTGUN_S, 0 );
}

static void asFunc_FireBullet( asvec3_t *origin, asvec3_t *angles, int range, int spread, int damage, int knockback, int stun, edict_t *owner )
{
	W_Fire_Bullet( owner, origin->v, angles->v, rand() & 255, range, spread, damage, knockback, stun, MOD_MACHINEGUN_S, 0 );
}

static edict_t *asFunc_FireBlast( asvec3_t *origin, asvec3_t *angles, int speed, int radius, int damage, int knockback, int stun, edict_t *owner )
{
	return W_Fire_GunbladeBlast( owner, origin->v, angles->v, damage, min( 1, knockback ), knockback, stun, min( 1, damage ), radius, speed, 5000, MOD_SPLASH, 0 );
}

static const asglobfuncs_t asGlobFuncs[] =
{

	// racesow
	{ "String @G_Md5( String & )", asFunc_G_Md5/*, asFunc_asGeneric_G_Md5*/ },
	{ "bool RS_MysqlPlayerAppear( String &, int, int, int, bool, String &, String &, String & )", asFunc_RS_MysqlPlayerAppear/*, asFunc_asGeneric_RS_MysqlPlayerAppear*/ },
	{ "bool RS_MysqlPlayerDisappear( String &, int, int, int, int, int, int, bool, bool )", asFunc_RS_MysqlPlayerDisappear/*, asFunc_asGeneric_RS_MysqlPlayerDisappear*/ },
	{ "bool RS_GetPlayerNick( int, int )", asFunc_RS_GetPlayerNick/*, asFunc_asGeneric_RS_GetPlayerNick*/ },
	{ "bool RS_UpdatePlayerNick( String &, int, int )", asFunc_RS_UpdatePlayerNick/*, asFunc_asGeneric_RS_UpdatePlayerNick*/ },
	{ "bool RS_MysqlLoadMap()", asFunc_RS_MysqlLoadMap/*, asFunc_asGeneric_RS_MysqlLoadMap*/ },
	{ "bool RS_MysqlInsertRace( int, int, int, int, int, int, int, String &, bool )", asFunc_RS_MysqlInsertRace/*, asFunc_asGeneric_RS_MysqlInsertRace*/ },
	{ "bool RS_MysqlLoadHighscores( int, int, int, String &, int)", asFunc_RS_MysqlLoadHighscores/*, asFunc_asGeneric_RS_MysqlLoadHighscores*/ },
	{ "bool RS_MysqlLoadRanking( int, int, String & )", asFunc_RS_MysqlLoadRanking/*, asFunc_asGeneric_RS_MysqlLoadRanking*/ },
	{ "bool RS_MysqlSetOneliner( int, int, int, String &)", asFunc_RS_MysqlSetOneliner/*, asFunc_asGeneric_RS_MysqlSetOneliner*/ },
	{ "bool RS_PopCallbackQueue( int &out, int &out, int &out, int &out, int &out, int &out, int &out, int &out )", asFunc_RS_PopCallbackQueue/*, asFunc_asGeneric_RS_PopCallbackQueue*/ },
	{ "bool RS_MapFilter( int, String &, int )", asFunc_RS_MapFilter/*, asFunc_asGeneric_RS_MapFilter*/},
	{ "bool RS_Maplist( int, int )", asFunc_RS_Maplist/*, asFunc_asGeneric_RS_Maplist*/},
	{ "bool RS_UpdateMapList( int playerNum)", asFunc_RS_UpdateMapList/*, asFunc_asGeneric_RS_UpdateMapList*/},
	{ "bool RS_LoadStats( int playerNum, String &, String & )", asFunc_RS_LoadStats/*, asFunc_asGeneric_RS_LoadStats*/},
	{ "String @RS_PrintQueryCallback( int )", asFunc_RS_PrintQueryCallback/*, asFunc_asGeneric_RS_PrintQueryCallback*/ },
	{ "String @RS_NextMap()", asFunc_RS_NextMap/*, asFunc_asGeneric_RS_NextMap*/ },
	{ "String @RS_LastMap()", asFunc_RS_LastMap/*, asFunc_asGeneric_RS_LastMap*/ },
	{ "void RS_LoadMapList( int )", asFunc_RS_LoadMapList/*, asFunc_asGeneric_RS_LoadMapList*/},
	{ "bool RS_QueryPjState( int playerNum)", asFunc_RS_QueryPjState/*, asFunc_asGeneric_RS_QueryPjState*/},
	{ "bool RS_ResetPjState( int playerNum)", asFunc_RS_ResetPjState/*, asFunc_asGeneric_RS_ResetPjState*/},
	// !racesow

	{ "cEntity @G_SpawnEntity( const String &in )", asFunc_G_Spawn },
	{ "String @G_SpawnTempValue( const String &in )", asFunc_G_SpawnTempValue },
	{ "cEntity @G_GetEntity( int entNum )", asFunc_GetEntity },
	{ "cClient @G_GetClient( int clientNum )", asFunc_GetClient },
	{ "cTeam @G_GetTeam( int team )", asFunc_GetTeamlist },
	{ "cItem @G_GetItem( int tag )", asFunc_GS_FindItemByTag },
	{ "cItem @G_GetItemByName( const String &in name )", asFunc_GS_FindItemByName },
	{ "cItem @G_GetItemByClassname( const String &in name )", asFunc_GS_FindItemByClassname },
	{ "cEntity @G_FindEntityInRadius( cEntity @, cEntity @, const Vec3 &in, float radius )", asFunc_FindEntityInRadiusExt },
	{ "cEntity @G_FindEntityInRadius( cEntity @, const Vec3 &in, float radius )", asFunc_FindEntityInRadius },
	{ "cEntity @G_FindEntityWithClassname( cEntity @, const String &in )", asFunc_FindEntityWithClassname },
	{ "cEntity @G_FindEntityWithClassName( cEntity @, const String &in )", asFunc_FindEntityWithClassname },

	// misc management utils
	{ "void G_RemoveAllProjectiles()", asFunc_G_Match_RemoveAllProjectiles },
	{ "void removeProjectiles( cEntity @ )", asFunc_RS_removeProjectiles/*, asFunc_asGeneric_RS_removeProjectiles*/ }, //racesow
	{ "void G_RemoveDeadBodies()", asFunc_G_Match_FreeBodyQueue },
	{ "void G_Items_RespawnByType( uint typeMask, int item_tag, float delay )", asFunc_G_Items_RespawnByType },

	// misc
	{ "void G_Print( const String &in )", asFunc_Print },
	{ "void G_PrintMsg( cEntity @, const String &in )", asFunc_PrintMsg },
	{ "void G_CenterPrintMsg( cEntity @, const String &in )", asFunc_CenterPrintMsg },
	{ "void G_Sound( cEntity @, int channel, int soundindex, float attenuation )", asFunc_G_Sound },
	{ "void G_PositionedSound( const Vec3 &in, int channel, int soundindex, float attenuation )", asFunc_PositionedSound },
	{ "void G_GlobalSound( int channel, int soundindex )", asFunc_G_GlobalSound },
	{ "void G_AnnouncerSound( cClient @, int soundIndex, int team, bool queued, cClient @ )", asFunc_G_AnnouncerSound },
	{ "float random()", asFunc_Random },
	{ "float brandom( float min, float max )", asFunc_BRandom },
	{ "int G_DirToByte( const Vec3 &in origin )", asFunc_DirToByte },
	{ "int G_PointContents( const Vec3 &in origin )", asFunc_PointContents },
	{ "bool G_InPVS( const Vec3 &in origin1, const Vec3 &in origin2 )", asFunc_InPVS },
	{ "bool G_WriteFile( const String &, const String & )", asFunc_WriteFile },
	{ "bool G_AppendToFile( const String &, const String & )", asFunc_AppendToFile },
	{ "String @G_LoadFile( const String & )", asFunc_LoadFile },
	{ "int G_FileLength( const String & )", asFunc_FileLength },
	{ "void G_CmdExecute( const String & )", asFunc_Cmd_ExecuteText },
	{ "String @G_LocationName( const Vec3 &in origin )", asFunc_LocationName },
	{ "int G_LocationTag( const String & )", asFunc_LocationTag },
	{ "String @G_LocationName( int tag )", asFunc_LocationForTag },

	{ "void __G_CallThink( cEntity @ent )", G_CallThink, &asEntityCallThinkFuncPtr },
	{ "void __G_CallTouch( cEntity @ent, cEntity @other, const Vec3 planeNormal, int surfFlags )", G_CallTouch, &asEntityCallTouchFuncPtr },
	{ "void __G_CallUse( cEntity @ent, cEntity @other, cEntity @activator )", G_CallUse, &asEntityCallUseFuncPtr },
	{ "void __G_CallStop( cEntity @ent )", G_CallStop, &asEntityCallStopFuncPtr },
	{ "void __G_CallPain( cEntity @ent, cEntity @other, float kick, float damage )", G_CallPain, &asEntityCallPainFuncPtr },
	{ "void __G_CallDie( cEntity @ent, cEntity @inflicter, cEntity @attacker )", G_CallDie, &asEntityCallDieFuncPtr },

	{ "int G_ImageIndex( const String &in )", asFunc_ImageIndex },
	{ "int G_SkinIndex( const String &in )", asFunc_SkinIndex },
	{ "int G_ModelIndex( const String &in )", asFunc_ModelIndex },
	{ "int G_SoundIndex( const String &in )", asFunc_SoundIndex },
	{ "int G_ModelIndex( const String &in, bool pure )", asFunc_ModelIndexExt },
	{ "int G_SoundIndex( const String &in, bool pure )", asFunc_SoundIndexExt },
	{ "void G_RegisterCommand( const String &in )", asFunc_RegisterCommand },
	{ "void G_RegisterCallvote( const String &in, const String &in, const String &in )", asFunc_RegisterCallvote },
	{ "void G_ConfigString( int index, const String &in )", asFunc_ConfigString },

	// projectile firing
	{ "void G_FireInstaShot( const Vec3 &in origin, const Vec3 &in angles, int range, int damage, int knockback, int stun, cEntity @owner )", asFunc_FireInstaShot },
	{ "cEntity @G_FireWeakBolt( const Vec3 &in origin, const Vec3 &in angles, int speed, int damage, int knockback, int stun, cEntity @owner )", asFunc_FireWeakBolt },
	{ "void G_FireStrongBolt( const Vec3 &in origin, const Vec3 &in angles, int range, int damage, int knockback, int stun, cEntity @owner )", asFunc_FireStrongBolt },
	{ "cEntity @G_FirePlasma( const Vec3 &in origin, const Vec3 &in angles, int speed, int radius, int damage, int knockback, int stun, cEntity @owner )", asFunc_FirePlasma },
	{ "cEntity @G_FireRocket( const Vec3 &in origin, const Vec3 &in angles, int speed, int radius, int damage, int knockback, int stun, cEntity @owner )", asFunc_FireRocket },
	{ "cEntity @G_FireGrenade( const Vec3 &in origin, const Vec3 &in angles, int speed, int radius, int damage, int knockback, int stun, cEntity @owner )", asFunc_FireGrenade },
	{ "void G_FireRiotgun( const Vec3 &in origin, const Vec3 &in angles, int range, int spread, int count, int damage, int knockback, int stun, cEntity @owner )", asFunc_FireRiotgun },
	{ "void G_FireBullet( const Vec3 &in origin, const Vec3 &in angles, int range, int spread, int damage, int knockback, int stun, cEntity @owner )", asFunc_FireBullet },
	{ "cEntity @G_FireBlast( const Vec3 &in origin, const Vec3 &in angles, int speed, int radius, int damage, int knockback, int stun, cEntity @owner )", asFunc_FireBlast },

	{ "bool ML_FilenameExists( String & )", asFunc_ML_FilenameExists },
	{ "String @ML_GetMapByNum( int num )", asFunc_ML_GetMapByNum },

	{ NULL, NULL }
};

static const asglobproperties_t asGlobProps[] =
{
	{ "const uint levelTime", &level.time },
	{ "const uint frameTime", &game.frametime },
	{ "const uint realTime", &game.realtime },
	//{ "const uint serverTime", &game.serverTime }, // I think this one isn't script business
	{ "const uint serverTime", &game.serverTime }, // racesow //FIXME: Well do we think that too? -K1ll
	{ "const uint64 localTime", &game.localTime },
	{ "const int maxEntities", &game.maxentities },
	{ "const int numEntities", &game.numentities },
	{ "const int maxClients", &gs.maxclients },
	{ "cGametypeDesc gametype", &level.gametype },
	{ "cMatch match", &level.gametype.match },
	// racesow
	{ "int mysqlConnected", &MysqlConnected },
	{ "int ircConnected", &ircConnected },
	// !racesow

	{ NULL, NULL }
};

static void G_asRegisterGlobalFunctions( int asEngineHandle )
{
	int error;
	int count = 0, failedcount = 0;
	const asglobfuncs_t *func;

	for( func = asGlobFuncs; func->declaration; func++ )
	{
		if( level.gametype.asEngineIsGeneric ) {
			error = -1;
		} else {
			error = angelExport->asRegisterGlobalFunction( asEngineHandle, func->declaration, func->pointer, asCALL_CDECL );
		}

		if( error < 0 )
		{
			failedcount++;
			continue;
		}

		count++;
	}
}

static void G_asRegisterGlobalProperties( int asEngineHandle )
{
	int error;
	int count = 0, failedcount = 0;
	const asglobproperties_t *prop;

	for( prop = asGlobProps; prop->declaration; prop++ )
	{
		error = angelExport->asRegisterGlobalProperty( asEngineHandle, prop->declaration, prop->pointer );
		if( error < 0 )
		{
			failedcount++;
			continue;
		}

		count++;
	}
}

static void G_InitializeGameModuleSyntax( int asEngineHandle )
{
	G_Printf( "* Initializing Game module syntax\n" );

	// register global variables
	G_asRegisterEnums( asEngineHandle );

	// register classes
	G_RegisterObjectClasses( asEngineHandle );

	// register global functions
	G_asRegisterGlobalFunctions( asEngineHandle );

	// register global properties
	G_asRegisterGlobalProperties( asEngineHandle );
}


static qboolean G_asExecutionErrorReport( int asEngineHandle, int asContextHandle, int error )
{
	if( error == asEXECUTION_FINISHED )
		return qfalse;

	// The execution didn't finish as we had planned. Determine why.
	if( error == asEXECUTION_ABORTED )
		G_Printf( "* The script was aborted before it could finish. Probably it timed out.\n" );

	else if( error == asEXECUTION_EXCEPTION )
	{
		void *func;

		G_Printf( "* The script ended with an exception.\n" );

		// Write some information about the script exception
		func = angelExport->asGetExceptionFunction( asContextHandle );
		G_Printf( "* func: %s\n", angelExport->asGetFunctionDeclaration( func, qtrue ) );
		G_Printf( "* modl: %s\n", SCRIPT_MODULE_NAME );
		G_Printf( "* sect: %s\n", angelExport->asGetFunctionSection( func ) );
		G_Printf( "* line: %i\n", angelExport->asGetExceptionLineNumber( asContextHandle ) );
		G_Printf( "* desc: %s\n", angelExport->asGetExceptionString( asContextHandle ) );
	}
	else
		G_Printf( "* The script ended for some unforeseen reason ( %i ).\n", error );

	return qtrue;
}

static void G_ResetGametypeScriptData( void )
{
	level.gametype.asEngineHandle = -1;
	level.gametype.asEngineIsGeneric = qfalse;
	level.gametype.initFunc = NULL;
	level.gametype.spawnFunc = NULL;
	level.gametype.matchStateStartedFunc = NULL;
	level.gametype.matchStateFinishedFunc = NULL;
	level.gametype.thinkRulesFunc = NULL;
	level.gametype.playerRespawnFunc = NULL;
	level.gametype.scoreEventFunc = NULL;
	level.gametype.scoreboardMessageFunc = NULL;
	level.gametype.selectSpawnPointFunc = NULL;
	level.gametype.clientCommandFunc = NULL;
	level.gametype.botStatusFunc = NULL;
	level.gametype.shutdownFunc = NULL;

	asEntityCallThinkFuncPtr = NULL;
	asEntityCallTouchFuncPtr = NULL;
	asEntityCallUseFuncPtr = NULL;
	asEntityCallStopFuncPtr = NULL;
	asEntityCallPainFuncPtr = NULL;
	asEntityCallDieFuncPtr = NULL;
}

void G_asShutdownGametypeScript( void )
{
	int i;
	edict_t *e;

	// release the callback and any other objects obtained from the script engine before releasing the engine
	for( i = 0; i < game.numentities; i++ ) {
		e = &game.edicts[i];

		if( e->scriptSpawned ) {
			G_asReleaseEntityBehavoirs( e );
		}
	}

	if( level.gametype.asEngineHandle > -1 ) {
		if( angelExport )
			angelExport->asReleaseScriptEngine( level.gametype.asEngineHandle );
	}

	G_ResetGametypeScriptData();
}

//"void GT_SpawnGametype()"
void G_asCallLevelSpawnScript( void )
{
	int error, asContextHandle;

	if( !level.gametype.spawnFunc )
		return;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.spawnFunc );
	if( error < 0 ) 
		return;

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();
}

//"void GT_MatchStateStarted()"
void G_asCallMatchStateStartedScript( void )
{
	int error, asContextHandle;

	if( !level.gametype.matchStateStartedFunc )
		return;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.matchStateStartedFunc );
	if( error < 0 ) 
		return;

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();
}

//"bool GT_MatchStateFinished( int incomingMatchState )"
qboolean G_asCallMatchStateFinishedScript( int incomingMatchState )
{
	int error, asContextHandle;
	qboolean result;

	if( !level.gametype.matchStateFinishedFunc )
		return qtrue;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.matchStateFinishedFunc );
	if( error < 0 ) 
		return qtrue;

	// Now we need to pass the parameters to the script function.
	angelExport->asSetArgDWord( asContextHandle, 0, incomingMatchState );

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();

	// Retrieve the return from the context
	result = angelExport->asGetReturnBool( asContextHandle );

	return result;
}

//"void GT_ThinkRules( void )"
void G_asCallThinkRulesScript( void )
{
	int error, asContextHandle;

	if( !level.gametype.thinkRulesFunc )
		return;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.thinkRulesFunc );
	if( error < 0 ) 
		return;

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();
}

//"void GT_playerRespawn( cEntity @ent, int old_team, int new_team )"
void G_asCallPlayerRespawnScript( edict_t *ent, int old_team, int new_team )
{
	int error, asContextHandle;

	if( !level.gametype.playerRespawnFunc )
		return;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.playerRespawnFunc );
	if( error < 0 ) 
		return;

	// Now we need to pass the parameters to the script function.
	angelExport->asSetArgObject( asContextHandle, 0, ent );
	angelExport->asSetArgDWord( asContextHandle, 1, old_team );
	angelExport->asSetArgDWord( asContextHandle, 2, new_team );

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();
}

//"void GT_scoreEvent( cClient @client, String &score_event, String &args )"
void G_asCallScoreEventScript( gclient_t *client, const char *score_event, const char *args )
{
	int error, asContextHandle;
	asstring_t *s1, *s2;

	if( !level.gametype.scoreEventFunc )
		return;

	if( !score_event || !score_event[0] )
		return;

	if( !args )
		args = "";

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.scoreEventFunc );
	if( error < 0 ) 
		return;

	// Now we need to pass the parameters to the script function.
	s1 = angelExport->asStringFactoryBuffer( score_event, strlen( score_event ) );
	s2 = angelExport->asStringFactoryBuffer( args, strlen( args ) );

	angelExport->asSetArgObject( asContextHandle, 0, client );
	angelExport->asSetArgObject( asContextHandle, 1, s1 );
	angelExport->asSetArgObject( asContextHandle, 2, s2 );

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();

	angelExport->asStringRelease( s1 );
	angelExport->asStringRelease( s2 );
}

//"String @GT_ScoreboardMessage( uint maxlen )"
char *G_asCallScoreboardMessage( unsigned int maxlen )
{
	asstring_t *string;
	int error, asContextHandle;

	scoreboardString[0] = 0;

	if( !level.gametype.scoreboardMessageFunc )
		return NULL;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.scoreboardMessageFunc );
	if( error < 0 ) 
		return NULL;

	// Now we need to pass the parameters to the script function.
	angelExport->asSetArgDWord( asContextHandle, 0, maxlen );

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();

	string = (asstring_t *)angelExport->asGetReturnObject( asContextHandle );
	if( !string || !string->len || !string->buffer )
		return NULL;

	Q_strncpyz( scoreboardString, string->buffer, sizeof( scoreboardString ) );

	return scoreboardString;
}

//"cEntity @GT_SelectSpawnPoint( cEntity @ent )"
edict_t *G_asCallSelectSpawnPointScript( edict_t *ent )
{
	int error, asContextHandle;
	edict_t *spot;

	if( !level.gametype.selectSpawnPointFunc )
		return SelectDeathmatchSpawnPoint( ent ); // should have a hardcoded backup

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.selectSpawnPointFunc );
	if( error < 0 ) 
		return SelectDeathmatchSpawnPoint( ent );

	// Now we need to pass the parameters to the script function.
	angelExport->asSetArgObject( asContextHandle, 0, ent );

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();

	spot = (edict_t *)angelExport->asGetReturnObject( asContextHandle );
	if( !spot )
		spot = SelectDeathmatchSpawnPoint( ent );

	return spot;
}

//"bool GT_Command( cClient @client, String &cmdString, String &argsString, int argc )"
qboolean G_asCallGameCommandScript( gclient_t *client, const char *cmd, const char *args, int argc )
{
	int error, asContextHandle;
	asstring_t *s1, *s2;

	if( !level.gametype.clientCommandFunc )
		return qfalse; // should have a hardcoded backup

	// check for having any command to parse
	if( !cmd || !cmd[0] )
		return qfalse;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.clientCommandFunc );
	if( error < 0 ) 
		return qfalse;

	// Now we need to pass the parameters to the script function.
	s1 = angelExport->asStringFactoryBuffer( cmd, strlen( cmd ) );
	s2 = angelExport->asStringFactoryBuffer( args, strlen( args ) );

	angelExport->asSetArgObject( asContextHandle, 0, client );
	angelExport->asSetArgObject( asContextHandle, 1, s1 );
	angelExport->asSetArgObject( asContextHandle, 2, s2 );
	angelExport->asSetArgDWord( asContextHandle, 3, argc );

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();

	angelExport->asStringRelease( s1 );
	angelExport->asStringRelease( s2 );

	// Retrieve the return from the context
	return angelExport->asGetReturnBool( asContextHandle );
}

//"bool GT_UpdateBotStatus( cEntity @ent )"
qboolean G_asCallBotStatusScript( edict_t *ent )
{
	int error, asContextHandle;

	if( !level.gametype.botStatusFunc )
		return qfalse; // should have a hardcoded backup

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.botStatusFunc );
	if( error < 0 ) 
		return qfalse;

	// Now we need to pass the parameters to the script function.
	angelExport->asSetArgObject( asContextHandle, 0, ent );

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();

	// Retrieve the return from the context
	return angelExport->asGetReturnBool( asContextHandle );
}

//"void GT_Shutdown()"
void G_asCallShutdownScript( void )
{
	int error, asContextHandle;

	if( !level.gametype.shutdownFunc || !angelExport )
		return;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.shutdownFunc );
	if( error < 0 ) 
		return;

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();
}

// map entity spawning
qboolean G_asCallMapEntitySpawnScript( const char *classname, edict_t *ent )
{
	char fdeclstr[MAX_STRING_CHARS];
	int error, asContextHandle;
	if (!angelExport) return qfalse;

	Q_snprintfz( fdeclstr, sizeof( fdeclstr ), "void %s( cEntity @ent )", classname );

	ent->asSpawnFunc = angelExport->asGetFunctionByDecl( level.gametype.asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( !ent->asSpawnFunc )
		return qfalse;

	// this is in case we might want to call G_asReleaseEntityBehavoirs
	// in the spawn function (an object may release itself, ugh)
	ent->scriptSpawned = qtrue;
	G_asClearEntityBehavoirs( ent );

	// call the spawn function
	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );
	error = angelExport->asPrepare( asContextHandle, ent->asSpawnFunc );
	if( error < 0 ) 
		return qfalse;

	// Now we need to pass the parameters to the script function.
	angelExport->asSetArgObject( asContextHandle, 0, ent );

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
	{
		G_asShutdownGametypeScript();
		ent->asSpawnFunc = NULL;
		return qfalse;
	}

	// the spawn function may remove the entity
	return ent->r.inuse;
}

/*
* G_asResetEntityBehavoirs
*/
void G_asResetEntityBehavoirs( edict_t *ent )
{
	ent->asThinkFunc = asEntityCallThinkFuncPtr;
	ent->asTouchFunc = asEntityCallTouchFuncPtr;
	ent->asUseFunc = asEntityCallUseFuncPtr;
	ent->asStopFunc = asEntityCallStopFuncPtr;
	ent->asPainFunc = asEntityCallPainFuncPtr;
	ent->asDieFunc = asEntityCallDieFuncPtr;
}

/*
* G_asClearEntityBehavoirs
*/
void G_asClearEntityBehavoirs( edict_t *ent )
{
	ent->asThinkFunc = NULL;
	ent->asTouchFunc = NULL;
	ent->asUseFunc = NULL;
	ent->asStopFunc = NULL;
	ent->asPainFunc = NULL;
	ent->asDieFunc = NULL;
}

/*
* G_asReleaseEntityBehavoirs
*
* Release callback function references held by the engine for script spawned entities
*/
void G_asReleaseEntityBehavoirs( edict_t *ent )
{
	if( ent->scriptSpawned && angelExport ) {
		if( ent->asThinkFunc ) {
			angelExport->asReleaseFunction( ent->asThinkFunc );
		}
		if( ent->asTouchFunc ) {
			angelExport->asReleaseFunction( ent->asTouchFunc );
		}
		if( ent->asUseFunc ) {
			angelExport->asReleaseFunction( ent->asUseFunc );
		}
		if( ent->asStopFunc ) {
			angelExport->asReleaseFunction( ent->asStopFunc );
		}
		if( ent->asPainFunc ) {
			angelExport->asReleaseFunction( ent->asPainFunc );
		}
		if( ent->asDieFunc ) {
			angelExport->asReleaseFunction( ent->asDieFunc );
		}
	}

	G_asClearEntityBehavoirs( ent );
}

//"void %s_think( cEntity @ent )"
void G_asCallMapEntityThink( edict_t *ent )
{
	int error, asContextHandle;

	assert( ent->scriptSpawned == qtrue );

	if( !ent->asThinkFunc )
		return;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, ent->asThinkFunc );
	if( error < 0 ) 
		return;

	// Now we need to pass the parameters to the script function.
	angelExport->asSetArgObject( asContextHandle, 0, ent );

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();
}

// "void %s_touch( cEntity @ent, cEntity @other, const Vec3 planeNormal, int surfFlags )"
void G_asCallMapEntityTouch( edict_t *ent, edict_t *other, cplane_t *plane, int surfFlags )
{
	int error, asContextHandle;
	asvec3_t normal;

	if( !ent->asTouchFunc )
		return;

	assert( ent->scriptSpawned == qtrue );

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, ent->asTouchFunc );
	if( error < 0 ) 
		return;

	if( plane )
	{
		VectorCopy( plane->normal, normal.v );
	}
	else
	{
		VectorClear( normal.v );
	}

	// Now we need to pass the parameters to the script function.
	angelExport->asSetArgObject( asContextHandle, 0, ent );
	angelExport->asSetArgObject( asContextHandle, 1, other );
	angelExport->asSetArgObject( asContextHandle, 2, &normal );
	angelExport->asSetArgDWord( asContextHandle, 3, surfFlags );

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();
}

// "void %s_use( cEntity @ent, cEntity @other, cEntity @activator )"
void G_asCallMapEntityUse( edict_t *ent, edict_t *other, edict_t *activator )
{
	int error, asContextHandle;

	if( !ent->asUseFunc )
		return;

	assert( ent->scriptSpawned == qtrue );

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, ent->asUseFunc );
	if( error < 0 ) 
		return;

	// Now we need to pass the parameters to the script function.
	angelExport->asSetArgObject( asContextHandle, 0, ent );
	angelExport->asSetArgObject( asContextHandle, 1, other );
	angelExport->asSetArgObject( asContextHandle, 2, activator );

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();
}

// "void %s_pain( cEntity @ent, cEntity @other, float kick, float damage )"
void G_asCallMapEntityPain( edict_t *ent, edict_t *other, float kick, float damage )
{
	int error, asContextHandle;

	if( !ent->asPainFunc )
		return;

	assert( ent->scriptSpawned == qtrue );

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, ent->asPainFunc );
	if( error < 0 ) 
		return;

	// Now we need to pass the parameters to the script function.
	angelExport->asSetArgObject( asContextHandle, 0, ent );
	angelExport->asSetArgObject( asContextHandle, 1, other );
	angelExport->asSetArgFloat( asContextHandle, 2, kick );
	angelExport->asSetArgFloat( asContextHandle, 3, damage );

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();
}

// "void %s_die( cEntity @ent, cEntity @inflicter, cEntity @attacker )"
void G_asCallMapEntityDie( edict_t *ent, edict_t *inflicter, edict_t *attacker, int damage, const vec3_t point )
{
	int error, asContextHandle;

	if( !ent->asDieFunc )
		return;

	assert( ent->scriptSpawned == qtrue );

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, ent->asDieFunc );
	if( error < 0 ) 
		return;

	// Now we need to pass the parameters to the script function.
	angelExport->asSetArgObject( asContextHandle, 0, ent );
	angelExport->asSetArgObject( asContextHandle, 1, inflicter );
	angelExport->asSetArgObject( asContextHandle, 2, attacker );

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();
}

//"void %s_stop( cEntity @ent )"
void G_asCallMapEntityStop( edict_t *ent )
{
	int error, asContextHandle;

	if( !ent->asStopFunc )
		return;

	assert( ent->scriptSpawned == qtrue );

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, ent->asStopFunc );
	if( error < 0 ) 
		return;

	// Now we need to pass the parameters to the script function.
	angelExport->asSetArgObject( asContextHandle, 0, ent );

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();
}

static char *G_LoadScriptSection( const char *script, int sectionNum )
{
	char filename[MAX_QPATH];
	qbyte *data;
	int length, filenum;
	char *sectionName;

	sectionName = G_ListNameForPosition( script, sectionNum, CHAR_GAMETYPE_SEPARATOR );
	if( !sectionName )
		return NULL;

	COM_StripExtension( sectionName );

	while( *sectionName == '\n' || *sectionName == ' ' || *sectionName == '\r' )
		sectionName++;

	if( sectionName[0] == '/' )
		Q_snprintfz( filename, sizeof( filename ), "progs%s%s", sectionName, GAMETYPE_SCRIPT_EXTENSION );
	else
		Q_snprintfz( filename, sizeof( filename ), "progs/gametypes/%s%s", sectionName, GAMETYPE_SCRIPT_EXTENSION );
	Q_strlwr( filename );

	length = trap_FS_FOpenFile( filename, &filenum, FS_READ );

	if( length == -1 )
	{
		G_Printf( "Couldn't find script section: '%s'\n", filename );
		return NULL;
	}

	//load the script data into memory
	data = G_Malloc( length + 1 );
	trap_FS_Read( data, length, filenum );
	trap_FS_FCloseFile( filenum );

	G_Printf( "* Loaded script section '%s'\n", filename );
	return (char *)data;
}

qboolean G_asInitializeGametypeScript( const char *script, const char *gametypeName )
{
	int asEngineHandle, asContextHandle, error;
	int numSections, sectionNum;
	int funcCount;
	char *section;
	const char *fdeclstr;
	const asglobfuncs_t *func;

	angelExport = trap_asGetAngelExport();
	if( !angelExport )
	{
		G_Printf( "G_asInitializeGametypeScript: Angelscript API unavailable\n" );
		return qfalse;
	}

	G_Printf( "* Initializing gametype scripts\n" );

	// count referenced script sections
	for( numSections = 0; ( section = G_ListNameForPosition( script, numSections, CHAR_GAMETYPE_SEPARATOR ) ) != NULL; numSections++ );

	if( !numSections )
	{
		G_Printf( "* Invalid gametype script: The gametype has no valid script sections included.\n" );
		goto releaseAll;
	}

	// initialize the engine
	asEngineHandle = angelExport->asCreateScriptEngine( &level.gametype.asEngineIsGeneric );
	if( asEngineHandle < 0 )
	{
		G_Printf( "* Couldn't initialize angelscript.\n" );
		goto releaseAll;
	}

	if( level.gametype.asEngineIsGeneric )
	{
		G_Printf( "* Generic calling convention detected, aborting.\n" );
		goto releaseAll;
	}

	G_InitializeGameModuleSyntax( asEngineHandle );

	// load up the script sections

	for( sectionNum = 0; ( section = G_LoadScriptSection( script, sectionNum ) ) != NULL; sectionNum++ )
	{
		char *sectionName = G_ListNameForPosition( script, sectionNum, CHAR_GAMETYPE_SEPARATOR );
		error = angelExport->asAddScriptSection( asEngineHandle, SCRIPT_MODULE_NAME, sectionName, section, strlen( section ) );

		G_Free( section );

		if( error )
		{
			G_Printf( "* Failed to add the script section %s with error %i\n", gametypeName, error );
			goto releaseAll;
		}
	}

	if( sectionNum != numSections )
	{

		G_Printf( "* Couldn't load all script sections. Can't continue.\n" );
		goto releaseAll;
	}

	error = angelExport->asBuildModule( asEngineHandle, SCRIPT_MODULE_NAME );
	if( error )
	{
		G_Printf( "* Failed to build the script %s\n", gametypeName );
		goto releaseAll;
	}

	// grab script function calls
	funcCount = 0;

	fdeclstr = "void GT_InitGametype()";
	level.gametype.initFunc = angelExport->asGetFunctionByDecl( asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( !level.gametype.initFunc )
	{
		G_Printf( "* The function '%s' was not found. Can not continue.\n", fdeclstr );
		goto releaseAll;
	}
	else
		funcCount++;

	fdeclstr = "void GT_SpawnGametype()";
	level.gametype.spawnFunc = angelExport->asGetFunctionByDecl( asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( !level.gametype.spawnFunc )
	{
		if( developer->integer || sv_cheats->integer )
			G_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	fdeclstr = "void GT_MatchStateStarted()";
	level.gametype.matchStateStartedFunc = angelExport->asGetFunctionByDecl( asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( !level.gametype.matchStateStartedFunc )
	{
		if( developer->integer || sv_cheats->integer )
			G_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	fdeclstr = "bool GT_MatchStateFinished( int incomingMatchState )";
	level.gametype.matchStateFinishedFunc = angelExport->asGetFunctionByDecl( asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( !level.gametype.matchStateFinishedFunc )
	{
		if( developer->integer || sv_cheats->integer )
			G_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	fdeclstr = "void GT_ThinkRules()";
	level.gametype.thinkRulesFunc = angelExport->asGetFunctionByDecl( asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( !level.gametype.thinkRulesFunc )
	{
		if( developer->integer || sv_cheats->integer )
			G_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	fdeclstr = "void GT_playerRespawn( cEntity @ent, int old_team, int new_team )";
	level.gametype.playerRespawnFunc = angelExport->asGetFunctionByDecl( asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( !level.gametype.playerRespawnFunc )
	{
		if( developer->integer || sv_cheats->integer )
			G_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	fdeclstr = "void GT_scoreEvent( cClient @client, String &score_event, String &args )";
	level.gametype.scoreEventFunc = angelExport->asGetFunctionByDecl( asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( !level.gametype.scoreEventFunc )
	{
		if( developer->integer || sv_cheats->integer )
			G_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	fdeclstr = "String @GT_ScoreboardMessage( uint maxlen )";
	level.gametype.scoreboardMessageFunc = angelExport->asGetFunctionByDecl( asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( !level.gametype.scoreboardMessageFunc )
	{
		if( developer->integer || sv_cheats->integer )
			G_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	fdeclstr = "cEntity @GT_SelectSpawnPoint( cEntity @ent )";
	level.gametype.selectSpawnPointFunc = angelExport->asGetFunctionByDecl( asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( !level.gametype.selectSpawnPointFunc )
	{
		if( developer->integer || sv_cheats->integer )
			G_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	fdeclstr = "bool GT_Command( cClient @client, String &cmdString, String &argsString, int argc )";
	level.gametype.clientCommandFunc = angelExport->asGetFunctionByDecl( asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( !level.gametype.clientCommandFunc )
	{
		if( developer->integer || sv_cheats->integer )
			G_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	fdeclstr = "bool GT_UpdateBotStatus( cEntity @ent )";
	level.gametype.botStatusFunc = angelExport->asGetFunctionByDecl( asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( level.gametype.botStatusFunc < 0 )
	{
		if( developer->integer || sv_cheats->integer )
			G_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	fdeclstr = "void GT_Shutdown()";
	level.gametype.shutdownFunc = angelExport->asGetFunctionByDecl( asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( !level.gametype.shutdownFunc )
	{
		if( developer->integer || sv_cheats->integer )
			G_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	// get AS function pointers
	for( func = asGlobFuncs; func->declaration; func++ ) {
		if( func->asFuncPtr ) {
			*func->asFuncPtr = angelExport->asGetGlobalFunctionByDecl( asEngineHandle, func->declaration );
		}
	}


	//
	// execute the GT_InitGametype function
	//

	level.gametype.asEngineHandle = asEngineHandle;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.initFunc );
	if( error < 0 ) 
		goto releaseAll;

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		goto releaseAll;

	return qtrue;

releaseAll:
	G_asShutdownGametypeScript();
	return qfalse;
}

qboolean G_asLoadGametypeScript( const char *gametypeName )
{
	int length, filenum;
	qbyte *data;
	char filename[MAX_QPATH];

	G_ResetGametypeScriptData();

	Q_snprintfz( filename, sizeof( filename ), "progs/gametypes/%s%s", gametypeName, GAMETYPE_PROJECT_EXTENSION );
	Q_strlwr( filename );

	length = trap_FS_FOpenFile( filename, &filenum, FS_READ );

	if( length == -1 )
	{
		G_Printf( "Failed to initialize: Couldn't find '%s'.\n", filename );
		return qfalse;
	}

	if( !length )
	{
		G_Printf( "Failed to initialize: Gametype '%s' is empty.\n", filename );
		trap_FS_FCloseFile( filenum );
		return qfalse;
	}

	//load the script data into memory
	data = G_Malloc( length + 1 );
	trap_FS_Read( data, length, filenum );
	trap_FS_FCloseFile( filenum );

	// Initialize the script
	if( !G_asInitializeGametypeScript( (const char *)data, gametypeName ) )
	{
		G_Printf( "Failed to initialize gametype: '%s'.\n", filename );
		G_Free( data );
		return qfalse;
	}

	G_Free( data );
	return qtrue;
}

/*
* G_asGarbageCollect
*
* Perform garbage collection procedure
*/
void G_asGarbageCollect( qboolean force )
{
	static unsigned int lastTime = 0;
	unsigned int currentSize, totalDestroyed, totalDetected;

	if (!angelExport) return;

	if( lastTime > game.serverTime )
		force = qtrue;

	if( force || lastTime + g_asGC_interval->value * 1000 < game.serverTime )
	{
		angelExport->asGetGCStatistics( level.gametype.asEngineHandle, &currentSize, &totalDestroyed, &totalDetected );

		if( g_asGC_stats->integer )
			G_Printf( "GC: t=%u size=%u destroyed=%u detected=%u\n", game.serverTime, currentSize, totalDestroyed, totalDetected );

		angelExport->asGarbageCollect( level.gametype.asEngineHandle );

		lastTime = game.serverTime;
	}
}

/*
* G_asDumpAPIToFile
*
* Dump all classes, global functions and variables into a file
*/
static void G_asDumpAPIToFile( const char *path )
{
	int i, j;
	int file;
	const asClassDescriptor_t *cDescr;
	const char *name;
	char *filename = NULL;
	size_t filename_size = 0;
	char string[1024];

	// dump class definitions, containing methods, behaviors and properties
	for( i = 0; ; i++ )
	{
		if( !(cDescr = asClassesDescriptors[i]) )
			break;

		name = cDescr->name;
		if( strlen( path ) + strlen( name ) + 2 >= filename_size )
		{
			if( filename_size )
				G_Free( filename );
			filename_size = (strlen( path ) + strlen( name ) + 2) * 2 + 1;
			filename = G_Malloc( filename_size );
		}

		Q_snprintfz( filename, filename_size, "%s%s.h", path, name, ".h" );
		if( trap_FS_FOpenFile( filename, &file, FS_WRITE ) == -1 )
		{
			G_Printf( "G_asDumpAPIToFile: Couldn't write %s.\n", filename );
			return;
		}

		// funcdefs
		if( cDescr->funcdefs )
		{
			Q_snprintfz( string, sizeof( string ), "/* funcdefs */\r\n" );
			trap_FS_Write( string, strlen( string ), file );

			for( j = 0; ; j++ )
			{
				const asFuncdef_t *funcdef = &cDescr->funcdefs[j];
				if( !funcdef->declaration )
					break;

				Q_snprintfz( string, sizeof( string ), "funcdef %s;\r\n", funcdef->declaration );
				trap_FS_Write( string, strlen( string ), file );
			}

			Q_snprintfz( string, sizeof( string ), "\r\n" );
			trap_FS_Write( string, strlen( string ), file );
		}

		Q_snprintfz( string, sizeof( string ), "/**\r\n * %s\r\n */\r\n", cDescr->name );
		trap_FS_Write( string, strlen( string ), file );

		Q_snprintfz( string, sizeof( string ), "class %s\r\n{\r\npublic:", cDescr->name );
		trap_FS_Write( string, strlen( string ), file );

		// object properties
		if( cDescr->objProperties )
		{
			Q_snprintfz( string, sizeof( string ), "\r\n\t/* object properties */\r\n" );
			trap_FS_Write( string, strlen( string ), file );

			for( j = 0; ; j++ )
			{
				const asProperty_t *objProperty = &cDescr->objProperties[j];
				if( !objProperty->declaration )
					break;

				Q_snprintfz( string, sizeof( string ), "\t%s;\r\n", objProperty->declaration );
				trap_FS_Write( string, strlen( string ), file );
			}
		}

		// object behaviors
		if( cDescr->objBehaviors )
		{
			Q_snprintfz( string, sizeof( string ), "\r\n\t/* object behaviors */\r\n" );
			trap_FS_Write( string, strlen( string ), file );

			for( j = 0; ; j++ )
			{
				const asBehavior_t *objBehavior = &cDescr->objBehaviors[j];
				if( !objBehavior->declaration )
					break;

				// ignore add/remove reference behaviors as they can not be used explicitly anyway
				if( objBehavior->behavior == asBEHAVE_ADDREF || objBehavior->behavior == asBEHAVE_RELEASE )
					continue;

				Q_snprintfz( string, sizeof( string ), "\t%s;%s\r\n", objBehavior->declaration,
					( objBehavior->behavior == asBEHAVE_FACTORY ? " /* factory */ " : "" )
					);
				trap_FS_Write( string, strlen( string ), file );
			}
		}

		// object methods
		if( cDescr->objMethods )
		{
			Q_snprintfz( string, sizeof( string ), "\r\n\t/* object methods */\r\n" );
			trap_FS_Write( string, strlen( string ), file );

			for( j = 0; ; j++ )
			{
				const asMethod_t *objMethod = &cDescr->objMethods[j];
				if( !objMethod->declaration )
					break;

				Q_snprintfz( string, sizeof( string ), "\t%s;\r\n", objMethod->declaration );
				trap_FS_Write( string, strlen( string ), file );
			}
		}

		Q_snprintfz( string, sizeof( string ), "};\r\n\r\n" );
		trap_FS_Write( string, strlen( string ), file );

		trap_FS_FCloseFile( file );

		G_Printf( "Wrote %s\n", filename );
	}

	// globals
	name = "globals";
	if( strlen( path ) + strlen( name ) + 2 >= filename_size )
	{
		if( filename_size )
			G_Free( filename );
		filename_size = (strlen( path ) + strlen( name ) + 2) * 2 + 1;
		filename = G_Malloc( filename_size );
	}

	Q_snprintfz( filename, filename_size, "%s%s.h", path, name, ".h" );
	if( trap_FS_FOpenFile( filename, &file, FS_WRITE ) == -1 )
	{
		G_Printf( "G_asDumpAPIToFile: Couldn't write %s.\n", filename );
		return;
	}

	// enums
	{
		const asEnum_t *asEnum;
		const asEnumVal_t *asEnumVal;

		Q_snprintfz( string, sizeof( string ), "/**\r\n * %s\r\n */\r\n", "Enums" );
		trap_FS_Write( string, strlen( string ), file );

		for( i = 0, asEnum = asEnums; asEnum->name != NULL; i++, asEnum++ )
		{
			Q_snprintfz( string, sizeof( string ), "typedef enum\r\n{\r\n" );
			trap_FS_Write( string, strlen( string ), file );

			for( j = 0, asEnumVal = asEnum->values; asEnumVal->name != NULL; j++, asEnumVal++ )
			{
				Q_snprintfz( string, sizeof( string ), "\t%s = 0x%x,\r\n", asEnumVal->name, asEnumVal->value );
				trap_FS_Write( string, strlen( string ), file );
			}

			Q_snprintfz( string, sizeof( string ), "} %s;\r\n\r\n", asEnum->name );
			trap_FS_Write( string, strlen( string ), file );
		}
	}

	// global properties
	{
		const asglobproperties_t *prop;

		Q_snprintfz( string, sizeof( string ), "/**\r\n * %s\r\n */\r\n", "Global properties" );
		trap_FS_Write( string, strlen( string ), file );

		for( prop = asGlobProps; prop->declaration; prop++ )
		{
			Q_snprintfz( string, sizeof( string ), "%s;\r\n", prop->declaration );
			trap_FS_Write( string, strlen( string ), file );
		}

		Q_snprintfz( string, sizeof( string ), "\r\n" );
		trap_FS_Write( string, strlen( string ), file );
	}

	// global functions
	{
		const asglobfuncs_t *func;

		Q_snprintfz( string, sizeof( string ), "/**\r\n * %s\r\n */\r\n", "Global functions" );
		trap_FS_Write( string, strlen( string ), file );

		for( func = asGlobFuncs; func->declaration; func++ )
		{
			Q_snprintfz( string, sizeof( string ), "%s;\r\n", func->declaration );
			trap_FS_Write( string, strlen( string ), file );
		}

		Q_snprintfz( string, sizeof( string ), "\r\n" );
		trap_FS_Write( string, strlen( string ), file );
	}

	trap_FS_FCloseFile( file );

	G_Printf( "Wrote %s\n", filename );
}

/*
* G_asDumpAPI_f
*
* Dump all classes, global functions and variables into a file
*/
void G_asDumpAPI_f( void )
{
	char path[MAX_QPATH];

	Q_snprintfz( path, sizeof( path ), "AS_API/v%.g/", trap_Cvar_Value( "version" ) );
	G_asDumpAPIToFile( path );
}
