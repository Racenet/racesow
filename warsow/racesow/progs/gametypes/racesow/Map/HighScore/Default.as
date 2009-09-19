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
	 * identify
	 * @return void
	 */
	void identify(Racesow_Player @player)
	{
		return;
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
	
	void reset()
	{
		this.finishTime = 0;
		this.playerName = "";
	
		this.checkPoints.resize( numCheckpoints );
		for ( int i = 0; i < numCheckpoints; i++ )
			this.checkPoints[i] = 0;
	}	
	
	void Copy(Racesow_Map_HighScore_Default @score)
	{
		this.finishTime = score.getTime();
		this.playerName = score.getPlayerName();
	
		this.checkPoints.resize( numCheckpoints );
		for ( int i = 0; i < numCheckpoints; i++ )
		{
			this.checkPoints[i] = score.getCheckPoint(i);
		}
	}	
	
	void Store(cClient @client)
	{
		Racesow_Player @player = Racesow_GetPlayerByClient( client );
		
		this.finishTime = player.getBestTime();
		this.playerName = client.getName();
		this.checkPoints.resize( numCheckpoints );
		for ( int i = 0; i < numCheckpoints; i++ )
			this.checkPoints[i] = player.getCheckPoint(i);
	}
}