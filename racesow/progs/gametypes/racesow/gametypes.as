/**
 * Racesow_Gametype
 */
class Racesow_Gametype
{
    Racesow_Player@[] players;
    RC_Map @commandMapInternal;
    RC_Map @commandMap;

    bool ammoSwitch;

    Racesow_Gametype() 
    {
        this.players = Racesow_Player@[](maxClients);
        @this.commandMapInternal = @RC_Map();
        commandMapInternal.set_opIndex( "callvotecheckpermission", @Command_CallvoteCheckPermission() );
        commandMapInternal.set_opIndex( "callvotevalidate", @Command_CallvoteValidate() );
        commandMapInternal.set_opIndex( "callvotepassed", @Command_CallvotePassed() );
        @this.commandMap = @RC_Map();
        insertDefaultCommands(this.commandMap);
        ammoSwitch = false;
    }

    void InitGametype() { }
    
    void SpawnGametype() { }
    
    void Shutdown() { }

    bool ammoSwitchAllowed() { return ammoSwitch; }
    
    bool MatchStateFinished( int incomingMatchState )
    {
        assert( false, "You have to overwrite 'bool MatchStateFinished( int incomingMatchState )' in your Racesow_Gametype" );
        return false;
    }
    
    void MatchStateStarted() { }
    
    void ThinkRules() { }
    
    void playerRespawn( cEntity @ent, int old_team, int new_team ) { }
    
    void scoreEvent( cClient @client, cString &score_event, cString &args ) { }
    
    cString @ScoreboardMessage( int maxlen )
    {
        assert( false, "You have to overwrite 'cString @ScoreboardMessage( int maxlen )' in your Racesow_Gametype." );
        return "This needs to be overwritten!\n";
    }
    
    cEntity @SelectSpawnPoint( cEntity @self )
    {
        assert( false, "You have to overwrite 'cEntity @SelectSpawnPoint( cEntity @self )' in your Racesow_Gametype." ); 
        return cEntity(); 
    }
    
    bool UpdateBotStatus( cEntity @self ) 
    {
        assert( false, "You have to overwrite 'bool UpdateBotStatus( cEntity @self )' in your Racesow_Gametype." ); 
        return false;
    }

	/**
	 * execute a Racesow_Command
     * @param client Client who wants to execute the command
	 * @param cmdString Name of Command to execute
	 * @param argsString Arguments passed to the command
	 * @param argc Number of arguments
	 * @return Success boolean
	 */
    bool Command( cClient @client, cString @cmdString, cString @argsString, int argc )
    {
        Racesow_Command @command;
        Racesow_Player @player = Racesow_GetPlayerByClient( client );

        @command = @this.commandMapInternal.get_opIndex(cmdString);

        if( @command == null )
            @command = @this.commandMap.get_opIndex(cmdString);

        if( @command != null ) {
	        if(command.validate(player, argsString, argc))
	        {
	            if(command.execute(player, argsString, argc))
	                return true;
	            else
	            {
	                player.sendErrorMessage( "Command " + command.name + " failed." );
	                return false;
	            }
	        }
//            player.sendMessage( command.getUsage() );
        }
        return false;
    }

    void registerCommands() { this.commandMap.register(); }
}
