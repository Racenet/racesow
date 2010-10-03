/**
 * Racesow Gametype Interface
 *
 * based on warsow 0.5 race gametype
 * @version 0.5.6
 */

const uint RACESOW_AUTH_REGISTERED	= 1;
const uint RACESOW_AUTH_MAP			= 2;
const uint RACESOW_AUTH_KICK		= 4;
const uint RACESOW_AUTH_TIMELIMIT	= 8;
const uint RACESOW_AUTH_RESTART		= 16;
const uint RACESOW_AUTH_ADMIN		= 30;

int numCheckpoints = 0;
bool demoRecording = false;
bool firstAnnouncement = false;
bool secondAnnouncement = false;
bool thirdAnnouncement = false;
const int MAX_RECORDS = 10;
const int MAPS_PER_PAGE = 20;
int oldTimelimit; //for restoring the old value

cString gameDataDir = "gamedata";
cString scbmsg; //scoreboard message for custom scoreboards
cString scb_specs;
cString[] specwho( maxClients );
uint nextTimeUpdate;
bool scbupdated = true;

cString maplist;
uint mapcount;

Racesow_Player[] players( maxClients );
Racesow_Map @map;
Racesow_Adapter_Full @racesowAdapter;

cVar rs_authField_Name( "rs_authField_Name", "", CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_NOSET );
cVar rs_authField_Pass( "rs_authField_Pass", "", CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_NOSET );
cVar rs_authField_Token( "rs_authField_Token", "", CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_NOSET );
cVar rs_extendtimeperiod( "rs_extendtimeperiod", "3", CVAR_ARCHIVE );
cVar rs_loadHighscores( "rs_loadHighscores", "0", CVAR_ARCHIVE );

cVar g_freestyle( "g_freestyle", "1", CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_NOSET );
cVar g_allowammoswitch( "g_allowammoswitch", "0", CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_NOSET );
cVar g_timelimit( "g_timelimit", "20", CVAR_ARCHIVE );
cVar g_extendtime( "g_extendtime", "10", CVAR_ARCHIVE );
cVar g_maprotation( "g_maprotation", "1", CVAR_ARCHIVE );

cVar rs_registrationDisabled( "rs_registrationDisabled", "0", CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_NOSET );
cVar rs_registrationInfo( "rs_registrationInfo", "Please ask the serveradmin how to create a new account.", CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_NOSET );

cVar sv_cheats( "sv_cheats", "0", CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_NOSET );

cString diffString( uint oldTime, uint newTime )
{
    if ( oldTime == 0 )
    {
        return TimeToString( newTime );
    }
    else if ( oldTime < newTime )
    {
        return S_COLOR_RED + "+" + TimeToString( newTime - oldTime );
    }
    else if ( oldTime == newTime )
    {
        return S_COLOR_YELLOW + "+-" + TimeToString( 0 );
    }
    else
    {
        return S_COLOR_GREEN + "-" + TimeToString( oldTime - newTime );
    }
}

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
 * Racesow_GetPlayerByNumber
 * @param int playerNum
 * @return Racesow_Player
 */
