#include "g_local.h"
#include "../gameshared/angelref.h"

// fixme: this should probably go into q_shared.h
#if defined ( _WIN64 ) || ( __x86_64__ )
#define FOFFSET(s,m)   (size_t)( (qintptr)&(((s *)0)->m) )
#else
#define FOFFSET(s,m)   (size_t)&(((s *)0)->m)
#endif

#define G_AsMalloc								G_LevelMalloc
#define G_AsFree								G_LevelFree

static angelwrap_api_t *angelExport = NULL;

#define SCRIPT_MODULE_NAME						"gametypes"

#define SCRIPT_ENUM_VAL(name)					{ #name,name }
#define SCRIPT_ENUM_VAL_NULL					{ NULL, 0 }

#define SCRIPT_ENUM_NULL						{ NULL, NULL }

#define SCRIPT_FUNCTION_DECL(type,name,params)	(#type " " #name #params)

#define SCRIPT_PROPERTY_DECL(type,name)			#type " " #name

#define SCRIPT_FUNCTION_NULL					NULL
#define SCRIPT_BEHAVIOR_NULL					{ 0, SCRIPT_FUNCTION_NULL, NULL, 0 }
#define SCRIPT_METHOD_NULL						{ SCRIPT_FUNCTION_NULL, NULL, 0 }
#define SCRIPT_PROPERTY_NULL					{ NULL, 0 }

typedef struct asEnumVal_s
{
	const char * const name;
	const int value;
} asEnumVal_t;

typedef struct asEnum_s
{
	const char * const name;
	const asEnumVal_t * const values;
} asEnum_t;

typedef struct asBehavior_s
{
	const unsigned int behavior;
	const char * const declaration;
	const void *funcPointer;
	const void *funcPointer_asGeneric;
	const int callConv;
} asBehavior_t;

typedef struct asMethod_s
{
	const char * const declaration;
	const void *funcPointer;
	const void *funcPointer_asGeneric;
	const int callConv;
} asMethod_t;

typedef struct asProperty_s
{
	const char * const declaration;
	const unsigned int offset;
} asProperty_t;

typedef struct asClassDescriptor_s
{
	const char * const name;
	const asEObjTypeFlags typeFlags;
	const size_t size;
	const asBehavior_t * const objBehaviors;
	const asBehavior_t * const globalBehaviors;
	const asMethod_t * const objMethods;
	const asProperty_t * const objProperties;
	const void * const stringFactory;
	const void * const stringFactory_asGeneric;
} asClassDescriptor_t;

//=======================================================================

/*
* ASCRIPT_GENERIC
*/

#define G_asGetReturnBool( x ) (qboolean)angelExport->asGetReturnByte( x )
#define G_asGeneric_GetObject( x ) ( angelExport->asIScriptGeneric_GetObject(x) )

static qbyte G_asGeneric_GetArgByte( void *gen, unsigned int arg )
{
	qas_argument_t *argument = angelExport->asIScriptGeneric_GetArg( gen, arg, QAS_ARG_UINT8 );
	assert( argument && argument->type >= 0 );

	return (qbyte)argument->integer;
}

static qboolean G_asGeneric_GetArgBool( void *gen, unsigned int arg )
{
	return (qboolean)G_asGeneric_GetArgByte( gen, arg );
}

static short G_asGeneric_GetArgShort( void *gen, unsigned int arg )
{
	qas_argument_t *argument = angelExport->asIScriptGeneric_GetArg( gen, arg, QAS_ARG_UINT16 );
	assert( argument && argument->type >= 0 );

	return ( (unsigned short)argument->integer );
}

static int G_asGeneric_GetArgInt( void *gen, unsigned int arg )
{
	qas_argument_t *argument = angelExport->asIScriptGeneric_GetArg( gen, arg, QAS_ARG_UINT );
	assert( argument && argument->type >= 0 );

	return ( (unsigned int)argument->integer );
}

static quint64 G_asGeneric_GetArgInt64( void *gen, unsigned int arg )
{
	qas_argument_t *argument = angelExport->asIScriptGeneric_GetArg( gen, arg, QAS_ARG_UINT64 );
	assert( argument && argument->type >= 0 );

	return (quint64)argument->integer64;
}

static float G_asGeneric_GetArgFloat( void *gen, unsigned int arg )
{
	qas_argument_t *argument = angelExport->asIScriptGeneric_GetArg( gen, arg, QAS_ARG_FLOAT );
	assert( argument && argument->type >= 0 );

	return argument->value;
}

static double G_asGeneric_GetArgDouble( void *gen, unsigned int arg )
{
	qas_argument_t *argument = angelExport->asIScriptGeneric_GetArg( gen, arg, QAS_ARG_DOUBLE );
	assert( argument && argument->type >= 0 );

	return argument->dvalue;
}

static void *G_asGeneric_GetArgObject( void *gen, unsigned int arg )
{
	qas_argument_t *argument = angelExport->asIScriptGeneric_GetArg( gen, arg, QAS_ARG_OBJECT );
	assert( argument && argument->type >= 0 );

	return argument->ptr;
}

static void *G_asGeneric_GetArgAddress( void *gen, unsigned int arg )
{
	qas_argument_t *argument = angelExport->asIScriptGeneric_GetArg( gen, arg, QAS_ARG_ADDRESS );
	assert( argument && argument->type >= 0 );

	return argument->ptr;
}

static void *G_asGeneric_GetAddressOfArg( void *gen, unsigned int arg )
{
	qas_argument_t *argument = angelExport->asIScriptGeneric_GetArg( gen, arg, QAS_ARG_POINTER );
	assert( argument && argument->type >= 0 );

	return argument->ptr;
}

static void G_asGeneric_SetReturnByte( void *gen, qbyte value )
{
	int error;
	qas_argument_t argument;

	argument.type = QAS_ARG_UINT8;
	argument.integer = value;

	error = angelExport->asIScriptGeneric_SetReturn( gen, &argument );
	assert( error >= 0 );
}

static void G_asGeneric_SetReturnBool( void *gen, qboolean value )
{
	G_asGeneric_SetReturnByte( gen, value );
}

static void G_asGeneric_SetReturnShort( void *gen, unsigned short value )
{
	int error;
	qas_argument_t argument;

	argument.type = QAS_ARG_UINT16;
	argument.integer = value;

	error = angelExport->asIScriptGeneric_SetReturn( gen, &argument );
	assert( error >= 0 );
}

static void G_asGeneric_SetReturnInt( void *gen, unsigned int value )
{
	int error;
	qas_argument_t argument;

	argument.type = QAS_ARG_UINT;
	argument.integer = value;

	error = angelExport->asIScriptGeneric_SetReturn( gen, &argument );
	assert( error >= 0 );
}

static void G_asGeneric_SetReturnInt64( void *gen, quint64 value )
{
	int error;
	qas_argument_t argument;

	argument.type = QAS_ARG_UINT64;
	argument.integer64 = value;

	error = angelExport->asIScriptGeneric_SetReturn( gen, &argument );
	assert( error >= 0 );
}

static void G_asGeneric_SetReturnFloat( void *gen, float value )
{
	int error;
	qas_argument_t argument;

	argument.type = QAS_ARG_FLOAT;
	argument.value = value;

	error = angelExport->asIScriptGeneric_SetReturn( gen, &argument );
	assert( error >= 0 );
}

static void G_asGeneric_SetReturnDouble( void *gen, double value )
{
	int error;
	qas_argument_t argument;

	argument.type = QAS_ARG_DOUBLE;
	argument.dvalue = value;

	error = angelExport->asIScriptGeneric_SetReturn( gen, &argument );
	assert( error >= 0 );
}

static void G_asGeneric_SetReturnObject( void *gen, void *ptr )
{
	int error;
	qas_argument_t argument;

	argument.type = QAS_ARG_OBJECT;
	argument.ptr = ptr;

	error = angelExport->asIScriptGeneric_SetReturn( gen, &argument );
	assert( error >= 0 );
}

static void G_asGeneric_SetReturnAddress( void *gen, void *ptr )
{
	int error;
	qas_argument_t argument;

	argument.type = QAS_ARG_ADDRESS;
	argument.ptr = ptr;

	error = angelExport->asIScriptGeneric_SetReturn( gen, &argument );
	assert( error >= 0 );
}

static void G_asGeneric_SetReturnPointer( void *gen, void *ptr )
{
	int error;
	qas_argument_t argument;

	argument.type = QAS_ARG_POINTER;
	argument.ptr = ptr;

	error = angelExport->asIScriptGeneric_SetReturn( gen, &argument );
	assert( error >= 0 );
}

//=======================================================================

static const asEnumVal_t asConfigstringEnumVals[] =
{
	SCRIPT_ENUM_VAL( CS_MODMANIFEST ),
	SCRIPT_ENUM_VAL( CS_MESSAGE ),
	SCRIPT_ENUM_VAL( CS_MAPNAME ),
	SCRIPT_ENUM_VAL( CS_AUDIOTRACK ),
	SCRIPT_ENUM_VAL( CS_HOSTNAME ),
	SCRIPT_ENUM_VAL( CS_TVSERVER ),
	SCRIPT_ENUM_VAL( CS_SKYBOX ),
	SCRIPT_ENUM_VAL( CS_SCORESTATNUMS ),
	SCRIPT_ENUM_VAL( CS_POWERUPEFFECTS ),
	SCRIPT_ENUM_VAL( CS_GAMETYPETITLE ),
	SCRIPT_ENUM_VAL( CS_GAMETYPENAME ),
	SCRIPT_ENUM_VAL( CS_GAMETYPEVERSION ),
	SCRIPT_ENUM_VAL( CS_GAMETYPEAUTHOR ),
	SCRIPT_ENUM_VAL( CS_AUTORECORDSTATE ),
	SCRIPT_ENUM_VAL( CS_SCB_PLAYERTAB_LAYOUT ),
	SCRIPT_ENUM_VAL( CS_SCB_PLAYERTAB_TITLES ),
	SCRIPT_ENUM_VAL( CS_TEAM_ALPHA_NAME ),
	SCRIPT_ENUM_VAL( CS_TEAM_BETA_NAME ),
	SCRIPT_ENUM_VAL( CS_MAXCLIENTS ),
	SCRIPT_ENUM_VAL( CS_MAPCHECKSUM ),
	SCRIPT_ENUM_VAL( CS_MATCHNAME ),

	SCRIPT_ENUM_VAL( CS_MODELS ),
	SCRIPT_ENUM_VAL( CS_SOUNDS ),
	SCRIPT_ENUM_VAL( CS_IMAGES ),
	SCRIPT_ENUM_VAL( CS_SKINFILES ),
	SCRIPT_ENUM_VAL( CS_LIGHTS ),
	SCRIPT_ENUM_VAL( CS_ITEMS ),
	SCRIPT_ENUM_VAL( CS_PLAYERINFOS ),
	SCRIPT_ENUM_VAL( CS_GAMECOMMANDS ),
	SCRIPT_ENUM_VAL( CS_LOCATIONS ),
	SCRIPT_ENUM_VAL( CS_GENERAL ),

	SCRIPT_ENUM_VAL_NULL
};

static const asEnumVal_t asEffectEnumVals[] =
{
	SCRIPT_ENUM_VAL( EF_ROTATE_AND_BOB ),
	SCRIPT_ENUM_VAL( EF_SHELL ),
	SCRIPT_ENUM_VAL( EF_STRONG_WEAPON ),
	SCRIPT_ENUM_VAL( EF_QUAD ),
	SCRIPT_ENUM_VAL( EF_CARRIER ),
	SCRIPT_ENUM_VAL( EF_BUSYICON ),
	SCRIPT_ENUM_VAL( EF_FLAG_TRAIL ),
	SCRIPT_ENUM_VAL( EF_TAKEDAMAGE ),
	SCRIPT_ENUM_VAL( EF_TEAMCOLOR_TRANSITION ),
	SCRIPT_ENUM_VAL( EF_EXPIRING_QUAD ),
	SCRIPT_ENUM_VAL( EF_EXPIRING_SHELL ),
	SCRIPT_ENUM_VAL( EF_GODMODE ),

	SCRIPT_ENUM_VAL( EF_PLAYER_STUNNED ),
	SCRIPT_ENUM_VAL( EF_PLAYER_HIDENAME ),

	SCRIPT_ENUM_VAL_NULL
};

static const asEnumVal_t asMatchStateEnumVals[] =
{
	//SCRIPT_ENUM_VAL( MATCH_STATE_NONE ), // I see no point in adding it
	SCRIPT_ENUM_VAL( MATCH_STATE_WARMUP ),
	SCRIPT_ENUM_VAL( MATCH_STATE_COUNTDOWN ),
	SCRIPT_ENUM_VAL( MATCH_STATE_PLAYTIME ),
	SCRIPT_ENUM_VAL( MATCH_STATE_POSTMATCH ),
	SCRIPT_ENUM_VAL( MATCH_STATE_WAITEXIT ),

	SCRIPT_ENUM_VAL_NULL
};

static const asEnumVal_t asSpawnSystemEnumVals[] =
{
	SCRIPT_ENUM_VAL( SPAWNSYSTEM_INSTANT ),
	SCRIPT_ENUM_VAL( SPAWNSYSTEM_WAVES ),
	SCRIPT_ENUM_VAL( SPAWNSYSTEM_HOLD ),

	SCRIPT_ENUM_VAL_NULL
};

static const asEnumVal_t asHUDStatEnumVals[] =
{
	SCRIPT_ENUM_VAL( STAT_PROGRESS_SELF ),
	SCRIPT_ENUM_VAL( STAT_PROGRESS_OTHER ),
	SCRIPT_ENUM_VAL( STAT_PROGRESS_ALPHA ),
	SCRIPT_ENUM_VAL( STAT_PROGRESS_BETA ),
	SCRIPT_ENUM_VAL( STAT_IMAGE_SELF ),
	SCRIPT_ENUM_VAL( STAT_IMAGE_OTHER ),
	SCRIPT_ENUM_VAL( STAT_IMAGE_ALPHA ),
	SCRIPT_ENUM_VAL( STAT_IMAGE_BETA ),
	SCRIPT_ENUM_VAL( STAT_TIME_SELF ),
	SCRIPT_ENUM_VAL( STAT_TIME_BEST ),
	SCRIPT_ENUM_VAL( STAT_TIME_RECORD ),
	SCRIPT_ENUM_VAL( STAT_TIME_ALPHA ),
	SCRIPT_ENUM_VAL( STAT_TIME_BETA ),
	SCRIPT_ENUM_VAL( STAT_MESSAGE_SELF ),
	SCRIPT_ENUM_VAL( STAT_MESSAGE_OTHER ),
	SCRIPT_ENUM_VAL( STAT_MESSAGE_ALPHA ),
	SCRIPT_ENUM_VAL( STAT_MESSAGE_BETA ),

	SCRIPT_ENUM_VAL_NULL
};

static const asEnumVal_t asTeamEnumVals[] =
{
	SCRIPT_ENUM_VAL( TEAM_SPECTATOR ),
	SCRIPT_ENUM_VAL( TEAM_PLAYERS ),
	SCRIPT_ENUM_VAL( TEAM_ALPHA ),
	SCRIPT_ENUM_VAL( TEAM_BETA ),
	SCRIPT_ENUM_VAL( GS_MAX_TEAMS ),

	SCRIPT_ENUM_VAL_NULL
};

static const asEnumVal_t asEntityTypeEnumVals[] =
{
	SCRIPT_ENUM_VAL( ET_GENERIC ),
	SCRIPT_ENUM_VAL( ET_PLAYER ),
	SCRIPT_ENUM_VAL( ET_CORPSE ),
	SCRIPT_ENUM_VAL( ET_BEAM ),
	SCRIPT_ENUM_VAL( ET_PORTALSURFACE ),
	SCRIPT_ENUM_VAL( ET_PUSH_TRIGGER ),
	SCRIPT_ENUM_VAL( ET_GIB ),
	SCRIPT_ENUM_VAL( ET_BLASTER ),
	SCRIPT_ENUM_VAL( ET_ELECTRO_WEAK ),
	SCRIPT_ENUM_VAL( ET_ROCKET ),
	SCRIPT_ENUM_VAL( ET_GRENADE ),
	SCRIPT_ENUM_VAL( ET_PLASMA ),
	SCRIPT_ENUM_VAL( ET_SPRITE ),
	SCRIPT_ENUM_VAL( ET_ITEM ),
	SCRIPT_ENUM_VAL( ET_LASERBEAM ),
	SCRIPT_ENUM_VAL( ET_CURVELASERBEAM ),
	SCRIPT_ENUM_VAL( ET_FLAG_BASE ),
	SCRIPT_ENUM_VAL( ET_MINIMAP_ICON ),
	SCRIPT_ENUM_VAL( ET_DECAL ),
	SCRIPT_ENUM_VAL( ET_ITEM_TIMER ),
	SCRIPT_ENUM_VAL( ET_PARTICLES ),

	SCRIPT_ENUM_VAL( ET_EVENT ),
	SCRIPT_ENUM_VAL( ET_SOUNDEVENT ),

	SCRIPT_ENUM_VAL_NULL
};

static const asEnumVal_t asSolidEnumVals[] =
{
	SCRIPT_ENUM_VAL( SOLID_NOT ),
	SCRIPT_ENUM_VAL( SOLID_TRIGGER ),
	SCRIPT_ENUM_VAL( SOLID_YES ),

	SCRIPT_ENUM_VAL_NULL
};

static const asEnumVal_t asMovetypeEnumVals[] =
{
	SCRIPT_ENUM_VAL( MOVETYPE_NONE ),
	SCRIPT_ENUM_VAL( MOVETYPE_PLAYER ),
	SCRIPT_ENUM_VAL( MOVETYPE_NOCLIP ),
	SCRIPT_ENUM_VAL( MOVETYPE_PUSH ),
	SCRIPT_ENUM_VAL( MOVETYPE_STOP ),
	SCRIPT_ENUM_VAL( MOVETYPE_FLY ),
	SCRIPT_ENUM_VAL( MOVETYPE_TOSS ),
	SCRIPT_ENUM_VAL( MOVETYPE_LINEARPROJECTILE ),
	SCRIPT_ENUM_VAL( MOVETYPE_BOUNCE ),
	SCRIPT_ENUM_VAL( MOVETYPE_BOUNCEGRENADE ),
	SCRIPT_ENUM_VAL( MOVETYPE_TOSSSLIDE ),

	SCRIPT_ENUM_VAL_NULL
};

static const asEnumVal_t asPMoveFeaturesVals[] =
{
	SCRIPT_ENUM_VAL( PMFEAT_CROUCH ),
	SCRIPT_ENUM_VAL( PMFEAT_WALK ),
	SCRIPT_ENUM_VAL( PMFEAT_JUMP ),
	SCRIPT_ENUM_VAL( PMFEAT_DASH ),
	SCRIPT_ENUM_VAL( PMFEAT_WALLJUMP ),
	SCRIPT_ENUM_VAL( PMFEAT_FWDBUNNY ),
	SCRIPT_ENUM_VAL( PMFEAT_AIRCONTROL ),
	SCRIPT_ENUM_VAL( PMFEAT_ZOOM ),
	SCRIPT_ENUM_VAL( PMFEAT_GHOSTMOVE ),
	SCRIPT_ENUM_VAL( PMFEAT_CONTINOUSJUMP ),
	SCRIPT_ENUM_VAL( PMFEAT_ITEMPICK ),
	SCRIPT_ENUM_VAL( PMFEAT_GUNBLADEAUTOATTACK ),
	SCRIPT_ENUM_VAL( PMFEAT_WEAPONSWITCH ),
	SCRIPT_ENUM_VAL( PMFEAT_ALL ),
	SCRIPT_ENUM_VAL( PMFEAT_DEFAULT ),

	SCRIPT_ENUM_VAL_NULL
};

static const asEnumVal_t asItemTypeEnumVals[] =
{
	SCRIPT_ENUM_VAL( IT_WEAPON ),
	SCRIPT_ENUM_VAL( IT_AMMO ),
	SCRIPT_ENUM_VAL( IT_ARMOR ),
	SCRIPT_ENUM_VAL( IT_POWERUP ),
	SCRIPT_ENUM_VAL( IT_HEALTH ),

	SCRIPT_ENUM_VAL_NULL
};

static const asEnumVal_t asInstagibNegItemMaskEnumVals[] =
{
	SCRIPT_ENUM_VAL( G_INSTAGIB_NEGATE_ITEMMASK ),

	SCRIPT_ENUM_VAL_NULL
};

static const asEnumVal_t asWeaponTagEnumVals[] =
{
	SCRIPT_ENUM_VAL( WEAP_NONE ),
	SCRIPT_ENUM_VAL( WEAP_GUNBLADE ),
	SCRIPT_ENUM_VAL( WEAP_MACHINEGUN ),
	SCRIPT_ENUM_VAL( WEAP_RIOTGUN ),
	SCRIPT_ENUM_VAL( WEAP_GRENADELAUNCHER ),
	SCRIPT_ENUM_VAL( WEAP_ROCKETLAUNCHER ),
	SCRIPT_ENUM_VAL( WEAP_PLASMAGUN ),
	SCRIPT_ENUM_VAL( WEAP_LASERGUN ),
	SCRIPT_ENUM_VAL( WEAP_ELECTROBOLT ),
	SCRIPT_ENUM_VAL( WEAP_INSTAGUN ),
	SCRIPT_ENUM_VAL( WEAP_TOTAL ),

	SCRIPT_ENUM_VAL_NULL
};

static const asEnumVal_t asAmmoTagEnumVals[] =
{
	SCRIPT_ENUM_VAL( AMMO_NONE ),
	SCRIPT_ENUM_VAL( AMMO_GUNBLADE ),
	SCRIPT_ENUM_VAL( AMMO_STRONG_BULLETS ),
	SCRIPT_ENUM_VAL( AMMO_SHELLS ),
	SCRIPT_ENUM_VAL( AMMO_GRENADES ),
	SCRIPT_ENUM_VAL( AMMO_ROCKETS ),
	SCRIPT_ENUM_VAL( AMMO_PLASMA ),
	SCRIPT_ENUM_VAL( AMMO_LASERS ),
	SCRIPT_ENUM_VAL( AMMO_BOLTS ),
	SCRIPT_ENUM_VAL( AMMO_INSTAS ),

	SCRIPT_ENUM_VAL( AMMO_WEAK_GUNBLADE ),
	SCRIPT_ENUM_VAL( AMMO_BULLETS ),
	SCRIPT_ENUM_VAL( AMMO_WEAK_SHELLS ),
	SCRIPT_ENUM_VAL( AMMO_WEAK_GRENADES ),
	SCRIPT_ENUM_VAL( AMMO_WEAK_ROCKETS ),
	SCRIPT_ENUM_VAL( AMMO_WEAK_PLASMA ),
	SCRIPT_ENUM_VAL( AMMO_WEAK_LASERS ),
	SCRIPT_ENUM_VAL( AMMO_WEAK_BOLTS ),
	SCRIPT_ENUM_VAL( AMMO_WEAK_INSTAS ),

	SCRIPT_ENUM_VAL( AMMO_TOTAL ),

	SCRIPT_ENUM_VAL_NULL
};

static const asEnumVal_t asArmorTagEnumVals[] =
{
	SCRIPT_ENUM_VAL( ARMOR_NONE ),
	SCRIPT_ENUM_VAL( ARMOR_GA ),
	SCRIPT_ENUM_VAL( ARMOR_YA ),
	SCRIPT_ENUM_VAL( ARMOR_RA ),
	SCRIPT_ENUM_VAL( ARMOR_SHARD ),

	SCRIPT_ENUM_VAL_NULL
};

static const asEnumVal_t asHealthTagEnumVals[] =
{
	SCRIPT_ENUM_VAL( HEALTH_NONE ),
	SCRIPT_ENUM_VAL( HEALTH_SMALL ),
	SCRIPT_ENUM_VAL( HEALTH_MEDIUM ),
	SCRIPT_ENUM_VAL( HEALTH_LARGE ),
	SCRIPT_ENUM_VAL( HEALTH_MEGA ),
	SCRIPT_ENUM_VAL( HEALTH_ULTRA ),

	SCRIPT_ENUM_VAL_NULL
};

static const asEnumVal_t asPowerupTagEnumVals[] =
{
	SCRIPT_ENUM_VAL( POWERUP_NONE ),
	SCRIPT_ENUM_VAL( POWERUP_QUAD ),
	SCRIPT_ENUM_VAL( POWERUP_SHELL ),

	SCRIPT_ENUM_VAL( POWERUP_TOTAL ),

	SCRIPT_ENUM_VAL_NULL
};

static const asEnumVal_t asMiscItemTagEnumVals[] =
{
	SCRIPT_ENUM_VAL( AMMO_PACK_WEAK ),
	SCRIPT_ENUM_VAL( AMMO_PACK_STRONG ),
	SCRIPT_ENUM_VAL( AMMO_PACK ),
	SCRIPT_ENUM_VAL_NULL
};

static const asEnumVal_t asClientStateEnumVals[] =
{
	SCRIPT_ENUM_VAL( CS_FREE ),
	SCRIPT_ENUM_VAL( CS_ZOMBIE ),
	SCRIPT_ENUM_VAL( CS_CONNECTING ),
	SCRIPT_ENUM_VAL( CS_CONNECTED ),
	SCRIPT_ENUM_VAL( CS_SPAWNED ),

	SCRIPT_ENUM_VAL_NULL
};

static const asEnumVal_t asSoundChannelEnumVals[] =
{
	SCRIPT_ENUM_VAL( CHAN_AUTO ),
	SCRIPT_ENUM_VAL( CHAN_PAIN ),
	SCRIPT_ENUM_VAL( CHAN_VOICE ),
	SCRIPT_ENUM_VAL( CHAN_ITEM ),
	SCRIPT_ENUM_VAL( CHAN_BODY ),
	SCRIPT_ENUM_VAL( CHAN_MUZZLEFLASH ),
	SCRIPT_ENUM_VAL( CHAN_FIXED ),

	SCRIPT_ENUM_VAL_NULL
};

static const asEnumVal_t asContentsEnumVals[] =
{
	SCRIPT_ENUM_VAL( CONTENTS_SOLID ),
	SCRIPT_ENUM_VAL( CONTENTS_LAVA ),
	SCRIPT_ENUM_VAL( CONTENTS_SLIME ),
	SCRIPT_ENUM_VAL( CONTENTS_WATER ),
	SCRIPT_ENUM_VAL( CONTENTS_FOG ),
	SCRIPT_ENUM_VAL( CONTENTS_AREAPORTAL ),
	SCRIPT_ENUM_VAL( CONTENTS_PLAYERCLIP ),
	SCRIPT_ENUM_VAL( CONTENTS_MONSTERCLIP ),
	SCRIPT_ENUM_VAL( CONTENTS_TELEPORTER ),
	SCRIPT_ENUM_VAL( CONTENTS_JUMPPAD ),
	SCRIPT_ENUM_VAL( CONTENTS_CLUSTERPORTAL ),
	SCRIPT_ENUM_VAL( CONTENTS_DONOTENTER ),
	SCRIPT_ENUM_VAL( CONTENTS_ORIGIN ),
	SCRIPT_ENUM_VAL( CONTENTS_BODY ),
	SCRIPT_ENUM_VAL( CONTENTS_CORPSE ),
	SCRIPT_ENUM_VAL( CONTENTS_DETAIL ),
	SCRIPT_ENUM_VAL( CONTENTS_STRUCTURAL ),
	SCRIPT_ENUM_VAL( CONTENTS_TRANSLUCENT ),
	SCRIPT_ENUM_VAL( CONTENTS_TRIGGER ),
	SCRIPT_ENUM_VAL( CONTENTS_NODROP ),
	SCRIPT_ENUM_VAL( MASK_ALL ),
	SCRIPT_ENUM_VAL( MASK_SOLID ),
	SCRIPT_ENUM_VAL( MASK_PLAYERSOLID ),
	SCRIPT_ENUM_VAL( MASK_DEADSOLID ),
	SCRIPT_ENUM_VAL( MASK_MONSTERSOLID ),
	SCRIPT_ENUM_VAL( MASK_WATER ),
	SCRIPT_ENUM_VAL( MASK_OPAQUE ),
	SCRIPT_ENUM_VAL( MASK_SHOT ),

	SCRIPT_ENUM_VAL_NULL
};

static const asEnumVal_t asSurfFlagEnumVals[] =
{
	SCRIPT_ENUM_VAL( SURF_NODAMAGE ),
	SCRIPT_ENUM_VAL( SURF_SLICK ),
	SCRIPT_ENUM_VAL( SURF_SKY ),
	SCRIPT_ENUM_VAL( SURF_LADDER ),
	SCRIPT_ENUM_VAL( SURF_NOIMPACT ),
	SCRIPT_ENUM_VAL( SURF_NOMARKS ),
	SCRIPT_ENUM_VAL( SURF_FLESH ),
	SCRIPT_ENUM_VAL( SURF_NODRAW ),
	SCRIPT_ENUM_VAL( SURF_HINT ),
	SCRIPT_ENUM_VAL( SURF_SKIP ),
	SCRIPT_ENUM_VAL( SURF_NOLIGHTMAP ),
	SCRIPT_ENUM_VAL( SURF_POINTLIGHT ),
	SCRIPT_ENUM_VAL( SURF_METALSTEPS ),
	SCRIPT_ENUM_VAL( SURF_NOSTEPS ),
	SCRIPT_ENUM_VAL( SURF_NONSOLID ),
	SCRIPT_ENUM_VAL( SURF_LIGHTFILTER ),
	SCRIPT_ENUM_VAL( SURF_ALPHASHADOW ),
	SCRIPT_ENUM_VAL( SURF_NODLIGHT ),
	SCRIPT_ENUM_VAL( SURF_DUST ),

	SCRIPT_ENUM_VAL_NULL
};

static const asEnumVal_t asSVFlagEnumVals[] =
{
	SCRIPT_ENUM_VAL( SVF_NOCLIENT ),
	SCRIPT_ENUM_VAL( SVF_PORTAL ),
	SCRIPT_ENUM_VAL( SVF_NOORIGIN2 ),
	SCRIPT_ENUM_VAL( SVF_TRANSMITORIGIN2 ),
	SCRIPT_ENUM_VAL( SVF_SOUNDCULL ),
	SCRIPT_ENUM_VAL( SVF_FAKECLIENT ),
	SCRIPT_ENUM_VAL( SVF_BROADCAST ),
	SCRIPT_ENUM_VAL( SVF_CORPSE ),
	SCRIPT_ENUM_VAL( SVF_PROJECTILE ),
	SCRIPT_ENUM_VAL( SVF_ONLYTEAM ),
	SCRIPT_ENUM_VAL( SVF_FORCEOWNER ),
	SCRIPT_ENUM_VAL( SVF_NOCULLATORIGIN2 ),

	SCRIPT_ENUM_VAL_NULL
};

static const asEnumVal_t asCvarFlagEnumVals[] =
{
	SCRIPT_ENUM_VAL( CVAR_ARCHIVE ),
	SCRIPT_ENUM_VAL( CVAR_USERINFO ),
	SCRIPT_ENUM_VAL( CVAR_SERVERINFO ),
	SCRIPT_ENUM_VAL( CVAR_NOSET ),
	SCRIPT_ENUM_VAL( CVAR_LATCH ),
	SCRIPT_ENUM_VAL( CVAR_LATCH_VIDEO ),
	SCRIPT_ENUM_VAL( CVAR_LATCH_SOUND ),
	SCRIPT_ENUM_VAL( CVAR_CHEAT ),
	SCRIPT_ENUM_VAL( CVAR_READONLY ),

	SCRIPT_ENUM_VAL_NULL
};

static const asEnumVal_t asMeaningsOfDeathEnumVals[] =
{
	SCRIPT_ENUM_VAL( MOD_GUNBLADE_W ),
	SCRIPT_ENUM_VAL( MOD_GUNBLADE_S ),
	SCRIPT_ENUM_VAL( MOD_MACHINEGUN_W ),
	SCRIPT_ENUM_VAL( MOD_MACHINEGUN_S ),
	SCRIPT_ENUM_VAL( MOD_RIOTGUN_W ),
	SCRIPT_ENUM_VAL( MOD_RIOTGUN_S ),
	SCRIPT_ENUM_VAL( MOD_GRENADE_W ),
	SCRIPT_ENUM_VAL( MOD_GRENADE_S ),
	SCRIPT_ENUM_VAL( MOD_ROCKET_W ),
	SCRIPT_ENUM_VAL( MOD_ROCKET_S ),
	SCRIPT_ENUM_VAL( MOD_PLASMA_W ),
	SCRIPT_ENUM_VAL( MOD_PLASMA_S ),
	SCRIPT_ENUM_VAL( MOD_ELECTROBOLT_W ),
	SCRIPT_ENUM_VAL( MOD_ELECTROBOLT_S ),
	SCRIPT_ENUM_VAL( MOD_INSTAGUN_W ),
	SCRIPT_ENUM_VAL( MOD_INSTAGUN_S ),
	SCRIPT_ENUM_VAL( MOD_LASERGUN_W ),
	SCRIPT_ENUM_VAL( MOD_LASERGUN_S ),
	SCRIPT_ENUM_VAL( MOD_GRENADE_SPLASH_W ),
	SCRIPT_ENUM_VAL( MOD_GRENADE_SPLASH_S ),
	SCRIPT_ENUM_VAL( MOD_ROCKET_SPLASH_W ),
	SCRIPT_ENUM_VAL( MOD_ROCKET_SPLASH_S ),
	SCRIPT_ENUM_VAL( MOD_PLASMA_SPLASH_W ),
	SCRIPT_ENUM_VAL( MOD_PLASMA_SPLASH_S ),

	// World damage
	SCRIPT_ENUM_VAL( MOD_WATER ),
	SCRIPT_ENUM_VAL( MOD_SLIME ),
	SCRIPT_ENUM_VAL( MOD_LAVA ),
	SCRIPT_ENUM_VAL( MOD_CRUSH ),
	SCRIPT_ENUM_VAL( MOD_TELEFRAG ),
	SCRIPT_ENUM_VAL( MOD_FALLING ),
	SCRIPT_ENUM_VAL( MOD_SUICIDE ),
	SCRIPT_ENUM_VAL( MOD_EXPLOSIVE ),

	// probably not used
	SCRIPT_ENUM_VAL( MOD_BARREL ),
	SCRIPT_ENUM_VAL( MOD_BOMB ),
	SCRIPT_ENUM_VAL( MOD_EXIT ),
	SCRIPT_ENUM_VAL( MOD_SPLASH ),
	SCRIPT_ENUM_VAL( MOD_TARGET_LASER ),
	SCRIPT_ENUM_VAL( MOD_TRIGGER_HURT ),
	SCRIPT_ENUM_VAL( MOD_HIT ),

	SCRIPT_ENUM_VAL_NULL
};

static const asEnumVal_t asDamageEnumVals[] =
{
	SCRIPT_ENUM_VAL( DAMAGE_NO ),
	SCRIPT_ENUM_VAL( DAMAGE_YES ),
	SCRIPT_ENUM_VAL( DAMAGE_AIM ),

	SCRIPT_ENUM_VAL_NULL
};

