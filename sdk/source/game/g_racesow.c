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
cvar_t *rs_queryGetPlayer;
cvar_t *rs_queryAddPlayer;
cvar_t *rs_queryRegisterPlayer;
cvar_t *rs_queryUpdatePlayerPlaytime;
cvar_t *rs_queryUpdatePlayerRaces;
cvar_t *rs_queryUpdatePlayerMaps;
cvar_t *rs_queryGetNick;
cvar_t *rs_queryCheckNick;
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
    
/**
 * as callback commands (must be similar to callback.as!)
 */
const unsigned int RACESOW_CALLBACK_AUTHENTICATE = 0;
const unsigned int RACESOW_CALLBACK_NICKPROTECT = 1;
const unsigned int RACESOW_CALLBACK_LOADMAP = 2;
const unsigned int RACESOW_CALLBACK_HIGHSCORES = 3;
const unsigned int RACESOW_CALLBACK_APPEAR = 4;
const unsigned int RACESOW_CALLBACK_RACEFINISH = 5;

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
    
    rs_queryGetPlayerAuth			= trap_Cvar_Get( "rs_queryGetPlayerAuth",				"SELECT `id`, `auth_mask` FROM `player` WHERE `auth_name` = '%s' AND `auth_pass` = MD5('%s') LIMIT 1;", CVAR_ARCHIVE );
	rs_queryGetPlayer				= trap_Cvar_Get( "rs_queryGetPlayer",			"SELECT `id`, `auth_mask` FROM `player` WHERE `simplified` = '%s' LIMIT 1;", CVAR_ARCHIVE );
	rs_queryAddPlayer				= trap_Cvar_Get( "rs_queryAddPlayer",				"INSERT INTO `player` (`name`, `simplified`, `created`) VALUES ('%s', '%s', NOW());", CVAR_ARCHIVE );
	rs_queryRegisterPlayer			= trap_Cvar_Get( "rs_queryRegisterPlayer",			"UPDATE `player` SET `auth_name` = '%s', `auth_email` = '%s', `auth_pass` = MD5('%s'), `auth_mask` = 1 WHERE `simplified` = '%s' AND (`auth_mask` = 0 OR `auth_mask` IS NULL) LIMIT 1;", CVAR_ARCHIVE );
	rs_queryUpdatePlayerPlaytime	= trap_Cvar_Get( "rs_queryUpdatePlayerPlaytime",	"UPDATE `player` SET `playtime` = playtime + %d WHERE `id` = %d LIMIT 1;", CVAR_ARCHIVE );
	rs_queryUpdatePlayerRaces		= trap_Cvar_Get( "rs_queryUpdatePlayerRaces",		"UPDATE `player` SET `races` = races + 1 WHERE `id` = %d LIMIT 1;", CVAR_ARCHIVE );
	rs_queryUpdatePlayerMaps		= trap_Cvar_Get( "rs_queryUpdatePlayerMaps",		"UPDATE `player` SET `maps` = (SELECT COUNT(`map_id`) FROM `player_map` WHERE `player_id` = `player`.`id`) WHERE `id` = %d LIMIT 1;", CVAR_ARCHIVE );

	rs_queryGetNick					= trap_Cvar_Get( "rs_queryGetNick",					"SELECT `id`, `player_id` FROM `nick` WHERE `simplified` = '%s';", CVAR_ARCHIVE );
	rs_queryAddNick					= trap_Cvar_Get( "rs_queryAddNick",					"INSERT INTO `nick` (`player_id`, `name`, `simplified`, `created`) VALUES (%d, '%s', '%s', NOW());", CVAR_ARCHIVE );
	rs_queryUpdateNickPlaytime		= trap_Cvar_Get( "rs_queryUpdateNickPlaytime",		"UPDATE `nick` SET `playtime` = `playtime` + %d WHERE `id` = %d", CVAR_ARCHIVE );
	rs_queryUpdateNickRaces			= trap_Cvar_Get( "rs_queryUpdateNickRaces",			"UPDATE `nick` SET `races` = races + 1 WHERE `id` = %d LIMIT 1;", CVAR_ARCHIVE );
	rs_queryUpdateNickMaps			= trap_Cvar_Get( "rs_queryUpdateNickMaps",			"UPDATE `nick` SET `maps` = (SELECT COUNT(`map_id`) FROM `nick_map` WHERE `nick_id` = `nick`.`id`) WHERE `id` = %d LIMIT 1;", CVAR_ARCHIVE );

	rs_queryGetMap					= trap_Cvar_Get( "rs_queryGetMapId",				"SELECT `id` FROM `map` WHERE `name` = '%s' LIMIT 1;", CVAR_ARCHIVE );
	rs_queryAddMap					= trap_Cvar_Get( "rs_queryAddMap",					"INSERT INTO `map` (`name`, `created`) VALUES('%s', NOW());", CVAR_ARCHIVE );
	rs_queryUpdateMapRating			= trap_Cvar_Get( "rs_queryUpdateMapRating",			"UPDATE `map` SET `rating` = (SELECT SUM(`value`) / COUNT(`player_id`) FROM `map_rating` WHERE `map_id` = `map`.`id`), `ratings` = (SELECT COUNT(`player_id`) FROM `map_rating` WHEHRE `map_id` = `map`.`id`) WHERE `id` = %d LIMIT 1;", CVAR_ARCHIVE );
	rs_queryUpdateMapPlaytime		= trap_Cvar_Get( "rs_queryUpdateMapPlaytime",		"UPDATE `map` SET `playtime` = `playtime` + %d WHERE `id` = %d LIMIT 1;", CVAR_ARCHIVE );
    rs_queryUpdateMapRaces			= trap_Cvar_Get( "rs_queryUpdateMapRaces",			"UPDATE `map` SET `races` = `races` + 1 WHERE `id` = %d LIMIT 1;", CVAR_ARCHIVE );

	rs_queryAddRace					= trap_Cvar_Get( "rs_queryAddRace",					"INSERT INTO `race` (`player_id`, `nick_id`, `map_id`, `time`, `created`) VALUES(%d, %d, %d, %d, NOW());", CVAR_ARCHIVE );
	
	rs_queryGetPlayerMap			= trap_Cvar_Get( "rs_queryGetPlayerMap",			"SELECT `points`, `time`, `races`, `playtime`, `created` FROM `player_map` WHERE `player_id` = %d AND `map_id` = %d LIMIT 1;", CVAR_ARCHIVE );
	rs_queryUpdatePlayerMap			= trap_Cvar_Get( "rs_queryUpdatePlayerMap",			"INSERT INTO `player_map` (`player_id`, `map_id`, `time`, `races`, `created`) VALUES(%d, %d, %d, 1, NOW()) ON DUPLICATE KEY UPDATE `time` = IF( VALUES(`time`) < `time` OR `time` = 0 OR `time` IS NULL, VALUES(`time`), `time` ), `races` = `races` + 1, `created` = IF( VALUES(`time`) < `time` OR `time` = 0 OR `time` IS NULL, NOW(), `created`);", CVAR_ARCHIVE );
	rs_queryUpdatePlayerMapPlaytime	= trap_Cvar_Get( "rs_queryUpdatePlayerMapPlaytime",	"INSERT INTO `player_map` (`player_id`, `map_id`, `playtime`) VALUES(%d, %d, %d) ON DUPLICATE KEY UPDATE `playtime` = `playtime` + VALUES(`playtime`);", CVAR_ARCHIVE );
	rs_queryGetPlayerMapHighscores	= trap_Cvar_Get( "rs_queryGetPlayerMapHighScores",	"SELECT `p`.`id`, `pm`.`time`, `p`.`name`, `pm`.`races`, `pm`.`playtime`, `pm`.`created` FROM `player_map` `pm` INNER JOIN `player` `p` ON `p`.`id` = `pm`.`player_id` WHERE `pm`.`time` IS NOT NULL AND `pm`.`time` > 0 AND `pm`.`map_id` = %d ORDER BY `pm`.`time` ASC;", CVAR_ARCHIVE );
    rs_queryResetPlayerMapPoints	= trap_Cvar_Get( "rs_queryResetPlayerMapPoints",	"UPDATE `player_map` SET `points` = 0 WHERE `map_id` = %d;", CVAR_ARCHIVE );
    rs_queryUpdatePlayerMapPoints	= trap_Cvar_Get( "rs_queryUpdatePlayerMapPoints",	"UPDATE `player_map` SET `points` = %d WHERE `map_id` = %d AND `player_id` = %d LIMIT 1;", CVAR_ARCHIVE );
    
	rs_queryUpdateNickMap			= trap_Cvar_Get( "rs_queryUpdateNickMap",			"INSERT INTO `nick_map` (`nick_id`, `map_id`, `time`, `races`, `created`) VALUES(%d, %d, %d, 1, NOW()) ON DUPLICATE KEY UPDATE `time` = IF( VALUES(`time`) < `time` OR `time` = 0 OR `time` IS NULL, VALUES(`time`), `time` ), `races` = `races` + 1, `created` = IF( VALUES(`time`) < `time` OR `time` = 0 OR `time` IS NULL, NOW(), `created`);", CVAR_ARCHIVE );
	rs_queryUpdateNickMapPlaytime	= trap_Cvar_Get( "rs_queryUpdateNickMapPlaytime",	"INSERT INTO `nick_map` (`nick_id`, `map_id`, `playtime`) VALUES(%d, %d, %d) ON DUPLICATE KEY UPDATE  `playtime` = `playtime` + VALUES(`playtime`);", CVAR_ARCHIVE );

	rs_querySetMapRating			= trap_Cvar_Get( "rs_querySetMapRating",			"INSERT INTO `map_rating` (`player_id`, `map_id`, `value`, `created`) VALUES(%d, %d, %d, NOW()) ON DUPLICATE KEY UPDATE `value` = VALUE(`value`), `changed` = NOW();", CVAR_ARCHIVE );
    
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
int callback_queue_index=0;
int callback_queue[MAX_SIZE_CALLBACK_QUEUE][4];

