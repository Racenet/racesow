cString config_general =
"//*\n" +
"//* Racesow General settings\n" +
"//* This file defines racesow general seetings\n" +
"//* See <gametype>.cfg for specific gametype settings\n" +
"//*\n" +
"\n" +
"// Displayed when someone makes a new global record\n" +
"set rs_networkName \"world\" \n" +
"// Restrict callvote extendtime to the last <n> minutes of the game\n" +
"set rs_extendtimeperiod \"3\" \n" +
"// Allow autoHop (continuous jump)\n" +
"set rs_allowAutoHop \"1\" \n" +
"// Load best time of map and players\n" +
"set rs_loadHighscores \"1\" \n" +
"// Load player's personal best race checkpoints\n" +
"set rs_loadPlayerCheckpoints \"1\" \n" +
"// Allow player to switch to strong ammo\n" +
"set g_allowammoswitch \"0\" \n" +
"// Message printed when player connects\n" +
"set rs_welcomeMessage \"Welcome. Type help to see available commands\" \n" +
"// Print info about connections and record to IRC\n" +
"set rs_IRCstream \"0\" \n" +
"\n" +
"// Uncomment the next line to enable database feature\n" +
"// exec configs/server/gametypes/racesow/database.cfg\n" +
"\n" +
"// Gameplay settings\n" +
"// WARNING: if you touch any of theese settings\n" +
"// it can really have a very negative impact on\n" +
"// racesow's gameplay!\n" +
"\n" +
"set g_allow_falldamage \"0\" \n" +
"set g_allow_selfdamage \"0\" \n" +
"set g_allow_stun \"0\" \n" +
"set g_allow_bunny \"0\" \n" +
"set g_antilag \"0\" \n" +
"set g_instagib \"0\" \n" +
"set rs_projectilePrestep \"24\" \n" +
"set g_gravity \"850\" \n" +
"\n" +
"exec configs/server/gametypes/racesow/racesow_weapondefs.cfg" +
"\n" +
"echo racesow.cfg executed\n"
;

cString config_database =
"//*\n" +
"//* Database settings\n" +
"//*\n" +
"\n" +
"// Enable mysql database\n" +
"set rs_mysqlEnabled \"1\" \n" +
"// Print debug info in server console\n" +
"set rs_mysqlDebug \"0\" \n" +
"// Database connection credentials\n" +
"set rs_mysqlHost \"localhost\" \n" +
"set rs_mysqPort \"0\" // leave 0 for you system's default port\n" +
"set rs_mysqlUser \"racesow\" \n" +
"set rs_mysqlDb \"racesow\" \n" +
"set rs_mysqlPass \"secret\" \n" +
"// Enable ingame registration\n" +
"set rs_registrationDisabled \"1\" \n" +
"// Registration instructions for the player\n" +
"set rs_registrationInfo \"Connect at xxx for registration\" \n" +
"// Number of days of the history (points diff) stored for a player\n" +
"set rs_historyDays \"30\" \n" +
"\n" +
"// set this to some MD5, SHA1 etc. digest to lower the risk\n" +
"// of token or password sniffing if you leave it blank, passwords\n" +
"// and tokens will be stored unsalted in the database\n" +
"set rs_tokenSalt \"\" \n" +
"\n" +
"// these serverside cvars define how the players can login on YOUR server\n" +
"// if you leave a setting empty the related authentication type will be disabled\n" +
"set rs_authField_Name \"racenet_user\" \n" +
"set rs_authField_Pass \"racenet_pass\" \n" +
"set rs_authField_Token \"racenet_token\" \n" +
"\n" +
"// the rs_authField_ cvars should always be unique for a server\n" +
"// with these serverside settings, the user has to configure his userinfo as follows\n" +
"// setu racenet_user \"myUsernameForRacenetHostings\" \n" +
"// setu racenet_pass \"myPasswordForRacenetHostings\" \n"
"\n" +
"echo database.cfg executed\n"
;

cString config_weapondef =
"//*\n" +
"//* Weapondefs\n" +
"//*\n" +
"// HINT: theese values were determined and tweaked\n" +
"// while playing at weqo's testserver.\n" +
"// you should only touch this config if you want\n" +
"// to help us to improove the weapons-phsysics\n" +
"\n" +
"// rocket weak\n" +
"set rs_rocketweak_knockback \"95\"\n" +
"set rs_rocketweak_splash \"140\"\n" +
"set rs_rocketweak_minknockback \"5\"\n" +
"// rocket strong\n" +
"set rs_rocket_knockback \"100\"\n" +
"set rs_rocket_splash \"120\"\n" +
"set rs_rocket_minknockback \"1\"\n" +
"set rs_rocket_prestep \"10\"\n" +
"set rs_rocket_antilag \"0\"\n" +
"// plasma weak\n" +
"set rs_plasmaweak_knockback \"14\"\n" +
"set rs_plasmaweak_splash \"45\"\n" +
"set rs_plasmaweak_minknockback \"1\"\n" +
"set rs_plasmaweak_speed \"1700\"\n" +
"// plasma strong\n" +
"set rs_plasma_knockback \"23\"\n" +
"set rs_plasma_splash \"40\"\n" +
"set rs_plasma_minknockback \"1\"\n" +
"set rs_plasma_speed \"1700\"\n" +
"set rs_plasma_prestep \"32\"\n" +
"set rs_plasma_hack \"1\"\n" +
"// grenade weak\n" +
"set rs_grenadeweak_timeout \"1650\"\n" +
"set rs_grenadeweak_knockback \"90\"\n" +
"set rs_grenadeweak_splash \"160\"\n" +
"set rs_grenadeweak_minknockback \"10\"\n" +
"set rs_grenadeweak_speed \"800\"\n" +
"// grenade strong\n" +
"set rs_grenade_timeout \"1650\"\n" +
"set rs_grenade_knockback \"120\"\n" +
"set rs_grenade_splash \"170\"\n" +
"set rs_grenade_minknockback \"1\"\n" +
"set rs_grenade_speed \"800\"\n" +
"set rs_grenade_friction \"0.85\"\n" +
"set rs_grenade_gravity \"1.22\"\n" +
"set rs_grenade_prestep \"24\"\n" +
"\n" +
"\necho racesow_weapondefs.cfg executed\n"
;