static const asEnumVal_t asMiscelaneaEnumVals[] =
{
	SCRIPT_ENUM_VAL_NULL
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
	{ "cvarflags_e", asCvarFlagEnumVals },
	{ "meaningsofdeath_e", asMeaningsOfDeathEnumVals },
	{ "takedamage_e", asDamageEnumVals },
	{ "miscelanea_e", asMiscelaneaEnumVals },

	SCRIPT_ENUM_NULL
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

// CLASS: cString
static int asstring_factored_count = 0;
static int asstring_released_count = 0;

typedef struct
{
	char *buffer;
	size_t len, size;
	int asRefCount, asFactored;
} asstring_t;

static inline asstring_t *objectString_Alloc( void )
{
	static asstring_t *object;

	object = G_AsMalloc( sizeof( asstring_t ) );
	object->asRefCount = 1;
	object->asFactored = 1;

	asstring_factored_count++;
	return object;
}

static asstring_t *objectString_FactoryBuffer( const char *buffer, unsigned int length )
{
	asstring_t *object;

	object = objectString_Alloc();
	object->buffer = G_AsMalloc( sizeof( char ) * ( length + 1 ) );
	object->len = length;
	object->buffer[length] = 0;
	object->size = length + 1;
	if( buffer )
		Q_strncpyz( object->buffer, buffer, object->size );

	return object;
}

static asstring_t *objectString_AssignString( asstring_t *self, const char *string, size_t strlen )
{
	if( strlen >= self->size )
	{
		G_AsFree( self->buffer );

		self->size = strlen + 1;
		self->buffer = G_AsMalloc( self->size );
	}

	self->len = strlen;
	Q_strncpyz( self->buffer, string, self->size );

	return self;
}

static asstring_t *objectString_AssignPattern( asstring_t *self, const char *pattern, ... )
{
	va_list	argptr;
	static char buf[4096];

	va_start( argptr, pattern );
	Q_vsnprintfz( buf, sizeof( buf ), pattern, argptr );
	va_end( argptr );

	return objectString_AssignString( self, buf, strlen( buf ) );
}

static asstring_t *objectString_AddAssignString( asstring_t *self, const char *string, size_t strlen )
{
	if( strlen )
	{
		char *tem = self->buffer;

		self->len = strlen + self->len;
		self->size = self->len + 1;
		self->buffer = G_AsMalloc( self->size );

		Q_snprintfz( self->buffer, self->size, "%s%s", tem, string );
		G_AsFree( tem );
	}

	return self;
}

static asstring_t *objectString_AddAssignPattern( asstring_t *self, const char *pattern, ... )
{
	va_list	argptr;
	static char buf[4096];

	va_start( argptr, pattern );
	Q_vsnprintfz( buf, sizeof( buf ), pattern, argptr );
	va_end( argptr );

	return objectString_AddAssignString( self, buf, strlen( buf ) );
}

static asstring_t *objectString_AddString( asstring_t *first, const char *second, size_t seclen )
{
	asstring_t *self = objectString_FactoryBuffer( NULL, first->len + seclen );

	Q_snprintfz( self->buffer, self->size, "%s%s", first->buffer, second );

	return self;
}

static asstring_t *objectString_AddPattern( asstring_t *first, const char *pattern, ... )
{
	va_list	argptr;
	static char buf[4096];

	va_start( argptr, pattern );
	Q_vsnprintfz( buf, sizeof( buf ), pattern, argptr );
	va_end( argptr );

	return objectString_AddString( first, buf, strlen( buf ) );
}

static asstring_t *objectString_Factory( void )
{
	return objectString_FactoryBuffer( NULL, 0 );
}

static void objectString_asGeneric_Factory( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectString_Factory() );
}

static asstring_t *objectString_FactoryCopy( const asstring_t *other )
{
	return objectString_FactoryBuffer( other->buffer, other->len );
}

static void objectString_asGeneric_FactoryCopy( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectString_FactoryCopy( (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 ) ) );
}

static void objectString_Addref( asstring_t *obj ) { obj->asRefCount++; }

static void objectString_asGeneric_Addref( void *gen )
{
	objectString_Addref( (asstring_t *)G_asGeneric_GetObject( gen ) );
}

static void objectString_Release( asstring_t *obj )
{
	obj->asRefCount--;
	clamp_low( obj->asRefCount, 0 );

	if( !obj->asRefCount && obj->asFactored )
	{
		G_AsFree( obj->buffer );
		G_AsFree( obj );
		asstring_released_count++;
	}
}

static void objectString_asGeneric_Release( void *gen )
{
	objectString_Release( (asstring_t *)G_asGeneric_GetObject( gen ) );
}

static asstring_t *StringFactory( unsigned int length, const char *s )
{
	return objectString_FactoryBuffer( s, length );
}

static void StringFactory_asGeneric( void *gen )
{
	unsigned int length = (unsigned int)G_asGeneric_GetArgInt( gen, 0 );
	char *s = G_asGeneric_GetArgAddress( gen, 1 );

	G_asGeneric_SetReturnAddress( gen, StringFactory( length, s ) );
}

static char *objectString_Index( unsigned int i, asstring_t *self )
{
	if( i > self->len )
	{
		G_Printf( "* WARNING: objectString_Index: Out of range\n" );
		return NULL;
	}

	return &self->buffer[i];
}

static void objectString_asGeneric_Index( void *gen )
{
	unsigned int i = (unsigned int)G_asGeneric_GetArgInt( gen, 0 );
	asstring_t *self = (asstring_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectString_Index( i, self ) );
}

static asstring_t *objectString_AssignBehaviour( asstring_t *other, asstring_t *self )
{
	return objectString_AssignString( self, other->buffer, other->len );
}

static void objectString_asGeneric_AssignBehaviour( void *gen )
{
	asstring_t *other = (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asstring_t *self = (asstring_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectString_AssignBehaviour( other, self ) );
}

static asstring_t *objectString_AssignBehaviourI( int other, asstring_t *self )
{
	return objectString_AssignPattern( self, "%i", other );
}

static void objectString_asGeneric_AssignBehaviourI( void *gen )
{
	int other = G_asGeneric_GetArgInt( gen, 0 );
	asstring_t *self = (asstring_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectString_AssignBehaviourI( other, self ) );
}

static asstring_t *objectString_AssignBehaviourD( double other, asstring_t *self )
{
	return objectString_AssignPattern( self, "%g", other );
}

static void objectString_asGeneric_AssignBehaviourD( void *gen )
{
	double other = G_asGeneric_GetArgDouble( gen, 0 );
	asstring_t *self = (asstring_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectString_AssignBehaviourD( other, self ) );
}

static asstring_t *objectString_AssignBehaviourF( float other, asstring_t *self )
{
	return objectString_AssignPattern( self, "%f", other );
}

static void objectString_asGeneric_AssignBehaviourF( void *gen )
{
	float other = G_asGeneric_GetArgFloat( gen, 0 );
	asstring_t *self = (asstring_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectString_AssignBehaviourF( other, self ) );
}

static asstring_t *objectString_AddAssignBehaviourSS( asstring_t *other, asstring_t *self )
{
	return objectString_AddAssignString( self, other->buffer, other->len );
}

static void objectString_asGeneric_AddAssignBehaviourSS( void *gen )
{
	asstring_t *other = (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asstring_t *self = (asstring_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectString_AddAssignBehaviourSS( other, self ) );
}

static asstring_t *objectString_AddAssignBehaviourSI( int other, asstring_t *self )
{
	return objectString_AddAssignPattern( self, "%i", other );
}

static void objectString_asGeneric_AddAssignBehaviourSI( void *gen )
{
	int other = G_asGeneric_GetArgInt( gen, 0 );
	asstring_t *self = (asstring_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectString_AddAssignBehaviourSI( other, self ) );
}

static asstring_t *objectString_AddAssignBehaviourSD( double other, asstring_t *self )
{
	return objectString_AddAssignPattern( self, "%g", other );
}

static void objectString_asGeneric_AddAssignBehaviourSD( void *gen )
{
	double other = G_asGeneric_GetArgDouble( gen, 0 );
	asstring_t *self = (asstring_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectString_AddAssignBehaviourSD( other, self ) );
}

static asstring_t *objectString_AddAssignBehaviourSF( float other, asstring_t *self )
{
	return objectString_AddAssignPattern( self, "%f", other );
}

static void objectString_asGeneric_AddAssignBehaviourSF( void *gen )
{
	float other = G_asGeneric_GetArgFloat( gen, 0 );
	asstring_t *self = (asstring_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectString_AddAssignBehaviourSF( other, self ) );
}

static asstring_t *objectString_AddBehaviourSS( asstring_t *first, asstring_t *second )
{
	return objectString_AddString( first, second->buffer, second->len );
}

static void objectString_asGeneric_AddBehaviourSS( void *gen )
{
	asstring_t *first = (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asstring_t *second = (asstring_t *)G_asGeneric_GetArgAddress( gen, 1 );

	G_asGeneric_SetReturnAddress( gen, objectString_AddBehaviourSS( first, second ) );
}

static asstring_t *objectString_AddBehaviourSI( asstring_t *first, int second )
{
	return objectString_AddPattern( first, "%i", second );
}

static void objectString_asGeneric_AddBehaviourSI( void *gen )
{
	asstring_t *first = (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 );
	int second = G_asGeneric_GetArgInt( gen, 1 );

	G_asGeneric_SetReturnAddress( gen, objectString_AddBehaviourSI( first, second ) );
}

static asstring_t *objectString_AddBehaviourIS( int first, asstring_t *second )
{
	asstring_t *res = objectString_Factory();
	return objectString_AssignPattern( res, "%i%s", first, second->buffer );
}

static void objectString_asGeneric_AddBehaviourIS( void *gen )
{
	int first = G_asGeneric_GetArgInt( gen, 0 );
	asstring_t *second = (asstring_t *)G_asGeneric_GetArgAddress( gen, 1 );

	G_asGeneric_SetReturnAddress( gen, objectString_AddBehaviourIS( first, second ) );
}

static asstring_t *objectString_AddBehaviourSD( asstring_t *first, double second )
{
	return objectString_AddPattern( first, "%g", second );
}

static void objectString_asGeneric_AddBehaviourSD( void *gen )
{
	asstring_t *first = (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 );
	double second = G_asGeneric_GetArgDouble( gen, 1 );

	G_asGeneric_SetReturnAddress( gen, objectString_AddBehaviourSD( first, second ) );
}

static asstring_t *objectString_AddBehaviourDS( double first, asstring_t *second )
{
	asstring_t *res = objectString_FactoryBuffer( NULL, 0 );
	return objectString_AssignPattern( res, "%g%s", first, second->buffer );
}

static void objectString_asGeneric_AddBehaviourDS( void *gen )
{
	double first = G_asGeneric_GetArgDouble( gen, 0 );
	asstring_t *second = (asstring_t *)G_asGeneric_GetArgAddress( gen, 1 );

	G_asGeneric_SetReturnAddress( gen, objectString_AddBehaviourDS( first, second ) );
}

static asstring_t *objectString_AddBehaviourSF( asstring_t *first, float second )
{
	return objectString_AddPattern( first, "%f", second );
}

static void objectString_asGeneric_AddBehaviourSF( void *gen )
{
	asstring_t *first = (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 );
	float second = G_asGeneric_GetArgFloat( gen, 1 );

	G_asGeneric_SetReturnAddress( gen, objectString_AddBehaviourSF( first, second ) );
}

static asstring_t *objectString_AddBehaviourFS( float first, asstring_t *second )
{
	asstring_t *res = objectString_FactoryBuffer( NULL, 0 );
	return objectString_AssignPattern( res, "%f%s", first, second->buffer );
}

static void objectString_asGeneric_AddBehaviourFS( void *gen )
{
	float first = G_asGeneric_GetArgFloat( gen, 0 );
	asstring_t *second = (asstring_t *)G_asGeneric_GetArgAddress( gen, 1 );

	G_asGeneric_SetReturnAddress( gen, objectString_AddBehaviourFS( first, second ) );
}

static qboolean objectString_EqualBehaviour( asstring_t *first, asstring_t *second )
{
	if( !first->len && !second->len )
		return qtrue;

	return ( Q_stricmp( first->buffer, second->buffer ) == 0 );
}

static void objectString_asGeneric_EqualBehaviour( void *gen )
{
	asstring_t *first = (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asstring_t *second = (asstring_t *)G_asGeneric_GetArgAddress( gen, 1 );

	G_asGeneric_SetReturnBool( gen, objectString_EqualBehaviour( first, second ) );
}

static qboolean objectString_NotEqualBehaviour( asstring_t *first, asstring_t *second )
{
	return !objectString_EqualBehaviour( first, second );
}

static void objectString_asGeneric_NotEqualBehaviour( void *gen )
{
	asstring_t *first = (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asstring_t *second = (asstring_t *)G_asGeneric_GetArgAddress( gen, 1 );

	G_asGeneric_SetReturnBool( gen, objectString_NotEqualBehaviour( first, second ) );
}

static int objectString_Len( asstring_t *self )
{
	return self->len;
}

static void objectString_asGeneric_Len( void *gen )
{
	G_asGeneric_SetReturnInt( gen, objectString_Len( (asstring_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectString_ToLower( asstring_t *self )
{
	asstring_t *string = objectString_FactoryBuffer( self->buffer, self->len );
	if( string->len )
		Q_strlwr( string->buffer );
	return string;
}

static void objectString_asGeneric_ToLower( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectString_ToLower( (asstring_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectString_ToUpper( asstring_t *self )
{
	asstring_t *string = objectString_FactoryBuffer( self->buffer, self->len );
	if( string->len )
		Q_strupr( string->buffer );
	return string;
}

static void objectString_asGeneric_ToUpper( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectString_ToUpper( (asstring_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectString_Trim( asstring_t *self )
{
	asstring_t *string = objectString_FactoryBuffer( self->buffer, self->len );
	if( string->len )
		Q_trim( string->buffer );
	return string;
}

static void objectString_asGeneric_Trim( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectString_Trim( (asstring_t *)G_asGeneric_GetObject( gen ) ) );
}

static int objectString_Locate( asstring_t *substr, int skip, asstring_t *self )
{
	int i;
	char *p, *s;

	if( !self->len || !substr->len )
		return -1;

	p = NULL;
	for( i = 0, s = self->buffer; i <= skip; i++, s = p + substr->len )
	{
		if( !(p = strstr( s, substr->buffer )) )
			break;
	}

	return p ? p - self->buffer : -1;
}

static void objectString_asGeneric_Locate( void *gen )
{
	asstring_t *substr = (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 );
	int skip = G_asGeneric_GetArgInt( gen, 1 );

	G_asGeneric_SetReturnInt( gen, objectString_Locate( substr, skip, (asstring_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectString_Substring( int start, int length, asstring_t *self )
{
	if( start < 0 || length <= 0 )
		return objectString_FactoryBuffer( NULL, 0 );
	if( start >= (int)self->len )
		return objectString_FactoryBuffer( NULL, 0 );

	return objectString_FactoryBuffer( self->buffer + start, min( length, (int)self->len - start ) );
}

static void objectString_asGeneric_Substring( void *gen )
{
	int start = G_asGeneric_GetArgInt( gen, 0 );
	int length = G_asGeneric_GetArgInt( gen, 1 );

	G_asGeneric_SetReturnAddress( gen, objectString_Substring( start, length, (asstring_t *)G_asGeneric_GetObject( gen ) ) );
}

static qboolean objectString_IsAlpha( asstring_t *self )
{
	size_t i;

	for( i = 0; i < self->len; i++ )
	{
		if( !isalpha( self->buffer[i] ) )
			return qfalse;
	}
	return qtrue;
}

static void objectString_asGeneric_IsAlpha( void *gen )
{
	G_asGeneric_SetReturnBool( gen, objectString_IsAlpha( (asstring_t *)G_asGeneric_GetObject( gen ) ) );
}

static qboolean objectString_IsNumerical( asstring_t *self )
{
	size_t i;

	for( i = 0; i < self->len; i++ )
	{
		if( !isdigit( self->buffer[i] ) )
			return qfalse;
	}
	return qtrue;
}

static void objectString_asGeneric_IsNumerical( void *gen )
{
	G_asGeneric_SetReturnBool( gen, objectString_IsNumerical( (asstring_t *)G_asGeneric_GetObject( gen ) ) );
}

static qboolean objectString_IsAlphaNumerical( asstring_t *self )
{
	size_t i;

	for( i = 0; i < self->len; i++ )
	{
		if( !isalnum( self->buffer[i] ) )
			return qfalse;
	}
	return qtrue;
}

static void objectString_asGeneric_IsAlphaNumerical( void *gen )
{
	G_asGeneric_SetReturnBool( gen, objectString_IsAlphaNumerical( (asstring_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectString_RemoveColorTokens( asstring_t *self )
{
	const char *s;

	if( !self->len )
		return objectString_FactoryBuffer( NULL, 0 );

	s = COM_RemoveColorTokens( self->buffer );
	return objectString_FactoryBuffer( s, strlen(s) );
}

static void objectString_asGeneric_RemoveColorTokens( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectString_RemoveColorTokens( (asstring_t *)G_asGeneric_GetObject( gen ) ) );
}

static int objectString_toInt( asstring_t *self )
{
	return atoi( self->buffer );
}

static void objectString_asGeneric_toInt( void *gen )
{
	G_asGeneric_SetReturnInt( gen, objectString_toInt( (asstring_t *)G_asGeneric_GetObject( gen ) ) );
}

static float objectString_toFloat( asstring_t *self )
{
	return atof( self->buffer );
}

static void objectString_asGeneric_toFloat( void *gen )
{
	G_asGeneric_SetReturnFloat( gen, objectString_toFloat( (asstring_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectString_getToken( int index, asstring_t *self )
{
	int i;
	char *s, *token = "";

	s = self->buffer;

	for( i = 0; i <= index; i++ )
	{
		token = COM_Parse( &s );
		if( !token[0] ) // string finished before finding the token
			break;
	}

	return objectString_FactoryBuffer( token, strlen( token ) );
}

static void objectString_asGeneric_getToken( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectString_getToken( G_asGeneric_GetArgInt( gen, 0 ), (asstring_t *)G_asGeneric_GetObject( gen ) ) );
}

static struct asvec3_s *objectString_toVec3( asstring_t *self );
static void objectString_asGeneric_toVec3( void *gen );

static const asBehavior_t asstring_ObjectBehaviors[] =
{
	/* factory */
	{ asBEHAVE_FACTORY, SCRIPT_FUNCTION_DECL(cString @, f, ()), objectString_Factory, objectString_asGeneric_Factory, asCALL_CDECL },
	{ asBEHAVE_FACTORY, SCRIPT_FUNCTION_DECL(cString @, f, (const cString &in)), objectString_FactoryCopy, objectString_asGeneric_FactoryCopy, asCALL_CDECL },
	{ asBEHAVE_ADDREF, SCRIPT_FUNCTION_DECL(void, f, ()), objectString_Addref, objectString_asGeneric_Addref, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_RELEASE, SCRIPT_FUNCTION_DECL(void, f, ()), objectString_Release, objectString_asGeneric_Release, asCALL_CDECL_OBJLAST },

	/* assignments */
	{ asBEHAVE_ASSIGNMENT, SCRIPT_FUNCTION_DECL(cString &, f, (const cString &in)), objectString_AssignBehaviour, objectString_asGeneric_AssignBehaviour, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_ASSIGNMENT, SCRIPT_FUNCTION_DECL(cString &, f, (int)), objectString_AssignBehaviourI, objectString_asGeneric_AssignBehaviourI, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_ASSIGNMENT, SCRIPT_FUNCTION_DECL(cString &, f, (double)), objectString_AssignBehaviourD, objectString_asGeneric_AssignBehaviourD, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_ASSIGNMENT, SCRIPT_FUNCTION_DECL(cString &, f, (float)), objectString_AssignBehaviourF, objectString_asGeneric_AssignBehaviourF, asCALL_CDECL_OBJLAST },

	/* register the index operator, both as a mutator and as an inspector */
	{ asBEHAVE_INDEX, SCRIPT_FUNCTION_DECL(uint8 &, f, (uint)), objectString_Index, objectString_asGeneric_Index, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_INDEX, SCRIPT_FUNCTION_DECL(const uint8 &, f, (uint)), objectString_Index, objectString_asGeneric_Index, asCALL_CDECL_OBJLAST },

	/* += */
	{ asBEHAVE_ADD_ASSIGN, SCRIPT_FUNCTION_DECL(cString &, f, (cString &in)), objectString_AddAssignBehaviourSS, objectString_asGeneric_AddAssignBehaviourSS, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_ADD_ASSIGN, SCRIPT_FUNCTION_DECL(cString &, f, (int)), objectString_AddAssignBehaviourSI, objectString_asGeneric_AddAssignBehaviourSI, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_ADD_ASSIGN, SCRIPT_FUNCTION_DECL(cString &, f, (double)), objectString_AddAssignBehaviourSD, objectString_asGeneric_AddAssignBehaviourSD, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_ADD_ASSIGN, SCRIPT_FUNCTION_DECL(cString &, f, (float)), objectString_AddAssignBehaviourSF, objectString_asGeneric_AddAssignBehaviourSF, asCALL_CDECL_OBJLAST },

	SCRIPT_BEHAVIOR_NULL
};

static const asBehavior_t asstring_GlobalBehaviors[] =
{
	/* + */
	{ asBEHAVE_ADD, SCRIPT_FUNCTION_DECL(cString @, f, (const cString &in, const cString &in)), objectString_AddBehaviourSS, objectString_asGeneric_AddBehaviourSS, asCALL_CDECL },
	{ asBEHAVE_ADD, SCRIPT_FUNCTION_DECL(cString @, f, (const cString &in, int)), objectString_AddBehaviourSI, objectString_asGeneric_AddBehaviourSI, asCALL_CDECL },
	{ asBEHAVE_ADD, SCRIPT_FUNCTION_DECL(cString @, f, (int, const cString &in)), objectString_AddBehaviourIS, objectString_asGeneric_AddBehaviourIS, asCALL_CDECL },
	{ asBEHAVE_ADD, SCRIPT_FUNCTION_DECL(cString @, f, (const cString &in, double)), objectString_AddBehaviourSD, objectString_asGeneric_AddBehaviourSD, asCALL_CDECL },
	{ asBEHAVE_ADD, SCRIPT_FUNCTION_DECL(cString @, f, (double, const cString &in)), objectString_AddBehaviourDS, objectString_asGeneric_AddBehaviourDS, asCALL_CDECL },
	{ asBEHAVE_ADD, SCRIPT_FUNCTION_DECL(cString @, f, (const cString &in, float)), objectString_AddBehaviourSF, objectString_asGeneric_AddBehaviourSF, asCALL_CDECL },
	{ asBEHAVE_ADD, SCRIPT_FUNCTION_DECL(cString @, f, (float, const cString &in)), objectString_AddBehaviourFS, objectString_asGeneric_AddBehaviourFS, asCALL_CDECL },

	/* == != */
	{ asBEHAVE_EQUAL, SCRIPT_FUNCTION_DECL(bool, f, (const cString &in, const cString &in)), objectString_EqualBehaviour, objectString_asGeneric_EqualBehaviour, asCALL_CDECL },
	{ asBEHAVE_NOTEQUAL, SCRIPT_FUNCTION_DECL(bool, f, (const cString &in, const cString &in)), objectString_NotEqualBehaviour, objectString_asGeneric_NotEqualBehaviour, asCALL_CDECL },

	SCRIPT_BEHAVIOR_NULL
};

static const asMethod_t asstring_Methods[] =
{
	{ SCRIPT_FUNCTION_DECL(int, len, ()), objectString_Len, objectString_asGeneric_Len, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(int, length, ()), objectString_Len, objectString_asGeneric_Len, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, tolower, ()), objectString_ToLower, objectString_asGeneric_ToLower, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, toupper, ()), objectString_ToUpper, objectString_asGeneric_ToUpper, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, trim, ()), objectString_Trim, objectString_asGeneric_Trim, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, removeColorTokens, ()), objectString_RemoveColorTokens, objectString_asGeneric_RemoveColorTokens, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, getToken, (const int)), objectString_getToken, objectString_asGeneric_getToken, asCALL_CDECL_OBJLAST },

	{ SCRIPT_FUNCTION_DECL(int, toInt, ()), objectString_toInt, objectString_asGeneric_toInt, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(float, toFloat, ()), objectString_toFloat, objectString_asGeneric_toFloat, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cVec3 &, toVec3, ()), objectString_toVec3, objectString_asGeneric_toVec3, asCALL_CDECL_OBJLAST },

	{ SCRIPT_FUNCTION_DECL(int, locate, (cString &, const int)), objectString_Locate, objectString_asGeneric_Locate, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, substr, (const int, const int)), objectString_Substring, objectString_asGeneric_Substring, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, subString, (const int, const int)), objectString_Substring, objectString_asGeneric_Substring, asCALL_CDECL_OBJLAST },

	{ SCRIPT_FUNCTION_DECL(bool, isAlpha, ()), objectString_IsAlpha, objectString_asGeneric_IsAlpha, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(bool, isNumerical, ()), objectString_IsNumerical, objectString_asGeneric_IsNumerical, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(bool, isAlphaNumerical, ()), objectString_IsAlphaNumerical, objectString_asGeneric_IsAlphaNumerical, asCALL_CDECL_OBJLAST },

	SCRIPT_METHOD_NULL
};

static const asClassDescriptor_t asStringClassDescriptor =
{
	"cString",					/* name */
	asOBJ_REF,					/* object type flags */
	sizeof( asstring_t ),		/* size */
	asstring_ObjectBehaviors,	/* object behaviors */
	asstring_GlobalBehaviors,	/* global behaviors */
	asstring_Methods,			/* methods */
	NULL,						/* properties */

	StringFactory, StringFactory_asGeneric	/* string factory hack */
};

//=======================================================================

// CLASS: cVec3
static int asvector_factored_count = 0;
static int asvector_released_count = 0;

typedef struct asvec3_s
{
	vec3_t v;
	int asRefCount, asFactored;
} asvec3_t;

static asvec3_t *objectVector_Factory( void )
{
	asvec3_t *object;

	object = G_AsMalloc( sizeof( asvec3_t ) );
	object->asRefCount = 1;
	object->asFactored = 1;
	VectorClear( object->v );
	asvector_factored_count++;
	return object;
}

static void objectVector_asGeneric_Factory( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectVector_Factory() );
}

static asvec3_t *objectVector_FactorySet( float x, float y, float z )
{
	asvec3_t *object = objectVector_Factory();
	object->v[0] = x;
	object->v[1] = y;
	object->v[2] = z;
	return object;
}

static void objectVector_asGeneric_FactorySet( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectVector_FactorySet( G_asGeneric_GetArgFloat( gen, 0 ), G_asGeneric_GetArgFloat( gen, 1 ),G_asGeneric_GetArgFloat( gen, 2 ) ) );
}

static asvec3_t *objectVector_FactorySet2( float v )
{
	asvec3_t *object = objectVector_Factory();
	object->v[0] = object->v[1] = object->v[2] = v;
	return object;
}

static void objectVector_asGeneric_FactorySet2( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectVector_FactorySet2( G_asGeneric_GetArgFloat( gen, 0 ) ) );
}

static void objectVector_Addref( asvec3_t *obj ) { obj->asRefCount++; }

static void objectVector_asGeneric_Addref( void *gen )
{
	objectVector_Addref( (asvec3_t *)G_asGeneric_GetObject( gen ) );
}

static void objectVector_Release( asvec3_t *obj )
{
	obj->asRefCount--;
	clamp_low( obj->asRefCount, 0 );
	if( !obj->asRefCount && obj->asFactored )
	{
		G_AsFree( obj );
		asvector_released_count++;
	}
}

static void objectVector_asGeneric_Release( void *gen )
{
	objectVector_Release( (asvec3_t *)G_asGeneric_GetObject( gen ) );
}


static asvec3_t *objectString_toVec3( asstring_t *self )
{
	float x = 0, y = 0, z = 0;
	asvec3_t *vec = objectVector_Factory();

	sscanf( self->buffer, "%f %f %f", &x, &y, &z );

	VectorSet( vec->v, x, y, z );
	return vec;
}

static void objectString_asGeneric_toVec3( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectString_toVec3( (asstring_t *)G_asGeneric_GetObject( gen ) ) );
}

static asvec3_t *objectVector_AssignBehaviour( asvec3_t *other, asvec3_t *self )
{
	VectorCopy( other->v, self->v );
	return self;
}

static void objectVector_asGeneric_AssignBehaviour( void *gen )
{
	asvec3_t *other = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asvec3_t *self = (asvec3_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectVector_AssignBehaviour( other, self ) );
}

static asvec3_t *objectVector_AssignBehaviourD( double other, asvec3_t *self )
{
	VectorSet( self->v, other, other, other );
	return self;
}

static void objectVector_asGeneric_AssignBehaviourD( void *gen )
{
	double other = G_asGeneric_GetArgDouble( gen, 0 );
	asvec3_t *self = (asvec3_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectVector_AssignBehaviourD( other, self ) );
}

static asvec3_t *objectVector_AssignBehaviourF( float other, asvec3_t *self )
{
	VectorSet( self->v, other, other, other );
	return self;
}

static void objectVector_asGeneric_AssignBehaviourF( void *gen )
{
	float other = G_asGeneric_GetArgFloat( gen, 0 );
	asvec3_t *self = (asvec3_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectVector_AssignBehaviourF( other, self ) );
}

static asvec3_t *objectVector_AssignBehaviourI( int other, asvec3_t *self )
{
	VectorSet( self->v, other, other, other );
	return self;
}

static void objectVector_asGeneric_AssignBehaviourI( void *gen )
{
	int other = G_asGeneric_GetArgInt( gen, 0 );
	asvec3_t *self = (asvec3_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectVector_AssignBehaviourI( other, self ) );
}

static asvec3_t *objectVector_AddAssignBehaviour( asvec3_t *other, asvec3_t *self )
{
	VectorAdd( self->v, other->v, self->v );
	return self;
}

static void objectVector_asGeneric_AddAssignBehaviour( void *gen )
{
	asvec3_t *other = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asvec3_t *self = (asvec3_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectVector_AddAssignBehaviour( other, self ) );
}

static asvec3_t *objectVector_SubAssignBehaviour( asvec3_t *other, asvec3_t *self )
{
	VectorSubtract( self->v, other->v, self->v );
	return self;
}

static void objectVector_asGeneric_SubAssignBehaviour( void *gen )
{
	asvec3_t *other = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asvec3_t *self = (asvec3_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectVector_SubAssignBehaviour( other, self ) );
}

static asvec3_t *objectVector_MulAssignBehaviour( asvec3_t *other, asvec3_t *self )
{
	vec_t product = DotProduct( self->v, other->v );

	VectorScale( self->v, product, self->v );
	return self;
}

static void objectVector_asGeneric_MulAssignBehaviour( void *gen )
{
	asvec3_t *other = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asvec3_t *self = (asvec3_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectVector_MulAssignBehaviour( other, self ) );
}

static asvec3_t *objectVector_XORAssignBehaviour( asvec3_t *other, asvec3_t *self )
{
	vec3_t product;

	CrossProduct( self->v, other->v, product );
	VectorCopy( product, self->v );
	return self;
}

static void objectVector_asGeneric_XORAssignBehaviour( void *gen )
{
	asvec3_t *other = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asvec3_t *self = (asvec3_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectVector_XORAssignBehaviour( other, self ) );
}

static asvec3_t *objectVector_MulAssignBehaviourI( int other, asvec3_t *self )
{
	VectorScale( self->v, other, self->v );
	return self;
}

static void objectVector_asGeneric_MulAssignBehaviourI( void *gen )
{
	int other = G_asGeneric_GetArgInt( gen, 0 );
	asvec3_t *self = (asvec3_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectVector_MulAssignBehaviourI( other, self ) );
}

static asvec3_t *objectVector_MulAssignBehaviourD( double other, asvec3_t *self )
{
	VectorScale( self->v, other, self->v );
	return self;
}

static void objectVector_asGeneric_MulAssignBehaviourD( void *gen )
{
	double other = G_asGeneric_GetArgDouble( gen, 0 );
	asvec3_t *self = (asvec3_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectVector_MulAssignBehaviourD( other, self ) );
}

static asvec3_t *objectVector_MulAssignBehaviourF( float other, asvec3_t *self )
{
	VectorScale( self->v, other, self->v );
	return self;
}

static void objectVector_asGeneric_MulAssignBehaviourF( void *gen )
{
	float other = G_asGeneric_GetArgFloat( gen, 0 );
	asvec3_t *self = (asvec3_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectVector_MulAssignBehaviourF( other, self ) );
}

static asvec3_t *objectVector_AddBehaviour( asvec3_t *first, asvec3_t *second )
{
	asvec3_t *vec = objectVector_Factory();

	VectorAdd( first->v, second->v, vec->v );
	return vec;
}

static void objectVector_asGeneric_AddBehaviour( void *gen )
{
	asvec3_t *first = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asvec3_t *second = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 );

	G_asGeneric_SetReturnAddress( gen, objectVector_AddBehaviour( first, second ) );
}

static asvec3_t *objectVector_SubtractBehaviour( asvec3_t *first, asvec3_t *second )
{
	asvec3_t *vec = objectVector_Factory();

	VectorSubtract( first->v, second->v, vec->v );
	return vec;
}

static void objectVector_asGeneric_SubtractBehaviour( void *gen )
{
	asvec3_t *first = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asvec3_t *second = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 );

	G_asGeneric_SetReturnAddress( gen, objectVector_SubtractBehaviour( first, second ) );
}

static float objectVector_MultiplyBehaviour( asvec3_t *first, asvec3_t *second )
{
	return DotProduct( first->v, second->v );
}

static void objectVector_asGeneric_MultiplyBehaviour( void *gen )
{
	asvec3_t *first = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asvec3_t *second = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 );

	G_asGeneric_SetReturnFloat( gen, objectVector_MultiplyBehaviour( first, second ) );
}

static asvec3_t *objectVector_MultiplyBehaviourVD( asvec3_t *first, double second )
{
	asvec3_t *vec = objectVector_Factory();

	VectorScale( first->v, second, vec->v );
	return vec;
}

static void objectVector_asGeneric_MultiplyBehaviourVD( void *gen )
{
	asvec3_t *first = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	double second = G_asGeneric_GetArgDouble( gen, 1 );

	G_asGeneric_SetReturnAddress( gen, objectVector_MultiplyBehaviourVD( first, second ) );
}

static asvec3_t *objectVector_MultiplyBehaviourDV( double first, asvec3_t *second )
{
	return objectVector_MultiplyBehaviourVD( second, first );
}

static void objectVector_asGeneric_MultiplyBehaviourDV( void *gen )
{
	double first = G_asGeneric_GetArgDouble( gen, 0 );
	asvec3_t *second = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 );

	G_asGeneric_SetReturnAddress( gen, objectVector_MultiplyBehaviourDV( first, second ) );
}

static asvec3_t *objectVector_MultiplyBehaviourVF( asvec3_t *first, float second )
{
	asvec3_t *vec = objectVector_Factory();

	VectorScale( first->v, second, vec->v );
	return vec;
}

static void objectVector_asGeneric_MultiplyBehaviourVF( void *gen )
{
	asvec3_t *first = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	float second = G_asGeneric_GetArgFloat( gen, 1 );

	G_asGeneric_SetReturnAddress( gen, objectVector_MultiplyBehaviourVF( first, second ) );
}

static asvec3_t *objectVector_MultiplyBehaviourFV( float first, asvec3_t *second )
{
	return objectVector_MultiplyBehaviourVF( second, first );
}

static void objectVector_asGeneric_MultiplyBehaviourFV( void *gen )
{
	float first = G_asGeneric_GetArgFloat( gen, 0 );
	asvec3_t *second = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 );

	G_asGeneric_SetReturnAddress( gen, objectVector_MultiplyBehaviourFV( first, second ) );
}

static asvec3_t *objectVector_MultiplyBehaviourVI( asvec3_t *first, int second )
{
	asvec3_t *vec = objectVector_Factory();

	VectorScale( first->v, second, vec->v );
	return vec;
}

static void objectVector_asGeneric_MultiplyBehaviourVI( void *gen )
{
	asvec3_t *first = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	int second = G_asGeneric_GetArgInt( gen, 1 );

	G_asGeneric_SetReturnAddress( gen, objectVector_MultiplyBehaviourVI( first, second ) );
}

static asvec3_t *objectVector_MultiplyBehaviourIV( int first, asvec3_t *second )
{
	return objectVector_MultiplyBehaviourVI( second, first );
}

static void objectVector_asGeneric_MultiplyBehaviourIV( void *gen )
{
	int first = G_asGeneric_GetArgInt( gen, 0 );
	asvec3_t *second = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 );

	G_asGeneric_SetReturnAddress( gen, objectVector_MultiplyBehaviourIV( first, second ) );
}

static asvec3_t *objectVector_XORBehaviour( asvec3_t *first, asvec3_t *second )
{
	asvec3_t *vec = objectVector_Factory();

	CrossProduct( first->v, second->v, vec->v );
	return vec;
}

static void objectVector_asGeneric_XORBehaviour( void *gen )
{
	asvec3_t *first = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asvec3_t *second = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 );

	G_asGeneric_SetReturnAddress( gen, objectVector_XORBehaviour( first, second ) );
}

static qboolean objectVector_EqualBehaviour( asvec3_t *first, asvec3_t *second )
{
	return VectorCompare( first->v, second->v );
}

static void objectVector_asGeneric_EqualBehaviour( void *gen )
{
	asvec3_t *first = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asvec3_t *second = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 );

	G_asGeneric_SetReturnBool( gen, objectVector_EqualBehaviour( first, second ) );
}

static qboolean objectVector_NotEqualBehaviour( asvec3_t *first, asvec3_t *second )
{
	return !VectorCompare( first->v, second->v );
}

static void objectVector_asGeneric_NotEqualBehaviour( void *gen )
{
	asvec3_t *first = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asvec3_t *second = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 );

	G_asGeneric_SetReturnBool( gen, objectVector_NotEqualBehaviour( first, second ) );
}

static void objectVector_Set( float x, float y, float z, asvec3_t *vec )
{
	VectorSet( vec->v, x, y, z );
}

static void objectVector_asGeneric_Set( void *gen )
{
	float x = G_asGeneric_GetArgFloat( gen, 0 );
	float y = G_asGeneric_GetArgFloat( gen, 1 );
	float z = G_asGeneric_GetArgFloat( gen, 2 );
	asvec3_t *self = (asvec3_t *)G_asGeneric_GetObject( gen );

	objectVector_Set( x, y, z, self );
}

static float objectVector_Length( const asvec3_t *vec )
{
	return VectorLength( vec->v );
}

static void objectVector_asGeneric_Length( void *gen )
{
	G_asGeneric_SetReturnFloat( gen, objectVector_Length( (asvec3_t *)G_asGeneric_GetObject( gen ) ) );
}

static float objectVector_Normalize( asvec3_t *vec )
{
	return VectorNormalize( vec->v );
}

static void objectVector_asGeneric_Normalize( void *gen )
{
	G_asGeneric_SetReturnFloat( gen, objectVector_Normalize( (asvec3_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectVector_toString( asvec3_t *vec )
{
	char *s = vtos( vec->v );
	return objectString_FactoryBuffer( s, strlen( s ) );
}

static void objectVector_asGeneric_toString( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectVector_toString( (asvec3_t *)G_asGeneric_GetObject( gen ) ) );
}

static float objectVector_Distance( asvec3_t *other, asvec3_t *self )
{
	return Distance( self->v, other->v );
}

static void objectVector_asGeneric_Distance( void *gen )
{
	asvec3_t *other = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asvec3_t *self = (asvec3_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnFloat( gen, objectVector_Distance( other, self ) );
}

static void objectVector_AngleVectors( asvec3_t *f, asvec3_t *r, asvec3_t *u, asvec3_t *self )
{
	AngleVectors( self->v, f ? f->v : NULL, r ? r->v : NULL, u ? u->v : NULL );
}

static void objectVector_asGeneric_AngleVectors( void *gen )
{
	asvec3_t *f = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asvec3_t *r = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 );
	asvec3_t *u = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 2 );
	asvec3_t *self = (asvec3_t *)G_asGeneric_GetObject( gen );

	objectVector_AngleVectors( f, r, u, self );
}

static void objectVector_VecToAngles( asvec3_t *angles, asvec3_t *self )
{
	VecToAngles( self->v, angles->v );
}

static void objectVector_asGeneric_VecToAngles( void *gen )
{
	asvec3_t *angles = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asvec3_t *self = (asvec3_t *)G_asGeneric_GetObject( gen );

	objectVector_VecToAngles( angles, self );
}

static void objectVector_Perpendicular( asvec3_t *dst, asvec3_t *self )
{
	PerpendicularVector( dst->v, self->v );
}

static void objectVector_asGeneric_Perpendicular( void *gen )
{
	asvec3_t *dst = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asvec3_t *self = (asvec3_t *)G_asGeneric_GetObject( gen );

	objectVector_Perpendicular( dst, self );
}

static void objectVector_MakeNormalVectors( asvec3_t *r, asvec3_t *u, asvec3_t *self )
{
	MakeNormalVectors( self->v, r->v, u->v );
}

static void objectVector_asGeneric_MakeNormalVectors( void *gen )
{
	asvec3_t *r = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asvec3_t *u = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 );
	asvec3_t *self = (asvec3_t *)G_asGeneric_GetObject( gen );

	objectVector_MakeNormalVectors( r, u, self );
}

static const asBehavior_t asvector_ObjectBehaviors[] =
{
	{ asBEHAVE_FACTORY, SCRIPT_FUNCTION_DECL(cVec3 @, f, ()), objectVector_Factory, objectVector_asGeneric_Factory, asCALL_CDECL },
	{ asBEHAVE_ADDREF, SCRIPT_FUNCTION_DECL(void, f, ()), objectVector_Addref, objectVector_asGeneric_Addref, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_RELEASE, SCRIPT_FUNCTION_DECL(void, f, ()), objectVector_Release, objectVector_asGeneric_Release, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_FACTORY, SCRIPT_FUNCTION_DECL(cVec3 @, f, (float x, float y, float z)), objectVector_FactorySet, objectVector_asGeneric_FactorySet, asCALL_CDECL },
	{ asBEHAVE_FACTORY, SCRIPT_FUNCTION_DECL(cVec3 @, f, (float v)), objectVector_FactorySet2, objectVector_asGeneric_FactorySet2, asCALL_CDECL },
	{ asBEHAVE_ASSIGNMENT, SCRIPT_FUNCTION_DECL(cVec3 &, f, (cVec3 &in)), objectVector_AssignBehaviour, objectVector_asGeneric_AssignBehaviour, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_ASSIGNMENT, SCRIPT_FUNCTION_DECL(cVec3 &, f, (int)), objectVector_AssignBehaviourI, objectVector_asGeneric_AssignBehaviourI, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_ASSIGNMENT, SCRIPT_FUNCTION_DECL(cVec3 &, f, (float)), objectVector_AssignBehaviourF, objectVector_asGeneric_AssignBehaviourF, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_ASSIGNMENT, SCRIPT_FUNCTION_DECL(cVec3 &, f, (double)), objectVector_AssignBehaviourD, objectVector_asGeneric_AssignBehaviourD, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_ADD_ASSIGN, SCRIPT_FUNCTION_DECL(cVec3 &, f, (cVec3 &in)), objectVector_AddAssignBehaviour, objectVector_asGeneric_AddAssignBehaviour, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_SUB_ASSIGN, SCRIPT_FUNCTION_DECL(cVec3 &, f, (cVec3 &in)), objectVector_SubAssignBehaviour, objectVector_asGeneric_SubAssignBehaviour, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_MUL_ASSIGN, SCRIPT_FUNCTION_DECL(cVec3 &, f, (cVec3 &in)), objectVector_MulAssignBehaviour, objectVector_asGeneric_MulAssignBehaviour, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_XOR_ASSIGN, SCRIPT_FUNCTION_DECL(cVec3 &, f, (cVec3 &in)), objectVector_XORAssignBehaviour, objectVector_asGeneric_XORAssignBehaviour, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_MUL_ASSIGN, SCRIPT_FUNCTION_DECL(cVec3 &, f, (int)), objectVector_MulAssignBehaviourI, objectVector_asGeneric_MulAssignBehaviourI, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_MUL_ASSIGN, SCRIPT_FUNCTION_DECL(cVec3 &, f, (float)), objectVector_MulAssignBehaviourF, objectVector_asGeneric_MulAssignBehaviourF, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_MUL_ASSIGN, SCRIPT_FUNCTION_DECL(cVec3 &, f, (double)), objectVector_MulAssignBehaviourD, objectVector_asGeneric_MulAssignBehaviourD, asCALL_CDECL_OBJLAST },

	SCRIPT_BEHAVIOR_NULL
};

static const asBehavior_t asvector_GlobalBehaviors[] =
{
	{ asBEHAVE_ADD, SCRIPT_FUNCTION_DECL(cVec3 @, f, (const cVec3 &in, const cVec3 &in)), objectVector_AddBehaviour, objectVector_asGeneric_AddBehaviour, asCALL_CDECL },
	{ asBEHAVE_SUBTRACT, SCRIPT_FUNCTION_DECL(cVec3 @, f, (const cVec3 &in, const cVec3 &in)), objectVector_SubtractBehaviour, objectVector_asGeneric_SubtractBehaviour, asCALL_CDECL },
	{ asBEHAVE_MULTIPLY, SCRIPT_FUNCTION_DECL(float, f, (const cVec3 &in, const cVec3 &in)), objectVector_MultiplyBehaviour, objectVector_asGeneric_MultiplyBehaviour, asCALL_CDECL },
	{ asBEHAVE_MULTIPLY, SCRIPT_FUNCTION_DECL(cVec3 @, f, (const cVec3 &in, double)), objectVector_MultiplyBehaviourVD, objectVector_asGeneric_MultiplyBehaviourVD, asCALL_CDECL },
	{ asBEHAVE_MULTIPLY, SCRIPT_FUNCTION_DECL(cVec3 @, f, (double, const cVec3 &in)), objectVector_MultiplyBehaviourDV, objectVector_asGeneric_MultiplyBehaviourDV, asCALL_CDECL },
	{ asBEHAVE_MULTIPLY, SCRIPT_FUNCTION_DECL(cVec3 @, f, (const cVec3 &in, float)), objectVector_MultiplyBehaviourVF, objectVector_asGeneric_MultiplyBehaviourVF, asCALL_CDECL },
	{ asBEHAVE_MULTIPLY, SCRIPT_FUNCTION_DECL(cVec3 @, f, (float, const cVec3 &in)), objectVector_MultiplyBehaviourFV, objectVector_asGeneric_MultiplyBehaviourFV, asCALL_CDECL },
	{ asBEHAVE_MULTIPLY, SCRIPT_FUNCTION_DECL(cVec3 @, f, (const cVec3 &in, int)), objectVector_MultiplyBehaviourVI, objectVector_asGeneric_MultiplyBehaviourVI, asCALL_CDECL },
	{ asBEHAVE_MULTIPLY, SCRIPT_FUNCTION_DECL(cVec3 @, f, (int, const cVec3 &in)), objectVector_MultiplyBehaviourIV, objectVector_asGeneric_MultiplyBehaviourIV, asCALL_CDECL },
	{ asBEHAVE_BIT_XOR, SCRIPT_FUNCTION_DECL(cVec3 @, f, (const cVec3 &in, const cVec3 &in)), objectVector_XORBehaviour, objectVector_asGeneric_XORBehaviour, asCALL_CDECL },
	{ asBEHAVE_EQUAL, SCRIPT_FUNCTION_DECL(bool, f, (const cVec3 &in, const cVec3 &in)), objectVector_EqualBehaviour, objectVector_asGeneric_EqualBehaviour, asCALL_CDECL },
	{ asBEHAVE_NOTEQUAL, SCRIPT_FUNCTION_DECL(bool, f, (const cVec3 &in, const cVec3 &in)), objectVector_NotEqualBehaviour, objectVector_asGeneric_NotEqualBehaviour, asCALL_CDECL },

	SCRIPT_BEHAVIOR_NULL
};

static const asMethod_t asvector_Methods[] =
{
	{ SCRIPT_FUNCTION_DECL(void, set, ( float x, float y, float z )), objectVector_Set, objectVector_asGeneric_Set, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(float, length, ()), objectVector_Length, objectVector_asGeneric_Length, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(float, normalize, ()), objectVector_Normalize, objectVector_asGeneric_Normalize, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, toString, ()), objectVector_toString, objectVector_asGeneric_toString, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(float, distance, (const cVec3 &in)), objectVector_Distance, objectVector_asGeneric_Distance, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, angleVectors, (cVec3 @+, cVec3 @+, cVec3 @+)), objectVector_AngleVectors, objectVector_asGeneric_AngleVectors, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, toAngles, (cVec3 &)), objectVector_VecToAngles, objectVector_asGeneric_VecToAngles, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, perpendicular, (cVec3 &)), objectVector_Perpendicular, objectVector_asGeneric_Perpendicular, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, makeNormalVectors, (cVec3 &, cVec3 &)), objectVector_MakeNormalVectors, objectVector_asGeneric_MakeNormalVectors, asCALL_CDECL_OBJLAST },

	SCRIPT_METHOD_NULL
};

