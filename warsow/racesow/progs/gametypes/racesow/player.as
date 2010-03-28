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
	 * Is the player allowed to join?
	 * @var bool
	 */
	bool isJoinlocked;

	/**
	 * Is the player allowed to call a vote?
	 * @var bool
	 */
	bool isVotemuted;

	/**
	 * Is the player using the chrono function?
	 * @var bool
	 */
	bool isUsingChrono;

	/**
	 * Was the player Telekilled?
	 * @var bool
	 */
	bool wasTelekilled;

	/**
	 * cEntity which stores the latest position after the telekill
	 * @var cEntity
	 */
	cEntity@ gravestone;

	/**
	 * The time when the player started the chrono
	 * @var uint
	 */
	uint chronoStartTime;

	/**
	 * The time when the player started idling
	 * @var uint
	 */
	uint idleTime;

    /**
     * the time when the player started to make
     * a perfect plasma climb
     * @var uint64
     */
    uint64 plasmaPerfectClimbSince;

	/**
	 * The player's best race
	 * @var uint
	 */
    uint bestRaceTime;

	/**
	 * Local time of the last top command (flood protection)
	 * @var uint
	 */
    uint top_lastcmd;

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
		@this.auth = Racesow_Player_Auth();
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
		this.isJoinlocked = false;
		this.isVotemuted = false;
		this.wasTelekilled = false;
		this.bestRaceTime = 0;
        this.plasmaPerfectClimbSince = 0;
		this.resetAuth();
		this.auth.setPlayer(@this);
		this.bestCheckPoints.resize( numCheckpoints );
		for ( int i = 0; i < numCheckpoints; i++ )
		{
			this.bestCheckPoints[i] = 0;
		}
		if(@this.gravestone != null)
			this.gravestone.freeEntity();
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
			this.auth.reset();
		}
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
     * Rate the current
     * plasma climb.
     * @return int
     */
    int ratePlasmaClimbing()
    {
        if ( this.client.weapon != WEAP_PLASMAGUN )
            return 0;

        if ( this.client.getEnt().getVelocity().z <= 0 )
            return 0;

        if ( (this.client.getEnt().getAngles().x) < 69 || (this.client.getEnt().getAngles().x > 73) )
            return 0;

        if ( (this.client.getEnt().getAngles().x >= 71) && (this.client.getEnt().getAngles().x <= 72) )
            return 2;

        if ( (this.client.getEnt().getAngles().x) >= 69 && (this.client.getEnt().getAngles().x <= 73) )
            return 1;

        return 0;
    }


    /**
     * Get the player's status concerning plasma climb.
     * @return int
     */
    int processPlasmaClimbStatus()
    {
        int rate = this.ratePlasmaClimbing();

        if ( rate == 0)
        {
            this.plasmaPerfectClimbSince = 0;
            return 0;
        }
        else
        {
            if ( this.plasmaPerfectClimbSince == 0 )
            {
                this.plasmaPerfectClimbSince = localTime;
                return 0;
            }

            int seconds = localTime - this.plasmaPerfectClimbSince;
            if ( seconds > 1 )
            {
                this.plasmaPerfectClimbSince = 0;
                return rate;
            }
            else
            {
                return 0;
            }

        }
    }

	/**
	 * startOvertime
	 * @return void
	 */
	void startOvertime()
	{
		this.inOvertime = true;
		this.sendMessage( S_COLOR_RED + "Please hurry up, the ther players are waiting for you to finish...\n" );
	}

	/**
	 * setupTelekilled
	 * @return void
	 */
	void setupTelekilled( cEntity@ gravestone)
	{
		@this.gravestone = @gravestone;
		this.wasTelekilled = true;
	}

	/**
	 * wasTelekilled
	 * @return bool
	 */
	bool wasTelekilled()
	{
		return this.wasTelekilled;
	}

	/**
	 * returnGravestone
	 * @return cEntity
	 */
	cEntity@ returnGravestone()
	{
		return @this.gravestone;
	}

	/**
	 * resetTelekilled
	 * @return void
	 */
	void resetTelekilled()
	{
		this.gravestone.freeEntity();
		this.wasTelekilled = false;
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
	* Send a message to another player's console
	* @param cString message, cClient @client
	* @return void
	*/
	void sendMessage( cString message, cClient @client )
	{
		G_PrintMsg( client.getEnt(), message );
	}


	/**
	 * Send a message to another player
	 * @param cString argString, cClient @client
	 * @return bool
	 */
	bool privSay( cString argsString, cClient @client )
	{
		if (argsString.getToken( 0 ) == "" || argsString.getToken( 1 ) == "" )
		{
			sendMessage( S_COLOR_RED + "Empty arguments.\n", @client );
			return false;
		}


		if ( argsString.getToken( 0 ).toInt() > maxClients)
			return false;
		cClient @target;

		if ( argsString.getToken( 0 ).isNumerical()==true)
			@target = @G_GetClient( argsString.getToken( 0 ).toInt() );
		else
			if (Racesow_GetClientNumber( argsString.getToken( 0 )) != -1)
				@target = @G_GetClient( Racesow_GetClientNumber( argsString.getToken( 0 )) );

		cString message = argsString.substr(argsString.getToken( 0 ).length()+1,argsString.length());

		// check if target doesn't exist
		if ( @target == null || target.getName().length() <=2)
		{
			this.sendMessage( S_COLOR_RED + "Invalid player!\n", @client );
			return false;
		}
		else
		{
			sendMessage( S_COLOR_RED + "(Private message to " + S_COLOR_WHITE + target.getName()
			+ S_COLOR_RED + " ) " + S_COLOR_WHITE + ": " + message + "\n", @client );
			sendMessage( S_COLOR_RED + "(Private message from " + S_COLOR_WHITE + client.getName()
			+ S_COLOR_RED + " ) " + S_COLOR_WHITE + ": " + message + "\n", @target );
		}
		return true;
	}

	/**
	 * Kick the player and leave a message for everyone
	 * @param cString message
	 * @return void
	 */
	void kick( cString message )
	{
		G_PrintMsg( null, S_COLOR_RED + message + "\n" );
		G_CmdExecute( "kick " + this.client.playerNum() );
		this.auth.lastViolateProtectionMessage = 0;
		this.auth.violateNickProtectionSince = 0;
	}

	/**
	 * Remove the player and leave a message for everyone
	 * @param cString message
	 * @return void
	 */
	void remove( cString message )
	{
		if( message.length() > 0)
			G_PrintMsg( null, S_COLOR_RED + message + "\n" );
		this.client.team = TEAM_SPECTATOR;
		this.client.respawn( true );
	}

	/**
	 * Cancel the current vote (equals to /opcall cancelvote)
	 */
	void cancelvote()
	{
		G_CmdExecute( "cancelvote" );
	}

	/**
	 * Switch player ammo between strong/weak
	 * @param cClient @client
	 * @return bool
	 */
	bool ammoSwitch( cClient @client )
	{
		if ( g_freestyle.getBool() || g_allowammoswitch.getBool() )
		{
			if ( client.inventoryCount( client.pendingWeapon ) == 0 )
			{
				// something is really wrong as the client can't
				// own a weapon which is not in his inventory
				return false;
			}

			uint strong_ammo = client.pendingWeapon + 9;
			uint weak_ammo = client.pendingWeapon + 18;

			if ( client.inventoryCount( strong_ammo ) > 0 )
			{
				client.inventorySetCount( weak_ammo, client.inventoryCount( strong_ammo ) );
				client.inventorySetCount( strong_ammo, 0 );
			}
			else
			{
				client.inventorySetCount( strong_ammo, client.inventoryCount( weak_ammo ) );
			}
		}
		else
		{
			sendMessage( S_COLOR_RED + "Ammoswitch is disabled.\n", @client );
		}
		return true;
	}

	/**
	 * Chrono function
	 * @parm cString cmdString
	 * @return bool
	 */
	bool chronoUse( cString cmdString )
	{
		cString command = cmdString.getToken( 0 );
		if ( g_freestyle.getBool() )
		{
			if( command == "start" ) // chrono start
			{
				this.chronoStartTime = levelTime;
				this.isUsingChrono = true;
			}
			else if( command == "reset" )
			{
				this.chronoStartTime = 0;
				this.isUsingChrono = false;
			}
			else
			{
				this.sendMessage( S_COLOR_WHITE + "Chrono function. Usage: chrono start/reset.\n" );
			}

			return true;
		}
		return false;
	}

	/**
	 * Chrono function
	 * @return bool
	 */
	bool chronoInUse()
	{
		return this.isUsingChrono;
	}

	/**
	 * Chrono time
	 * @return uint
	 */
	uint chronoTime()
	{
		return this.chronoStartTime;
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

		// remove command
		else if ( commandExists = command == "remove" )
		{
			if ( !this.auth.allow( RACESOW_AUTH_KICK ) )
			{
				this.sendMessage( S_COLOR_RED + "You are not permitted "
					+ "to execute the command 'admin "+ cmdString +"'.\n" );

				return false;
			}

			int playerNum = cmdString.getToken( 1 ).toInt();
			if( playerNum > maxClients )
				return false;
			if( @players[playerNum].getClient() == null )
				return false;
			players[playerNum].remove( cmdString.getToken( 2 ) );
			showNotification = true;
		}

		// joinlock command
		else if ( commandExists = command == "joinlock" )
		{
			if ( !this.auth.allow( RACESOW_AUTH_KICK ) )
			{
				this.sendMessage( S_COLOR_RED + "You are not permitted "
					+ "to execute the command 'admin "+ cmdString +"'.\n" );

				return false;
			}

			int playerNum = cmdString.getToken( 1 ).toInt();
			if( playerNum > maxClients )
				return false;
			if( @players[ playerNum ].getClient() == null )
				return false;
			players[playerNum].remove( cmdString.getToken( 2 ) );
			players[playerNum].isJoinlocked = true;
			showNotification = true;
		}

		// unjoinlock command
		else if ( commandExists = command == "unjoinlock" )
		{
			if ( !this.auth.allow( RACESOW_AUTH_KICK ) )
			{
				this.sendMessage( S_COLOR_RED + "You are not permitted "
					+ "to execute the command 'admin "+ cmdString +"'.\n" );

				return false;
			}

			int playerNum = cmdString.getToken( 1 ).toInt();
			if( playerNum > maxClients )
				return false;
			if( @players[ playerNum ].getClient() == null )
				return false;
			players[playerNum].isJoinlocked = false;
			showNotification = true;
		}

		// unjoinlock command
		else if ( commandExists = command == "votemute" )
		{
			if ( !this.auth.allow( RACESOW_AUTH_KICK ) )
			{
				this.sendMessage( S_COLOR_RED + "You are not permitted "
					+ "to execute the command 'admin "+ cmdString +"'.\n" );

				return false;
			}

			int playerNum = cmdString.getToken( 1 ).toInt();
			if( playerNum > maxClients )
				return false;
			if( @players[ playerNum ].getClient() == null )
				return false;
			players[playerNum].isVotemuted = true;
			showNotification = true;
		}

		// unvotemute command
		else if ( commandExists = command == "unvotemute" )
		{
			if ( !this.auth.allow( RACESOW_AUTH_KICK ) )
			{
				this.sendMessage( S_COLOR_RED + "You are not permitted "
					+ "to execute the command 'admin "+ cmdString +"'.\n" );

				return false;
			}

			int playerNum = cmdString.getToken( 1 ).toInt();
			if( playerNum > maxClients )
				return false;
			if( @players[ playerNum ].getClient() == null )
				return false;
			players[playerNum].isVotemuted = false;
			showNotification = true;
		}


		// mute command
		else if ( commandExists = command == "mute" )
		{
			if ( !this.auth.allow( RACESOW_AUTH_KICK ) )
			{
				this.sendMessage( S_COLOR_RED + "You are not permitted "
					+ "to execute the command 'admin "+ cmdString +"'.\n" );

				return false;
			}
			cString playerNum = cmdString.getToken( 1 );
			if( playerNum.toInt() > maxClients )
				return false;
			int mute = G_GetClient( playerNum.toInt() ).muted;
			if( mute == 0 || mute == 2 )
				G_GetClient( playerNum.toInt() ).muted = mute + 1;
			showNotification = true;
		}

		// unmute command
		else if ( commandExists = command == "unmute" )
		{
			if ( !this.auth.allow( RACESOW_AUTH_KICK ) )
			{
				this.sendMessage( S_COLOR_RED + "You are not permitted "
					+ "to execute the command 'admin "+ cmdString +"'.\n" );

				return false;
			}

			cString playerNum = cmdString.getToken( 1 );
			if( playerNum.toInt() > maxClients )
				return false;
			int mute = G_GetClient( playerNum.toInt() ).muted;
			if( mute == 1 || mute == 3 )
				G_GetClient( playerNum.toInt() ).muted = mute - 1;
			showNotification = true;
		}

		// vmute command
		else if ( commandExists = command == "vmute" )
		{
			if ( !this.auth.allow( RACESOW_AUTH_KICK ) )
			{
				this.sendMessage( S_COLOR_RED + "You are not permitted "
					+ "to execute the command 'admin "+ cmdString +"'.\n" );

				return false;
			}

			cString playerNum = cmdString.getToken( 1 );
			if( playerNum.toInt() > maxClients )
				return false;
			int mute = G_GetClient( playerNum.toInt() ).muted;
			if( mute == 0 || mute == 1 )
				G_GetClient( playerNum.toInt() ).muted = mute + 2;
			showNotification = true;
		}

		// vunmute command
		else if ( commandExists = command == "vunmute" )
		{
			if ( !this.auth.allow( RACESOW_AUTH_KICK ) )
			{
				this.sendMessage( S_COLOR_RED + "You are not permitted "
					+ "to execute the command 'admin "+ cmdString +"'.\n" );

				return false;
			}

			cString playerNum = cmdString.getToken( 1 );
			if( playerNum.toInt() > maxClients )
				return false;
			int mute = G_GetClient( playerNum.toInt() ).muted;
			if( mute == 2 || mute == 3 )
				G_GetClient( playerNum.toInt() ).muted = mute - 2;
			showNotification = true;
		}

		// cancelvote command
		else if ( commandExists = command == "cancelvote" )
		{
			if ( !this.auth.allow( RACESOW_AUTH_KICK ) )
			{
				this.sendMessage( S_COLOR_RED + "You are not permitted "
					+ "to execute the command 'admin "+ cmdString +"'.\n" );

				return false;
			}
			cancelvote();
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
		help += S_COLOR_RED + "help           " + S_COLOR_YELLOW + "display this help ;)\n";
		help += S_COLOR_RED + "top             " + S_COLOR_YELLOW + "display the top 30 times on this map\n";
        help += S_COLOR_RED + "racerestart " + S_COLOR_YELLOW + "go back to the start-area whenever you want\n";
		help += S_COLOR_RED + "register      " +  S_COLOR_YELLOW + "register a new account on this server\n";
		help += S_COLOR_RED + "auth            " + S_COLOR_YELLOW + "authenticate to the server (alternatively you can use setu\n";
		help += "                  " + S_COLOR_YELLOW + "to set auth_name and auth_pass in your autoexec.cfg)\n";
		help += S_COLOR_RED + "admin          " + S_COLOR_YELLOW + "more info with 'admin help'\n";
        help += S_COLOR_RED + "weapondef   " + S_COLOR_YELLOW + "change the weapon values, more info with 'weapondef help'\n";
		help += S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n\n";

		G_PrintMsg( this.client.getEnt(), help );
		return true;
	}
}
