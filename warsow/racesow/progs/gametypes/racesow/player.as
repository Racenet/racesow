/**
 * Racesow_Player
 *
 * @package Racesow
 * @version 0.5.1b
 * @author soh-zolex <zolex@warsow-race.net>
 */
class Racesow_Player
{
	/**
	 * Racesow account name
	 * @var cString
	 */
	cString authName;
	
	/**
	 * Racesow authorizations bitmask
	 * @var uint
	 */
	uint authMask;
	
	/**
	 * Number of failed auths in a row
	 * @var uint
	 */
	uint authFailCount;
	
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
		//@this.race = Racesow_Player_Race();
		//this.race.setPlayer(@this);
    }

	/**
	 * Destructor
	 *
	 */
    ~Racesow_Player()
	{
	}
	
	/**
	 * Reset the player
	 * @return void
	 */
	void reset()
	{
		this.authFailCount = 0;
		this.idleTime = 0;
		this.isSpawned = true;
		this.bestRaceTime = 0;
		this.bestCheckPoints.resize( numCheckpoints );
		for ( int i = 0; i < numCheckpoints; i++ )
			this.bestCheckPoints[i] = 0;
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
		if ( id >= this.bestCheckPoints.length() && time < this.bestCheckPoints[id])
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
		this.race.getPlayer().getClient().team = TEAM_SPECTATOR;
		@this.race = null;
    }
	
	/**
	 * startIdling
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
		G_PrintMsg( this.client.getEnt(), S_COLOR_RED + "Please hurry up, theo ther players are waiting for you to finish...\n" );
	}
	
	/**
	 * Register a new server account
	 *
	 * You can login using either your authName, your email or any
	 * of your nicknames plus your password. This requires to avoid
	 * cross-type duplicates, means you cannot for example register
	 * an auth-name which has already been taken as nickname or email.
	 *
	 * @param cString &authName
	 * @param cString &authEmail
	 * @param cString &password
	 * @param cString &confirmation
	 * @return bool
	 */
	bool registerAccount(cString &authName, cString &authEmail, cString &password, cString &confirmation)
	{
		cString nickName = this.getName().removeColorTokens();
		
		cString authFile = gameDataDir + "/auths/" + authName.substr(0,1) + "/" + authName;
		cString mailShadow = gameDataDir + "/emails/" + authEmail.substr(0,1) + "/" + authEmail;
		cString nickShadow = gameDataDir + "/nicknames/" + nickName.substr(0,1) + "/" + nickName;
		
		cString duplicateNameCheckNick = gameDataDir + "/auths/" + nickName.substr(0,1) + "/" + nickName;
		cString duplicateNameCheckEmail = gameDataDir + "/auths/" + authEmail.substr(0,1) + "/" + authEmail;
		cString duplicateEmailCheckName = gameDataDir + "/emails/" + authName.substr(0,1) + "/" + authName;
		cString duplicateEmailCheckNick = gameDataDir + "/emails/" + nickName.substr(0,1) + "/" + nickName;
		cString duplicateNickCheckEmail = gameDataDir + "/nicknames/" + authEmail.substr(0,1) + "/" + authEmail;
		cString duplicateNickCheckName = gameDataDir + "/nicknames/" + authName.substr(0,1) + "/" + authName;
		
		if ( authName == "" || authEmail == "" || password == "" || confirmation == "" )
		{
			G_PrintMsg( this.client.getEnt(), S_COLOR_RED + "usage: register <account name> <account email> <password> <confirm password>\n" );
			return false;
		}
		
		if ( password != confirmation )
		{
			G_PrintMsg( this.client.getEnt(), S_COLOR_RED + "Error: passwords do not match\n" );
			return false;
		}
		
	    if ( G_FileLength( authFile ) != -1 ||
			 G_FileLength( duplicateNameCheckNick ) != -1 ||
			 G_FileLength( duplicateNameCheckEmail ) != -1 )
		{
			G_PrintMsg( this.client.getEnt(), S_COLOR_RED + "Error: Your login '" + authName + "' is already registered.\n" );
			return false;
		}
		
		if ( G_FileLength( mailShadow ) != -1 ||
			G_FileLength( duplicateEmailCheckName ) != -1 ||
			G_FileLength( duplicateEmailCheckNick ) != -1 )
		{
			G_PrintMsg( this.client.getEnt(), S_COLOR_RED + "Error: Your email '" + authEmail + "' is already registered.\n" );
			return false;
		}		
		
		if ( G_FileLength( nickShadow ) != -1 ||
			G_FileLength( duplicateNickCheckName ) != -1 ||
			G_FileLength( duplicateNickCheckEmail ) != -1 )
		{
			G_PrintMsg( this.client.getEnt(), S_COLOR_RED + "Your nickname '" + this.getName() + "' is already registered.\n" );
			return false;
		}
		
		// TODO: check email for valid format
		
		if ( g_secureAuth.getBool() )
		{
			password = G_Md5( password );
		}
		
		// "authName" "authEmail" "authPass" "authMask" "timeStamp"
		G_WriteFile( authFile, '"'+ authName + '" "' + authEmail + '" "' + password + '" "' + 1 + '" "' + localTime + '"\n' );
		G_WriteFile( mailShadow, authName );
		G_WriteFile( nickShadow, authName );
		
		G_PrintMsg( this.client.getEnt(), S_COLOR_GREEN + "Successfully registered as "
			+ authName + ". " + S_COLOR_WHITE + "Don't forget your password...\n" );
		
		return true;
	}
	
	/**
	 * Authenticate server account
	 * @param cString &authName
	 * @param cString &password
	 * @return bool
	 */
	bool authenticate( cString &authName, cString &authPass, bool autoAuth )
	{
		if ( authName == "" || authPass == "" )
		{
			if ( !autoAuth )
				G_PrintMsg( client.getEnt(), S_COLOR_RED + "usage: auth <account name> <password>\n" );
			return false;
		}
		
		cString authFile = gameDataDir + "/auths/" + authName.substr(0,1) + "/" + authName;
		
		if ( G_FileLength( authFile ) == -1 )
		{
			authFile = gameDataDir + "/emails/" + authName.substr(0,1) + "/" + authName;
			if ( G_FileLength( authFile ) == -1 )
			{
				authFile = gameDataDir + "/nicknames/" + authName.substr(0,1) + "/" + authName;
				if ( G_FileLength( authFile ) != -1 )
				{
					authName = G_LoadFile( authFile );
					authFile = gameDataDir + "/auths/" + authName.substr(0,1) + "/" + authName;
				}
			}
			else
			{
				authName =  G_LoadFile( authFile );
				authFile = gameDataDir + "/auths/" + authName.substr(0,1) + "/" + authName;
			}
			
			if ( G_FileLength( authFile ) == -1 )
			{
				G_PrintMsg( client.getEnt(), S_COLOR_RED + "Error: "+ authName +" is not registered\n" );
				return false;
			}
		}

		cString authContent = G_LoadFile( authFile );
		
		if ( g_secureAuth.getBool() )
		{
			authPass = G_Md5( authPass );
		}
		
		if ( authPass != authContent.getToken( 2 ) )
		{
			if ( this.authFailCount >= 3 ) 
			{
				// TODO: kick player with reason instead of this print
				G_PrintMsg( null, S_COLOR_WHITE + this.getName() + S_COLOR_RED
					+ " kicked due to too many failed logins.\n" );
			}
			else 
			{
				G_PrintMsg( null, S_COLOR_WHITE + this.getName() + S_COLOR_RED 
					+ " failed in authenticating as "+ authName +"\n" );
			}
				
			return false;
		}
	
		this.authFailCount = 0;
		this.authName = authName;
		this.authMask = uint( authContent.getToken( 3 ).toInt() );
		
		G_PrintMsg( null, S_COLOR_WHITE + this.getName() + S_COLOR_GREEN
			+ " successfully authenticated as "+ authName +"\n" );
		
		return true;
	}
	
	/**
	 * Execute an admin command
	 * @param cString &cmdString
	 * @return bool
	 */
	bool adminCommand( cString &cmdString )
	{
		if ( this.authMask & RACESOW_AUTH_ADMIN == 0 )
		{
			G_PrintMsg( null, S_COLOR_WHITE + this.getName() + S_COLOR_RED
				+ " tried to execute an admin command without permission.\n" );
					
			return false;
		}
		
		cString command = cmdString.getToken( 0 );
		
		if ( command == "map" )
		{
			if ( this.authMask & RACESOW_AUTH_MAP == 0 )
			{
				G_PrintMsg( this.client.getEnt(), S_COLOR_RED + " you are not permitted to execute 'admin "+ cmdString +"'.\n" );
				return false;
			}
		}
		
		G_PrintMsg( null, S_COLOR_WHITE + this.getName() + S_COLOR_GREEN
			+ " executed command '"+ cmdString +"'\n" );

		return true;
	}
	
	/**
	 * Display the help to the player
	 * @return bool
	 */
	bool displayHelp()
	{
		cString myHelp = gametype.getName() + " " + gametype.getVersion() + " Help\n";
		myHelp += "--------------------------------------------------------------------------------------------------------------------------\n";
		myHelp += "help				display this help ;)\n";
		myHelp += "racerestart		go back to the start-area whenever you want\n";
		myHelp += "register			register a new account on this server\n";
		myHelp += "auth				authenticate to the server (alternatively you can SETU (!) auth_name and auth_pass in your autoexec.cfg)\n";
		myHelp += "admin			more info with 'admin help'\n";
		myHelp += "--------------------------------------------------------------------------------------------------------------------------\n\n";
	
		G_PrintMsg( this.client.getEnt(), myHelp );
		return true;
	}
}
