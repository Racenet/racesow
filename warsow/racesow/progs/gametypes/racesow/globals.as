int numCheckpoints = 0;
bool demoRecording = false;
const int MAX_RECORDS = 10;

Racesow_Player[] players( maxClients );
Racesow_Map @map;

cVar g_roundlimit( "g_freestyle", "0", CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_NOSET );

/**
 * TimeToString
 * @param uint time
 * @return cString
 */
cString TimeToString( uint time )
{
    // convert times to printable form
    cString minsString, secsString, millString;
    uint min, sec, milli;

    milli = time;
    min = milli / 60000;
    milli -= min * 60000;
    sec = milli / 1000;
    milli -= sec * 1000;

    if ( min == 0 )
        minsString = "00";
    else if ( min < 10 )
        minsString = "0" + min;
    else
        minsString = min;

    if ( sec == 0 )
        secsString = "00";
    else if ( sec < 10 )
        secsString = "0" + sec;
    else
        secsString = sec;

    if ( milli == 0 )
        millString = "000";
    else if ( milli < 10 )
        millString = "00" + milli;
    else if ( milli < 100 )
        millString = "0" + milli;
    else
        millString = milli;

    return minsString + ":" + secsString + "." + millString;
}

/**
 * Racesow_GetPlayerByClient
 * @param cClient @client
 * @return Racesow_Player
 */
Racesow_Player @Racesow_GetPlayerByClient( cClient @client )
{
    if ( @client == null || client.playerNum() < 0 )
        return null;

	return @players[ client.playerNum() ].setClient(client);
}

/**
 * race_respawner_think
 * the player has finished the race. This entity times his automatic respawning
 * @param cEntity @respawner
 * @return void
 */
void race_respawner_think( cEntity @respawner )
{
    cClient @client = G_GetClient( respawner.count );

    // the client may have respawned on his own. If the last time was erased, don't respawn him
    if ( !Racesow_GetPlayerByClient( client ).isSpawned )
	{
		client.respawn( false );
        Racesow_GetPlayerByClient( client ).isSpawned = true;
	}

    respawner.freeEntity(); // free the respawner
}

/**
 * RACE_playerKilled
 *
 * a player has just died. The script is warned about it so it can account scores
 *
 * @param cEntity @target,
 * @param cEntity @attacker
 * @param cEntity @inflicter
 * @return viod
 */
void RACE_playerKilled( cEntity @target, cEntity @attacker, cEntity @inflicter )
{
    if ( @target == null || @target.client == null )
        return;

    Racesow_GetPlayerByClient( target.client ).cancelRace();
}

/**
 * RACE_SetUpMatch
 * @return void
 */
void RACE_SetUpMatch()
{
    int i, j;
    cEntity @ent;
    cTeam @team;

    gametype.shootingDisabled = false;
    gametype.readyAnnouncementEnabled = false;
    gametype.scoreAnnouncementEnabled = false;
    gametype.countdownEnabled = true;

    gametype.pickableItemsMask = gametype.spawnableItemsMask;
    gametype.dropableItemsMask = gametype.spawnableItemsMask;

    // clear player stats and scores, team scores

    for ( i = TEAM_PLAYERS; i < GS_MAX_TEAMS; i++ )
    {
        @team = @G_GetTeam( i );
        team.stats.clear();
    }

    G_RemoveDeadBodies();
}