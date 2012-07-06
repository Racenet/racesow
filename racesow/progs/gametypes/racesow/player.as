/**
 * Racesow_Player
 *
 * @package Racesow
 * @version 0.6.2
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
	 * Is the player practising? has the player completed the map in practicemode?
	 * @var bool
	 */
	bool practicing;
	bool completedInPracticemode;

	/**
	 * What state is the player in: racing, practicing, prerace?
	 * @var String
	 */
	String state;

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
	 * Should we print welcome message to the player ?
	 */
	bool printWelcomeMessage;

    /**
	 * The player's best race
	 * @var uint
	 */
    uint bestRaceTime;

    /**
     * Overall number of started races on the current map
     */
    uint overallTries;

    /**
     * Number of started races on the current map for current session
     */
    uint tries;

    /**
     * Number of started races since last race
     */
    uint triesSinceLastRace;

    /**
     * Racing time before a race is actually finished
     */
    uint racingTimeSinceLastRace;

    /**
     * Racing time
     */
    uint racingTime;

    /**
     * Distance
     */
    uint64 distance;

    /**
     * Old Position
     */
    Vec3 oldPosition;

	/**
	 * Local time of the last top command (flood protection)
	 * @var uint
	 */
    uint topLastcmd;

    /**
     * Current session speed record
     */
    int highestSpeed;

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
	Racesow_Player_Race @lastRace;

	/**
	 * Controls the demo recording on the client
	 * @var Racesow_Player_ClientDemo
	 */
	Racesow_Player_ClientDemo @demo;

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
     * Stores all spectators of the player in a list ((int)id (int)ping)
     * @var String
     */
	String challengerList;

	/**
	 * Variables for the position function
	 */
	uint positionLastcmd; //flood protection
	bool positionSaved; //is a position saved?
	Vec3 positionOrigin; //stored origin
	Vec3 positionAngles; //stored angles
	int positionWeapon; //stored weapon

    /**
     * Determines if the player normally blocks others
     */
    bool Blocking;

	/**
	 * Constructor
	 *
	 */
    Racesow_Player()
    {
		@this.auth = Racesow_Player_Auth();
		@this.demo = Racesow_Player_ClientDemo();
        this.Blocking = false;
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
		this.practicing = false;
		this.completedInPracticemode = false;
		this.idleTime = 0;
		this.isSpawned = true;
		this.isJoinlocked = false;
        this.inOvertime = false;
		this.isVotemuted = false;
		this.wasTelekilled = false;
		this.onQuad = false;
		this.isWaitingForCommand = false;
		this.bestRaceTime = 0;
        this.resetAuth();
		this.auth.setPlayer(@this);
		this.demo.setPlayer(@this);
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
		this.tries = 0;
		this.overallTries = 0;
		this.racingTime = 0;
		this.racingTimeSinceLastRace = 0;
		this.challengerList = "";
		this.printWelcomeMessage = false;
		this.highestSpeed = 0;
		this.state = "";
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
        racesowAdapter.playerAppear(@this);
    }

	void disappear(String nickName, bool threaded)
    {
        racesowAdapter.playerDisappear(@this, nickName, threaded);
    }

    /**
     * Callback for a finished race
     * @return void
     */
    void raceCallback(uint allPoints, uint oldPoints, uint newPoints, uint oldTime, uint oldBestTime, uint newTime)
    {

        //G_PrintMsg( null, this.getName() + ": aP: "+ allPoints + ", oP: "+ oldPoints + ", nP: " + newPoints + ", oT: "+ oldTime + ", oBT: "+ oldBestTime + ", nT: " + newTime + "\n");
		uint bestTime;
        int earnedPoints;
		uint oldServerBestTime;
        bestTime = oldTime; // diff to own best
        //bestTime = oldBestTime // diff to server best

        oldServerBestTime = map.getHighScore().getTime();

        //print general info to player
        this.sendAward( S_COLOR_CYAN + "Race Finished!" );
        bool noDelta = 0 == bestTime;

        if ( @this.getClient() != null)
		{
            G_CenterPrintMsg( this.getClient().getEnt(),
                              "Time: " + TimeToString( newTime ) + "\n"
                              + ( noDelta ? "" : diffString( bestTime, newTime ) ) );

            this.sendMessage(S_COLOR_WHITE + "Race " + S_COLOR_ORANGE + "#"
                    + this.tries + S_COLOR_WHITE + " finished: "
                    + TimeToString( newTime)
                    + S_COLOR_ORANGE + " Distance: " + S_COLOR_WHITE + ((this.lastRace.stopDistance - this.lastRace.startDistance)/1000) // racing distance
                    + S_COLOR_ORANGE + " Personal: " + S_COLOR_WHITE + diffString(oldTime, newTime) // personal best
                    + S_COLOR_ORANGE + "/Server: " + S_COLOR_WHITE + diffString(oldServerBestTime, newTime) // server best
                    + S_COLOR_ORANGE + "/" + Capitalize(rs_networkName.get_string()) + ": " + S_COLOR_WHITE + diffString(oldBestTime, newTime) // database best
                    + "\n");
		}

        if ( this.lastRace.checkPointsString.len() > 0 )
            this.sendMessage( this.lastRace.checkPointsString );

        earnedPoints = newPoints - oldPoints;
        if (earnedPoints > 0)
        {
            String pointsAward =  S_COLOR_BLUE + "You earned "+ earnedPoints
                                   + ((earnedPoints > 1)? " points!" : " point!")
                                   + "\n";
            this.sendAward( pointsAward );
            this.sendMessage( pointsAward );
        }

        //personal record
        if ( oldTime == 0 || newTime < oldTime )
        {
            this.setBestTime(newTime);
            this.setBestCheckPointsFromRace(this.lastRace);
            this.sendAward( "Personal record!" );
        }

        //server record
        if ( oldServerBestTime == 0 || newTime < oldServerBestTime )
        {
            map.getHighScore().fromRace(this.lastRace);
            this.sendAward( S_COLOR_GREEN + "New server record!" );
            G_PrintMsg(null, this.getName() + " "
                             + S_COLOR_YELLOW + "made a new server record: "
                             + TimeToString( newTime ) + "\n");

            RS_ircSendMessage( this.getName().removeColorTokens()
                               + " made a new server record: "
                               + TimeToString( newTime ) );
        }

        //world record
        if ( ( oldBestTime == 0 || newTime < oldBestTime ) and ( newTime < oldTime ) )
        {
            this.sendAward( S_COLOR_GREEN + "New " + rs_networkName.get_string() + " record!" );
            G_PrintMsg(null, this.getName() + " "
                             + S_COLOR_YELLOW + "made a new "
                             + S_COLOR_GREEN  + rs_networkName.get_string()
                             + S_COLOR_YELLOW + " record: " + TimeToString( newTime ) + "\n");

            if ( mysqlConnected == 1)
            {
                this.sendMessage(S_COLOR_YELLOW
                                 + "Congratulations! You can now set a "
                                 + S_COLOR_WHITE + "oneliner"
                                 + S_COLOR_YELLOW +
                                 ". Careful though, only one try.\n");
            }
        }
    }

    void setLastRace(Racesow_Player_Race @race)
    {
        @this.lastRace = @race;
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
	 * @return String
	 */
	String getName()
	{
		if (@this.client != null)
        {
            return this.client.getName();
        }

        return "";
	}

	/**
	 * getAuth
	 * @return String
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
	 * Set player best checkpoints from a given race
	 *
	 * @param race The race from which to take the checkpoints
	 * @return true
	 */
	bool setBestCheckPointsFromRace(Racesow_Player_Race @race)
	{
	    for ( int i = 0; i < numCheckpoints; i++)
	    {
	        this.bestCheckPoints[i] = race.getCheckPoint(i);
	    }
	    return true;
	}

	/**
	 * Player spawn event
	 * @return void
	 */
	void onSpawn()
	{
	    this.isSpawned = true;

	    if ( this.demo.isStopping() )
	    	this.demo.stopNow();
	    else if ( this.demo.isRecording() )
	    	this.demo.cancel();

	    this.demo.start();
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
     * Checks if the player is blocking others.
     * @return bool
     */
    bool isBlocking()
    {
        return this.Blocking;
    }

	/**
	 * Get the player current speed
	 */
	int getSpeed()
	{
	    Vec3 globalSpeed = this.getClient().getEnt().getVelocity();
	    Vec3 horizontalSpeed = Vec3(globalSpeed.x, globalSpeed.y, 0);
	    return horizontalSpeed.length();
	}

	/**
	 * Get the state of the player;
	 * @return String
	 */
	String getState()
	{
		if ( this.practicing )
			this.state = "^5practicing";
		else if ( this.isRacing() )
			this.state = "^2racing";
		else
			this.state = "^3prerace";

		return state;
	}
	/**
	 * crossStartLine
	 * @return void
	 */
    void touchStartTimer()
    {
        if( !this.isSpawned )
            return;

		if ( this.isRacing() )
            return;

		if ( this.practicing )
			return;

		@this.race = Racesow_Player_Race();
		this.race.setPlayer(@this);
		this.race.start();
        this.tries++;
        this.triesSinceLastRace++;
        int tries = this.overallTries+this.tries;

		this.race.prejumped=RS_QueryPjState(this.getClient().playerNum());
		if (this.race.prejumped)
		{
		    this.sendAward(S_COLOR_RED + "Prejumped!");
		}
    }

	/**
	 * touchCheckPoint
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

    void touchStopTimer_gametype()
    {
        if ( this.bestRaceTime == 0 || this.race.getTime() < this.bestRaceTime )
        {
            this.getClient().stats.setScore(this.race.getTime()/100);
        }
        racesowAdapter.raceFinish(@this.race);
    }

	/**
	 * touchStopTimer
	 * @return void
	 */
    void touchStopTimer()
    {
		if ( this.practicing && !this.completedInPracticemode )
		{
			this.sendAward( S_COLOR_CYAN + "You completed the map in practicemode, no time was set" );

			this.isSpawned = false;
			this.completedInPracticemode = true;
			// set up for respawning the player with a delay
			cEntity @practiceRespawner = G_SpawnEntity( "practice_respawner" );
			@practiceRespawner.think = practice_respawner_think; //FIXME: Workaround because the practice_respawner function isn't called
			practiceRespawner.nextThink = levelTime + 3000;
			practiceRespawner.count = client.playerNum();
		}

		// when the race can not be finished something is very wrong, maybe small penis playing, or practicemode is enabled.
		if ( @this.race == null || !this.race.stop() )
            return;

		this.demo.stop( this.race.getTime() );

        this.setLastRace(@this.race);

        touchStopTimer_gametype();

		this.isSpawned = false;
		this.racingTime += this.race.getTime();
		this.racingTimeSinceLastRace += this.race.getTime();
		this.triesSinceLastRace = 0;
		this.racingTimeSinceLastRace = 0;
		@this.race = null;

    // set up for respawning the player with a delay
    cEntity @respawner = G_SpawnEntity( "race_respawner" );
    @respawner.think = race_respawner_think; //FIXME: Workaround because the race_respawner function isn't called
    respawner.nextThink = levelTime + 3000;
    respawner.count = client.playerNum();
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
            this.client.team = TEAM_PLAYERS;
            this.client.respawn( false );
        }
    }

	/**
	 * restartingRace
	 * @return void
	 */
    void restartingRace()
    {
  		this.isSpawned = true;

  		if ( this.practicing && this.positionSaved )
  		{
  			this.teleport( this.positionOrigin, this.positionAngles, false, false );
  			this.client.selectWeapon( this.positionWeapon );
  			if ( this.completedInPracticemode )
  				this.completedInPracticemode = false;

  		} else if ( this.isRacing() )
  		{
  			this.racingTime += this.race.getCurrentTime();
  			this.racingTimeSinceLastRace += this.race.getCurrentTime();
  			this.sendMessage( this.race.checkPointsString );
  		}

  		@this.race = null;
  		//remove all projectiles.
  		if( @this.client.getEnt() != null )
  			removeProjectiles( this.client.getEnt() );
  		if ( !this.practicing )
  			RS_ResetPjState(this.getClient().playerNum());
    }

	/**
	 * cancelRace
	 * @return void
	 */
    void cancelRace()
    {
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
	 * advanceDistance, this must be called once per frame
	 * @return void
	 */
	void advanceDistance()
	{
        Vec3 position = this.getClient().getEnt().getOrigin();
        position.z = 0;
        this.distance += ( position.distance( this.oldPosition ) * 1000 );
        this.oldPosition = position;
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
	    cEntity@ ent = this.client.getEnt();
		if( ent.moveType == MOVETYPE_NOCLIP )
		{
		    Vec3 mins, maxs;
		    client.getEnt().getSize( mins, maxs );
		    cTrace tr;
            if( tr.doTrace( this.client.getEnt().getOrigin(), mins, maxs,
                    this.client.getEnt().getOrigin(), 0, MASK_PLAYERSOLID ))
            {
                //don't allow players to end noclip inside others or the world
                this.sendMessage( S_COLOR_WHITE + "WARNING: can't switch noclip back when being in something solid.\n" );
                return false;
            }
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
        if( this.onQuad )
        {
            this.client.inventorySetCount( POWERUP_QUAD, 0 );
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
	bool teleport( Vec3 origin, Vec3 angles, bool keepVelocity, bool kill )
	{
		cEntity@ ent = @this.client.getEnt();
		if( @ent == null )
			return false;
		if( ent.team != TEAM_SPECTATOR )
		{
			Vec3 mins, maxs;
			ent.getSize(mins, maxs);
			cTrace tr;
			if(	this.isBlocking() && tr.doTrace( origin, mins, maxs, origin, 0, MASK_PLAYERSOLID ))
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
						other.sustainDamage( @other, null, Vec3(0,0,0), 9999, 0, 0, MOD_TELEFRAG );
						//spawn a gravestone to store the postition
						cEntity @gravestone = @G_SpawnEntity( "gravestone" );
						// copy client position
						gravestone.setOrigin( other.getOrigin() + Vec3( 0.0f, 0.0f, 50.0f ) );
						Racesow_GetPlayerByClient( other.client ).setupTelekilled( @gravestone );
					}

				}
			}
		}
		if( ent.team != TEAM_SPECTATOR )
            ent.teleportEffect( true );
		if(!keepVelocity)
			ent.setVelocity( Vec3(0,0,0) );
		ent.setOrigin( origin );
		ent.setAngles( angles );
		if( ent.team != TEAM_SPECTATOR )
			ent.teleportEffect( false );
		return true;
	}



	/**
	 * position Commands
	 * @return bool
	 */
	bool positionSave()
	{
		this.positionSaved = true;
		cEntity@ ent = @this.client.getEnt();
		if( @ent == null )
			return false;
		this.positionOrigin = ent.getOrigin();
		this.positionAngles = ent.getAngles();
		if ( ent.moveType == MOVETYPE_NOCLIP )
			this.positionWeapon = this.noclipWeapon;
		else
			this.positionWeapon = client.weapon;
		this.positionLastcmd = realTime;
		return true;
	}

	bool positionLoad()
	{
		if(!this.positionSaved)
			return false;
		if( this.teleport( this.positionOrigin, this.positionAngles, false, false ) )
			this.client.selectWeapon( this.positionWeapon );
		this.positionLastcmd = realTime;
		return true;
	}

	bool positionSet( Vec3 origin, Vec3 angles )
	{
		this.positionLastcmd = realTime;
		return this.teleport( origin, angles, false, false );
	}

	bool positionStore( int id, String name )
	{
		Vec3 position = client.getEnt().getOrigin();
		Vec3 angles = client.getEnt().getAngles();
		//position set <x> <y> <z> <pitch> <yaw>
		this.client.execGameCommand("cmd seta storedposition_" + id
				+ " \"" + name + " "
				+ position.x + " " + position.y + " " + position.z + " "
				+ angles.x + " " + angles.y + "\";writeconfig config.cfg");
		this.positionLastcmd = realTime;
		return true;
	}

	bool positionRestore( int id )
	{
		G_CmdExecute( "cvarcheck " + this.client.playerNum()
				+ " storedposition_" + id );
		this.positionLastcmd = realTime;
		return true;
	}

	bool positionStoredlist( int limit )
	{
		this.sendMessage( S_COLOR_WHITE + "###\n#List of stored positions\n###\n" );
		int i;
		for( i = 0; i < limit; i++ )
		{
			this.client.execGameCommand("cmd  echo ~~~;echo id#" + i
					+ ";storedposition_" + i +";echo ~~~;" );
		}
		this.positionLastcmd = realTime;
		return true;
	}




//	/**
//	 * position Command
//	 * @return bool
//	 */
//	bool position( String argsString )
//	{
//		if( this.positionLastcmd + 500 > realTime )
//			return false;
//		this.positionLastcmd = realTime;
//
//		String action = argsString.getToken( 0 );
//
//		if( action == "save" )
//		{
//			this.positionSaved = true;
//			cEntity@ ent = @this.client.getEnt();
//			if( @ent == null )
//				return false;
//			this.positionOrigin = ent.getOrigin();
//			this.positionAngles = ent.getAngles();
//			if ( ent.moveType == MOVETYPE_NOCLIP )
//				this.positionWeapon = this.noclipWeapon;
//			else
//			this.positionWeapon = client.weapon;
//		}
//		else if( action == "load" )
//		{
//			if(!this.positionSaved)
//				return false;
//			if( this.teleport( this.positionOrigin, this.positionAngles, false, false ) )
//				this.client.selectWeapon( this.positionWeapon );
//			return true;
//		}
//		else if( action == "set" && argsString.getToken( 5 ) != "" )
//		{
//			Vec3 origin, angles;
//
//			origin.x = argsString.getToken( 1 ).toFloat();
//			origin.y = argsString.getToken( 2 ).toFloat();
//			origin.z = argsString.getToken( 3 ).toFloat();
//			angles.x = argsString.getToken( 4 ).toFloat();
//			angles.y = argsString.getToken( 5 ).toFloat();
//
//			return this.teleport( origin, angles, false, false );
//		}
//		else if( action == "store" && argsString.getToken( 2 ) != "" )
//		{
//			Vec3 position = client.getEnt().getOrigin();
//			Vec3 angles = client.getEnt().getAngles();
//			//position set <x> <y> <z> <pitch> <yaw>
//			this.client.execGameCommand("cmd seta storedposition_" + argsString.getToken(1)
//					+ " \"" +  argsString.getToken(2) + " "
//					+ position.x + " " + position.y + " " + position.z + " "
//					+ angles.x + " " + angles.y + "\";writeconfig config.cfg");
//		}
//		else if( action == "restore" && argsString.getToken( 1 ) != "" )
//		{
//			G_CmdExecute( "cvarcheck " + this.client.playerNum()
//					+ " storedposition_" + argsString.getToken(1) );
//		}
//		else if( action == "storedlist" && argsString.getToken( 1 ) != "" )
//		{
//			if( argsString.getToken(1).toInt() > 50 )
//			{
//				this.sendMessage( S_COLOR_WHITE + "You can only list the 50 the most\n" );
//				return false;
//			}
//			this.sendMessage( S_COLOR_WHITE + "###\n#List of stored positions\n###\n" );
//			int i;
//			for( i = 0; i < argsString.getToken(1).toInt(); i++ )
//			{
//				this.client.execGameCommand("cmd  echo ~~~;echo id#" + i
//						+ ";storedposition_" + i +";echo ~~~;" );
//			}
//		}
//		else
//		{
//			cEntity@ ent = @this.client.getEnt();
//			if( @ent == null )
//				return false;
//			String msg;
//			msg = "Usage:\nposition save - Save current position\n";
//			msg += "position load - Teleport to saved position\n";
//			msg += "position set <x> <y> <z> <pitch> <yaw> - Teleport to specified position\n";
//			msg += "position store <id> <name> - Store a position for another session\n";
//			msg += "position restore <id> - Restore a stored position from another session\n";
//			msg += "position storedlist <limit> - Sends you a list of your stored positions\n";
//			msg += "Current position: " + " " + ent.getOrigin().x + " " + ent.getOrigin().y + " " +
//		ent.getOrigin().z + " " + ent.getAngles().x + " " + ent.getAngles().y + "\n";
//			this.sendMessage( msg );
//		}
//
//		return true;
//	}

	/**
	 * Send a message to console of the player
	 * @param String message
	 * @return void
	 */
	void sendMessage( String message )
	{
		if (@this.client == null)
            return;

        // just send to original func
		G_PrintMsg( this.client.getEnt(), message );

		// maybe log messages for some reason to figure out ;)
	}

   /**
     * Send an unlogged award to the player
     * @param String message
     * @return void
     */
    void sendAward( String message )
    {
        if (@this.client == null)
            return;
        this.client.execGameCommand( "aw \"" + message + "\"" );
        //print the checkpoint times to specs too
        cTeam @spectators = @G_GetTeam( TEAM_SPECTATOR );
        cEntity @other;
        for ( int i = 0; @spectators.ent( i ) != null; i++ )
        {
            @other = @spectators.ent( i );
            if ( @other.client != null && other.client.chaseActive )
            {
                if( other.client.chaseTarget == this.client.playerNum() + 1 )
                {
                    other.client.execGameCommand( "aw \"" + message + "\"" );
                }
            }
        }
    }

    /**
     * Send a message to console of the player
     * when the message is too long, split it in several parts
     * to avoid print buffer overflow
     * @param String message
     * @return void
     */
    void sendLongMessage( String message )
    {
        if (@this.client == null)
            return;

        const uint maxsize = 1000;
        uint partsNumber = message.length()/maxsize;

        if ( partsNumber*maxsize < message.length() )//compute the ceil instead of floor
            partsNumber++;

        for ( uint i = 0; i < partsNumber; i++ )
        {
            G_PrintMsg( this.client.getEnt(), message.substr(i*maxsize,maxsize) );
        }

        // maybe log messages for some reason to figure out ;)
    }

    /**
     * Send an error message with red warning.
     * @param String message
     * @return void
     */
    void sendErrorMessage( String message )
    {
        if (@this.client == null)
            return;

        G_PrintMsg( this.client.getEnt(), S_COLOR_RED + "Error: " + S_COLOR_WHITE
                    + message +"\n");
    }

    // joki: make this a global function and move to helpers.as, IF THIS FUNCTION IS USED AT ALL
	/**
	* Send a message to another player's console
	* @param String message, cClient @client
	* @return void
	*/
	void sendMessage( String message, cClient @client )
	{
		G_PrintMsg( client.getEnt(), message );
	}


	// joki: is this used???
	/**
	 * Send a message to another player
	 * @param String argString, cClient @client
	 * @return bool
	 */
	bool privSay( String message, cClient @target )
	{
	    this.sendMessage( S_COLOR_RED + "(Private message to " + S_COLOR_WHITE + target.getName()
	            + S_COLOR_RED + " ) " + S_COLOR_WHITE + ": " + message + "\n");
	    sendMessage( S_COLOR_RED + "(Private message from " + S_COLOR_WHITE + client.getName()
	            + S_COLOR_RED + " ) " + S_COLOR_WHITE + ": " + message + "\n", @target );
		return true;
	}

	/**
	 * Kick the player and leave a message for everyone
	 * @param String message
	 * @return void
	 */
	void kick( String message )
	{
        int playerNum = this.client.playerNum();
        if( message.length() > 0)
            G_PrintMsg( null, S_COLOR_RED + "Kicked "+ this.getName() + S_COLOR_RED + " Reason: " + message + "\n" );
        this.reset();
        G_CmdExecute( "kick " + playerNum );
	}

	/**
	 * Remove the player and leave a message for everyone
	 * @param String message
	 * @return void
	 */
	void remove( String message )
	{
		if( message.length() > 0)
			G_PrintMsg( null, S_COLOR_RED + message + "\n" );
		this.client.team = TEAM_SPECTATOR;
		this.client.respawn( true ); // true means ghost
	}

   /**
     * Ban the player
     * @param String message
     * @return void
     */
    void kickban( String message )
    {
        String ip = this.client.getUserInfoKey( "ip" );
        this.reset();
        G_CmdExecute( "addip " + ip + " 15;kick " + this.client.playerNum() );
    }

    /**
     * Move the player to spec and leave a message to him
     * @param String message
     * @return void
     */
    void moveToSpec( String message )
    {
        this.client.team = TEAM_SPECTATOR;
        this.client.respawn( true ); // true means ghost
        this.sendMessage( message );
    }

    //TODO: move this to Command_AmmoSwitch
	/**
	 * Switch player ammo between strong/weak
	 * @param cClient @client
	 * @return bool
	 */
	bool ammoSwitch(  )
	{
		if ( racesowGametype.ammoSwitchAllowed() || g_allowammoswitch.get_boolean() )
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
			this.sendMessage( S_COLOR_RED + "Ammoswitch is disabled.\n" );
		}
		return true;
	}

	/**
	 * Chrono time
	 * @return uint
	 */
	uint chronoTime()
	{
		return this.chronoStartTime;
	}

//
//    /**
//     * Execute an admin command
//     * @param String &cmdString
//     * @return bool
//     */
//    bool adminCommand( String &cmdString )
//    {
//        bool showNotification = false;
//        String command = cmdString.getToken( 0 );
//
//        //Commented out for release - Per server admin/authmasks will be finished when the new http DB-Interaction is done
//        // add command - adds a new admin (sets all permissions except RACESOW_AUTH_SETPERMISSION)
//        // delete command - deletes admin rights for the given player
//        /*if ( command == "add" || command == "delete" )
//        {
//            if( !this.auth.allow( RACESOW_AUTH_ADMIN | RACESOW_AUTH_SETPERMISSION ) )
//            {
//                this.sendErrorMessage( "You are not permitted to execute the commmand 'admin "+ cmdString);
//                return false;
//            }
//
//            if( cmdString.getToken( 1 ) == "" )
//            {
//                this.client.execGameCommand("cmd players");
//                showNotification = false;
//                return false;
//            }
//
//            Racesow_Player @player = @Racesow_GetPlayerByNumber( cmdString.getToken( 1 ).toInt() );
//            if (@player == null )
//                return false;
//
//            //Set authmask
//            if( command == "add" )
//                this.sendErrorMessage("added");
//                //player.setAuthmask( RACESOW_AUTH_ADMIN );
//            else
//                this.sendErrorMessage("deleted");
//                //player.setAuthmask( RACESOW_AUTH_REGISTERED );
//        }
//
//        // setpermission command - sets/unsets the given permission for the given player
//        else if( command == "setpermission" )
//        {
//            uint permission;
//            if( !this.auth.allow( RACESOW_AUTH_ADMIN | RACESOW_AUTH_SETPERMISSION ) )
//            {
//                this.sendErrorMessage( "You are not permitted to execute the commmand 'admin "+ cmdString);
//                return false;
//            }
//
//            if( cmdString.getToken( 1 ) == "" )
//            {
//                this.sendErrorMessage( "Usage: admin setpermission <playernum> <permission> <enable/disable>" );
//                this.client.execGameCommand("cmd players");
//                showNotification = false;
//                return false;
//            }
//
//            if( cmdString.getToken( 2 ) == "" )
//            {
//                //show list of permissions: map, mute, kick, timelimit, restart, setpermission
//                this.sendErrorMessage( "No permission specified. Available permissions:\n map, mute, kick, timelimit, restart, setpermission" );
//                return false;
//            }
//
//            if( cmdString.getToken( 3 ) == "" || cmdString.getToken( 3 ).toInt() > 1 )
//            {
//                //show: 1 to enable 0 to disable current: <enabled/disabled>
//                this.sendErrorMessage( "1 to enable permission 0 to disable permission" );
//                return false;
//            }
//
//            if( cmdString.getToken( 2 ) == "map" )
//                permission = RACESOW_AUTH_MAP;
//            else if( cmdString.getToken( 2 ) == "mute" )
//                permission = RACESOW_AUTH_MUTE;
//            else if( cmdString.getToken( 2 ) == "kick" )
//                permission = RACESOW_AUTH_KICK;
//            else if( cmdString.getToken( 2 ) == "timelimit" )
//                permission = RACESOW_AUTH_TIMELIMIT;
//            else if( cmdString.getToken( 2 ) == "restart" )
//                permission = RACESOW_AUTH_RESTART;
//            else if( cmdString.getToken( 2 ) == "setpermission" )
//                permission = RACESOW_AUTH_SETPERMISSION;
//            else
//                return false;
//
//            Racesow_Player @player = @Racesow_GetPlayerByNumber( cmdString.getToken( 1 ).toInt() );
//            if (@player == null )
//                return false;
//
//            if( cmdString.getToken( 3 ).toInt() == 1 )
//                this.sendErrorMessage( cmdString.getToken( 2 ) + "enabled" );
//                //player.setAuthmask( player.authmask | permission );
//            else
//                this.sendErrorMessage( cmdString.getToken( 2 ) + "disabled" );
//                //player.setAuthmask( player.authmask & ~permission );
//        }*/
//
//        // map command
//        if ( command == "map" )
//        {
//            if ( !this.auth.allow( RACESOW_AUTH_MAP ) )
//            {
//                this.sendErrorMessage( "You are not permitted to execute the command 'admin "+ cmdString);
//                return false;
//            }
//
//            String mapName = cmdString.getToken( 1 );
//            if ( mapName == "" )
//            {
//                this.sendErrorMessage( "No map name given" );
//                return false;
//            }
//
//            G_CmdExecute( "gamemap " + mapName + "\n" );
//            showNotification = true;
//        }
//
//        // update maplist
//        else if ( command == "updateml" )
//        {
//            if ( !this.auth.allow(RACESOW_AUTH_ADMIN) )
//            {
//                this.sendErrorMessage( "You are not permitted to execute the command 'admin "+ cmdString);
//                return false;
//            }
//            RS_UpdateMapList( this.client.playerNum() );
//            showNotification = true;
//        }
//
//        // restart command
//        else if ( command == "restart" )
//        {
//            if ( !this.auth.allow( RACESOW_AUTH_MAP ) )
//            {
//                this.sendErrorMessage( "You are not permitted to execute the command 'admin "+ cmdString);
//                return false;
//            }
//            G_CmdExecute("match restart\n");
//            showNotification = true;
//        }
//
//        // extend_time command
//        else if ( command == "extend_time" )
//        {
//            if ( !this.auth.allow( RACESOW_AUTH_MAP ) )
//            {
//                this.sendErrorMessage( "You are not permitted to execute the command 'admin "+ cmdString);
//                return false;
//            }
//            if( g_timelimit.getInteger() <= 0 )
//            {
//                this.sendErrorMessage( "This command is only available for timelimits.\n");
//                return false;
//            }
//            g_timelimit.set(g_timelimit.getInteger() + g_extendtime.getInteger());
//
//            map.cancelOvertime();
//            for ( int i = 0; i < maxClients; i++ )
//            {
//                racesowGametype.players[i].cancelOvertime();
//            }
//            showNotification = true;
//        }
//
//        // votemute, mute commands (RACESOW_AUTH_MUTE)
//        else if ( command == "mute" || command == "unmute" || command == "vmute" ||
//                  command == "vunmute" || command == "votemute" || command == "unvotemute" )
//        {
//            if ( !this.auth.allow( RACESOW_AUTH_MUTE ) )
//            {
//                this.sendErrorMessage( "You are not permitted to execute the command 'admin "+ cmdString );
//                return false;
//            }
//            if( cmdString.getToken( 1 ) == "" )
//            {
//                this.client.execGameCommand("cmd players");
//                showNotification = false;
//                return false;
//            }
//            Racesow_Player @player = @Racesow_GetPlayerByNumber( cmdString.getToken( 1 ).toInt() );
//            if (@player == null )
//                return false;
//
//            if( command == "votemute" )
//                player.isVotemuted = true;
//            else if( command == "unvotemute" )
//                player.isVotemuted = false;
//            else if( command == "mute" )
//                player.client.muted |= 1;
//            else if( command == "unmute" )
//                player.client.muted &= ~1;
//            else if( command == "vmute" )
//                player.client.muted |= 2;
//            else if( command == "vunmute" )
//                player.client.muted &= ~2;
//            showNotification = true;
//        }
//
//        // kick, remove, joinlock  commands (RACESOW_AUTH_KICK)
//        else if ( command == "remove"|| command == "kick" || command == "joinlock" || command == "joinunlock" )
//        {
//            if ( !this.auth.allow( RACESOW_AUTH_KICK ) )
//            {
//                this.sendErrorMessage( "You are not permitted to execute the command 'admin "+ cmdString );
//                return false;
//            }
//            if( cmdString.getToken( 1 ) == "" )
//            {
//                this.client.execGameCommand("cmd players");
//                showNotification = false;
//                return false;
//            }
//            Racesow_Player @player = @Racesow_GetPlayerByNumber( cmdString.getToken( 1 ).toInt() );
//            if (@player == null )
//                return false;
//
//            if( command == "kick" )
//                player.kick("");
//            else if( command == "remove" )
//                player.remove("");
//            else if( command == "joinlock" )
//                player.isJoinlocked = true;
//            else if( command == "joinunlock" )
//                player.isJoinlocked = false;
//            showNotification = true;
//        }
//
//        // cancelvote command
//        else if ( command == "cancelvote" )
//        {
//            if ( !this.auth.allow( RACESOW_AUTH_MAP ) )
//            {
//                this.sendErrorMessage("You are not permitted to execute the command 'admin "+ cmdString);
//                return false;
//            }
//            RS_cancelvote();
//            showNotification = true;
//        }
//
//
//        // ban
//        else if ( command == "kickban" )
//        {
//            if ( !this.auth.allow(RACESOW_AUTH_ADMIN) )
//            {
//                this.sendErrorMessage( "You are not permitted to execute the command 'admin "+ cmdString );
//                return false;
//            }
//            if( cmdString.getToken( 1 ) == "" )
//            {
//                this.client.execGameCommand("cmd players");
//                showNotification = false;
//                return false;
//            }
//            Racesow_Player @player = @Racesow_GetPlayerByNumber( cmdString.getToken( 1 ).toInt() );
//            if (@player != null )
//                player.kickban("");
//        }
//
//        else
//        {
//            this.sendErrorMessage("The command 'admin " + cmdString + "' does not exist" );
//            return false;
//        }
//
//        if ( showNotification )
//        {
//            G_PrintMsg( null, S_COLOR_WHITE + this.getName() + S_COLOR_GREEN
//                        + " executed command '"+ cmdString + "'\n" );
//        }
//        return true;
//    }
}
