/**
 * Racesow_Player
 *
 * @package Racesow
 * @version 0.5.1a
 * @global int levelTime
 */
class Racesow_Player
{
	/**
	 * Did the player respawn on his own after finishing a race?
	 */
	bool isSpawned;

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
		@this.race = Racesow_Player_Race();
		this.race.setPlayer(@this);
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
		this.race.reset();
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
	
	cString getName()
	{
		return this.client.getName();
	}	
	
	uint getBestCheckPoint(uint id)
	{
		if ( id >= this.bestCheckPoints.length() )
			return 0;
			
		return this.bestCheckPoints[id];
	}	
	
	bool setBestCheckPoint(uint id, uint time)
	{
		if ( id >= this.bestCheckPoints.length() && time < this.bestCheckPoints[id])
			return false;
			
		this.bestCheckPoints[id] = time;
		return true;
	}
	
	/**
	 * Player spawn event
	 *
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
	 * cancelRace
	 * @return void
	 */
    void cancelRace()
    {
		this.isSpawned = true;
        this.race.reset();
		this.race.setPlayer(this);
    }
	
	uint getCheckPoint(uint id)
	{
		if (id >= this.bestCheckPoints.length())
			return 0;
	
		return this.bestCheckPoints[id];
	}
}