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

cvar_t *rs_queryGetPlayer;
cvar_t *rs_queryAddPlayer;
cvar_t *rs_queryAddNick;
cvar_t *rs_queryGetMap;
cvar_t *rs_queryAddMap;
cvar_t *rs_queryAddRace;


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

/**
 * Initializes racesow specific stuff
 *
 * @return void
 */
void RS_Init()
{
	// initialize threading
    pthread_mutex_init(&mutexsum, NULL);
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
    
    rs_queryGetPlayer			= trap_Cvar_Get( "rs_queryGetPlayer",			"SELECT `id`, `auth_mask` FROM `player` WHERE `auth_name` = '%s' AND `auth_pass` = MD5('%s') LIMIT 1;", CVAR_ARCHIVE );
	rs_queryAddPlayer			= trap_Cvar_Get( "rs_queryAddPlayer",			"INSERT INTO `player` (`name`, `simplified`, `auth_name`, `auth_pass`, `auth_mask`, `created`) VALUES ('%s', '%s', '%s', MD5('%s'), %d, NOW());", CVAR_ARCHIVE );
	rs_queryAddNick				= trap_Cvar_Get( "rs_queryAddNick",				"INSERT INTO `nick` (`name`, `player_id`, `ip`, `created`) VALUES ('%s', %d, '%s', NOW());", CVAR_ARCHIVE );
	rs_queryGetMap				= trap_Cvar_Get( "rs_queryGetMapId",			"SELECT `id` FROM `map` WHERE `name` = '%s' LIMIT 1;", CVAR_ARCHIVE );
	rs_queryAddMap				= trap_Cvar_Get( "rs_queryAddMap",				"INSERT INTO `map` (`name`, `created`) VALUES('%s', NOW());", CVAR_ARCHIVE );
	
	rs_queryAddRace				= trap_Cvar_Get( "rs_queryAddRace",				"INSERT INTO `race` (`player_id`, `nick_id`, `map_id`, `time`, `created`) VALUES(%d, %d, %d, %d, NOW());", CVAR_ARCHIVE );
	rs_querySetMapRating		= trap_Cvar_Get( "rs_querySetMapRating",		"INSERT INTO `map_rating` (`player_id`, `map_id`, `value`, `created`) VALUES(%d, %d, %d, NOW()) ON DUPLICATE KEY UPDATE `value` = VALUE(`value`), `changed` = NOW();", CVAR_ARCHIVE );
	rs_queryUpdateMapRating		= trap_Cvar_Get( "rs_queryUpdateMapRating",		"UPDATE `map` SET `rating` = (SELECT SUM(`value`) / COUNT(`player_id`) FROM `map_rating` WHERE `map_id` = `map`.`id`), `ratings` = (SELECT COUNT(`player_id`) FROM `map_rating` WHEHRE `map_id` = `map`.`id`) WHERE `id` = %d LIMIT 1;

	rs_queryUpdatePlayerMap     = trap_Cvar_Get( "rs_queryUpdatePlayerMap",     "INSERT INTO `player_map` (`player_id`, `map_id`, `time`, `races`, `created`) VALUES(%d, %d, %d, 1, NOW()) ON DUPLICATE KEY UPDATE `time` = IF( VALUES(`time`) < `time` OR `time` = 0 OR `time` IS NULL, VALUES(`time`), `time` ), `races` = `races` + 1, `created` = NOW();", CVAR_ARCHIVE );
    
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
    // TODO: shutdown mysql
    
    // shutdown threading
	pthread_mutex_destroy(&mutexsum);
	pthread_exit(NULL);
}

/**
 * Authenticate the player
 *
 * @param edict_t *ent
 * @param char *authName
 * @param char *authPass
 * @return qboolean
 */
qboolean RS_MysqlAuthenticate( edict_t *ent, char *authName, char *authPass )
{
	int returnCode;
	pthread_t thread;
    struct authenticationData *authData=malloc(sizeof(struct authenticationData));
    
    authData->ent = ent;
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
 /*
qboolean RS_MysqlNickProtection( edict_t *ent )
{
	pthread_t thread;
	int returnCode = pthread_create(&thread, &threadAttr, RS_MysqlNickProtection_Thread, (void *)&ent);
	if (returnCode) {

		printf("THREAD ERROR: return code from pthread_create() is %d\n", returnCode);
		return qfalse;
	}
	
	return qtrue;
}
*/

/**
 * Insert a new race
 *
 * @param int player_id
 * @param int map_id
 * @param int race_time
 * @return qboolean
 */
qboolean RS_MysqlInsertRace( int player_id, int nick_id, int map_id, int race_time ) {

	int returnCode;
	pthread_t thread;
    struct raceDataStruct *raceData=malloc(sizeof(struct raceDataStruct));

    raceData->player_id = player_id;
	raceData->nick_id= nick_id;
	raceData->map_id = map_id;
	raceData->race_time = race_time;
    
	returnCode = pthread_create(&thread, &threadAttr, RS_MysqlInsertRace_Thread, (void *)raceData);

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
 /*
void *RS_MysqlNickProtection_Thread(void *in)
{
    char query[250];
    MYSQL_ROW  row;
    MYSQL_RES  *mysql_res;
    edict_t *ent;

	pthread_mutex_lock(&mutexsum);
	mysql_thread_init();

    ent = (edict_t *)in;
	sprintf(query, rs_queryCheckNick->string, ent->r.client->netname);
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();
    
    mysql_res = mysql_store_result(&mysql);
    RS_CheckMysqlThreadError();
    
    if ((row = mysql_fetch_row(mysql_res)) != NULL) {
    
        RS_MysqlAuthenticate_Callback(authData->ent, atoi(row[0]), atoi(row[2]));
    
    } else {
    
        RS_MysqlAuthenticate_Callback(authData->ent, 0, 0);
    }
    
    mysql_free_result(mysql_res);
	RS_EndMysqlThread();
    
    return NULL;
}
*/

/**
 * Authentication thread
 *
 * @param void *in
 * @return void
 */
void *RS_MysqlAuthenticate_Thread( void *in )
{
    char query[1000];
    MYSQL_ROW  row;
    MYSQL_RES  *mysql_res;
    struct authenticationData *authData = (struct authenticationData *)in;

	pthread_mutex_lock(&mutexsum);
	mysql_thread_init();
	sprintf(query, rs_queryGetPlayer->string, authData->authName, authData->authPass);
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();
    
    mysql_res = mysql_store_result(&mysql);
    RS_CheckMysqlThreadError();
    
    if ((row = mysql_fetch_row(mysql_res)) != NULL) {
		if (row[0]!=NULL && row[1]!=NULL) 
	        RS_MysqlAuthenticate_Callback(authData->ent, atoi(row[0]), atoi(row[1]));
    
    } else {
    
        RS_MysqlAuthenticate_Callback(authData->ent, 0, 0);
    }
    
    mysql_free_result(mysql_res);
	free(authData->authName);
	free(authData->authPass);
	free(authData);	
	RS_EndMysqlThread();
    
    return NULL;
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
	struct raceDataStruct *raceData=(struct raceDataStruct *)in;
    
	pthread_mutex_lock(&mutexsum);
	mysql_thread_init();
	sprintf(query, rs_queryAddRace->string, raceData->player_id, raceData->nick_id, raceData->map_id, raceData->race_time);
    mysql_real_query(&mysql, query, strlen(query));
    RS_CheckMysqlThreadError();
        
	// this one has no callback, i don't think that's needed - r2
    
	free(raceData);	
	RS_EndMysqlThread();
    
    return NULL;
}

/**
 * Cleanly end the mysql thread
 * 
 * @param void *threadid
 * @return void
 */
void RS_EndMysqlThread()
{
	mysql_thread_end();
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

//==============
//RS_UseShooter
//==============
void RS_UseShooter( edict_t *self, edict_t *other, edict_t *activator ) {

	vec3_t      dir;
	vec3_t      angles;
    gs_weapon_definition_t *weapondef = NULL;

    if ( self->enemy ) {
        VectorSubtract( self->enemy->s.origin, self->s.origin, dir );
        VectorNormalize( dir );
    } else {
        VectorCopy( self->moveinfo.movedir, dir );
        VectorNormalize( dir );
    }
    VecToAngles( dir, angles );
	switch ( self->s.weapon ) {
        case WEAP_GRENADELAUNCHER:
        	weapondef = GS_GetWeaponDef( WEAP_GRENADELAUNCHER );
            W_Fire_Grenade( activator, self->s.origin, angles, weapondef->firedef.speed, weapondef->firedef.damage, weapondef->firedef.minknockback, weapondef->firedef.knockback, weapondef->firedef.stun, weapondef->firedef.mindamage, weapondef->firedef.splash_radius, weapondef->firedef.timeout, MOD_GRENADE_W, 0, qfalse );
            break;
        case WEAP_ROCKETLAUNCHER:
			weapondef = GS_GetWeaponDef( WEAP_ROCKETLAUNCHER );
			W_Fire_Rocket( activator, self->s.origin, angles, weapondef->firedef.speed, weapondef->firedef.damage, weapondef->firedef.minknockback, weapondef->firedef.knockback, weapondef->firedef.stun, weapondef->firedef.mindamage, weapondef->firedef.splash_radius, weapondef->firedef.timeout, MOD_ROCKET_W, 0 );
            break;
        case WEAP_PLASMAGUN:
        	weapondef = GS_GetWeaponDef( WEAP_PLASMAGUN );
            W_Fire_Plasma( activator, self->s.origin, angles, weapondef->firedef.speed, weapondef->firedef.damage, weapondef->firedef.minknockback, weapondef->firedef.knockback, weapondef->firedef.stun, weapondef->firedef.mindamage, weapondef->firedef.splash_radius, weapondef->firedef.timeout, MOD_PLASMA_W, 0 );
            break;
    }

    //G_AddEvent( self, EV_MUZZLEFLASH, FIRE_MODE_WEAK, qtrue );

}

//======================
//RS_InitShooter_Finish
//======================
void RS_InitShooter_Finish( edict_t *self ) {

	self->enemy = G_PickTarget( self->target );
    self->think = 0;
    self->nextThink = 0;
}

//===============
//RS_InitShooter
//===============
void RS_InitShooter( edict_t *self, int weapon ) {

	self->use = RS_UseShooter;
    self->s.weapon = weapon;
    G_SetMovedir( self->s.angles, self->moveinfo.movedir );
    // target might be a moving object, so we can't set movedir for it
    if ( self->target ) {
        self->think = RS_InitShooter_Finish;
        self->nextThink = level.time + 500;
    }
    GClip_LinkEntity( self );

}

//=================
//RS_shooter_rocket
//===============
void RS_shooter_rocket( edict_t *self ) {
    RS_InitShooter( self, WEAP_ROCKETLAUNCHER );
}

//=================
//RS_shooter_plasma
//===============
void RS_shooter_plasma( edict_t *self ) {
    RS_InitShooter( self, WEAP_PLASMAGUN);
}

//=================
//RS_shooter_grenade
//===============
void RS_shooter_grenade( edict_t *self ) {
    RS_InitShooter( self, WEAP_GRENADELAUNCHER);
}

//=================
//QUAKED target_delay (1 0 0) (-8 -8 -8) (8 8 8)
//"wait" seconds to pause before firing targets.
//=================
void Think_Target_Delay( edict_t *self ) {

    G_UseTargets( self, self->activator );
}

//=================
//Use_Target_Delay
//=================
void Use_Target_Delay( edict_t *self, edict_t *other, edict_t *activator ) {
    self->nextThink = level.time + self->wait * 1000;
    self->think = Think_Target_Delay;
    self->activator = activator;
}

//=================
//RS_target_delay
//=================
void RS_target_delay( edict_t *self ) {

    if ( !self->wait ) {
        self->wait = 1;
    }
    self->use = Use_Target_Delay;
}


//==========================================================
//QUAKED target_relay (0 .7 .7) (-8 -8 -8) (8 8 8) RED_ONLY BLUE_ONLY RANDOM
//This can only be activated by other triggers which will cause it in turn to activate its own targets.
//-------- KEYS --------
//targetname : activating trigger points to this.
//target : this points to entities to activate when this entity is triggered.
//notfree : when set to 1, entity will not spawn in "Free for all" and "Tournament" modes.
//notteam : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes.
//notsingle : when set to 1, entity will not spawn in Single Player mode (bot play mode).
//-------- SPAWNFLAGS --------
//RED_ONLY : only red team players can activate trigger. <- WRONG
//BLUE_ONLY : only red team players can activate trigger. <- WRONG
//RANDOM : one one of the targeted entities will be triggered at random.
void target_relay_use (edict_t *self, edict_t *other, edict_t *activator) {
    if ( ( self->spawnflags & 1 ) && activator->s.team && activator->s.team != TEAM_ALPHA ) {
        return;
    }
    if ( ( self->spawnflags & 2 ) && activator->s.team && activator->s.team != TEAM_BETA ) {
        return;
    }
    if ( self->spawnflags & 4 ) {
        edict_t *ent;
        ent = G_PickTarget( self->target );
        if ( ent && ent->use ) {
            ent->use( ent, self, activator );
        }
        return;
    }
    G_UseTargets (self, activator);
}

//=================
//RS_target_relay
//=================
void RS_target_relay (edict_t *self) {
    self->use = target_relay_use;
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
