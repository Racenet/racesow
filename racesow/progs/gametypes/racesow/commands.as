/**
 * MODFLAG, used to determine in which gametype a command is registered
 */
const int MODFLAG_RACE = 1;
const int MODFLAG_FREESTYLE = 2;
const int MODFLAG_FASTCAP = 4;
const int MODFLAG_DRACE = 8;
const int MODFLAG_DURACE = 16;
const int MODFLAG_TRACE = 32;
const int MODFLAG_ALL = 63;

const cString COMMAND_COLOR_LINE = S_COLOR_BLACK;
const cString COMMAND_COLOR_HEAD = S_COLOR_RED;
const cString COMMAND_COLOR_LEFT = S_COLOR_ORANGE;
const cString COMMAND_COLOR_DEFAULT = S_COLOR_WHITE;
const cString COMMAND_COLOR_SPECIAL = S_COLOR_YELLOW;

const cString[] DEVS =  { "R2", "Zaran", "Zolex", "Schaaf", "K1ll", "Weqo", "Joki" };

/**
 * Generic command class, all commands inherit from this class
 * and should override the validate and execute function
 */
class Racesow_Command
{
	/**
	 * Pointer to parent command
	 * null, if this is not a subcommand
	 */
	Racesow_Command @baseCommand;

    /**
     * The name of the command.
     * That's what the user will have to type to call the command
     */
    cString name;

    /**
     * Description of what the command does.
     * This is displayed in the help
     */
    cString description;

    /**
     * Usage of the command. This should document the command syntax.
     * Displayed when you do "help <command>" and also when you make an error
     * with the command syntax (e.g wrong number of arguments).
     * When describing the syntax, put the arguments name into brackets (e.g <mapname>)
     */
    cString usage;

    /**
     * In which mode should the command only be available ?
     */
    int modFlag; //FIXME: soon obsolete
    
	/**
	 * Should the command be available in practice mode ?
	 */
	bool practiceEnabled;
	
	/**
	 * Required User Privileges
	 */
	int permissionMask;

    /**
     * Non-Default constructor to be used with super() in the derived classes
     */
    Racesow_Command(cString &in name, cString &in description, cString &in usage)
    {
        this.name = name;
        this.description = description;
        this.usage = usage;
        this.permissionMask = 0;
        @this.baseCommand = null;
    }

    /**
     * This is called before the actual work is done.
     *
     * Here you should only check the number of arguments and that
     * the player has the right to call the command
     *
     * @param player The player who calls the command
     * @param args The tokenized string of arguments
     * @param argc The number of arguments
     * @return success boolean
     */
    bool validate(Racesow_Player @player, cString &args, int argc)
    {
        return true;
    }

    /**
     * This is called after the validate function.
     *
     * Technically no validation should be done here, only the real work.
     * Most probably a call to a player method.
     * @param player The player who calls the command
     * @param args The tokenized string of arguments
     * @param argc The number of arguments
     * @return success boolean
     */
    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        return true;
    }

    /**
     * Return the subcommand level
     */
    int getLevel()
    {
    	Racesow_Command @command = @baseCommand;
    	int i = 0;
    	while( @command != null )
    	{
    		@command = @baseCommand.baseCommand;
    		i++;
    	}
    	return i;
    }

    /**
     * Return the command description in a nice way to be printed
     */
    cString getDescription()
    {
        return COMMAND_COLOR_LEFT + this.name + ": " + COMMAND_COLOR_DEFAULT + this.description + "\n";
    }

    /**
     * Return the command usage in a nice way to be printed
     */
    cString getUsage()
    {
        if ( this.usage.len() > 0 )
            return COMMAND_COLOR_LEFT + "Usage: " + COMMAND_COLOR_DEFAULT + this.usage + "\n";
        else
            return "";
    }
}

class Command_Admin : Racesow_Command
{
	RC_Map @commandMap;

    Command_Admin() {
        super("admin", "Execute an admin command", "<subcommand> [args...]");

        @this.commandMap = @RC_Map();
        this.commandMap.set_opIndex( "map", @Command_AdminMap( @this ) );
        this.commandMap.set_opIndex( "restart", @Command_AdminRestart( @this ) );
        this.commandMap.set_opIndex( "extend_time", @Command_AdminExtendtime( @this ) );
        this.commandMap.set_opIndex( "remove", @Command_AdminRemove( @this ) );
        this.commandMap.set_opIndex( "kick", @Command_AdminKick( @this ) );
        this.commandMap.set_opIndex( "kickban", @Command_AdminKickban( @this ) );
        this.commandMap.set_opIndex( "mute", @Command_AdminMute( @this ) );
        this.commandMap.set_opIndex( "unmute", @Command_AdminUnmute( @this ) );
        this.commandMap.set_opIndex( "vmute", @Command_AdminVmute( @this ) );
        this.commandMap.set_opIndex( "vunmute", @Command_AdminVunmute( @this ) );
        this.commandMap.set_opIndex( "votemute", @Command_AdminVotemute( @this ) );
        this.commandMap.set_opIndex( "voteunmute", @Command_AdminUnvotemute( @this ) );
        this.commandMap.set_opIndex( "joinlock", @Command_AdminJoinlock( @this ) );
        this.commandMap.set_opIndex( "cancelvote", @Command_AdminCancelvote( @this ) );
        this.commandMap.set_opIndex( "updateml", @Command_AdminUpdateml( @this ) );
        this.commandMap.set_opIndex( "help", @Command_Help( @this, @this.commandMap ) );
    }

    //validate only the first level of the command
    bool validate(Racesow_Player @player, cString &args, int argc)
    {
        if ( argc < this.getLevel() + 1 )
        {
            player.sendErrorMessage( "No subcommand given. Use '" + this.name + " help' for more information" );
            return false;
        }

        Racesow_Command @cmd = commandMap.get_opIndex( args.getToken( this.getLevel() ) );
    	if ( @cmd == null )
    		return false;

        if ( !player.auth.allow( cmd.permissionMask ) )
        {
//            G_PrintMsg( null, S_COLOR_WHITE + player.getName() + S_COLOR_RED
//                + " tried to execute an admin command without permission.\n" );
            player.sendErrorMessage( "You don't have permission to execute this command" );
            return false;
        }

        return true;
    }

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
    	Racesow_Command @subCommand = commandMap.get_opIndex( args.getToken( this.getLevel() ) );

    	if( subCommand.validate( player, args, argc ) )
    		if( subCommand.execute( player, args, argc ) )
    	    	return true;
    	return false;
    }
}

class Command_AmmoSwitch : Racesow_Command
{
    Command_AmmoSwitch() {
        super("ammoswitch", "Switch between weak and strong Ammo", "");
    }

    bool execute( Racesow_Player @player, cString &args, int argc ) {
        return player.ammoSwitch();
    }
}

class Command_Auth : Racesow_Command
{

    Command_Auth() {
        super("auth", "Authenticate with the server (alternatively you can use setu to save your login)", "<authname> <password>");
    }

    bool validate(Racesow_Player @player, cString &args, int argc)
    {
        if ( argc < 2 )
        {
            player.sendErrorMessage("You must provide your name and password");
            return false;
        }

        return true;
    }

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        return player.getAuth().authenticate(
                args.getToken( 0 ).removeColorTokens(),
                args.getToken( 1 ),
                false );
    }
}

class Command_CallvoteCheckPermission : Racesow_Command {

    Command_CallvoteCheckPermission() {
        super("callvotecheckpermission", "Check if you have the permission to call the specified Callvote", "<callvote>");
    }

    //When adding callvotes to a gametype derive from this command and add yours into this function
    //Don't forget to add your Derived Class Object instead of adding a Base Class Object!
    bool gametypeVotes( Racesow_Player @player, cString &args, int argc ) {
        return false;
    }

