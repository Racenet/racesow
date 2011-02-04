#include "g_local.h"
#ifdef WIN32
#include <winsock.h>
#endif
#include <pthread.h>
#include <mysql.h>
#include <errmsg.h>

cvar_t *sv_port;
cvar_t *sv_hostname;

/**
 * MySQL CVARs
 */
cvar_t *rs_mysqlEnabled;
cvar_t *rs_mysqlHost;
cvar_t *rs_mysqlPort;
cvar_t *rs_mysqlUser;
cvar_t *rs_mysqlPass;
cvar_t *rs_mysqlDb;

cvar_t *rs_queryGetPlayerAuth;
cvar_t *rs_queryGetPlayerAuthByToken;
cvar_t *rs_queryGetPlayerAuthBySession;
cvar_t *rs_queryGetPlayerByToken;
cvar_t *rs_querySetTokenForPlayer;
cvar_t *rs_querySetSessionForPlayer;
cvar_t *rs_queryGetPlayer;
cvar_t *rs_queryAddPlayer;
cvar_t *rs_queryGetPlayerStats;
cvar_t *rs_queryGetServer;
cvar_t *rs_queryGetServerById;
cvar_t *rs_queryAddServer;
cvar_t *rs_queryIncrementServerRaces;
cvar_t *rs_queryUpdateServerData;
cvar_t *rs_queryGetPlayerPoints;
cvar_t *rs_queryRegisterPlayer;
cvar_t *rs_queryGetPlayerNick;
cvar_t *rs_queryUpdatePlayerNick;
cvar_t *rs_queryUpdatePlayerPlaytime;
cvar_t *rs_queryUpdatePlayerRaces;
cvar_t *rs_queryUpdatePlayerMaps;
cvar_t *rs_queryUpdatePlayerPoints;
cvar_t *rs_queryUpdatePlayerHistory;
cvar_t *rs_queryPurgePlayerHistory;
cvar_t *rs_queryGetMap;
cvar_t *rs_queryAddMap;
cvar_t *rs_queryGetMapStats;
cvar_t *rs_queryUpdateMapRating;
cvar_t *rs_queryUpdateMapPlaytime;
cvar_t *rs_queryUpdateMapRaces;
cvar_t *rs_queryAddRace;
cvar_t *rs_queryGetPlayerMap;
cvar_t *rs_queryUpdatePlayerMap;
cvar_t *rs_queryUpdatePlayerMapInfo;
cvar_t *rs_queryGetPlayerMapHighscore;
cvar_t *rs_queryGetPlayerMapHighscores;
cvar_t *rs_queryResetPlayerMapPoints;
cvar_t *rs_queryUpdatePlayerMapPoints;
cvar_t *rs_querySetMapRating;
cvar_t *rs_queryLoadMapList;
cvar_t *rs_queryLoadMapHighscores;
cvar_t *rs_queryLoadMapOneliners;
cvar_t *rs_querySetMapOneliner;
cvar_t *rs_queryMapFilter;
cvar_t *rs_queryMapFilterCount;
cvar_t *rs_queryUpdateCheckpoint;
cvar_t *rs_queryGetPlayerMapCheckpoints;


cvar_t *rs_authField_Name;
cvar_t *rs_authField_Pass;
cvar_t *rs_authField_Token;
cvar_t *rs_tokenSalt;

cvar_t *rs_loadHighscores;
cvar_t *rs_loadPlayerCheckpoints;
cvar_t *rs_historyDays;

cvar_t *rs_IRCstream;
dynvar_t *irc_connected;
int ircConnected = 0;

/**
 * Store the result of different requests from players.
 */
char *players_query[MAX_CLIENTS]={0};

/**
 * map-list related global variables
 */
unsigned int mapcount = 0;
char maplist[50000] = {0}; // around 5000 maps..
char previousMapName[MAX_CONFIGSTRING_CHARS];

/**
 * as callback commands (must be similar to callback.as!)
 */
const unsigned int RACESOW_CALLBACK_LOADMAP = 2;
const unsigned int RACESOW_CALLBACK_HIGHSCORES = 3;
const unsigned int RACESOW_CALLBACK_APPEAR = 4;
const unsigned int RACESOW_CALLBACK_RACE = 5;
const unsigned int RACESOW_CALLBACK_MAPFILTER = 6;
const unsigned int RACESOW_CALLBACK_MAPLIST = 7;
const unsigned int RACESOW_CALLBACK_PLAYERNICK = 8;
const unsigned int RACESOW_CALLBACK_ONELINER = 9;

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
 * callback queue variables used for de-threadization of mysql calls
 */
#define MAX_SIZE_CALLBACK_QUEUE 50
int callback_queue_size=0;
int callback_queue_write_index=0;
int callback_queue_read_index=0;
int callback_queue[MAX_SIZE_CALLBACK_QUEUE][8];

int MysqlConnected = 0;


// some queries can be very long! keep this over 3k ok?
#define MYSQL_QUERY_LENGTH 3000


/**
 * MySQL errorhandler for threads
 *
 * not a function anymore
 */
#define RS_CheckMysqlThreadError() {if (RS_MysqlError()) { G_Printf("file=%s line=%d MySQL Error!\n",__FILE__,__LINE__); RS_EndMysqlThread(); } }

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

    rs_mysqlEnabled = trap_Cvar_Get( "rs_mysqlEnabled", "1", CVAR_ARCHIVE|CVAR_NOSET);
    rs_IRCstream = trap_Cvar_Get( "rs_IRCstream","0", CVAR_ARCHIVE|CVAR_NOSET );
    rs_loadHighscores = trap_Cvar_Get( "rs_loadHighscores", "0", CVAR_ARCHIVE);
    rs_loadPlayerCheckpoints = trap_Cvar_Get( "rs_loadPlayerCheckpoints", "0", CVAR_ARCHIVE);

	previousMapName[0]='\0';

    if ( rs_mysqlEnabled->integer )
    {
        // initialize mysql
        RS_LoadCvars();
        MysqlConnected = RS_MysqlConnect();
    }

    if (MysqlConnected != 0 && rs_historyDays->integer != 0)
    {
        char query[MYSQL_QUERY_LENGTH];
        mysql_real_query(&mysql, rs_queryUpdatePlayerHistory->string, strlen(rs_queryUpdatePlayerHistory->string));
        RS_MysqlError();

        sprintf(query, rs_queryPurgePlayerHistory->string, rs_historyDays->integer);
        mysql_real_query(&mysql, query, strlen(query));
        RS_MysqlError();
    }
    RS_AddServerCommands();
    if( rs_IRCstream->integer )
        trap_Cmd_ExecuteText( EXEC_APPEND, "irc_connect" );
    irc_connected = trap_Dynvar_Lookup( "irc_connected" );
    trap_Dynvar_AddListener( irc_connected, RS_Irc_ConnectedListener_f );

}

/**
 * RS_LoadCvars
 *
 * @return void
 */
