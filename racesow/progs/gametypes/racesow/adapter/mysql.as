/**
 * Racesow_Adapter_Mysql
 *
 * @package Racesow_Adapter
 * @version 0.6.2
 *
 * Everything which requires the mysql feature on the modded game
 * library should be piped through this class, so it
 * can easily be exchanged with the compatibility adapter.
 */

enum Racesow_Adapter_Enum
{
    RACESOW_ADAPTER_LOADMAP = 2,
    RACESOW_ADAPTER_HIGHSCORES,
    RACESOW_ADAPTER_APPEAR,
    RACESOW_ADAPTER_RACE,
    RACESOW_ADAPTER_MAPFILTER,
    RACESOW_ADAPTER_MAPLIST,
    RACESOW_ADAPTER_PLAYERNICK,
    RACESOW_ADAPTER_ONELINER
}

class Racesow_Adapter_Mysql : Racesow_Adapter_Abstract
{
    /**
	 * Event: player finishes a race
     * 
     * @param ...
	 * @return void
	 */
    void raceFinish(Racesow_Player_Race @race)
    {
	
		bool success;

        // Call to a c-function which should result
        // in a callback to player.raceCallback()
        // ..unless the player uses a protected nick
        success = RS_MysqlInsertRace( race.getPlayer().getId(),
                race.getPlayer().getNickId(), map.getId(), race.getTime(),
                race.getPlayer().getClient().playerNum(),
                race.getPlayer().triesSinceLastRace,
                race.getPlayer().racingTimeSinceLastRace,
                race.getCheckpoints(), race.prejumped );

        if (!success) {
            race.getPlayer().sendErrorMessage("Could not insert this race into the database, are you using a protected nick?");
        }

    }

    /**
	 * Event: initialize the gametype
     * 
	 * @return void
	 */
	void initGametype()
	{
		previousMapName=RS_LastMap();
		
        // Call to a c-function which should result
        // in a callback to map.loadCallback()
        RS_MysqlLoadMap(); 
	}  
    
    /**
	 * Event: player appears
     * 
     * @param ...
	 * @return void
	 */
	void playerAppear(Racesow_Player @player)
	{
        // Call to a c-function which should result
        // in a callback to player_auth.appearCallback()
        RS_MysqlPlayerAppear(
            player.getName(),
            player.getClient().playerNum(),
            player.getId(),
            map.getId(),
            player.getAuth().isAuthenticated(),
            player.getAuth().authenticationName,
            player.getAuth().authenticationPass,
            player.getAuth().authenticationToken
        );
	}   
    
    /**
	 * Event: player disappears
     * 
     * @param ...
	 * @return void
	 */
    void playerDisappear(Racesow_Player @player, cString nickName, bool threaded)
    {
        // Call to a c-function which should result
        // in a callback to map.loadCallback()
        RS_MysqlPlayerDisappear(
            nickName,
            levelTime-player.joinedTime,
            player.tries,
            player.racingTime,
            player.getId(),
            player.getNickId(),
            map.getId(),
            player.getAuth().isAuthenticated(),
            threaded
        );
    }

    /**
     * Check the callback queue for a pending callback to perform
     * Callbacks are added to this queue in the C game code when
     * a reply to a mysql query has to be sent to the client
     *
     * @return void
     */
    void think()
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

                /* why printing that complicated and not just in RS_PopCallbackQueue() ?
                 *
                 * This is somewhat complicated but necessary, the RS_PopCallbackQueue
                 * sets a fixed number of variables and cannot contain arbitrary strings.
                 * So when a mysql query results in a string that must be printed to the client
                 * the result string is stored in a C array and a the callback is just a
                 * notification to say that a message is available. The actual message
                 * is retrievd using the RS_PrintQueryCallback function
                 *
                 */
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