    bool execute( Racesow_Player @player, cString &args, int argc ) {
        if ( player.isVotemuted )
        {
            player.sendErrorMessage( "You are votemuted" );
            return false;
        }
        else
        {
            cString vote = args.getToken( 0 );
            if( vote == "mute" || vote == "vmute" ||
                vote == "kickban" || vote == "kick" || vote == "remove" ||
                vote == "joinlock" || vote == "joinunlock" )
            {
                Racesow_Player @victimPlayer;
                cString victim = args.getToken( 1 );

                if ( Racesow_GetClientNumber( victim ) != -1 )
                    @victimPlayer = racesowGametype.players[ Racesow_GetClientNumber( victim ) ];
                else if( victim.isNumerical() )
                {
                    if ( victim.toInt() > maxClients )
                        return true;
                    else
                        @victimPlayer = racesowGametype.players[ victim.toInt() ];
                }
                else
                    return true;

                if( victimPlayer.auth.allow(RACESOW_AUTH_ADMIN) )
                {
                    G_PrintMsg( null, S_COLOR_WHITE + player.getName()
                                + S_COLOR_RED + " tried to "
                                + args.getToken( 0 ) + " an admin.\n" );
                    return false;
                }
                else
                {
                    return true;
                }
            }
            else
            {
                return true;
            }
        }
    }
}

class Command_CallvoteValidate : Racesow_Command
{

    Command_CallvoteValidate() {
        super("callvotevalidate","", ""); //FIXME: Needs description and usage
    }

    //When adding callvotes to a gametype derive from this command and add yours into this function
    //Don't forget to add your Derived Class Object instead of adding a Base Class Object!
    bool gametypeVotes( Racesow_Player @player, cString &args, int argc ) {
        return false;
    }

    bool execute( Racesow_Player @player, cString &args, int argc ) {
		cString vote = args.getToken( 0 );

        if( this.gametypeVotes(player, args, argc) )
            return true;

		if ( vote == "extend_time" )
		{
			if( g_timelimit.getInteger() <= 0 )
			{
				player.getClient().printMessage( "This vote is only available for timelimits.\n");
				return false;
			}
			uint timelimit = g_timelimit.getInteger() * 60000;//convert mins to ms
			uint extendtimeperiod = rs_extendtimeperiod.getInteger() * 60000;//convert mins to ms
			uint time = levelTime - match.startTime(); //in ms
			uint remainingtime = timelimit - time;
			bool isNegative = (timelimit < time ) ? true : false;
			if( remainingtime > extendtimeperiod && !isNegative )
			{
				player.getClient().printMessage( "This vote is only in the last " + rs_extendtimeperiod.getString() + " minutes available.\n" );
				return false;
			}
			return true;
		}
        if ( vote == "timelimit" )
		{
			int new_timelimit = args.getToken( 1 ).toInt();

			if ( new_timelimit < 0 )
			{
				player.getClient().printMessage( "Can't set negative timelimit\n");
				return false;
			}

			if ( new_timelimit == g_timelimit.getInteger() )
			{
				player.getClient().printMessage( S_COLOR_RED + "Timelimit is already set to " + new_timelimit + "\n" );
				return false;
			}

			return true;
		}

		if ( vote == "spec" )
		{
			if ( ! map.inOvertime )
			{
				player.getClient().printMessage( S_COLOR_RED + "Callvote spec is only valid during overtime\n");
				return false;
			}

			return true;
		}

		if ( vote == "joinlock" || vote == "joinunlock" )
    {
      if( argc != 2 )
    	{
        player.getClient().printMessage( "Usage: callvote " + vote + " <id or name>\n" );
    		player.getClient().printMessage( "- List of current players:\n" );

    		for ( int i = 0; i < maxClients; i++ )
    		{
    		  if ( @racesowGametype.players[i].getClient() != null )
            player.getClient().printMessage( "  " + racesowGametype.players[i].getClient().playerNum() + ": " + racesowGametype.players[i].getClient().getName() + "\n");
        }

       	return false;
    	}
      else
    	{
    	  cClient@ target = null;

        if ( args.getToken( 1 ).isNumerical() && args.getToken( 1 ).toInt() <= maxClients )
            @target = @G_GetClient( args.getToken( 1 ).toInt() );
        else if ( Racesow_GetClientNumber( args.getToken( 1 ) ) != -1 )
            @target = @G_GetClient( Racesow_GetClientNumber( args.getToken( 1 ) ) );

        if ( @target == null || !target.getEnt().inuse )
        {
            player.getClient().printMessage( S_COLOR_RED + "Invalid player\n" );
            return false;
        }
      }

			return true;
		}

		player.getClient().printMessage( "Unknown callvote " + vote + "\n" );
		return false;
    }
}

class Command_Chase : Racesow_Command
{

    Command_Chase() {
        super("chase", "Spectate", "");
    }

    bool validate(Racesow_Player @player, cString &args, int argc)
    {
        return true;
    }

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        player.getClient().team = TEAM_SPECTATOR;
        player.getClient().respawn( true ); // true means ghost
        return true;
    }

}

class Command_Chrono : Racesow_Command
{

    Command_Chrono() {
        super("chrono", "Chrono for tricks timing", "<start/reset>");
    }

    bool validate(Racesow_Player @player, cString &args, int argc)
    {
        if ( argc < 1 )
        {
            player.sendErrorMessage( "you must provide a chrono command" );
            return false;
        }

        return true;
    }

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        cString command = args.getToken( 0 );
        if( command == "start" )
        {
            player.chronoStartTime = levelTime;
            player.isUsingChrono = true;
        }
        else if( command == "reset" )
        {
            player.chronoStartTime = 0;
            player.isUsingChrono = false;
        }
        else
        {
            player.sendErrorMessage( "wrong chrono command");
            return false;
        }

        return true;
    }
}

class Command_CvarInfo : Racesow_Command
{
    Command_CvarInfo() {
        super("cvarinfo", "", ""); //FIXME: Add description and usage
    }

    bool execute( Racesow_Player @player, cString &args, int argc ) {
		//token0: cVar name; token1: cVar value
		cString cvarName = args.getToken(0);
		cString cvarValue = args.getToken(1);

		if( cvarName.substr(0,15) == "storedposition_")
		{
			cString positionValues = cvarValue;
			cVec3 origin, angles;
			origin.x = positionValues.getToken(1).toFloat();
			origin.y = positionValues.getToken(2).toFloat();
			origin.z = positionValues.getToken(3).toFloat();
			angles.x = positionValues.getToken(4).toFloat();
			angles.y = positionValues.getToken(5).toFloat();
			player.teleport( origin, angles, false, false );
		}
    return true;
    }
}

class Command_Gametype : Racesow_Command
{

    Command_Gametype() {
        super("gametype", "Print info about the game type", "");
    }

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        cString response = "";
        cVar fs_game( "fs_game", "", 0 );
        cString manifest = gametype.getManifest();

        response += "\n";
        response += "Gametype " + gametype.getName() + " : " + gametype.getTitle() + "\n";
        response += "----------------\n";
        response += "Version: " + gametype.getVersion() + "\n";
        response += "Author: " + gametype.getAuthor() + "\n";
        response += "Mod: " + fs_game.getString() + (manifest.length() > 0 ? " (manifest: " + manifest + ")" : "") + "\n";
        response += "----------------\n";

        player.sendMessage(response);
        return true;
    }
}

class Command_Help : Racesow_Command // (should be subclass of Racesow_AdminCommand )
{
    RC_Map @commandMap;

