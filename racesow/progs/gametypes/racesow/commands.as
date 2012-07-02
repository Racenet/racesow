/**
 * Change this if more than 50 commands are needed
 * There is another limitation in C so this number will never be reached anyway
 */
const int MAX_COMMANDS = 50;

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

/**
 * Container for all the commands, filled in RS_CreateCommands
 */
Racesow_Command@[] commands(MAX_COMMANDS);

/**
 * Total number of commands, set in RS_CreateCommands
 */
int commandCount = 0;

/**
 * Generic command class, all commands inherit from this class
 * and should override the validate and execute function
 */
class Racesow_Command
{

    /**
     * The name of the command.
     * That's what the user will have to type to call the command
     */
    String name;

    /**
     * Description of what the command does.
     * This is displayed in the help
     */
    String description;

    /**
     * Usage of the command. This should document the command syntax.
     * Displayed when you do "help <command>" and also when you make an error
     * with the command syntax (e.g wrong number of arguments).
     * When describing the syntax, put the arguments name into brackets (e.g <mapname>)
     */
    String usage;

    /**
     * In which mode should the command only be available ?
     */
    int modFlag;
    
	/**
	 * Should the command be available in practice mode ?
	 */
	bool practiceEnabled;
	
    /**
     * Default constructor
     */
    Racesow_Command()
    {
		    this.modFlag = MODFLAG_ALL ;
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
    bool validate(Racesow_Player @player, String &args, int argc)
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
    bool execute(Racesow_Player @player, String &args, int argc)
    {
        return true;
    }

    /**
     * Return the command description in a nice way to be printed
     */
    String getDescription()
    {
        return S_COLOR_ORANGE + this.name + ": " + S_COLOR_WHITE + this.description + "\n";
    }

    /**
     * Return the command usage in a nice way to be printed
     */
    String getUsage()
    {
        if ( this.usage.len() > 0 )
            return S_COLOR_ORANGE + "Usage: " + S_COLOR_WHITE + this.usage + "\n";
        else
            return "";
    }
}

class Command_Mapfilter : Racesow_Command
{
    bool validate(Racesow_Player @player, String &args, int argc)
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

    bool execute(Racesow_Player @player, String &args, int argc)
    {
        String filter = args.getToken( 0 );
        int page = 1;
        if ( argc >= 2 )
            page = args.getToken( 1 ).toInt();

        player.isWaitingForCommand = true;
        return RS_MapFilter(player.client.playerNum(),filter,page);
    }
}

class Command_Gametype : Racesow_Command
{
    bool execute(Racesow_Player @player, String &args, int argc)
    {
        String response = "";
        Cvar fs_game( "fs_game", "", 0 );
        String manifest = gametype.getManifest();

        response += "\n";
        response += "Gametype " + gametype.getName() + " : " + gametype.getTitle() + "\n";
        response += "----------------\n";
        response += "Version: " + gametype.getVersion() + "\n";
        response += "Author: " + gametype.getAuthor() + "\n";
        response += "Mod: " + fs_game.get_string() + (manifest.length() > 0 ? " (manifest: " + manifest + ")" : "") + "\n";
        response += "----------------\n";

        player.sendMessage(response);
        return true;
    }
}

class Command_RaceRestart : Racesow_Command
{
    bool validate(Racesow_Player @player, String &args, int argc)
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

    bool execute(Racesow_Player @player, String &args, int argc)
    {
        player.restartRace();
		    return true;
    }
}

class Command_Top : Racesow_Command
{
    int limit;
    String mapname;
    int prejumped;

    bool validate(Racesow_Player @player, String &args, int argc)
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
            String firstToken = args.getToken(0);
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

    bool execute(Racesow_Player @player, String &args, int argc)
    {
        player.isWaitingForCommand=true;
        RS_MysqlLoadHighscores(player.getClient().playerNum(), this.limit, map.getId(), this.mapname, this.prejumped);
        return true;
    }

}

class Command_Ranking : Racesow_Command
{
    int page;
	String order;
	
    bool validate(Racesow_Player @player, String &args, int argc)
    {
        this.page = 1;
		this.order = "points";
		
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
            String firstToken = args.getToken(0);
            if ( firstToken.isNumerical() )
            {
                this.page = firstToken.toInt();
            }
        }
		
