#include "g_local.h"
#ifdef WIN32 
#include "pthread_win32\pthread.h"
#include <winsock.h>
#else
#include <pthread.h>
#endif
#include <mysql.h>


/**
 * MySQL CVARs
 */
cvar_t *rs_mysqlHost;
cvar_t *rs_mysqlPort;
cvar_t *rs_mysqlUser;
cvar_t *rs_mysqlPass;
cvar_t *rs_mysqlDb;

cvar_t *rs_queryGetPlayerAuth;
cvar_t *rs_queryGetPlayerAuthByToken;
cvar_t *rs_queryGetPlayer;
cvar_t *rs_queryAddPlayer;
cvar_t *rs_queryRegisterPlayer;
cvar_t *rs_queryUpdatePlayerPlaytime;
cvar_t *rs_queryUpdatePlayerRaces;
cvar_t *rs_queryUpdatePlayerMaps;
cvar_t *rs_queryGetMap;
cvar_t *rs_queryAddMap;
cvar_t *rs_queryUpdateMapRating;
cvar_t *rs_queryUpdateMapPlaytime;
cvar_t *rs_queryUpdateMapRaces;
cvar_t *rs_queryAddRace;
cvar_t *rs_queryGetPlayerMap;
cvar_t *rs_queryUpdatePlayerMap;
cvar_t *rs_queryUpdatePlayerMapPlaytime;
cvar_t *rs_queryGetPlayerMapHighscore;
cvar_t *rs_queryGetPlayerMapHighscores;
cvar_t *rs_queryResetPlayerMapPoints;
cvar_t *rs_queryUpdatePlayerMapPoints;
cvar_t *rs_querySetMapRating;
cvar_t *rs_queryLoadMapList;
cvar_t *rs_queryLoadMapHighscores;
cvar_t *rs_queryMapFilter;
cvar_t *rs_queryMapFilterCount;

cvar_t *rs_authField_Name;
cvar_t *rs_authField_Pass;
cvar_t *rs_authField_Token;
cvar_t *rs_tokenSalt;

/**
 * as callback commands (must be similar to callback.as!)
 */
const unsigned int RACESOW_CALLBACK_RACE = 1;
const unsigned int RACESOW_CALLBACK_LOADMAP = 2;
const unsigned int RACESOW_CALLBACK_HIGHSCORES = 3;
const unsigned int RACESOW_CALLBACK_APPEAR = 4;
const unsigned int RACESOW_CALLBACK_MAPFILTER = 6;

/**
 * MySQL Socket
 */
static MYSQL mysql;

/**
 * Attributes given to eachs thread
 */
pthread_attr_t threadAttr;

/**
 * handler for thread synchronization
 */
pthread_mutex_t mutexsum;
pthread_mutex_t mutex_callback;

/**
 * Initializes racesow specific stuff
 *
 * @return void
 */
void RS_Init()
{
	// initialize threading
    pthread_mutex_init(&mutexsum, NULL);
	pthread_mutex_init(&mutex_callback, NULL);
	pthread_attr_init(&threadAttr);
	pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);
    
    // initialize mysql
    RS_MysqlLoadInfo();
    RS_MysqlConnect();
}

/**
 * RS_MysqlLoadInfo
 *
 * @return void
 */