	Command_Help( Racesow_Command @baseCommand, RC_Map @commandMap )
	{
		super( "help", "Print this help, or give help on a specific command", "[command]" );
        @this.commandMap = @commandMap;
        @this.baseCommand = @baseCommand;
	}

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
    	cString baseCommandString = "";
    	Racesow_Command @command = @baseCommand;
    	while( @command != null )
    	{
    		baseCommandString += command.name + " ";
    		@command = @baseCommand.baseCommand;
    	}
    	if ( argc >= this.getLevel() + 1 )
        {
            @command = @this.commandMap.get_opIndex( args.getToken( this.getLevel() ) );
            if ( @command != null)
            {
                player.sendMessage( COMMAND_COLOR_LEFT + baseCommandString + command.name + " " + command.usage + ": " + COMMAND_COLOR_DEFAULT + command.description + "\n" );
                return true;
            }
            else
            {
                player.sendErrorMessage("Command " + COMMAND_COLOR_SPECIAL + baseCommandString + command.name + COMMAND_COLOR_DEFAULT + " not found");
                return true;
            }
        }
        else
        {
            cString help;
            help += COMMAND_COLOR_LINE + "--------------------------------------------------------------------------------------------------------------------------\n";
            help += COMMAND_COLOR_HEAD + baseCommandString.toupper() + "HELP for Racesow " + gametype.getVersion() + "\n";
            help += COMMAND_COLOR_LINE + "--------------------------------------------------------------------------------------------------------------------------\n";
            player.sendMessage(help);
            help = "";

            for (uint i = 0; i < this.commandMap.size(); i++)
            {
                @command = @this.commandMap.getCommandAt(i);

                help += COMMAND_COLOR_LEFT + baseCommandString + command.name + " " + command.usage + ": " + COMMAND_COLOR_DEFAULT + command.description + "\n";
                if ( (i/5)*5 == i ) //to avoid print buffer overflow
                {
                    player.sendMessage(help);
                    help = "";
                }
            }

            player.sendMessage(help);
            player.sendMessage( COMMAND_COLOR_LINE + "--------------------------------------------------------------------------------------------------------------------------\n");
            return true;
        }
    }
}

class Command_Join : Racesow_Command
{

    Command_Join() {
        super("join", "Join the game", "");
    }

    bool validate(Racesow_Player @player, cString &args, int argc)
    {
        if( player.isJoinlocked )
        {
            player.sendErrorMessage( "You can't join: You are join locked");
            return false;
        }

        if( map.inOvertime )
        {
            player.sendErrorMessage( "You can't join during overtime period" );
            return false;
        }
        return true;
    }

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        player.getClient().team = TEAM_PLAYERS;
        player.getClient().respawn( false );
        return true;
    }
}

class Command_Kill : Racesow_Command
{

    Command_Kill() {
        super("kill", "Go back to the start area whenever you want", "");
    }

    bool validate(Racesow_Player @player, cString &args, int argc)
    {
        if (player.isJoinlocked)
        {
            player.sendErrorMessage( "You can't join, you are join locked" );
            return false;
        }

        //racerestart command is only avaiblable in DRACE during WARMUP 
        if ( gametypeFlag == MODFLAG_DRACE && this.name == "racerestart" && match.getState() != MATCH_STATE_WARMUP ) //FIXME: This doesn't seem right ...
          return false;
                  
        return true;
    }

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        player.restartRace();
		    return true;
    }
}

class Command_Machinegun : Racesow_Command
{

    Command_Machinegun() {
        super("machinegun", "Gives you a machinegun", "");
    }

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
		//give machinegun (this is default behavior in defrag and usefull in some maps to shoot buttons)
		player.getClient().inventoryGiveItem( WEAP_MACHINEGUN );
		return true;
    }
}

class Command_Mapfilter : Racesow_Command
{

    Command_Mapfilter() {
        super("mapfilter", "Search for maps matching a given name", "<filter> <pagenum>");
    }

    bool validate(Racesow_Player @player, cString &args, int argc)
    {
        if (argc < 1)
        {
            player.sendErrorMessage( "You must provide a filter name" );
            return false;
        }

        if (player.isWaitingForCommand)
        {
            player.sendErrorMessage( "Flood protection. Slow down cowboy, wait for the "
                    +"results of your previous command");
            return false;
        }

        return true;
    }

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        cString filter = args.getToken( 0 );
        int page = 1;
        if ( argc >= 2 )
            page = args.getToken( 1 ).toInt();

        player.isWaitingForCommand = true;
        return RS_MapFilter(player.getClient().playerNum(),filter,page);
    }
}

class Command_Maplist : Racesow_Command
{

    Command_Maplist() {
        super("maplist", "Print the maplist", "<pagenum>");
    }

    bool validate(Racesow_Player @player, cString &args, int argc)
    {
        if( player.isWaitingForCommand )
        {
            player.sendErrorMessage( "Flood protection. Slow down cowboy, wait for the "
                    +"results of your previous command");
            return false;
        }

        return true;
    }

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        int page = 1;
        if (argc >= 1)
            page = args.getToken(0).toInt();

        return RS_Maplist(player.getClient().playerNum(),page);
    }
}

class Command_Mapname : Racesow_Command
{

    Command_Mapname() {
        super("mapname", "Print the name of current map", "");
    }

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        player.sendMessage( map.name + "\n" );
        return true;
    }
}

class Command_LastMap : Racesow_Command
{

    Command_LastMap() {
        super("lastmap", "Print the name of the previous map on the server, before this one", "");
    }

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        player.sendMessage( previousMapName + "\n" );
        return true;
    }
}

class Command_NextMap : Racesow_Command
{

    Command_NextMap() {
        super("nextmap", "Print the name of the next map in map rotation", "");
    }

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        player.sendMessage( RS_NextMap() + "\n" );
        return true;
    }
}

class Command_Noclip : Racesow_Command
{

    Command_Noclip() {
        super("noclip", "Disable your interaction with other players and objects", "");
    }

    bool validate(Racesow_Player @player, cString &args, int argc)
    {
        if( @player.getClient().getEnt() == null || player.getClient().getEnt().team == TEAM_SPECTATOR )
        {
            player.sendErrorMessage("Noclip is not available in your current state");
            return false;
        }
        if ( gametypeFlag == MODFLAG_RACE && !player.practicing )
			return false;

        return true;
    }

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        return player.noclip();
    }
}

class Command_Oneliner : Racesow_Command
{

    Command_Oneliner() {
        super("oneliner", "Set a one-line message that is displayed right next to your top time", "");
    }

    bool validate(Racesow_Player @player, cString &args, int argc)
    {
        if ( mysqlConnected == 0 )
        {
            player.sendMessage("This server doesn't store the best times, this command is useless\n" );
            return false;
        }
		
		if ( args.len() > 100 )
        {
            player.sendMessage("Oneliner too long (" + args.len() + " chars), please keep it under 100 characters.\n" );
            return false;
        }
		
		if ( argc < 1 )
        {
			player.sendMessage("If you are #1 on a map, you can enter a one-line message that will be printed in the highscores.\n" );
            return false;
        }
		
        return true;
    }

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
	    player.isWaitingForCommand = true;
        RS_MysqlSetOneliner(player.getClient().playerNum(), player.getId(), map.getId(), args);
        return true;
    }
}

class Command_Position : Racesow_Command
{
	RC_Map @commandMap;
	Racesow_Command @cmd;
	Command_Position() {
        super("position", "Commands to store and load position", "<subcommand> [args...]");

        @this.commandMap = @RC_Map();
        @cmd = @Command_PositionSave( @this );
        this.commandMap.set_opIndex( cmd.name, @cmd );
        @cmd = @Command_PositionLoad( @this );
        this.commandMap.set_opIndex( cmd.name, @cmd );
        @cmd = @Command_PositionSet( @this );
        this.commandMap.set_opIndex( cmd.name, @cmd );
        @cmd = @Command_PositionStore( @this );
        this.commandMap.set_opIndex( cmd.name, @cmd );
        @cmd = @Command_PositionRestore( @this );
        this.commandMap.set_opIndex( cmd.name, @cmd );
        @cmd = @Command_PositionStoredlist( @this );
        this.commandMap.set_opIndex( cmd.name, @cmd );
        @cmd = @Command_Help( @this, @this.commandMap );
        this.commandMap.set_opIndex( cmd.name, @cmd );
    }
    //validate only the first level of the command
    bool validate(Racesow_Player @player, cString &args, int argc)
    {
        if ( argc < this.getLevel() + 1 )
        {
            player.sendErrorMessage( "No subcommand given. Use '" + this.name + " help' for more information" );
            return false;
        }
        Racesow_Command @cmd = commandMap.get_opIndex( args.getToken( this.getLevel() ) );
    	if ( @cmd == null )
    	{
            player.sendErrorMessage( "Subcommand " + args.getToken( this.getLevel() + 1 ) + " not found" );
    		return false;
    	}
        return true;
    }
    bool execute(Racesow_Player @player, cString &args, int argc)
    {
    	Racesow_Command @subCommand = commandMap.get_opIndex( args.getToken( this.getLevel() ) );
    	if( subCommand.validate( player, args, argc ) )
    		if( subCommand.execute( player, args, argc ) )
    	    	return true;

    	return false;
    }
}

