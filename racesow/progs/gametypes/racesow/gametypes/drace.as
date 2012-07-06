Cvar g_drace_max_victories( "g_drace_max_victories", "0", CVAR_ARCHIVE );

const int DRACE_ROUNDSTATE_NONE = 0;
const int DRACE_ROUNDSTATE_PREROUND = 1;
const int DRACE_ROUNDSTATE_ROUND = 2;
const int DRACE_ROUNDSTATE_ROUNDFINISHED = 3;
const int DRACE_ROUNDSTATE_POSTROUND = 4;

class cDRACERound
{
    int state;
    int numRounds;
    uint roundStateStartTime;
    uint roundStateEndTime;
    int countDown;
    int[] DRACEChallengersQueue;
    cEntity @alphaSpawn;
    cEntity @betaSpawn;
    cClient @roundWinner;
    cClient @roundChallenger;
    
    int nbVictories;
    cClient @lastWinner;

    cDRACERound()
    {
        this.state = DRACE_ROUNDSTATE_NONE;
        this.numRounds = 0;
        this.roundStateStartTime = 0;
        this.countDown = 0;
        @this.alphaSpawn = null;
        @this.betaSpawn = null;
        @this.roundWinner = null;
        @this.roundChallenger = null;
        @this.lastWinner = null;
        this.nbVictories = 0;
    }

    ~cDRACERound() {}

    void init()
    {
        this.clearChallengersQueue();
    }

    void clearChallengersQueue()
    {
        if ( this.DRACEChallengersQueue.length() != uint( maxClients ) )
            this.DRACEChallengersQueue.resize( maxClients );

        for ( int i = 0; i < maxClients; i++ )
            this.DRACEChallengersQueue[i] = -1;
    }

    void challengersQueueAddPlayer( cClient @client )
    {
        if ( @client == null )
            return;

        // check for already added
        for ( int i = 0; i < maxClients; i++ )
        {
            if ( this.DRACEChallengersQueue[i] == client.playerNum() )
                return;
        }

        for ( int i = 0; i < maxClients; i++ )
        {
            if ( this.DRACEChallengersQueue[i] < 0 || this.DRACEChallengersQueue[i] >= maxClients )
            {
                this.DRACEChallengersQueue[i] = client.playerNum();
                break;
            }
        }
    }

    bool challengersQueueRemovePlayer( cClient @client )
    {
        if ( @client == null )
            return false;

        for ( int i = 0; i < maxClients; i++ )
        {
            if ( this.DRACEChallengersQueue[i] == client.playerNum() )
            {
                int j;
                for ( j = i + 1; j < maxClients; j++ )
                {
                    this.DRACEChallengersQueue[j - 1] = this.DRACEChallengersQueue[j];
                    if ( DRACEChallengersQueue[j] == -1 )
                        break;
                }

                this.DRACEChallengersQueue[j] = -1;
                return true;
            }
        }

        return false;
    }

    cClient @challengersQueueGetNextPlayer()
    {
        cClient @client = @G_GetClient( this.DRACEChallengersQueue[0] );

        if ( @client != null )
        {
            this.challengersQueueRemovePlayer( client );
        }

        return client;
    }

    void playerTeamChanged( cClient @client, int new_team )
    {
        if ( new_team != TEAM_PLAYERS )
        {
            this.challengersQueueRemovePlayer( client );

            if ( this.state != DRACE_ROUNDSTATE_NONE )
            {
                if ( @client == @this.roundWinner )
                {
                    @this.roundWinner = null;
                    this.newRoundState( DRACE_ROUNDSTATE_ROUNDFINISHED );
                }

                if ( @client == @this.roundChallenger )
                {
                    @this.roundChallenger = null;
                    this.newRoundState( DRACE_ROUNDSTATE_ROUNDFINISHED );
                }
            }
        }
        else if ( new_team == TEAM_PLAYERS )
        {
            this.challengersQueueAddPlayer( client );
        }
    }

    void roundAnnouncementPrint( String &string )
    {
        if ( string.len() <= 0 )
            return;
        
        if ( @this.roundWinner != null )
          if ( !this.roundWinner.getEnt().isGhosting() )
            this.roundWinner.addAward( string );

        if ( @this.roundChallenger != null )
          if ( !this.roundChallenger.getEnt().isGhosting() )
            this.roundChallenger.addAward( string );
            
        if ( @this.roundWinner != null && @this.roundChallenger != null )
          if ( this.roundWinner.getEnt().isGhosting() && this.roundChallenger.getEnt().isGhosting() )
          {
            this.roundWinner.addAward( string );
            this.roundChallenger.addAward( string );
          }

        // also add it to spectators who are not in chasecam

        cTeam @team = @G_GetTeam( TEAM_SPECTATOR );
        cEntity @ent;

        // displays award to all clients
        for ( int i = 0; @team.ent( i ) != null; i++ )
        {
            @ent = @team.ent( i );
            if ( !ent.isGhosting() )
                ent.client.addAward( string );
        }
    }

