/**
 * Racesow Gametype Interface
 *
 * based on warsow 0.5 race gametype
 * @version 0.5.1c
 * @date 24.09.2009
 * @author soh#zolex <zolex@warsow-race.net>
 * @author you? <you@warsow-race.net>
 */

const uint RACESOW_AUTH_REGISTERED	= 1;
const uint RACESOW_AUTH_MAP			= 2;
const uint RACESOW_AUTH_KICK		= 4;
const uint RACESOW_AUTH_TIMELIMIT	= 8;
const uint RACESOW_AUTH_RESTART		= 16;
const uint RACESOW_AUTH_ADMIN		= 30;

int numCheckpoints = 0;
bool demoRecording = false;
const int MAX_RECORDS = 10;

cString gameDataDir = "gamedata";

Racesow_Player[] players( maxClients );
Racesow_Map @map;

cVar g_freestyle( "g_freestyle", "0", CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_NOSET ); // move to where it's needed...
cVar g_allowammoswitch( "g_allowammoswitch", "0", CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_NOSET );

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
 * DateToString
 * @param uint64 dateuint64
 * @return cString
 */
cString DateToString( uint64 dateuint64 )
{
    // convert dates to printable form
	cTime date = cTime(dateuint64);
    cString daysString, monsString, yearsString, hoursString, minsString, secsString;

    if ( date.min == 0 )
        minsString = "00";
    else if ( date.min < 10 )
        minsString = "0" + date.min;
    else
        minsString = date.min;

    if ( date.sec == 0 )
        secsString = "00";
    else if ( date.sec < 10 )
        secsString = "0" + date.sec;
    else
        secsString = date.sec;

    if ( date.hour == 0 )
        hoursString = "00";
    else if ( date.hour < 10 )
        hoursString = "0" + date.hour;
    else
        hoursString = date.hour;

	if ( date.mon == 0 )
        monsString = "00";
    else if ( date.mon < 10 )
        monsString = "0" + date.mon;
    else
        monsString = date.mon;

	if ( date.mday == 0 )
        daysString = "00";
    else if ( date.mday < 10 )
        daysString = "0" + date.mday;
    else
        daysString = date.mday;

    return daysString + "/" + monsString + "/" + (1900+date.year) + " " + hoursString +":" + minsString + ":" + secsString;
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

	return @players[ client.playerNum() ].setClient( @client );
}


/**
 * Racesow_GetPlayerNumber
 * @param cString playerName
 * @return int
 */