//implements validate() and execute() for position commands //TODO: integrate execution of position commands here
class Racesow_PositionCommand : Racesow_Command
{
	Racesow_PositionCommand( cString &in name, cString &in description, cString &in usage )
	{
        super( name, description, usage );
	}
	bool validate(Racesow_Player @player, cString &args, int argc)
	{
		if ( gametypeFlag == MODFLAG_RACE && !player.practicing )
			return false;
		return true;
	}
    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        return player.position(args);
    }
}

class Command_PositionSave : Racesow_PositionCommand
{
	Command_PositionSave( Racesow_Command @baseCommand ) {
        super("save", "Save position", "");
        @this.baseCommand = @baseCommand;
    }
}

class Command_PositionLoad : Racesow_PositionCommand
{
	Command_PositionLoad( Racesow_Command @baseCommand ) {
        super("load", "Teleport to saved position", "");
        @this.baseCommand = @baseCommand;
    }
}

class Command_PositionSet : Racesow_PositionCommand
{
	Command_PositionSet( Racesow_Command @baseCommand ) {
        super("set", "Teleport to specified position", "<x> <y> <z> <pitch> <yaw>");
        @this.baseCommand = @baseCommand;
    }
}

class Command_PositionStore : Racesow_PositionCommand
{
	Command_PositionStore( Racesow_Command @baseCommand ) {
        super("store", "Store a position for another session", "<id> <name>");
        @this.baseCommand = @baseCommand;
    }
}

class Command_PositionRestore : Racesow_PositionCommand
{
	Command_PositionRestore( Racesow_Command @baseCommand ) {
        super("restore", "Restore a stored position from another session", "<id>");
        @this.baseCommand = @baseCommand;
    }
}

class Command_PositionStoredlist : Racesow_PositionCommand
{
	Command_PositionStoredlist( Racesow_Command @baseCommand ) {
        super("storedlist", "Sends you a list of your stored positions", "<limit>");
        @this.baseCommand = @baseCommand;
    }
}

class Command_Spec : Racesow_Command
{

    Command_Spec() {
        super("spec", "Spectate", "");
    }

    bool validate(Racesow_Player @player, cString &args, int argc)
    {
        return true;
    }

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        player.getClient().team = TEAM_SPECTATOR;
        player.getClient().respawn( true ); // true means ghost
        return true;
    }
}

class Command_Practicemode : Racesow_Command
{

    Command_Practicemode() {
        super("practicemode", "Allows usage of the position and noclip commands", "");
    }

	bool validate( Racesow_Player @player, cString &args, int argc )
	{
		if ( player.getClient().team != TEAM_PLAYERS )
		{
			player.sendErrorMessage( "You must join the game before going into practice mode" );
			return false;
		}
		return true;
	}
	bool execute( Racesow_Player @player, cString &args, int argc )
	{
		if ( @player.getClient() != null )
		{
			if ( player.isRacing() )
				player.cancelRace();
				
			if ( player.practicing )
			{
				player.practicing = false;
				player.sendAward( S_COLOR_GREEN + "Leaving practice mode" );
				player.restartRace();
					
				
			} else {
				player.practicing = true;
				player.sendAward( S_COLOR_GREEN + "You have entered practice mode" );
			}
			return true;
		}
		/* something went wrong */
		return false;
		
	}
}

class Command_Privsay : Racesow_Command
{

    Command_Privsay() {
        super("privsay", "Send a private message to a player", "<playerid/playername> <message>");
    }

    bool validate(Racesow_Player @player, cString &args, int argc)
    {
        if ( player.getClient().muted == 1 )
        {
            player.sendErrorMessage("You can't talk, you're muted");
            return false;
        }

        if (argc < 2)
        {
            player.sendErrorMessage( "You must provide a player id and a message");
            return false;
        }
        return true;
    }

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        cClient@ target = null;

        if ( args.getToken( 0 ).isNumerical() && args.getToken( 0 ).toInt() <= maxClients )
        {
            @target = @G_GetClient( args.getToken( 0 ).toInt() );
        }
        else if ( Racesow_GetClientNumber( args.getToken( 0 ) ) != -1 )
        {
            @target = @G_GetClient( Racesow_GetClientNumber( args.getToken( 0 ) ) );
        }

        if ( @target == null || !target.getEnt().inuse )
        {
            player.sendErrorMessage("Invalid player");
            return false;
        }
        cString message = args.substr(args.getToken( 0 ).length()+1, args.len() );
        return player.privSay(message, target);
    }
}

class Command_ProtectedNick : Racesow_Command
{

    Command_ProtectedNick() {
        super("protectednick", "Show/update your current protected nick", "[newnick]");
    }

	bool validate(Racesow_Player @player, cString &args, int argc)
    {
		bool is_authenticated = player.getAuth().isAuthenticated();
		bool is_nickprotected = player.getAuth().wontGiveUpViolatingNickProtection() == 0;
		bool valid = ( is_authenticated and is_nickprotected );
		
		if (not valid)
		{
			player.sendErrorMessage( "You must be authenticated and not under nick protection.");
			return false;
		}
		
		if( player.isWaitingForCommand )
        {
            player.sendErrorMessage( "Flood protection. Slow down cowboy, wait for the "
                    +"results of your previous command");
            return false;
        }
		
		return true;
	}
	
    bool execute(Racesow_Player @player, cString &args, int argc)
    {
		if ( argc < 1 )
		{
			RS_GetPlayerNick( player.getClient().playerNum(), player.getId() );
		}
		else if ( args.getToken(0) == "update" )
		{
			RS_UpdatePlayerNick( player.getName(), player.getClient().playerNum(), player.getId() );
		}
		player.isWaitingForCommand = true;
        return true;
    }
}

class Command_Quad : Racesow_Command
{

    Command_Quad() {
        super("quad", "Activate or desactivate the quad for weapons", "");
    }

    bool validate(Racesow_Player @player, cString &args, int argc)
    {
        if( @player.getClient().getEnt() == null || player.getClient().getEnt().team == TEAM_SPECTATOR )
        {
            player.sendErrorMessage("Quad is not available in your current state");
            return false;
        }
        return true;
    }

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        return player.quad();
    }
}

class Command_RaceRestart : Racesow_Command
{
    Command_RaceRestart() {
        super("racerestart", "Go back to the start area whenever you want", "");
    }

    bool validate(Racesow_Player @player, cString &args, int argc)
    {
        if (player.isJoinlocked)
        {
            player.sendErrorMessage( "You can't join, you are join locked" );
            return false;
        }

        //racerestart command is only avaiblable in DRACE during WARMUP 
        if ( gametypeFlag == MODFLAG_DRACE && this.name == "racerestart" && match.getState() != MATCH_STATE_WARMUP ) 
          return false;
                  
        return true;
    }

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        player.restartRace();
		    return true;
    }
}

class Command_Register : Racesow_Command
{

    Command_Register() {
        super("register", "Register a new account on this server", "<authname> <email> <password> <confirmation>");
    }

