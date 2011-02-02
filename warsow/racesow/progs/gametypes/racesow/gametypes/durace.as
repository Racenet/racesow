class Racesow_Gametype_Durace : Racesow_Gametype
{
    
    Racesow_Gametype_Durace()
    {
        
    }
    
    ~Racesow_Gametype_Durace()
    {
        
    }
    
    void InitGametype()
    {
        gametype.setTitle( "Duel Race 1vs1" );
        // if the gametype doesn't have a config file, create it
        if ( !G_FileExists( "configs/server/gametypes/durace.cfg" ) )
        {
            cString config;
    
            // the config file doesn't exist or it's empty, create it
            config = "// '" + gametype.getTitle() + "' gametype configuration file\n"
                     + "// This config will be executed each time the gametype is started\n"
                     + "\n// game settings\n"
                     + "set g_scorelimit \"0\"\n"
                     + "set g_timelimit \"10\"\n"
                     + "set g_warmup_enabled \"1\"\n"
                     + "set g_warmup_timelimit \"3\"\n"
                     + "set g_match_extendedtime \"0\"\n"
                     + "set g_teams_maxplayers \"1\"\n"
                     + "set g_teams_allow_uneven \"0\"\n"
                     + "set g_countdown_time \"3\"\n"
                     + "set g_maxtimeouts \"-1\" // -1 = unlimited\n"
                     + "set g_challengers_queue \"1\"\n"
                     
                     + "\necho durace.cfg executed\"\n";
    
            G_WriteFile( "configs/server/gametypes/durace.cfg", config );
            G_Print( "Created default config file for 'durace'\n" );
            G_CmdExecute( "exec configs/server/gametypes/durace.cfg silent" );
        }
        
        //store the timelimit because value in DURACE is not the same than in RACE
        oldTimelimit = g_timelimit.getInteger();  
    
        gametype.isTeamBased = true;
        gametype.hasChallengersQueue = true;
        gametype.maxPlayersPerTeam = 1;
        gametype.spawnpointRadius = 0;
    
        // define the scoreboard layout
        G_ConfigString( CS_SCB_PLAYERTAB_LAYOUT, "%n 112 %s 52 %i 52 %t 96 %l 48 %b 50 %p 18" );
        G_ConfigString( CS_SCB_PLAYERTAB_TITLES, "Name Clan Score Time Ping Racing R" );
    
        G_Print( "Gametype '" + gametype.getTitle() + "' initialized\n" );
    }
    
    void SpawnGametype()
    {
        cEntity @from = null;
            
        for( int tag = WEAP_NONE; tag < POWERUP_TOTAL; tag++ )
        {
        	cItem @Item = G_GetItem( tag );
        	if( @Item == null)
        		continue;
        	cString itemClassname = Item.getClassname();
        	@from = null;
        	cEntity @item = @G_FindEntityWithClassname( @from, itemClassname );
        	if( @item == null )
        		continue;
      		do
      		{
      			if( ( item.solid == SOLID_NOT ) && ( ( @item.findTargetingEntity( null ) != null ) && ( item.findTargetingEntity( null ).getClassname() == "target_give" ) ) ) //connected to target_give
      			{
      				@from = @item;
      			}
      			else
      			{
      				item.setClassname( "AS_" + itemClassname );
      				replacementItem( @item );
      				@from = @item;
      			}
      			@item = @G_FindEntityWithClassname( @from, itemClassname );
      		} while( @item != null );
      	}
    }
    
    void Shutdown()
    {
        
    }
    
    bool MatchStateFinished( int incomingMatchState )
    {
        if ( match.getState() <= MATCH_STATE_WARMUP && incomingMatchState > MATCH_STATE_WARMUP
                && incomingMatchState < MATCH_STATE_POSTMATCH )
            match.startAutorecord();
    
        if ( match.getState() == MATCH_STATE_POSTMATCH )
        {
            match.stopAutorecord();
            demoRecording = false;
        }
    
        // check maxHealth rule
        for ( int i = 0; i < maxClients; i++ )
        {
            cEntity @ent = @G_GetClient( i ).getEnt();
            if ( ent.client.state() >= CS_SPAWNED && ent.team != TEAM_SPECTATOR )
            {
                if ( ent.health > ent.maxHealth )
                    ent.health -= ( frameTime * 0.001f );
            }
        }
    
        return true;
    }
    
    void MatchStateStarted()
    {
      switch ( match.getState() )
      {
      case MATCH_STATE_WARMUP:
          GENERIC_SetUpWarmup();
          break;
  
      case MATCH_STATE_COUNTDOWN:
          GENERIC_SetUpCountdown();
          break;
  
      case MATCH_STATE_PLAYTIME:
          DURACE_SetUpMatch();
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
        if ( match.scoreLimitHit() || match.timeLimitHit() || match.suddenDeathFinished() )
        {
            if ( !match.checkExtendPlayTime() )
                match.launchState( match.getState() + 1 );
        }
        
        GENERIC_DetectTeamsAndMatchNames();
    
        if ( match.getState() >= MATCH_STATE_POSTMATCH )
            return;
    
        if ( match.getState() == MATCH_STATE_PLAYTIME )
        {
            // if there is no player in TEAM_PLAYERS finish the match and restart
            if ( G_GetTeam( TEAM_PLAYERS ).numPlayers == 0 && demoRecording )
            {
                match.stopAutorecord();
                demoRecording = false;
            }
            else if ( !demoRecording && G_GetTeam( TEAM_PLAYERS ).numPlayers > 0 )
            {
                match.startAutorecord();
                demoRecording = true;
            }
        }
    
        // set all clients race stats
        cClient @client;
    
        for ( int i = 0; i < maxClients; i++ )
        {
            @client = @G_GetClient( i );
            if ( client.state() < CS_SPAWNED )
                continue;
    
            // always clear all before setting
            client.setHUDStat( STAT_PROGRESS_SELF, 0 );
            client.setHUDStat( STAT_PROGRESS_OTHER, 0 );
            client.setHUDStat( STAT_IMAGE_SELF, 0 );
            client.setHUDStat( STAT_IMAGE_OTHER, 0 );
            client.setHUDStat( STAT_PROGRESS_ALPHA, 0 );
            client.setHUDStat( STAT_PROGRESS_BETA, 0 );
            client.setHUDStat( STAT_IMAGE_ALPHA, 0 );
            client.setHUDStat( STAT_IMAGE_BETA, 0 );
            client.setHUDStat( STAT_MESSAGE_SELF, 0 );
            client.setHUDStat( STAT_MESSAGE_OTHER, 0 );
            client.setHUDStat( STAT_MESSAGE_ALPHA, 0 );
            client.setHUDStat( STAT_MESSAGE_BETA, 0 );
    
            // all stats are set to 0 each frame, so it's only needed to set a stat if it's going to get a value
            if ( Racesow_GetPlayerByClient( client ).isRacing() )
                client.setHUDStat( STAT_TIME_SELF, (levelTime - Racesow_GetPlayerByClient( client ).race.startTime) / 100 );
    
            client.setHUDStat( STAT_TIME_BEST, Racesow_GetPlayerByClient( client ).bestRaceTime / 100 );
            client.setHUDStat( STAT_TIME_RECORD, map.getHighScore().getTime() / 100 );
    
            client.setHUDStat( STAT_TIME_ALPHA, -9999 );
            client.setHUDStat( STAT_TIME_BETA, -9999 );
        }
    }
    
    void playerRespawn( cEntity @ent, int old_team, int new_team )
    {
        if ( ent.isGhosting() )
	        return;
          
        Racesow_GetPlayerByClient( ent.client ).cancelRace();
        
        // set player movement to pass through other players and remove gunblade auto attacking
        ent.client.setPMoveFeatures( ent.client.pmoveFeatures & ~PMFEAT_GUNBLADEAUTOATTACK | PMFEAT_GHOSTMOVE );
        
        ent.client.inventorySetCount( WEAP_GUNBLADE, 1 );
    }
    
    void scoreEvent( cClient @client, cString &score_event, cString &args )
    {
        if ( score_event == "dmg" )
        {
        }
        else if ( score_event == "kill" )
        {
            cEntity @attacker = null;
    
            if ( @client != null )
                @attacker = @client.getEnt();
    
            int arg1 = args.getToken( 0 ).toInt();
            int arg2 = args.getToken( 1 ).toInt();
    
            // target, attacker, inflictor
            DURACE_playerKilled( G_GetEntity( arg1 ), attacker, G_GetEntity( arg2 ) );
        }
        else if ( score_event == "award" )
        {
        }
        else if ( score_event == "enterGame" )
        {
            Racesow_GetPlayerByClient( client ).reset();
        }
    }
    
    cString @ScoreboardMessage( int maxlen )
    {
        cString scoreboardMessage = "";
        cString entry;
        cTeam @team;
        cEntity @ent;
        int i, t, readyIcon;
        int racing;
    
        for ( t = TEAM_ALPHA; t < GS_MAX_TEAMS; t++ )
        {
            @team = @G_GetTeam( t );
    
            // &t = team tab, team tag, team score (doesn't apply), team ping (doesn't apply)
            entry = "&t " + t + " " + team.stats.score + " " + team.ping + " ";
            if ( scoreboardMessage.len() + entry.len() < maxlen )
                scoreboardMessage += entry;
    
            for ( i = 0; @team.ent( i ) != null; i++ )
            {
                @ent = @team.ent( i );
    
                if ( ent.client.isReady() )
                    readyIcon = prcYesIcon;
                else
                    readyIcon = 0;
    
                int playerID = ( ent.isGhosting() && ( match.getState() == MATCH_STATE_PLAYTIME ) ) ? -( ent.playerNum() + 1 ) : ent.playerNum();
    
                racing = int( Racesow_GetPlayerByClient( ent.client ).isRacing() ? 1 : 0 );
                
                //"Name Clan Score Time Ping Racing R"
                entry = "&p " + playerID + " "
                        + ent.client.getClanName() + " "
                        + ent.client.stats.score + " "
                        + Racesow_GetPlayerByClient( ent.client ).bestRaceTime + " "
                        + ent.client.ping + " "
                        + racing + " "
                        + readyIcon + " ";
    
                if ( scoreboardMessage.len() + entry.len() < maxlen )
                    scoreboardMessage += entry;
            }
        }
        return @scoreboardMessage;
    }
    
    cEntity @SelectSpawnPoint( cEntity @self )
    {
        return GENERIC_SelectBestRandomSpawnPoint( self, "info_player_deathmatch" );
    }
    
    bool UpdateBotStatus( cEntity @self )
    {
        return false;
    }
    
    bool Command( cClient @client, cString @cmdString, cString @argsString, int argc )
    {
        return false;
    }
}

void DURACE_SetUpMatch()
{
    int i, j;
    cEntity @ent;
    cTeam @team;

    gametype.shootingDisabled = false;
    gametype.readyAnnouncementEnabled = false;
    gametype.scoreAnnouncementEnabled = true;
    gametype.countdownEnabled = true;

    gametype.pickableItemsMask = gametype.spawnableItemsMask;
    gametype.dropableItemsMask = gametype.spawnableItemsMask;

    // clear player stats and scores, team scores
    for ( i = TEAM_PLAYERS; i < GS_MAX_TEAMS; i++ )
    {
        @team = @G_GetTeam( i );
        team.stats.clear();
        
        // respawn all clients inside the playing teams
        for ( j = 0; @team.ent( j ) != null; j++ )
        {
            @ent = @team.ent( j );
            ent.client.stats.clear(); // clear player scores & stats
            ent.client.respawn( false );
        }
    }

    G_RemoveDeadBodies();
    
    // Countdowns should be made entirely client side, because we now can
    int soundindex = G_SoundIndex( "sounds/announcer/countdown/fight0" + int( brandom( 1, 2 ) ) );
    G_AnnouncerSound( null, soundindex, GS_MAX_TEAMS, false, null );
    G_CenterPrintMsg( null, "FIGHT!\n" );
}

// a player has just died. The script is warned about it so it can account scores
void DURACE_playerKilled( cEntity @target, cEntity @attacker, cEntity @inflicter )
{
    if ( @target == null || @target.client == null )
      return;

    Racesow_GetPlayerByClient( target.client ).cancelRace();
}