/**
 *
 * Add a command to the callback queue
 * 
 * @return void
 */
void RS_PushCallbackQueue( int command, int arg1, int arg2, int arg3)
{
	pthread_mutex_lock(&mutex_callback);
	if (callback_queue_size<MAX_SIZE_CALLBACK_QUEUE)
	{
		// push!
		callback_queue_size++;
		callback_queue_index=(callback_queue_index+1)%MAX_SIZE_CALLBACK_QUEUE;
		callback_queue[callback_queue_index][0]=command;
		callback_queue[callback_queue_index][1]=arg1;
		callback_queue[callback_queue_index][2]=arg2;
		callback_queue[callback_queue_index][3]=arg3;
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
qboolean RS_PopCallbackQueue(int *command, int *arg1, int *arg2, int *arg3)
{

	// if the queue is empty, do nothing this time
	// (a push might have happened, but it doesn't matter, we'll catch it next time!)
	if (callback_queue_size==0)
		return qfalse;

	// retrieving a callback result
	pthread_mutex_lock(&mutex_callback);
	// pop!
	*command=callback_queue[callback_queue_index][0];
	*arg1=callback_queue[callback_queue_index][1];
	*arg2=callback_queue[callback_queue_index][2];
	*arg3=callback_queue[callback_queue_index][3];
	callback_queue_size--;
	pthread_mutex_unlock(&mutex_callback);
	return qtrue;
}

/**
 * Authenticate the player
 *
 * @param edict_t *ent
 * @param char *authName
 * @param char *authPass
 * @return qboolean
 */
qboolean RS_MysqlAuthenticate( unsigned int playerNum, char *authName, char *authPass )
{
	int returnCode;
	pthread_t thread;
    struct authenticationData *authData=malloc(sizeof(struct authenticationData));
    
    authData->playerNum = playerNum;
    authData->authName = strdup(authName);	
    authData->authPass = strdup(authPass);

	returnCode = pthread_create(&thread, &threadAttr, RS_MysqlAuthenticate_Thread, authData);

	if (returnCode) {

		printf("THREAD ERROR: return code from pthread_create() is %d\n", returnCode);
		return qfalse;
	}
	
	return qtrue;
}

/**
 * Calls the nickname protection thread
 *
 * @param edict_t *ent
 * @return void
 */
qboolean RS_MysqlNickProtection( edict_t *ent )
{
	pthread_t thread;
	int returnCode = pthread_create(&thread, &threadAttr, RS_MysqlNickProtection_Thread, (void *)ent);
	if (returnCode) {

		printf("THREAD ERROR: return code from pthread_create() is %d\n", returnCode);
		return qfalse;
	}
	
	return qtrue;
}

/**
 * Check if the player violates against the nickname protection
 *
 * @param void *in
 * @return void
 */
void *RS_MysqlNickProtection_Thread(void *in)
{
    char query[250];
    char name[64];
    MYSQL_ROW  row;
    MYSQL_RES  *mysql_res;
    edict_t *ent;

	RS_StartMysqlThread();

    ent = (edict_t *)in;
    Q_strncpyz ( name, COM_RemoveColorTokens( ent->r.client->netname ), sizeof(name) );
    mysql_real_escape_string(&mysql, name, name, strlen(name));
    
	sprintf(query, rs_queryCheckPlayer->string, name);
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();

    mysql_res = mysql_store_result(&mysql);
    RS_CheckMysqlThreadError();
    
    if ((row = mysql_fetch_row(mysql_res)) != NULL) {
    
        RS_PushCallbackQueue(RACESOW_CALLBACK_NICKPROTECT, PLAYERNUM(ent), atoi(row[0]), 0);
    
    } else {
    
        RS_PushCallbackQueue(RACESOW_CALLBACK_NICKPROTECT, PLAYERNUM(ent), 0, 0);
    }
    
    mysql_free_result(mysql_res);
	RS_EndMysqlThread();
    
    return NULL;
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
 * Getting map id and returning it as a callback
 *
 * @param void *in
 * @return void
 */
void *RS_MysqlLoadMap_Thread(void *in)
{
    char query[250];
    char name[64];
    MYSQL_ROW  row;
    MYSQL_RES  *mysql_res;

	RS_StartMysqlThread();

    Q_strncpyz ( name, COM_RemoveColorTokens( level.mapname ), sizeof(name) );
    mysql_real_escape_string(&mysql, name, name, strlen(name));
    
	sprintf(query, rs_queryGetMap->string, name);
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();
    
    mysql_res = mysql_store_result(&mysql);
    RS_CheckMysqlThreadError();
    
    if ((row = mysql_fetch_row(mysql_res)) != NULL) {
    
        RS_PushCallbackQueue(RACESOW_CALLBACK_LOADMAP, atoi(row[0]), 0, 0);
    
    } else {
    
        sprintf(query, rs_queryAddMap->string, name);
        mysql_real_query(&mysql, query, strlen(query));
        RS_CheckMysqlThreadError();
        
        RS_PushCallbackQueue(RACESOW_CALLBACK_LOADMAP, (int)mysql_insert_id(&mysql), 0, 0);
    }
    
    mysql_free_result(mysql_res);
	RS_EndMysqlThread();
    
	return NULL;
}

/**
 * Authentication thread
 *
 * @param void *in
 * @return void
 */
void *RS_MysqlAuthenticate_Thread( void *in )
{
    char query[1000];
    char name[64];
    char pass[64];
    MYSQL_ROW  row;
    MYSQL_RES  *mysql_res;
    struct authenticationData *authData = (struct authenticationData *)in;

	RS_StartMysqlThread();
    
    Q_strncpyz ( name, authData->authName, sizeof(name) );
    mysql_real_escape_string(&mysql, name, name, strlen(name));   
    
    Q_strncpyz ( pass, authData->authPass, sizeof(pass) );
    mysql_real_escape_string(&mysql, pass, pass, strlen(pass));
    
	sprintf(query, rs_queryGetPlayerAuth->string, name, pass);
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();
    
    mysql_res = mysql_store_result(&mysql);
    RS_CheckMysqlThreadError();
    
    if ((row = mysql_fetch_row(mysql_res)) != NULL) {
		if (row[0]!=NULL && row[1]!=NULL) 
			RS_PushCallbackQueue(RACESOW_CALLBACK_AUTHENTICATE,authData->playerNum, atoi(row[0]), atoi(row[1]));
    
    } else {
		RS_PushCallbackQueue(RACESOW_CALLBACK_AUTHENTICATE,authData->playerNum, 0,0);
    }
    
    mysql_free_result(mysql_res);
	free(authData->authName);
	free(authData->authPass);
	free(authData);	
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
        
        /*
        // insert player
        char name[64];
        char simplified[64];
        
        Q_strncpyz ( name, raceData->ent->r.client->netname, sizeof(name) );
        mysql_real_escape_string(&mysql, name, name, strlen(name));    
        
        Q_strncpyz ( simplified, COM_RemoveColorTokens( raceData->ent->r.client->netname ), sizeof(simplified) );
        mysql_real_escape_string(&mysql, simplified, simplified, strlen(simplified));
        
        sprintf(query, rs_queryAddPlayer->string, name, simplified);
        mysql_real_query(&mysql, query, strlen(query));
        RS_CheckMysqlThreadError();
        */
    }
    
	// insert race
	sprintf(query, rs_queryAddRace->string, raceData->player_id, raceData->nick_id, raceData->map_id, raceData->race_time);
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();

	// increment player races
	sprintf(query, rs_queryUpdatePlayerRaces->string, raceData->player_id);
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();

	// increment nick races
	sprintf(query, rs_queryUpdateNickRaces->string, raceData->nick_id);
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

	// insert or update nick_map
	sprintf(query, rs_queryUpdateNickMap->string, raceData->nick_id, raceData->map_id, raceData->race_time);
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
    sprintf(query, rs_queryResetPlayerMapPoints->string);
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

	// awards are now handled by a AS callback (it was unsafe to do this in a thread)

	// TODO: instead of returning the current best time, retrieve and return the -previous- best time,
	// so the correct delta can be printed
	RS_PushCallbackQueue(RACESOW_CALLBACK_RACEFINISH,raceData->playerNum, raceData->race_time, bestTime);

	/*

    // convert time into MM:SS:mmm
    milli = (int)raceData->race_time;
    min = milli / 60000;
    milli -= min * 60000;
    sec = milli / 1000;
    milli -= sec * 1000;
    
    if (raceData->race_time < bestTime || bestTime == 0)
    {
        dmilli = (int)raceData->race_time - (int)bestTime;
    }
    else
    {
        dmilli = (int)raceData->race_time - (int)currentRaceTime;
    }
    
    dmin = dmilli / 60000;
    dmilli -= dmin * 60000;
    dsec = dmilli / 1000;
    dmilli -= dsec * 1000;
    Q_strncpyz( draw_time, va( "%d:%d.%03d", min, sec, milli ), sizeof(draw_time) );
    Q_strncpyz( diff_time, va( "%d:%d.%03d", dmin, dsec, dmilli ), sizeof(diff_time) );
    
    if (raceData->race_time < bestTime)
    {
        G_PlayerAward( raceData->ent, va("%New server record!", S_COLOR_GREEN) );
        G_CenterPrintMsg( raceData->ent, va("Time: %s\n%s-%s", draw_time, S_COLOR_GREEN, diff_time ) );
    }
    else if (bestTime == 0)
    {
        G_PlayerAward( raceData->ent, va("%New server record!", S_COLOR_GREEN) );
        G_CenterPrintMsg( raceData->ent, va("Time: %s", S_COLOR_GREEN, draw_time ) );
    }
    else if (raceData->race_time == bestTime)
    {
        G_PlayerAward( raceData->ent, va("%Server record!", S_COLOR_GREEN) );
        G_CenterPrintMsg( raceData->ent, va("Time: %s\n%s+-%s", draw_time, S_COLOR_GREEN, diff_time ) );
    }
    else if (raceData->race_time < currentRaceTime)
    {
        G_PlayerAward( raceData->ent, va("%sNew personal record!\n", S_COLOR_YELLOW) );
        G_CenterPrintMsg( raceData->ent, va("Time: %s\n%s-%s", draw_time, S_COLOR_YELLOW, diff_time ) );
    }
    else if (currentRaceTime == 0)
    {
        G_PlayerAward( raceData->ent, va("%sPersonal record!\n", S_COLOR_YELLOW) );
        G_CenterPrintMsg( raceData->ent, va("%sTime: %s", S_COLOR_YELLOW, draw_time ) );
    }
    else
    {
        G_PlayerAward( raceData->ent, va("%sRace finished!\n", S_COLOR_WHITE) );
        G_CenterPrintMsg( raceData->ent, va("Time: %s\n%s+%s", draw_time, S_COLOR_RED, diff_time ) );
    }
	*/

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
qboolean RS_MysqlPlayerAppear( char *name, int playerNum, int player_id, int is_authed )
{
	pthread_t thread;
	int returnCode;
	struct playerDataStruct *playerData=malloc(sizeof(struct playerDataStruct));

    playerData->name = strdup(name);
    playerData->player_id = player_id;
	playerData->playerNum = playerNum;
	playerData->is_authed = is_authed;

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
    char query[1000];
	char name[100];
	char simplified[100];
	MYSQL_ROW  row;
    MYSQL_RES  *mysql_res;
	int is_authed,playerNum;
	int nick_id,player_id, auth_mask;
	struct playerDataStruct *playerData;

	playerNum=0;
	is_authed=0;
	nick_id=0;
	player_id=0;
	auth_mask=0;
	
	RS_StartMysqlThread();

    playerData = (struct playerDataStruct*)in;
	player_id=playerData->player_id;
	is_authed=playerData->is_authed;
	playerNum=playerData->playerNum;

    Q_strncpyz ( simplified, COM_RemoveColorTokens( playerData->name), sizeof(simplified) );
    mysql_real_escape_string(&mysql, simplified, simplified, strlen(simplified));
	Q_strncpyz ( name, playerData->name, sizeof(name) );
    mysql_real_escape_string(&mysql, name, name, strlen(name));

	//printf("%s appeared (player num: %d) %s\n",name,playerNum, is_authed?", authed":"");

	// check if that nick already exists and retrieve the nick_id 
	sprintf(query, rs_queryGetNick->string, simplified);
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();
    
    mysql_res = mysql_store_result(&mysql);
    RS_CheckMysqlThreadError();
    
    if ((row = mysql_fetch_row(mysql_res)) != NULL) 
		if (row[0]!=NULL && row[1]!=NULL) 
			nick_id=atoi(row[0]);
	mysql_free_result(mysql_res);

	if (is_authed)
		{
		if (nick_id==0)
			{
			// new nick but registered player, so add it
			sprintf(query, rs_queryAddNick->string, player_id,name,simplified);
			mysql_real_query(&mysql, query, strlen(query));
			RS_CheckMysqlThreadError();   
			}

		// since the player is authed, we don't have to do anything else
		RS_PushCallbackQueue(RACESOW_CALLBACK_APPEAR, playerNum, player_id, nick_id);
		}
	else
		{
		// now, if the player is not authed

		if (nick_id==0)
			{
			// it's a new nick, hence, a new player, so add both
			sprintf(query, rs_queryAddPlayer->string, name,simplified);
			mysql_real_query(&mysql, query, strlen(query));
			RS_CheckMysqlThreadError();    

			// retrieve the new player_id
			player_id=(int)mysql_insert_id(&mysql);

			// add his nick
			sprintf(query, rs_queryAddNick->string, player_id,name,simplified);
			mysql_real_query(&mysql, query, strlen(query));
			RS_CheckMysqlThreadError();    

			// retrieve the new nick_id
			nick_id=(int)mysql_insert_id(&mysql);
			}
		else
			{
			// that nick already exists, hence it belongs to a player P. 
			// if P is registered (authmask>0), then trigger nick protection
			// if P is not registered (authmask==0), then don't do anything

			// retrieve player_id corresponding to that name
			int player_id_for_nick=0;
			sprintf(query, rs_queryGetPlayer->string, simplified);
			mysql_real_query(&mysql, query, strlen(query));
			RS_CheckMysqlThreadError();
			mysql_res = mysql_store_result(&mysql);
			RS_CheckMysqlThreadError();
			if ((row = mysql_fetch_row(mysql_res)) != NULL) 
				if (row[0]!=NULL && row[1]!=NULL )
					{
					player_id_for_nick=atoi(row[0]);
					auth_mask=atoi(row[1]);
					}
			mysql_free_result(mysql_res);

			if (auth_mask > 0) 
				{
				if (player_id_for_nick!=player_id)
						// player numbers not matched: trigger nick protection!
						RS_PushCallbackQueue(RACESOW_CALLBACK_NICKPROTECT, playerData->playerNum, player_id_for_nick, 0);
				}
			else 
				player_id=player_id_for_nick;
			}
		}

	// store player_id and nick_id in AS
	RS_PushCallbackQueue(RACESOW_CALLBACK_APPEAR, playerNum, player_id, nick_id);

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
 * Thread when player disappear,
 *
 * it does what's written in mysql concept.txt 
 *
 * @param void *in
 * @return void
 */
void *RS_MysqlPlayerDisappear_Thread(void *in)
{
    char query[1000];
	char name[100];
	char simplified[100];
	unsigned int player_id, map_id, playtime, nick_id, auth_mask, is_authed;
	struct playtimeDataStruct *playtimeData;

	player_id=0;
	nick_id=0;
	map_id=0;
	playtime=0;
	auth_mask=0;
	is_authed=0;
	
	RS_StartMysqlThread();

    playtimeData = (struct playtimeDataStruct*)in;
	map_id=playtimeData->map_id;
	player_id=playtimeData->player_id;
	nick_id=playtimeData->nick_id;
	is_authed=playtimeData->is_authed;
	playtime=playtimeData->playtime;

    Q_strncpyz ( simplified, COM_RemoveColorTokens( playtimeData->name ), sizeof(simplified) );
    mysql_real_escape_string(&mysql, simplified, simplified, strlen(simplified));
	Q_strncpyz ( name, playtimeData->name, sizeof(name) );
    mysql_real_escape_string(&mysql, name, name, strlen(name));

	//printf("%s disappeared, gonna update playtime (%d) for player_id %d nick_id %d is_authed %d and map_id %d\n",name,playtime,player_id,nick_id,is_authed,map_id);

	if (player_id!=0 && !is_authed)
	{
			// well well.. he was using a registered nick, so he was under nickprotection warning.
			// let's not record any playtime in this situation

			free(playtimeData->name);	
			free(playtimeData);	
			RS_EndMysqlThread();

			return NULL;
	}

	// increment player playtime
	sprintf(query, rs_queryUpdatePlayerPlaytime->string, playtime, player_id);
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();

	// increment nick playtime
	sprintf(query, rs_queryUpdateNickPlaytime->string, playtime, nick_id);
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();

	// increment map playtime
	sprintf(query, rs_queryUpdateMapPlaytime->string, playtime, map_id);
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();

	// increment player map playtime
	sprintf(query, rs_queryUpdatePlayerMapPlaytime->string, player_id, map_id, playtime);
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();

	// increment nick map playtime
	sprintf(query, rs_queryUpdateNickMapPlaytime->string, nick_id, map_id, playtime);
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();

	free(playtimeData->name);	
	free(playtimeData);	
	RS_EndMysqlThread();
    
    return NULL;
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

char *highscores_players[256]={0}; // no more than 256 players at the same time on a server, right?

void *RS_MysqlLoadHighscores_Thread( void* in ) {
    
		MYSQL_ROW  row;
        MYSQL_RES  *mysql_res;
        char query[1024];
		int playerNum;
		int map_id;
		int limit;
		char highscores[10000];
		struct highscoresDataStruct *highscoresData;


		highscoresData=(struct highscoresDataStruct *)in;
		playerNum=highscoresData->playerNum;
		map_id=highscoresData->map_id;

		limit=30;
		
		RS_StartMysqlThread();

        // get top players on map
        Q_strncpyz ( query, va( "SELECT  MIN( time ) AS time, p.name, r.created FROM race AS r LEFT JOIN player AS p ON p.id = r.player_id LEFT JOIN map AS m ON m.id = r.map_id WHERE m.id = %i GROUP BY p.id ORDER BY time ASC LIMIT %i", map_id, limit ), sizeof(query) );
        mysql_real_query(&mysql, query, strlen(query));
        RS_CheckMysqlThreadError();
        mysql_res = mysql_store_result(&mysql);
        RS_CheckMysqlThreadError();
        
        if( (unsigned long)mysql_num_rows( mysql_res ) == 0 )
            Q_strncatz(highscores, va( "%sNo highscores found yet!\n", S_COLOR_RED ), sizeof(highscores));
        
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
 
            Q_strncatz(highscores, va( "%sTop %d players on map '%s'%s\n", S_COLOR_ORANGE, limit, level.mapname, S_COLOR_WHITE ), sizeof(highscores));

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
                    Q_strncatz( highscores,  va( "%s%3d. %s%6s  %s[%s]  %s %s  %s(%s)\n", S_COLOR_WHITE, draw_position, S_COLOR_GREEN, draw_time, S_COLOR_YELLOW, diff_time, S_COLOR_WHITE, row[1], S_COLOR_WHITE, row[2] ), sizeof(highscores) );
                else if( position <= 20 )
                    Q_strncatz( highscores, va( "%s%3d. %s%6s  %s[%s]  %s %s  %s(%s)\n", S_COLOR_WHITE, draw_position, S_COLOR_GREEN, draw_time, S_COLOR_YELLOW, diff_time, S_COLOR_WHITE, row[1], S_COLOR_WHITE, row[2] ), sizeof(highscores) );
                else if( position <= 30 )
                    Q_strncatz( highscores, va( "%s%3d. %s%6s  %s[%s]  %s %s  %s(%s)\n", S_COLOR_WHITE, draw_position, S_COLOR_GREEN, draw_time, S_COLOR_YELLOW, diff_time, S_COLOR_WHITE, row[1], S_COLOR_WHITE, row[2] ), sizeof(highscores) );
            }
			}

        mysql_free_result(mysql_res);
		
		highscores_players[playerNum]=malloc(strlen(highscores));
		Q_strncpyz( highscores_players[playerNum],highscores, strlen(highscores));
		RS_PushCallbackQueue(RACESOW_CALLBACK_HIGHSCORES, playerNum, 0, 0);
		
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
	if( ent != NULL )
      G_PrintMsg( ent, highscores_players[playerNum] );
	G_PrintMsg( ent, "\n" );
	free(highscores_players[playerNum]);
	highscores_players[playerNum]=NULL;
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

        
		/*
		// not implemented
        switch( g_maprotation->integer ) {
        
            case 2:
                Q_strncpyz( orderby, "RAND()", sizeof(orderby) );
                break;
            
            case 1:
            default:
                Q_strncpyz( orderby, "name", sizeof(orderby) );  
                break;      
        }
		*/
		// replaced by:
		Q_strncpyz( orderby, "name", sizeof(orderby) );  
        
        Q_strncpyz( query, va( "SELECT name FROM map WHERE freestyle = '%s' AND status = 'enabled' ORDER BY %s;", ( is_freestyle ? "true" : "false" ), orderby ), sizeof(query) );
        mysql_real_query(&mysql, query, strlen(query));
       	if (mysql_errno(&mysql) != 0) {
            printf("MySQL ERROR: %s\n", mysql_error(&mysql));
	    }
        mysql_res = mysql_store_result(&mysql);
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
        G_Printf( va( "Found %i maps in databse.\n", mapcount ) );
    
 
    /*
	// not implemented (should we?)
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
