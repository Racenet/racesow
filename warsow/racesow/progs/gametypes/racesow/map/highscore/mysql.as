/**
 * Racesow_Map_HighScore_Mysql
 *
 * Manage stats in mysql mod, stores all race info in a mysql database
 *
 * @package Racesow
 * @subpackage Map_HighScore
 * @version 0.5.6
 */
class Racesow_Map_HighScore_Mysql : Racesow_Map_HighScore_Abstract
{
	/**
	 * Add a race to the highscores
	 * @param Racesow_Player_Race @race
	 * @return void
	 */
	void addRace(Racesow_Player_Race @race)
	{
		RS_MysqlInsertRace( race.getPlayer().getId(), race.getPlayer().getNickId(), map.getId(), race.getTime(), race.getPlayer().getClient().playerNum());
	}
	
	/**
	 * Load map id and server best
	 * @return void
	 */
	void loadStats()
	{
        RS_MysqlLoadMap( );
	}
	
	/**
	 * Update the stats in the user HUDs
	 * @return void
	 */
	void updateHud()
	{
		// removed for now - r2
		int i_like_to_see_top_in_hud=0;
		if (i_like_to_see_top_in_hud==1)
		{
			for ( int i = 0; i < MAX_RECORDS; i++ )
			{
				if ( this.highScores[i].finishTime > 0 )
				{
					G_ConfigString( CS_GENERAL + i, "#" + ( i + 1 ) + " - "
						+ this.highScores[i].playerName + " - " + TimeToString( this.highScores[i].finishTime ) );
				}
			}
		}
	}
	
	/**
	 * Get the stats as a string
	 * @return cString
	 */
	cString getStats()
	{
		return "TODO...\n";
	}
}