    void newGame()
    {
        gametype.readyAnnouncementEnabled = false;
        gametype.scoreAnnouncementEnabled = true;
        gametype.countdownEnabled = false;

        // set spawnsystem type to not respawn the players when they die
        for ( int team = TEAM_PLAYERS; team < GS_MAX_TEAMS; team++ )
            gametype.setTeamSpawnsystem( team, SPAWNSYSTEM_HOLD, 0, 0, true );

        // clear scores

        cEntity @ent;
        cTeam @team;

        for ( int i = TEAM_PLAYERS; i < GS_MAX_TEAMS; i++ )
        {
            @team = @G_GetTeam( i );
            team.stats.clear();

            // respawn all clients inside the playing teams
            for ( int j = 0; @team.ent( j ) != null; j++ )
            {
                @ent = @team.ent( j );
                ent.client.stats.clear(); // clear player scores & stats
            }
        }

        this.numRounds = 0;
        this.newRound();
    }

    void endGame()
    {
        this.newRoundState( DRACE_ROUNDSTATE_NONE );

        if ( @this.roundWinner != null )
        {
            Cvar scoreLimit( "g_scorelimit", "", 0 );
            if ( this.roundWinner.stats.score == scoreLimit.get_integer() )
            {
                this.roundAnnouncementPrint( S_COLOR_WHITE + this.roundWinner.getName() + S_COLOR_GREEN + " wins the game!" );
            }
        }

        DRACE_SetUpEndMatch();
    }

    void newRound()
    {
        G_RemoveDeadBodies();
        G_RemoveAllProjectiles();

        this.newRoundState( DRACE_ROUNDSTATE_PREROUND );
        this.numRounds++;
    }

    void newRoundState( int newState )
    {
        if ( newState > DRACE_ROUNDSTATE_POSTROUND )
        {
            this.newRound();
            return;
        }

        this.state = newState;
        this.roundStateStartTime = levelTime;

        switch ( this.state )
        {
        case DRACE_ROUNDSTATE_NONE:

            this.roundStateEndTime = 0;
            this.countDown = 0;
            break;

        case DRACE_ROUNDSTATE_PREROUND:
        {
            this.roundStateEndTime = levelTime + 3000;
            this.countDown = 3;

            // respawn everyone and disable shooting
            gametype.shootingDisabled = true;

            // pick the last round winner and the first in queue,
            // or, if no round winner, the 2 first in queue
            if ( @this.roundWinner == null )
            {
                @this.roundWinner = @this.challengersQueueGetNextPlayer();
                @this.roundChallenger = @this.challengersQueueGetNextPlayer();
            }
            else
            {
                @this.roundChallenger = @this.challengersQueueGetNextPlayer();
            }

            cEntity @ent;
            cTeam @team;

            @team = @G_GetTeam( TEAM_PLAYERS );

            // respawn all clients inside the playing teams
            for ( int j = 0; @team.ent( j ) != null; j++ )
            {
                @ent = @team.ent( j );

                if ( @ent.client == @this.roundWinner || @ent.client == @this.roundChallenger )
                {
                    ent.client.respawn( false );
                    //we immobilize the players
                    ent.client.setPMoveMaxSpeed( 0 );
                    ent.client.setPMoveDashSpeed( 0 );
                }
                else
                {
                    ent.client.respawn( true );
                    ent.client.chaseCam( null, true );
                }
            }

            this.roundAnnouncementPrint( S_COLOR_GREEN + "New Round:" );
            this.roundAnnouncementPrint( S_COLOR_WHITE + this.roundWinner.getName()
                                         + S_COLOR_GREEN + " vs. "
                                         + S_COLOR_WHITE + this.roundChallenger.getName() );
        }
        break;

        case DRACE_ROUNDSTATE_ROUND:
        {
            gametype.shootingDisabled = false;
            this.countDown = 0;
            this.roundStateEndTime = 0;
            
            cEntity @ent;
            cTeam @team;

            @team = @G_GetTeam( TEAM_PLAYERS );

            // allows all players to move again
            for ( int j = 0; @team.ent( j ) != null; j++ )
            {
                @ent = @team.ent( j );
                ent.client.setPMoveMaxSpeed( -1 );
                ent.client.setPMoveDashSpeed( -1 );
                
                if ( @ent.client == @DRACERound.roundWinner || @ent.client == @DRACERound.roundChallenger )
                {
                  //nothing to do if it is winner or challenger
                }
                else
                  ent.client.chaseCam( null, true );
                
            }
            
            int soundIndex = G_SoundIndex( "sounds/announcer/countdown/fight0" + int( brandom( 1, 2 ) ) );
            G_AnnouncerSound( null, soundIndex, GS_MAX_TEAMS, false, null );
            G_CenterPrintMsg( null, "go\n" );
        }
        break;

        case DRACE_ROUNDSTATE_ROUNDFINISHED:

            gametype.shootingDisabled = true;
            this.roundStateEndTime = levelTime + 1500;
            this.countDown = 0;
            break;

        case DRACE_ROUNDSTATE_POSTROUND:
        {
            this.roundStateEndTime = levelTime + 3000;

            // add score to round-winning player
            cClient @winner = null;
            cClient @loser = null;
                    
            // watch for one of the players removing from the game
            if ( @this.roundWinner == null || @this.roundChallenger == null )
            {
                if ( @this.roundWinner != null )
                    @winner = @this.roundWinner;
                else if ( @this.roundChallenger != null )
                    @winner = @this.roundChallenger;
            }
            else if ( !this.roundWinner.getEnt().isGhosting() && this.roundChallenger.getEnt().isGhosting() )
            {
                @winner = @this.roundWinner;
                @loser = @this.roundChallenger;
            }
            else if ( this.roundWinner.getEnt().isGhosting() && !this.roundChallenger.getEnt().isGhosting() )
            {
                @winner = @this.roundChallenger;
                @loser = @this.roundWinner;
            }

            // if we didn't find a winner, it was a draw round
            if ( @winner == null )
            {
                this.roundAnnouncementPrint( S_COLOR_ORANGE + "Draw Round!" );
	       				this.challengersQueueAddPlayer( this.roundWinner );
				        this.challengersQueueAddPlayer( this.roundChallenger );
            }
            else
            {
                int soundIndex;

                soundIndex = G_SoundIndex( "sounds/announcer/ctf/score0" + int( brandom( 1, 2 ) ) );
                G_AnnouncerSound( winner, soundIndex, GS_MAX_TEAMS, false, null );
                winner.stats.addScore( 1 );
                
                if ( @loser != null )
                {
                    soundIndex = G_SoundIndex( "sounds/announcer/ctf/score_enemy0" + int( brandom( 1, 2 ) ) );
                    G_AnnouncerSound( loser, soundIndex, GS_MAX_TEAMS, false, null );
                    this.challengersQueueAddPlayer( loser );
                }

                this.roundAnnouncementPrint( S_COLOR_WHITE + winner.getName() + S_COLOR_GREEN + " wins the round!" );
                //if the winner reached the max successive victories
                if ( !this.checkMaxVictories( winner ) )
                {
                  G_CenterPrintMsg( winner.getEnt(), "You reached the maximum number of successive victories.\n" );
                  //add the player in the challengers queue
                  this.challengersQueueAddPlayer( winner );
                  //ghost him
                  winner.respawn( true );
                  @winner = null ;
                }
            }

            @this.roundWinner = @winner;
            @this.roundChallenger = null;			
        }
        break;

        default:
            break;
        }
    }