Racesow_Player @Racesow_GetPlayerByNumber(int playerNum)
{
    if ( playerNum < 0 )
        return null;

	return @players[ playerNum ];
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
	Racesow_Command@ command = RS_GetCommandByName( cmdString );

	if ( command !is null )
	{
	    player.executeCommand(command, argsString, argc);
	}


	else if ( ( cmdString == "ammoswitch" ) || ( cmdString == "classaction1" ) )
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

    scbmsg = scoreboardMessage;
    scbupdated = true;
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
            player.client.respawn( false ); //FIXME : is it really necessary ?
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

	if ( match.timeLimitHit() )
    {
        match.launchState( match.getState() + 1 );
    }

    map.allowEndGame();

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

	if ( !firstAnnouncement && ( ( match.duration() - levelTime ) < 180000 ) && ( match.duration() >= 180000 ) )
	{
	    firstAnnouncement = true;
	    G_PrintMsg( null, S_COLOR_GREEN + "3 minutes left...\n");
	}
    if ( !secondAnnouncement && ( ( match.duration() - levelTime ) < 120000 ) && ( match.duration() >= 120000 ) )
    {
        secondAnnouncement = true;
        G_PrintMsg( null, S_COLOR_YELLOW + "2 minutes left...\n");
    }
    if ( !thirdAnnouncement && ( ( match.duration() - levelTime ) < 60000 ) && ( match.duration() >= 60000 ) )
    {
        thirdAnnouncement = true;
        G_PrintMsg( null, S_COLOR_RED + "1 minute left...\n");
    }

	if( levelTime > nextTimeUpdate )
	{
		//custom scoreboard
		cTeam @specs;
		@specs = @G_GetTeam( TEAM_SPECTATOR );
		cEntity @spec_ent;
		for( int j = 0; j < maxClients; j++ )
		{
			specwho[j] = "";
		}

		scb_specs = "&s ";
		for ( int i = 0; @specs.ent( i ) != null; i++ )
		{
			@spec_ent = @specs.ent( i );
            if (@spec_ent != null && @spec_ent.client != null)
            {

                cClient @spec_client = spec_ent.client;
                if(spec_client.connecting)
                {
                    scb_specs += spec_client.playerNum() + " " + -1 + " ";
                }
                else
                {
                    scb_specs += spec_client.playerNum() + " " + spec_client.ping + " ";
                }
                if( spec_client.chaseActive )
                {
                    specwho[spec_client.chaseTarget - 1] += spec_ent.client.playerNum() + " " + spec_ent.client.ping + " ";
                }
            }
		}
	nextTimeUpdate = levelTime + 2500;
	}

    // set all clients race stats
    cClient @client;

    for ( int i = 0; i < maxClients; i++ )
    {
        @client = @G_GetClient( i );

    	if( scbupdated && specwho[i] != "" )
    		client.execGameCommand("scb \"" + scbmsg + " &w " + specwho[i] + " " + scb_specs + " \"");

        if ( client.state() < CS_SPAWNED )
            continue;

		Racesow_Player @player = Racesow_GetPlayerByClient( client );

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
        if( ( player.client.team == TEAM_SPECTATOR ) && !( player.client.chaseActive ) )
            @player.race = null;

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

    	if( player.isUsingChrono )
    		client.setHUDStat( STAT_TIME_ALPHA, (levelTime - player.chronoTime()) / 100 );

    	if ( g_freestyle.getBool() && client.inventoryCount( WEAP_GUNBLADE ) != 0)
    	{
			client.inventorySetCount( AMMO_GUNBLADE, 10 );
			if( player.onQuad )
			    client.inventorySetCount( POWERUP_QUAD, 30 );
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
    cEntity @trigger_multiple;
    cEntity @from = null;
    cEntity @from2 = null;
    bool found = false;

    @map = Racesow_Map();
    @racesowAdapter = Racesow_Adapter_Full();
    
    racesowAdapter.initGametype();

	// setup players
    for ( int i = 0; i < maxClients; i++ )
        players[i].reset();

    @trigger_multiple = @G_FindEntityWithClassname( null, "trigger_multiple" );

    while( @trigger_multiple != null )
    {
        while( ( @trigger_multiple.findTargetEntity( @from2 ) != null ) )
        {
            if( ( trigger_multiple.findTargetEntity( @from2 ).getClassname() == "target_startTimer" )
                || ( trigger_multiple.findTargetEntity( @from2 ).getClassname() == "target_starttimer" ) )
            {
                trigger_multiple.wait = 0;
                found = true;
                break;
            }
            @from2 = @trigger_multiple.findTargetEntity( @from2 );
        }
        if( found == true )
            break;
        @from = @trigger_multiple;
        @trigger_multiple = @G_FindEntityWithClassname( from, "trigger_multiple" );
    }

    //TODOSOW fastcap if there are flag entitys
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
    gametype.setVersion( "0.5.6" );
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
                 + "set g_warmup_enabled \"0\" // ...\n"
                 + "set rs_projectilePrestep \"24\" // is it used?\n"
                 + "set rs_movementStyle \"1\"\n"
                 + "\n"
                 + "exec configs/server/gametypes/racesow_weapondefs.cfg"
                 + "\n"
				 + "echo racesow.cfg executed\n";

        G_WriteFile( "configs/server/gametypes/racesow.cfg", config );
        G_Print( "Created default base config file for racesow\n" );
        G_CmdExecute( "exec configs/server/gametypes/racesow.cfg silent" );
    }

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
    	G_ConfigString( CS_SCB_PLAYERTAB_LAYOUT, "%n 112 %s 52 %t 96 %l 48 %b 48" );
    	G_ConfigString( CS_SCB_PLAYERTAB_TITLES, "Name Clan Time Ping Racing" );
	}
	else
	{
    	G_ConfigString( CS_SCB_PLAYERTAB_LAYOUT, "%n 152 %s 90 %l 48" );
    	G_ConfigString( CS_SCB_PLAYERTAB_TITLES, "Name Clan Ping" );
	}

    // add commands
	RS_InitCommands();
	G_RegisterCommand( "ammoswitch" );
	//G_RegisterCommand( "weapondef" );
	G_RegisterCommand( "classaction1" );

	//add callvotes
	G_RegisterCallvote( "extend_time", "", "Extends the matchtime." );


    demoRecording = false;

	if ( G_Md5( "www.warsow-race.net" ) != "bdd5b303ccc88e5c63ce71bfc250a561" )
	{
		G_Print( "* " + S_COLOR_RED + "MD5 hashing test failed!!!\n" );
	}
	else
	{
		G_Print( "* " + S_COLOR_GREEN + "MD5 hashing works fine...\n" );
	}

	if ( g_freestyle.getBool() || !g_maprotation.getBool() )
    {
		g_timelimit.set( "0" );
    }

	oldTimelimit = g_timelimit.getInteger(); //store for restoring it later

	RS_LoadMapList( g_freestyle.getInteger() );
    G_Print( "Gametype '" + gametype.getTitle() + "' initialized\n" );
}