    bool validate(Racesow_Player @player, cString &args, int argc)
    {
        if ( argc < 4)
        {
            player.sendErrorMessage("You must provide all required information");
            return false;
        }

        return true;
    }

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        cString authName = args.getToken( 0 ).removeColorTokens();
        cString authEmail = args.getToken( 1 );
        cString password = args.getToken( 2 );
        cString confirmation = args.getToken( 3 );

        return player.getAuth().signUp( authName, authEmail, password, confirmation );
    }
}

class Command_Stats : Racesow_Command
{
    cString what;
    cString which;
    
    Command_Stats() {
        super("stats", "Show statistics", 
        "stats <command> when no options given 'stats player' is executed\n"
        +"stats player <name> - Prints stats for the given player or yourself if no name given\n"
        +"stats map <name> - Print stats for the given map or the current map is no name given\n");
    }

    bool validate(Racesow_Player @player, cString &args, int argc)
    {
        if( player.isWaitingForCommand )
        {
            player.sendErrorMessage( "Flood protection. Slow down cowboy, wait for the "
                    +"results of your previous command");
            return false;
        }
    
        this.what = "";
        this.which = "";
    
        if (argc == 0)
        {
            this.what = "player";
            this.which = player.getName();
        }
        else
        {
            if (args.getToken(0) == "player")
            {
                this.what = "player";
                if (argc == 2)
                {
                    this.which = args.getToken(1);
                }
                else
                {
                    this.which = player.getName();
                }
            }
            else if (args.getToken(0) == "map")
            {
                this.what = "map";
                if (argc == 2)
                {
                    this.which = args.getToken(1);
                }
                else
                {
                    this.which = map.name;
                }
            }
            else
            {
                player.sendErrorMessage("stats command '"+ args.getToken(0) +"' not found");
            }
        }
        
        if (this.what == "")
        {
            return false;
        }
        
        if (this.which == "")
        {
            return false;
        }
        
        return true;
    }

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        //player.sendMessage( S_COLOR_RED + "TODO: " + S_COLOR_WHITE + "retrieve stats for " + this.what + " " + this.which + "\n" );
        return RS_LoadStats(player.getClient().playerNum(), this.what, this.which);
    }
}

class Command_Timeleft : Racesow_Command
{

    Command_Timeleft() {
        super("timeleft", "Print remaining time before map change", "");
    }

    bool validate(Racesow_Player @player, cString &args, int argc)
    {
        if( match.getState() == MATCH_STATE_POSTMATCH )
        {
            player.sendErrorMessage( "The command isn't available in this match state");
            return false;
        }
		
		if ( !g_maprotation.getBool() )
		{
			player.sendErrorMessage( "The command isn't available when g_maprotation == 0.");
            return false;
		}
		
        if( g_timelimit.getInteger() <= 0 )
        {
            player.sendErrorMessage( "There is no timelimit set");
            return false;
        }
        return true;
    }

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        uint timelimit = g_timelimit.getInteger() * 60000;//convert mins to ms
        uint time = levelTime - match.startTime(); //in ms
        uint timeleft = timelimit - time;
        if( timelimit < time )
        {
            player.sendMessage( "We are already in overtime.\n" );
            return true;
        }
        else
        {
            player.sendMessage( "Time left: " + TimeToString( timeleft ) + "\n" );
            return true;
        }
    }
}

class Command_Token : Racesow_Command
{

    Command_Token() {
        super("token", "When authenticated, shows your unique login token you may setu in your config", "");
    }

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        return player.getAuth().showToken();
    }
}

class Command_Top : Racesow_Command
{
    int limit;
    cString mapname;
    int prejumped;

    Command_Top() {
        super("top", "Print the best times of a given map (default: current map)", "<pj/nopj> <limit(3-30)> <mapname>");
    }

    bool validate(Racesow_Player @player, cString &args, int argc)
    {
        this.limit = 30;
        this.mapname = "";
        this.prejumped = 2;

        if ( mysqlConnected == 0 )
        {
            player.sendMessage("This server doesn't store the best times, this command is useless\n" );
            return false;
        }

        if (player.isWaitingForCommand)
        {
            player.sendErrorMessage( "Flood protection. Slow down cowboy, wait for the "
                    +"results of your previous command");
            return false;
        }

        if ( argc > 0 )
        {
            cString firstToken = args.getToken(0);
            if ( firstToken.isNumerical() )
            {
                this.limit = firstToken.toInt();
                if ( argc > 1 )
                    mapname = args.getToken(1);
            }
            else
            {
                if (firstToken == "pj")
                    this.prejumped = 0;
                else if (firstToken == "nopj")
                    this.prejumped = 1;
                else
                    return false;
                if ( argc > 1 )
                    this.limit = args.getToken(1).toInt();
                if ( argc > 2 )
                    mapname = args.getToken(2);
            }
            if ( this.limit < 3 || this.limit > 30)
            {
                player.sendErrorMessage("You must use a limit between 3 and 30");
                return false;
            }
        }
        return true;
    }

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        player.isWaitingForCommand=true;
        RS_MysqlLoadHighscores(player.getClient().playerNum(), this.limit, map.getId(), this.mapname, this.prejumped);
        return true;
    }

}

/*class Command_Weapondef : Racesow_Command
{
    Command_Weapondef() {
        super( "weapondef", "", "" ) //FIXME: Either fully remove or add description and usage
    }

    bool execute( Racesow_Player @player, cString &args, int argc ) {
		return weaponDefCommand( args, @client );
    }
}*/

class Command_WhoIsGod : Racesow_Command
{
    cString[] devs;
    Command_WhoIsGod() {
        super("whoisgod", "Which one is yours?", "");
        this.devs.resize(DEVS.length());
        for( uint i = 0; i < DEVS.length(); i++)
            this.devs[i] = DEVS[i];
    }

    Command_WhoIsGod(cString &in extraDevName) {
        super("whoisgod", "Which one is yours?", "");
        this.devs.resize(DEVS.length()+1);
        for( uint i = 0; i < DEVS.length(); i++)
            this.devs[i] = DEVS[i];
        this.devs[DEVS.length()] = extraDevName;
    }

    Command_WhoIsGod(cString[] &in extraDevNames) {
        super("whoisgod", "Which one is yours?", "");
        this.devs.resize(DEVS.length()+extraDevNames.length());
        for( uint i = 0; i < DEVS.length(); i++)
            this.devs[i] = DEVS[i];
        for( uint i = 0; i < extraDevNames.length(); i++)
            this.devs[i+DEVS.length()] = extraDevNames[i];
    }

    bool execute( Racesow_Player @player, cString &args, int argc ) {
        player.sendMessage( devs[brandom( 0, devs.length())] + "\n");
        return true;
    }
}

/**
 * Fill the commands array and set the command counter to the correct value - FIXME soon obsolete
 */
