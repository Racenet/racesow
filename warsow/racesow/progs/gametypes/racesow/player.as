/**
 * Racesow_Player
 *
 * @package Racesow
 * @version 0.5.1c
 * @date 24.09.2009
 * @author soh-zolex <zolex@warsow-race.net>
 */
class Racesow_Player
{
	/**
	 * Did the player respawn on his own after finishing a race?
     * Info for the respawn thinker not to respawn the player again.
	 * @var bool
	 */
	bool isSpawned;

	/**
	 * Is the player still racing in the overtime?
	 * @var bool
	 */
	bool inOvertime;
	
	/**
	 * The time when the player started idling
	 * @var uint
	 */
	uint idleTime;
	
	/**
	 * The player's best race
	 * @var uint
	 */
    uint bestRaceTime;

	/**
	 * The player's best checkpoints
	 * stored across races
	 * @var uint[]
	 */
    uint[] bestCheckPoints;
	
	/**
	 * The player's client
	 * @var uint
	 */
	cClient @client;
	
	/**
	 * Player authentication and authorization
	 * @var Racesow_Player_Auth
	 */
	Racesow_Player_Auth @auth;
	
	/**
	 * The current race of the player
	 * @var Racesow_Player_Race
	 */
	Racesow_Player_Race @race;
	
	/**
	 * Constructor
	 *
	 */
    Racesow_Player()
    {
    }

	/**
	 * Destructor
	 *
	 */
    ~Racesow_Player()
	{
	}
	
	/**
	 * Reset the player, just f*ckin remove this and use the constructor...
	 * @return void
	 */
	void reset()
	{
		this.idleTime = 0;
		this.isSpawned = true;
		this.bestRaceTime = 0;
		this.resetAuth();
		this.auth.setPlayer(@this);
		this.bestCheckPoints.resize( numCheckpoints );
		for ( int i = 0; i < numCheckpoints; i++ )
		{
			this.bestCheckPoints[i] = 0;
		}	
	}
	
	/**
	 * Reset the players auth
	 * @return void
	 */
	void resetAuth()
	{
		if (@this.auth != null)
		{
			this.auth.killSession();
		}
		
		@this.auth = Racesow_Player_Auth();
	}
	
	/**
	 * Set the player's client
	 * @return void
	 */
	Racesow_Player @setClient( cClient @client )
	{
		@this.client = @client;
		return @this;
	}
	
	/**
	 * Get the player's client
	 * @return cClient
	 */
	cClient @getClient()
	{
		return @this.client;
	}
	
	/**
	 * Get the players best time
	 * @return uint
	 */
	uint getBestTime()
	{
		return this.bestRaceTime;
	}	
	
	/**
	 * Set the players best time
	 * @return void
	 */
	void setBestTime(uint time)
	{
		this.bestRaceTime = time;
	}
	
	/**
	 * getName
	 * @return cString
	 */
	cString getName()
	{
		return this.client.getName();
	}		
	
	/**
	 * getAuth
	 * @return cString
	 */
	Racesow_Player_Auth @getAuth()
	{
		return @this.auth;
	}	
	
	/**
	 * getBestCheckPoint
	 * @param uint id
	 * @return uint
	 */
	uint getBestCheckPoint(uint id)
	{
		if ( id >= this.bestCheckPoints.length() )
			return 0;
			
		return this.bestCheckPoints[id];
	}	
	
	/**
	 * setBestCheckPoint
	 * @param uint id
	 * @param uint time
	 * @return uint
	 */
	bool setBestCheckPoint(uint id, uint time)
	{
		if ( id >= this.bestCheckPoints.length() || time < this.bestCheckPoints[id])
			return false;
			
		this.bestCheckPoints[id] = time;
		return true;
	}
	
	/**
	 * Player spawn event
	 * @return void
	 */
	void onSpawn()
	{
	}
	
	/**
	 * Check if the player is currently racing
	 * @return uint
	 */
	bool isRacing()
	{
		if (@this.race == null) 
			return false;
			
		return this.race.inRace();
	}
	
	/**
	 * crossStartLine
	 * @return void
	 */
    void touchStartTimer()
    {
		if ( this.isRacing() )
            return;
		
		@this.race = Racesow_Player_Race();
		this.race.setPlayer(@this);
		this.race.start();
    }
	
	/**
	 * touchCheckPoint
	 * @param cClient @client
	 * @param int id
	 * @return void
	 */
    void touchCheckPoint( int id )
    {
		if ( id < 0 || id >= numCheckpoints )
            return;

        if ( !this.isRacing() )
            return;

		this.race.check( id );
    }
	
	/**
	 * completeRace
	 * @param cClient @client
	 * @return void
	 */
    void touchStopTimer()
    {
        // when the race can not be finished something is very wrong, maybe small penis playing
		if ( !this.isRacing() || !this.race.stop() )
            return;
			
		this.isSpawned = false;
		map.getStatsHandler().addRace(@this.race);
		
        // set up for respawning the player with a delay
        cEntity @respawner = G_SpawnEntity( "race_respawner" );
        respawner.nextThink = levelTime + 3000;
        respawner.count = client.playerNum();
    }
	
	/**
	 * restartRace
	 * @return void
	 */
    void restartRace()
    {
		this.isSpawned = true;
		@this.race = null;
    }	
	
	/**
	 * cancelRace
	 * @return void
	 */
    void cancelRace()
    {
		this.client.team = TEAM_SPECTATOR;
		@this.race = null;
    }
	
