class Racesow_Map_HighScore_Default
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
	 * @var cString
	 */
    cString playerName;
	
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
	
	bool setCheckPoint(uint id, uint time)
	{
		if ( id >= this.checkPoints.length() ) 
			return false;
		
		this.checkPoints[id] = time;
		return true;
	}
	
	void reset()
	{
		this.finishTime = 0;
		this.playerName = "";
	
		this.checkPoints.resize( numCheckpoints );
		for ( int i = 0; i < numCheckpoints; i++ )
			this.checkPoints[i] = 0;
	}	
	
	Racesow_Map_HighScore_Default@ opAssign(const Racesow_Map_HighScore_Default &highScore)
	{
		this.finishTime = highScore.finishTime;
		this.playerName = highScore.playerName;
		this.checkPoints.resize( highScore.checkPoints.length() );
		for ( int i = 0; i < highScore.checkPoints.length(); i++ )
			this.checkPoints[i] = highScore.checkPoints[i];

		return this;	
	}

	void fromRace( Racesow_Player_Race &race )
	{
		this.finishTime = race.getTime();
		this.playerName = race.getPlayer().getClient().getName();
		this.checkPoints.resize( numCheckpoints );
		for ( int i = 0; i < numCheckpoints; i++ )
			this.checkPoints[i] = race.getCheckPoint(i);
	}
}