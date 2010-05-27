/**
 * Racesow weapondefs cvar interface file
 *
 * @package Racesow
 * @version 0.5.2
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
		cString property = cmdString.getToken( 1 );
		
		if (property != "")
		{
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
		else
		{
			
			cString help;
			if (command == "rocket")
			{
				cVar cvar_skb( "rs_rocket_knockback", "", CVAR_ARCHIVE );cVar cvar_wkb( "rs_rocketweak_knockback", "", CVAR_ARCHIVE );
				cVar cvar_ssp( "rs_rocket_splash", "", CVAR_ARCHIVE );cVar cvar_wsp( "rs_rocketweak_splash", "", CVAR_ARCHIVE );
				cVar cvar_smk( "rs_rocket_minknockback", "", CVAR_ARCHIVE );cVar cvar_wmk( "rs_rocketweak_minknockback", "", CVAR_ARCHIVE );
				cVar cvar_sps( "rs_rocket_prestep", "", CVAR_ARCHIVE );
				cVar cvar_sal( "rs_rocket_antilag", "", CVAR_ARCHIVE );
				cString skb=cvar_skb.getString(); cString wkb=cvar_wkb.getString();
				cString ssp=cvar_ssp.getString(); cString wsp=cvar_wsp.getString();
				cString smk=cvar_smk.getString(); cString wmk=cvar_wmk.getString();
				cString sps=cvar_sps.getString(); 
				cString sal=cvar_sal.getString();
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
				cVar cvar_skb( "rs_plasma_knockback", "", CVAR_ARCHIVE );cVar cvar_wkb( "rs_plasmaweak_knockback", "", CVAR_ARCHIVE );
				cVar cvar_ssp( "rs_plasma_splash", "", CVAR_ARCHIVE );cVar cvar_wsp( "rs_plasmaweak_splash", "", CVAR_ARCHIVE );
				cVar cvar_smk( "rs_plasma_minknockback", "", CVAR_ARCHIVE );cVar cvar_wmk( "rs_plasmaweak_minknockback", "", CVAR_ARCHIVE );
				cVar cvar_spd( "rs_plasma_speed", "", CVAR_ARCHIVE );cVar cvar_wpd( "rs_plasma_speed", "", CVAR_ARCHIVE );
				cVar cvar_shk( "rs_plasma_hack", "", CVAR_ARCHIVE );
				cVar cvar_sps( "rs_plasma_prestep", "", CVAR_ARCHIVE );
				cString skb=cvar_skb.getString(); cString wkb=cvar_wkb.getString();
				cString ssp=cvar_ssp.getString(); cString wsp=cvar_wsp.getString();
				cString smk=cvar_smk.getString(); cString wmk=cvar_wmk.getString();
				cString spd=cvar_spd.getString(); cString wpd=cvar_wpd.getString();
				cString shk=cvar_shk.getString(); 
				cString sps=cvar_sps.getString(); 
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
				cVar cvar_stm( "rs_grenade_timeout", "", CVAR_ARCHIVE );cVar cvar_wtm( "rs_grenadeweak_timeout", "", CVAR_ARCHIVE );
				cVar cvar_skb( "rs_grenade_knockback", "", CVAR_ARCHIVE );cVar cvar_wkb( "rs_grenadeweak_knockback", "", CVAR_ARCHIVE );
				cVar cvar_ssp( "rs_grenade_splash", "", CVAR_ARCHIVE );cVar cvar_wsp( "rs_grenadeweak_splash", "", CVAR_ARCHIVE );
				cVar cvar_smk( "rs_grenade_minknockback", "", CVAR_ARCHIVE );cVar cvar_wmk( "rs_grenadeweak_minknockback", "", CVAR_ARCHIVE );
				cVar cvar_spd( "rs_grenade_speed", "", CVAR_ARCHIVE );cVar cvar_wpd( "rs_grenadeweak_speed", "", CVAR_ARCHIVE );
				cVar cvar_sfr( "rs_grenade_friction", "", CVAR_ARCHIVE );
				cVar cvar_sgr( "rs_grenade_gravity", "", CVAR_ARCHIVE );
				cVar cvar_sps( "rs_grenade_prestep", "", CVAR_ARCHIVE );
				cString stm=cvar_stm.getString(); cString wtm=cvar_wtm.getString();
				cString skb=cvar_skb.getString(); cString wkb=cvar_wkb.getString();
				cString ssp=cvar_ssp.getString(); cString wsp=cvar_wsp.getString();
				cString smk=cvar_smk.getString(); cString wmk=cvar_wmk.getString();
				cString spd=cvar_spd.getString(); cString wpd=cvar_wpd.getString();
				cString sfr=cvar_sfr.getString(); 
				cString sgr=cvar_sgr.getString(); 
				cString sps=cvar_sps.getString(); 
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
		cString help;
		help += S_COLOR_BLACK + "--------------------------------------------------------------------------------------------------------------------------\n";
		help += S_COLOR_RED + "WEAPONDEF HELP for " + gametype.getName() + "\n";
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
		G_CmdExecute( "exec configs/server/gametypes/" + gametype.getName() + "_weapondef_original.cfg silent" );
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
	* @param cString message, cClient @client
	* @return void
	*/