static const asProperty_t asvector_Properties[] =
{
	{ SCRIPT_PROPERTY_DECL(float, x), FOFFSET(asvec3_t, v[0]) },
	{ SCRIPT_PROPERTY_DECL(float, y), FOFFSET(asvec3_t, v[1]) },
	{ SCRIPT_PROPERTY_DECL(float, z), FOFFSET(asvec3_t, v[2]) },

	SCRIPT_PROPERTY_NULL
};

static const asClassDescriptor_t asVectorClassDescriptor =
{
	"cVec3",					/* name */
	asOBJ_REF,					/* object type flags */
	sizeof( asvec3_t ),			/* size */
	asvector_ObjectBehaviors,	/* object behaviors */
	asvector_GlobalBehaviors,	/* global behaviors */
	asvector_Methods,			/* methods */
	asvector_Properties,		/* properties */

	NULL, NULL					/* string factory hack */
};

//=======================================================================

// CLASS: cVar
static int cvar_factored_count = 0;
static int cvar_released_count = 0;

typedef struct
{
	cvar_t *cvar;
	int asFactored, asRefCount;
}ascvar_t;

static ascvar_t *objectCVar_Factory()
{
	static ascvar_t *object;

	object = G_AsMalloc( sizeof( ascvar_t ) );
	object->asRefCount = 1;
	object->asFactored = 1;
	cvar_factored_count++;
	return object;
}

static void objectCVar_asGeneric_Factory( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectCVar_Factory() );
}

static ascvar_t *objectCVar_FactoryRegister( asstring_t *name, asstring_t *def, unsigned int flags )
{
	ascvar_t *self = objectCVar_Factory();

	self->cvar = trap_Cvar_Get( name->buffer, def->buffer, flags );

	return self;
}

static void objectCVar_asGeneric_FactoryRegister( void *gen )
{
	G_asGeneric_SetReturnAddress( gen,
		objectCVar_FactoryRegister( (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 ),
									(asstring_t *)G_asGeneric_GetArgAddress( gen, 1 ),
									(unsigned int)G_asGeneric_GetArgInt( gen, 2 ) )
		);
}

static void objectCVar_Addref( ascvar_t *obj ) { obj->asRefCount++; }

static void objectCVar_asGeneric_Addref( void *gen )
{
	objectCVar_Addref( (ascvar_t *)G_asGeneric_GetObject( gen ) );
}

static void objectCVar_Release( ascvar_t *obj )
{
	obj->asRefCount--;
	clamp_low( obj->asRefCount, 0 );
	if( !obj->asRefCount && obj->asFactored )
	{
		G_AsFree( obj );
		cvar_released_count++;
	}
}

static void objectCVar_asGeneric_Release( void *gen )
{
	objectCVar_Release( (ascvar_t *)G_asGeneric_GetObject( gen ) );
}

static void objectCVar_Register( asstring_t *name, asstring_t *def, unsigned int flags, ascvar_t *self )
{
	if( !name || !def )
		return;

	self->cvar = trap_Cvar_Get( name->buffer, def->buffer, flags );
}

static void objectCVar_asGeneric_Register( void *gen )
{
	objectCVar_Register( (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 ),
		(asstring_t *)G_asGeneric_GetArgAddress( gen, 1 ),
		(unsigned int)G_asGeneric_GetArgInt( gen, 2 ),
		(ascvar_t *)G_asGeneric_GetObject( gen ) );
}

static void objectCVar_Reset( ascvar_t *self )
{
	if( !self->cvar )
		return;

	trap_Cvar_Set( self->cvar->name, self->cvar->dvalue );
}

static void objectCVar_asGeneric_Reset( void *gen )
{
	objectCVar_Reset( (ascvar_t *)G_asGeneric_GetObject( gen ) );
}

static void objectCVar_setS( asstring_t *str, ascvar_t *self )
{
	if( !str || !self->cvar )
		return;

	trap_Cvar_Set( self->cvar->name, str->buffer );
}

static void objectCVar_asGeneric_setS( void *gen )
{
	objectCVar_setS( (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 ), (ascvar_t *)G_asGeneric_GetObject( gen ) );
}

static void objectCVar_setF( float value, ascvar_t *self )
{
	if( !self->cvar )
		return;

	trap_Cvar_SetValue( self->cvar->name, value );
}

static void objectCVar_asGeneric_setF( void *gen )
{
	objectCVar_setF( G_asGeneric_GetArgFloat( gen, 0 ), (ascvar_t *)G_asGeneric_GetObject( gen ) );
}

static void objectCVar_setI( int value, ascvar_t *self )
{
	objectCVar_setF( (float)value, self );
}

static void objectCVar_asGeneric_setI( void *gen )
{
	objectCVar_setI( G_asGeneric_GetArgInt( gen, 0 ), (ascvar_t *)G_asGeneric_GetObject( gen ) );
}

static void objectCVar_setD( double value, ascvar_t *self )
{
	objectCVar_setF( (float)value, self );
}

static void objectCVar_asGeneric_setD( void *gen )
{
	objectCVar_setD( G_asGeneric_GetArgDouble( gen, 0 ), (ascvar_t *)G_asGeneric_GetObject( gen ) );
}

static void objectCVar_forceSetS( asstring_t *str, ascvar_t *self )
{
	if( !str || !self->cvar )
		return;

	trap_Cvar_ForceSet( self->cvar->name, str->buffer );
}

static void objectCVar_asGeneric_forceSetS( void *gen )
{
	objectCVar_forceSetS( (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 ), (ascvar_t *)G_asGeneric_GetObject( gen ) );
}

static void objectCVar_forceSetF( float value, ascvar_t *self )
{
	if( !self->cvar )
		return;

	trap_Cvar_ForceSet( self->cvar->name, va( "%f", value ) );
}

static void objectCVar_asGeneric_forceSetF( void *gen )
{
	objectCVar_forceSetF( G_asGeneric_GetArgFloat( gen, 0 ), (ascvar_t *)G_asGeneric_GetObject( gen ) );
}

static void objectCVar_forceSetI( int value, ascvar_t *self )
{
	objectCVar_forceSetF( (float)value, self );
}

static void objectCVar_asGeneric_forceSetI( void *gen )
{
	objectCVar_forceSetI( G_asGeneric_GetArgInt( gen, 0 ), (ascvar_t *)G_asGeneric_GetObject( gen ) );
}

static void objectCVar_forceSetD( double value, ascvar_t *self )
{
	objectCVar_forceSetF( (float)value, self );
}

static void objectCVar_asGeneric_forceSetD( void *gen )
{
	objectCVar_forceSetD( G_asGeneric_GetArgDouble( gen, 0 ), (ascvar_t *)G_asGeneric_GetObject( gen ) );
}

static qboolean objectCVar_getBool( ascvar_t *self )
{
	if( !self->cvar )
		return 0;

	return ( self->cvar->integer != 0 );
}

static void objectCVar_asGeneric_getBool( void *gen )
{
	G_asGeneric_SetReturnBool( gen, objectCVar_getBool( (ascvar_t *)G_asGeneric_GetObject( gen ) ) );
}

static qboolean objectCVar_getModified( ascvar_t *self )
{
	if( !self->cvar )
		return 0;

	return self->cvar->modified;
}

static void objectCVar_asGeneric_getModified( void *gen )
{
	G_asGeneric_SetReturnBool( gen, objectCVar_getModified( (ascvar_t *)G_asGeneric_GetObject( gen ) ) );
}

static int objectCVar_getInteger( ascvar_t *self )
{
	if( !self->cvar )
		return 0;

	return self->cvar->integer;
}

static void objectCVar_asGeneric_getInteger( void *gen )
{
	G_asGeneric_SetReturnInt( gen, objectCVar_getInteger( (ascvar_t *)G_asGeneric_GetObject( gen ) ) );
}

static float objectCVar_getValue( ascvar_t *self )
{
	if( !self->cvar )
		return 0;

	return self->cvar->value;
}

static void objectCVar_asGeneric_getValue( void *gen )
{
	G_asGeneric_SetReturnFloat( gen, objectCVar_getValue( (ascvar_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectCVar_getName( ascvar_t *self )
{
	if( !self->cvar || !self->cvar->name )
		return objectString_FactoryBuffer( NULL, 0 );

	return objectString_FactoryBuffer( self->cvar->name, strlen( self->cvar->name ) );
}

static void objectCVar_asGeneric_getName( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectCVar_getName( (ascvar_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectCVar_getString( ascvar_t *self )
{
	if( !self->cvar || !self->cvar->string )
		return objectString_FactoryBuffer( NULL, 0 );

	return objectString_FactoryBuffer( self->cvar->string, strlen( self->cvar->string ) );
}

static void objectCVar_asGeneric_getString( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectCVar_getString( (ascvar_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectCVar_getDefaultString( ascvar_t *self )
{
	if( !self->cvar || !self->cvar->dvalue )
		return objectString_FactoryBuffer( NULL, 0 );

	return objectString_FactoryBuffer( self->cvar->dvalue, strlen( self->cvar->dvalue ) );
}

static void objectCVar_asGeneric_getDefaultString( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectCVar_getDefaultString( (ascvar_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectCVar_getLatchedString( ascvar_t *self )
{
	if( !self->cvar || !self->cvar->latched_string )
		return objectString_FactoryBuffer( NULL, 0 );

	return objectString_FactoryBuffer( self->cvar->latched_string, strlen( self->cvar->latched_string ) );
}

static void objectCVar_asGeneric_getLatchedString( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectCVar_getLatchedString( (ascvar_t *)G_asGeneric_GetObject( gen ) ) );
}

static const asBehavior_t ascvar_ObjectBehaviors[] =
{
	{ asBEHAVE_FACTORY, SCRIPT_FUNCTION_DECL(cVar@, f, ()), objectCVar_Factory, objectCVar_asGeneric_Factory, asCALL_CDECL },
	{ asBEHAVE_ADDREF, SCRIPT_FUNCTION_DECL(void, f, ()), objectCVar_Addref, objectCVar_asGeneric_Addref, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_RELEASE, SCRIPT_FUNCTION_DECL(void, f, ()), objectCVar_Release, objectCVar_asGeneric_Release, asCALL_CDECL_OBJLAST },

	// alternative factory
	{ asBEHAVE_FACTORY, SCRIPT_FUNCTION_DECL(cVar@, f, ( cString &in, cString &in, uint flags )), objectCVar_FactoryRegister, objectCVar_asGeneric_FactoryRegister, asCALL_CDECL },

	SCRIPT_BEHAVIOR_NULL
};

static const asMethod_t ascvar_Methods[] =
{
	{ SCRIPT_FUNCTION_DECL(void , get, ( cString &in, cString &in, uint flags )), objectCVar_Register, objectCVar_asGeneric_Register, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void , reset, ()), objectCVar_Reset, objectCVar_asGeneric_Reset, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void , set, ( cString &in )), objectCVar_setS, objectCVar_asGeneric_setS, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void , set, ( float value )), objectCVar_setF, objectCVar_asGeneric_setF, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void , set, ( int value )), objectCVar_setI, objectCVar_asGeneric_setI, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void , set, ( double value )), objectCVar_setD, objectCVar_asGeneric_setD, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void , forceSet, ( cString &in )), objectCVar_forceSetS, objectCVar_asGeneric_forceSetS, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void , forceSet, ( float value )), objectCVar_forceSetF, objectCVar_asGeneric_forceSetF, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void , forceSet, ( int value )), objectCVar_forceSetI, objectCVar_asGeneric_forceSetI, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void , forceSet, ( double value )), objectCVar_forceSetD, objectCVar_asGeneric_forceSetD, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(bool , modified, ()), objectCVar_getModified, objectCVar_asGeneric_getModified, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(bool , getBool, ()), objectCVar_getBool, objectCVar_asGeneric_getBool, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(int , getInteger, ()), objectCVar_getInteger, objectCVar_asGeneric_getInteger, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(float , getValue, ()), objectCVar_getValue, objectCVar_asGeneric_getValue, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @ , getName, ()), objectCVar_getName, objectCVar_asGeneric_getName, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @ , getString, ()), objectCVar_getString, objectCVar_asGeneric_getString, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @ , getDefaultString, ()), objectCVar_getDefaultString, objectCVar_asGeneric_getDefaultString, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @ , getLatchedString, ()), objectCVar_getLatchedString, objectCVar_asGeneric_getLatchedString, asCALL_CDECL_OBJLAST },

	SCRIPT_METHOD_NULL
};

static const asProperty_t ascvar_Properties[] =
{
	SCRIPT_PROPERTY_NULL
};

static const asClassDescriptor_t asCVarClassDescriptor =
{
	"cVar",						/* name */
	asOBJ_REF,					/* object type flags */
	sizeof( ascvar_t ),			/* size */
	ascvar_ObjectBehaviors,		/* object behaviors */
	NULL,						/* global behaviors */
	ascvar_Methods,				/* methods */
	ascvar_Properties,			/* properties */

	NULL, NULL					/* string factory hack */
};

//=======================================================================

// CLASS: cTime
static int astime_factored_count = 0;
static int astime_released_count = 0;

typedef struct
{
	time_t time;
	struct tm localtime;
	int asFactored, asRefCount;
} astime_t;

static astime_t *objectTime_Factory( time_t time )
{
	static astime_t *object;

	object = G_AsMalloc( sizeof( *object ) );
	object->asRefCount = 1;
	object->asFactored = 1;
	object->time = time;

	if( time )
	{
		struct tm *tm;
		tm = localtime( &time );
		object->localtime = *tm;
	}

	astime_factored_count++;
	return object;
}

static void objectTime_asGeneric_Factory( void *gen )
{
	time_t time = ( time_t )G_asGeneric_GetArgInt64( gen, 0 );
	G_asGeneric_SetReturnAddress( gen, objectTime_Factory( time ) );
}

static astime_t *objectTime_FactoryEmpty( void )
{
	return objectTime_Factory( 0 );
}

static void objectTime_asGeneric_FactoryEmpty( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectTime_FactoryEmpty() );
}

static void objectTime_Addref( astime_t *obj ) { obj->asRefCount++; }

static void objectTime_asGeneric_Addref( void *gen )
{
	objectTime_Addref( (astime_t *)G_asGeneric_GetObject( gen ) );
}

static void objectTime_Release( astime_t *obj )
{
	obj->asRefCount--;
	clamp_low( obj->asRefCount, 0 );
	if( !obj->asRefCount && obj->asFactored )
	{
		G_AsFree( obj );
		astime_released_count++;
	}
}

static void objectTime_asGeneric_Release( void *gen )
{
	objectTime_Release( (astime_t *)G_asGeneric_GetObject( gen ) );
}

static astime_t *objectTime_AssignBehaviour( astime_t *other, astime_t *self )
{
	self->time = other->time;
	memcpy( &(self->localtime), &(other->localtime), sizeof( struct tm ) );
	return self;
}

static void objectTime_asGeneric_AssignBehaviour( void *gen )
{
	astime_t *other = (astime_t *)G_asGeneric_GetArgAddress( gen, 0 );
	astime_t *self = (astime_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectTime_AssignBehaviour( other, self ) );
}

static qboolean objectTime_EqualBehaviour( astime_t *first, astime_t *second )
{
	return (first->time == second->time);
}

static void objectTime_asGeneric_EqualBehaviour( void *gen )
{
	astime_t *first = (astime_t *)G_asGeneric_GetArgAddress( gen, 0 );
	astime_t *second = (astime_t *)G_asGeneric_GetArgAddress( gen, 1 );

	G_asGeneric_SetReturnBool( gen, objectTime_EqualBehaviour( first, second ) );
}

static qboolean objectTime_NotEqualBehaviour( astime_t *first, astime_t *second )
{
	return !objectTime_EqualBehaviour( first, second );
}

static void objectTime_asGeneric_NotEqualBehaviour( void *gen )
{
	astime_t *first = (astime_t *)G_asGeneric_GetArgAddress( gen, 0 );
	astime_t *second = (astime_t *)G_asGeneric_GetArgAddress( gen, 1 );

	G_asGeneric_SetReturnBool( gen, objectTime_NotEqualBehaviour( first, second ) );
}

static const asBehavior_t astime_ObjectBehaviors[] =
{
	{ asBEHAVE_FACTORY, SCRIPT_FUNCTION_DECL(cTime @, f, ()), objectTime_FactoryEmpty, objectTime_asGeneric_FactoryEmpty, asCALL_CDECL },
	{ asBEHAVE_FACTORY, SCRIPT_FUNCTION_DECL(cTime @, f, (uint64 t)), objectTime_Factory, objectTime_asGeneric_Factory, asCALL_CDECL },
	{ asBEHAVE_ADDREF, SCRIPT_FUNCTION_DECL(void, f, ()), objectTime_Addref, objectTime_asGeneric_Addref, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_RELEASE, SCRIPT_FUNCTION_DECL(void, f, ()), objectTime_Release, objectTime_asGeneric_Release, asCALL_CDECL_OBJLAST },

	/* assignments */
	{ asBEHAVE_ASSIGNMENT, SCRIPT_FUNCTION_DECL(cTime &, f, (const cTime &in)), objectTime_AssignBehaviour, objectTime_asGeneric_AssignBehaviour, asCALL_CDECL_OBJLAST },

	SCRIPT_BEHAVIOR_NULL
};

static const asBehavior_t astime_GlobalBehaviors[] =
{
	/* == != */
	{ asBEHAVE_EQUAL, SCRIPT_FUNCTION_DECL(bool, f, (const cTime &in, const cTime &in)), objectTime_EqualBehaviour, objectTime_asGeneric_EqualBehaviour, asCALL_CDECL },
	{ asBEHAVE_NOTEQUAL, SCRIPT_FUNCTION_DECL(bool, f, (const cTime &in, const cTime &in)), objectTime_NotEqualBehaviour, objectTime_asGeneric_NotEqualBehaviour, asCALL_CDECL },

	SCRIPT_BEHAVIOR_NULL
};

static const asProperty_t astime_Properties[] =
{
	{ SCRIPT_PROPERTY_DECL(const uint64, time), FOFFSET(astime_t, time) },
	{ SCRIPT_PROPERTY_DECL(const int, sec), FOFFSET(astime_t, localtime.tm_sec) },
	{ SCRIPT_PROPERTY_DECL(const int, min), FOFFSET(astime_t, localtime.tm_min) },
	{ SCRIPT_PROPERTY_DECL(const int, hour), FOFFSET(astime_t, localtime.tm_hour) },
	{ SCRIPT_PROPERTY_DECL(const int, mday), FOFFSET(astime_t, localtime.tm_mday) },
	{ SCRIPT_PROPERTY_DECL(const int, mon), FOFFSET(astime_t, localtime.tm_mon) },
	{ SCRIPT_PROPERTY_DECL(const int, year), FOFFSET(astime_t, localtime.tm_year) },
	{ SCRIPT_PROPERTY_DECL(const int, wday), FOFFSET(astime_t, localtime.tm_wday) },
	{ SCRIPT_PROPERTY_DECL(const int, yday), FOFFSET(astime_t, localtime.tm_yday) },
	{ SCRIPT_PROPERTY_DECL(const int, isdst), FOFFSET(astime_t, localtime.tm_isdst) },

	SCRIPT_PROPERTY_NULL
};

static const asClassDescriptor_t asTimeClassDescriptor =
{
	"cTime",					/* name */
	asOBJ_REF,					/* object type flags */
	sizeof( astime_t ),			/* size */
	astime_ObjectBehaviors,		/* object behaviors */
	astime_GlobalBehaviors,		/* global behaviors */
	NULL,						/* methods */
	astime_Properties,			/* properties */

	NULL, NULL					/* string factory hack */
};

//=======================================================================

// CLASS: cTrace
static int trace_factored_count = 0;
static int trace_released_count = 0;

typedef struct
{
	trace_t trace;
	int asFactored, asRefCount;
}astrace_t;

static astrace_t *objectTrace_Factory()
{
	static astrace_t *object;

	object = G_AsMalloc( sizeof( astrace_t ) );
	object->asRefCount = 1;
	object->asFactored = 1;
	trace_factored_count++;
	return object;
}

static void objectTrace_asGeneric_Factory( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectTrace_Factory() );
}

static void objectTrace_Addref( astrace_t *obj ) { obj->asRefCount++; }

static void objectTrace_asGeneric_Addref( void *gen )
{
	objectTrace_Addref( (astrace_t *)G_asGeneric_GetObject( gen ) );
}

static void objectTrace_Release( astrace_t *obj )
{
	obj->asRefCount--;
	clamp_low( obj->asRefCount, 0 );
	if( !obj->asRefCount && obj->asFactored )
	{
		G_AsFree( obj );
		trace_released_count++;
	}
}

static void objectTrace_asGeneric_Release( void *gen )
{
	objectTrace_Release( (astrace_t *)G_asGeneric_GetObject( gen ) );
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

static void objectTrace_asGeneric_doTrace( void *gen )
{
	asvec3_t *start = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asvec3_t *mins = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 );
	asvec3_t *maxs = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 2 );
	asvec3_t *end = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 3 );
	int ignore = G_asGeneric_GetArgInt( gen, 4 );
	int contentMask = G_asGeneric_GetArgInt( gen, 5 );
	astrace_t *self = (astrace_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnBool( gen, objectTrace_doTrace( start, mins, maxs, end, ignore, contentMask, self ) );
}

static asvec3_t *objectTrace_getEndPos( astrace_t *self )
{
	asvec3_t *asvec = objectVector_Factory();

	VectorCopy( self->trace.endpos, asvec->v );
	return asvec;
}

static void objectTrace_asGeneric_getEndPos( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectTrace_getEndPos( (astrace_t *)G_asGeneric_GetObject( gen ) ) );
}

static asvec3_t *objectTrace_getPlaneNormal( astrace_t *self )
{
	asvec3_t *asvec = objectVector_Factory();

	VectorCopy( self->trace.plane.normal, asvec->v );
	return asvec;
}

static void objectTrace_asGeneric_getPlaneNormal( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectTrace_getPlaneNormal( (astrace_t *)G_asGeneric_GetObject( gen ) ) );
}

static const asBehavior_t astrace_ObjectBehaviors[] =
{
	{ asBEHAVE_FACTORY, SCRIPT_FUNCTION_DECL(cTrace@, f, ()), objectTrace_Factory, objectTrace_asGeneric_Factory, asCALL_CDECL },
	{ asBEHAVE_ADDREF, SCRIPT_FUNCTION_DECL(void, f, ()), objectTrace_Addref, objectTrace_asGeneric_Addref, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_RELEASE, SCRIPT_FUNCTION_DECL(void, f, ()), objectTrace_Release, objectTrace_asGeneric_Release, asCALL_CDECL_OBJLAST },

	SCRIPT_BEHAVIOR_NULL
};

static const asMethod_t astrace_Methods[] =
{
	{ SCRIPT_FUNCTION_DECL(bool, doTrace, ( cVec3 &in, cVec3 &, cVec3 &, cVec3 &in, int ignore, int contentMask )), objectTrace_doTrace, objectTrace_asGeneric_doTrace, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cVec3 @, getEndPos, ()), objectTrace_getEndPos, objectTrace_asGeneric_getEndPos, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cVec3 @, getPlaneNormal, ()), objectTrace_getPlaneNormal, objectTrace_asGeneric_getPlaneNormal, asCALL_CDECL_OBJLAST },

	SCRIPT_METHOD_NULL
};