	/**
	 * Player has just started idling (triggered in overtime)
	 * @return void
	 */
	void startIdling()
	{
		this.idleTime = levelTime;
	}	
	
	/**
	 * stopIdling
	 * @return void
	 */
	void stopIdling()
	{
		this.idleTime = 0;
	}
	
	/**
	 * getIdleTime
	 * @return uint
	 */
	uint getIdleTime()
	{
		if ( !this.startedIdling() )
			return 0;
			
		return levelTime - this.idleTime;
	}	
	
	/**
	 * startedIdling
	 * @return bool
	 */
	bool startedIdling()
	{
		return this.idleTime != 0;
	}
	
	/**
	 * startOvertime
	 * @return void
	 */
	void startOvertime()
	{
		this.inOvertime = true;
		this.sendMessage( S_COLOR_RED + "Please hurry up, theo ther players are waiting for you to finish...\n" );
	}
	
	/**
	 * Send a message to console of the player
	 * @param cString message
	 * @return void
	 */
	void sendMessage( cString message )
	{
		// just send to original func
		G_PrintMsg( this.client.getEnt(), message );
		
		// maybe log messages for some reason to figure out ;)
	}
	
	/**
	 * Kick the player and leave a message for everyone
	 * @param cString message
	 * @return void
	 */
	void kick( cString message )
	{
		G_PrintMsg( null, message + "\nHe should now get kicked (TODO)\n");
		this.auth.lastViolateProtectionMessage = 0;
		this.auth.violateNickProtectionSince = 0;
	}
	
	/**
	 * Execute an admin command
	 * @param cString &cmdString
	 * @return bool
	 */
	bool adminCommand( cString &cmdString )
	{
		bool commandExists = false;
		bool showNotification = false;
		cString command = cmdString.getToken( 0 );
		
		if ( !this.auth.allow( RACESOW_AUTH_ADMIN ) )
		{
			G_PrintMsg( null, S_COLOR_WHITE + this.getName() + S_COLOR_RED
				+ " tried to execute an admin command without permission.\n" );
					
			return false;
		}
	
		// no command
		if ( command == "" )
		{
			this.sendMessage( S_COLOR_RED + "No command given. Use 'admin help' for more information.\n" );
			return false;
		}
		
		// map command
		else if ( commandExists = command == "map" )
		{
			if ( !this.auth.allow( RACESOW_AUTH_MAP ) )
			{
				this.sendMessage( S_COLOR_RED + "You are not permitted "
					+ "to execute the command 'admin "+ cmdString +"'.\n" );
					
				return false;
			}
			
			cString mapName = cmdString.getToken( 1 );
			if ( mapName == "" )
			{
				this.sendMessage( S_COLOR_RED + "No map name given.\n" );
				return false;
			}
			
			G_CmdExecute("gamemap "+ mapName + "\n");
			showNotification = true;
		}	
		
		// kick command
		else if ( commandExists = command == "kick" )
		{
			if ( !this.auth.allow( RACESOW_AUTH_KICK ) )
			{
				this.sendMessage( S_COLOR_RED + "You are not permitted "
					+ "to execute the command 'admin "+ cmdString +"'.\n" );
					
				return false;
			}
			
			cString playerNum = cmdString.getToken( 1 );
			G_CmdExecute("kick "+ playerNum + "\n");
			showNotification = true;
		}
		
		// help command
		else if( commandExists = command == "help" )
		{
			this.displayAdminHelp();
		}
		
		// don't touch the rest of the method!
		if( !commandExists )
		{
			this.sendMessage( S_COLOR_RED + "The command 'admin " + cmdString + "' does not exist.\n" );
			return false;
		}
		else if ( showNotification )
		{
			G_PrintMsg( null, S_COLOR_WHITE + this.getName() + S_COLOR_GREEN
				+ " executed command '"+ cmdString +"'\n" );
		}
		
		return true;
	}
	
	/**
	 * Display the help to the player
	 * @return bool
	 */
	void displayAdminHelp()
	{
		cString help;
		help += S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n";
		help += S_COLOR_RED + "ADMIN HELP for " + gametype.getName() + "\n";
		help += S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n";
		help += S_COLOR_RED + "admin map     " + S_COLOR_YELLOW + "change to the given map immedeatly\n";
		help += S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n\n";
	
		G_PrintMsg( this.client.getEnt(), help );
	}	
	
	/**
	 * Display the help to the player
	 * @return bool
	 */
	bool displayHelp()
	{
		cString help;
		help += S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n";
		help += S_COLOR_RED + "HELP for " + gametype.getName() + "\n";
		help += S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n";
		help += S_COLOR_RED + "help            " + S_COLOR_YELLOW + "display this help ;)\n";
		help += S_COLOR_RED + "racerestart  " + S_COLOR_YELLOW + "go back to the start-area whenever you want\n";
		help += S_COLOR_RED + "register      " +  S_COLOR_YELLOW + "register a new account on this server\n";
		help += S_COLOR_RED + "auth            " + S_COLOR_YELLOW + "authenticate to the server (alternatively you can use setu\n";
		help += "                  " + S_COLOR_YELLOW + "to set auth_name and auth_pass in your autoexec.cfg)\n";
		help += S_COLOR_RED + "admin          " + S_COLOR_YELLOW + "more info with 'admin help'\n";
		help += S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n\n";
	
		G_PrintMsg( this.client.getEnt(), help );
		return true;
	}
}
