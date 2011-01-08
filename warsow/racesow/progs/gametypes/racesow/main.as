/**
 * Racesow Gametype Interface
 *
 * based on warsow 0.5 race gametype
 * @version 0.6.0
 */

int numCheckpoints = 0;
bool demoRecording = false;
int oldTimelimit; // for restoring the original value, because extend_time changes it

cString playerList; //scoreboard message for custom scoreboards
cString spectatorList; //list of all spectators for custom scoreboards
uint scoreboardLastUpdate; //when got the scoreboard updated? (levelTime)
bool scoreboardUpdated = false; //GT_ScoreboardMessage got called

cString previousMapName; // to remember the previous map on the server

Racesow_Player[] players( maxClients );
Racesow_Map @map;
Racesow_Adapter_Abstract @racesowAdapter;

int prcFlagIconStolen;

cVar rs_authField_Name( "rs_authField_Name", "", CVAR_ARCHIVE|CVAR_NOSET );
cVar rs_authField_Pass( "rs_authField_Pass", "", CVAR_ARCHIVE|CVAR_NOSET );
cVar rs_authField_Token( "rs_authField_Token", "", CVAR_ARCHIVE|CVAR_NOSET );
cVar rs_networkName( "rs_networkName", "racenet", CVAR_ARCHIVE|CVAR_NOSET );
cVar rs_extendtimeperiod( "rs_extendtimeperiod", "3", CVAR_ARCHIVE );
cVar rs_loadHighscores( "rs_loadHighscores", "0", CVAR_ARCHIVE );
cVar rs_loadPlayerCheckpoints( "rs_loadPlayerCheckpoints", "0", CVAR_ARCHIVE );
cVar rs_allowAutoHop( "rs_allowAutoHop", "1", CVAR_ARCHIVE );

cVar g_freestyle( "g_freestyle", "1", CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_NOSET );
cVar g_allowammoswitch( "g_allowammoswitch", "0", CVAR_ARCHIVE|CVAR_NOSET );
cVar g_timelimit_reset( "g_timelimit_reset", "1", CVAR_ARCHIVE|CVAR_NOSET );
cVar g_timelimit( "g_timelimit", "20", CVAR_ARCHIVE );
cVar g_extendtime( "g_extendtime", "10", CVAR_ARCHIVE );
cVar g_maprotation( "g_maprotation", "1", CVAR_ARCHIVE );
cVar g_warmup_timelimit( "g_warmup_timelimit", "0", CVAR_ARCHIVE ); //cvar g_warmup_enabled was removed in warsow 0.6

cVar rs_welcomeMessage ("rs_welcomeMessage", S_COLOR_WHITE + "Welcome to this Racesow server. Type " + S_COLOR_ORANGE + "help" + S_COLOR_WHITE + " to get a list of commands\n", CVAR_ARCHIVE );
cVar rs_registrationDisabled( "rs_registrationDisabled", "0", CVAR_ARCHIVE|CVAR_NOSET );
cVar rs_registrationInfo( "rs_registrationInfo", "Please ask the serveradmin how to create a new account.", CVAR_ARCHIVE|CVAR_NOSET );

cVar sv_cheats( "sv_cheats", "0", CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_NOSET );

cVar g_gravity( "g_gravity", "850", CVAR_ARCHIVE );
cVar g_self_knockback( "g_self_knockback", "1.18", CVAR_ARCHIVE);

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
 * GT_Command
 *
 * @param cClient @client,
 * @param cString &cmdString
 * @param cString &argsString
 * @param int argc
 * @return void
 */