    void think()
    {
        if ( this.state == DRACE_ROUNDSTATE_NONE )
            return;

        if ( match.getState() != MATCH_STATE_PLAYTIME )
        {
            this.endGame();
            return;
        }

        if ( this.roundStateEndTime != 0 )
        {
            if ( this.roundStateEndTime < levelTime )
            {
                this.newRoundState( this.state + 1 );
                return;
            }

            if ( this.countDown > 0 )
            {
                // we can't use the authomatic countdown announces because their are based on the
                // matchstate timelimit, and prerounds don't use it. So, fire the announces "by hand".
                int remainingSeconds = int( ( this.roundStateEndTime - levelTime ) * 0.001f ) + 1;
                if ( remainingSeconds < 0 )
                    remainingSeconds = 0;

                if ( remainingSeconds < this.countDown )
                {
                    this.countDown = remainingSeconds;

                    if ( this.countDown == 4 )
                    {
                        int soundIndex = G_SoundIndex( "sounds/announcer/countdown/ready0" + int( brandom( 1, 2 ) ) );
                        G_AnnouncerSound( null, soundIndex, GS_MAX_TEAMS, false, null );
                    }
                    else if ( this.countDown <= 3 )
                    {
                        int soundIndex = G_SoundIndex( "sounds/announcer/countdown/" + this.countDown + "_0" + int( brandom( 1, 2 ) ) );
                        G_AnnouncerSound( null, soundIndex, GS_MAX_TEAMS, false, null );
                    }
                    
                    G_CenterPrintMsg( null, "-" + this.countDown + "-\n" );
                }
            }
        }

        // if one of the teams has no player alive move from DRACE_ROUNDSTATE_ROUND
        if ( this.state == DRACE_ROUNDSTATE_ROUND )
        {
            cEntity @ent;
            cTeam @team;
            int count;

            @team = @G_GetTeam( TEAM_PLAYERS );
            count = 0;

            for ( int j = 0; @team.ent( j ) != null; j++ )
            {
                @ent = @team.ent( j );
                if ( !ent.isGhosting() )
                    count++;
            }

            if ( count < 2 )
                this.newRoundState( this.state + 1 );
        }
    }

