/**
 * Racesow_Player_Race
 *
 * @package Racesow
 * @subpackage Player
 * @version 0.5.1a
 * @global int numCheckpoints
 */
class Racesow_Player_Race
{
	/**
	 * Checkpoints of the race
	 * @var uint
	 */
    uint[] checkPoints;

	/**
	 * Leveltime when started race
	 * @var uint
	 */
    uint startTime;
	
	/**
	 *  Duration of the race
	 * @var uint
	 */
    uint stopTime;

	/**
	 * Difference to best race
	 */
	int delta;
	
	/**
	 * The last passed checkpoint in the race
	 * @var int
	 */
    int lastCheckPoint;
	
	/**
	 * The player who races
	 */
	Racesow_Player @player;
	
	/**
	 * Constructor
	 *
	 */
    Racesow_Player_Race()
    {
		this.reset();
    }

	/**
	 * Destructor
	 *
	 * This little friend will later look if the race was completed or aborted and conditionally add a database entry :)
	 */
    ~Racesow_Player_Race()
	{
	}
	
	/**
	 * Set the player
	 * @param Racesow_Player @player
	 * @return void
	 */
	void setPlayer( Racesow_Player @player )
	{
		@this.player = @player;
	}
	
	/**
	 * Cleanup any data collected from a preious race
	 * this is not so nice, better would be to use new instances
	 * @return void
	 */
	void reset()
	{
		this.delta = 0;
		this.stopTime = 0;
		this.checkPoints.resize( numCheckpoints );
		this.lastCheckPoint = 0;
		this.startTime = 0;
		
		for ( int i = 0; i < numCheckpoints; i++ )
		{
            this.checkPoints[i] = 0;
		}
	}

	/**
	 * Check if it's is currently beeing raced
	 * @return bool
	 */
	bool inRace()
	{
		if ( this.startTime != 0 && this.stopTime == 0 )
			return true;
		else
			return false;
	}
	
	/**
	 * Get the race time
	 * @return uint
	 */
	uint getTime()
	{
		return this.stopTime - this.startTime;
	}
	
	/**
	 * Get the race delta
	 * @return void
	 */
	int getDelta()
	{
		return this.delta;
	}	
	
	/**
	 * Get the race delta
	 * @return void
	 */
	Racesow_Player @getPlayer()
	{
		return @this.player;
	}
	
	uint getCheckPoint(uint id)
	{
		if ( id >= this.checkPoints.length() )
            return 0;
		
		return this.checkPoints[id];
	}	
	
	uint getStartTime()
	{
		return this.startTime;
	}
	
	/**
	 * Save a checkpoint in the race
	 * @param int id
	 * @return void
	 */
	void check(int id)
	{
		if ( this.checkPoints[id] != 0 ) // already past this checkPoint
            return;

        if ( this.startTime > levelTime ) // something is very wrong here
            return;

		this.checkPoints[id] = levelTime - this.startTime;
		
		cString str;
		//uint oldTime = this.player.bestCheckPoints[id]; // diff to own best
		uint oldTime = map.highScores[0].getCheckPoint(id); // diff to server best
		uint newTime =  this.checkPoints[id];
		
		bool noDelta = 0 == oldTime;
		
		if ( noDelta || newTime < oldTime )
        {
            this.delta = (noDelta ? 0 : oldTime - newTime);
			str = S_COLOR_GREEN + (noDelta ? "" : "-");
			
        }
		else if ( newTime == oldTime )
		{
		    this.delta = 0;
            str = S_COLOR_YELLOW + "+-";
		}
        else
        {
            this.delta = newTime - oldTime;
            str = S_COLOR_RED + "+";
        }

        G_CenterPrintMsg( this.player.getClient().getEnt(), "Current: " + TimeToString( newTime ) + "\n"
			+ ( noDelta ? "" : str + TimeToString( this.delta ) ) );

		map.handleCheckpoint(@this, id);

        this.lastCheckPoint++;
	}
	
	void triggerAward(cString text)
	{
		this.player.getClient().addAward(text);
	}
	
	/**
	 * Start the race timer
	 * @return void
	 */
	void start()
	{
		this.reset();
		this.startTime = levelTime;
	}
	
	/**
	 * Stop the race timer and check the result
	 * @return bool
	 */
	bool stop()
	{
        if ( this.startTime > levelTime ) // something is very wrong here
            return false;

		cString str;
        this.stopTime = levelTime;
		
		//uint oldTime = this.player.getBestTime(); // diff to own best
		uint oldTime = map.highScores[0].getTime(); // diff to server best
		uint newTime = this.getTime();
		
		bool noDelta = 0 == oldTime;
		
        if ( noDelta || newTime < oldTime )
        {
			this.delta = newTime;
            str = S_COLOR_GREEN + (noDelta ? "" : "-");
        }
		else if ( newTime == oldTime )
		{
		    this.delta = 0;
            str = S_COLOR_YELLOW + "+-";
		}
        else
        {
            this.delta = newTime - oldTime;
            str = S_COLOR_RED + "+";
        }

        G_CenterPrintMsg( this.player.getClient().getEnt(), "Time: " + TimeToString( newTime ) + "\n"
			+ ( noDelta ? "" : str + TimeToString( this.delta ) ) );
		
		map.handleFinishLine(@this);
		
		return true;
	}
}