static const asProperty_t astrace_Properties[] =
{
	{ SCRIPT_PROPERTY_DECL(const bool, allSolid), FOFFSET(astrace_t, trace.allsolid) },
	{ SCRIPT_PROPERTY_DECL(const bool, startSolid), FOFFSET(astrace_t, trace.startsolid) },
	{ SCRIPT_PROPERTY_DECL(const float, fraction), FOFFSET(astrace_t, trace.fraction) },
	{ SCRIPT_PROPERTY_DECL(const int, surfFlags), FOFFSET(astrace_t, trace.surfFlags) },
	{ SCRIPT_PROPERTY_DECL(const int, contents), FOFFSET(astrace_t, trace.contents) },
	{ SCRIPT_PROPERTY_DECL(const int, entNum), FOFFSET(astrace_t, trace.ent) },
	{ SCRIPT_PROPERTY_DECL(const float, planeDist), FOFFSET(astrace_t, trace.plane.dist) },
	{ SCRIPT_PROPERTY_DECL(const int16, planeType), FOFFSET(astrace_t, trace.plane.type) },
	{ SCRIPT_PROPERTY_DECL(const int16, planeSignBits), FOFFSET(astrace_t, trace.plane.signbits) },

	SCRIPT_PROPERTY_NULL
};

static const asClassDescriptor_t asTraceClassDescriptor =
{
	"cTrace",					/* name */
	asOBJ_REF,					/* object type flags */
	sizeof( astrace_t ),		/* size */
	astrace_ObjectBehaviors,	/* object behaviors */
	NULL,						/* global behaviors */
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

static void objectGItem_asGeneric_Factory( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGItem_Factory() );
}

static void objectGItem_Addref( gsitem_t *obj ) { obj->asRefCount++; }

static void objectGItem_asGeneric_Addref( void *gen )
{
	objectGItem_Addref( (gsitem_t *)G_asGeneric_GetObject( gen ) );
}

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

static void objectGItem_asGeneric_Release( void *gen )
{
	objectGItem_Release( (gsitem_t *)G_asGeneric_GetObject( gen ) );
}

static asstring_t *objectGItem_getClassName( gsitem_t *self )
{
	return objectString_FactoryBuffer( self->classname, self->classname ? strlen(self->classname) : 0 );
}

static void objectGItem_asGeneric_getClassName( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGItem_getClassName( (gsitem_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectGItem_getName( gsitem_t *self )
{
	return objectString_FactoryBuffer( self->name, self->name ? strlen(self->name) : 0 );
}

static void objectGItem_asGeneric_getName( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGItem_getName( (gsitem_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectGItem_getShortName( gsitem_t *self )
{
	return objectString_FactoryBuffer( self->shortname, self->shortname ? strlen(self->shortname) : 0 );
}

static void objectGItem_asGeneric_getShortName( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGItem_getShortName( (gsitem_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectGItem_getModelName( gsitem_t *self )
{
	return objectString_FactoryBuffer( self->world_model[0], self->world_model[0] ? strlen(self->world_model[0]) : 0 );
}

static void objectGItem_asGeneric_getModelName( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGItem_getModelName( (gsitem_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectGItem_getModel2Name( gsitem_t *self )
{
	return objectString_FactoryBuffer( self->world_model[1], self->world_model[1] ? strlen(self->world_model[1]) : 0 );
}

static void objectGItem_asGeneric_getModel2Name( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGItem_getModel2Name( (gsitem_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectGItem_getIconName( gsitem_t *self )
{
	return objectString_FactoryBuffer( self->icon, self->icon ? strlen(self->icon) : 0 );
}

static void objectGItem_asGeneric_getIconName( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGItem_getIconName( (gsitem_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectGItem_getSimpleItemName( gsitem_t *self )
{
	return objectString_FactoryBuffer( self->simpleitem, self->simpleitem ? strlen(self->simpleitem) : 0 );
}

static void objectGItem_asGeneric_getSimpleItemName( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGItem_getSimpleItemName( (gsitem_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectGItem_getPickupSoundName( gsitem_t *self )
{
	return objectString_FactoryBuffer( self->pickup_sound, self->pickup_sound ? strlen(self->pickup_sound) : 0 );
}

static void objectGItem_asGeneric_getPickupSoundName( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGItem_getPickupSoundName( (gsitem_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectGItem_getColorToken( gsitem_t *self )
{
	return objectString_FactoryBuffer( self->color, self->color ? strlen(self->color) : 0 );
}

static void objectGItem_asGeneric_getColorToken( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGItem_getColorToken( (gsitem_t *)G_asGeneric_GetObject( gen ) ) );
}

static qboolean objectGItem_isPickable( gsitem_t *self )
{
	return ( self && ( self->flags & ITFLAG_PICKABLE ) ) ? qtrue : qfalse;
}

static void objectGItem_asGeneric_isPickable( void *gen )
{
	G_asGeneric_SetReturnBool( gen, objectGItem_isPickable( (gsitem_t *)G_asGeneric_GetObject( gen ) ) );
}

static qboolean objectGItem_isUsable( gsitem_t *self )
{
	return ( self && ( self->flags & ITFLAG_USABLE ) ) ? qtrue : qfalse;
}

static void objectGItem_asGeneric_isUsable( void *gen )
{
	G_asGeneric_SetReturnBool( gen, objectGItem_isUsable( (gsitem_t *)G_asGeneric_GetObject( gen ) ) );
}

static qboolean objectGItem_isDropable( gsitem_t *self )
{
	return ( self && ( self->flags & ITFLAG_DROPABLE ) ) ? qtrue : qfalse;
}

static void objectGItem_asGeneric_isDropable( void *gen )
{
	G_asGeneric_SetReturnBool( gen, objectGItem_isDropable( (gsitem_t *)G_asGeneric_GetObject( gen ) ) );
}

static const asBehavior_t asitem_ObjectBehaviors[] =
{
	{ asBEHAVE_FACTORY, SCRIPT_FUNCTION_DECL(cItem@, f, ()), objectGItem_Factory, objectGItem_asGeneric_Factory, asCALL_CDECL },
	{ asBEHAVE_ADDREF, SCRIPT_FUNCTION_DECL(void, f, ()), objectGItem_Addref, objectGItem_asGeneric_Addref, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_RELEASE, SCRIPT_FUNCTION_DECL(void, f, ()), objectGItem_Release, objectGItem_asGeneric_Release, asCALL_CDECL_OBJLAST },

	SCRIPT_BEHAVIOR_NULL
};

static const asMethod_t asitem_Methods[] =
{
	{ SCRIPT_FUNCTION_DECL(cString @, getClassname, ()), objectGItem_getClassName, objectGItem_asGeneric_getClassName, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, getClassName, ()), objectGItem_getClassName, objectGItem_asGeneric_getClassName, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, getName, ()), objectGItem_getName, objectGItem_asGeneric_getName, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, getShortName, ()), objectGItem_getShortName, objectGItem_asGeneric_getShortName, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, getModelString, ()), objectGItem_getModelName, objectGItem_asGeneric_getModelName, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, getModel2String, ()), objectGItem_getModel2Name, objectGItem_asGeneric_getModel2Name, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, getIconString, ()), objectGItem_getIconName, objectGItem_asGeneric_getIconName, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, getSimpleItemString, ()), objectGItem_getSimpleItemName, objectGItem_asGeneric_getSimpleItemName, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, getPickupSoundString, ()), objectGItem_getPickupSoundName, objectGItem_asGeneric_getPickupSoundName, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, getColorToken, ()), objectGItem_getColorToken, objectGItem_asGeneric_getColorToken, asCALL_CDECL_OBJLAST },

	{ SCRIPT_FUNCTION_DECL(bool, isPickable, ()), objectGItem_isPickable, objectGItem_asGeneric_isPickable, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(bool, isUsable, ()), objectGItem_isUsable, objectGItem_asGeneric_isUsable, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(bool, isDropable, ()), objectGItem_isDropable, objectGItem_asGeneric_isDropable, asCALL_CDECL_OBJLAST },

	SCRIPT_METHOD_NULL
};

static const asProperty_t asitem_Properties[] =
{
	{ SCRIPT_PROPERTY_DECL(const int, tag), FOFFSET(gsitem_t, tag) },
	{ SCRIPT_PROPERTY_DECL(const uint, type), FOFFSET(gsitem_t, type) },
	{ SCRIPT_PROPERTY_DECL(const int, flags), FOFFSET(gsitem_t, flags) },
	{ SCRIPT_PROPERTY_DECL(const int, quantity), FOFFSET(gsitem_t, quantity) },
	{ SCRIPT_PROPERTY_DECL(const int, inventoryMax), FOFFSET(gsitem_t, inventory_max) },
	{ SCRIPT_PROPERTY_DECL(const int, ammoTag), FOFFSET(gsitem_t, ammo_tag) },
	{ SCRIPT_PROPERTY_DECL(const int, weakAmmoTag), FOFFSET(gsitem_t, weakammo_tag) },

	SCRIPT_PROPERTY_NULL
};

static const asClassDescriptor_t asItemClassDescriptor =
{
	"cItem",					/* name */
	asOBJ_REF,					/* object type flags */
	sizeof( gsitem_t ),			/* size */
	asitem_ObjectBehaviors,		/* object behaviors */
	NULL,						/* global behaviors */
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

static void objectMatch_asGeneric_launchState( void *gen )
{
	objectMatch_launchState( G_asGeneric_GetArgInt( gen, 0 ), (match_t *)G_asGeneric_GetObject( gen ) );
}

static void objectMatch_startAutorecord( match_t *self )
{
	G_Match_Autorecord_Start();
}

static void objectMatch_asGeneric_startAutorecord( void *gen )
{
	objectMatch_startAutorecord( (match_t *)G_asGeneric_GetObject( gen ) );
}

static void objectMatch_stopAutorecord( match_t *self )
{
	G_Match_Autorecord_Stop();
}

static void objectMatch_asGeneric_stopAutorecord( void *gen )
{
	objectMatch_stopAutorecord( (match_t *)G_asGeneric_GetObject( gen ) );
}

static qboolean objectMatch_scoreLimitHit( match_t *self )
{
	return G_Match_ScorelimitHit();
}

static void objectMatch_asGeneric_scoreLimitHit( void *gen )
{
	G_asGeneric_SetReturnBool( gen,
		objectMatch_scoreLimitHit( (match_t *)G_asGeneric_GetObject( gen ) )
		);
}

static qboolean objectMatch_timeLimitHit( match_t *self )
{
	return G_Match_TimelimitHit();
}

static void objectMatch_asGeneric_timeLimitHit( void *gen )
{
	G_asGeneric_SetReturnBool( gen,
		objectMatch_timeLimitHit( (match_t *)G_asGeneric_GetObject( gen ) )
		);
}

static qboolean objectMatch_isTied( match_t *self )
{
	return G_Match_Tied();
}

static void objectMatch_asGeneric_isTied( void *gen )
{
	G_asGeneric_SetReturnBool( gen,
		objectMatch_isTied( (match_t *)G_asGeneric_GetObject( gen ) )
		);
}

static qboolean objectMatch_checkExtendPlayTime( match_t *self )
{
	return G_Match_CheckExtendPlayTime();
}

static void objectMatch_asGeneric_checkExtendPlayTime( void *gen )
{
	G_asGeneric_SetReturnBool( gen,
		objectMatch_checkExtendPlayTime( (match_t *)G_asGeneric_GetObject( gen ) )
		);
}

static qboolean objectMatch_suddenDeathFinished( match_t *self )
{
	return G_Match_SuddenDeathFinished();
}

static void objectMatch_asGeneric_suddenDeathFinished( void *gen )
{
	G_asGeneric_SetReturnBool( gen,
		objectMatch_suddenDeathFinished( (match_t *)G_asGeneric_GetObject( gen ) )
		);
}

static qboolean objectMatch_isPaused( match_t *self )
{
	return GS_MatchPaused();
}

static void objectMatch_asGeneric_isPaused( void *gen )
{
	G_asGeneric_SetReturnBool( gen,
		objectMatch_isPaused( (match_t *)G_asGeneric_GetObject( gen ) )
		);
}

static qboolean objectMatch_isWaiting( match_t *self )
{
	return GS_MatchWaiting();
}

static void objectMatch_asGeneric_isWaiting( void *gen )
{
	G_asGeneric_SetReturnBool( gen,
		objectMatch_isWaiting( (match_t *)G_asGeneric_GetObject( gen ) )
		);
}

static qboolean objectMatch_isExtended( match_t *self )
{
	return GS_MatchExtended();
}

static void objectMatch_asGeneric_isExtended( void *gen )
{
	G_asGeneric_SetReturnBool( gen,
		objectMatch_isExtended( (match_t *)G_asGeneric_GetObject( gen ) )
		);
}

static unsigned int objectMatch_duration( match_t *self )
{
	return GS_MatchDuration();
}

static void objectMatch_asGeneric_duration( void *gen )
{
	G_asGeneric_SetReturnInt( gen,
		(int)objectMatch_duration( (match_t *)G_asGeneric_GetObject( gen ) )
		);
}

static unsigned int objectMatch_startTime( match_t *self )
{
	return GS_MatchStartTime();
}

static void objectMatch_asGeneric_startTime( void *gen )
{
	G_asGeneric_SetReturnInt( gen,
		(int)objectMatch_startTime( (match_t *)G_asGeneric_GetObject( gen ) )
		);
}

static unsigned int objectMatch_endTime( match_t *self )
{
	return GS_MatchStartTime();
}

static void objectMatch_asGeneric_endTime( void *gen )
{
	G_asGeneric_SetReturnInt( gen,
		(int)objectMatch_endTime( (match_t *)G_asGeneric_GetObject( gen ) )
		);
}

static int objectMatch_getState( match_t *self )
{
	return GS_MatchState();
}

static void objectMatch_asGeneric_getState( void *gen )
{
	G_asGeneric_SetReturnInt( gen,
		objectMatch_getState( (match_t *)G_asGeneric_GetObject( gen ) )
		);
}

static asstring_t *objectMatch_getName( match_t *self )
{
	const char *s = trap_GetConfigString( CS_MATCHNAME );

	return StringFactory( strlen( s ), s );
}

static void objectMatch_asGeneric_getName( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectMatch_getName( (match_t *)G_asGeneric_GetObject( gen ) ) );
}

static void objectMatch_setName( asstring_t *name, match_t *self )
{
	char buf[MAX_CONFIGSTRING_CHARS];

	COM_SanitizeColorString( name->buffer, buf, sizeof( buf ), -1, COLOR_WHITE );

	trap_ConfigString( CS_MATCHNAME, buf );
}

static void objectMatch_asGeneric_setName( void *gen )
{
	asstring_t *str = (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 );
	objectMatch_setName( str, (match_t *)G_asGeneric_GetObject( gen ) );
}

static void objectMatch_setClockOverride( unsigned int time, match_t *self )
{
	gs.gameState.longstats[GAMELONG_CLOCKOVERRIDE] = time;
}

static void objectMatch_asGeneric_setClockOverride( void *gen )
{
	objectMatch_setClockOverride( G_asGeneric_GetArgInt( gen, 0 ), (match_t *)G_asGeneric_GetObject( gen ) );
}

static const asBehavior_t match_ObjectBehaviors[] =
{
	SCRIPT_BEHAVIOR_NULL
};

static const asMethod_t match_Methods[] =
{
	{ SCRIPT_FUNCTION_DECL(void, launchState, (int state)), objectMatch_launchState, objectMatch_asGeneric_launchState, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, startAutorecord, ()), objectMatch_startAutorecord, objectMatch_asGeneric_startAutorecord, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, stopAutorecord, ()), objectMatch_stopAutorecord, objectMatch_asGeneric_stopAutorecord, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(bool, scoreLimitHit, ()), objectMatch_scoreLimitHit, objectMatch_asGeneric_scoreLimitHit, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(bool, timeLimitHit, ()), objectMatch_timeLimitHit, objectMatch_asGeneric_timeLimitHit, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(bool, isTied, ()), objectMatch_isTied, objectMatch_asGeneric_isTied, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(bool, checkExtendPlayTime, ()), objectMatch_checkExtendPlayTime, objectMatch_asGeneric_checkExtendPlayTime, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(bool, suddenDeathFinished, ()), objectMatch_suddenDeathFinished, objectMatch_asGeneric_suddenDeathFinished, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(bool, isPaused, ()), objectMatch_isPaused, objectMatch_asGeneric_isPaused, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(bool, isWaiting, ()), objectMatch_isWaiting, objectMatch_asGeneric_isWaiting, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(bool, isExtended, ()), objectMatch_isExtended, objectMatch_asGeneric_isExtended, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(uint, duration, ()), objectMatch_duration, objectMatch_asGeneric_duration, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(uint, startTime, ()), objectMatch_startTime, objectMatch_asGeneric_startTime, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(uint, endTime, ()), objectMatch_endTime, objectMatch_asGeneric_endTime, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(int, getState, ()), objectMatch_getState, objectMatch_asGeneric_getState, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, getName, ()), objectMatch_getName, objectMatch_asGeneric_getName, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, setName, ( cString &in )), objectMatch_setName, objectMatch_asGeneric_setName, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, setClockOverride, ( uint milliseconds )), objectMatch_setClockOverride, objectMatch_asGeneric_setClockOverride, asCALL_CDECL_OBJLAST },

	SCRIPT_METHOD_NULL
};

static const asProperty_t match_Properties[] =
{
	SCRIPT_PROPERTY_NULL
};

static const asClassDescriptor_t asMatchClassDescriptor =
{
	"cMatch",					/* name */
	asOBJ_REF|asOBJ_NOHANDLE,	/* object type flags */
	sizeof( match_t ),		/* size */
	match_ObjectBehaviors, /* object behaviors */
	NULL,						/* global behaviors */
	match_Methods,				/* methods */
	match_Properties,			/* properties */

	NULL, NULL					/* string factory hack */
};

//=======================================================================

// CLASS: cGametypeDesc

static asstring_t *objectGametypeDescriptor_getTitle( gametype_descriptor_t *self )
{
	const char *s = trap_GetConfigString( CS_GAMETYPETITLE );

	return StringFactory( strlen( s ), s );
}

static void objectGametypeDescriptor_asGeneric_getTitle( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGametypeDescriptor_getTitle( (gametype_descriptor_t *)G_asGeneric_GetObject( gen ) ) );
}

static void objectGametypeDescriptor_setTitle( asstring_t *other, gametype_descriptor_t *self )
{
	if( !other || !other->buffer )
		return;

	trap_ConfigString( CS_GAMETYPETITLE, other->buffer );
}

static void objectGametypeDescriptor_asGeneric_setTitle( void *gen )
{
	objectGametypeDescriptor_setTitle( (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 ), (gametype_descriptor_t *)G_asGeneric_GetObject( gen ) );
}

static asstring_t *objectGametypeDescriptor_getName( gametype_descriptor_t *self )
{
	return StringFactory( strlen(gs.gametypeName), gs.gametypeName );
}

static void objectGametypeDescriptor_asGeneric_getName( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGametypeDescriptor_getName( (gametype_descriptor_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectGametypeDescriptor_getVersion( gametype_descriptor_t *self )
{
	const char *s = trap_GetConfigString( CS_GAMETYPEVERSION );

	return StringFactory( strlen( s ), s );
}

static void objectGametypeDescriptor_asGeneric_getVersion( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGametypeDescriptor_getVersion( (gametype_descriptor_t *)G_asGeneric_GetObject( gen ) ) );
}

static void objectGametypeDescriptor_setVersion( asstring_t *other, gametype_descriptor_t *self )
{
	if( !other || !other->buffer )
		return;

	trap_ConfigString( CS_GAMETYPEVERSION, other->buffer );
}

static void objectGametypeDescriptor_asGeneric_setVersion( void *gen )
{
	objectGametypeDescriptor_setVersion( (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 ), (gametype_descriptor_t *)G_asGeneric_GetObject( gen ) );
}

static asstring_t *objectGametypeDescriptor_getAuthor( gametype_descriptor_t *self )
{
	const char *s = trap_GetConfigString( CS_GAMETYPEAUTHOR );

	return StringFactory( strlen( s ), s );
}

static void objectGametypeDescriptor_asGeneric_getAuthor( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGametypeDescriptor_getAuthor( (gametype_descriptor_t *)G_asGeneric_GetObject( gen ) ) );
}

static void objectGametypeDescriptor_setAuthor( asstring_t *other, gametype_descriptor_t *self )
{
	if( !other || !other->buffer )
		return;

	trap_ConfigString( CS_GAMETYPEAUTHOR, other->buffer );
}

static void objectGametypeDescriptor_asGeneric_setAuthor( void *gen )
{
	objectGametypeDescriptor_setAuthor( (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 ), (gametype_descriptor_t *)G_asGeneric_GetObject( gen ) );
}

static asstring_t *objectGametypeDescriptor_getManifest( gametype_descriptor_t *self )
{
	const char *s = trap_GetConfigString( CS_MODMANIFEST );

	return StringFactory( strlen( s ), s );
}

static void objectGametypeDescriptor_asGeneric_getManifest( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGametypeDescriptor_getManifest( (gametype_descriptor_t *)G_asGeneric_GetObject( gen ) ) );
}

static void objectGametypeDescriptor_SetTeamSpawnsystem( int team, int spawnsystem, int wave_time, int wave_maxcount, qboolean spectate_team, gametype_descriptor_t *self )
{
	G_SpawnQueue_SetTeamSpawnsystem( team, spawnsystem, wave_time, wave_maxcount, spectate_team );
}

static void objectGametypeDescriptor_asGeneric_SetTeamSpawnsystem( void *gen )
{
	objectGametypeDescriptor_SetTeamSpawnsystem(
		G_asGeneric_GetArgInt( gen, 0 ),
		G_asGeneric_GetArgInt( gen, 1 ),
		G_asGeneric_GetArgInt( gen, 2 ),
		G_asGeneric_GetArgInt( gen, 3 ),
		G_asGeneric_GetArgBool( gen, 4 ),
		(gametype_descriptor_t *)G_asGeneric_GetObject( gen ) );
}

static qboolean objectGametypeDescriptor_isInstagib( gametype_descriptor_t *self )
{
	return GS_Instagib();
}

static void objectGametypeDescriptor_asGeneric_isInstagib( void *gen )
{
	G_asGeneric_SetReturnBool( gen, objectGametypeDescriptor_isInstagib( (gametype_descriptor_t *)G_asGeneric_GetObject( gen ) ) );
}

static qboolean objectGametypeDescriptor_hasFallDamage( gametype_descriptor_t *self )
{
	return GS_FallDamage();
}

static void objectGametypeDescriptor_asGeneric_hasFallDamage( void *gen )
{
	G_asGeneric_SetReturnBool( gen, objectGametypeDescriptor_hasFallDamage( (gametype_descriptor_t *)G_asGeneric_GetObject( gen ) ) );
}

static qboolean objectGametypeDescriptor_hasSelfDamage( gametype_descriptor_t *self )
{
	return GS_SelfDamage();
}

static void objectGametypeDescriptor_asGeneric_hasSelfDamage( void *gen )
{
	G_asGeneric_SetReturnBool( gen, objectGametypeDescriptor_hasSelfDamage( (gametype_descriptor_t *)G_asGeneric_GetObject( gen ) ) );
}

static qboolean objectGametypeDescriptor_isInvidualGameType( gametype_descriptor_t *self )
{
	return GS_InvidualGameType();
}

static void objectGametypeDescriptor_asGeneric_isInvidualGameType( void *gen )
{
	G_asGeneric_SetReturnBool( gen, objectGametypeDescriptor_isInvidualGameType( (gametype_descriptor_t *)G_asGeneric_GetObject( gen ) ) );
}

static const asBehavior_t gametypedescr_ObjectBehaviors[] =
{
	SCRIPT_BEHAVIOR_NULL
};

static const asMethod_t gametypedescr_Methods[] =
{
	{ SCRIPT_FUNCTION_DECL(cString @, getName, ()), objectGametypeDescriptor_getName, objectGametypeDescriptor_asGeneric_getName, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, getTitle, ()), objectGametypeDescriptor_getTitle, objectGametypeDescriptor_asGeneric_getTitle, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, setTitle, ( cString & )), objectGametypeDescriptor_setTitle, objectGametypeDescriptor_asGeneric_setTitle, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, getVersion, ()), objectGametypeDescriptor_getVersion, objectGametypeDescriptor_asGeneric_getVersion, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, setVersion, ( cString & )), objectGametypeDescriptor_setVersion, objectGametypeDescriptor_asGeneric_setVersion, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, getAuthor, ()), objectGametypeDescriptor_getAuthor, objectGametypeDescriptor_asGeneric_getAuthor, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, setAuthor, ( cString & )), objectGametypeDescriptor_setAuthor, objectGametypeDescriptor_asGeneric_setAuthor, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, getManifest, ()), objectGametypeDescriptor_getManifest, objectGametypeDescriptor_asGeneric_getManifest, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, setTeamSpawnsystem, ( int team, int spawnsystem, int wave_time, int wave_maxcount, bool deadcam )), objectGametypeDescriptor_SetTeamSpawnsystem, objectGametypeDescriptor_asGeneric_SetTeamSpawnsystem, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(bool, isInstagib, ()), objectGametypeDescriptor_isInstagib, objectGametypeDescriptor_asGeneric_isInstagib, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(bool, hasFallDamage, ()), objectGametypeDescriptor_hasFallDamage, objectGametypeDescriptor_asGeneric_hasFallDamage, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(bool, hasSelfDamage, ()), objectGametypeDescriptor_hasSelfDamage, objectGametypeDescriptor_asGeneric_hasSelfDamage, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(bool, isInvidualGameType, ()), objectGametypeDescriptor_isInvidualGameType, objectGametypeDescriptor_asGeneric_isInvidualGameType, asCALL_CDECL_OBJLAST },

	SCRIPT_METHOD_NULL
};

static const asProperty_t gametypedescr_Properties[] =
{
	{ SCRIPT_PROPERTY_DECL(uint, spawnableItemsMask), FOFFSET(gametype_descriptor_t, spawnableItemsMask) },
	{ SCRIPT_PROPERTY_DECL(uint, respawnableItemsMask), FOFFSET(gametype_descriptor_t, respawnableItemsMask) },
	{ SCRIPT_PROPERTY_DECL(uint, dropableItemsMask), FOFFSET(gametype_descriptor_t, dropableItemsMask) },
	{ SCRIPT_PROPERTY_DECL(uint, pickableItemsMask), FOFFSET(gametype_descriptor_t, pickableItemsMask) },
	{ SCRIPT_PROPERTY_DECL(bool, isTeamBased), FOFFSET(gametype_descriptor_t, isTeamBased) },
	{ SCRIPT_PROPERTY_DECL(bool, isRace), FOFFSET(gametype_descriptor_t, isRace) },
	{ SCRIPT_PROPERTY_DECL(bool, hasChallengersQueue), FOFFSET(gametype_descriptor_t, hasChallengersQueue) },
	{ SCRIPT_PROPERTY_DECL(int, maxPlayersPerTeam), FOFFSET(gametype_descriptor_t, maxPlayersPerTeam) },
	{ SCRIPT_PROPERTY_DECL(int, ammoRespawn), FOFFSET(gametype_descriptor_t, ammo_respawn) },
	{ SCRIPT_PROPERTY_DECL(int, armorRespawn), FOFFSET(gametype_descriptor_t, armor_respawn) },
	{ SCRIPT_PROPERTY_DECL(int, weaponRespawn), FOFFSET(gametype_descriptor_t, weapon_respawn) },
	{ SCRIPT_PROPERTY_DECL(int, healthRespawn), FOFFSET(gametype_descriptor_t, health_respawn) },
	{ SCRIPT_PROPERTY_DECL(int, powerupRespawn), FOFFSET(gametype_descriptor_t, powerup_respawn) },
	{ SCRIPT_PROPERTY_DECL(int, megahealthRespawn), FOFFSET(gametype_descriptor_t, megahealth_respawn) },
	{ SCRIPT_PROPERTY_DECL(int, ultrahealthRespawn), FOFFSET(gametype_descriptor_t, ultrahealth_respawn) },
	{ SCRIPT_PROPERTY_DECL(bool, readyAnnouncementEnabled), FOFFSET(gametype_descriptor_t, readyAnnouncementEnabled) },
	{ SCRIPT_PROPERTY_DECL(bool, scoreAnnouncementEnabled), FOFFSET(gametype_descriptor_t, scoreAnnouncementEnabled) },
	{ SCRIPT_PROPERTY_DECL(bool, countdownEnabled), FOFFSET(gametype_descriptor_t, countdownEnabled) },
	{ SCRIPT_PROPERTY_DECL(bool, mathAbortDisabled), FOFFSET(gametype_descriptor_t, mathAbortDisabled) },
	{ SCRIPT_PROPERTY_DECL(bool, shootingDisabled), FOFFSET(gametype_descriptor_t, shootingDisabled) },
	{ SCRIPT_PROPERTY_DECL(bool, infiniteAmmo), FOFFSET(gametype_descriptor_t, infiniteAmmo) },
	{ SCRIPT_PROPERTY_DECL(bool, canForceModels), FOFFSET(gametype_descriptor_t, canForceModels) },
	{ SCRIPT_PROPERTY_DECL(bool, canShowMinimap), FOFFSET(gametype_descriptor_t, canShowMinimap) },
	{ SCRIPT_PROPERTY_DECL(bool, teamOnlyMinimap), FOFFSET(gametype_descriptor_t, teamOnlyMinimap) },
	{ SCRIPT_PROPERTY_DECL(int, spawnpointRadius), FOFFSET(gametype_descriptor_t, spawnpoint_radius) },
	{ SCRIPT_PROPERTY_DECL(bool, customDeadBodyCam), FOFFSET(gametype_descriptor_t, customDeadBodyCam) },

	//racesow
	{ SCRIPT_PROPERTY_DECL(bool, autoInactivityRemove), FOFFSET(gametype_descriptor_t, autoInactivityRemove) },
	{ SCRIPT_PROPERTY_DECL(bool, playerInteraction), FOFFSET(gametype_descriptor_t, playerInteraction) },
	{ SCRIPT_PROPERTY_DECL(bool, freestyleMapFix), FOFFSET(gametype_descriptor_t, freestyleMapFix) },
	{ SCRIPT_PROPERTY_DECL(bool, enableDrowning), FOFFSET(gametype_descriptor_t, enableDrowning) },
	//!racesow

	SCRIPT_PROPERTY_NULL
};

static const asClassDescriptor_t asGametypeClassDescriptor =
{
	"cGametypeDesc",				/* name */
	asOBJ_REF|asOBJ_NOHANDLE,		/* object type flags */
	sizeof( gametype_descriptor_t ), /* size */
	gametypedescr_ObjectBehaviors, /* object behaviors */
	NULL,						/* global behaviors */
	gametypedescr_Methods,		/* methods */
	gametypedescr_Properties,	/* properties */

	NULL, NULL					/* string factory hack */
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

static void objectTeamlist_asGeneric_Factory( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectTeamlist_Factory() );
}

static void objectTeamlist_Addref( g_teamlist_t *obj ) { obj->asRefCount++; }

static void objectTeamlist_asGeneric_Addref( void *gen )
{
	objectTeamlist_Addref( (g_teamlist_t *)G_asGeneric_GetObject( gen ) );
}

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

static void objectTeamlist_asGeneric_Release( void *gen )
{
	objectTeamlist_Release( (g_teamlist_t *)G_asGeneric_GetObject( gen ) );
}

static edict_t *objectTeamlist_GetPlayerEntity( int index, g_teamlist_t *obj )
{
	if( index < 0 || index >= obj->numplayers )
		return NULL;

	if( obj->playerIndices[index] < 1 || obj->playerIndices[index] > gs.maxclients )
		return NULL;

	return &game.edicts[ obj->playerIndices[index] ];
}

static void objectTeamlist_asGeneric_GetPlayerEntity( void *gen )
{
	int index = G_asGeneric_GetArgInt( gen, 0 );
	g_teamlist_t *self = (g_teamlist_t *)G_asGeneric_GetObject( gen );

	G_asGeneric_SetReturnAddress( gen, objectTeamlist_GetPlayerEntity( index, self ) );
}

static asstring_t *objectTeamlist_getName( g_teamlist_t *obj )
{
	const char *name = GS_TeamName( obj - teamlist );

	return objectString_FactoryBuffer( name, name ? strlen( name ) : 0 );
}

static void objectTeamlist_asGeneric_getName( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectTeamlist_getName( (g_teamlist_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectTeamlist_getDefaultName( g_teamlist_t *obj )
{
	const char *name = GS_DefaultTeamName( obj - teamlist );

	return objectString_FactoryBuffer( name, name ? strlen( name ) : 0 );
}

static void objectTeamlist_asGeneric_getDefaultName( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectTeamlist_getDefaultName( (g_teamlist_t *)G_asGeneric_GetObject( gen ) ) );
}

static void objectTeamlist_setName( asstring_t *str, g_teamlist_t *obj )
{
	int team;
	char buf[MAX_CONFIGSTRING_CHARS];

	team = obj - teamlist;
	if( team < TEAM_ALPHA && team > TEAM_BETA )
		return;

	COM_SanitizeColorString( str->buffer, buf, sizeof( buf ), -1, COLOR_WHITE );

	trap_ConfigString( CS_TEAM_ALPHA_NAME + team - TEAM_ALPHA, buf );
}

static void objectTeamlist_asGeneric_setName( void *gen )
{
	asstring_t *str = (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 );
	objectTeamlist_setName( str, (g_teamlist_t *)G_asGeneric_GetObject( gen ) );
}

static qboolean objectTeamlist_IsLocked( g_teamlist_t *obj )
{
	return G_Teams_TeamIsLocked( obj - teamlist );
}

static void objectTeamlist_asGeneric_IsLocked( void *gen )
{
	G_asGeneric_SetReturnBool( gen, objectTeamlist_IsLocked( (g_teamlist_t *)G_asGeneric_GetObject( gen ) ) );
}

static qboolean objectTeamlist_Lock( g_teamlist_t *obj )
{
	return ( obj ? G_Teams_LockTeam( obj - teamlist ) : qfalse );
}

static void objectTeamlist_asGeneric_Lock( void *gen )
{
	G_asGeneric_SetReturnBool( gen, objectTeamlist_Lock( (g_teamlist_t *)G_asGeneric_GetObject( gen ) ) );
}

static qboolean objectTeamlist_Unlock( g_teamlist_t *obj )
{
	return ( obj ? G_Teams_UnLockTeam( obj - teamlist ) : qfalse );
}

static void objectTeamlist_asGeneric_Unlock( void *gen )
{
	G_asGeneric_SetReturnBool( gen, objectTeamlist_Unlock( (g_teamlist_t *)G_asGeneric_GetObject( gen ) ) );
}

static void objectTeamlist_ClearInvites( g_teamlist_t *obj )
{
	obj->invited[0] = 0;
}

static void objectTeamlist_asGeneric_ClearInvites( void *gen )
{
	objectTeamlist_ClearInvites( (g_teamlist_t *)G_asGeneric_GetObject( gen ) );
}

static int objectTeamlist_getTeamIndex( g_teamlist_t *obj )
{
	int index = ( obj - teamlist );

	if( index < 0 || index >= GS_MAX_TEAMS )
		return -1;

	return index;
}

static void objectTeamlist_asGeneric_getTeamIndex( void *gen )
{
	G_asGeneric_SetReturnInt( gen, objectTeamlist_getTeamIndex( (g_teamlist_t *)G_asGeneric_GetObject( gen ) ) );
}

static const asBehavior_t teamlist_ObjectBehaviors[] =
{
	{ asBEHAVE_FACTORY, SCRIPT_FUNCTION_DECL(cTeam @, f, ()), objectTeamlist_Factory, objectTeamlist_asGeneric_Factory, asCALL_CDECL },
	{ asBEHAVE_ADDREF, SCRIPT_FUNCTION_DECL(void, f, ()), objectTeamlist_Addref, objectTeamlist_asGeneric_Addref, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_RELEASE, SCRIPT_FUNCTION_DECL(void, f, ()), objectTeamlist_Release, objectTeamlist_asGeneric_Release, asCALL_CDECL_OBJLAST },

	SCRIPT_BEHAVIOR_NULL
};

static const asMethod_t teamlist_Methods[] =
{
	{ SCRIPT_FUNCTION_DECL(cEntity @, ent, ( int index )), objectTeamlist_GetPlayerEntity, objectTeamlist_asGeneric_GetPlayerEntity, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, getName, ()), objectTeamlist_getName, objectTeamlist_asGeneric_getName, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, getDefaultName, ()), objectTeamlist_getDefaultName, objectTeamlist_asGeneric_getDefaultName, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, setName, ( cString &in )), objectTeamlist_setName, objectTeamlist_asGeneric_setName, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(bool, isLocked, ()), objectTeamlist_IsLocked, objectTeamlist_asGeneric_IsLocked, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(bool, lock, ()), objectTeamlist_Lock, objectTeamlist_asGeneric_Lock, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(bool, unlock, ()), objectTeamlist_Unlock, objectTeamlist_asGeneric_Unlock, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, clearInvites, ()), objectTeamlist_ClearInvites, objectTeamlist_asGeneric_ClearInvites, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(int, team, ()), objectTeamlist_getTeamIndex, objectTeamlist_asGeneric_getTeamIndex, asCALL_CDECL_OBJLAST },

	SCRIPT_METHOD_NULL
};

