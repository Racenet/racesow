/**
 * Everything which requires the modded racesow game
 * library should pe piped through this class, so it
 * can easily be excanhed with the compatibility
 * adapter.
 */
class Racesow_Adapter_Full : Racesow_Adapter_Abstract
{
    /**
	 * Event: player finishes a race
     * 
     * @param ...
	 * @return void
	 */
    void raceFinish(Racesow_Player_Race @race)
    {
        // Call to a c-function which should result
        // in a callback to player.raceCallback()
        RS_MysqlInsertRace(
            race.getPlayer().getId(),
            race.getPlayer().getNickId(),
            map.getId(),
            race.getTime(),
            race.getPlayer().getClient().playerNum(),
            race.getPlayer().numberOfRacesSinceLastRace,
            race.getPlayer().raceDuration
        );
    }

    /**
	 * Event: initialize the gametype
     * 
	 * @return void
	 */
	void initGametype()
	{
        // Call to a c-function which should result
        // in a callback to map.loadCallback()
        RS_MysqlLoadMap(); 
	}  
    
    /**
	 * Event: player disappears
     * 
     * @param ...
	 * @return void
	 */
    void playerDisappear(Racesow_Player @player, cString nickName, bool threaded)
    {
        if ( mysqlConnected != 0 )
        {
            RS_MysqlPlayerDisappear(
                nickName,
                levelTime-player.joinedTime,
                player.getId(),
                player.getNickId(),
                map.getId(),
                player.getAuth().isAuthenticated(),
                threaded
            );
        }
    }
}
