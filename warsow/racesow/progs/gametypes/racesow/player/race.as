/**
 * Racesow_Player_Race
 *
 * @package Racesow
 * @subpackage Player
 * @version 0.5.6
 */
class Racesow_Player_Race : Racesow_Player_Implemented
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
	 * Duration of the race
	 * @var uint
	 */
    uint stopTime;
	
	/**
	 * Local time when race has finished
	 * @var uint64
	 */
	uint64 timeStamp;

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
	 * Check if it's is currently being raced
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
	 * See if the race was finished
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
	 * getTimeStamp
	 * @return uint64
	 */
	uint64 getTimeStamp()
	{
		return this.timeStamp;
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
		
		uint newTime =  this.checkPoints[id];
		uint serverBestTime = map.getHighScore(0).getCheckPoint(id);
		uint personalBestTime = this.player.getBestCheckPoint(id);
		bool noDelta = 0 == serverBestTime;
		
        G_CenterPrintMsg( this.player.getClient().getEnt(), "Current: " + TimeToString( newTime ) + "\n"
			+ ( noDelta ? "" : diffString( serverBestTime, newTime ) ) );

        if ( newTime < serverBestTime || serverBestTime == 0 )
        {
            this.player.getClient().addAward( S_COLOR_GREEN + "#" + (lastCheckPoint + 1) + " checkpoint record!" );
        }
        else if ( newTime < personalBestTime || personalBestTime == 0 )
        {
            this.player.getClient().addAward( S_COLOR_YELLOW + "#" + (lastCheckPoint + 1) + " checkpoint personal record!" );
        }

        this.player.sendMessage( S_COLOR_ORANGE + "#" + (lastCheckPoint +1) + ": "
                + S_COLOR_WHITE + TimeToString( newTime )
                + S_COLOR_ORANGE + "/" + S_COLOR_WHITE + diffString( personalBestTime, newTime )
                + S_COLOR_ORANGE + "/" + S_COLOR_WHITE + diffString( serverBestTime, newTime ) + "\n");
		
       this.lastCheckPoint++;
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

        if ( !this.inRace() )
            return false;

        this.stopTime = levelTime;
		this.timeStamp = localTime;
        
		return true;
	}
	
	/**
	 * Get race data as string
	 * @return cString
	 */
	cString toString()
	{
		cString raceString;
		raceString += "\"" + this.getTime() + "\" \"" + this.player.getName() + "\" \"" + localTime + "\" ";
		raceString += "\"" + this.checkPoints.length() + "\" ";
		for ( uint i = 0; i < this.checkPoints.length(); i++ )
			raceString += "\"" + this.getCheckPoint( i ) + "\" ";

		raceString += "\n";
	
		return raceString;
	}
}
