/**
 * Enums
 */
typedef enum
{
	CS_MODMANIFEST = 0x3,
	CS_MESSAGE = 0x5,
	CS_MAPNAME = 0x6,
	CS_AUDIOTRACK = 0x7,
	CS_HOSTNAME = 0x0,
	CS_TVSERVER = 0x1,
	CS_SKYBOX = 0x8,
	CS_SCORESTATNUMS = 0x9,
	CS_POWERUPEFFECTS = 0xa,
	CS_GAMETYPETITLE = 0xb,
	CS_GAMETYPENAME = 0xc,
	CS_GAMETYPEVERSION = 0xd,
	CS_GAMETYPEAUTHOR = 0xe,
	CS_AUTORECORDSTATE = 0xf,
	CS_SCB_PLAYERTAB_LAYOUT = 0x10,
	CS_SCB_PLAYERTAB_TITLES = 0x11,
	CS_TEAM_ALPHA_NAME = 0x14,
	CS_TEAM_BETA_NAME = 0x15,
	CS_MAXCLIENTS = 0x2,
	CS_MAPCHECKSUM = 0x1f,
	CS_MATCHNAME = 0x16,
	CS_MODELS = 0x20,
	CS_SOUNDS = 0x120,
	CS_IMAGES = 0x220,
	CS_SKINFILES = 0x320,
	CS_LIGHTS = 0x420,
	CS_ITEMS = 0x520,
	CS_PLAYERINFOS = 0x560,
	CS_GAMECOMMANDS = 0x660,
	CS_LOCATIONS = 0x6a0,
	CS_GENERAL = 0x720,
} configstrings_e;

typedef enum
{
	EF_ROTATE_AND_BOB = 0x1,
	EF_SHELL = 0x2,
	EF_STRONG_WEAPON = 0x4,
	EF_QUAD = 0x8,
	EF_CARRIER = 0x10,
	EF_BUSYICON = 0x20,
	EF_FLAG_TRAIL = 0x40,
	EF_TAKEDAMAGE = 0x80,
	EF_TEAMCOLOR_TRANSITION = 0x100,
	EF_EXPIRING_QUAD = 0x200,
	EF_EXPIRING_SHELL = 0x400,
	EF_GODMODE = 0x800,
	EF_PLAYER_STUNNED = 0x1,
	EF_PLAYER_HIDENAME = 0x100,
} state_effects_e;

typedef enum
{
	MATCH_STATE_WARMUP = 0x1,
	MATCH_STATE_COUNTDOWN = 0x2,
	MATCH_STATE_PLAYTIME = 0x3,
	MATCH_STATE_POSTMATCH = 0x4,
	MATCH_STATE_WAITEXIT = 0x5,
} matchstates_e;

typedef enum
{
	SPAWNSYSTEM_INSTANT = 0x0,
	SPAWNSYSTEM_WAVES = 0x1,
	SPAWNSYSTEM_HOLD = 0x2,
} spawnsystem_e;

typedef enum
{
	STAT_PROGRESS_SELF = 0xf,
	STAT_PROGRESS_OTHER = 0x10,
	STAT_PROGRESS_ALPHA = 0x11,
	STAT_PROGRESS_BETA = 0x12,
	STAT_IMAGE_SELF = 0x13,
	STAT_IMAGE_OTHER = 0x14,
	STAT_IMAGE_ALPHA = 0x15,
	STAT_IMAGE_BETA = 0x16,
	STAT_TIME_SELF = 0x17,
	STAT_TIME_BEST = 0x18,
	STAT_TIME_RECORD = 0x19,
	STAT_TIME_ALPHA = 0x1a,
	STAT_TIME_BETA = 0x1b,
	STAT_MESSAGE_SELF = 0x1c,
	STAT_MESSAGE_OTHER = 0x1d,
	STAT_MESSAGE_ALPHA = 0x1e,
	STAT_MESSAGE_BETA = 0x1f,
} hudstats_e;

typedef enum
{
	TEAM_SPECTATOR = 0x0,
	TEAM_PLAYERS = 0x1,
	TEAM_ALPHA = 0x2,
	TEAM_BETA = 0x3,
	GS_MAX_TEAMS = 0x4,
} teams_e;

