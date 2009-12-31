/**
* Racesow weapondefs cvar interface file
*
* @package Racesow
* @version 0.5.1d
* @date 29.12.2009
* @author r2
*/

/**
	* Execute a weapondef command
	* @param cString &cmdString, cClient @client
	* @return bool
*/
bool weaponDefCommand( cString &cmdString, cClient @client )
{
	bool commandExists = false;
	bool showNotification = false;
	cString command = cmdString.getToken( 0 );
	
	// this is currently public for testing purposes, will be set to admin-only later
	/*
	if ( !client.auth.allow( RACESOW_AUTH_ADMIN ) )
	{
		G_PrintMsg( null, S_COLOR_WHITE + client.getName() + S_COLOR_RED
		+ " tried to execute an admin command without permission.\n" );
		
		return false;
	}
	*/
	
	
	// no command
	if ( command == "" )
	{
		sendMessage( S_COLOR_RED + "No command given. Use 'weapondef help' for more information.\n", @client);
		return false;
	}
	
	// map command
	else if ( commandExists = ( command == "rocketweak" || command == "rocketstrong" || command == "grenadeweak"  || command == "grenadestrong" || command == "plasmaweak" || command == "plasmastrong") )
	{
		cString property = cmdString.getToken( 1 );
		cString value = cmdString.getToken( 2 );
		
		cVar wdefCvar( "rs_"+ command + "_" + property , "", CVAR_ARCHIVE );
		if (wdefCvar.getString() != "")
		{
			if (value != "")
			{
				wdefCvar.set(value);
				showNotification = true;
			}
			else 
			{
				G_PrintMsg( client.getEnt(), "Current value for rs_" + command + "_" + property + " : " + wdefCvar.getString() + "\n");
			}
		}
		else
			G_PrintMsg( client.getEnt(), "The variable you entered does not exist: " + "rs_"+ command + "_" + property + "\n");
	}	
	
	// help command
	else if( commandExists = command == "help" )
	{
		cString help;
		help += S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n";
		help += S_COLOR_RED + "WEAPONDEF HELP for " + gametype.getName() + "\n";
		help += S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n";
		help += S_COLOR_RED + "weapondef [weapon] [property] [value], where:\n";
		help += S_COLOR_RED + "  weapon = [(rocket|plasma|grenade)(strong|weak)]\n";
		help += S_COLOR_RED + "  property = [speed|damage|knockback|splash|minknockback|mindamage|timeout]\n";
		help += S_COLOR_RED + "example:" + S_COLOR_WHITE + " weapondef rocketstrong knockback 150\n";
		help += S_COLOR_RED + "to see default values, type: " + S_COLOR_WHITE + "weapondef reference [(rocket|plasma|grenade)]\n";
		help += S_COLOR_RED + "to restore default .5 values, type: " + S_COLOR_WHITE + "weapondef restore\n";
		help += S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n\n";
		
		G_PrintMsg( client.getEnt(), help );
		showNotification=false;
	}
	
	// default weapondefs
	else if( commandExists = command == "reference" )
	{
		cString property = cmdString.getToken( 1 );
		cString help;
		if (property == "rocket")
		{
			help += S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n";
			help += S_COLOR_RED + "rocket   |strong 0.5 0.42 |weak 0.5 0.42\n";
			help += S_COLOR_RED + "damage   |       85   90  |     75   80 \n";
			help += S_COLOR_RED + "knockbk  |      100  100  |     95  100\n";
			help += S_COLOR_RED + "splash   |      140  120  |    140  120\n";
			help += S_COLOR_RED + "mindmg   |        4   15  |      4    8\n";
			help += S_COLOR_RED + "minknock |       10  none |      5  none\n";
			help += S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n\n";
		}
		else if (property == "plasma")
		{
			help += S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n";
			help += S_COLOR_RED + "plasma   |strong 0.5 0.42 |weak 0.5 0.42\n";
			help += S_COLOR_RED + "damage   |       15   15  |     14   12\n";
			help += S_COLOR_RED + "selfdmg  |      0.5  0.75 |    0.5  0.5\n";
			help += S_COLOR_RED + "knockbk  |       20   28  |     14   19\n";
			help += S_COLOR_RED + "splash   |       45   40  |     45   20\n";
			help += S_COLOR_RED + "mindmg   |        5    5  |      0    1\n";
			help += S_COLOR_RED + "minknock |        1  none |      1  none\n";
			help += S_COLOR_RED + "speed    |      2.4k 1.7k |    2.4k 1.7k\n";
			help += S_COLOR_RED + "spread   |        0    0  |     90    0\n";
			help += S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n\n";
		}
		else if (property == "grenade")
		{
			help += S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n";
			help += S_COLOR_RED + "grenade  |strong 0.5 0.42 |weak 0.5 0.42\n";
			help += S_COLOR_RED + "timeout  |     1250 2000  |   1250 2000\n";
			help += S_COLOR_RED + "damage   |       65  100  |     60  100\n";
			help += S_COLOR_RED + "selfdmg  |     0.35  0.5  |   0.35  0.5\n";
			help += S_COLOR_RED + "knockbk  |      100  120  |     90  120\n";
			help += S_COLOR_RED + "splash   |      170  150  |    160  150\n";
			help += S_COLOR_RED + "mindmg   |       15    5  |     15    5\n";
			help += S_COLOR_RED + "minknock |       10  none |      5  none\n";
			help += S_COLOR_RED + "speed    |      900  800  |    900  800\n";
			help += S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n\n";
		}
		else help = S_COLOR_RED  + "invalid weapondef default property, see " + S_COLOR_WHITE + "weapondef help\n";
		
		G_PrintMsg( client.getEnt(), help );
		showNotification=false;
	}
	else if( commandExists = command == "restore" )
	{
		G_CmdExecute( "exec configs/server/gametypes/" + gametype.getName() + ".cfg silent" );
		G_PrintMsg( client.getEnt(), "Restored default weapondefs\n" );
		showNotification = true;
	}
	
	// don't touch the rest of the method!
	if( !commandExists )
	{
		sendMessage( S_COLOR_RED + "The command 'weapondef " + cmdString + "' does not exist.\n", @client);
		return false;
	}
	else if ( showNotification )
	{
		G_PrintMsg( null, S_COLOR_WHITE + client.getName() + S_COLOR_GREEN
		+ " executed command 'weapondef "+ cmdString +"'\n" );
	}
	
	return true;
}

/**
	* Send a message to console of the player
	* @param cString message, cClient @client
	* @return void
	*/
void sendMessage( cString message, cClient @client )
{
	// just send to original func
	G_PrintMsg( client.getEnt(), message );
	
	// maybe log messages for some reason to figure out ;)
}