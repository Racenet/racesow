/**
 * Racesow_Player
 *
 * @package Racesow
 * @version 0.5.4
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
     * Is the player using the quad command?
     * @var bool
     */
    bool onQuad;
	
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
	 * Time when the player joined (used to compute playtime)
	 * @var uint
	 */
	uint joinedTime;

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
    uint topLastcmd;

	/**
	 * Is the player waiting for the result of a command (like "top")?
	 * (this is another kind of flood protection)
	 * @var bool
	 */
	bool isWaitingForCommand;


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
	 * The weapon which was used before the noclip command, in order to restore it
	 * @var int
	 */
	int noclipWeapon;

	/**
	 * When is he allowed to trigger again?(= leveltime + timeout)
	 * @var uint
	 */
	uint triggerTimeout;

	/**
	 * Storage for the triggerd entity
	 * @var @cEntity
	 */
	cEntity @triggerEntity;


	/**
	 * Variables for the position function
	 */
	uint positionLastcmd; //flood protection
	bool positionSaved; //is a position saved?
	cVec3 positionOrigin; //stored origin
	cVec3 positionAngles; //stored angles
	int positionWeapon; //stored weapon

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
        this.inOvertime = false;
		this.isVotemuted = false;
		this.wasTelekilled = false;
		this.onQuad = false;
		this.isWaitingForCommand = false;
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
		this.positionSaved = false;
		@this.triggerEntity = null;
		this.triggerTimeout = 0;
	}

	/**
	 * Reset the players auth
	 * @return void
	 */
	void resetAuth()
	{
		if (@this.auth != null)
		{
			this.auth.reset();
		}
	}

    /**
     * The player appears in the game
     * @return void
     */
    void appear()
    {
        this.joinedTime = levelTime;
        RS_MysqlPlayerAppear(
            this.getName(),
            this.getClient().playerNum(),
            this.getId(),
            map.getId(),
            this.getAuth().isAuthenticated(),
            this.getAuth().authenticationName,
            this.getAuth().authenticationPass,
            this.getAuth().authenticationToken
        );
    }

    void disappear(cString nickName)
    {
        RS_MysqlPlayerDisappear(
            nickName,
            levelTime-this.joinedTime,
            this.getId(),
            this.getNickId(),
            map.getId(),
            this.getAuth().isAuthenticated()
        );
    }

    /**
	 * Get the player's id
	 * @return int
	 */
    int getId()
    {
        return this.auth.playerId;
    }

    /**
	 * Get the id of the current nickname
	 * @return int
	 */
    int getNickId()
    {
        return this.auth.nickId;
    }

	/**
	 * SGet the player's id
	 * @return int
	 */
    void setId(int playerId)
    {
        this.sendMessage( S_COLOR_BLUE + "Your PlayerID: "+ playerId +"\n" );
        this.auth.setPlayerId(playerId);
    }

    /**
	 * Set the id of the current nickname
	 * @return int
	 */
    void setNickId(int nickId)
    {
        this.auth.nickId=nickId;
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
		if (@this.client != null)
        {
            return this.client.getName();
        }

        return "";
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

		if ( g_freestyle.getBool() )
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
		//remove all projectiles.
		if( @this.client.getEnt() != null )
			removeProjectiles( this.client.getEnt() );
    }

	/**
	 * cancelRace
	 * @return void
	 */
    void cancelRace()
    {
		this.remove("");
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
		this.sendMessage( S_COLOR_RED + "Please hurry up, the other players are waiting for you to finish...\n" );
	}

	/**
	 * resetOvertime
	 * @return void
	 */
	void cancelOvertime()
	{
		this.inOvertime = false;
	}


	/**
	 * set Trigger Timout for map entitys
	 * @return void
	 */
	void setTriggerTimeout(uint timeout)
	{
		this.triggerTimeout = timeout;
	}

	/**
	 * get the Trigger Timout
	 * @return uint
    */
	uint getTriggerTimeout()
	{
		return this.triggerTimeout;
	}

	/**
	 * set the Triggered map Entity
	 * @return void
	 */
	void setTriggerEntity( cEntity @ent )
	{
		@this.triggerEntity = @ent;
	}

	/**
	 * setupTelekilled
	 * @return void
	 */
	void setupTelekilled( cEntity @gravestone)
	{
		@this.gravestone = @gravestone;
		this.wasTelekilled = true;
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
	 * noclip Command
	 * @return bool
	 */
	bool noclip()
	{
		if( !g_freestyle.getBool() && !sv_cheats.getBool() )
			return false;
		cEntity @ent = @this.client.getEnt();
		if( @ent == null || ent.team == TEAM_SPECTATOR )
			return false;
		if( ent.moveType == MOVETYPE_NOCLIP )
		{
		    cVec3 mins, maxs;
		    client.getEnt().getSize( mins, maxs );
		    cTrace tr;
            if( tr.doTrace( this.client.getEnt().getOrigin(), mins, maxs,
                    this.client.getEnt().getOrigin(), 0, MASK_PLAYERSOLID ))
                //don't allow players to end noclip inside others or the world
                return false;
			ent.moveType = MOVETYPE_PLAYER;
			ent.solid = SOLID_YES;
			this.client.selectWeapon( this.noclipWeapon );
		}
		else
		{
			ent.moveType = MOVETYPE_NOCLIP;
            ent.solid = SOLID_NOT;//don't get hit by projectiles/splash damage; don't block
			this.noclipWeapon = client.weapon;
		}

		return true;
	}
	
   /**
     * quad Command
     * @return bool
     */
    bool quad()
    {
        if( !g_freestyle.getBool() && !sv_cheats.getBool() )
            return false;
        cEntity @ent = @this.client.getEnt();
        if( @ent == null || ent.team == TEAM_SPECTATOR )
            return false;
        if( this.onQuad )
        {
            this.onQuad = false;
        }
        else
        {
            this.onQuad = true;
        }

        return true;
    }

	/**
	 * teleport the player
	 * @return bool
	 */
	bool teleport( cVec3 origin, cVec3 angles, bool keepVelocity, bool kill )
	{
		cEntity@ ent = @this.client.getEnt();
		if( @ent == null )
			return false;
		if( ent.team != TEAM_SPECTATOR )
		{
			cVec3 mins, maxs;
			ent.getSize(mins, maxs);
			cTrace tr;
			if(	g_freestyle.getBool() && tr.doTrace( origin, mins, maxs, origin, 0, MASK_PLAYERSOLID ))
			{
				cEntity @other = @G_GetEntity(tr.entNum);
				if(@other == @ent)
				{
					//do nothing
				}
				else if(!kill) // we aren't allowed to kill :(
					return false;
				else // kill! >:D
				{
					if(@other != null && other.type == ET_PLAYER )
					{
						other.takeDamage( @other, null, cVec3(0,0,0), 9999, 0, 0, MOD_TELEFRAG );
						//spawn a gravestone to store the postition
						cEntity @gravestone = @G_SpawnEntity( "gravestone" );
						// copy client position
						gravestone.setOrigin( other.getOrigin() + cVec3( 0.0f, 0.0f, 50.0f ) );
						Racesow_GetPlayerByClient( other.client ).setupTelekilled( @gravestone );
					}

				}
			}
			ent.teleportEffect( false );
		}
		if(!keepVelocity)
			ent.setVelocity( cVec3(0,0,0) );
		ent.setOrigin( origin );
		ent.setAngles( angles );
		if( ent.team != TEAM_SPECTATOR )
			ent.teleportEffect( true );
		return true;
	}

	/**
	 * position Command
	 * @return bool
	 */
	bool position( cString argsString )
	{
		if( !g_freestyle.getBool() && !sv_cheats.getBool() )
			return false;
		if( this.positionLastcmd + 500 > realTime )
			return false;
		this.positionLastcmd = realTime;

		cString action = argsString.getToken( 0 );

		if( action == "save" )
		{
			this.positionSaved = true;
			cEntity@ ent = @this.client.getEnt();
			if( @ent == null )
				return false;
			this.positionOrigin = ent.getOrigin();
			this.positionAngles = ent.getAngles();
			this.positionWeapon = client.weapon;
		}
		else if( action == "load" )
		{
			if(!this.positionSaved)
				return false;
			if( this.teleport( this.positionOrigin, this.positionAngles, false, false ) )
				this.client.selectWeapon( this.positionWeapon );
			return true;
		}
		else if( action == "set" && argsString.getToken( 5 ) != "" )
		{
			cVec3 origin, angles;

			origin.x = argsString.getToken( 1 ).toFloat();
			origin.y = argsString.getToken( 2 ).toFloat();
			origin.z = argsString.getToken( 3 ).toFloat();
			angles.x = argsString.getToken( 4 ).toFloat();
			angles.y = argsString.getToken( 5 ).toFloat();

			return this.teleport( origin, angles, false, false );
		}
		else if( action == "store" && argsString.getToken( 2 ) != "" )
		{
			cVec3 position = client.getEnt().getOrigin();
			cVec3 angles = client.getEnt().getAngles();
			//position set <x> <y> <z> <pitch> <yaw>
			this.client.execGameCommand("cmd seta storedposition_" + argsString.getToken(1)
					+ " \"" +  argsString.getToken(2) + " "
					+ position.x + " " + position.y + " " + position.z + " "
					+ angles.x + " " + angles.y + "\";writeconfig config.cfg");
		}
		else if( action == "restore" && argsString.getToken( 1 ) != "" )
		{
			G_CmdExecute( "cvarcheck " + this.client.playerNum()
					+ " storedposition_" + argsString.getToken(1) );
		}
		else if( action == "storedlist" && argsString.getToken( 1 ) != "" )
		{
			if( argsString.getToken(1).toInt() > 50 )
			{
				this.sendMessage( S_COLOR_WHITE + "You can only list the 50 the most\n" );
				return false;
			}
			this.sendMessage( S_COLOR_WHITE + "###\n#List of stored positions\n###\n" );
			int i;
			for( i = 0; i < argsString.getToken(1).toInt(); i++ )
			{
				this.client.execGameCommand("cmd  echo ~~~;echo id#" + i
						+ ";storedposition_" + i +";echo ~~~;" );
			}
		}
		else
		{
			cEntity@ ent = @this.client.getEnt();
			if( @ent == null )
				return false;
			cString msg;
			msg = "Usage:\nposition save - Save current position\n";
			msg += "position load - Teleport to saved position\n";
			msg += "position set <x> <y> <z> <pitch> <yaw> - Teleport to specified position\n";
			msg += "position store <id> <name> - Store a position for another session\n";
			msg += "position restore <id> - Restore a stored position from another session\n";
			msg += "position storedlist <limit> - Sends you a list of your stored positions\n";
			msg += "Current position: " + " " + ent.getOrigin().x + " " + ent.getOrigin().y + " " +
		ent.getOrigin().z + " " + ent.getAngles().x + " " + ent.getAngles().y + "\n";
			this.sendMessage( msg );
		}

		return true;
	}

	/**
	 * Send a message to console of the player
	 * @param cString message
	 * @return void
	 */
	void sendMessage( cString message )
	{
		if (@this.client == null)
            return;

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
        int playerNum = this.client.playerNum();
		G_PrintMsg( null, S_COLOR_RED + "Kicked "+ this.getName() + S_COLOR_RED + " Reason: " + message + "\n" );
		this.reset();
        G_CmdExecute( "kick " + playerNum );
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
	bool ammoSwitch(  )
	{
		if ( g_freestyle.getBool() || g_allowammoswitch.getBool() )
		{
			if ( @this.client.getEnt() == null )
			{
				return false;
			}
			cItem @item;
		    cItem @weakItem;
		    cItem @strongItem;
		    @item = @G_GetItem( this.client.getEnt().weapon );
		    if(@item == null || this.client.getEnt().weapon == 1 )
		    	return false;
		    @weakItem = @G_GetItem( item.weakAmmoTag );
		    @strongItem = @G_GetItem( item.ammoTag );
			uint strong_ammo = this.client.pendingWeapon + 9;
			uint weak_ammo = this.client.pendingWeapon + 18;

			if ( this.client.inventoryCount( item.ammoTag ) > 0 )
			{
				this.client.inventorySetCount( item.weakAmmoTag, weakItem.inventoryMax );
				this.client.inventorySetCount( item.ammoTag, 0 );
			}
			else
			{
				this.client.inventorySetCount( item.ammoTag, strongItem.inventoryMax );
			}
		}
		else
		{
			sendMessage( S_COLOR_RED + "Ammoswitch is disabled.\n", @this.client );
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
	 
	bool chronoInUse()
	{
		return this.isUsingChrono;
	}*/

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

		// kick, votemute, remove, mute, joinlock  commands (RACESOW_AUTH_KICK)
		else if ( command == "mute" || command == "unmute" || command == "vmute" ||
				command == "vunmute" || command == "remove"|| command == "kick"
					|| command == "votemute" || command == "unvotemute")
		{
			commandExists = true;
			if ( !this.auth.allow( RACESOW_AUTH_KICK ) )
			{
				this.sendMessage( S_COLOR_RED + "You are not permitted "
					+ "to execute the command 'admin "+ cmdString +"'.\n" );

				return false;
			}
			if( cmdString.getToken( 1 ) == "" )
			{
				this.client.execGameCommand("cmd players");
				showNotification = false;
				return false;
			}
			int playerNum = cmdString.getToken( 1 ).toInt();
			if( playerNum > maxClients )
				return false;

			if( command == "kick" )
				G_CmdExecute("kick "+ playerNum + "\n");
			else if( command == "votemute" )
				players[playerNum].isVotemuted = true;
			else if( command == "unvotemute" )
				players[playerNum].isVotemuted = false;
			else if( command == "remove" )
				players[playerNum].remove("");
			else if( command == "mute" )
				G_GetClient( playerNum ).muted |= 1;
			else if( command == "unmute" )
				G_GetClient( playerNum ).muted &= ~1;
			else if( command == "vmute" )
				G_GetClient( playerNum ).muted |= 2;
			else if( command == "vunmute" )
				G_GetClient( playerNum ).muted &= ~2;
			else if( command == "joinlock" )
				players[playerNum].isJoinlocked = true;
			else if( command == "unjoinlock" )
				players[playerNum].isJoinlocked = false;
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
         * Print the map list
         * @param uint pageNum
         * @param uint mapCount
         * @param uint pagesPerPage
         * @param cString mapList
         * @return void
         */

        void printMapList(uint pageNum,uint mapCount, uint pagesPerPage, cString mapList )
        {
            uint numPages = mapCount/pagesPerPage + 1;
            if ( (pageNum > numPages) || (pageNum < 1)) {
                this.sendMessage ( "There are only " + numPages + ".\n" );
            }
            this.sendMessage( "Printing page " + pageNum + "/" + numPages + " : \n" );
            uint mapNumber = ( pageNum - 1 )*( pagesPerPage);
            uint i = 0 ;
                while ( ( i < pagesPerPage ) && (mapNumber < mapCount) )
                {
                    this.sendMessage ( S_COLOR_ORANGE + "#" + mapNumber + S_COLOR_WHITE +" : " + mapList.getToken(mapNumber) + "\n");
                    i++;
                    mapNumber++;
                }
        }

        /** Filter the map list
         * @param cString filter
         * @param cString mapList
         * @param uint mapCount
         * @return void
         */

        void filterMapList(cString filter, cString mapList, uint mapCount)
        {
            uint mapNumber = 0;
            uint filterLength = filter.len();
            this.sendMessage ( "Printing maps containing " + S_COLOR_ORANGE + filter + " :\n");

            while (mapNumber < mapCount)
            {
                cString mapName = mapList.getToken( mapNumber );
                uint mapNameLength = mapName.len();

                if ( mapNameLength >= filterLength )
                {
                    for ( uint k = 0 ; k < (mapNameLength - filterLength ) ; k++ )
                    {
                        if ( mapName.substr( k,filterLength ) == filter )
                        {
                            this.sendMessage ( S_COLOR_ORANGE + "#" + mapNumber + S_COLOR_WHITE +" : " + mapName + "\n");
                            break;
                        }
                    }
                }
                mapNumber++;
            }
        }
	/**
	 * Display the help to the player
	 * @return bool
	 */
	void displayAdminHelp()
	{
		cString help;
		help += S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n";
		help += S_COLOR_RED + "ADMIN HELP for Racesow " + gametype.getVersion() + "\n";
		help += S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n";
		help += S_COLOR_RED + "admin map     " + S_COLOR_YELLOW + "change to the given map immedeatly\n";
		help += S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n\n";
		//TODOSOW add missing commands
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
		help += S_COLOR_RED + "HELP for Racesow " + gametype.getVersion() + "\n";
		help += S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n";
		help += S_COLOR_RED + "help           " + S_COLOR_YELLOW + "display this help ;)\n";
		help += S_COLOR_RED + "top             " + S_COLOR_YELLOW + "display the top 30 times on this map\n";
                help += S_COLOR_RED + "maplist       " + S_COLOR_YELLOW + "display the maplist by pages\n";
                help += S_COLOR_RED + "mapfilter   " + S_COLOR_YELLOW + "list the maps containing a specific string.\n";
        help += S_COLOR_RED + "racerestart " + S_COLOR_YELLOW + "go back to the start-area whenever you want\n";
		help += S_COLOR_RED + "register      " +  S_COLOR_YELLOW + "register a new account on this server\n";

        if (rs_registrationDisabled.getBool()) {

            help += S_COLOR_YELLOW + "                   " + rs_registrationInfo.getString() + "\n";
        }

		help += S_COLOR_RED + "auth            " + S_COLOR_YELLOW + "authenticate with the server (alternatively you can use setu to save your login\n";
		help += "                  " + S_COLOR_YELLOW + rs_authField_Name.getString() +" AND "+ rs_authField_Pass.getString() +" OR solely "+ rs_authField_Token.getString() +" in your racesow autoexec.cfg)\n";
		help += S_COLOR_RED + "token            " + S_COLOR_YELLOW + "only when authenticated, shows your unique login token you may setu in your config\n";
        G_PrintMsg( this.client.getEnt(), help );

		help = S_COLOR_RED + "admin          " + S_COLOR_YELLOW + "more info with 'admin help'\n";
		help += S_COLOR_RED + "position save " + S_COLOR_YELLOW + "freestyle only, save the current poisition in the map\n";
		help += S_COLOR_RED + "position load " + S_COLOR_YELLOW + "freestyle only, load the last saved position in the map\n";
		help += S_COLOR_RED + "noclip       " + S_COLOR_YELLOW + "freestyle only, switch clip/noclip mode\n";
//        help += S_COLOR_RED + "weapondef   " + S_COLOR_YELLOW + "change the weapon values, more info with 'weapondef help'\n";
		help += S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n\n";
		G_PrintMsg( this.client.getEnt(), help );

		return true;
	}
}
