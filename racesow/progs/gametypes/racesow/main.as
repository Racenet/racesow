/**
 * Racesow Gametype Interface
 *
 * based on warsow 0.5 race gametype
 * @version 0.6.2
 */

int numCheckpoints = 0;
bool demoRecording = false;
int oldTimelimit; // for restoring the original value, because extend_time changes it

String playerList; //scoreboard message for custom scoreboards
String spectatorList; //list of all spectators for custom scoreboards
uint scoreboardLastUpdate; //when got the scoreboard updated? (levelTime)
bool scoreboardUpdated = false; //GT_ScoreboardMessage got called

String previousMapName; // to remember the previous map on the server

Racesow_Map @map;
Racesow_Adapter_Abstract @racesowAdapter;
Racesow_Gametype @racesowGametype;

int prcFlagIconStolen;
int prcYesIcon;

Cvar dedicated( "dedicated", "0", CVAR_ARCHIVE);
Cvar rs_authField_Name( "rs_authField_Name", "", CVAR_ARCHIVE|CVAR_NOSET );
Cvar rs_authField_Pass( "rs_authField_Pass", "", CVAR_ARCHIVE|CVAR_NOSET );
Cvar rs_authField_Token( "rs_authField_Token", "", CVAR_ARCHIVE|CVAR_NOSET );
Cvar rs_networkName( "rs_networkName", "racenet", CVAR_ARCHIVE|CVAR_NOSET );
Cvar rs_extendtimeperiod( "rs_extendtimeperiod", "3", CVAR_ARCHIVE );
Cvar rs_loadHighscores( "rs_loadHighscores", "0", CVAR_ARCHIVE );
Cvar rs_loadPlayerCheckpoints( "rs_loadPlayerCheckpoints", "0", CVAR_ARCHIVE );
Cvar rs_allowAutoHop( "rs_allowAutoHop", "1", CVAR_ARCHIVE );

Cvar g_allowammoswitch( "g_allowammoswitch", "0", CVAR_ARCHIVE|CVAR_NOSET );
Cvar g_timelimit_reset( "g_timelimit_reset", "1", CVAR_ARCHIVE|CVAR_NOSET );
Cvar g_timelimit( "g_timelimit", "20", CVAR_ARCHIVE );
Cvar g_extendtime( "g_extendtime", "10", CVAR_ARCHIVE );
Cvar g_maprotation( "g_maprotation", "1", CVAR_ARCHIVE );
Cvar g_warmup_timelimit( "g_warmup_timelimit", "0", CVAR_ARCHIVE ); //cvar g_warmup_enabled was removed in warsow 0.6
Cvar g_gametype( "g_gametype", "race", CVAR_ARCHIVE);

Cvar rs_welcomeMessage ("rs_welcomeMessage", S_COLOR_WHITE + "Welcome to this Racesow server. Type " + S_COLOR_ORANGE + "help" + S_COLOR_WHITE + " to get a list of commands\n", CVAR_ARCHIVE );
Cvar rs_registrationDisabled( "rs_registrationDisabled", "0", CVAR_ARCHIVE|CVAR_NOSET );
Cvar rs_registrationInfo( "rs_registrationInfo", "Please ask the serveradmin how to create a new account.", CVAR_ARCHIVE|CVAR_NOSET );

Cvar sv_cheats( "sv_cheats", "0", CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_NOSET );

Cvar g_gravity( "g_gravity", "850", CVAR_ARCHIVE );
Cvar g_self_knockback( "g_self_knockback", "1.18", CVAR_ARCHIVE);

/**
 * Determines if the current client is using Racesow game library
 * @param cClient @client
 * @return bool
 */
bool isUsingRacesowClient( cClient @client )
{
	// TODO: use something like a client userinfo variable to determine if the player is running racesow client
	return false;
}

/**
 * GT_Command
 *
 * @param cClient @client,
 * @param String &cmdString
 * @param String &argsString
 * @param int argc
 * @return void
 */
bool GT_Command( cClient @client, String &cmdString, String &argsString, int argc )
{
    //We let the gametype handle everything
    return racesowGametype.Command( client, cmdString, argsString, argc );
}

