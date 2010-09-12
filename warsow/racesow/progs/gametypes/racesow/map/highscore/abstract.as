/**
 * Racesow_Map_HighScore_Abstract
 *
 * @package Racesow
 * @subpackage Map_HighScore
 * @version 0.5.4
 */
class Racesow_Map_HighScore_Abstract
{
	/**
	 * Pointer to the map
	 * @var Racesow_Map
	 */
	Racesow_Map @map;
	
	/**
	 * Logfile id
	 * @var uint
	 */
	uint64 logTime;
	
	/**
	 * The map's highscores
	 * @var Racesow_Map_HighScore
	 */
	Racesow_Map_HighScore[] highScores;
	
	/**
	 * Constructor
	 *
	 */
	Racesow_Map_HighScore_Abstract()
	{
		this.logTime = 0;
		this.highScores.resize( MAX_RECORDS );
		for ( int i = 0; i < MAX_RECORDS; i++ )
		{
			this.highScores[i].reset();
		}
	}	
	
	/**
	 * Constructor with map
	 * @param Racesow_Map @map
	 */
	Racesow_Map_HighScore_Abstract( Racesow_Map @map )
	{
		this.logTime = 0;
		this.setMap(@map);
		this.highScores.resize( MAX_RECORDS );
		for ( int i = 0; i < MAX_RECORDS; i++ )
		{
			this.highScores[i].reset();
		}
	}

	/**
	 * Get a highscore model
	 * @param uint id
	 * @return Racesow_Map_HighScore
	 */
	Racesow_Map_HighScore @getHighScore(uint id)
	{
		if (id >= this.highScores.length())
			return Racesow_Map_HighScore();
		
		return @this.highScores[id];
	}
	
	/**
	 * Setter for map
	 * @param Racesow_Map @map
	 * @return void
	 */
	void setMap( Racesow_Map @map )
	{
		@this.map = @map;
	}
	
	/**
	 * Add a race to the highscores
	 * @param Racesow_Player_Race @race
	 * @return void
	 */
	void addRace(Racesow_Player_Race @race)
	{
	}
	
	/**
	 * Write the stats somewhere
	 * @return void
	 */
	void writeStats()
	{
	}
	
	/**
	 * Load the stats from somewhere
	 * @return void
	 */
	void loadStats()
	{
	}
	
	/**
	 * Update the stats in the user HUDs
	 * @return void
	 */
	void updateHud()
	{
	}
	
	/**
	 * Get the stats as a string
	 * @return cString
	 */
	cString getStats()
	{
		return "";
	}
}