bool GT_Command( cClient @client, cString &cmdString, cString &argsString, int argc )
{
	Racesow_Player @player = Racesow_GetPlayerByClient( client );
	Racesow_Command@ command = RS_GetCommandByName( cmdString );

	if ( command !is null )
	{
	    player.executeCommand(command, argsString, argc);
	}

	else if ( cmdString == "whoisgod" )
	{
	    cString[] devs = { "R2", "Zaran", "Zolex", "Schaaf", "K1ll", "Weqo" };
	    int index = brandom(0, 6);
	    player.sendMessage( devs[index] + "\n" );
	}

	else if ( ( cmdString == "ammoswitch" ) )
	{
		return player.ammoSwitch();
	}

   	else if ( ( cmdString == "callvotecheckpermission" ) )
	{
		if( player.isVotemuted )
		{
			sendMessage( S_COLOR_RED + "You are votemuted\n", @client );
			return false;
		}
		else
		{
			cString vote = argsString.getToken( 0 );
			if( vote == "mute" || vote == "vmute" ||
					vote == "kickban" || vote == "kick" || vote == "remove" )
			{
				Racesow_Player @victimPlayer;
				cString victim = argsString.getToken( 1 );

				if ( Racesow_GetClientNumber( victim ) != -1)
					@victimPlayer = players[ Racesow_GetClientNumber( victim ) ];
				else if( victim.isNumerical() )
					if ( victim.toInt() > maxClients )
						return true;
					else
						@victimPlayer = players[ victim.toInt() ];
				else
					return true;

				if( victimPlayer.auth.allow( RACESOW_AUTH_ADMIN ))
				{
					G_PrintMsg( null, S_COLOR_WHITE + player.getName() + S_COLOR_RED
						+ " tried to " + argsString.getToken( 0 ) + " an admin.\n" );
					return false;
				}
				else
				{
					return true;
				}
			}
			else
			{
				return true;
			}
		}
	}

	else if ( cmdString == "callvotevalidate" )
	{
		cString vote = argsString.getToken( 0 );

		if ( vote == "extend_time" )
		{
			if( g_timelimit.getInteger() <= 0 )
			{
				client.printMessage( "This vote is only available for timelimits.\n");
				return false;
			}
			uint timelimit = g_timelimit.getInteger() * 60000;//convert mins to ms
			uint extendtimeperiod = rs_extendtimeperiod.getInteger() * 60000;//convert mins to ms
			uint time = levelTime - match.startTime(); //in ms
			uint remainingtime = timelimit - time;
			bool isNegative = (timelimit < time ) ? true : false;
			if( remainingtime > extendtimeperiod && !isNegative )
			{
				client.printMessage( "This vote is only in the last " + rs_extendtimeperiod.getString() + " minutes available.\n" );
				return false;
			}
			return true;
		}

		if ( vote == "timelimit" )
		{
			int new_timelimit = argsString.getToken( 1 ).toInt();

			if ( new_timelimit < 0 )
			{
				client.printMessage( "Can't set negative timelimit\n");
				return false;
			}

			if ( new_timelimit == g_timelimit.getInteger() )
			{
				client.printMessage( S_COLOR_RED + "Timelimit is already set to " + new_timelimit + "\n" );
				return false;
			}

			return true;
		}

		if ( vote == "spec" )
		{
			if ( ! map.inOvertime )
			{
				client.printMessage( S_COLOR_RED + "Callvote spec is only valid during overtime\n");
				return false;
			}

			return true;
		}

		client.printMessage( "Unknown callvote " + vote + "\n" );
		return false;
	}

	else if ( cmdString == "callvotepassed" )
	{
            cString vote = argsString.getToken( 0 );

            if ( vote == "extend_time" )
            {
            	g_timelimit.set(g_timelimit.getInteger() + g_extendtime.getInteger());

                map.cancelOvertime();
				for ( int i = 0; i < maxClients; i++ )
				{
					players[i].cancelOvertime();
				}
            }

			if ( vote == "timelimit" )
            {
				int new_timelimit = argsString.getToken( 1 ).toInt();
				g_timelimit.set(new_timelimit);

				// g_timelimit_reset == 1: this timelimit value is not kept after current map
				// g_timelimit_reset == 0: current value is permanently stored in g_timelimit as long as the server runs
				if (g_timelimit_reset.getBool() == false)
				{
					oldTimelimit = g_timelimit.getInteger();
				}
            }

			if ( vote == "spec" )
			{
				for ( int i = 0; i < maxClients; i++ )
				{
					if ( @players[i].getClient() != null )
					{
						players[i].moveToSpec( S_COLOR_RED + "You have been moved to spec cause you were playing in overtime.\n");
					}
				}

			}


            return true;
    }
    /*
	else if ( ( cmdString == "weapondef" ) )
    {
		return weaponDefCommand( argsString, @client );
    }
    */
	else if ( ( cmdString == "cvarinfo" ) )
    {
		//token0: cVar name; token1: cVar value
		cString cvarName = argsString.getToken(0);
		cString cvarValue = argsString.getToken(1);

		if( cvarName.substr(0,15) == "storedposition_")
		{
			cString positionValues = cvarValue;
			cVec3 origin, angles;
			origin.x = positionValues.getToken(1).toFloat();
			origin.y = positionValues.getToken(2).toFloat();
			origin.z = positionValues.getToken(3).toFloat();
			angles.x = positionValues.getToken(4).toFloat();
			angles.y = positionValues.getToken(5).toFloat();
			player.teleport( origin, angles, false, false );
		}

    }

	else if ( ( cmdString == "position" ) )
	{
		return player.position( argsString );
	}

    return false;
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
    return false; // let the default code handle it itself
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
	Racesow_Player @player = Racesow_GetPlayerByClient(self.client);
	player.onSpawn();
	if( player.wasTelekilled )
	{
		return @player.gravestone;
	}
	else if( @alphaFlagBase != null )
	{
	    return @bestFastcapSpawnpoint();
	}
	else
	{
		return null; // select random
	}
}

