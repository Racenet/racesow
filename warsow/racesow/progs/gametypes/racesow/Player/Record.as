/**
 * Racesow_Player_Record
 *
 */
class Racesow_Player_Record
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
	 * Constructor
	 *
	 */
    Racesow_Player_Record()
    {
       this.reset();
    }

	/**
	 * Destructor
	 *
	 */
    ~Racesow_Player_Record() {}
	
	/**
	 * Reset to initial values
	 *
	 */
	void reset()
	{
		this.finishTime = 0;
		this.checkPoints.resize( numCheckpoints );
        for ( int i = 0; i < numCheckpoints; i++ )
            this.checkPoints[i] = 0;
	}
	
	/**
	 * Copy
	 * @param Racesow_Player_Record &other
	 */
    void Copy( Racesow_Player_Record &record )
    {
        this.finishTime = record.finishTime;
        this.playerName = record.playerName;
        for ( int i = 0; i < numCheckpoints; i++ )
            this.checkPoints[i] = record.checkPoints[i];
    }

	/**
	 * Store
	 * @param cClient @client
	 */
    void Store( cClient @client )
    {
        Racesow_Player @player = @Racesow_GetPlayerByClient( client );

        this.finishTime = player.getBestTime();
        this.playerName = client.getName();
        for ( int i = 0; i < numCheckpoints; i++ )
            this.checkPoints[i] = player.bestCheckPoints[i];
    }
}