/**
 * Racesow_Player_Race
 *
 * @package Racesow
 * @subpackage Player
 * @version 0.5.1b
 * @author soh-zolex <zolex@warsow-race.net>
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
	 * Destructor
	 *
	 * This little friend will later look if the race was completed or aborted and conditionally add a database entry :)
	 */
    ~Racesow_Player_Race()
	{
		if ( this.isFinished() )
		{
			map.getStatsHandler().addRace(@this);
		}
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
	 * See if teh race was finished
	 * @return bool
	 */
	bool isFinished()
	{
		return (this.stopTime != 0);
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
	
	/**
	 * getCheckPoint
	 * @param uint id
	 * @return uint
	 */
	uint getCheckPoint(uint id)
	{
		if ( id >= this.checkPoints.length() )
            return 0;
		
		return this.checkPoints[id];
	}	
	
	/**
	 * getStartTime
	 * @return uint
	 */
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
		uint newTime =  this.checkPoints[id];
		//uint bestTime = this.player.bestCheckPoints[id]; // diff to own best
		uint bestTime = map.getStatsHandler().getHighScore(0).getCheckPoint(id); // diff to server best
		uint personalBestTime = this.player.getBestCheckPoint(id);
		
		bool noDelta = 0 == bestTime;
		
		if ( noDelta || newTime < bestTime )
        {
            this.delta = (noDelta ? 0 : bestTime - newTime);
			str = S_COLOR_GREEN + (noDelta ? "" : "-");
			
        }
		else if ( newTime == bestTime )
		{
		    this.delta = 0;
            str = S_COLOR_YELLOW + "+-";
		}
        else
        {
            this.delta = newTime - bestTime;
            str = S_COLOR_RED + "+";
        }

        G_CenterPrintMsg( this.player.getClient().getEnt(), "Current: " + TimeToString( newTime ) + "\n"
			+ ( noDelta ? "" : str + TimeToString( this.delta ) ) );
		
        if ( newTime < bestTime || bestTime == 0 )
        {
			if ( map.getStatsHandler().getHighScore(0).setCheckPoint(id, newTime) && this.player.setBestCheckPoint(id, newTime) )
			{
				this.triggerAward( S_COLOR_GREEN + (id + 1) + ". checkpoint record!" );
				G_Print(this.player.getName() + " " + S_COLOR_WHITE + "made a new "
					+ (id + 1) + ". checkpoint record: " + TimeToString( newTime ) + "\n" );
			}
        }
        else if ( newTime < personalBestTime || personalBestTime == 0 )
        {
            if ( this.player.setBestCheckPoint(id, newTime) )
				this.triggerAward( S_COLOR_YELLOW + (id + 1) + ". checkpoint personal record!" );
        }

        this.lastCheckPoint++;
	}
	
	/**
	 * triggerAward
	 * @param cString text
	 * @return void
	 */
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
		this.player.inOvertime = false;
		
		uint newTime = this.getTime();
		//uint bestTime = this.player.getBestTime(); // diff to own best
		uint bestTime = map.getStatsHandler().getHighScore(0).getTime(); // diff to server best
		uint personalBestTime = this.player.getBestTime();
		
		bool noDelta = 0 == bestTime;
		
        if ( noDelta || newTime < bestTime )
        {
			this.delta = newTime;
            str = S_COLOR_GREEN + (noDelta ? "" : "-");
        }
		else if ( newTime == bestTime )
		{
		    this.delta = 0;
            str = S_COLOR_YELLOW + "+-";
		}
        else
        {
            this.delta = newTime - bestTime;
            str = S_COLOR_RED + "+";
        }

        G_CenterPrintMsg( this.player.getClient().getEnt(), "Time: " + TimeToString( newTime ) + "\n"
			+ ( noDelta ? "" : str + TimeToString( this.delta ) ) );
	
		this.triggerAward( S_COLOR_CYAN + "Race Finished!" );
		G_PrintMsg(this.player.getClient().getEnt(), S_COLOR_WHITE
			+ "race finished: " + TimeToString( newTime ) + "\n");
		
        if ( newTime < bestTime || bestTime == 0 )
        {
            player.setBestTime(newTime);
			this.triggerAward( S_COLOR_GREEN + "New server record!" );
			G_Print(player.getName() + " " + S_COLOR_YELLOW
				+ "made a new server record: " + TimeToString( newTime ) + "\n");
        }
        else if ( newTime < personalBestTime || personalBestTime == 0 )
        {
            player.setBestTime(newTime);
			this.triggerAward( "Personal record!" );
        }
		
		return true;
	}
}