void RS_LoadCvars( void )
{
    sv_port = trap_Cvar_Get( "sv_port", "44400", CVAR_ARCHIVE );
    sv_hostname = trap_Cvar_Get( "sv_hostname", "racesow server", CVAR_ARCHIVE );

    rs_mysqlHost = trap_Cvar_Get( "rs_mysqlHost", "localhost", CVAR_ARCHIVE );
    rs_mysqlPort = trap_Cvar_Get( "rs_mysqlPort", "0", CVAR_ARCHIVE ); // if 0 it will use the system's default port
    rs_mysqlUser = trap_Cvar_Get( "rs_mysqlUser", "root", CVAR_ARCHIVE );
    rs_mysqlPass = trap_Cvar_Get( "rs_mysqlPass", "", CVAR_ARCHIVE );
    rs_mysqlDb = trap_Cvar_Get( "rs_mysqlDb", "racesow", CVAR_ARCHIVE );

    rs_authField_Name = trap_Cvar_Get( "rs_authField_Name", "", CVAR_ARCHIVE|CVAR_NOSET);
    rs_authField_Pass = trap_Cvar_Get( "rs_authField_Pass", "", CVAR_ARCHIVE|CVAR_NOSET);
    rs_authField_Token = trap_Cvar_Get( "rs_authField_Token", "", CVAR_ARCHIVE|CVAR_NOSET);
    rs_tokenSalt = trap_Cvar_Get( "rs_tokenSalt", "", CVAR_ARCHIVE|CVAR_NOSET);

    rs_historyDays = trap_Cvar_Get( "rs_historyDays", "30", CVAR_ARCHIVE|CVAR_LATCH);

    if (!Q_stricmp( rs_authField_Name->string, "" ) || !Q_stricmp( rs_authField_Pass->string, "" ) ||!Q_stricmp( rs_authField_Token->string, "" ))
    {
        G_Error("\033[31;40m\nthe cVars rs_authField_Name, rs_authField_Pass and rs_authField_Token must be set and should be unique for your database!\n\nie. racenet uses the following settings:\n\tset rs_authField_Name \"racenet_user\"\n\tset rs_authField_Pass \"racenet_pass\"\n\tset rs_authField_Token \"racenet_token\"\n\nlike that you can store authentications for multiple servers in a single config.\n\033[0m\n");
    }

    G_Printf("-------------------------------------\nClient authentication via userinfo:\nsetu %s \"username\"\nsetu %s \"password\"\nor the more secure method using an enrypted token\nsetu %s \"token\"\n-------------------------------------\n", rs_authField_Name->string, rs_authField_Pass->string, rs_authField_Token->string);

    rs_queryGetPlayerAuth			= trap_Cvar_Get( "rs_queryGetPlayerAuth",			"SELECT `id`, `auth_mask`, `auth_token` FROM `player` WHERE `auth_name` = '%s' AND `auth_pass` = MD5('%s%s') LIMIT 1;", CVAR_ARCHIVE );
    rs_queryGetPlayerAuthByToken    = trap_Cvar_Get( "rs_queryGetPlayerAuthByToken",	"SELECT `id`, `auth_mask` FROM `player` WHERE `auth_token` = MD5('%s%s') LIMIT 1;", CVAR_ARCHIVE );
    rs_queryGetPlayerAuthBySession  = trap_Cvar_Get( "rs_queryGetPlayerAuthBySession",	"SELECT `id`, `auth_mask` FROM `player` WHERE `session_token` = '%s' LIMIT 1;", CVAR_ARCHIVE );
    rs_querySetTokenForPlayer       = trap_Cvar_Get( "rs_querySetTokenForPlayer",	    "UPDATE `player` SET `auth_token` = MD5('%s%s') WHERE `id` = %d;", CVAR_ARCHIVE );
    rs_querySetSessionForPlayer     = trap_Cvar_Get( "rs_querySetSessionForPlayer",	    "UPDATE `player` SET `session_token` = '%s' WHERE `id` = %d;", CVAR_ARCHIVE );
	rs_queryGetPlayer				= trap_Cvar_Get( "rs_queryGetPlayer",			    "SELECT `id`, `auth_mask` FROM `player` WHERE `simplified` = '%s' LIMIT 1;", CVAR_ARCHIVE );
	rs_queryAddPlayer				= trap_Cvar_Get( "rs_queryAddPlayer",				"INSERT INTO `player` (`name`, `simplified`, `created`) VALUES ('%s', '%s', NOW());", CVAR_ARCHIVE );
	rs_queryGetPlayerStats  		= trap_Cvar_Get( "rs_queryGetPlayerStats",			"SELECT `points`, `diff_points`, `races`, (SELECT SUM(`overall_tries`) FROM `player_map` WHERE `player_id` = `p`.`id` LIMIT 1) `race_tries`, `maps`, `playtime`, (SELECT SUM(`racing_time`) FROM `player_map` WHERE `player_id` = `p`.`id` LIMIT 1) `racing_time`, DATE_FORMAT(`created`, '%%Y-%%m-%%d') `first_seen`, (SELECT `date` FROM `player_history` WHERE `player_id` = `p`.`id` ORDER BY `date` DESC LIMIT 1) `last_seen` FROM `player` `p` WHERE `simplified` = '%s' LIMIT 1;", CVAR_ARCHIVE );
	rs_queryGetPlayerPoints			= trap_Cvar_Get( "rs_queryGetPlayerPoints",		    "SELECT `points` FROM `player` WHERE `id` = '%d' LIMIT 1;", CVAR_ARCHIVE );
	rs_queryRegisterPlayer			= trap_Cvar_Get( "rs_queryRegisterPlayer",			"UPDATE `player` SET `auth_name` = '%s', `auth_email` = '%s', `auth_pass` = MD5('%s%s'), `auth_mask` = 1, `auth_token` = MD5('%s%s') WHERE `simplified` = '%s' AND (`auth_mask` = 0 OR `auth_mask` IS NULL);", CVAR_ARCHIVE );
	rs_queryGetPlayerNick			= trap_Cvar_Get( "rs_queryGetPlayerNick",			"SELECT `name` FROM `player` WHERE `id` = '%d' LIMIT 1;", CVAR_ARCHIVE );
	rs_queryUpdatePlayerNick		= trap_Cvar_Get( "rs_queryUpdatePlayerNick",		"UPDATE `player` SET `name` = '%s', `simplified` = '%s' WHERE `id` = %d;", CVAR_ARCHIVE );
	rs_queryUpdatePlayerPlaytime	= trap_Cvar_Get( "rs_queryUpdatePlayerPlaytime",	"UPDATE `player` SET `playtime` = `playtime` + %d WHERE `id` = %d;", CVAR_ARCHIVE );
	rs_queryUpdatePlayerRaces		= trap_Cvar_Get( "rs_queryUpdatePlayerRaces",		"UPDATE `player` SET `races` = races + 1 WHERE `id` = %d;", CVAR_ARCHIVE );
	rs_queryUpdatePlayerMaps		= trap_Cvar_Get( "rs_queryUpdatePlayerMaps",		"UPDATE `player` SET `maps` = (SELECT COUNT(`map_id`) FROM `player_map` WHERE `player_id` = `player`.`id`) WHERE `id` = %d;", CVAR_ARCHIVE );
	rs_queryUpdatePlayerPoints		= trap_Cvar_Get( "rs_queryUpdatePlayerPoints",		"UPDATE `player` SET `points` = (SELECT SUM(`points`) FROM `player_map` WHERE `player_id` = `player`.`id`), `diff_points` = (`points` - (SELECT `points` FROM `player_history` WHERE `player_id` = `player`.`id` ORDER BY `date` DESC LIMIT 1)) WHERE `id` IN(%s);", CVAR_ARCHIVE );
	rs_queryUpdatePlayerHistory		= trap_Cvar_Get( "rs_queryUpdatePlayerHistory",		"INSERT INTO `player_history` (`player_id`, `date`, `races`, `maps`, `playtime`, `points`) SELECT `p`.`id`, DATE_SUB(NOW(), INTERVAL 1 DAY), `p`.`races`, `p`.`maps`, `p`.`playtime`, `p`.`points` FROM `player` `p` WHERE `p`.`playtime` IS NOT NULL AND `p`.`playtime` > 0 AND (SELECT `player_id` FROM `player_history` WHERE `player_id` = `p`.`id` AND ( `date` = DATE_SUB(NOW(), INTERVAL 1 DAY) OR ( `date` < DATE_SUB(NOW(), INTERVAL 1 DAY) AND `races` = `p`.`races` AND `points` = `p`.`points` AND `maps` = `p`.`maps` AND `playtime` = `p`.`playtime`) ) ORDER BY `date` DESC LIMIT 1) IS NULL;", CVAR_ARCHIVE );
	rs_queryPurgePlayerHistory		= trap_Cvar_Get( "rs_queryPurgePlayerHistory",		"DELETE FROM `player_history` WHERE `date` < DATE_SUB(NOW(), INTERVAL %d DAY);", CVAR_ARCHIVE );
    rs_queryGetServer               = trap_Cvar_Get( "rs_queryGetServer",				"SELECT `id`, `user` FROM `gameserver` WHERE `user` = CONCAT(USER(), ':', %d) LIMIT 1;", CVAR_ARCHIVE );
    rs_queryGetServerById           = trap_Cvar_Get( "rs_queryGetServerById",			"SELECT `id`, `user` FROM `gameserver` WHERE `id` = %d LIMIT 1;", CVAR_ARCHIVE );
    rs_queryAddServer               = trap_Cvar_Get( "rs_queryAddServer",				"INSERT INTO `gameserver` (`user`, `servername`, `created`) VALUES(CONCAT(USER(), ':', %d), '%s', NOW());", CVAR_ARCHIVE );
    rs_queryIncrementServerRaces    = trap_Cvar_Get( "rs_queryIncrementServerRaces",    "UPDATE `gameserver` SET `races` = `races` + 1 WHERE `user` = CONCAT(USER(), ':', %d);", CVAR_ARCHIVE );
    rs_queryUpdateServerData        = trap_Cvar_Get( "rs_queryUpdateServerData",        "UPDATE `gameserver` SET `servername` = '%s', `playtime` = `playtime` + %d, `maps` = (SELECT COUNT(DISTINCT map_id) FROM `race` WHERE `server_id` = `gameserver`.`id`) WHERE `user` = CONCAT(USER(), ':', %d);", CVAR_ARCHIVE );
	rs_queryGetMap					= trap_Cvar_Get( "rs_queryGetMapId",				"SELECT `id` FROM `map` WHERE `name` = '%s' LIMIT 1;", CVAR_ARCHIVE );
	rs_queryAddMap					= trap_Cvar_Get( "rs_queryAddMap",					"INSERT INTO `map` (`name`, `created`) VALUES('%s', NOW());", CVAR_ARCHIVE );
	rs_queryGetMapStats  		    = trap_Cvar_Get( "rs_queryGetMapStats",			    "SELECT IF(`freestyle`, 'freestyle', 'race') `type`, `m`.`races`, SUM(`pm`.`overall_tries`) as `tries`, CAST(AVG(`pm`.`overall_tries`) AS UNSIGNED) `avg_overall_tries`, CAST(AVG(`pm`.`tries`) AS UNSIGNED) `avg_tries`, CAST(AVG(`pm`.`duration`) AS UNSIGNED) `avg_duration`, `m`.`playtime`, DATE_FORMAT(`m`.`created`, '%%Y-%%m-%%d') `created`, MIN(`pm`.`time`) `best_time`, MAX(`pm`.`time`) `worst_time`, COUNT(`pm`.`player_id`) `players` FROM map m LEFT JOIN player_map pm ON pm.map_id = m.id WHERE name = '%s' GROUP BY m.`id`;", CVAR_ARCHIVE );
	rs_queryUpdateMapRating			= trap_Cvar_Get( "rs_queryUpdateMapRating",			"UPDATE `map` SET `rating` = (SELECT SUM(`value`) / COUNT(`player_id`) FROM `map_rating` WHERE `map_id` = `map`.`id`), `ratings` = (SELECT COUNT(`player_id`) FROM `map_rating` WHERE `map_id` = `map`.`id`) WHERE `id` = %d;", CVAR_ARCHIVE );
	rs_queryUpdateMapPlaytime		= trap_Cvar_Get( "rs_queryUpdateMapPlaytime",		"UPDATE `map` SET `playtime` = `playtime` + %d WHERE `id` = %d;", CVAR_ARCHIVE );
    rs_queryUpdateMapRaces			= trap_Cvar_Get( "rs_queryUpdateMapRaces",			"UPDATE `map` SET `races` = `races` + 1 WHERE `id` = %d;", CVAR_ARCHIVE );
	rs_queryAddRace					= trap_Cvar_Get( "rs_queryAddRace",					"INSERT INTO `race` (`player_id`, `map_id`, `time`, `tries`, `duration`, `server_id`, `created`, `prejumped`) VALUES(%d, %d, %d, %d, %d, %d, NOW(), '%s');", CVAR_ARCHIVE );
	rs_queryGetPlayerMap			= trap_Cvar_Get( "rs_queryGetPlayerMap",			"SELECT `points`, `time`, `prejumped`, `races`, `playtime`, `created` FROM `player_map` WHERE `player_id` = %d AND `map_id` = %d LIMIT 1;", CVAR_ARCHIVE );
	rs_queryUpdatePlayerMap			= trap_Cvar_Get( "rs_queryUpdatePlayerMap",			"INSERT INTO `player_map` (`player_id`, `map_id`, `time`, `races`, `server_id`, `tries`, `duration`, `created`, `prejumped`) VALUES(%d, %d, %d, 1, %d, (SELECT SUM(`tries`) FROM `race` WHERE `player_id` = %d AND `map_id` = %d AND `tries` IS NOT NULL), (SELECT SUM(`duration`) FROM `race` WHERE `player_id` = %d AND `map_id` = %d AND `duration` IS NOT NULL), NOW(), '%s') ON DUPLICATE KEY UPDATE `races` = `races` + 1, `created` = IF( VALUES(`time`) < `time` OR `time` = 0 OR `time` IS NULL, NOW(), `created`), `server_id` = IF( VALUES(`time`) < `time` OR `time` = 0 OR `time` IS NULL, VALUES(`server_id`), `server_id`), `tries` = IF( VALUES(`time`) < `time` OR `time` = 0 OR `time` IS NULL, VALUES(`tries`), `tries`), `duration` = IF( VALUES(`time`) < `time` OR `time` = 0 OR `time` IS NULL, VALUES(`duration`), `duration`), `prejumped` = IF( VALUES(`time`) < `time` OR `time` = 0 OR `time` IS NULL, VALUES(`prejumped`), `prejumped`), `time` = IF( VALUES(`time`) < `time` OR `time` = 0 OR `time` IS NULL, VALUES(`time`), `time`);", CVAR_ARCHIVE );
	rs_queryUpdatePlayerMapInfo  	= trap_Cvar_Get( "rs_queryUpdatePlayerMapInfo",	    "INSERT INTO `player_map` (`player_id`, `map_id`, `playtime`, overall_tries, racing_time) VALUES(%d, %d, %d, %d, %d) ON DUPLICATE KEY UPDATE `playtime` = `playtime` + VALUES(`playtime`), `overall_tries` = `overall_tries` + VALUES(`overall_tries`), `racing_time` = `racing_time` + VALUES(`racing_time`);", CVAR_ARCHIVE );
	rs_queryGetPlayerMapHighscore	= trap_Cvar_Get( "rs_queryGetPlayerMapHighScore",	"SELECT `p`.`id`, `pm`.`time`, `pm`.`overall_tries`, `p`.`name`, `pm`.`races`, `pm`.`playtime`, `pm`.`created` FROM `player_map` `pm` INNER JOIN `player` `p` ON `p`.`id` = `pm`.`player_id` WHERE `pm`.`map_id` = %d AND `pm`.`player_id` = %d LIMIT 1;", CVAR_ARCHIVE );
    rs_queryGetPlayerMapHighscores	= trap_Cvar_Get( "rs_queryGetPlayerMapHighScores",	"SELECT `p`.`id`, `pm`.`time`, `p`.`name`, `pm`.`races`, `pm`.`playtime`, `pm`.`created`, `pm`.`prejumped` FROM `player_map` `pm` INNER JOIN `player` `p` ON `p`.`id` = `pm`.`player_id` WHERE `pm`.`time` IS NOT NULL AND `pm`.`time` > 0 AND `pm`.`map_id` = %d AND `pm`.`prejumped` in (%s) ORDER BY `pm`.`time` ASC;", CVAR_ARCHIVE );
    rs_queryResetPlayerMapPoints	= trap_Cvar_Get( "rs_queryResetPlayerMapPoints",	"UPDATE `player_map` SET `points` = 0 WHERE `map_id` = %d;", CVAR_ARCHIVE );
    rs_queryUpdatePlayerMapPoints	= trap_Cvar_Get( "rs_queryUpdatePlayerMapPoints",	"UPDATE `player_map` SET `points` = %d WHERE `map_id` = %d AND `player_id` = %d;", CVAR_ARCHIVE );
	rs_querySetMapRating			= trap_Cvar_Get( "rs_querySetMapRating",			"INSERT INTO `map_rating` (`player_id`, `map_id`, `value`, `created`) VALUES(%d, %d, %d, NOW()) ON DUPLICATE KEY UPDATE `value` = VALUE(`value`), `changed` = NOW();", CVAR_ARCHIVE );
	rs_queryLoadMapList				= trap_Cvar_Get( "rs_queryLoadMapList",				"SELECT name FROM map WHERE freestyle = '%d' AND status = 'enabled' ORDER BY %s;", CVAR_ARCHIVE);
	rs_queryMapFilter               = trap_Cvar_Get( "rs_queryMapFilter",               "SELECT id, name FROM map WHERE name LIKE '%%%s%%' AND freestyle = '%s' LIMIT %u, %u;", CVAR_ARCHIVE );
	rs_queryMapFilterCount          = trap_Cvar_Get( "rs_queryMapFilterCount",          "SELECT COUNT(id)FROM map WHERE name LIKE '%%%s%%' AND freestyle = '%s';", CVAR_ARCHIVE );
    rs_queryUpdateCheckpoint        = trap_Cvar_Get( "rs_queryUpdateCheckpoint",        "INSERT INTO `checkpoint` (`player_id`, `map_id`, `time`, `num`) VALUES(%d, %d, %d, %d) ON DUPLICATE KEY UPDATE `time` = VALUES(`time`);", CVAR_ARCHIVE );
    rs_queryGetPlayerMapCheckpoints = trap_Cvar_Get( "rs_queryGetPlayerMapCheckpoints", "SELECT `time` FROM `checkpoint` WHERE `map_id` = %d AND `player_id` = %d ORDER BY `num` ASC;", CVAR_ARCHIVE );
	rs_queryLoadMapHighscores		= trap_Cvar_Get( "rs_queryLoadMapHighscores",		"SELECT pm.time, p.name, pm.created, pm.prejumped FROM player_map AS pm LEFT JOIN player AS p ON p.id = pm.player_id LEFT JOIN map AS m ON m.id = pm.map_id WHERE pm.time IS NOT NULL AND pm.time > 0 AND m.id = %d AND pm.prejumped in (%s) ORDER BY time ASC LIMIT %d", CVAR_ARCHIVE );
	rs_queryLoadMapOneliners		= trap_Cvar_Get( "rs_queryLoadMapOneliners",		"SELECT `oneliner`, `pj_oneliner` FROM `map` WHERE `id` = %d;", CVAR_ARCHIVE );
	rs_querySetMapOneliner			= trap_Cvar_Get( "rs_querySetMapOneliner",			"UPDATE `map` SET `%s` = '%s' WHERE `id` = %d;", CVAR_ARCHIVE );

}

