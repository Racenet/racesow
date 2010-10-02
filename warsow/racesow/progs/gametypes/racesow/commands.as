/**
 * Change this if more than 100 commands are needed
 */
const int MAX_COMMANDS = 100;

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

    cString name;
    cString description;
    cString usage;

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
     * Return the command description in a nice way to be printed
     */
    cString getDescription()
    {
        return S_COLOR_ORANGE + this.name + ": " + S_COLOR_WHITE + this.description + "\n";
    }

    /**
     * Return the command usage in a nice way to be printed
     */
    cString getUsage()
    {
        if ( this.usage.len() > 0 )
            return S_COLOR_ORANGE + "Usage: " + S_COLOR_WHITE + this.usage + "\n";
        else
            return "";
    }
}

class Command_Mapfilter : Racesow_Command
{
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
        return RS_MapFilter(player.client.playerNum(),filter,page);
    }
}

class Command_Gametype : Racesow_Command
{
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

class Command_RaceRestart : Racesow_Command
{
    bool validate(Racesow_Player @player, cString &args, int argc)
    {
        if (player.isJoinlocked)
        {
            player.sendErrorMessage( "You can't join, you are join locked" );
            return false;
        }

        return true;
    }

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        if ( @player.client !is null )
        {
            player.client.team = TEAM_PLAYERS;
            player.client.respawn( false );
            return true;
        }

        return false;
    }
}

class Command_Top : Racesow_Command
{
    bool validate(Racesow_Player @player, cString &args, int argc)
    {
        if( g_freestyle.getBool() )
        {
            player.sendErrorMessage( "Command only available for race");
            return false;
        }

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
            if ( args.getToken(0).toInt() < 3 || args.getToken(0).toInt() > 30)
            {
                player.sendErrorMessage( "You must enter a limit between 0 and 30");
                return false;
            }
        }

        return true;
    }

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        int limit = 30;
        cString mapname;
        player.isWaitingForCommand=true;

        if ( argc > 0 )
            limit = args.getToken(0).toInt();
        if ( argc > 1 )
            mapname = args.getToken(1);

        RS_MysqlLoadHighscores(player.getClient().playerNum(), limit, map.getId(), mapname);
        return true;
    }

}

class Command_NextMap : Racesow_Command
{
    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        player.sendMessage( RS_NextMap() + "\n" );
        return true;
    }
}

class Command_Chrono : Racesow_Command
{
    bool validate(Racesow_Player @player, cString &args, int argc)
    {
        if ( !g_freestyle.getBool() )
        {
            player.sendErrorMessage( "chrono is only available in freestyle mode");
            return false;
        }

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

class Command_Maplist : Racesow_Command
{
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

        return RS_Maplist(player.client.playerNum(),page);
    }
}

class Command_Token : Racesow_Command
{
    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        return player.getAuth().showToken();
    }
}