/**
 * GT_ScoreboardMessage
 * @param int maxlen
 * @return cString
 */
cString @GT_ScoreboardMessage( int maxlen )
{
    cString scoreboardMessage = "";
    cString entry;
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

    // "Name Time Ping Racing"
    for ( i = 0; @team.ent( i ) != null; i++ )
    {
        @ent = @team.ent( i );
        Racesow_Player @player = Racesow_GetPlayerByClient( ent.client );

        int playerID = ( ent.isGhosting() && ( match.getState() == MATCH_STATE_PLAYTIME ) ) ? -( ent.playerNum() + 1 ) : ent.playerNum();
        racing = int( player.isRacing() ? 1 : 0 );
    	if ( !(g_freestyle.getBool()) ) // use a different scoreboard for freestyle
    	{
			entry = "&p " + playerID + " " + ent.client.getClanName() + " "
					+ player.getBestTime() + " "
					+ player.highestSpeed + " "
					+ ent.client.ping + " " + racing + " ";
    	}
    	else
    	{
			entry = "&p " + playerID + " " + ent.client.getClanName() + " "
					+ ent.client.ping + " ";
    	}

        if ( scoreboardMessage.len() + entry.len() < maxlen )
            scoreboardMessage += entry;
    }
    
    //custom scoreboard for ppl who are getting spectated
    if( levelTime > scoreboardLastUpdate + 1800 )
    {
        for ( int i = 0; i < maxClients; i++ )
        {
            players[i].challengerList = "";
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
                    spectatorList += other.client.playerNum() + " " + other.client.ping + " ";
                }
                else if( other.client.connecting ) //add connecting spectators
                {
                    spectatorList += other.client.playerNum() + " " + -1 + " ";
                }
                if( other.client.chaseActive && other.client.chaseTarget != 0)
                    //add him to the challenger list of the player he's spectating
                {
                    Racesow_Player @player = players[other.client.chaseTarget-1];
                    player.challengerList += other.client.playerNum() + " " + other.client.ping + " ";
                }
            }
        }
        playerList = scoreboardMessage;
        scoreboardLastUpdate = levelTime;
    }
    scoreboardUpdated = true;
    return scoreboardMessage;
}

