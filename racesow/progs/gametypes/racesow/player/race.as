/**
 * Racesow_Player_Race
 *
 * @package Racesow
 * @subpackage Player
 * @version 0.6.1
 */
class Racesow_Player_Race : Racesow_Player_Implemented
{
	/**
	 * Checkpoints of the race
	 * @var uint
	 */
    uint[] checkPoints;

    /**
     * The checkpoints of the race as a string
     */
    cString checkPointsString;

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
	 * Distance when started race
	 * @var uint64
	 */
    uint64 startDistance;

	/**
	 * Distance when stopped race
	 * @var uint64
	 */
    uint64 stopDistance;

	/**
	 * Local time when race has finished
	 * @var uint64
	 */
	uint64 timeStamp;

	/**
	 * Difference to best race
	 * @var int
	 */
	int delta;

	/**
	 * The last passed checkpoint in the race
	 * @var int
	 */
    int lastCheckPoint;

	/* Prejumped race flag
	 * @var bool
	 */
    bool prejumped;

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
		this.prejumped = false;

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
	 * Get the current race distance
	 * @return uint
	 */
	uint getCurrentDistance()
	{
	    if ( this.startDistance > 0 )
            return (this.player.distance - this.startDistance)/1000;
        else
            return 0;
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
	 * Get all the checkpoint times as a token string
	 * @return cString
	 */
	cString getCheckpoints()
	{
        cString checkpoints = "";
        for ( int i = 0; i < numCheckpoints; i++ )
        {
            checkpoints +=  this.checkPoints[i] + " ";
        }
        return checkpoints;
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
	    this.lastCheckPoint++;
		if ( this.checkPoints[id] != 0 ) // already past this checkPoint
            return;

        if ( this.startTime > levelTime ) // something is very wrong here
            return;

		this.checkPoints[id] = levelTime - this.startTime;

		uint newTime =  this.checkPoints[id];
		uint serverBestTime = map.getHighScore().getCheckPoint(id);
		uint personalBestTime = this.player.getBestCheckPoint(id);
		bool noDelta = 0 == serverBestTime;

        G_CenterPrintMsg( this.player.getClient().getEnt(), "Current: " + TimeToString( newTime ) + "\n"
			+ ( noDelta ? "" : diffString( serverBestTime, newTime ) ) );

        //print the checkpoint times to specs too
        cTeam @spectators = @G_GetTeam( TEAM_SPECTATOR );
        cEntity @other;
        for ( int i = 0; @spectators.ent( i ) != null; i++ )
        {
            @other = @spectators.ent( i );
            if ( @other.client != null && other.client.chaseActive )
            {
                if( other.client.chaseTarget == this.player.getClient().playerNum() + 1 )
                {
                    G_CenterPrintMsg( other.client.getEnt(), "Current: " + TimeToString( newTime ) + "\n"
                        + ( noDelta ? "" : diffString( serverBestTime, newTime ) ) );
                }
            }
        }

        if ( newTime < serverBestTime || serverBestTime == 0 )
        {
            this.player.sendAward( S_COLOR_GREEN + "#" + lastCheckPoint + " checkpoint record!" );
        }
        else if ( newTime < personalBestTime || personalBestTime == 0 )
        {
            this.player.sendAward( S_COLOR_YELLOW + "#" + lastCheckPoint + " checkpoint personal record!" );
        }

        this.checkPointsString += S_COLOR_ORANGE + "#" + lastCheckPoint + ": "
                + S_COLOR_WHITE + TimeToString( newTime )
                + S_COLOR_ORANGE + " Distance: " + S_COLOR_WHITE + this.getCurrentDistance()
                + S_COLOR_ORANGE + " Personal: " + S_COLOR_WHITE + diffString( personalBestTime, newTime )
                + S_COLOR_ORANGE + "/Server: " + S_COLOR_WHITE + diffString( serverBestTime, newTime ) + "\n";
	}

	/**
	 * Start the race timer
	 * @return void
	 */
	void start()
	{
		this.startTime = levelTime;
		this.startDistance = this.player.distance;
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
		this.stopDistance = this.player.distance;
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