void RS_MysqlLoadInfo( void )
{
    rs_mysqlHost = trap_Cvar_Get( "rs_mysqlHost", "localhost", CVAR_ARCHIVE );
    rs_mysqlPort = trap_Cvar_Get( "rs_mysqlPort", "0", CVAR_ARCHIVE ); // if 0 it will use the system's default port
    rs_mysqlUser = trap_Cvar_Get( "rs_mysqlUser", "root", CVAR_ARCHIVE );
    rs_mysqlPass = trap_Cvar_Get( "rs_mysqlPass", "", CVAR_ARCHIVE );
    rs_mysqlDb = trap_Cvar_Get( "rs_mysqlDb", "racesow", CVAR_ARCHIVE );
    
    rs_authField_Name = trap_Cvar_Get( "rs_authField_Name", "", CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_NOSET);
    rs_authField_Pass = trap_Cvar_Get( "rs_authField_Pass", "", CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_NOSET);
    rs_authField_Token = trap_Cvar_Get( "rs_authField_Token", "", CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_NOSET);
    rs_tokenSalt = trap_Cvar_Get( "rs_tokenSalt", "", CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_NOSET);
    
    if (!Q_stricmp( rs_authField_Name->string, "" ) || !Q_stricmp( rs_authField_Pass->string, "" ) ||!Q_stricmp( rs_authField_Token->string, "" ))
    {
        G_Error("\033[31;40m\nthe cVars rs_authField_Name, rs_authField_Pass and rs_authField_Token must be set and should be unique for your database!\n\nie. racenet uses the following settings:\n\tset rs_authField_Name \"racenet_user\"\n\tset rs_authField_Pass \"racenet_pass\"\n\tset rs_authField_Token \"racenet_token\"\n\nlike that you can store authentications for multiple servers in a single config.\n\033[0m\n");
    }
    
    G_Printf("-------------------------------------\nClient authentication via userinfo:\nsetu %s \"username\"\nsetu %s \"password\"\nor the more secure method using an enrypted token\nsetu %s \"token\"\n-------------------------------------\n", rs_authField_Name->string, rs_authField_Pass->string, rs_authField_Token->string);
    
    rs_queryGetPlayerAuth			= trap_Cvar_Get( "rs_queryGetPlayerAuth",			"SELECT `id`, `auth_mask`, `auth_token` FROM `player` WHERE `auth_name` = '%s' AND `auth_pass` = MD5('%s%s') LIMIT 1;", CVAR_ARCHIVE );
    rs_queryGetPlayerAuthByToken    = trap_Cvar_Get( "rs_queryGetPlayerAuthByToken",	"SELECT `id`, `auth_mask` FROM `player` WHERE `auth_token` = MD5('%s%s') LIMIT 1;", CVAR_ARCHIVE );
	rs_queryGetPlayer				= trap_Cvar_Get( "rs_queryGetPlayer",			    "SELECT `id`, `auth_mask` FROM `player` WHERE `simplified` = '%s' LIMIT 1;", CVAR_ARCHIVE );
	rs_queryAddPlayer				= trap_Cvar_Get( "rs_queryAddPlayer",				"INSERT INTO `player` (`name`, `simplified`, `created`) VALUES ('%s', '%s', NOW());", CVAR_ARCHIVE );
	rs_queryRegisterPlayer			= trap_Cvar_Get( "rs_queryRegisterPlayer",			"UPDATE `player` SET `auth_name` = '%s', `auth_email` = '%s', `auth_pass` = MD5('%s%s'), `auth_mask` = 1, `auth_token` = MD5('%s%s') WHERE `simplified` = '%s' AND (`auth_mask` = 0 OR `auth_mask` IS NULL) LIMIT 1;", CVAR_ARCHIVE );
	rs_queryUpdatePlayerPlaytime	= trap_Cvar_Get( "rs_queryUpdatePlayerPlaytime",	"UPDATE `player` SET `playtime` = playtime + %d WHERE `id` = %d LIMIT 1;", CVAR_ARCHIVE );
	rs_queryUpdatePlayerRaces		= trap_Cvar_Get( "rs_queryUpdatePlayerRaces",		"UPDATE `player` SET `races` = races + 1 WHERE `id` = %d LIMIT 1;", CVAR_ARCHIVE );
	rs_queryUpdatePlayerMaps		= trap_Cvar_Get( "rs_queryUpdatePlayerMaps",		"UPDATE `player` SET `maps` = (SELECT COUNT(`map_id`) FROM `player_map` WHERE `player_id` = `player`.`id`) WHERE `id` = %d LIMIT 1;", CVAR_ARCHIVE );

	rs_queryGetMap					= trap_Cvar_Get( "rs_queryGetMapId",				"SELECT `id` FROM `map` WHERE `name` = '%s' LIMIT 1;", CVAR_ARCHIVE );
	rs_queryAddMap					= trap_Cvar_Get( "rs_queryAddMap",					"INSERT INTO `map` (`name`, `created`) VALUES('%s', NOW());", CVAR_ARCHIVE );
	rs_queryUpdateMapRating			= trap_Cvar_Get( "rs_queryUpdateMapRating",			"UPDATE `map` SET `rating` = (SELECT SUM(`value`) / COUNT(`player_id`) FROM `map_rating` WHERE `map_id` = `map`.`id`), `ratings` = (SELECT COUNT(`player_id`) FROM `map_rating` WHEHRE `map_id` = `map`.`id`) WHERE `id` = %d LIMIT 1;", CVAR_ARCHIVE );
	rs_queryUpdateMapPlaytime		= trap_Cvar_Get( "rs_queryUpdateMapPlaytime",		"UPDATE `map` SET `playtime` = `playtime` + %d WHERE `id` = %d LIMIT 1;", CVAR_ARCHIVE );
    rs_queryUpdateMapRaces			= trap_Cvar_Get( "rs_queryUpdateMapRaces",			"UPDATE `map` SET `races` = `races` + 1 WHERE `id` = %d LIMIT 1;", CVAR_ARCHIVE );

	rs_queryAddRace					= trap_Cvar_Get( "rs_queryAddRace",					"INSERT INTO `race` (`player_id`, `map_id`, `time`, `created`) VALUES(%d, %d, %d, NOW());", CVAR_ARCHIVE );
	
	rs_queryGetPlayerMap			= trap_Cvar_Get( "rs_queryGetPlayerMap",			"SELECT `points`, `time`, `races`, `playtime`, `created` FROM `player_map` WHERE `player_id` = %d AND `map_id` = %d LIMIT 1;", CVAR_ARCHIVE );
	rs_queryUpdatePlayerMap			= trap_Cvar_Get( "rs_queryUpdatePlayerMap",			"INSERT INTO `player_map` (`player_id`, `map_id`, `time`, `races`, `created`) VALUES(%d, %d, %d, 1, NOW()) ON DUPLICATE KEY UPDATE `races` = `races` + 1, `created` = IF( VALUES(`time`) < `time` OR `time` = 0 OR `time` IS NULL, NOW(), `created`), `time` = IF( VALUES(`time`) < `time` OR `time` = 0 OR `time` IS NULL, VALUES(`time`), `time` )", CVAR_ARCHIVE );
	rs_queryUpdatePlayerMapPlaytime	= trap_Cvar_Get( "rs_queryUpdatePlayerMapPlaytime",	"INSERT INTO `player_map` (`player_id`, `map_id`, `playtime`) VALUES(%d, %d, %d) ON DUPLICATE KEY UPDATE `playtime` = `playtime` + VALUES(`playtime`);", CVAR_ARCHIVE );
	rs_queryGetPlayerMapHighscore	= trap_Cvar_Get( "rs_queryGetPlayerMapHighScore",	"SELECT `p`.`id`, `pm`.`time`, `p`.`name`, `pm`.`races`, `pm`.`playtime`, `pm`.`created` FROM `player_map` `pm` INNER JOIN `player` `p` ON `p`.`id` = `pm`.`player_id` WHERE `pm`.`map_id` = %d AND `pm`.`player_id` = %d LIMIT 1;", CVAR_ARCHIVE );
    rs_queryGetPlayerMapHighscores	= trap_Cvar_Get( "rs_queryGetPlayerMapHighScores",	"SELECT `p`.`id`, `pm`.`time`, `p`.`name`, `pm`.`races`, `pm`.`playtime`, `pm`.`created` FROM `player_map` `pm` INNER JOIN `player` `p` ON `p`.`id` = `pm`.`player_id` WHERE `pm`.`time` IS NOT NULL AND `pm`.`time` > 0 AND `pm`.`map_id` = %d ORDER BY `pm`.`time` ASC;", CVAR_ARCHIVE );
    rs_queryResetPlayerMapPoints	= trap_Cvar_Get( "rs_queryResetPlayerMapPoints",	"UPDATE `player_map` SET `points` = 0 WHERE `map_id` = %d;", CVAR_ARCHIVE );
    rs_queryUpdatePlayerMapPoints	= trap_Cvar_Get( "rs_queryUpdatePlayerMapPoints",	"UPDATE `player_map` SET `points` = %d WHERE `map_id` = %d AND `player_id` = %d LIMIT 1;", CVAR_ARCHIVE );
    
	rs_querySetMapRating			= trap_Cvar_Get( "rs_querySetMapRating",			"INSERT INTO `map_rating` (`player_id`, `map_id`, `value`, `created`) VALUES(%d, %d, %d, NOW()) ON DUPLICATE KEY UPDATE `value` = VALUE(`value`), `changed` = NOW();", CVAR_ARCHIVE );
	rs_queryLoadMapList				= trap_Cvar_Get( "rs_queryLoadMapList",				"SELECT name FROM map WHERE freestyle = '%s' AND status = 'enabled' ORDER BY %s;", CVAR_ARCHIVE);
	rs_queryMapFilter               = trap_Cvar_Get( "rs_queryMapFilter",               "SELECT id, name FROM map WHERE name LIKE '%%%s%%' LIMIT %u, %u;", CVAR_ARCHIVE );
	rs_queryMapFilterCount          = trap_Cvar_Get( "rs_queryMapFilterCount",          "SELECT COUNT(id)FROM map WHERE name LIKE '%%%s%%';", CVAR_ARCHIVE );
	// TODO: I think this one can be replaced by a query to player_map similar to rs_queryGetPlayerMapHighscores, or maybe even remove it and use the latter to display highscores (because querying the big "race" table is probably expensive)
	rs_queryLoadMapHighscores		= trap_Cvar_Get( "rs_queryLoadMapHighscores",		"SELECT pm.time, p.name, pm.created FROM player_map AS pm LEFT JOIN player AS p ON p.id = pm.player_id LEFT JOIN map AS m ON m.id = pm.map_id WHERE pm.time IS NOT NULL AND pm.time > 0 AND m.id = %d ORDER BY time ASC LIMIT %d", CVAR_ARCHIVE );
    
}

