/**
 * Racesow Gametype Interface
 *
 * based on warsow 0.5 race gametype
 * @version 0.5.1b 
 * @author soh#zolex <zolex@warsow-race.net>
 * @author you? <you@warsow-race.net>
 */
 
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
        if ( @client != null )
        {
            player.restartRace();
            client.team = TEAM_PLAYERS;
			client.respawn( false );
        }

        return true;
    }
	else if ( ( cmdString == "top" ) || ( cmdString == "highscores" ) )
    {
		G_PrintMsg( client.getEnt(), map.getStatsHandler().getStats() );
    }
	else if ( ( cmdString == "racesow_register" ) )
    {
		cString authName = argsString.getToken(0).removeColorTokens();
		cString authEmail = argsString.getToken(1);
		cString password = argsString.getToken(2);
		cString confirmation = argsString.getToken(3);
		
		return player.registerAccount( authName, authEmail, password, confirmation );
    }
	else if ( ( cmdString == "racesow_auth" ) )
    {
		cString authName = argsString.getToken(0).removeColorTokens();
		cString authPass = argsString.getToken(1);
		
		return player.authenticate( authName, authPass, false );
    }
	else if ( ( cmdString == "admin" ) )
    {
		return player.adminCommand( argsString );
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
	Racesow_GetPlayerByClient(self.client).onSpawn();
	return null; // select random
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

        entry = "&p " + playerID + " " + ent.client.getClanName() + " "
                + Racesow_GetPlayerByClient( ent.client ).getBestTime() + " "
                + ent.client.ping + " " + racing + " ";

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
        RACE_playerKilled( G_GetEntity( arg1 ), attacker, G_GetEntity( arg2 ) );
    }
    else if ( score_event == "award" )
    {
    }
    else if ( score_event == "enterGame" )
    {
		player.reset();
        player.setClient(@client);
		player.authenticate( client.getUserInfoKey("racesow_auth_name"), client.getUserInfoKey("racesow_auth_pass"), true );
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
    Racesow_GetPlayerByClient( ent.client ).restartRace();

    if ( ent.isGhosting() )
        return;

    // set player movement to pass through other players
    ent.client.setPMoveFeatures( ent.client.pmoveFeatures | PMFEAT_GHOSTMOVE );
	ent.client.inventorySetCount( WEAP_GUNBLADE, 1 );

    // select rocket launcher if available
    if ( ent.client.canSelectWeapon( WEAP_ROCKETLAUNCHER ) )
        ent.client.selectWeapon( WEAP_ROCKETLAUNCHER );
    else
        ent.client.selectWeapon( -1 ); // auto-select best weapon in the inventory
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

    // set all clients race stats
    cClient @client;

    for ( int i = 0; i < maxClients; i++ )
    {
        @client = @G_GetClient( i );
        if ( client.state() < CS_SPAWNED )
            continue;
			
		Racesow_Player @player = Racesow_GetPlayerByClient( client );
		
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

        client.setHUDStat( STAT_TIME_BEST, player.getBestTime() / 100 );
        client.setHUDStat( STAT_TIME_RECORD, map.getStatsHandler().getHighScore(0).getTime() / 100 );

        client.setHUDStat( STAT_TIME_ALPHA, -9999 );
        client.setHUDStat( STAT_TIME_BETA, -9999 );

        if ( map.getStatsHandler().getHighScore(0).playerName.len() > 0 )
            client.setHUDStat( STAT_MESSAGE_OTHER, CS_GENERAL );
        if ( map.getStatsHandler().getHighScore(1).playerName.len() > 0 )
            client.setHUDStat( STAT_MESSAGE_ALPHA, CS_GENERAL + 1 );
        if ( map.getStatsHandler().getHighScore(2).playerName.len() > 0 )
            client.setHUDStat( STAT_MESSAGE_BETA, CS_GENERAL + 2 );
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
        RACE_SetUpMatch();
        break;

    case MATCH_STATE_COUNTDOWN:
        break;

    case MATCH_STATE_PLAYTIME:
        RACE_SetUpMatch();
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

    // setup the checkpoints arrays sizes adjusted to numCheckPoints
    for ( int i = 0; i < maxClients; i++ )
        players[i].reset();

		/*
		for ( int i = 0; i < MAX_RECORDS; i++ )
			map.getHighScore(i).reset();
		*/
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
    gametype.setVersion( "0.5.1b" );
    gametype.setAuthor( "warsow-race.net" );

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
                 + "set g_timelimit \"0\"\n"
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
                 + "set g_maxtimeouts \"-1\" // -1 = unlimited\n"
                 + "set g_challengers_queue \"0\"\n"
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

    gametype.ammoRespawn = 1;
    gametype.armorRespawn = 1;
    gametype.weaponRespawn = 1;
    gametype.healthRespawn = 1;
    gametype.powerupRespawn = 1;
    gametype.megahealthRespawn = 1;
    gametype.ultrahealthRespawn = 1;

    gametype.readyAnnouncementEnabled = false;
    gametype.scoreAnnouncementEnabled = false;
    gametype.countdownEnabled = false;
    gametype.mathAbortDisabled = true;
    gametype.shootingDisabled = false;
    gametype.infiniteAmmo = true;
    gametype.canForceModels = true;
    gametype.canShowMinimap = false;
	gametype.teamOnlyMinimap = true;

    gametype.spawnpointRadius = 0;

    // set spawnsystem type
    for ( int team = TEAM_PLAYERS; team < GS_MAX_TEAMS; team++ )
        gametype.setTeamSpawnsystem( team, SPAWNSYSTEM_INSTANT, 0, 0, false );

    // define the scoreboard layout
    G_ConfigString( CS_SCB_PLAYERTAB_LAYOUT, "%n 112 %s 52 %t 96 %l 48 %b 48" );
    G_ConfigString( CS_SCB_PLAYERTAB_TITLES, "Name Clan Time Ping Racing" );

    // add commands
    G_RegisterCommand( "gametype" );
    G_RegisterCommand( "racerestart" );
    G_RegisterCommand( "top" );
    G_RegisterCommand( "racesow_register" );
	G_RegisterCommand( "racesow_auth" );
	G_RegisterCommand( "admin" );

    demoRecording = false;

    G_Print( "Gametype '" + gametype.getTitle() + "' initialized\n" );
}