static const asProperty_t teamlist_Properties[] =
{
	{ SCRIPT_PROPERTY_DECL(cStats, stats), FOFFSET(g_teamlist_t, stats) },
	{ SCRIPT_PROPERTY_DECL(const int, numPlayers), FOFFSET(g_teamlist_t, numplayers) },
	{ SCRIPT_PROPERTY_DECL(const int, ping), FOFFSET(g_teamlist_t, ping) },
	{ SCRIPT_PROPERTY_DECL(const bool, hasCoach), FOFFSET(g_teamlist_t, has_coach) },

	SCRIPT_PROPERTY_NULL
};

static const asClassDescriptor_t asTeamListClassDescriptor =
{
	"cTeam",					/* name */
	asOBJ_REF,					/* object type flags */
	sizeof( g_teamlist_t ),		/* size */
	teamlist_ObjectBehaviors,	/* object behaviors */
	NULL,						/* global behaviors */
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

static void objectScoreStats_asGeneric_Factory( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectScoreStats_Factory() );
}

static void objectScoreStats_Addref( score_stats_t *obj ) { obj->asRefCount++; }

static void objectScoreStats_asGeneric_Addref( void *gen )
{
	objectScoreStats_Addref( (score_stats_t *)G_asGeneric_GetObject( gen ) );
}

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

static void objectScoreStats_asGeneric_Release( void *gen )
{
	objectScoreStats_Release( (score_stats_t *)G_asGeneric_GetObject( gen ) );
}

static void objectScoreStats_Clear( score_stats_t *obj )
{
	memset( obj, 0, sizeof( *obj ) );
}

static void objectScoreStats_asGeneric_Clear( void *gen )
{
	objectScoreStats_Clear( (score_stats_t *)G_asGeneric_GetObject( gen ) );
}

static int objectScoreStats_AccShots( int ammo, score_stats_t *obj )
{
	if( ammo < AMMO_GUNBLADE || ammo >= AMMO_TOTAL )
		return 0;

	return obj->accuracy_shots[ ammo - AMMO_GUNBLADE ];
}

static void objectScoreStats_asGeneric_AccShots( void *gen )
{
	int ammo = G_asGeneric_GetArgInt( gen, 0 );

	G_asGeneric_SetReturnInt( gen, objectScoreStats_AccShots( ammo, (score_stats_t *)G_asGeneric_GetObject( gen ) ) );
}

static int objectScoreStats_AccHits( int ammo, score_stats_t *obj )
{
	if( ammo < AMMO_GUNBLADE || ammo >= AMMO_TOTAL )
		return 0;

	return obj->accuracy_hits[ ammo - AMMO_GUNBLADE ];
}

static void objectScoreStats_asGeneric_AccHits( void *gen )
{
	int ammo = G_asGeneric_GetArgInt( gen, 0 );

	G_asGeneric_SetReturnInt( gen, objectScoreStats_AccHits( ammo, (score_stats_t *)G_asGeneric_GetObject( gen ) ) );
}

static int objectScoreStats_AccHitsDirect( int ammo, score_stats_t *obj )
{
	if( ammo < AMMO_GUNBLADE || ammo >= AMMO_TOTAL )
		return 0;

	return obj->accuracy_hits_direct[ ammo - AMMO_GUNBLADE ];
}

static void objectScoreStats_asGeneric_AccHitsDirect( void *gen )
{
	int ammo = G_asGeneric_GetArgInt( gen, 0 );

	G_asGeneric_SetReturnInt( gen, objectScoreStats_AccHitsDirect( ammo, (score_stats_t *)G_asGeneric_GetObject( gen ) ) );
}

static int objectScoreStats_AccHitsAir( int ammo, score_stats_t *obj )
{
	if( ammo < AMMO_GUNBLADE || ammo >= AMMO_TOTAL )
		return 0;

	return obj->accuracy_hits_air[ ammo - AMMO_GUNBLADE ];
}

static void objectScoreStats_asGeneric_AccHitsAir( void *gen )
{
	int ammo = G_asGeneric_GetArgInt( gen, 0 );

	G_asGeneric_SetReturnInt( gen, objectScoreStats_AccHitsAir( ammo, (score_stats_t *)G_asGeneric_GetObject( gen ) ) );
}

static int objectScoreStats_AccDamage( int ammo, score_stats_t *obj )
{
	if( ammo < AMMO_GUNBLADE || ammo >= AMMO_TOTAL )
		return 0;

	return obj->accuracy_damage[ ammo - AMMO_GUNBLADE ];
}

static void objectScoreStats_asGeneric_AccDamage( void *gen )
{
	int ammo = G_asGeneric_GetArgInt( gen, 0 );

	G_asGeneric_SetReturnInt( gen, objectScoreStats_AccDamage( ammo, (score_stats_t *)G_asGeneric_GetObject( gen ) ) );
}

static void objectScoreStats_ScoreSet( int newscore, score_stats_t *obj )
{
	obj->score = newscore;
}

static void objectScoreStats_asGeneric_ScoreSet( void *gen )
{
	int newscore = G_asGeneric_GetArgInt( gen, 0 );

	objectScoreStats_ScoreSet( newscore, (score_stats_t *)G_asGeneric_GetObject( gen ) );
}

static void objectScoreStats_ScoreAdd( int score, score_stats_t *obj )
{
	obj->score += score;
}

static void objectScoreStats_asGeneric_ScoreAdd( void *gen )
{
	int score = G_asGeneric_GetArgInt( gen, 0 );

	objectScoreStats_ScoreAdd( score, (score_stats_t *)G_asGeneric_GetObject( gen ) );
}

static const asBehavior_t scorestats_ObjectBehaviors[] =
{
	{ asBEHAVE_FACTORY, SCRIPT_FUNCTION_DECL(cStats @, f, ()), objectScoreStats_Factory, objectScoreStats_asGeneric_Factory, asCALL_CDECL },
	{ asBEHAVE_ADDREF, SCRIPT_FUNCTION_DECL(void, f, ()), objectScoreStats_Addref, objectScoreStats_asGeneric_Addref, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_RELEASE, SCRIPT_FUNCTION_DECL(void, f, ()), objectScoreStats_Release, objectScoreStats_asGeneric_Release, asCALL_CDECL_OBJLAST },

	SCRIPT_BEHAVIOR_NULL
};

static const asMethod_t scorestats_Methods[] =
{
	{ SCRIPT_FUNCTION_DECL(void, setScore, ( int i )), objectScoreStats_ScoreSet, objectScoreStats_asGeneric_ScoreSet, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, addScore, ( int i )), objectScoreStats_ScoreAdd, objectScoreStats_asGeneric_ScoreAdd, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, clear, ()), objectScoreStats_Clear, objectScoreStats_asGeneric_Clear, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(int, accuracyShots, ( int ammo )), objectScoreStats_AccShots, objectScoreStats_asGeneric_AccShots, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(int, accuracyHits, ( int ammo )), objectScoreStats_AccHits, objectScoreStats_asGeneric_AccHits, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(int, accuracyHitsDirect, ( int ammo )), objectScoreStats_AccHitsDirect, objectScoreStats_asGeneric_AccHitsDirect, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(int, accuracyHitsAir, ( int ammo )), objectScoreStats_AccHitsAir, objectScoreStats_asGeneric_AccHitsAir, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(int, accuracyDamage, ( int ammo )), objectScoreStats_AccDamage, objectScoreStats_asGeneric_AccDamage, asCALL_CDECL_OBJLAST },

	SCRIPT_METHOD_NULL
};

static const asProperty_t scorestats_Properties[] =
{
	{ SCRIPT_PROPERTY_DECL(const int, score), FOFFSET(score_stats_t, score) },
	{ SCRIPT_PROPERTY_DECL(const int, deaths), FOFFSET(score_stats_t, deaths) },
	{ SCRIPT_PROPERTY_DECL(const int, frags), FOFFSET(score_stats_t, frags) },
	{ SCRIPT_PROPERTY_DECL(const int, suicides), FOFFSET(score_stats_t, suicides) },
	{ SCRIPT_PROPERTY_DECL(const int, teamFrags), FOFFSET(score_stats_t, teamfrags) },
	{ SCRIPT_PROPERTY_DECL(const int, awards), FOFFSET(score_stats_t, awards) },
	{ SCRIPT_PROPERTY_DECL(const int, totalDamageGiven), FOFFSET(score_stats_t, total_damage_given) },
	{ SCRIPT_PROPERTY_DECL(const int, totalDamageReceived), FOFFSET(score_stats_t, total_damage_received) },
	{ SCRIPT_PROPERTY_DECL(const int, totalTeamDamageGiven), FOFFSET(score_stats_t, total_teamdamage_given) },
	{ SCRIPT_PROPERTY_DECL(const int, totalTeamDamageReceived), FOFFSET(score_stats_t, total_teamdamage_received) },
	{ SCRIPT_PROPERTY_DECL(const int, healthTaken), FOFFSET(score_stats_t, health_taken) },
	{ SCRIPT_PROPERTY_DECL(const int, armorTaken), FOFFSET(score_stats_t, armor_taken) },

	SCRIPT_PROPERTY_NULL
};

static const asClassDescriptor_t asScoreStatsClassDescriptor =
{
	"cStats",					/* name */
	asOBJ_REF,					/* object type flags */
	sizeof( score_stats_t ),	/* size */
	scorestats_ObjectBehaviors, /* object behaviors */
	NULL,						/* global behaviors */
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

static void objectBot_asGeneric_Factory( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectBot_Factory() );
}

static void objectBot_Addref( asbot_t *obj ) { obj->asRefCount++; }

static void objectBot_asGeneric_Addref( void *gen )
{
	objectBot_Addref( (asbot_t *)G_asGeneric_GetObject( gen ) );
}

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

static void objectBot_asGeneric_Release( void *gen )
{
	objectBot_Release( (asbot_t *)G_asGeneric_GetObject( gen ) );
}

static void objectBot_ClearWeights( asbot_t *obj )
{
	memset( obj->status.entityWeights, 0, sizeof( obj->status.entityWeights ) );
}

static void objectBot_asGeneric_ClearWeights( void *gen )
{
	objectBot_ClearWeights( (asbot_t *)G_asGeneric_GetObject( gen ) );
}

static void objectBot_ResetWeights( asbot_t *obj )
{
	AI_ResetWeights( obj );
}

static void objectBot_asGeneric_ResetWeights( void *gen )
{
	objectBot_ResetWeights( (asbot_t *)G_asGeneric_GetObject( gen ) );
}

static void objectBot_SetGoalWeight( int index, float weight, asbot_t *obj )
{
	if( index < 0 || index >= MAX_EDICTS )
		return;

	obj->status.entityWeights[index] = weight;
}

static void objectBot_asGeneric_SetGoalWeight( void *gen )
{
	objectBot_SetGoalWeight(
		G_asGeneric_GetArgInt( gen, 0 ),
		G_asGeneric_GetArgFloat( gen, 1 ),
		(asbot_t *)G_asGeneric_GetObject( gen )
		);
}

static edict_t *objectBot_GetGoalEnt( int index, asbot_t *obj )
{
	return AI_GetGoalEnt( index );
}

static void objectBot_asGeneric_GetGoalEnt( void *gen )
{
	G_asGeneric_SetReturnAddress( gen,
		objectBot_GetGoalEnt( G_asGeneric_GetArgInt( gen, 0 ), (asbot_t *)G_asGeneric_GetObject( gen ) )
		);
}

static float objectBot_GetItemWeight( gsitem_t *item, asbot_t *obj )
{
	if( !item )
		return 0.0f;
	return obj->pers.inventoryWeights[item->tag];
}

static void objectBot_asGeneric_GetItemWeight( void *gen )
{
	G_asGeneric_SetReturnFloat( gen,
		objectBot_GetItemWeight(
		G_asGeneric_GetArgAddress( gen, 0 ),
		(asbot_t *)G_asGeneric_GetObject( gen )
		)
	);
}

static const asBehavior_t asbot_ObjectBehaviors[] =
{
	{ asBEHAVE_FACTORY, SCRIPT_FUNCTION_DECL(cBot @, f, ()), objectBot_Factory, objectBot_asGeneric_Factory, asCALL_CDECL },
	{ asBEHAVE_ADDREF, SCRIPT_FUNCTION_DECL(void, f, ()), objectBot_Addref, objectBot_asGeneric_Addref, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_RELEASE, SCRIPT_FUNCTION_DECL(void, f, ()), objectBot_Release, objectBot_asGeneric_Release, asCALL_CDECL_OBJLAST },

	SCRIPT_BEHAVIOR_NULL
};

static const asMethod_t asbot_Methods[] =
{
	{ SCRIPT_FUNCTION_DECL(void, clearGoalWeights, ()), objectBot_ClearWeights, objectBot_asGeneric_ClearWeights, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, resetGoalWeights, ()), objectBot_ResetWeights, objectBot_asGeneric_ResetWeights, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, setGoalWeight, ( int i, float weight )), objectBot_SetGoalWeight, objectBot_asGeneric_SetGoalWeight, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cEntity @, getGoalEnt, ( int i )), objectBot_GetGoalEnt, objectBot_asGeneric_GetGoalEnt, asCALL_CDECL_OBJLAST },

	{ SCRIPT_FUNCTION_DECL(float, getItemWeight, ( cItem @item )), objectBot_GetItemWeight, objectBot_asGeneric_GetItemWeight, asCALL_CDECL_OBJLAST },

	SCRIPT_METHOD_NULL
};

static const asProperty_t asbot_Properties[] =
{
	{ SCRIPT_PROPERTY_DECL(const float, skill), FOFFSET(asbot_t, pers.skillLevel) },
	{ SCRIPT_PROPERTY_DECL(const int, currentNode), FOFFSET(asbot_t, current_node) },
	{ SCRIPT_PROPERTY_DECL(const int, nextNode), FOFFSET(asbot_t, next_node) },
	{ SCRIPT_PROPERTY_DECL(uint, moveTypesMask), FOFFSET(asbot_t, status.moveTypesMask) },

	// character
	{ SCRIPT_PROPERTY_DECL(const float, reactionTime), FOFFSET(asbot_t, pers.cha.reaction_time) },
	{ SCRIPT_PROPERTY_DECL(const float, offensiveness), FOFFSET(asbot_t, pers.cha.offensiveness) },
	{ SCRIPT_PROPERTY_DECL(const float, campiness), FOFFSET(asbot_t, pers.cha.campiness) },
	{ SCRIPT_PROPERTY_DECL(const float, firerate), FOFFSET(asbot_t, pers.cha.firerate) },

	SCRIPT_PROPERTY_NULL
};

static const asClassDescriptor_t asBotClassDescriptor =
{
	"cBot",						/* name */
	asOBJ_REF,					/* object type flags */
	sizeof( asbot_t ),			/* size */
	asbot_ObjectBehaviors,		/* object behaviors */
	NULL,						/* global behaviors */
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

static void objectGameClient_asGeneric_Factory( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGameClient_Factory() );
}

static void objectGameClient_Addref( gclient_t *obj ) { obj->asRefCount++; }

static void objectGameClient_asGeneric_Addref( void *gen )
{
	objectGameClient_Addref( (gclient_t *)G_asGeneric_GetObject( gen ) );
}

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

static void objectGameClient_asGeneric_Release( void *gen )
{
	objectGameClient_Release( (gclient_t *)G_asGeneric_GetObject( gen ) );
}

static int objectGameClient_PlayerNum( gclient_t *self )
{
	if( self->asFactored )
		return -1;

	return (int)( self - game.clients );
}

static void objectGameClient_asGeneric_PlayerNum( void *gen )
{
	G_asGeneric_SetReturnInt( gen, objectGameClient_PlayerNum( (gclient_t *)G_asGeneric_GetObject( gen ) ) );
}

static qboolean objectGameClient_isReady( gclient_t *self )
{
	if( self->asFactored )
		return qfalse;

	return ( level.ready[self - game.clients] || GS_MatchState() == MATCH_STATE_PLAYTIME );
}

static void objectGameClient_asGeneric_isReady( void *gen )
{
	G_asGeneric_SetReturnBool( gen, objectGameClient_isReady( (gclient_t *)G_asGeneric_GetObject( gen ) ) );
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

static void objectGameClient_asGeneric_isBot( void *gen )
{
	G_asGeneric_SetReturnBool( gen, objectGameClient_isBot( (gclient_t *)G_asGeneric_GetObject( gen ) ) );
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

static void objectGameClient_asGeneric_getBot( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGameClient_getBot( (gclient_t *)G_asGeneric_GetObject( gen ) ) );
}

static int objectGameClient_ClientState( gclient_t *self )
{
	if( self->asFactored )
		return CS_FREE;

	return trap_GetClientState( (int)( self - game.clients ) );
}

static void objectGameClient_asGeneric_ClientState( void *gen )
{
	G_asGeneric_SetReturnInt( gen, objectGameClient_ClientState( (gclient_t *)G_asGeneric_GetObject( gen ) ) );
}
static void objectGameClient_ClearPlayerStateEvents( gclient_t *self )
{
	G_ClearPlayerStateEvents( self );
}

static void objectGameClient_asGeneric_ClearPlayerStateEvents( void *gen )
{
	objectGameClient_ClearPlayerStateEvents( (gclient_t *)G_asGeneric_GetObject( gen ) );
}

static asstring_t *objectGameClient_getName( gclient_t *self )
{
	char temp[MAX_NAME_BYTES*2];

	Q_strncpyz( temp, self->netname, sizeof( temp ) );
	Q_strncatz( temp, S_COLOR_WHITE, sizeof( temp ) );

	return objectString_FactoryBuffer( temp, strlen( temp ) );
}

static void objectGameClient_asGeneric_getName( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGameClient_getName( (gclient_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectGameClient_getClanName( gclient_t *self )
{
	char temp[MAX_CLANNAME_CHARS*2];

	Q_strncpyz( temp, self->clanname, sizeof( temp ) );
	Q_strncatz( temp, S_COLOR_WHITE, sizeof( temp ) );

	return objectString_FactoryBuffer( temp, strlen( temp ) );
}

static void objectGameClient_asGeneric_getClanName( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGameClient_getClanName( (gclient_t *)G_asGeneric_GetObject( gen ) ) );
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

static void objectGameClient_asGeneric_Respawn( void *gen )
{
	objectGameClient_Respawn(
		G_asGeneric_GetArgBool( gen, 0 ),
		(gclient_t *)G_asGeneric_GetObject( gen ) );
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

static void objectGameClient_asGeneric_GetEntity( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGameClient_GetEntity( (gclient_t *)G_asGeneric_GetObject( gen ) ) );
}

static int objectGameClient_InventoryCount( int index, gclient_t *self )
{
	if( index < 0 || index >= MAX_ITEMS )
		return 0;

	return self->ps.inventory[ index ];
}

static void objectGameClient_asGeneric_InventoryCount( void *gen )
{
	int index = G_asGeneric_GetArgInt( gen, 0 );

	G_asGeneric_SetReturnInt( gen, objectGameClient_InventoryCount( index, (gclient_t *)G_asGeneric_GetObject( gen ) ) );
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

static void objectGameClient_asGeneric_InventorySetCount( void *gen )
{
	int index = G_asGeneric_GetArgInt( gen, 0 );
	int newcount = G_asGeneric_GetArgInt( gen, 1 );

	objectGameClient_InventorySetCount( index, newcount, (gclient_t *)G_asGeneric_GetObject( gen ) );
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

static void objectGameClient_asGeneric_InventoryGiveItemExt( void *gen )
{
	int index = G_asGeneric_GetArgInt( gen, 0 );
	int count = G_asGeneric_GetArgInt( gen, 1 );

	objectGameClient_InventoryGiveItemExt( index, count, (gclient_t *)G_asGeneric_GetObject( gen ) );
}

static void objectGameClient_InventoryGiveItem( int index, gclient_t *self )
{
	objectGameClient_InventoryGiveItemExt( index, -1, self );
}

static void objectGameClient_asGeneric_InventoryGiveItem( void *gen )
{
	int index = G_asGeneric_GetArgInt( gen, 0 );

	objectGameClient_InventoryGiveItem( index, (gclient_t *)G_asGeneric_GetObject( gen ) );
}

static void objectGameClient_InventoryClear( gclient_t *self )
{
	memset( self->ps.inventory, 0, sizeof( self->ps.inventory ) );

	self->ps.stats[STAT_WEAPON] = self->ps.stats[STAT_PENDING_WEAPON] = WEAP_NONE;
	self->ps.weaponState = WEAPON_STATE_READY;
	self->ps.stats[STAT_WEAPON_TIME] = 0;
}

static void objectGameClient_asGeneric_InventoryClear( void *gen )
{
	objectGameClient_InventoryClear( (gclient_t *)G_asGeneric_GetObject( gen ) );
}

static qboolean objectGameClient_CanSelectWeapon( int index, gclient_t *self )
{
	if( index < WEAP_NONE || index >= WEAP_TOTAL )
		return qfalse;

	return ( GS_CheckAmmoInWeapon( &self->ps, index ) );
}

static void objectGameClient_asGeneric_CanSelectWeapon( void *gen )
{
	int index = G_asGeneric_GetArgInt( gen, 0 );

	G_asGeneric_SetReturnBool( gen, objectGameClient_CanSelectWeapon( index, (gclient_t *)G_asGeneric_GetObject( gen ) ) );
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

static void objectGameClient_asGeneric_SelectWeapon( void *gen )
{
	int index = G_asGeneric_GetArgInt( gen, 0 );

	objectGameClient_SelectWeapon( index, (gclient_t *)G_asGeneric_GetObject( gen ) );
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

static void objectGameClient_asGeneric_addAward( void *gen )
{
	objectGameClient_addAward(
		G_asGeneric_GetArgAddress( gen, 0 ),
		(gclient_t *)G_asGeneric_GetObject( gen )
		);
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

static void objectGameClient_asGeneric_execGameCommand( void *gen )
{
	objectGameClient_execGameCommand(
		G_asGeneric_GetArgAddress( gen, 0 ),
		(gclient_t *)G_asGeneric_GetObject( gen )
		);
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

static void objectGameClient_asGeneric_setHUDStat( void *gen )
{
	objectGameClient_setHUDStat(
		G_asGeneric_GetArgInt( gen, 0 ),
		G_asGeneric_GetArgInt( gen, 1 ),
		(gclient_t *)G_asGeneric_GetObject( gen )
		);
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

static void objectGameClient_asGeneric_getHUDStat( void *gen )
{
	G_asGeneric_SetReturnInt( gen,
		objectGameClient_getHUDStat( G_asGeneric_GetArgInt( gen, 0 ),
		(gclient_t *)G_asGeneric_GetObject( gen ) )
		);
}

static void objectGameClient_setPMoveFeatures( unsigned int bitmask, gclient_t *self )
{
	self->ps.pmove.stats[PM_STAT_FEATURES] = ( bitmask & PMFEAT_ALL );
}

static void objectGameClient_asGeneric_setPMoveFeatures( void *gen )
{
	objectGameClient_setPMoveFeatures(
		G_asGeneric_GetArgInt( gen, 0 ),
		(gclient_t *)G_asGeneric_GetObject( gen )
		);
}

static void objectGameClient_setPMoveMaxSpeed( float speed, gclient_t *self )
{
	if( speed < 0.0f )
		self->ps.pmove.stats[PM_STAT_MAXSPEED] = DEFAULT_PLAYERSPEED;
	else
		self->ps.pmove.stats[PM_STAT_MAXSPEED] = ( (int)speed & 0xFFFF );
}

static void objectGameClient_asGeneric_setPMoveMaxSpeed( void *gen )
{
	objectGameClient_setPMoveMaxSpeed(
		G_asGeneric_GetArgFloat( gen, 0 ),
		(gclient_t *)G_asGeneric_GetObject( gen )
		);
}

static void objectGameClient_setPMoveJumpSpeed( float speed, gclient_t *self )
{
	if( speed < 0.0f )
		self->ps.pmove.stats[PM_STAT_JUMPSPEED] = DEFAULT_JUMPSPEED;
	else
		self->ps.pmove.stats[PM_STAT_JUMPSPEED] = ( (int)speed & 0xFFFF );
}

static void objectGameClient_asGeneric_setPMoveJumpSpeed( void *gen )
{
	objectGameClient_setPMoveJumpSpeed(
		G_asGeneric_GetArgFloat( gen, 0 ),
		(gclient_t *)G_asGeneric_GetObject( gen )
		);
}

static void objectGameClient_setPMoveDashSpeed( float speed, gclient_t *self )
{
	if( speed < 0.0f )
		self->ps.pmove.stats[PM_STAT_DASHSPEED] = DEFAULT_DASHSPEED;
	else
		self->ps.pmove.stats[PM_STAT_DASHSPEED] = ( (int)speed & 0xFFFF );
}

static void objectGameClient_asGeneric_setPMoveDashSpeed( void *gen )
{
	objectGameClient_setPMoveDashSpeed(
		G_asGeneric_GetArgFloat( gen, 0 ),
		(gclient_t *)G_asGeneric_GetObject( gen )
		);
}

static asstring_t *objectGameClient_getUserInfoKey( asstring_t *key, gclient_t *self )
{
	char *s;

	if( !key || !key->buffer || !key->buffer[0] )
		return objectString_FactoryBuffer( NULL, 0 );

	s = Info_ValueForKey( self->userinfo, key->buffer );
	if( !s || !*s )
		return objectString_FactoryBuffer( NULL, 0 );

	return objectString_FactoryBuffer( s, strlen( s ) );
}

static void objectGameClient_asGeneric_getUserInfoKey( void *gen )
{
	G_asGeneric_SetReturnAddress( gen,
		objectGameClient_getUserInfoKey(
		(asstring_t *)G_asGeneric_GetArgAddress( gen, 0 ),
		(gclient_t *)G_asGeneric_GetObject( gen ) )
		);
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

static void objectGameClient_asGeneric_printMessage( void *gen )
{
		objectGameClient_printMessage(
		(asstring_t *)G_asGeneric_GetArgAddress( gen, 0 ),
		(gclient_t *)G_asGeneric_GetObject( gen )
		);
}

static void objectGameClient_ChaseCam( asstring_t *str, qboolean teamonly, gclient_t *self )
{
	int playerNum;

	playerNum = (int)( self - game.clients );
	if( playerNum < 0 || playerNum >= gs.maxclients )
		return;

	G_ChasePlayer( &game.edicts[ playerNum + 1 ], str ? str->buffer : NULL, teamonly, 0 );
}

static void objectGameClient_asGeneric_ChaseCam( void *gen )
{
	objectGameClient_ChaseCam(
		(asstring_t *)G_asGeneric_GetArgAddress( gen, 0 ),
		G_asGeneric_GetArgBool( gen, 1 ),
		(gclient_t *)G_asGeneric_GetObject( gen )
		);
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

static void objectGameClient_asGeneric_SetChaseActive( void *gen )
{
	objectGameClient_SetChaseActive(
		G_asGeneric_GetArgBool( gen, 0 ),
		(gclient_t *)G_asGeneric_GetObject( gen )
		);
}

static const asBehavior_t gameclient_ObjectBehaviors[] =
{
	{ asBEHAVE_FACTORY, SCRIPT_FUNCTION_DECL(cClient @, f, ()), objectGameClient_Factory, objectGameClient_asGeneric_Factory, asCALL_CDECL },
	{ asBEHAVE_ADDREF, SCRIPT_FUNCTION_DECL(void, f, ()), objectGameClient_Addref, objectGameClient_asGeneric_Addref, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_RELEASE, SCRIPT_FUNCTION_DECL(void, f, ()), objectGameClient_Release, objectGameClient_asGeneric_Release, asCALL_CDECL_OBJLAST },

	SCRIPT_BEHAVIOR_NULL
};

static const asMethod_t gameclient_Methods[] =
{
	{ SCRIPT_FUNCTION_DECL(int, playerNum, ()), objectGameClient_PlayerNum, objectGameClient_asGeneric_PlayerNum, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(bool, isReady, ()), objectGameClient_isReady, objectGameClient_asGeneric_isReady, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(bool, isBot, ()), objectGameClient_isBot, objectGameClient_asGeneric_isBot, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cBot @, getBot, ()), objectGameClient_getBot, objectGameClient_asGeneric_getBot, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(int, state, ()), objectGameClient_ClientState, objectGameClient_asGeneric_ClientState, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, respawn, ( bool ghost )), objectGameClient_Respawn, objectGameClient_asGeneric_Respawn, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, clearPlayerStateEvents, ()), objectGameClient_ClearPlayerStateEvents, objectGameClient_asGeneric_ClearPlayerStateEvents, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, getName, ()), objectGameClient_getName, objectGameClient_asGeneric_getName, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, getClanName, ()), objectGameClient_getClanName, objectGameClient_asGeneric_getClanName, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cEntity @, getEnt, ()), objectGameClient_GetEntity, objectGameClient_asGeneric_GetEntity, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(int, inventoryCount, ( int tag )), objectGameClient_InventoryCount, objectGameClient_asGeneric_InventoryCount, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, inventorySetCount, ( int tag, int count )), objectGameClient_InventorySetCount, objectGameClient_asGeneric_InventorySetCount, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, inventoryGiveItem, ( int tag, int count )), objectGameClient_InventoryGiveItemExt, objectGameClient_asGeneric_InventoryGiveItemExt, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, inventoryGiveItem, ( int tag )), objectGameClient_InventoryGiveItem, objectGameClient_asGeneric_InventoryGiveItem, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, inventoryClear, ()), objectGameClient_InventoryClear, objectGameClient_asGeneric_InventoryClear, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(bool, canSelectWeapon, ( int tag )), objectGameClient_CanSelectWeapon, objectGameClient_asGeneric_CanSelectWeapon, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, selectWeapon, ( int tag )), objectGameClient_SelectWeapon, objectGameClient_asGeneric_SelectWeapon, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, addAward, ( cString &in )), objectGameClient_addAward, objectGameClient_asGeneric_addAward, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, execGameCommand, ( cString &in )), objectGameClient_execGameCommand, objectGameClient_asGeneric_execGameCommand, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, setHUDStat, ( int stat, int value )), objectGameClient_setHUDStat, objectGameClient_asGeneric_setHUDStat, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(int, getHUDStat, ( int stat )), objectGameClient_getHUDStat, objectGameClient_asGeneric_getHUDStat, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, setPMoveFeatures, ( uint bitmask )), objectGameClient_setPMoveFeatures, objectGameClient_asGeneric_setPMoveFeatures, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, setPMoveMaxSpeed, ( float speed )), objectGameClient_setPMoveMaxSpeed, objectGameClient_asGeneric_setPMoveMaxSpeed, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, setPMoveJumpSpeed, ( float speed )), objectGameClient_setPMoveJumpSpeed, objectGameClient_asGeneric_setPMoveJumpSpeed, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, setPMoveDashSpeed, ( float speed )), objectGameClient_setPMoveDashSpeed, objectGameClient_asGeneric_setPMoveDashSpeed, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, getUserInfoKey, ( cString &in )), objectGameClient_getUserInfoKey, objectGameClient_asGeneric_getUserInfoKey, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, printMessage, ( cString &in )), objectGameClient_printMessage, objectGameClient_asGeneric_printMessage, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, chaseCam, ( cString @, bool teamOnly )), objectGameClient_ChaseCam, objectGameClient_asGeneric_ChaseCam, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, setChaseActive, ( bool active )), objectGameClient_ChaseCam, objectGameClient_asGeneric_ChaseCam, asCALL_CDECL_OBJLAST },

	SCRIPT_METHOD_NULL
};

static const asProperty_t gameclient_Properties[] =
{
	{ SCRIPT_PROPERTY_DECL(cStats, stats), FOFFSET(gclient_t, level.stats) },
	{ SCRIPT_PROPERTY_DECL(const bool, connecting), FOFFSET(gclient_t, connecting) },
	{ SCRIPT_PROPERTY_DECL(const bool, multiview), FOFFSET(gclient_t, multiview) },
	{ SCRIPT_PROPERTY_DECL(const bool, tv), FOFFSET(gclient_t, tv) },
	{ SCRIPT_PROPERTY_DECL(int, team), FOFFSET(gclient_t, team) },
	{ SCRIPT_PROPERTY_DECL(const int, hand), FOFFSET(gclient_t, hand) },
	{ SCRIPT_PROPERTY_DECL(const int, fov), FOFFSET(gclient_t, fov) },
	{ SCRIPT_PROPERTY_DECL(const int, zoomFov), FOFFSET(gclient_t, zoomfov) },
	{ SCRIPT_PROPERTY_DECL(const bool, isOperator), FOFFSET(gclient_t, isoperator) },
	{ SCRIPT_PROPERTY_DECL(const uint, queueTimeStamp), FOFFSET(gclient_t, queueTimeStamp) },
	{ SCRIPT_PROPERTY_DECL(int, muted), FOFFSET(gclient_t, muted) }, //racesow allow editing the int
	{ SCRIPT_PROPERTY_DECL(float, armor), FOFFSET(gclient_t, resp.armor) },
	{ SCRIPT_PROPERTY_DECL(uint, gunbladeChargeTimeStamp), FOFFSET(gclient_t, resp.gunbladeChargeTimeStamp) },
	{ SCRIPT_PROPERTY_DECL(const bool, chaseActive), FOFFSET(gclient_t, resp.chase.active) },
	{ SCRIPT_PROPERTY_DECL(int, chaseTarget), FOFFSET(gclient_t, resp.chase.target) },
	{ SCRIPT_PROPERTY_DECL(bool, chaseTeamonly), FOFFSET(gclient_t, resp.chase.teamonly) },
	{ SCRIPT_PROPERTY_DECL(int, chaseFollowMode), FOFFSET(gclient_t, resp.chase.followmode) },
	{ SCRIPT_PROPERTY_DECL(const bool, coach), FOFFSET(gclient_t, teamstate.is_coach) },
	{ SCRIPT_PROPERTY_DECL(const int, ping), FOFFSET(gclient_t, r.ping) },
	{ SCRIPT_PROPERTY_DECL(const uint, serverTimeStamp), FOFFSET(gclient_t, ucmd.serverTimeStamp) },
	{ SCRIPT_PROPERTY_DECL(const int16, weapon), FOFFSET(gclient_t, ps.stats[STAT_WEAPON]) },
	{ SCRIPT_PROPERTY_DECL(const int16, pendingWeapon), FOFFSET(gclient_t, ps.stats[STAT_PENDING_WEAPON]) },
	{ SCRIPT_PROPERTY_DECL(const int16, pmoveFeatures), FOFFSET(gclient_t, ps.pmove.stats[PM_STAT_FEATURES]) },
	{ SCRIPT_PROPERTY_DECL(bool, takeStun), FOFFSET(gclient_t, resp.takeStun) },

	SCRIPT_PROPERTY_NULL
};

static const asClassDescriptor_t asGameClientDescriptor =
{
	"cClient",					/* name */
	asOBJ_REF,					/* object type flags */
	sizeof( gclient_t ),		/* size */
	gameclient_ObjectBehaviors,	/* object behaviors */
	NULL,						/* global behaviors */
	gameclient_Methods,			/* methods */
	gameclient_Properties,		/* properties */

	NULL, NULL					/* string factory hack */
};