/**
 * GT_UpdateBotStatus (maybe we can use this for race record display using a bot? :o)
 *
 * When this function is called the weights of items have been reset to their default values,
 * this means, the weights *are set*, and what this function does is scaling them depending
 * on the current bot status.
 * Player, and non-item entities don't have any weight set. So they will be ignored by the bot
 * unless a weight is assigned here.
 *
 * @param cEntity @self
 * @return bool
 */
bool GT_UpdateBotStatus( cEntity @self )
{
    return racesowGametype.UpdateBotStatus( @self );
}

/**
 * GT_SelectSpawnPoint
 *
 * select a spawning point for a player
 * @param cEntity @self
 * @return cEntity
 */
cEntity @GT_SelectSpawnPoint( cEntity @self )
{
	Racesow_Player @player = racesowGametype.getPlayer(self.client);
	if(@player != null)
	    player.onSpawn();
	return racesowGametype.SelectSpawnPoint( @self );
}

/**
 * GT_ScoreboardMessage
 * @param int maxlen
 * @return String
 */
String @GT_ScoreboardMessage( uint maxlen )
{
    String @scoreboardMessage = @racesowGametype.ScoreboardMessage( maxlen );

    //custom scoreboard for ppl who are getting spectated
    if( levelTime > scoreboardLastUpdate + 1800 )
    {
        for ( int i = 0; i < maxClients; i++ )
        {
            if(@racesowGametype.players[i] != null)
                racesowGametype.players[i].challengerList = "";
        }
        cTeam @spectators = @G_GetTeam( TEAM_SPECTATOR );
        cEntity @other;
        spectatorList = "";
        for ( int i = 0; @spectators.ent( i ) != null; i++ )
        {
            @other = @spectators.ent( i );
            if ( @other.client != null )
            {
                if( !other.client.connecting && other.client.state() >= CS_SPAWNED )
                    //add all other spectators
                {
                    spectatorList += other.client.playerNum + " " + other.client.ping + " ";
                }
                else if( other.client.connecting ) //add connecting spectators
                {
                    spectatorList += other.client.playerNum + " " + -1 + " ";
                }
                if( other.client.chaseActive && other.client.chaseTarget != 0)
                    //add him to the challenger list of the player he's spectating
                {
                    Racesow_Player @player = racesowGametype.players[other.client.chaseTarget-1];
                    player.challengerList += other.client.playerNum + " " + other.client.ping + " ";
                }
            }
        }
        playerList = scoreboardMessage;
        scoreboardLastUpdate = levelTime;
    }
    scoreboardUpdated = true;
    return @scoreboardMessage;
}

/**
 * GT_scoreEvent
 *
 * handles different game events
 *
 * @param cClient @client
 * @param String &score_event
 * @param String &args
 * @return void
 */
void GT_scoreEvent( cClient @client, String &score_event, String &args )
{
    if( @client == null)//basewsw does check that too ("clients can be null")
        return;

	if ( score_event == "connect" )
	{
		RS_ircSendMessage( client.name.removeColorTokens() + " connected" );
	}
	else if ( score_event == "enterGame" )
	{
		racesowGametype.onEnterGame( client );
	}

	Racesow_Player @player = racesowGametype.getPlayer( client );
	if (@player != null )
	{
		if ( score_event == "dmg" )
		{
		}
		else if ( score_event == "kill" )
		{
		}
		else if ( score_event == "award" )
		{
		}
		else if ( score_event == "enterGame" )
		{
			//FIXME: This could be problematic when done in the Players Constructor because it adds a reference to the player object
			// maybe it can be fixed by refactoring -K1ll
			player.appear();
			RS_ircSendMessage( player.getName().removeColorTokens() + " entered the game" );
		}
		else if ( score_event == "disconnect" )
		{
			player.disappear(player.getName(), true);
			RS_ircSendMessage( player.getName().removeColorTokens() + " disconnected" );
			racesowGametype.onDisconnect( client );
		}
		else if ( score_event == "userinfochanged" )
		{
			if( !client.connecting ) {

                player.getAuth().refresh( args );


				// auto-hop check
				if ( rs_allowAutoHop.boolean == false )
				{
					// checking if the player is restoring his autojump (we can't cheatprotect a client variable from the server, can we?)
					if ( client.getUserInfoKey("cg_noAutohop").toInt() == 0 )
					{
						client.setPMoveFeatures( client.pmoveFeatures & ~PMFEAT_CONTINOUSJUMP );
					}
				}
			}
            if( args.removeColorTokens() != player.getName().removeColorTokens() && client.state() >= CS_SPAWNED )
                RS_ircSendMessage( args.removeColorTokens() + " is now known as " + player.getName().removeColorTokens() );
		}
	}
	racesowGametype.scoreEvent( @client, score_event, args );
}