    void playerKilled( cEntity @target, cEntity @attacker, cEntity @inflicter )
    {
        if ( this.state != DRACE_ROUNDSTATE_ROUND )
            return;

        if ( @target == null || @target.client == null )
            return;

        if ( @attacker == null || @attacker.client == null )
            return;

        target.client.printMessage( "You were killed by " + attacker.client.getName() + " (health: " + int( attacker.health ) + ", armor: " + int( attacker.client.armor ) + ")\n" );

        for ( int i = 0; i < maxClients; i++ )
        {
            cClient @client = @G_GetClient( i );
            if ( client.state() < CS_SPAWNED )
                continue;

            if ( @client == @this.roundWinner || @client == @this.roundChallenger )
                continue;

            client.printMessage( target.client.getName() + " was killed by " + attacker.client.getName() + " (health: " + int( attacker.health ) + ", armor: " + int( attacker.client.armor ) + ")\n" );
        }
        
        Racesow_GetPlayerByClient( target.client ).cancelRace();
    }
    
    bool checkMaxVictories( cClient @client )
    {
        //we only check the max victories if the CVAR is set to 1 or more
        if ( g_drace_max_victories.get_value() > 0 )
        {
          //if not the same player, nbVictories set to 0, and last winner to current player
          if ( @client != @lastWinner )
          {
            @this.lastWinner = @client;
            this.nbVictories = 0;
          }
          
          // number of victories incremented, and compared to the max autorized
          this.nbVictories++;
          
          if ( this.nbVictories >= g_drace_max_victories.get_integer() )
          {
            //reset
            @this.lastWinner = null;
            this.nbVictories = 0;
            return false;
          }
        }
        return true;
    }

}

cDRACERound DRACERound;

class Racesow_Gametype_Drace : Racesow_Gametype
{
    Racesow_Gametype_Drace()
    {
        for( int i= 0; i < maxClients; i++ )
            @this.players[i] = @Racesow_Player_Drace();

        //@this.commandMap["callvotevalidate"] = @Command_CallvoteValidateDrace();
        //@this.commandMap["callvotecheckpermission"] = @Command_CallvoteCheckPermissionDrace();
        //@this.commandMap["callvotepassed"] = @Command_CallvotePassed();
        @this.commandMap["join"] = @Command_Join();
        @this.commandMap["spec"] = @Command_Spec();
        @this.commandMap["chase"] = @Command_Spec();
        @this.commandMap["racerestart"] = @Command_RaceRestart_Drace();
        @this.commandMap["resetcam"] = @Command_ResetCam();
        @this.commandMap["top"] = @Command_Top();
        @this.commandMap["whoisgod"] = @Command_WhoIsGod("Jerm's");

        @this.commandMap["draw"] = @Command_CallvoteDraw();
        @this.commandMap["max_victories"] = @Command_CallvoteMaxVictories();
    }
    
    void InitGametype()
    {
        gametype.setTitle( "Duel Race" );
        DRACERound.init();
    
        // if the gametype doesn't have a config file, create it
        if ( !G_FileExists( "configs/server/gametypes/drace.cfg" ) )
        {
            String config;
    
            // the config file doesn't exist or it's empty, create it
            config = "// '" + gametype.getTitle() + "' gametype configuration file\n"
                     + "// This config will be executed each time the gametype is started\n"
                     + "\n// game settings\n"
                     + "set g_scorelimit \"11\"\n"
                     + "set g_timelimit \"0\"\n"
                     + "set g_warmup_enabled \"1\"\n"
                     + "set g_warmup_timelimit \"5\"\n"
                     + "set g_match_extendedtime \"0\"\n"
                     + "set g_teams_maxplayers \"0\"\n"
                     + "set g_teams_allow_uneven \"0\"\n"
                     + "set g_countdown_time \"3\"\n"
                     + "set g_maxtimeouts \"3\" // -1 = unlimited\n"
                     + "set g_challengers_queue \"0\"\n"
                     + "set g_inactivity_maxtime \"360.0\"\n"
                     + "set g_drace_max_victories \"0\"\n"
    
                     + "\necho drace.cfg executed\"\n";
    
            G_WriteFile( "configs/server/gametypes/drace.cfg", config );
            G_Print( "Created default config file for 'drace'\n" );
            G_CmdExecute( "exec configs/server/gametypes/drace.cfg silent" );
        }

        gametype.isTeamBased = false;
        gametype.hasChallengersQueue = false;
        gametype.maxPlayersPerTeam = 0;
        gametype.spawnpointRadius = 0;
        gametype.mathAbortDisabled = false;
        gametype.autoInactivityRemove = true;
        gametype.playerInteraction = false;
        gametype.freestyleMapFix = false;
        gametype.enableDrowning = true;

        // define the scoreboard layout
        G_ConfigString( CS_SCB_PLAYERTAB_LAYOUT, "%n 112 %s 52 %i 52 %t 96 %l 48 %b 50 %p 18" );
        G_ConfigString( CS_SCB_PLAYERTAB_TITLES, "Name Clan Score Time Ping Racing R" );
        
        // add callvotes
//        G_RegisterCallvote( "draw", "", "Declares the current round Draw." ) ;
//        G_RegisterCallvote( "max_victories", "<number>", "Maximum successive victories." ) ;
    
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
        	String itemClassname = Item.getClassname();
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
                && incomingMatchState < MATCH_STATE_POSTMATCH ) {
            match.startAutorecord();
            demoRecording = false;
        }
    
