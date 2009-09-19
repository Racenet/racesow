int numCheckpoints = 0;
bool demoRecording = false;
const int MAX_RECORDS = 3;

uint[] levelRecordSectors;
uint   levelRecordFinishTime;
cString levelRecordPlayerName;

Racesow_Player_Record[] levelRecords( MAX_RECORDS );
Racesow_Player[] players( maxClients );

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
 * RACE_UpdateHUDTopScores
 * @return void
 */
void RACE_UpdateHUDTopScores()
{
    for ( int i = 0; i < MAX_RECORDS; i++ )
    {
        if ( levelRecords[i].finishTime > 0 && levelRecords[i].playerName.len() > 0 )
        {
            G_ConfigString( CS_GENERAL + i, "#" + ( i + 1 ) + " - " + levelRecords[i].playerName + " - " + TimeToString( levelRecords[i].finishTime ) );
        }
    }
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

void RACE_WriteTopScores()
{
    cString topScores;
    cVar mapName( "mapname", "", 0 );

    topScores = "//" + mapName.getString() + " top scores\n\n";

    for ( int i = 0; i < MAX_RECORDS; i++ )
    {
        if ( levelRecords[i].finishTime > 0 && levelRecords[i].playerName.len() > 0 )
        {
            topScores += "\"" + int( levelRecords[i].finishTime ) + "\" \"" + levelRecords[i].playerName + "\" ";

            // add the sectors
            topScores += "\"" + numCheckpoints+ "\" ";

            for ( int j = 0; j < numCheckpoints; j++ )
                topScores += "\"" + int( levelRecords[i].checkPoints[j] ) + "\" ";

            topScores += "\n";
        }
    }

    G_WriteFile( "topscores/race/" + mapName.getString() + ".txt", topScores );
}

void RACE_LoadTopScores()
{
    cString topScores;
    cVar mapName( "mapname", "", 0 );

    topScores = G_LoadFile( "topscores/race/" + mapName.getString() + ".txt" );

    if ( topScores.len() > 0 )
    {
        cString timeToken, nameToken, sectorToken;
        int count = 0;

        for ( int i = 0; i < MAX_RECORDS; i++ )
        {
            timeToken = topScores.getToken( count++ );
            if ( timeToken.len() == 0 )
                break;

            nameToken = topScores.getToken( count++ );
            if ( nameToken.len() == 0 )
                break;

            sectorToken = topScores.getToken( count++ );
            if ( sectorToken.len() == 0 )
                break;

            int numSectors = sectorToken.toInt();

            // store this one
            for ( int j = 0; j < numSectors; j++ )
            {
                sectorToken = topScores.getToken( count++ );
                if ( sectorToken.len() == 0 )
                    break;

                levelRecords[i].checkPoints[j] = uint( sectorToken.toInt() );
            }

            levelRecords[i].finishTime = uint( timeToken.toInt() );
            levelRecords[i].playerName = nameToken;
        }

        RACE_UpdateHUDTopScores();
		
		G_Print("JAAAAAAAAAAAAAAAAAAA\n");
    } else {
	G_Print("NEEEEEEEEEEEEEEEEEEEEEEEEE\n");
	}
}
