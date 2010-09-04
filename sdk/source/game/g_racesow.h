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
cvar_t *rs_queryRegisterPlayer;
cvar_t *rs_queryUpdatePlayerPlaytime;
cvar_t *rs_queryUpdatePlayerRaces;
cvar_t *rs_queryUpdatePlayerMaps;
cvar_t *rs_queryGetNick;
cvar_t *rs_queryAddNick;
cvar_t *rs_queryUpdateNickPlaytime;
cvar_t *rs_queryUpdateNickRaces;
cvar_t *rs_queryUpdateNickMaps;
cvar_t *rs_queryGetMap;
cvar_t *rs_queryAddMap;
cvar_t *rs_queryUpdateMapRating;
cvar_t *rs_queryUpdateMapPlaytime;
cvar_t *rs_queryUpdateMapRaces;
cvar_t *rs_queryAddRace;
cvar_t *rs_queryGetPlayerMap;
cvar_t *rs_queryUpdatePlayerMap;
cvar_t *rs_queryUpdatePlayerMapPlaytime;
cvar_t *rs_queryGetPlayerMapHighscores;
cvar_t *rs_queryResetPlayerMapPoints;
cvar_t *rs_queryUpdatePlayerMapPoints;
cvar_t *rs_queryUpdateNickMap;
cvar_t *rs_queryUpdateNickMapPlaytime;
cvar_t *rs_querySetMapRating;

char maplist[50000];

void RS_MysqlLoadInfo( void );
qboolean RS_MysqlConnect( void );
qboolean RS_MysqlDisconnect( void );
qboolean RS_MysqlQuery( char *query );
qboolean RS_MysqlError( edict_t *ent );

void RS_StartMysqlThread( void );
void RS_EndMysqlThread( void );
void RS_CheckMysqlThreadError( void );

// Functions

// alternative version of G_SplashFrac from g_combat.c
void rs_SplashFrac( const vec3_t origin, const vec3_t mins, const vec3_t maxs, const vec3_t point, float maxradius, vec3_t pushdir, float *kickFrac, float *dmgFrac );

void RS_removeProjectiles( edict_t *owner ); //remove the projectiles by an owner


struct authenticationData {

   unsigned int playerNum;
   char *authName;
   char *authPass;
   char *authToken;
};

struct raceDataStruct {

	unsigned int player_id;
	unsigned int nick_id;
	unsigned int map_id;
	unsigned int race_time;
	unsigned int playerNum;
};

struct playerDataStruct {

	char *name;
	unsigned int player_id;
	unsigned int map_id;
	unsigned int playerNum;
	unsigned int is_authed;
	char *authName;
	char *authPass;
	char *authToken;
};

struct playtimeDataStruct {

	char *name;
	unsigned int playtime;
	unsigned int map_id;
	unsigned int player_id;
	unsigned int nick_id;
	unsigned int is_authed;
};

struct highscoresDataStruct {

	int playerNum;
	unsigned int map_id;
};

extern void RS_Init( void );
extern void RS_Shutdown( void );

extern qboolean RS_MysqlAuthenticate( unsigned int playerNum, char *authName, char *authPass );
extern void *RS_MysqlAuthenticate_Thread( void *in );

extern qboolean RS_MysqlLoadMap();
extern void *RS_MysqlLoadMap_Thread( void *in );

extern qboolean RS_MysqlInsertRace( unsigned int player_id, unsigned int nick_id, unsigned int map_id, unsigned int race_time, unsigned int playerNum );
extern void *RS_MysqlInsertRace_Thread( void *in );

extern qboolean RS_MysqlPlayerAppear( char *name, int playerNum, int player_id, int map_id, int is_authed, char* authName, char* authPass, char* authToken );
extern void *RS_MysqlPlayerAppear_Thread( void *in );

extern qboolean RS_MysqlPlayerDisappear( char *name, int playtime, int player_id, int nick_id, int map_id, int is_authed);
extern void *RS_MysqlPlayerDisappear_Thread( void *in );

extern qboolean RS_MysqlLoadHighscores( int playerNum, int map_id );
extern void *RS_MysqlLoadHighscores_Thread( void *in );

extern qboolean RS_PrintHighscoresTo( edict_t *ent, int playerNum );

extern void RS_PushCallbackQueue( int command, int arg1, int arg2, int arg3 );
extern qboolean RS_PopCallbackQueue( int *command, int *arg1, int *arg2, int *arg3 );

void rs_TimeDeltaPrestepProjectile( edict_t *projectile, int timeDelta );

char *RS_MysqlLoadMaplist( int is_freestyle );
unsigned int RS_GetNumberOfMaps();
