/**
 * Racesow_Adapter_Abstract
 *
 * @package Racesow_Adapter
 * @version 0.6.2
 */

const uint RACESOW_ADAPTER_LOADMAP = 2;
const uint RACESOW_ADAPTER_HIGHSCORES = 3;
const uint RACESOW_ADAPTER_APPEAR = 4;
const uint RACESOW_ADAPTER_RACE = 5;
const uint RACESOW_ADAPTER_MAPFILTER = 6;
const uint RACESOW_ADAPTER_MAPLIST = 7;
const uint RACESOW_ADAPTER_PLAYERNICK = 8;
const uint RACESOW_ADAPTER_ONELINER = 9;

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
     * Perform a callback if there is at least one pending
     *
     * @return void
     */
    void thinkCallbackQueue()
    {
        int command, playerNum, arg2, arg3, arg4, arg5, arg6, arg7;
        cString result;
        Racesow_Player @player;
        
        if( !RS_PopCallbackQueue( command, playerNum, arg2, arg3, arg4, arg5, arg6, arg7 ) )
            return;

        if (command == 0)
        {
            G_PrintMsg( null, "Warning: Callback without command\n" );
            return;
        }

        // handle the loadmap callback differently cause it's not attached to any player
        if ( command == RACESOW_ADAPTER_LOADMAP )
        {
            map.loadCallback(arg2, arg3);
        }
        else
        {
            @player = Racesow_GetPlayerByNumber(playerNum);

            if ( @player == null )
            {
                G_PrintMsg( null, "Warning: Callback to non-existant playerNum: " + playerNum + "\n" );
                return;
            }

            if ( @player.getClient() == null )
            {
                G_PrintMsg( null, "Player without client: " + playerNum + "\n" );
                return;
            }

            switch( command )
            {
                case RACESOW_ADAPTER_APPEAR:
                    player.getAuth().appearCallback(arg2, arg3, arg4, arg5, arg6, arg7);
                    break;

                case RACESOW_ADAPTER_RACE:
                    player.raceCallback(arg2, arg3, arg4, arg5, arg6, arg7);
                    break;

                case RACESOW_ADAPTER_PLAYERNICK:
                    result = RS_PrintQueryCallback( playerNum );
                    player.isWaitingForCommand = false;
                    player.getAuth().nickCallback( arg2, result );
                    break;

                // why printing that complicated and not just in RS_PopCallbackQueue() ?
                case RACESOW_ADAPTER_HIGHSCORES:
                case RACESOW_ADAPTER_MAPFILTER:
                case RACESOW_ADAPTER_MAPLIST:
                case RACESOW_ADAPTER_ONELINER:
                    result = RS_PrintQueryCallback(playerNum);
                    player.sendLongMessage(result);
                    player.isWaitingForCommand = false;
                    break;
            }
        }
    }
}