void sendMessage( cString message, cClient @client )
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
* @param cString suffix
*
* @return void
*/
void weaponDefInitCfg(cString suffix)
{
	cString config_wpdef;

	config_wpdef = "// racesow weapon defs, with default warsow.5 values\n"
	+ "\n"
	+ "// rocket weak\n"
	+ "set rs_rocketweak_knockback \"95\"\n"
	+ "set rs_rocketweak_splash \"140\"\n"
	+ "set rs_rocketweak_minknockback \"5\"\n"
	+ "// rocket strong\n"
	+ "set rs_rocket_knockback \"100\"\n"
	+ "set rs_rocket_splash \"140\"\n"
	+ "set rs_rocket_minknockback \"10\"\n"
	+ "set rs_rocket_prestep \"90\"\n"
	+ "set rs_rocket_antilag \"0\"\n"
	+ "// plasma weak\n"
	+ "set rs_plasmaweak_knockback \"14\"\n"
	+ "set rs_plasmaweak_splash \"45\"\n"
	+ "set rs_plasmaweak_minknockback \"1\"\n"
	+ "set rs_plasmaweak_speed \"2400\"\n"
	+ "// plasma strong\n"
	+ "set rs_plasma_knockback \"20\"\n"
	+ "set rs_plasma_splash \"45\"\n"
	+ "set rs_plasma_minknockback \"1\"\n"
	+ "set rs_plasma_speed \"2400\"\n"
	+ "set rs_plasma_prestep \"90\"\n"
	+ "set rs_plasma_hack \"1\"\n"
	+ "// grenade weak\n"
	+ "set rs_grenadeweak_timeout \"1250\"\n"
	+ "set rs_grenadeweak_knockback \"90\"\n"
	+ "set rs_grenadeweak_splash \"160\"\n"
	+ "set rs_grenadeweak_minknockback \"5\"\n"
	+ "set rs_grenadeweak_speed \"900\"\n"
	+ "// grenade strong\n"
	+ "set rs_grenade_timeout \"1250\"\n"
	+ "set rs_grenade_knockback \"100\"\n"
	+ "set rs_grenade_splash \"170\"\n"
	+ "set rs_grenade_minknockback \"10\"\n"
	+ "set rs_grenade_speed \"900\"\n"
	+ "set rs_grenade_friction \"0.85\"\n"
	+ "set rs_grenade_gravity \"1.3\"\n"
	+ "set rs_grenade_prestep \"90\"\n"
	+ "\necho " + gametype.getName() + "_weapondef.cfg executed\n";
	
	G_WriteFile( "configs/server/gametypes/" + gametype.getName() + "_weapondef"+suffix+".cfg", config_wpdef );
	G_Print( "Created default weapondef file for '" + gametype.getName() + "'\n" );
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

	cString config_wpdef;
	{
		cVar cvar_skb( "rs_rocket_knockback", "", CVAR_ARCHIVE );cVar cvar_wkb( "rs_rocketweak_knockback", "", CVAR_ARCHIVE );
		cVar cvar_ssp( "rs_rocket_splash", "", CVAR_ARCHIVE );cVar cvar_wsp( "rs_rocketweak_splash", "", CVAR_ARCHIVE );
		cVar cvar_smk( "rs_rocket_minknockback", "", CVAR_ARCHIVE );cVar cvar_wmk( "rs_rocketweak_minknockback", "", CVAR_ARCHIVE );
		cVar cvar_sps( "rs_rocket_prestep", "", CVAR_ARCHIVE );
		cVar cvar_sal( "rs_rocket_antilag", "", CVAR_ARCHIVE );
		cString skb=cvar_skb.getString(); cString wkb=cvar_wkb.getString();
		cString ssp=cvar_ssp.getString(); cString wsp=cvar_wsp.getString();
		cString smk=cvar_smk.getString(); cString wmk=cvar_wmk.getString();
		cString sps=cvar_sps.getString(); 
		cString sal=cvar_sal.getString();
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
		cVar cvar_skb( "rs_plasma_knockback", "", CVAR_ARCHIVE );cVar cvar_wkb( "rs_plasmaweak_knockback", "", CVAR_ARCHIVE );
		cVar cvar_ssp( "rs_plasma_splash", "", CVAR_ARCHIVE );cVar cvar_wsp( "rs_plasmaweak_splash", "", CVAR_ARCHIVE );
		cVar cvar_smk( "rs_plasma_minknockback", "", CVAR_ARCHIVE );cVar cvar_wmk( "rs_plasmaweak_minknockback", "", CVAR_ARCHIVE );
		cVar cvar_spd( "rs_plasma_speed", "", CVAR_ARCHIVE );cVar cvar_wpd( "rs_plasma_speed", "", CVAR_ARCHIVE );
		cVar cvar_shk( "rs_plasma_hack", "", CVAR_ARCHIVE );
		cVar cvar_sps( "rs_plasma_prestep", "", CVAR_ARCHIVE );
		cString skb=cvar_skb.getString(); cString wkb=cvar_wkb.getString();
		cString ssp=cvar_ssp.getString(); cString wsp=cvar_wsp.getString();
		cString smk=cvar_smk.getString(); cString wmk=cvar_wmk.getString();
		cString spd=cvar_spd.getString(); cString wpd=cvar_wpd.getString();
		cString shk=cvar_shk.getString(); 
		cString sps=cvar_sps.getString(); 
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
		cVar cvar_stm( "rs_grenade_timeout", "", CVAR_ARCHIVE );cVar cvar_wtm( "rs_grenadeweak_timeout", "", CVAR_ARCHIVE );
		cVar cvar_skb( "rs_grenade_knockback", "", CVAR_ARCHIVE );cVar cvar_wkb( "rs_grenadeweak_knockback", "", CVAR_ARCHIVE );
		cVar cvar_ssp( "rs_grenade_splash", "", CVAR_ARCHIVE );cVar cvar_wsp( "rs_grenadeweak_splash", "", CVAR_ARCHIVE );
		cVar cvar_smk( "rs_grenade_minknockback", "", CVAR_ARCHIVE );cVar cvar_wmk( "rs_grenadeweak_minknockback", "", CVAR_ARCHIVE );
		cVar cvar_spd( "rs_grenade_speed", "", CVAR_ARCHIVE );cVar cvar_wpd( "rs_grenadeweak_speed", "", CVAR_ARCHIVE );
		cVar cvar_sfr( "rs_grenade_friction", "", CVAR_ARCHIVE );
		cVar cvar_sgr( "rs_grenade_gravity", "", CVAR_ARCHIVE );
		cVar cvar_sps( "rs_grenade_prestep", "", CVAR_ARCHIVE );
		cString stm=cvar_stm.getString(); cString wtm=cvar_wtm.getString();
		cString skb=cvar_skb.getString(); cString wkb=cvar_wkb.getString();
		cString ssp=cvar_ssp.getString(); cString wsp=cvar_wsp.getString();
		cString smk=cvar_smk.getString(); cString wmk=cvar_wmk.getString();
		cString spd=cvar_spd.getString(); cString wpd=cvar_wpd.getString();
		cString sfr=cvar_sfr.getString(); 
		cString sgr=cvar_sgr.getString(); 
		cString sps=cvar_sps.getString(); 
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
		+ "\necho " + gametype.getName() + "_weapondef.cfg executed\n";;
	}
	G_WriteFile( "configs/server/gametypes/" + gametype.getName() + "_weapondef.cfg", config_wpdef );
	G_Print( "Saved weapondef file in '" + gametype.getName() + "'\n" );
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
	if ( !G_FileExists( "configs/server/gametypes/" + gametype.getName() + "_weapondef.cfg" ) )
		weaponDefInitCfg("");
}