int Racesow_GetClientNumber( cString playerName )
{
    cClient @client;

    for ( int i = 0; i < maxClients; i++ )
    {
        @client = @G_GetClient( i );
        if ( client.state() < CS_SPAWNED )
            continue;

		if (client.getName().removeColorTokens() == playerName)
			return client.playerNum();
	}
    return -1;
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

    if ( cmdString == "gametype" )
    {
        cString response = "";
        cVar fs_game( "fs_game", "", 0 );
        cString manifest = gametype.getManifest();

        response += "\n";
        response += "Gametype " + gametype.getName() + " : " + gametype.getTitle() + "\n";
        response += "----------------\n";
        response += "Version: " + gametype.getVersion() + "\n";
        response += "Author: " + gametype.getAuthor() + "\n";
        response += "Mod: " + fs_game.getString() + (manifest.length() > 0 ? " (manifest: " + manifest + ")" : "") + "\n";
        response += "----------------\n";

        G_PrintMsg( client.getEnt(), response );
        return true;
    }
    else if ( ( cmdString == "racerestart" ) || ( cmdString == "restartrace" ) )
    {
        if ( @client != null && !player.isJoinlocked )
        {
        	if(client.team == TEAM_SPECTATOR)
        	{
        		sendMessage( S_COLOR_WHITE + "Join the game first.\n", @client );
        	}
        	else
        	{
        		player.restartRace();
        		client.team = TEAM_PLAYERS;
        		client.respawn( false );
        	}
        }

        return true;
    }

    else if ( cmdString == "join" )
    {
        if ( @client != null && !player.isJoinlocked)
        {
        	if( client.team != TEAM_PLAYERS ) //spam protection
    			G_PrintMsg( null, S_COLOR_WHITE + client.getName() + S_COLOR_WHITE
    					+ " joined the PLAYERS team.\n" ); // leave a message for everyone
            player.restartRace();
            client.team = TEAM_PLAYERS;
			client.respawn( false );
        }

        if( player.isJoinlocked )
        	sendMessage( S_COLOR_RED + "You can't join: You are join locked.\n", @client );

        return true;
    }

	else if ( ( cmdString == "top" ) || ( cmdString == "highscores" ) )
    {
		if( g_freestyle.getBool() )
		{
			sendMessage( S_COLOR_RED + "Command only available for race\n", @client );
			return false;
		}
		else if( player.top_lastcmd + 30 > localTime )
		{
			sendMessage( S_COLOR_RED + "Flood protection. You can use the top command"
					+"only every 30 seconds\n", @client );
			return false;
		}
		else
		{
			player.top_lastcmd = localTime;
			G_PrintMsg( client.getEnt(), map.getStatsHandler().getStats() );
		}
    }
	else if ( ( cmdString == "register" ) )
    {
		cString authName = argsString.getToken( 0 ).removeColorTokens();
		cString authEmail = argsString.getToken( 1 );
		cString password = argsString.getToken( 2 );
		cString confirmation = argsString.getToken( 3 );

		return player.getAuth().signUp( authName, authEmail, password, confirmation );
    }
	else if ( ( cmdString == "auth" ) )
    {
		return player.getAuth().authenticate( argsString.getToken( 0 ).removeColorTokens(), argsString.getToken( 1 ), false );
    }
	else if ( ( cmdString == "admin" ) )
    {
		return player.adminCommand( argsString );
    }
	else if ( ( cmdString == "help" ) )
    {
		return player.displayHelp();
    }
	else if ( ( cmdString == "ammoswitch" ) || ( cmdString == "classaction1" ) )
	{
		return player.ammoSwitch( @client );
	}
	else if ( ( cmdString == "chrono" ) )
	{
		return player.chronoUse( argsString );
	}
	else if ( ( cmdString == "privsay" ) )
    {
		return player.privSay( argsString, @client );

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
	else if ( ( cmdString == "weapondef" ) )
    {
		return weaponDefCommand( argsString, @client );
    }
	else if ( ( cmdString == "cvarinfo" ) )
    {
		//token0: cVar name; token1: cVar value
		cString cvarName = argsString.getToken(0);
		cString cvarValue = argsString.getToken(1);

		if( cvarName.substr(0,15) == "storedposition_")
		{
			cString positionValues = cvarValue;
			client.execGameCommand("cmd position set " + positionValues.getToken(1) + " " + positionValues.getToken(2)
					+ " " + positionValues.getToken(3) + " " + positionValues.getToken(4) + " "  + positionValues.getToken(5)
					+ ";echo position '" + positionValues.getToken(0) + "' restored" );
		}

    }
	else if ( ( cmdString == "positionstore" ) )
    {
		if( argsString.getToken(0) == "" || argsString.getToken(1) == "" )
		{
			sendMessage( S_COLOR_WHITE + "Usage: /positionstore (id) (name)\n", @client );
			return false;
		}
		cVec3 position = client.getEnt().getOrigin();
		cVec3 angles = client.getEnt().getAngles();
		//position set <x> <y> <z> <pitch> <yaw>
		client.execGameCommand("cmd seta storedposition_" + argsString.getToken(0)  + " \"" +  argsString.getToken(1) + " "
				+ position.x + " " + position.y + " " + position.z + " "
				+ angles.x + " " + angles.y + "\";writeconfig config.cfg");
    }
	else if ( ( cmdString == "positionrestore" ) )
    {
		if( argsString.getToken(0) == "" || !g_freestyle.getBool() )
		{
			sendMessage( S_COLOR_WHITE + "Usage: /positionrestore (id)\n", @client );
			return false;
		}
		G_CmdExecute( "cvarcheck " + client.playerNum() + " storedposition_" + argsString.getToken(0) );
    }
	else if ( ( cmdString == "storedpositionslist" ) )
    {
		if( argsString.getToken(0).toInt() == 0 )
		{
			sendMessage( S_COLOR_WHITE + "Usage: /storedpositionslist (limit)\n", @client );
			return false;
		}
		sendMessage( S_COLOR_WHITE + "###\n#List of stored positions\n###\n", @client );
		int i;
		for( i = 0; i < argsString.getToken(0).toInt(); i++ )
		{
			client.execGameCommand("cmd  echo ~~~;echo id#" + i + ";storedposition_" + i +";echo ~~~;" );
		}
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
		return player.returnGravestone();
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

        int playerID = ( ent.isGhosting() && ( match.getState() == MATCH_STATE_PLAYTIME ) ) ? -( ent.playerNum() + 1 ) : ent.playerNum();
        racing = int( Racesow_GetPlayerByClient( ent.client ).isRacing() ? 1 : 0 );
    	if ( !(g_freestyle.getBool()) ) // use a different scoreboard for freestyle
    	{
			entry = "&p " + playerID + " " + ent.client.getClanName() + " "
					+ Racesow_GetPlayerByClient( ent.client ).getBestTime() + " "
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
			if( g_freestyle.getBool()) //telekills
			{
				if( !client.getEnt().inuse)
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
			player_edict.restartRace();
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
			if ( !player.getAuth().isAuthenticated() )
			{
				player.getAuth().loadSession();
			}

			player.getAuth().checkProtectedNickname();
		}
		else if ( score_event == "disconnect" )
		{
			player.resetAuth();
		}
		else if ( score_event == "userinfochanged" )
		{
			if( !client.connecting )
			player.getAuth().refresh( args );
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
	player.restartRace();

	if ( ent.isGhosting() )
	return;

	if ( g_freestyle.getBool() )
	{
		if( player.wasTelekilled() )
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
		// set player movement to pass through other players
		ent.client.setPMoveFeatures( ent.client.pmoveFeatures | PMFEAT_GHOSTMOVE );
	}

	ent.client.inventorySetCount( WEAP_GUNBLADE, 1 );

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
	if ( match.timeLimitHit() && map.allowEndGame() )
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

	// set the logTime once
	if ( map.getStatsHandler().logTime == 0 && localTime != 0 )
		map.getStatsHandler().logTime = localTime;

    // set all clients race stats
    cClient @client;

    for ( int i = 0; i < maxClients; i++ )
    {
        @client = @G_GetClient( i );
        if ( client.state() < CS_SPAWNED )
            continue;

		Racesow_Player @player = Racesow_GetPlayerByClient( client );

		int countdownState;
		if ( 0 != ( countdownState = player.getAuth().wontGiveUpViolatingNickProtection() ) )
		{
		    if ( countdownState == 1 )
		        player.getClient().addAward(player.getAuth().getViolateCountDown());
		    else if ( countdownState == 2 )
		        player.kick( "You violated against the nickname protection." );
		}

        int status;
        if ( (status = player.processPlasmaClimbStatus()) > 0 )
        {
            if (status == 1)
                player.getClient().addAward(S_COLOR_YELLOW + "Good Plasma Climb !");

            if (status == 2)
                player.getClient().addAward(S_COLOR_GREEN + "Perfect Plasma Climb !");
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
        if ( player.isRacing() )
            client.setHUDStat( STAT_TIME_SELF, (levelTime - player.race.getStartTime()) / 100 );

		if ( !(g_freestyle.getBool()) ) // remove the time stats in freestyle
		{
     	  	client.setHUDStat( STAT_TIME_BEST, player.getBestTime() / 100 );
        	client.setHUDStat( STAT_TIME_RECORD, map.getStatsHandler().getHighScore(0).getTime() / 100 );
		}


        if ( map.getStatsHandler().getHighScore(0).playerName.len() > 0 )
            client.setHUDStat( STAT_MESSAGE_OTHER, CS_GENERAL );
        if ( map.getStatsHandler().getHighScore(1).playerName.len() > 0 )
            client.setHUDStat( STAT_MESSAGE_ALPHA, CS_GENERAL + 1 );
        if ( map.getStatsHandler().getHighScore(2).playerName.len() > 0 )
            client.setHUDStat( STAT_MESSAGE_BETA, CS_GENERAL + 2 );

    	if( player.chronoInUse() )
    		client.setHUDStat( STAT_TIME_ALPHA, (levelTime - player.chronoTime()) / 100 );
    }

	if ( g_freestyle.getBool() )
	{
	    for ( int i = 0; i < maxClients; i++ )  // charge gunblade for freestyle
	    {
	        if ( G_GetClient( i ).inventoryCount( WEAP_GUNBLADE ) == 0 )
	            continue;
	    	G_GetClient( i ).inventorySetCount( AMMO_GUNBLADE, 10 );
	    }
	}
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
    if ( match.getState() == MATCH_STATE_POSTMATCH )
    {
        match.stopAutorecord();
        demoRecording = false;

        map.getStatsHandler().writeStats();
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
        map.setUpMatch();
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
    @map = Racesow_Map();
    map.getStatsHandler().loadStats();

	// setup players
    for ( int i = 0; i < maxClients; i++ )
        players[i].reset();
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
    gametype.setVersion( "0.5.1d" );
    gametype.setAuthor( "warsow-race.net" );

	// initalize weapondef config
	weaponDefInit();

    // if the gametype doesn't have a config file, create it
    if ( !G_FileExists( "configs/server/gametypes/" + gametype.getName() + ".cfg" ) )
    {
        cString config;

        // the config file doesn't exist or it's empty, create it
        config = "// '" + gametype.getTitle() + "' gametype configuration file\n"
                 + "// This config will be executed each time the gametype is started\n"
                 + "\n\n// map rotation\n"
                 + "set g_maplist \"\" // list of maps in automatic rotation\n"
                 + "set g_maprotation \"0\"   // 0 = same map, 1 = in order, 2 = random\n"
                 + "\n// game settings\n"
                 + "set g_scorelimit \"0\"\n"
                 + "set g_timelimit \"60\"\n"
                 + "set g_warmup_enabled \"0\"\n"
                 + "set g_warmup_timelimit \"0\"\n"
                 + "set g_match_extendedtime \"0\"\n"
                 + "set g_allow_falldamage \"0\"\n"
                 + "set g_allow_selfdamage \"0\"\n"
                 + "set g_allow_teamdamage \"0\"\n"
                 + "set g_allow_stun \"0\"\n"
                 + "set g_teams_maxplayers \"0\"\n"
                 + "set g_teams_allow_uneven \"0\"\n"
                 + "set g_countdown_time \"5\"\n"
                 + "set g_maxtimeouts \"0\" // -1 = unlimited\n"
                 + "set g_challengers_queue \"0\"\n"
				 + "set g_logRaces \"0\"\n"
				 + "set g_secureAuth \"0\"\n"
				 + "set g_freestyle \"0\"\n"
				 + "set g_allowammoswitch \"0\"\n"
				 + "exec configs/server/gametypes/" + gametype.getName() + "_weapondef.cfg silent\n"
				 + "\necho " + gametype.getName() + ".cfg executed\n";

        G_WriteFile( "configs/server/gametypes/" + gametype.getName() + ".cfg", config );
        G_Print( "Created default config file for '" + gametype.getName() + "'\n" );
        G_CmdExecute( "exec configs/server/gametypes/" + gametype.getName() + ".cfg silent" );
    }

    gametype.spawnableItemsMask = ( IT_AMMO | IT_WEAPON | IT_POWERUP );
    if ( gametype.isInstagib() )
        gametype.spawnableItemsMask &= ~uint(G_INSTAGIB_NEGATE_ITEMMASK);

    gametype.respawnableItemsMask = gametype.spawnableItemsMask;
    gametype.dropableItemsMask = 0;
    gametype.pickableItemsMask = ( gametype.spawnableItemsMask | gametype.dropableItemsMask );

    gametype.isTeamBased = false;
    gametype.isRace = true;
    gametype.hasChallengersQueue = false;
    gametype.maxPlayersPerTeam = 0;

    gametype.ammoRespawn = 0;
    gametype.armorRespawn = 1;
    gametype.weaponRespawn = 0;
    gametype.healthRespawn = 1;
    gametype.powerupRespawn = 0;
    gametype.megahealthRespawn = 1;
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
    	G_ConfigString( CS_SCB_PLAYERTAB_LAYOUT, "%n 112 %s 52 %t 96 %l 48 %b 48" );
    	G_ConfigString( CS_SCB_PLAYERTAB_TITLES, "Name Clan Time Ping Racing" );
	}
	else
	{
    	G_ConfigString( CS_SCB_PLAYERTAB_LAYOUT, "%n 152 %s 90 %l 48" );
    	G_ConfigString( CS_SCB_PLAYERTAB_TITLES, "Name Clan Ping" );
	}

    // add commands
    G_RegisterCommand( "gametype" );
    G_RegisterCommand( "racerestart" );
    G_RegisterCommand( "join" );
    G_RegisterCommand( "top" );
	G_RegisterCommand( "highscores" );
    G_RegisterCommand( "register" );
	G_RegisterCommand( "auth" );
	G_RegisterCommand( "admin" );
	G_RegisterCommand( "help" );
	G_RegisterCommand( "ammoswitch" );
	G_RegisterCommand( "weapondef" );
	G_RegisterCommand( "classaction1" );
	G_RegisterCommand( "chrono" );
	G_RegisterCommand( "privsay" );
	G_RegisterCommand( "positionrestore" );
	G_RegisterCommand( "storedpositionslist" );
	G_RegisterCommand( "positionstore" );

    demoRecording = false;

	if ( G_Md5( "www.warsow-race.net" ) != "bdd5b303ccc88e5c63ce71bfc250a561" )
	{
		G_Print( "* " + S_COLOR_RED + "MD5 hashing test failed!!!\n" );
	}
	else
	{
		G_Print( "* " + S_COLOR_GREEN + "MD5 hashing works fine...\n" );
	}

    G_Print( "Gametype '" + gametype.getTitle() + "' initialized\n" );
}