typedef enum
{
	ET_GENERIC = 0x0,
	ET_PLAYER = 0x1,
	ET_CORPSE = 0x2,
	ET_BEAM = 0x3,
	ET_PORTALSURFACE = 0x4,
	ET_PUSH_TRIGGER = 0x5,
	ET_GIB = 0x6,
	ET_BLASTER = 0x7,
	ET_ELECTRO_WEAK = 0x8,
	ET_ROCKET = 0x9,
	ET_GRENADE = 0xa,
	ET_PLASMA = 0xb,
	ET_SPRITE = 0xc,
	ET_ITEM = 0xd,
	ET_LASERBEAM = 0xe,
	ET_CURVELASERBEAM = 0xf,
	ET_FLAG_BASE = 0x10,
	ET_MINIMAP_ICON = 0x11,
	ET_DECAL = 0x12,
	ET_ITEM_TIMER = 0x13,
	ET_PARTICLES = 0x14,
	ET_EVENT = 0x60,
	ET_SOUNDEVENT = 0x61,
} entitytype_e;

typedef enum
{
	SOLID_NOT = 0x0,
	SOLID_TRIGGER = 0x1,
	SOLID_YES = 0x2,
} solid_e;

typedef enum
{
	MOVETYPE_NONE = 0x0,
	MOVETYPE_PLAYER = 0x1,
	MOVETYPE_NOCLIP = 0x2,
	MOVETYPE_PUSH = 0x3,
	MOVETYPE_STOP = 0x4,
	MOVETYPE_FLY = 0x5,
	MOVETYPE_TOSS = 0x6,
	MOVETYPE_LINEARPROJECTILE = 0x7,
	MOVETYPE_BOUNCE = 0x8,
	MOVETYPE_BOUNCEGRENADE = 0x9,
	MOVETYPE_TOSSSLIDE = 0xa,
} movetype_e;

typedef enum
{
	PMFEAT_CROUCH = 0x1,
	PMFEAT_WALK = 0x2,
	PMFEAT_JUMP = 0x4,
	PMFEAT_DASH = 0x8,
	PMFEAT_WALLJUMP = 0x10,
	PMFEAT_FWDBUNNY = 0x20,
	PMFEAT_AIRCONTROL = 0x40,
	PMFEAT_ZOOM = 0x80,
	PMFEAT_GHOSTMOVE = 0x100,
	PMFEAT_CONTINOUSJUMP = 0x200,
	PMFEAT_ITEMPICK = 0x400,
	PMFEAT_GUNBLADEAUTOATTACK = 0x800,
	PMFEAT_WEAPONSWITCH = 0x1000,
	PMFEAT_ALL = 0xffff,
	PMFEAT_DEFAULT = 0xfeff,
} pmovefeats_e;

typedef enum
{
	IT_WEAPON = 0x1,
	IT_AMMO = 0x2,
	IT_ARMOR = 0x4,
	IT_POWERUP = 0x8,
	IT_HEALTH = 0x40,
} itemtype_e;

typedef enum
{
	G_INSTAGIB_NEGATE_ITEMMASK = 0x4f,
} G_INSTAGIB_NEGATE_ITEMMASK_e;

typedef enum
{
	WEAP_NONE = 0x0,
	WEAP_GUNBLADE = 0x1,
	WEAP_MACHINEGUN = 0x2,
	WEAP_RIOTGUN = 0x3,
	WEAP_GRENADELAUNCHER = 0x4,
	WEAP_ROCKETLAUNCHER = 0x5,
	WEAP_PLASMAGUN = 0x6,
	WEAP_LASERGUN = 0x7,
	WEAP_ELECTROBOLT = 0x8,
	WEAP_INSTAGUN = 0x9,
	WEAP_TOTAL = 0xa,
} weapon_tag_e;

typedef enum
{
	AMMO_NONE = 0x0,
	AMMO_GUNBLADE = 0xa,
	AMMO_STRONG_BULLETS = 0xb,
	AMMO_SHELLS = 0xc,
	AMMO_GRENADES = 0xd,
	AMMO_ROCKETS = 0xe,
	AMMO_PLASMA = 0xf,
	AMMO_LASERS = 0x10,
	AMMO_BOLTS = 0x11,
	AMMO_INSTAS = 0x12,
	AMMO_WEAK_GUNBLADE = 0x13,
	AMMO_BULLETS = 0x14,
	AMMO_WEAK_SHELLS = 0x15,
	AMMO_WEAK_GRENADES = 0x16,
	AMMO_WEAK_ROCKETS = 0x17,
	AMMO_WEAK_PLASMA = 0x18,
	AMMO_WEAK_LASERS = 0x19,
	AMMO_WEAK_BOLTS = 0x1a,
	AMMO_WEAK_INSTAS = 0x1b,
	AMMO_TOTAL = 0x1c,
} ammo_tag_e;