/**
 * GT_playerRespawn
 *
 * a player is being respawned. This can happen from several ways, as dying, changing team,
 * being moved to ghost state, be placed in respawn queue, being spawned from spawn queue, etc
 *
 * @param cEntity @ent
 * @param int old_team
 * @param int new_team
 * @return void
 */
void GT_playerRespawn( cEntity @ent, int old_team, int new_team )

{
	Racesow_Player @player = racesowGametype.getPlayer( ent.client );

  if (new_team == TEAM_PLAYERS) {

      if (map.inOvertime) {
          player.client.team = TEAM_SPECTATOR;
          player.client.respawn( true );
          player.sendMessage(S_COLOR_RED + "No spawning in overtime. Please wait for the other players to finish.\n");
          return;
      }
  }

  racesowGametype.playerRespawn( @ent, old_team, new_team );

  // select rocket launcher if available
  if ( ent.client.canSelectWeapon( WEAP_ROCKETLAUNCHER ) )
      ent.client.selectWeapon( WEAP_ROCKETLAUNCHER );
  else
      ent.client.selectWeapon( -1 ); // auto-select best weapon in the inventory

	// make dash 450
	ent.client.setPMoveDashSpeed( 450 );
}

/**
 * GT_ThinkRules
 *
 * Thinking function. Called each frame
 * @return void
 */
void GT_ThinkRules()
{
	// perform a C callback if there is one pending
	racesowAdapter.thinkCallbackQueue();

	if ( racesowGametype.timelimited || g_maprotation.boolean )
	{

		if ( match.timeLimitHit() )
		{
			match.launchState( match.getState() + 1 );
		}

		map.PrintMinutesLeft();
	}

	// needs to be always executed, because overtime occurs even in time-unlimited mode
	map.allowEndGame();

	// allowEndGame() should -always- be called at each think even during POSTMATCH, hence before this line.
	if ( match.getState() >= MATCH_STATE_POSTMATCH )
	{
		// that piece of code needs to be always executed during postmatch when g_maprotation=0 or freestyle=1
		if ( ( !racesowGametype.timelimited || !g_maprotation.boolean ) && match.timeLimitHit() )
		{
			match.launchState( match.getState() + 1 );
		}

        return;
	}

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

	// set the logTime once
	if ( map.logTime == 0 && localTime != 0 )
		map.logTime = localTime;

    // set all clients race stats
    cClient @client;

    for ( int i = 0; i < maxClients; i++ )
    {
        @client = @G_GetClient( i );

        if ( client.state() < CS_SPAWNED )
            continue;

		Racesow_Player @player = racesowGametype.getPlayer( client );

		player.advanceDistance();

		player.demo.think();

		if( scoreboardUpdated && player.challengerList != "")//send the scoreboard to the player
		{
            String command = "scb \""
                    + playerList + " "
                    + "&s " + spectatorList + " "
                    + "&w " + player.challengerList + "\"";
            client.execGameCommand( command );
		}

		int countdownState;
		if ( 0 != ( countdownState = player.getAuth().wontGiveUpViolatingNickProtection() ) )
		{
		    if ( countdownState == 1 )
            {
		        //player.sendAward(player.getAuth().getViolateCountDown());
                G_PrintMsg( player.getClient().getEnt(), player.getAuth().getViolateCountDown() + "\n" );
            }
            else if ( countdownState == 2 )
            {
		        player.kick( "You violated against the nickname protection." );
            }
		}

        if ( player.printWelcomeMessage and levelTime - player.joinedTime > 1000 )
        {
            player.printWelcomeMessage = false;
            player.sendMessage( rs_welcomeMessage.string + "\n" );
            player.sendMessage( rs_registrationInfo.string + "\n" );
        }

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

    	if( player.isUsingChrono )
    		client.setHUDStat( STAT_TIME_ALPHA, (levelTime - player.chronoTime()) / 100 );
    }

    racesowGametype.ThinkRules();
    if( scoreboardUpdated )
        scoreboardUpdated = false; //custom scoreboard got updated
}

