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

		cString str;
		this.checkPoints[id] = levelTime - this.startTime;
		
		//uint oldTime = this.player.bestCheckPoints[id]; // diff to own best
		uint oldTime = levelRecords[0].checkPoints[id]; // diff to server best
		uint newTime =  this.checkPoints[id];
		
		bool noDelta = 0 == oldTime;
		
		if ( noDelta || newTime < oldTime )
        {
			this.player.bestCheckPoints[id] = newTime;
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

        G_CenterPrintMsg( this.player.client.getEnt(), "Current: " + TimeToString( newTime ) + "\n"
			+ ( noDelta ? "" : str + TimeToString( this.delta ) ) );

        // if beating the checkpoint record on this sector give an award
        if ( newTime < levelRecords[0].checkPoints[id] ||levelRecords[0].checkPoints[id] == 0 )
        {
            this.player.client.addAward( (this.lastCheckPoint + 1) + ". checkpoint record!" );
			G_Print(this.player.client.getName() + " " + S_COLOR_YELLOW + "made a new "
				+ (this.lastCheckPoint + 1) + ". checkpoint record: " + TimeToString( newTime ) + "\n" );
			levelRecords[0].checkPoints[id] = newTime;
			
        }
        // else if beating his own record on this secotr give an award
        else if ( newTime < this.player.bestCheckPoints[id] || this.player.bestCheckPoints[id] == 0 )
        {
            this.player.client.addAward( (this.lastCheckPoint + 1) + ". checkpoint personal record!" );
            this.player.bestCheckPoints[id] = newTime;
        }

        this.lastCheckPoint++;
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
		uint oldTime = levelRecords[0].finishTime; // diff to server best
		uint newTime = this.getTime();
		
		bool noDelta = 0 == oldTime;
		
        if ( noDelta || newTime < oldTime )
        {
            this.player.bestRaceTime = newTime;
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

        G_CenterPrintMsg( this.player.getClient().getEnt(), "Finish: " + TimeToString( newTime ) + "\n"
			+ ( noDelta ? "" : str + TimeToString( this.delta ) ) );
		
		// if beating the level record on this sector give an award
        if ( newTime < levelRecords[0].finishTime || levelRecords[0].finishTime == 0 )
        {
            this.player.client.addAward( "Server record!" );
			G_Print(this.player.client.getName() + " " + S_COLOR_YELLOW
				+ "made a new server record: " + TimeToString( newTime ) + "\n");
        }
        // else if beating his own record on this secotr give an award
        else if ( newTime < oldTime || oldTime == 0 )
        {
            this.player.client.addAward( "Personal record!" );
        }
		
        // see if the player improved one of the top scores
		for ( int top = 0; top < MAX_RECORDS; top++ )
		{
			if ( newTime < levelRecords[top].finishTime || levelRecords[top].finishTime == 0 )
			{
				// move the other records down
				for ( int i = MAX_RECORDS - 1; i > top; i-- )
				{
					levelRecords[i].Copy( levelRecords[i - 1] );
				}

				levelRecords[top].Store( this.player.client );

				RACE_WriteTopScores();
				RACE_UpdateHUDTopScores();
				break;
			}
		}
		
		return true;
	}
}