        if ( match.getState() == MATCH_STATE_POSTMATCH ) {
            match.stopAutorecord();
            demoRecording = false;
        }
        
        return true;
    }
    
    void MatchStateStarted()
    {
      switch ( match.getState() )
      {
      case MATCH_STATE_WARMUP:
          DRACE_SetUpWarmup();
          break;
  
      case MATCH_STATE_COUNTDOWN:
          DRACE_SetUpCountdown();
          break;
  
      case MATCH_STATE_PLAYTIME:
          DRACERound.newGame();
          break;
  
      case MATCH_STATE_POSTMATCH:
          gametype.pickableItemsMask = 0;
          gametype.dropableItemsMask = 0;
          DRACERound.endGame();
          break;
  
      default:
          break;
      }
    }
    
    void ThinkRules()
    {
        if ( match.scoreLimitHit() || match.timeLimitHit() || match.suddenDeathFinished() )
            match.launchState( match.getState() + 1 );
    
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
                client.setHUDStat( STAT_TIME_SELF, Racesow_GetPlayerByClient( client ).race.getCurrentTime() / 100 );
    
            client.setHUDStat( STAT_TIME_BEST, Racesow_GetPlayerByClient( client ).bestRaceTime / 100 );
            client.setHUDStat( STAT_TIME_RECORD, map.getHighScore().getTime() / 100 );
    
            client.setHUDStat( STAT_TIME_ALPHA, -9999 );
            client.setHUDStat( STAT_TIME_BETA, -9999 );
        }
        
       DRACERound.think();
    }
    
    void playerRespawn( cEntity @ent, int old_team, int new_team )
    {
      Racesow_Player @player = Racesow_GetPlayerByClient( ent.client );
      player.cancelRace();

      if ( old_team != new_team )
      {
          DRACERound.playerTeamChanged( ent.client, new_team );
      }

      if ( ent.isGhosting() )
          return;

      if ( DRACERound.state == DRACE_ROUNDSTATE_ROUND && new_team == TEAM_PLAYERS )
      {
          ent.client.respawn( true );
          ent.client.chaseCam( null, true );
          player.sendErrorMessage("You can't join during round.");
          return;
      }
      
      // set player movement to pass through other players and remove gunblade auto attacking
      ent.client.setPMoveFeatures( ent.client.pmoveFeatures & ~PMFEAT_GUNBLADEAUTOATTACK | PMFEAT_GHOSTMOVE );
      ent.client.inventorySetCount( WEAP_GUNBLADE, 1 );
    }
    
    void scoreEvent( cClient @client, String &score_event, String &args )
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
          DRACERound.playerKilled( G_GetEntity( arg1 ), attacker, G_GetEntity( arg2 ) );
      }
      else if ( score_event == "award" )
      {
      }
      else if ( score_event == "enterGame" )
      {
          Racesow_Player @player = Racesow_GetPlayerByClient( client );
          player.reset();
      }
    }
    
    String @ScoreboardMessage( uint maxlen )
    {
      String scoreboardMessage = "";
      String entry;
      cTeam @team;
      cClient @client;
      cEntity @ent;
      int i, readyIcon, playerID;
      int racing;
  
      @team = @G_GetTeam( TEAM_PLAYERS );
  
      // &t = team tab, team tag, team score (doesn't apply), team ping (doesn't apply)
      entry = "&t " + int( TEAM_PLAYERS ) + " " + team.stats.score + " 0 ";
      if ( scoreboardMessage.len() + entry.len() < maxlen )
          scoreboardMessage += entry;
  
      // add first the two players in the duel
      @client = @DRACERound.roundWinner;
      if ( @client != null )
      {
          if ( client.isReady() )
              readyIcon = prcYesIcon;
          else
              readyIcon = 0;
  
          if ( match.getState() != MATCH_STATE_PLAYTIME )
              playerID = client.playerNum();
          else
              playerID = client.getEnt().isGhosting() ? -( client.playerNum() + 1 ) : client.playerNum();
  
          racing = int( Racesow_GetPlayerByClient( client ).isRacing() ? 1 : 0 );
          entry = "&p " + playerID + " "
                  + client.getClanName() + " "
                  + client.stats.score + " "
                  + Racesow_GetPlayerByClient( client ).bestRaceTime + " "
                  + client.ping + " "
                  + racing + " "
                  + readyIcon + " ";
  
          if ( scoreboardMessage.len() + entry.len() < maxlen )
              scoreboardMessage += entry;
      }
  
      @client = @DRACERound.roundChallenger;
      if ( @client != null )
      {
          if ( client.isReady() )
              readyIcon = prcYesIcon;
          else
              readyIcon = 0;
  
          if ( match.getState() != MATCH_STATE_PLAYTIME )
              playerID = client.playerNum();
          else
              playerID = client.getEnt().isGhosting() ? -( client.playerNum() + 1 ) : client.playerNum();
  
          racing = int( Racesow_GetPlayerByClient( client ).isRacing() ? 1 : 0 );
          entry = "&p " + playerID + " "
                  + client.getClanName() + " "
                  + client.stats.score + " "
                  + Racesow_GetPlayerByClient( client ).bestRaceTime + " "
                  + client.ping + " "
                  + racing + " "
                  + readyIcon + " ";
  
          if ( scoreboardMessage.len() + entry.len() < maxlen )
              scoreboardMessage += entry;
      }
  
      // then add all the players in the queue
      //"Name Clan Score Time Ping Racing R"
      for ( i = 0; i < maxClients; i++ )
      {
          if ( DRACERound.DRACEChallengersQueue[i] < 0 || DRACERound.DRACEChallengersQueue[i] >= maxClients )
              break;
  
          @client = @G_GetClient( DRACERound.DRACEChallengersQueue[i] );
          if ( @client == null )
              break;
  
          if ( client.isReady() )
              readyIcon = prcYesIcon;
          else
              readyIcon = 0;
  
          if ( match.getState() != MATCH_STATE_PLAYTIME )
              playerID = client.playerNum();
          else
              playerID = client.getEnt().isGhosting() ? -( client.playerNum() + 1 ) : client.playerNum();
          
          racing = int( Racesow_GetPlayerByClient( client ).isRacing() ? 1 : 0 );
  
          entry = "&p " + playerID + " "
                  + client.getClanName() + " "
                  + client.stats.score + " "
                  + Racesow_GetPlayerByClient( client ).bestRaceTime + " "
                  + client.ping + " "
                  + racing + " "
                  + readyIcon + " ";
  
          if ( scoreboardMessage.len() + entry.len() < maxlen )
              scoreboardMessage += entry;
      }
      
      return scoreboardMessage;
    }
    
    cEntity @SelectSpawnPoint( cEntity @self )
    {
        return GENERIC_SelectBestRandomSpawnPoint( self, "info_player_deathmatch" );
    }
    
    bool UpdateBotStatus( cEntity @self )
    {
        return false;
    }
    
    /*bool Command( cClient @client, String @cmdString, String @argsString, int argc )
    {
        if (cmdString == "resetcam")
        {
          if ( @client == @DRACERound.roundWinner || @client == @DRACERound.roundChallenger )
          {
            //nothing to do if it is winner or challenger
          }
          else
            client.chaseCam( null, true );
          
          return true;
        }
        else if (cmdString == "max_victories")
        {
          G_PrintMsg( client.getEnt(), "Current: " + g_drace_max_victories.get_string() + "\n" );      
          return true;
        }
      	else if ( cmdString == "callvotevalidate" )
      	{
          String vote = argsString.getToken( 0 );
           
          if ( vote == "draw" )
          {
            if ( match.getState() == MATCH_STATE_WARMUP )
          	{
              G_PrintMsg( client.getEnt(), S_COLOR_RED + "Callvote draw unavailable in warmup\n");
          	  return false;
          	}
          	if (client.muted == 2 )
            {
              G_PrintMsg( client.getEnt(), S_COLOR_RED + "You are votemuted\n");
          	  return false;
          	}
            return true;
          }
          
          if ( vote == "max_victories" )
          {
            //if ( argsString.getToken( 1 ) )
              //return false;
              
            return true;
          }
          
          G_PrintMsg( client.getEnt(), S_COLOR_RED + "Unknown callvote " + vote + "\n" );
          return false;
      	}
      	else if ( cmdString == "callvotepassed" )
      	{
            String vote = argsString.getToken( 0 );
      


            if ( vote == "draw" )
            {
              if ( @DRACERound.roundWinner != null )
            	 DRACERound.roundWinner.getEnt().health = -1;
            	 
            	if ( @DRACERound.roundChallenger != null )
            	 DRACERound.roundChallenger.getEnt().health = -1;
            	 
            	return true;
            }
            
            if ( vote == "max_victories" )
            {
              G_CmdExecute( "g_drace_max_victories " + argsString.getToken( 1 ).toInt() + "\n" );
              return true;
            }
            
            return false;
        }
        return false;
    }*/
}