typedef enum
{
	ARMOR_NONE = 0x0,
	ARMOR_GA = 0x1c,
	ARMOR_YA = 0x1d,
	ARMOR_RA = 0x1e,
	ARMOR_SHARD = 0x1f,
} armor_tag_e;

typedef enum
{
	HEALTH_NONE = 0x0,
	HEALTH_SMALL = 0x20,
	HEALTH_MEDIUM = 0x21,
	HEALTH_LARGE = 0x22,
	HEALTH_MEGA = 0x23,
	HEALTH_ULTRA = 0x24,
} health_tag_e;

typedef enum
{
	POWERUP_NONE = 0x0,
	POWERUP_QUAD = 0x25,
	POWERUP_SHELL = 0x26,
	POWERUP_TOTAL = 0x27,
} powerup_tag_e;

typedef enum
{
	AMMO_PACK_WEAK = 0x27,
	AMMO_PACK_STRONG = 0x28,
	AMMO_PACK = 0x29,
} otheritems_tag_e;

typedef enum
{
	CS_FREE = 0x0,
	CS_ZOMBIE = 0x1,
	CS_CONNECTING = 0x2,
	CS_CONNECTED = 0x3,
	CS_SPAWNED = 0x4,
} client_statest_e;

typedef enum
{
	CHAN_AUTO = 0x0,
	CHAN_PAIN = 0x1,
	CHAN_VOICE = 0x2,
	CHAN_ITEM = 0x3,
	CHAN_BODY = 0x4,
	CHAN_MUZZLEFLASH = 0x5,
	CHAN_FIXED = 0x80,
} sound_channels_e;

typedef enum
{
	CONTENTS_SOLID = 0x1,
	CONTENTS_LAVA = 0x8,
	CONTENTS_SLIME = 0x10,
	CONTENTS_WATER = 0x20,
	CONTENTS_FOG = 0x40,
	CONTENTS_AREAPORTAL = 0x8000,
	CONTENTS_PLAYERCLIP = 0x10000,
	CONTENTS_MONSTERCLIP = 0x20000,
	CONTENTS_TELEPORTER = 0x40000,
	CONTENTS_JUMPPAD = 0x80000,
	CONTENTS_CLUSTERPORTAL = 0x100000,
	CONTENTS_DONOTENTER = 0x200000,
	CONTENTS_ORIGIN = 0x1000000,
	CONTENTS_BODY = 0x2000000,
	CONTENTS_CORPSE = 0x4000000,
	CONTENTS_DETAIL = 0x8000000,
	CONTENTS_STRUCTURAL = 0x10000000,
	CONTENTS_TRANSLUCENT = 0x20000000,
	CONTENTS_TRIGGER = 0x40000000,
	CONTENTS_NODROP = 0x80000000,
	MASK_ALL = 0xffffffff,
	MASK_SOLID = 0x1,
	MASK_PLAYERSOLID = 0x2010001,
	MASK_DEADSOLID = 0x10001,
	MASK_MONSTERSOLID = 0x2020001,
	MASK_WATER = 0x38,
	MASK_OPAQUE = 0x19,
	MASK_SHOT = 0x6000001,
} contents_e;

typedef enum
{
	SURF_NODAMAGE = 0x1,
	SURF_SLICK = 0x2,
	SURF_SKY = 0x4,
	SURF_LADDER = 0x8,
	SURF_NOIMPACT = 0x10,
	SURF_NOMARKS = 0x20,
	SURF_FLESH = 0x40,
	SURF_NODRAW = 0x80,
	SURF_HINT = 0x100,
	SURF_SKIP = 0x200,
	SURF_NOLIGHTMAP = 0x400,
	SURF_POINTLIGHT = 0x800,
	SURF_METALSTEPS = 0x1000,
	SURF_NOSTEPS = 0x2000,
	SURF_NONSOLID = 0x4000,
	SURF_LIGHTFILTER = 0x8000,
	SURF_ALPHASHADOW = 0x10000,
	SURF_NODLIGHT = 0x20000,
	SURF_DUST = 0x40000,
} surfaceflags_e;

