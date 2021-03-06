#include "../qcommon/md5.h"
#include "mosquitto.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#else
#include <process.h>
#include <winsock2.h>
#define snprintf sprintf_s
#endif

#define MSGMODE_NONE 0
#define MSGMODE_CMD 1
#define MSGMODE_STDIN_LINE 2
#define MSGMODE_STDIN_FILE 3
#define MSGMODE_FILE 4
#define MSGMODE_NULL 5

void mqtt_connect_callback(void *obj, int result);
void mqtt_disconnect_callback(void *obj);
void mqtt_publish_callback(void *obj, uint16_t mid);

char maplist[50000];
unsigned int mapcount;
int MysqlConnected;
char previousMapName[MAX_CONFIGSTRING_CHARS];
int ircConnected;

typedef enum{
    RS_PREJUMPED,
    RS_NOTPREJUMPED,
    RS_BOTH
} pjflag;

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
	qboolean prejumped;
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
	pjflag prejumpflag;
};

struct rankingDataStruct {

	int playerNum;
	unsigned int page;
	char *order;
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

struct onelinerDataStruct {

	int playerNum;
	int player_id;
	int map_id;
	char *oneliner;
};

qboolean RS_LoadCvars( void );
qboolean RS_MysqlConnect( void );
qboolean RS_MysqlDisconnect( void );
qboolean RS_MysqlQuery( char *query );
qboolean RS_MysqlError( void );
void RS_EscapeString( char* string );
void RS_StartMysqlThread( void );
void RS_EndMysqlThread( void );
void rs_SplashFrac( const vec3_t origin, const vec3_t mins, const vec3_t maxs, const vec3_t point, float maxradius, vec3_t pushdir, float *kickFrac, float *dmgFrac );
void RS_removeProjectiles( edict_t *owner ); //remove the projectiles by an owner
void RS_Init( void );
void RS_Shutdown( void );
//char *RS_GenerateNewToken( int );
qboolean RS_MysqlLoadMap();
void *RS_MysqlLoadMap_Thread( void *in );
qboolean RS_MysqlInsertRace( unsigned int player_id, unsigned int nick_id, unsigned int map_id, unsigned int race_time, unsigned int playerNum, unsigned int tries, unsigned int duration, char *checkpoints, qboolean prejumped );
void *RS_MysqlInsertRace_Thread( void *in );
qboolean RS_MysqlPlayerAppear( char *name, int playerNum, int player_id, int map_id, int is_authed, char* authName, char* authPass, char* authToken );
void *RS_MysqlPlayerAppear_Thread( void *in );
qboolean RS_MysqlPlayerDisappear( char *name, int playtime, int overall_tries, int racing_time, int player_id, int nick_id, int map_id, int is_authed, int is_threaded );
void *RS_MysqlPlayerDisappear_Thread( void *in );
qboolean RS_GetPlayerNick( int playerNum, int player_id );
void *RS_GetPlayerNick_Thread( void *in );
qboolean RS_UpdatePlayerNick( char *name, int playerNum, int player_id );
void *RS_UpdatePlayerNick_Thread( void *in );
qboolean RS_MysqlLoadMaplist( int is_freestyle );
qboolean RS_MysqlLoadHighscores( int playerNum, int limit, int map_id, char *mapname, pjflag prejumpflag );
void *RS_MysqlLoadHighscores_Thread( void *in );
qboolean RS_MysqlLoadRanking( int playerNum, int page, char *order );
void *RS_MysqlLoadRanking_Thread( void *in );
qboolean RS_MysqlSetOneliner( int playerNum, int player_id, int map_id, char *oneliner);
void *RS_MysqlSetOneliner_Thread( void *in );
char *RS_PrintQueryCallback(int player_id );
void RS_PushCallbackQueue( int command, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7 );
qboolean RS_PopCallbackQueue( int *command, int *arg1, int *arg2, int *arg3, int *arg4, int *arg5, int *arg6, int *arg7 );
qboolean RS_LoadStats( int player_id, char *what, char *which );
void *RS_LoadStats_Thread( void *in );
qboolean RS_MapFilter( int playerNum, char *filter, unsigned int page );
void *RS_MapFilter_Thread( void *in );
qboolean RS_Maplist( int playerNum, unsigned int page );
void *RS_Maplist_Thread(void *in);
qboolean RS_MapValidate( char *mapname );
void RS_LoadMaplist( int is_freestyle );
char *RS_ChooseNextMap();
char *RS_GetMapByNum(int num);
void rs_TimeDeltaPrestepProjectile( edict_t *projectile, int timeDelta );
void RS_ircSendMessage( const char *name, const char *text );
void RS_AddServerCommands( void );
void RS_RemoveServerCommands( void );
static void RS_Irc_ConnectedListener_f( void *connected );
void RS_VoteMapExtraHelp( edict_t *ent );
qboolean RS_UpdateMapList(int playerNum);
void *RS_UpdateMapList_Thread(void *in);
//char *RS_StartPlayerSession(int playerId);
