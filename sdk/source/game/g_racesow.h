#include "../qcommon/md5.h"

char maplist[50000];
unsigned int mapcount;
int MysqlConnected;
char previousMapName[MAX_CONFIGSTRING_CHARS];

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
	unsigned int tries;
	unsigned int duration;
	char *checkpoints;
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

struct playernickDataStruct {

	char *name;
	char *simplified;

};

struct playtimeDataStruct {

	char *name;
	unsigned int playtime;
	unsigned int map_id;
	unsigned int player_id;
	unsigned int nick_id;
	unsigned int is_authed;
	unsigned int overall_tries;
	unsigned int racing_time;
	int is_threaded;
};

struct highscoresDataStruct {

	int playerNum;
	unsigned int map_id;
	int limit;
	char *mapname;
};

struct filterDataStruct {

    char *filter;
    int playerNum;
    unsigned int page;
};

struct statsRequest_t {

    int playerNum;
    char *what;
    char *which;
};

struct maplistDataStruct {

    int playerNum;
    unsigned int page;
};

void RS_LoadCvars( void );
qboolean RS_MysqlConnect( void );
qboolean RS_MysqlDisconnect( void );
qboolean RS_MysqlQuery( char *query );
qboolean RS_MysqlError( void );
void RS_StartMysqlThread( void );
void RS_EndMysqlThread( void );
void RS_CheckMysqlThreadError( void );
void rs_SplashFrac( const vec3_t origin, const vec3_t mins, const vec3_t maxs, const vec3_t point, float maxradius, vec3_t pushdir, float *kickFrac, float *dmgFrac );
void RS_removeProjectiles( edict_t *owner ); //remove the projectiles by an owner
extern void RS_Init( void );
extern void RS_Shutdown( void );
char *RS_GenerateNewToken( int );
extern qboolean RS_MysqlLoadMap();
extern void *RS_MysqlLoadMap_Thread( void *in );
extern qboolean RS_MysqlInsertRace( unsigned int player_id, unsigned int nick_id, unsigned int map_id, unsigned int race_time, unsigned int playerNum, unsigned int tries, unsigned int duration, char *checkpoints );
extern void *RS_MysqlInsertRace_Thread( void *in );
extern qboolean RS_MysqlPlayerAppear( char *name, int playerNum, int player_id, int map_id, int is_authed, char* authName, char* authPass, char* authToken );
extern void *RS_MysqlPlayerAppear_Thread( void *in );
extern qboolean RS_MysqlPlayerDisappear( char *name, int playtime, int overall_tries, int racing_time, int player_id, int nick_id, int map_id, int is_authed, int is_threaded );
extern void *RS_MysqlPlayerDisappear_Thread( void *in );
extern qboolean RS_GetPlayerNick( int playerNum, int player_id );
void *RS_GetPlayerNick_Thread( void *in );
extern qboolean RS_UpdatePlayerNick( char *name, int playerNum, int player_id );
void *RS_UpdatePlayerNick_Thread( void *in );
extern qboolean RS_MysqlLoadMaplist( int is_freestyle );
extern qboolean RS_MysqlLoadHighscores( int playerNum, int limit, int map_id, char *mapname );
extern void *RS_MysqlLoadHighscores_Thread( void *in );
extern char *RS_PrintQueryCallback(int player_id );
extern void RS_PushCallbackQueue( int command, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7 );
extern qboolean RS_PopCallbackQueue( int *command, int *arg1, int *arg2, int *arg3, int *arg4, int *arg5, int *arg6, int *arg7 );
extern qboolean RS_LoadStats( int player_id, char *what, char *which );
extern void *RS_LoadStats_Thread( void *in );
extern qboolean RS_MapFilter( int playerNum, char *filter, unsigned int page );
extern void *RS_MapFilter_Thread( void *in );
extern qboolean RS_Maplist( int playerNum, unsigned int page );
extern void *RS_Maplist_Thread(void *in);
extern qboolean RS_MapValidate( char *mapname );
extern void RS_LoadMaplist( int is_freestyle );
extern char *RS_ChooseNextMap();
extern char *RS_GetMapByNum(int num);
void rs_TimeDeltaPrestepProjectile( edict_t *projectile, int timeDelta );