typedef enum
{
	SVF_NOCLIENT = 0x1,
	SVF_PORTAL = 0x2,
	SVF_NOORIGIN2 = 0x4,
	SVF_TRANSMITORIGIN2 = 0x8,
	SVF_SOUNDCULL = 0x10,
	SVF_FAKECLIENT = 0x20,
	SVF_BROADCAST = 0x40,
	SVF_CORPSE = 0x80,
	SVF_PROJECTILE = 0x100,
	SVF_ONLYTEAM = 0x200,
	SVF_FORCEOWNER = 0x400,
	SVF_NOCULLATORIGIN2 = 0x4,
} serverflags_e;

typedef enum
{
	CVAR_ARCHIVE = 0x1,
	CVAR_USERINFO = 0x2,
	CVAR_SERVERINFO = 0x4,
	CVAR_NOSET = 0x8,
	CVAR_LATCH = 0x10,
	CVAR_LATCH_VIDEO = 0x20,
	CVAR_LATCH_SOUND = 0x40,
	CVAR_CHEAT = 0x80,
	CVAR_READONLY = 0x100,
} cvarflags_e;

typedef enum
{
	MOD_GUNBLADE_W = 0x24,
	MOD_GUNBLADE_S = 0x25,
	MOD_MACHINEGUN_W = 0x26,
	MOD_MACHINEGUN_S = 0x27,
	MOD_RIOTGUN_W = 0x28,
	MOD_RIOTGUN_S = 0x29,
	MOD_GRENADE_W = 0x2a,
	MOD_GRENADE_S = 0x2b,
	MOD_ROCKET_W = 0x2c,
	MOD_ROCKET_S = 0x2d,
	MOD_PLASMA_W = 0x2e,
	MOD_PLASMA_S = 0x2f,
	MOD_ELECTROBOLT_W = 0x30,
	MOD_ELECTROBOLT_S = 0x31,
	MOD_INSTAGUN_W = 0x32,
	MOD_INSTAGUN_S = 0x33,
	MOD_LASERGUN_W = 0x34,
	MOD_LASERGUN_S = 0x35,
	MOD_GRENADE_SPLASH_W = 0x36,
	MOD_GRENADE_SPLASH_S = 0x37,
	MOD_ROCKET_SPLASH_W = 0x38,
	MOD_ROCKET_SPLASH_S = 0x39,
	MOD_PLASMA_SPLASH_W = 0x3a,
	MOD_PLASMA_SPLASH_S = 0x3b,
	MOD_WATER = 0x3c,
	MOD_SLIME = 0x3d,
	MOD_LAVA = 0x3e,
	MOD_CRUSH = 0x3f,
	MOD_TELEFRAG = 0x40,
	MOD_FALLING = 0x41,
	MOD_SUICIDE = 0x42,
	MOD_EXPLOSIVE = 0x43,
	MOD_BARREL = 0x44,
	MOD_BOMB = 0x45,
	MOD_EXIT = 0x46,
	MOD_SPLASH = 0x47,
	MOD_TARGET_LASER = 0x48,
	MOD_TRIGGER_HURT = 0x49,
	MOD_HIT = 0x4a,
} meaningsofdeath_e;

typedef enum
{
	DAMAGE_NO = 0x0,
	DAMAGE_YES = 0x1,
	DAMAGE_AIM = 0x2,
} takedamage_e;

typedef enum
{
} miscelanea_e;

/**
 * Global properties
 */
const uint levelTime;
const uint frameTime;
const uint realTime;
const uint64 localTime;
const int maxEntities;
const int numEntities;
const int maxClients;
cGametypeDesc gametype;
cMatch match;
int mysqlConnected;

/**
 * Global functions
 */