/**
 * RS_MysqlConnect
 *
 * @return qboolean
 */
qboolean RS_MysqlConnect( void )
{
    char query[MYSQL_QUERY_LENGTH];
    char user[255];
    MYSQL_ROW  row;
    MYSQL_RES  *mysql_res;
	int server_id=0;

    my_bool reconnect = 1;
    mysql_options(&mysql, MYSQL_OPT_RECONNECT, &reconnect);

    G_Printf( va( "MySQL Connection String\nmysql://%s:*****@%s:%d/%s\n", rs_mysqlUser->string, rs_mysqlHost->string, rs_mysqlPort->integer, rs_mysqlDb->string ) );
    if( !Q_stricmp( rs_mysqlHost->string, "" ) || !Q_stricmp( rs_mysqlUser->string, "user" ) || !Q_stricmp( rs_mysqlDb->string, "" ) ) {
        G_Printf( "-------------------------------------\nMySQL ERROR1: Connection-data incomplete or not available\n" );
        return qfalse;
    }
    mysql_init( &mysql );
    if( &mysql == NULL ) {
        RS_MysqlError();
        return qfalse;
    }
    if( !mysql_real_connect ( &mysql, rs_mysqlHost->string, rs_mysqlUser->string, rs_mysqlPass->string, rs_mysqlDb->string, rs_mysqlPort->integer, NULL, 0 ) ) {
        RS_MysqlError();
        return qfalse;
    }
    G_Printf( "-------------------------------------\nConnected to MySQL Server\n-------------------------------------\n" );


    //Q_strncpyz ( user, COM_RemoveColorTokens( level.mapname ), sizeof(user) );
	//RS_EscapeString(user);

	sprintf(query, rs_queryGetServer->string, sv_port->integer);
    mysql_real_query(&mysql, query, strlen(query));
    RS_MysqlError();
    mysql_res = mysql_store_result(&mysql);
    RS_MysqlError();

	if ((row = mysql_fetch_row(mysql_res)) != NULL)
    {
        if (row[0] !=NULL && row[1] != NULL)
        {
            server_id = atoi(row[0]);
            Q_strncpyz(user, row[1], sizeof(user));
        }
    }

	mysql_free_result(mysql_res);

    if (server_id == 0)
    {
        char servername[255];
        Q_strncpyz ( servername, sv_hostname->string, sizeof(servername) );
		RS_EscapeString(servername);
        sprintf(query, rs_queryAddServer->string, sv_port->integer, servername);
        mysql_real_query(&mysql, query, strlen(query));
        RS_MysqlError();

		server_id=(int)mysql_insert_id(&mysql);

        sprintf(query, rs_queryGetServerById->string, server_id);
        mysql_real_query(&mysql, query, strlen(query));
        RS_MysqlError();
        mysql_res = mysql_store_result(&mysql);
        RS_MysqlError();

        if ((row = mysql_fetch_row(mysql_res)) != NULL)
        {
            if (row[0] !=NULL && row[1] != NULL)
            {
                Q_strncpyz(user, row[1], sizeof(user));
            }
        }

		mysql_free_result(mysql_res);
    }

    G_Printf( va("authenticated as %s/%d\n-------------------------------------\n", user, server_id ) );

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
 * @return qboolean
 */
qboolean RS_MysqlError( void )
{
    int errNo = mysql_errno(&mysql);
    if (errNo != 0) {

        G_Printf("%sMySQL ERROR: %s (%d)\n", S_COLOR_RED, mysql_error(&mysql), errNo);

        if (errNo == CR_SERVER_GONE_ERROR || errNo == CR_SERVER_LOST)
        {
            RS_LoadCvars();
            MysqlConnected = RS_MysqlConnect();
        }

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
	RS_RemoveServerCommands();
	trap_Dynvar_RemoveListener( irc_connected, RS_Irc_ConnectedListener_f );
}


/**
 * Escapes the string, in-place
 *
 * @param char* string
 * @return void
 */
void RS_EscapeString( char* string )
{
	char escaped_buffer[MAX_STRING_CHARS];

	// one cannot call mysql_real_escape_string with dest string == source string
	mysql_real_escape_string(&mysql, escaped_buffer, string, strlen(string));
	Q_strncpyz ( string, escaped_buffer, sizeof(escaped_buffer) );
}

/**
 *
 * Add a command to the callback queue
 *
 * @return void
 */
void RS_PushCallbackQueue(int command, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7)
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
		callback_queue[callback_queue_write_index][7]=arg7;

		callback_queue_write_index=(callback_queue_write_index+1)%MAX_SIZE_CALLBACK_QUEUE;
		callback_queue_size++;
	}
	else
		G_Printf("Error: callback queue overflow\n");
	pthread_mutex_unlock(&mutex_callback);
}

/**
 *
 * "Is there a callback result to execute?"
 * queried at each game frame
 *
 * @return qboolean
 */