/**
 * RS_MysqlConnect
 *
 * @return qboolean
 */
qboolean RS_MysqlConnect( void )
{
    G_Printf( va( "-------------------------------------\nMySQL Connection Data:\nhost: %s:%d\nuser: %s\npass: ******\ndb: %s\n", rs_mysqlHost->string, rs_mysqlPort->integer, rs_mysqlUser->string, rs_mysqlDb->string ) );
    if( !Q_stricmp( rs_mysqlHost->string, "" ) || !Q_stricmp( rs_mysqlUser->string, "user" ) || !Q_stricmp( rs_mysqlDb->string, "" ) ) {
        G_Printf( "-------------------------------------\nMySQL ERROR: Connection-data incomplete or not available\n" );
        return qfalse;
    }
    mysql_init( &mysql );
    if( &mysql == NULL ) {
        RS_MysqlError(NULL);
        return qfalse;
    }
    if( !mysql_real_connect ( &mysql, rs_mysqlHost->string, rs_mysqlUser->string, rs_mysqlPass->string, rs_mysqlDb->string, rs_mysqlPort->integer, NULL, 0 ) ) {
        RS_MysqlError(NULL);
        return qfalse;
    }
    G_Printf( "-------------------------------------\nConnected to MySQL Server\n-------------------------------------\n" );
    return qtrue;
}

/**
 * RS_MysqlDisconnect
 *
 * @return qboolean
 */
qboolean RS_MysqlDisconnect( void )
{
    mysql_close( &mysql );
    return qtrue;
}

/**
 * RS_MysqlError
 *
 * @param edict_t *ent
 * @return qboolean
 */
qboolean RS_MysqlError( edict_t *ent )
{
    if( mysql_errno( &mysql ) != 0 ) {
        if( ent != NULL )
            G_PrintMsg( ent, va( "%sMySQL Error: %s\n", S_COLOR_RED, mysql_error( &mysql ) ) );
        else
            G_Printf( va( "-------------------------------------\nMySQL ERROR: %s\n", mysql_error( &mysql ) ) );
        return qtrue;
    }
    return qfalse;
}


/**
 * Shutdown racesow specific stuff
 *
 * @return void
 */
void RS_Shutdown()
{
	RS_MysqlDisconnect();

    // shutdown threading
	pthread_mutex_destroy(&mutexsum);
	pthread_mutex_destroy(&mutex_callback);
	
	// removed it because of crash in win32 implementation, also this may be not necessary at all because this isnt in a thread
	//pthread_exit(NULL);

}



/**
 * callback queue variables used for de-threadization of mysql calls 
 */
#define MAX_SIZE_CALLBACK_QUEUE 50
int callback_queue_size=0;
int callback_queue_write_index=0;
int callback_queue_read_index=0;
int callback_queue[MAX_SIZE_CALLBACK_QUEUE][7];

/**
 *
 * Add a command to the callback queue
 * 
 * @return void
 */
void RS_PushCallbackQueue( int command, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6)
{
	pthread_mutex_lock(&mutex_callback);
	if (callback_queue_size<MAX_SIZE_CALLBACK_QUEUE)
	{
		// push!
		callback_queue[callback_queue_write_index][0]=command;
		callback_queue[callback_queue_write_index][1]=arg1;
		callback_queue[callback_queue_write_index][2]=arg2;
		callback_queue[callback_queue_write_index][3]=arg3;
		callback_queue[callback_queue_write_index][4]=arg4;
		callback_queue[callback_queue_write_index][5]=arg5;
		callback_queue[callback_queue_write_index][6]=arg6;
		
		callback_queue_write_index=(callback_queue_write_index+1)%MAX_SIZE_CALLBACK_QUEUE;
		callback_queue_size++;
	}
	else
		printf("Error: callback queue overflow\n");
	pthread_mutex_unlock(&mutex_callback);
}

/**
 *
 * "Is there a callback result to execute?"
 * queried at each game frame
 * 
 * @return qboolean
 */
qboolean RS_PopCallbackQueue(int *command, int *arg1, int *arg2, int *arg3, int *arg4, int *arg5, int *arg6)
{

	// if the queue is empty, do nothing this time
	// (a push might have happened, but it doesn't matter, we'll catch it next time!)
	if (callback_queue_size==0)
		return qfalse;

	// retrieving a callback result
	pthread_mutex_lock(&mutex_callback);
	// pop!
	*command=callback_queue[callback_queue_read_index][0];
	*arg1=callback_queue[callback_queue_read_index][1];
	*arg2=callback_queue[callback_queue_read_index][2];
	*arg3=callback_queue[callback_queue_read_index][3];
	*arg4=callback_queue[callback_queue_read_index][4];
	*arg5=callback_queue[callback_queue_read_index][5];
	*arg6=callback_queue[callback_queue_read_index][6];

	callback_queue_read_index=(callback_queue_read_index+1)%MAX_SIZE_CALLBACK_QUEUE;
	callback_queue_size--;
	pthread_mutex_unlock(&mutex_callback);
	return qtrue;
}

/**
 * Calls the load-map thread
 *
 * @param char *name
 * @return void
 */
qboolean RS_MysqlLoadMap()
{
	pthread_t thread;
	int returnCode = pthread_create(&thread, &threadAttr, RS_MysqlLoadMap_Thread, NULL);
	if (returnCode) {

		printf("THREAD ERROR: return code from pthread_create() is %d\n", returnCode);
		return qfalse;
	}
	
	return qtrue;
}

/**
 * Getting map id and servbest, and returning them as a callback
 *
 * @param void *in
 * @return void
 */