/**
 * GT_MatchStateFinished
 *
 * The game has detected the end of the match state, but it
 * doesn't advance it before calling this function.
 * This function must give permission to move into the next
 * state by returning true.
 *
 * @param int incomingMatchState
 * @return void
 */
bool GT_MatchStateFinished( int incomingMatchState )
{
    if ( match.getState() == MATCH_STATE_POSTMATCH ) // LOL this should not be in here ;)
    {
    	g_timelimit.set(oldTimelimit); //restore the old timelimit
        match.stopAutorecord();
        demoRecording = false;
    }

    return racesowGametype.MatchStateFinished( incomingMatchState );
}

/**
 * GT_MatchStateStarted
 *
 * the match state has just moved into a new state. Here is the
 * place to set up the new state rules
 *
 * @return void
 */
void GT_MatchStateStarted()
{
    racesowGametype.MatchStateStarted();
}

/**
 * GT_Shutdown
 * the gametype is shutting down cause of a match restart or map change
 * @return void
 */
void GT_Shutdown()
{
    if( g_gravity.defaultString != g_gravity.string )
    {
        //some maps might have set a custom g_gravity which normaly won't get restored
        g_gravity.reset();
        G_Print( "Note: custom g_gravity reset\n" );
    }

    for ( int i = 0; i < maxClients; i++ )
		if ( @racesowGametype.players[i] != null )
		{
		    // run it unthreaded to prevent a mysql crash
			racesowGametype.players[i].disappear(racesowGametype.players[i].getName(),false);
		}

    racesowGametype.Shutdown();
    RS_ircSendMessage( "Map changed to: \'" + RS_NextMap() + "\'" );
}

/**
 * GT_SpawnGametype
 * The map entities have just been spawned. The level is initialized for
 * playing, but nothing has yet started.
 *
 * @return void
 */
void GT_SpawnGametype()
{
    bool found = false;

    @map = Racesow_Map();

    if (mysqlConnected != 0) {

        @racesowAdapter = Racesow_Adapter_Full();

    } else {

        @racesowAdapter = Racesow_Adapter_Compat();
    }

    racesowAdapter.initGametype();


	// setup players //FIXME: Necessary? i don't see why -K1ll
    /*for ( int i = 0; i < maxClients; i++ )
        racesowGametype.players[i].reset();*/

    for ( int i = 0; i <= numEntities; i++ )
    {
        cEntity @ent = @G_GetEntity( i );
        if( @ent == null )
            continue;

        if( ent.classname == "trigger_multiple" )
        {
            cEntity @target = @ent.findTargetEntity( @ent );
            if( @target != null && ( ( target.classname == "target_startTimer" )
                            || ( target.classname == "target_starttimer" ) ) )
            {
                ent.wait = 0;
            }
        }
        else if( ent.classname == "target_give" )
        {
            cEntity @target = @ent.findTargetEntity( null );
            if( @target == null )
            {
                G_Print(" WARNING: target_give has no targets\n");
                ent.unlinkEntity();
                ent.freeEntity();
            }
        }
        else if( ent.type == ET_ITEM )
        {
            cItem @Item = @ent.item;
            if( @Item != null && ent.classname == Item.classname )
            {
                if( ( ent.solid != SOLID_NOT ) || ( ( @ent.findTargetingEntity( null ) != null ) && ( ent.findTargetingEntity( null ).classname != "target_give" ) ) ) //ok, not connected to target_give
                {
                    ent.classname = "AS_" + Item.classname;
                    replacementItem( @ent );
                }
            }
        }
    }
    racesowGametype.SpawnGametype();

}

/**
 * GT_InitGametype
 *
 * Important: This function is called before any entity is spawned, and
 * spawning entities from it is forbidden. If you want to make any entity
 * spawning at initialization do it in GT_SpawnGametype, which is called
 * right after the map entities spawning.
 *
 * @return void
 */