/*void RS_CreateCommands()
{
    Command_Admin admin;
    admin.name = "admin";
    admin.description = "Execute an admin command";
    admin.usage =
            S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n"
            + S_COLOR_RED + "ADMIN HELP for Racesow " + gametype.getVersion() + "\n"
            + S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n"
            /*+ S_COLOR_RED + "admin add           " + S_COLOR_YELLOW + "add player as an admin\n" //The command string is not long enough to hold all commands
            + S_COLOR_RED + "admin delete           " + S_COLOR_YELLOW + "remove admin rights from player\n"
            + S_COLOR_RED + "admin setpermission           " + S_COLOR_YELLOW + "change permissions of a player\n"*/
            /*+ S_COLOR_RED + "admin map           " + S_COLOR_YELLOW + "change to the given map immediately\n"
            + S_COLOR_RED + "admin restart  " + S_COLOR_YELLOW + "restart the match immediately\n"
            + S_COLOR_RED + "admin extend_time  " + S_COLOR_YELLOW + "extend the matchtime immediately\n"
            + S_COLOR_RED + "admin remove  " + S_COLOR_YELLOW + "remove the given player immediately\n"
            + S_COLOR_RED + "admin kick          " + S_COLOR_YELLOW + "kick the given player immediately\n"
            + S_COLOR_RED + "admin kickban       " + S_COLOR_YELLOW + "kickban the given player immediately\n"
            + S_COLOR_RED + "admin [v](un)mute  " + S_COLOR_YELLOW + "[v](un)mute the given player immediately\n"
            + S_COLOR_RED + "admin vote(un)mute  " + S_COLOR_YELLOW + "enable/disable voting for the given player\n"
            + S_COLOR_RED + "admin joinlock  " + S_COLOR_YELLOW + "prevent the given player from joining\n"
            + S_COLOR_RED + "admin cancelvote    " + S_COLOR_YELLOW + "cancel the currently active vote\n"
            + S_COLOR_RED + "admin updateml    " + S_COLOR_YELLOW + "Update the maplist\n"
            + S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n\n";
    @commands[commandCount] = @admin;
    commandCount++;

    Command_Auth auth;
    auth.name = "auth";
    auth.description = "Authenticate with the server (alternatively you can use setu to save your login)";
    auth.usage = "auth <authname> <password>";
    @commands[commandCount] = @auth;
    commandCount++;

    Command_Chrono chrono;
    chrono.name = "chrono";
    chrono.description = "Chrono for tricks timing";
    chrono.usage = "chrono <start/reset>";
    chrono.modFlag = MODFLAG_FREESTYLE;
    @commands[commandCount] = @chrono;
    commandCount++;

    Command_Gametype gameType;
    gameType.name = "gametype";
    gameType.description = "Print info about the game type";
    gameType.usage = "";
    @commands[commandCount] = @gameType;
    commandCount++;

    Command_Help help;
    help.name = "help";
    help.description = "Print this help, or give help on a specific command";
    help.usage = "help <command>";
    @commands[commandCount] = @help;
    commandCount++;

    Command_Join join;
    join.name = "join";
    join.description = "Join the game";
    join.usage = "";
    //registered in all mod except in durace and trace
    //durace because of challengers queue, trace to be able to join alpha or beta team
    join.modFlag = MODFLAG_ALL ^ MODFLAG_DURACE ^ MODFLAG_TRACE;
    @commands[commandCount] = @join;
    commandCount++;

    Command_Spec spec;
    spec.name = "spec";
    spec.description = "Spectate";
    spec.usage = "";
    spec.modFlag = MODFLAG_ALL ^ MODFLAG_DURACE;  //registered in all mod except in durace, because of challengers queue
    @commands[commandCount] = @spec;
    commandCount++;

	// do we really need this alias? commands are limited you know..
    Command_Spec chase;
    chase.name = "chase";
    chase.description = "Spectate";
    chase.usage = "";
    chase.modFlag = MODFLAG_ALL ^ MODFLAG_DURACE;  //registered in all mod except in durace, because of challengers queue
    @commands[commandCount] = @chase;
    commandCount++;
	
    Command_RaceRestart racerestart;
    racerestart.name = "racerestart";
    racerestart.description = "Go back to the start area whenever you want";
    racerestart.usage = "";
    racerestart.modFlag = MODFLAG_ALL ^ MODFLAG_FREESTYLE;
    @commands[commandCount] = @racerestart;
    commandCount++;

    Command_RaceRestart kill;
    kill.name = "kill";
    kill.description = "Go back to the start area whenever you want";
    kill.usage = "";
    kill.modFlag = MODFLAG_ALL ^ MODFLAG_DRACE; //for DRACE we need to be able to kill himself
    @commands[commandCount] = @kill;
    commandCount++;
    
    Command_LastMap lastmap;
    lastmap.name = "lastmap";
    lastmap.description = "Print the name of the previous map on the server, before this one";
    lastmap.usage = "";
    @commands[commandCount] = @lastmap;
    commandCount++;

    Command_Mapfilter mapfilter;
    mapfilter.name = "mapfilter";
    mapfilter.description = "Search for maps matching a given name";
    mapfilter.usage = "mapfilter <filter> <pagenum>";
    @commands[commandCount] = @mapfilter;
    commandCount++;

    Command_Maplist maplist;
    maplist.name = "maplist";
    maplist.description = "Print the maplist";
    maplist.usage = "maplist <pagenum>";
    @commands[commandCount] = @maplist;
    commandCount++;

    if (dedicated.getBool())
    {
        Command_Mapname mapname;
        mapname.name = "mapname";
        mapname.description = "Print the name of current map";
        mapname.usage = "";
        @commands[commandCount] = @mapname;
        commandCount++;

        Command_NextMap nextmap;
        nextmap.name = "nextmap";
        nextmap.description = "Print the name of the next map in map rotation";
        nextmap.usage = "";
        @commands[commandCount] = @nextmap;
        commandCount++;
    }

	Command_Oneliner oneliner;
    oneliner.name = "oneliner";
    oneliner.description = "Set a one-line message that is displayed right next to your top time";
    oneliner.usage = "";
	  oneliner.modFlag = MODFLAG_RACE;
    @commands[commandCount] = @oneliner;
    commandCount++;

    Command_Position position;
    position.name = "position";
    position.description = "Commands to store and load position";
    position.usage =
            "position <command> where command is one of :\n"
            + "position load - Teleport to saved position\n"
            + "position set <x> <y> <z> <pitch> <yaw> - Teleport to specified position\n"
            + "position store <id> <name> - Store a position for another session\n"
            + "position restore <id> - Restore a stored position from another session\n"
            + "position storedlist <limit> - Sends you a list of your stored positions\n";
    position.modFlag = MODFLAG_FREESTYLE |MODFLAG_RACE;
    position.practiceEnabled = true;
    @commands[commandCount] = @position;
    commandCount++;

    Command_Privsay privsay;
    privsay.name = "privsay";
    privsay.description = "Send a private message to a player";
    privsay.usage = "privsay <playerid/playername>";
    @commands[commandCount] = @privsay;
    commandCount++;
	
    Command_ProtectedNick protectednick;
    protectednick.name = "protectednick";
    protectednick.description = "Show/update your current protected nick";
    protectednick.usage = "protectednick <newnick>";
    @commands[commandCount] = @protectednick;
    commandCount++;

    Command_Quad quad;
    quad.name = "quad";
    quad.description = "Activate or desactivate the quad for weapons";
    quad.usage = "";
    quad.modFlag = MODFLAG_FREESTYLE;
    @commands[commandCount] = @quad;
    commandCount++;
	
    Command_Noclip noclip;
    noclip.name = "noclip";
    noclip.description = "Disable your interaction with other players and objects";
    noclip.usage = "";
    noclip.modFlag = MODFLAG_FREESTYLE | MODFLAG_RACE;
    noclip.practiceEnabled = true;
    @commands[commandCount] = @noclip;
    commandCount++;
	
    Command_Machinegun machinegun;
    machinegun.name = "machinegun";
    machinegun.description = "Gives you a machinegun";
    machinegun.usage = "";
    machinegun.modFlag = MODFLAG_RACE;
    @commands[commandCount] = @machinegun;
    commandCount++;

    Command_Register register;
    register.name = "register";
    register.description = "Register a new account on this server";
    register.usage = "register <authname> <email> <password> <confirmation>";
    @commands[commandCount] = @register;
    commandCount++;
    
    Command_Stats stats;
    stats.name = "stats";
    stats.description = "Show statistics";
    stats.usage =
            "stats <command> when no options given 'stats player' is executed\n"
            + "stats player <name> - Prints stats for the given player or yourself if no name given\n"
            + "stats map <name> - Print stats for the given map or the current map is no name given\n";
    @commands[commandCount] = @stats;
    commandCount++;

    Command_Timeleft timeleft;
    timeleft.name = "timeleft";
    timeleft.description = "Print remaining time before map change";
    timeleft.usage = "";
    timeleft.modFlag = MODFLAG_RACE;
    @commands[commandCount] = @timeleft;
    commandCount++;

    Command_Token token;
    token.name = "token";
    token.description = "When authenticated, shows your unique login token you may setu in your config";
    token.usage = "";
    @commands[commandCount] = @token;
    commandCount++;

    Command_Top top;
    top.name = "top";
    top.description = "Print the best times of a given map (default: current map)";
    top.usage = "top <pj/nopj> <limit(3-30)> <mapname>";
    top.modFlag = MODFLAG_RACE | MODFLAG_DURACE | MODFLAG_DRACE | MODFLAG_TRACE;
    @commands[commandCount] = @top;
    commandCount++;
	
    Command_Practicemode practicemode;
    practicemode.name = "practicemode";
    practicemode.description = "Enable or disable practicemode";
    practicemode.usage = "practicemode\nAllows usage of the position and noclip commands";
    practicemode.modFlag = MODFLAG_RACE;
    @commands[commandCount] = @practicemode;
    commandCount++;
}*/