//=======================================================================

// CLASS: cEntity
static int edict_factored_count = 0;
static int edict_released_count = 0;

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

static void objectGameEntity_asGeneric_Factory( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGameEntity_Factory() );
}

static void objectGameEntity_Addref( edict_t *obj ) { obj->asRefCount++; }

static void objectGameEntity_asGeneric_Addref( void *gen )
{
	objectGameEntity_Addref( (edict_t *)G_asGeneric_GetObject( gen ) );
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

static void objectGameEntity_asGeneric_Release( void *gen )
{
	objectGameEntity_Release( (edict_t *)G_asGeneric_GetObject( gen ) );
}

static qboolean objectGameEntity_EqualBehaviour( edict_t *first, edict_t *second )
{
	return ( first == second );
}

static void objectGameEntity_asGeneric_EqualBehaviour( void *gen )
{
	G_asGeneric_SetReturnBool( gen, objectGameEntity_EqualBehaviour( (edict_t *)G_asGeneric_GetArgAddress( gen, 0 ), (edict_t *)G_asGeneric_GetArgAddress( gen, 1 ) ) );
}

static qboolean objectGameEntity_NotEqualBehaviour( edict_t *first, edict_t *second )
{
	return ( first != second );
}

static void objectGameEntity_asGeneric_NotEqualBehaviour( void *gen )
{
	G_asGeneric_SetReturnBool( gen, objectGameEntity_NotEqualBehaviour( (edict_t *)G_asGeneric_GetArgAddress( gen, 0 ), (edict_t *)G_asGeneric_GetArgAddress( gen, 1 ) ) );
}

static asvec3_t *objectGameEntity_GetVelocity( edict_t *obj )
{
	asvec3_t *velocity = objectVector_Factory();

	VectorCopy( obj->velocity, velocity->v );

	return velocity;
}

static void objectGameEntity_asGeneric_GetVelocity( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGameEntity_GetVelocity( (edict_t *)G_asGeneric_GetObject( gen ) ) );
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

static void objectGameEntity_asGeneric_SetVelocity( void *gen )
{
	asvec3_t *vel = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );

	objectGameEntity_SetVelocity( vel, (edict_t *)G_asGeneric_GetObject( gen ) );
}

static asvec3_t *objectGameEntity_GetAVelocity( edict_t *obj )
{
	asvec3_t *velocity = objectVector_Factory();

	VectorCopy( obj->avelocity, velocity->v );

	return velocity;
}

static void objectGameEntity_asGeneric_GetAVelocity( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGameEntity_GetAVelocity( (edict_t *)G_asGeneric_GetObject( gen ) ) );
}

static void objectGameEntity_SetAVelocity( asvec3_t *vel, edict_t *self )
{
	VectorCopy( vel->v, self->avelocity );
}

static void objectGameEntity_asGeneric_SetAVelocity( void *gen )
{
	asvec3_t *vel = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );

	objectGameEntity_SetAVelocity( vel, (edict_t *)G_asGeneric_GetObject( gen ) );
}

static asvec3_t *objectGameEntity_GetOrigin( edict_t *obj )
{
	asvec3_t *origin = objectVector_Factory();

	VectorCopy( obj->s.origin, origin->v );

	return origin;
}

static void objectGameEntity_asGeneric_GetOrigin( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGameEntity_GetOrigin( (edict_t *)G_asGeneric_GetObject( gen ) ) );
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

static void objectGameEntity_asGeneric_SetOrigin( void *gen )
{
	asvec3_t *vel = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );

	objectGameEntity_SetOrigin( vel, (edict_t *)G_asGeneric_GetObject( gen ) );
}

static asvec3_t *objectGameEntity_GetOrigin2( edict_t *obj )
{
	asvec3_t *origin = objectVector_Factory();

	VectorCopy( obj->s.origin2, origin->v );

	return origin;
}

static void objectGameEntity_asGeneric_GetOrigin2( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGameEntity_GetOrigin2( (edict_t *)G_asGeneric_GetObject( gen ) ) );
}

static void objectGameEntity_SetOrigin2( asvec3_t *vec, edict_t *self )
{
	VectorCopy( vec->v, self->s.origin2 );
}

static void objectGameEntity_asGeneric_SetOrigin2( void *gen )
{
	asvec3_t *vel = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );

	objectGameEntity_SetOrigin2( vel, (edict_t *)G_asGeneric_GetObject( gen ) );
}

static asvec3_t *objectGameEntity_GetAngles( edict_t *obj )
{
	asvec3_t *angles = objectVector_Factory();

	VectorCopy( obj->s.angles, angles->v );

	return angles;
}

static void objectGameEntity_asGeneric_GetAngles( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGameEntity_GetAngles( (edict_t *)G_asGeneric_GetObject( gen ) ) );
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

static void objectGameEntity_asGeneric_SetAngles( void *gen )
{
	asvec3_t *vel = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );

	objectGameEntity_SetAngles( vel, (edict_t *)G_asGeneric_GetObject( gen ) );
}

static void objectGameEntity_GetSize( asvec3_t *mins, asvec3_t *maxs, edict_t *self )
{
	if( maxs )
		VectorCopy( self->r.maxs, maxs->v );
	if( mins )
		VectorCopy( self->r.mins, mins->v );
}

static void objectGameEntity_asGeneric_GetSize( void *gen )
{
	asvec3_t *mins = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asvec3_t *maxs = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 );

	objectGameEntity_GetSize( mins, maxs, (edict_t *)G_asGeneric_GetObject( gen ) );
}

static void objectGameEntity_SetSize( asvec3_t *mins, asvec3_t *maxs, edict_t *self )
{
	if( mins )
		VectorCopy( mins->v, self->r.mins );
	if( maxs )
		VectorCopy( maxs->v, self->r.maxs );
}

static void objectGameEntity_asGeneric_SetSize( void *gen )
{
	asvec3_t *mins = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asvec3_t *maxs = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 );

	objectGameEntity_SetSize( mins, maxs, (edict_t *)G_asGeneric_GetObject( gen ) );
}

static void objectGameEntity_GetMovedir( asvec3_t *movedir, edict_t *self )
{
	VectorCopy( self->moveinfo.movedir, movedir->v );
}

static void objectGameEntity_asGeneric_GetMovedir( void *gen )
{
	asvec3_t *movedir = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );

	objectGameEntity_GetMovedir( movedir, (edict_t *)G_asGeneric_GetObject( gen ) );
}

static void objectGameEntity_SetMovedir( edict_t *self )
{
	G_SetMovedir( self->s.angles, self->moveinfo.movedir );
}

static void objectGameEntity_asGeneric_SetMovedir( void *gen )
{
	objectGameEntity_SetMovedir( (edict_t *)G_asGeneric_GetObject( gen ) );
}

static qboolean objectGameEntity_isBrushModel( edict_t *self )
{
	return ISBRUSHMODEL( self->s.modelindex );
}

static void objectGameEntity_asGeneric_isBrushModel( void *gen )
{
	G_asGeneric_SetReturnBool( gen, objectGameEntity_isBrushModel( (edict_t *)G_asGeneric_GetObject( gen ) ) );
}

static qboolean objectGameEntity_IsGhosting( edict_t *self )
{
	if( self->r.client && trap_GetClientState( PLAYERNUM( self ) ) < CS_SPAWNED )
		return qtrue;

	return G_ISGHOSTING( self );
}

static void objectGameEntity_asGeneric_IsGhosting( void *gen )
{
	G_asGeneric_SetReturnBool( gen, objectGameEntity_IsGhosting( (edict_t *)G_asGeneric_GetObject( gen ) ) );
}

static int objectGameEntity_EntNum( edict_t *self )
{
	if( self->asFactored )
		return -1;
	return ( ENTNUM( self ) );
}

static void objectGameEntity_asGeneric_EntNum( void *gen )
{
	G_asGeneric_SetReturnInt( gen, objectGameEntity_EntNum( (edict_t *)G_asGeneric_GetObject( gen ) ) );
}

static int objectGameEntity_PlayerNum( edict_t *self )
{
	if( self->asFactored )
		return -1;
	return ( PLAYERNUM( self ) );
}

static void objectGameEntity_asGeneric_PlayerNum( void *gen )
{
	G_asGeneric_SetReturnInt( gen, objectGameEntity_PlayerNum( (edict_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectGameEntity_getModelName( edict_t *self )
{
	return objectString_FactoryBuffer( self->model, self->model ? strlen(self->model) : 0 );
}

static void objectGameEntity_asGeneric_getModelName( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGameEntity_getModelName( (edict_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectGameEntity_getModel2Name( edict_t *self )
{
	return objectString_FactoryBuffer( self->model2, self->model2 ? strlen(self->model2) : 0 );
}

static void objectGameEntity_asGeneric_getModel2Name( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGameEntity_getModel2Name( (edict_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectGameEntity_getClassname( edict_t *self )
{
	return objectString_FactoryBuffer( self->classname, self->classname ? strlen(self->classname) : 0 );
}

static void objectGameEntity_asGeneric_getClassname( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGameEntity_getClassname( (edict_t *)G_asGeneric_GetObject( gen ) ) );
}

/*
static asstring_t *objectGameEntity_getSpawnKey( asstring_t *key, edict_t *self )
{
	const char *val;

	if( !key )
		return objectString_FactoryBuffer( NULL, 0 );

	val = G_GetEntitySpawnKey( key->buffer, self );

	return objectString_FactoryBuffer( val, strlen( val ) );
}

static void objectGameEntity_asGeneric_getSpawnKey( void *gen )
{
	G_asGeneric_SetReturnAddress( gen,
		objectGameEntity_getSpawnKey( G_asGeneric_GetArgAddress( gen, 0 ), (edict_t *)G_asGeneric_GetObject( gen ) )
		);
}
*/

static asstring_t *objectGameEntity_getTargetname( edict_t *self )
{
	return objectString_FactoryBuffer( self->targetname, self->targetname ? strlen(self->targetname) : 0 );
}

static void objectGameEntity_asGeneric_getTargetname( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGameEntity_getTargetname( (edict_t *)G_asGeneric_GetObject( gen ) ) );
}

static void objectGameEntity_setTargetname( asstring_t *targetname, edict_t *self )
{
	self->targetname = G_RegisterLevelString( targetname->buffer );
}

static void objectGameEntity_asGeneric_setTargetname( void *gen )
{
	asstring_t *str = (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 );
	objectGameEntity_setTargetname( str, (edict_t *)G_asGeneric_GetObject( gen ) );
}

static asstring_t *objectGameEntity_getTarget( edict_t *self )
{
	return objectString_FactoryBuffer( self->target, self->target ? strlen(self->target) : 0 );
}

static void objectGameEntity_asGeneric_getTarget( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGameEntity_getTarget( (edict_t *)G_asGeneric_GetObject( gen ) ) );
}

static void objectGameEntity_setTarget( asstring_t *target, edict_t *self )
{
	self->target = G_RegisterLevelString( target->buffer );
}

static void objectGameEntity_asGeneric_setTarget( void *gen )
{
	asstring_t *str = (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 );
	objectGameEntity_setTarget( str, (edict_t *)G_asGeneric_GetObject( gen ) );
}

static asstring_t *objectGameEntity_getMap( edict_t *self )
{
	return objectString_FactoryBuffer( self->map, self->map ? strlen(self->map) : 0 );
}

static void objectGameEntity_asGeneric_getMap( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGameEntity_getTarget( (edict_t *)G_asGeneric_GetObject( gen ) ) );
}

static asstring_t *objectGameEntity_getSoundName( edict_t *self )
{
	return objectString_FactoryBuffer( self->sounds, self->sounds ? strlen(self->sounds) : 0 );
}

static void objectGameEntity_asGeneric_getSoundName( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, objectGameEntity_getSoundName( (edict_t *)G_asGeneric_GetObject( gen ) ) );
}

static void objectGameEntity_setClassname( asstring_t *classname, edict_t *self )
{
	self->classname = G_RegisterLevelString( classname->buffer );
}

static void objectGameEntity_asGeneric_setClassname( void *gen )
{
	asstring_t *str = (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 );
	objectGameEntity_setClassname( str, (edict_t *)G_asGeneric_GetObject( gen ) );
}

static void objectGameEntity_setMap( asstring_t *map, edict_t *self )
{
	self->map = G_RegisterLevelString( map->buffer );
}

static void objectGameEntity_asGeneric_setMap( void *gen )
{
	asstring_t *str = (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 );
	objectGameEntity_setMap( str, (edict_t *)G_asGeneric_GetObject( gen ) );
}

static void objectGameEntity_GhostClient( edict_t *self )
{
	if( self->r.client )
		G_GhostClient( self );
}

static void objectGameEntity_asGeneric_GhostClient( void *gen )
{
	objectGameEntity_GhostClient( (edict_t *)G_asGeneric_GetObject( gen ) );
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

static void objectGameEntity_asGeneric_SetupModelExt( void *gen )
{
	asstring_t *modelstr = (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asstring_t *skinstr = (asstring_t *)G_asGeneric_GetArgAddress( gen, 1 );

	objectGameEntity_SetupModelExt( modelstr, skinstr, (edict_t *)G_asGeneric_GetObject( gen ) );
}

static void objectGameEntity_SetupModel( asstring_t *modelstr, edict_t *self )
{
	objectGameEntity_SetupModelExt( modelstr, NULL, self );
}

static void objectGameEntity_asGeneric_SetupModel( void *gen )
{
	asstring_t *modelstr = (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 );

	objectGameEntity_SetupModelExt( modelstr, NULL, (edict_t *)G_asGeneric_GetObject( gen ) );
}

static void objectGameEntity_UseTargets( edict_t *activator, edict_t *self )
{
	G_UseTargets( self, activator );
}

static void objectGameEntity_asGeneric_UseTargets( void *gen )
{
	edict_t *activator = (edict_t *)G_asGeneric_GetArgAddress( gen, 0 );
	objectGameEntity_UseTargets( activator, (edict_t *)G_asGeneric_GetObject( gen ) );
}

static edict_t *objectGameEntity_DropItem( int tag, edict_t *self )
{
	gsitem_t *item = GS_FindItemByTag( tag );

	if( !item )
		return NULL;

	return Drop_Item( self, item );
}

static void objectGameEntity_asGeneric_DropItem( void *gen )
{
	int tag = G_asGeneric_GetArgInt( gen, 0 );

	G_asGeneric_SetReturnAddress( gen, objectGameEntity_DropItem( tag, (edict_t *)G_asGeneric_GetObject( gen ) ) );
}

static edict_t *objectGameEntity_findTarget( edict_t *from, edict_t *self )
{
	if( !self->target )
		return NULL;

	return G_Find( from, FOFS( targetname ), self->target );
}

static void objectGameEntity_asGeneric_findTarget( void *gen )
{
	edict_t *from = (edict_t *)G_asGeneric_GetArgAddress( gen, 0 );

	G_asGeneric_SetReturnAddress( gen, objectGameEntity_findTarget( from, (edict_t *)G_asGeneric_GetObject( gen ) ) );
}

static edict_t *objectGameEntity_findTargeting( edict_t *from, edict_t *self )
{
	if( !self->targetname )
		return NULL;

	return G_Find( from, FOFS( target ), self->targetname );
}

static void objectGameEntity_asGeneric_findTargeting( void *gen )
{
	edict_t *from = (edict_t *)G_asGeneric_GetArgAddress( gen, 0 );

	G_asGeneric_SetReturnAddress( gen, objectGameEntity_findTargeting( from, (edict_t *)G_asGeneric_GetObject( gen ) ) );
}

static void objectGameEntity_Use( edict_t *targeter, edict_t *activator, edict_t *self )
{
	G_CallUse( self, targeter, activator );
}

static void objectGameEntity_asGeneric_Use( void *gen )
{
	edict_t *targeter = (edict_t *)G_asGeneric_GetArgAddress( gen, 0 );
	edict_t *activator = (edict_t *)G_asGeneric_GetArgAddress( gen, 1 );

	objectGameEntity_Use( targeter, activator, (edict_t *)G_asGeneric_GetObject( gen ) );
}

static void objectGameEntity_setAIgoal( qboolean customReach, edict_t *self )
{
	if( customReach )
		AI_AddGoalEntityCustom( self );
	else
		AI_AddGoalEntity( self );
}

static void objectGameEntity_asGeneric_setAIgoal( void *gen )
{
	objectGameEntity_setAIgoal( G_asGeneric_GetArgBool( gen, 0 ), (edict_t *)G_asGeneric_GetObject( gen ) );
}

static void objectGameEntity_setAIgoalSimple( edict_t *self )
{
	objectGameEntity_setAIgoal( qfalse, self );
}

static void objectGameEntity_asGeneric_setAIgoalSimple( void *gen )
{
	objectGameEntity_setAIgoalSimple( (edict_t *)G_asGeneric_GetObject( gen ) );
}

static void objectGameEntity_removeAIgoal( edict_t *self )
{
	AI_RemoveGoalEntity( self );
}

static void objectGameEntity_asGeneric_removeAIgoal( void *gen )
{
	objectGameEntity_removeAIgoal( (edict_t *)G_asGeneric_GetObject( gen ) );
}

static void objectGameEntity_reachedAIgoal( edict_t *self )
{
	AI_ReachedEntity( self );
}

static void objectGameEntity_asGeneric_reachedAIgoal( void *gen )
{
	objectGameEntity_reachedAIgoal( (edict_t *)G_asGeneric_GetObject( gen ) );
}

static void objectGameEntity_asGeneric_G_FreeEdict( void *gen )
{
	G_FreeEdict( (edict_t *)G_asGeneric_GetObject( gen ) );
}

static void objectGameEntity_asGeneric_GClip_LinkEntity( void *gen )
{
	GClip_LinkEntity( (edict_t *)G_asGeneric_GetObject( gen ) );
}

static void objectGameEntity_asGeneric_GClip_UnlinkEntity( void *gen )
{
	GClip_UnlinkEntity( (edict_t *)G_asGeneric_GetObject( gen ) );
}

static void objectGameEntity_asGeneric_G_SpawnQueue_AddClient( void *gen )
{
	G_SpawnQueue_AddClient( (edict_t *)G_asGeneric_GetObject( gen ) );
}

static void objectGameEntity_TeleportEffect( qboolean in, edict_t *self )
{
	G_TeleportEffect( self, in );
}

static void objectGameEntity_asGeneric_TeleportEffect( void *gen )
{
	objectGameEntity_TeleportEffect( G_asGeneric_GetArgBool( gen, 0 ), (edict_t *)G_asGeneric_GetObject( gen ) );
}

static void objectGameEntity_asGeneric_G_RespawnEffect( void *gen )
{
	G_RespawnEffect( (edict_t *)G_asGeneric_GetObject( gen ) );
}

static void objectGameEntity_takeDamage( edict_t *inflictor, edict_t *attacker, asvec3_t *dir, float damage, float knockback, float stun, int mod, edict_t *self )
{
	G_TakeDamage( self, inflictor, attacker,
		dir ? dir->v : NULL, dir ? dir->v : NULL,
		inflictor ? inflictor->s.origin : self->s.origin,
		damage, knockback, stun, 0, mod >= 0 ? mod : 0 );
}

static void objectGameEntity_asGeneric_takeDamage( void *gen )
{
	objectGameEntity_takeDamage(
		(edict_t *)G_asGeneric_GetArgAddress( gen, 0 ),
		(edict_t *)G_asGeneric_GetArgAddress( gen, 1 ),
		(asvec3_t *)G_asGeneric_GetArgAddress( gen, 2 ),
		G_asGeneric_GetArgFloat( gen, 3 ),
		G_asGeneric_GetArgFloat( gen, 4 ),
		G_asGeneric_GetArgFloat( gen, 5 ),
		G_asGeneric_GetArgInt( gen, 6 ),
		(edict_t *)G_asGeneric_GetObject( gen )
		);
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

	G_TakeRadiusDamage( self, attacker, NULL, self, mod >= 0 ? mod : 0 );
}

static void objectGameEntity_asGeneric_splashDamage( void *gen )
{
	objectGameEntity_splashDamage(
		(edict_t *)G_asGeneric_GetArgAddress( gen, 0 ),
		G_asGeneric_GetArgInt( gen, 1 ),
		G_asGeneric_GetArgFloat( gen, 2 ),
		G_asGeneric_GetArgFloat( gen, 3 ),
		G_asGeneric_GetArgFloat( gen, 4 ),
		G_asGeneric_GetArgInt( gen, 5 ),
		(edict_t *)G_asGeneric_GetObject( gen )
		);
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
	event->r.svflags |= SVF_NOORIGIN2;
}

static void objectGameEntity_asGeneric_explosionEffect( void *gen )
{
	objectGameEntity_explosionEffect(
		G_asGeneric_GetArgInt( gen, 0 ),
		(edict_t *)G_asGeneric_GetObject( gen )
		);
}

static const asBehavior_t gedict_ObjectBehaviors[] =
{
	{ asBEHAVE_FACTORY, SCRIPT_FUNCTION_DECL(cEntity @, f, ()), objectGameEntity_Factory, objectGameEntity_asGeneric_Factory, asCALL_CDECL },
	{ asBEHAVE_ADDREF, SCRIPT_FUNCTION_DECL(void, f, ()), objectGameEntity_Addref, objectGameEntity_asGeneric_Addref, asCALL_CDECL_OBJLAST },
	{ asBEHAVE_RELEASE, SCRIPT_FUNCTION_DECL(void, f, ()), objectGameEntity_Release, objectGameEntity_asGeneric_Release, asCALL_CDECL_OBJLAST },

	SCRIPT_BEHAVIOR_NULL
};

static const asBehavior_t gedict_GlobalBehaviors[] =
{
	/* == != */
	//{ asBEHAVE_EQUAL, SCRIPT_FUNCTION_DECL(bool, f, (cEntity @, cEntity @)), objectGameEntity_EqualBehaviour, objectGameEntity_asGeneric_EqualBehaviour, asCALL_CDECL },
	//{ asBEHAVE_NOTEQUAL, SCRIPT_FUNCTION_DECL(bool, f, (cEntity @, cEntity @)), objectGameEntity_NotEqualBehaviour, objectGameEntity_asGeneric_NotEqualBehaviour, asCALL_CDECL },

	SCRIPT_BEHAVIOR_NULL
};

static const asMethod_t gedict_Methods[] =
{
	{ SCRIPT_FUNCTION_DECL(cVec3 @, getVelocity, ()), objectGameEntity_GetVelocity, objectGameEntity_asGeneric_GetVelocity, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, setVelocity, (cVec3 &in)), objectGameEntity_SetVelocity, objectGameEntity_asGeneric_SetVelocity, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cVec3 @, getAVelocity, ()), objectGameEntity_GetAVelocity, objectGameEntity_asGeneric_GetAVelocity, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, setAVelocity, (cVec3 &in)), objectGameEntity_SetAVelocity, objectGameEntity_asGeneric_SetAVelocity, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cVec3 @, getOrigin, ()), objectGameEntity_GetOrigin, objectGameEntity_asGeneric_GetOrigin, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, setOrigin, (cVec3 &in)), objectGameEntity_SetOrigin, objectGameEntity_asGeneric_SetOrigin, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cVec3 @, getOrigin2, ()), objectGameEntity_GetOrigin2, objectGameEntity_asGeneric_GetOrigin2, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, setOrigin2, (cVec3 &in)), objectGameEntity_SetOrigin2, objectGameEntity_asGeneric_SetOrigin2, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cVec3 @, getAngles, ()), objectGameEntity_GetAngles, objectGameEntity_asGeneric_GetAngles, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, setAngles, (cVec3 &in)), objectGameEntity_SetAngles, objectGameEntity_asGeneric_SetAngles, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, getSize, (cVec3 @+mins, cVec3 @+maxs)), objectGameEntity_GetSize, objectGameEntity_asGeneric_GetSize, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, setSize, (cVec3 @+mins, cVec3 @+maxs)), objectGameEntity_SetSize, objectGameEntity_asGeneric_SetSize, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, getMovedir, (cVec3 &)), objectGameEntity_GetMovedir, objectGameEntity_asGeneric_GetMovedir, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, setMovedir, ()), objectGameEntity_SetMovedir, objectGameEntity_asGeneric_SetMovedir, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(bool, isBrushModel, ()), objectGameEntity_isBrushModel, objectGameEntity_asGeneric_isBrushModel, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, freeEntity, ()), G_FreeEdict, objectGameEntity_asGeneric_G_FreeEdict, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, linkEntity, ()), GClip_LinkEntity, objectGameEntity_asGeneric_GClip_LinkEntity, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, unlinkEntity, ()), GClip_UnlinkEntity, objectGameEntity_asGeneric_GClip_UnlinkEntity, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(bool, isGhosting, ()), objectGameEntity_IsGhosting, objectGameEntity_asGeneric_IsGhosting, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(int, entNum, ()), objectGameEntity_EntNum, objectGameEntity_asGeneric_EntNum, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(int, playerNum, ()), objectGameEntity_PlayerNum, objectGameEntity_asGeneric_PlayerNum, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, getModelString, ()), objectGameEntity_getModelName, objectGameEntity_asGeneric_getModelName, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, getModel2String, ()), objectGameEntity_getModel2Name, objectGameEntity_asGeneric_getModel2Name, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, getSoundString, ()), objectGameEntity_getSoundName, objectGameEntity_asGeneric_getSoundName, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, getClassname, ()), objectGameEntity_getClassname, objectGameEntity_asGeneric_getClassname, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, getClassName, ()), objectGameEntity_getClassname, objectGameEntity_asGeneric_getClassname, asCALL_CDECL_OBJLAST },
	//{ SCRIPT_FUNCTION_DECL(cString @, getSpawnKey, ( cString &in )), objectGameEntity_getSpawnKey, objectGameEntity_asGeneric_getSpawnKey, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, getTargetnameString, ()), objectGameEntity_getTargetname, objectGameEntity_asGeneric_getTargetname, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, getTargetString, ()), objectGameEntity_getTarget, objectGameEntity_asGeneric_getTarget, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cString @, getMapString, ()), objectGameEntity_getMap, objectGameEntity_asGeneric_getMap, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, setTargetString, ( cString &in )), objectGameEntity_setTarget, objectGameEntity_asGeneric_setTarget, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, setTargetnameString, ( cString &in )), objectGameEntity_setTargetname, objectGameEntity_asGeneric_setTargetname, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, setClassname, ( cString &in )), objectGameEntity_setClassname, objectGameEntity_asGeneric_setClassname, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, setClassName, ( cString &in )), objectGameEntity_setClassname, objectGameEntity_asGeneric_setClassname, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, setMapString, ( cString &in )), objectGameEntity_setMap, objectGameEntity_asGeneric_setMap, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, ghost, ()), objectGameEntity_GhostClient, objectGameEntity_asGeneric_GhostClient, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, spawnqueueAdd, ()), G_SpawnQueue_AddClient, objectGameEntity_asGeneric_G_SpawnQueue_AddClient, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, teleportEffect, ( bool )), objectGameEntity_TeleportEffect, objectGameEntity_asGeneric_TeleportEffect, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, respawnEffect, ()), G_RespawnEffect, objectGameEntity_asGeneric_G_RespawnEffect, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, setupModel, ( cString &in )), objectGameEntity_SetupModel, objectGameEntity_asGeneric_SetupModel, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, setupModel, ( cString &in, cString &in )), objectGameEntity_SetupModelExt, objectGameEntity_asGeneric_SetupModelExt, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cEntity @, findTargetEntity, ( cEntity @from )), objectGameEntity_findTarget, objectGameEntity_asGeneric_findTarget, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cEntity @, findTargetingEntity, ( cEntity @from )), objectGameEntity_findTargeting, objectGameEntity_asGeneric_findTargeting, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, use, ( cEntity @targeter, cEntity @activator )), objectGameEntity_Use, objectGameEntity_asGeneric_Use, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, useTargets, ( cEntity @activator )), objectGameEntity_UseTargets, objectGameEntity_asGeneric_UseTargets, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(cEntity @, dropItem, ( int tag )), objectGameEntity_DropItem, objectGameEntity_asGeneric_DropItem, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, addAIGoal, ( bool customReach )), objectGameEntity_setAIgoal, objectGameEntity_asGeneric_setAIgoal, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, addAIGoal, ()), objectGameEntity_setAIgoalSimple, objectGameEntity_asGeneric_setAIgoalSimple, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, removeAIGoal, ()), objectGameEntity_removeAIgoal, objectGameEntity_asGeneric_removeAIgoal, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, reachedAIGoal, ()), objectGameEntity_reachedAIgoal, objectGameEntity_asGeneric_reachedAIgoal, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, takeDamage, ( cEntity @inflicter, cEntity @attacker, cVec3 @dir, float damage, float knockback, float stun, int mod )), objectGameEntity_takeDamage, objectGameEntity_asGeneric_takeDamage, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, splashDamage, ( cEntity @attacker, int radius, float damage, float knockback, float stun, int mod )), objectGameEntity_splashDamage, objectGameEntity_asGeneric_splashDamage, asCALL_CDECL_OBJLAST },
	{ SCRIPT_FUNCTION_DECL(void, explosionEffect, ( int radius )), objectGameEntity_explosionEffect, objectGameEntity_asGeneric_explosionEffect, asCALL_CDECL_OBJLAST },

	SCRIPT_METHOD_NULL
};

static const asProperty_t gedict_Properties[] =
{
	{ SCRIPT_PROPERTY_DECL(cClient @, client), FOFFSET(edict_t, r.client) },
	{ SCRIPT_PROPERTY_DECL(cItem @, item), FOFFSET(edict_t, item) },
	{ SCRIPT_PROPERTY_DECL(cEntity @, groundEntity), FOFFSET(edict_t, groundentity) },
	{ SCRIPT_PROPERTY_DECL(cEntity @, owner), FOFFSET(edict_t, r.owner) },
	{ SCRIPT_PROPERTY_DECL(cEntity @, enemy), FOFFSET(edict_t, enemy) },
	{ SCRIPT_PROPERTY_DECL(cEntity @, activator), FOFFSET(edict_t, activator) },
	{ SCRIPT_PROPERTY_DECL(int, type), FOFFSET(edict_t, s.type) },
	{ SCRIPT_PROPERTY_DECL(int, modelindex), FOFFSET(edict_t, s.modelindex) },
	{ SCRIPT_PROPERTY_DECL(int, modelindex2), FOFFSET(edict_t, s.modelindex2) },
	{ SCRIPT_PROPERTY_DECL(int, frame), FOFFSET(edict_t, s.frame) },
	{ SCRIPT_PROPERTY_DECL(int, ownerNum), FOFFSET(edict_t, s.ownerNum) },
	{ SCRIPT_PROPERTY_DECL(int, counterNum), FOFFSET(edict_t, s.counterNum) },
	{ SCRIPT_PROPERTY_DECL(int, skinNum), FOFFSET(edict_t, s.skinnum) },
	{ SCRIPT_PROPERTY_DECL(int, colorRGBA), FOFFSET(edict_t, s.colorRGBA) },
	{ SCRIPT_PROPERTY_DECL(int, weapon), FOFFSET(edict_t, s.weapon) },
	{ SCRIPT_PROPERTY_DECL(bool, teleported), FOFFSET(edict_t, s.teleported) },
	{ SCRIPT_PROPERTY_DECL(uint, effects), FOFFSET(edict_t, s.effects) },
	{ SCRIPT_PROPERTY_DECL(int, sound), FOFFSET(edict_t, s.sound) },
	{ SCRIPT_PROPERTY_DECL(int, team), FOFFSET(edict_t, s.team) },
	{ SCRIPT_PROPERTY_DECL(int, light), FOFFSET(edict_t, s.light) },
	{ SCRIPT_PROPERTY_DECL(const bool, inuse), FOFFSET(edict_t, r.inuse) },
	{ SCRIPT_PROPERTY_DECL(uint, svflags), FOFFSET(edict_t, r.svflags) },
	{ SCRIPT_PROPERTY_DECL(int, solid), FOFFSET(edict_t, r.solid) },
	{ SCRIPT_PROPERTY_DECL(int, clipMask), FOFFSET(edict_t, r.clipmask) },
	{ SCRIPT_PROPERTY_DECL(int, spawnFlags), FOFFSET(edict_t, spawnflags) },
	{ SCRIPT_PROPERTY_DECL(int, style), FOFFSET(edict_t, style) },
	{ SCRIPT_PROPERTY_DECL(int, moveType), FOFFSET(edict_t, movetype) },
	{ SCRIPT_PROPERTY_DECL(uint, nextThink), FOFFSET(edict_t, nextThink) },
	{ SCRIPT_PROPERTY_DECL(float, health), FOFFSET(edict_t, health) },
	{ SCRIPT_PROPERTY_DECL(int, maxHealth), FOFFSET(edict_t, max_health) },
	{ SCRIPT_PROPERTY_DECL(int, viewHeight), FOFFSET(edict_t, viewheight) },
	{ SCRIPT_PROPERTY_DECL(int, takeDamage), FOFFSET(edict_t, takedamage) },
	{ SCRIPT_PROPERTY_DECL(int, damage), FOFFSET(edict_t, dmg) },
	{ SCRIPT_PROPERTY_DECL(int, projectileMaxDamage), FOFFSET(edict_t, projectileInfo.maxDamage) },
	{ SCRIPT_PROPERTY_DECL(int, projectileMinDamage), FOFFSET(edict_t, projectileInfo.minDamage) },
	{ SCRIPT_PROPERTY_DECL(int, projectileMaxKnockback), FOFFSET(edict_t, projectileInfo.maxKnockback) },
	{ SCRIPT_PROPERTY_DECL(int, projectileMinKnockback), FOFFSET(edict_t, projectileInfo.minKnockback) },
	{ SCRIPT_PROPERTY_DECL(float, projectileSplashRadius), FOFFSET(edict_t, projectileInfo.radius) },
	{ SCRIPT_PROPERTY_DECL(int, count), FOFFSET(edict_t, count) },
	{ SCRIPT_PROPERTY_DECL(float, wait), FOFFSET(edict_t, wait) },
	{ SCRIPT_PROPERTY_DECL(float, delay), FOFFSET(edict_t, delay) },
	{ SCRIPT_PROPERTY_DECL(int, waterLevel), FOFFSET(edict_t, waterlevel) },
	{ SCRIPT_PROPERTY_DECL(float, attenuation), FOFFSET(edict_t, attenuation) },
	{ SCRIPT_PROPERTY_DECL(int, mass), FOFFSET(edict_t, mass) },
	{ SCRIPT_PROPERTY_DECL(uint, timeStamp), FOFFSET(edict_t, timeStamp) },

	// specific for ET_PARTICLES
	{ SCRIPT_PROPERTY_DECL(int, particlesSpeed), FOFFSET(edict_t, particlesInfo.speed) },
	{ SCRIPT_PROPERTY_DECL(int, particlesShaderIndex), FOFFSET(edict_t, particlesInfo.shaderIndex) },
	{ SCRIPT_PROPERTY_DECL(int, particlesSpread), FOFFSET(edict_t, particlesInfo.spread) },
	{ SCRIPT_PROPERTY_DECL(int, particlesSize), FOFFSET(edict_t, particlesInfo.size) },
	{ SCRIPT_PROPERTY_DECL(int, particlesTime), FOFFSET(edict_t, particlesInfo.time) },
	{ SCRIPT_PROPERTY_DECL(int, particlesFrequency), FOFFSET(edict_t, particlesInfo.frequency) },
	{ SCRIPT_PROPERTY_DECL(bool, particlesSpherical), FOFFSET(edict_t, particlesInfo.spherical) },
	{ SCRIPT_PROPERTY_DECL(bool, particlesBounce), FOFFSET(edict_t, particlesInfo.bounce) },
	{ SCRIPT_PROPERTY_DECL(bool, particlesGravity), FOFFSET(edict_t, particlesInfo.gravity) },
	{ SCRIPT_PROPERTY_DECL(bool, particlesExpandEffect), FOFFSET(edict_t, particlesInfo.expandEffect) },
	{ SCRIPT_PROPERTY_DECL(bool, particlesShrinkEffect), FOFFSET(edict_t, particlesInfo.shrinkEffect) },

	SCRIPT_PROPERTY_NULL
};