cString @G_Md5( cString & );
bool RS_MysqlPlayerAppear( cString &, int, int, int, bool, cString &, cString &, cString & );
bool RS_MysqlPlayerDisappear( cString &, int, int, int, int, int, int, bool, bool );
bool RS_GetPlayerNick( int, int );
bool RS_UpdatePlayerNick( cString &, int, int );
bool RS_MysqlLoadMap();
bool RS_MysqlInsertRace( int, int, int, int, int, int, int, cString & );
bool RS_MysqlLoadHighscores( int, int, int, cString &);
bool RS_MysqlSetOneliner( int, int, int, cString &);
bool RS_PopCallbackQueue( int &out, int &out, int &out, int &out, int &out, int &out, int &out, int &out );
bool RS_MapFilter( int, cString &, int );
bool RS_Maplist( int, int );
bool RS_LoadStats( int playerNum, cString &, cString & );
cString @RS_PrintQueryCallback( int );
cString @RS_NextMap();
cString @RS_LastMap();
void RS_LoadMapList( int );
cEntity @G_SpawnEntity( cString & );
cString @G_SpawnTempValue( cString & );
cEntity @G_GetEntity( int entNum );
cClient @G_GetClient( int clientNum );
cTeam @G_GetTeam( int team );
cItem @G_GetItem( int tag );
cItem @G_GetItemByName( cString &name );
cItem @G_GetItemByClassname( cString &name );
cEntity @G_FindEntityInRadius( cEntity @, cEntity @, cVec3 &, float radius );
cEntity @G_FindEntityInRadius( cEntity @, cVec3 &, float radius );
cEntity @G_FindEntityWithClassname( cEntity @, cString & );
cEntity @G_FindEntityWithClassName( cEntity @, cString & );
void G_RemoveAllProjectiles();
void removeProjectiles( cEntity @ );
void G_RemoveDeadBodies();
void G_Items_RespawnByType( uint typeMask, int item_tag, float delay );
void G_Print( cString &in );
void G_PrintMsg( cEntity @, cString &in );
void G_CenterPrintMsg( cEntity @, cString &in );
void G_Sound( cEntity @, int channel, int soundindex, float attenuation );
void G_PositionedSound( cVec3 &in, int channel, int soundindex, float attenuation );
void G_GlobalSound( int channel, int soundindex );
void G_AnnouncerSound( cClient @, int soundIndex, int team, bool queued, cClient @ );
float random();
float brandom( float min, float max );
int G_DirToByte( cVec3 &origin );
int G_PointContents( cVec3 &origin );
bool G_InPVS( cVec3 &origin1, cVec3 &origin2 );
bool G_WriteFile( cString &, cString & );
bool G_AppendToFile( cString &, cString & );
cString @G_LoadFile( cString & );
int G_FileLength( cString & );
void G_CmdExecute( cString & );
cString @G_LocationName( cVec3 &origin );
int G_LocationTag( cString & );
cString @G_LocationName( int tag );
int G_ImageIndex( cString &in );
int G_SkinIndex( cString &in );
int G_ModelIndex( cString &in );
int G_SoundIndex( cString &in );
int G_ModelIndex( cString &in, bool pure );
int G_SoundIndex( cString &in, bool pure );
void G_RegisterCommand( cString &in );
void G_RegisterCallvote( cString &in, cString &in, cString &in );
void G_ConfigString( int index, cString &in );
void G_FireInstaShot( cVec3 &origin, cVec3 &angles, int range, int damage, int knockback, int stun, cEntity @owner );
cEntity @G_FireWeakBolt( cVec3 &origin, cVec3 &angles, int speed, int damage, int knockback, int stun, cEntity @owner );
void G_FireStrongBolt( cVec3 &origin, cVec3 &angles, int range, int damage, int knockback, int stun, cEntity @owner );
cEntity @G_FirePlasma( cVec3 &origin, cVec3 &angles, int speed, int radius, int damage, int knockback, int stun, cEntity @owner );
cEntity @G_FireRocket( cVec3 &origin, cVec3 &angles, int speed, int radius, int damage, int knockback, int stun, cEntity @owner );
cEntity @G_FireGrenade( cVec3 &origin, cVec3 &angles, int speed, int radius, int damage, int knockback, int stun, cEntity @owner );
void G_FireRiotgun( cVec3 &origin, cVec3 &angles, int range, int spread, int count, int damage, int knockback, int stun, cEntity @owner );
void G_FireBullet( cVec3 &origin, cVec3 &angles, int range, int spread, int damage, int knockback, int stun, cEntity @owner );
cEntity @G_FireBlast( cVec3 &origin, cVec3 &angles, int speed, int radius, int damage, int knockback, int stun, cEntity @owner );
bool ML_FilenameExists( cString & );
cString @ML_GetMapByNum( int num );