void *RS_MysqlLoadMap_Thread(void *in)
{
    char query[2000];
    char name[64];
    MYSQL_ROW  row;
    MYSQL_RES  *mysql_res;
	int map_id=0;
	unsigned int bestTime=0;

	RS_StartMysqlThread();

    Q_strncpyz ( name, COM_RemoveColorTokens( level.mapname ), sizeof(name) );
    mysql_real_escape_string(&mysql, name, name, strlen(name));
    
	sprintf(query, rs_queryGetMap->string, name);
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();
    mysql_res = mysql_store_result(&mysql);
    RS_CheckMysqlThreadError();

	if ((row = mysql_fetch_row(mysql_res)) != NULL) {
        map_id=atoi(row[0]);
    } else {
    
        sprintf(query, rs_queryAddMap->string, name);
        mysql_real_query(&mysql, query, strlen(query));
        RS_CheckMysqlThreadError();
        
		map_id=(int)mysql_insert_id(&mysql);
    }
    mysql_free_result(mysql_res);
	
    // retrieve server best
	sprintf(query, rs_queryGetPlayerMapHighscores->string, map_id);
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();
    mysql_res = mysql_store_result(&mysql);
    RS_CheckMysqlThreadError();
    while ((row = mysql_fetch_row(mysql_res)) != NULL)
	{
		if (row[0] !=NULL && row[1] != NULL)
		{
			unsigned int playerId, raceTime;
            playerId = atoi(row[0]);
            raceTime = atoi(row[1]);

            if (bestTime == 0)
                bestTime = raceTime;
		}
	}
	mysql_free_result(mysql_res);

	RS_PushCallbackQueue(RACESOW_CALLBACK_LOADMAP, map_id, bestTime, 0, 0, 0, 0);

	RS_EndMysqlThread();
    
	return NULL;
}

/**
 * Insert a new race
 *
 * @param int player_id
 * @param int map_id
 * @param int race_time
 * @return qboolean
 */
qboolean RS_MysqlInsertRace( unsigned int player_id, unsigned int nick_id, unsigned int map_id, unsigned int race_time, unsigned int playerNum ) {

	int returnCode;
    struct raceDataStruct *raceData=malloc(sizeof(struct raceDataStruct));
	pthread_t thread;

    raceData->player_id = player_id;
	raceData->nick_id= nick_id;
	raceData->map_id = map_id;
	raceData->race_time = race_time;
	raceData->playerNum = playerNum;
    
	returnCode = pthread_create(&thread, &threadAttr, RS_MysqlInsertRace_Thread, (void *)raceData);

	if (returnCode) {

		printf("THREAD ERROR: return code from pthread_create() is %d\n", returnCode);
		return qfalse;
	}
	
	return qtrue;

}

/**
 * Thread to insert a new race 
 *
 * @param void *in
 * @return void
 */
void *RS_MysqlInsertRace_Thread(void *in)
{
    char query[1000];
	unsigned int maxPositions, currentPoints, currentRaceTime, newPoints, currentPosition, realPosition, offset, points, lastRaceTime, bestTime;
	struct raceDataStruct *raceData;
    MYSQL_ROW  row;
    MYSQL_RES  *mysql_res;
	
    currentRaceTime = 0;
    bestTime = 0;
    maxPositions = 30;
    offset = 0;
	newPoints = 0;
	currentPoints = 0;
	currentPosition = 0;
    lastRaceTime = 0;
	raceData =(struct raceDataStruct *)in;
    
	RS_StartMysqlThread();

    if( raceData->player_id == 0 )
    {
        // don't insert a race if the player doesn't have a player_id (ie. he's under nick protection)
		G_Printf(va("did not insert race for a null player (time %d)\n",raceData->race_time));
		free(raceData);	
		RS_EndMysqlThread();
        return NULL;
    }
    
	// insert race
	sprintf(query, rs_queryAddRace->string, raceData->player_id, raceData->map_id, raceData->race_time);
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();

	// increment player races
	sprintf(query, rs_queryUpdatePlayerRaces->string, raceData->player_id);
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();

	// increment map races
	sprintf(query, rs_queryUpdateMapRaces->string, raceData->map_id);
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();

	// insert or update player_map
	sprintf(query, rs_queryUpdatePlayerMap->string, raceData->player_id, raceData->map_id, raceData->race_time);
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();    

	// read current points and time
	sprintf(query, rs_queryGetPlayerMap->string, raceData->player_id, raceData->map_id);
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();    
	mysql_res = mysql_store_result(&mysql);
    RS_CheckMysqlThreadError();
    if ((row = mysql_fetch_row(mysql_res)) != NULL) {
		if (row[0] != NULL && row[1] != NULL)
        {
			currentPoints = atoi(row[0]);
            currentRaceTime = atoi(row[1]);
        }
    }
	mysql_free_result(mysql_res);

	// reset points in player_map
    sprintf(query, rs_queryResetPlayerMapPoints->string, raceData->map_id);
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();
    
    // update points in player_map
	sprintf(query, rs_queryGetPlayerMapHighscores->string, raceData->map_id);
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();
    mysql_res = mysql_store_result(&mysql);
    RS_CheckMysqlThreadError();
    while ((row = mysql_fetch_row(mysql_res)) != NULL)
	{
        points = 0;
		if (row[0] !=NULL && row[1] != NULL)
		{
            unsigned int playerId, raceTime;
            playerId = atoi(row[0]);
            raceTime = atoi(row[1]);
        
            if (bestTime == 0)
                bestTime = raceTime;
        
			if (raceTime == lastRaceTime)
				offset++;
			else
				offset = 0;

			currentPosition++;
            realPosition = currentPosition - offset;
            points = (maxPositions + 1) - realPosition;
			switch (realPosition)
			{
				case 1:
					points += 10;
					break;
				case 2:
					points += 5;
					break;
				case 3:
					points += 3;
					break;
			}
			
			if (playerId == raceData->player_id)
				newPoints = points;

			lastRaceTime = raceTime;
            
            // set points in player_map
            sprintf(query, rs_queryUpdatePlayerMapPoints->string, points, raceData->map_id, playerId);
            mysql_real_query(&mysql, query, strlen(query));
            RS_CheckMysqlThreadError();
		}
    }
	mysql_free_result(mysql_res);

	// should be printed in AS. TODO: another callback for that

	/*
    G_PrintMsg( NULL, va("You earned %d points.\n", (newPoints - currentPoints)) );
    */


	free(raceData);	
	RS_EndMysqlThread();
  
    return NULL;
}

/**
 * Calls the player appear thread
 *
 * @param edict_t *ent
 * @param int is_authed
 * @return void
 */