// a player has just died. The script is warned about it so it can account scores
void DRACE_playerKilled( cEntity @target, cEntity @attacker, cEntity @inflicter )
{
    if ( @target == null || @target.client == null )
      return;

    Racesow_GetPlayerByClient( target.client ).cancelRace();
}

void DRACE_SetUpWarmup()
{
    GENERIC_SetUpWarmup();

    gametype.pickableItemsMask = gametype.spawnableItemsMask;
    gametype.dropableItemsMask = gametype.spawnableItemsMask;
    
    // set spawnsystem type to instant while players join
    for ( int team = TEAM_PLAYERS; team < GS_MAX_TEAMS; team++ )
        gametype.setTeamSpawnsystem( team, SPAWNSYSTEM_INSTANT, 0, 0, false );

    gametype.readyAnnouncementEnabled = true;
}

void DRACE_SetUpCountdown()
{
    gametype.shootingDisabled = true;
    gametype.readyAnnouncementEnabled = false;
    gametype.scoreAnnouncementEnabled = false;
    gametype.countdownEnabled = false;
    G_RemoveAllProjectiles();

    // lock teams
    bool any_bool = false; //FIXME: any is now a keyword. i've changed it to any_bool but it really should get a more clarifying name from someone who knows what it's for -K1ll
    if ( gametype.isTeamBased )
    {
        for ( int team = TEAM_ALPHA; team < GS_MAX_TEAMS; team++ )
        {
            if ( G_GetTeam( team ).lock() )
                any_bool = true;
        }
    }
    else
    {
        if ( G_GetTeam( TEAM_PLAYERS ).lock() )
            any_bool = true;
    }

    if ( any_bool )
        G_PrintMsg( null, "Teams locked.\n" );

    // Countdowns should be made entirely client side, because we now can

    int soundIndex = G_SoundIndex( "sounds/announcer/countdown/get_ready_to_fight0" + int( brandom( 1, 2 ) ) );
    G_AnnouncerSound( null, soundIndex, GS_MAX_TEAMS, false, null );
}
void DRACE_SetUpEndMatch()
{
    cClient @client;

    gametype.shootingDisabled = true;
    gametype.readyAnnouncementEnabled = false;
    gametype.scoreAnnouncementEnabled = false;
    gametype.countdownEnabled = false;

    for ( int i = 0; i < maxClients; i++ )
    {
        @client = @G_GetClient( i );

        if ( client.state() >= CS_SPAWNED )
            client.respawn( true ); // ghost them all
    }

    int soundIndex = G_SoundIndex( "sounds/announcer/postmatch/game_over0" + int( brandom( 1, 2 ) ) );
    G_AnnouncerSound( null, soundIndex, GS_MAX_TEAMS, true, null );
}

