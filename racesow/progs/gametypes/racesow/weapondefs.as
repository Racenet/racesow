/**
* Racesow weapondefs cvar interface file
*
* @package Racesow
* @version 0.6.2
*/

/**
	* Execute a weapondef command
	* @param String &cmdString, cClient @client
	* @return bool
*/
bool weaponDefCommand( String &cmdString, cClient @client )
{
	bool commandExists = false;
	bool showNotification = false;
	String command = cmdString.getToken( 0 );
	
	if ( !Racesow_GetPlayerByClient(client).auth.allow( RACESOW_AUTH_ADMIN ) )
	{
		G_PrintMsg( null, S_COLOR_WHITE + client.getName() + S_COLOR_RED
		+ " tried to execute an admin command without permission.\n" );
		
		return false;
	}
	
	// no command
	if ( command == "" )
	{
		sendMessage( S_COLOR_RED + "No command given. Use '" + S_COLOR_WHITE +"weapondef help" + S_COLOR_RED +"' for more information.\n", @client);
		return false;
	}
	
	// map command
	else if ( commandExists = ( command == "rocketweak" || command == "rocket" || command == "grenadeweak"  || command == "grenade" || command == "plasmaweak" || command == "plasma") )
	{
		String property = cmdString.getToken( 1 );
		
		if (property != "")
		{
			String value = cmdString.getToken( 2 );
			
			Cvar wdefCvar( "rs_"+ command + "_" + property , "", CVAR_ARCHIVE );
			if (wdefCvar.get_string() != "")
			{
				if (value != "")
				{
					wdefCvar.set(value);
					showNotification = true;
				}
				else 
				{
					G_PrintMsg( client.getEnt(), "Current value for rs_" + command + "_" + property + " : " + wdefCvar.get_string() + "\n");
				}
			}
			else
			G_PrintMsg( client.getEnt(), "The variable you entered does not exist: " + "rs_"+ command + "_" + property + "\n");
		}
		else
		{
			
			String help;
			if (command == "rocket")
			{
				Cvar cvar_skb( "rs_rocket_knockback", "", CVAR_ARCHIVE );Cvar cvar_wkb( "rs_rocketweak_knockback", "", CVAR_ARCHIVE );
				Cvar cvar_ssp( "rs_rocket_splash", "", CVAR_ARCHIVE );Cvar cvar_wsp( "rs_rocketweak_splash", "", CVAR_ARCHIVE );
				Cvar cvar_smk( "rs_rocket_minknockback", "", CVAR_ARCHIVE );Cvar cvar_wmk( "rs_rocketweak_minknockback", "", CVAR_ARCHIVE );
				Cvar cvar_sps( "rs_rocket_prestep", "", CVAR_ARCHIVE );
				Cvar cvar_sal( "rs_rocket_antilag", "", CVAR_ARCHIVE );
				String skb=cvar_skb.get_string(); String wkb=cvar_wkb.get_string();
				String ssp=cvar_ssp.get_string(); String wsp=cvar_wsp.get_string();
				String smk=cvar_smk.get_string(); String wmk=cvar_wmk.get_string();
				String sps=cvar_sps.get_string(); 
				String sal=cvar_sal.get_string();
				help += S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n";
				help += S_COLOR_RED + "rocket___|strong__cur__0.5__0.42__|_weak__cur__0.5__0.42\n";
				help += S_COLOR_RED + "knockbk__|_______"+skb+"__100__100__|_______"+wkb+"___95__100\n";
				help += S_COLOR_RED + "splash___|_______"+ssp+"__140__120__|______"+wsp+"__140__120\n";
				help += S_COLOR_RED + "minknock_|________"+smk+"___10__none_|_______"+wmk+"____5__none\n";
				help += S_COLOR_RED + "prestep__|________"+sps+"___90____0\n";
				help += S_COLOR_RED + "antilag__|_________"+sal+"____0__dunno\n";
				help += S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n\n";
			}
			else if (command == "plasma")
			{
				Cvar cvar_skb( "rs_plasma_knockback", "", CVAR_ARCHIVE );Cvar cvar_wkb( "rs_plasmaweak_knockback", "", CVAR_ARCHIVE );
				Cvar cvar_ssp( "rs_plasma_splash", "", CVAR_ARCHIVE );Cvar cvar_wsp( "rs_plasmaweak_splash", "", CVAR_ARCHIVE );
				Cvar cvar_smk( "rs_plasma_minknockback", "", CVAR_ARCHIVE );Cvar cvar_wmk( "rs_plasmaweak_minknockback", "", CVAR_ARCHIVE );
				Cvar cvar_spd( "rs_plasma_speed", "", CVAR_ARCHIVE );Cvar cvar_wpd( "rs_plasma_speed", "", CVAR_ARCHIVE );
				Cvar cvar_shk( "rs_plasma_hack", "", CVAR_ARCHIVE );
				Cvar cvar_sps( "rs_plasma_prestep", "", CVAR_ARCHIVE );
				String skb=cvar_skb.get_string(); String wkb=cvar_wkb.get_string();
				String ssp=cvar_ssp.get_string(); String wsp=cvar_wsp.get_string();
				String smk=cvar_smk.get_string(); String wmk=cvar_wmk.get_string();
				String spd=cvar_spd.get_string(); String wpd=cvar_wpd.get_string();
				String shk=cvar_shk.get_string(); 
				String sps=cvar_sps.get_string(); 
				help += S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n";
				help += S_COLOR_RED + "plasma___|strong__cur__0.5__0.42_|_weak__cur__0.5__0.42\n";
				help += S_COLOR_RED + "knockbk__|_______"+skb+"__20___28__|________"+wkb+"___14___19\n";
				help += S_COLOR_RED + "splash___|_______"+ssp+"__45___40__|________"+wsp+"___45___20\n";
				help += S_COLOR_RED + "minknock_|________"+smk+"___1__none_|________"+wmk+"____1__none\n";
				help += S_COLOR_RED + "speed____|_____"+spd+"_2.4k_1.7k__|______"+wpd+"__2.4k__1.7k\n";
				help += S_COLOR_RED + "hack_____|________"+shk+"___1___0\n";
				help += S_COLOR_RED + "prestep__|_______"+sps+"__90___32\n";
				help += S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n\n";
			}
			else if (command == "grenade")
			{
				Cvar cvar_stm( "rs_grenade_timeout", "", CVAR_ARCHIVE );Cvar cvar_wtm( "rs_grenadeweak_timeout", "", CVAR_ARCHIVE );
				Cvar cvar_skb( "rs_grenade_knockback", "", CVAR_ARCHIVE );Cvar cvar_wkb( "rs_grenadeweak_knockback", "", CVAR_ARCHIVE );
				Cvar cvar_ssp( "rs_grenade_splash", "", CVAR_ARCHIVE );Cvar cvar_wsp( "rs_grenadeweak_splash", "", CVAR_ARCHIVE );
				Cvar cvar_smk( "rs_grenade_minknockback", "", CVAR_ARCHIVE );Cvar cvar_wmk( "rs_grenadeweak_minknockback", "", CVAR_ARCHIVE );
				Cvar cvar_spd( "rs_grenade_speed", "", CVAR_ARCHIVE );Cvar cvar_wpd( "rs_grenadeweak_speed", "", CVAR_ARCHIVE );
				Cvar cvar_sfr( "rs_grenade_friction", "", CVAR_ARCHIVE );
				Cvar cvar_sgr( "rs_grenade_gravity", "", CVAR_ARCHIVE );
				Cvar cvar_sps( "rs_grenade_prestep", "", CVAR_ARCHIVE );
				String stm=cvar_stm.get_string(); String wtm=cvar_wtm.get_string();
				String skb=cvar_skb.get_string(); String wkb=cvar_wkb.get_string();
				String ssp=cvar_ssp.get_string(); String wsp=cvar_wsp.get_string();
				String smk=cvar_smk.get_string(); String wmk=cvar_wmk.get_string();
				String spd=cvar_spd.get_string(); String wpd=cvar_wpd.get_string();
				String sfr=cvar_sfr.get_string(); 
				String sgr=cvar_sgr.get_string(); 
				String sps=cvar_sps.get_string(); 
				help += S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n";
				help += S_COLOR_RED + "grenade__|strong_cur__0.5__0.42_|_weak__cur__0.5__0.42\n";
				help += S_COLOR_RED + "timeout__|_____"+stm+"_1250_2000__|_______"+wtm+"__1250_2000\n";
				help += S_COLOR_RED + "knockbk__|______"+skb+"__100__120__|_________"+wkb+"___90__120\n";
				help += S_COLOR_RED + "splash___|______"+ssp+"__170__150__|________"+wsp+"___160__150\n";
				help += S_COLOR_RED + "minknock_|_______"+smk+"___10__none_|_________"+wmk+"_____5__none\n";
				help += S_COLOR_RED + "speed____|______"+spd+"__900__800__|________"+wpd+"___900__800\n";
				help += S_COLOR_RED + "friction_|______"+sfr+"__0.85__0.80\n";
				help += S_COLOR_RED + "gravity__|______"+sgr+"__1.3__1.3\n";
				help += S_COLOR_RED + "prestep__|_______"+sps+"___90___24\n";
				help += S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n\n";
			}
			else help = S_COLOR_RED  + "invalid weapondef default property, see " + S_COLOR_WHITE + "weapondef help\n";
			
			G_PrintMsg( client.getEnt(), help );
			showNotification=false;
		}
		
	}	
	
	// help command
	else if( commandExists = command == "help" )
	{
		String help;
		help += S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n";
		help += S_COLOR_RED + "WEAPONDEF HELP for racesow\n";
		help += S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n";
		help += S_COLOR_RED + "weapondef weapon [property] [value], where:\n";
		help += S_COLOR_RED + "  weapon = ( rocket | plasma | grenade )[weak]\n";
		help += S_COLOR_RED + "  property = ( speed | knockback | splash | minknockback | timeout | friction | gravity | prestep | antilag )\n\n";
		help += S_COLOR_RED + "example:" + S_COLOR_WHITE + " weapondef rocket knockback 150\n";
		help += S_COLOR_RED + "to restore default .5 values, type: " + S_COLOR_WHITE + "weapondef restore\n";
		help += S_COLOR_RED + "to save current values, type: " + S_COLOR_WHITE + "weapondef save\n";
		help += S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n\n";
		
		G_PrintMsg( client.getEnt(), help );
		showNotification=false;
	}
	else if( commandExists = command == "restore" )
	{
		weaponDefInitCfg("_original");
		G_CmdExecute( "exec configs/server/gametypes/racesow_weapondefs_original.cfg silent" );
		showNotification = true;
	}
	else if( commandExists = command == "save" )
	{
		weaponDefSave();
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
	* @param String message, cClient @client
	* @return void
	*/
void sendMessage( String message, cClient @client )
{
	// just send to original func
	G_PrintMsg( client.getEnt(), message );
	
	// maybe log messages for some reason to figure out ;)
}

/**
* weaponDefInitCfg()
*
* create weapondef.cfg,
* suffix=="": to the default location
* suffix=="_original": to later restore original values
* @param String suffix
*
* @return void
*/
void weaponDefInitCfg(String suffix)
{
	String config_wpdef;

	config_wpdef = "//*\n"
        + "//* Weapondefs\n"
        + "//*\n"
        + "// HINT: theese values were determined and tweaked\n"
        + "// while playing at weqo's testserver.\n"
        + "// you should only touch this config if you want\n"
        + "// to help us to improove the weapons-phsysics\n"
        + "\n"
        + "// rocket weak\n"
        + "set rs_rocketweak_knockback \"95\"\n"
        + "set rs_rocketweak_splash \"140\"\n"
        + "set rs_rocketweak_minknockback \"5\"\n"
        + "// rocket strong\n"
        + "set rs_rocket_knockback \"100\"\n"
        + "set rs_rocket_splash \"120\"\n"
        + "set rs_rocket_minknockback \"1\"\n"
        + "set rs_rocket_prestep \"10\"\n"
        + "set rs_rocket_antilag \"0\"\n"
        + "// plasma weak\n"
        + "set rs_plasmaweak_knockback \"14\"\n"
        + "set rs_plasmaweak_splash \"45\"\n"
        + "set rs_plasmaweak_minknockback \"1\"\n"
        + "set rs_plasmaweak_speed \"1700\"\n"
        + "// plasma strong\n"
        + "set rs_plasma_knockback \"23\"\n"
        + "set rs_plasma_splash \"40\"\n"
        + "set rs_plasma_minknockback \"1\"\n"
        + "set rs_plasma_speed \"1700\"\n"
        + "set rs_plasma_prestep \"32\"\n"
        + "set rs_plasma_hack \"1\"\n"
        + "// grenade weak\n"
        + "set rs_grenadeweak_timeout \"1650\"\n"
        + "set rs_grenadeweak_knockback \"90\"\n"
        + "set rs_grenadeweak_splash \"160\"\n"
        + "set rs_grenadeweak_minknockback \"10\"\n"
        + "set rs_grenadeweak_speed \"800\"\n"
        + "// grenade strong\n"
        + "set rs_grenade_timeout \"1650\"\n"
        + "set rs_grenade_knockback \"120\"\n"
        + "set rs_grenade_splash \"170\"\n"
        + "set rs_grenade_minknockback \"1\"\n"
        + "set rs_grenade_speed \"800\"\n"
        + "set rs_grenade_friction \"0.85\"\n"
        + "set rs_grenade_gravity \"1.22\"\n"
        + "set rs_grenade_prestep \"24\"\n"
        + "\n"
        + "\necho racesow_weapondefs.cfg executed\n";
	
	G_WriteFile( "configs/server/gametypes/racesow/racesow_weapondefs"+suffix+".cfg", config_wpdef );
	G_Print( "Created default weapondefs config for 'racesow'\n" );
}

/**
* weaponDefSave()
*
* overwrite weapondef.cfg with current values
*
* @return void
*/
void weaponDefSave()
{

	String config_wpdef;
	{
		Cvar cvar_skb( "rs_rocket_knockback", "", CVAR_ARCHIVE );Cvar cvar_wkb( "rs_rocketweak_knockback", "", CVAR_ARCHIVE );
		Cvar cvar_ssp( "rs_rocket_splash", "", CVAR_ARCHIVE );Cvar cvar_wsp( "rs_rocketweak_splash", "", CVAR_ARCHIVE );
		Cvar cvar_smk( "rs_rocket_minknockback", "", CVAR_ARCHIVE );Cvar cvar_wmk( "rs_rocketweak_minknockback", "", CVAR_ARCHIVE );
		Cvar cvar_sps( "rs_rocket_prestep", "", CVAR_ARCHIVE );
		Cvar cvar_sal( "rs_rocket_antilag", "", CVAR_ARCHIVE );
		String skb=cvar_skb.get_string(); String wkb=cvar_wkb.get_string();
		String ssp=cvar_ssp.get_string(); String wsp=cvar_wsp.get_string();
		String smk=cvar_smk.get_string(); String wmk=cvar_wmk.get_string();
		String sps=cvar_sps.get_string(); 
		String sal=cvar_sal.get_string();
		config_wpdef = "// racesow weapon defs, with default warsow.5 values\n"
		+ "\n"
		+ "// rocket weak\n"
		+ "set rs_rocketweak_knockback \""+wkb+"\"\n"
		+ "set rs_rocketweak_splash \""+wsp+"\"\n"
		+ "set rs_rocketweak_minknockback \""+wmk+"\"\n"
		+ "// rocket strong\n"
		+ "set rs_rocket_knockback \""+skb+"\"\n"
		+ "set rs_rocket_splash \""+ssp+"\"\n"
		+ "set rs_rocket_minknockback \""+smk+"\"\n"
		+ "set rs_rocket_prestep \""+sps+"\"\n"
		+ "set rs_rocket_antilag \""+sal+"\"\n";
	}
	{	
		Cvar cvar_skb( "rs_plasma_knockback", "", CVAR_ARCHIVE );Cvar cvar_wkb( "rs_plasmaweak_knockback", "", CVAR_ARCHIVE );
		Cvar cvar_ssp( "rs_plasma_splash", "", CVAR_ARCHIVE );Cvar cvar_wsp( "rs_plasmaweak_splash", "", CVAR_ARCHIVE );
		Cvar cvar_smk( "rs_plasma_minknockback", "", CVAR_ARCHIVE );Cvar cvar_wmk( "rs_plasmaweak_minknockback", "", CVAR_ARCHIVE );
		Cvar cvar_spd( "rs_plasma_speed", "", CVAR_ARCHIVE );Cvar cvar_wpd( "rs_plasma_speed", "", CVAR_ARCHIVE );
		Cvar cvar_shk( "rs_plasma_hack", "", CVAR_ARCHIVE );
		Cvar cvar_sps( "rs_plasma_prestep", "", CVAR_ARCHIVE );
		String skb=cvar_skb.get_string(); String wkb=cvar_wkb.get_string();
		String ssp=cvar_ssp.get_string(); String wsp=cvar_wsp.get_string();
		String smk=cvar_smk.get_string(); String wmk=cvar_wmk.get_string();
		String spd=cvar_spd.get_string(); String wpd=cvar_wpd.get_string();
		String shk=cvar_shk.get_string(); 
		String sps=cvar_sps.get_string(); 
		config_wpdef = config_wpdef + "// plasma weak\n"
		+ "set rs_plasmaweak_knockback \""+wkb+"\"\n"
		+ "set rs_plasmaweak_splash \""+wsp+"\"\n"
		+ "set rs_plasmaweak_minknockback \""+wmk+"\"\n"
		+ "set rs_plasmaweak_speed \""+wpd+"\"\n"
		+ "// plasma strong\n"
		+ "set rs_plasma_knockback \""+skb+"\"\n"
		+ "set rs_plasma_splash \""+ssp+"\"\n"
		+ "set rs_plasma_minknockback \""+smk+"\"\n"
		+ "set rs_plasma_speed \""+spd+"\"\n"
		+ "set rs_plasma_hack \""+shk+"\"\n"
		+ "set rs_plasma_prestep \""+sps+"\"\n";
	}
	{
		Cvar cvar_stm( "rs_grenade_timeout", "", CVAR_ARCHIVE );Cvar cvar_wtm( "rs_grenadeweak_timeout", "", CVAR_ARCHIVE );
		Cvar cvar_skb( "rs_grenade_knockback", "", CVAR_ARCHIVE );Cvar cvar_wkb( "rs_grenadeweak_knockback", "", CVAR_ARCHIVE );
		Cvar cvar_ssp( "rs_grenade_splash", "", CVAR_ARCHIVE );Cvar cvar_wsp( "rs_grenadeweak_splash", "", CVAR_ARCHIVE );
		Cvar cvar_smk( "rs_grenade_minknockback", "", CVAR_ARCHIVE );Cvar cvar_wmk( "rs_grenadeweak_minknockback", "", CVAR_ARCHIVE );
		Cvar cvar_spd( "rs_grenade_speed", "", CVAR_ARCHIVE );Cvar cvar_wpd( "rs_grenadeweak_speed", "", CVAR_ARCHIVE );
		Cvar cvar_sfr( "rs_grenade_friction", "", CVAR_ARCHIVE );
		Cvar cvar_sgr( "rs_grenade_gravity", "", CVAR_ARCHIVE );
		Cvar cvar_sps( "rs_grenade_prestep", "", CVAR_ARCHIVE );
		String stm=cvar_stm.get_string(); String wtm=cvar_wtm.get_string();
		String skb=cvar_skb.get_string(); String wkb=cvar_wkb.get_string();
		String ssp=cvar_ssp.get_string(); String wsp=cvar_wsp.get_string();
		String smk=cvar_smk.get_string(); String wmk=cvar_wmk.get_string();
		String spd=cvar_spd.get_string(); String wpd=cvar_wpd.get_string();
		String sfr=cvar_sfr.get_string(); 
		String sgr=cvar_sgr.get_string(); 
		String sps=cvar_sps.get_string(); 
		config_wpdef = config_wpdef + "// grenade weak\n"
		+ "set rs_grenadeweak_timeout \""+wtm+"\"\n"
		+ "set rs_grenadeweak_knockback \""+wkb+"\"\n"
		+ "set rs_grenadeweak_splash \""+wsp+"\"\n"
		+ "set rs_grenadeweak_minknockback \""+wmk+"\"\n"
		+ "set rs_grenadeweak_speed \""+wpd+"\"\n"
		+ "// grenade strong\n"
		+ "set rs_grenade_timeout \""+stm+"\"\n"
		+ "set rs_grenade_knockback \""+skb+"\"\n"
		+ "set rs_grenade_splash \""+ssp+"\"\n"
		+ "set rs_grenade_minknockback \""+smk+"\"\n"
		+ "set rs_grenade_speed \""+spd+"\"\n"
		+ "set rs_grenade_friction \""+sfr+"\"\n"
		+ "set rs_grenade_gravity \""+sgr+"\"\n"
		+ "set rs_grenade_prestep \""+sps+"\"\n"
		+ "\necho racesow_weapondefs.cfg executed\n";;
	}
	G_WriteFile( "configs/server/gametypes/racesow/racesow_weapondefs.cfg", config_wpdef );
	G_Print( "Saved weapondef file in 'racesow'\n" );
}

/**
* weaponDefInit()
*
* create weapondef.cfg if it doesnt exist
*
* @return void
*/
void weaponDefInit()
{
	// if weapondefs don't have a config file, create it
	if ( !G_FileExists( "configs/server/gametypes/racesow/racesow_weapondefs.cfg" ) )
		weaponDefInitCfg("");
}
