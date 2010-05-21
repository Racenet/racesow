/**
 * Racesow_Map_HighScore
 *
 * Stores a highscore on a map
 *
 * @package Racesow
 * @subpackage Map
 * @version 0.5.1d
 * @date 24.09.2009
 * @author soh-zolex <zolex@warsow-race.net>
 */
class Racesow_Map_HighScore
{
	/**
	 * @var uint
	 */
    uint[] checkPoints;
	
	/**
	 * @var uint
	 */
    uint finishTime;
	
	/**
	 * @var uint64
	 */
	uint64 timeStamp;
	
	/**
	 * @var cString
	 */
    cString playerName;
	
	/**
	 * Overloaded operator =
	 * @param const Racesow_Map_HighScore &highScore
	 * @return Racesow_Map_HighScore
	 */
	Racesow_Map_HighScore@ opAssign(const Racesow_Map_HighScore &highScore)
	{
		this.finishTime = highScore.finishTime;
		this.playerName = highScore.playerName;
		this.checkPoints.resize( highScore.checkPoints.length() );
		for ( uint i = 0; i < highScore.checkPoints.length(); i++ )
			this.checkPoints[i] = highScore.checkPoints[i];

		return this;	
	}
	
	/**
	 * getPlayerName
	 * @return cString
	 */
	cString getPlayerName()
	{
		return this.playerName;
	}
	
	/**
	 * getTime
	 * @return int
	 */
	int getTime()
	{
		return this.finishTime;
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
	 * getCheckPoint
	 * @param uint
	 * @return int
	 */
	uint getCheckPoint(uint id)
	{
		if (id >= this.checkPoints.length())
			return 0;
		
		return this.checkPoints[id];
	}
	
	/**
	 * Set a checkpoint time
	 * @param uint id
	 * @param uint time
	 * @return bool
	 */
	bool setCheckPoint(uint id, uint time)
	{
		if ( id >= this.checkPoints.length() ) 
			return false;
		
		this.checkPoints[id] = time;
		return true;
	}
	
	/**
	 * Reset the highscore object to default values
	 * @return void
	 */
	void reset()
	{
		this.finishTime = 0;
		this.playerName = "";
	
		this.checkPoints.resize( numCheckpoints );
		for ( int i = 0; i < numCheckpoints; i++ )
			this.checkPoints[i] = 0;
	}	

	/**
	 * Set the highscore from a race
	 * @param Racesow_Player_Race &race
	 * @return void
	 */
	void fromRace( Racesow_Player_Race &race )
	{
		this.finishTime = race.getTime();
		this.playerName = race.getPlayer().getClient().getName();
		this.timeStamp = race.getTimeStamp();
		this.checkPoints.resize( numCheckpoints );
		for ( int i = 0; i < numCheckpoints; i++ )
			this.checkPoints[i] = race.getCheckPoint(i);
	}
}