Racesow_Gametype@ getRacesowGametype() {
    return @Racesow_Gametype_Drace();
}

class Racesow_Player_Drace : Racesow_Player
{
    void touchStopTimer_gametype()
    {
        // we kill the player who lost
        if (@DRACERound.roundChallenger != null) {
            if (@this.client == @DRACERound.roundWinner)
                DRACERound.roundChallenger.getEnt().health = -1;
            }

            if (@DRACERound.roundWinner != null) {
                if (@this.client == @DRACERound.roundChallenger)
                    DRACERound.roundWinner.getEnt().health = -1;
            }

            this.raceCallback(0,0,0,
                              this.bestRaceTime,
                              map.getHighScore().getTime(),
                              this.race.getTime());
    }

    /**
	 * restartRace
	 * @return void
	 * for raceRestart and practicing mode.
	 */
	void restartRace()
	{
        if ( @this.client != null )
        {
            this.cancelRace();
            this.client.respawn( false );
        }
	}
}


/*
 * Commands only used in this gametype
 */
class Command_RaceRestart_Drace : Command_RaceRestart
{
    bool validate(Racesow_Player @player, String &args, int argc)
    {
        if( !Command_RaceRestart::validate( player, args, argc ) )
            return false;
        //racerestart command is only avaiblable during WARMUP
        if ( match.getState() == MATCH_STATE_WARMUP )
            return true;
        player.sendErrorMessage( "This command is only avaiblable during WARMUP." );
        return false;
    }
}

class Command_ResetCam : Racesow_Command
{

    Command_ResetCam() {
        super( "Reset the Camera", "" ); //FIXME: Dunno if everything is right here
    }