qboolean RS_PopCallbackQueue(int *command, int *arg1, int *arg2, int *arg3, int *arg4, int *arg5, int *arg6, int *arg7)
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
	*arg7=callback_queue[callback_queue_read_index][7];

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

		G_Printf("THREAD ERROR: return code from pthread_create() is %d\n", returnCode);
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
    char query[MYSQL_QUERY_LENGTH];
    char name[64];
    MYSQL_ROW  row;
    MYSQL_RES  *mysql_res;
	int map_id=0;
	unsigned int bestTime=0;
	RS_StartMysqlThread();
    Q_strncpyz ( name, COM_RemoveColorTokens( level.mapname ), sizeof(name) );
	RS_EscapeString(name);

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
	sprintf(query, rs_queryGetPlayerMapHighscores->string, map_id, "'false'");
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();
    mysql_res = mysql_store_result(&mysql);
    RS_CheckMysqlThreadError();
    if ((row = mysql_fetch_row(mysql_res)) != NULL)
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

	RS_PushCallbackQueue(RACESOW_CALLBACK_LOADMAP, 0, map_id, bestTime, 0, 0, 0, 0);

	// remember this map name
	strcpy(previousMapName,level.mapname);

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
qboolean RS_MysqlInsertRace( unsigned int player_id, unsigned int nick_id, unsigned int map_id, unsigned int race_time, unsigned int playerNum, unsigned int tries, unsigned int duration, char *checkpoints, qboolean prejumped) {

	int returnCode;
    struct raceDataStruct *raceData=malloc(sizeof(struct raceDataStruct));
	pthread_t thread;

    // player finished a race while using a protected nickname
    if (player_id == 0) {

        return qfalse;
    }
    raceData->player_id = player_id;
	raceData->nick_id= nick_id;
	raceData->map_id = map_id;
	raceData->race_time = race_time;
	raceData->playerNum = playerNum;
	raceData->tries = tries;
	raceData->duration = duration;
	raceData->checkpoints = strdup( checkpoints );
	raceData->prejumped = prejumped;

	returnCode = pthread_create(&thread, &threadAttr, RS_MysqlInsertRace_Thread, (void *)raceData);

	if (returnCode) {

		G_Printf("THREAD ERROR: return code from pthread_create() is %d\n", returnCode);
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
    char query[1500];
    char affectedPlayerIds[500];
	unsigned int maxPositions, newPoints, currentPosition, currentCleanPosition, realPosition, offset, cleanOffset, points, lastRaceTime, lastCleanRaceTime, bestTime, oldTime, oldPoints, oldBestTime, oldOtherBestTime, oldBestPlayerId, oldOtherBestPlayerId, allPoints, newPosition, server_id;
	struct raceDataStruct *raceData;
    MYSQL_ROW  row;
    MYSQL_RES  *mysql_res;
    char *t; //token to parse checkpoints
    static const char *seps = " "; //token separator in checkpoints string
    int index = 0; //checkpoint number

    affectedPlayerIds[0] = '\0';
    oldTime = 0;
    bestTime = 0;
    maxPositions = 30;
    oldBestTime = 0;
    oldOtherBestTime = 0;
    oldBestPlayerId = 0;
    oldOtherBestPlayerId = 0;
    offset = 0;
    cleanOffset = 0;
    newPosition = 0;
	newPoints = 0;
	oldPoints = 0;
	currentPosition = 0;
	currentCleanPosition = 0;
    lastRaceTime = 0;
    lastCleanRaceTime = 0;
    allPoints = 0;
    server_id = 0;
	raceData =(struct raceDataStruct *)in;

	RS_StartMysqlThread();

	// read current points and time
	sprintf(query, rs_queryGetPlayerMap->string, raceData->player_id, raceData->map_id);
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();
	mysql_res = mysql_store_result(&mysql);
    RS_CheckMysqlThreadError();
    if ((row = mysql_fetch_row(mysql_res)) != NULL) {
		if (row[0] != NULL && row[1] != NULL)
        {
			oldPoints = atoi(row[0]);
            oldTime = atoi(row[1]);
            newPoints = oldPoints;
        }
    }
	mysql_free_result(mysql_res);

    // retrieve server best in the category (pj/nopj) of the race to be inserted
	sprintf(query, rs_queryGetPlayerMapHighscores->string, raceData->map_id, raceData->prejumped?"'true'":"'false'");
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();
    mysql_res = mysql_store_result(&mysql);
    RS_CheckMysqlThreadError();
    if ((row = mysql_fetch_row(mysql_res)) != NULL)
	{
		if (row[1] !=NULL)
		{
		    oldBestPlayerId = atoi(row[0]);
            oldBestTime = atoi(row[1]);
		}
	}
	mysql_free_result(mysql_res);

    // retrieve server best in the other category (pj/nopj) of the race to be inserted
    sprintf(query, rs_queryGetPlayerMapHighscores->string, raceData->map_id, raceData->prejumped?"'false'":"'true'");
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();
    mysql_res = mysql_store_result(&mysql);
    RS_CheckMysqlThreadError();
    if ((row = mysql_fetch_row(mysql_res)) != NULL)
    {
        if (row[1] !=NULL)
        {
            oldOtherBestPlayerId = atoi(row[0]);
            oldOtherBestTime = atoi(row[1]);
        }
    }
    mysql_free_result(mysql_res);


    // get server_id
    sprintf(query, rs_queryGetServer->string, sv_port->integer);
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();
    mysql_res = mysql_store_result(&mysql);
    RS_CheckMysqlThreadError();

	if ((row = mysql_fetch_row(mysql_res)) != NULL)
    {
        if (row[0] !=NULL && row[1] != NULL)
        {
            server_id = atoi(row[0]);
        }
    }

	mysql_free_result(mysql_res);

    if (server_id != 0)
    {
        // insert race
        sprintf(query, rs_queryAddRace->string, raceData->player_id, raceData->map_id, raceData->race_time, raceData->tries, raceData->duration, server_id, raceData->prejumped?"true":"false");
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

        // increment server races
        sprintf(query, rs_queryIncrementServerRaces->string, sv_port->integer);
        mysql_real_query(&mysql, query, strlen(query));
        RS_CheckMysqlThreadError();

        // insert or update player_map (aka personal record)
        sprintf(query, rs_queryUpdatePlayerMap->string, raceData->player_id, raceData->map_id, raceData->race_time, server_id, raceData->player_id, raceData->map_id, raceData->player_id, raceData->map_id, raceData->prejumped?"true":"false");
        mysql_real_query(&mysql, query, strlen(query));
        RS_CheckMysqlThreadError();

        // only when the new time is better than the old one, recompute the points
        if (oldTime == 0 || raceData->race_time < oldTime)
        {

            // clear the current oneliner in the race category if the rec of this category was beaten
            if ( raceData->race_time < oldBestTime )
            {
                char empty_oneliner[1];
                empty_oneliner[0] = '\0';
                sprintf(query, rs_querySetMapOneliner->string, raceData->prejumped?"pj_oneliner":"oneliner", empty_oneliner, raceData->map_id);
                mysql_real_query(&mysql, query, strlen(query));
                RS_CheckMysqlThreadError();
            }

            // - clear the current oneliner in the other category if current racer was the record holder in this category
            //   cause he just changed category (he was the best but improved) so he leaves his right to have the oneliner in his previous category
            // - also clear the prejumped oneliner if a nopj absolute record was made
            if ( raceData->player_id == oldOtherBestPlayerId || ( ( !raceData->prejumped ) && ( raceData->race_time < oldOtherBestTime ) ) )
            {
                char empty_oneliner[1];
                empty_oneliner[0] = '\0';
                sprintf(query, rs_querySetMapOneliner->string, raceData->prejumped?"oneliner":"pj_oneliner", empty_oneliner, raceData->map_id);
                mysql_real_query(&mysql, query, strlen(query));
                RS_CheckMysqlThreadError();
            }

            //update player checkpoints
            t = strtok( raceData->checkpoints , seps );
            while( t != NULL )
            {
                index++;
                sprintf(query, rs_queryUpdateCheckpoint->string, raceData->player_id, raceData->map_id, atoi(t), index);
                mysql_real_query(&mysql, query, strlen(query));
                RS_CheckMysqlThreadError();
                t = strtok( NULL, seps);
            }

            // reset points in player_map
            sprintf(query, rs_queryResetPlayerMapPoints->string, raceData->map_id);
            mysql_real_query(&mysql, query, strlen(query));
            RS_CheckMysqlThreadError();

            // update points in player_map
            sprintf(query, rs_queryGetPlayerMapHighscores->string, raceData->map_id, "'true','false'");
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
                    qboolean prejumped;

                    playerId = atoi(row[0]);
                    raceTime = atoi(row[1]);

                    if ( !Q_stricmp(row[6],"true") )
                        prejumped = qtrue;
                    else
                        prejumped = qfalse;

                    if ( Q_stricmp( affectedPlayerIds, "" ) )
                        Q_strncatz( affectedPlayerIds, ",", sizeof(affectedPlayerIds));

                    Q_strncatz( affectedPlayerIds, row[0], sizeof(affectedPlayerIds));

                    if (bestTime == 0)
                        bestTime = raceTime;

                    if (raceTime == lastRaceTime)
                        offset++;
                    else
                        offset = 0;

                    if ( !prejumped && raceTime == lastCleanRaceTime )
                        cleanOffset++;
                    else
                        cleanOffset = 0;

                    currentPosition++;
                    if ( !prejumped )
                    {
                        currentCleanPosition++;
                        realPosition = currentCleanPosition - cleanOffset;
                        lastCleanRaceTime = raceTime;
                    }
                    else
                    {
                        realPosition = currentPosition - offset;
                    }

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

                    if (playerId == raceData->player_id) {

                        newPoints = points;
                        newPosition = realPosition;
                    }

                    lastRaceTime = raceTime;

                    // set points in player_map
                    sprintf(query, rs_queryUpdatePlayerMapPoints->string, points, raceData->map_id, playerId);
                    mysql_real_query(&mysql, query, strlen(query));
                    RS_CheckMysqlThreadError();
                }
            }

            mysql_free_result(mysql_res);
            RS_CheckMysqlThreadError();

            // update points for affected players
            sprintf(query, rs_queryUpdatePlayerPoints->string, affectedPlayerIds);
            mysql_real_query(&mysql, query, strlen(query));
            RS_CheckMysqlThreadError();

        }

        sprintf(query, rs_queryGetPlayerPoints->string, raceData->player_id);
        mysql_real_query(&mysql, query, strlen(query));
        RS_CheckMysqlThreadError();
        mysql_res = mysql_store_result(&mysql);
        RS_CheckMysqlThreadError();
        if ((row = mysql_fetch_row(mysql_res)) != NULL)
        {
            if (row[0] !=NULL)
            {
                allPoints = atoi(row[0]);
            }
        }

		mysql_free_result(mysql_res);

        RS_PushCallbackQueue(RACESOW_CALLBACK_RACE, raceData->playerNum, allPoints, oldPoints, newPoints, oldTime, oldBestTime, raceData->race_time);
    }

    free(raceData->checkpoints);
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

		G_Printf("THREAD ERROR: return code from pthread_create() is %d\n", returnCode);
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
    char query[MYSQL_QUERY_LENGTH];
	char name[64];
	char simplified[64];
	char authName[64];
	char authPass[64];
	char authToken[64];
	char checkpoints[MAX_STRING_CHARS];
	//char sessionToken[64];
	MYSQL_ROW  row;
    MYSQL_RES  *mysql_res;
	unsigned int player_id, auth_mask, player_id_for_nick, auth_mask_for_nick, personalBest, player_id_for_time, overall_tries;
	int size;
	struct playerDataStruct *playerData;
	//edict_t *ent;

	RS_StartMysqlThread();
	checkpoints[0]='\0';
	size = 0;
	player_id = 0;
	auth_mask = 0;
	player_id_for_nick = 0;
	auth_mask_for_nick = 0;
    player_id_for_time = 0;
    personalBest = 0;
    overall_tries = 0;
    playerData = (struct playerDataStruct*)in;

    //ent = &game.edicts[ playerData->playerNum + 1 ];

    /*
    if( ent->r.client )
    {
        char *existingSession = Info_ValueForKey( ent->r.client->userinfo, "racesow_session" );
        if (existingSession) {
            Q_strncpyz(sessionToken, existingSession, sizeof(sessionToken));
        }
    }
    */

    // escape strings
    Q_strncpyz ( simplified, COM_RemoveColorTokens(playerData->name), sizeof(simplified) );
    RS_EscapeString(simplified);

	Q_strncpyz ( name, playerData->name, sizeof(name) );
	RS_EscapeString(name);

    Q_strncpyz ( authName, playerData->authName, sizeof(authName) );
	RS_EscapeString(authName);

    Q_strncpyz ( authPass, playerData->authPass, sizeof(authPass) );
	RS_EscapeString(authPass);

    Q_strncpyz ( authToken, playerData->authToken, sizeof(authToken) );
	RS_EscapeString(authToken);

    /*
    // try to authenticate by session
    if (Q_stricmp( sessionToken, "" ))
    {
        sprintf(query, rs_queryGetPlayerAuthBySession->string, sessionToken);
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
    */

    // try to authenticate by token
    if (player_id == 0 && Q_stricmp( authToken, "" ))
    {
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
    if (player_id == 0 && Q_stricmp( authName, "" ) && Q_stricmp( authPass, "" ))
    {
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

                if (row[2] != NULL)
                {
                    Q_strncpyz(authToken, row[2], sizeof(authToken));
            }
        }
        }

        mysql_free_result(mysql_res);
    }

    if (!Q_stricmp( authToken, "" ) && player_id != 0)
    {
        char *newToken = RS_GenerateNewToken(player_id);
        Q_strncpyz ( authToken, newToken, sizeof(authToken) );
        free(newToken);
        /*
        if( ent->r.inuse && ent->r.client )
        {
            G_PrintMsg(ent, "%san AUTHENTICATION TOKEN  has been generated for you: '%s' keep it secure!\n", S_COLOR_BLUE, authToken);
        }
        */
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

	mysql_free_result(mysql_res);

    // only add a new player if
	// 1) noone has this nick already, and
	// 2) the player isn't already authed (if he his, we keep using the nick associated to his auth)
    if ( player_id_for_nick == 0 && player_id == 0)
    {
        sprintf(query, rs_queryAddPlayer->string, name, simplified);
        mysql_real_query(&mysql, query, strlen(query));
        RS_CheckMysqlThreadError();

        player_id_for_nick = (int)mysql_insert_id(&mysql);
    }

    if (player_id != 0)
    {
        player_id_for_time = player_id;
    }
    else
    {
        player_id_for_time = player_id_for_nick;
    }

	if (rs_loadHighscores->integer)
	{
		// retrieve personal best
		sprintf(query, rs_queryGetPlayerMapHighscore->string, playerData->map_id, player_id_for_time);
		mysql_real_query(&mysql, query, strlen(query));
		RS_CheckMysqlThreadError();
		mysql_res = mysql_store_result(&mysql);
		RS_CheckMysqlThreadError();

		if ((row = mysql_fetch_row(mysql_res)) != NULL)
		{

			if (row[0]!=NULL && row[1] != NULL)
				personalBest = atoi(row[1]);
			if (row[2] != NULL)
				overall_tries = atoi(row[2]);
		}

		mysql_free_result(mysql_res);
	}

	if ( rs_loadPlayerCheckpoints->integer )
	{
	    //get player checkpoints on this map
	    sprintf(query, rs_queryGetPlayerMapCheckpoints->string, playerData->map_id, player_id_for_time);
        mysql_real_query(&mysql, query, strlen(query));
        RS_CheckMysqlThreadError();
        mysql_res = mysql_store_result(&mysql);
        RS_CheckMysqlThreadError();

        while ( (row = mysql_fetch_row(mysql_res) ) != NULL )
        {
            Q_strncatz( checkpoints, va("%s ",row[0]), sizeof( checkpoints ) );
        }

		mysql_free_result(mysql_res);

        size = strlen( checkpoints )+1;
        players_query[playerData->playerNum] = malloc( size );
        Q_strncpyz( players_query[playerData->playerNum], checkpoints, size);
	}

    /*
    if (player_id != 0 && ent->r.inuse && ent->r.client)
    {
        if (!Q_stricmp(sessionToken, "")) {

            char *newSession = RS_StartPlayerSession(player_id);
            if (newSession) {
                Q_strncpyz(sessionToken, newSession, sizeof(sessionToken));
                free(newSession);
            }
        }

        // always set him his session token... (may be lost when client changes his userinfo, so better set it immediately before changing a map etc.)
        // also maybe better do this outside the thread?
        //Info_SetValueForKey( ent->r.client->userinfo, "racesow_session", sessionToken );
        //ClientUserinfoChanged( ent, ent->r.client->userinfo );
    }
    */

    RS_PushCallbackQueue(RACESOW_CALLBACK_APPEAR, playerData->playerNum, player_id, auth_mask, player_id_for_nick, auth_mask_for_nick, personalBest, overall_tries);

	free(playerData->name);
	free(playerData->authName);
	free(playerData->authPass);
	free(playerData->authToken);
	free(playerData);
	RS_EndMysqlThread();

    return NULL;
}


