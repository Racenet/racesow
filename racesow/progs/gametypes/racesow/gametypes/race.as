class Racesow_Gametype_Race : Racesow_Gametype
{
    Racesow_Gametype_Race()
    {
        for( int i= 0; i < maxClients; i++ )
            @this.players[i] = @Racesow_Player();
        //FIXME: These will hopefully be useable with the new angelscript version
        /*@this.commandMap["join"] = @Command_Join();
        @this.commandMap["spec"] = @Command_Spec();
        @this.commandMap["chase"] = @Command_Chase();
        @this.commandMap["racerestart"] = @Command_RaceRestart();
        @this.commandMap["kill"] = @Command_Kill();
        @this.commandMap["oneliner"] = @Command_Oneliner();
        @this.commandMap["position"] = @Command_Position();
        @this.commandMap["noclip"] = @Command_Noclip();
        @this.commandMap["machinegun"] = @Command_Machinegun();
        @this.commandMap["timeleft"] = @Command_Timeleft();
        @this.commandMap["top"] = @Command_Top();
        @this.commandMap["practicemode"] = @Command_Practicemode();*/
        commandMap.set_opIndex("join", @Command_Join());
        commandMap.set_opIndex("spec", @Command_Spec());
        commandMap.set_opIndex("chase", @Command_Chase());
        commandMap.set_opIndex("racerestart", @Command_RaceRestart());
        commandMap.set_opIndex("kill", @Command_Kill());
        commandMap.set_opIndex("oneliner", @Command_Oneliner());
        commandMap.set_opIndex("position", @Command_Position());
        commandMap.set_opIndex("noclip", @Command_Noclip());
        commandMap.set_opIndex("machinegun", @Command_Machinegun());
        commandMap.set_opIndex("timeleft", @Command_Timeleft());
        commandMap.set_opIndex("top", @Command_Top());
        commandMap.set_opIndex("practicemode", @Command_Practicemode());
    }
    
    void InitGametype()
    {
      gametype.setTitle( "Race" );
      
      // if the gametype doesn't have a config file, create it
      if ( !G_FileExists( "configs/server/gametypes/race.cfg" ) )
      {
          cString config;
    
          // the config file doesn't exist or it's empty, create it
          config = "//*\n"
                   + "//* Race settings\n"
                   + "//*\n"
                   + "set g_scorelimit \"0\" // a new feature..?\n"
                   + "set g_warmup_timelimit \"0\" // ... \n"
                   + "set g_maxtimeouts \"0\" \n"
                   + "set g_disable_vote_timeout \"1\" \n"
                   + "set g_disable_vote_timein \"1\" \n"
                   + "set g_disable_vote_scorelimit \"1\" \n"
                   + "\n"
    			 + "echo race.cfg executed\n";
    
          G_WriteFile( "configs/server/gametypes/race.cfg", config );
          G_Print( "Created default base config file for race\n" );
          G_CmdExecute( "exec configs/server/gametypes/race.cfg silent" );
      }
   
      gametype.isTeamBased = false;
      gametype.hasChallengersQueue = false;
      gametype.maxPlayersPerTeam = 0;
      gametype.spawnpointRadius = 0;
      gametype.autoInactivityRemove = true;
      gametype.playerInteraction = false;
      gametype.freestyleMapFix = false;
      gametype.enableDrowning = true;
    
    	// disallow warmup, no matter what config files say, because it's bad for racesow timelimit.
      g_warmup_timelimit.set("0"); //g_warmup_enabled was removed in warsow 0.6
	
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
        if (incomingMatchState == MATCH_STATE_POSTMATCH)
        {
            map.startOvertime();
            return map.allowEndGame();
        }
        return true;
    }
    
    void MatchStateStarted()
    {
        switch ( match.getState() )
        {
        case MATCH_STATE_WARMUP:
            match.launchState( MATCH_STATE_PLAYTIME );
            break;
    
        case MATCH_STATE_COUNTDOWN:
            break;
    
        case MATCH_STATE_PLAYTIME:
            map.setUpMatch();
            break;
    
        case MATCH_STATE_POSTMATCH:
            gametype.pickableItemsMask = 0;
            gametype.dropableItemsMask = 0;
            GENERIC_SetUpEndMatch();
            break;
    
        default:
            break;
        }
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
        if ( ent.isGhosting() )
	        return;
	        
        Racesow_Player @player = Racesow_GetPlayerByClient( ent.client );
        // set player movement to pass through other players and remove gunblade auto attacking
        ent.client.setPMoveFeatures( ent.client.pmoveFeatures & ~PMFEAT_GUNBLADEAUTOATTACK | PMFEAT_GHOSTMOVE );

        // disable autojump
        if ( rs_allowAutoHop.getBool() == false )
        {
            ent.client.setPMoveFeatures( ent.client.pmoveFeatures & ~PMFEAT_CONTINOUSJUMP );
        }
        
        ent.client.inventorySetCount( WEAP_GUNBLADE, 1 );
        player.getClient().stats.setScore(player.bestRaceTime);
        player.restartingRace();
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

    bool practiceCheck(Racesow_Player @player, cString &commandName)
    {
	    // if a practiceEnabled command fails to validate, send this message instead of the cmd.getUsage().
	    // i guess we'll have a better solution to this when the new command system is up. - K1ll i guess not :/
	    player.sendErrorMessage( "The " + commandName + " command is only available in practice mode." );
	    return true;        
    }
}

Racesow_Gametype@ getRacesowGametype() {
    return @Racesow_Gametype_Race();
}