    bool execute(Racesow_Player @player, String &args, int argc)
    {
          if ( @player.getClient() == @DRACERound.roundWinner || @player.getClient() == @DRACERound.roundChallenger )
          {
            //nothing to do if it is winner or challenger
          }
          else
            player.getClient().chaseCam( null, true );
          
          return true;
    }
}

class Command_MaxVictories : Racesow_Command
{

    Command_MaxVictories() {
        super( "", "" ); //FIXME: Description needed
    }

    bool execute(Racesow_Player @player, String &args, int argc)
    {
          G_PrintMsg( player.getClient().getEnt(), "Current: " + g_drace_max_victories.getString() + "\n" ); // why not player.sendMessage() ?
          return true;
    }
}

class Command_CallvoteDraw : Racesow_Command
{
    Command_CallvoteDraw()
    {
        super( "Declares the current round Draw.", "" );
    }

    bool validate( Racesow_Player @player, String &args, int argc )
    {
        if( match.getState() == MATCH_STATE_WARMUP )
        {
            G_PrintMsg( player.getClient().getEnt(), S_COLOR_RED + "Callvote draw unavailable in warmup\n" );
            return false;
        }
        if( player.getClient().muted == 2 )
        {
            G_PrintMsg( player.getClient().getEnt(), S_COLOR_RED + "You are votemuted\n" );
            return false;
        }
        return true;
    }

    bool execute( Racesow_Player @player, String &args, int argc )
    {
        if( @DRACERound.roundWinner != null )
            DRACERound.roundWinner.getEnt().health = -1;

        if( @DRACERound.roundChallenger != null )
            DRACERound.roundChallenger.getEnt().health = -1;

        return true;
    }
}

class Command_CallvoteMaxVictories : Racesow_Command
{
    Command_CallvoteMaxVictories()
    {
        super( "Maximum successive victories.", "<number>" );
    }

    bool execute( Racesow_Player @player, String &args, int argc )
    {
        G_CmdExecute( "g_drace_max_victories " + args.getToken( 1 ).toInt() + "\n" );
        return true;
    }
}

//FIXME: K1ll - I don't know if this should extend or replace the original object
//class Command_CallvoteValidateDrace : Command_CallvoteValidate
//{
//    bool gametypeVotes( Racesow_Player @player, String &args, int argc ) {
//        String vote = args.getToken( 0 );
//
//          if ( vote == "draw" )
//          {
//            if ( match.getState() == MATCH_STATE_WARMUP )
//          	{
//              G_PrintMsg( player.getClient().getEnt(), S_COLOR_RED + "Callvote draw unavailable in warmup\n");
//          	  return false;
//          	}
//          	if (player.getClient().muted == 2 )
//            {
//              G_PrintMsg( player.getClient().getEnt(), S_COLOR_RED + "You are votemuted\n");
//          	  return false;
//          	}
//            return true;
//          }
//
//
//          if ( vote == "max_victories" )
//          {
//            //if ( argsString.getToken( 1 ) )
//              //return false;
//
//            return true;
//
//          }
//
//          G_PrintMsg( player.getClient().getEnt(), S_COLOR_RED + "Unknown callvote " + vote + "\n" );
//          return false;
//    }
//}

/*class Command_CallvoteCheckPermissionDrace : Command_CallvoteCheckPermission
{

    bool gametypeVotes( Racesow_Player @player, String &args, int argc ) {
            String vote = argsString.getToken( 0 );

            if ( vote == "draw" )
            {
              if ( @DRACERound.roundWinner != null )
            	 DRACERound.roundWinner.getEnt().health = -1;
            	 

            	if ( @DRACERound.roundChallenger != null )
            	 DRACERound.roundChallenger.getEnt().health = -1;
            	 
            	return true;
            }

            
            if ( vote == "max_victories" )
            {
              G_CmdExecute( "g_drace_max_victories " + argsString.getToken( 1 ).toInt() + "\n" );
              return true;

            }
            
            return false;

        }
        return false;
    }

}*/

//class Command_CallvotePassed : Racesow_Command
//{
//    Command_CallvotePassed() {
//        super("callvotepassed", "", ""); //FIXME: Description needed
//    }
//
//    bool execute(Racesow_Player @player, String &args, int argc)
//    {
//            String vote = args.getToken( 0 );
//
//            if ( vote == "draw" )
//            {
//              if ( @DRACERound.roundWinner != null )
//            	 DRACERound.roundWinner.getEnt().health = -1;
//
//            	if ( @DRACERound.roundChallenger != null )
//            	 DRACERound.roundChallenger.getEnt().health = -1;
//
//            	return true;
//            }
//
//            if ( vote == "max_victories" )
//            {
//              G_CmdExecute( "g_drace_max_victories " + args.getToken( 1 ).toInt() + "\n" );
//              return true;
//            }
//
//            return false;
//    }
//}
