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
	 * Did the player respawn on his own after finishing a race?
	 */
	bool isSpawned;

	/**
	 * Is the player still racing in the overtime?
	 */
	bool inOvertime;
	
	/**
	 * The time when the player started idling
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
}