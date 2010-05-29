/**
 * MySQL CVARs
 * @var cvar_t*
 */
cvar_t *rs_mysql_host;
cvar_t *rs_mysql_port;
cvar_t *rs_mysql_user;
cvar_t *rs_mysql_pass;
cvar_t *rs_mysql_db;

cvar_t *rs_queryGetPlayer;
cvar_t *rs_queryAddPlayer;
cvar_t *rs_queryAddNick;
cvar_t *rs_queryGetMap;
cvar_t *rs_queryAddMap;
cvar_t *rs_queryAddRace;

void RS_MysqlLoadInfo( void );
qboolean RS_MysqlConnect( void );
qboolean RS_MysqlDisconnect( void );
qboolean RS_MysqlQuery( char *query );
qboolean RS_MysqlError( edict_t *ent );

void RS_EndMysqlThread( void );
void RS_CheckMysqlThreadError( void );

// Functions

// alternative version of G_SplashFrac from g_combat.c
void rs_SplashFrac( const vec3_t origin, const vec3_t mins, const vec3_t maxs, const vec3_t point, float maxradius, vec3_t pushdir, float *kickFrac, float *dmgFrac );

// Triggers
void RS_UseShooter( edict_t *self, edict_t *other, edict_t *activator );
void RS_InitShooter( edict_t *self, int weapon );
void RS_InitShooter_Finish( edict_t *self );
void RS_shooter_rocket( edict_t *self );
void RS_shooter_plasma( edict_t *self );
void RS_shooter_grenade( edict_t *self );

// target_delay
void RS_target_delay( edict_t *self );
void RS_target_relay (edict_t *self);

void RS_removeProjectiles( edict_t *owner ); //remove the projectiles by an owner


struct authenticationData {

   edict_t *ent;
   char *authName;
   char *authPass;
};

struct raceDataStruct {

   int player_id;
   int nick_id;
   int map_id;
   int race_time;
};

extern void RS_Init( void );
extern void RS_Shutdown( void );
extern qboolean RS_MysqlAuthenticate( edict_t *ent, char *authName, char *authPass );
extern void *RS_MysqlAuthenticate_Thread( void *in );
extern void RS_MysqlAuthenticate_Callback( edict_t *ent, int playerId, int authMask );
extern qboolean RS_MysqlInsertRace( int player_id, int nick_id, int map_id, int race_time );
extern void *RS_MysqlInsertRace_Thread(void *in);