/*
 * Create all the commands and register them
 */
/*void RS_InitCommands()
{
    RS_CreateCommands();

    for (int i = 0; i < commandCount; i++)
    {
        if ( commands[i].modFlag & gametypeFlag == 0 )
            continue;

        G_RegisterCommand( commands[i].name );
    }
}*/

/**
 * Find a command by its name
 *
 * @param name The name of the command you are looking for
 * @return Racesow_Command@ handler to the command found or null if not found
 */
/*Racesow_Command@ RS_GetCommandByName(cString name)
{
    for (int i = 0; i < commandCount; i++)
    {
        if ( commands[i].name == name )
            return commands[i];
    }

    return null;
}*/

//class Racesow_AdminCommand : Racesow_Command
//{
//    /**
//     * Required User Privileges
//     */
//    int permissionMask;
//
//    /**
//     * Non-Default constructor to be used with super() in the derived classes
//     */
//    Racesow_AdminCommand( cString &in name, cString &in description, cString &in usage, int auth )
//    {
//        super( name, description, usage );
//        this.permissionMask = auth;
//    }
//
//    bool validate(Racesow_Player @player, cString &args, int argc) { return true; }
//
//    bool execute(Racesow_Player @player, cString &args, int argc) { return true; }
//}




class Command_AdminMap : Racesow_Command // (should be subclass of Racesow_AdminCommand )
{
	Command_AdminMap( Racesow_Command @baseCommand )
	{
		super( "map", "change to the given map immediately", "<mapname>" );
        this.permissionMask = RACESOW_AUTH_MAP;
        @this.baseCommand = @baseCommand;
	}

    bool validate(Racesow_Player @player, cString &args, int argc)
    {
        if ( argc < this.getLevel() + 1 )
        {
            return false;
        }

        cString mapName = args.getToken( this.getLevel() + 0 );
        if ( mapName == "" )
        {
            player.sendErrorMessage( "No map name given" );
            return false;
        }
        return true;
    }

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        cString mapName = args.getToken( this.getLevel() + 0 );
        G_CmdExecute( "gamemap " + mapName + "\n" );
        return true;
    }
}


class Command_AdminUpdateml : Racesow_Command // (should be subclass of Racesow_AdminCommand )
{
	Command_AdminUpdateml( Racesow_Command @baseCommand )
	{
		super( "updateml", "Update the maplist", "" );
        this.permissionMask = RACESOW_AUTH_ADMIN;
        @this.baseCommand = @baseCommand;
	}
    bool validate(Racesow_Player @player, cString &args, int argc)
    {
        return true;
    }

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        RS_UpdateMapList( player.client.playerNum() );
        return true;
    }
}


class Command_AdminRestart : Racesow_Command // (should be subclass of Racesow_AdminCommand )
{
	Command_AdminRestart( Racesow_Command @baseCommand )
	{
		super( "restart", "restart the match immediately", "" );
        this.permissionMask = RACESOW_AUTH_MAP;
        @this.baseCommand = @baseCommand;
	}
    bool validate(Racesow_Player @player, cString &args, int argc)
    {
        return true;
    }

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        G_CmdExecute("match restart\n");
        return true;
    }
}


class Command_AdminExtendtime : Racesow_Command // (should be subclass of Racesow_AdminCommand )
{
	Command_AdminExtendtime( Racesow_Command @baseCommand )
	{
		super( "extend_time", "extend the matchtime immediately", "" );
        this.permissionMask = RACESOW_AUTH_MAP;
        @this.baseCommand = @baseCommand;
	}
    bool validate(Racesow_Player @player, cString &args, int argc)
    {
        if( g_timelimit.getInteger() <= 0 )
        {
            player.sendErrorMessage( "This command is only available for timelimits.\n");
            return false;
        }
        return true;
    }

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        g_timelimit.set(g_timelimit.getInteger() + g_extendtime.getInteger());

        map.cancelOvertime(); //FIXME: merge player.cancelOvertime and map.cancelOvertime into a gametype function?
        for ( int i = 0; i < maxClients; i++ )
        {
            racesowGametype.players[i].cancelOvertime();
        }
        return true;
    }
}


class Command_AdminCancelvote : Racesow_Command // (should be subclass of Racesow_AdminCommand )
{
	Command_AdminCancelvote( Racesow_Command @baseCommand )
	{
		super( "cancelvote", "cancel the currently active vote", "" );
        this.permissionMask = RACESOW_AUTH_MAP;
        @this.baseCommand = @baseCommand;
	}

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        RS_cancelvote();
        return true;
    }
}

// implementation of validate() for mute/kick commands
class Racesow_TargetCommand : Racesow_Command // (should be subclass of Racesow_AdminCommand )
{
	Racesow_TargetCommand( cString &in name, cString &in description, cString &in usage )
	{
        super( name, description, usage );
	}

    bool validate(Racesow_Player @player, cString &args, int argc)
    {
        if( args.getToken( this.getLevel() + 0 ) == "" )
        {
            player.client.execGameCommand("cmd players");
            return false;
        }
        Racesow_Player @targetPlayer = @Racesow_GetPlayerByNumber( args.getToken( this.getLevel() + 0 ).toInt() );
        if (@targetPlayer == null )
            return false;
        return true;
    }
}

class Command_AdminMute : Racesow_TargetCommand
{
	Command_AdminMute( Racesow_Command @baseCommand )
	{
		super( "mute", "mute the given player immediately", "<playerid>" );
        this.permissionMask = RACESOW_AUTH_MUTE;
        @this.baseCommand = @baseCommand;
	}

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        Racesow_Player @targetPlayer = @Racesow_GetPlayerByNumber( args.getToken( this.getLevel() + 0 ).toInt() );
        targetPlayer.client.muted |= 1;
        return true;
    }
}

class Command_AdminUnmute : Racesow_TargetCommand
{
	Command_AdminUnmute( Racesow_Command @baseCommand )
	{
		super( "unmute", "unmute the given player immediately", "<playerid>" );
        this.permissionMask = RACESOW_AUTH_MUTE;
        @this.baseCommand = @baseCommand;
	}

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        Racesow_Player @targetPlayer = @Racesow_GetPlayerByNumber( args.getToken( this.getLevel() + 0 ).toInt() );
        targetPlayer.client.muted &= ~1;
        return true;
    }
}

class Command_AdminVmute : Racesow_TargetCommand
{
	Command_AdminVmute( Racesow_Command @baseCommand )
	{
		super( "vmute", "vmute the given player immediately", "<playerid>" );
        this.permissionMask = RACESOW_AUTH_MUTE;
        @this.baseCommand = @baseCommand;
	}

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        Racesow_Player @targetPlayer = @Racesow_GetPlayerByNumber( args.getToken( this.getLevel() + 0 ).toInt() );
        targetPlayer.client.muted |= 2;
        return true;
    }
}