		if ( argc > 1 )
		{
			String secondToken = args.getToken(1);
			if ( secondToken == "points" || secondToken == "diff_points" ||
				secondToken == "maps" || secondToken == "races" ||
				secondToken == "playtime" ) {
			
				this.order = secondToken;
				
			} else if ( secondToken != "" ) {
			
				player.sendErrorMessage("invalid order given");
				return false;
			}
		}
		
        return true;
    }

    bool execute(Racesow_Player @player, String &args, int argc)
    {
        player.isWaitingForCommand = true;
        RS_MysqlLoadRanking(player.getClient().playerNum(), this.page, this.order);
        return true;
    }

}

class Command_Oneliner : Racesow_Command
{
    bool validate(Racesow_Player @player, String &args, int argc)
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

    bool execute(Racesow_Player @player, String &args, int argc)
    {
	    player.isWaitingForCommand = true;
        RS_MysqlSetOneliner(player.getClient().playerNum(), player.getId(), map.getId(), args);
        return true;
    }

}

class Command_NextMap : Racesow_Command
{
    bool execute(Racesow_Player @player, String &args, int argc)
    {
        player.sendMessage( RS_NextMap() + "\n" );
        return true;
    }
}

class Command_LastMap : Racesow_Command
{
    bool execute(Racesow_Player @player, String &args, int argc)
    {
        player.sendMessage( previousMapName + "\n" );
        return true;
    }
}


class Command_Chrono : Racesow_Command
{
    bool validate(Racesow_Player @player, String &args, int argc)
    {
        if ( argc < 1 )
        {
            player.sendErrorMessage( "you must provide a chrono command" );
            return false;
        }

        return true;
    }

