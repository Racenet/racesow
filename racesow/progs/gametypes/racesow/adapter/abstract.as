/**
 * Racesow_Adapter_Abstract
 *
 * @package Racesow_Adapter
 * @version 0.6.2
 *
 * This file contains abstract definitions that must be defined in subclasses.
 */

class Racesow_Adapter_Abstract
{
    /**
	 * Event: player finishes a race
     * 
     * @param ...
	 * @return void
	 */
    void raceFinish(Racesow_Player_Race @race)
    {
    }
    
    /**
	 * Callback for "raceFinish"
     * 
     * @param ...
	 * @return void
	 */
	void raceCallback()
	{
	}
    
    /**
	 * Event: player appears
     * 
     * @param ...
	 * @return void
	 */
	void playerAppear(Racesow_Player @player)
	{
	}    
  
    
    /**
	 * Event: player disappears
     * 
     * @param ...
	 * @return void
	 */
	void playerDisappear(Racesow_Player @player, cString nickName, bool threaded)
	{
	}
    
    /**
	 * Event: initialize the gametype
     * 
	 * @return void
	 */
	void initGametype()
	{
	}
    
    /**
     * Allow the adapter to perform a think.
     * Usefull for stats callback
     *
     * @return void
     */
    void think()
    {
    }
}