void GT_InitGametype()
{
    gametype.title = "Racesow";
    gametype.version = "0.6.2";
    gametype.author = "warsow-race.net";

    // initalize weapondef config
    if( !G_FileExists( "configs/server/gametypes/racesow/racesow_weapondefs.cfg" ) )
    {
        G_WriteFile( "configs/server/gametypes/racesow/racesow_weapondefs.cfg", config_weapondef );
        G_Print( "Created default weapondefs config for 'racesow'\n" );
    }

    if( !G_FileExists( "configs/server/gametypes/racesow/database.cfg" ) )
    {
        G_WriteFile( "configs/server/gametypes/racesow/database.cfg", config_database );
        G_Print( "Created default base config file for database\n" );
    }

    if( !G_FileExists( "configs/server/gametypes/racesow/racesow.cfg" ) )
    {
        G_WriteFile( "configs/server/gametypes/racesow/racesow.cfg", config_general );
        G_Print( "Created default base config file for racesow\n" );
    }

    // always execute racesow.cfg
    G_CmdExecute( "exec configs/server/gametypes/racesow/racesow.cfg silent" );

    gametype.spawnableItemsMask = ( IT_WEAPON | IT_AMMO | IT_ARMOR | IT_POWERUP | IT_HEALTH );
    if ( gametype.isInstagib )
      gametype.spawnableItemsMask &= ~uint(G_INSTAGIB_NEGATE_ITEMMASK);

    gametype.respawnableItemsMask = gametype.spawnableItemsMask;
    gametype.dropableItemsMask = 0;
    gametype.pickableItemsMask = 0;

    gametype.isRace = true;

    gametype.ammoRespawn = 0;
    gametype.armorRespawn = 0;
    gametype.weaponRespawn = 0;
    gametype.healthRespawn = 0;
    gametype.powerupRespawn = 0;
    gametype.megahealthRespawn = 0;
    gametype.ultrahealthRespawn = 0;

    gametype.readyAnnouncementEnabled = false;
    gametype.scoreAnnouncementEnabled = false;
    gametype.countdownEnabled = false;
    gametype.mathAbortDisabled = true;
    gametype.shootingDisabled = false;
    gametype.infiniteAmmo = true;
    gametype.canForceModels = true;
    gametype.canShowMinimap = false;
    gametype.teamOnlyMinimap = true;



    // set spawnsystem type
    for ( int team = TEAM_PLAYERS; team < GS_MAX_TEAMS; team++ )
      gametype.setTeamSpawnsystem( team, SPAWNSYSTEM_INSTANT, 0, 0, false );

    // precache images that can be used by the scoreboard
    prcYesIcon = G_ImageIndex( "gfx/hud/icons/vsay/yes" );
    prcFlagIconStolen = G_ImageIndex( "gfx/hud/icons/flags/iconflag_stolen" );

    // add commands
    //RS_InitCommands();

    // weapondef not needed anymore, we're not testing weapons
    //G_RegisterCommand( "weapondef" );

    /*G_RegisterCommand( "ammoswitch" );
    G_RegisterCommand( "whoisgod" );*/

    //add callvotes
//    G_RegisterCallvote( "extend_time", "", "Extends the matchtime." );
//    G_RegisterCallvote( "timelimit", "<minutes>", "Set match timelimit." );
//    G_RegisterCallvote( "spec", "", "During overtime, move all players to spectators." );
//    G_RegisterCallvote( "joinlock", "<id or name>", "Prevent the player from joining the game." );
//    G_RegisterCallvote( "joinunlock", "<id or name>", "Allow the player to join the game." );

    demoRecording = false;

	if ( G_Md5( "www.warsow-race.net" ) != "bdd5b303ccc88e5c63ce71bfc250a561" )
	{
		G_Print( "* " + S_COLOR_RED + "MD5 hashing test failed!!!\n" );
	}
	else
	{
		G_Print( "* " + S_COLOR_GREEN + "MD5 hashing works fine...\n" );
	}

	g_self_knockback.forceSet("1.25"); // 1.18 in basewsw.6

	//store g_timelimit for restoring it at the end of the map (it will be altered by extend_time votes)
	oldTimelimit = g_timelimit.integer;

    @racesowGametype = @getRacesowGametype();

	racesowGametype.InitGametype();

    racesowGametype.LoadMapList();

    racesowGametype.registerCommands();
    racesowGametype.registerVotes();
}