qboolean RS_MysqlPlayerAppear( char *name, int playerNum, int player_id, int map_id, int is_authed, char *authName, char *authPass, char *authToken )
{
	pthread_t thread;
	int returnCode;
	struct playerDataStruct *playerData=malloc(sizeof(struct playerDataStruct));

    playerData->name = strdup(name);
    playerData->player_id = player_id;
	playerData->map_id = map_id;
	playerData->playerNum = playerNum;
    playerData->authName = strdup(authName);
    playerData->authPass = strdup(authPass);
    playerData->authToken = strdup(authToken);

	returnCode = pthread_create(&thread, &threadAttr, RS_MysqlPlayerAppear_Thread, (void *)playerData);
	if (returnCode) {

		printf("THREAD ERROR: return code from pthread_create() is %d\n", returnCode);
		return qfalse;
	}
	
	return qtrue;
}

/**
 * Thread when player appear,
 *
 * the player just arrived, so his nick_id was not previously stored in AS
 * however, if he's authed, the player_id won't change.
 * if he's not authed, his player_id will jump to another player_id 
 *
 * @param void *in
 * @return void
 */
void *RS_MysqlPlayerAppear_Thread(void *in)
{
    char query[1024];
	char name[64];
	char simplified[64];
	char authName[64];
	char authPass[64];
	char authToken[64];
    qboolean hasUserinfo;
	MYSQL_ROW  row;
    MYSQL_RES  *mysql_res;
	unsigned int player_id, auth_mask, player_id_for_nick, auth_mask_for_nick, personalBest;
	struct playerDataStruct *playerData;

	RS_StartMysqlThread();
    
    hasUserinfo = qfalse;
	player_id = 0;
	auth_mask = 0;
	player_id_for_nick = 0;
	auth_mask_for_nick = 0;
    personalBest = 0;
    playerData = (struct playerDataStruct*)in;

    // escape strings
    Q_strncpyz ( simplified, COM_RemoveColorTokens(playerData->name), sizeof(simplified) );
    mysql_real_escape_string(&mysql, simplified, simplified, strlen(simplified));
    
	Q_strncpyz ( name, playerData->name, sizeof(name) );
    mysql_real_escape_string(&mysql, name, name, strlen(name));
    
    Q_strncpyz ( authName, playerData->authName, sizeof(authName) );
    mysql_real_escape_string(&mysql, authName, authName, strlen(authName));
    
    Q_strncpyz ( authPass, playerData->authPass, sizeof(authPass) );
    mysql_real_escape_string(&mysql, authPass, authPass, strlen(authPass));
    
    Q_strncpyz ( authToken, playerData->authToken, sizeof(authToken) );
    mysql_real_escape_string(&mysql, authToken, authToken, strlen(authToken));
    
    // try to authenticate by token
    if (Q_stricmp( authToken, "" ))
    {
        hasUserinfo = qtrue;
        
        sprintf(query, rs_queryGetPlayerAuthByToken->string, authToken, rs_tokenSalt->string);
        mysql_real_query(&mysql, query, strlen(query));
        
        RS_CheckMysqlThreadError();
        mysql_res = mysql_store_result(&mysql);
        RS_CheckMysqlThreadError();
        if ((row = mysql_fetch_row(mysql_res)) != NULL) 
        {
            if (row[0]!=NULL && row[1]!=NULL )
            {
                player_id = atoi(row[0]);
                auth_mask = atoi(row[1]);
            }
        }
        
        mysql_free_result(mysql_res);
    }
   
    // when no token is given or was invalid, try by username and password
    if (!hasUserinfo && Q_stricmp( authName, "" ) && Q_stricmp( authPass, "" ))
    {
        hasUserinfo = qtrue;
        sprintf(query, rs_queryGetPlayerAuth->string, authName, authPass, rs_tokenSalt->string);
        
        mysql_real_query(&mysql, query, strlen(query));
        RS_CheckMysqlThreadError();
        mysql_res = mysql_store_result(&mysql);
        RS_CheckMysqlThreadError();
        if ((row = mysql_fetch_row(mysql_res)) != NULL) 
        {
            if (row[0]!=NULL && row[1]!=NULL) // token may be null
            {
                player_id = atoi(row[0]);
                auth_mask = atoi(row[1]);
            }
        }
        
        mysql_free_result(mysql_res);
    }
        
    
    // try to get information about the player the nickname belongs to
    sprintf(query, rs_queryGetPlayer->string, simplified);
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();
    mysql_res = mysql_store_result(&mysql);
    RS_CheckMysqlThreadError();
    if ((row = mysql_fetch_row(mysql_res)) != NULL)
    {
        if (row[0]!=NULL && row[1]!=NULL)
        {
            player_id_for_nick = atoi(row[0]);
            auth_mask_for_nick = atoi(row[1]);
        }
    }
    // and only add a new player if the player isn't already authed
    else if (player_id == 0)
    {
        sprintf(query, rs_queryAddPlayer->string, name, simplified);
        mysql_real_query(&mysql, query, strlen(query));
        RS_CheckMysqlThreadError();  
        
        player_id_for_nick = (int)mysql_insert_id(&mysql);
    }
    mysql_free_result(mysql_res);

    
    
    /*
    if (player_id != 0)
    {
        // update the players's number of played maps
        sprintf(query, rs_queryUpdatePlayerMaps->string, player_id);
        mysql_real_query(&mysql, query, strlen(query));
        RS_CheckMysqlThreadError();
        
        // retrieve personal best
        sprintf(query, rs_queryGetPlayerMapHighscore->string, playerData->map_id, player_id);
        mysql_real_query(&mysql, query, strlen(query));
        RS_CheckMysqlThreadError();
        mysql_res = mysql_store_result(&mysql);
        RS_CheckMysqlThreadError();
        if ((row = mysql_fetch_row(mysql_res)) != NULL)
        {
            if (row[0] !=NULL && row[1] != NULL)
            {
                personalBest = atoi(row[1]);
            }
        }
        
        mysql_free_result(mysql_res);
    }
    */
    
    RS_PushCallbackQueue(RACESOW_CALLBACK_APPEAR, playerData->playerNum, player_id, auth_mask, player_id_for_nick, auth_mask_for_nick, personalBest);
    
	free(playerData->name);
	free(playerData);
	RS_EndMysqlThread();
	   
    return NULL;
}


/**
 * Calls the player disappear thread
 *
 * @param edict_t *ent
 * @param int map_id
 * @return void
 */
