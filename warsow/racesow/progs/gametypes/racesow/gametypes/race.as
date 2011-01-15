class Racesow_Gametype_Race : Racesow_Gametype
{
    
    Racesow_Gametype_Race()
    {
        
    }
    
    ~Racesow_Gametype_Race()
    {
        
    }
    
    void InitGametype()
    {
        gametype.spawnpointRadius = 0;
        G_ConfigString( CS_SCB_PLAYERTAB_LAYOUT, "%n 112 %s 52 %t 96 %i 48 %l 48 %s 85" );
	    G_ConfigString( CS_SCB_PLAYERTAB_TITLES, "Name Clan Time Speed Ping State" );
    }
    
    void SpawnGametype()
    {
        
    }
    
    void Shutdown()
    {
        
    }
    
    bool MatchStateFinished( int incomingMatchState )
    {
        return true;
    }
    
    void MatchStateStarted()
    {
        
    }
    
    void ThinkRules()
    {
        // set all clients race stats
        cClient @client;

        for ( int i = 0; i < maxClients; i++ )
        {
            @client = @G_GetClient( i );

            if ( client.state() < CS_SPAWNED )
                continue;

            Racesow_Player @player = Racesow_GetPlayerByClient( client );

            if( ( player.client.team == TEAM_SPECTATOR ) && !( player.client.chaseActive ) )
                @player.race = null;

            if ( player.isRacing() )
            {
                client.setHUDStat( STAT_TIME_SELF, (levelTime - player.race.getStartTime()) / 100 );
                if ( player.highestSpeed < player.getSpeed() )
                    player.highestSpeed = player.getSpeed(); // updating the heighestSpeed attribute.
            }

            client.setHUDStat( STAT_TIME_BEST, player.getBestTime() / 100 );
            client.setHUDStat( STAT_TIME_RECORD, map.getHighScore().getTime() / 100 );
            if ( isUsingRacesowClient(client) )
            {
                client.setHUDStat( STAT_TIME_ALPHA, map.worldBest / 100 );
            }
            addFastcapHUDStats( @client );
            if ( client.getEnt().health > client.getEnt().maxHealth )
                client.getEnt().health -= ( frameTime * 0.001f );
        }
    }
    
    void playerRespawn( cEntity @ent, int old_team, int new_team )
    {
        // set player movement to pass through other players and remove gunblade auto attacking
        ent.client.setPMoveFeatures( ent.client.pmoveFeatures & ~PMFEAT_GUNBLADEAUTOATTACK | PMFEAT_GHOSTMOVE );

        // disable autojump
        if ( rs_allowAutoHop.getBool() == false )
        {
            ent.client.setPMoveFeatures( ent.client.pmoveFeatures & ~PMFEAT_CONTINOUSJUMP );
        }
        
        ent.client.inventorySetCount( WEAP_GUNBLADE, 1 );
    }
    
    void scoreEvent( cClient @client, cString &score_event, cString &args )
    {
        
    }
    
    cString @ScoreboardMessage( int maxlen )
    {
        cString scoreboardMessage, entry;
        cTeam @team;
        cEntity @ent;
        int i, playerID;
        int racing;
        //int readyIcon;

        @team = @G_GetTeam( TEAM_PLAYERS );

        // &t = team tab, team tag, team score (doesn't apply), team ping (doesn't apply)
        entry = "&t " + int( TEAM_PLAYERS ) + " 0 " + team.ping + " ";
        if ( scoreboardMessage.len() + entry.len() < maxlen )
            scoreboardMessage += entry;

        // "Name Time Ping State"
        for ( i = 0; @team.ent( i ) != null; i++ )
        {
            @ent = @team.ent( i );
            Racesow_Player @player = Racesow_GetPlayerByClient( ent.client );

            int playerID = ( ent.isGhosting() && ( match.getState() == MATCH_STATE_PLAYTIME ) ) ? -( ent.playerNum() + 1 ) : ent.playerNum();
			
            entry = "&p " + playerID + " " + ent.client.getClanName() + " "
                + player.getBestTime() + " "
                + player.highestSpeed + " "
                + ent.client.ping + " " + player.getState() + " ";
            if ( scoreboardMessage.len() + entry.len() < maxlen )
                scoreboardMessage += entry;
        }
        return @scoreboardMessage;
    }
    
    cEntity @SelectSpawnPoint( cEntity @self )
    {
        if( @alphaFlagBase != null )
        {
            return @bestFastcapSpawnpoint();
        }
        else
        {
            return null; // select random
        }
    }
    
    bool UpdateBotStatus( cEntity @self )
    {
        return false;// let the default code handle it itself
    }
    
    bool Command( cClient @client, cString @cmdString, cString @argsString, int argc )
    {
        return false;
    }
}