class Command_Auth : Racesow_Command
{
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

class Command_Register : Racesow_Command
{
    bool validate(Racesow_Player @player, cString &args, int argc)
    {
        if ( argc < 4)
        {
            player.sendErrorMessage("You must provide all needed info");
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

class Command_Help : Racesow_Command
{
    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        if ( argc >= 1 )
        {
            Racesow_Command@ command = RS_GetCommandByName( args.getToken(0) );
            if ( command !is null)
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
            cString help;
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
    bool validate(Racesow_Player @player, cString &args, int argc)
    {
        if( match.getState() == MATCH_STATE_POSTMATCH )
        {
            player.sendErrorMessage( "The command isn't available in this match state");
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

class Command_Privsay : Racesow_Command
{
    bool validate(Racesow_Player @player, cString &args, int argc)
    {
        if ( player.client.muted == 1 )
        {
            player.sendErrorMessage("You can\'t talk, you're muted");
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

        if (target is null)
        {
            player.sendErrorMessage("Invalid player");
            return false;
        }
        cString message = args.substr(args.getToken( 0 ).length()+1, args.len() );
        return player.privSay(message, target);
    }
}

class Command_Quad : Racesow_Command
{
    bool validate(Racesow_Player @player, cString &args, int argc)
    {
        if( !g_freestyle.getBool() || !sv_cheats.getBool() )
        {
            player.sendErrorMessage("Quad is not available on this server");
            return false;
        }
        if( @player.client.getEnt() == null || player.client.getEnt().team == TEAM_SPECTATOR )
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

class Command_Admin : Racesow_Command
{
    bool validate(Racesow_Player @player, cString &args, int argc)
    {
        if ( player.auth.allow( RACESOW_AUTH_ADMIN ) )
        {
            G_PrintMsg( null, S_COLOR_WHITE + player.getName() + S_COLOR_RED
                + " tried to execute an admin command without permission.\n" );
            return false;
        }

        if ( argc < 1 )
        {
            player.sendErrorMessage( "No command given. Use 'help admin' for more information" );
            return false;
        }

        return true;
    }

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        return player.adminCommand( args );
    }
}

class Command_Position : Racesow_Command
{
    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        return player.position(args);
    }
}

class Command_Join : Racesow_Command
{
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
        player.client.team = TEAM_PLAYERS;
        player.client.respawn( false );
        return true;
    }
}

class Command_Noclip : Racesow_Command
{
    bool validate(Racesow_Player @player, cString &args, int argc)
    {
        if( !g_freestyle.getBool() && !sv_cheats.getBool() )
        {
            player.sendErrorMessage( "Noclip is not available on this server" );
            return false;
        }

        if( @player.client.getEnt() == null || player.client.getEnt().team == TEAM_SPECTATOR )
        {
            player.sendErrorMessage("Noclip is not available in your current state");
            return false;
        }

        return true;
    }

    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        return player.noclip();
    }
}

class Command_Mapname : Racesow_Command
{
    bool execute(Racesow_Player @player, cString &args, int argc)
    {
        player.sendMessage( map.name + "\n" );
        return true;
    }
}

/**
 * Fill the commands array and set the command counter to the correct value
 */
void RS_CreateCommands()
{
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

    Command_Gametype gameType;
    gameType.name = "gametype";
    gameType.description = "Print info about the game type";
    gameType.usage = "";
    @commands[commandCount] = @gameType;
    commandCount++;

    Command_RaceRestart racerestart;
    racerestart.name = "racerestart";
    racerestart.description = "Go back to the start area whenever you want";
    racerestart.usage = "";
    @commands[commandCount] = @racerestart;
    commandCount++;

    Command_Top top;
    top.name = "top";
    top.description = "Print the best times of a given map (current by default)";
    top.usage = "top <limit(3-30)> <mapname>";
    @commands[commandCount] = @top;
    commandCount++;

    Command_NextMap nextmap;
    nextmap.name = "nextmap";
    nextmap.description = "Print the name of the next map in map rotation";
    nextmap.usage = "";
    @commands[commandCount] = @nextmap;
    commandCount++;

    Command_Mapname mapname;
    mapname.name = "mapname";
    mapname.description = "Print the name of current map";
    mapname.usage = "";
    @commands[commandCount] = @mapname;
    commandCount++;

    Command_Chrono chrono;
    chrono.name = "chrono";
    chrono.description = "Chrono for tricks timing";
    chrono.usage = "chrono <start/reset>";
    @commands[commandCount] = @chrono;
    commandCount++;

    Command_Token token;
    token.name = "token";
    token.description = "When authenticated, shows your unique login token you may setu in your config";
    token.usage = "";
    @commands[commandCount] = @token;
    commandCount++;

    Command_Auth auth;
    auth.name = "auth";
    auth.description = "Authenticate with the server (alternatively you can use setu to save your login)";
    auth.usage = "auth <authname> <password>";
    @commands[commandCount] = @auth;
    commandCount++;

    Command_Register register;
    register.name = "register";
    register.description = "Register a new account on this server";
    register.usage = "register <authname> <email> <password> <confirmation>";
    @commands[commandCount] = @register;
    commandCount++;

    Command_Help help;
    help.name = "help";
    help.description = "Print this help, or give help on a specific command";
    help.usage = "help <command>";
    @commands[commandCount] = @help;
    commandCount++;

    Command_Timeleft timeleft;
    timeleft.name = "timeleft";
    timeleft.description = "Print remaining time before map change";
    timeleft.usage = "";
    @commands[commandCount] = @timeleft;
    commandCount++;

    Command_Privsay privsay;
    privsay.name = "privsay";
    privsay.description = "Send a private message to a player";
    privsay.usage = "privsay <playerid/playername>";
    @commands[commandCount] = @privsay;
    commandCount++;

    Command_Quad quad;
    quad.name = "quad";
    quad.description = "Activate or desactivate the quad for weapons";
    quad.usage = "";
    @commands[commandCount] = @quad;
    commandCount++;

    Command_Noclip noclip;
    noclip.name = "noclip";
    noclip.description = "Disable your interaction with other players";
    noclip.usage = "";
    @commands[commandCount] = @noclip;
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
    @commands[commandCount] = @position;
    commandCount++;

    Command_Join join;
    join.name = "join";
    join.description = "Join the game";
    join.usage = "";
    @commands[commandCount] = @join;
    commandCount++;

    Command_Admin admin;
    admin.name = "admin";
    admin.description = "Execute an admin command";
    admin.usage =
            S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n"
            + S_COLOR_RED + "ADMIN HELP for Racesow " + gametype.getVersion() + "\n"
            + S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n"
            + S_COLOR_RED + "admin map     " + S_COLOR_YELLOW + "change to the given map immedeatly\n"
            + S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n\n";
    @commands[commandCount] = @admin;
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
        G_RegisterCommand( commands[i].name );
    }
}

/**
 * Find a command by its name
 *
 * @param name The name of the command you are looking for
 * @return Racesow_Command@ handler to the command found or null if not found
 */
Racesow_Command@ RS_GetCommandByName(cString name)
{
    for (int i = 0; i < commandCount; i++)
    {
        if ( commands[i].name == name )
            return commands[i];
    }

    return null;
}