qboolean RS_MysqlPlayerDisappear( char *name, int playtime, int player_id, int nick_id, int map_id, int is_authed)
{
	pthread_t thread;
	int returnCode;
	struct playtimeDataStruct *playtimeData=malloc(sizeof(struct playtimeDataStruct));

    playtimeData->name=strdup(name);
    playtimeData->map_id=map_id;
	playtimeData->player_id=player_id;
	playtimeData->nick_id=nick_id;
	playtimeData->playtime=playtime;
	playtimeData->is_authed=is_authed;

	returnCode = pthread_create(&thread, &threadAttr, RS_MysqlPlayerDisappear_Thread, (void *)playtimeData);
	if (returnCode) {

		printf("THREAD ERROR: return code from pthread_create() is %d\n", returnCode);
		return qfalse;
	}
	
	return qtrue;
}

/**
 * Thread when player disappears
 *
 * @param void *in
 * @return void
 */
void *RS_MysqlPlayerDisappear_Thread(void *in)
{
    char query[1024];
	char name[64];
	char simplified[64];
	struct playtimeDataStruct *playtimeData;
	
	RS_StartMysqlThread();
    
    playtimeData = (struct playtimeDataStruct*)in;

    Q_strncpyz ( simplified, COM_RemoveColorTokens( playtimeData->name ), sizeof(simplified) );
    mysql_real_escape_string(&mysql, simplified, simplified, strlen(simplified));
	Q_strncpyz ( name, playtimeData->name, sizeof(name) );
    mysql_real_escape_string(&mysql, name, name, strlen(name));

    // increment map playtime
	sprintf(query, rs_queryUpdateMapPlaytime->string, playtimeData->playtime, playtimeData->map_id);
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();
    
    // TODO: do we need to check if it's allowed to update the player playtime?
    if (qtrue)
    {
        // increment player playtime
        sprintf(query, rs_queryUpdatePlayerPlaytime->string, playtimeData->playtime, playtimeData->player_id);
        mysql_real_query(&mysql, query, strlen(query));
        RS_CheckMysqlThreadError();
        
        // increment player map playtime
        sprintf(query, rs_queryUpdatePlayerMapPlaytime->string, playtimeData->player_id, playtimeData->map_id, playtimeData->playtime);
        mysql_real_query(&mysql, query, strlen(query));
        RS_CheckMysqlThreadError();
    }

	free(playtimeData->name);	
	free(playtimeData);	
	RS_EndMysqlThread();
    
    return NULL;
}

/**
 * Store the result of filter requests from players.
 * 256 players at max.
 */
char *filter_players[256]={0};

/**
 * Thread for filter request.
 *
 * Store the result in filter_players and push RACESOW_CALLBACK_MAPFILTER
 * to the callback queue.
 *
 * @param in The input data, cast to filterDataStruct
 * @return NULL on success
 */
void *RS_MysqlMapFilter_Thread( void *in)
{
    MYSQL_ROW  row;
    MYSQL_RES  *mysql_res;
    char query[1024];
    struct filterDataStruct *filterData = (struct filterDataStruct *)in;
    char result[10000];
    const int MAPS_PER_PAGE = 10;
    int count = 0;
    int totalPages = 0;
    int page = filterData->page;
    int start = (page-1)*MAPS_PER_PAGE;
	unsigned int size = strlen(result)+1;

	result[0]='\0';

    //first count the total number of matching maps
    mysql_real_escape_string(&mysql, filterData->filter, filterData->filter, strlen(filterData->filter));
    sprintf(query, rs_queryMapFilterCount->string, filterData->filter);
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();
    if (mysql_errno(&mysql) != 0) {
        printf("MySQL ERROR: %s\n", mysql_error(&mysql));
    }
    mysql_res = mysql_store_result(&mysql);

    while( ( row = mysql_fetch_row( mysql_res ) ) != NULL ) {
        count = atoi(row[0]);
        break;
    }

    if ( count == 0 )
    {
        Q_strncatz( result, va( "No maps found for your search on %s%s.\n", S_COLOR_YELLOW, filterData->filter ),  sizeof(result));
    }
    else
    {
        totalPages = count/MAPS_PER_PAGE+1;
        Q_strncatz( result, va( "Printing page %d/%d of maps matching %s%s.\n%sUse %smapfilter %s <pagenum> %sto print other pages.\n",
                page,
                totalPages,
                S_COLOR_YELLOW,
                filterData->filter,
                S_COLOR_WHITE,
                S_COLOR_YELLOW,
                filterData->filter,
                S_COLOR_WHITE),  sizeof(result));
        //now fetch the matching maps
        sprintf(query, rs_queryMapFilter->string, filterData->filter, start, MAPS_PER_PAGE);
        mysql_real_query(&mysql, query, strlen(query));
        RS_CheckMysqlThreadError();
        if (mysql_errno(&mysql) != 0) {
            printf("MySQL ERROR: %s\n", mysql_error(&mysql));
        }
        mysql_res = mysql_store_result(&mysql);

        while( ( row = mysql_fetch_row( mysql_res ) ) != NULL ) {
            Q_strncatz( result, va( "%s#%02d%s : %s\n", S_COLOR_ORANGE, atoi(row[0]), S_COLOR_WHITE, row[1] ),  sizeof(result));
        }
    }

    filter_players[filterData->player_id]=malloc(size);
    Q_strncpyz( filter_players[filterData->player_id], result, size);
    RS_PushCallbackQueue(RACESOW_CALLBACK_MAPFILTER, filterData->player_id, count, 0, 0, 0, 0);


    mysql_free_result(mysql_res);
    free(filterData->filter);
    free(filterData);
    RS_EndMysqlThread();

    return NULL;
}

/**
 * Mapfilter function registered in AS API.
 *
 * Calls RS_MysqlMapFilter
 *
 * @param player_id Id of the player making the request
 * @param filter String used to filter maplist
 * @param page Number of the result page
 * @return Sucess boolean
 */
qboolean RS_MysqlMapFilter(int player_id, char *filter,unsigned int page)
{
    pthread_t thread;
    int returnCode;
    struct filterDataStruct *filterdata=malloc(sizeof(struct filterDataStruct));
    filterdata->player_id = player_id;
    filterdata->filter = strdup(filter);
    filterdata->page = page;

    returnCode = pthread_create(&thread, &threadAttr, RS_MysqlMapFilter_Thread, (void *)filterdata);

    if (returnCode) {

        printf("THREAD ERROR: return code from pthread_create() is %d\n", returnCode);
        return qfalse;
    }

    return qtrue;
}

/**
 * This function is called from angelscript when RACESOW_CALLBACK_MAPFILTER is
 * popped out of the callback queue.
 *
 * @param player_id Id of the player making the request
 * @return the result string to print to the player
 */
char *RS_MysqlMapFilterCallback(int player_id )
{
    unsigned int size = strlen(filter_players[player_id])+1;
    char *result=malloc(size);
    Q_strncpyz(result, filter_players[player_id] , size);
    free(filter_players[player_id]);
    filter_players[player_id] = NULL;
    return result;
}