// should return a new UNIQUE token for the playerId
char *RS_GenerateNewToken(int playerId)
{
    char query[MYSQL_QUERY_LENGTH];
    char *hex_output = (char *)malloc(16*2+1);
    srand((unsigned)time(NULL));

    while (qtrue)
    {
        static const char alphanum[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";

        MYSQL_RES  *mysql_res;
        int di, i;
        md5_state_t state;
        md5_byte_t digest[16];
        char str[33];
        hex_output[0] = 0;

        for (i = 0; i < 32; ++i)
        {
            str[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
        }
        str[32] = 0;

        md5_init(&state);
        md5_append(&state, (const md5_byte_t *)str, 32);
        md5_finish(&state, digest);
        for (di = 0; di < 16; ++di)
        {
	       	sprintf(hex_output + di * 2, "%02x", digest[di]);
        }

        sprintf(query, rs_queryGetPlayerAuthByToken->string, hex_output, rs_tokenSalt->string);
        mysql_real_query(&mysql, query, strlen(query));
        RS_CheckMysqlThreadError();
        mysql_res = mysql_store_result(&mysql);
        RS_CheckMysqlThreadError();
        if (mysql_fetch_row(mysql_res) == NULL)
        {
            sprintf(query, rs_querySetTokenForPlayer->string, hex_output, rs_tokenSalt->string, playerId);
            mysql_real_query(&mysql, query, strlen(query));
            RS_CheckMysqlThreadError();

            mysql_free_result(mysql_res);
            return hex_output;
        }

        mysql_free_result(mysql_res);
    }

    return NULL;
}

// should return a new UNIQUE token for the playerId
char *RS_StartPlayerSession(int playerId)
{
    char query[MYSQL_QUERY_LENGTH];
    char *hex_output = (char *)malloc(16*2+1);
    srand((unsigned)time(NULL));

    while (qtrue)
    {
        static const char alphanum[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";

        MYSQL_RES  *mysql_res;
        int di, i;
        md5_state_t state;
        md5_byte_t digest[16];
        char str[33];
        hex_output[0] = 0;

        for (i = 0; i < 32; ++i)
        {
            str[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
        }
        str[32] = 0;

        md5_init(&state);
        md5_append(&state, (const md5_byte_t *)str, 32);
        md5_finish(&state, digest);
        for (di = 0; di < 16; ++di)
        {
	       	sprintf(hex_output + di * 2, "%02x", digest[di]);
        }

        sprintf(query, rs_queryGetPlayerAuthBySession->string, hex_output);
        mysql_real_query(&mysql, query, strlen(query));
        RS_CheckMysqlThreadError();
        mysql_res = mysql_store_result(&mysql);
        RS_CheckMysqlThreadError();
        if (mysql_fetch_row(mysql_res) == NULL)
        {
            sprintf(query, rs_querySetSessionForPlayer->string, hex_output, playerId);
            mysql_real_query(&mysql, query, strlen(query));
            RS_CheckMysqlThreadError();

            mysql_free_result(mysql_res);
            return hex_output;
        }

        mysql_free_result(mysql_res);
    }

    return NULL;
}

/**
 * Calls the player disappear thread
 *
 * @param edict_t *ent
 * @param int map_id
 * @return void
 */
qboolean RS_MysqlPlayerDisappear( char *name, int playtime, int overall_tries, int racing_time, int player_id, int nick_id, int map_id, int is_authed, int is_threaded)
{
	pthread_t thread;
	int returnCode;
	struct playtimeDataStruct *playtimeData=malloc(sizeof(struct playtimeDataStruct));

    // player disappeard while using a protected nickname
    if (player_id == 0) {

        return qfalse;
    }

    playtimeData->name=strdup(name);
    playtimeData->map_id=map_id;
	playtimeData->player_id=player_id;
	playtimeData->nick_id=nick_id;
	playtimeData->playtime=playtime;
	playtimeData->is_authed=is_authed;
	playtimeData->is_threaded=is_threaded;
	playtimeData->overall_tries=overall_tries;
	playtimeData->racing_time=racing_time;

	if (!is_threaded)
	{
		RS_MysqlPlayerDisappear_Thread(playtimeData);
		return qtrue;
	}

	returnCode = pthread_create(&thread, &threadAttr, RS_MysqlPlayerDisappear_Thread, (void *)playtimeData);
	if (returnCode) {

		G_Printf("THREAD ERROR: return code from pthread_create() is %d\n", returnCode);
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
    char query[MYSQL_QUERY_LENGTH];
	char name[64];
    char servername[255];
	char simplified[64];
	struct playtimeDataStruct *playtimeData;
	int is_threaded;

	playtimeData = (struct playtimeDataStruct*)in;
	is_threaded=playtimeData->is_threaded;

	// allow unthreaded behavior to prevent a mysql crash when GT_Shutdown()
	if (is_threaded)
		RS_StartMysqlThread();
	else
		pthread_mutex_lock(&mutexsum);

    Q_strncpyz ( simplified, COM_RemoveColorTokens( playtimeData->name ), sizeof(simplified) );
	RS_EscapeString(simplified);
	Q_strncpyz ( name, playtimeData->name, sizeof(name) );
	RS_EscapeString(name);

    // increment map playtime
	sprintf(query, rs_queryUpdateMapPlaytime->string, playtimeData->playtime, playtimeData->map_id);
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();

    // increment player playtime
    sprintf(query, rs_queryUpdatePlayerPlaytime->string, playtimeData->playtime, playtimeData->player_id);
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();

    // update player map info
    sprintf(query, rs_queryUpdatePlayerMapInfo->string, playtimeData->player_id, playtimeData->map_id, playtimeData->playtime, playtimeData->overall_tries, playtimeData->racing_time);
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();

    // update the players's number of played maps
    sprintf(query, rs_queryUpdatePlayerMaps->string,  playtimeData->player_id);
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();

    // update the server's number of played maps and playtime and the hostname
    Q_strncpyz ( servername, sv_hostname->string, sizeof(servername) );
	RS_EscapeString(servername);
    sprintf(query, rs_queryUpdateServerData->string, servername, playtimeData->playtime, sv_port->integer);
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();

	free(playtimeData->name);
	free(playtimeData);

	if (is_threaded)
		RS_EndMysqlThread();
	else
		pthread_mutex_unlock(&mutexsum);


    return NULL;
}

/**
 * Request the player current protected nick
 *
 * @param player_id Id of the player making the request
 * @param playerNum Num of the player making the request
 * @return Success boolean
 */
qboolean RS_GetPlayerNick( int playerNum, int player_id )
{
    pthread_t thread;
    int returnCode;
	struct playerDataStruct *playerData=malloc(sizeof(struct playerDataStruct));

	playerData->name = NULL;
	playerData->playerNum = playerNum;
	playerData->player_id = player_id;

    returnCode = pthread_create(&thread, &threadAttr, RS_GetPlayerNick_Thread, (void *)playerData);

    if (returnCode) {

        G_Printf("THREAD ERROR: return code from pthread_create() is %d\n", returnCode);
        return qfalse;
    }

    return qtrue;
}

/**
 * Thread that calls the database and returns the player current protected nick
 *
 * @param in Input data (int player_id)
 * @return NULL on success
 */
void *RS_GetPlayerNick_Thread( void *in )
{
	struct playerDataStruct *playerData;
    char name[MAX_STRING_CHARS];
	int size;
	MYSQL_ROW  row;
    MYSQL_RES  *mysql_res;
	char query[MYSQL_QUERY_LENGTH];

	RS_StartMysqlThread();
	playerData = (struct playerDataStruct*)in;

	sprintf(query, rs_queryGetPlayerNick->string, playerData->player_id);
    mysql_real_query(&mysql, query, strlen(query));
    RS_MysqlError();
    mysql_res = mysql_store_result(&mysql);
    RS_MysqlError();

	if ((row = mysql_fetch_row(mysql_res)) != NULL)
    {
        if (row[0] !=NULL)
		{
			Q_strncpyz(name, row[0], sizeof(name));
		}

    }

	mysql_free_result(mysql_res);

	size = strlen( name )+1;
    players_query[playerData->playerNum] = malloc( size );
    Q_strncpyz( players_query[playerData->playerNum], name, size);
    RS_PushCallbackQueue(RACESOW_CALLBACK_PLAYERNICK, playerData->playerNum, 1, 0, 0, 0, 0, 0);

	free(playerData->name);
	free(playerData);

	RS_EndMysqlThread();

    return NULL;
}

/**
 * Updates the player current protected nick
 *
 * @param name Name of the player making the request
 * @param player_id Id of the player making the request
 * @param playerNum Num of the player making the request
 * @return Success boolean
 */
qboolean RS_UpdatePlayerNick( char *name, int playerNum, int player_id )
{
    pthread_t thread;
    int returnCode;
	struct playerDataStruct *playerData=malloc(sizeof(struct playerDataStruct));

    playerData->name = strdup(name);
	playerData->playerNum = playerNum;
	playerData->player_id = player_id;

    returnCode = pthread_create(&thread, &threadAttr, RS_UpdatePlayerNick_Thread, (void *)playerData);

    if (returnCode) {

        G_Printf("THREAD ERROR: return code from pthread_create() is %d\n", returnCode);
        return qfalse;
    }

    return qtrue;
}

/**
 * Thread that updates the database with the player new protected nick
 *
 * @param in Input data
 * @return NULL on success
 */
void *RS_UpdatePlayerNick_Thread( void *in )
{
	struct playerDataStruct *playerData;
	char query[MYSQL_QUERY_LENGTH];
	char name[64];
	char simplified[64];
	MYSQL_ROW  row;
    MYSQL_RES  *mysql_res;
	int size;
	unsigned int player_id_for_nick, auth_mask_for_nick;

	RS_StartMysqlThread();

    playerData = (struct playerDataStruct*)in;
	player_id_for_nick = 0;
	auth_mask_for_nick = 0;

	Q_strncpyz ( simplified, COM_RemoveColorTokens(playerData->name), sizeof(simplified) );
	RS_EscapeString(simplified);

	Q_strncpyz ( name, playerData->name, sizeof(name) );
	RS_EscapeString(name);

	// test if the wanted nick is protected
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

	mysql_free_result(mysql_res);

	// if it's already protected, stop
	if (auth_mask_for_nick > 0 && player_id_for_nick != playerData->player_id )
	{
		size = strlen( name )+1;
		players_query[playerData->playerNum] = malloc( size );
		Q_strncpyz( players_query[playerData->playerNum], name, size);
		RS_PushCallbackQueue(RACESOW_CALLBACK_PLAYERNICK, playerData->playerNum, 0, 0, 0, 0, 0, 0);

		free(playerData->name);
		free(playerData);
		RS_EndMysqlThread();
		return NULL;
	}


	// update nick
	sprintf(query, rs_queryUpdatePlayerNick->string, name, simplified, playerData->player_id);
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();

	// return confirmation of the new nick to the player
	size = strlen( name )+1;
    players_query[playerData->playerNum] = malloc( size );
    Q_strncpyz( players_query[playerData->playerNum], name, size);
    RS_PushCallbackQueue(RACESOW_CALLBACK_PLAYERNICK, playerData->playerNum, 2, 0, 0, 0, 0, 0);

	free(playerData->name);
 	free(playerData);

	RS_EndMysqlThread();

    return NULL;
}


/**
 * Mapfilter function registered in AS API.
 *
 * Calls RS_BasicMapFilter
 *
 * @param player_id Id of the player making the request
 * @param filter String used to filter maplist
 * @param page Number of the result page
 * @return Success boolean
 */
qboolean RS_MapFilter(int playerNum, char *filter, unsigned int page )
{
    pthread_t thread;
    int returnCode;
    struct filterDataStruct *filterdata=malloc(sizeof(struct filterDataStruct));
    filterdata->playerNum = playerNum;
    filterdata->filter = strdup(filter);
    filterdata->page = page;
    returnCode = pthread_create(&thread, &threadAttr, RS_MapFilter_Thread, (void *)filterdata);

    if (returnCode) {

        G_Printf("THREAD ERROR: return code from pthread_create() is %d\n", returnCode);
        return qfalse;
    }

    return qtrue;
}

/**
 * Map filter thread, parse the maplist array
 *
 * @param in Input data, cast to filterDataStruct
 * @return NULL on success
 */
void *RS_MapFilter_Thread( void *in )
{
    struct filterDataStruct *filterData = (struct filterDataStruct *)in;
    int filterCount = 0, totalPages = 0, mapCount = 0, size = 0;
    static const char *seps = " ,\n\r";
    char *s, *t;
    int page = filterData->page;
    const int MAPS_PER_PAGE = 15;
    char result[MAX_STRING_CHARS]; //stores the result string
    char filter[MAX_CONFIGSTRING_CHARS]; //stores the lowercase version of the filter
    char temp[MAX_CONFIGSTRING_CHARS]; //stores the lowercase version of the map name

	result[0]='\0';
    filter[0]='\0';
    temp[0]='\0';
    Q_strncpyz(filter,filterData->filter, sizeof( filter ) );
    Q_strlwr(filter);

    s = G_CopyString( maplist );
    t = strtok( s, seps );

    //first get the count of maps matching the search
    while( t != NULL )
    {
        Q_strncpyz( temp, t, sizeof( temp ) );
        Q_strlwr( temp );

        if ( !( strstr( temp, filter ) == NULL ) )
            filterCount++;

        t = strtok( NULL, seps);
    }

    G_Free( s );

    if ( filterCount == 0 )
        Q_strncatz( result, va( "No maps found for your search on %s%s.\n", S_COLOR_YELLOW, filterData->filter ), sizeof( result ) );

    else
    {
        //dirty hack to compute the number of pages, should use ceil function
        if ( (filterCount/MAPS_PER_PAGE)*MAPS_PER_PAGE == filterCount )
            totalPages = filterCount/MAPS_PER_PAGE;
        else
            totalPages = filterCount/MAPS_PER_PAGE + 1;

        if ( page > totalPages )
        {
            Q_strncatz( result , va( "There are only%s %d%s pages for your search on%s %s%s\n",
                    S_COLOR_YELLOW,
                    totalPages,
                    S_COLOR_WHITE,
                    S_COLOR_YELLOW,
                    filterData->filter,
                    S_COLOR_WHITE ), sizeof ( result ) );
    }
    else
    {
        Q_strncatz( result, va( "Printing page %d/%d of maps matching %s%s.\n%sUse %smapfilter %s <pagenum> %sto print other pages.\n",
                page,
                totalPages,
                S_COLOR_YELLOW,
                filterData->filter,
                S_COLOR_WHITE,
                S_COLOR_YELLOW,
                filterData->filter,
                S_COLOR_WHITE),  sizeof(result));

            s = G_CopyString( maplist );
            t = strtok( s, seps );
            filterCount = 0;

            while( ( t != NULL ) && ( filterCount <= page*MAPS_PER_PAGE ) )
            {
                mapCount++;
                Q_strncpyz( temp, t, sizeof( temp ) );
                Q_strlwr( temp );
                if ( !( strstr( temp, filter ) == NULL ) )
                {
                    filterCount++;
                    if ( ( filterCount >= ((page-1)*MAPS_PER_PAGE + 1) ) && ( filterCount <= page*MAPS_PER_PAGE  ) )
                        Q_strncatz( result, va( "%s#%4d%s : %s\n", S_COLOR_ORANGE, mapCount, S_COLOR_WHITE, t ),  sizeof( result ) );
        }

                t = strtok( NULL, seps);
        }

            G_Free( s );
    }
    }

    size = strlen( result )+1;
    players_query[filterData->playerNum] = malloc( size );
    Q_strncpyz( players_query[filterData->playerNum], result, size);
    RS_PushCallbackQueue(RACESOW_CALLBACK_MAPFILTER, filterData->playerNum, filterCount, 0, 0, 0, 0, 0);

    free(filterData->filter);
    free(filterData);
    RS_EndMysqlThread();

    return NULL;
}

/**
 * Load stats requested from AS api
 *
 * @param playerNum playerNum making the request
 * @param what map or player
 * @param which name for map or player
 * @return Success boolean
 */
qboolean RS_LoadStats(int playerNum, char *what, char *which)
{
    pthread_t thread;
    int returnCode;
    struct statsRequest_t *statsRequest=malloc(sizeof(struct statsRequest_t));
    statsRequest->playerNum = playerNum;
    statsRequest->what = strdup(what);
    statsRequest->which = strdup(which);
    returnCode = pthread_create(&thread, &threadAttr, RS_LoadStats_Thread, (void *)statsRequest);

    if (returnCode) {

        G_Printf("THREAD ERROR: return code from pthread_create() is %d\n", returnCode);
        return qfalse;
    }

    return qtrue;
}


/**
 * Load stats thread
 *
 * @return NULL on success
 */
void *RS_LoadStats_Thread( void *in )
{
    struct statsRequest_t *statsRequest = (struct statsRequest_t *)in;
    char result[MAX_STRING_CHARS];
    char query[MYSQL_QUERY_LENGTH];
    char which[64];
    MYSQL_ROW  row;
    MYSQL_RES  *mysql_res;
    int size = 0;
	result[0]='\0';

    RS_StartMysqlThread();

    if (!Q_stricmp(statsRequest->what, "map"))
    {
        int agMilli, agHour, agMin, btMilli, btMin, btSec, wtMilli, wtMin, wtSec, ptMilli, ptHour, ptMin, avgTries;
        agMilli = 0;
        agHour = 0;
        agMin = 0;
        btMilli = 0;
        btMin = 0;
        btSec = 0;
        wtMilli = 0;
        wtMin = 0;
        wtSec = 0;
        ptMilli = 0;
        ptHour = 0;
        ptMin = 0;
        avgTries = 0;

        Q_strncpyz( which, COM_RemoveColorTokens(statsRequest->which), sizeof( which ) );
		RS_EscapeString(which);
        sprintf(query, rs_queryGetMapStats->string, which);
        mysql_real_query(&mysql, query, strlen(query));
        RS_CheckMysqlThreadError();
        mysql_res = mysql_store_result(&mysql);
        RS_CheckMysqlThreadError();
        if ((row = mysql_fetch_row(mysql_res)) != NULL)
        {
            if (row[0]!=NULL)
            {
                 //  0        1        2            3                  4            5              6           7           8            9             10
                // `type`, `races`, `tries`, `avg_overall_tries`, `avg_tries`, `avg_duration`, `playtime`, `created`, `best_time`, `worst_time`, `players`

                    if (row[4] != NULL) {

                        avgTries = atoi(row[4]);
                    }

                    if (row[5] != NULL) {
                        agMilli = atoi( row[5] );
                        agHour = agMilli / 3600000;
                        agMilli -= agHour * 3600000;
                        agMin = agMilli / 60000 + 1;
                    }

                    if (row[6] != NULL) {
                        ptMilli = atoi( row[6] );
                        ptHour = ptMilli / 3600000;
                        ptMilli -= ptHour * 3600000;
                        ptMin = ptMilli / 60000 + 1;
                    }

                    if (row[8] != NULL) {
                        btMilli = atoi( row[8] );
                        btMin = btMilli / 60000;
                        btMilli -= btMin * 60000;
                        btSec = btMilli / 1000;
                        btMilli -= btSec * 1000;
                    }

                    if (row[9] != NULL) {
                        wtMilli = atoi( row[9] );
                        wtMin = wtMilli / 60000;
                        wtMilli -= wtMin * 60000;
                        wtSec = wtMilli / 1000;
                        wtMilli -= wtSec * 1000;
                    }

                    Q_strncatz( result, va( "%sStats for %s:\n", S_COLOR_YELLOW, statsRequest->which ), sizeof( result ) );
                    if (row[0]!=NULL)
                        Q_strncatz( result, va( "%sMap type: %s%s\n", S_COLOR_ORANGE, S_COLOR_WHITE, row[0] ), sizeof( result ) );
                    if (row[7]!=NULL)
                        Q_strncatz( result, va( "%sAvaiable since: %s%s\n", S_COLOR_ORANGE, S_COLOR_WHITE, row[7] ), sizeof( result ) );
                    Q_strncatz( result, va( "%sPlaytime: %s%d hours %d minutes\n", S_COLOR_ORANGE, S_COLOR_WHITE, ptHour, ptMin ), sizeof( result ) );
                    if (row[10]!=NULL)
                        Q_strncatz( result, va( "%sNumber of players: %s%d\n", S_COLOR_ORANGE, S_COLOR_WHITE, atoi(row[10]) ), sizeof( result ) );
                    if (row[1]!=NULL)
                        Q_strncatz( result, va( "%sFinished races: %s%d\n", S_COLOR_ORANGE, S_COLOR_WHITE, atoi(row[1])), sizeof( result ) );
                    if (row[2]!=NULL)
                        Q_strncatz( result, va( "%sStarted races (overall tries): %s%d\n", S_COLOR_ORANGE, S_COLOR_WHITE, atoi(row[2])), sizeof( result ) );
                    if (row[3]!=NULL)
                        Q_strncatz( result, va( "%sAvg. started races: %s%d\n", S_COLOR_ORANGE, S_COLOR_WHITE, atoi(row[3])), sizeof( result ) );
                    Q_strncatz( result, va( "%sAvg. tries to personal best: %s%d\n", S_COLOR_ORANGE, S_COLOR_WHITE, avgTries), sizeof( result ) );
                    Q_strncatz( result, va( "%sAvg. duration to personal Best: %s%d hours %d minutes\n", S_COLOR_ORANGE, S_COLOR_WHITE, agHour, agMin), sizeof( result ) );
                    Q_strncatz( result, va( "%sBest personal record: %s%d:%d:%03d\n", S_COLOR_ORANGE, S_COLOR_WHITE, btMin, btSec, btMilli), sizeof( result ) );
                    Q_strncatz( result, va( "%sWorst personal record: %s%d:%d:%03d\n", S_COLOR_ORANGE, S_COLOR_WHITE, wtMin, wtSec, wtMilli), sizeof( result ) );
            }
        }
        else
        {
            Q_strncatz( result, va( "%sError: map '%s' not found\n", S_COLOR_RED, statsRequest->which ), sizeof( result ) );
        }

        mysql_free_result(mysql_res);
    }
    else if (!Q_stricmp(statsRequest->what, "player"))
    {
        Q_strncpyz( which, COM_RemoveColorTokens(statsRequest->which), sizeof( which ) );
		RS_EscapeString(which);
        sprintf(query, rs_queryGetPlayerStats->string, which);
        mysql_real_query(&mysql, query, strlen(query));
        RS_CheckMysqlThreadError();
        mysql_res = mysql_store_result(&mysql);
        RS_CheckMysqlThreadError();
        if ((row = mysql_fetch_row(mysql_res)) != NULL)
        {
            int oHour, oMin, oMilli, rHour, rMin, rMilli;
            oHour = 0;
            oMin = 0;
            oMilli = 0;
            rHour = 0;
            rMin = 0;
            rMilli = 0;

            //   0        1               2         3           4        5            6             7                  8
            // `points`, `diff_points`, `races`, `race_tries`, `maps`, `playtime`, `racing_time`, `first_seen`,  `last_seen`

            if (row[5] != NULL) {
                oMilli = atoi( row[5] );
                oHour = oMilli / 3600000;
                oMilli -= oHour * 3600000;
                oMin = oMilli / 60000 + 1;
            }

            if (row[6] != NULL) {
                rMilli = atoi( row[6] );
                rHour = rMilli / 3600000;
                rMilli -= rHour * 3600000;
                rMin = rMilli / 60000 + 1;
            }

            Q_strncatz( result, va( "%sStats for %s:\n", S_COLOR_YELLOW, statsRequest->which ), sizeof( result ) );
            if (row[0]!=NULL && row[1]!=NULL)
                Q_strncatz( result, va( "%sPoints: %s%d (%s%d)\n", S_COLOR_ORANGE, S_COLOR_WHITE, atoi(row[0]), (atoi(row[1]) < 0 ? "" : "+"), atoi(row[1])), sizeof( result ) );
            if (row[2]!=NULL)
                Q_strncatz( result, va( "%sFinished races: %s%d\n", S_COLOR_ORANGE, S_COLOR_WHITE, atoi(row[2]) ), sizeof( result ) );
            if (row[3]!=NULL)
                Q_strncatz( result, va( "%sStarted races: %s%d\n", S_COLOR_ORANGE, S_COLOR_WHITE, atoi(row[3]) ), sizeof( result ) );
            if (row[4]!=NULL)
                Q_strncatz( result, va( "%sPlayed maps: %s%d\n", S_COLOR_ORANGE, S_COLOR_WHITE, atoi(row[4]) ), sizeof( result ) );
                Q_strncatz( result, va( "%sOnline time: %s%d hours %d minutes \n", S_COLOR_ORANGE, S_COLOR_WHITE, oHour, oMin ), sizeof( result ) );
                Q_strncatz( result, va( "%sRacing time: %s%d hours %d minutes\n", S_COLOR_ORANGE, S_COLOR_WHITE, rHour, rMin ), sizeof( result ) );
            if (row[7]!=NULL)
                Q_strncatz( result, va( "%sFirst seen: %s%s\n", S_COLOR_ORANGE, S_COLOR_WHITE, row[7] ), sizeof( result ) );
            if (row[8]!=NULL)
                Q_strncatz( result, va( "%sLast seen: %s%s\n", S_COLOR_ORANGE, S_COLOR_WHITE, row[8] ), sizeof( result ) );
        }
        else
        {
            Q_strncatz( result, va( "%sError: player '%s' not found\n", S_COLOR_RED, statsRequest->which ), sizeof( result ) );
        }

        mysql_free_result(mysql_res);
    }
    else
    {
        Q_strncatz( result, va( "%sError: %invalid stats type: '%s' %s %s\n", S_COLOR_RED, S_COLOR_WHITE, statsRequest->what ), sizeof( result ) );
    }

    //ent = &game.edicts[ playerData->playerNum + 1 ];

    /*

    */


    size = strlen( result )+1;
    players_query[statsRequest->playerNum] = malloc( size );
    Q_strncpyz( players_query[statsRequest->playerNum], result, size);
    RS_PushCallbackQueue(RACESOW_CALLBACK_MAPFILTER, statsRequest->playerNum, 0, 0, 0, 0, 0, 0);

    free(statsRequest->what);
    free(statsRequest->which);
    free(statsRequest);
    RS_EndMysqlThread();

    return NULL;
}

/**
 * This function is called from angelscript when callbacks of queries returning
 * a string are popped out of the callback queue.
 * Mainly : RACESOW_CALLBACK_HIGHSCORES/MAPLIST/MAPFILTER
 *
 * @param player_id Id of the player making the request
 * @return the result string to print to the player
 */
char *RS_PrintQueryCallback(int player_id )
{
    unsigned int size;
    char *result;

    if ( players_query[player_id] != NULL )
    {
        size = strlen(players_query[player_id])+1;
        result=malloc(size);
        Q_strncpyz(result, players_query[player_id] , size);
        free(players_query[player_id]);
        players_query[player_id] = NULL;
        return result;
    }
    else
    {
        return NULL;
    }

}

/**
 * Maplist function registered in AS API.
 *
 * Calls RS_Maplist
 *
 * @param player_id Id of the player making the request
 * @param Page number of the result page
 * @return Success boolean
 */
qboolean RS_Maplist(int playerNum, unsigned int page)
{
    pthread_t thread;
    int returnCode;
    struct maplistDataStruct *maplistdata=malloc(sizeof(struct maplistDataStruct));
    maplistdata->playerNum = playerNum;
    maplistdata->page = page;
    returnCode = pthread_create(&thread, &threadAttr, RS_Maplist_Thread, (void *)maplistdata);

    if (returnCode) {

        G_Printf("THREAD ERROR: return code from pthread_create() is %d\n", returnCode);
        return qfalse;
    }

    return qtrue;
}

/**
 * Maplist thread, parse the maplist array
 *
 * @param in Input data, cast to maplistDataStruct
 */
void *RS_Maplist_Thread(void *in)
    {
    struct maplistDataStruct *maplistData = (struct maplistDataStruct *)in ;
    int mapsFound = 0, totalPages = 0, size = 0;
    int page = maplistData->page;
    static const char *seps = " ,\n\r";
    char *s, *t;
    const int MAPS_PER_PAGE = 20;
    char result[MAX_STRING_CHARS]; //stores the result string
    result[0]='\0';

    if ( (mapcount/MAPS_PER_PAGE)*MAPS_PER_PAGE == mapcount )
        totalPages = mapcount/MAPS_PER_PAGE;
    else
        totalPages = mapcount/MAPS_PER_PAGE + 1;

    if ( ( page > totalPages ) || (page < 1 ) )
    {
        Q_strncatz( result , va( "You should enter a page number between%s %d%s and%s %d%s\n",
                     S_COLOR_YELLOW,
                1,
                     S_COLOR_WHITE,
                     S_COLOR_YELLOW,
                totalPages,
                     S_COLOR_WHITE),  sizeof(result));
    }
    else
    {
        Q_strncatz( result, va( "Printing page%s %d/%d%s \n",
                S_COLOR_YELLOW,
                page,
                totalPages,
                S_COLOR_YELLOW ),  sizeof( result ) );

        s = G_CopyString( maplist );
        t = strtok( s, seps );

        while( ( t != NULL ) && ( mapsFound <= page*MAPS_PER_PAGE ) )
        {
            mapsFound++;
            if ( ( mapsFound >= ((page-1)*MAPS_PER_PAGE + 1) ) && ( mapsFound <= page*MAPS_PER_PAGE  ) )
                Q_strncatz( result, va( "%s#%4d%s : %s\n", S_COLOR_ORANGE, mapsFound, S_COLOR_WHITE, t ),  sizeof( result ) );

            t = strtok( NULL, seps);
        }

        G_Free( s );
    }

    size = strlen( result )+1;
    players_query[maplistData->playerNum] = malloc( size );
    Q_strncpyz( players_query[maplistData->playerNum], result, size);
    RS_PushCallbackQueue(RACESOW_CALLBACK_MAPLIST, maplistData->playerNum, 0, 0, 0, 0, 0, 0);

    free(maplistData);
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
qboolean RS_MysqlLoadHighscores( int playerNum, int  limit, int map_id, char *mapname, pjflag prejumpFlag)
{
	pthread_t thread;
	int returnCode;

	struct highscoresDataStruct *highscoresData=malloc(sizeof(struct highscoresDataStruct));

    highscoresData->map_id=map_id;
	highscoresData->playerNum=playerNum;
	highscoresData->mapname = strdup(mapname);
	highscoresData->limit = limit;
	highscoresData->prejumpflag = prejumpFlag;

	returnCode = pthread_create(&thread, &threadAttr, RS_MysqlLoadHighscores_Thread, (void *)highscoresData);
	if (returnCode) {

		G_Printf("THREAD ERROR: return code from pthread_create() is %d\n", returnCode);
		return qfalse;
	}

	return qtrue;
}


//=================
//RS_MysqlLoadHighScores
// straight from 0.42 with some changes
//=================

void *RS_MysqlLoadHighscores_Thread( void* in ) {

		MYSQL_ROW  row;
        MYSQL_RES  *mysql_res;
        char query[MYSQL_QUERY_LENGTH];
		char oneliner[100];
		char pjoneliner[100];
		int playerNum;
		int map_id;
		int limit;
		int mapNumber;
		char highscores[10000];
		struct highscoresDataStruct *highscoresData;
		char *mapname;
		char *prejumpflag;
		qboolean prejumped;

		highscoresData = (struct highscoresDataStruct *)in;
		playerNum = highscoresData->playerNum;
		limit = highscoresData->limit;
		mapname = strdup(highscoresData->mapname);
		prejumped = qfalse;

		switch (highscoresData->prejumpflag)
		{
		    case RS_PREJUMPED :
		        prejumpflag = "'true'";
		        break;
		    case RS_NOTPREJUMPED :
		        prejumpflag = "'false'";
		        break;
		    case RS_BOTH :
		    default :
		        prejumpflag = "'true','false'";
                break;
		}

		RS_StartMysqlThread();

		//if a mapname was provided we must use it
		if ( mapname != NULL && strlen(mapname) > 0)
		{
		    mapNumber = atoi( mapname );
		    if( !Q_stricmp( mapname, va( "%i", mapNumber ) ) )
		    {
		        //a mapnumber was given, get the corresponding mapname in maplist
				//first, free the old mapname, as a new one is malloc'd in RS_GetMapByNum
				free(mapname);
		        mapname = RS_GetMapByNum( mapNumber );
		    }

		    //get the map_id corresponding to the mapname
		    sprintf(query, rs_queryGetMap->string, mapname);
		    mysql_real_query(&mysql, query, strlen(query));
	        RS_CheckMysqlThreadError();
	        mysql_res = mysql_store_result(&mysql);
	        RS_CheckMysqlThreadError();

	        if ( (row = mysql_fetch_row(mysql_res)) != NULL )
	            map_id = atoi(row[0]);
	        else
	            map_id = 0;

	        mysql_free_result(mysql_res);
		}
		else //use given map_id (current map id)
		{
		    mapname = malloc(strlen(level.mapname)+1);
		    Q_strncpyz(mapname,level.mapname,strlen(level.mapname)+1);
		    map_id = highscoresData->map_id;
		}

		// get map oneliners (may not be present)
		oneliner[0]='\0';
		pjoneliner[0]='\0';
		sprintf(query, rs_queryLoadMapOneliners->string, map_id);
		mysql_real_query(&mysql, query, strlen(query));
        RS_CheckMysqlThreadError();
        mysql_res = mysql_store_result(&mysql);
        RS_CheckMysqlThreadError();
		if ((row = mysql_fetch_row(mysql_res)) != NULL)
	    {
		    if (row[0] !=NULL)
			{
				if ( strlen(row[0]) > 0 )
				{
						Q_strncpyz(oneliner, va("\"%s\"", row[0]), sizeof(oneliner));
				}
			}
			if (row[1] !=NULL)
			{
                if ( strlen(row[1]) > 0 )
                {
                        Q_strncpyz(pjoneliner, va("\"%s\"", row[1]), sizeof(pjoneliner));
                }
			}
	    }
		mysql_free_result(mysql_res);

        // get top players on map
		sprintf(query, rs_queryLoadMapHighscores->string, map_id, prejumpflag, limit);
        mysql_real_query(&mysql, query, strlen(query));
        RS_CheckMysqlThreadError();
        mysql_res = mysql_store_result(&mysql);
        RS_CheckMysqlThreadError();

		highscores[0]='\0';

        if( (unsigned long)mysql_num_rows( mysql_res ) == 0 )
            Q_strncatz(highscores, va( "%sNo highscores found yet!\n", S_COLOR_RED ), sizeof(highscores));

        else {
            unsigned int position = 0;
            unsigned int last_position = 0;
            unsigned int draw_position = 0;
            unsigned int cleanBest = 0;
            unsigned int pjBest = 0;
            char last_time[16];
            char draw_time[16];
            char diff_time[16];
            int min, sec, milli, dmin, dsec, dmilli;
			int replay_record;

			replay_record=0;
			last_time[0]='\0';
			draw_time[0]='\0';
			diff_time[0]='\0';


            Q_strncatz(highscores, va( "%sTop %d players on map '%s'%s\n", S_COLOR_ORANGE, limit, mapname, S_COLOR_WHITE ), sizeof(highscores));

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
                //check is the time is prejumped
                if ( !Q_stricmp(row[3], "true") )
                    prejumped = qtrue;
                else
                    prejumped = qfalse;

                if (!prejumped && cleanBest == 0) //update the positions
                    cleanBest = position;
                if (prejumped && pjBest == 0)
                    pjBest = position;

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

				Q_strncatz( highscores, va( "%s%3d. %s%6s  %s[%s]  %s %s  %s(%s) %s%s%s\n", S_COLOR_WHITE, draw_position, prejumped?S_COLOR_RED:S_COLOR_GREEN, draw_time, S_COLOR_YELLOW, diff_time, S_COLOR_WHITE, row[1], S_COLOR_WHITE, row[2], S_COLOR_YELLOW, ( position == pjBest ? pjoneliner : "" ), ( position == cleanBest ? oneliner : "" ) ), sizeof(highscores) );
			}
        }

        mysql_free_result(mysql_res);

        players_query[playerNum]=malloc(strlen(highscores)+1);
        players_query[playerNum][0]='\0';
        Q_strncpyz( players_query[playerNum],highscores, strlen(highscores) + 1);

		RS_PushCallbackQueue(RACESOW_CALLBACK_HIGHSCORES, playerNum, 0, 0, 0, 0, 0, 0);

		free(highscoresData->mapname);
		free(mapname);
		free(highscoresData);
		RS_EndMysqlThread();
		return NULL;
}

/**
 * Set a map oneliner
 *
 * @return Success boolean
 */
qboolean RS_MysqlSetOneliner( int playerNum, int player_id, int map_id, char *oneliner)
{
    pthread_t thread;
    int returnCode;
	struct onelinerDataStruct *onelinerData=malloc(sizeof(struct onelinerDataStruct));

    onelinerData->oneliner = strdup(oneliner);
	onelinerData->playerNum = playerNum;
	onelinerData->player_id = player_id;
	onelinerData->map_id = map_id;

    returnCode = pthread_create(&thread, &threadAttr, RS_MysqlSetOneliner_Thread, (void *)onelinerData);

    if (returnCode) {

        G_Printf("THREAD ERROR: return code from pthread_create() is %d\n", returnCode);
        return qfalse;
    }

    return qtrue;
}

/**
 * Thread that updates the database with the player new protected nick
 *
 * @param in Input data
 * @return NULL on success
 */
void *RS_MysqlSetOneliner_Thread( void *in )
{
	struct onelinerDataStruct *onelinerData;
	char query[MYSQL_QUERY_LENGTH];
	char response[1024];
	MYSQL_ROW  row;
    MYSQL_RES  *mysql_res;
	int size;
	int failure;
	int best_player_id;
	char old_oneliner[100];
	char oneliner[100];
	qboolean prejumped;

	RS_StartMysqlThread();

    onelinerData = (struct onelinerDataStruct*)in;
	best_player_id = 0;
	failure = 0;
	prejumped = qfalse;

    // read current player record to know if it was prejumped or not
    sprintf(query, rs_queryGetPlayerMap->string, onelinerData->player_id, onelinerData->map_id);
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();
    mysql_res = mysql_store_result(&mysql);
    RS_CheckMysqlThreadError();
    if ((row = mysql_fetch_row(mysql_res)) != NULL) {
        if (row[0] != NULL && row[1] != NULL)
        {
            if ( !Q_stricmp(row[2],"true") )
                prejumped = qtrue;
            else
                prejumped = qfalse;
        }
    }
    mysql_free_result(mysql_res);

	// retrieve server best
	sprintf(query, rs_queryGetPlayerMapHighscores->string, onelinerData->map_id, prejumped?"'true','false'":"'false'");
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();
    mysql_res = mysql_store_result(&mysql);
    RS_CheckMysqlThreadError();
    if ((row = mysql_fetch_row(mysql_res)) != NULL)
	{
		if (row[0] !=NULL && row[1] != NULL)
		{
            best_player_id = atoi(row[0]);
		}
	}
	mysql_free_result(mysql_res);

	// test if the player is allowed to set the oneliner, else stop
	if ( best_player_id == 0 || best_player_id != onelinerData->player_id )
	{
	    Q_strncpyz( response, "You need to be #1 on this map to set a oneliner.\n", sizeof(response));
		failure = 1;
	}
	else
	{

		// testing if the player has already set a oneliner
		// retrieve the existing oneliner
		old_oneliner[0]='\0';
		sprintf(query, rs_queryLoadMapOneliners->string, onelinerData->map_id);
		mysql_real_query(&mysql, query, strlen(query));
        RS_CheckMysqlThreadError();
        mysql_res = mysql_store_result(&mysql);
        RS_CheckMysqlThreadError();
		if ((row = mysql_fetch_row(mysql_res)) != NULL)
	    {
		    if ( !prejumped && row[0] !=NULL)
			{
				Q_strncpyz(old_oneliner, row[0], sizeof(old_oneliner));
			}
            if ( prejumped && row[1] !=NULL)
            {
                Q_strncpyz(old_oneliner, row[1], sizeof(old_oneliner));
            }
	    }

		mysql_free_result(mysql_res);

		if ( strlen(old_oneliner) > 0 )
		{
			Q_strncpyz( response, "You have already set a oneliner for this map, it's written in stone now!\n", sizeof(response));
			failure=1;
		}
	}

	// return a failure message
	if ( failure )
	{
		size = strlen( response )+1;
		players_query[onelinerData->playerNum] = malloc( size );
		Q_strncpyz( players_query[onelinerData->playerNum], response, size);
		RS_PushCallbackQueue(RACESOW_CALLBACK_ONELINER, onelinerData->playerNum, 0, 0, 0, 0, 0, 0);

		free(onelinerData->oneliner);
		free(onelinerData);
		RS_EndMysqlThread();
		return NULL;
	}

	// update the new oneliner
	Q_strncpyz ( oneliner, onelinerData->oneliner, sizeof(oneliner) );
	RS_EscapeString(oneliner);
	sprintf(query, rs_querySetMapOneliner->string, prejumped?"pj_oneliner":"oneliner", oneliner, onelinerData->map_id);
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();

	// return confirmation to the player (needed, because the command is waiting for a callback)
	Q_strncpyz( response, va("Oneliner successfully set to: %s\n", oneliner), sizeof(response));
	size = strlen( response )+1;
    players_query[onelinerData->playerNum] = malloc( size );
    Q_strncpyz( players_query[onelinerData->playerNum], response, size);
    RS_PushCallbackQueue(RACESOW_CALLBACK_ONELINER, onelinerData->playerNum, 0, 0, 0, 0, 0, 0);

	free(onelinerData->oneliner);
 	free(onelinerData);

	RS_EndMysqlThread();

    return NULL;
}


/**
 * Test if a map name is valid and print some warnings if it's not.
 */
qboolean RS_MapValidate( char *mapname)
{
    char local[MAX_CONFIGSTRING_CHARS];
    local[0]='\0';

    if( strlen( "maps/" ) + strlen( mapname ) + strlen( ".bsp" ) >= MAX_CONFIGSTRING_CHARS )
    {
        G_Printf("%sWarning: %s%s, map name is too long\n", S_COLOR_RED, S_COLOR_WHITE, mapname );
        return qfalse;
    }

    Q_strncpyz( local, mapname, sizeof( local ) );
    COM_SanitizeFilePath( local );

    if( !COM_ValidateRelativeFilename( local ) || strchr( local, '/' ) || strchr( local, '.' ) )
    {
        G_Printf("%sWarning: %s%s, invalid map-name\n", S_COLOR_RED, S_COLOR_WHITE, local );
        return qfalse;
    }

    if ( !trap_ML_FilenameExists( local ) )
    {
        G_Printf("%sWarning: %s%s, map does not exist\n", S_COLOR_RED, S_COLOR_WHITE, local );
        return qfalse;
    }

    return qtrue;
}

/**
 * Load Maplist from mysql database.
 *
 * maplist is loaded only once at the beginning of a map, so we don't really
 * need to thread it, also because threads dont support string output.
 *
 * @param is_freestyle
 * @return success boolean
 */
qboolean RS_MysqlLoadMaplist( int is_freestyle )
{
        MYSQL_ROW  row;
        MYSQL_RES  *mysql_res;
        char query[MYSQL_QUERY_LENGTH];
        mapcount = 0;
        maplist[0] = '\0';

        sprintf(query, rs_queryLoadMapList->string, is_freestyle, "name");
        mysql_real_query(&mysql, query, strlen(query));
       	if (RS_MysqlError()) {
            return qfalse;
        }

        mysql_res = mysql_store_result(&mysql);
       	if (RS_MysqlError()) {
            return qfalse;
        }

       	while( ( row = mysql_fetch_row( mysql_res ) ) ) {

            if ( !RS_MapValidate( row[0] ) )
       	        continue;

       	    Q_strncatz( maplist, va( "%s ", row[0] ), sizeof( maplist ) );
            mapcount++;
            }

        mysql_free_result(mysql_res);
        return qtrue;
}

/**
 * Load the maplist from a given string map list
 *
 * @param stringMapList a list of map names separated with spaces
 * @return boolean success
 */
qboolean RS_BasicLoadMaplist(char *stringMapList)
{
    char *s, *t;
    static const char *seps = " ,\n\r";
    maplist[0] = '\0';
    mapcount = 0;

    s = G_CopyString( stringMapList );
    t = strtok( s, seps );

    while( t != NULL )
    {
        if ( RS_MapValidate( t ) )
        {
            Q_strncatz( maplist, va("%s ", t), sizeof( maplist ) );
        mapcount++;
        }

        t = strtok( NULL, seps);
    }

    G_Free( s );
    return qtrue;
}

/**
 * General maplist load function
 */
void RS_LoadMaplist( int is_freestyle)
{
    if ( g_enforce_map_pool->integer )
    {
        RS_BasicLoadMaplist( g_map_pool->string );
    }
    else if ( MysqlConnected )
    {
        RS_MysqlLoadMaplist( is_freestyle );
    }
    else
    {
        RS_BasicLoadMaplist( g_maplist->string );
    }

    G_Printf( "Maplist loaded : %u maps found.\n", mapcount );
}

/**
 * Start the mysql thread
 *
 * @param void
 * @return void
 */
void RS_StartMysqlThread()
{
    int errNo;
    unsigned long threadId;

    pthread_mutex_lock(&mutexsum);

    threadId = mysql_thread_id(&mysql);
    errNo = mysql_ping(&mysql);

    if (mysql_thread_id(&mysql) != threadId) {

        G_Printf("-------------------------------------\nReconnected to Mysql server\n-------------------------------------\n");
    }


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

char *RS_ChooseNextMap()
{
    edict_t *ent = NULL;
     char *s, *t, *f;
     static const char *seps = " ,\n\r";

     if( *level.forcemap )
    {
        return level.forcemap;
    }

     if( !( *maplist ) || strlen( maplist ) == 0 || g_maprotation->integer == 0 )
    {
        //same map again
        return level.mapname;
    }
     else if( g_maprotation->integer == 1 )
     {
         // next map in list
         s = G_CopyString( maplist );
         f = NULL;
         t = strtok( s, seps );

         while( t != NULL )
    {
             if( !Q_stricmp( t, level.mapname ) )
        {
                 // it's in the list, go to the next one
                 t = strtok( NULL, seps );
                 if( t == NULL )
                 { // end of list, go to first one
                     if( f == NULL )// there isn't a first one, same level
            {
                         G_Free( s );
                         return level.mapname;
                     }
                else
                {
                         G_Free( s );
                         return f;
                }
            }
                 else
                 {
                     G_Free( s );
                     return t;
        }
             }

             if( !f )
                 f = t;

             t = strtok( NULL, seps );
        }

         // not in the list, we go for the first one
         G_Free( s );
         return f;
    }
     else if( g_maprotation->integer == 2 )
     {
         // random from the list, but not the same
         int count = 0;
         s = G_CopyString( maplist );

         t = strtok( s, seps );
         while( t != NULL )
    {
             if( Q_stricmp( t, level.mapname ) )
                 count++;
             t = strtok( NULL, seps );
         }

         G_Free( s );
         s = G_CopyString( maplist );

         if( count < 1 )
         {
             // no other maps found, restart
             G_Free( s );
             return level.mapname;
         }
         else
         {
            int seed = game.realtime;
             count -= (int)Q_brandom( &seed, 0, count ); // this should give random integer from 0 to count-1
             ent = NULL; // shutup compiler warning;

             t = strtok( s, seps );
             while( t != NULL )
            {
                 if( Q_stricmp( t, level.mapname ) )
                {
                     count--;
                     if( count == 0 )
                     {
                         G_Free( s );
                         return t;
                         break;
                }
            }
                 t = strtok( NULL, seps );
    }
         }
     }

    if( level.nextmap[0] )  // go to a specific map
        return level.nextmap;

    // search for a changelevel
    ent = G_Find( NULL, FOFS( classname ), "target_changelevel" );
    if( !ent )
    {
        // the map designer didn't include a changelevel,
         // so create a fake ent that goes back to the same level
        return level.mapname;
    }
    return ent->map;
}

char *RS_GetMapByNum(int num)
{
    char *s, *tok;
    char *result;
    int size = 0;
    static const char *seps = " ,\n\r";
    int maps = 0;

    s = G_CopyString( maplist );
    tok = strtok( s, seps );

    while ( tok != NULL )
    {
        maps++;
        if ( maps == num )
        {
            size = strlen(tok)+1;
            result = malloc(size);
            Q_strncpyz(result, tok, size);
            G_Free( s );
            return result;
        }
        else
            tok = strtok( NULL, seps );
    }

    G_Free( s );
    return NULL;
}

static void RS_Irc_ConnectedListener_f( void *connected )
{
    qboolean* c;
    trap_Dynvar_GetValue( irc_connected, (void**) &c );
    if( *c )
    {
        ircConnected = 1;
    }
    else
    {
        ircConnected = 0;
        G_Printf("%sWarning:%s IRC disconnected\n", S_COLOR_RED, S_COLOR_WHITE);
    }
}

void RS_ircSendMessage( const char *name,  const char *text )
{
    if( ircConnected )
        trap_Cmd_ExecuteText( EXEC_APPEND, va( "irc_chanmsg %s: %s\n", name , text ) );
}

void RS_ircPrint( const char *format, ... )
{
    const char *nick = trap_Cmd_Argv( 1 );
    char *msg = trap_Cmd_Argv( 2 );
    char *s;
    s = va( "%s %s \"%s\n\"", "tvch", nick, msg );

    trap_GameCmd( NULL, s );
}

static void RS_Cmd_ircPrint_f( void )
{
    RS_ircPrint( trap_Cmd_Args() );
}

void RS_AddServerCommands( void )
{
    if( rs_IRCstream->integer )
        trap_Cmd_AddCommand( "ircprint", RS_Cmd_ircPrint_f );
}

void RS_RemoveServerCommands( void )
{
    if( rs_IRCstream->integer )
        trap_Cmd_RemoveCommand( "ircprint" );
}