class Command_AdminVunmute : Racesow_TargetCommand
{
	Command_AdminVunmute( Racesow_Command @baseCommand )
	{
		super( "vunmute", "vunmute the given player immediately", "<playerid>" );
		this.permissionMask = RACESOW_AUTH_MUTE;
        @this.baseCommand = @baseCommand;
	}

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        Racesow_Player @targetPlayer = @Racesow_GetPlayerByNumber( args.getToken( this.getLevel() + 0 ).toInt() );
        targetPlayer.client.muted &= ~2;
        return true;
    }
}

class Command_AdminVotemute : Racesow_TargetCommand
{
	Command_AdminVotemute( Racesow_Command @baseCommand )
	{
		super( "votemute", "disable voting for the given player", "<playerid>" );
		this.permissionMask = RACESOW_AUTH_MUTE;
        @this.baseCommand = @baseCommand;
	}

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        Racesow_Player @targetPlayer = @Racesow_GetPlayerByNumber( args.getToken( this.getLevel() + 0 ).toInt() );
        targetPlayer.isVotemuted = true;
        return true;
    }
}

class Command_AdminUnvotemute : Racesow_TargetCommand
{
	Command_AdminUnvotemute( Racesow_Command @baseCommand )
	{
		super( "unvotemute", "enable voting for the given player", "<playerid>" );
		this.permissionMask = RACESOW_AUTH_MUTE;
        @this.baseCommand = @baseCommand;
	}

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        Racesow_Player @targetPlayer = @Racesow_GetPlayerByNumber( args.getToken( this.getLevel() + 0 ).toInt() );
        targetPlayer.isVotemuted = false;
        return true;
    }
}

class Command_AdminRemove : Racesow_TargetCommand
{
	Command_AdminRemove( Racesow_Command @baseCommand )
	{
		super( "remove", "remove the given player immediately", "<playerid>" );
		this.permissionMask = RACESOW_AUTH_KICK;
        @this.baseCommand = @baseCommand;
	}

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        Racesow_Player @targetPlayer = @Racesow_GetPlayerByNumber( args.getToken( this.getLevel() + 0 ).toInt() );
        targetPlayer.remove("");
        return true;
    }
}

class Command_AdminKick : Racesow_TargetCommand
{
	Command_AdminKick( Racesow_Command @baseCommand )
	{
		super( "kick", "kick the given player immediately", "<playerid>" );
		this.permissionMask = RACESOW_AUTH_KICK;
        @this.baseCommand = @baseCommand;
	}

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        Racesow_Player @targetPlayer = @Racesow_GetPlayerByNumber( args.getToken( this.getLevel() + 0 ).toInt() );
        targetPlayer.kick("");
        return true;
    }
}

class Command_AdminJoinlock : Racesow_TargetCommand
{
	Command_AdminJoinlock( Racesow_Command @baseCommand )
	{
		super( "joinlock", "prevent the given player from joining", "<playerid>" );
		this.permissionMask = RACESOW_AUTH_KICK;
        @this.baseCommand = @baseCommand;
	}

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        Racesow_Player @targetPlayer = @Racesow_GetPlayerByNumber( args.getToken( this.getLevel() + 0 ).toInt() );
        targetPlayer.isJoinlocked = true;
        return true;
    }
}

class Command_AdminJoinunlock : Racesow_TargetCommand
{
	Command_AdminJoinunlock( Racesow_Command @baseCommand )
	{
		super( "joinunlock", "allow the given player to joining", "<playerid>" );
		this.permissionMask = RACESOW_AUTH_KICK;
        @this.baseCommand = @baseCommand;
	}

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        Racesow_Player @targetPlayer = @Racesow_GetPlayerByNumber( args.getToken( this.getLevel() + 0 ).toInt() );
        targetPlayer.isJoinlocked = false;
        return true;
    }
}

class Command_AdminKickban : Racesow_TargetCommand
{
	Command_AdminKickban( Racesow_Command @baseCommand )
	{
		super( "kickban", "kickban the given player immediately", "<playerid>" );
		this.permissionMask = RACESOW_AUTH_ADMIN;
        @this.baseCommand = @baseCommand;
	}

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        Racesow_Player @targetPlayer = @Racesow_GetPlayerByNumber( args.getToken( this.getLevel() + 0 ).toInt() );
        targetPlayer.kickban("");
        return true;
    }
}






//Commented out for release - Per server admin/authmasks will be finished when the new http DB-Interaction is done
// add command - adds a new admin (sets all permissions except RACESOW_AUTH_SETPERMISSION)
// delete command - deletes admin rights for the given player
/*
class Command_AdminAdd : Racesow_TargetCommand
{
	Command_AdminAdd( Racesow_Command @baseCommand )
	{
		super( "add", "add", "<playerid>" );
		this.permissionMask = RACESOW_AUTH_ADMIN | RACESOW_AUTH_SETPERMISSION;
        @this.baseCommand = @baseCommand;
	}

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        player.sendErrorMessage("added");
        //player.setAuthmask( RACESOW_AUTH_ADMIN );
        return true;
    }
}

class Command_AdminDelete : Racesow_TargetCommand
{
	Command_AdminDelete( Racesow_Command @baseCommand )
	{
		super( "delete", "delete", "<playerid>" );
		this.permissionMask = RACESOW_AUTH_ADMIN | RACESOW_AUTH_SETPERMISSION;
        @this.baseCommand = @baseCommand;
	}

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        player.sendErrorMessage("deleted");
        //player.setAuthmask( RACESOW_AUTH_REGISTERED );
        return true;
    }
}


class Command_AdminSetpermission : Racesow_TargetCommand
{
	Command_AdminSetpermission( Racesow_Command @baseCommand )
    {
        super( "setpermission", "setpermission", "<playerid> <permission> <value>" );
		this.permissionMask = RACESOW_AUTH_ADMIN | RACESOW_AUTH_SETPERMISSION;
        @this.baseCommand = @baseCommand;
    }

	bool validate( Racesow_Player @player, cString &args, int argc )
	{
		if( Racesow_TargetCommand::validate( player, args, argc ) ) //FIXME: does this work?
		{

	        if( args.getToken( this.getLevel() + 1 ) == "" )
	        {
	            //show list of permissions: map, mute, kick, timelimit, restart, setpermission
	            player.sendErrorMessage( "No permission specified. Available permissions:\n map, mute, kick, timelimit, restart, setpermission" );
	            return false;
	        }

	        if( args.getToken( this.getLevel() + 2 ) == "" || args.getToken( this.getLevel() + 2 ).toInt() > 1 )
	        {
	            //show: 1 to enable 0 to disable current: <enabled/disabled>
	            player.sendErrorMessage( "1 to enable permission 0 to disable permission" );
	            return false;
	        }
			return true;
		}
	}

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
    	uint permission;

        if( args.getToken( 2 ) == "map" )
            permission = RACESOW_AUTH_MAP;
        else if( args.getToken( 2 ) == "mute" )
            permission = RACESOW_AUTH_MUTE;
        else if( args.getToken( 2 ) == "kick" )
            permission = RACESOW_AUTH_KICK;
        else if( args.getToken( 2 ) == "timelimit" )
            permission = RACESOW_AUTH_TIMELIMIT;
        else if( args.getToken( 2 ) == "restart" )
            permission = RACESOW_AUTH_RESTART;
        else if( args.getToken( 2 ) == "setpermission" )
            permission = RACESOW_AUTH_SETPERMISSION;
        else
            return false;

        Racesow_Player @targetPlayer = @Racesow_GetPlayerByNumber( args.getToken( 1 ).toInt() );

        if( args.getToken( 3 ).toInt() == 1 )
            player.sendErrorMessage( args.getToken( 2 ) + "enabled" );
            //targetPlayer.setAuthmask( player.authmask | permission );
        else
            player.sendErrorMessage( args.getToken( 2 ) + "disabled" );
            //targetPlayer.setAuthmask( player.authmask & ~permission );
    }
}
*/