static const asClassDescriptor_t asGameEntityClassDescriptor =
{
	"cEntity",					/* name */
	asOBJ_REF,					/* object type flags */
	sizeof( edict_t ),			/* size */
	gedict_ObjectBehaviors,		/* object behaviors */
	gedict_GlobalBehaviors,		/* global behaviors */
	gedict_Methods,				/* methods */
	gedict_Properties,			/* properties */

	NULL, NULL					/* string factory hack */
};

//=======================================================================

static const asClassDescriptor_t * const asClassesDescriptors[] =
{
	&asStringClassDescriptor,
	&asVectorClassDescriptor,
	&asTimeClassDescriptor,
	&asTraceClassDescriptor,
	&asCVarClassDescriptor,
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

		if( cDescr->stringFactory )
		{
			char decl[64];

			Q_snprintfz( decl, sizeof( decl ), "%s @", cDescr->name );

			if( level.gametype.asEngineIsGeneric )
				error = angelExport->asRegisterStringFactory( asEngineHandle, decl, cDescr->stringFactory_asGeneric, asCALL_GENERIC );
			else
				error = angelExport->asRegisterStringFactory( asEngineHandle, decl, cDescr->stringFactory, asCALL_CDECL );

			if( error < 0 )
				G_Error( "angelExport->asRegisterStringFactory for object %s returned error %i\n", cDescr->name, error );
		}
	}

	// now register object and global behaviors, then methods and properties
	for( i = 0; ; i++ )
	{
		if( !(cDescr = asClassesDescriptors[i]) )
			break;

		// object behaviors
		if( cDescr->objBehaviors )
		{
			for( j = 0; ; j++ )
			{
				const asBehavior_t *objBehavior = &cDescr->objBehaviors[j];
				if( !objBehavior->declaration )
					break;

				if( level.gametype.asEngineIsGeneric )
					error = angelExport->asRegisterObjectBehaviour( asEngineHandle, cDescr->name, objBehavior->behavior, objBehavior->declaration, objBehavior->funcPointer_asGeneric, asCALL_GENERIC );
				else
					error = angelExport->asRegisterObjectBehaviour( asEngineHandle, cDescr->name, objBehavior->behavior, objBehavior->declaration, objBehavior->funcPointer, objBehavior->callConv );
			}
		}

		// global behaviors
		if( cDescr->globalBehaviors )
		{
			for( j = 0; ; j++ )
			{
				const asBehavior_t *globalBehavior = &cDescr->globalBehaviors[j];
				if( !globalBehavior->declaration )
					break;

				if( level.gametype.asEngineIsGeneric )
					error = angelExport->asRegisterGlobalBehaviour( asEngineHandle, globalBehavior->behavior, globalBehavior->declaration, globalBehavior->funcPointer_asGeneric, asCALL_GENERIC );
				else
					error = angelExport->asRegisterGlobalBehaviour( asEngineHandle, globalBehavior->behavior, globalBehavior->declaration, globalBehavior->funcPointer, globalBehavior->callConv );
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

				if( level.gametype.asEngineIsGeneric )
					error = angelExport->asRegisterObjectMethod( asEngineHandle, cDescr->name, objMethod->declaration, objMethod->funcPointer_asGeneric, asCALL_GENERIC );
				else
					error = angelExport->asRegisterObjectMethod( asEngineHandle, cDescr->name, objMethod->declaration, objMethod->funcPointer, objMethod->callConv );
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

void G_asGetEntityEventScriptFunctions( const char *classname, edict_t *ent );

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

	if( classname && classname->len )
	{
		ent->classname = G_RegisterLevelString( classname->buffer );
		G_asGetEntityEventScriptFunctions( classname->buffer, ent );
	}

	ent->scriptSpawned = qtrue;

	return ent;
}

static void asFunc_asGeneric_G_Spawn( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, asFunc_G_Spawn( G_asGeneric_GetArgAddress( gen, 0 ) ) );
}

static edict_t *asFunc_GetEntity( int entNum )
{
	if( entNum < 0 || entNum >= game.numentities )
		return NULL;

	return &game.edicts[ entNum ];
}

static void asFunc_asGeneric_GetEntity( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, asFunc_GetEntity( G_asGeneric_GetArgInt( gen, 0 ) ) );
}

static gclient_t *asFunc_GetClient( int clientNum )
{
	if( clientNum < 0 || clientNum >= gs.maxclients )
		return NULL;

	return &game.clients[ clientNum ];
}

static void asFunc_asGeneric_GetClient( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, asFunc_GetClient( G_asGeneric_GetArgInt( gen, 0 ) ) );
}

static g_teamlist_t *asFunc_GetTeamlist( int teamNum )
{
	if( teamNum < TEAM_SPECTATOR || teamNum >= GS_MAX_TEAMS )
		return NULL;

	return &teamlist[teamNum];
}

static void asFunc_asGeneric_GetTeamlist( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, asFunc_GetTeamlist( G_asGeneric_GetArgInt( gen, 0 ) ) );
}

static gsitem_t *asFunc_GS_FindItemByTag( int tag )
{
	return GS_FindItemByTag( tag );
}

static void asFunc_asGeneric_GS_FindItemByTag( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, asFunc_GS_FindItemByTag( G_asGeneric_GetArgInt( gen, 0 ) ) );
}

static gsitem_t *asFunc_GS_FindItemByName( asstring_t *name )
{
	return ( !name || !name->len ) ? NULL : GS_FindItemByName( name->buffer );
}

static void asFunc_asGeneric_GS_FindItemByName( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, asFunc_GS_FindItemByName( G_asGeneric_GetArgAddress( gen, 0 ) ) );
}

static gsitem_t *asFunc_GS_FindItemByClassname( asstring_t *name )
{
	return ( !name || !name->len ) ? NULL : GS_FindItemByName( name->buffer );
}

static void asFunc_asGeneric_GS_FindItemByClassname( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, asFunc_GS_FindItemByClassname( G_asGeneric_GetArgAddress( gen, 0 ) ) );
}

static void asFunc_G_Match_RemoveAllProjectiles( void )
{
	G_Match_RemoveAllProjectiles();
}

static void asFunc_asGeneric_G_Match_RemoveAllProjectiles( void *gen )
{
	G_Match_RemoveAllProjectiles();
}

static void asFunc_G_Match_FreeBodyQueue( void )
{
	G_Match_FreeBodyQueue();
}

static void asFunc_asGeneric_G_Match_FreeBodyQueue( void *gen )
{
	G_Match_FreeBodyQueue();
}

static void asFunc_G_Items_RespawnByType( unsigned int typeMask, int item_tag, float delay )
{
	G_Items_RespawnByType( typeMask, item_tag, delay );
}

static void asFunc_asGeneric_G_Items_RespawnByType( void *gen )
{
	unsigned int typeMask = G_asGeneric_GetArgInt( gen, 0 );
	int item_tag = G_asGeneric_GetArgInt( gen, 1 );
	float delay = G_asGeneric_GetArgFloat( gen, 2 );

	G_Items_RespawnByType( typeMask, item_tag, delay );
}

static void asFunc_Print( const asstring_t *str )
{
	if( !str || !str->buffer )
		return;

	G_Printf( "%s", str->buffer );
}

static void asFunc_asGeneric_Print( void *gen )
{
	asFunc_Print( (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 ) );
}

static void asFunc_PrintMsg( edict_t *ent, asstring_t *str )
{
	if( !str || !str->buffer )
		return;

	G_PrintMsg( ent, "%s", str->buffer );
}

static void asFunc_asGeneric_PrintMsg( void *gen )
{
	asFunc_PrintMsg( (edict_t *)G_asGeneric_GetArgAddress( gen, 0 ), (asstring_t *)G_asGeneric_GetArgAddress( gen, 1 ) );
}

static void asFunc_CenterPrintMsg( edict_t *ent, asstring_t *str )
{
	if( !str || !str->buffer )
		return;

	G_CenterPrintMsg( ent, "%s", str->buffer );
}

static void asFunc_asGeneric_CenterPrintMsg( void *gen )
{
	asFunc_CenterPrintMsg( (edict_t *)G_asGeneric_GetArgAddress( gen, 0 ), (asstring_t *)G_asGeneric_GetArgAddress( gen, 1 ) );
}

static void asFunc_G_Sound( edict_t *owner, int channel, int soundindex, float attenuation )
{
	G_Sound( owner, channel, soundindex, attenuation );
}

static void asFunc_asGeneric_G_Sound( void *gen )
{
	edict_t *owner = (edict_t *)G_asGeneric_GetArgAddress( gen, 0 );
	int channel = G_asGeneric_GetArgInt( gen, 1 );
	int soundindex = G_asGeneric_GetArgInt( gen, 2 );
	float attenuation = G_asGeneric_GetArgFloat( gen, 3 );

	G_Sound( owner, channel, soundindex, attenuation );
}

static float asFunc_Random( void )
{
	return random();
}

static void asFunc_asGeneric_Random( void *gen )
{
	G_asGeneric_SetReturnFloat( gen, asFunc_Random() );
}

static float asFunc_BRandom( float min, float max )
{
	return brandom( min, max );
}

static void asFunc_asGeneric_BRandom( void *gen )
{
	G_asGeneric_SetReturnFloat( gen, asFunc_BRandom( G_asGeneric_GetArgFloat( gen, 0 ), G_asGeneric_GetArgFloat( gen, 1 ) ) );
}

static int asFunc_DirToByte( asvec3_t *vec )
{
	if( !vec )
		return 0;

	return DirToByte( vec->v );
}

static void asFunc_asGeneric_DirToByte( void *gen )
{
	G_asGeneric_SetReturnInt( gen, asFunc_DirToByte( (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 ) ) );
}

static int asFunc_PointContents( asvec3_t *vec )
{
	if( !vec )
		return 0;

	return G_PointContents( vec->v );
}

static void asFunc_asGeneric_PointContents( void *gen )
{
	G_asGeneric_SetReturnInt( gen, asFunc_PointContents( (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 ) ) );
}

static qboolean asFunc_InPVS( asvec3_t *origin1, asvec3_t *origin2 )
{
	return trap_inPVS( origin1->v, origin2->v );
}

static void asFunc_asGeneric_InPVS( void *gen )
{
	G_asGeneric_SetReturnBool( gen,
		asFunc_InPVS( (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 ),
		(asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 ) )
		);
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

static void asFunc_asGeneric_WriteFile( void *gen )
{
	G_asGeneric_SetReturnBool( gen,
		asFunc_WriteFile( (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 ),
		(asstring_t *)G_asGeneric_GetArgAddress( gen, 1 ) )
		);
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

static void asFunc_asGeneric_AppendToFile( void *gen )
{
	G_asGeneric_SetReturnBool( gen,
		asFunc_AppendToFile( (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 ),
		(asstring_t *)G_asGeneric_GetArgAddress( gen, 1 ) )
		);
}

static asstring_t *asFunc_LoadFile( asstring_t *path )
{
	int filelen, filehandle;
	qbyte *buf = NULL;
	asstring_t *data;

	if( !path || !path->len )
		return objectString_FactoryBuffer( NULL, 0 );

	filelen = trap_FS_FOpenFile( path->buffer, &filehandle, FS_READ );
	if( filehandle && filelen > 0 )
	{
		buf = G_Malloc( filelen + 1 );
		filelen = trap_FS_Read( buf, filelen, filehandle );
	}

	trap_FS_FCloseFile( filehandle );

	if( !buf )
		return objectString_FactoryBuffer( NULL, 0 );

	data = objectString_FactoryBuffer( (char *)buf, filelen );
	G_Free( buf );

	return data;
}

static void asFunc_asGeneric_LoadFile( void *gen )
{
	G_asGeneric_SetReturnAddress( gen,
		asFunc_LoadFile( (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 ) ) );
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
    return objectString_FactoryBuffer(hex_output, strlen(hex_output));
}
static void asFunc_asGeneric_G_Md5( void *gen )
{
    G_asGeneric_SetReturnAddress( gen, asFunc_G_Md5( G_asGeneric_GetArgAddress( gen, 0 ) ) );
}

// RS_MysqlMapFilter
static qboolean asFunc_RS_MapFilter(int player_id,asstring_t *filter,unsigned int page )
{
     return RS_MapFilter( player_id, filter->buffer, page);
}
static void asFunc_asGeneric_RS_MapFilter( void *gen )
{
    G_asGeneric_SetReturnBool( gen, asFunc_RS_MapFilter(
            (int)G_asGeneric_GetArgInt(gen, 0),
            (asstring_t *)G_asGeneric_GetArgAddress(gen, 1),
            (unsigned int)G_asGeneric_GetArgInt(gen, 2)));
}

// RS_UpdateMapList
static qboolean asFunc_RS_UpdateMapList( int playerNum )
{
     return RS_UpdateMapList( playerNum );
}

static void asFunc_asGeneric_RS_UpdateMapList( void *gen )
{
    G_asGeneric_SetReturnBool( gen, asFunc_RS_UpdateMapList(
            (int)G_asGeneric_GetArgInt(gen, 0)
    ));
}

// RS_LoadStats
static qboolean asFunc_RS_LoadStats(int player_id, asstring_t *what, asstring_t *which)
{
     return RS_LoadStats( player_id, what->buffer, which->buffer);
}
static void asFunc_asGeneric_RS_LoadStats( void *gen )
{
    G_asGeneric_SetReturnBool( gen, asFunc_RS_LoadStats(
            (int)G_asGeneric_GetArgInt(gen, 0),
            (asstring_t *)G_asGeneric_GetArgAddress(gen, 1),
            (asstring_t *)G_asGeneric_GetArgAddress(gen, 2)));
}

// RS_MysqlMapFilterCallback
static asstring_t *asFunc_RS_PrintQueryCallback(int player_id )
{
     char *result;
     asstring_t *result_as;
     result = RS_PrintQueryCallback(player_id);
     result_as=objectString_FactoryBuffer(result, strlen(result));
     free(result);
     return result_as;
}

static void asFunc_asGeneric_RS_PrintQueryCallback( void *gen )
{
    G_asGeneric_SetReturnAddress( gen, asFunc_RS_PrintQueryCallback(
            (unsigned int)G_asGeneric_GetArgInt(gen, 0)));
}

// RS_Maplist
static qboolean asFunc_RS_Maplist(int player_id, unsigned int page )
{
     return RS_Maplist( player_id, page);
}

static void asFunc_asGeneric_RS_Maplist( void *gen )
{
    G_asGeneric_SetReturnBool( gen, asFunc_RS_Maplist(
            (int)G_asGeneric_GetArgInt(gen, 0),
            (unsigned int)G_asGeneric_GetArgInt(gen, 1)));
}

// RS_NextMap
static asstring_t *asFunc_RS_NextMap( )
{
     char *result;
     result = RS_ChooseNextMap( );
     return objectString_FactoryBuffer(result, strlen(result));
}

static void asFunc_asGeneric_RS_NextMap( void *gen )
{
    G_asGeneric_SetReturnAddress( gen, asFunc_RS_NextMap( ) );
}

// RS_LastMap
static asstring_t *asFunc_RS_LastMap( )
{
     char *result;
     result = previousMapName;
     return objectString_FactoryBuffer(result, strlen(result));
}

static void asFunc_asGeneric_RS_LastMap( void *gen )
{
    G_asGeneric_SetReturnAddress( gen, asFunc_RS_LastMap( ) );
}

// RS_LoadMapList
static void asFunc_RS_LoadMapList( int is_freestyle )
{
    RS_LoadMaplist( is_freestyle );
}
static void asFunc_asGeneric_RS_LoadMapList( void *gen )
{
    asFunc_RS_LoadMapList( (int)G_asGeneric_GetArgInt( gen, 0 ) );
}

// RS_removeProjectiles
static void asFunc_RS_removeProjectiles( edict_t *owner )
{
    RS_removeProjectiles( owner );
}
static void asFunc_asGeneric_RS_removeProjectiles( void *gen )
{
    edict_t *owner = (edict_t *)G_asGeneric_GetArgAddress( gen, 0 );
    RS_removeProjectiles( owner );
}

// RS_QueryPjState
static qboolean asFunc_RS_QueryPjState(int playerNum)
{
     return RS_QueryPjState(playerNum);
}

static void asFunc_asGeneric_RS_QueryPjState( void *gen )
{
    G_asGeneric_SetReturnBool( gen, asFunc_RS_QueryPjState(
            (int)G_asGeneric_GetArgInt(gen, 0)));
}

// RS_ResetPjState
static qboolean asFunc_RS_ResetPjState(int playerNum)
{
     RS_ResetPjState(playerNum);
     return qtrue;
}

static void asFunc_asGeneric_RS_ResetPjState( void *gen )
{
    G_asGeneric_SetReturnBool( gen, asFunc_RS_ResetPjState(
            (int)G_asGeneric_GetArgInt(gen, 0)));
}

// RS_MysqlLoadMap
static qboolean asFunc_RS_MysqlLoadMap()
{
    return RS_MysqlLoadMap();
}

static void asFunc_asGeneric_RS_MysqlLoadMap( void *gen )
{
    G_asGeneric_SetReturnBool(gen, asFunc_RS_MysqlLoadMap());
}

// RS_MysqlInsertRace
static qboolean asFunc_RS_MysqlInsertRace( int player_id, int nick_id, int map_id, int race_time, int playerNum, int tries, int duration, asstring_t *checkpoints, qboolean prejumped)
{
    return RS_MysqlInsertRace(player_id, nick_id, map_id, race_time, playerNum, tries, duration, checkpoints->buffer, prejumped );
}

static void asFunc_asGeneric_RS_MysqlInsertRace( void *gen )
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
}

// RS_MysqlPlayerAppear
static qboolean asFunc_RS_MysqlPlayerAppear( asstring_t *playerName, int playerNum, int player_id, int map_id, int is_authed, asstring_t *authName, asstring_t *authPass, asstring_t *authToken)
{
    return RS_MysqlPlayerAppear(playerName->buffer, playerNum, player_id, map_id, is_authed, authName->buffer, authPass->buffer, authToken->buffer);
}

static void asFunc_asGeneric_RS_MysqlPlayerAppear( void *gen )
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
}

// RS_MysqlPlayerDisappear
static qboolean asFunc_RS_MysqlPlayerDisappear( asstring_t *playerName, int playtime, int overall_tries, int racing_time, int player_id, int nick_id, int map_id, qboolean is_authed, qboolean is_threaded)
{
    return RS_MysqlPlayerDisappear(playerName->buffer, playtime, overall_tries, racing_time, player_id, nick_id, map_id, (int)(is_authed==qtrue), (int)(is_threaded==qtrue) );
}

static void asFunc_asGeneric_RS_MysqlPlayerDisappear( void *gen )
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
}

// RS_GetPlayerNick
static qboolean asFunc_RS_GetPlayerNick( int playerNum, int player_id )
{
    return RS_GetPlayerNick(playerNum, player_id);
}

static void asFunc_asGeneric_RS_GetPlayerNick( void *gen )
{
    G_asGeneric_SetReturnBool(gen, asFunc_RS_GetPlayerNick(
        (int)G_asGeneric_GetArgInt(gen, 0),
        (int)G_asGeneric_GetArgInt(gen, 1)));
}

// RS_UpdatePlayerNick
static qboolean asFunc_RS_UpdatePlayerNick( asstring_t * name, int playerNum, int player_id )
{
    return RS_UpdatePlayerNick(name->buffer, playerNum, player_id);
}

static void asFunc_asGeneric_RS_UpdatePlayerNick( void *gen )
{
    G_asGeneric_SetReturnBool(gen, asFunc_RS_UpdatePlayerNick(
        (asstring_t *)G_asGeneric_GetArgAddress(gen, 0),
        (int)G_asGeneric_GetArgInt(gen, 1),
        (int)G_asGeneric_GetArgInt(gen, 2)));
}

// RS_MysqlLoadHighScores
static qboolean asFunc_RS_MysqlLoadHighscores( int playerNum, int limit, int map_id, asstring_t *mapname, pjflag prejumped )
{
    return RS_MysqlLoadHighscores(playerNum, limit, map_id, mapname->buffer, prejumped);
}

static void asFunc_asGeneric_RS_MysqlLoadHighscores( void *gen )
{
    G_asGeneric_SetReturnBool(gen, asFunc_RS_MysqlLoadHighscores(
        (int)G_asGeneric_GetArgInt(gen, 0),
        (int)G_asGeneric_GetArgInt(gen, 1),
        (int)G_asGeneric_GetArgInt(gen, 2),
        (asstring_t *)G_asGeneric_GetArgAddress(gen, 3),
        (int)G_asGeneric_GetArgInt(gen, 4)));
}

// RS_MysqlLoadRanking
static qboolean asFunc_RS_MysqlLoadRanking( int playerNum, int page, asstring_t *order )
{
    return RS_MysqlLoadRanking(playerNum, page, order->buffer);
}

static void asFunc_asGeneric_RS_MysqlLoadRanking( void *gen )
{
    G_asGeneric_SetReturnBool(gen, asFunc_RS_MysqlLoadRanking(
        (int)G_asGeneric_GetArgInt(gen, 0),
        (int)G_asGeneric_GetArgInt(gen, 1),
		(asstring_t *)G_asGeneric_GetArgAddress(gen, 2)));
}

// RS_MysqlSetOneliner
static qboolean asFunc_RS_MysqlSetOneliner( int playerNum, int player_id, int map_id, asstring_t *oneliner)
{
    return RS_MysqlSetOneliner(playerNum, player_id, map_id, oneliner->buffer);
}

static void asFunc_asGeneric_RS_MysqlSetOneliner( void *gen )
{
    G_asGeneric_SetReturnBool(gen, asFunc_RS_MysqlSetOneliner(
        (int)G_asGeneric_GetArgInt(gen, 0),
        (int)G_asGeneric_GetArgInt(gen, 1),
        (int)G_asGeneric_GetArgInt(gen, 2),
        (asstring_t *)G_asGeneric_GetArgAddress(gen, 3)));
}


// RS_PopCallbackQueue
static qboolean asFunc_RS_PopCallbackQueue( int *command, int *arg1, int *arg2, int *arg3, int *arg4, int *arg5, int *arg6, int *arg7 )
{
    return RS_PopCallbackQueue(command, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
}

static void asFunc_asGeneric_RS_PopCallbackQueue( void *gen )
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
}
// !racesow

static int asFunc_FileLength( asstring_t *path )
{
	if( !path || !path->len )
		return qfalse;

	return ( trap_FS_FOpenFile( path->buffer, NULL, FS_READ ) );
}

static void asFunc_asGeneric_FileLength( void *gen )
{
	G_asGeneric_SetReturnInt( gen,
		asFunc_FileLength( (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 ) ) );
}

static void asFunc_Cmd_ExecuteText( asstring_t *str )
{
	if( !str || !str->buffer || !str->buffer[0] )
		return;

	trap_Cmd_ExecuteText( EXEC_APPEND, str->buffer );
}

static void asFunc_asGeneric_Cmd_ExecuteText( void *gen )
{
	asFunc_Cmd_ExecuteText( (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 ) );
}

static qboolean asFunc_ML_FilenameExists( asstring_t *filename )
{
	return trap_ML_FilenameExists( filename->buffer );
}

static void asFunc_asGeneric_ML_FilenameExists( void *gen )
{
	G_asGeneric_SetReturnBool( gen,
		asFunc_ML_FilenameExists( (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 ) ) );
}

static asstring_t *asFunc_ML_GetMapByNum( int num )
{
	char mapname[MAX_QPATH];
	asstring_t *data;

	if( !trap_ML_GetMapByNum( num, mapname, sizeof( mapname ) ) )
		return NULL;

	data = objectString_FactoryBuffer( (char *)mapname, strlen( mapname ) );
	return data;
}

static void asFunc_asGeneric_ML_GetMapByNum( void *gen )
{
	G_asGeneric_SetReturnAddress( gen,
		asFunc_ML_GetMapByNum( G_asGeneric_GetArgInt( gen, 0 ) ) );
}

static asstring_t *asFunc_LocationName( asvec3_t *origin )
{
	char buf[MAX_CONFIGSTRING_CHARS];
	asstring_t *data;

	G_LocationName( origin->v, buf, sizeof( buf ) );

	data = objectString_FactoryBuffer( (char *)buf, strlen( buf ) );
	return data;
}

static void asFunc_asGeneric_LocationName( void *gen )
{
	G_asGeneric_SetReturnAddress( gen,
		asFunc_LocationName( (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 ) ) );
}

static int asFunc_LocationTag( asstring_t *str )
{
	return G_LocationTAG( str->buffer );
}

static void asFunc_asGeneric_LocationTag( void *gen )
{
	G_asGeneric_SetReturnInt( gen, asFunc_LocationTag( (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 ) ) );
}

static asstring_t *asFunc_LocationForTag( int tag )
{
	char buf[MAX_CONFIGSTRING_CHARS];
	asstring_t *data;

	G_LocationForTAG( tag, buf, sizeof( buf ) );

	data = objectString_FactoryBuffer( (char *)buf, strlen( buf ) );

	return data;
}

static void asFunc_asGeneric_LocationForTag( void *gen )
{
	G_asGeneric_SetReturnAddress( gen,
		asFunc_LocationForTag( G_asGeneric_GetArgInt( gen, 0 ) ) );
}

static int asFunc_ImageIndex( asstring_t *str )
{
	if( !str || !str->buffer )
		return 0;

	return trap_ImageIndex( str->buffer );
}

static void asFunc_asGeneric_ImageIndex( void *gen )
{
	G_asGeneric_SetReturnInt( gen, asFunc_ImageIndex( (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 ) ) );
}

static int asFunc_SkinIndex( asstring_t *str )
{
	if( !str || !str->buffer )
		return 0;

	return trap_SkinIndex( str->buffer );
}