/**
 * GT_scoreEvent
 *
 * handles different game events
 *
 * @param cClient @client
 * @param cString &score_event
 * @param cString &args
 * @return void
 */
void GT_scoreEvent( cClient @client, cString &score_event, cString &args )
{
    Racesow_Player @player = Racesow_GetPlayerByClient( client );
	if (@player != null )
	{
		if ( score_event == "dmg" )
		{
		}
		else if ( score_event == "kill" )
		{
			cEntity @edict = @G_GetEntity( args.getToken( 0 ).toInt() );
			if( @edict.client == null )
				return;

			Racesow_Player @player_edict = Racesow_GetPlayerByClient( edict.client );
			if( @player_edict == null )
				return;

			if( g_freestyle.getBool() && @edict.client != @client ) //telekills
			{
				if( @client.getEnt() == null)
					return;
				cTrace tr;
				if(	tr.doTrace( client.getEnt().getOrigin(), vec3Origin, vec3Origin, client.getEnt().getOrigin() + cVec3( 0.0f, 0.0f, 50.0f ), 0, MASK_DEADSOLID ))//avoid bugs
					return;
				//spawn a gravestone to store the postition
				cEntity @gravestone = @G_SpawnEntity( "gravestone" );
				// copy client position
				gravestone.setOrigin( client.getEnt().getOrigin() + cVec3( 0.0f, 0.0f, 50.0f ) );
				player_edict.setupTelekilled( @gravestone );
			}
		}
		else if ( score_event == "award" )
		{
		}
		else if ( score_event == "connect" )
		{
			player.reset();
		}
		else if ( score_event == "enterGame" )
		{
            player.getAuth().setName(client.getUserInfoKey(rs_authField_Name.getString()));
            player.getAuth().setPass(client.getUserInfoKey(rs_authField_Pass.getString()));
            player.getAuth().setToken(client.getUserInfoKey(rs_authField_Token.getString()));

            player.appear();
		}
		else if ( score_event == "disconnect" )
		{
			player.disappear(player.getName(), true);
			player.reset();
		}
		else if ( score_event == "userinfochanged" )
		{
			if( !client.connecting ) {

                player.getAuth().refresh( args );


				// auto-hop check
				if ( rs_allowAutoHop.getBool() == false )
				{
					// checking if the player is restoring his autojump (we can't cheatprotect a client variable from the server, can we?)
					if ( client.getUserInfoKey("cg_noAutohop").toInt() == 0 )
					{
						client.setPMoveFeatures( client.pmoveFeatures & ~PMFEAT_CONTINOUSJUMP );
					}
				}
			}
		}
	}
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
	cItem @item;
	cItem @ammoItem;
	Racesow_Player @player = Racesow_GetPlayerByClient( ent.client );

    if (new_team == TEAM_PLAYERS) {

        if (map.inOvertime) {
            player.client.team = TEAM_SPECTATOR;
            player.client.respawn( true );
            player.sendMessage(S_COLOR_RED + "No spawning in overtime. Please wait for the other players to finish.\n");
            return;
        }
    }

	if ( ent.isGhosting() )
	    return;

	if ( g_freestyle.getBool() )
	{
		if( player.wasTelekilled )
			player.resetTelekilled();
		ent.client.inventorySetCount( WEAP_ROCKETLAUNCHER, 1 );
		ent.client.inventorySetCount( AMMO_WEAK_ROCKETS, 10 );


		// give all weapons
		for ( int i = WEAP_GUNBLADE + 1; i < WEAP_TOTAL; i++ )
		{
			if ( i == WEAP_INSTAGUN ) // dont add instagun...
			continue;

			ent.client.inventoryGiveItem( i );

			@item = @G_GetItem( i );

			@ammoItem = @G_GetItem( item.weakAmmoTag );
			if ( @ammoItem != null )
			ent.client.inventorySetCount( ammoItem.tag, ammoItem.inventoryMax );

			@ammoItem = @G_GetItem( item.ammoTag );
			if ( @ammoItem != null )
			ent.client.inventorySetCount( ammoItem.tag, ammoItem.inventoryMax );
		}

		// TODO: let player choose if allow interacting with others, maybe also who to interact with
	}
	else
	{
		// set player movement to pass through other players and remove gunblade auto attacking
		ent.client.setPMoveFeatures( ent.client.pmoveFeatures & ~PMFEAT_GUNBLADEAUTOATTACK | PMFEAT_GHOSTMOVE );

		// disable autojump
		if ( rs_allowAutoHop.getBool() == false )
		{
			ent.client.setPMoveFeatures( ent.client.pmoveFeatures & ~PMFEAT_CONTINOUSJUMP );
		}
	}

	ent.client.inventorySetCount( WEAP_GUNBLADE, 1 );

    // select rocket launcher if available
    if ( ent.client.canSelectWeapon( WEAP_ROCKETLAUNCHER ) )
        ent.client.selectWeapon( WEAP_ROCKETLAUNCHER );
    else
        ent.client.selectWeapon( -1 ); // auto-select best weapon in the inventory

	// make dash 450
	ent.client.setPMoveDashSpeed( 450 );

    player.restartRace();
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

	bool timelimited = not (g_freestyle.getBool() || !g_maprotation.getBool());
	
	if ( timelimited )
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
		if ( ( not timelimited ) and match.timeLimitHit() )
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

		Racesow_Player @player = Racesow_GetPlayerByClient( client );

		if( scoreboardUpdated && player.challengerList != "")//send the scoreboard to the player
		{
            cString command = "scb \""
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
		        //player.getClient().addAward(player.getAuth().getViolateCountDown());
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
            player.sendMessage( rs_welcomeMessage.getString() + "\n" );
            player.sendMessage( rs_registrationInfo.getString() + "\n" );
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

        // all stats are set to 0 each frame, so it's only needed to set a stat if it's going to get a value
        if( ( player.client.team == TEAM_SPECTATOR ) && !( player.client.chaseActive ) )
            @player.race = null;

        if ( player.isRacing() )
        {
            client.setHUDStat( STAT_TIME_SELF, (levelTime - player.race.getStartTime()) / 100 );
            if ( player.highestSpeed < player.getSpeed() )
                player.highestSpeed = player.getSpeed(); // updating the heighestSpeed attribute.
        }

		if ( !(g_freestyle.getBool()) ) // remove the time stats in freestyle
		{
     	  	client.setHUDStat( STAT_TIME_BEST, player.getBestTime() / 100 );
        	client.setHUDStat( STAT_TIME_RECORD, map.getHighScore().getTime() / 100 );
			if ( isUsingRacesowClient(client) )
			{
				client.setHUDStat( STAT_TIME_ALPHA, map.worldBest / 100 );
			}
			addFastcapHUDStats( @client );
			int PLAYER_MASS = 200;
			if( client.inventoryCount( POWERUP_QUAD ) > 0 )
			    client.getEnt().mass = PLAYER_MASS * 1/3;// * QUAD_KNOCKBACK_SCALE
			else
			    client.getEnt().mass = PLAYER_MASS;
		}

		
		// what is this for? should we del it? please add a meaningful comment
        if ( map.getHighScore().playerName.len() > 0 )
            client.setHUDStat( STAT_MESSAGE_OTHER, CS_GENERAL );
        if ( map.getHighScore().playerName.len() > 0 )
            client.setHUDStat( STAT_MESSAGE_ALPHA, CS_GENERAL + 1 );
        if ( map.getHighScore().playerName.len() > 0 )
            client.setHUDStat( STAT_MESSAGE_BETA, CS_GENERAL + 2 );

    	if( player.isUsingChrono )
    		client.setHUDStat( STAT_TIME_ALPHA, (levelTime - player.chronoTime()) / 100 );

    	if ( g_freestyle.getBool() && client.inventoryCount( WEAP_GUNBLADE ) != 0)
    	{
			client.inventorySetCount( AMMO_GUNBLADE, 10 );
			if( player.onQuad )
			    client.inventorySetCount( POWERUP_QUAD, 30 );
    	}
    }
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
    if (incomingMatchState == MATCH_STATE_POSTMATCH)
    {
        map.startOvertime();
        return map.allowEndGame();
    }

    if ( match.getState() == MATCH_STATE_POSTMATCH ) // LOL this should not be in here ;)
    {
    	g_timelimit.set(oldTimelimit); //restore the old timelimit
        match.stopAutorecord();
        demoRecording = false;
    }

    return true;
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

/**
 * GT_Shutdown
 * the gametype is shutting down cause of a match restart or map change
 * @return void
 */
void GT_Shutdown()
{
    if( g_gravity.getDefaultString() != g_gravity.getString() )
    {
        //some maps might have set a custom g_gravity which normaly won't get restored
        g_gravity.reset();
        G_Print( "Note: custom g_gravity reset\n" );
    }
    
    for ( int i = 0; i < maxClients; i++ )
		if ( @players[i].getClient() != null )
		{
		    // run it unthreaded to prevent a mysql crash
			players[i].disappear(players[i].getName(),false);
		}
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


	// setup players
    for ( int i = 0; i < maxClients; i++ )
        players[i].reset();

    for ( int i = 0; i <= numEntities; i++ )
    {
        cEntity @ent = @G_GetEntity( i );
        if( @ent == null )
            continue;
        
        if( ent.getClassname() == "trigger_multiple" )
        {
            if( ( ent.findTargetEntity( @ent ).getClassname() == "target_startTimer" )
                            || ( ent.findTargetEntity( @ent ).getClassname() == "target_starttimer" ) )
            {
                ent.wait = 0;
            }
        }
        else if( ent.getClassname() == "target_give" )
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
            if( @Item != null && ent.getClassname() == Item.getClassname() )
            {
                if( ent.solid != SOLID_NOT ) //ok, not connected
                {
                    ent.setClassname( "AS_" + Item.getClassname() );
                    replacementItem( @ent );
                }
            }
        }
    }
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
    gametype.setTitle( "Racesow" );
    gametype.setVersion( "0.6.0" );
    gametype.setAuthor( "warsow-race.net" );

	// initalize weapondef config
	weaponDefInit();

    G_WriteFile( "configs/server/gametypes/race.cfg", "" );

    // if the gametype doesn't have a config file, create it
    if ( !G_FileExists( "configs/server/gametypes/racesow.cfg" ) )
    {
        cString config;

        // the config file doesn't exist or it's empty, create it
        config = "//*\n"
                 + "//* Racesow Base settings\n"
                 + "//*\n"
                 + "// WARNING: if you touch any of theese settings\n"
                 + "// it can really have a very negative impact on\n"
                 + "// racesow's gameplay!\n"
                 + "\n"
                 + "set g_gametype \"race\"\n"
                 + "set g_allow_falldamage \"0\" // suxx\n"
                 + "set g_allow_selfdamage \"0\" // meeeh\n"
                 + "set g_allow_stun \"0\" // LOL!\n"
                 + "set g_allow_bunny \"0\" // learn it!\n"
                 + "set g_antilag \"0\" // do NEVER touch!\n"
                 + "set g_scorelimit \"0\" // a new feature..?\n"
								 + "set g_warmup_timelimit \"0\" // ... \n"
                 + "set rs_projectilePrestep \"24\" // is it used?\n"
                 + "set rs_movementStyle \"1\"\n"
                 + "\n"
                 + "exec configs/server/gametypes/racesow_weapondefs.cfg"
                 + "\n"
				 + "echo racesow.cfg executed\n";

        G_WriteFile( "configs/server/gametypes/racesow.cfg", config );
        G_Print( "Created default base config file for racesow\n" );
	}

	// always execute racesow.cfg
    G_CmdExecute( "exec configs/server/gametypes/racesow.cfg silent" );

    gametype.spawnableItemsMask = ( IT_WEAPON | IT_AMMO | IT_ARMOR | IT_POWERUP | IT_HEALTH );
    if ( gametype.isInstagib() )
        gametype.spawnableItemsMask &= ~uint(G_INSTAGIB_NEGATE_ITEMMASK);

    gametype.respawnableItemsMask = gametype.spawnableItemsMask;
    gametype.dropableItemsMask = 0;
    gametype.pickableItemsMask = 0;

    gametype.isTeamBased = false;
    gametype.isRace = true;
    gametype.hasChallengersQueue = false;
    gametype.maxPlayersPerTeam = 0;

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

	if ( !(g_freestyle.getBool()) )
	{
		gametype.spawnpointRadius = 0;
	}
	else
	{
		 // use a different spawnpoint radius for freestyle (avoid massacres when spawning)
		gametype.spawnpointRadius = 256;
	}

    // set spawnsystem type
    for ( int team = TEAM_PLAYERS; team < GS_MAX_TEAMS; team++ )
        gametype.setTeamSpawnsystem( team, SPAWNSYSTEM_INSTANT, 0, 0, false );

    // define the scoreboard layout
	if ( !(g_freestyle.getBool()) ) // use a different scoreboard for freestyle
	{
	    G_ConfigString( CS_SCB_PLAYERTAB_LAYOUT, "%n 112 %s 52 %t 96 %i 48 %l 48 %b 48" );
	    G_ConfigString( CS_SCB_PLAYERTAB_TITLES, "Name Clan Time Speed Ping Racing" );
	}
	else
	{
    	G_ConfigString( CS_SCB_PLAYERTAB_LAYOUT, "%n 152 %s 90 %l 48" );
    	G_ConfigString( CS_SCB_PLAYERTAB_TITLES, "Name Clan Ping" );
	}

	prcFlagIconStolen = G_ImageIndex( "gfx/hud/icons/flags/iconflag_stolen" );	
	
    // add commands
	RS_InitCommands();
	
	// weapondef not needed anymore, we're not testing weapons
	//G_RegisterCommand( "weapondef" );
	
	G_RegisterCommand( "ammoswitch" );
	G_RegisterCommand( "whoisgod" );

	//add callvotes
	G_RegisterCallvote( "extend_time", "", "Extends the matchtime." );
	G_RegisterCallvote( "timelimit", "<minutes>", "Set match timelimit." );
	G_RegisterCallvote( "spec", "", "During overtime, move all players to spectators." );


    demoRecording = false;

	if ( G_Md5( "www.warsow-race.net" ) != "bdd5b303ccc88e5c63ce71bfc250a561" )
	{
		G_Print( "* " + S_COLOR_RED + "MD5 hashing test failed!!!\n" );
	}
	else
	{
		G_Print( "* " + S_COLOR_GREEN + "MD5 hashing works fine...\n" );
	}

	// disallow warmup, no matter what config files say, because it's bad for racesow timelimit.
	g_warmup_timelimit.set("0"); //g_warmup_enabled was removed in warsow 0.6

	g_self_knockback.forceSet("1.25"); // 1.18 in basewsw.6
	
	//store g_timelimit for restoring it at the end of the map (it will be altered by extend_time votes)
	oldTimelimit = g_timelimit.getInteger();

	// load maps list (basic or mysql)
	RS_LoadMapList( g_freestyle.getInteger() );

    G_Print( "Gametype '" + gametype.getTitle() + "' initialized\n" );
}
