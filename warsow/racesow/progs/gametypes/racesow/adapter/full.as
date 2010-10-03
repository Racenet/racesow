/**
 * Everything which requires the modded racesow game
 * library should pe piped through this class, so it
 * can easily be excanhed with the compatibility
 * adapter.
 */
class Racesow_Adapter_Full : Racesow_Adapter_Abstract
{
    /**
	 * Event: initialize the gametype
     * 
	 * @return void
	 */
	void initGametype()
	{
        map.getStatsHandler().loadStats();
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