static void asFunc_asGeneric_SkinIndex( void *gen )
{
	G_asGeneric_SetReturnInt( gen, asFunc_SkinIndex( (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 ) ) );
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

static void asFunc_asGeneric_ModelIndexExt( void *gen )
{
	G_asGeneric_SetReturnInt( gen, asFunc_ModelIndexExt( (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 ), G_asGeneric_GetArgBool( gen, 1 ) ) );
}

static int asFunc_ModelIndex( asstring_t *str )
{
	return asFunc_ModelIndexExt( str, qfalse );
}

static void asFunc_asGeneric_ModelIndex( void *gen )
{
	G_asGeneric_SetReturnInt( gen, asFunc_ModelIndex( (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 ) ) );
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

static void asFunc_asGeneric_SoundIndexExt( void *gen )
{
	G_asGeneric_SetReturnInt( gen, asFunc_SoundIndexExt( (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 ), G_asGeneric_GetArgBool( gen, 1 ) ) );
}

static int asFunc_SoundIndex( asstring_t *str )
{
	return asFunc_SoundIndexExt( str, qfalse );
}

static void asFunc_asGeneric_SoundIndex( void *gen )
{
	G_asGeneric_SetReturnInt( gen, asFunc_SoundIndex( (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 ) ) );
}

static void asFunc_RegisterCommand( asstring_t *str )
{
	if( !str || !str->buffer || !str->len )
		return;

	G_AddCommand( str->buffer, NULL );
}

static void asFunc_asGeneric_RegisterCommand( void *gen )
{
	asFunc_RegisterCommand( (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 ) );
}

static void asFunc_RegisterCallvote( asstring_t *asname, asstring_t *asusage, asstring_t *ashelp )
{
	if( !asname || !asname->buffer || !asname->buffer[0]  )
		return;

	G_RegisterGametypeScriptCallvote( asname->buffer, asusage ? asusage->buffer : NULL, ashelp ? ashelp->buffer : NULL );
}

static void asFunc_asGeneric_RegisterCallvote( void *gen )
{
	asFunc_RegisterCallvote( (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 ),
		(asstring_t *)G_asGeneric_GetArgAddress( gen, 1 ),
		(asstring_t *)G_asGeneric_GetArgAddress( gen, 2 ) );
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

static void asFunc_asGeneric_ConfigString( void *gen )
{
	int index = G_asGeneric_GetArgInt( gen, 0 );
	asstring_t *str = (asstring_t *)G_asGeneric_GetArgAddress( gen, 1 );
	asFunc_ConfigString( index, str );
}

static edict_t *asFunc_FindEntityInRadiusExt( edict_t *from, edict_t *to, asvec3_t *org, float radius )
{
	if( !org )
		return NULL;
	return G_FindBoxInRadius( from, to, org->v, radius );
}

static void asFunc_asGeneric_FindEntityInRadiusExt( void *gen )
{
	edict_t *from = (edict_t *)G_asGeneric_GetArgAddress( gen, 0 );
	edict_t *to = (edict_t *)G_asGeneric_GetArgAddress( gen, 1 );
	asvec3_t *org = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 2 );
	float radius = G_asGeneric_GetArgFloat( gen, 3 );

	G_asGeneric_SetReturnAddress( gen, asFunc_FindEntityInRadiusExt( from, to, org, radius ) );
}

static edict_t *asFunc_FindEntityInRadius( edict_t *from, asvec3_t *org, float radius )
{
	return asFunc_FindEntityInRadiusExt( from, NULL, org, radius );
}

static void asFunc_asGeneric_FindEntityInRadius( void *gen )
{
	edict_t *from = (edict_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asvec3_t *org = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 );
	float radius = G_asGeneric_GetArgFloat( gen, 2 );

	G_asGeneric_SetReturnAddress( gen, asFunc_FindEntityInRadius( from, org, radius ) );
}

static edict_t *asFunc_FindEntityWithClassname( edict_t *from, asstring_t *str )
{
	if( !str || !str->buffer )
		return NULL;

	return G_Find( from, FOFS( classname ), str->buffer );
}

static void asFunc_asGeneric_FindEntityWithClassname( void *gen )
{
	edict_t *from = (edict_t *)G_asGeneric_GetArgAddress( gen, 0 );
	asstring_t *str = (asstring_t *)G_asGeneric_GetArgAddress( gen, 1 );

	G_asGeneric_SetReturnAddress( gen, asFunc_FindEntityWithClassname( from, str ) );
}

static void asFunc_PositionedSound( asvec3_t *origin, int channel, int soundindex, float attenuation )
{
	if( !origin )
		return;

	G_PositionedSound( origin->v, channel, soundindex, attenuation );
}

static void asFunc_asGeneric_PositionedSound( void *gen )
{
	asvec3_t *origin = (asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 );
	int channel = G_asGeneric_GetArgInt( gen, 1 );
	int soundindex = G_asGeneric_GetArgInt( gen, 2 );
	int attenuation = G_asGeneric_GetArgInt( gen, 3 );

	asFunc_PositionedSound( origin, channel, soundindex, attenuation );
}

static void asFunc_G_GlobalSound( int channel, int soundindex )
{
	G_GlobalSound( channel, soundindex );
}

static void asFunc_asGeneric_G_GlobalSound( void *gen )
{
	int channel = G_asGeneric_GetArgInt( gen, 0 );
	int soundindex = G_asGeneric_GetArgInt( gen, 1 );

	asFunc_G_GlobalSound( channel, soundindex );
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

static void asFunc_asGeneric_G_AnnouncerSound( void *gen )
{
	gclient_t *target = (gclient_t *)G_asGeneric_GetArgAddress( gen, 0 );
	int soundindex = G_asGeneric_GetArgInt( gen, 1 );
	int team = G_asGeneric_GetArgInt( gen, 2 );
	qboolean queued = ( G_asGeneric_GetArgBool( gen, 3 ) != 0 );
	gclient_t *ignore = (gclient_t *)G_asGeneric_GetArgAddress( gen, 4 );

	asFunc_G_AnnouncerSound( target, soundindex, team, queued, ignore );
}

static asstring_t *asFunc_G_SpawnTempValue( asstring_t *key )
{
	const char *val;

	if( !key )
		return objectString_FactoryBuffer( NULL, 0 );

	if( level.spawning_entity == NULL )
		G_Printf( "WARNING: G_SpawnTempValue: Spawn temp values can only be grabbed during the entity spawning process\n" );

	val = G_GetEntitySpawnKey( key->buffer, level.spawning_entity );

	return objectString_FactoryBuffer( val, strlen( val ) );
}

static void asFunc_asGeneric_G_SpawnTempValue( void *gen )
{
	G_asGeneric_SetReturnAddress( gen, asFunc_G_SpawnTempValue( (asstring_t *)G_asGeneric_GetArgAddress( gen, 0 ) ) );
}

static void asFunc_FireInstaShot( asvec3_t *origin, asvec3_t *angles, int range, int damage, int knockback, int stun, edict_t *owner )
{
	W_Fire_Instagun( owner, origin->v, angles->v, damage, knockback, stun, 0, range, MOD_INSTAGUN_S, 0 );
}

static void asFunc_asGeneric_FireInstaShot( void *gen )
{
	asFunc_FireInstaShot(
		(asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 ),
		(asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 ),
		G_asGeneric_GetArgInt( gen, 2 ),
		G_asGeneric_GetArgInt( gen, 3 ),
		G_asGeneric_GetArgInt( gen, 4 ),
		G_asGeneric_GetArgInt( gen, 5 ),
		(edict_t *)G_asGeneric_GetArgAddress( gen, 6 )
		);
}

static edict_t *asFunc_FireWeakBolt( asvec3_t *origin, asvec3_t *angles, int speed, int damage, int knockback, int stun, edict_t *owner )
{
	return W_Fire_Electrobolt_Weak( owner, origin->v, angles->v, speed, damage, min( 1, knockback ), knockback, stun, 5000, MOD_ELECTROBOLT_W, 0 );
}

static void asFunc_asGeneric_FireWeakBolt( void *gen )
{
	G_asGeneric_SetReturnAddress( gen,
		asFunc_FireWeakBolt(
		(asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 ),
		(asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 ),
		G_asGeneric_GetArgInt( gen, 2 ),
		G_asGeneric_GetArgInt( gen, 3 ),
		G_asGeneric_GetArgInt( gen, 4 ),
		G_asGeneric_GetArgInt( gen, 5 ),
		(edict_t *)G_asGeneric_GetArgAddress( gen, 6 )
		)
		);
}

static void asFunc_FireStrongBolt( asvec3_t *origin, asvec3_t *angles, int range, int damage, int knockback, int stun, edict_t *owner )
{
	W_Fire_Electrobolt_FullInstant( owner, origin->v, angles->v, damage, damage, knockback, knockback, stun, range, range, MOD_ELECTROBOLT_S, 0 );
}

static void asFunc_asGeneric_FireStrongBolt( void *gen )
{
	asFunc_FireStrongBolt(
		(asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 ),
		(asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 ),
		G_asGeneric_GetArgInt( gen, 2 ),
		G_asGeneric_GetArgInt( gen, 3 ),
		G_asGeneric_GetArgInt( gen, 4 ),
		G_asGeneric_GetArgInt( gen, 5 ),
		(edict_t *)G_asGeneric_GetArgAddress( gen, 6 )
		);
}

static edict_t *asFunc_FirePlasma( asvec3_t *origin, asvec3_t *angles, int speed, int radius, int damage, int knockback, int stun, edict_t *owner )
{
	return W_Fire_Plasma( owner, origin->v, angles->v, damage, min( 1, knockback ), knockback, stun, min( 1, damage ), radius, speed, 5000, MOD_PLASMA_S, 0 );
}

static void asFunc_asGeneric_FirePlasma( void *gen )
{
	G_asGeneric_SetReturnAddress( gen,
		asFunc_FirePlasma(
		(asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 ),
		(asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 ),
		G_asGeneric_GetArgInt( gen, 2 ),
		G_asGeneric_GetArgInt( gen, 3 ),
		G_asGeneric_GetArgInt( gen, 4 ),
		G_asGeneric_GetArgInt( gen, 5 ),
		G_asGeneric_GetArgInt( gen, 6 ),
		(edict_t *)G_asGeneric_GetArgAddress( gen, 7 )
		)
		);
}

static edict_t *asFunc_FireRocket( asvec3_t *origin, asvec3_t *angles, int speed, int radius, int damage, int knockback, int stun, edict_t *owner )
{
	return W_Fire_Rocket( owner, origin->v, angles->v, speed, damage, min( 1, knockback ), knockback, stun, min( 1, damage ), radius, 5000, MOD_ROCKET_S, 0 );
}

static void asFunc_asGeneric_FireRocket( void *gen )
{
	G_asGeneric_SetReturnAddress( gen,
		asFunc_FireRocket(
		(asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 ),
		(asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 ),
		G_asGeneric_GetArgInt( gen, 2 ),
		G_asGeneric_GetArgInt( gen, 3 ),
		G_asGeneric_GetArgInt( gen, 4 ),
		G_asGeneric_GetArgInt( gen, 5 ),
		G_asGeneric_GetArgInt( gen, 6 ),
		(edict_t *)G_asGeneric_GetArgAddress( gen, 7 )
		)
		);
}

static edict_t *asFunc_FireGrenade( asvec3_t *origin, asvec3_t *angles, int speed, int radius, int damage, int knockback, int stun, edict_t *owner )
{
	return W_Fire_Grenade( owner, origin->v, angles->v, speed, damage, min( 1, knockback ), knockback, stun, min( 1, damage ), radius, 5000, MOD_GRENADE_S, 0, qfalse );
}

static void asFunc_asGeneric_FireGrenade( void *gen )
{
	G_asGeneric_SetReturnAddress( gen,
		asFunc_FireGrenade(
		(asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 ),
		(asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 ),
		G_asGeneric_GetArgInt( gen, 2 ),
		G_asGeneric_GetArgInt( gen, 3 ),
		G_asGeneric_GetArgInt( gen, 4 ),
		G_asGeneric_GetArgInt( gen, 5 ),
		G_asGeneric_GetArgInt( gen, 6 ),
		(edict_t *)G_asGeneric_GetArgAddress( gen, 7 )
		)
		);
}

static void asFunc_FireRiotgun( asvec3_t *origin, asvec3_t *angles, int range, int spread, int count, int damage, int knockback, int stun, edict_t *owner )
{
	W_Fire_Riotgun( owner, origin->v, angles->v, rand() & 255, range, spread, count, damage, knockback, stun, MOD_RIOTGUN_S, 0 );
}

static void asFunc_asGeneric_FireRiotgun( void *gen )
{
	asFunc_FireRiotgun(
		(asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 ),
		(asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 ),
		G_asGeneric_GetArgInt( gen, 2 ),
		G_asGeneric_GetArgInt( gen, 3 ),
		G_asGeneric_GetArgInt( gen, 4 ),
		G_asGeneric_GetArgInt( gen, 5 ),
		G_asGeneric_GetArgInt( gen, 6 ),
		G_asGeneric_GetArgInt( gen, 7 ),
		(edict_t *)G_asGeneric_GetArgAddress( gen, 8 )
		);
}

static void asFunc_FireBullet( asvec3_t *origin, asvec3_t *angles, int range, int spread, int damage, int knockback, int stun, edict_t *owner )
{
	W_Fire_Bullet( owner, origin->v, angles->v, rand() & 255, range, spread, damage, knockback, stun, MOD_MACHINEGUN_S, 0 );
}

static void asFunc_asGeneric_FireBullet( void *gen )
{
	asFunc_FireBullet(
		(asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 ),
		(asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 ),
		G_asGeneric_GetArgInt( gen, 2 ),
		G_asGeneric_GetArgInt( gen, 3 ),
		G_asGeneric_GetArgInt( gen, 4 ),
		G_asGeneric_GetArgInt( gen, 5 ),
		G_asGeneric_GetArgInt( gen, 6 ),
		(edict_t *)G_asGeneric_GetArgAddress( gen, 7 )
		);
}

static edict_t *asFunc_FireBlast( asvec3_t *origin, asvec3_t *angles, int speed, int radius, int damage, int knockback, int stun, edict_t *owner )
{
	return W_Fire_GunbladeBlast( owner, origin->v, angles->v, damage, min( 1, knockback ), knockback, stun, min( 1, damage ), radius, speed, 5000, MOD_SPLASH, 0 );
}

static void asFunc_asGeneric_FireBlast( void *gen )
{
	G_asGeneric_SetReturnAddress( gen,
		asFunc_FireBlast(
		(asvec3_t *)G_asGeneric_GetArgAddress( gen, 0 ),
		(asvec3_t *)G_asGeneric_GetArgAddress( gen, 1 ),
		G_asGeneric_GetArgInt( gen, 2 ),
		G_asGeneric_GetArgInt( gen, 3 ),
		G_asGeneric_GetArgInt( gen, 4 ),
		G_asGeneric_GetArgInt( gen, 5 ),
		G_asGeneric_GetArgInt( gen, 6 ),
		(edict_t *)G_asGeneric_GetArgAddress( gen, 7 )
		)
		);
}


typedef struct
{
	char *declaration;
	void *pointer;
	void *pointer_asGeneric;
}asglobfuncs_t;

static asglobfuncs_t asGlobFuncs[] =
{
    // racesow
    { "cString @G_Md5( cString & )", asFunc_G_Md5, asFunc_asGeneric_G_Md5 },
    { "bool RS_MysqlPlayerAppear( cString &, int, int, int, bool, cString &, cString &, cString & )", asFunc_RS_MysqlPlayerAppear, asFunc_asGeneric_RS_MysqlPlayerAppear },
    { "bool RS_MysqlPlayerDisappear( cString &, int, int, int, int, int, int, bool, bool )", asFunc_RS_MysqlPlayerDisappear, asFunc_asGeneric_RS_MysqlPlayerDisappear },
    { "bool RS_GetPlayerNick( int, int )", asFunc_RS_GetPlayerNick, asFunc_asGeneric_RS_GetPlayerNick },
    { "bool RS_UpdatePlayerNick( cString &, int, int )", asFunc_RS_UpdatePlayerNick, asFunc_asGeneric_RS_UpdatePlayerNick },
    { "bool RS_MysqlLoadMap()", asFunc_RS_MysqlLoadMap, asFunc_asGeneric_RS_MysqlLoadMap },
    { "bool RS_MysqlInsertRace( int, int, int, int, int, int, int, cString &, bool )", asFunc_RS_MysqlInsertRace, asFunc_asGeneric_RS_MysqlInsertRace },
    { "bool RS_MysqlLoadHighscores( int, int, int, cString &, int)", asFunc_RS_MysqlLoadHighscores, asFunc_asGeneric_RS_MysqlLoadHighscores },
    { "bool RS_MysqlLoadRanking( int, int, cString & )", asFunc_RS_MysqlLoadRanking, asFunc_asGeneric_RS_MysqlLoadRanking },
    { "bool RS_MysqlSetOneliner( int, int, int, cString &)", asFunc_RS_MysqlSetOneliner, asFunc_asGeneric_RS_MysqlSetOneliner },
    { "bool RS_PopCallbackQueue( int &out, int &out, int &out, int &out, int &out, int &out, int &out, int &out )", asFunc_RS_PopCallbackQueue, asFunc_asGeneric_RS_PopCallbackQueue },
    { "bool RS_MapFilter( int, cString &, int )", asFunc_RS_MapFilter, asFunc_asGeneric_RS_MapFilter},
    { "bool RS_Maplist( int, int )", asFunc_RS_Maplist, asFunc_asGeneric_RS_Maplist},
    { "bool RS_UpdateMapList( int playerNum)", asFunc_RS_UpdateMapList, asFunc_asGeneric_RS_UpdateMapList},
    { "bool RS_LoadStats( int playerNum, cString &, cString & )", asFunc_RS_LoadStats, asFunc_asGeneric_RS_LoadStats},
    { "cString @RS_PrintQueryCallback( int )", asFunc_RS_PrintQueryCallback, asFunc_asGeneric_RS_PrintQueryCallback },
    { "cString @RS_NextMap()", asFunc_RS_NextMap, asFunc_asGeneric_RS_NextMap },
    { "cString @RS_LastMap()", asFunc_RS_LastMap, asFunc_asGeneric_RS_LastMap },
    { "void RS_LoadMapList( int )", asFunc_RS_LoadMapList, asFunc_asGeneric_RS_LoadMapList},
    { "bool RS_QueryPjState( int playerNum)", asFunc_RS_QueryPjState, asFunc_asGeneric_RS_QueryPjState},
    { "bool RS_ResetPjState( int playerNum)", asFunc_RS_ResetPjState, asFunc_asGeneric_RS_ResetPjState},
    // !racesow

	{ "cEntity @G_SpawnEntity( cString & )", asFunc_G_Spawn, asFunc_asGeneric_G_Spawn },
	{ "cString @G_SpawnTempValue( cString & )", asFunc_G_SpawnTempValue, asFunc_asGeneric_G_SpawnTempValue },
	{ "cEntity @G_GetEntity( int entNum )", asFunc_GetEntity, asFunc_asGeneric_GetEntity },
	{ "cClient @G_GetClient( int clientNum )", asFunc_GetClient, asFunc_asGeneric_GetClient },
	{ "cTeam @G_GetTeam( int team )", asFunc_GetTeamlist, asFunc_asGeneric_GetTeamlist },
	{ "cItem @G_GetItem( int tag )", asFunc_GS_FindItemByTag, asFunc_asGeneric_GS_FindItemByTag },
	{ "cItem @G_GetItemByName( cString &name )", asFunc_GS_FindItemByName, asFunc_asGeneric_GS_FindItemByName },
	{ "cItem @G_GetItemByClassname( cString &name )", asFunc_GS_FindItemByClassname, asFunc_asGeneric_GS_FindItemByClassname },
	{ "cEntity @G_FindEntityInRadius( cEntity @, cEntity @, cVec3 &, float radius )", asFunc_FindEntityInRadiusExt, asFunc_asGeneric_FindEntityInRadiusExt },
	{ "cEntity @G_FindEntityInRadius( cEntity @, cVec3 &, float radius )", asFunc_FindEntityInRadius, asFunc_asGeneric_FindEntityInRadius },
	{ "cEntity @G_FindEntityWithClassname( cEntity @, cString & )", asFunc_FindEntityWithClassname, asFunc_asGeneric_FindEntityWithClassname },
	{ "cEntity @G_FindEntityWithClassName( cEntity @, cString & )", asFunc_FindEntityWithClassname, asFunc_asGeneric_FindEntityWithClassname },

	// misc management utils
	{ "void G_RemoveAllProjectiles()", asFunc_G_Match_RemoveAllProjectiles, asFunc_asGeneric_G_Match_RemoveAllProjectiles },
	{ "void removeProjectiles( cEntity @ )", asFunc_RS_removeProjectiles, asFunc_asGeneric_RS_removeProjectiles }, //racesow
	{ "void G_RemoveDeadBodies()", asFunc_G_Match_FreeBodyQueue, asFunc_asGeneric_G_Match_FreeBodyQueue },
	{ "void G_Items_RespawnByType( uint typeMask, int item_tag, float delay )", asFunc_G_Items_RespawnByType, asFunc_asGeneric_G_Items_RespawnByType },

	// misc
	{ "void G_Print( cString &in )", asFunc_Print, asFunc_asGeneric_Print },
	{ "void G_PrintMsg( cEntity @, cString &in )", asFunc_PrintMsg, asFunc_asGeneric_PrintMsg },
	{ "void G_CenterPrintMsg( cEntity @, cString &in )", asFunc_CenterPrintMsg, asFunc_asGeneric_CenterPrintMsg },
	{ "void G_Sound( cEntity @, int channel, int soundindex, float attenuation )", asFunc_G_Sound, asFunc_asGeneric_G_Sound },
	{ "void G_PositionedSound( cVec3 &in, int channel, int soundindex, float attenuation )", asFunc_PositionedSound, asFunc_asGeneric_PositionedSound },
	{ "void G_GlobalSound( int channel, int soundindex )", asFunc_G_GlobalSound, asFunc_asGeneric_G_GlobalSound },
	{ "void G_AnnouncerSound( cClient @, int soundIndex, int team, bool queued, cClient @ )", asFunc_G_AnnouncerSound, asFunc_asGeneric_G_AnnouncerSound },
	{ "float random()", asFunc_Random, asFunc_asGeneric_Random },
	{ "float brandom( float min, float max )", asFunc_BRandom, asFunc_asGeneric_BRandom },
	{ "int G_DirToByte( cVec3 &origin )", asFunc_DirToByte, asFunc_asGeneric_DirToByte },
	{ "int G_PointContents( cVec3 &origin )", asFunc_PointContents, asFunc_asGeneric_PointContents },
	{ "bool G_InPVS( cVec3 &origin1, cVec3 &origin2 )", asFunc_InPVS, asFunc_asGeneric_InPVS },
	{ "bool G_WriteFile( cString &, cString & )", asFunc_WriteFile, asFunc_asGeneric_WriteFile },
	{ "bool G_AppendToFile( cString &, cString & )", asFunc_AppendToFile, asFunc_asGeneric_AppendToFile },
	{ "cString @G_LoadFile( cString & )", asFunc_LoadFile, asFunc_asGeneric_LoadFile },
	{ "int G_FileLength( cString & )", asFunc_FileLength, asFunc_asGeneric_FileLength },
	{ "void G_CmdExecute( cString & )", asFunc_Cmd_ExecuteText, asFunc_asGeneric_Cmd_ExecuteText },
	{ "cString @G_LocationName( cVec3 &origin )", asFunc_LocationName, asFunc_asGeneric_LocationName },
	{ "int G_LocationTag( cString & )", asFunc_LocationTag, asFunc_asGeneric_LocationTag },
	{ "cString @G_LocationName( int tag )", asFunc_LocationForTag, asFunc_asGeneric_LocationForTag },

	{ "int G_ImageIndex( cString &in )", asFunc_ImageIndex, asFunc_asGeneric_ImageIndex },
	{ "int G_SkinIndex( cString &in )", asFunc_SkinIndex, asFunc_asGeneric_SkinIndex },
	{ "int G_ModelIndex( cString &in )", asFunc_ModelIndex, asFunc_asGeneric_ModelIndex },
	{ "int G_SoundIndex( cString &in )", asFunc_SoundIndex, asFunc_asGeneric_SoundIndex },
	{ "int G_ModelIndex( cString &in, bool pure )", asFunc_ModelIndexExt, asFunc_asGeneric_ModelIndexExt },
	{ "int G_SoundIndex( cString &in, bool pure )", asFunc_SoundIndexExt, asFunc_asGeneric_SoundIndexExt },
	{ "void G_RegisterCommand( cString &in )", asFunc_RegisterCommand, asFunc_asGeneric_RegisterCommand },
	{ "void G_RegisterCallvote( cString &in, cString &in, cString &in )", asFunc_RegisterCallvote, asFunc_asGeneric_RegisterCallvote },
	{ "void G_ConfigString( int index, cString &in )", asFunc_ConfigString, asFunc_asGeneric_ConfigString },

	// projectile firing
	{ "void G_FireInstaShot( cVec3 &origin, cVec3 &angles, int range, int damage, int knockback, int stun, cEntity @owner )", asFunc_FireInstaShot, asFunc_asGeneric_FireInstaShot },
	{ "cEntity @G_FireWeakBolt( cVec3 &origin, cVec3 &angles, int speed, int damage, int knockback, int stun, cEntity @owner )", asFunc_FireWeakBolt, asFunc_asGeneric_FireWeakBolt },
	{ "void G_FireStrongBolt( cVec3 &origin, cVec3 &angles, int range, int damage, int knockback, int stun, cEntity @owner )", asFunc_FireStrongBolt, asFunc_asGeneric_FireStrongBolt },
	{ "cEntity @G_FirePlasma( cVec3 &origin, cVec3 &angles, int speed, int radius, int damage, int knockback, int stun, cEntity @owner )", asFunc_FirePlasma, asFunc_asGeneric_FirePlasma },
	{ "cEntity @G_FireRocket( cVec3 &origin, cVec3 &angles, int speed, int radius, int damage, int knockback, int stun, cEntity @owner )", asFunc_FireRocket, asFunc_asGeneric_FireRocket },
	{ "cEntity @G_FireGrenade( cVec3 &origin, cVec3 &angles, int speed, int radius, int damage, int knockback, int stun, cEntity @owner )", asFunc_FireGrenade, asFunc_asGeneric_FireGrenade },
	{ "void G_FireRiotgun( cVec3 &origin, cVec3 &angles, int range, int spread, int count, int damage, int knockback, int stun, cEntity @owner )", asFunc_FireRiotgun, asFunc_asGeneric_FireRiotgun },
	{ "void G_FireBullet( cVec3 &origin, cVec3 &angles, int range, int spread, int damage, int knockback, int stun, cEntity @owner )", asFunc_FireBullet, asFunc_asGeneric_FireBullet },
	{ "cEntity @G_FireBlast( cVec3 &origin, cVec3 &angles, int speed, int radius, int damage, int knockback, int stun, cEntity @owner )", asFunc_FireBlast, asFunc_asGeneric_FireBlast },

	{ "bool ML_FilenameExists( cString & )", asFunc_ML_FilenameExists, asFunc_asGeneric_ML_FilenameExists },
	{ "cString @ML_GetMapByNum( int num )", asFunc_ML_GetMapByNum, asFunc_asGeneric_ML_GetMapByNum },

	{ NULL, NULL }
};

typedef struct
{
	char *declaration;
	void *pointer;
} asglobproperties_t;

static asglobproperties_t asGlobProps[] =
{
	{ "const uint levelTime", &level.time },
	{ "const uint frameTime", &game.frametime },
	{ "const uint realTime", &game.realtime },
	{ "const uint serverTime", &game.serverTime }, // racesow
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
	asglobfuncs_t *func;

	for( func = asGlobFuncs; func->declaration; func++ )
	{
		if( level.gametype.asEngineIsGeneric )
			error = angelExport->asRegisterGlobalFunction( asEngineHandle, func->declaration, func->pointer_asGeneric, asCALL_GENERIC );
		else
			error = angelExport->asRegisterGlobalFunction( asEngineHandle, func->declaration, func->pointer, asCALL_CDECL );

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
	asglobproperties_t *prop;

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

void G_InitializeGameModuleSyntax( int asEngineHandle )
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


qboolean G_asExecutionErrorReport( int asEngineHandle, int asContextHandle, int error )
{
	int funcID;

	if( error == asEXECUTION_FINISHED )
		return qfalse;

	// The execution didn't finish as we had planned. Determine why.
	if( error == asEXECUTION_ABORTED )
		G_Printf( "* The script was aborted before it could finish. Probably it timed out.\n" );

	else if( error == asEXECUTION_EXCEPTION )
	{
		G_Printf( "* The script ended with an exception.\n" );

		// Write some information about the script exception
		funcID = angelExport->asGetExceptionFunction( asContextHandle );
		G_Printf( "* func: %s\n", angelExport->asGetFunctionDeclaration( asEngineHandle, SCRIPT_MODULE_NAME, funcID ) );
		G_Printf( "* modl: %s\n", SCRIPT_MODULE_NAME );
		G_Printf( "* sect: %s\n", angelExport->asGetFunctionSection( asEngineHandle, SCRIPT_MODULE_NAME, funcID ) );
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
	level.gametype.initFuncID = -1;
	level.gametype.spawnFuncID = -1;
	level.gametype.matchStateStartedFuncID = -1;
	level.gametype.matchStateFinishedFuncID = -1;
	level.gametype.thinkRulesFuncID = -1;
	level.gametype.playerRespawnFuncID = -1;
	level.gametype.scoreEventFuncID = -1;
	level.gametype.scoreboardMessageFuncID = -1;
	level.gametype.selectSpawnPointFuncID = -1;
	level.gametype.clientCommandFuncID = -1;
	level.gametype.botStatusFuncID = -1;
	level.gametype.shutdownFuncID = -1;
}

void G_asShutdownGametypeScript( void )
{
	if( level.gametype.asEngineHandle > -1 )
	{
		if( angelExport )
			angelExport->asReleaseScriptEngine( level.gametype.asEngineHandle );
	}

	G_ResetGametypeScriptData();
}

//"void GT_SpawnGametype()"
void G_asCallLevelSpawnScript( void )
{
	int error, asContextHandle;

	if( level.gametype.spawnFuncID < 0 )
		return;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.spawnFuncID );
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

	if( level.gametype.matchStateStartedFuncID < 0 )
		return;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.matchStateStartedFuncID );
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

	if( level.gametype.matchStateFinishedFuncID < 0 )
		return qtrue;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.matchStateFinishedFuncID );
	if( error < 0 )
		return qtrue;

	// Now we need to pass the parameters to the script function.
	angelExport->asSetArgDWord( asContextHandle, 0, incomingMatchState );

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();

	// Retrieve the return from the context
	result = G_asGetReturnBool( asContextHandle );

	return result;
}

//"void GT_ThinkRules( void )"
void G_asCallThinkRulesScript( void )
{
	int error, asContextHandle;

	if( level.gametype.thinkRulesFuncID < 0 )
		return;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.thinkRulesFuncID );
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

	if( level.gametype.playerRespawnFuncID < 0 )
		return;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.playerRespawnFuncID );
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

//"void GT_scoreEvent( cClient @client, cString &score_event, cString &args )"
void G_asCallScoreEventScript( gclient_t *client, const char *score_event, const char *args )
{
	int error, asContextHandle;
	asstring_t *s1, *s2;

	if( level.gametype.scoreEventFuncID < 0 )
		return;

	if( !score_event || !score_event[0] )
		return;

	if( !args )
		args = "";

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.scoreEventFuncID );
	if( error < 0 )
		return;

	// Now we need to pass the parameters to the script function.
	s1 = objectString_FactoryBuffer( score_event, strlen( score_event ) );
	s2 = objectString_FactoryBuffer( args, strlen( args ) );

	angelExport->asSetArgObject( asContextHandle, 0, client );
	angelExport->asSetArgObject( asContextHandle, 1, s1 );
	angelExport->asSetArgObject( asContextHandle, 2, s2 );

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();

	objectString_Release( s1 );
	objectString_Release( s2 );
}

//"cString @GT_ScoreboardMessage( int maxlen )"
char *G_asCallScoreboardMessage( int maxlen )
{
	asstring_t *string;
	int error, asContextHandle;

	scoreboardString[0] = 0;

	if( level.gametype.scoreboardMessageFuncID < 0 )
		return NULL;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.scoreboardMessageFuncID );
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

	if( level.gametype.selectSpawnPointFuncID < 0 )
		return SelectDeathmatchSpawnPoint( ent ); // should have a hardcoded backup

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.selectSpawnPointFuncID );
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

//"bool GT_Command( cClient @client, cString &cmdString, cString &argsString, int argc )"
qboolean G_asCallGameCommandScript( gclient_t *client, char *cmd, char *args, int argc )
{
	int error, asContextHandle;
	asstring_t *s1, *s2;

	if( level.gametype.clientCommandFuncID < 0 )
		return qfalse; // should have a hardcoded backup

	// check for having any command to parse
	if( !cmd || !cmd[0] )
		return qfalse;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.clientCommandFuncID );
	if( error < 0 )
		return qfalse;

	// Now we need to pass the parameters to the script function.
	s1 = objectString_FactoryBuffer( cmd, strlen( cmd ) );
	s2 = objectString_FactoryBuffer( args, strlen( args ) );

	angelExport->asSetArgObject( asContextHandle, 0, client );
	angelExport->asSetArgObject( asContextHandle, 1, s1 );
	angelExport->asSetArgObject( asContextHandle, 2, s2 );
	angelExport->asSetArgDWord( asContextHandle, 3, argc );

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();

	objectString_Release( s1 );
	objectString_Release( s2 );

	// Retrieve the return from the context
	return G_asGetReturnBool( asContextHandle );
}

//"bool GT_UpdateBotStatus( cEntity @ent )"
qboolean G_asCallBotStatusScript( edict_t *ent )
{
	int error, asContextHandle;

	if( level.gametype.botStatusFuncID < 0 )
		return qfalse; // should have a hardcoded backup

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.botStatusFuncID );
	if( error < 0 )
		return qfalse;

	// Now we need to pass the parameters to the script function.
	angelExport->asSetArgObject( asContextHandle, 0, ent );

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();

	// Retrieve the return from the context
	return G_asGetReturnBool( asContextHandle );
}

//"void GT_Shutdown()"
void G_asCallShutdownScript( void )
{
	int error, asContextHandle;

	if( level.gametype.shutdownFuncID < 0 || !angelExport )
		return;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.shutdownFuncID );
	if( error < 0 )
		return;

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();
}

void G_asGetEntityEventScriptFunctions( const char *classname, edict_t *ent )
{
	char fdeclstr[MAX_STRING_CHARS];

	if( !classname )
		return;

	ent->think = NULL;
	ent->touch = NULL;
	ent->use = NULL;
	ent->pain = NULL;
	ent->die = NULL;
	ent->stop = NULL;

	// _think
	Q_snprintfz( fdeclstr, sizeof( fdeclstr ), "void %s_think( cEntity @ent )", classname );
	ent->asThinkFuncID = angelExport->asGetFunctionIDByDecl( level.gametype.asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );

	// _touch
	Q_snprintfz( fdeclstr, sizeof( fdeclstr ), "void %s_touch( cEntity @ent, cEntity @other, const cVec3 @planeNormal, int surfFlags )", classname );
	ent->asTouchFuncID = angelExport->asGetFunctionIDByDecl( level.gametype.asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );

	// _use
	Q_snprintfz( fdeclstr, sizeof( fdeclstr ), "void %s_use( cEntity @ent, cEntity @other, cEntity @activator )", classname );
	ent->asUseFuncID = angelExport->asGetFunctionIDByDecl( level.gametype.asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );

	// _pain
	Q_snprintfz( fdeclstr, sizeof( fdeclstr ), "void %s_pain( cEntity @ent, cEntity @other, float kick, float damage )", classname );
	ent->asPainFuncID = angelExport->asGetFunctionIDByDecl( level.gametype.asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );

	// _die
	Q_snprintfz( fdeclstr, sizeof( fdeclstr ), "void %s_die( cEntity @ent, cEntity @inflicter, cEntity @attacker )", classname );
	ent->asDieFuncID = angelExport->asGetFunctionIDByDecl( level.gametype.asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );

	// _stop
	Q_snprintfz( fdeclstr, sizeof( fdeclstr ), "void %s_stop( cEntity @ent )", classname );
	ent->asStopFuncID = angelExport->asGetFunctionIDByDecl( level.gametype.asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
}

// map entity spawning
qboolean G_asCallMapEntitySpawnScript( const char *classname, edict_t *ent )
{
	char fdeclstr[MAX_STRING_CHARS];
	int error, asContextHandle;
	if (!angelExport) return qfalse;

	Q_snprintfz( fdeclstr, sizeof( fdeclstr ), "void %s( cEntity @ent )", classname );

	ent->asSpawnFuncID = angelExport->asGetFunctionIDByDecl( level.gametype.asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( ent->asSpawnFuncID < 0 )
		return qfalse;

	// call the spawn function
	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );
	error = angelExport->asPrepare( asContextHandle, ent->asSpawnFuncID );
	if( error < 0 )
		return qfalse;

	// Now we need to pass the parameters to the script function.
	angelExport->asSetArgObject( asContextHandle, 0, ent );

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
	{
		G_asShutdownGametypeScript();
		ent->asSpawnFuncID = -1;
		return qfalse;
	}

	// the spawn function may remove the entity
	if( ent->r.inuse )
	{
		ent->scriptSpawned = qtrue;

		// if we found a spawn function, try also to find a _think, _use and _touch functions
		// and keep their ids so they don't have to be re-found each execution
		G_asGetEntityEventScriptFunctions( classname, ent );
	}

	return qtrue;
}

//"void %s_think( cEntity @ent )"
void G_asCallMapEntityThink( edict_t *ent )
{
	int error, asContextHandle;

	if( ent->asThinkFuncID < 0 )
		return;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, ent->asThinkFuncID );
	if( error < 0 )
		return;

	// Now we need to pass the parameters to the script function.
	angelExport->asSetArgObject( asContextHandle, 0, ent );

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();
}

// "void %s_touch( cEntity @ent, cEntity @other, const cVec3 @planeNormal, int surfFlags )"
void G_asCallMapEntityTouch( edict_t *ent, edict_t *other, cplane_t *plane, int surfFlags )
{
	int error, asContextHandle;
	asvec3_t asv, *normal = NULL;

	if( ent->asTouchFuncID < 0 )
		return;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, ent->asTouchFuncID );
	if( error < 0 )
		return;

	if( plane )
	{
		normal = &asv;
		VectorCopy( plane->normal, normal->v );
		normal->asFactored = qfalse;
	}

	// Now we need to pass the parameters to the script function.
	angelExport->asSetArgObject( asContextHandle, 0, ent );
	angelExport->asSetArgObject( asContextHandle, 1, other );
	angelExport->asSetArgObject( asContextHandle, 2, normal );
	angelExport->asSetArgDWord( asContextHandle, 3, surfFlags );

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();
}

// "void %s_use( cEntity @ent, cEntity @other, cEntity @activator )"
void G_asCallMapEntityUse( edict_t *ent, edict_t *other, edict_t *activator )
{
	int error, asContextHandle;

	if( ent->asUseFuncID < 0 )
		return;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, ent->asUseFuncID );
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

	if( ent->asPainFuncID < 0 )
		return;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, ent->asPainFuncID );
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
void G_asCallMapEntityDie( edict_t *ent, edict_t *inflicter, edict_t *attacker )
{
	int error, asContextHandle;

	if( ent->asDieFuncID < 0 )
		return;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, ent->asDieFuncID );
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

	if( ent->asStopFuncID < 0 )
		return;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, ent->asStopFuncID );
	if( error < 0 )
		return;

	// Now we need to pass the parameters to the script function.
	angelExport->asSetArgObject( asContextHandle, 0, ent );

	error = angelExport->asExecute( asContextHandle );
	if( G_asExecutionErrorReport( level.gametype.asEngineHandle, asContextHandle, error ) )
		G_asShutdownGametypeScript();
}

char *G_LoadScriptSection( const char *script, int sectionNum )
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
	level.gametype.initFuncID = angelExport->asGetFunctionIDByDecl( asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( level.gametype.initFuncID < 0 )
	{
		G_Printf( "* The function '%s' was not found. Can not continue.\n", fdeclstr );
		goto releaseAll;
	}
	else
		funcCount++;

	fdeclstr = "void GT_SpawnGametype()";
	level.gametype.spawnFuncID = angelExport->asGetFunctionIDByDecl( asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( level.gametype.spawnFuncID < 0 )
	{
		if( developer->integer || sv_cheats->integer )
			G_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	fdeclstr = "void GT_MatchStateStarted()";
	level.gametype.matchStateStartedFuncID = angelExport->asGetFunctionIDByDecl( asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( level.gametype.matchStateStartedFuncID < 0 )
	{
		if( developer->integer || sv_cheats->integer )
			G_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	fdeclstr = "bool GT_MatchStateFinished( int incomingMatchState )";
	level.gametype.matchStateFinishedFuncID = angelExport->asGetFunctionIDByDecl( asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( level.gametype.matchStateFinishedFuncID < 0 )
	{
		if( developer->integer || sv_cheats->integer )
			G_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	fdeclstr = "void GT_ThinkRules()";
	level.gametype.thinkRulesFuncID = angelExport->asGetFunctionIDByDecl( asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( level.gametype.thinkRulesFuncID < 0 )
	{
		if( developer->integer || sv_cheats->integer )
			G_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	fdeclstr = "void GT_playerRespawn( cEntity @ent, int old_team, int new_team )";
	level.gametype.playerRespawnFuncID = angelExport->asGetFunctionIDByDecl( asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( level.gametype.playerRespawnFuncID < 0 )
	{
		if( developer->integer || sv_cheats->integer )
			G_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	fdeclstr = "void GT_scoreEvent( cClient @client, cString &score_event, cString &args )";
	level.gametype.scoreEventFuncID = angelExport->asGetFunctionIDByDecl( asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( level.gametype.scoreEventFuncID < 0 )
	{
		if( developer->integer || sv_cheats->integer )
			G_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	fdeclstr = "cString @GT_ScoreboardMessage( int maxlen )";
	level.gametype.scoreboardMessageFuncID = angelExport->asGetFunctionIDByDecl( asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( level.gametype.scoreboardMessageFuncID < 0 )
	{
		if( developer->integer || sv_cheats->integer )
			G_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	fdeclstr = "cEntity @GT_SelectSpawnPoint( cEntity @ent )";
	level.gametype.selectSpawnPointFuncID = angelExport->asGetFunctionIDByDecl( asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( level.gametype.selectSpawnPointFuncID < 0 )
	{
		if( developer->integer || sv_cheats->integer )
			G_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	fdeclstr = "bool GT_Command( cClient @client, cString &cmdString, cString &argsString, int argc )";
	level.gametype.clientCommandFuncID = angelExport->asGetFunctionIDByDecl( asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( level.gametype.clientCommandFuncID < 0 )
	{
		if( developer->integer || sv_cheats->integer )
			G_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	fdeclstr = "bool GT_UpdateBotStatus( cEntity @ent )";
	level.gametype.botStatusFuncID = angelExport->asGetFunctionIDByDecl( asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( level.gametype.botStatusFuncID < 0 )
	{
		if( developer->integer || sv_cheats->integer )
			G_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	fdeclstr = "void GT_Shutdown()";
	level.gametype.shutdownFuncID = angelExport->asGetFunctionIDByDecl( asEngineHandle, SCRIPT_MODULE_NAME, fdeclstr );
	if( level.gametype.shutdownFuncID < 0 )
	{
		if( developer->integer || sv_cheats->integer )
			G_Printf( "* The function '%s' was not present in the script.\n", fdeclstr );
	}
	else
		funcCount++;

	//
	// execute the GT_InitGametype function
	//

	level.gametype.asEngineHandle = asEngineHandle;

	asContextHandle = angelExport->asAdquireContext( level.gametype.asEngineHandle );

	error = angelExport->asPrepare( asContextHandle, level.gametype.initFuncID );
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
				const char * const behaviorMarks[] =
				{
					" /* = */", " /* += */", " /* -= */", " /* *= */", " /* /= */", " /* %= */",
					" /* |= */", " /* &= */", " /* ^| */",
					" /* <<= */", " /* >>= */"
				};

				const asBehavior_t *objBehavior = &cDescr->objBehaviors[j];
				if( !objBehavior->declaration )
					break;

				// ignore add/remove reference behaviors as they can not be used explicitly anyway
				if( objBehavior->behavior == asBEHAVE_ADDREF || objBehavior->behavior == asBEHAVE_RELEASE )
					continue;

				Q_snprintfz( string, sizeof( string ), "\t%s;%s\r\n", objBehavior->declaration,
				( objBehavior->behavior == asBEHAVE_FACTORY ? " /* factory */ " :
					( objBehavior->behavior < asBEHAVE_ASSIGNMENT ||  objBehavior->behavior > asBEHAVE_SRA_ASSIGN
						? "" : behaviorMarks[objBehavior->behavior-asBEHAVE_ASSIGNMENT] ) )
				);
				trap_FS_Write( string, strlen( string ), file );
			}
		}

		// global behaviors
		if( cDescr->globalBehaviors )
		{
			const char * const globalBehaviorMarks[] =
			{
				" /* + */", " /* - */", " /* * */", " /* / */", " /* % */",
				" /* == */", " /* != */", " /* < */", " /* > */", " /* <= */", " /* >= */",
				" /* | */", " /* & */", " /* ^ */", " /* << */", " /* >> */", " /* >> */"
			};

			Q_snprintfz( string, sizeof( string ), "\r\n\t/* global behaviors */\r\n" );
			trap_FS_Write( string, strlen( string ), file );

			for( j = 0; ; j++ )
			{
				const asBehavior_t *globalBehavior = &cDescr->globalBehaviors[j];
				if( !globalBehavior->declaration )
					break;

				Q_snprintfz( string, sizeof( string ), "\t%s;%s\r\n", globalBehavior->declaration,
					( globalBehavior->behavior < asBEHAVE_ADD ||  globalBehavior->behavior > asBEHAVE_BIT_SRA
						? "" : globalBehaviorMarks[globalBehavior->behavior-asBEHAVE_ADD] )
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
		asglobproperties_t *prop;

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
		asglobfuncs_t *func;

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