/**
 * Calls the highscores thread
 *
 * @param edict_t *ent
 * @param int map_id
 * @return void
 */
qboolean RS_MysqlLoadHighscores( int playerNum, int map_id )
{
	pthread_t thread;
	int returnCode;

	struct highscoresDataStruct *highscoresData=malloc(sizeof(struct highscoresDataStruct));

    highscoresData->map_id=map_id;
	highscoresData->playerNum=playerNum;
    
	returnCode = pthread_create(&thread, &threadAttr, RS_MysqlLoadHighscores_Thread, (void *)highscoresData);
	if (returnCode) {

		printf("THREAD ERROR: return code from pthread_create() is %d\n", returnCode);
		return qfalse;
	}
	
	return qtrue;
}


//=================
//RS_MysqlLoadHighScores
// straight from 0.42 with some changes
//=================

char *highscores_players[3][256]={0}; // no more than 256 players at the same time on a server, right?

void *RS_MysqlLoadHighscores_Thread( void* in ) {
    
		MYSQL_ROW  row;
        MYSQL_RES  *mysql_res;
        char query[1024];
		int playerNum;
		int map_id;
		int limit;
		char highscores[3][10000];
		struct highscoresDataStruct *highscoresData;
		int highscore_part;


		highscoresData=(struct highscoresDataStruct *)in;
		playerNum=highscoresData->playerNum;
		map_id=highscoresData->map_id;

		limit=30;
		
		RS_StartMysqlThread();

        // get top players on map
		sprintf(query, rs_queryLoadMapHighscores->string, map_id, limit);
        mysql_real_query(&mysql, query, strlen(query));
        RS_CheckMysqlThreadError();
        mysql_res = mysql_store_result(&mysql);
        RS_CheckMysqlThreadError();

		highscores[0][0]='\0';
		highscores[1][0]='\0';
		highscores[2][0]='\0';
        
        if( (unsigned long)mysql_num_rows( mysql_res ) == 0 )
            Q_strncatz(highscores[0], va( "%sNo highscores found yet!\n", S_COLOR_RED ), sizeof(highscores[0]));
        
        else {
            unsigned int position = 0;
            unsigned int last_position = 0;
            unsigned int draw_position = 0;
            char last_time[16];
            char draw_time[16];
            char diff_time[16];
            int min, sec, milli, dmin, dsec, dmilli;
			int replay_record;
			
			replay_record=0;
 
            Q_strncatz(highscores[0], va( "%sTop %d players on map '%s'%s\n", S_COLOR_ORANGE, limit, level.mapname, S_COLOR_WHITE ), sizeof(highscores[0]));

            while( ( row = mysql_fetch_row( mysql_res ) ) != NULL )
			{
				
				if( /*load_replay_record && */	!position && row[0] ) {
                    replay_record = atoi( row[0] );
                    /*
					Q_strncpyz(level_items.record_player, row[1], sizeof(level_items.record_player));
                    if( rs_restoreHighscores->integer )
                        game.race_record = atoi( row[0] );
					*/
                } 
                position++;

                // convert time into MM:SS:mmm
                milli = atoi( row[0] );
                min = milli / 60000;
                milli -= min * 60000;
                sec = milli / 1000;
                milli -= sec * 1000;

				dmilli = atoi( row[0] ) - replay_record;
                dmin = dmilli / 60000;
                dmilli -= dmin * 60000;
                dsec = dmilli / 1000;
                dmilli -= dsec * 1000;

                Q_strncpyz( draw_time, va( "%d:%d.%03d", min, sec, milli ), sizeof(draw_time) );
                Q_strncpyz( diff_time, va( "+%d:%d.%03d", dmin, dsec, dmilli ), sizeof(diff_time) );

                if( !Q_stricmp( va( "%s", last_time ), va( "%s", draw_time ) ) )
                    draw_position = last_position;
                else
                    draw_position = position;

                last_position = draw_position;
                Q_strncpyz( last_time, va( "%d:%d.%d", min, sec, milli ), sizeof(last_time) );

                if( position <= 10 )
                    Q_strncatz( highscores[0], va( "%s%3d. %s%6s  %s[%s]  %s %s  %s(%s)\n", S_COLOR_WHITE, draw_position, S_COLOR_GREEN, draw_time, S_COLOR_YELLOW, diff_time, S_COLOR_WHITE, row[1], S_COLOR_WHITE, row[2] ), sizeof(highscores[0]) );
                else if( position <= 20 )
                    Q_strncatz( highscores[1], va( "%s%3d. %s%6s  %s[%s]  %s %s  %s(%s)\n", S_COLOR_WHITE, draw_position, S_COLOR_GREEN, draw_time, S_COLOR_YELLOW, diff_time, S_COLOR_WHITE, row[1], S_COLOR_WHITE, row[2] ), sizeof(highscores[1]) );
                else if( position <= 30 )
                    Q_strncatz( highscores[2], va( "%s%3d. %s%6s  %s[%s]  %s %s  %s(%s)\n", S_COLOR_WHITE, draw_position, S_COLOR_GREEN, draw_time, S_COLOR_YELLOW, diff_time, S_COLOR_WHITE, row[1], S_COLOR_WHITE, row[2] ), sizeof(highscores[2]) );
            }
			}

        mysql_free_result(mysql_res);

		for (highscore_part=0;highscore_part<3;highscore_part++)
		{
				highscores_players[highscore_part][playerNum]=malloc(strlen(highscores[highscore_part])+1);
				highscores_players[highscore_part][playerNum][0]='\0';
				Q_strncpyz( highscores_players[highscore_part][playerNum],highscores[highscore_part], strlen(highscores[highscore_part]));
		}

		RS_PushCallbackQueue(RACESOW_CALLBACK_HIGHSCORES, playerNum, 0, 0, 0, 0, 0);
		
		free(highscoresData);	
		RS_EndMysqlThread();
		return NULL;
}

/**
 * Print highscores to a player
 * 
 * @param edict_t *ent
 * @param int playerNum
 * @return void
 */

qboolean RS_PrintHighscoresTo( edict_t *ent, int playerNum )
{
	int highscore_part;
	if( ent != NULL )
	{
		G_PrintMsg( ent, va( "\n%s\n", highscores_players[0][playerNum]) );
		G_PrintMsg( ent, va( "%s\n", highscores_players[1][playerNum]) ); 
		G_PrintMsg( ent, va( "%s\n",highscores_players[2][playerNum]) );
	}
	
	for (highscore_part=0;highscore_part<3;highscore_part++)
	{
		free(highscores_players[highscore_part][playerNum]);
		highscores_players[highscore_part][playerNum]=NULL;
	}
	return qtrue;
}


