/**
* Racesow callbacks 
*
* @package Racesow
* @version 0.5.3
*/

/**
 * callback codes
 */
const uint RACESOW_CALLBACK_AUTHENTICATE = 0;
const uint RACESOW_CALLBACK_NICKPROTECT = 1;
const uint RACESOW_CALLBACK_LOADMAP = 2;
const uint RACESOW_CALLBACK_HIGHSCORES = 3;
const uint RACESOW_CALLBACK_APPEAR = 4;
const uint RACESOW_CALLBACK_PERSONALBEST = 5;

 /**
 * Racesow_ThinkCallbackQueue()
 * perform a callback if there is at least one pending
 */
void Racesow_ThinkCallbackQueue()
{
	int command, arg1, arg2, arg3;
    Racesow_Player @player;
	
    if( !RS_QueryCallbackQueue( command, arg1, arg2, arg3 ) )
        return;
        
    switch( command )	
    {
        case RACESOW_CALLBACK_AUTHENTICATE:
			@player = Racesow_GetPlayerByNumber(arg1);
			if ( @player != null )
	            player.getAuth().authCallback( arg2, arg3 );
            break;
            
        case RACESOW_CALLBACK_NICKPROTECT:
            @player = Racesow_GetPlayerByNumber(arg1);
			if ( @player != null )
	            player.getAuth().nickProtectCallback( arg2, arg3 );
            break;    

        case RACESOW_CALLBACK_LOADMAP:
            map.setId( arg1 );
			if ( rs_loadHighscores.getBool() )
			{
				map.getStatsHandler().getHighScore(0).finishTime = arg2 ;
				// we could also do a getHighScore(0).fromRace(arg2), ie. also recover checkpoints, but I don't want that, and arg2 can only be an int..
			}
            break;
			
		case RACESOW_CALLBACK_HIGHSCORES:
			@player = Racesow_GetPlayerByNumber(arg1);
			if ( @player != null )
			{
				RS_PrintHighscoresTo(player.getClient().getEnt(),arg1);
				player.isWaitingForCommand=false;
			}
            break;
			
		case RACESOW_CALLBACK_APPEAR:
			@player = Racesow_GetPlayerByNumber(arg1);
			if ( @player != null )
			{
                player.getAuth().appearCallback(arg2);
			}
            break;

		case RACESOW_CALLBACK_PERSONALBEST:
			@player = Racesow_GetPlayerByNumber(arg1);
			if ( @player != null )
			{
				if ( rs_loadHighscores.getBool() )
				{
					player.bestRaceTime = arg2;
				}
			}
            break;
   }
}