    bool execute(Racesow_Player @player, String &args, int argc)
    {
        String command = args.getToken( 0 );
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

class Command_Maplist : Racesow_Command
{
    bool validate(Racesow_Player @player, String &args, int argc)
    {
        if( player.isWaitingForCommand )
        {
            player.sendErrorMessage( "Flood protection. Slow down cowboy, wait for the "
                    +"results of your previous command");
            return false;
        }

        return true;
    }

    bool execute(Racesow_Player @player, String &args, int argc)
    {
        int page = 1;
        if (argc >= 1)
            page = args.getToken(0).toInt();

        return RS_Maplist(player.client.playerNum(),page);
    }
}

class Command_Token : Racesow_Command
{
    bool execute(Racesow_Player @player, String &args, int argc)
    {
        return player.getAuth().showToken();
    }
}

class Command_Auth : Racesow_Command
{
    bool validate(Racesow_Player @player, String &args, int argc)
    {
        if ( argc < 2 )
        {
            player.sendErrorMessage("You must provide your name and password");
            return false;
        }

        return true;
    }

    bool execute(Racesow_Player @player, String &args, int argc)
    {
        return player.getAuth().authenticate(
                args.getToken( 0 ).removeColorTokens(),
                args.getToken( 1 ),
                false );
    }
}

class Command_ProtectedNick : Racesow_Command
{
	bool validate(Racesow_Player @player, String &args, int argc)
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
	
    bool execute(Racesow_Player @player, String &args, int argc)
    {
		if ( argc < 1 )
		{
			RS_GetPlayerNick( player.client.playerNum(), player.getId() );
		}
		else if ( args.getToken(0) == "update" )
		{
			RS_UpdatePlayerNick( player.getName(), player.client.playerNum(), player.getId() );
		}
		player.isWaitingForCommand = true;
        return true;
    }
}


class Command_Register : Racesow_Command
{
    bool validate(Racesow_Player @player, String &args, int argc)
    {
        if ( argc < 4)
        {
            player.sendErrorMessage("You must provide all required information");
            return false;
        }

        return true;
    }

    bool execute(Racesow_Player @player, String &args, int argc)
    {
        String authName = args.getToken( 0 ).removeColorTokens();
        String authEmail = args.getToken( 1 );
        String password = args.getToken( 2 );
        String confirmation = args.getToken( 3 );

        return player.getAuth().signUp( authName, authEmail, password, confirmation );
    }
}

class Command_Help : Racesow_Command
{
    bool execute(Racesow_Player @player, String &args, int argc)
    {
        if ( argc >= 1 )
        {
            Racesow_Command@ command = RS_GetCommandByName( args.getToken(0) );
            if ( @command != null)
            {
                player.sendMessage( command.getDescription() + command.getUsage() );
                return true;
            }
            else
            {
                player.sendErrorMessage("Command " + S_COLOR_YELLOW + args.getToken(0) + S_COLOR_WHITE + " not found");
                return true;
            }
        }
        else
        {
            String help;
            help += S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n";
            help += S_COLOR_RED + "HELP for Racesow " + gametype.getVersion() + "\n";
            help += S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n";
            player.sendMessage(help);
            help = "";

            for (int i = 0; i < commandCount; i++)
            {
                Racesow_Command@ command = commands[i];

                help += command.getDescription();
                if ( (i/5)*5 == i ) //to avoid print buffer overflow
                {
                    player.sendMessage(help);
                    help = "";
                }
            }

            player.sendMessage(help);
            player.sendMessage( S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n\n");
            return true;
        }
    }
}

class Command_Timeleft : Racesow_Command
{
    bool validate(Racesow_Player @player, String &args, int argc)
    {
        if( match.getState() == MATCH_STATE_POSTMATCH )
        {
            player.sendErrorMessage( "The command isn't available in this match state");
            return false;
        }
		
		if ( !g_maprotation.get_boolean() )
		{
			player.sendErrorMessage( "The command isn't available when g_maprotation == 0.");
            return false;
		}
		
        if( g_timelimit.get_integer() <= 0 )
        {
            player.sendErrorMessage( "There is no timelimit set");
            return false;
        }
        return true;
    }

    bool execute(Racesow_Player @player, String &args, int argc)
    {
        uint timelimit = g_timelimit.get_integer() * 60000;//convert mins to ms
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

class Command_Privsay : Racesow_Command
{
    bool validate(Racesow_Player @player, String &args, int argc)
    {
        if ( player.client.muted == 1 )
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

    bool execute(Racesow_Player @player, String &args, int argc)
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
        String message = args.substr(args.getToken( 0 ).length()+1, args.len() );
        return player.privSay(message, target);
    }
}

class Command_Quad : Racesow_Command
{
    bool validate(Racesow_Player @player, String &args, int argc)
    {
        if( @player.client.getEnt() == null || player.client.getEnt().team == TEAM_SPECTATOR )
        {
            player.sendErrorMessage("Quad is not available in your current state");
            return false;
        }
        return true;
    }

    bool execute(Racesow_Player @player, String &args, int argc)
    {
        return player.quad();
    }
}

class Command_Admin : Racesow_Command
{
    bool validate(Racesow_Player @player, String &args, int argc)
    {
        // i think this needs to be removed
        /*if ( !player.auth.allow( RACESOW_AUTH_ADMIN ) )
        {
            G_PrintMsg( null, S_COLOR_WHITE + player.getName() + S_COLOR_RED
                + " tried to execute an admin command without permission.\n" );
            return false;
        }*/

        if ( argc < 1 )
        {
            player.sendErrorMessage( "No command given. Use 'help admin' for more information" );
            return false;
        }

        return true;
    }

    bool execute(Racesow_Player @player, String &args, int argc)
    {
        return player.adminCommand( args );
    }
}

class Command_Position : Racesow_Command
{
	bool validate(Racesow_Player @player, String &args, int argc)
	{
		if ( gametypeFlag == MODFLAG_RACE && !player.practicing )
			return false;
			
		return true;
	}
	
    bool execute(Racesow_Player @player, String &args, int argc)
    {
        return player.position(args);
    }
}

class Command_Join : Racesow_Command
{
    bool validate(Racesow_Player @player, String &args, int argc)
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

    bool execute(Racesow_Player @player, String &args, int argc)
    {
        player.client.team = TEAM_PLAYERS;
        player.client.respawn( false );
        return true;
    }
}

class Command_Spec : Racesow_Command
{
    bool validate(Racesow_Player @player, String &args, int argc)
    {
        return true;
    }

    bool execute(Racesow_Player @player, String &args, int argc)
    {
        player.client.team = TEAM_SPECTATOR;
        player.client.respawn( true ); // true means ghost
        return true;
    }
}

class Command_Noclip : Racesow_Command
{
    bool validate(Racesow_Player @player, String &args, int argc)
    {
        if( @player.client.getEnt() == null || player.client.getEnt().team == TEAM_SPECTATOR )
        {
            player.sendErrorMessage("Noclip is not available in your current state");
            return false;
        }
        if ( gametypeFlag == MODFLAG_RACE && !player.practicing )
			return false;

        return true;
    }

    bool execute(Racesow_Player @player, String &args, int argc)
    {
        return player.noclip();
    }
}

class Command_Machinegun : Racesow_Command
{
    bool execute(Racesow_Player @player, String &args, int argc)
    {
		//give machinegun (this is default behavior in defrag and usefull in some maps to shoot buttons)
		player.client.inventoryGiveItem( WEAP_MACHINEGUN );
		return true;
    }
}

class Command_Mapname : Racesow_Command
{
    bool execute(Racesow_Player @player, String &args, int argc)
    {
        player.sendMessage( map.name + "\n" );
        return true;
    }
}

class Command_Stats : Racesow_Command
{
    String what;
    String which;
    
    bool validate(Racesow_Player @player, String &args, int argc)
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

    bool execute(Racesow_Player @player, String &args, int argc)
    {
        //player.sendMessage( S_COLOR_RED + "TODO: " + S_COLOR_WHITE + "retrieve stats for " + this.what + " " + this.which + "\n" );
        return RS_LoadStats(player.client.playerNum(), this.what, this.which);
    }
}

class Command_Practicemode : Racesow_Command
{
	bool validate( Racesow_Player @player, String &args, int argc )
	{
		if ( player.client.team != TEAM_PLAYERS )
		{
			player.sendErrorMessage( "You must join the game before going into practice mode" );
			return false;
		}
		return true;
	}
	bool execute( Racesow_Player @player, String &args, int argc )
	{
		if ( @player.client != null )
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

/**
 * Fill the commands array and set the command counter to the correct value
 */
void RS_CreateCommands()
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
            + S_COLOR_RED + "admin map           " + S_COLOR_YELLOW + "change to the given map immedeatly\n"
            + S_COLOR_RED + "admin restart  " + S_COLOR_YELLOW + "restart the match immedeatly\n"
            + S_COLOR_RED + "admin extend_time  " + S_COLOR_YELLOW + "extend the matchtime immedeatly\n"
            + S_COLOR_RED + "admin remove  " + S_COLOR_YELLOW + "remove the given player immedeatly\n"
            + S_COLOR_RED + "admin kick          " + S_COLOR_YELLOW + "kick the given player immedeatly\n"
            + S_COLOR_RED + "admin kickban       " + S_COLOR_YELLOW + "kickban the given player immedeatly\n"
            + S_COLOR_RED + "admin [v](un)mute  " + S_COLOR_YELLOW + "[v](un)mute the given player immedeatly\n"
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

    if (dedicated.get_boolean())
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

	/*
    Command_Token token;
    token.name = "token";
    token.description = "When authenticated, shows your unique login token you may setu in your config";
    token.usage = "";
    @commands[commandCount] = @token;
    commandCount++;
	*/

    Command_Top top;
    top.name = "top";
    top.description = "Print the best times of a given map (default: current map)";
    top.usage = "top <pj/nopj> <limit(3-30)> <mapname>";
    top.modFlag = MODFLAG_RACE | MODFLAG_DURACE | MODFLAG_DRACE | MODFLAG_TRACE;
    @commands[commandCount] = @top;
    commandCount++;
	
	Command_Ranking ranking;
    ranking.name = "ranking";
    ranking.description = "Print the server ranking";
    ranking.usage = "ranking <page> <order(points|diff_points|races|maps|playtime)>";
    ranking.modFlag = MODFLAG_RACE | MODFLAG_DURACE | MODFLAG_DRACE | MODFLAG_TRACE;
    @commands[commandCount] = @ranking;
    commandCount++;
	
    Command_Practicemode practicemode;
    practicemode.name = "practicemode";
    practicemode.description = "Enable or disable practicemode";
    practicemode.usage = "practicemode\nAllows usage of the position and noclip commands";
    practicemode.modFlag = MODFLAG_RACE;
    @commands[commandCount] = @practicemode;
    commandCount++;
}

/*
 * Create all the commands and register them
 */
void RS_InitCommands()
{
    RS_CreateCommands();

    for (int i = 0; i < commandCount; i++)
    {
        if ( commands[i].modFlag & gametypeFlag == 0 )
            continue;

        G_RegisterCommand( commands[i].name );
    }
}

/**
 * Find a command by its name
 *
 * @param name The name of the command you are looking for
 * @return Racesow_Command@ handler to the command found or null if not found
 */
Racesow_Command@ RS_GetCommandByName(String name)
{
    for (int i = 0; i < commandCount; i++)
    {
        if ( commands[i].name == name )
            return commands[i];
    }

    return null;
}