/* map-list related global variables */

unsigned int mapcount=0;
char maplist[50000]; // around 5000 maps..

//=================
//RS_MysqlLoadMaplist
// straight from 0.42 with some changes
// maplist is loaded only once at the beginning of a map, so we don't really need to thread it, also because threads dont support string output.
//=================
char *RS_MysqlLoadMaplist( int is_freestyle ) {
        MYSQL_ROW  row;
        MYSQL_RES  *mysql_res;
        char query[1024];
        char orderby[10];

	    maplist[0] = 0;
	    mapcount = 0;

       	Q_strncpyz( orderby, "name", sizeof(orderby) );
        
		sprintf(query, rs_queryLoadMapList->string, is_freestyle ? "true" : "false", orderby);
        mysql_real_query(&mysql, query, strlen(query));
		RS_CheckMysqlThreadError();
       	if (mysql_errno(&mysql) != 0) {
            printf("MySQL ERROR: %s\n", mysql_error(&mysql));
	    }
        mysql_res = mysql_store_result(&mysql);
		RS_CheckMysqlThreadError();
       	if (mysql_errno(&mysql) != 0) {
            printf("MySQL ERROR: %s\n", mysql_error(&mysql));
	    }
        if( (unsigned long)mysql_num_rows( mysql_res ) != 0 ) {
        
            while( ( row = mysql_fetch_row( mysql_res ) ) != NULL ) {
                Q_strncatz( maplist, va( "%s ", row[0] ), sizeof( maplist ) );
                mapcount++;
            }
        
        }
        mysql_free_result(mysql_res);
        G_Printf( va( "Found %i maps in database.\n", mapcount ) );
    
 
    /*
	// (from .42) not implemented (should we?)
	if(!mapcount)
        RS_LoadConfigMaplist();
	*/
	return maplist;
}

unsigned int RS_GetNumberOfMaps()
{
	return mapcount;
}

/**
 * Start the mysql thread
 * 
 * @param void 
 * @return void
 */
void RS_StartMysqlThread()
{
    pthread_mutex_lock(&mutexsum);
}

/**
 * Cleanly end the mysql thread
 * 
 * @param void 
 * @return void
 */
void RS_EndMysqlThread()
{
	//mysql_thread_end(); // looks like thread_init and thread_end are not really needed, saw that on some webpage
    pthread_mutex_unlock(&mutexsum);
	pthread_exit(NULL);
}

/**
 * MySQL errorhandler for threads
 * 
 * @param void *threadid
 * @return void
 */
void RS_CheckMysqlThreadError()
{
	if (mysql_errno(&mysql) != 0) {
    
        printf("MySQL ERROR: %s\n", mysql_error(&mysql));
		RS_EndMysqlThread();
    }
}


// Functions

//================================================
//rs_SplashFrac - racesow version of G_SplashFrac
//================================================
void rs_SplashFrac( const vec3_t origin, const vec3_t mins, const vec3_t maxs, const vec3_t point, float maxradius, vec3_t pushdir, float *kickFrac, float *dmgFrac )
{
	vec3_t boxcenter = { 0, 0, 0 };
	float distance =0;
	int i;
	float innerradius;
	float outerradius;
	float g_distance;
	float h_distance;

	if( maxradius <= 0 )
	{
		if( kickFrac )
			*kickFrac = 0;
		if( dmgFrac )
			*dmgFrac = 0;
		return;
	}

	innerradius = ( maxs[0] + maxs[1] - mins[0] - mins[1] ) * 0.25;
	outerradius = ( maxs[2] - mins[2] ); //cylinder height

	// find center of the box
	for( i = 0; i < 3; i++ )
		boxcenter[i] = origin[i] + maxs[i] + mins[i];

	// find box radius to explosion origin direction
	VectorSubtract( boxcenter, point, pushdir );

	g_distance = sqrt( pushdir[0]*pushdir[0] + pushdir[1]*pushdir[1] ); // distance on the virtual ground
	h_distance = fabs( pushdir[2] );                                // corrected distance in height

	if( ( h_distance <= outerradius/2 ) || ( g_distance > innerradius ) )
	{
		distance = g_distance - innerradius;
	}
	if( ( h_distance > outerradius/2 ) || ( g_distance <= innerradius ) )
	{
		distance = h_distance - outerradius/2;
	}
	if( ( h_distance > outerradius/2 ) || ( g_distance > innerradius ) )
	{
		distance = sqrt( ( g_distance - innerradius )*( g_distance - innerradius ) + ( h_distance - outerradius/2 )*( h_distance - outerradius/2 ) );
	}

	if( dmgFrac )
	{
		// soft sin curve
		*dmgFrac = sin( DEG2RAD( ( distance / maxradius ) * 80 ) );
		clamp( *dmgFrac, 0.0f, 1.0f );
	}

	if( kickFrac )
	{
		*kickFrac = 1.0 - fabs( distance / maxradius );
		clamp( *kickFrac, 0.0f, 1.0f );
	}

	VectorSubtract( boxcenter, point, pushdir );
	VectorNormalizeFast( pushdir );
}

/*
* RS_removeProjectiles
*/
void RS_removeProjectiles( edict_t *owner )
{
	edict_t *ent;

	for( ent = game.edicts + gs.maxclients; ENTNUM( ent ) < game.numentities; ent++ )
	{
		if( ent->r.inuse && !ent->r.client && ent->r.svflags & SVF_PROJECTILE && ent->r.solid != SOLID_NOT
				&& ent->r.owner == owner )
		{
			G_FreeEdict( ent );
		}
	}
}
/*
 * racesow 0.42 time prestep function
*/
void rs_TimeDeltaPrestepProjectile( edict_t *projectile, int timeDelta )
{
	vec3_t forward, start;
	trace_t	trace;
	int i;
	edict_t *passent=projectile->r.owner;

	VectorScale( projectile->velocity, -( timeDelta )*0.001, forward );
	VectorMA( projectile->s.origin, 1, forward, start );

	G_Trace4D( &trace, projectile->s.origin, projectile->r.mins, projectile->r.maxs, start, passent, MASK_SHOT, 0 );

	for( i = 0; i < 3; i++ )
		projectile->s.origin[i] = projectile->s.origin2[i] = projectile->olds.origin[i] = trace.endpos[i];

	GClip_LinkEntity( projectile );
	SV_Impact( projectile, &trace );

	if( projectile->r.inuse )
		projectile->waterlevel = ( G_PointContents4D( projectile->s.origin, projectile->timeDelta ) & MASK_WATER ) ? qtrue : qfalse;
}
